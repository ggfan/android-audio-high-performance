//
// Created by Gerry Fan on 5/2/17.
//
#include <vector>
#include <audio_common.h>
#include "android_debug.h"
#include "stream_builder.h"
#include "validate_outstream.h"

#define WAITING_TIME_IN_SECOND 10
#define WAITING_TIME_IN_MS     0

// define stable state macro: including all non-transient states
#define IS_STABLE_STATE(state) \
    ((state) == AAUDIO_STREAM_STATE_UNINITIALIZED || \
    (state) == AAUDIO_STREAM_STATE_UNKNOWN || \
    (state) == AAUDIO_STREAM_STATE_OPEN ||    \
    (state) == AAUDIO_STREAM_STATE_STARTED || \
    (state) == AAUDIO_STREAM_STATE_PAUSED ||  \
    (state) == AAUDIO_STREAM_STATE_FLUSHED || \
    (state) == AAUDIO_STREAM_STATE_STOPPED || \
    (state) == AAUDIO_STREAM_STATE_CLOSED)

/*
 * State Machine validation structure
 *     action:  function pointer to trigger into the state to test
 *     state:   one state want to validate
 *     result:  validation result of the state
 */
struct ValidationStateInfo {
    AAUDIO_API aaudio_result_t  (*action)(AAudioStream*);
    aaudio_stream_state_t  state;
    bool   result;
};

/*
 * validation vector: state machine in the documentation
 */
static ValidationStateInfo stateMachine [] = {
  { nullptr,                    AAUDIO_STREAM_STATE_OPEN,    false },
  { &AAudioStream_requestStart, AAUDIO_STREAM_STATE_STARTED, false },
  { &AAudioStream_requestPause, AAUDIO_STREAM_STATE_PAUSED,  false },
  { &AAudioStream_requestFlush, AAUDIO_STREAM_STATE_FLUSHED, false },
  { &AAudioStream_requestStart, AAUDIO_STREAM_STATE_STARTED, false },
  { &AAudioStream_requestStop,  AAUDIO_STREAM_STATE_STOPPED, false },
  { &AAudioStream_close,        AAUDIO_STREAM_STATE_CLOSED,  false },
};
static const int32_t stateMachineSize  = sizeof (stateMachine)
                                      / sizeof (stateMachine[0]);

/*
 * Our global stream variable
 */
static AAudioStream *inputStream = nullptr;

/* local function to validate individual state */
bool CheckState(aaudio_stream_state_t state);

/*
 * Main State Machine Validation Function:
 *   create a stream and walk through state machine in ValVector[]
 */
bool ValidateStreamStateMachine(aaudio_audio_format_t format,
                       int32_t samplesPerFrame,
                       aaudio_direction_t direction) {
  assert(inputStream == nullptr);
  StreamBuilder builder;
  inputStream = builder.CreateStream(format,
                                     samplesPerFrame,
                                     AAUDIO_SHARING_MODE_SHARED,
                                     direction,
                                     48000);
  PrintAudioStreamInfo(inputStream);

  for (int32_t idx = 0; idx < stateMachineSize; idx++) {
     // Activate into the state
    aaudio_result_t status;
    ValidationStateInfo *pState = & stateMachine[idx];
    if (pState->action) {
      status = pState->action(inputStream);
      if (status != AAUDIO_OK) {
        LOGE("******ERROR: failed action to go to state %s, status = %s",
             AAudio_convertStreamStateToText(pState->state),
             AAudio_convertResultToText(status));
      }
      // assert(status == AAUDIO_OK);
    }

    if (pState->state == AAUDIO_STREAM_STATE_CLOSED) {
      // not validating the close state command, default to true
      // because once closed, stream is not accessible anymore ...
      pState->result = true;
      continue;
    }

    // wait for it to leave previous state
    if (idx) {
      aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
      do {
        status = AAudioStream_waitForStateChange(
                inputStream,
                stateMachine[idx - 1].state,
                &nextState,
                (int64_t)(WAITING_TIME_IN_MS + WAITING_TIME_IN_SECOND * 1000000) * 1000);
      } while ((status == AAUDIO_OK || status == AAUDIO_ERROR_TIMEOUT)
               && nextState == AAUDIO_STREAM_STATE_UNINITIALIZED);
    }

    pState->result = CheckState(pState->state);
  }

  // Report validation result
  LOGI("==========================================================");
  LOGI("******** Validation Result for %s stream",
       direction == AAUDIO_DIRECTION_INPUT ? "input" : "output");
  bool passed = true;
  for (int32_t idx = 0; idx < stateMachineSize; idx++ ) {
      passed = (passed && stateMachine[idx].result);
      LOGI("%s %s", AAudio_convertStreamStateToText(stateMachine[idx].state),
           stateMachine[idx].result ? "PASS" : "FAILED");
  }
  if (passed) {
    LOGI("Overall state machine test: PASS");
  } else {
    LOGE("Overall state machine test: FAILED");
  }
  LOGI("=============State Machine Validation Report End==========");

  inputStream = nullptr;
  return passed;
}

/*
 * Check and Wait for Stream in the stable State
 */
bool CheckState(aaudio_stream_state_t state) {

  assert(IS_STABLE_STATE(state));
  aaudio_stream_state_t curState = AAudioStream_getState(inputStream);
  if (state == curState) {
    return true;
  }

  if (state == AAUDIO_STREAM_STATE_UNKNOWN) {
    LOGE("stream(%p) is in illegal state: %s",
         inputStream,
         AAudio_convertStreamStateToText(state));
    return false;
  }

  if (state == AAUDIO_STREAM_STATE_UNINITIALIZED ||
      state == AAUDIO_STREAM_STATE_OPEN) {
    /*
     * There is no transient state for this, just sleep for sometime, assuming aaudio
     * thread is still going
     */
    struct timespec time;
    memset(&time, 0, sizeof(time));
    time.tv_sec = WAITING_TIME_IN_SECOND;
    time.tv_nsec = WAITING_TIME_IN_MS *  1000;
    nanosleep(&time, nullptr);

    curState = AAudioStream_getState(inputStream);
    return (curState == state);
  }

  // assuming we are already in transient state
  //   NOTE: this has to be commented out because STARTED state error.
  //         should be enabled after bug is fixed
  //   assert(curState == (state - 1));

  aaudio_result_t status;
  aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
  do {
    status = AAudioStream_waitForStateChange(
            inputStream,
            curState,
            &nextState,
            (int64_t)(WAITING_TIME_IN_MS + WAITING_TIME_IN_SECOND * 1000000) * 1000);
  } while ((status == AAUDIO_OK || status == AAUDIO_ERROR_TIMEOUT)
           && nextState == AAUDIO_STREAM_STATE_UNINITIALIZED);
  curState = AAudioStream_getState(inputStream);

  return (curState == state);
}
