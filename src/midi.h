#include "common.h"

#define Track std::vector<TrackEvent>

static uint8_t RESET_GM_SYSEX[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 };
static uint8_t ROLAND_REVERB_SYSEX[] = { 0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x30, 0x02, 0x04, 0x00, 0x40, 0x40, 0x00, 0x00, 0x09, 0xF7 };

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

    Result TransmitSysex(uint8_t* data, size_t length);

    UINT id;
    HMIDIOUT device;
    std::vector<TrackEvent> queue;
    DWORD startTime = -1;

    uint16_t tickdiv = 1;
    uint32_t timerCtr = 0;
    double tempo = 500000;
};