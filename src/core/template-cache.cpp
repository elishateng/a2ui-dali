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

#include "template-cache.h"
#include <dali/integration-api/debug.h>
#include <algorithm>
#include <sstream>
#include <cctype>

namespace A2ui
{

TemplateCache::TemplateCache(size_t maxSize, std::chrono::seconds ttl)
  : mMaxSize(maxSize)
  , mTTL(ttl)
{
}

std::string TemplateCache::ComputeKey(const std::string& userMessage) const
{
  // Normalize: lowercase + collapse whitespace + trim
  std::string normalized;
  normalized.reserve(userMessage.size());

  bool lastWasSpace = true;
  for(char c : userMessage)
  {
    if(std::isspace(static_cast<unsigned char>(c)))
    {
      if(!lastWasSpace)
      {
        normalized += ' ';
        lastWasSpace = true;
      }
    }
    else
    {
      normalized += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      lastWasSpace = false;
    }
  }

  // Trim trailing space
  if(!normalized.empty() && normalized.back() == ' ')
  {
    normalized.pop_back();
  }

  return normalized;
}

bool TemplateCache::IsExpired(const CacheEntry& entry) const
{
  auto now = std::chrono::steady_clock::now();
  return (now - entry.timestamp) > mTTL;
}

std::string TemplateCache::Get(const std::string& userMessage)
{
  std::string key = ComputeKey(userMessage);
  auto it = mCache.find(key);

  if(it == mCache.end())
  {
    mMissCount++;
    return "";
  }

  if(IsExpired(it->second))
  {
    // Remove expired entry
    auto lruIt = mLruMap.find(key);
    if(lruIt != mLruMap.end())
    {
      mLruOrder.erase(lruIt->second);
      mLruMap.erase(lruIt);
    }
    mCache.erase(it);
    mMissCount++;
    return "";
  }

  // Hit — move to front of LRU
  it->second.hitCount++;
  auto lruIt = mLruMap.find(key);
  if(lruIt != mLruMap.end())
  {
    mLruOrder.erase(lruIt->second);
  }
  mLruOrder.push_front(key);
  mLruMap[key] = mLruOrder.begin();

  mHitCount++;

  DALI_LOG_ERROR("[A2UI] TemplateCache: HIT (key='%s', hits=%d)\n",
                 key.substr(0, 40).c_str(), it->second.hitCount);

  return it->second.jsonl;
}

void TemplateCache::Put(const std::string& userMessage, const std::string& jsonl)
{
  if(jsonl.empty()) return;

  std::string key = ComputeKey(userMessage);

  // Update existing entry or insert new
  auto it = mCache.find(key);
  if(it != mCache.end())
  {
    it->second.jsonl = jsonl;
    it->second.timestamp = std::chrono::steady_clock::now();

    // Move to front
    auto lruIt = mLruMap.find(key);
    if(lruIt != mLruMap.end())
    {
      mLruOrder.erase(lruIt->second);
    }
    mLruOrder.push_front(key);
    mLruMap[key] = mLruOrder.begin();
    return;
  }

  // New entry — evict if needed
  Evict();

  CacheEntry entry;
  entry.jsonl = jsonl;
  entry.timestamp = std::chrono::steady_clock::now();
  entry.hitCount = 0;

  mCache[key] = std::move(entry);
  mLruOrder.push_front(key);
  mLruMap[key] = mLruOrder.begin();

  DALI_LOG_ERROR("[A2UI] TemplateCache: STORE (key='%s', size=%zu/%zu)\n",
                 key.substr(0, 40).c_str(), mCache.size(), mMaxSize);
}

bool TemplateCache::Has(const std::string& userMessage) const
{
  std::string key = ComputeKey(userMessage);
  auto it = mCache.find(key);
  if(it == mCache.end()) return false;
  return !IsExpired(it->second);
}

void TemplateCache::Clear()
{
  mCache.clear();
  mLruOrder.clear();
  mLruMap.clear();
  mHitCount = 0;
  mMissCount = 0;
}

float TemplateCache::GetHitRate() const
{
  size_t total = mHitCount + mMissCount;
  if(total == 0) return 0.0f;
  return static_cast<float>(mHitCount) / static_cast<float>(total);
}

void TemplateCache::Evict()
{
  // Remove expired entries first
  auto it = mLruOrder.rbegin();
  while(it != mLruOrder.rend() && mCache.size() >= mMaxSize)
  {
    auto cacheIt = mCache.find(*it);
    if(cacheIt != mCache.end() && IsExpired(cacheIt->second))
    {
      mCache.erase(cacheIt);
      mLruMap.erase(*it);
      auto toErase = std::next(it).base();
      mLruOrder.erase(toErase);
      it = mLruOrder.rbegin(); // restart
      continue;
    }
    ++it;
  }

  // If still over capacity, remove LRU (back)
  while(mCache.size() >= mMaxSize && !mLruOrder.empty())
  {
    std::string key = mLruOrder.back();
    mLruOrder.pop_back();
    mCache.erase(key);
    mLruMap.erase(key);
  }
}

} // namespace A2ui
