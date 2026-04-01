#pragma once
#include "sdr/SweepConfig.h"
#include <functional>
#include <optional>

// Toolbar/controls panel.
// Renders an ImGui window with frequency range inputs and an Apply button.
// When the user clicks Apply (or presses Enter), emits the new SweepConfig
// via the provided callback.
class ControlsView {
public:
    using ApplyCallback = std::function<void(SweepConfig)>;

    // seed: initial config used to populate the input fields.
    void init(SweepConfig seed);
    void draw(bool sweep_running, ApplyCallback on_apply);

private:
    float    m_start_mhz{80.0f};
    float    m_end_mhz{108.0f};
    int      m_fft_size{2048};
    int      m_crop_pct{10};
};
