#include "sdr/SweepConfig.h"
#include "ui/App.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <csignal>

static uint32_t parse_freq(const char* s) {
    char*  end = nullptr;
    double v   = std::strtod(s, &end);
    if (end && (*end == 'M' || *end == 'm')) v *= 1e6;
    else if (end && (*end == 'G' || *end == 'g')) v *= 1e9;
    else if (end && (*end == 'K' || *end == 'k')) v *= 1e3;
    return static_cast<uint32_t>(v);
}

int main(int argc, char** argv) {
    SweepConfig cfg;
    cfg.freq_start_hz = 80'000'000;
    cfg.freq_end_hz   = 108'000'000;
    cfg.sample_rate   = 2'560'000;
    cfg.fft_size      = 2048;
    cfg.crop_pct      = 10;

    if (argc >= 2) cfg.freq_start_hz = parse_freq(argv[1]);
    if (argc >= 3) cfg.freq_end_hz   = parse_freq(argv[2]);

    if (cfg.freq_end_hz <= cfg.freq_start_hz) {
        std::fprintf(stderr, "freq_end must be > freq_start\n");
        return 1;
    }

    float span_mhz = (cfg.freq_end_hz - cfg.freq_start_hz) / 1e6f;
    float bin_hz   = static_cast<float>(cfg.sample_rate) / cfg.fft_size;
    int   crop     = (cfg.fft_size * cfg.crop_pct) / 100;
    float hop_mhz  = (cfg.fft_size - 2 * crop) * bin_hz / 1e6f;
    int   n_hops   = static_cast<int>(std::ceil(span_mhz / hop_mhz));
    std::fprintf(stdout,
        "Sweep: %.3f – %.3f MHz  (%.1f MHz span, ~%d hops, %.0f Hz/bin)\n",
        cfg.freq_start_hz / 1e6f, cfg.freq_end_hz / 1e6f,
        span_mhz, n_hops, bin_hz);

    App app;
    if (!app.init(cfg)) {
        std::fprintf(stderr, "Failed to initialise window/GL\n");
        return 1;
    }

    app.run();
    return 0;
}
