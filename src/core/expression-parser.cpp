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

#include "expression-parser.h"
#include <dali/integration-api/debug.h>
#include <regex>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>

using Dali::Ui::TreeNode;

namespace A2ui
{

ExpressionParser::ExpressionParser()
{
  // === Validation functions ===

  RegisterFunction("required", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string val = ResolveArg(args, "value", ctx);
    return val.empty() ? "false" : "true";
  });

  RegisterFunction("email", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string val = ResolveArg(args, "value", ctx);
    if(val.empty()) return "false";
    static const std::regex emailRegex(R"([a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,})");
    return std::regex_match(val, emailRegex) ? "true" : "false";
  });

  RegisterFunction("regex", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string val = ResolveArg(args, "value", ctx);
    std::string pattern = GetArgString(args, "pattern");
    if(pattern.empty()) return "false";
    try
    {
      return std::regex_match(val, std::regex(pattern)) ? "true" : "false";
    }
    catch(const std::regex_error&)
    {
      DALI_LOG_ERROR("[A2UI] ExpressionParser: invalid regex pattern: %s\n", pattern.c_str());
      return "false";
    }
  });

  RegisterFunction("length", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string val = ResolveArg(args, "value", ctx);
    int len = static_cast<int>(val.size());
    int minLen = GetArgInt(args, "min", 0);
    int maxLen = GetArgInt(args, "max", 0x7FFFFFFF);
    return (len >= minLen && len <= maxLen) ? "true" : "false";
  });

  // === Format functions ===

  RegisterFunction("formatString", [this](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string value = ResolveArg(args, "value", ctx);
    if(value.empty()) return "";
    return InterpolateString(value, ctx);
  });

  // === Phase 4 validation functions ===

  RegisterFunction("numeric", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string val = ResolveArg(args, "value", ctx);
    if(val.empty()) return "false";
    char* end = nullptr;
    std::strtod(val.c_str(), &end);
    return (end != val.c_str() && *end == '\0') ? "true" : "false";
  });

  // === Phase 4 logical functions ===

  RegisterFunction("and", [this](const TreeNode& args, const DataContext& ctx) -> std::string {
    const TreeNode* valuesNode = args.Find("values");
    if(!valuesNode || valuesNode->GetType() != TreeNode::ARRAY) return "false";
    for(auto it = valuesNode->CBegin(); it != valuesNode->CEnd(); ++it)
    {
      const TreeNode& child = (*it).second;
      std::string result = this->Evaluate(child, ctx);
      if(result != "true") return "false";
    }
    return "true";
  });

  RegisterFunction("or", [this](const TreeNode& args, const DataContext& ctx) -> std::string {
    const TreeNode* valuesNode = args.Find("values");
    if(!valuesNode || valuesNode->GetType() != TreeNode::ARRAY) return "false";
    for(auto it = valuesNode->CBegin(); it != valuesNode->CEnd(); ++it)
    {
      const TreeNode& child = (*it).second;
      std::string result = this->Evaluate(child, ctx);
      if(result == "true") return "true";
    }
    return "false";
  });

  RegisterFunction("not", [this](const TreeNode& args, const DataContext& ctx) -> std::string {
    const TreeNode* valueNode = args.Find("value");
    if(!valueNode) return "true";
    // If value is a function call (has "call" field), evaluate it
    if(valueNode->GetType() == TreeNode::OBJECT && valueNode->Find("call"))
    {
      std::string result = this->Evaluate(*valueNode, ctx);
      return (result == "true") ? "false" : "true";
    }
    std::string val = ResolveArg(args, "value", ctx);
    return (val == "true") ? "false" : "true";
  });

  // === Phase 4 format functions ===

  RegisterFunction("formatCurrency", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string valStr = ResolveArg(args, "value", ctx);
    if(valStr.empty()) return "";

    double num = std::strtod(valStr.c_str(), nullptr);
    std::string currency = GetArgString(args, "currency");
    if(currency.empty()) currency = "USD";

    // Determine symbol
    std::string symbol = "$";
    if(currency == "EUR") symbol = "\xe2\x82\xac";      // Euro sign
    else if(currency == "GBP") symbol = "\xc2\xa3";      // Pound sign
    else if(currency == "JPY") symbol = "\xc2\xa5";      // Yen sign
    else if(currency == "KRW") symbol = "\xe2\x82\xa9";  // Won sign
    else if(currency == "CNY") symbol = "\xc2\xa5";      // Yuan sign

    // Format with 2 decimal places and grouping
    bool isNegative = (num < 0);
    if(isNegative) num = -num;

    std::ostringstream oss;
    oss.precision(currency == "JPY" || currency == "KRW" ? 0 : 2);
    oss << std::fixed << num;

    std::string result = oss.str();

    // Insert thousands separators
    std::string intPart, fracPart;
    size_t dotPos = result.find('.');
    if(dotPos != std::string::npos)
    {
      intPart = result.substr(0, dotPos);
      fracPart = result.substr(dotPos);
    }
    else
    {
      intPart = result;
    }

    std::string grouped;
    int count = 0;
    for(int i = static_cast<int>(intPart.size()) - 1; i >= 0; --i)
    {
      if(count > 0 && count % 3 == 0)
        grouped = "," + grouped;
      grouped = intPart[i] + grouped;
      count++;
    }

    result = symbol + grouped + fracPart;
    if(isNegative) result = "-" + result;
    return result;
  });

  RegisterFunction("formatDate", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string valStr = ResolveArg(args, "value", ctx);
    if(valStr.empty()) return "";

    std::string format = GetArgString(args, "format");
    if(format.empty()) format = "YYYY-MM-DD";

    // Parse ISO 8601 date: YYYY-MM-DDTHH:MM:SSZ
    std::string year, month, day, hour, minute, second;
    if(valStr.size() >= 10)
    {
      year   = valStr.substr(0, 4);
      month  = valStr.substr(5, 2);
      day    = valStr.substr(8, 2);
    }
    if(valStr.size() >= 19)
    {
      hour   = valStr.substr(11, 2);
      minute = valStr.substr(14, 2);
      second = valStr.substr(17, 2);
    }

    // Simple pattern replacement
    std::string result = format;
    auto replace = [&result](const std::string& from, const std::string& to) {
      size_t pos = 0;
      while((pos = result.find(from, pos)) != std::string::npos)
      {
        result.replace(pos, from.size(), to);
        pos += to.size();
      }
    };

    replace("YYYY", year);
    replace("MM", month);
    replace("DD", day);
    replace("HH", hour);
    replace("mm", minute);
    replace("ss", second);

    return result;
  });

  RegisterFunction("pluralize", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string valStr = ResolveArg(args, "value", ctx);
    double count = valStr.empty() ? 0 : std::strtod(valStr.c_str(), nullptr);

    // Support both spec keys (one/other) and legacy keys (singular/plural)
    std::string one   = GetArgString(args, "one");
    std::string other = GetArgString(args, "other");
    if(one.empty())   one   = GetArgString(args, "singular");
    if(other.empty()) other = GetArgString(args, "plural");
    if(other.empty()) other = one + "s";

    return (std::abs(count) == 1.0) ? one : other;
  });

  RegisterFunction("openUrl", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string url = ResolveArg(args, "url", ctx);
    if(url.empty()) return "false";
    // Validate URL scheme (security: only http/https allowed)
    if(url.substr(0, 7) != "http://" && url.substr(0, 8) != "https://")
    {
      DALI_LOG_ERROR("[A2UI] openUrl: blocked non-http URL: %s\n", url.c_str());
      return "false";
    }
    DALI_LOG_ERROR("[A2UI] openUrl: %s\n", url.c_str());
    // On Tizen, use app_control to open browser.
    // For desktop, use fork+exec to avoid command injection.
#ifdef __linux__
    pid_t pid = fork();
    if(pid == 0)
    {
      // Child process — exec xdg-open with URL as a separate argument
      execlp("xdg-open", "xdg-open", url.c_str(), nullptr);
      _exit(127); // exec failed
    }
    // Parent does not wait (fire-and-forget)
#endif
    return "true";
  });

  RegisterFunction("formatNumber", [](const TreeNode& args, const DataContext& ctx) -> std::string {
    std::string valStr = ResolveArg(args, "value", ctx);
    if(valStr.empty()) return "";

    double num = std::strtod(valStr.c_str(), nullptr);

    // Grouping defaults to true (A2UI spec: locale-appropriate formatting)
    const TreeNode* groupingNode = args.Find("grouping");
    bool grouping = true;
    if(groupingNode && groupingNode->GetType() == TreeNode::BOOLEAN)
      grouping = groupingNode->GetBoolean();

    // Check for decimals
    int decimals = GetArgInt(args, "decimals", -1);

    std::ostringstream oss;
    if(decimals >= 0)
    {
      oss.precision(decimals);
      oss << std::fixed << num;
    }
    else
    {
      oss << num;
    }

    std::string result = oss.str();

    if(grouping)
    {
      // Insert thousands separators
      std::string intPart, fracPart;
      size_t dotPos = result.find('.');
      if(dotPos != std::string::npos)
      {
        intPart = result.substr(0, dotPos);
        fracPart = result.substr(dotPos);
      }
      else
      {
        intPart = result;
      }

      // Insert commas from right
      std::string grouped;
      int count = 0;
      bool negative = (!intPart.empty() && intPart[0] == '-');
      size_t startIdx = negative ? 1 : 0;

      for(int i = static_cast<int>(intPart.size()) - 1; i >= static_cast<int>(startIdx); --i)
      {
        if(count > 0 && count % 3 == 0)
        {
          grouped = "," + grouped;
        }
        grouped = intPart[i] + grouped;
        count++;
      }

      if(negative) grouped = "-" + grouped;
      result = grouped + fracPart;
    }

    return result;
  });
}

