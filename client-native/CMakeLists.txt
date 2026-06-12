cmake_minimum_required(VERSION 3.24)

project(mmo4wheels_client LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# Con generadores multi-config (Visual Studio) el exe y los shaders deben
# acabar en el mismo bin/, no en bin/<config>/.
foreach(CFG IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

include(FetchContent)

# ---------------- Dependencias (versiones fijadas) ----------------

# SDL3: capa de plataforma (ventana, input, audio) + GPU API
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(sdl3
  URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.2.24.tar.gz)

# Dear ImGui: HUD de debug (no trae build system; la lib se define abajo)
FetchContent_Declare(imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.4.tar.gz)

# glslang: compilador GLSL -> SPIR-V, construido como herramienta del propio
# build para no exigir el SDK de Vulkan instalado
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES ON CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glslang
  URL https://github.com/KhronosGroup/glslang/archive/refs/tags/15.4.0.tar.gz)

FetchContent_MakeAvailable(sdl3 imgui glslang)

add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_lib PUBLIC SDL3::SDL3)

# ---------------- Shaders (GLSL 450 -> SPIR-V) ----------------

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

set(SHADERS
  triangle.vert
  triangle.frag)

set(SPV_FILES "")
foreach(SHADER IN LISTS SHADERS)
  set(SPV ${SHADER_OUT_DIR}/${SHADER}.spv)
  add_custom_command(
    OUTPUT ${SPV}
    COMMAND glslang-standalone -V ${SHADER_DIR}/${SHADER} -o ${SPV}
    DEPENDS ${SHADER_DIR}/${SHADER} glslang-standalone
    COMMENT "GLSL -> SPIR-V: ${SHADER}")
  list(APPEND SPV_FILES ${SPV})
endforeach()
add_custom_target(shaders ALL DEPENDS ${SPV_FILES})

# ---------------- Ejecutable ----------------

add_executable(mmo4wheels
  src/main.cpp
  src/platform/app.cpp
  src/render/renderer.cpp)
target_include_directories(mmo4wheels PRIVATE src)
target_link_libraries(mmo4wheels PRIVATE SDL3::SDL3 imgui_lib)
add_dependencies(mmo4wheels shaders)
