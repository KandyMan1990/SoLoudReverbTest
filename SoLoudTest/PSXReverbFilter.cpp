#include "PSXReverbFilter.h"

SoLoud::FilterInstance* PSXReverbFilter::createInstance() {
	return new PSXReverbFilterInstance();
}

PSXReverbFilterInstance::PSXReverbFilterInstance() {
    activate(&mReverb);

    mPreset = 4;

    setPort(&mReverb, PortIndex::PSX_REV_WET, &mWet);
    setPort(&mReverb, PortIndex::PSX_REV_DRY, &mDry);
    setPort(&mReverb, PortIndex::PSX_REV_PRESET, &mPreset);
    setPort(&mReverb, PortIndex::PSX_REV_MASTER, &mMaster);
}

void PSXReverbFilterInstance::filter(float* aBuffer, unsigned int aSamples, unsigned int aBufferSize,
	unsigned int aChannels, float aSamplerate, SoLoud::time aTime) {

    setPort(&mReverb, PortIndex::PSX_REV_MAIN0_IN, (float*)aBuffer);
    setPort(&mReverb, PortIndex::PSX_REV_MAIN1_IN, (float*)aBuffer + 1);
    setPort(&mReverb, PortIndex::PSX_REV_MAIN0_OUT, (float*)aBuffer);
    setPort(&mReverb, PortIndex::PSX_REV_MAIN1_OUT, (float*)aBuffer + 1);

    run(&mReverb, aSamples);
}