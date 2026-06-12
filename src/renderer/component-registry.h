#pragma once

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 */

#include "core/component-model.h"
#include "renderer/render-context.h"

#include <functional>
#include <string>
#include <unordered_map>

namespace A2ui
{

/// A component renderer: builds a DALi view for one A2UI component using the render context.
using ComponentRenderFn = std::function<Dali::Ui::View(const ComponentModel&, RenderContext&)>;

/**
 * @brief Maps an A2UI component-type string to its renderer (the catalog, in code).
 *
 * The standard catalog registers the built-in handlers; an app extends it for a custom
 * catalog by registering additional handlers under new type names (or overriding a
 * standard one) — the A2UI model is "basic handlers + extra registered handlers in the
 * same renderer", not a separate renderer per catalog.
 */
class ComponentRegistry
{
public:
  void Register(const std::string& type, ComponentRenderFn fn) { mFns[type] = std::move(fn); }

  /// @return the handler for @p type, or nullptr if none is registered.
  const ComponentRenderFn* Get(const std::string& type) const
  {
    auto it = mFns.find(type);
    return it == mFns.end() ? nullptr : &it->second;
  }

  bool   Has(const std::string& type) const { return mFns.count(type) != 0; }
  size_t Size() const { return mFns.size(); }

private:
  std::unordered_map<std::string, ComponentRenderFn> mFns;
};

} // namespace A2ui