std::string ExpressionParser::Evaluate(const TreeNode& callNode, const DataContext& ctx) const
{
  const TreeNode* nameNode = callNode.Find("call");
  if(!nameNode || nameNode->GetType() != TreeNode::STRING)
  {
    return "";
  }

  std::string funcName = nameNode->GetString();
  auto it = mFunctions.find(funcName);
  if(it == mFunctions.end())
  {
    DALI_LOG_ERROR("[A2UI] ExpressionParser: unknown function '%s'\n", funcName.c_str());
    return "";
  }

  const TreeNode* argsNode = callNode.Find("args");
  if(!argsNode)
  {
    // No args provided — pass callNode but functions should handle missing keys gracefully
    DALI_LOG_ERROR("[A2UI] ExpressionParser: function '%s' called without args\n", funcName.c_str());
    return "";
  }

  return it->second(*argsNode, ctx);
}

void ExpressionParser::RegisterFunction(const std::string& name, FunctionImpl impl)
{
  mFunctions[name] = std::move(impl);
}

std::string ExpressionParser::ResolveArg(const TreeNode& args, const char* key,
                                         const DataContext& ctx)
{
  const TreeNode* node = args.Find(key);
  if(!node) return "";

  // Literal string
  if(node->GetType() == TreeNode::STRING)
  {
    return node->GetString();
  }

  // Data binding: {path: "/..."}
  if(node->GetType() == TreeNode::OBJECT)
  {
    const TreeNode* pathNode = node->Find("path");
    if(pathNode && pathNode->GetType() == TreeNode::STRING)
    {
      return ctx.GetString(pathNode->GetString());
    }
  }

  // Numeric values
  if(node->GetType() == TreeNode::INTEGER)
  {
    return std::to_string(node->GetInteger());
  }
  if(node->GetType() == TreeNode::FLOAT)
  {
    std::ostringstream oss;
    oss << node->GetFloat();
    return oss.str();
  }
  if(node->GetType() == TreeNode::BOOLEAN)
  {
    return node->GetBoolean() ? "true" : "false";
  }

  return "";
}

