#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderAudioPlayer(const ComponentModel& comp, DataContext& ctx)
{
  // No native audio widget — render an audio-player look: round play button + track.
  // dp-scaled so the control keeps its size at any capture DPI (raw literals rendered at
  // half size under the 2x capture).
  FlexLayout bar = FlexLayout::New();
  bar.SetDirection(FlexDirection::ROW);
  bar.SetAlignItems(FlexAlign::CENTER);
  bar.SetRequestedWidth(MATCH_PARENT);
  bar.SetRequestedHeight(Metrics::Dp(44));
  bar.SetBackgroundColor(COLOR_INPUT_BG);
  bar.SetBorderlineWidth(Metrics::BorderInput());
  bar.SetBorderlineColor(COLOR_INPUT_BORDER);
  bar.SetCornerRadius(Metrics::RadiusCard());
  bar.SetPadding(Extents(static_cast<uint16_t>(Metrics::Dp(12)), static_cast<uint16_t>(Metrics::Dp(12)),
                         static_cast<uint16_t>(Metrics::Dp(6)), static_cast<uint16_t>(Metrics::Dp(6))));
  bar.SetMargin(Extents(0, 0, static_cast<uint16_t>(Metrics::Dp(6)), static_cast<uint16_t>(Metrics::Dp(6))));

  FlexLayout btn = FlexLayout::New();
  btn.SetJustifyContent(FlexJustify::CENTER);
  btn.SetAlignItems(FlexAlign::CENTER);
  btn.SetRequestedWidth(Metrics::Dp(32));
  btn.SetRequestedHeight(Metrics::Dp(32));
  btn.SetBackgroundColor(COLOR_PRIMARY);
  btn.SetCornerRadius(Metrics::Dp(16));
  btn.SetMargin(Extents(0, static_cast<uint16_t>(Metrics::Dp(12)), 0, 0));
  std::string playPath = mImageDir + "icons/play.png";
  std::ifstream f(playPath);
  if(f.is_open())
  {
    f.close();
    ImageView play = ImageView::New(playPath.c_str());
    play.SetRequestedWidth(Metrics::Dp(18));
    play.SetRequestedHeight(Metrics::Dp(18));
    play.SetImageColor(UiColor(0xFFFFFF));
    btn.Add(play);
  }
  bar.Add(btn);

  View track = View::New();
  track.SetRequestedHeight(Metrics::Dp(4));
  track.SetBackgroundColor(COLOR_INPUT_BORDER);
  track.SetCornerRadius(Metrics::RadiusSliderTrack());
  track.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexBasis(0.0f));
  bar.Add(track);

  return bar;
}
} // namespace A2ui
