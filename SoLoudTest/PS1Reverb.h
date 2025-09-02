#pragma once

#include <soloud.h>
#include <soloud_filter.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <cstdint>

// PS1 Reverb Registers
struct PS1ReverbRegs {
    uint16_t vCOMB1, vCOMB2, vCOMB3, vCOMB4;
    uint16_t mLCOMB1, mLCOMB2, mLCOMB3, mLCOMB4;
    uint16_t mRCOMB1, mRCOMB2, mRCOMB3, mRCOMB4;
    uint16_t dAPF1, dAPF2;
    uint16_t mLAPF1, mLAPF2;
    uint16_t mRAPF1, mRAPF2;
    uint16_t vWALL;
    uint16_t vLIN, vRIN;
    uint16_t vLOUT, vROUT;
    uint32_t regionSizeBytes;
    uint16_t reserved1;
    uint16_t reserved2;

    PS1ReverbRegs(
        uint16_t vC1, uint16_t vC2, uint16_t vC3, uint16_t vC4,
        uint16_t mLC1, uint16_t mLC2, uint16_t mLC3, uint16_t mLC4,
        uint16_t mRC1, uint16_t mRC2, uint16_t mRC3, uint16_t mRC4,
        uint16_t dAPF1_, uint16_t dAPF2_,
        uint16_t mLAPF1_, uint16_t mLAPF2_,
        uint16_t mRAPF1_, uint16_t mRAPF2_,
        uint16_t vWALL_, uint16_t vLIN_, uint16_t vRIN_,
        uint16_t vLOUT_, uint16_t vROUT_,
        uint32_t regionSizeBytes_,
        uint16_t res1, uint16_t res2
    )
        : vCOMB1(vC1), vCOMB2(vC2), vCOMB3(vC3), vCOMB4(vC4),
        mLCOMB1(mLC1), mLCOMB2(mLC2), mLCOMB3(mLC3), mLCOMB4(mLC4),
        mRCOMB1(mRC1), mRCOMB2(mRC2), mRCOMB3(mRC3), mRCOMB4(mRC4),
        dAPF1(dAPF1_), dAPF2(dAPF2_),
        mLAPF1(mLAPF1_), mLAPF2(mLAPF2_),
        mRAPF1(mRAPF1_), mRAPF2(mRAPF2_),
        vWALL(vWALL_), vLIN(vLIN_), vRIN(vRIN_),
        vLOUT(vLOUT_), vROUT(vROUT_),
        regionSizeBytes(regionSizeBytes_),
        reserved1(res1), reserved2(res2) {
    }
};

// Full PS1 canonical presets
extern std::unordered_map<std::string, PS1ReverbRegs> PS1_CANONICAL_PRESETS;

// Forward declaration
class PS1ReverbFilter;

// PS1 Filter Instance
class PS1ReverbFilterInstance : public SoLoud::FilterInstance
{
public:
    PS1ReverbFilterInstance(PS1ReverbFilter* aParent);

    void filter(float* aBuffer, unsigned int aSamples, unsigned int aBufferSize,
        unsigned int aChannels, float aSamplerate, SoLoud::time aTime) override;

private:
    PS1ReverbFilter* mParent;

    // Comb filter buffers (4 per channel)
    std::array<std::vector<float>, 4> mLComb;
    std::array<std::vector<float>, 4> mRComb;
    std::array<size_t, 4> mLIndex{};
    std::array<size_t, 4> mRIndex{};

    // All-pass filter buffers
    float mLAPF1Buf = 0.0f;
    float mLAPF2Buf = 0.0f;
    float mRAPF1Buf = 0.0f;
    float mRAPF2Buf = 0.0f;
};

// PS1 Filter
class PS1ReverbFilter : public SoLoud::Filter
{
public:
    PS1ReverbFilter(const std::string& presetName);
    ~PS1ReverbFilter() override = default;

    SoLoud::FilterInstance* createInstance() override;

    float mWet = 0.5f;
    PS1ReverbRegs mRegs;
};
