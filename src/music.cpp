
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
   MidiStream stream = MidiStream();
   stream.Open();
   stream.SetTimeDiv(60);

   std::vector<DWORD> data;
   data.push_back(sendMIDIEvent(0x90, 0x45, 0x40));
   data.push_back(sendMIDIEvent(0x90, 0x43, 0x40));
   data.push_back(sendMIDIEvent(0x80, 0x43, 0x00));
   data.push_back(sendMIDIEvent(0x90, 0x45, 0x40));
   data.push_back(sendMIDIEvent(0x80, 0x45, 0x00));
   data.push_back(sendMIDIEvent(0x80, 0x40, 0x00));
   data.push_back(sendMIDIEvent(0x90, 0x3C, 0x40));
   data.push_back(sendMIDIEvent(0x90, 0x47, 0x40));
   data.push_back(sendMIDIEvent(0x80, 0x3C, 0x00));
   data.push_back(sendMIDIEvent(0x90, 0x48, 0x40));
   data.push_back(sendMIDIEvent(0x80, 0x48, 0x00));
   data.push_back(sendMIDIEvent(0x80, 0x3C, 0x40));
   stream.Queue(&data);
   
   std::cout << data.size() << (uint32_t) stream.Start() << std::endl;
   
   Sleep(5000);
   stream.Close();
   return 0;
}