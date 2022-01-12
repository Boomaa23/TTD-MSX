#include "common.h"
#include "midi.h"


/** FILE **/
MidiFile::MidiFile(std::string filename) {
    this->filename = filename;
}

MidiFile::~MidiFile() {
    if (this->file) {
        CloseFile();
    }
}

Result MidiFile::OpenFile() {
    if (!(this->file = fopen(filename.c_str(), "rb"))) {
        return Result::FOPEN_ERR;
    }
    std::cout << file << std::endl;
    this->header = new MidiHeader();
    return Result::SUCCESS;
}

Result MidiFile::ReadHeader() {
    if (!this->file) {
        return Result::FILE_NOT_INIT;
    }
    /* Try to read header, fixed size */
    uint8_t buffer[14];
    if (fread(&buffer, sizeof(buffer), 1, file) != 1) {
        return Result::FREAD_ERR;
    }

    /* Check magic, 'MThd' followed by 4 uint8_t length indicator (always = 6 in SMF) */
    const uint8_t magic[] = { 'M', 'T', 'h', 'd', 0x00, 0x00, 0x00, 0x06 };
    if (memcmp(buffer, magic, sizeof(magic)) != 0) {
        return Result::MEMCMP_ERR;
    }

    /* Read the parameters of the file */
    this->header->format = (buffer[8] << 8) | buffer[9];
    this->header->tracks = (buffer[10] << 8) | buffer[11];
    this->header->tickdiv = (buffer[12] << 8) | buffer[13];
    return Result::SUCCESS;
}

Result MidiFile::ReadChunk() {
    if (!this->file) {
        return Result::FILE_NOT_INIT;
    }

    uint8_t buf[4];
    const uint8_t magic[] = { 'M', 'T', 'r', 'k' };
    if (fread(buf, sizeof(magic), 1, file) != 1) {
        return Result::FREAD_ERR;
    }
    if (memcmp(magic, buf, sizeof(magic)) != 0) {
        return Result::MEMCMP_ERR;
    }

    // DWORD chunk;
    // if (fread(&chunk, 4, 1, this->file) != 1) {
    // 	return Result::FREAD_ERR;
    // }

    // if (fread(&chunk.data, 1, chunk.size, this->file) != 1) {
    // 	return Result::FREAD_ERR;
    // }
    // this->chunks.push_back(chunk);
    return Result::SUCCESS;
}

Result MidiFile::CloseFile() {
    if (!this->file) {
        return Result::FILE_NOT_INIT;
    }

    if (!fclose(this->file)) {
        return Result::FCLOSE_ERR;
    }
    this->file = 0;
    return Result::SUCCESS;
}

MidiHeader* MidiFile::GetHeader() {
    return this->header;
}

std::vector<DWORD>* MidiFile::GetChunks() {
    return &this->chunks;
}


/** STREAM **/
MidiStream::MidiStream() {
    this->device = 0;
}

MidiStream::~MidiStream() {
    if (this->stream) {
        Close();
    }
}

Result MidiStream::Open() {
    if (midiStreamOpen(&this->stream, &this->device, 1, (DWORD) this, NULL, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        return Result::MIDI_OPEN_ERR;
    }
    return Result::SUCCESS;
}

Result MidiStream::SetTimeDiv(DWORD timeDiv) {
    if (!this->stream) {
        return Result::MIDI_NOT_INIT;
    }
    
    MIDIPROPTIMEDIV tdProp;
    tdProp.dwTimeDiv = timeDiv;
    tdProp.cbStruct = sizeof(MIDIPROPTIMEDIV);
    if (midiStreamProperty(this->stream, (LPBYTE) &tdProp, MIDIPROP_SET | MIDIPROP_TIMEDIV) != MMSYSERR_NOERROR) {
        return Result::MIDI_PROP_ERR;
    }
    return Result::SUCCESS;
}

Result MidiStream::Queue(std::vector<DWORD>* data) {
    if (!this->stream) {
        return Result::MIDI_NOT_INIT;
    }

    MIDIEVENT* messages;
    size_t dataSize = data->size();
    messages = new MIDIEVENT[dataSize];
    for (size_t i = 0; i < dataSize; i++) {
        messages[i].dwStreamID = 0;
        messages[i].dwEvent = data->at(i);
    }

    this->header.lpData = (LPSTR) &messages;
    this->header.dwBufferLength = this->header.dwBytesRecorded = (sizeof(MIDIEVENT) - sizeof(DWORD)) * dataSize;
    this->header.dwFlags = 0;

    if (midiOutPrepareHeader((HMIDIOUT) this->stream, &this->header, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
        return Result::MIDI_HEAD_ERR;
    }

    if (midiStreamOut(this->stream, &this->header, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
        midiOutUnprepareHeader((HMIDIOUT) this->stream, &this->header, sizeof(MIDIHDR));
        return Result::MIDI_OUT_ERR;
    }
    return Result::SUCCESS;
}

Result MidiStream::Start() {
    if (midiStreamRestart(this->stream) != MMSYSERR_NOERROR) {
        return Result::MIDI_START_ERR;
    }
    return Result::SUCCESS;
}

Result MidiStream::Close() {
    if (!this->stream) {
        return Result::MIDI_NOT_INIT;
    }
    if (midiOutUnprepareHeader((HMIDIOUT) this->stream, &this->header, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
        return Result::MIDI_CLOSE_ERR;
    }
    if (midiStreamStop(this->stream) != MMSYSERR_NOERROR) {
        return Result::MIDI_CLOSE_ERR;
    }
    if (midiStreamClose(this->stream) != MMSYSERR_NOERROR) {
        return Result::MIDI_CLOSE_ERR;
    }
    
    return Result::SUCCESS;
}

uint32_t MidiStream::GetDeviceID() {
    return this->device;
}

HMIDISTRM* MidiStream::GetStream() {
    return &this->stream;
}