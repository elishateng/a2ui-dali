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

#ifndef DALI_GENERATIVE_UI_VIEW_POOL_H
#define DALI_GENERATIVE_UI_VIEW_POOL_H

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace A2ui
{

/**
 * Pools released Views by component type for reuse.
 * Reduces allocation overhead for frequently created/destroyed views.
 *
 * Usage:
 *   View v = pool.Acquire("Text");
 *   if(!v) v = Label::New(...);  // create fresh if pool empty
 *   // ... use v ...
 *   pool.Release("Text", v);     // return to pool when done
 */
class ViewPool
{
public:
  /**
   * @param maxPerType  Maximum pooled views per component type (default 20)
   */
  explicit ViewPool(size_t maxPerType = 20);

  /**
   * Try to acquire a pooled View for the given component type.
   * @return View handle (empty if no pooled view available)
   */
  Dali::Ui::View Acquire(const std::string& componentType);

  /**
   * Release a View back into the pool.
   * The view is unparented and reset before pooling.
   * If the pool for this type is full, the view is discarded.
   */
  void Release(const std::string& componentType, Dali::Ui::View view);

  /**
   * Clear all pooled views.
   */
  void Clear();

  /**
   * Get the number of pooled views for a given type.
   */
  size_t GetPooledCount(const std::string& componentType) const;

  /**
   * Get total number of pooled views across all types.
   */
  size_t GetTotalPooled() const;

private:
  size_t mMaxPerType;
  std::unordered_map<std::string, std::vector<Dali::Ui::View>> mPool;
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_VIEW_POOL_H
