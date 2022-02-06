#pragma comment (lib, "winmm.lib")

#include "common.h"
#include "midi.h"

int main() {
    MidiFile file = MidiFile("../openmsx/ultimate_run.mid");
    std::cout << (uint32_t) file.OpenFile() << std::endl;
    std::cout << (uint32_t) file.ReadHeader() << std::endl;
    std::cout << (uint32_t) file.ReadTracks() << std::endl;
    std::cout << file.GetTracks()->size() << std::endl;
    std::cout << file.GetHeader()->tickdiv << std::endl;

    MidiDevice device = MidiDevice();
    device.Open();
    device.Reset();
    std::cout <<(uint32_t)  device.Queue(file.GetTracks()) << std::endl;
    device.Start(file.GetHeader()->tickdiv);
    device.Close();
    return 0;
}