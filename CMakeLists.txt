cmake_minimum_required(VERSION 4.0)
project(SoLoudReverbTest)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES "src/*.cpp" "soloud/src/*.c" "soloud/src/*.cpp")

add_executable(SoLoudReverbTest ${SOURCES})

target_include_directories(SoLoudReverbTest PRIVATE
        ${CMAKE_SOURCE_DIR}/soloud/include
)

target_compile_definitions(SoLoudReverbTest PRIVATE WITH_WINMM)

target_link_libraries(SoLoudReverbTest PRIVATE winmm)

add_custom_command(TARGET SoLoudReverbTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/sfx"
        $<TARGET_FILE_DIR:SoLoudReverbTest>/sfx
)