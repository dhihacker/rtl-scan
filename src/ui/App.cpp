#include "App.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <cstdio>

static void glfw_error_callback(int error, const char* desc) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, desc);
}

App::App()  = default;
App::~App() { shutdown(); }

bool App::init(SweepConfig initial_cfg, int width, int height) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, "rtl-scan", nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowBorderSize = 1.0f;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    m_waterfall.init();
    m_controls.init(initial_cfg);

    restart_sweep(initial_cfg);
    return true;
}

// Build the default 3-row layout the first time the dockspace has no children
// (i.e. no imgui.ini was present). Safe to call every frame — it checks first.
static void setup_default_layout(ImGuiID dock_id, ImVec2 size) {
    ImGui::DockBuilderRemoveNode(dock_id);
    ImGui::DockBuilderAddNode(dock_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dock_id, size);

    // Split off a thin strip at the top for Controls (~9%)
    ImGuiID dock_controls, dock_lower;
    ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Up, 0.09f,
                                &dock_controls, &dock_lower);

    // Split the lower portion: Spectrum top ~38%, Waterfall the rest
    ImGuiID dock_spectrum, dock_waterfall;
    ImGui::DockBuilderSplitNode(dock_lower, ImGuiDir_Up, 0.38f,
                                &dock_spectrum, &dock_waterfall);

    ImGui::DockBuilderDockWindow("Controls",  dock_controls);
    ImGui::DockBuilderDockWindow("Spectrum",  dock_spectrum);
    ImGui::DockBuilderDockWindow("Waterfall", dock_waterfall);
    ImGui::DockBuilderFinish(dock_id);
}

void App::restart_sweep(SweepConfig cfg) {
    m_sweeper.stop();
    while (m_ring.try_pop()) {}
    m_spectrum  = SpectrumView{};
    m_waterfall.clear();
    m_sweeper.start(cfg, [this](SweepFrame f) {
        push_frame(std::move(f));
    });
}

void App::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        if (m_pending_cfg) {
            restart_sweep(*m_pending_cfg);
            m_pending_cfg.reset();
        }

        render();
    }
}

void App::render() {
    while (auto frame = m_ring.try_pop()) {
        m_spectrum.push(*frame);
        m_waterfall.push(*frame);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ── Full-screen dockspace host ────────────────────────────────────────────
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##dockspace_host", nullptr, host_flags);
    ImGui::PopStyleVar();

    ImGuiID dock_id = ImGui::GetID("MainDock");

    // Apply default layout when no ini has been loaded yet.
    ImGuiDockNode* node = ImGui::DockBuilderGetNode(dock_id);
    if (!node || node->IsEmpty())
        setup_default_layout(dock_id, vp->WorkSize);

    ImGui::DockSpace(dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    // ── Panels ────────────────────────────────────────────────────────────────
    m_controls.draw(m_sweeper.running(), [this](SweepConfig cfg) {
        m_pending_cfg = cfg;
    });
    m_spectrum.draw();
    m_waterfall.draw();

    ImGui::Render();

    int fb_w, fb_h;
    glfwGetFramebufferSize(m_window, &fb_w, &fb_h);
    glViewport(0, 0, fb_w, fb_h);
    glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}

void App::shutdown() {
    if (!m_window) return;
    m_sweeper.stop();
    m_waterfall.destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(m_window);
    glfwTerminate();
    m_window = nullptr;
}

void App::push_frame(SweepFrame frame) {
    m_ring.push(std::move(frame));
}
