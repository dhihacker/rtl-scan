#include "SweepController.h"
#include "SdrDevice.h"
#include "FftProcessor.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

bool SweepController::start(SweepConfig config, FrameCallback cb, int device_index) {
    m_stop    = false;
    m_running = true;
    m_thread  = std::thread(&SweepController::sweep_loop, this,
                            std::move(config), std::move(cb), device_index);
    return true;
}

void SweepController::stop() {
    m_stop = true;
    if (m_thread.joinable())
        m_thread.join();
}

void SweepController::sweep_loop(SweepConfig cfg, FrameCallback cb, int device_index) {
    SdrDevice sdr;
    if (!sdr.open(cfg.sample_rate, device_index)) {
        m_error   = sdr.error();
        m_running = false;
        return;
    }

    FftProcessor fft;

    // Bin resolution
    const float  bin_hz       = static_cast<float>(cfg.sample_rate) / cfg.fft_size;
    const int    crop_bins    = (cfg.fft_size * cfg.crop_pct) / 100;
    const int    usable_bins  = cfg.fft_size - 2 * crop_bins;
    const float  hop_step_hz  = usable_bins * bin_hz;

    // Align first hop so its first usable bin starts at freq_start_hz.
    // First usable bin of a hop centered at fc has frequency:
    //   fc - sample_rate/2 + crop_bins * bin_hz
    // Set that equal to freq_start_hz:
    const uint32_t first_hop_center = cfg.freq_start_hz
        + cfg.sample_rate / 2
        - static_cast<uint32_t>(crop_bins * bin_hz + 0.5f);

    // Total output bins to cover [freq_start, freq_end]
    const int total_bins = static_cast<int>(
        std::ceil((cfg.freq_end_hz - cfg.freq_start_hz) / bin_hz)) + 1;

    // Read buffer: one FFT window per hop (fft_size IQ samples = fft_size*2 bytes)
    const int buf_bytes = cfg.fft_size * 2;
    std::vector<uint8_t> buf(buf_bytes);

    while (!m_stop) {
        SweepFrame frame;
        frame.freq_start_hz = cfg.freq_start_hz;
        frame.freq_end_hz   = cfg.freq_end_hz;
        frame.bins.assign(total_bins, -120.0f);

        uint32_t hop_center = first_hop_center;

        while (!m_stop) {
            // Check if this hop's lowest usable bin is past freq_end
            float hop_start_freq = static_cast<float>(hop_center)
                                 - cfg.sample_rate * 0.5f
                                 + crop_bins * bin_hz;
            if (hop_start_freq >= static_cast<float>(cfg.freq_end_hz))
                break;

            sdr.set_center_freq(hop_center);

            // Discard first buffer after frequency change (settling)
            sdr.read_sync(buf.data(), buf_bytes);

            // Actual read
            int n = sdr.read_sync(buf.data(), buf_bytes);
            if (n < buf_bytes) {
                hop_center += static_cast<uint32_t>(hop_step_hz);
                continue;
            }

            fft.reset();
            FftFrame fft_frame;
            bool got_frame = false;
            fft.process(buf.data(), buf_bytes, [&](FftFrame f) {
                fft_frame  = std::move(f);
                got_frame  = true;
            });

            if (got_frame) {
                // Interpolate through DC spike (center ±2 bins of the shifted FFT)
                const int dc = cfg.fft_size / 2;
                for (int d = -2; d <= 2; ++d) {
                    int idx = dc + d;
                    if (idx > 0 && idx < cfg.fft_size - 1)
                        fft_frame.bins[idx] = (fft_frame.bins[idx - 1]
                                             + fft_frame.bins[idx + 1]) * 0.5f;
                }

                // Stitch usable bins into the output frame
                for (int i = crop_bins; i < cfg.fft_size - crop_bins; ++i) {
                    float bin_freq = static_cast<float>(hop_center)
                                   - cfg.sample_rate * 0.5f
                                   + i * bin_hz;
                    int out_idx = static_cast<int>(
                        (bin_freq - cfg.freq_start_hz) / bin_hz + 0.5f);
                    if (out_idx >= 0 && out_idx < total_bins)
                        frame.bins[out_idx] = fft_frame.bins[i];
                }
            }

            hop_center += static_cast<uint32_t>(hop_step_hz);
        }

        if (!m_stop)
            cb(std::move(frame));
    }

    sdr.close();
    m_running = false;
}
