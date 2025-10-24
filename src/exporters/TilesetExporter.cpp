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
  std::shared_ptr<SceneData> sceneDataPtr =
      PrepareGltfModel(OutputFolder(), raw, GLTFOptions(), *gltf);

  WriteGLTF(outStream, GLTFOptions(), gltf, *sceneDataPtr);

  std::unique_ptr<ModelData> data_render_model =
      std::make_unique<ModelData>(ModelData(gltf->binary));

  if (GLTFOptions().outputBinary) {
    fmt::printf(
        "Wrote %lu bytes of binary glTF to %s.\n",
        static_cast<uint64_t>(outStream.tellp() - streamStart),
        outputPath);
  }

  assert(!OutputFolder().empty());

  const std::string binaryPath = std::filesystem::path(OutputFolder()) / extBufferFilename;
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
  // 使用模型的实际边界框数据
  const RawNode& rootNode = raw.GetNode(raw.GetRootNode());

  // 计算整个模型的包围盒（合并所有surface的bounds）
  Bounds<float, 3> modelBounds;
  modelBounds.Clear();

  // 遍历所有surface，合并它们的bounds
  for (int i = 0; i < raw.GetSurfaceCount(); i++) {
    const RawSurface& surface = raw.GetSurface(i);
    if (surface.bounds.initialized) {
      // 将surface的min点加入模型bounds
      Vec3f minPoint(surface.bounds.min[0], surface.bounds.min[1], surface.bounds.min[2]);
      modelBounds.AddPoint(minPoint);

      // 将surface的max点加入模型bounds
      Vec3f maxPoint(surface.bounds.max[0], surface.bounds.max[1], surface.bounds.max[2]);
      modelBounds.AddPoint(maxPoint);
    }
  }

  // 如果没有初始化，则使用默认值
  if (!modelBounds.initialized) {
    modelBounds.min = {0.0f, 0.0f, 0.0f};
    modelBounds.max = {0.0f, 0.0f, 0.0f};
  }

  // 定义bboxMin和bboxMax变量
  Vec3f bboxMin(modelBounds.min[0], modelBounds.min[1], modelBounds.min[2]);
  Vec3f bboxMax(modelBounds.max[0], modelBounds.max[1], modelBounds.max[2]);

  // 计算包围盒中心点和半尺寸
  Vec3f center = (bboxMin + bboxMax) * 0.5f;
  Vec3f size = (bboxMax - bboxMin) * 0.5f;

  // 构建3D Tiles包围盒数组 [ centerX, centerY, centerZ, halfSizeX, 0, 0, 0, halfSizeY, 0, 0, 0,
  // halfSizeZ ]
  // 修正：使用正确的3D Tiles包围盒格式
  // 3D Tiles包围盒格式：[centerX, centerY, centerZ, halfX, 0, 0, 0, halfY, 0, 0, 0, halfZ]
  std::array<float, 12> boundingBox = {
      center[0], center[1], center[2],  // 包围盒中心点
      size[0], 0.0f, 0.0f,              // X轴半尺寸向量
      0.0f, size[1], 0.0f,              // Y轴半尺寸向量
      0.0f, 0.0f, size[2]               // Z轴半尺寸向量
  };

  // 修改为正确的坐标系变换矩阵
  // 将Y轴旋转90度，使FBX的Y轴对应Cesium的Z轴
  // 并添加平移到指定地理坐标
  const double longitude = 119.911269; // 经度
  const double latitude = 32.454958; // 纬度
  const double height = 100.0; // 海拔高度（米）- 增加高度以确保模型在地面上方可见

  // WGS84椭球体参数
  const double a = 6378137.0; // 长半轴
  const double f = 1.0 / 298.257223563; // 扁率
  const double b = a * (1.0 - f); // 短半轴

  // 将经纬度转换为笛卡尔坐标
  // 使用WGS84椭球体计算（修复：正确使用短半轴b相关的公式）
  double latRad = latitude * M_PI / 180.0;
  double lonRad = longitude * M_PI / 180.0;

  // 正确的WGS84公式：使用短半轴b计算
  double e2 = 1.0 - (b * b) / (a * a); // 第一偏心率平方，通过短半轴b计算
  double N = a / sqrt(1.0 - e2 * sin(latRad) * sin(latRad));
  double x = (N + height) * cos(latRad) * cos(lonRad);
  double y = (N + height) * cos(latRad) * sin(lonRad);
  double z = (N * (b * b) / (a * a) + height) * sin(latRad); // 使用短半轴b直接计算

  // 计算局部东-北-天(ENU)坐标系到地心地固(ECEF)坐标系的变换矩阵
  double sinLat = sin(latRad);
  double cosLat = cos(latRad);
  double sinLon = sin(lonRad);
  double cosLon = cos(lonRad);

  // ENU到ECEF的变换矩阵（行主序）
  double enuToEcef[9] = {
      -sinLon, -sinLat * cosLon, cosLat * cosLon,
       cosLon, -sinLat * sinLon, cosLat * sinLon,
       0.0,     cosLat,          sinLat
  };

  // 构建完整的4x4变换矩阵（列主序）
  // 这个矩阵将模型从局部坐标系变换到ECEF坐标系
  // 确保模型的Z轴（上方向）与地面法线对齐
  std::array<double, 16> transformMatrix = {
      enuToEcef[0], enuToEcef[3], enuToEcef[6], 0.0,
      enuToEcef[1], enuToEcef[4], enuToEcef[7], 0.0,
      enuToEcef[2], enuToEcef[5], enuToEcef[8], 0.0,
      x, y, z, 1.0
  };

  std::string tilesetPath = std::filesystem::path(outputFolder_) / "tileset.json";
  std::ofstream jsonFile;
  jsonFile.open(tilesetPath, std::ios::trunc | std::ios::binary | std::ios::out | std::ios::ate);
  if (!jsonFile.is_open()) {
    fmt::fprintf(stderr, "ERROR:: Couldn't open file '%s' for writing.\n", tilesetPath.c_str());
    return false;
  }

  // Build the transform matrix string with proper decimal formatting
  std::string transformStr = "";
  for (int i = 0; i < 16; i++) {
    if (i > 0)
      transformStr += ", ";
    transformStr += fmt::format("{:.6f}", transformMatrix[i]);
  }

  // Build the bounding box string with proper decimal formatting
  std::string boundingBoxStr = "";
  for (int i = 0; i < 12; i++) {
    if (i > 0)
      boundingBoxStr += ", ";
    boundingBoxStr += fmt::format("{:.6f}", boundingBox[i]);
  }

  jsonFile << "{\n"
           << "  \"asset\": {\"version\": \"1.0\"},\n"
           << "  \"geometricError\": 1000,\n"
           << "  \"root\": {\n"
           << "    \"transform\": [" << transformStr << "],\n"
           << "    \"boundingVolume\": {\"box\": [" << boundingBoxStr << "]},\n"
           << "    \"geometricError\": 0,\n"
           << "    \"refine\": \"ADD\",\n"
           << "    \"content\": {\"uri\": \"./NoLod0.glb\"}\n"
           << "  }\n"
           << "}";

  if (!jsonFile.good()) {
    fmt::fprintf(stderr, "ERROR: Failed to write to file '%s'.\n", tilesetPath.c_str());
    return false;
  }

  fmt::printf("Wrote 3D Tiles tileset to %s.\n", tilesetPath.c_str());

  return true;
}