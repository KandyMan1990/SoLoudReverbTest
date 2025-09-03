// PS1ReverbFilter.cpp
#include "PS1Reverb.h"

#include <array>
#include <vector>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdexcept>

// -----------------------------
// Utility helpers
static inline float reg_to_gain(uint16_t v)
{
    if (v == 0x8000) return -1.0f;
    // Treat as signed 16-bit
    int16_t s = static_cast<int16_t>(v);
    return (float)s / 32768.0f;
}

static inline size_t addr_to_delay(uint16_t addr)
{
    // Map PS1 SPU address-like register to a delay length in samples.
    // Use lower 12 bits (0..4095) as base length. Avoid 0 by picking a small fallback.
    // This mapping is deterministic and consistent across presets.
    unsigned int low12 = addr & 0x0FFFu;
    if (low12 == 0)
        low12 = 64; // avoid zero-length combs (small default)
    // Cap to a sane maximum to avoid huge memory use in POC
    constexpr unsigned int MAX_DELAY = 32768u;
    if (low12 > MAX_DELAY) low12 = MAX_DELAY;
    return static_cast<size_t>(low12);
}

// -----------------------------
// Canonical presets (from PSXSPX) as 32-value arrays.
// Each PS1ReverbRegs constructor in your header expects the 32 values in the header order.
// We create PS1ReverbRegs via an array-to-struct memcpy constructor (header provided).
// Keep these entries exactly as PSXSPX lists them.
const std::unordered_map<std::string, PS1ReverbRegs> PS1_CANONICAL_PRESETS = {
    { "Room", PS1ReverbRegs(std::array<uint16_t,32>{
        0x007D,0x005B,0x6D80,0x54B8,0xBED0,0x0000,0x0000,0xBA80,
        0x5800,0x5300,0x04D6,0x0333,0x03F0,0x0227,0x0374,0x01EF,
        0x0334,0x01B5,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x01B4,0x0136,0x00B8,0x005C,0x8000,0x8000 }) },

    { "Studio Small", PS1ReverbRegs(std::array<uint16_t,32>{
        0x0033,0x0025,0x70F0,0x4FA8,0xBCE0,0x4410,0xC0F0,0x9C00,
        0x5280,0x4EC0,0x03E4,0x031B,0x03A4,0x02AF,0x0372,0x0266,
        0x031C,0x025D,0x025C,0x018E,0x022F,0x0135,0x01D2,0x00B7,
        0x018F,0x00B5,0x00B4,0x0080,0x004C,0x0026,0x8000,0x8000 }) },

    { "Studio Medium", PS1ReverbRegs(std::array<uint16_t,32>{
        0x00B1,0x007F,0x70F0,0x4FA8,0xBCE0,0x4510,0xBEF0,0xB4C0,
        0x5280,0x4EC0,0x0904,0x076B,0x0824,0x065F,0x07A2,0x0616,
        0x076C,0x05ED,0x05EC,0x042E,0x050F,0x0305,0x0462,0x02B7,
        0x042F,0x0265,0x0264,0x01B2,0x0100,0x0080,0x8000,0x8000 }) },

    { "Studio Large", PS1ReverbRegs(std::array<uint16_t,32>{
        0x00E3,0x00A9,0x6F60,0x4FA8,0xBCE0,0x4510,0xBEF0,0xA680,
        0x5680,0x52C0,0x0DFB,0x0B58,0x0D09,0x0A3C,0x0BD9,0x0973,
        0x0B59,0x08DA,0x08D9,0x05E9,0x07EC,0x04B0,0x06EF,0x03D2,
        0x05EA,0x031D,0x031C,0x0238,0x0154,0x00AA,0x8000,0x8000 }) },

    { "Hall", PS1ReverbRegs(std::array<uint16_t,32>{
        0x01A5,0x0139,0x6000,0x5000,0x4C00,0xB800,0xBC00,0xC000,
        0x6000,0x5C00,0x15BA,0x11BB,0x14C2,0x10BD,0x11BC,0x0DC1,
        0x11C0,0x0DC3,0x0DC0,0x09C1,0x0BC4,0x07C1,0x0A00,0x06CD,
        0x09C2,0x05C1,0x05C0,0x041A,0x0274,0x013A,0x8000,0x8000 }) },

    { "Half Echo", PS1ReverbRegs(std::array<uint16_t,32>{
        0x0017,0x0013,0x70F0,0x4FA8,0xBCE0,0x4510,0xBEF0,0x8500,
        0x5F80,0x54C0,0x0371,0x02AF,0x02E5,0x01DF,0x02B0,0x01D7,
        0x0358,0x026A,0x01D6,0x011E,0x012D,0x00B1,0x011F,0x0059,
        0x01A0,0x00E3,0x0058,0x0040,0x0028,0x0014,0x8000,0x8000 }) },

    { "Space Echo", PS1ReverbRegs(std::array<uint16_t,32>{
        0x033D,0x0231,0x7E00,0x5000,0xB400,0xB000,0x4C00,0xB000,
        0x6000,0x5400,0x1ED6,0x1A31,0x1D14,0x183B,0x1BC2,0x16B2,
        0x1A32,0x15EF,0x15EE,0x1055,0x1334,0x0F2D,0x11F6,0x0C5D,
        0x1056,0x0AE1,0x0AE0,0x07A2,0x0464,0x0232,0x8000,0x8000 }) },

    { "Chaos Echo", PS1ReverbRegs(std::array<uint16_t,32>{
        0x0001,0x0001,0x7FFF,0x7FFF,0x0000,0x0000,0x0000,0x8100,
        0x0000,0x0000,0x1FFF,0x0FFF,0x1005,0x0005,0x0000,0x0000,
        0x1005,0x0005,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x1004,0x1002,0x0004,0x0002,0x8000,0x8000 }) },

    { "Delay", PS1ReverbRegs(std::array<uint16_t,32>{
        0x0001,0x0001,0x7FFF,0x7FFF,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x1FFF,0x0FFF,0x1005,0x0005,0x0000,0x0000,
        0x1005,0x0005,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x1004,0x1002,0x0004,0x0002,0x8000,0x8000 }) },

    { "Reverb Off", PS1ReverbRegs(std::array<uint16_t,32>{
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
        0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
        0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0000,0x0000 }) }
};

