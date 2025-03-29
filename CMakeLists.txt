cmake_minimum_required(VERSION 3.10.0)
project(YlandsExtractor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_SKIP_INSTALL_RULES YES)
if (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/Ox")
endif()


#######################################
# Primary cmd program
#######################################
set(target_1_name extractor)
set(target_1_sources
    main.cpp
    config.cpp
    utils.cpp
    space.cpp
    exporter.cpp
    extractor.cpp
    scene.cpp
    combomesh.cpp
    reducer.cpp
    ylands.cpp
    objwavefront.cpp
    gltf.cpp
    workpool.cpp
)
list(TRANSFORM target_1_sources PREPEND "src/")

add_executable(${target_1_name} ${target_1_sources})
target_compile_features(${target_1_name} PRIVATE cxx_std_17)
if (MSVC)
    target_compile_options(${target_1_name} PRIVATE /W3)
    # Disable MSVC warnings for intentional type violations
    target_compile_options(${target_1_name} PRIVATE /wd4996 /wd4018 /wd4244 /wd4305)
else()
    # Disable clang warnings for intentional type violations
    target_compile_options(${target_1_name} PRIVATE -Wno-string-plus-int -Wno-writable-strings)
endif()
set_target_properties(${target_1_name} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug/base
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release/base
)
add_custom_command(
        TARGET ${target_1_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/lookup.json
                ${CMAKE_SOURCE_DIR}/core/config.json
                ${CMAKE_SOURCE_DIR}/core/from_ylands/blockdef_2025_03.json
                ${CMAKE_SOURCE_DIR}/core/from_ylands/ref_scene/scene_ref.json
                $<TARGET_FILE_DIR:${target_1_name}>)
add_custom_command(
    TARGET ${target_1_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/models
            $<TARGET_FILE_DIR:${target_1_name}>/models)


#######################################
# Secondary Win32 App
#######################################
set(target_2_name extractorapp)
set(target_2_sources
    winapp.cpp
    yextractv2.rc
)
list(TRANSFORM target_2_sources PREPEND "src/winapp/")

add_executable(${target_2_name} WIN32 ${target_2_sources})
target_compile_features(${target_2_name} PRIVATE cxx_std_17)
target_compile_options(${target_2_name} PRIVATE /W3)
target_link_libraries(${target_2_name} comctl32.lib dwmapi.lib)
set_target_properties(${target_2_name} PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release
)
