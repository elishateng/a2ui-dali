#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderSlider(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetMargin(Extents(0, 0, 4, 4));

  // Label (optional)
  const char* labelText = GetNodeString(*comp.rawNode, "label", "");
  if(labelText[0] != '\0')
  {
    Label label = Label::New(labelText);
    label.SetFontSize(Metrics::FontLabel());
    label.SetTextColor(A2uiTheme::Color("OnSurfaceContainerLow"));  // OneUIColor.OnSurfaceContainerLow
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    container.Add(label);
  }

  float minVal = GetNodeFloat(*comp.rawNode, "min", 0.0f);
  float maxVal = GetNodeFloat(*comp.rawNode, "max", 100.0f);

  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";

  float currentVal = !boundPath.empty()
    ? ctx.GetDataModel().GetFloat(boundPath, minVal)
    : minVal;

  // Value display
  std::ostringstream valStr;
  valStr << static_cast<int>(currentVal);
  Label valueLabel = Label::New(valStr.str().c_str());
  valueLabel.SetFontSize(Metrics::FontButton());
  valueLabel.SetTextColor(COLOR_TEXT_DEFAULT);
  valueLabel.SetRequestedWidth(WRAP_CONTENT);
  valueLabel.SetRequestedHeight(WRAP_CONTENT);
  valueLabel.SetMargin(Extents(0, 0, 2, 4));

  // Track container (visual slider representation)
  FlexLayout trackRow = FlexLayout::New();
  trackRow.SetDirection(FlexDirection::ROW);
  trackRow.SetAlignItems(FlexAlign::CENTER);
  trackRow.SetRequestedWidth(MATCH_PARENT);
  trackRow.SetRequestedHeight(24.0f);

  // Filled portion
  float ratio = (maxVal > minVal) ? (currentVal - minVal) / (maxVal - minVal) : 0.0f;

  View fillTrack = View::New();
  fillTrack.SetRequestedHeight(4.0f);
  fillTrack.SetBackgroundColor(COLOR_SLIDER_FILL);
  fillTrack.SetCornerRadius(Metrics::RadiusSliderTrack());
  fillTrack.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(ratio > 0.01f ? ratio : 0.01f).SetFlexBasis(0.0f));

  View thumb = View::New();
  thumb.SetRequestedWidth(16.0f);
  thumb.SetRequestedHeight(16.0f);
  thumb.SetCornerRadius(Metrics::RadiusSliderThumb());
  thumb.SetBackgroundColor(COLOR_SLIDER_THUMB);

  View emptyTrack = View::New();
  emptyTrack.SetRequestedHeight(4.0f);
  emptyTrack.SetBackgroundColor(COLOR_SLIDER_TRACK);
  emptyTrack.SetCornerRadius(Metrics::RadiusSliderTrack());
  float emptyRatio = 1.0f - ratio;
  emptyTrack.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(emptyRatio > 0.01f ? emptyRatio : 0.01f).SetFlexBasis(0.0f));

  trackRow.Add(fillTrack);
  trackRow.Add(thumb);
  trackRow.Add(emptyTrack);

  // Click on track to change value (simplified interaction)
  if(!boundPath.empty())
  {
    // +/- buttons for keyboard-friendly slider control
    FlexLayout btnRow = FlexLayout::New();
    btnRow.SetDirection(FlexDirection::ROW);
    btnRow.SetJustifyContent(FlexJustify::SPACE_BETWEEN);
    btnRow.SetRequestedWidth(MATCH_PARENT);
    btnRow.SetRequestedHeight(32.0f);

    float step = GetNodeFloat(*comp.rawNode, "step", 1.0f);

    FlexLayout minusBtn = FlexLayout::New();
    minusBtn.SetDirection(FlexDirection::ROW);
    minusBtn.SetJustifyContent(FlexJustify::CENTER);
    minusBtn.SetAlignItems(FlexAlign::CENTER);
    minusBtn.SetRequestedWidth(48.0f);
    minusBtn.SetRequestedHeight(32.0f);
    minusBtn.SetBackgroundColor(COLOR_BTN_SECONDARY);
    minusBtn.SetCornerRadius(Metrics::RadiusInput());
    Label minusLabel = Label::New("-");
    minusLabel.SetFontSize(Metrics::FontStepper());
    minusLabel.SetTextColor(UiColor(0xFFFFFF));
    minusLabel.SetRequestedWidth(WRAP_CONTENT);
    minusLabel.SetRequestedHeight(WRAP_CONTENT);
    minusBtn.Add(minusLabel);

    minusBtn.AsInteractive([this, boundPath, step, minVal, ctx](InteractiveTrait& trait) mutable {
      trait.ClickedSignal().Connect(this,
        [boundPath, step, minVal, ctx](View, const InputEvent&) mutable -> bool {
          float cur = ctx.GetDataModel().GetFloat(boundPath, 0.0f);
          float newVal = cur - step;
          if(newVal < minVal) newVal = minVal;
          ctx.GetDataModel().SetFloatValue(boundPath, newVal);
          return true;
        });
    });

    FlexLayout plusBtn = FlexLayout::New();
    plusBtn.SetDirection(FlexDirection::ROW);
    plusBtn.SetJustifyContent(FlexJustify::CENTER);
    plusBtn.SetAlignItems(FlexAlign::CENTER);
    plusBtn.SetRequestedWidth(48.0f);
    plusBtn.SetRequestedHeight(32.0f);
    plusBtn.SetBackgroundColor(COLOR_BTN_SECONDARY);
    plusBtn.SetCornerRadius(Metrics::RadiusInput());
    Label plusLabel = Label::New("+");
    plusLabel.SetFontSize(Metrics::FontStepper());
    plusLabel.SetTextColor(UiColor(0xFFFFFF));
    plusLabel.SetRequestedWidth(WRAP_CONTENT);
    plusLabel.SetRequestedHeight(WRAP_CONTENT);
    plusBtn.Add(plusLabel);

    plusBtn.AsInteractive([this, boundPath, step, maxVal, ctx](InteractiveTrait& trait) mutable {
      trait.ClickedSignal().Connect(this,
        [boundPath, step, maxVal, ctx](View, const InputEvent&) mutable -> bool {
          float cur = ctx.GetDataModel().GetFloat(boundPath, 0.0f);
          float newVal = cur + step;
          if(newVal > maxVal) newVal = maxVal;
          ctx.GetDataModel().SetFloatValue(boundPath, newVal);
          return true;
        });
    });

    btnRow.Add(minusBtn);
    btnRow.Add(plusBtn);

    container.Add(valueLabel);
    container.Add(trackRow);
    container.Add(btnRow);

    // Watch for value changes
    ctx.GetDataModel().Watch(boundPath,
      [valueLabel, fillTrack, emptyTrack, minVal, maxVal](const std::string&, const std::string& val) mutable {
        float v = 0.0f;
        try { v = std::stof(val); } catch(...) {}
        std::ostringstream oss;
        oss << static_cast<int>(v);
        valueLabel.SetText(Dali::String(oss.str().c_str()));

        float r = (maxVal > minVal) ? (v - minVal) / (maxVal - minVal) : 0.0f;
        if(r < 0.01f) r = 0.01f;
        if(r > 0.99f) r = 0.99f;
        fillTrack.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(r).SetFlexBasis(0.0f));
        emptyTrack.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f - r).SetFlexBasis(0.0f));
      });
  }
  else
  {
    container.Add(valueLabel);
    container.Add(trackRow);
  }

  return container;
}
} // namespace A2ui
