#pragma once

#include <soloud.h>
#include <soloud_filter.h>
#include "PSXReverb.hpp"

class PSXReverbFilter : public SoLoud::Filter {
public:
	virtual SoLoud::FilterInstance* createInstance();
};

class PSXReverbFilterInstance : public SoLoud::FilterInstance {
public:
	PSXReverbFilterInstance();

	void filter(float* aBuffer, unsigned int aSamples, unsigned int aBufferSize,
		unsigned int aChannels, float aSamplerate, SoLoud::time aTime) override;

private:
	PsxReverb mReverb;
	float mWet;
	float mDry;
	float mPreset;
	float mMaster;
};