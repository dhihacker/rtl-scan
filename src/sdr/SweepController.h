#pragma once
#include "SweepConfig.h"
#include <atomic>
#include <functional>
#include <string>
#include <thread>

// Runs a dedicated sweep thread: hops the RTL-SDR center frequency across
// [config.freq_start_hz, config.freq_end_hz] using synchronous reads, computes
// an FFT per hop, stitches bins into a SweepFrame, and emits it via callback.
class SweepController {
public:
    using FrameCallback = std::function<void(SweepFrame)>;

    SweepController()  = default;
    ~SweepController() { stop(); }

    SweepController(const SweepController&)            = delete;
    SweepController& operator=(const SweepController&) = delete;

    bool start(SweepConfig config, FrameCallback cb, int device_index = 0);
    void stop();

    bool        running() const { return m_running.load(); }
    std::string error()   const { return m_error; }

private:
    void sweep_loop(SweepConfig config, FrameCallback cb, int device_index);

    std::thread       m_thread;
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_running{false};
    std::string       m_error;
};
