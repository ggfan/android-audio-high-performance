/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "audio_common.h"


static const int32_t audioFormatEnum[] = {
    AAUDIO_FORMAT_INVALID,
    AAUDIO_FORMAT_UNSPECIFIED,
    AAUDIO_FORMAT_PCM_I16,
    AAUDIO_FORMAT_PCM_FLOAT
};
static const int32_t audioFormatCount = sizeof(audioFormatEnum)/
                                        sizeof(audioFormatEnum[0]);

static const uint32_t sampleFormatBPP[] = {
    0xffff,
    0xffff,
    16, //I16
    32, //FLOAT
};
uint16_t SampleFormatToBpp(aaudio_audio_format_t format) {
    for (int32_t i = 0; i < audioFormatCount; ++i) {
      if (audioFormatEnum[i] == format)
        return sampleFormatBPP[i];
    }
    return 0xffff;
}
static const char * audioFormatStr[] = {
    "AAUDIO_FORMAT_INVALID", // = -1,
    "AAUDIO_FORMAT_UNSPECIFIED", // = 0,
    "AAUDIO_FORMAT_PCM_I16",
    "AAUDIO_FORMAT_PCM_FLOAT",
    "AAUDIO_FORMAT_PCM_I8_24",
    "AAUDIO_FORMAT_PCM_I32",
};
const char* FormatToString(aaudio_audio_format_t format) {
    for (int32_t i = 0; i < audioFormatCount; ++i) {
        if (audioFormatEnum[i] == format)
            return audioFormatStr[i];
    }
    return "UNKNOW_AUDIO_FORMAT";
}

void PrintAudioStreamInfo(const AAudioStream * stream) {
#define STREAM_CALL(c) AAudioStream_##c((AAudioStream*)stream)
    LOGI("StreamID: %p", stream);

    LOGW("BufferCapacity: %d", STREAM_CALL(getBufferCapacityInFrames));
    LOGI("BufferSize: %d", STREAM_CALL(getBufferSizeInFrames));
    LOGI("FramesPerBurst: %d", STREAM_CALL(getFramesPerBurst));
    int32_t  xrun  = STREAM_CALL(getXRunCount);
    if (xrun < 0 ) {
        LOGE("*******ERROR: AAudioStream_GetXRunCount() returns %d (%s)",
            xrun, AAudio_convertResultToText(xrun));
    } else {
        LOGI("XRunCount: %d", STREAM_CALL(getXRunCount));
    }
    LOGI("SampleRate: %d", STREAM_CALL(getSampleRate));
    LOGI("SamplesPerFrame: %d", STREAM_CALL(getSamplesPerFrame));
    LOGI("DeviceId: %d", STREAM_CALL(getDeviceId));
    LOGI("Format: %s",  FormatToString(STREAM_CALL(getFormat)));
    LOGI("SharingMode: %s", (STREAM_CALL(getSharingMode)) == AAUDIO_SHARING_MODE_EXCLUSIVE ?
             "execlusive mode" : "sharing mode");
    aaudio_direction_t  dir = STREAM_CALL(getDirection);
    LOGI("Direction: %d", dir);
    if (dir == AAUDIO_DIRECTION_OUTPUT) {
        LOGI("FramesReadByDevice: %d", (int32_t)STREAM_CALL(getFramesRead));
        LOGI("FramesWriteByApp: %d", (int32_t)STREAM_CALL(getFramesWritten));
    } else {
        LOGI("FramesReadByApp: %d", (int32_t)STREAM_CALL(getFramesRead));
        LOGI("FramesWriteByDevice: %d", (int32_t)STREAM_CALL(getFramesWritten));
    }
#undef SREAM_CALL
}

