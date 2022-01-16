#include "common.h"

#define Track std::vector<TrackEvent>

struct VarLen {
    uint32_t data;
    uint32_t len;
};

struct MidiHeader {
    uint16_t format;
    uint16_t tracks;
    uint16_t tickdiv;
};

enum EventType {
    MIDI,
    META,
    SYSEX
};

struct TrackEvent {
    uint8_t track;
    uint32_t trackTime;
    uint32_t deltaTime;

    EventType type;
    uint8_t* data;
    uint8_t len;
};

class MidiFile {
public:
    MidiFile(std::string filename);
    ~MidiFile();

    Result OpenFile();
    Result ReadHeader();
    Result ReadTracks();
    Result SortTracks();
    Result CloseFile();

    MidiHeader* GetHeader();
    std::vector<Track>* GetTracks();

private:
    std::string filename;
    FILE* file;
    MidiHeader* header;
    std::vector<Track> tracks;
};

class MidiDevice {
public:
    MidiDevice();
    ~MidiDevice();

    Result Open();
    Result Queue(std::vector<Track>* data);
    Result Start(uint16_t tickdiv);
    Result Reset();
    Result Close();

    uint32_t GetID();
    HMIDIOUT* GetDevice();

private:
    UINT id;
    HMIDIOUT device;
    std::vector<TrackEvent> queue;
};