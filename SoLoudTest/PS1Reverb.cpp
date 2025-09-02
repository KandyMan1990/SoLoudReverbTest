#include "PS1Reverb.h"
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------
// PS1 canonical presets (full set)
std::unordered_map<std::string, PS1ReverbRegs> PS1_CANONICAL_PRESETS = {
    { "Room", PS1ReverbRegs(
        0x007D,0x005B,0x6D80,0x54B8,
        0xBED0,0x0000,0x0000,0xBA80,
        0x5800,0x5300,0x04D6,0x0333,
        0x03F0,0x0227,0x0374,0x01EF,
        0x0334,0x01B5,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,
        0x01B4,0x0136
    ) },
    { "Studio Small", PS1ReverbRegs(
        0x0033,0x0025,0x70F0,0x4FA8,
        0xBCE0,0x4410,0xC0F0,0x9C00,
        0x5280,0x4EC0,0x03E4,0x031B,
        0x03A4,0x02AF,0x0372,0x0266,
        0x031C,0x025D,0x025C,0x018E,
        0x022F,0x0135,0x01D2,0x00B7,
        0x018F,0x00B5
    ) },
    { "Studio Medium", PS1ReverbRegs(
        0x00B1,0x007F,0x70F0,0x4FA8,
        0xBCE0,0x4510,0xBEF0,0xB4C0,
        0x5280,0x4EC0,0x0904,0x076B,
        0x0824,0x065F,0x07A2,0x0616,
        0x076C,0x05ED,0x05EC,0x042E,
        0x050F,0x0305,0x0462,0x02B7,
        0x042F,0x0265
    ) },
    { "Studio Large", PS1ReverbRegs(
        0x00E3,0x00A9,0x6F60,0x4FA8,
        0xBCE0,0x4510,0xBEF0,0xA680,
        0x5680,0x52C0,0x0DFB,0x0B58,
        0x0D09,0x0A3C,0x0BD9,0x0973,
        0x0B59,0x08DA,0x08D9,0x05E9,
        0x07EC,0x04B0,0x06EF,0x03D2,
        0x05EA,0x031D
    ) },
    { "Hall", PS1ReverbRegs(
        0x01A5,0x0139,0x6000,0x5000,
        0x4C00,0xB800,0xBC00,0xC000,
        0x6000,0x5C00,0x15BA,0x11BB,
        0x14C2,0x10BD,0x11BC,0x0DC1,
        0x11C0,0x0DC3,0x0DC0,0x09C1,
        0x0BC4,0x07C1,0x0A00,0x06CD,
        0x09C2,0x05C1
    ) },
    { "Half Echo", PS1ReverbRegs(
        0x0017,0x0013,0x70F0,0x4FA8,
        0xBCE0,0x4510,0xBEF0,0x8500,
        0x5F80,0x54C0,0x0371,0x02AF,
        0x02E5,0x01DF,0x02B0,0x01D7,
        0x0358,0x026A,0x01D6,0x011E,
        0x012D,0x00B1,0x011F,0x0059,
        0x01A0,0x00E3
    ) },
    { "Space Echo", PS1ReverbRegs(
        0x033D,0x0231,0x7E00,0x5000,
        0xB400,0xB000,0x4C00,0xB000,
        0x6000,0x5400,0x1ED6,0x1A31,
        0x1D14,0x183B,0x1BC2,0x16B2,
        0x1A32,0x15EF,0x15EE,0x1055,
        0x1334,0x0F2D,0x11F6,0x0C5D,
        0x1056,0x0AE1
    ) },
    { "Chaos Echo", PS1ReverbRegs(
        0x0001,0x0001,0x7FFF,0x7FFF,
        0x0000,0x0000,0x0000,0x8100,
        0x0000,0x0000,0x1FFF,0x0FFF,
        0x1005,0x0005,0x0000,0x0000,
        0x1005,0x0005,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,
        0x1004,0x1002
    ) },
    { "Delay", PS1ReverbRegs(
        0x0001,0x0001,0x7FFF,0x7FFF,
        0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x1FFF,0x0FFF,
        0x1005,0x0005,0x0000,0x0000,
        0x1005,0x0005,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,
        0x1004,0x1002
    ) },
    { "Reverb Off", PS1ReverbRegs(
        0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0001,0x0001,
        0x0001,0x0001,0x0001,0x0001,
        0x0000,0x0000,0x0001,0x0001,
        0x0001,0x0001,0x0001,0x0001,
        0x0000,0x0000
    ) }
};

