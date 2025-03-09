cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_SKIP_INSTALL_RULES YES)

set(target_name ExtractorV2)
set(target_sources
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
list(TRANSFORM target_sources PREPEND "src/")

project(YlandsExtractor VERSION 0.1.0 LANGUAGES C CXX)

add_executable(${target_name} ${target_sources})
target_compile_features(${target_name} PRIVATE cxx_std_17)
if ( MSVC )
   set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<0:>)
    # Disable MSVC warnings for intentional type violations
    target_compile_options(${target_name} PRIVATE /wd4996 /wd4018 /wd4244 /wd4305)
endif ( MSVC )

add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/lookup.json
                ${CMAKE_SOURCE_DIR}/extract_config.json
                ${CMAKE_SOURCE_DIR}/from_ylands/blockdef_2025_02.json
                ${CMAKE_SOURCE_DIR}/from_ylands/scene_ref.json
                ${CMAKE_BINARY_DIR})
add_custom_command(
    TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/models
            ${CMAKE_BINARY_DIR}/models)
