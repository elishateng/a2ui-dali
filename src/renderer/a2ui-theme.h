#pragma once

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <string>

namespace A2ui
{

/**
 * @brief Semantic OneUI colour tokens for the renderer.
 *
 * The token NAMES and the theming system are owned by dali-ui (UiColorManager /
 * ThemeLoaderInterface). a2ui-dali does NOT define its own design tokens; it
 * consumes dali-ui's by colour id ("Primary", "Outline", "SurfaceContainerLow"...).
 *
 * Until the platform supplies an OneUI ThemeLoader on this target, A2uiTheme resolves
 * from a small BUNDLED OneUI light palette (values lifted verbatim from OneUI's
 * oneui_color.json). When dali-ui's theme manager returns a value for an id, that
 * wins — so this migrates automatically once dali-ui supplies the theme. Nothing here
 * is a new token definition; it is a stopgap loader.
 */
class A2uiTheme
{
public:
  /// Resolve a OneUI colour id to a UiColor (dali-ui theme first, bundled palette fallback).
  static Dali::Ui::UiColor Color(const std::string& id);
};

} // namespace A2ui
