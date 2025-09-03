#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <array>
#include <vector>
#include <soloud.h>
#include <soloud_filter.h>

// PS1 Reverb registers struct (32 values)
struct PS1ReverbRegs {
    uint16_t vLOUT, vROUT;                     // Reverb Output Volume L/R
    uint16_t dAPF1, dAPF2;                     // APF offsets (addresses in SPU, used as delay sizes here)
    uint16_t vIIR;                             // Reflection / extra volume
    uint16_t vCOMB1, vCOMB2, vCOMB3, vCOMB4;   // Comb volumes (feedback)
    uint16_t vWALL;                            // Wall/reflection volume
    uint16_t vAPF1, vAPF2;                     // APF volumes (gains)
    uint16_t mLSAME, mRSAME;                   // same-side reflection addresses
    uint16_t mLCOMB1, mRCOMB1;                 // comb addresses 1
    uint16_t mLCOMB2, mRCOMB2;                 // comb addresses 2
    uint16_t dLSAME, dRSAME;                   // same-side reflection addresses 2
    uint16_t mLDIFF, mRDIFF;                   // different-side reflect addresses 1
    uint16_t mLCOMB3, mRCOMB3;                 // comb addresses 3
    uint16_t mLCOMB4, mRCOMB4;                 // comb addresses 4
    uint16_t dLDIFF, dRDIFF;                   // different-side reflect addresses 2
    uint16_t mLAPF1, mRAPF1;                   // APF addresses 1
    uint16_t mLAPF2, mRAPF2;                   // APF addresses 2

    PS1ReverbRegs() = default;

    // Construct from array-of-32 (keeps ordering explicit)
    PS1ReverbRegs(const std::array<uint16_t, 32>& a) {
        // copy 32*2 bytes directly into the struct layout
        std::memcpy(this, a.data(), sizeof(uint16_t) * 32);
    }
};

// The canonical preset map is defined in the cpp
extern const std::unordered_map<std::string, PS1ReverbRegs> PS1_CANONICAL_PRESETS;


// Forward declarations
class PS1ReverbFilter;

// Per-instance filter (SoLoud requires a FilterInstance for per-playback state)
class PS1ReverbFilterInstance : public SoLoud::FilterInstance
{
public:
    explicit PS1ReverbFilterInstance(PS1ReverbFilter* aParent);
    ~PS1ReverbFilterInstance() override = default;

    // SoLoud's FilterInstance filter() signature
    void filter(float* aBuffer, unsigned int aSamples, unsigned int aBufferSize,
        unsigned int aChannels, float aSamplerate, SoLoud::time aTime) override;

private:
    PS1ReverbFilter* mParent;

    // Comb buffers — 4 per channel
    std::array<std::vector<float>, 4> mLComb;
    std::array<std::vector<float>, 4> mRComb;

    // circular indices
    std::array<size_t, 4> mLIndex{};
    std::array<size_t, 4> mRIndex{};

    // simple two-stage scalar APF state per channel (keeps implementation compact & robust)
    float mLAPF1State = 0.0f;
    float mLAPF2State = 0.0f;
    float mRAPF1State = 0.0f;
    float mRAPF2State = 0.0f;

    // Helper to (re)initialise buffers when sample rate or preset changes
    void initBuffers(float aSamplerate);
};


// Filter object (shared preset data here; createInstance will produce instances)
class PS1ReverbFilter : public SoLoud::Filter
{
public:
    explicit PS1ReverbFilter(const std::string& presetName = "Hall");
    ~PS1ReverbFilter() override = default;

    // SoLoud expects this to be implemented
    SoLoud::FilterInstance* createInstance() override { return new PS1ReverbFilterInstance(this); }

    // Parameters (exposed)
    float mWet = 1;       // default wet/dry mix
    PS1ReverbRegs mRegs;     // current preset registers (copied from PS1_CANONICAL_PRESETS)
};
