//#pragma once
//
//#include <soloud.h>
//#include <soloud_filter.h>
//#include <cstdint>
//#include <string>
//#include <vector>
//#include <unordered_map>
//
//// ---------------------------
//// PS1 reverb register block
//// (names follow PSXSPX, units/meaning per spec)
//// All src/dst/disp/base regs are SPU RAM addresses divided by 8 (addr/8).
//// Volumes are s15 fixed-point (0x8000 == +1.0, 0x0000 == 0).
//// Size is bytes of the reverb work area.
//struct PS1ReverbRegs
//{
//    uint16_t vLOUT, vROUT;   // output volumes
//    uint16_t mBASE;          // work area base (addr/8)
//    uint16_t dAPF1, dAPF2;   // APF displacements (addr/8)
//    uint16_t vIIR;           // reflection (IIR) volume
//    uint16_t vCOMB1, vCOMB2, vCOMB3, vCOMB4; // comb volumes
//    uint16_t vWALL;          // reflection 2 (WALL) volume
//    uint16_t vAPF1, vAPF2;   // APF volumes
//    // Same-side reflection addresses (L/R)
//    uint16_t mLSAME, mRSAME;
//    // Comb read/write addresses (L/R)
//    uint16_t mLCOMB1, mRCOMB1;
//    uint16_t mLCOMB2, mRCOMB2;
//    uint16_t mLCOMB3, mRCOMB3;
//    uint16_t mLCOMB4, mRCOMB4;
//    // Different-side reflect addresses (L/R)
//    uint16_t mLDIFF, mRDIFF;
//    // APF addresses (L/R)
//    uint16_t mLAPF1, mRAPF1;
//    uint16_t mLAPF2, mRAPF2;
//    // Input volumes
//    uint16_t vLIN, vRIN;
//    // Size (bytes) of work area
//    uint32_t regionSizeBytes;
//
//    PS1ReverbRegs() = default;
//
//    PS1ReverbRegs(
//        uint16_t vLOUT_, uint16_t vROUT_,
//        uint16_t mBASE_,
//        uint16_t dAPF1_, uint16_t dAPF2_,
//        uint16_t vIIR_,
//        uint16_t vCOMB1_, uint16_t vCOMB2_, uint16_t vCOMB3_, uint16_t vCOMB4_,
//        uint16_t vWALL_,
//        uint16_t vAPF1_, uint16_t vAPF2_,
//        uint16_t mLSAME_, uint16_t mRSAME_,
//        uint16_t mLCOMB1_, uint16_t mRCOMB1_,
//        uint16_t mLCOMB2_, uint16_t mRCOMB2_,
//        uint16_t mLCOMB3_, uint16_t mRCOMB3_,
//        uint16_t mLCOMB4_, uint16_t mRCOMB4_,
//        uint16_t mLDIFF_, uint16_t mRDIFF_,
//        uint16_t mLAPF1_, uint16_t mRAPF1_,
//        uint16_t mLAPF2_, uint16_t mRAPF2_,
//        uint16_t vLIN_, uint16_t vRIN_,
//        uint32_t regionSizeBytes_
//    )
//        : vLOUT(vLOUT_), vROUT(vROUT_), mBASE(mBASE_),
//        dAPF1(dAPF1_), dAPF2(dAPF2_), vIIR(vIIR_),
//        vCOMB1(vCOMB1_), vCOMB2(vCOMB2_), vCOMB3(vCOMB3_), vCOMB4(vCOMB4_),
//        vWALL(vWALL_), vAPF1(vAPF1_), vAPF2(vAPF2_),
//        mLSAME(mLSAME_), mRSAME(mRSAME_),
//        mLCOMB1(mLCOMB1_), mRCOMB1(mRCOMB1_),
//        mLCOMB2(mLCOMB2_), mRCOMB2(mRCOMB2_),
//        mLCOMB3(mLCOMB3_), mRCOMB3(mRCOMB3_),
//        mLCOMB4(mLCOMB4_), mRCOMB4(mRCOMB4_),
//        mLDIFF(mLDIFF_), mRDIFF(mRDIFF_),
//        mLAPF1(mLAPF1_), mRAPF1(mRAPF1_), mLAPF2(mLAPF2_), mRAPF2(mRAPF2_),
//        vLIN(vLIN_), vRIN(vRIN_),
//        regionSizeBytes(regionSizeBytes_) {
//    }
//};
//
//// Canonical presets (accurate to PSXSPX)
//extern std::unordered_map<std::string, PS1ReverbRegs> PS1_CANONICAL_PRESETS;
//
//class PS1ReverbFilter;
//
//// ---------------------------------
//class PS1ReverbFilterInstance : public SoLoud::FilterInstance
//{
//public:
//    explicit PS1ReverbFilterInstance(PS1ReverbFilter* parent);
//    void filter(float* buf, unsigned int samples, unsigned int bufSize,
//        unsigned int channels, float samplerate, SoLoud::time time) override;
//
//private:
//    PS1ReverbFilter* mParent;
//
//    // Reverb work area (16-bit words), circular
//    std::vector<int16_t> mRvb;   // size = regionSizeBytes/2 words
//    uint32_t mCur = 0;           // current write pointer (word index)
//
//    // Cached word offsets (converted from regs: addr/8 -> words == *4)
//    struct Offsets {
//        uint32_t base;
//        uint32_t dAPF1, dAPF2;
//        uint32_t LSAME, RSAME;
//        uint32_t LCOMB1, RCOMB1;
//        uint32_t LCOMB2, RCOMB2;
//        uint32_t LCOMB3, RCOMB3;
//        uint32_t LCOMB4, RCOMB4;
//        uint32_t LDIFF, RDIFF;
//        uint32_t LAPF1, RAPF1;
//        uint32_t LAPF2, RAPF2;
//        uint32_t sizeWords;
//    } mOfs;
//
//    inline uint32_t wrap(uint32_t w) const { return (w % mOfs.sizeWords); }
//    inline uint32_t addr(uint32_t off) const { return wrap(mOfs.base + mCur + off); }
//
//    static inline int32_t mul_s15(int32_t a, int32_t b) { // (a * b) >> 15 with sign
//        return (int32_t)(((int64_t)a * (int64_t)b) >> 15);
//    }
//    static inline int16_t clamp16(int32_t v) {
//        if (v > 32767) return 32767;
//        if (v < -32768) return -32768;
//        return (int16_t)v;
//    }
//};
//
//// ---------------------------------
//class PS1ReverbFilter : public SoLoud::Filter
//{
//public:
//    // Construct with a preset name (e.g., "Hall")
//    explicit PS1ReverbFilter(const std::string& presetName = "Hall");
//
//    SoLoud::FilterInstance* createInstance() override;
//
//    float mWet = 1.0f;         // wet/dry mix
//    PS1ReverbRegs mRegs;
//};
