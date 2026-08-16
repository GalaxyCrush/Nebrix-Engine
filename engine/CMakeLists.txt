add_library(Nebrix STATIC
    src/Core/Log.cpp
    src/Core/GameLoop.cpp
    src/Platform/Input.cpp
    src/Platform/Window.cpp
    src/Platform/Stb.cpp
    src/Renderer/Shader.cpp
    src/Renderer/Buffer.cpp
    src/Renderer/Texture.cpp
    src/Renderer/TextureAtlas.cpp
    src/Renderer/Camera2D.cpp
    src/Renderer/Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glad/src/glad.c
)

target_include_directories(Nebrix
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/stb
)

target_link_libraries(Nebrix PUBLIC glfw)

target_compile_options(Nebrix PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(Nebrix PRIVATE $<$<CONFIG:Debug>:NEBRIX_DEBUG>)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../vendor/glad/src/glad.c
    PROPERTIES COMPILE_OPTIONS "-w")
