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

#include "a2ui-payload-fixer.h"
#include <algorithm>
#include <stack>

namespace A2ui
{

std::string A2uiPayloadFixer::Trim(const std::string& s)
{
  auto start = s.find_first_not_of(" \t\r\n");
  if(start == std::string::npos) return "";
  auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

std::string A2uiPayloadFixer::StripMarkdownFence(const std::string& line)
{
  std::string trimmed = Trim(line);

  // Skip markdown code fences entirely
  if(trimmed.substr(0, 3) == "```")
  {
    return "";
  }

  return trimmed;
}

std::string A2uiPayloadFixer::RemoveTrailingCommas(const std::string& line)
{
  std::string result = line;
  bool inString = false;
  bool escaped = false;

  // Find trailing commas before } or ] (outside strings)
  for(size_t i = 0; i < result.size(); i++)
  {
    char c = result[i];
    if(escaped)
    {
      escaped = false;
      continue;
    }
    if(c == '\\' && inString)
    {
      escaped = true;
      continue;
    }
    if(c == '"')
    {
      inString = !inString;
      continue;
    }
    if(inString) continue;

    if(c == ',')
    {
      // Check if the next non-whitespace is } or ]
      size_t next = result.find_first_not_of(" \t\r\n", i + 1);
      if(next != std::string::npos && (result[next] == '}' || result[next] == ']'))
      {
        result.erase(i, 1);
        i--; // Re-check this position
      }
    }
  }

  return result;
}

std::string A2uiPayloadFixer::CompletePartialJson(const std::string& line)
{
  std::stack<char> brackets;
  bool inString = false;
  bool escaped = false;

  for(char c : line)
  {
    if(escaped)
    {
      escaped = false;
      continue;
    }
    if(c == '\\' && inString)
    {
      escaped = true;
      continue;
    }
    if(c == '"')
    {
      inString = !inString;
      continue;
    }
    if(inString) continue;

    if(c == '{') brackets.push('}');
    else if(c == '[') brackets.push(']');
    else if(c == '}' || c == ']')
    {
      if(!brackets.empty()) brackets.pop();
    }
  }

  // Close any unclosed string
  std::string result = line;
  if(inString)
  {
    result += '"';
  }

  // Close unclosed brackets/braces
  while(!brackets.empty())
  {
    result += brackets.top();
    brackets.pop();
  }

  return result;
}

std::string A2uiPayloadFixer::AddVersionIfMissing(const std::string& line)
{
  // Only add version to lines that look like A2UI messages (start with {)
  if(line.empty() || line[0] != '{') return line;

  // Check if version field exists
  if(line.find("\"version\"") != std::string::npos)
  {
    return line;
  }

  // Insert "version":"v0.9" after the opening brace
  return "{\"version\":\"v0.9\"," + line.substr(1);
}

std::string A2uiPayloadFixer::Fix(const std::string& line)
{
  // 1. Trim whitespace
  std::string fixed = Trim(line);
  if(fixed.empty()) return "";

  // 2. Strip markdown fences (returns empty if it's a fence line)
  fixed = StripMarkdownFence(fixed);
  if(fixed.empty()) return "";

  // 3. Must start with { to be a JSON object
  if(fixed[0] != '{') return "";

  // 4. Remove trailing commas
  fixed = RemoveTrailingCommas(fixed);

  // 5. Complete partial JSON (close unclosed brackets)
  fixed = CompletePartialJson(fixed);

  // 6. Add version if missing
  fixed = AddVersionIfMissing(fixed);

  return fixed;
}

} // namespace A2ui
