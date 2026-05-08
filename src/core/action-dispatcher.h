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

#ifndef DALI_GENERATIVE_UI_ACTION_DISPATCHER_H
#define DALI_GENERATIVE_UI_ACTION_DISPATCHER_H

#include "data-context.h"
#include <string>
#include <functional>
#include <dali-ui-foundation/devel-api/builder/tree-node.h>

namespace A2ui
{

/**
 * Dispatches A2UI client actions.
 *
 * Resolves action context bindings and emits the serialized `action` JSON
 * through SendCallback. The caller is responsible for delivering the JSON
 * to the agent over whichever transport is in use (e.g. A2A, WebSocket).
 */
class ActionDispatcher
{
public:
  using SendCallback = std::function<void(const std::string& actionJson)>;

  void SetSurfaceId(const std::string& surfaceId) { mSurfaceId = surfaceId; }
  void SetSendDataModel(bool enabled) { mSendDataModel = enabled; }
  void SetSendCallback(SendCallback cb) { mSendCallback = std::move(cb); }

  /**
   * Dispatch an action from a component.
   * @param actionNode  The "action" JSON node from the component definition
   * @param sourceComponentId  ID of the component that triggered the action
   * @param ctx  Data context for resolving bound values in action context
   */
  void Dispatch(const Dali::Ui::TreeNode& actionNode,
                const std::string& sourceComponentId,
                const DataContext& ctx);

  bool GetSendDataModel() const { return mSendDataModel; }

private:
  std::string ResolveContextValue(const Dali::Ui::TreeNode& node,
                                  const DataContext& ctx) const;
  std::string BuildContextJson(const Dali::Ui::TreeNode& contextNode,
                               const DataContext& ctx) const;

  std::string  mSurfaceId;
  bool         mSendDataModel = false;
  SendCallback mSendCallback;
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_ACTION_DISPATCHER_H
