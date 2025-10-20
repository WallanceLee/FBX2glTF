//
// Created by Wallance on 2025/10/14.
//
#pragma once
#include <functional>
#include <vector>

#include "mathfu.hpp"
#include "raw/RawModel.hpp"

class FbxLoader {
 public:
  FbxLoader(const std::string& filePath)
      : filePath_(filePath), textureTransforms_(std::vector<std::function<Vec2f(Vec2f)>>()) {
    _load();
  }
  FbxLoader(
      const std::string& filePath,
      std::vector<std::function<Vec2f(Vec2f)>>& textureTransforms)
      : filePath_(filePath), textureTransforms_(textureTransforms) {
    _load();
  }
  ~FbxLoader() = default;
  bool Load();
  RawModel& FbxRawModel() const {
    return *rawModelPtr_;
  }
  const std::string& FilePath() const {
    return filePath_;
  }
  const std::vector<std::function<Vec2f(Vec2f)>>& FbxTextureTransforms() const {
    return textureTransforms_;
  }
  const GltfOptions& GLTFOptions() const {
    return gltfOptions_;
  }

 private:
  std::unique_ptr<RawModel> rawModelPtr_;
  std::string filePath_;
  std::vector<std::function<Vec2f(Vec2f)>> textureTransforms_;
  GltfOptions gltfOptions_;
  bool _load();
};
