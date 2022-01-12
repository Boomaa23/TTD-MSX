#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <windows.h> // MUST GO BEFORE MMSYSTEM
#include <mmsystem.h>

enum class Result {
    SUCCESS,

    FOPEN_ERR,
    FREAD_ERR,
    FCLOSE_ERR,
    MEMCMP_ERR,
    FILE_NOT_INIT,

    MIDI_OPEN_ERR,
    MIDI_PROP_ERR,
    MIDI_OUT_ERR,
    MIDI_HEAD_ERR,
    MIDI_START_ERR,
    MIDI_CLOSE_ERR,
    MIDI_NOT_INIT,
};