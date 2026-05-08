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

#include "a2ui-sanitizer.h"
#include <dali/integration-api/debug.h>
#include <algorithm>
#include <cctype>

namespace A2ui
{

// Default: 1MB max data model size
size_t A2uiSanitizer::sMaxDataSize = 1024 * 1024;
std::vector<std::string> A2uiSanitizer::sAllowedDomains;

bool A2uiSanitizer::ValidateUrl(const std::string& url)
{
  if(url.empty()) return false;

  // Only allow http:// and https://
  if(url.substr(0, 7) != "http://" && url.substr(0, 8) != "https://")
  {
    DALI_LOG_ERROR("[A2UI] Sanitizer: blocked non-http URL scheme: %s\n",
                   url.substr(0, 30).c_str());
    return false;
  }

  // Check allowed domains if configured
  if(!sAllowedDomains.empty())
  {
    // Extract domain from URL
    size_t start = url.find("://") + 3;
    size_t end = url.find('/', start);
    std::string domain = (end != std::string::npos) ? url.substr(start, end - start)
                                                     : url.substr(start);

    // Remove port if present
    size_t colonPos = domain.find(':');
    if(colonPos != std::string::npos)
    {
      domain = domain.substr(0, colonPos);
    }

    bool allowed = false;
    for(const auto& d : sAllowedDomains)
    {
      if(domain == d || (domain.size() > d.size() &&
          domain.substr(domain.size() - d.size() - 1) == "." + d))
      {
        allowed = true;
        break;
      }
    }

    if(!allowed)
    {
      DALI_LOG_ERROR("[A2UI] Sanitizer: domain '%s' not in allowlist\n", domain.c_str());
      return false;
    }
  }

  return true;
}

bool A2uiSanitizer::ValidateComponentId(const std::string& id)
{
  if(id.empty() || id.size() > 128)
  {
    return false;
  }

  for(char c : id)
  {
    if(!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
    {
      DALI_LOG_ERROR("[A2UI] Sanitizer: invalid character '%c' in component ID '%s'\n",
                     c, id.c_str());
      return false;
    }
  }

  return true;
}

std::string A2uiSanitizer::SanitizeText(const std::string& text)
{
  // Strip HTML tags (DALi Label doesn't render HTML, but sanitize for safety)
  std::string result;
  result.reserve(text.size());

  bool inTag = false;
  for(size_t i = 0; i < text.size(); i++)
  {
    if(text[i] == '<')
    {
      // Check if it looks like an HTML tag
      if(i + 1 < text.size() && (std::isalpha(static_cast<unsigned char>(text[i + 1])) ||
                                   text[i + 1] == '/' || text[i + 1] == '!'))
      {
        inTag = true;
        continue;
      }
    }

    if(inTag)
    {
      if(text[i] == '>')
      {
        inTag = false;
      }
      continue;
    }

    result += text[i];
  }

  // Size limit for individual text content (64KB)
  if(result.size() > 65536)
  {
    DALI_LOG_ERROR("[A2UI] Sanitizer: text content truncated from %zu to 65536 bytes\n",
                   result.size());
    result.resize(65536);
  }

  return result;
}

bool A2uiSanitizer::ValidateDataSize(size_t jsonSize)
{
  if(jsonSize > sMaxDataSize)
  {
    DALI_LOG_ERROR("[A2UI] Sanitizer: data payload too large (%zu bytes, max %zu)\n",
                   jsonSize, sMaxDataSize);
    return false;
  }
  return true;
}

bool A2uiSanitizer::ValidateShader(const std::string& source)
{
  // Max shader size: 4KB
  if(source.size() > 4096)
  {
    DALI_LOG_ERROR("[A2UI] Sanitizer: shader source too large (%zu bytes)\n", source.size());
    return false;
  }

  // Block obvious infinite loop patterns
  if(source.find("while(true)") != std::string::npos ||
     source.find("while (true)") != std::string::npos ||
     source.find("for(;;)") != std::string::npos ||
     source.find("for (;;)") != std::string::npos)
  {
    DALI_LOG_ERROR("[A2UI] Sanitizer: blocked potential infinite loop in shader\n");
    return false;
  }

  return true;
}

void A2uiSanitizer::SetMaxDataSize(size_t bytes)
{
  sMaxDataSize = bytes;
}

void A2uiSanitizer::SetAllowedDomains(const std::vector<std::string>& domains)
{
  sAllowedDomains = domains;
}

} // namespace A2ui
