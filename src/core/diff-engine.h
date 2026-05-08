/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DALI_GENERATIVE_UI_DIFF_ENGINE_H
#define DALI_GENERATIVE_UI_DIFF_ENGINE_H

#include "component-model.h"
#include "surface-components-model.h"
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace A2ui
{

/**
 * Result of diffing old vs new component sets.
 */
struct ComponentDiff
{
  std::vector<std::string> added;     // Newly added component IDs
  std::vector<std::string> removed;   // Removed component IDs
  std::vector<std::string> modified;  // Changed component IDs (properties differ)
  std::vector<std::string> unchanged; // Same as before
};

/**
 * Tracks component-to-View mapping and computes minimal diffs
 * between successive updateComponents messages.
 */
class DiffEngine
{
public:
  /**
   * Compute diff between the current snapshot and new incoming components.
   * @param currentMap  Current components in the surface
   * @param newComponents  Newly received components array
   * @return ComponentDiff describing what changed
   */
  ComponentDiff ComputeDiff(
    const std::unordered_map<std::string, ComponentModel>& currentMap,
    const std::vector<ComponentModel>& newComponents) const;

  /**
   * Register a rendered View for a component ID.
   */
  void RegisterView(const std::string& componentId, Dali::Ui::View view);

  /**
   * Get the rendered View for a component ID.
   * @return View handle (may be empty if not registered)
   */
  Dali::Ui::View GetView(const std::string& componentId) const;

  /**
   * Check if a component has a registered View.
   */
  bool HasView(const std::string& componentId) const;

  /**
   * Remove the View mapping for a component ID.
   */
  void UnregisterView(const std::string& componentId);

  /**
   * Clear all View mappings.
   */
  void Clear();

  /**
   * Get all registered component IDs.
   */
  std::vector<std::string> GetRegisteredIds() const;

  /**
   * Store a snapshot of the current component properties for future diffing.
   * We hash the raw JSON string of each component for efficient comparison.
   */
  void TakeSnapshot(const SurfaceComponentsModel& components);

private:
  /**
   * Compute a simple hash of a component's properties for change detection.
   * Compares type, childIds, childId, and raw JSON content.
   */
  static std::string ComputeComponentHash(const ComponentModel& comp);

  // Component ID → rendered DALi View
  std::unordered_map<std::string, Dali::Ui::View> mViewMap;

  // Component ID → property hash (from last snapshot)
  std::unordered_map<std::string, std::string> mSnapshotHashes;
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_DIFF_ENGINE_H
