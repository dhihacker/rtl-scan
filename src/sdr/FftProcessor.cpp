#include "FftProcessor.h"
#include <fftw3.h>
#include <cmath>
#include <functional>

static constexpr float kScale = 1.0f / (127.5f * FFT_SIZE);

FftProcessor::FftProcessor()
    : m_hann(FFT_SIZE), m_input(FFT_SIZE * 2), m_output(FFT_SIZE * 2)
{
    // Pre-compute Hann window
    for (int i = 0; i < FFT_SIZE; ++i)
        m_hann[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (FFT_SIZE - 1)));

    // Create FFTW plan: in-place complex-to-complex forward FFT
    m_plan = fftwf_plan_dft_1d(
        FFT_SIZE,
        reinterpret_cast<fftwf_complex*>(m_input.data()),
        reinterpret_cast<fftwf_complex*>(m_output.data()),
        FFTW_FORWARD,
        FFTW_ESTIMATE);
}

FftProcessor::~FftProcessor() {
    if (m_plan)
        fftwf_destroy_plan(static_cast<fftwf_plan>(m_plan));
}

void FftProcessor::process(const uint8_t* buf, uint32_t len, FrameCallback cb) {
    // buf contains interleaved uint8 IQ: [I0,Q0, I1,Q1, ...]
    for (uint32_t i = 0; i + 1 < len; i += 2) {
        float re = (static_cast<float>(buf[i])     - 127.5f);
        float im = (static_cast<float>(buf[i + 1]) - 127.5f);

        m_input[m_fill * 2]     = re * m_hann[m_fill];
        m_input[m_fill * 2 + 1] = im * m_hann[m_fill];

        if (++m_fill == FFT_SIZE) {
            compute_fft(cb);
            m_fill = 0;
        }
    }
}

void FftProcessor::compute_fft(FrameCallback& cb) {
    fftwf_execute(static_cast<fftwf_plan>(m_plan));

    FftFrame frame;
    // FFT output is [0..N-1]; rearrange to [-N/2..N/2] (DC centred).
    for (int i = 0; i < FFT_SIZE; ++i) {
        int src  = (i + FFT_SIZE / 2) % FFT_SIZE;
        float re = m_output[src * 2];
        float im = m_output[src * 2 + 1];
        float mag = std::sqrt(re * re + im * im) * kScale;
        // Floor at a tiny value to avoid log(0)
        frame.bins[i] = 20.0f * std::log10(std::max(mag, 1e-10f));
    }
    cb(std::move(frame));
}
