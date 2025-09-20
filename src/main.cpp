#define WITH_SDL2

#include <iostream>
#include <string>
#include <thread>

#include "../soloud/include/soloud.h"
#include "../soloud/include/soloud_wav.h"
#include "reverb/PSXReverbFilter.h"

int main()
{
    SoLoud::Soloud soloud;
    soloud.init();

    PSXReverbFilter filter;

    SoLoud::Bus reverbBus;
    soloud.play(reverbBus);
    reverbBus.setFilter(0, &filter);

    std::string prefix = "sfx/";
    std::string suffix = ".wav";

    for (int i = 0; i < 1544; i++) {
        std::cout << "Playing sound " << i << std::endl;

        std::string filename = prefix + std::to_string(i) + suffix;

        SoLoud::Wav sound;
        sound.load(filename.c_str());
        float lengthInSeconds = sound.getLength() + 2;

        reverbBus.play(sound);

        std::this_thread::sleep_for(std::chrono::duration<float>(lengthInSeconds));
    }

    soloud.deinit();

    return 0;
}