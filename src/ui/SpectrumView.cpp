#include "SpectrumView.h"
#include <imgui.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>

void SpectrumView::push(const SweepFrame& frame) {
    m_latest   = frame;
    m_has_data = true;
}

void SpectrumView::draw() {
    ImGui::SetNextWindowSize(ImVec2(900, 280), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Spectrum")) {
        ImGui::End();
        return;
    }

    ImVec2 canvas_pos  = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.x < 10 || canvas_size.y < 10) {
        ImGui::End();
        return;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvas_pos,
                      ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                      IM_COL32(15, 15, 20, 255));

    if (!m_has_data || m_latest.bins.empty()) {
        const char* msg = "Waiting for sweep data...";
        ImVec2 ts = ImGui::CalcTextSize(msg);
        dl->AddText(ImVec2(canvas_pos.x + (canvas_size.x - ts.x) * 0.5f,
                           canvas_pos.y + (canvas_size.y - ts.y) * 0.5f),
                    IM_COL32(200, 200, 200, 200), msg);
        ImGui::Dummy(canvas_size);
        ImGui::End();
        return;
    }

    const float freq_lo  = static_cast<float>(m_latest.freq_start_hz);
    const float freq_hi  = static_cast<float>(m_latest.freq_end_hz);
    const float bw_hz    = freq_hi - freq_lo;

    // Frequency grid (6 vertical lines)
    constexpr int V_DIVS = 6;
    for (int i = 0; i <= V_DIVS; ++i) {
        float fx    = canvas_pos.x + canvas_size.x * i / V_DIVS;
        float f_mhz = (freq_lo + bw_hz * i / V_DIVS) / 1e6f;
        dl->AddLine(ImVec2(fx, canvas_pos.y),
                    ImVec2(fx, canvas_pos.y + canvas_size.y),
                    IM_COL32(50, 50, 60, 200));
        char label[32];
        std::snprintf(label, sizeof(label), "%.3f", f_mhz);
        dl->AddText(ImVec2(fx + 2, canvas_pos.y + canvas_size.y - 14),
                    IM_COL32(160, 160, 160, 255), label);
    }

    // dB grid (5 horizontal lines)
    constexpr int H_DIVS = 5;
    for (int i = 0; i <= H_DIVS; ++i) {
        float db = m_db_max - (m_db_max - m_db_min) * i / H_DIVS;
        float fy = canvas_pos.y + canvas_size.y * i / H_DIVS;
        dl->AddLine(ImVec2(canvas_pos.x, fy),
                    ImVec2(canvas_pos.x + canvas_size.x, fy),
                    IM_COL32(50, 50, 60, 200));
        char label[16];
        std::snprintf(label, sizeof(label), "%+.0f", db);
        dl->AddText(ImVec2(canvas_pos.x + 2, fy + 1),
                    IM_COL32(130, 130, 130, 255), label);
    }

    // Downsample sweep bins to display width
    const int   n_bins    = static_cast<int>(m_latest.bins.size());
    const int   n_pixels  = static_cast<int>(canvas_size.x);
    const float db_range  = m_db_max - m_db_min;
    const float baseline  = canvas_pos.y + canvas_size.y;

    std::vector<ImVec2> pts;
    pts.reserve(n_pixels + 2);
    pts.push_back(ImVec2(canvas_pos.x, baseline));

    for (int px = 0; px < n_pixels; ++px) {
        // Map display pixel → bin range (max-hold to preserve peaks)
        int bin_lo = (px * n_bins) / n_pixels;
        int bin_hi = ((px + 1) * n_bins) / n_pixels;
        if (bin_hi <= bin_lo) bin_hi = bin_lo + 1;
        bin_hi = std::min(bin_hi, n_bins);

        float peak = m_db_min;
        for (int b = bin_lo; b < bin_hi; ++b)
            peak = std::max(peak, m_latest.bins[b]);

        float db = std::clamp(peak, m_db_min, m_db_max);
        float t  = 1.0f - (db - m_db_min) / db_range;
        pts.push_back(ImVec2(canvas_pos.x + px, canvas_pos.y + canvas_size.y * t));
    }

    pts.push_back(ImVec2(canvas_pos.x + canvas_size.x, baseline));

    dl->AddConvexPolyFilled(pts.data(), static_cast<int>(pts.size()),
                            IM_COL32(20, 80, 90, 180));
    dl->AddPolyline(pts.data() + 1, n_pixels, IM_COL32(0, 210, 200, 255), 0, 1.0f);

    ImGui::Dummy(canvas_size);
    ImGui::End();
}
