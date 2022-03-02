# TTD-MSX

Music player for OpenTTD MIDI tracks.

## Overview
This program plays back MIDI files using the win32 builtin soundfont/synth. It uses the MIDI low-level API found in `mmsystem.h`. Currently it officially supports the OpenMSX MIDI files (the music included with the video game OpenTTD) but could likely play any regular MIDI file.

## Usage
Use `compile.cmd` to compile and run the program. This will read and play a MIDI file as listed in the `MidiFile` constructor in `music.cpp`.

### Library

Alternatively this code can be used as a library, with `MidiFile` being the input/parser and `MidiDevice` being the playback device.

The full API can be found in `midi.h`. A brief description is also provided below:

```
MidiFile:
    OpenFile: Open the file specified by the constructor for reading.
    ReadHeader: Read the header from the already-opened file.
    ReadTracks: Read all the tracks after the header from the file.
    SortTracks: Sort all read tracks by real time.
    CloseFile: Close the opened file handle.

    GetHeader: Return the header struct.
    GetTracks: Get all track data in a vector of TrackEvent vectors.
```
```
MidiDevice:
    Open: Open the device with the system.
    Queue: Add all passed track data (in vector of vector of TrackEvent format) to the internal queue.
    Start: Start the (already opened) playback device with division per quarter tick specified as a parameter.
    Reset: Reset the device and set all notes to off.
    Close: Close the MIDI device handle.
    
    TransmitSysex: Transmit a long message (SysEx) to an opened device.
```

## Resources
- [MIDI Specification](http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html)
- [win32 MIDI Low-Level API Guide](http://midi.teragonaudio.com/tech/lowmidi.htm)
- [OpenTTD Source Code MIDI Playback](https://github.com/OpenTTD/OpenTTD/blob/master/src/music/win32_m.cpp)