#include "loader/FbxLoader.h"

#include "fbx/Fbx2Raw.hpp"
#include "gltf/Raw2Gltf.hpp"
//
// Created by Wallance on 2025/10/14.
//
bool FbxLoader::_load() {

  RawModel raw;

  if (verboseOutput) {
    fmt::printf("Loading FBX File: %s\n", FilePath());
  }
  const GltfOptions& gltfOptions = GLTFOptions();
  if (!LoadFBXFile(raw, FilePath(), {"png", "jpg", "jpeg"}, gltfOptions)) {
    fmt::fprintf(stderr, "ERROR:: Failed to parse FBX: %s\n", FilePath());
    return false;
  }

  if (!FbxTextureTransforms().empty()) {
    raw.TransformTextures(FbxTextureTransforms());
  }
  raw.Condense();
  raw.TransformGeometry(gltfOptions.computeNormals);
  return true;
}