#include "exporters/TilesetExporter.h"

#include <fstream>

#include "fbx/Fbx2Raw.hpp"
#include "gltf/Raw2Gltf.hpp"
#include "loader/FbxLoader.h"
//
// Created by Wallance on 2025/10/14.
//
bool TilesetExporter::Export(
    const std::string& outputPath,
    std::vector<std::function<Vec2f(Vec2f)>>& texturesTransforms) {
  FbxLoader loader(FilePath(), texturesTransforms);
  std::ofstream outStream; // note: auto-flushes in destructor
  const auto streamStart = outStream.tellp();

  outStream.open(outputPath, std::ios::trunc | std::ios::ate | std::ios::out | std::ios::binary);
  if (outStream.fail()) {
    fmt::fprintf(stderr, "ERROR:: Couldn't open file for writing: %s\n", outputPath.c_str());
    return false;
  }
  std::unique_ptr<ModelData> data_render_model(
      Raw2Gltf(outStream, OutputFolder(), loader.FbxRawModel(), GLTFOptions()));

  return true;
}

TilesetExporter::~TilesetExporter() {}
