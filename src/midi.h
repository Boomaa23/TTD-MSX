#include "common.h"

struct MidiHeader {
    uint16_t format;
    uint16_t tracks;
    uint16_t tickdiv;
};

struct TrackEvent {
    uint32_t deltaTime;
    uint8_t* data;
    uint8_t len;
};

class MidiFile {
public:
    MidiFile(std::string filename);
    ~MidiFile();

    Result OpenFile();
    Result ReadHeader();
    Result ReadChunk();
    Result CloseFile();

    MidiHeader* GetHeader();
    std::vector<TrackEvent>* GetChunks();

private:
    std::string filename;
    FILE* file;
    MidiHeader* header;
    std::vector<TrackEvent> chunks;
};

class MidiDevice {
public:
    MidiDevice();
    ~MidiDevice();

    Result Open();
    Result Queue(std::vector<TrackEvent>* data);
    Result Start();
    Result Reset();
    Result Close();

    uint32_t GetID();
    HMIDIOUT* GetDevice();

private:
    UINT id;
    HMIDIOUT device;
    std::vector<TrackEvent> queue;
};

uint32_t ReadVariableLen(FILE* file) {
    uint8_t byte = getc(file);
    uint32_t data = byte & 0x7fu;

	if (byte & 0x80u) {
		do {
			data <<= 7u;
			data |= (byte = getc(file)) & 0x7fu;
		} while (byte & 0x80u);
	}
    return data;
}