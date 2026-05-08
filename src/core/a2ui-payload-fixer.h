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

#ifndef DALI_GENERATIVE_UI_A2UI_PAYLOAD_FIXER_H
#define DALI_GENERATIVE_UI_A2UI_PAYLOAD_FIXER_H

#include <string>

namespace A2ui
{

/**
 * Fixes common LLM output errors in A2UI JSONL lines.
 *
 * LLMs sometimes produce malformed output:
 * - Markdown code fences (```json ... ```)
 * - Missing "version" field
 * - Trailing commas in JSON
 * - Unclosed brackets/braces
 * - Leading/trailing whitespace
 *
 * Ported from A2UI Python SDK's A2uiPayloadFixer pattern.
 */
class A2uiPayloadFixer
{
public:
  /**
   * Fix a single JSONL line.
   * @return Fixed line, or empty string if the line should be skipped
   *         (e.g. markdown fences, comments, blank lines).
   */
  static std::string Fix(const std::string& line);

private:
  /** Strip markdown code block fences (```json, ```) */
  static std::string StripMarkdownFence(const std::string& line);

  /** Remove trailing commas before } or ] */
  static std::string RemoveTrailingCommas(const std::string& line);

  /** Close unclosed brackets/braces */
  static std::string CompletePartialJson(const std::string& line);

  /** Add version field if missing */
  static std::string AddVersionIfMissing(const std::string& line);

  /** Trim whitespace from both ends */
  static std::string Trim(const std::string& s);
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_A2UI_PAYLOAD_FIXER_H
