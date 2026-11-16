cmake_minimum_required(VERSION 3.21)
project(sysinfo LANGUAGES CXX)

# Shared host Qt is a Release runtime. Do not default single-config builds
# to Debug, which imports Qt6*d.dll unavailable in an installed WinShun.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

# hwinfo 2 is a C++23 library. Keep its static libraries on the same CRT as
# the plugin, and compile its UTF-8 sources correctly on non-UTF-8 code pages.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime" FORCE)
    add_compile_options(/utf-8)
endif()

option(SYSINFO_BUILD_PLUGIN "Build the plugin and native UI integration" ON)
if(SYSINFO_BUILD_PLUGIN)
set(HWINFO_SHARED OFF CACHE BOOL "" FORCE)
set(HWINFO_STATIC ON CACHE BOOL "" FORCE)
set(HWINFO_COMPUTER OFF CACHE BOOL "" FORCE)
set(HWINFO_MAINBOARD OFF CACHE BOOL "" FORCE)
set(HWINFO_GPU_BACKENDS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/hwinfo EXCLUDE_FROM_ALL)
if(MSVC)
    # hwinfo vendors SMBIOS data with non-ASCII bytes. Retain its warning level
    # but don't promote the code-page diagnostic to an error.
    foreach(target hwinfo_os hwinfo_computer hwinfo_battery hwinfo_cpu hwinfo_disk hwinfo_gpu hwinfo_network hwinfo_ram)
        if(TARGET ${target})
            target_compile_options(${target} PRIVATE /WX-)
        endif()
    endforeach()
endif()

add_subdirectory(third_party/liquid-glass-cpp)

qt_add_executable(sysinfo
    main.cpp
    native_overlay.cpp
    sysinfo_sampler.cpp
    sysinfo_settings.cpp
    sysinfo_ui.cpp)
target_include_directories(sysinfo PRIVATE "${CMAKE_SOURCE_DIR}")
target_compile_definitions(sysinfo PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
    WINVER=0x0A00 _WIN32_WINNT=0x0A00)
if(MSVC)
    target_compile_options(sysinfo PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(sysinfo PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8 -Wall -Wextra -Wpedantic)
    target_link_options(sysinfo PRIVATE -static -static-libgcc -static-libstdc++)
endif()
target_link_libraries(sysinfo PRIVATE Qt6::Core Qt6::Gui liquid_glass::liquid_glass
    lfreist-hwinfo::hwinfo iphlpapi powrprof dxgi d3d11 d3dcompiler d2d1 dwrite dcomp pdh dwmapi user32 advapi32)
set_target_properties(sysinfo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(sysinfo PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_SOURCE_DIR}")
endforeach()

endif()

qt_add_executable(sysinfo_tests tests/test_settings.cpp sysinfo_settings.cpp)
target_compile_definitions(sysinfo_tests PRIVATE SYSINFO_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(sysinfo_tests PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(sysinfo_tests PRIVATE Qt6::Core Qt6::Gui)
if(MSVC)
    target_compile_options(sysinfo_tests PRIVATE /utf-8 /W4 /permissive- /UNDEBUG)
else()
    target_compile_options(sysinfo_tests PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8 -Wall -Wextra -Wpedantic -UNDEBUG)
endif()
set_target_properties(sysinfo_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(sysinfo_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/tests")
endforeach()

enable_testing()
add_test(NAME sysinfo_core COMMAND sysinfo_tests)
set_tests_properties(sysinfo_core PROPERTIES TIMEOUT 60 LABELS "plugin;logic")
