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

#ifndef DALI_GENERATIVE_UI_A2UI_STREAM_PARSER_H
#define DALI_GENERATIVE_UI_A2UI_STREAM_PARSER_H

#include "a2ui-message-processor.h"
#include "surface-model.h"
#include <string>
#include <vector>
#include <functional>

namespace A2ui
{

/**
 * LineBuffer: accumulates network chunks and extracts complete lines.
 *
 * JSONL framing is newline-based: each complete line is a self-contained
 * JSON object. Incomplete trailing data stays in the buffer until the
 * next chunk arrives.
 */
class LineBuffer
{
public:
  /**
   * Feed a network chunk into the buffer.
   * @return vector of complete lines (without trailing newline)
   */
  std::vector<std::string> Feed(const std::string& chunk);

  /** Flush any remaining data in the buffer as a final line. */
  std::string Flush();

  /** Clear all buffered data. */
  void Clear();

private:
  std::string mBuffer;
};

/**
 * A2uiStreamParser: streaming JSONL parser for A2UI messages.
 *
 * Receives raw network data (possibly split mid-line), extracts complete
 * JSONL lines via LineBuffer, optionally fixes malformed LLM output via
 * A2uiPayloadFixer, then routes each line through A2uiMessageProcessor.
 *
 * Lifecycle:
 *   1. BeginStream() — reset state for a new streaming session
 *   2. OnChunk(data) — called per network chunk (any thread, queued)
 *   3. ProcessPending() — called on main thread to process queued lines
 *   4. EndStream() — flush remaining buffer
 */
class A2uiStreamParser
{
public:
  using LineCallback = std::function<void(const std::string& line)>;
  using ErrorCallback = std::function<void(const std::string& error)>;

  A2uiStreamParser();

  /** Reset for a new streaming session. */
  void BeginStream();

  /**
   * Feed raw data from the network. Extracts complete lines and
   * processes them immediately via A2uiMessageProcessor.
   * Must be called from the main thread.
   */
  void OnData(const std::string& data, SurfaceModel& surface);

  /**
   * Flush remaining buffer data at end of stream.
   * Must be called from the main thread.
   */
  void EndStream(SurfaceModel& surface);

  /** Set callback invoked for each successfully parsed line. */
  void SetOnLine(LineCallback cb) { mOnLine = std::move(cb); }

  /** Set callback invoked on parse errors. */
  void SetOnError(ErrorCallback cb) { mOnError = std::move(cb); }

  /** Get total number of lines processed in current session. */
  int GetLineCount() const { return mLineCount; }

  /** Get number of errors in current session. */
  int GetErrorCount() const { return mErrorCount; }

  /** Enable/disable payload fixing for LLM output. */
  void SetFixEnabled(bool enabled) { mFixEnabled = enabled; }

private:
  void ProcessLine(const std::string& line, SurfaceModel& surface);

  LineBuffer            mLineBuffer;
  A2uiMessageProcessor  mProcessor;
  LineCallback          mOnLine;
  ErrorCallback         mOnError;
  int                   mLineCount = 0;
  int                   mErrorCount = 0;
  bool                  mFixEnabled = true;
};

} // namespace A2ui

#endif // DALI_GENERATIVE_UI_A2UI_STREAM_PARSER_H
