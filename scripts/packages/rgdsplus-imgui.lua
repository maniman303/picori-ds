-- Build the three SDL3 backends without re-resolving SDL in a nested xmake
-- project (which would lose the local cross recipe).
package("imgui")
    set_homepage("https://github.com/ocornut/imgui")
    set_description("Dear ImGui with Picori's SDL3 backends")
    set_license("MIT")
    add_urls("https://github.com/ocornut/imgui/archive/refs/tags/$(version).tar.gz")
    add_versions("v1.92.7", "b21b14ce1ef6dd4d85fa54f68f8449a9be94f3b453aba7fe4ea2a9764e43a5ef")
    for _, backend in ipairs({"sdl3", "sdl3_renderer", "sdl3_gpu"}) do
        add_configs(backend, {description = "Compile SDL3 backend", default = true, type = "boolean"})
    end
    add_deps("cmake", "libsdl3 3.4.12")
    on_install("linux", function (package)
        io.writefile("CMakeLists.txt", [[
cmake_minimum_required(VERSION 3.16)
project(picori_imgui LANGUAGES CXX)
add_library(imgui STATIC imgui.cpp imgui_draw.cpp imgui_tables.cpp
    imgui_widgets.cpp imgui_demo.cpp misc/cpp/imgui_stdlib.cpp
    backends/imgui_impl_sdl3.cpp backends/imgui_impl_sdlrenderer3.cpp
    backends/imgui_impl_sdlgpu3.cpp)
target_compile_features(imgui PRIVATE cxx_std_14)
target_include_directories(imgui PRIVATE . "${PICORI_SDL_INCLUDE_DIR}")
install(TARGETS imgui ARCHIVE DESTINATION lib)
install(DIRECTORY ./ DESTINATION include FILES_MATCHING PATTERN "*.h"
    PATTERN ".git" EXCLUDE PATTERN "build*" EXCLUDE)
]])
        import("package.tools.cmake").install(package, {
            "-DCMAKE_BUILD_TYPE=Release",
            "-DPICORI_SDL_INCLUDE_DIR=" .. package:dep("libsdl3"):installdir("include")
        })
    end)
package_end()
