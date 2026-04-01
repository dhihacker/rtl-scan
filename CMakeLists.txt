cmake_minimum_required(VERSION 3.20)
project(rtl-scan VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTLSDR  REQUIRED librtlsdr)
pkg_check_modules(FFTW3F  REQUIRED fftw3f)
pkg_check_modules(GLFW    REQUIRED glfw3)

find_package(OpenGL REQUIRED)

# ── Dear ImGui (vendored, docking branch) ─────────────────────────────────────
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GL3W=0)
target_link_libraries(imgui PUBLIC ${GLFW_LIBRARIES} OpenGL::GL)
target_include_directories(imgui PUBLIC ${GLFW_INCLUDE_DIRS})
target_compile_options(imgui PUBLIC ${GLFW_CFLAGS_OTHER})

# ── rtl-scan executable ────────────────────────────────────────────────────────
add_executable(rtl-scan
    src/main.cpp
    src/sdr/SdrDevice.cpp
    src/sdr/FftProcessor.cpp
    src/sdr/SweepController.cpp
    src/ui/App.cpp
    src/ui/ControlsView.cpp
    src/ui/SpectrumView.cpp
    src/ui/WaterfallView.cpp
)

target_include_directories(rtl-scan PRIVATE
    src
    ${RTLSDR_INCLUDE_DIRS}
    ${FFTW3F_INCLUDE_DIRS}
)

target_link_libraries(rtl-scan PRIVATE
    imgui
    ${RTLSDR_LIBRARIES}
    ${FFTW3F_LIBRARIES}
    OpenGL::GL
    pthread
)

target_compile_options(rtl-scan PRIVATE
    -Wall -Wextra -O2
    ${RTLSDR_CFLAGS_OTHER}
    ${FFTW3F_CFLAGS_OTHER}
)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS rtl-scan DESTINATION bin)
