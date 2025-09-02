#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include "soloud.h"
#include "soloud_wav.h"
#include "PS1Reverb.h"

using namespace std;

int main()
{
    SoLoud::Soloud soloud;
    soloud.init();

    PS1ReverbFilter* roomReverb = new PS1ReverbFilter("Studio Medium");
    //PS1ReverbFilter* studioSmallReverb = new PS1ReverbFilter("Studio Small");
    //PS1ReverbFilter* studioMediumReverb = new PS1ReverbFilter("Studio Medium");
    //PS1ReverbFilter* studioLargeReverb = new PS1ReverbFilter("Studio Large");
    //PS1ReverbFilter* hallReverb = new PS1ReverbFilter("Hall");
    //PS1ReverbFilter* halfEchoReverb = new PS1ReverbFilter("Half Echo");
    //PS1ReverbFilter* spaceEchoReverb = new PS1ReverbFilter("Space Echo");
    //PS1ReverbFilter* chaosEchoReverb = new PS1ReverbFilter("Chaos Echo");
    //PS1ReverbFilter* delayReverb = new PS1ReverbFilter("Delay");
    //PS1ReverbFilter* offReverb = new PS1ReverbFilter("Reverb Off");

    SoLoud::Wav myWav;
    std::string prefix = "sfx/";
    std::string suffix = ".wav";

    for (int i = 0; i < 1544; i++) {
        cout << "Playing sound " << i << endl;

        std::string index = std::to_string(i);
        std::string filename = prefix + index + suffix;

        myWav.load(filename.c_str());
        myWav.setFilter(0, roomReverb);
        soloud.play(myWav);

        float lengthInSeconds = myWav.getLength() + 1;
    
        std::this_thread::sleep_for(std::chrono::duration<float>(lengthInSeconds));
    }

    soloud.deinit();

    cout << "Done" << endl;
    return 0;
}
