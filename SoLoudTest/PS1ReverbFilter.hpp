#pragma once
#include "soloud.h"
#include "soloud_filter.h"
#include "PS1Reverb.h" // Your Mednafen-style PS1Reverb implementation
#include <string>
#include <mutex>
#include <array>

// -------------------- Parent filter first --------------------
class PS1ReverbFilter : public SoLoud::Filter
{
public:
    PS1ReverbFilter() = default;

    // Factory for instance
    SoLoud::FilterInstance* createInstance() override;

    void setPreset(const std::string& presetName)
    {
        std::lock_guard<std::mutex> lock(mPresetMutex);
        mPreset = presetName;
    }

    std::string getPreset() const
    {
        std::lock_guard<std::mutex> lock(mPresetMutex);
        return mPreset;
    }

    mutable std::mutex mPresetMutex;
    std::string mPreset{ "Reverb Off" };
};

// -------------------- Instance filter after --------------------
class PS1ReverbFilterInstance : public SoLoud::FilterInstance
{
public:
    explicit PS1ReverbFilterInstance(PS1ReverbFilter* aParent)
        : mParent(aParent)
    {
        // Initialize internal PS1Reverb state
        mReverb.Init(); // Ensure PS1Reverb::Init() sets RvbResPos=0, ReverbCur=ReverbWA, clears buffers
    }

    void filter(float* aBuffer, unsigned int aSamples, unsigned int aBufferSize,
        unsigned int aChannels, float aSamplerate, SoLoud::time aTime) override
    {
        if (aChannels < 2) return; // PS1Reverb is stereo

        // Get the current preset string safely
        std::string preset;
        {
            std::lock_guard<std::mutex> lock(mParent->mPresetMutex);
            preset = mParent->mPreset;
        }

        // Apply preset if changed
        mReverb.SetPreset(preset);

        int32_t inBlock[2]{ 0 };
        int32_t outBlock[2]{ 0 };

        for (unsigned int s = 0; s < aSamples; s++)
        {
            // Convert float [-1,1] to int16
            for (unsigned int ch = 0; ch < 2; ch++)
                inBlock[ch] = static_cast<int16_t>(aBuffer[s * aChannels + ch] * 32767.0f);

            // Run PS1 reverb
            mReverb.RunReverb(inBlock, outBlock);

            // Write wet-only output back to buffer
            for (unsigned int ch = 0; ch < 2; ch++)
                aBuffer[s * aChannels + ch] = outBlock[ch] / 32768.0f;
        }
    }

private:
    PS1ReverbFilter* mParent;
    PS1Reverb mReverb;
};

// -------------------- Implementation of createInstance --------------------
inline SoLoud::FilterInstance* PS1ReverbFilter::createInstance()
{
    return new PS1ReverbFilterInstance(this);
}
