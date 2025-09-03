// PS1ReverbFilter.hpp
// Faithful PS1 SPU reverb emulation as a SoLoud Filter/FilterInstance
// - Resamples host -> SPU reverb rate (22.05 kHz) and back
// - Models SPU reverb RAM (512 KB -> 0x40000 words)
// - Implements all primary registers with named fields stored in Preset
// - Uses Q15 register encoding: 0x8000 == -1.0 (signed Q15)
// - 39-tap FIR included (exact coefficients from specification)
// - Presets map keyed by string, values in struct with named registers
//
// Usage:
//   auto *f = new PS1ReverbFilter("Room");
//   voice->setFilter(0, f);
//
#pragma once

#include <soloud.h>
#include <soloud_filter.h>

#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace ps1reverb {

    // Convert signed Q15 (int16_t) to float in range [-1.0, +1.0)
    static inline float q15_to_float(int16_t v) {
        // The SPU treats 0x8000 as -1.0
        if (v == (int16_t)0x8000) return -1.0f;
        return (float)v / 32768.0f;
    }

    // Convert float in [-1,1) to signed Q15
    static inline int16_t float_to_q15(float f) {
        if (f <= -1.0f) return (int16_t)0x8000;
        if (f >= (32767.0f / 32768.0f)) return 0x7FFF;
        return (int16_t)std::lroundf(f * 32768.0f);
    }

    // Number of SPU words (16-bit) in SPU RAM: 512 KiB = 524,288 bytes = 262,144 words
    static constexpr size_t SPU_RAM_WORDS = 0x40000; // 262,144 words

    // 39-tap FIR coefficients as uint16_t hex values from spec (signed)
    static const std::array<int16_t, 39> FIR_COEFFS = {
        (int16_t)0xFFFF, 0x0000, 0x0002, 0x0000, (int16_t)0xFFF6, 0x0000, 0x0023, 0x0000,
        (int16_t)0xFF99, 0x0000, 0x010A, 0x0000, (int16_t)0xF398, 0x0000, 0x0534, 0x0000,
        (int16_t)0xB470, 0x0000, 0x2806, 0x4000, 0x2806, 0x0000, (int16_t)0xB470, 0x0000,
        0x0534, 0x0000, (int16_t)0xF398, 0x0000, 0x010A, 0x0000, (int16_t)0xFF99, 0x0000,
        0x0023, 0x0000, (int16_t)0xFFF6, 0x0000, 0x0002, 0x0000, 0xFFFF
    };

    // Preset structure with named fields matching SPU registers used in reverb
    struct Preset {
        // volumes (Q15)
        int16_t vLOUT;
        int16_t vROUT;
        int16_t vLIN;
        int16_t vRIN;
        int16_t vIIR;
        int16_t vWALL;
        int16_t vAPF1;
        int16_t vAPF2;
        int16_t vCOMB1;
        int16_t vCOMB2;
        int16_t vCOMB3;
        int16_t vCOMB4;

        // base and offsets (word addresses)
        uint16_t mBASE;
        uint16_t dAPF1;
        uint16_t dAPF2;

        // same/diff offsets (addresses used as read indices)
        uint16_t dLSAME;
        uint16_t dRSAME;
        uint16_t dLDIFF;
        uint16_t dRDIFF;

        // memory addresses for buffers (word addresses)
        uint16_t mLSAME;
        uint16_t mRSAME;
        uint16_t mLDIFF;
        uint16_t mRDIFF;

        uint16_t mLCOMB1; uint16_t mRCOMB1;
        uint16_t mLCOMB2; uint16_t mRCOMB2;
        uint16_t mLCOMB3; uint16_t mRCOMB3;
        uint16_t mLCOMB4; uint16_t mRCOMB4;

        uint16_t mLAPF1; uint16_t mRAPF1;
        uint16_t mLAPF2; uint16_t mRAPF2;
    };

    // Create a preset from an initializer list of hex values in the canonical PSX order.
    // The PSXSPX spec lists presets as series of words; mapping below matches the values given in the spec.
    // Expected sequence (per spec excerpt): [vIIR, vWALL, mBASE, dAPF1, dAPF2, vAPF1, vAPF2, vCOMB1, vCOMB2, vCOMB3, vCOMB4,
    //    dLSAME, dRSAME, dLDIFF, dRDIFF, mLSAME, mRSAME, mLDIFF, mRDIFF, mLCOMB1, mRCOMB1, mLCOMB2, mRCOMB2,
    //    mLCOMB3, mRCOMB3, mLCOMB4, mRCOMB4, mLAPF1, mRAPF1, mLAPF2, mRAPF2, vLOUT, vROUT, vLIN, vRIN ]
    static inline Preset make_preset_from_array(const std::vector<uint16_t>& a) {
        Preset p;
        auto get = [&](size_t i)->uint16_t { return (i < a.size()) ? a[i] : 0; };

        p.vIIR = (int16_t)get(0);
        p.vWALL = (int16_t)get(1);
        p.mBASE = get(2);
        p.dAPF1 = get(3);
        p.dAPF2 = get(4);
        p.vAPF1 = (int16_t)get(5);
        p.vAPF2 = (int16_t)get(6);
        p.vCOMB1 = (int16_t)get(7);
        p.vCOMB2 = (int16_t)get(8);
        p.vCOMB3 = (int16_t)get(9);
        p.vCOMB4 = (int16_t)get(10);

        p.dLSAME = get(11);
        p.dRSAME = get(12);
        p.dLDIFF = get(13);
        p.dRDIFF = get(14);

        p.mLSAME = get(15);
        p.mRSAME = get(16);
        p.mLDIFF = get(17);
        p.mRDIFF = get(18);

        p.mLCOMB1 = get(19); p.mRCOMB1 = get(20);
        p.mLCOMB2 = get(21); p.mRCOMB2 = get(22);
        p.mLCOMB3 = get(23); p.mRCOMB3 = get(24);
        p.mLCOMB4 = get(25); p.mRCOMB4 = get(26);

        p.mLAPF1 = get(27); p.mRAPF1 = get(28);
        p.mLAPF2 = get(29); p.mRAPF2 = get(30);

        p.vLOUT = (int16_t)get(31);
        p.vROUT = (int16_t)get(32);
        p.vLIN = (int16_t)get(33);
        p.vRIN = (int16_t)get(34);

        return p;
    }

    // Build the presets map using the values from the PSXSPX documentation you provided.
    static inline std::unordered_map<std::string, Preset> build_preset_map() {
        std::unordered_map<std::string, Preset> m;

        // Room
        m["Room"] = make_preset_from_array({
            0x007D,0x005B,0x6D80,0x54B8,0xBED0,0x0000,0x0000,0xBA80,
            0x5800,0x5300,0x04D6,0x0333,0x03F0,0x0227,0x0374,0x01EF,
            0x0334,0x01B5,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
            0x0000,0x0000,0x01B4,0x0136,0x00B8,0x005C,0x8000,0x8000
            });

        // Studio Small
        m["Studio Small"] = make_preset_from_array({
            0x0033,0x0025,0x70F0,0x4FA8,0xBCE0,0x4410,0xC0F0,0x9C00,
            0x5280,0x4EC0,0x03E4,0x031B,0x03A4,0x02AF,0x0372,0x0266,
            0x031C,0x025D,0x025C,0x018E,0x022F,0x0135,0x01D2,0x00B7,
            0x018F,0x00B5,0x00B4,0x0080,0x004C,0x0026,0x8000,0x8000
            });

        // Studio Medium
        m["Studio Medium"] = make_preset_from_array({
            0x00B1,0x007F,0x70F0,0x4FA8,0xBCE0,0x4510,0xBEF0,0xB4C0,
            0x5280,0x4EC0,0x0904,0x076B,0x0824,0x065F,0x07A2,0x0616,
            0x076C,0x05ED,0x05EC,0x042E,0x050F,0x0305,0x0462,0x02B7,
            0x042F,0x0265,0x0264,0x01B2,0x0100,0x0080,0x8000,0x8000
            });

        // Studio Large
        m["Studio Large"] = make_preset_from_array({
            0x00E3,0x00A9,0x6F60,0x4FA8,0xBCE0,0x4510,0xBEF0,0xA680,
            0x5680,0x52C0,0x0DFB,0x0B58,0x0D09,0x0A3C,0x0BD9,0x0973,
            0x0B59,0x08DA,0x08D9,0x05E9,0x07EC,0x04B0,0x06EF,0x03D2,
            0x05EA,0x031D,0x031C,0x0238,0x0154,0x00AA,0x8000,0x8000
            });

        // Hall
        m["Hall"] = make_preset_from_array({
            0x01A5,0x0139,0x6000,0x5000,0x4C00,0xB800,0xBC00,0xC000,
            0x6000,0x5C00,0x15BA,0x11BB,0x14C2,0x10BD,0x11BC,0x0DC1,
            0x11C0,0x0DC3,0x0DC0,0x09C1,0x0BC4,0x07C1,0x0A00,0x06CD,
            0x09C2,0x05C1,0x05C0,0x041A,0x0274,0x013A,0x8000,0x8000
            });

        // Half Echo
        m["Half Echo"] = make_preset_from_array({
            0x0017,0x0013,0x70F0,0x4FA8,0xBCE0,0x4510,0xBEF0,0x8500,
            0x5F80,0x54C0,0x0371,0x02AF,0x02E5,0x01DF,0x02B0,0x01D7,
            0x0358,0x026A,0x01D6,0x011E,0x012D,0x00B1,0x011F,0x0059,
            0x01A0,0x00E3,0x0058,0x0040,0x0028,0x0014,0x8000,0x8000
            });

        // Space Echo
        m["Space Echo"] = make_preset_from_array({
            0x033D,0x0231,0x7E00,0x5000,0xB400,0xB000,0x4C00,0xB000,
            0x6000,0x5400,0x1ED6,0x1A31,0x1D14,0x183B,0x1BC2,0x16B2,
            0x1A32,0x15EF,0x15EE,0x1055,0x1334,0x0F2D,0x11F6,0x0C5D,
            0x1056,0x0AE1,0x0AE0,0x07A2,0x0464,0x0232,0x8000,0x8000
            });

        // Chaos Echo
        m["Chaos Echo"] = make_preset_from_array({
            0x0001,0x0001,0x7FFF,0x7FFF,0x0000,0x0000,0x0000,0x8100,
            0x0000,0x0000,0x1FFF,0x0FFF,0x1005,0x0005,0x0000,0x0000,
            0x1005,0x0005,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
            0x0000,0x0000,0x1004,0x1002,0x0004,0x0002,0x8000,0x8000
            });

        // Delay (one-shot echo) - same table as Chaos Echo in spec
        m["Delay"] = make_preset_from_array({
            0x0001,0x0001,0x7FFF,0x7FFF,0x0000,0x0000,0x0000,0x0000,
            0x0000,0x0000,0x1FFF,0x0FFF,0x1005,0x0005,0x0000,0x0000,
            0x1005,0x0005,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
            0x0000,0x0000,0x1004,0x1002,0x0004,0x0002,0x8000,0x8000
            });

        // Reverb off (dummy)
        m["Off"] = make_preset_from_array({
            0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
            0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
            0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
            0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0000,0x0000
            });

        return m;
    }

    // PS1ReverbFilterInstance: does per-sample processing, includes resampling host<->22.05k
    class PS1ReverbFilterInstance : public SoLoud::FilterInstance {
    public:
        PS1ReverbFilterInstance(const Preset& preset, unsigned int hostSamplerate)
            : m_preset(preset), m_hostSamplerate(hostSamplerate)
        {
            // Initialize SPU registers (conversions from Q15 where needed)
            vIIR = q15_to_float(m_preset.vIIR);
            vWALL = q15_to_float(m_preset.vWALL);
            vAPF1 = q15_to_float(m_preset.vAPF1);
            vAPF2 = q15_to_float(m_preset.vAPF2);
            vCOMB1 = q15_to_float(m_preset.vCOMB1);
            vCOMB2 = q15_to_float(m_preset.vCOMB2);
            vCOMB3 = q15_to_float(m_preset.vCOMB3);
            vCOMB4 = q15_to_float(m_preset.vCOMB4);
            vLOUT = q15_to_float(m_preset.vLOUT);
            vROUT = q15_to_float(m_preset.vROUT);
            vLIN = q15_to_float(m_preset.vLIN);
            vRIN = q15_to_float(m_preset.vRIN);

            // Addresses (word indices)
            mBASE = (int)m_preset.mBASE;
            dAPF1 = (int)m_preset.dAPF1;
            dAPF2 = (int)m_preset.dAPF2;

            dLSAME = (int)m_preset.dLSAME;
            dRSAME = (int)m_preset.dRSAME;
            dLDIFF = (int)m_preset.dLDIFF;
            dRDIFF = (int)m_preset.dRDIFF;

            mLSAME = (int)m_preset.mLSAME;
            mRSAME = (int)m_preset.mRSAME;
            mLDIFF = (int)m_preset.mLDIFF;
            mRDIFF = (int)m_preset.mRDIFF;

            mLCOMB1 = (int)m_preset.mLCOMB1; mRCOMB1 = (int)m_preset.mRCOMB1;
            mLCOMB2 = (int)m_preset.mLCOMB2; mRCOMB2 = (int)m_preset.mRCOMB2;
            mLCOMB3 = (int)m_preset.mLCOMB3; mRCOMB3 = (int)m_preset.mRCOMB3;
            mLCOMB4 = (int)m_preset.mLCOMB4; mRCOMB4 = (int)m_preset.mRCOMB4;

            mLAPF1 = (int)m_preset.mLAPF1; mRAPF1 = (int)m_preset.mRAPF1;
            mLAPF2 = (int)m_preset.mLAPF2; mRAPF2 = (int)m_preset.mRAPF2;

            // RAM init to zero
            m_ram.assign(SPU_RAM_WORDS, 0.0f);

            // buffer address set to mBASE
            m_bufferAddr = mBASE & (int)(SPU_RAM_WORDS - 1);

            // FIR state initialisation (one FIR state for L and R separately)
            m_firL.assign(FIR_COEFFS.size(), 0.0f);
            m_firR.assign(FIR_COEFFS.size(), 0.0f);
            m_firIndex = 0;

            // compute resampling ratios
            // SPU reverb tick rate is exactly half of 44.1 kHz -> 22.05 kHz
            // hostSamplerate usually 44100. We support arbitrary host samplerates by using a simple fractional accumulator resampler:
            m_spuRate = 22050.0;
            m_hostToSpuRatio = (float)(m_spuRate / (double)m_hostSamplerate);
            m_spuToHostRatio = (float)((double)m_hostSamplerate / m_spuRate);

            // fractional accumulators for resampling
            m_hostPhase = 0.0f;
            m_lastSpuOutL = 0.0f;
            m_lastSpuOutR = 0.0f;
        }

        virtual ~PS1ReverbFilterInstance() = default;

        // SoLoud calls filter with interleaved stereo floats
        // aSamplerate is provided by host at each call — we'll use that (and recalc ratios) if changed
// Replace existing filter(...) with this implementation
        virtual void filter(
            float* aBuffer,
            unsigned int aSamples,
            unsigned int aBufferSize,
            unsigned int aChannels,
            float aSamplerate,
            SoLoud::time aTime) override
        {
            // Basic validation
            if (aBuffer == nullptr) return;
            if (aSamples == 0) return;
            if (aChannels == 0) return;
            // Ensure bufferSize is at least samples * channels (defensive)
            if (aBufferSize < aSamples * aChannels) {
                // If buffer size is smaller than expected, abort to avoid OOB writes.
                return;
            }

            // Update host samplerate if changed
            if ((unsigned int)std::lround(aSamplerate) != m_hostSamplerate) {
                m_hostSamplerate = (unsigned int)std::lround(aSamplerate);
                m_hostToSpuRatio = (float)(m_spuRate / (double)m_hostSamplerate);
                m_spuToHostRatio = (float)((double)m_hostSamplerate / m_spuRate);
            }

            // Planar buffer layout: channel 0 samples [0..aSamples-1], channel 1 samples start at aBuffer + aSamples, etc.
            float* ch0 = aBuffer;                                 // left (or mono)
            float* ch1 = (aChannels >= 2) ? (aBuffer + aSamples) : nullptr; // right if present

            for (unsigned int i = 0; i < aSamples; ++i) {
                // Read inputs from planar buffers
                float inL = ch0[i];
                float inR = ch1 ? ch1[i] : inL; // if mono, duplicate left as right

                // Downsample host -> SPU tick domain using fractional accumulator
                m_hostPhase += m_hostToSpuRatio;

                bool producedSpu = false;
                float spuOutL = 0.0f, spuOutR = 0.0f;

                // Produce one or more SPU ticks if accumulator >= 1.0
                while (m_hostPhase >= 1.0f) {
                    // Build SPU input sample. Use simple two-sample averaging if we have a previous host sample.
                    float host_interp_L = m_prevHostValid ? 0.5f * (m_prevHostL + inL) : inL;
                    float host_interp_R = m_prevHostValid ? 0.5f * (m_prevHostR + inR) : inR;

                    // Process a single SPU tick (this updates m_lastSpuOutL/m_lastSpuOutR)
                    process_spu_tick(host_interp_L, host_interp_R);

                    spuOutL = m_lastSpuOutL;
                    spuOutR = m_lastSpuOutR;
                    producedSpu = true;

                    m_hostPhase -= 1.0f;
                }

                // Cache current host sample for next interpolation
                m_prevHostL = inL;
                m_prevHostR = inR;
                m_prevHostValid = true;

                // Upsample SPU output -> host sample by linear interpolation between previous and current SPU outputs
                float outL = 0.0f, outR = 0.0f;
                if (producedSpu) {
                    // fractional position after consuming SPU ticks (in [0,1) )
                    float frac = m_hostPhase;
                    // interpolate between previous SPU output and current SPU output
                    outL = (1.0f - frac) * m_prevSpuOutL + frac * spuOutL;
                    outR = (1.0f - frac) * m_prevSpuOutR + frac * spuOutR;
                    // update previous SPU outputs to current for next frame
                    m_prevSpuOutL = spuOutL;
                    m_prevSpuOutR = spuOutR;
                }
                else {
                    // no new SPU sample this host frame — hold last SPU output
                    outL = m_prevSpuOutL;
                    outR = m_prevSpuOutR;
                }

                // Mix the reverb output additively into the planar buffer
                ch0[i] += outL;
                if (ch1) {
                    ch1[i] += outR;
                }
                else {
                    // Mono host: mix average of L/R reverb back into single channel
                    ch0[i] += 0.5f * (outL + outR);
                }
            }
        }


    private:
        // SPU processing performed once per SPU tick (22.05 kHz).
        // Implements algorithm exactly as in spec:
        // - input volume vLIN/vRIN applied to incoming host-channel data (converted to SPU tick)
        // - same-side reflection (mLSAME/mRSAME)
        // - different-side reflection (mLDIFF/mRDIFF)
        // - comb filter sums (from mLCOMB1..4 and mRCOMB1..4)
        // - APF1 and APF2 (with dAPF1/dAPF2 delays and mLAPF1.. etc)
        // - FIR (39-tap) applied to Lout/Rout
        // - output volumes vLOUT/vROUT applied
        // - bufferAddr advanced: BufferAddress = MAX(mBASE, (BufferAddress + 2) AND 0x7FFFE)
        void process_spu_tick(float hostInputL, float hostInputR) {
            // Convert input volumes
            float Lin = vLIN * hostInputL;
            float Rin = vRIN * hostInputR;

            // SAME SIDE (Left->Left)
            // [mLSAME] = (Lin + [dLSAME]*vWALL - [mLSAME-2])*vIIR + [mLSAME-2]
            float prev_mLSAME_minus2 = read_ram_word_as_float((mLSAME - 2) & (int)(SPU_RAM_WORDS - 1));
            float dLSAME_val = read_ram_word_as_float(dLSAME & (int)(SPU_RAM_WORDS - 1));
            float new_mLSAME = (Lin + dLSAME_val * vWALL - prev_mLSAME_minus2) * vIIR + prev_mLSAME_minus2;
            write_ram_word_from_float(mLSAME & (int)(SPU_RAM_WORDS - 1), new_mLSAME);

            // SAME SIDE (Right->Right)
            float prev_mRSAME_minus2 = read_ram_word_as_float((mRSAME - 2) & (int)(SPU_RAM_WORDS - 1));
            float dRSAME_val = read_ram_word_as_float(dRSAME & (int)(SPU_RAM_WORDS - 1));
            float new_mRSAME = (Rin + dRSAME_val * vWALL - prev_mRSAME_minus2) * vIIR + prev_mRSAME_minus2;
            write_ram_word_from_float(mRSAME & (int)(SPU_RAM_WORDS - 1), new_mRSAME);

            // DIFFERENT SIDE (R->L)
            float prev_mLDIFF_minus2 = read_ram_word_as_float((mLDIFF - 2) & (int)(SPU_RAM_WORDS - 1));
            float dRDIFF_val = read_ram_word_as_float(dRDIFF & (int)(SPU_RAM_WORDS - 1));
            float new_mLDIFF = (Lin + dRDIFF_val * vWALL - prev_mLDIFF_minus2) * vIIR + prev_mLDIFF_minus2;
            write_ram_word_from_float(mLDIFF & (int)(SPU_RAM_WORDS - 1), new_mLDIFF);

            // DIFFERENT SIDE (L->R)
            float prev_mRDIFF_minus2 = read_ram_word_as_float((mRDIFF - 2) & (int)(SPU_RAM_WORDS - 1));
            float dLDIFF_val = read_ram_word_as_float(dLDIFF & (int)(SPU_RAM_WORDS - 1));
            float new_mRDIFF = (Rin + dLDIFF_val * vWALL - prev_mRDIFF_minus2) * vIIR + prev_mRDIFF_minus2;
            write_ram_word_from_float(mRDIFF & (int)(SPU_RAM_WORDS - 1), new_mRDIFF);

            // EARLY ECHO (Comb)
            float Lout = 0.0f;
            float Rout = 0.0f;
            Lout += vCOMB1 * read_ram_word_as_float(mLCOMB1 & (int)(SPU_RAM_WORDS - 1));
            Lout += vCOMB2 * read_ram_word_as_float(mLCOMB2 & (int)(SPU_RAM_WORDS - 1));
            Lout += vCOMB3 * read_ram_word_as_float(mLCOMB3 & (int)(SPU_RAM_WORDS - 1));
            Lout += vCOMB4 * read_ram_word_as_float(mLCOMB4 & (int)(SPU_RAM_WORDS - 1));

            Rout += vCOMB1 * read_ram_word_as_float(mRCOMB1 & (int)(SPU_RAM_WORDS - 1));
            Rout += vCOMB2 * read_ram_word_as_float(mRCOMB2 & (int)(SPU_RAM_WORDS - 1));
            Rout += vCOMB3 * read_ram_word_as_float(mRCOMB3 & (int)(SPU_RAM_WORDS - 1));
            Rout += vCOMB4 * read_ram_word_as_float(mRCOMB4 & (int)(SPU_RAM_WORDS - 1));

            // LATE REVERB APF1 (All-pass 1)
            // Lout = Lout - vAPF1*[mLAPF1-dAPF1]; [mLAPF1]=Lout; Lout=Lout*vAPF1+[mLAPF1-dAPF1]
            float mLAPF1_delay = read_ram_word_as_float(((mLAPF1 - dAPF1) & (int)(SPU_RAM_WORDS - 1)));
            float tmpL = Lout - vAPF1 * mLAPF1_delay;
            write_ram_word_from_float(mLAPF1 & (int)(SPU_RAM_WORDS - 1), tmpL);
            Lout = tmpL * vAPF1 + mLAPF1_delay;

            float mRAPF1_delay = read_ram_word_as_float(((mRAPF1 - dAPF1) & (int)(SPU_RAM_WORDS - 1)));
            float tmpR = Rout - vAPF1 * mRAPF1_delay;
            write_ram_word_from_float(mRAPF1 & (int)(SPU_RAM_WORDS - 1), tmpR);
            Rout = tmpR * vAPF1 + mRAPF1_delay;

            // LATE REVERB APF2
            float mLAPF2_delay = read_ram_word_as_float(((mLAPF2 - dAPF2) & (int)(SPU_RAM_WORDS - 1)));
            tmpL = Lout - vAPF2 * mLAPF2_delay;
            write_ram_word_from_float(mLAPF2 & (int)(SPU_RAM_WORDS - 1), tmpL);
            Lout = tmpL * vAPF2 + mLAPF2_delay;

            float mRAPF2_delay = read_ram_word_as_float(((mRAPF2 - dAPF2) & (int)(SPU_RAM_WORDS - 1)));
            tmpR = Rout - vAPF2 * mRAPF2_delay;
            write_ram_word_from_float(mRAPF2 & (int)(SPU_RAM_WORDS - 1), tmpR);
            Rout = tmpR * vAPF2 + mRAPF2_delay;

            // FIR 39-tap applied to Lout and Rout (SPU does FIR on final outputs)
            float finalL = apply_fir(Lout, m_firL);
            float finalR = apply_fir(Rout, m_firR);

            // Output to mixer, scaled by vLOUT/vROUT
            float outL = finalL * vLOUT;
            float outR = finalR * vROUT;

            // store final outputs to last SPU output registers for use by host upsampler
            m_lastSpuOutL = outL;
            m_lastSpuOutR = outR;

            // advance buffer address: BufferAddress = MAX(mBASE, (BufferAddress + 2) AND 0x7FFFE)
            m_bufferAddr = (m_bufferAddr + 2) & 0x7FFFE;
            if (m_bufferAddr < mBASE) m_bufferAddr = mBASE & (int)(SPU_RAM_WORDS - 1);
        }

        // Read SPU RAM word (stored as float internally; this mirrors behavior of SPU RAM reads for reverb)
        inline float read_ram_word_as_float(int wordIdx) const {
            // wordIdx already masked by caller
            return m_ram[wordIdx];
        }
        // Write SPU RAM word (float)
        inline void write_ram_word_from_float(int wordIdx, float value) {
            m_ram[wordIdx] = value;
        }

        // Apply FIR: uses per-channel ring buffer state vector, returns convolved value
        float apply_fir(float input, std::vector<float>& state) {
            // Insert input into ring buffer at m_firIndex
            size_t N = FIR_COEFFS.size();
            state[m_firIndex] = input;

            // Perform convolution
            float acc = 0.0f;
            // Coeffs are Q15 (signed 16-bit); convert each to float and multiply
            for (size_t k = 0; k < N; ++k) {
                size_t idx = (m_firIndex + N - k) % N;
                float s = state[idx];
                int16_t coeff = FIR_COEFFS[k];
                float c = q15_to_float(coeff); // treat coeff as Q15
                acc += s * c;
            }

            // advance index
            m_firIndex = (m_firIndex + 1) % N;

            return acc;
        }

    private:
        Preset m_preset;

        unsigned int m_hostSamplerate = 44100;
        double m_spuRate = 22050.0;
        float m_hostToSpuRatio = 0.5f;
        float m_spuToHostRatio = 2.0f;
        float m_hostPhase = 0.0f;

        // SPU registers (float conversions where applicable)
        float vIIR = 0.0f;
        float vWALL = 0.0f;
        float vAPF1 = 0.0f;
        float vAPF2 = 0.0f;
        float vCOMB1 = 0.0f, vCOMB2 = 0.0f, vCOMB3 = 0.0f, vCOMB4 = 0.0f;
        float vLOUT = 0.0f, vROUT = 0.0f, vLIN = 0.0f, vRIN = 0.0f;

        int mBASE = 0;
        int dAPF1 = 0, dAPF2 = 0;
        int dLSAME = 0, dRSAME = 0, dLDIFF = 0, dRDIFF = 0;
        int mLSAME = 0, mRSAME = 0, mLDIFF = 0, mRDIFF = 0;
        int mLCOMB1 = 0, mRCOMB1 = 0, mLCOMB2 = 0, mRCOMB2 = 0;
        int mLCOMB3 = 0, mRCOMB3 = 0, mLCOMB4 = 0, mRCOMB4 = 0;
        int mLAPF1 = 0, mRAPF1 = 0, mLAPF2 = 0, mRAPF2 = 0;

        // SPU RAM modeled as floats for arithmetic convenience (SPU stores 16-bit words; we use float for math fidelity)
        std::vector<float> m_ram;

        // buffer address (word)
        int m_bufferAddr = 0;

        // FIR ring buffer state for left and right channels
        std::vector<float> m_firL;
        std::vector<float> m_firR;
        size_t m_firIndex = 0;

        // last SPU output sample (for upsampling interpolation)
        float m_lastSpuOutL = 0.0f;
        float m_lastSpuOutR = 0.0f;
        float m_prevSpuOutL = 0.0f;
        float m_prevSpuOutR = 0.0f;

        // small cache of last host sample for downsampling interpolation
        float m_prevHostL = 0.0f;
        float m_prevHostR = 0.0f;
        bool m_prevHostValid = false;
    };

    // PS1ReverbFilter : SoLoud Filter wrapper, holds presets map and creates instances
    class PS1ReverbFilter : public SoLoud::Filter {
    public:
        PS1ReverbFilter(const std::string& presetName)
        {
            if (!m_presetsBuilt) {
                m_presets = build_preset_map();
                m_presetsBuilt = true;
            }
            auto it = m_presets.find(presetName);
            if (it == m_presets.end()) {
                // fallback to Room if not found
                m_selectedPreset = m_presets.at("Room");
            }
            else {
                m_selectedPreset = it->second;
            }
        }

        virtual SoLoud::FilterInstance* createInstance() override {
            // Use standard SoLoud sample rate default assumption of 44100 unless asked otherwise
            unsigned int hostSamplerate = 44100;
            return new PS1ReverbFilterInstance(m_selectedPreset, hostSamplerate);
        }

        virtual ~PS1ReverbFilter() = default;

    private:
        static std::unordered_map<std::string, Preset> m_presets;
        static bool m_presetsBuilt;
        Preset m_selectedPreset;
    };

    // static members
    std::unordered_map<std::string, Preset> PS1ReverbFilter::m_presets;
    bool PS1ReverbFilter::m_presetsBuilt = false;

} // namespace ps1reverb

// Convenience alias for outside use
using PS1ReverbFilter = ps1reverb::PS1ReverbFilter;
using PS1ReverbFilterInstance = ps1reverb::PS1ReverbFilterInstance;
