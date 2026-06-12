#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderCheckBox(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout row = FlexLayout::New();
  row.SetDirection(FlexDirection::ROW);
  row.SetAlignItems(FlexAlign::CENTER);
  row.SetRequestedWidth(MATCH_PARENT);
  row.SetRequestedHeight(WRAP_CONTENT);
  row.SetMargin(Extents(0, 0, 4, 4));

  // Check icon
  View checkIcon = View::New();
  checkIcon.SetRequestedWidth(24.0f);
  checkIcon.SetRequestedHeight(24.0f);
  checkIcon.SetCornerRadius(Metrics::RadiusCheck());

  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";

  bool checked = !boundPath.empty() ? ctx.GetDataModel().GetBool(boundPath, false) : false;
  checkIcon.SetBackgroundColor(checked ? COLOR_CHECK_ON : COLOR_CHECK_OFF);

  // Label
  const char* labelText = GetNodeString(*comp.rawNode, "label", "");
  Label label = Label::New(labelText);
  label.SetFontSize(Metrics::FontChoice());
  label.SetTextColor(COLOR_TEXT_DEFAULT);
  label.SetRequestedWidth(WRAP_CONTENT);
  label.SetRequestedHeight(WRAP_CONTENT);
  label.SetMargin(Extents(8, 0, 0, 0));

  row.Add(checkIcon);
  row.Add(label);

  // Click toggle
  if(!boundPath.empty())
  {
    row.AsInteractive([this, boundPath, ctx](InteractiveTrait& trait) mutable {
      trait.ClickedSignal().Connect(this,
        [boundPath, ctx](View, const InputEvent&) mutable -> bool {
          bool current = ctx.GetDataModel().GetBool(boundPath, false);
          ctx.GetDataModel().SetBoolValue(boundPath, !current);
          return true;
        });
    });

    // Watch for data changes → update icon
    ctx.GetDataModel().Watch(boundPath,
      [checkIcon](const std::string&, const std::string& val) mutable {
        bool on = (val == "true");
        checkIcon.SetBackgroundColor(on ? COLOR_CHECK_ON : COLOR_CHECK_OFF);
      });
  }

  return row;
}
} // namespace A2ui
