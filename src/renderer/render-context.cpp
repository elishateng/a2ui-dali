/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 */

#include "renderer/render-context.h"
#include "core/expression-parser.h"

#include <sstream>

namespace A2ui
{
using Dali::Ui::Integration::TreeNode;

std::string RenderContext::ResolveString(const TreeNode* propNode) const
{
  if(!propNode) return std::string();
  if(propNode->GetType() == TreeNode::STRING) return propNode->GetString();
  if(propNode->GetType() == TreeNode::OBJECT)
  {
    // "call" is checked first — Find() may recurse into nested args.value.path.
    if(propNode->Find("call")) return exprParser.Evaluate(*propNode, data);
    const TreeNode* pathNode = propNode->Find("path");
    if(pathNode && pathNode->GetType() == TreeNode::STRING) return data.GetString(pathNode->GetString());
  }
  if(propNode->GetType() == TreeNode::INTEGER) return std::to_string(propNode->GetInteger());
  if(propNode->GetType() == TreeNode::FLOAT)
  {
    std::ostringstream oss; oss << propNode->GetFloat(); return oss.str();
  }
  return std::string();
}

float RenderContext::ResolveFloat(const TreeNode* propNode, float fallback) const
{
  if(!propNode) return fallback;
  if(propNode->GetType() == TreeNode::FLOAT) return propNode->GetFloat();
  if(propNode->GetType() == TreeNode::INTEGER) return static_cast<float>(propNode->GetInteger());
  if(propNode->GetType() == TreeNode::OBJECT)
  {
    const TreeNode* pathNode = propNode->Find("path");
    if(pathNode && pathNode->GetType() == TreeNode::STRING)
      return data.GetFloat(pathNode->GetString(), fallback);
  }
  return fallback;
}

std::string RenderContext::GetBoundPath(const TreeNode* propNode) const
{
  if(!propNode || propNode->GetType() != TreeNode::OBJECT) return std::string();
  const TreeNode* pathNode = propNode->Find("path");
  if(!pathNode || pathNode->GetType() != TreeNode::STRING) return std::string();
  return data.Resolve(pathNode->GetString());
}

} // namespace A2ui
