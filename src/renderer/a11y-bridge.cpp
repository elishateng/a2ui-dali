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

#include "a11y-bridge.h"
#include <dali/integration-api/debug.h>

using namespace Dali;
using namespace Dali::Ui;
// dali-ui 2.5.28 moved the builder TreeNode into the Dali::Ui::Integration namespace.
using Dali::Ui::Integration::TreeNode;

namespace A2ui
{

namespace
{
const char* GetNodeString(const TreeNode& node, const char* key, const char* fallback)
{
  const TreeNode* child = node.Find(key);
  if(child && child->GetType() == TreeNode::STRING) return child->GetString();
  return fallback;
}

// AT-SPI2 role string for logging/debug
const char* GetRoleString(const std::string& type)
{
  if(type == "Text")         return "LABEL";
  if(type == "Button")       return "PUSH_BUTTON";
  if(type == "TextField")    return "ENTRY";
  if(type == "CheckBox")     return "CHECK_BOX";
  if(type == "Slider")       return "SLIDER";
  if(type == "Image")        return "IMAGE";
  if(type == "Row" || type == "Column") return "PANEL";
  if(type == "Card")         return "GROUP";
  if(type == "Tabs")         return "PAGE_TAB_LIST";
  if(type == "Modal")        return "DIALOG";
  if(type == "List")         return "LIST";
  if(type == "Icon")         return "ICON";
  if(type == "Divider")      return "SEPARATOR";
  return "UNKNOWN";
}
} // anonymous namespace

std::string A11yBridge::ResolveLabel(const ComponentModel& comp, const DataContext& ctx)
{
  if(!comp.rawNode) return "";

  const TreeNode* labelNode = comp.rawNode->Find("label");
  if(labelNode && labelNode->GetType() == TreeNode::STRING)
  {
    return labelNode->GetString();
  }

  const TreeNode* textNode = comp.rawNode->Find("text");
  if(textNode && textNode->GetType() == TreeNode::STRING)
  {
    return textNode->GetString();
  }
  if(textNode && textNode->GetType() == TreeNode::OBJECT)
  {
    const TreeNode* pathNode = textNode->Find("path");
    if(pathNode && pathNode->GetType() == TreeNode::STRING)
    {
      return ctx.GetString(pathNode->GetString());
    }
  }

  const TreeNode* altNode = comp.rawNode->Find("alt");
  if(altNode && altNode->GetType() == TreeNode::STRING)
  {
    return altNode->GetString();
  }

  return "";
}

void A11yBridge::Apply(View& view, const ComponentModel& comp, const DataContext& ctx)
{
  if(!view) return;

  // Use DALi's custom properties for accessibility information.
  // The AT-SPI2 bridge in DALi adaptor reads these properties
  // to expose them to screen readers.

  const std::string& type = comp.type;
  const char* role = GetRoleString(type);

  // Register accessibility info as READ_WRITE custom properties
  // (String properties cannot be ANIMATABLE in DALi)
  view.RegisterProperty("a2uiRole", Property::Value(Dali::String(role)), Property::READ_WRITE);
  view.RegisterProperty("a2uiComponentType", Property::Value(Dali::String(type.c_str())), Property::READ_WRITE);
  view.RegisterProperty("a2uiComponentId", Property::Value(Dali::String(comp.id.c_str())), Property::READ_WRITE);

  // Set accessible name
  std::string label = ResolveLabel(comp, ctx);
  if(!label.empty())
  {
    view.RegisterProperty("a2uiAccessibleName", Property::Value(Dali::String(label.c_str())), Property::READ_WRITE);
  }

  // Set description from accessibilityLabel or description field
  if(comp.rawNode)
  {
    const char* desc = GetNodeString(*comp.rawNode, "accessibilityLabel", "");
    if(desc[0] == '\0')
    {
      desc = GetNodeString(*comp.rawNode, "description", "");
    }
    if(desc[0] != '\0')
    {
      view.RegisterProperty("a2uiAccessibleDescription",
                            Property::Value(Dali::String(desc)), Property::READ_WRITE);
    }
  }
}

void A11yBridge::SetAccessibleName(View& view, const std::string& name)
{
  if(!view) return;
  view.RegisterProperty("a2uiAccessibleName", Property::Value(Dali::String(name.c_str())), Property::READ_WRITE);
}

void A11yBridge::SetAccessibleDescription(View& view, const std::string& desc)
{
  if(!view) return;
  view.RegisterProperty("a2uiAccessibleDescription", Property::Value(Dali::String(desc.c_str())), Property::READ_WRITE);
}

} // namespace A2ui
