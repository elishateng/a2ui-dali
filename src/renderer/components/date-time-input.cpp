#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderDateTimeInput(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetMargin(Extents(0, 0, 4, 4));

  const char* labelText = GetNodeString(*comp.rawNode, "label", "Date/Time");
  Label label = Label::New(labelText);
  label.SetFontSize(Metrics::FontLabel());
  label.SetTextColor(A2uiTheme::Color("OnSurfaceContainerLow"));  // OneUIColor.OnSurfaceContainerLow
  label.SetRequestedWidth(MATCH_PARENT);
  label.SetRequestedHeight(WRAP_CONTENT);
  label.SetMargin(Extents(0, 0, 0, 4));
  container.Add(label);

  // Same Label-based workaround as TextField (InputField OnChildAdd crash)
  FlexLayout inputBox = FlexLayout::New();
  inputBox.SetDirection(FlexDirection::ROW);
  inputBox.SetAlignItems(FlexAlign::CENTER);
  inputBox.SetRequestedWidth(MATCH_PARENT);
  inputBox.SetRequestedHeight(44.0f);
  inputBox.SetBackgroundColor(COLOR_INPUT_BG);
  inputBox.SetCornerRadius(Metrics::RadiusInput());
  inputBox.SetPadding(Extents(12, 12, 0, 0));

  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";

  std::string displayText;
  if(!boundPath.empty())
  {
    displayText = ctx.GetDataModel().GetString(boundPath);
  }
  if(displayText.empty()) displayText = "YYYY-MM-DD";

  Label inputLabel = Label::New(displayText.c_str());
  inputLabel.SetFontSize(Metrics::FontInput());
  inputLabel.SetTextColor(displayText == "YYYY-MM-DD" ? A2uiTheme::Color("OnSurfaceContainerLow") : COLOR_TEXT_DEFAULT);
  inputLabel.SetRequestedWidth(MATCH_PARENT);
  inputLabel.SetRequestedHeight(WRAP_CONTENT);
  inputBox.Add(inputLabel);

  if(!boundPath.empty())
  {
    ctx.GetDataModel().Watch(boundPath,
      [inputLabel](const std::string&, const std::string& val) mutable {
        inputLabel.SetText(Dali::String(val.c_str()));
        inputLabel.SetTextColor(val.empty() ? A2uiTheme::Color("OnSurfaceContainerLow") : COLOR_TEXT_DEFAULT);
      });
  }

  container.Add(inputBox);

  // Error label
  Label errorLabel = Label::New("");
  errorLabel.SetFontSize(Metrics::FontError());
  errorLabel.SetTextColor(COLOR_ERROR);
  errorLabel.SetRequestedWidth(MATCH_PARENT);
  errorLabel.SetRequestedHeight(WRAP_CONTENT);
  errorLabel.SetProperty(Actor::Property::VISIBLE, false);
  container.Add(errorLabel);

  if(!boundPath.empty())
  {
    SetupChecks(comp, ctx, errorLabel, boundPath);
  }

  return container;
}
} // namespace A2ui
