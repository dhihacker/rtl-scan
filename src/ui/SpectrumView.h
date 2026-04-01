#pragma once
#include "sdr/SweepConfig.h"

// Draws a live FFT spectrum panel using ImGui's DrawList.
class SpectrumView {
public:
    void push(const SweepFrame& frame);
    void draw();

private:
    SweepFrame m_latest;
    bool       m_has_data{false};

    float m_db_min{-100.0f};
    float m_db_max{-20.0f};
};
