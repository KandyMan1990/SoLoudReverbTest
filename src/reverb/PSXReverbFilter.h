#pragma once

#include <soloud.h>
#include <soloud_filter.h>
#include "PSXReverb.hpp"

class PSXReverbFilter : public SoLoud::Filter {
public:
    SoLoud::FilterInstance* createInstance() override;
};

class PSXReverbFilterInstance : public SoLoud::FilterInstance {
public:
    PSXReverbFilterInstance();

    void filter(float* aBuffer, unsigned int aSamples, unsigned int aChannels, float aSamplerate, SoLoud::time aTime) override;

private:
    PsxReverb mReverb;
    float mWet;
    float mDry;
    float mPreset;
    float mMaster;
};