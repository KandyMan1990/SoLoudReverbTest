#include "PS1Reverb.h"
#include <algorithm>
#include <cstring>

using namespace std;

PS1Reverb::PS1Reverb()
{
    Power();
    InitPresets();
}

void PS1Reverb::Power()
{
    ReverbCur = 0;
    ReverbWA = 0;
    RvbResPos = 0;
    SPURAM.fill(0);
    for (int i = 0; i < 2; i++)
    {
        RUSB[i].fill(0);
        RDSB[i].fill(0);
    }
}

void PS1Reverb::Init()
{
    // Reset buffer positions
    RvbResPos = 0;
    ReverbCur = ReverbWA;

    // Zero stereo resampling buffers
    for (int ch = 0; ch < 2; ch++)
    {
        for (int i = 0; i < 64; i++)
            RDSB[ch][i] = 0;

        for (int i = 0; i < 32; i++)
            RUSB[ch][i] = 0;
    }

    // Zero SPU RAM used for reverb
    for (uint32_t i = 0; i < 0x40000; i++)
        SPURAM[i] = 0;

    // Reset any other internal state
    ReverbCur = ReverbWA; // Current write position
}


void PS1Reverb::InitPresets()
{
    // Official PSXSPX 10 presets
    Presets.clear();

    Presets["Room"] = { {{0x007D,0x005B,0x6D80,0x54B8,0xBED0,0x0000,0x0000,0xBA80,
                         0x5800,0x5300,0x04D6,0x0333,0x03F0,0x0227,0x0374,0x01EF,
                         0x0334,0x01B5,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
                         0x0000,0x0000,0x01B4,0x0136,0x00B8,0x005C,0x8000,0x8000}} };

    Presets["Studio Small"] = { {{0x0033,0x0025,0x70F0,0x4FA8,0xBCE0,0x4410,0xC0F0,0x9C00,
                                 0x5280,0x4EC0,0x03E4,0x031B,0x03A4,0x02AF,0x0372,0x0266,
                                 0x031C,0x025D,0x025C,0x018E,0x022F,0x0135,0x01D2,0x00B7,
                                 0x018F,0x00B5,0x00B4,0x0080,0x004C,0x0026,0x8000,0x8000}} };

    Presets["Studio Medium"] = { {{0x00B1,0x007F,0x70F0,0x4FA8,0xBCE0,0x4510,0xBEF0,0xB4C0,
                                  0x5280,0x4EC0,0x0904,0x076B,0x0824,0x065F,0x07A2,0x0616,
                                  0x076C,0x05ED,0x05EC,0x042E,0x050F,0x0305,0x0462,0x02B7,
                                  0x042F,0x0265,0x0264,0x01B2,0x0100,0x0080,0x8000,0x8000}} };

    Presets["Studio Large"] = { {{0x00E3,0x00A9,0x6F60,0x4FA8,0xBCE0,0x4510,0xBEF0,0xA680,
                                  0x5680,0x52C0,0x0DFB,0x0B58,0x0D09,0x0A3C,0x0BD9,0x0973,
                                  0x0B59,0x08DA,0x08D9,0x05E9,0x07EC,0x04B0,0x06EF,0x03D2,
                                  0x05EA,0x031D,0x031C,0x0238,0x0154,0x00AA,0x8000,0x8000}} };

    Presets["Hall"] = { {{0x01A5,0x0139,0x6000,0x5000,0x4C00,0xB800,0xBC00,0xC000,
                         0x6000,0x5C00,0x15BA,0x11BB,0x14C2,0x10BD,0x11BC,0x0DC1,
                         0x11C0,0x0DC3,0x0DC0,0x09C1,0x0BC4,0x07C1,0x0A00,0x06CD,
                         0x09C2,0x05C1,0x05C0,0x041A,0x0274,0x013A,0x8000,0x8000}} };

    Presets["Half Echo"] = { {{0x0017,0x0013,0x70F0,0x4FA8,0xBCE0,0x4510,0xBEF0,0x8500,
                              0x5F80,0x54C0,0x0371,0x02AF,0x02E5,0x01DF,0x02B0,0x01D7,
                              0x0358,0x026A,0x01D6,0x011E,0x012D,0x00B1,0x011F,0x0059,
                              0x01A0,0x00E3,0x0058,0x0040,0x0028,0x0014,0x8000,0x8000}} };

    Presets["Space Echo"] = { {{0x033D,0x0231,0x7E00,0x5000,0xB400,0xB000,0x4C00,0xB000,
                               0x6000,0x5400,0x1ED6,0x1A31,0x1D14,0x183B,0x1BC2,0x16B2,
                               0x1A32,0x15EF,0x15EE,0x1055,0x1334,0x0F2D,0x11F6,0x0C5D,
                               0x1056,0x0AE1,0x0AE0,0x07A2,0x0464,0x0232,0x8000,0x8000}} };

    Presets["Chaos Echo"] = { {{0x0001,0x0001,0x7FFF,0x7FFF,0x0000,0x0000,0x0000,0x8100,
                               0x0000,0x0000,0x1FFF,0x0FFF,0x1005,0x0005,0x0000,0x0000,
                               0x1005,0x0005,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
                               0x0000,0x0000,0x1004,0x1002,0x0004,0x0002,0x8000,0x8000}} };

    Presets["Delay"] = { {{0x0001,0x0001,0x7FFF,0x7FFF,0x0000,0x0000,0x0000,0x0000,
                          0x0000,0x0000,0x1FFF,0x0FFF,0x1005,0x0005,0x0000,0x0000,
                          0x1005,0x0005,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
                          0x0000,0x0000,0x1004,0x1002,0x0004,0x0002,0x8000,0x8000}} };

    Presets["Reverb Off"] = { {{0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
                               0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
                               0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
                               0x0000,0x0000,0x0001,0x0001,0x0001,0x0001,0x0000,0x0000}} };

    // Set default preset to "Reverb Off"
    SetPreset("Reverb Off");
}

