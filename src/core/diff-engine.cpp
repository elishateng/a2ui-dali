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

#include "diff-engine.h"
#include <functional>
#include <sstream>

using Dali::Ui::Integration::TreeNode;
using Dali::Ui::View;

namespace A2ui
{

namespace
{

void TreeNodeToString(const TreeNode& node, std::ostringstream& oss)
{
  switch(node.GetType())
  {
    case TreeNode::STRING:
      oss << '"' << node.GetString() << '"';
      break;
    case TreeNode::INTEGER:
      oss << node.GetInteger();
      break;
    case TreeNode::FLOAT:
      oss << node.GetFloat();
      break;
    case TreeNode::BOOLEAN:
      oss << (node.GetBoolean() ? "true" : "false");
      break;
    case TreeNode::OBJECT:
    {
      oss << '{';
      bool first = true;
      for(auto it = node.CBegin(); it != node.CEnd(); ++it)
      {
        if(!first) oss << ',';
        first = false;
        if((*it).first) oss << '"' << (*it).first << "\":";
        TreeNodeToString((*it).second, oss);
      }
      oss << '}';
      break;
    }
    case TreeNode::ARRAY:
    {
      oss << '[';
      bool first = true;
      for(auto it = node.CBegin(); it != node.CEnd(); ++it)
      {
        if(!first) oss << ',';
        first = false;
        TreeNodeToString((*it).second, oss);
      }
      oss << ']';
      break;
    }
    default:
      oss << "null";
      break;
  }
}

} // anonymous namespace

std::string DiffEngine::ComputeComponentHash(const ComponentModel& comp)
{
  std::ostringstream oss;
  oss << comp.type << '|';

  // Include children
  for(const auto& cid : comp.childIds)
  {
    oss << cid << ',';
  }
  oss << '|' << comp.childId << '|';

  // Include raw JSON properties if available
  if(comp.rawNode)
  {
    TreeNodeToString(*comp.rawNode, oss);
  }

  return oss.str();
}

ComponentDiff DiffEngine::ComputeDiff(
  const std::unordered_map<std::string, ComponentModel>& currentMap,
  const std::vector<ComponentModel>& newComponents) const
{
  ComponentDiff diff;

  // Build set of new IDs
  std::unordered_set<std::string> newIds;
  std::unordered_map<std::string, std::string> newHashes;

  for(const auto& comp : newComponents)
  {
    newIds.insert(comp.id);
    newHashes[comp.id] = ComputeComponentHash(comp);
  }

  // Find removed: in current but not in new
  for(const auto& [id, comp] : currentMap)
  {
    if(newIds.find(id) == newIds.end())
    {
      diff.removed.push_back(id);
    }
  }

  // Find added and modified
  for(const auto& comp : newComponents)
  {
    auto currentIt = currentMap.find(comp.id);
    if(currentIt == currentMap.end())
    {
      // New component
      diff.added.push_back(comp.id);
    }
    else
    {
      // Existing — check if modified using snapshot hashes
      auto snapshotIt = mSnapshotHashes.find(comp.id);
      if(snapshotIt == mSnapshotHashes.end() ||
         snapshotIt->second != newHashes[comp.id])
      {
        diff.modified.push_back(comp.id);
      }
      else
      {
        diff.unchanged.push_back(comp.id);
      }
    }
  }

  return diff;
}

void DiffEngine::RegisterView(const std::string& componentId, View view)
{
  mViewMap[componentId] = view;
}

View DiffEngine::GetView(const std::string& componentId) const
{
  auto it = mViewMap.find(componentId);
  if(it != mViewMap.end())
  {
    return it->second;
  }
  return View();
}

bool DiffEngine::HasView(const std::string& componentId) const
{
  return mViewMap.find(componentId) != mViewMap.end();
}

void DiffEngine::UnregisterView(const std::string& componentId)
{
  mViewMap.erase(componentId);
}

void DiffEngine::Clear()
{
  mViewMap.clear();
  mSnapshotHashes.clear();
}

std::vector<std::string> DiffEngine::GetRegisteredIds() const
{
  std::vector<std::string> ids;
  ids.reserve(mViewMap.size());
  for(const auto& [id, view] : mViewMap)
  {
    ids.push_back(id);
  }
  return ids;
}

void DiffEngine::TakeSnapshot(const SurfaceComponentsModel& components)
{
  mSnapshotHashes.clear();

  // Snapshot ALL components, not just those with registered views
  for(const auto& [id, comp] : components.GetAll())
  {
    mSnapshotHashes[id] = ComputeComponentHash(comp);
  }
}

} // namespace A2ui
