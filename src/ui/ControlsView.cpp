#include "ControlsView.h"
#include <imgui.h>
#include <cmath>

// Common named presets
static const struct { const char* label; float start, end; } kPresets[] = {
    {"FM",         76.0f,  108.0f},
    {"Air",       108.0f,  137.0f},
    {"NOAA APT",  137.0f,  138.5f},
    {"2m Ham",    144.0f,  146.0f},
    {"PMR/LPD",   430.0f,  470.0f},
    {"ADS-B",    1089.0f, 1091.0f},
};

void ControlsView::init(SweepConfig seed) {
    m_start_mhz = seed.freq_start_hz / 1e6f;
    m_end_mhz   = seed.freq_end_hz   / 1e6f;
    m_fft_size  = seed.fft_size;
    m_crop_pct  = seed.crop_pct;
}

void ControlsView::draw(bool sweep_running, ApplyCallback on_apply) {
    ImGui::SetNextWindowSize(ImVec2(520, 60), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Controls")) {
        ImGui::End();
        return;
    }

    bool apply = false;

    // ── Frequency range ───────────────────────────────────────────────────────
    ImGui::SetNextItemWidth(110);
    if (ImGui::InputFloat("Start MHz", &m_start_mhz, 0.1f, 1.0f, "%.3f",
                          ImGuiInputTextFlags_EnterReturnsTrue))
        apply = true;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    if (ImGui::InputFloat("End MHz", &m_end_mhz, 0.1f, 1.0f, "%.3f",
                          ImGuiInputTextFlags_EnterReturnsTrue))
        apply = true;

    // ── Presets ───────────────────────────────────────────────────────────────
    ImGui::SameLine();
    ImGui::SetNextItemWidth(110);
    if (ImGui::BeginCombo("##preset", "Presets")) {
        for (auto& p : kPresets) {
            if (ImGui::Selectable(p.label)) {
                m_start_mhz = p.start;
                m_end_mhz   = p.end;
                apply       = true;
            }
        }
        ImGui::EndCombo();
    }

    // ── Apply button ──────────────────────────────────────────────────────────
    ImGui::SameLine();
    if (sweep_running)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.5f, 0.1f, 1.0f));

    if (ImGui::Button(sweep_running ? "Apply" : "Apply (no SDR)"))
        apply = true;
    ImGui::PopStyleColor();

    // ── Span info ─────────────────────────────────────────────────────────────
    ImGui::SameLine();
    float span = m_end_mhz - m_start_mhz;
    if (span > 0) {
        int   crop   = (m_fft_size * m_crop_pct) / 100;
        float bin_hz = 2'560'000.0f / m_fft_size;
        float hop    = (m_fft_size - 2 * crop) * bin_hz / 1e6f;
        int   hops   = static_cast<int>(std::ceil(span / hop));
        ImGui::TextDisabled("%.1f MHz · ~%d hops · %.0f Hz/bin",
                            span, hops, bin_hz);
    } else {
        ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "end must be > start");
    }

    if (apply && m_end_mhz > m_start_mhz) {
        SweepConfig cfg;
        cfg.freq_start_hz = static_cast<uint32_t>(m_start_mhz * 1e6f);
        cfg.freq_end_hz   = static_cast<uint32_t>(m_end_mhz   * 1e6f);
        cfg.sample_rate   = 2'560'000;
        cfg.fft_size      = m_fft_size;
        cfg.crop_pct      = m_crop_pct;
        on_apply(cfg);
    }

    ImGui::End();
}
