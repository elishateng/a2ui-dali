#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderAudioPlayer(const ComponentModel& comp, DataContext& ctx)
{
  // No native audio widget — render an audio-player look: round play button + track.
  FlexLayout bar = FlexLayout::New();
  bar.SetDirection(FlexDirection::ROW);
  bar.SetAlignItems(FlexAlign::CENTER);
  bar.SetRequestedWidth(MATCH_PARENT);
  bar.SetRequestedHeight(56.0f);
  bar.SetBackgroundColor(COLOR_INPUT_BG);
  bar.SetBorderlineWidth(Metrics::BorderInput());
  bar.SetBorderlineColor(COLOR_INPUT_BORDER);
  bar.SetCornerRadius(Metrics::RadiusCard());
  bar.SetPadding(Extents(12, 12, 8, 8));
  bar.SetMargin(Extents(0, 0, 6, 6));

  FlexLayout btn = FlexLayout::New();
  btn.SetJustifyContent(FlexJustify::CENTER);
  btn.SetAlignItems(FlexAlign::CENTER);
  btn.SetRequestedWidth(40.0f);
  btn.SetRequestedHeight(40.0f);
  btn.SetBackgroundColor(COLOR_PRIMARY);
  btn.SetCornerRadius(20.0f);
  btn.SetMargin(Extents(0, 12, 0, 0));
  std::string playPath = mImageDir + "icons/play.png";
  std::ifstream f(playPath);
  if(f.is_open())
  {
    f.close();
    ImageView play = ImageView::New(playPath.c_str());
    play.SetRequestedWidth(22.0f);
    play.SetRequestedHeight(22.0f);
    play.SetImageColor(UiColor(0xFFFFFF));
    btn.Add(play);
  }
  bar.Add(btn);

  View track = View::New();
  track.SetRequestedHeight(4.0f);
  track.SetBackgroundColor(COLOR_INPUT_BORDER);
  track.SetCornerRadius(Metrics::RadiusSliderTrack());
  track.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexBasis(0.0f));
  bar.Add(track);

  return bar;
}
} // namespace A2ui
