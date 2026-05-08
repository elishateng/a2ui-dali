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

#include "a2ui-stream-parser.h"
#include "a2ui-payload-fixer.h"
#include <dali/integration-api/debug.h>

namespace A2ui
{

// ========================================================================
// LineBuffer
// ========================================================================

std::vector<std::string> LineBuffer::Feed(const std::string& chunk)
{
  std::vector<std::string> lines;
  mBuffer += chunk;

  size_t pos = 0;
  size_t newlinePos;
  while((newlinePos = mBuffer.find('\n', pos)) != std::string::npos)
  {
    std::string line = mBuffer.substr(pos, newlinePos - pos);
    // Trim trailing \r for CRLF compatibility
    if(!line.empty() && line.back() == '\r')
    {
      line.pop_back();
    }
    if(!line.empty())
    {
      lines.push_back(std::move(line));
    }
    pos = newlinePos + 1;
  }

  // Keep incomplete trailing data
  mBuffer = mBuffer.substr(pos);
  return lines;
}

std::string LineBuffer::Flush()
{
  std::string remaining;
  std::swap(remaining, mBuffer);
  // Trim trailing \r
  if(!remaining.empty() && remaining.back() == '\r')
  {
    remaining.pop_back();
  }
  return remaining;
}

void LineBuffer::Clear()
{
  mBuffer.clear();
}

// ========================================================================
// A2uiStreamParser
// ========================================================================

A2uiStreamParser::A2uiStreamParser()
{
}

void A2uiStreamParser::BeginStream()
{
  mLineBuffer.Clear();
  mLineCount = 0;
  mErrorCount = 0;
}

void A2uiStreamParser::OnData(const std::string& data, SurfaceModel& surface)
{
  auto lines = mLineBuffer.Feed(data);
  for(auto& line : lines)
  {
    ProcessLine(line, surface);
  }
}

void A2uiStreamParser::EndStream(SurfaceModel& surface)
{
  std::string remaining = mLineBuffer.Flush();
  if(!remaining.empty())
  {
    ProcessLine(remaining, surface);
  }
}

void A2uiStreamParser::ProcessLine(const std::string& line, SurfaceModel& surface)
{
  std::string processedLine = line;

  // Apply payload fixing if enabled (for LLM output)
  if(mFixEnabled)
  {
    processedLine = A2uiPayloadFixer::Fix(processedLine);
    if(processedLine.empty())
    {
      return; // Fixer determined line is not processable (e.g. markdown fence)
    }
  }

  mLineCount++;

  bool ok = mProcessor.ProcessLine(processedLine, surface);
  if(ok)
  {
    if(mOnLine)
    {
      mOnLine(processedLine);
    }
  }
  else
  {
    mErrorCount++;
    if(mOnError)
    {
      mOnError(mProcessor.GetLastError());
    }
    DALI_LOG_ERROR("[A2UI] StreamParser error on line %d: %s\n",
                   mLineCount, mProcessor.GetLastError().c_str());
  }
}

} // namespace A2ui
