//
// Created by Gerry Fan on 5/2/17.
//

#ifndef AAUDIO_VALIDATEOUTSTREAM_H
#define AAUDIO_VALIDATEOUTSTREAM_H

bool ValidateStreamStateMachine(
        aaudio_audio_format_t format,
        int32_t samplesPerFrame,
        aaudio_direction_t dir);

bool ValidateStreamStateMachine2(
    aaudio_audio_format_t format,
    int32_t samplesPerFrame,
    aaudio_direction_t dir);
#endif //AAUDIO_VALIDATEOUTSTREAM_H