// -----------------------------
// PS1ReverbFilter implementation
PS1ReverbFilter::PS1ReverbFilter(const std::string& presetName)
{
    auto it = PS1_CANONICAL_PRESETS.find(presetName);
    if (it == PS1_CANONICAL_PRESETS.end()) {
        // fall back to Hall if unknown
        mRegs = PS1_CANONICAL_PRESETS.at("Hall");
    }
    else {
        mRegs = it->second;
    }
    mWet = 0.5;
}

// -----------------------------
// PS1ReverbFilterInstance implementation
PS1ReverbFilterInstance::PS1ReverbFilterInstance(PS1ReverbFilter* aParent)
    : mParent(aParent)
{
    // lazy init of comb vectors will happen on first filter() call because we need sample-rate
}

// Initialize per-instance comb buffers based on current preset and samplerate
void PS1ReverbFilterInstance::initBuffers(float aSamplerate)
{
    const PS1ReverbRegs& r = mParent->mRegs;

    // map comb addresses to delays
    const uint16_t combAddrsL[4] = { r.mLCOMB1, r.mLCOMB2, r.mLCOMB3, r.mLCOMB4 };
    const uint16_t combAddrsR[4] = { r.mRCOMB1, r.mRCOMB2, r.mRCOMB3, r.mRCOMB4 };

    for (int c = 0; c < 4; ++c)
    {
        size_t lenL = addr_to_delay(combAddrsL[c]);
        size_t lenR = addr_to_delay(combAddrsR[c]);

        // allocate with +1 for safe modulo operations
        mLComb[c].assign(lenL + 1, 0.0f);
        mRComb[c].assign(lenR + 1, 0.0f);

        mLIndex[c] = 0;
        mRIndex[c] = 0;
    }

    // reset APF states
    mLAPF1State = mLAPF2State = mRAPF1State = mRAPF2State = 0.0f;
}

