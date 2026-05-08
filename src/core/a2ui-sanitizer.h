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

#ifndef DALI_GENERATIVE_UI_A2UI_SANITIZER_H
#define DALI_GENERATIVE_UI_A2UI_SANITIZER_H

#include <string>
#include <vector>

namespace A2ui
{

/**
 * Input sanitizer for A2UI messages.
 *
 * Security checks:
 * 1. URL validation: only http/https schemes
 * 2. Component ID validation: alphanumeric + underscore only
 * 3. Data size limits: prevent DoS via large payloads
 * 4. Text sanitization: strip HTML/script tags
 * 5. Shader validation: block dangerous shader patterns
 */
class A2uiSanitizer
{
public:
  /**
   * Validate a URL. Only allows http:// and https:// schemes.
   * @return true if the URL is safe
   */
  static bool ValidateUrl(const std::string& url);

  /**
   * Validate a component ID. Only allows [a-zA-Z0-9_-].
   * @return true if valid
   */
  static bool ValidateComponentId(const std::string& id);

  /**
   * Sanitize text content (strip HTML tags, prevent injection).
   * @return sanitized text
   */
  static std::string SanitizeText(const std::string& text);

  /**
   * Check if a data payload is within size limits.
   * @param jsonSize  Size of the JSON payload in bytes
   * @return true if within limits
   */
  static bool ValidateDataSize(size_t jsonSize);

  /**
   * Validate a GLSL shader source for safety.
   * @return true if safe
   */
  static bool ValidateShader(const std::string& source);

  /**
   * Set the maximum allowed data model size (default 1MB).
   */
  static void SetMaxDataSize(size_t bytes);

  /**
   * Set allowed URL domains (empty = allow all http/https).
   */
  static void SetAllowedDomains(const std::vector<std::string>& domains);

private:
  static size_t sMaxDataSize;
  static std::vector<std::string> sAllowedDomains;
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_A2UI_SANITIZER_H
