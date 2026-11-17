# pico-sdk: add_subdirectory(helix-mp3-pico) after pico_sdk_init(), then
# target_link_libraries(your_app helix_mp3_pico).
cmake_minimum_required(VERSION 3.13)
add_library(helix_mp3_pico INTERFACE)
file(GLOB HELIX_MP3_PICO_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.c)
target_sources(helix_mp3_pico INTERFACE ${HELIX_MP3_PICO_SOURCES})
target_include_directories(helix_mp3_pico INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src)
