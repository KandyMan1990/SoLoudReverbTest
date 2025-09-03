#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <array>

class PS1Reverb
{
public:
    PS1Reverb();
    ~PS1Reverb() = default;

    void Init();

    // Reset all reverb state
    void Power();

    // Run the reverb for a stereo pair (input/output are 32-bit accumulators)
    void RunReverb(const int32_t in[2], int32_t out[2]);

    // Set a preset by name
    bool SetPreset(const std::string& name);

    // Optional: read/write the SPU RAM if needed
    void WriteSPURAM(uint32_t addr, uint16_t value);
    uint16_t ReadSPURAM(uint32_t addr) const;

private:
    // Core reverb buffers
    std::array<int16_t, 0x40000> SPURAM{};      // 256 KB SPU RAM
    std::array<int16_t, 0x40> RUSB[2]{};        // Reverb upsampling buffer
    std::array<int16_t, 0x40> RDSB[2]{};        // Reverb downsampling buffer

    // Reverb state
    uint32_t ReverbCur;
    uint32_t ReverbWA;
    uint32_t RvbResPos;

    // Internal functions
    uint32_t Get_Reverb_Offset(uint32_t in_offset) const;
    int16_t RD_RVB(uint16_t raw_offs, int32_t extra_offs) const;
    void WR_RVB(uint16_t raw_offs, int16_t sample);

    static int16_t ReverbSat(int32_t samp);
    static int32_t Reverb2244(const int16_t* src);
    static int32_t Reverb4422(const int16_t* src);
    static int32_t IIASM(int16_t IIR_ALPHA, int16_t insamp);

    // Preset storage
    struct Preset
    {
        std::array<uint16_t, 32> values;
    };
    std::map<std::string, Preset> Presets;

    // Load all official presets
    void InitPresets();
};
