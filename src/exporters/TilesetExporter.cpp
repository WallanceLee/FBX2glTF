#include "exporters/TilesetExporter.h"

#include <fstream>

#include "gltf/GltfModel.hpp"
#include "gltf/Raw2Gltf.hpp"
#include "loader/FbxLoader.h"
//
// Created by Wallance on 2025/10/14.
//
bool TilesetExporter::Export(std::vector<std::function<Vec2f(Vec2f)>>& texturesTransforms) {
  FbxLoader loader(FilePath(), texturesTransforms);
  loader.Load();
  RawModel& raw = loader.FbxRawModel();

  std::ofstream outStream; // note: auto-flushes in destructor
  const auto streamStart = outStream.tellp();
  std::string outputPath = std::filesystem::path(outputFolder_) / "NoLod0.glb";
  outStream.open(outputPath, std::ios::trunc | std::ios::ate | std::ios::out | std::ios::binary);
  if (outStream.fail()) {
    fmt::fprintf(stderr, "ERROR:: Couldn't open file for writing: %s\n", outputPath.c_str());
    return false;
  }

  std::unique_ptr<GltfModel> gltf(new GltfModel(GLTFOptions()));
  std::shared_ptr<SceneData> sceneDataPtr = PrepareGltfModel(OutputFolder(), raw, GLTFOptions(), *gltf);

  WriteGLTF(outStream, GLTFOptions(), gltf, *sceneDataPtr);

  std::unique_ptr<ModelData> data_render_model = std::make_unique<ModelData>(ModelData(gltf->binary));

  if (GLTFOptions().outputBinary) {
    fmt::printf(
        "Wrote %lu bytes of binary glTF to %s.\n",
        static_cast<uint64_t>(outStream.tellp() - streamStart),
        outputPath);
    return true;
  }

  fmt::printf(
      "Wrote %lu bytes of glTF to %s.\n",
      static_cast<uint64_t>(outStream.tellp() - streamStart),
      outputPath);

  if (GLTFOptions().embedResources) {
    // we're done: everything was inlined into the glTF JSON
    return true;
  }

  assert(!OutputFolder().empty());

  const std::string binaryPath = OutputFolder() + extBufferFilename;
  std::ofstream fileStream(binaryPath, std::ios::binary);
  if (!fileStream.is_open()) {
    fmt::fprintf(stderr, "ERROR:: Couldn't open file '%s' for writing.\n", binaryPath);
    return false;
  }

  if (!data_render_model->binary->empty()) {
    const void* binaryData = static_cast<const void*>(data_render_model->binary->data());
    auto dataSize = static_cast<std::streamsize>(data_render_model->binary->size());
    fileStream.write(static_cast<const char*>(binaryData), dataSize);
    if (!fileStream.good()) {
      fmt::fprintf(
          stderr,
          "ERROR: Failed to write %lu bytes to file '%s'.\n",
          static_cast<uint64_t>(data_render_model->binary->size()),
          binaryPath);
      return false;
    }
    fmt::printf(
        "Wrote %lu bytes of binary data to %s.\n",
        static_cast<uint64_t>(data_render_model->binary->size()),
        binaryPath);
  }

  // 4. 生成 tileset.json
  // TODO: this just test
  std::ofstream jsonFile(std::filesystem::path(outputFolder_) / "tileset.json");
  jsonFile << R"({
      "asset": {"version": "1.0"},
      "geometricError": 1000,
      "root": {
        "boundingVolume": {"region": [0, 0, 0, 0, 0, 0]},
        "geometricError": 0,
        "refine": "ADD",
        "content": {"uri": "model.b3dm"}
      }
    })";

  return true;
}
