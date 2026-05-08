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

#include "data-model.h"
#include <dali/integration-api/debug.h>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>

using Dali::Ui::JsonParser;
using Dali::Ui::TreeNode;

namespace A2ui
{

// ========================================================================
// Data mutation
// ========================================================================

bool DataModel::SetData(const std::string& path, const std::string& jsonString)
{
  if(path == "/" || path.empty())
  {
    mJsonString = jsonString;
    mParser = JsonParser::New();
    if(!mParser.Parse(mJsonString))
    {
      DALI_LOG_ERROR("[A2UI] DataModel: JSON parse error: %s\n",
                     mParser.GetErrorDescription().c_str());
      mParser.Reset();
      return false;
    }
    NotifyObservers(path);
    return true;
  }

  if(MergeAtPath(path, jsonString))
  {
    NotifyObservers(path);
    return true;
  }
  return false;
}

bool DataModel::SetDataFromNode(const std::string& path, const TreeNode& value)
{
  std::string valueJson = TreeNodeToJson(value);

  if(path == "/" || path.empty())
  {
    return SetData("/", valueJson);
  }

  return SetData(path, valueJson);
}

bool DataModel::SetValue(const std::string& path, const std::string& value)
{
  // Wrap string value in quotes for JSON
  std::ostringstream oss;
  oss << "\"";
  EscapeJsonString(oss, value.c_str());
  oss << "\"";
  return SetData(path, oss.str());
}

bool DataModel::SetBoolValue(const std::string& path, bool value)
{
  return SetData(path, value ? "true" : "false");
}

bool DataModel::SetFloatValue(const std::string& path, float value)
{
  std::ostringstream oss;
  oss << value;
  return SetData(path, oss.str());
}

// ========================================================================
// Data reading
// ========================================================================

const TreeNode* DataModel::ResolvePath(const std::string& path) const
{
  if(!mParser)
  {
    return nullptr;
  }

  const TreeNode* root = mParser.GetRoot();
  if(!root)
  {
    return nullptr;
  }

  if(path == "/" || path.empty())
  {
    return root;
  }

  const TreeNode* current = root;
  size_t start = 1; // Skip leading '/'

  while(start < path.size())
  {
    size_t end = path.find('/', start);
    std::string segment;
    if(end == std::string::npos)
    {
      segment = path.substr(start);
      start = path.size();
    }
    else
    {
      segment = path.substr(start, end - start);
      start = end + 1;
    }

    if(segment.empty())
    {
      continue;
    }

    // Try as object key first
    const TreeNode* child = current->Find(segment.c_str());
    if(child)
    {
      current = child;
      continue;
    }

    // Try as array index
    if(current->GetType() == TreeNode::ARRAY)
    {
      char* endPtr = nullptr;
      long idx = strtol(segment.c_str(), &endPtr, 10);
      if(endPtr && *endPtr == '\0' && idx >= 0)
      {
        long count = 0;
        for(auto it = current->CBegin(); it != current->CEnd(); ++it)
        {
          if(count == idx)
          {
            current = &(*it).second;
            break;
          }
          count++;
        }
        if(count == idx)
        {
          continue;
        }
      }
    }

    return nullptr;
  }

  return current;
}

std::string DataModel::GetString(const std::string& path) const
{
  const TreeNode* node = ResolvePath(path);
  if(!node)
  {
    return "";
  }

  switch(node->GetType())
  {
    case TreeNode::STRING:
      return node->GetString();
    case TreeNode::INTEGER:
      return std::to_string(node->GetInteger());
    case TreeNode::FLOAT:
    {
      std::ostringstream oss;
      oss << node->GetFloat();
      return oss.str();
    }
    case TreeNode::BOOLEAN:
      return node->GetBoolean() ? "true" : "false";
    default:
      return "";
  }
}

float DataModel::GetFloat(const std::string& path, float fallback) const
{
  const TreeNode* node = ResolvePath(path);
  if(!node)
  {
    return fallback;
  }

  if(node->GetType() == TreeNode::FLOAT)
    return node->GetFloat();
  if(node->GetType() == TreeNode::INTEGER)
    return static_cast<float>(node->GetInteger());

  return fallback;
}

bool DataModel::GetBool(const std::string& path, bool fallback) const
{
  const TreeNode* node = ResolvePath(path);
  if(!node)
  {
    return fallback;
  }

  if(node->GetType() == TreeNode::BOOLEAN)
    return node->GetBoolean();

  // Also accept string "true"/"false"
  if(node->GetType() == TreeNode::STRING)
  {
    const char* str = node->GetString();
    if(strcmp(str, "true") == 0) return true;
    if(strcmp(str, "false") == 0) return false;
  }

  return fallback;
}

std::string DataModel::Serialize() const
{
  if(!mParser || !mParser.GetRoot())
  {
    return "{}";
  }
  return TreeNodeToJson(*mParser.GetRoot());
}

// ========================================================================
// Observer pattern
// ========================================================================

uint32_t DataModel::Watch(const std::string& path, DataChangeCallback cb)
{
  uint32_t id = mNextObserverId++;
  if(id == 0) id = mNextObserverId++; // skip 0

  if(mNotifying)
  {
    // Defer addition until notification loop completes
    mPendingAdds.push_back({id, path, std::move(cb)});
  }
  else
  {
    mObservers.push_back({id, path, std::move(cb)});
  }
  return id;
}

void DataModel::Unwatch(uint32_t observerId)
{
  if(mNotifying)
  {
    // Defer removal until notification loop completes
    mPendingRemoves.push_back(observerId);
  }
  else
  {
    mObservers.erase(
      std::remove_if(mObservers.begin(), mObservers.end(),
        [observerId](const Observer& o) { return o.id == observerId; }),
      mObservers.end());
  }
}

static bool IsHierarchicalMatch(const std::string& a, const std::string& b)
{
  // Check if 'a' is a hierarchical prefix of 'b'.
  // "/contact" matches "/contact/name" but NOT "/contactName"
  if(b.find(a) != 0) return false;
  if(b.size() == a.size()) return true; // exact match
  return b[a.size()] == '/'; // next char must be separator
}

void DataModel::NotifyObservers(const std::string& changedPath)
{
  if(mNotifying)
  {
    return; // prevent re-entrancy
  }
  mNotifying = true;

  // Use index-based iteration to be safe if observers are modified during callbacks
  for(size_t i = 0; i < mObservers.size(); ++i)
  {
    const auto& obs = mObservers[i];
    bool notify = false;

    if(changedPath == "/" || changedPath.empty())
    {
      notify = true;
    }
    else if(obs.path == changedPath)
    {
      notify = true;
    }
    else if(IsHierarchicalMatch(obs.path, changedPath))
    {
      notify = true; // changed "/contact/name", observer "/contact"
    }
    else if(IsHierarchicalMatch(changedPath, obs.path))
    {
      notify = true; // changed "/contact", observer "/contact/name"
    }

    if(notify)
    {
      std::string newValue = GetString(obs.path);
      obs.callback(obs.path, newValue);
    }
  }

  mNotifying = false;

  // Apply deferred modifications
  for(auto id : mPendingRemoves)
  {
    mObservers.erase(
      std::remove_if(mObservers.begin(), mObservers.end(),
        [id](const Observer& o) { return o.id == id; }),
      mObservers.end());
  }
  mPendingRemoves.clear();

  for(auto& obs : mPendingAdds)
  {
    mObservers.push_back(std::move(obs));
  }
  mPendingAdds.clear();
}

void DataModel::Clear()
{
  if(mParser)
  {
    mParser.Reset();
  }
  mJsonString.clear();
  mObservers.clear();
  mPendingAdds.clear();
  mPendingRemoves.clear();
  mNextObserverId = 1;
}

void DataModel::ClearObservers()
{
  mObservers.clear();
  mPendingAdds.clear();
  mPendingRemoves.clear();
}

// ========================================================================
// Internal merge
// ========================================================================

bool DataModel::MergeAtPath(const std::string& path, const std::string& valueJson)
{
  if(!mParser || !mParser.GetRoot())
  {
    // No existing data - build nested structure from path
    // e.g., path="/contact/firstName", value="\"John\"" -> {"contact":{"firstName":"John"}}
    std::vector<std::string> segments;
    size_t start = 1;
    while(start < path.size())
    {
      size_t end = path.find('/', start);
      if(end == std::string::npos)
      {
        segments.push_back(path.substr(start));
        break;
      }
      segments.push_back(path.substr(start, end - start));
      start = end + 1;
    }

    if(segments.empty())
    {
      return false;
    }

    // Build nested JSON from inside out
    std::string json = valueJson;
    for(int i = static_cast<int>(segments.size()) - 1; i >= 0; --i)
    {
      json = "{\"" + segments[i] + "\":" + json + "}";
    }

    mJsonString = json;
    mParser = JsonParser::New();
    if(!mParser.Parse(mJsonString))
    {
      DALI_LOG_ERROR("[A2UI] DataModel: MergeAtPath parse error\n");
      mParser.Reset();
      return false;
    }
    return true;
  }

  // Existing data: rebuild JSON with the path updated
  const TreeNode* root = mParser.GetRoot();

  // Parse path segments
  std::vector<std::string> segments;
  size_t start = 1;
  while(start < path.size())
  {
    size_t end = path.find('/', start);
    if(end == std::string::npos)
    {
      segments.push_back(path.substr(start));
      break;
    }
    segments.push_back(path.substr(start, end - start));
    start = end + 1;
  }

  if(segments.empty())
  {
    return false;
  }

  // For single-level paths, use the optimized approach
  if(segments.size() == 1)
  {
    const std::string& key = segments[0];
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    bool found = false;

    if(root->GetType() == TreeNode::OBJECT)
    {
      for(auto it = root->CBegin(); it != root->CEnd(); ++it)
      {
        if(!first) oss << ",";
        first = false;

        const char* name = (*it).first;
        if(name && strcmp(name, key.c_str()) == 0)
        {
          oss << "\"";
          EscapeJsonString(oss, name);
          oss << "\":" << valueJson;
          found = true;
        }
        else if(name)
        {
          oss << "\"";
          EscapeJsonString(oss, name);
          oss << "\":" << TreeNodeToJson((*it).second);
        }
      }
    }

    if(!found)
    {
      if(!first) oss << ",";
      oss << "\"" << key << "\":" << valueJson;
    }

    oss << "}";

    mJsonString = oss.str();
    mParser = JsonParser::New();
    if(!mParser.Parse(mJsonString))
    {
      DALI_LOG_ERROR("[A2UI] DataModel: MergeAtPath re-parse error\n");
      mParser.Reset();
      return false;
    }
    return true;
  }

  // Multi-level path: serialize the whole tree, updating the nested value
  // Strategy: serialize root, but intercept at the first segment level
  // and recursively handle deeper segments
  std::string rootJson = TreeNodeToJson(*root);

  // Re-parse into a working copy, then do recursive merge
  // Approach: build the intermediate objects from the path
  // First, get or create the top-level key
  std::string topKey = segments[0];

  // Build the nested value from segments[1..] wrapping valueJson
  std::string nestedValue = valueJson;
  for(int i = static_cast<int>(segments.size()) - 1; i >= 1; --i)
  {
    // Check if intermediate node exists
    std::string intermediatePath = "/";
    for(int j = 0; j <= i - 1; ++j)
    {
      if(j > 0) intermediatePath += "/";
      intermediatePath += segments[j];
    }
    const TreeNode* intermediateNode = ResolvePath(intermediatePath);

    if(intermediateNode && intermediateNode->GetType() == TreeNode::OBJECT)
    {
      // Merge into existing object
      std::ostringstream oss;
      oss << "{";
      bool first = true;
      bool found = false;

      for(auto it = intermediateNode->CBegin(); it != intermediateNode->CEnd(); ++it)
      {
        if(!first) oss << ",";
        first = false;

        const char* name = (*it).first;
        if(name && strcmp(name, segments[i].c_str()) == 0)
        {
          oss << "\"";
          EscapeJsonString(oss, name);
          oss << "\":" << nestedValue;
          found = true;
        }
        else if(name)
        {
          oss << "\"";
          EscapeJsonString(oss, name);
          oss << "\":" << TreeNodeToJson((*it).second);
        }
      }

      if(!found)
      {
        if(!first) oss << ",";
        oss << "\"" << segments[i] << "\":" << nestedValue;
      }

      oss << "}";
      nestedValue = oss.str();
    }
    else
    {
      // Create new intermediate object
      nestedValue = "{\"" + segments[i] + "\":" + nestedValue + "}";
    }
  }

  // Now merge nestedValue at top-level key
  return MergeAtPath("/" + topKey, nestedValue);
}

// ========================================================================
// Serialization helpers
// ========================================================================

void DataModel::EscapeJsonString(std::ostringstream& oss, const char* str)
{
  while(*str)
  {
    switch(*str)
    {
      case '"':  oss << "\\\""; break;
      case '\\': oss << "\\\\"; break;
      case '\n': oss << "\\n";  break;
      case '\r': oss << "\\r";  break;
      case '\t': oss << "\\t";  break;
      default:   oss << *str;   break;
    }
    ++str;
  }
}

std::string DataModel::TreeNodeToJson(const TreeNode& node)
{
  std::ostringstream oss;

  switch(node.GetType())
  {
    case TreeNode::OBJECT:
    {
      oss << "{";
      bool first = true;
      for(auto it = node.CBegin(); it != node.CEnd(); ++it)
      {
        if(!first) oss << ",";
        first = false;
        if((*it).first)
        {
          oss << "\"";
          EscapeJsonString(oss, (*it).first);
          oss << "\":";
        }
        oss << TreeNodeToJson((*it).second);
      }
      oss << "}";
      break;
    }
    case TreeNode::ARRAY:
    {
      oss << "[";
      bool first = true;
      for(auto it = node.CBegin(); it != node.CEnd(); ++it)
      {
        if(!first) oss << ",";
        first = false;
        oss << TreeNodeToJson((*it).second);
      }
      oss << "]";
      break;
    }
    case TreeNode::STRING:
    {
      oss << "\"";
      EscapeJsonString(oss, node.GetString());
      oss << "\"";
      break;
    }
    case TreeNode::INTEGER:
      oss << node.GetInteger();
      break;
    case TreeNode::FLOAT:
      oss << node.GetFloat();
      break;
    case TreeNode::BOOLEAN:
      oss << (node.GetBoolean() ? "true" : "false");
      break;
    default:
      oss << "null";
      break;
  }

  return oss.str();
}

} // namespace A2ui