std::string ExpressionParser::GetArgString(const TreeNode& args, const char* key)
{
  const TreeNode* node = args.Find(key);
  if(!node) return "";
  if(node->GetType() == TreeNode::STRING) return node->GetString();
  return "";
}

float ExpressionParser::GetArgFloat(const TreeNode& args, const char* key, float fallback)
{
  const TreeNode* node = args.Find(key);
  if(!node) return fallback;
  if(node->GetType() == TreeNode::FLOAT) return node->GetFloat();
  if(node->GetType() == TreeNode::INTEGER) return static_cast<float>(node->GetInteger());
  return fallback;
}

int ExpressionParser::GetArgInt(const TreeNode& args, const char* key, int fallback)
{
  const TreeNode* node = args.Find(key);
  if(!node) return fallback;
  if(node->GetType() == TreeNode::INTEGER) return node->GetInteger();
  if(node->GetType() == TreeNode::FLOAT) return static_cast<int>(node->GetFloat());
  return fallback;
}

std::string ExpressionParser::InterpolateString(const std::string& input, const DataContext& ctx) const
{
  std::string result = input;

  // Iteratively resolve innermost ${...} patterns (inside-out)
  for(int iter = 0; iter < 20; ++iter)
  {
    size_t pos = 0;
    bool found = false;

    while(pos < result.size())
    {
      size_t start = result.find("${", pos);
      if(start == std::string::npos) break;

      // Find matching '}' — skip nested ${...} by looking for innermost
      size_t end = result.find('}', start + 2);
      if(end == std::string::npos) break;

      size_t nested = result.find("${", start + 2);
      if(nested != std::string::npos && nested < end)
      {
        // There's a deeper nested ${ — skip to it
        pos = start + 2;
        continue;
      }

      // This is an innermost ${...}
      std::string expr = result.substr(start + 2, end - start - 2);
      std::string resolved = ResolveInlineExpression(expr, ctx);

      result = result.substr(0, start) + resolved + result.substr(end + 1);
      found = true;
      break; // Restart from beginning after replacement
    }

    if(!found) break;
  }

  return result;
}

