#include "common.h"

struct MidiHeader {
    uint16_t format;
    uint16_t tracks;
    uint16_t tickdiv;
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
    std::vector<DWORD>* GetChunks();

private:
    std::string filename;
    FILE* file;
    MidiHeader* header;
    std::vector<DWORD> chunks;
};

class MidiStream {
public:
    MidiStream();
    ~MidiStream();

    Result Open();
    Result SetTimeDiv(DWORD timeDiv);
    Result Queue(std::vector<DWORD>* data);
    Result Start();
    Result Close();

    uint32_t GetDeviceID();
    HMIDISTRM* GetStream();

private:
    uint32_t device;
    HMIDISTRM stream;
    MIDIHDR header;
};