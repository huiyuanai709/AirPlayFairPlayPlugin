cmake_minimum_required(VERSION 3.16)
project(AirPlayFairPlayPlugin VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(PLAYFAIR_SOURCES
    third_party/playfair/hand_garble.c
    third_party/playfair/modified_md5.c
    third_party/playfair/omg_hax.c
    third_party/playfair/playfair.c
    third_party/playfair/sap_hash.c
)

add_library(AirPlayFairPlayPlugin SHARED
    src/plugin.c
    src/fairplay_session.c
    ${PLAYFAIR_SOURCES}
)

target_include_directories(AirPlayFairPlayPlugin
    PUBLIC include
    PRIVATE src third_party/playfair
)

if(MSVC)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_sources(AirPlayFairPlayPlugin PRIVATE src/AirPlayFairPlayPlugin.x86.def)
    else()
        target_sources(AirPlayFairPlayPlugin PRIVATE src/AirPlayFairPlayPlugin.def)
    endif()
    target_compile_options(AirPlayFairPlayPlugin PRIVATE /W4)
    set_source_files_properties(${PLAYFAIR_SOURCES} PROPERTIES COMPILE_OPTIONS "/w")
else()
    target_compile_options(AirPlayFairPlayPlugin PRIVATE -Wall -Wextra -Wpedantic)
    set_source_files_properties(${PLAYFAIR_SOURCES} PROPERTIES
        COMPILE_OPTIONS "-w;-fno-strict-aliasing")
    if(UNIX AND NOT APPLE)
        target_link_options(AirPlayFairPlayPlugin PRIVATE "LINKER:--no-undefined")
    endif()
    target_link_libraries(AirPlayFairPlayPlugin PRIVATE m)
    find_package(Threads REQUIRED)
    target_link_libraries(AirPlayFairPlayPlugin PRIVATE Threads::Threads)
endif()

set_target_properties(AirPlayFairPlayPlugin PROPERTIES
    OUTPUT_NAME AirPlayFairPlayPlugin
    C_VISIBILITY_PRESET hidden
)

option(AIRPLAY_FP_BUILD_SMOKE "Build the dlopen smoke test" ON)
if(AIRPLAY_FP_BUILD_SMOKE)
    add_executable(fairplay_plugin_smoke tests/smoke.c)
    target_link_libraries(fairplay_plugin_smoke PRIVATE ${CMAKE_DL_LIBS})
    if(MSVC)
        target_compile_options(fairplay_plugin_smoke PRIVATE /W4)
    else()
        target_compile_options(fairplay_plugin_smoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    enable_testing()
    add_test(NAME smoke COMMAND fairplay_plugin_smoke $<TARGET_FILE:AirPlayFairPlayPlugin>)
endif()
