/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0.
 */

// Custom-catalog example: add an app-specific component to the SAME renderer with
// RegisterComponent — no renderer subclass. The "Badge" handler below builds a coloured
// pill from the component's `label` prop (resolved through the render context, so it can
// be a literal or a {path} data binding), and is rendered alongside the standard catalog.

#include "renderer/a2ui-host.h"
#include "renderer/render-context.h"

#include <dali/public-api/adaptor-framework/application.h>
#include <dali/public-api/adaptor-framework/window-data.h>
#include <dali/public-api/math/rect.h>
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/layouts/flex-layout-params.h>

#include <cstdio>
#include <string>

namespace
{
using namespace Dali;
using namespace Dali::Ui;

// The custom "Badge" component renderer — a green pill with a centred label.
View RenderBadge(const A2ui::ComponentModel& comp, A2ui::RenderContext& rc)
{
  std::string label = comp.rawNode ? rc.ResolveString(comp.rawNode->Find("label")) : std::string();

  FlexLayout badge = FlexLayout::New();
  badge.SetDirection(FlexDirection::ROW);
  badge.SetJustifyContent(FlexJustify::CENTER);
  badge.SetAlignItems(FlexAlign::CENTER);
  badge.SetRequestedWidth(WRAP_CONTENT);
  badge.SetRequestedHeight(28.0f);
  badge.SetCornerRadius(14.0f);
  badge.SetPadding(Extents(14, 14, 4, 4));
  badge.SetBackgroundColor(UiColor(0x0BB075)); // OneUI green
  badge.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));

  Label text = Label::New(label.c_str());
  text.SetTextColor(UiColor(0xFFFFFF));
  text.SetFontSize(13.0f);
  text.SetRequestedWidth(WRAP_CONTENT);
  text.SetRequestedHeight(20.0f);
  badge.Add(text);
  return badge;
}

class CustomComponentApp : public ConnectionTracker
{
public:
  explicit CustomComponentApp(Application& app) : mApp(app)
  {
    mApp.InitSignal().Connect(this, &CustomComponentApp::OnInit);
  }

private:
  void OnInit(Application app)
  {
    Window window = app.GetWindow();
    window.SetBackgroundColor(UiColor(0xFFFFFF));

    FlexLayout root = FlexLayout::New();
    root.SetDirection(FlexDirection::COLUMN);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetPadding(Extents(16, 16, 16, 16));
    window.Add(root);

    mHost.GetRenderer().SetImageDir("res/");

    // Register the custom-catalog component on the same renderer.
    mHost.GetRenderer().RegisterComponent("Badge", &RenderBadge);

    mHost.SetOnBeginRenderingSurface([root](const std::string&, View view) mutable {
      root.Add(view);
    });

    // A surface that mixes standard components with the custom "Badge".
    mHost.JsonFeed(R"JSON([
      {"version":"v0.9","createSurface":{"surfaceId":"s","catalogId":"custom"}},
      {"version":"v0.9","updateComponents":{"surfaceId":"s","components":[
        {"id":"root","component":"Card","child":"col"},
        {"id":"col","component":"Column","children":["title","subtitle","badge"]},
        {"id":"title","component":"Text","text":"Custom catalog demo","variant":"h2"},
        {"id":"subtitle","component":"Text","text":"The green pill below is an app-registered component","variant":"caption"},
        {"id":"badge","component":"Badge","label":"IN STOCK"}
      ]}}
    ])JSON");
  }

  Application&   mApp;
  A2ui::A2uiHost mHost;
};

} // namespace

int DALI_EXPORT_API main(int argc, char** argv)
{
  Dali::WindowData windowData;
  Dali::Rect<int> positionSize(0, 0, 720, 600);
  windowData.SetPositionSize(positionSize);
  Dali::Application application = Dali::Application::New(&argc, &argv, "", false, windowData);
  Dali::Ui::UiConfig::New().Apply();

  CustomComponentApp demo(application);
  application.MainLoop();
  return 0;
}
