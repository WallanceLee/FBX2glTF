/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gltf/properties/TextureData.hpp>

#include <gltf/properties/ImageData.hpp>
#include <gltf/properties/SamplerData.hpp>

TextureData::TextureData(std::string name, const SamplerData& sampler, const ImageData& source)
    : Holdable(), name(std::move(name)), sampler(sampler.ix), source(source.ix) {}

json TextureData::serialize() const {
  return {{"name", name}, {"sampler", sampler}, {"source", source}};
}
