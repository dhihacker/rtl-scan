#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

constexpr int FFT_SIZE = 2048;

// One processed FFT frame: FFT_SIZE magnitude bins in dBFS.
struct FftFrame {
    std::array<float, FFT_SIZE> bins{};  // dBFS values, index 0 = DC
};

// Accumulates raw IQ bytes from the SDR callback, applies a Hann window,
// runs an FFTW3f plan, and converts to dBFS magnitude.
// Thread-safety: process() is called from the SDR thread only.
class FftProcessor {
public:
    using FrameCallback = std::function<void(FftFrame)>;

    FftProcessor();
    ~FftProcessor();

    FftProcessor(const FftProcessor&)            = delete;
    FftProcessor& operator=(const FftProcessor&) = delete;

    // Feed raw uint8 IQ samples (interleaved I,Q, DC-offset 127).
    // Emits a frame via cb each time FFT_SIZE samples are accumulated.
    void process(const uint8_t* buf, uint32_t len, FrameCallback cb);

    // Reset accumulation state (call between hops in sweep mode).
    void reset() { m_fill = 0; }

private:
    void compute_fft(FrameCallback& cb);

    std::vector<float>  m_hann;
    std::vector<float>  m_input;   // FFTW input buffer (re/im interleaved: [r0,i0,r1,i1,...])
    std::vector<float>  m_output;  // FFTW output buffer (complex: [r0,i0,r1,i1,...])
    void*               m_plan{nullptr};
    int                 m_fill{0};
};