// The *exact* SoLoud signature the engine expects
void PS1ReverbFilterInstance::filter(float* aBuffer, unsigned int aSamples,
    unsigned int /*aBufferSize*/, unsigned int aChannels, float aSamplerate, SoLoud::time /*aTime*/)
{
    if (!aBuffer || aSamples == 0 || aChannels == 0) return;

    // Lazy init on first filter call (we need samplerate to compute lengths if we wanted sample-rate scaling)
    if (mLComb[0].empty())
        initBuffers(aSamplerate);

    const PS1ReverbRegs& r = mParent->mRegs;

    // Gains
    const float combGains[4] = {
        reg_to_gain(r.vCOMB1),
        reg_to_gain(r.vCOMB2),
        reg_to_gain(r.vCOMB3),
        reg_to_gain(r.vCOMB4)
    };
    const float apfGain1 = reg_to_gain(r.vAPF1);
    const float apfGain2 = reg_to_gain(r.vAPF2);
    const float wallGain = reg_to_gain(r.vWALL);
    const float outGainL = reg_to_gain(r.vLOUT);
    const float outGainR = reg_to_gain(r.vROUT);

    // Main sample loop
    for (unsigned int i = 0; i < aSamples; ++i)
    {
        const unsigned int baseIndex = i * aChannels;
        float inL = aBuffer[baseIndex + 0];
        float inR = (aChannels > 1) ? aBuffer[baseIndex + 1] : inL;

        // Comb stage
        float combOutL = 0.0f;
        float combOutR = 0.0f;

        for (int c = 0; c < 4; ++c)
        {
            // Left comb
            auto& bufL = mLComb[c];
            size_t idxL = mLIndex[c] % bufL.size();
            float delayedL = bufL[idxL];
            // write new sample: input + previous delayed * feedback
            float newL = inL + delayedL * combGains[c];
            bufL[idxL] = newL;
            combOutL += delayedL;
            mLIndex[c] = (idxL + 1) % bufL.size();

            // Right comb
            auto& bufR = mRComb[c];
            size_t idxR = mRIndex[c] % bufR.size();
            float delayedR = bufR[idxR];
            float newR = inR + delayedR * combGains[c];
            bufR[idxR] = newR;
            combOutR += delayedR;
            mRIndex[c] = (idxR + 1) % bufR.size();
        }

        // Wall/reflection scaling
        combOutL *= wallGain;
        combOutR *= wallGain;

        // APF stages (two scalar APFs per channel implemented via simple state form)
        // L channel
        float apf1_out_L = apfGain1 * combOutL + mLAPF1State;
        mLAPF1State = combOutL - apfGain1 * apf1_out_L;
        float apf2_out_L = apfGain2 * apf1_out_L + mLAPF2State;
        mLAPF2State = apf1_out_L - apfGain2 * apf2_out_L;

        // R channel
        float apf1_out_R = apfGain1 * combOutR + mRAPF1State;
        mRAPF1State = combOutR - apfGain1 * apf1_out_R;
        float apf2_out_R = apfGain2 * apf1_out_R + mRAPF2State;
        mRAPF2State = apf1_out_R - apfGain2 * apf2_out_R;

        // Mix wet/dry and apply output volumes
        float wet = mParent->mWet;
        float dry = 1.0f - wet;

        float outL = dry * inL + wet * apf2_out_L * outGainL;
        float outR = dry * inR + wet * apf2_out_R * outGainR;

        // clamp for SoLoud pipeline safety
        outL = std::clamp(outL, -1.0f, 1.0f);
        outR = std::clamp(outR, -1.0f, 1.0f);

        aBuffer[baseIndex + 0] = outL;
        if (aChannels > 1)
            aBuffer[baseIndex + 1] = outR;
    }
}
