#pragma once
#include "sdr/SweepConfig.h"
#include <array>
#include <vector>
#include <cstdint>

// Fixed display width for the waterfall texture (horizontal resolution).
constexpr int WATERFALL_WIDTH = 4096;
constexpr int WATERFALL_ROWS  = 512;

// Scrolling waterfall panel.
// Maintains a WATERFALL_WIDTH × WATERFALL_ROWS RGBA texture.
// Each SweepFrame is downsampled to WATERFALL_WIDTH and becomes the new top row.
class WaterfallView {
public:
    void init();
    void destroy();
    void clear();   // Wipe history (call on range change)

    void push(const SweepFrame& frame);
    void draw();

private:
    static std::array<uint8_t, 4> colormap(float t);

    unsigned int m_tex{0};
    std::vector<uint8_t> m_pixels;  // WATERFALL_WIDTH * WATERFALL_ROWS * 4

    float m_db_min{-100.0f};
    float m_db_max{-20.0f};
    bool  m_dirty{false};

    // Frequency extents of the last pushed frame (for labels)
    uint32_t m_freq_start_hz{0};
    uint32_t m_freq_end_hz{0};
};
