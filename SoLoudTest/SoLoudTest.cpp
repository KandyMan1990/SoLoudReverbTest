#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_freeverbfilter.h"
#include "PS1Reverb.h"
//#include "PS1ReverbFilter.hpp"

using namespace std;



int main()
{
    SoLoud::Soloud soloud;
    soloud.init();

    //SoLoud::FreeverbFilter filter;
    //filter.setParams(0.0f, 0.8f, 0.5f, 1.0f);

    //PS1ReverbFilter *filter = new PS1ReverbFilter("Room");
    //PS1ReverbFilter *filter = new PS1ReverbFilter("Studio Small");
    PS1ReverbFilter *filter = new PS1ReverbFilter("Studio Medium");
    //PS1ReverbFilter *filter = new PS1ReverbFilter("Studio Large");
    //PS1ReverbFilter *filter = new PS1ReverbFilter("Hall");
    //PS1ReverbFilter *filter = new PS1ReverbFilter("Half Echo");
    //PS1ReverbFilter *filter = new PS1ReverbFilter("Space Echo");
    //PS1ReverbFilter *filter = new PS1ReverbFilter("Chaos Echo");
    //PS1ReverbFilter *filter = new PS1ReverbFilter("Delay");
    //PS1ReverbFilter *filter = new PS1ReverbFilter("Reverb Off");

    SoLoud::Bus mainBus;
    SoLoud::Bus reverbBus;

    soloud.play(mainBus);
    mainBus.play(reverbBus);

    reverbBus.setFilter(0, filter);

    std::string prefix = "sfx/";
    std::string suffix = ".wav";

    for (int i = 0; i < 1544; i++) {
        cout << "Playing sound " << i << endl;

        std::string filename = prefix + std::to_string(i) + suffix;

        SoLoud::Wav sound;
        sound.load(filename.c_str());
        float lengthInSeconds = sound.getLength() + 2;

        mainBus.play(sound);
        mainBus.play(reverbBus);
        reverbBus.play(sound);

        // wait full length so next clip doesn't overlap
        this_thread::sleep_for(chrono::duration<float>(lengthInSeconds));
    }

    soloud.deinit();

    cout << "Done" << endl;
    return 0;
}
