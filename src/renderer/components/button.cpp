#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderButton(const ComponentModel& comp,
                                const SurfaceComponentsModel& components,
                                DataContext& ctx)
{
  // Live-composer styles every button as a white outlined pill (default/primary alike);
  // only `borderless` drops the fill+border to a text-only button. The A2UI default
  // variant is "default".
  const char* variant = comp.rawNode ? GetNodeString(*comp.rawNode, "variant", "default") : "default";
  bool isBorderless = (strcmp(variant, "borderless") == 0);

  // Single FlexLayout — the previous nested (borderWrap + button) structure
  // collapsed to zero under DALi's column-align:stretch when the child label
  // was WRAP_CONTENT. Fixed size + a single container avoids that entirely.
  const float kBtnHeight = Metrics::ButtonHeight();       // 40 (dp-scaled)
  const float kBtnMinWidth = Metrics::ButtonMinWidth();   // 140
  const float kCharWidth14pt = Metrics::ButtonCharWidth();// glyph advance at the button font
  const float kLabelPadding = Metrics::ButtonLabelPad();  // cushion each side inside the button
  const uint16_t kPadX = static_cast<uint16_t>(Metrics::Dp(16));
  const uint16_t kPadY = static_cast<uint16_t>(Metrics::Dp(8));

  FlexLayout button = FlexLayout::New();
  button.SetDirection(FlexDirection::ROW);
  button.SetJustifyContent(FlexJustify::CENTER);
  button.SetAlignItems(FlexAlign::CENTER);
  button.SetRequestedHeight(kBtnHeight);
  button.SetMinimumHeight(kBtnHeight);
  button.SetMaximumHeight(kBtnHeight);  // pin the height so every button is uniform
  button.SetCornerRadius(kBtnHeight * 0.5f);  // OneUI CornerRadius 0.5 → pill
  button.SetPadding(Extents(kPadX, kPadX, kPadY, kPadY));
  // No self-margin: the container's gap handles spacing. (A self-margin was overwritten
  // on the 2nd+ row child by the gap logic, making buttons in a Row look uneven.)
  // Don't let the parent Column's align:stretch override our fixed size.
  button.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));

  // White outlined pill: white fill, light outline, near-black label. borderless = text-only.
  UiColor bgColor = COLOR_CARD_BG;       // white surface
  UiColor fgColor = COLOR_TEXT_DEFAULT;  // near-black label
  bool outlined = true;
  if(isBorderless)
  {
    bgColor = UiColor(0x00000000);
    outlined = false;
  }
  button.SetBackgroundColor(bgColor);
  if(outlined)
  {
    button.SetBorderlineWidth(Metrics::BorderInput());      // 1 dp
    button.SetBorderlineColor(COLOR_BTN_BORDER);            // Outline #e5e5e5
  }

  float btnWidth = kBtnMinWidth;
  if(!comp.childId.empty())
  {
    View childView = RenderComponent(comp.childId, components, ctx);
    Label labelChild = Label::DownCast(childView);
    if(labelChild)
    {
      labelChild.SetTextColor(fgColor);
      labelChild.SetFontWeight(Text::FontWeight::MEDIUM);
      labelChild.SetFontSize(Metrics::FontButton());
      labelChild.SetHorizontalTextAlignment(Text::Alignment::CENTER);
      // Grow the label box to fit the full glyph run so text like "Submit
      // Reservation" isn't ellipsized. Button width tracks the label so the
      // padding is uniform on both sides regardless of text length.
      Dali::String text = labelChild.GetText();
      std::size_t byteLen = text.Size();
      labelChild.SetMultiLine(false);
      labelChild.SetRequestedHeight(Metrics::Dp(20));
      // Only an actual single glyph / emoji / symbol gets the fixed square; short ASCII
      // words like "Yes"/"No"/"OK" must be sized as text or they clip to a stub ("—").
      const char* tbytes = text.CStr();
      bool hasNonAscii = false;
      for(std::size_t i = 0; i < byteLen; ++i)
        if(static_cast<unsigned char>(tbytes[i]) >= 0x80) { hasNonAscii = true; break; }
      bool isGlyphButton = (byteLen <= 4 && (hasNonAscii || byteLen <= 1));
      if(isGlyphButton)
      {
        // Single glyph / icon / emoji button → a fixed square so a row of them is
        // uniform (e.g. music-player prev/play/next); the pill radius = a circle.
        labelChild.SetRequestedWidth(kBtnHeight);
        btnWidth = kBtnHeight;
      }
      else
      {
        // Text button → size to content with a small floor (the reference renderer has no
        // large MinimumWidth) so e.g. "Sign up" stays snug instead of a wide pill.
        float labelWidth = static_cast<float>(byteLen) * kCharWidth14pt + kLabelPadding * 2.0f;
        labelChild.SetRequestedWidth(labelWidth);
        btnWidth = std::max(Metrics::Dp(72), labelWidth + Metrics::Dp(32));
      }  // dp padding each side
    }
    button.Add(childView);
  }
  button.SetRequestedWidth(btnWidth);

  // Action handling
  //
  // Note: dali-ui has no native Button — this FlexLayout *is* the button.
  // We attach an independent TapGestureDetector instead of relying on
  // InteractiveTrait or TouchedSignal, because neither fires reliably for
  // Button layouts nested inside a ScrollView/List ancestor (the parent's
  // pan detector ends up consuming the touch sequence before our view's
  // click signal can be synthesized). The detector handle must outlive the
  // view, so we park it in mTapDetectors.
  if(comp.rawNode)
  {
    const TreeNode* actionNode = comp.rawNode->Find("action");
    if(actionNode)
    {
      std::string compId = comp.id;
      DataContext capturedCtx = ctx;
      Dali::TapGestureDetector detector = Dali::TapGestureDetector::New();
      detector.Attach(button);
      detector.DetectedSignal().Connect(this,
        [this, actionNode, compId, capturedCtx](
          Dali::Actor, const Dali::TapGesture&) mutable {
          mActionDispatcher.Dispatch(*actionNode, compId, capturedCtx);
        });
      mTapDetectors.push_back(detector);
    }
  }

  return button;
}
} // namespace A2ui
