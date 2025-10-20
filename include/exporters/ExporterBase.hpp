#pragma once
#include <raw/RawModel.hpp>

class ExporterBase {
 public:
  virtual ~ExporterBase() = default;
  virtual bool Export(std::vector<std::function<Vec2f(Vec2f)>>& texturesTransforms) = 0;
};