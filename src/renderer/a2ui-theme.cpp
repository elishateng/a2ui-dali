/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 */

#include "renderer/a2ui-theme.h"

#include <dali-ui-foundation/public-api/ui-color-manager.h>
#include <dali/integration-api/debug.h>
#include <unordered_map>

using namespace Dali;
using namespace Dali::Ui;

namespace A2ui
{
namespace
{
// Bundled OneUI light palette — values copied verbatim from OneUI's oneui_color.json.
// This is a stopgap until the platform supplies an OneUI ThemeLoader to dali-ui; it is
// NOT a redefinition of OneUI's tokens, just a local copy of the published values.
const std::unordered_map<std::string, uint32_t>& BundledOneUILight()
{
  static const std::unordered_map<std::string, uint32_t> kPalette = {
    {"Primary", 0x387AFF},
    {"PrimaryHigh", 0x2E65D4},
    {"Background", 0xF1F1F3},
    {"BackgroundApp", 0xFFFFFF},
    {"Surface", 0xFFFFFF},
    {"SurfaceContainerLow", 0xF1F1F3},
    {"SurfaceContainer", 0xE4E4E7},
    {"SurfaceContainerHigh", 0xD6D6D8},
    {"SurfaceContainerSemi", 0xE4E4E7},
    {"OnSurfaceContainerHighest", 0x010102},
    {"OnSurfaceContainer", 0x4D4D52},
    {"OnSurfaceContainerLow", 0x636368},
    {"OnSurfaceContainerFixed", 0xFCFCFF},
    {"Outline", 0xB7B7BB},
    {"OutlineLow", 0xD6D6D8},
    {"Red", 0xF64141},
    {"Green", 0x0BB075},
  };
  return kPalette;
}
} // namespace

UiColor A2uiTheme::Color(const std::string& id)
{
  // Bundled OneUI values are authoritative on this target (the platform OneUI
  // ThemeLoader is not yet wired into dali-ui here). When it is, flip the order so
  // UiColorManager wins — the call site code does not change.
  const auto& palette = BundledOneUILight();
  auto it = palette.find(id);
  if(it != palette.end())
  {
    return UiColor(it->second);
  }

  Vector4 v;
  if(UiColorManager::Get().GetColor(id.c_str(), v))
  {
    return UiColor(v);
  }

  DALI_LOG_ERROR("[A2UI][theme] unknown colour id '%s'\n", id.c_str());
  return UiColor(0xFF00FF); // magenta = missing token (loud, so it is noticed)
}

} // namespace A2ui
