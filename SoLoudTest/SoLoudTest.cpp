#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_freeverbfilter.h"
#include "PS1ReverbFilter.hpp"

using namespace std;

int main()
{
    SoLoud::Soloud soloud;
    soloud.init();

    //SoLoud::FreeverbFilter filter;
    //filter.setParams(0.0f, 0.8f, 0.5f, 1.0f);

    PS1ReverbFilter filter;
    //filter.setPreset("Room");
    //filter.setPreset("Studio Small");
    //filter.setPreset("Studio Medium");
    filter.setPreset("Studio Large");
    //filter.setPreset("Hall");
    //filter.setPreset("Half Echo");
    //filter.setPreset("Space Echo");
    //filter.setPreset("Chaos Echo");
    //filter.setPreset("Delay");
    //filter.setPreset("Reverb Off");

    SoLoud::Bus mainBus;
    SoLoud::Bus reverbBus;

    soloud.play(mainBus);
    mainBus.play(reverbBus);

    reverbBus.setFilter(0, &filter);

    std::string prefix = "sfx/";
    std::string suffix = ".wav";

    for (int i = 0; i < 1544; i++) {
        cout << "Playing sound " << i << endl;

        std::string filename = prefix + std::to_string(i) + suffix;

        SoLoud::Wav sound;
        sound.load(filename.c_str());
        float lengthInSeconds = sound.getLength() + 2;

        mainBus.play(sound);
        reverbBus.play(sound);

        // wait full length so next clip doesn't overlap
        this_thread::sleep_for(chrono::duration<float>(lengthInSeconds));
    }

    soloud.deinit();

    cout << "Done" << endl;
    return 0;
}
