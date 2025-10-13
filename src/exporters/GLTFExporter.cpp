#include "exporters/GLTFExporter.h"

#include <fstream>
#include <functional>
#include <string>
#include <vector>
#include "fbx/Fbx2Raw.hpp"
#include "gltf/Raw2Gltf.hpp"
bool GLTFExporter::Export(
    const std::string& outputPath,
    std::vector<std::function<Vec2f(Vec2f)>>& texturesTransforms) {
  ModelData* data_render_model = nullptr;
  RawModel raw;

  if (verboseOutput) {
    fmt::printf("Loading FBX File: %s\n", FilePath());
  }
  if (!LoadFBXFile(raw,  FilePath(), {"png", "jpg", "jpeg"}, GLTFOptions())) {
    fmt::fprintf(stderr, "ERROR:: Failed to parse FBX: %s\n", FilePath());
    return 1;
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
    return 1;
  }
  data_render_model = Raw2Gltf(outStream, OutputFolder(), raw, GLTFOptions());

  if (GLTFOptions().outputBinary) {
    fmt::printf(
        "Wrote %lu bytes of binary glTF to %s.\n",
        (unsigned long)(outStream.tellp() - streamStart),
        outputPath);
    delete data_render_model;
    return 0;
  }

  fmt::printf(
      "Wrote %lu bytes of glTF to %s.\n",
      (unsigned long)(outStream.tellp() - streamStart),
      outputPath);

  if (GLTFOptions().embedResources) {
    // we're done: everything was inlined into the glTF JSON
    delete data_render_model;
    return 0;
  }

  assert(!OutputFolder().empty());

  const std::string binaryPath = OutputFolder() + extBufferFilename;
  FILE* fp = fopen(binaryPath.c_str(), "wb");
  if (fp == nullptr) {
    fmt::fprintf(stderr, "ERROR:: Couldn't open file '%s' for writing.\n", binaryPath);
    return false;
  }

  if (data_render_model->binary->empty() == false) {
    const unsigned char* binaryData = &(*data_render_model->binary)[0];
    unsigned long binarySize = data_render_model->binary->size();
    if (fwrite(binaryData, binarySize, 1, fp) != 1) {
      fmt::fprintf(
          stderr, "ERROR: Failed to write %lu bytes to file '%s'.\n", binarySize, binaryPath);
      fclose(fp);
      return false;
    }
    fclose(fp);
    fmt::printf("Wrote %lu bytes of binary data to %s.\n", binarySize, binaryPath);
  }

  delete data_render_model;
  return true;
}
