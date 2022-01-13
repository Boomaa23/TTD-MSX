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

    uint32_t length;
    if (fread(&length, 4, 1, this->file) != 1) {
    	return Result::FREAD_ERR;
    }

    uint32_t i = 0;
    while (i < length) {
        TrackEvent event;
        event.deltaTime = ReadVariableLen(this->file);
        uint8_t status = getc(this->file);

        switch (status) {
            case 0xF0:
            case 0xF7:
                // sysex event
                event.len = getc(this->file);
                event.data = new uint8_t[event.len];
                for (uint8_t i = 0; i < event.len; ++i) {
                    event.data[i] = getc(this->file);
                }
                break;
            case 0xFF:
                // meta event
                if (!fseek(this->file, 1, SEEK_CUR)) {
                    return Result::FSEEK_ERR;
                }
                event.len = getc(this->file);
                event.data = new uint8_t[event.len];
                for (uint8_t i = 0; i < event.len; ++i) {
                    event.data[i] = getc(this->file);
                }
                break;
            default:
                // midi message
                switch (status & 0xF0) {
                    case 0xC0:
                    case 0xD0:
                        event.len = 1;
                        event.data = new uint8_t[2] {
                            status,
                            (uint8_t) getc(this->file)
                        };
                        break;
                    case 0x80:
                    case 0x90:
                    case 0xA0:
                    case 0xB0:
                    case 0xE0:
                        event.len = 1;
                        event.data = new uint8_t[3] {
                            status,
                            (uint8_t) getc(this->file),
                            (uint8_t) getc(this->file)
                        };
                        break;
                }
                break;
        }
        this->chunks.push_back(event);
    }
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

std::vector<TrackEvent>* MidiFile::GetChunks() {
    return &this->chunks;
}


/** DEVICE **/
MidiDevice::MidiDevice() {
    this->id = 0;
    this->device = NULL;
}

MidiDevice::~MidiDevice() {
    if (this->device) {
        Close();
    }
}

Result MidiDevice::Open() {
    if (midiOutOpen(&this->device, this->id, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        return Result::MIDI_OPEN_ERR;
    }
    return Result::SUCCESS;
}

Result MidiDevice::Queue(std::vector<TrackEvent>* data) {
    if (!this->device) {
        return Result::MIDI_NOT_INIT;
    }
    this->queue.insert(this->queue.end(), data->begin(), data->end());
    return Result::SUCCESS;
}

Result MidiDevice::Start() {
    for (size_t i = 0; i < this->queue.size(); i++) {
        TrackEvent* event = &this->queue.at(i);
        if (event->data.size() == 2 || event->data.size() == 3) {
            DWORD data = event->data.at(0) | (event->data.at(1) << 8);
            if (event->data.size() == 3) {
                data |= event->data.at(2) << 16;
            }
            if (midiOutShortMsg(this->device, data) != MMSYSERR_NOERROR) {
                return Result::MIDI_OUT_ERR;
            }
        } else {
            // long msg playback
        }
    }
    return Result::SUCCESS;
}

Result MidiDevice::Reset() {
    if (!this->device) {
        return Result::MIDI_NOT_INIT;
    }
    if (midiOutReset(this->device) != MMSYSERR_NOERROR) {
        return Result::MIDI_CLOSE_ERR;
    }
    
    return Result::SUCCESS;
}


Result MidiDevice::Close() {
    if (!this->device) {
        return Result::MIDI_NOT_INIT;
    }
    if (midiOutClose(this->device) != MMSYSERR_NOERROR) {
        return Result::MIDI_CLOSE_ERR;
    }
    
    return Result::SUCCESS;
}

UINT MidiDevice::GetID() {
    return this->id;
}

HMIDIOUT* MidiDevice::GetDevice() {
    return &this->device;
}