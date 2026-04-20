cmake_minimum_required(VERSION 3.25)
project(sms-emulator LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3  CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# ── Main executable ─────────────────────────────────────────────────────────────
file(GLOB_RECURSE SMS_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(sms-emulator ${SMS_SOURCES})

target_include_directories(sms-emulator PRIVATE src/)

target_link_libraries(sms-emulator PRIVATE
    SDL3::SDL3
    imgui::imgui
)

target_compile_options(sms-emulator PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>
)

# ── Tests ───────────────────────────────────────────────────────────────────────
enable_testing()

file(GLOB_RECURSE SMS_TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")

if(SMS_TEST_SOURCES)
    include(GoogleTest)

    add_executable(sms-tests ${SMS_TEST_SOURCES})

    target_include_directories(sms-tests PRIVATE src/)

    target_link_libraries(sms-tests PRIVATE
        SDL3::SDL3
        imgui::imgui
        GTest::gtest
        GTest::gtest_main
    )

    target_compile_options(sms-tests PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>
    )

    gtest_discover_tests(sms-tests)
endif()
