cmake_minimum_required(VERSION 3.18)
project(FBX2glTF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
if (LINUX)
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_CXX_COMPILER g++)
elseif(APPLE)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
elseif(WIN32)
    set(CMAKE_C_COMPILER cl)
    set(CMAKE_CXX_COMPILER cl)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(ExternalProject)

# FBX
foreach (FBXSDK_VERSION "2020.3.7")
    find_package(FBX)
    if (FBXSDK_FOUND)
        break()
    endif ()
endforeach (FBXSDK_VERSION)
if (NOT FBXSDK_FOUND)
    message(FATAL_ERROR
            "Can't find FBX SDK in either:\n"
            " - Mac OS X: ${FBXSDK_APPLE_ROOT}\n"
            " - Windows: ${FBXSDK_WINDOWS_ROOT}\n"
            " - Linux: ${FBXSDK_LINUX_ROOT}"
    )
endif ()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# stuff we get from Conan
find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(draco CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(Iconv MODULE REQUIRED)

# create a compilation database for e.g. clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (APPLE)
    find_library(CF_FRAMEWORK CoreFoundation)
    message("CoreFoundation Framework: ${CF_FRAMEWORK}")
    set(FRAMEWORKS ${CF_FRAMEWORK})
endif ()

file(GLOB_RECURSE HEADER_FILES
        include/*.h
        include/*.hpp
)

set(LIB_HEADERS ${HEADER_FILES})

set(LIB_SOURCE_FILES
        src/fbx/materials/3dsMaxPhysicalMaterial.cpp
        src/fbx/materials/FbxMaterials.cpp
        src/fbx/materials/StingrayPBSMaterial.cpp
        src/fbx/materials/TraditionalMaterials.cpp
        src/fbx/Fbx2Raw.cpp
        src/fbx/FbxBlendShapesAccess.cpp
        src/fbx/FbxSkinningAccess.cpp
        src/gltf/Raw2Gltf.cpp
        src/gltf/GltfModel.cpp
        src/gltf/TextureBuilder.cpp
        src/gltf/properties/AccessorData.cpp
        src/gltf/properties/AnimationData.cpp
        src/gltf/properties/BufferData.cpp
        src/gltf/properties/BufferViewData.cpp
        src/gltf/properties/CameraData.cpp
        src/gltf/properties/ImageData.cpp
        src/gltf/properties/LightData.cpp
        src/gltf/properties/MaterialData.cpp
        src/gltf/properties/MeshData.cpp
        src/gltf/properties/NodeData.cpp
        src/gltf/properties/PrimitiveData.cpp
        src/gltf/properties/SceneData.cpp
        src/gltf/properties/SkinData.cpp
        src/gltf/properties/TextureData.cpp
        src/raw/RawModel.cpp
        src/utils/File_Utils.cpp
        src/utils/Image_Utils.cpp
)

add_library(libFBX2glTF STATIC ${LIB_SOURCE_FILES})
set_target_properties(libFBX2glTF PROPERTIES OUTPUT_NAME "FBX2glTF")
add_executable(appFBX2glTF src/FBX2glTF.cpp)
set_target_properties(appFBX2glTF PROPERTIES OUTPUT_NAME "FBX2glTF")


if (NOT MSVC)
    # Disable annoying & spammy warning from FBX SDK header file
    target_compile_options(libFBX2glTF PRIVATE
            "-Wno-null-dereference"
            "-Wunused"
    )
    target_compile_options(appFBX2glTF PRIVATE
            "-Wno-null-dereference"
            "-Wunused"
    )
endif ()

if (MSVC)
target_link_libraries(libFBX2glTF
        ${FRAMEWORKS}
	    draco::draco
        optimized ${FBXSDK_LIBRARY} ${FBXSDK_LIBXML2_LIBRARY} ${FBXSDK_ZLIB_LIBRARY} ${FBXSDK_ALEMBIC_LIBRARY}
        debug ${FBXSDK_LIBRARY_DEBUG} ${FBXSDK_LIBXML2_LIBRARY_DEBUG} ${FBXSDK_ZLIB_LIBRARY_DEBUG} ${FBXSDK_ALEMBIC_LIBRARY_DEBUG}
        fmt::fmt-header-only
        ${CMAKE_DL_LIBS}
        ${CMAKE_THREAD_LIBS_INIT}
        glm::glm-header-only
        nlohmann_json::nlohmann_json
        CLI11::CLI11
        Iconv::Iconv
)
else()
target_link_libraries(libFBX2glTF
        ${FRAMEWORKS}
	    draco::draco
        optimized ${FBXSDK_LIBRARY}
        debug ${FBXSDK_LIBRARY_DEBUG}
        fmt::fmt-header-only
        LibXml2::LibXml2
        ZLIB::ZLIB
        ${CMAKE_DL_LIBS}
        ${CMAKE_THREAD_LIBS_INIT}
        glm::glm-header-only
        nlohmann_json::nlohmann_json
        CLI11::CLI11
        Iconv::Iconv
)
endif()

target_include_directories(libFBX2glTF PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
         ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
         ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(libFBX2glTF SYSTEM PUBLIC
        ${FBXSDK_INCLUDE_DIR}
        ${draco_INCLUDE_DIRS}
        ${Stb_INCLUDE_DIR}
)

target_link_libraries(appFBX2glTF libFBX2glTF)

install(TARGETS libFBX2glTF appFBX2glTF
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
)