// -------------------------------
// Filter instance constructor
PS1ReverbFilterInstance::PS1ReverbFilterInstance(PS1ReverbFilter* aParent)
    : mParent(aParent)
{
    const uint16_t combSizesL[4] = {
        mParent->mRegs.mLCOMB1, mParent->mRegs.mLCOMB2,
        mParent->mRegs.mLCOMB3, mParent->mRegs.mLCOMB4
    };
    const uint16_t combSizesR[4] = {
        mParent->mRegs.mRCOMB1, mParent->mRegs.mRCOMB2,
        mParent->mRegs.mRCOMB3, mParent->mRegs.mRCOMB4
    };

    for (int c = 0; c < 4; ++c) {
        mLComb[c].resize(combSizesL[c], 0.0f);
        mRComb[c].resize(combSizesR[c], 0.0f);
        mLIndex[c] = 0;
        mRIndex[c] = 0;
    }
}

// ---------------------------------------------------------
void PS1ReverbFilterInstance::filter(float* aBuffer, unsigned int aSamples,
    unsigned int aBufferSize,
    unsigned int aChannels, float aSamplerate,
    SoLoud::time aTime)
{
    const auto& r = mParent->mRegs;

    const float combGainL[4] = { r.vCOMB1 / 32768.0f, r.vCOMB2 / 32768.0f,
                                 r.vCOMB3 / 32768.0f, r.vCOMB4 / 32768.0f };
    const float combGainR[4] = { r.vCOMB1 / 32768.0f, r.vCOMB2 / 32768.0f,
                                 r.vCOMB3 / 32768.0f, r.vCOMB4 / 32768.0f };

    const float apfGainL[2] = { r.mLAPF1 / 32768.0f, r.mLAPF2 / 32768.0f };
    const float apfGainR[2] = { r.mRAPF1 / 32768.0f, r.mRAPF2 / 32768.0f };

    for (unsigned int i = 0; i < aSamples; ++i)
    {
        float inL = aBuffer[i * aChannels + 0];
        float inR = aBuffer[i * aChannels + 1];
        float outL = 0.0f;
        float outR = 0.0f;

        // --- Comb filters ---
        for (int c = 0; c < 4; ++c)
        {
            float fbL = mLComb[c][mLIndex[c]] * combGainL[c];
            float fbR = mRComb[c][mRIndex[c]] * combGainR[c];

            mLComb[c][mLIndex[c]] = inL + fbL;
            mRComb[c][mRIndex[c]] = inR + fbR;

            outL += mLComb[c][mLIndex[c]] / 4.0f;
            outR += mRComb[c][mRIndex[c]] / 4.0f;

            mLIndex[c] = (mLIndex[c] + 1) % mLComb[c].size();
            mRIndex[c] = (mRIndex[c] + 1) % mRComb[c].size();
        }

        // --- All-pass filters ---
        float apfL1 = outL + apfGainL[0] * mLAPF1Buf;
        mLAPF1Buf = apfL1 - mLAPF1Buf;
        float apfL2 = apfL1 + apfGainL[1] * mLAPF2Buf;
        mLAPF2Buf = apfL2 - mLAPF2Buf;
        outL = apfL2;

        float apfR1 = outR + apfGainR[0] * mRAPF1Buf;
        mRAPF1Buf = apfR1 - mRAPF1Buf;
        float apfR2 = apfR1 + apfGainR[1] * mRAPF2Buf;
        mRAPF2Buf = apfR2 - mRAPF2Buf;
        outR = apfR2;

        // --- Wet/dry mix ---
        aBuffer[i * aChannels + 0] = inL * (1.0f - mParent->mWet) + outL * mParent->mWet;
        aBuffer[i * aChannels + 1] = inR * (1.0f - mParent->mWet) + outR * mParent->mWet;
    }
}

// ---------------------------------------------------------
// PS1ReverbFilter
PS1ReverbFilter::PS1ReverbFilter(const std::string& presetName)
    : mRegs(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
{
    mRegs = PS1_CANONICAL_PRESETS.at(presetName);
}

// ---------------------------------------------------------
SoLoud::FilterInstance* PS1ReverbFilter::createInstance()
{
    return new PS1ReverbFilterInstance(this);
}