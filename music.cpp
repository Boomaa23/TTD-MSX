
#pragma comment (lib, "winmm.lib")

#include <iostream>

#include <windows.h>
#include <mmsystem.h>


int main() {
   int retval;
   HMIDIOUT device;

   retval = midiOutOpen(&device, 0, 0, 0, CALLBACK_NULL);
   if (retval != MMSYSERR_NOERROR) {
      printf("Error opening MIDI Output.\n");
      return 1;
   }

   midiOutReset(device);
   midiOutClose(device);
   return 0;
}