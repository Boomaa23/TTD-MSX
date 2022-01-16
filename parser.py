import os

class Chunk:
    def __init__(self, track_num, chunk_num, delta_time):
        self.track_num = track_num
        self.chunk_num = chunk_num
        self.delta_time = delta_time

    def __repr__(self):
        return f", dt={self.delta_time}, tn={self.track_num}, cn={self.chunk_num}"

META_EVENT_MAP = {
    0x00: "Sequence Number",
    0x01: "Text Event",
    0x02: "Copyright Notice",
    0x03: "Sequence/Track Name",
    0x04: "Instrument Name",
    0x05: "Lyric",
    0x06: "Marker",
    0x07: "Cue Point",
    0x20: "MIDI Channel Prefix",
    0x2F: "End of Track",
    0x51: "Set Tempo",
    0x54: "SMPTE Offset",
    0x58: "Time Signature",
    0x59: "Key Signature",
    0x7F: "Sequencer Specific Meta-Event"
}

class MetaChunk(Chunk):
    def __init__(self, track_num, chunk_num, delta_time, meta_type, meta_len, data):
        super().__init__(track_num, chunk_num, delta_time)
        self.meta_type = meta_type
        self.meta_len = meta_len
        self.data = data

    def __repr__(self):
        return f"\nMetaChunk: type={META_EVENT_MAP[self.meta_type]}({hex(self.meta_type)}), value={self.data}" + super().__repr__()


class SysExChunk(Chunk):
    def __init__(self, track_num, chunk_num, delta_time, sysex_len, data):
        super().__init__(track_num, chunk_num, delta_time)
        self.sysex_len = sysex_len
        self.data = data

    def __repr__(self):
        return f"\nSysExChunk: len={self.sysex_len}, value={self.data}" + super().__repr__()

MIDI_EVENT_MAP = {
    0x80: "Note Off",
    0x90: "Note On",
    0xA0: "Polyphonic Key Pressure (Aftertouch)",
    0xB0: "Control Change",
    0xC0: "Program Change",
    0xD0: "Channel Pressure (After-touch)",
    0xE0: "Pitch Wheel Change"
}

class MidiChunk(Chunk):
    def __init__(self, track_num, chunk_num, delta_time, message_code, data):
        super().__init__(track_num, chunk_num, delta_time)
        self.message_code = message_code
        self.data = data

    def __repr__(self):
        return f"\nMidiChunk: type={MIDI_EVENT_MAP[self.message_code]}({hex(self.message_code)}), value={self.data}" + super().__repr__()

        

def btoi(bytes, is_signed=False):
    return int.from_bytes(bytes, byteorder="big", signed=is_signed)


def read_vlq(file):
    length = 0
    has_next_byte = True
    value = 0
    bts = []
    while has_next_byte:
        b = file.read(1)
        chr = ord(b)
        bts.append(hex(b[0]))
        length += 1
        # is the hi-bit set?
        if not (chr & 0x80):
            # no next BYTE
            has_next_byte = False
        # mask out the 8th bit
        chr = chr & 0x7f
        # shift last value up 7 bits
        value = value << 7
        # add new value
        value += chr
    print(bts)
    return value, length

def read_header(file):
    file_header = file.read(4)
    if file_header != b"MThd":
        print("ERROR: Header invalid")
        return
    length = btoi(file.read(4))
    format = btoi(file.read(2))
    ntrks = btoi(file.read(2))
    division = btoi(file.read(2))
    return file_header, length, format, ntrks, division


def read_tracks(file):
    meta_chunks = []
    sysex_chunks = []
    midi_chunks = []
    track_num = 0
    while True:
        track_header = file.read(4)
        if btoi(track_header) == 0:
            break
        if track_header != b"MTrk":
            print(btoi(track_header))
            print("ERROR: Track header expected")
            break
        length = btoi(file.read(4))

        read_len = 0
        chunk_num = 0
        last_status = 0
        while read_len < length:
            delta_time, delta_len = read_vlq(file)
            read_len += delta_len
            
            event_code = btoi(file.read(1))
            if event_code & 0x80:
                last_status = event_code
                read_len += 1
            else:
                event_code = last_status
                file.seek(-1, os.SEEK_CUR)

            if event_code == 0xFF:
                meta_type = btoi(file.read(1))
                meta_len, vlq_len = read_vlq(file)
                meta_data = file.read(meta_len)
                read_len += 1 + vlq_len + meta_len
                meta_chunks.append(MetaChunk(track_num, chunk_num, delta_time, meta_type, meta_len, meta_data))
            elif event_code == 0xF7 or event_code == 0xF0:
                sysex_len, vlq_len = read_vlq(file)
                sysex_data = file.read(sysex_len)
                read_len += vlq_len + sysex_len
                sysex_chunks.append(SysExChunk(track_num, chunk_num, delta_time, sysex_len, sysex_data))
            else:
                event_code &= 0xF0
                if event_code == 0xC0 or event_code  == 0xD0:
                    midi_event_1b = btoi(file.read(1))
                    read_len += 1
                    midi_chunks.append(MidiChunk(track_num, chunk_num, delta_time, event_code, midi_event_1b))
                elif event_code == 0x80 or event_code == 0x90 \
                        or event_code == 0xA0 or event_code == 0xB0 \
                        or event_code == 0xE0:
                    midi_event_2b = btoi(file.read(2))
                    read_len += 2
                    midi_chunks.append(MidiChunk(track_num, chunk_num, delta_time, event_code, midi_event_2b))
            chunk_num += 1
        track_num += 1
    return meta_chunks, sysex_chunks, midi_chunks


def main():
    with open("openmsx/coconut_run2.mid", "rb") as file:
        h = read_header(file)
        t = read_tracks(file)
        print(len(t[0]) + len(t[1]) + len(t[2]))
        

if __name__ == "__main__":
    main()