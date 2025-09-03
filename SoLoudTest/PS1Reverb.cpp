//#include "PS1Reverb.h"
//#include <algorithm>
//#include <cmath>
//#include <stdexcept>
//
//// Helper: convert SPU addr/8 register to WORD offset (16-bit) inside work area
//static inline uint32_t reg_to_words(uint16_t reg) {
//    // SPU stores addresses divided by 8. Each word = 2 bytes. (8/2) = 4
//    return (uint32_t)reg * 4u;
//}
//
//// Helper clamp (portable; avoids std::clamp availability issues)
//template<typename T>
//static inline T clamp_val(T v, T lo, T hi) {
//    if (v < lo) return lo;
//    if (v > hi) return hi;
//    return v;
//}
//
//// ===================== Canonical Presets (PSXSPX) =====================
//// Values match the PSXSPX tables: volumes are s15, addresses are addr/8,
//// size is the "Reverb Work Area Size" (bytes). See PSXSPX 7.10. :contentReference[oaicite:3]{index=3}
//std::unordered_map<std::string, PS1ReverbRegs> PS1_CANONICAL_PRESETS = {
//    // Room
//    { "Room", PS1ReverbRegs(
//        0x5800, 0x5300, 0x0000,
//        0x04D6, 0x0333, 0x0334,
//        0x03F0, 0x0227, 0x0374, 0x01EF,
//        0x01B5,
//        0x03F0, 0x0227,
//        0x03F0, 0x0227,
//        0x03F0, 0x0000,  // mLCOMB1, mRCOMB1
//        0x0000, 0x0000,  // mLCOMB2, mRCOMB2
//        0x0374, 0x0000,  // mLCOMB3, mRCOMB3
//        0x01EF, 0x0000,  // mLCOMB4, mRCOMB4
//        0x0334, 0x01B5,  // mLDIFF, mRDIFF
//        0x0000, 0x0000,  // mLAPF1, mRAPF1 (not used in this preset table row)
//        0x0000, 0x0000,  // mLAPF2, mRAPF2
//        0x8000, 0x8000,
//        0x26C0
//    )}//,
//        //// Studio Small
//        //{ "Studio Small", PS1ReverbRegs(
//        //    0x5280, 0x4EC0, 0x0000,
//        //    0x03E4, 0x031B, 0x031C,
//        //    0x03A4, 0x02AF, 0x0372, 0x0266,
//        //    0x025D,
//        //    0x03A4, 0x02AF,
//        //    0x03A4, 0x02AF,
//        //    0x031C, 0x025C,
//        //    0x0372, 0x022F,
//        //    0x0266, 0x0135,
//        //    0x025D, 0x018E,
//        //    0x031C, 0x025D,
//        //    0x01D2, 0x00B7,
//        //    0x8000, 0x8000,
//        //    0x2620
//        //)},
//        //// Studio Medium
//        //{ "Studio Medium", PS1ReverbRegs(
//        //    0x5280, 0x4EC0, 0x0000,
//        //    0x0904, 0x076B, 0x076C,
//        //    0x0824, 0x065F, 0x07A2, 0x0616,
//        //    0x05ED,
//        //    0x0824, 0x065F,
//        //    0x0824, 0x065F,
//        //    0x076C, 0x05EC,
//        //    0x07A2, 0x050F,
//        //    0x0616, 0x0305,
//        //    0x05ED, 0x042E,
//        //    0x076C, 0x05ED,
//        //    0x0462, 0x02B7,
//        //    0x8000, 0x8000,
//        //    0x4F50
//        //)},
//        //// Studio Large
//        //{ "Studio Large", PS1ReverbRegs(
//        //    0x5680, 0x52C0, 0x0000,
//        //    0x0DFB, 0x0B58, 0x0B59,
//        //    0x0D09, 0x0A3C, 0x0BD9, 0x0973,
//        //    0x08DA,
//        //    0x0D09, 0x0A3C,
//        //    0x0D09, 0x0A3C,
//        //    0x0B59, 0x08D9,
//        //    0x0BD9, 0x07EC,
//        //    0x0973, 0x04B0,
//        //    0x08DA, 0x05E9,
//        //    0x0B59, 0x08DA,
//        //    0x06EF, 0x03D2,
//        //    0x8000, 0x8000,
//        //    0x64F0
//        //)},
//        //// Hall
//        //{ "Hall", PS1ReverbRegs(
//        //    0x6000, 0x5C00, 0x0000,
//        //    0x15BA, 0x11BB, 0x11C0,
//        //    0x14C2, 0x10BD, 0x11BC, 0x0DC1,
//        //    0x0DC3,
//        //    0x14C2, 0x10BD,
//        //    0x14C2, 0x10BD,
//        //    0x11C0, 0x0DC0,
//        //    0x11BC, 0x0BC4,
//        //    0x0DC1, 0x07C1,
//        //    0x0DC3, 0x09C1,
//        //    0x11C0, 0x0DC3,
//        //    0x0A00, 0x06CD,
//        //    0x8000, 0x8000,
//        //    0x7400
//        //)},
//        //// Half Echo
//        //{ "Half Echo", PS1ReverbRegs(
//        //    0x5F80, 0x54C0, 0x0000,
//        //    0x0371, 0x02AF, 0x0358,
//        //    0x02E5, 0x01DF, 0x02B0, 0x01D7,
//        //    0x026A,
//        //    0x02E5, 0x01DF,
//        //    0x02E5, 0x01DF,
//        //    0x0358, 0x01D6,
//        //    0x02B0, 0x012D,
//        //    0x01D7, 0x00B1,
//        //    0x026A, 0x011E,
//        //    0x0358, 0x026A,
//        //    0x011F, 0x0059,
//        //    0x8000, 0x8000,
//        //    0x2180
//        //)},
//        //// Space Echo
//        //{ "Space Echo", PS1ReverbRegs(
//        //    0x6000, 0x5400, 0x0000,
//        //    0x1ED6, 0x1A31, 0x1A32,
//        //    0x1D14, 0x183B, 0x1BC2, 0x16B2,
//        //    0x15EF,
//        //    0x1D14, 0x183B,
//        //    0x1D14, 0x183B,
//        //    0x1A32, 0x15EE,
//        //    0x1BC2, 0x1334,
//        //    0x16B2, 0x0F2D,
//        //    0x15EF, 0x1055,
//        //    0x1A32, 0x15EF,
//        //    0x11F6, 0x0C5D,
//        //    0x8000, 0x8000,
//        //    0x8B60
//        //)},
//        //// Chaos Echo
//        //{ "Chaos Echo", PS1ReverbRegs(
//        //    0x0000, 0x0000, 0x8100, // base is non-zero here per table
//        //    0x1FFF, 0x0FFF, 0x1005,
//        //    0x1005, 0x0005, 0x0000, 0x0000,
//        //    0x0000,
//        //    0x1005, 0x0005,
//        //    0x1005, 0x0005,
//        //    0x1005, 0x0000,
//        //    0x0000, 0x0000,
//        //    0x0000, 0x0000,
//        //    0x0000, 0x0000,
//        //    0x1005, 0x0005,
//        //    0x1004, 0x1002,
//        //    0x8000, 0x8000,
//        //    0x3400
//        //)},
//        //// Delay
//        //{ "Delay", PS1ReverbRegs(
//        //    0x0000, 0x0000, 0x0000,
//        //    0x1FFF, 0x0FFF, 0x1005,
//        //    0x1005, 0x0005, 0x0000, 0x0000,
//        //    0x0000,
//        //    0x1005, 0x0005,
//        //    0x1005, 0x0005,
//        //    0x1005, 0x0000,
//        //    0x0000, 0x0000,
//        //    0x0000, 0x0000,
//        //    0x0000, 0x0000,
//        //    0x1005, 0x0005,
//        //    0x1004, 0x1002,
//        //    0x8000, 0x8000,
//        //    0x3400
//        //)},
//        //// Reverb Off  (IMPORTANT: non-zero size & offsets)
//        //{ "Reverb Off", PS1ReverbRegs(
//        //    0x0000, 0x0000, 0x0001,
//        //    0x0001, 0x0001, 0x0001,
//        //    0x0001, 0x0001, 0x0001, 0x0001,
//        //    0x0001,
//        //    0x0001, 0x0001,
//        //    0x0001, 0x0001,
//        //    0x0001, 0x0001,
//        //    0x0001, 0x0001,
//        //    0x0001, 0x0001,
//        //    0x0001, 0x0001,
//        //    0x0001, 0x0001,
//        //    0x0001, 0x0001,
//        //    0x8000, 0x8000,
//        //    0x0010
//        //)}
//};
//
//// ===================== Filter =====================
//
//PS1ReverbFilter::PS1ReverbFilter(const std::string& presetName)
//{
//    auto it = PS1_CANONICAL_PRESETS.find(presetName);
//    if (it == PS1_CANONICAL_PRESETS.end())
//        throw std::runtime_error("Unknown PS1 reverb preset: " + presetName);
//    mRegs = it->second;
//}
//
//SoLoud::FilterInstance* PS1ReverbFilter::createInstance()
//{
//    return new PS1ReverbFilterInstance(this);
//}
//
//PS1ReverbFilterInstance::PS1ReverbFilterInstance(PS1ReverbFilter* parent)
//    : mParent(parent)
//{
//    // Allocate work area
//    mOfs.sizeWords = std::max<uint32_t>(8u, mParent->mRegs.regionSizeBytes / 2u);
//    mRvb.assign(mOfs.sizeWords, 0);
//
//    // Convert address/offset regs to word offsets
//    mOfs.base = reg_to_words(mParent->mRegs.mBASE);
//    mOfs.dAPF1 = reg_to_words(mParent->mRegs.dAPF1);
//    mOfs.dAPF2 = reg_to_words(mParent->mRegs.dAPF2);
//
//    mOfs.LSAME = reg_to_words(mParent->mRegs.mLSAME);
//    mOfs.RSAME = reg_to_words(mParent->mRegs.mRSAME);
//    mOfs.LCOMB1 = reg_to_words(mParent->mRegs.mLCOMB1);
//    mOfs.RCOMB1 = reg_to_words(mParent->mRegs.mRCOMB1);
//    mOfs.LCOMB2 = reg_to_words(mParent->mRegs.mLCOMB2);
//    mOfs.RCOMB2 = reg_to_words(mParent->mRegs.mRCOMB2);
//    mOfs.LCOMB3 = reg_to_words(mParent->mRegs.mLCOMB3);
//    mOfs.RCOMB3 = reg_to_words(mParent->mRegs.mRCOMB3);
//    mOfs.LCOMB4 = reg_to_words(mParent->mRegs.mLCOMB4);
//    mOfs.RCOMB4 = reg_to_words(mParent->mRegs.mRCOMB4);
//    mOfs.LDIFF = reg_to_words(mParent->mRegs.mLDIFF);
//    mOfs.RDIFF = reg_to_words(mParent->mRegs.mRDIFF);
//    mOfs.LAPF1 = reg_to_words(mParent->mRegs.mLAPF1);
//    mOfs.RAPF1 = reg_to_words(mParent->mRegs.mRAPF1);
//    mOfs.LAPF2 = reg_to_words(mParent->mRegs.mLAPF2);
//    mOfs.RAPF2 = reg_to_words(mParent->mRegs.mRAPF2);
//}
//
//// Core equations per PSXSPX, done at 16-bit with s15 gains. :contentReference[oaicite:4]{index=4}
//void PS1ReverbFilterInstance::filter(float* aBuffer, unsigned int aSamples,
//    unsigned int, unsigned int aChannels, float, SoLoud::time)
//{
//    const auto& r = mParent->mRegs;
//
//    const int32_t vLIN = (int16_t)r.vLIN;
//    const int32_t vRIN = (int16_t)r.vRIN;
//    const int32_t vLOUT = (int16_t)r.vLOUT;
//    const int32_t vROUT = (int16_t)r.vROUT;
//
//    const int32_t vIIR = (int16_t)r.vIIR;
//    const int32_t vWALL = (int16_t)r.vWALL;
//    const int32_t vAPF1 = (int16_t)r.vAPF1;
//    const int32_t vAPF2 = (int16_t)r.vAPF2;
//    const int32_t vC1 = (int16_t)r.vCOMB1;
//    const int32_t vC2 = (int16_t)r.vCOMB2;
//    const int32_t vC3 = (int16_t)r.vCOMB3;
//    const int32_t vC4 = (int16_t)r.vCOMB4;
//
//    const uint32_t d1 = mOfs.dAPF1;
//    const uint32_t d2 = mOfs.dAPF2;
//
//    for (unsigned int i = 0; i < aSamples; ++i)
//    {
//        // 1) Read dry input, convert to s16
//        float inL_f = aBuffer[i * aChannels + 0];
//        float inR_f = aBuffer[i * aChannels + 1];
//        int32_t inL = (int32_t)std::lround(clamp_val(inL_f, -1.0f, 1.0f) * 32767.0f);
//        int32_t inR = (int32_t)std::lround(clamp_val(inR_f, -1.0f, 1.0f) * 32767.0f);
//
//        // Helpers for memory
//        auto R = [&](uint32_t off)->int32_t { return (int16_t)mRvb[addr(off)]; };
//        auto W = [&](uint32_t off, int32_t v) { mRvb[addr(off)] = clamp16(v); };
//
//        // 2) Same-side reflections (IIR)
//        // [mLSAME] = (Lin + [dLSAME]*vWALL - [mLSAME-2]) * vIIR + [mLSAME-2]
//        {
//            int32_t prev = R(mOfs.LSAME + mOfs.sizeWords - 1); // (-2 bytes) == (-1 word)
//            int32_t term = inL; term = mul_s15(term, vLIN);
//            int32_t wall = mul_s15(R(mOfs.LSAME + mOfs.LDIFF), vWALL); // [dLSAME] share mLDIFF as offset base in table layouts
//            int32_t x = mul_s15((term + wall - prev), vIIR) + prev;
//            W(mOfs.LSAME, x);
//        }
//        {
//            int32_t prev = R(mOfs.RSAME + mOfs.sizeWords - 1);
//            int32_t term = inR; term = mul_s15(term, vRIN);
//            int32_t wall = mul_s15(R(mOfs.RSAME + mOfs.RDIFF), vWALL);
//            int32_t x = mul_s15((term + wall - prev), vIIR) + prev;
//            W(mOfs.RSAME, x);
//        }
//
//        // 3) Different-side reflections
//        // [mLDIFF] = Rin * vRIN + [mRDIFF]
//        // [mRDIFF] = Lin * vLIN + [mLDIFF]
//        int32_t LDIFFv = mul_s15(inR, vRIN) + R(mOfs.RDIFF);
//        int32_t RDIFFv = mul_s15(inL, vLIN) + R(mOfs.LDIFF);
//        W(mOfs.LDIFF, LDIFFv);
//        W(mOfs.RDIFF, RDIFFv);
//
//        // 4) SumHalf
//        int32_t SumHalfL = R(mOfs.LSAME) + mul_s15(R(mOfs.LDIFF), vRIN);
//        int32_t SumHalfR = R(mOfs.RSAME) + mul_s15(R(mOfs.RDIFF), vLIN);
//
//        // 5) Combine -> write Comb buffers with simple feedback via [-1 word]
//        W(mOfs.LCOMB1, SumHalfL + mul_s15(R(mOfs.LCOMB3), vC3) + R(mOfs.LCOMB1 + mOfs.sizeWords - 1));
//        W(mOfs.RCOMB1, SumHalfR + mul_s15(R(mOfs.RCOMB3), vC3) + R(mOfs.RCOMB1 + mOfs.sizeWords - 1));
//
//        // 6) Comb -> APF1
//        W(mOfs.LAPF1, mul_s15(R(mOfs.LCOMB2), vC2) + R(mOfs.LAPF1 + mOfs.sizeWords - 1));
//        W(mOfs.RAPF1, mul_s15(R(mOfs.RCOMB4), vC4) + R(mOfs.RAPF1 + mOfs.sizeWords - 1));
//
//        // 7) APF (write APF2)
//        W(mOfs.LAPF2,
//            mul_s15(R(mOfs.LAPF1 + mOfs.sizeWords - d1), vAPF1) +
//            mul_s15(R(mOfs.LAPF2 + mOfs.sizeWords - d2), vAPF2) +
//            mul_s15(R(mOfs.LCOMB1), vC1) +
//            R(mOfs.LAPF2 + mOfs.sizeWords - 1));
//
//        W(mOfs.RAPF2,
//            mul_s15(R(mOfs.RAPF1 + mOfs.sizeWords - d1), vAPF1) +
//            mul_s15(R(mOfs.RAPF2 + mOfs.sizeWords - d2), vAPF2) +
//            mul_s15(R(mOfs.RCOMB2), vC2) +
//            R(mOfs.RAPF2 + mOfs.sizeWords - 1));
//
//        // 8) Output
//        int32_t Lout = mul_s15(R(mOfs.LAPF2 + mOfs.sizeWords - d2), vLOUT) + mul_s15(R(mOfs.RSAME), vRIN);
//        int32_t Rout = mul_s15(R(mOfs.RAPF2 + mOfs.sizeWords - d2), vROUT) + mul_s15(R(mOfs.LSAME), vLIN);
//
//        // Mix wet/dry (convert back to float)
//        float wetL = (float)clamp_val(Lout, -32768, 32767) / 32768.0f;
//        float wetR = (float)clamp_val(Rout, -32768, 32767) / 32768.0f;
//
//        float dryMix = 1.0f - mParent->mWet;
//        float wetMix = mParent->mWet;
//        aBuffer[i * aChannels + 0] = inL_f * dryMix + wetL * wetMix;
//        aBuffer[i * aChannels + 1] = inR_f * dryMix + wetR * wetMix;
//
//        // Advance the SPU "buffer address" by one word (per sample step)
//        mCur = wrap(mCur + 1);
//    }
//}
