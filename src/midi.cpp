#include "common.h"
#include "midi.h"

uint32_t swap_uint32(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF); 
    return (val << 16) | (val >> 16);
}

VarLen ReadVariableLen(FILE* file) {
    uint8_t byte = getc(file);
    VarLen out;
    out.data = byte & 0x7fu;
    out.len = 1;

    if (byte & 0x80u) {
        do {
            out.data <<= 7u;
            out.data |= (byte = getc(file)) & 0x7fu;
            out.len++;
        } while (byte & 0x80u);
    }
    return out;
}


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
    printf("PPQN: %u \n", this->header->tickdiv);
    return Result::SUCCESS;
}

Result MidiFile::ReadTracks() {
    if (!this->file) {
        return Result::FILE_NOT_INIT;
    }

    while (true) {
        uint8_t buf[4];
        const uint8_t magic[] = { 'M', 'T', 'r', 'k' };
        uint8_t retVal = fread(buf, sizeof(magic), 1, file);
        if (retVal == EOF || retVal == 0) {
            // File is done reading, reached EOF
            return Result::SUCCESS;
        } else if (retVal != 1) {
            return Result::FREAD_ERR;
        }
        if (memcmp(magic, buf, sizeof(magic)) != 0) {
            return Result::MEMCMP_ERR;
        }

        uint32_t trackLength;
        if (fread(&trackLength, 4, 1, this->file) != 1) {
            return Result::FREAD_ERR;
        }
        trackLength = swap_uint32(trackLength);

        Track track;
        uint32_t i = 0;
        uint32_t currTime = 0;
        uint8_t lastStatus = 0;
        while (i < trackLength) {
            TrackEvent event;
            VarLen deltaTime = ReadVariableLen(this->file);
            event.deltaTime = deltaTime.data;
            event.trackTime = currTime + deltaTime.data;
            currTime += deltaTime.data;
            i += deltaTime.len;
            
            uint8_t status = getc(this->file);
            if (status & 0x80) {
                lastStatus = status;
                i++;
            } else {
                status = lastStatus;
                fseek(this->file, -1, SEEK_CUR);
            }

            switch (status) {
                case 0xF0:
                case 0xF7: {
                    // sysex event
                    printf("FOUND SYSEX\n");
                    event.type = EventType::SYSEX;
                    VarLen sysexLen = ReadVariableLen(this->file);
                    event.len = sysexLen.data;
                    event.data = new uint8_t[event.len];
                    if (fread(&event.data, 1, event.len, this->file) != 1) {
                        return Result::FREAD_ERR;
                    }
                    i += event.len + sysexLen.len;
                    break;
                }
                case 0xFF: {
                    // meta event
                    event.type = EventType::META;
                    uint8_t metaType = getc(this->file);
                    VarLen metaLen = ReadVariableLen(this->file);
                    event.len = metaLen.data;
                    event.data = new uint8_t[event.len + 1];
                    event.data[0] = metaType;
                    for (uint8_t i = 0; i < event.len; ++i) {
                        event.data[i + 1] = getc(this->file);
                    }
                    i += event.len + metaLen.len + 1;
                    break;
                }
                default: {
                    // midi message
                    event.type = EventType::MIDI;
                    switch (status & 0xF0) {
                        case 0xC0:
                        case 0xD0:
                            event.len = 2;
                            event.data = new uint8_t[2] {
                                status,
                                (uint8_t) getc(this->file)
                            };
                            i += 1;
                            break;
                        case 0x80:
                        case 0x90:
                        case 0xA0:
                        case 0xB0:
                        case 0xE0:
                            event.len = 3;
                            event.data = new uint8_t[3] {
                                status,
                                (uint8_t) getc(this->file),
                                (uint8_t) getc(this->file)
                            };
                            i += 2;
                            break;
                    }
                    break;
                }
            }
            track.push_back(event);
        }
        this->tracks.push_back(track);
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

std::vector<Track>* MidiFile::GetTracks() {
    return &this->tracks;
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

bool TracktimeAsc(TrackEvent &a, TrackEvent &b) {
    return a.trackTime < b.trackTime;
}

Result MidiDevice::Queue(std::vector<Track>* data) {
    if (!this->device) {
        return Result::MIDI_NOT_INIT;
    }
    for (size_t i = 0; i < data->size(); i++) {
        this->queue.insert(this->queue.end(), data->at(i).begin(), data->at(i).end());
    }
    std::sort(this->queue.begin(), this->queue.end(), TracktimeAsc);
    return Result::SUCCESS;
}

void CALLBACK TimerCallback(UINT uTimerID, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR) {
    MidiDevice* midi = reinterpret_cast<MidiDevice*>(dwUser);
    
    DWORD time = timeGetTime();
    DWORD* startTime = &midi->startTime;
    if (*startTime == -1) {
        *startTime = time;
    }
    time -= *startTime;

    while (midi->timerCtr < midi->queue.size()) {
        TrackEvent* event = &midi->queue.at(midi->timerCtr);
        double eventTime = event->trackTime * (double) midi->tempo / 1000.0 / (uint32_t) midi->tickdiv;
        if (eventTime > time) {
            break;
        }
        if (event->type == EventType::MIDI) {
            DWORD data = event->data[0] | event->data[1] << 8;
            if (event->len == 3) {
                data |= event->data[2] << 16;
            }
            if (midiOutShortMsg(midi->device, data) != MMSYSERR_NOERROR) {
                return;
            }
        } else if (event->type == EventType::META) {
            if (event->data[0] == 0x51) {
                uint32_t data = (event->data[1] << 16) | (event->data[2] << 8) | event->data[3];
                midi->tempo = data;
            }
        } else if (event->type == EventType::SYSEX) {
            Result retval = midi->TransmitSysex(event->data, event->len);
            if (retval != Result::SUCCESS) {
                return;
            }
        }
        midi->timerCtr++;
    }
    std::vector<TrackEvent>::iterator begin = midi->queue.begin();
}

Result MidiDevice::Start(uint16_t tickdiv) {
    TransmitSysex(RESET_GM_SYSEX, sizeof(RESET_GM_SYSEX));
    TransmitSysex(ROLAND_REVERB_SYSEX, sizeof(ROLAND_REVERB_SYSEX));
    
    this->tickdiv = tickdiv;
    TIMECAPS timecaps;
	if (timeGetDevCaps(&timecaps, sizeof(timecaps)) != MMSYSERR_NOERROR) {
		return Result::MIDI_OPEN_ERR;
	}
    UINT timePeriod = 1;
    timePeriod = std::min(std::max(timePeriod, timecaps.wPeriodMin), timecaps.wPeriodMax);
    if (timeBeginPeriod(timePeriod) != MMSYSERR_NOERROR) {
        return Result::MIDI_OPEN_ERR;
    }
    
    this->startTime = timeGetTime();
    timeSetEvent(timePeriod, timePeriod, TimerCallback, (DWORD_PTR)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
    while (this->timerCtr < this->queue.size() - 1) {
        Sleep(100);
    }
    
    return Result::SUCCESS;
}

Result MidiDevice::TransmitSysex(uint8_t* data, size_t length) {
    MIDIHDR* hdr = (MIDIHDR*)calloc(1, sizeof(MIDIHDR));
    hdr->lpData = reinterpret_cast<LPSTR>(data);
    hdr->dwBufferLength = length;
    MMRESULT i = midiOutPrepareHeader(this->device, hdr, sizeof(*hdr));
    printf("HELLO, %i\n", i);
    if (i != MMSYSERR_NOERROR) {
        return Result::MIDI_HDR_ERR;
    }
    hdr->dwBytesRecorded = hdr->dwBufferLength;
    if (midiOutLongMsg(this->device, hdr, sizeof(*hdr)) != MMSYSERR_NOERROR) {
        return Result::MIDI_OUT_ERR;
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