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

#ifndef DALI_GENERATIVE_UI_TEMPLATE_CACHE_H
#define DALI_GENERATIVE_UI_TEMPLATE_CACHE_H

#include <string>
#include <unordered_map>
#include <list>
#include <chrono>

namespace A2ui
{

/**
 * LRU cache for A2UI JSONL templates.
 * Caches the JSONL output for frequent requests so LLM doesn't need to regenerate.
 *
 * Key: normalized request string (lowercase, stopwords removed)
 * Value: complete JSONL response
 */
class TemplateCache
{
public:
  explicit TemplateCache(size_t maxSize = 50,
                          std::chrono::seconds ttl = std::chrono::seconds(3600));

  /**
   * Look up a cached JSONL response.
   * @param userMessage  The user's request message
   * @return cached JSONL string, or empty if not found / expired
   */
  std::string Get(const std::string& userMessage);

  /**
   * Store a JSONL response in the cache.
   * @param userMessage  The user's request message (key)
   * @param jsonl        The complete JSONL response (value)
   */
  void Put(const std::string& userMessage, const std::string& jsonl);

  /**
   * Check if a response is cached for the given message.
   */
  bool Has(const std::string& userMessage) const;

  /**
   * Clear the entire cache.
   */
  void Clear();

  /**
   * Get cache statistics.
   */
  size_t GetSize() const { return mCache.size(); }
  size_t GetHitCount() const { return mHitCount; }
  size_t GetMissCount() const { return mMissCount; }
  float GetHitRate() const;

private:
  struct CacheEntry
  {
    std::string jsonl;
    std::chrono::steady_clock::time_point timestamp;
    int hitCount = 0;
  };

  /**
   * Normalize user message into a cache key.
   * Lowercase + collapse extra whitespace + trim.
   */
  std::string ComputeKey(const std::string& userMessage) const;

  /**
   * Evict least recently used entries if over capacity.
   */
  void Evict();

  /**
   * Check if an entry has expired.
   */
  bool IsExpired(const CacheEntry& entry) const;

  size_t mMaxSize;
  std::chrono::seconds mTTL;

  // LRU order: front = most recently used, back = least recently used
  std::list<std::string> mLruOrder;
  std::unordered_map<std::string, CacheEntry> mCache;
  std::unordered_map<std::string, std::list<std::string>::iterator> mLruMap;

  size_t mHitCount = 0;
  size_t mMissCount = 0;
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_TEMPLATE_CACHE_H
