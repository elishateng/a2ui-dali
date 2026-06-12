#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderVideo(const ComponentModel& comp, DataContext& ctx)
{
  // dali-ui has no native video surface — render a media-frame look: a dark
  // frame with a centred play glyph (conveys "video here" without a real player).
  FlexLayout frame = FlexLayout::New();
  frame.SetDirection(FlexDirection::COLUMN);
  frame.SetJustifyContent(FlexJustify::CENTER);
  frame.SetAlignItems(FlexAlign::CENTER);
  frame.SetRequestedWidth(MATCH_PARENT);
  frame.SetRequestedHeight(200.0f);
  frame.SetBackgroundColor(UiColor(0x202024));
  frame.SetCornerRadius(Metrics::RadiusCard());
  frame.SetMargin(Extents(0, 0, 6, 6));

  std::string playPath = mImageDir + "icons/play.png";
  std::ifstream f(playPath);
  if(f.is_open())
  {
    f.close();
    ImageView play = ImageView::New(playPath.c_str());
    play.SetRequestedWidth(48.0f);
    play.SetRequestedHeight(48.0f);
    play.SetImageColor(UiColor(0xFFFFFF));
    frame.Add(play);
  }
  return frame;
}
} // namespace A2ui
