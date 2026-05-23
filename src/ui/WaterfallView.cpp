#include "WaterfallView.h"
#include <imgui.h>
#include <GL/gl.h>
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

std::array<uint8_t, 4> WaterfallView::colormap(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    struct Stop { float pos; uint8_t r, g, b; };
    constexpr Stop stops[] = {
        {0.00f,   0,   0,  40},
        {0.20f,   0,   0, 200},
        {0.40f,   0, 210, 210},
        {0.60f,   0, 200,   0},
        {0.80f, 255, 220,   0},
        {1.00f, 255, 255, 255},
    };
    constexpr int N = sizeof(stops) / sizeof(stops[0]);
    for (int i = 0; i < N - 1; ++i) {
        if (t <= stops[i + 1].pos) {
            float a = (t - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            auto lerp = [&](uint8_t x, uint8_t y) -> uint8_t {
                return static_cast<uint8_t>(x + a * (static_cast<float>(y) - x));
            };
            return {lerp(stops[i].r, stops[i+1].r),
                    lerp(stops[i].g, stops[i+1].g),
                    lerp(stops[i].b, stops[i+1].b), 255};
        }
    }
    return {255, 255, 255, 255};
}

void WaterfallView::init() {
    m_pixels.assign(WATERFALL_WIDTH * WATERFALL_ROWS * 4, 0);

    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 WATERFALL_WIDTH, WATERFALL_ROWS, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, m_pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void WaterfallView::destroy() {
    if (m_tex) {
        glDeleteTextures(1, &m_tex);
        m_tex = 0;
    }
}

void WaterfallView::clear() {
    std::fill(m_pixels.begin(), m_pixels.end(), uint8_t{0});
    m_freq_start_hz = 0;
    m_freq_end_hz   = 0;
    m_dirty         = true;
}

void WaterfallView::push(const SweepFrame& frame) {
    if (frame.bins.empty()) return;

    m_freq_start_hz = frame.freq_start_hz;
    m_freq_end_hz   = frame.freq_end_hz;

    // Shift all rows down by one row
    const int row_bytes = WATERFALL_WIDTH * 4;
    std::memmove(m_pixels.data() + row_bytes,
                 m_pixels.data(),
                 row_bytes * (WATERFALL_ROWS - 1));

    // Downsample sweep frame to WATERFALL_WIDTH pixels (max-hold per pixel bucket)
    const int   n_bins   = static_cast<int>(frame.bins.size());
    const float db_range = m_db_max - m_db_min;

    for (int px = 0; px < WATERFALL_WIDTH; ++px) {
        int bin_lo = (px * n_bins) / WATERFALL_WIDTH;
        int bin_hi = ((px + 1) * n_bins) / WATERFALL_WIDTH;
        if (bin_hi <= bin_lo) bin_hi = bin_lo + 1;
        bin_hi = std::min(bin_hi, n_bins);

        float peak = m_db_min;
        for (int b = bin_lo; b < bin_hi; ++b)
            peak = std::max(peak, frame.bins[b]);

        float t   = (peak - m_db_min) / db_range;
        auto  col = colormap(t);
        int   off = px * 4;
        m_pixels[off + 0] = col[0];
        m_pixels[off + 1] = col[1];
        m_pixels[off + 2] = col[2];
        m_pixels[off + 3] = col[3];
    }
    m_dirty = true;
}

void WaterfallView::draw() {
    ImGui::SetNextWindowSize(ImVec2(900, 300), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Waterfall")) {
        ImGui::End();
        return;
    }

    if (m_dirty && m_tex) {
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        WATERFALL_WIDTH, WATERFALL_ROWS,
                        GL_RGBA, GL_UNSIGNED_BYTE, m_pixels.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        m_dirty = false;
    }

    ImVec2 canvas_pos  = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.x < 10 || canvas_size.y < 10) {
        ImGui::End();
        return;
    }

    ImGui::Image((ImTextureID)(intptr_t)m_tex, canvas_size,
                 ImVec2(0, 0), ImVec2(1, 1));

    // Frequency labels overlay
    if (m_freq_end_hz > m_freq_start_hz) {
        ImDrawList* dl  = ImGui::GetWindowDrawList();
        float bw_hz     = static_cast<float>(m_freq_end_hz - m_freq_start_hz);
        constexpr int V_DIVS = 6;
        for (int i = 0; i <= V_DIVS; ++i) {
            float fx    = canvas_pos.x + canvas_size.x * i / V_DIVS;
            float f_mhz = (m_freq_start_hz + bw_hz * i / V_DIVS) / 1e6f;
            char  label[32];
            snprintf(label, sizeof(label), "%.3f", f_mhz);
            dl->AddLine(ImVec2(fx, canvas_pos.y),
                        ImVec2(fx, canvas_pos.y + canvas_size.y),
                        IM_COL32(80, 80, 80, 120));
            dl->AddText(ImVec2(fx + 2, canvas_pos.y + canvas_size.y - 14),
                        IM_COL32(200, 200, 200, 220), label);
        }
    }

    ImGui::End();
}