bool PS1Reverb::SetPreset(const std::string& name)
{
    auto it = Presets.find(name);
    if (it == Presets.end())
        return false;

    // First value is ReverbWA
    ReverbWA = it->second.values[0];
    ReverbCur = ReverbWA;

    // Load all 32 preset values into SPURAM at ReverbWA
    for (int i = 0; i < 32; i++)
        SPURAM[(ReverbWA + i) & 0x3FFFF] = it->second.values[i];

    // Reset internal reverb buffers
    RvbResPos = 0;
    for (int ch = 0; ch < 2; ch++)
    {
        for (int i = 0; i < 64; i++) RDSB[ch][i] = 0;
        for (int i = 0; i < 32; i++) RUSB[ch][i] = 0;
    }

    return true;
}


void PS1Reverb::WriteSPURAM(uint32_t addr, uint16_t value)
{
    SPURAM[addr & 0x3FFFF] = value;
}

uint16_t PS1Reverb::ReadSPURAM(uint32_t addr) const
{
    return SPURAM[addr & 0x3FFFF];
}

inline uint32_t PS1Reverb::Get_Reverb_Offset(uint32_t in_offset) const
{
    uint32_t offset = ReverbCur + (in_offset & 0x3FFFF);
    offset += ReverbWA & ((int32_t)(offset << 13) >> 31);
    return offset & 0x3FFFF;
}

inline int16_t PS1Reverb::RD_RVB(uint16_t raw_offs, int32_t extra_offs) const
{
    return ReadSPURAM(Get_Reverb_Offset((raw_offs << 2) + extra_offs));
}

inline void PS1Reverb::WR_RVB(uint16_t raw_offs, int16_t sample)
{
    SPURAM[Get_Reverb_Offset(raw_offs << 2)] = sample;
}

inline int16_t PS1Reverb::ReverbSat(int32_t samp)
{
    return static_cast<int16_t>(std::clamp(samp, -32768, 32767));
}

inline int32_t PS1Reverb::Reverb2244(const int16_t* src)
{
    static const int16_t ResampTable[20] =
    { -1, 2, -10, 35, -103, 266, -616, 1332, -2960, 10246,
     10246, -2960, 1332, -616, 266, -103, 35, -10, 2, -1 };

    int32_t out = 0;
    for (unsigned i = 0; i < 20; i++)
        out += ResampTable[i] * src[i];

    out >>= 14;
    return ReverbSat(out);
}

inline int32_t PS1Reverb::Reverb4422(const int16_t* src)
{
    static const int16_t ResampTable[20] =
    { -1, 2, -10, 35, -103, 266, -616, 1332, -2960, 10246,
     10246, -2960, 1332, -616, 266, -103, 35, -10, 2, -1 };

    int32_t out = 0;
    for (unsigned i = 0; i < 20; i++)
        out += ResampTable[i] * src[i * 2];

    out += 0x4000 * src[19];
    out >>= 15;
    return ReverbSat(out);
}

inline int32_t PS1Reverb::IIASM(int16_t IIR_ALPHA, int16_t insamp)
{
    if (IIR_ALPHA == -32768)
        return (insamp == -32768) ? 0 : insamp * -65536;
    return insamp * (32768 - IIR_ALPHA);
}

void PS1Reverb::RunReverb(const int32_t in[2], int32_t out[2])
{
    int32_t upsampled[2] = { 0, 0 };

    // Store input sample in the resample buffers
    for (int ch = 0; ch < 2; ch++)
    {
        RDSB[ch][RvbResPos | 0x00] = in[ch];
        RDSB[ch][RvbResPos | 0x40] = in[ch];
    }

    if (RvbResPos & 1)
    {
        int32_t downsampled[2] = { 0, 0 };

        // Downsample using Reverb4422
        for (int ch = 0; ch < 2; ch++)
            downsampled[ch] = Reverb4422(&RDSB[ch][(RvbResPos - 38) & 0x3F]);

        // IIR/feedback processing
        for (int ch = 0; ch < 2; ch++)
        {
            int16_t val = ReverbSat(downsampled[ch]);
            RUSB[ch][RvbResPos >> 1] = val;
            upsampled[ch] = Reverb2244(&RUSB[ch][((RvbResPos >> 1) - 19) & 0x1F]);
        }

        // Advance SPU RAM write pointer
        ReverbCur = (ReverbCur + 1) & 0x3FFFF;
        if (!ReverbCur) ReverbCur = ReverbWA;
    }
    else
    {
        // Even RvbResPos: just use previous upsampled value
        for (int ch = 0; ch < 2; ch++)
        {
            const int16_t* src = &RUSB[ch][((RvbResPos >> 1) - 19) & 0x1F];
            upsampled[ch] = src[9];
        }
    }

    // Advance resample position
    RvbResPos = (RvbResPos + 1) & 0x3F;

    // Output wet-only signal
    out[0] = upsampled[0];
    out[1] = upsampled[1];
}