std::string ExpressionParser::ResolveInlineExpression(const std::string& expr, const DataContext& ctx) const
{
  // 1. Data path: starts with /
  if(!expr.empty() && expr[0] == '/')
  {
    return ctx.GetString(expr);
  }

  // 2. Function call: funcName(key: val, key: 'val', ...)
  size_t parenOpen = expr.find('(');
  if(parenOpen != std::string::npos && !expr.empty() && expr.back() == ')')
  {
    std::string funcName = expr.substr(0, parenOpen);
    std::string argsStr  = expr.substr(parenOpen + 1, expr.size() - parenOpen - 2);

    auto it = mFunctions.find(funcName);
    if(it == mFunctions.end())
    {
      DALI_LOG_ERROR("[A2UI] InterpolateString: unknown function '%s'\n", funcName.c_str());
      return "";
    }

    // Parse inline args "key: val, key: 'str'" → JSON {"key":"val","key":"str"}
    std::ostringstream json;
    json << "{";

    bool first = true;
    size_t pos = 0;
    while(pos < argsStr.size())
    {
      // Skip whitespace
      while(pos < argsStr.size() && argsStr[pos] == ' ') pos++;
      if(pos >= argsStr.size()) break;

      // Read key (until ':')
      size_t colonPos = argsStr.find(':', pos);
      if(colonPos == std::string::npos) break;

      std::string key = argsStr.substr(pos, colonPos - pos);
      while(!key.empty() && key.back() == ' ') key.pop_back();
      pos = colonPos + 1;

      // Skip whitespace after colon
      while(pos < argsStr.size() && argsStr[pos] == ' ') pos++;

      // Read value
      std::string value;
      if(pos < argsStr.size() && (argsStr[pos] == '\'' || argsStr[pos] == '"'))
      {
        // Quoted string
        char quote = argsStr[pos];
        pos++;
        size_t endQ = argsStr.find(quote, pos);
        if(endQ == std::string::npos) endQ = argsStr.size();
        value = argsStr.substr(pos, endQ - pos);
        pos = endQ + 1;
      }
      else
      {
        // Unquoted value (until comma or end)
        size_t endV = argsStr.find(',', pos);
        if(endV == std::string::npos) endV = argsStr.size();
        value = argsStr.substr(pos, endV - pos);
        while(!value.empty() && value.back() == ' ') value.pop_back();
        pos = endV;
      }

      // Skip comma
      if(pos < argsStr.size() && argsStr[pos] == ',') pos++;

      if(!first) json << ",";
      first = false;

      json << "\"" << key << "\":";

      // Check if value is numeric
      char* numEnd = nullptr;
      std::strtod(value.c_str(), &numEnd);
      if(numEnd && numEnd != value.c_str() && *numEnd == '\0' && !value.empty())
      {
        json << value;
      }
      else
      {
        json << "\"" << value << "\"";
      }
    }

    json << "}";

    Dali::Ui::JsonParser parser = Dali::Ui::JsonParser::New();
    if(!parser.Parse(json.str()))
    {
      DALI_LOG_ERROR("[A2UI] InterpolateString: failed to parse inline args: %s\n", json.str().c_str());
      return "";
    }

    const TreeNode* root = parser.GetRoot();
    if(!root) return "";

    return it->second(*root, ctx);
  }

  // 3. Simple variable name
  std::string val = ctx.GetString(expr);
  if(val.empty()) val = ctx.GetString("/" + expr);
  return val;
}

} // namespace A2ui
