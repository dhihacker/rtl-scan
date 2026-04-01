#pragma once
#include <cstdint>
#include <vector>

struct SweepConfig {
    uint32_t freq_start_hz  = 80'000'000;
    uint32_t freq_end_hz    = 108'000'000;
    uint32_t sample_rate    = 2'560'000;
    int      fft_size       = 2048;
    // Percentage of bins to discard from each edge (anti-alias rolloff).
    // 10 → throw away 10% from each side, use middle 80%.
    int      crop_pct       = 10;
};

// One complete sweep across [freq_start_hz, freq_end_hz].
// bins[i] is the dBFS value for frequency:
//   freq_start_hz + i * (sample_rate / fft_size)
struct SweepFrame {
    std::vector<float> bins;
    uint32_t freq_start_hz{0};
    uint32_t freq_end_hz{0};

    float bin_hz() const {
        return bins.empty() ? 1.0f
             : static_cast<float>(freq_end_hz - freq_start_hz) / bins.size();
    }
};
