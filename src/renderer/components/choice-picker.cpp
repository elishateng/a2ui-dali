#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderChoicePicker(const ComponentModel& comp, DataContext& ctx)
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
    label.SetMargin(Extents(0, 0, 0, 4));
    container.Add(label);
  }

  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";
  std::string currentValue = !boundPath.empty() ? ctx.GetDataModel().GetString(boundPath) : "";

  const TreeNode* optionsNode = comp.rawNode->Find("options");
  if(!optionsNode || optionsNode->GetType() != TreeNode::ARRAY)
  {
    return container;
  }

  // Collect option views for reactive updates
  std::vector<std::pair<View, std::string>> optionIcons;

  for(auto it = optionsNode->CBegin(); it != optionsNode->CEnd(); ++it)
  {
    const TreeNode& optNode = (*it).second;
    const TreeNode* optLabelNode = optNode.Find("label");
    const TreeNode* optValueNode = optNode.Find("value");

    std::string optLabel = (optLabelNode && optLabelNode->GetType() == TreeNode::STRING)
                           ? optLabelNode->GetString() : "";
    std::string optValue = (optValueNode && optValueNode->GetType() == TreeNode::STRING)
                           ? optValueNode->GetString() : "";

    FlexLayout optRow = FlexLayout::New();
    optRow.SetDirection(FlexDirection::ROW);
    optRow.SetAlignItems(FlexAlign::CENTER);
    optRow.SetRequestedWidth(MATCH_PARENT);
    optRow.SetRequestedHeight(WRAP_CONTENT);
    optRow.SetPadding(Extents(8, 8, 10, 10));

    // Radio circle
    View radioIcon = View::New();
    radioIcon.SetRequestedWidth(20.0f);
    radioIcon.SetRequestedHeight(20.0f);
    radioIcon.SetCornerRadius(Metrics::RadiusRadio());
    bool selected = (optValue == currentValue);
    radioIcon.SetBackgroundColor(selected ? COLOR_CHECK_ON : COLOR_CHECK_OFF);
    optRow.Add(radioIcon);

    optionIcons.push_back({radioIcon, optValue});

    // Option label
    Label optLbl = Label::New(optLabel.c_str());
    optLbl.SetFontSize(Metrics::FontChoice());
    optLbl.SetTextColor(COLOR_TEXT_DEFAULT);
    optLbl.SetRequestedWidth(WRAP_CONTENT);
    optLbl.SetRequestedHeight(WRAP_CONTENT);
    optLbl.SetMargin(Extents(8, 0, 0, 0));
    optRow.Add(optLbl);

    // Click handler
    if(!boundPath.empty())
    {
      optRow.AsInteractive([this, optValue, boundPath, ctx](InteractiveTrait& trait) mutable {
        trait.ClickedSignal().Connect(this,
          [optValue, boundPath, ctx](View, const InputEvent&) mutable -> bool {
            ctx.GetDataModel().SetValue(boundPath, optValue);
            return true;
          });
      });
    }

    container.Add(optRow);
  }

  // Watch for value changes → update all radio icons
  if(!boundPath.empty())
  {
    ctx.GetDataModel().Watch(boundPath,
      [optionIcons](const std::string&, const std::string& val) mutable {
        for(auto& pair : optionIcons)
        {
          bool selected = (pair.second == val);
          pair.first.SetBackgroundColor(selected ? COLOR_CHECK_ON : COLOR_CHECK_OFF);
        }
      });
  }

  return container;
}
} // namespace A2ui
