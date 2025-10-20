//
// Created by Wallance on 2025/10/14.
//

#pragma once
#include "ExporterBase.hpp"
class TilesetExporter : public ExporterBase {
public:
  TilesetExporter(const std::string& filePath,
      const std::string& outputFolder,
      GltfOptions& glTFOptions)
        : filePath_(filePath), outputFolder_(outputFolder), glTFOptions_(glTFOptions) {}
  ~TilesetExporter() override = default;
  bool Export(std::vector<std::function<Vec2f(Vec2f)>>& texturesTransforms) override;
  std::string FilePath() const {
    return filePath_;
  }
  std::string OutputFolder() const {
    return outputFolder_;
  }
  GltfOptions GLTFOptions() const {
    return glTFOptions_;
  }
private:
  std::string filePath_;
  std::string outputFolder_;
  GltfOptions glTFOptions_;
};