#include "renderer/render-internal.h"

namespace A2ui
{

// Helper structure for choice picker options
struct ChoiceOptionData {
  std::string label;
  std::string value;
};

// Extract option label/value from TreeNode
ChoiceOptionData ExtractOptionData(const TreeNode& optNode) {
  const TreeNode* optLabelNode = optNode.Find("label");
  const TreeNode* optValueNode = optNode.Find("value");

  std::string optLabel = (optLabelNode && optLabelNode->GetType() == TreeNode::STRING)
                        ? optLabelNode->GetString() : "";
  std::string optValue = (optValueNode && optValueNode->GetType() == TreeNode::STRING)
                        ? optValueNode->GetString() : "";

  return {optLabel, optValue};
}

View A2uiRenderer::RenderChoicePicker(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  std::string displayStyle = GetNodeString(*comp.rawNode, "displayStyle", "");

  // Common: Get bound path and current value
  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";
  const Dali::Ui::TreeNode* arrayNode = !boundPath.empty() ? ctx.GetDataModel().ResolvePath(boundPath) : nullptr;
  std::string currentValue = "";
  if(arrayNode && arrayNode->GetType() == TreeNode::ARRAY && arrayNode->CBegin() != arrayNode->CEnd()) {
    // Check if array is not empty and get the first element's value
    currentValue = (*arrayNode->CBegin()).second.GetString();
  } else if (arrayNode) {
    currentValue = ctx.GetDataModel().GetString(boundPath);
  }

  // Common: Validate options
  const TreeNode* optionsNode = comp.rawNode->Find("options");
  if(!optionsNode || optionsNode->GetType() != TreeNode::ARRAY) {
    return View::New();
  }

  if(displayStyle == "chips") {
    return RenderChoicePickerChips(comp, ctx, boundPath, currentValue, optionsNode);
  } else {
    return RenderChoicePickerRadio(comp, ctx, boundPath, currentValue, optionsNode);
  }
}

// Chips style rendering
View A2uiRenderer::RenderChoicePickerChips(const ComponentModel& comp, DataContext& ctx,
                                           const std::string& boundPath,
                                           const std::string& currentValue,
                                           const TreeNode* optionsNode)
{
  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::ROW);
  container.SetAlignItems(FlexAlign::CENTER);
  container.SetRequestedWidth(WRAP_CONTENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetMargin(Extents(4, 4, 0, 0));

  // Collect option views for reactive updates
  std::vector<std::pair<Label, std::string>> optionViews;

  for(auto it = optionsNode->CBegin(); it != optionsNode->CEnd(); ++it) {
    auto optData = ExtractOptionData((*it).second);
    const std::string& optLabel = optData.label;
    const std::string& optValue = optData.value;

    // Option label
    Label optLbl = Label::New(optLabel.c_str());
    optLbl.SetFontSize(15.0f);
    optLbl.SetRequestedWidth(WRAP_CONTENT);
    optLbl.SetRequestedHeight(WRAP_CONTENT);
    optLbl.SetMargin(Extents(8, 0, 0, 0));
    optLbl.SetPadding(Extents(10, 10, 8, 8));
    optLbl.SetCornerRadius(10);

    bool selected = (optValue == currentValue);
    optLbl.SetTextColor(selected ? COLOR_CHECK_OFF : COLOR_CHECK_ON);
    optLbl.SetBackgroundColor(selected ? COLOR_CHECK_ON : COLOR_CHECK_OFF);

    optionViews.push_back({optLbl, optValue});

    // Click handler
    if(!boundPath.empty()) {
      optLbl.AsInteractive([this, optValue, boundPath, ctx](InteractiveTrait& trait) mutable {
        trait.ClickedSignal().Connect(this,
          [optValue, boundPath, ctx](View, const InputEvent&) mutable -> bool {
            ctx.GetDataModel().SetValue(boundPath, optValue);
            return true;
          });
       });
    }

    container.Add(optLbl);
  }

  // Watch for value changes → update all option views
  if(!boundPath.empty()) {
    ctx.GetDataModel().Watch(boundPath,
      [optionViews](const std::string&, const std::string& val) mutable {
        for(auto& pair : optionViews) {
          bool selected = (pair.second == val);
          pair.first.SetBackgroundColor(selected ? COLOR_CHECK_ON : COLOR_CHECK_OFF);
          pair.first.SetTextColor(selected ? COLOR_CHECK_OFF : COLOR_CHECK_ON);
        }
      });
  }

  return container;
}

// Radio style rendering
View A2uiRenderer::RenderChoicePickerRadio(const ComponentModel& comp, DataContext& ctx,
                                           const std::string& boundPath,
                                           const std::string& currentValue,
                                           const TreeNode* optionsNode)
{
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

  // Collect option views for reactive updates
  std::vector<std::pair<View, std::string>> optionIcons;

  for(auto it = optionsNode->CBegin(); it != optionsNode->CEnd(); ++it)
  {
    auto optData = ExtractOptionData((*it).second);
    const std::string& optLabel = optData.label;
    const std::string& optValue = optData.value;

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
