#include "loader/FbxLoader.h"

#include "fbx/Fbx2Raw.hpp"
#include "gltf/Raw2Gltf.hpp"

bool FbxLoader::Load() {
  return _load();
}

bool FbxLoader::_load() {
  // Initialize rawModelPtr_ with a new RawModel
  rawModelPtr_ = std::make_unique<RawModel>();

  if (verboseOutput) {
    fmt::printf("Loading FBX File: %s\n", FilePath());
  }
  const GltfOptions& gltfOptions = GLTFOptions();
  if (!LoadFBXFile(*rawModelPtr_, FilePath(), {"png", "jpg", "jpeg"}, gltfOptions)) {
    fmt::fprintf(stderr, "ERROR:: Failed to parse FBX: %s\n", FilePath());
    return false;
  }

  if (!FbxTextureTransforms().empty()) {
    rawModelPtr_->TransformTextures(FbxTextureTransforms());
  }
  rawModelPtr_->Condense();
  rawModelPtr_->TransformGeometry(gltfOptions.computeNormals);
  return true;
}