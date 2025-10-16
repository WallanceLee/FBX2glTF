#include "exporters/GLTFExporter.h"

#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <vector>
#include "fbx/Fbx2Raw.hpp"
#include "gltf/Raw2Gltf.hpp"
bool GLTFExporter::Export(
    const std::string& outputPath,
    std::vector<std::function<Vec2f(Vec2f)>>& texturesTransforms) {
  RawModel raw;

  if (verboseOutput) {
    fmt::printf("Loading FBX File: %s\n", FilePath());
  }
  if (!LoadFBXFile(raw, FilePath(), {"png", "jpg", "jpeg"}, GLTFOptions())) {
    fmt::fprintf(stderr, "ERROR:: Failed to parse FBX: %s\n", FilePath());
    return false;
  }

  if (!texturesTransforms.empty()) {
    raw.TransformTextures(texturesTransforms);
  }
  raw.Condense();
  raw.TransformGeometry(GLTFOptions().computeNormals);

  std::ofstream outStream; // note: auto-flushes in destructor
  const auto streamStart = outStream.tellp();

  outStream.open(outputPath, std::ios::trunc | std::ios::ate | std::ios::out | std::ios::binary);
  if (outStream.fail()) {
    fmt::fprintf(stderr, "ERROR:: Couldn't open file for writing: %s\n", outputPath.c_str());
    return false;
  }
  std::unique_ptr<ModelData> data_render_model(
      Raw2Gltf(outStream, OutputFolder(), raw, GLTFOptions()));

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

  return true;
}
