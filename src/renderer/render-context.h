#pragma once

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 */

#include "core/data-context.h"
#include "core/surface-components-model.h"

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali/public-api/events/tap-gesture-detector.h>
#include <functional>
#include <string>
#include <vector>

namespace A2ui
{
class ActionDispatcher;
class ExpressionParser;

/**
 * @brief Per-render services handed to every component renderer.
 *
 * Bundles the shared renderer state a component handler needs plus a callback to render a
 * child component by id, so a handler (including an app's custom-catalog handler) can
 * build its view tree without depending on the A2uiRenderer class directly.
 */
struct RenderContext
{
  DataContext&                           data;               ///< input components write back here
  const SurfaceComponentsModel&          components;         ///< child component lookup
  ActionDispatcher&                      actionDispatcher;   ///< outgoing user actions
  ExpressionParser&                      exprParser;         ///< bindings / checks evaluation
  const std::string&                     imageDir;           ///< local image/icon resolution root
  std::vector<Dali::TapGestureDetector>& tapDetectors;       ///< keep tap handles alive
  bool&                                  imageThumbnailHint; ///< Row→Image "this is a thumbnail" hint
  std::function<Dali::Ui::View(const std::string& childId)> renderChild;

  /// Render a child component by id (re-enters the dispatcher).
  Dali::Ui::View RenderChild(const std::string& id) const { return renderChild(id); }

  // Resolve a component property that may be a literal or a {path}/{call} data binding.
  std::string ResolveString(const Dali::Ui::Integration::TreeNode* propNode) const;
  float       ResolveFloat(const Dali::Ui::Integration::TreeNode* propNode, float fallback = 0.0f) const;
  /// The resolved data-model path a {path} binding points at (empty if not a binding).
  std::string GetBoundPath(const Dali::Ui::Integration::TreeNode* propNode) const;
};

} // namespace A2ui
