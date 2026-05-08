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

#include "action-dispatcher.h"
#include <dali/integration-api/debug.h>
#include <sstream>
#include <cstring>

using Dali::Ui::TreeNode;

namespace A2ui
{

namespace
{
void EscapeJsonStr(std::ostringstream& oss, const std::string& str)
{
  for(char c : str)
  {
    switch(c)
    {
      case '"':  oss << "\\\""; break;
      case '\\': oss << "\\\\"; break;
      case '\n': oss << "\\n";  break;
      case '\r': oss << "\\r";  break;
      case '\t': oss << "\\t";  break;
      default:   oss << c;      break;
    }
  }
}
} // anonymous namespace

void ActionDispatcher::Dispatch(const TreeNode& actionNode,
                                const std::string& sourceComponentId,
                                const DataContext& ctx)
{
  // Extract event definition
  const TreeNode* eventNode = actionNode.Find("event");
  if(!eventNode) return;

  const TreeNode* nameNode = eventNode->Find("name");
  if(!nameNode || nameNode->GetType() != TreeNode::STRING) return;

  std::string actionName = nameNode->GetString();

  // Build A2UI ClientEvent payload. The key is "userAction" to match the
  // ADK sample (restaurant_finder/agent_executor.py extracts
  // `data["userAction"]`). Some earlier drafts of the spec used "action" —
  // align with the upstream sample here.
  std::ostringstream oss;
  oss << "{\"userAction\":{";
  oss << "\"name\":\""; EscapeJsonStr(oss, actionName); oss << "\"";
  oss << ",\"surfaceId\":\""; EscapeJsonStr(oss, mSurfaceId); oss << "\"";
  oss << ",\"sourceComponentId\":\""; EscapeJsonStr(oss, sourceComponentId); oss << "\"";

  // Resolve context bindings
  const TreeNode* contextNode = eventNode->Find("context");
  if(contextNode && contextNode->GetType() == TreeNode::OBJECT)
  {
    oss << ",\"context\":" << BuildContextJson(*contextNode, ctx);
  }

  // wantResponse
  const TreeNode* wantResponseNode = eventNode->Find("wantResponse");
  if(wantResponseNode && wantResponseNode->GetType() == TreeNode::BOOLEAN &&
     wantResponseNode->GetBoolean())
  {
    oss << ",\"wantResponse\":true";
    // Generate a simple action ID
    static uint32_t sActionCounter = 0;
    oss << ",\"actionId\":\"act_" << (++sActionCounter) << "\"";
  }

  oss << "}}";

  std::string actionJson = oss.str();

  DALI_LOG_ERROR("[A2UI] Action dispatched: %s\n", actionJson.c_str());

  // Include data model if sendDataModel is enabled
  if(mSendDataModel)
  {
    std::string dataJson = ctx.GetDataModel().Serialize();
    DALI_LOG_ERROR("[A2UI] DataModel snapshot: %s\n", dataJson.c_str());
  }

  if(mSendCallback)
  {
    mSendCallback(actionJson);
  }
}

std::string ActionDispatcher::BuildContextJson(const TreeNode& contextNode,
                                               const DataContext& ctx) const
{
  std::ostringstream oss;
  oss << "{";
  bool first = true;

  for(auto it = contextNode.CBegin(); it != contextNode.CEnd(); ++it)
  {
    if(!first) oss << ",";
    first = false;

    const char* key = (*it).first;
    if(key)
    {
      oss << "\"" << key << "\":";
      oss << ResolveContextValue((*it).second, ctx);
    }
  }

  oss << "}";
  return oss.str();
}

std::string ActionDispatcher::ResolveContextValue(const TreeNode& node,
                                                  const DataContext& ctx) const
{
  if(node.GetType() == TreeNode::STRING)
  {
    std::ostringstream oss;
    oss << "\"";
    EscapeJsonStr(oss, node.GetString());
    oss << "\"";
    return oss.str();
  }

  if(node.GetType() == TreeNode::OBJECT)
  {
    // Check for data binding {path: "/..."}
    const TreeNode* pathNode = node.Find("path");
    if(pathNode && pathNode->GetType() == TreeNode::STRING)
    {
      std::string resolved = ctx.GetString(pathNode->GetString());
      // Try to determine if it's a boolean or number
      if(resolved == "true") return "true";
      if(resolved == "false") return "false";

      // Check if it's numeric
      char* endPtr = nullptr;
      std::strtod(resolved.c_str(), &endPtr);
      if(endPtr && *endPtr == '\0' && !resolved.empty())
      {
        return resolved; // numeric
      }

      {
        std::ostringstream oss;
        oss << "\"";
        EscapeJsonStr(oss, resolved);
        oss << "\"";
        return oss.str();
      }
    }

    // Nested object
    return BuildContextJson(node, ctx);
  }

  if(node.GetType() == TreeNode::INTEGER)
  {
    return std::to_string(node.GetInteger());
  }
  if(node.GetType() == TreeNode::FLOAT)
  {
    std::ostringstream oss;
    oss << node.GetFloat();
    return oss.str();
  }
  if(node.GetType() == TreeNode::BOOLEAN)
  {
    return node.GetBoolean() ? "true" : "false";
  }

  return "null";
}

} // namespace A2ui
