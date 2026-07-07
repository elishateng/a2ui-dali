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

#include "surface-model.h"

namespace A2ui
{

void SurfaceModel::Create(const std::string& surfaceId, const std::string& catalogId)
{
  mSurfaceId = surfaceId;
  mCatalogId = catalogId;
  mCreated   = true;
  mComponents.Clear();
  mDataModel.Clear();
  mParsers.clear();
}

void SurfaceModel::UpdateComponents(const Dali::Ui::Integration::TreeNode& componentsArray)
{
  mComponents.AddComponents(componentsArray);
}

void SurfaceModel::UpdateDataModel(const std::string& path, const Dali::Ui::Integration::TreeNode& value)
{
  mDataModel.SetDataFromNode(path, value);
}

void SurfaceModel::Delete()
{
  Reset();
}

void SurfaceModel::Reset()
{
  mSurfaceId.clear();
  mCatalogId.clear();
  mCreated = false;
  mComponents.Clear();
  mDataModel.Clear();
  mParsers.clear();
}

void SurfaceModel::KeepParser(Dali::Ui::Integration::JsonParser parser)
{
  mParsers.push_back(parser);
}

} // namespace A2ui
