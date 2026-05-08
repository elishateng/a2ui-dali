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
// Loads an A2UI v0.9 JSONL stream from a file (default: ./sample.jsonl),
// feeds it through A2uiMessageProcessor, and renders the resulting surface
// into a DALi window. This is the smallest end-to-end integration of the
// library; production apps will typically wire the processor to a live
// transport (e.g. A2A) instead of a static file.

#include "core/a2ui-message-processor.h"
#include "core/surface-model.h"
#include "renderer/a2ui-renderer.h"

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
  void OnInit(Dali::Application& app)
  {
    Dali::Window window = app.GetWindow();
    window.SetBackgroundColor(Dali::Ui::UiColor(0xf5f5f5));

    Dali::Ui::FlexLayout root = Dali::Ui::FlexLayout::New();
    root.SetDirection(Dali::Ui::FlexDirection::COLUMN);
    root.SetRequestedWidth(Dali::Ui::MATCH_PARENT);
    root.SetRequestedHeight(Dali::Ui::MATCH_PARENT);
    root.SetPadding(Dali::Extents(16, 16, 16, 16));
    window.Add(root);

    if(!mProcessor.ProcessFile(mJsonlPath, mSurface))
    {
      std::fprintf(stderr, "[a2ui-dali] Failed to process '%s': %s\n",
                   mJsonlPath.c_str(), mProcessor.GetLastError().c_str());
      return;
    }

    Dali::Ui::View rendered = mRenderer.Render(mSurface);
    if(rendered)
    {
      root.Add(rendered);
    }
  }

  Dali::Application&        mApplication;
  std::string               mJsonlPath;
  A2ui::A2uiMessageProcessor mProcessor;
  A2ui::SurfaceModel         mSurface;
  A2ui::A2uiRenderer         mRenderer;
};

} // namespace

int DALI_EXPORT_API main(int argc, char** argv)
{
  std::string jsonlPath = (argc > 1) ? argv[1] : "sample.jsonl";

  Dali::WindowData windowData;
  Dali::Rect<int> positionSize(0, 0, 720, 1080);
  windowData.SetPositionSize(positionSize);

  Dali::Application application = Dali::Application::New(
      &argc, &argv, "", /*useUiThread=*/false, windowData);
  Dali::Ui::UiConfig::New().Apply();

  BasicRendererApp demo(application, jsonlPath);
  application.MainLoop();
  return 0;
}
