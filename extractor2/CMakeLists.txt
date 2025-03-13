cmake_minimum_required(VERSION 3.10.0)
project(YlandsExtractor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_SKIP_INSTALL_RULES YES)


# Primary cmd program
set(target_1_name ExtractorV2)
set(target_1_sources
    main.cpp
    config.cpp
    utils.cpp
    space.cpp
    exporter.cpp
    extractor.cpp
    scene.cpp
    combomesh.cpp
    ylands.cpp
    objwavefront.cpp
    workpool.cpp
)
list(TRANSFORM target_1_sources PREPEND "src/")

add_executable(${target_1_name} ${target_1_sources})
target_compile_features(${target_1_name} PRIVATE cxx_std_17)
if (MSVC)
    # Do not build to subdir Debug or Release
    set_target_properties(${target_1_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/core/$<0:>)
    # Disable MSVC warnings for intentional type violations
    target_compile_options(${target_1_name} PRIVATE /wd4996 /wd4018 /wd4244 /wd4305)
    # if (CMAKE_BUILD_TYPE STREQUAL "Release")
    #     target_compile_options(${target_1_name} PRIVATE /Ox)
    # endif()
endif()

add_custom_command(
        TARGET ${target_1_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/lookup.json
                ${CMAKE_SOURCE_DIR}/extract_config.json
                ${CMAKE_SOURCE_DIR}/from_ylands/blockdef_2025_02.json
                ${CMAKE_SOURCE_DIR}/from_ylands/scene_ref.json
                ${CMAKE_BINARY_DIR}/core)
add_custom_command(
    TARGET ${target_1_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/models
            ${CMAKE_BINARY_DIR}/core/models)


# Secondary Win32 App
set(target_2_name ExtractorV2App)
set(target_2_sources
    winapp.cpp
    yextractv2.rc
)
list(TRANSFORM target_2_sources PREPEND "src/winapp/")

add_executable(${target_2_name} WIN32 ${target_2_sources})
target_compile_features(${target_2_name} PRIVATE cxx_std_17)
target_link_libraries(${target_2_name} comctl32.lib dwmapi.lib)
if (MSVC)
    # Do not build to subdir Debug or Release
    set_target_properties(${target_2_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<0:>)
endif()