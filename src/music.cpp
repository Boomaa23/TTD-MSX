
#pragma comment (lib, "winmm.lib")

#include "common.h"
#include "midi.h"


DWORD sendMIDIEvent(BYTE bStatus, BYTE bData1, BYTE bData2) { 
    union { 
        DWORD dwData; 
        BYTE bData[4]; 
    } u; 
 
    // Construct the MIDI message. 
 
    u.bData[0] = bStatus;  // MIDI status byte 
    u.bData[1] = bData1;   // first MIDI data byte 
    u.bData[2] = bData2;   // second MIDI data byte 
    u.bData[3] = 0; 
 
    // Send the message. 
    return u.dwData;
} 

int main() {
    MidiFile file = MidiFile("../openmsx/flying_scotsman.mid");
    std::cout << (uint32_t) file.OpenFile() << std::endl;
    std::cout << (uint32_t) file.ReadHeader() << std::endl;
    std::cout << (uint32_t) file.ReadTracks() << std::endl;
    std::cout << file.GetTracks()->size() << std::endl;
    std::cout << file.GetHeader()->tickdiv << std::endl;
    MidiDevice device = MidiDevice();
    device.Open();
    device.Queue(file.GetTracks());
    device.Start(file.GetHeader()->tickdiv);
    Sleep(5000);
    device.Close();
//    MidiDevice stream = MidiDevice();
//    stream.Open();
// //    midiOutShortMsg((HMIDIOUT) *stream.GetStream(), sendMIDIEvent(0x90, 0x45, 0x40));
// //    midiOutShortMsg((HMIDIOUT) *stream.GetStream(), sendMIDIEvent(0x90, 0x43, 0x40));
//    std::vector<DWORD> data;
//    data.push_back(sendMIDIEvent(0x90, 0x45, 0x40));
//    data.push_back(sendMIDIEvent(0x90, 0x43, 0x40));
//    data.push_back(sendMIDIEvent(0x80, 0x43, 0x00));
//    data.push_back(sendMIDIEvent(0x90, 0x45, 0x40));
//    data.push_back(sendMIDIEvent(0x80, 0x45, 0x00));
//    data.push_back(sendMIDIEvent(0x80, 0x40, 0x00));
//    data.push_back(sendMIDIEvent(0x90, 0x3C, 0x40));
//    data.push_back(sendMIDIEvent(0x90, 0x47, 0x40));
//    data.push_back(sendMIDIEvent(0x80, 0x3C, 0x00));
//    data.push_back(sendMIDIEvent(0x90, 0x48, 0x40));
//    data.push_back(sendMIDIEvent(0x80, 0x48, 0x00));
//    data.push_back(sendMIDIEvent(0x80, 0x3C, 0x40));
//    stream.Queue(&data);
//    std::cout << (uint32_t) stream.Start() << std::endl;
   
//    Sleep(5000);
//    stream.Reset();
//    stream.Close();
   return 0;
}