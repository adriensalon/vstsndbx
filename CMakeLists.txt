cmake_minimum_required(VERSION 3.12)
project(vstsndbx VERSION 1.0.0)

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON)
add_subdirectory(external/vst3sdk)
smtg_enable_vst3_sdk()

set(RAPIDJSON_BUILD_DOC OFF)
set(RAPIDJSON_BUILD_EXAMPLES OFF)
set(RAPIDJSON_BUILD_TESTS OFF)
add_subdirectory(external/rapidjson)

if(SMTG_MAC)
    set(moduleinfotool_sources
        ${moduleinfotool_sources}
        ${CMAKE_CURRENT_LIST_DIR}/external/vst3sdk/public.sdk/source/vst/hosting/module_mac.mm
    )
    set_source_files_properties(
        "${CMAKE_CURRENT_LIST_DIR}/external/vst3sdk/public.sdk/source/vst/hosting/module_mac.mm" PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )
elseif(SMTG_LINUX)
    set(moduleinfotool_sources
        ${moduleinfotool_sources}
        ${CMAKE_CURRENT_LIST_DIR}/external/vst3sdk/public.sdk/source/vst/hosting/module_linux.cpp
    )
elseif(SMTG_WIN)
    set(moduleinfotool_sources
        ${moduleinfotool_sources}
        ${CMAKE_CURRENT_LIST_DIR}/external/vst3sdk/public.sdk/source/vst/hosting/module_win32.cpp
    )
endif()

set(moduleinfotool_sources
    ${moduleinfotool_sources}
    ${CMAKE_CURRENT_LIST_DIR}/external/vst3sdk/public.sdk/source/vst/hosting/plugprovider.cpp
    ${CMAKE_CURRENT_LIST_DIR}/external/vst3sdk/public.sdk/source/vst/hosting/plugprovider.h
)

file(GLOB_RECURSE vstsndbx_source "source/*.cpp")
smtg_add_vst3plugin(vstsndbx ${vstsndbx_source} ${moduleinfotool_sources})
set_target_properties(vstsndbx PROPERTIES CXX_STANDARD 17)
target_link_libraries(vstsndbx PRIVATE sdk sdk_hosting sdk_common)
target_compile_definitions(vstsndbx PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS) # for rapidjson
target_include_directories(vstsndbx PRIVATE source)
target_include_directories(vstsndbx PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/vst3sdk)
target_include_directories(vstsndbx PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/rapidjson/include)

