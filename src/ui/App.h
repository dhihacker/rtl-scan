#pragma once
#include "ControlsView.h"
#include "SpectrumView.h"
#include "WaterfallView.h"
#include "sdr/SweepConfig.h"
#include "sdr/SweepController.h"
#include "util/RingBuffer.h"
#include <mutex>
#include <optional>

struct GLFWwindow;

constexpr int RING_SIZE = 16;

class App {
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    bool init(SweepConfig initial_cfg, int width = 1280, int height = 800);
    void run();
    void shutdown();

    // Thread-safe — called from the sweep thread.
    void push_frame(SweepFrame frame);

private:
    void render();
    void restart_sweep(SweepConfig cfg);

    GLFWwindow*                        m_window{nullptr};
    RingBuffer<SweepFrame, RING_SIZE>  m_ring;
    SpectrumView                       m_spectrum;
    WaterfallView                      m_waterfall;
    ControlsView                       m_controls;
    SweepController                    m_sweeper;

    // Pending restart requested from the controls panel (main thread only).
    std::optional<SweepConfig>         m_pending_cfg;
};
