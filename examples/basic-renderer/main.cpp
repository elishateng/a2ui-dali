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

// Minimal a2ui-dali example.
//
//   ./a2ui-basic-renderer [path/to/messages.jsonl]
//
// Loads an A2UI v0.9 JSONL stream from a file (default: ./sample.jsonl) and feeds it
// through the A2uiHost facade, which renders each surface and hands its root view back
// via a callback. This is the smallest end-to-end integration of the library; production
// apps wire JsonFeed() to a live transport (e.g. A2A) instead of a static file.

#include "renderer/a2ui-host.h"

#include <dali/public-api/adaptor-framework/application.h>
#include <dali/public-api/adaptor-framework/window-data.h>
#include <dali/public-api/math/rect.h>
#include <dali-ui-foundation/dali-ui-foundation.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{

class BasicRendererApp : public Dali::ConnectionTracker
{
public:
  BasicRendererApp(Dali::Application& app, std::string jsonlPath)
  : mApplication(app), mJsonlPath(std::move(jsonlPath))
  {
    mApplication.InitSignal().Connect(this, &BasicRendererApp::OnInit);
  }

private:
  void OnInit(Dali::Application app)
  {
    Dali::Window window = app.GetWindow();
    window.SetBackgroundColor(Dali::Ui::UiColor(0xFFFFFF)); // OneUI BackgroundApp (cards = SurfaceContainerLow on top)

    Dali::Ui::FlexLayout root = Dali::Ui::FlexLayout::New();
    root.SetDirection(Dali::Ui::FlexDirection::COLUMN);
    root.SetRequestedWidth(Dali::Ui::MATCH_PARENT);
    root.SetRequestedHeight(Dali::Ui::MATCH_PARENT);
    root.SetPadding(Dali::Extents(16, 16, 16, 16));
    window.Add(root);

    // Resolve local image/icon resources (icons/<name>.png, etc.) relative to res/.
    mHost.GetRenderer().SetImageDir("res/");

    // The host hands back each surface's rooted view; the app just adds it to its layout.
    mHost.SetOnBeginRenderingSurface([root](const std::string&, Dali::Ui::View view) mutable {
      root.Add(view);
    });
    mHost.SetOnUserAction([](const std::string& surfaceId, const std::string& json) {
      std::printf("[a2ui-dali] action surface=%s %s\n", surfaceId.c_str(), json.c_str());
    });

    if(!mHost.JsonFeedFile(mJsonlPath))
    {
      std::fprintf(stderr, "[a2ui-dali] Failed to open '%s'\n", mJsonlPath.c_str());
    }
  }

  Dali::Application& mApplication;
  std::string        mJsonlPath;
  A2ui::A2uiHost     mHost;
};

} // namespace

int DALI_EXPORT_API main(int argc, char** argv)
{
  std::string jsonlPath = (argc > 1) ? argv[1] : "sample.jsonl";

  // Window size: CLI args (w h) override the DALI_WINDOW_WIDTH/HEIGHT env, default 720x1080.
  int winW = 720, winH = 1080;
  if(const char* e = std::getenv("DALI_WINDOW_WIDTH"))  { int v = std::atoi(e); if(v > 0) winW = v; }
  if(const char* e = std::getenv("DALI_WINDOW_HEIGHT")) { int v = std::atoi(e); if(v > 0) winH = v; }
  if(argc > 3) { int w = std::atoi(argv[2]), h = std::atoi(argv[3]); if(w > 0 && h > 0) { winW = w; winH = h; } }

  Dali::WindowData windowData;
  Dali::Rect<int> positionSize(0, 0, winW, winH);
  windowData.SetPositionSize(positionSize);

  Dali::Application application = Dali::Application::New(
      &argc, &argv, "", /*useUiThread=*/false, windowData);

  // DPI drives the _dp unit factor (default 160 → 1dp=1px). Raising it scales the whole
  // UI, e.g. DALI_UI_DPI=320 → 2x — pair it with a window scaled by the same factor
  // (DALI_WINDOW_WIDTH/HEIGHT) to supersample for crisp high-res captures at an unchanged
  // layout.
  int uiDpi = 160;
  if(const char* e = std::getenv("DALI_UI_DPI")) { int v = std::atoi(e); if(v > 0) uiDpi = v; }
  Dali::Ui::UiConfig::New().SetDpi(uiDpi).Apply();

  BasicRendererApp demo(application, jsonlPath);
  application.MainLoop();
  return 0;
}
