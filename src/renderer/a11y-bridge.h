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

#ifndef DALI_GENERATIVE_UI_A11Y_BRIDGE_H
#define DALI_GENERATIVE_UI_A11Y_BRIDGE_H

#include "../core/component-model.h"
#include "../core/data-context.h"
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <string>

namespace A2ui
{

/**
 * Maps A2UI component types to AT-SPI2 accessibility roles and properties.
 *
 * Uses DALi custom properties to expose accessibility info.
 * The AT-SPI2 bridge in DALi adaptor can read these for screen readers.
 *
 * Mapping:
 *   Text        → LABEL
 *   Button      → PUSH_BUTTON
 *   TextField   → ENTRY
 *   CheckBox    → CHECK_BOX
 *   Slider      → SLIDER
 *   Image       → IMAGE
 *   Row/Column  → PANEL
 *   Card        → GROUP
 *   Tabs        → PAGE_TAB_LIST
 *   Modal       → DIALOG
 *   List        → LIST
 *   Icon        → ICON
 */
class A11yBridge
{
public:
  /**
   * Apply accessibility properties to a rendered View based on its component type.
   * @param view  The rendered DALi View
   * @param comp  The A2UI component model
   * @param ctx   Data context for resolving bound values
   */
  static void Apply(Dali::Ui::View& view, const ComponentModel& comp, const DataContext& ctx);

  /**
   * Set a custom accessibility label on a view.
   */
  static void SetAccessibleName(Dali::Ui::View& view, const std::string& name);

  /**
   * Set accessibility description.
   */
  static void SetAccessibleDescription(Dali::Ui::View& view, const std::string& desc);

private:
  static std::string ResolveLabel(const ComponentModel& comp, const DataContext& ctx);
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_A11Y_BRIDGE_H
