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

#ifndef DALI_GENERATIVE_UI_A2UI_RENDERER_H
#define DALI_GENERATIVE_UI_A2UI_RENDERER_H

#include "../core/surface-model.h"
#include "../core/data-context.h"
#include "../core/expression-parser.h"
#include "../core/action-dispatcher.h"
#include "../core/diff-engine.h"
#include "view-pool.h"
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali/public-api/events/tap-gesture-detector.h>
#include <string>
#include <vector>

namespace A2ui
{

/**
 * Converts SurfaceComponentsModel + DataModel into a DALi View tree.
 *
 * Supports the standard A2UI catalog:
 *   Layout : Row, Column, Card, List, Tabs, Modal, Divider
 *   Text   : Text, Icon, Image
 *   Inputs : Button, TextField, CheckBox, ChoicePicker, Slider, DateTimeInput
 */
class A2uiRenderer : public Dali::ConnectionTracker
{
public:
  A2uiRenderer();

  void SetImageDir(const std::string& imageDir) { mImageDir = imageDir; }

  /**
   * Render the surface model into a DALi View tree.
   * DataModel is non-const because input components write back to it.
   */
  Dali::Ui::View Render(SurfaceModel& surface);

  ActionDispatcher& GetActionDispatcher() { return mActionDispatcher; }
  DiffEngine& GetDiffEngine() { return mDiffEngine; }
  ViewPool& GetViewPool() { return mViewPool; }

private:
  Dali::Ui::View RenderComponent(const std::string& id,
                                 const SurfaceComponentsModel& components,
                                 DataContext& ctx);

  // === Layout & visual components ===
  Dali::Ui::View RenderText(const ComponentModel& comp, DataContext& ctx);
  Dali::Ui::View RenderFlexContainer(const ComponentModel& comp,
                                     const SurfaceComponentsModel& components,
                                     DataContext& ctx,
                                     Dali::Ui::FlexDirection direction);
  Dali::Ui::View RenderCard(const ComponentModel& comp,
                            const SurfaceComponentsModel& components,
                            DataContext& ctx);
  Dali::Ui::View RenderButton(const ComponentModel& comp,
                              const SurfaceComponentsModel& components,
                              DataContext& ctx);
  Dali::Ui::View RenderImage(const ComponentModel& comp, DataContext& ctx);
  Dali::Ui::View RenderDivider(const ComponentModel& comp);
  Dali::Ui::View RenderPlaceholder(const ComponentModel& comp);

  // === Input components ===
  Dali::Ui::View RenderTextField(const ComponentModel& comp, DataContext& ctx);
  Dali::Ui::View RenderCheckBox(const ComponentModel& comp, DataContext& ctx);
  Dali::Ui::View RenderChoicePicker(const ComponentModel& comp, DataContext& ctx);
  Dali::Ui::View RenderSlider(const ComponentModel& comp, DataContext& ctx);
  Dali::Ui::View RenderDateTimeInput(const ComponentModel& comp, DataContext& ctx);

  // === Layout containers ===
  Dali::Ui::View RenderTabs(const ComponentModel& comp,
                            const SurfaceComponentsModel& components,
                            DataContext& ctx);
  Dali::Ui::View RenderModal(const ComponentModel& comp,
                             const SurfaceComponentsModel& components,
                             DataContext& ctx);
  Dali::Ui::View RenderList(const ComponentModel& comp,
                            const SurfaceComponentsModel& components,
                            DataContext& ctx);
  Dali::Ui::View RenderIcon(const ComponentModel& comp, DataContext& ctx);

  // === Checks validation ===
  void SetupChecks(const ComponentModel& comp, DataContext& ctx,
                   Dali::Ui::Label errorLabel, const std::string& boundPath);

  // === Data binding helpers ===
  std::string ResolveString(const Dali::Ui::TreeNode* propNode, const DataContext& ctx) const;
  float       ResolveFloat(const Dali::Ui::TreeNode* propNode, const DataContext& ctx,
                           float fallback = 0.0f) const;
  std::string GetBoundPath(const Dali::Ui::TreeNode* propNode, const DataContext& ctx) const;

  // === Property access helpers ===
  static const char* GetNodeString(const Dali::Ui::TreeNode& node, const char* key,
                                   const char* fallback = "");
  static float GetNodeFloat(const Dali::Ui::TreeNode& node, const char* key,
                            float fallback = 0.0f);
  static float VariantToFontSize(const char* variant);
  static Dali::Ui::UiColor ParseHexColor(const char* hex);

  std::string        mImageDir;
  ExpressionParser   mExprParser;
  ActionDispatcher   mActionDispatcher;
  DiffEngine         mDiffEngine;
  ViewPool           mViewPool;

  // Owns TapGestureDetector handles for the current surface. dali-ui has no
  // native Button; we make Button/Card layouts tappable by attaching a
  // detector and must keep the handles alive for as long as the view tree.
  std::vector<Dali::TapGestureDetector> mTapDetectors;

  // Pointer to current components model (valid only during Render call)
  const SurfaceComponentsModel* mCurrentComponents = nullptr;
  DataContext*                   mCurrentCtx = nullptr;
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_A2UI_RENDERER_H
