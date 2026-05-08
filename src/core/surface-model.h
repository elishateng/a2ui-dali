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

#ifndef DALI_GENERATIVE_UI_SURFACE_MODEL_H
#define DALI_GENERATIVE_UI_SURFACE_MODEL_H

#include "surface-components-model.h"
#include "data-model.h"
#include <dali-ui-foundation/devel-api/builder/json-parser.h>

namespace A2ui
{

/**
 * Manages a single Surface's state: components + data model.
 * Phase 1 supports a single surface. Phase 4 will add SurfaceGroupModel
 * for multi-surface management.
 */
class SurfaceModel
{
public:
  void Create(const std::string& surfaceId, const std::string& catalogId);

  void UpdateComponents(const Dali::Ui::TreeNode& componentsArray);

  void UpdateDataModel(const std::string& path, const Dali::Ui::TreeNode& value);

  void Delete();

  void Reset();

  const std::string& GetSurfaceId() const { return mSurfaceId; }
  const std::string& GetCatalogId() const { return mCatalogId; }
  bool IsCreated() const { return mCreated; }
  bool GetSendDataModel() const { return mSendDataModel; }
  void SetSendDataModel(bool v) { mSendDataModel = v; }

  SurfaceComponentsModel& GetComponentsModel() { return mComponents; }
  const SurfaceComponentsModel& GetComponentsModel() const { return mComponents; }

  DataModel& GetDataModel() { return mDataModel; }
  const DataModel& GetDataModel() const { return mDataModel; }

  size_t GetComponentCount() const { return mComponents.GetCount(); }

  /**
   * Get the JsonParser instances used for component parsing.
   * We keep parsers alive so TreeNode pointers in ComponentModel remain valid.
   */
  void KeepParser(Dali::Ui::JsonParser parser);

private:
  std::string            mSurfaceId;
  std::string            mCatalogId;
  bool                   mCreated = false;
  bool                   mSendDataModel = false;
  SurfaceComponentsModel mComponents;
  DataModel              mDataModel;

  // Keep parsers alive for TreeNode pointer lifetime
  std::vector<Dali::Ui::JsonParser> mParsers;
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_SURFACE_MODEL_H
