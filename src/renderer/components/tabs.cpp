#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderTabs(const ComponentModel& comp,
                              const SurfaceComponentsModel& components,
                              DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);

  const TreeNode* tabsNode = comp.rawNode->Find("tabs");
  if(!tabsNode || tabsNode->GetType() != TreeNode::ARRAY)
  {
    return container;
  }

  // Tab header row
  FlexLayout tabHeader = FlexLayout::New();
  tabHeader.SetDirection(FlexDirection::ROW);
  tabHeader.SetRequestedWidth(MATCH_PARENT);
  tabHeader.SetRequestedHeight(44.0f);

  // Content area
  FlexLayout contentArea = FlexLayout::New();
  contentArea.SetDirection(FlexDirection::COLUMN);
  contentArea.SetRequestedWidth(MATCH_PARENT);
  contentArea.SetRequestedHeight(WRAP_CONTENT);

  // Collect tab content views
  std::vector<View> tabContentViews;
  std::vector<FlexLayout> tabButtons;

  int tabIndex = 0;
  for(auto it = tabsNode->CBegin(); it != tabsNode->CEnd(); ++it)
  {
    const TreeNode& tabDef = (*it).second;
    const TreeNode* titleNode = tabDef.Find("title");
    const TreeNode* childNode = tabDef.Find("child");

    std::string title = (titleNode && titleNode->GetType() == TreeNode::STRING)
                        ? titleNode->GetString() : "Tab";
    std::string childId = (childNode && childNode->GetType() == TreeNode::STRING)
                          ? childNode->GetString() : "";

    // Tab button
    FlexLayout tabBtn = FlexLayout::New();
    tabBtn.SetDirection(FlexDirection::ROW);
    tabBtn.SetJustifyContent(FlexJustify::CENTER);
    tabBtn.SetAlignItems(FlexAlign::CENTER);
    tabBtn.SetRequestedHeight(44.0f);
    tabBtn.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexBasis(0.0f));
    tabBtn.SetBackgroundColor(tabIndex == 0 ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE);
    tabBtn.SetCornerRadius(6.0f, 6.0f, 0.0f, 0.0f);

    Label tabLabel = Label::New(title.c_str());
    tabLabel.SetFontSize(Metrics::FontButton());
    tabLabel.SetTextColor(UiColor(0xFFFFFF));
    tabLabel.SetRequestedWidth(WRAP_CONTENT);
    tabLabel.SetRequestedHeight(WRAP_CONTENT);
    tabBtn.Add(tabLabel);

    tabButtons.push_back(tabBtn);
    tabHeader.Add(tabBtn);

    // Tab content
    View tabContent;
    if(!childId.empty())
    {
      tabContent = RenderComponent(childId, components, ctx);
    }
    else
    {
      tabContent = View::New();
    }
    tabContent.SetRequestedWidth(MATCH_PARENT);
    tabContent.SetProperty(Actor::Property::VISIBLE, tabIndex == 0);
    tabContentViews.push_back(tabContent);
    contentArea.Add(tabContent);

    tabIndex++;
  }

  // Tab click handlers
  for(int i = 0; i < static_cast<int>(tabButtons.size()); ++i)
  {
    tabButtons[i].AsInteractive(
      [this, i, tabButtons, tabContentViews](InteractiveTrait& trait) mutable {
        trait.ClickedSignal().Connect(this,
          [i, tabButtons, tabContentViews](View, const InputEvent&) mutable -> bool {
            for(int j = 0; j < static_cast<int>(tabButtons.size()); ++j)
            {
              tabButtons[j].SetBackgroundColor(j == i ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE);
              tabContentViews[j].SetProperty(Actor::Property::VISIBLE, j == i);
            }
            return true;
          });
      });
  }

  container.Add(tabHeader);
  container.Add(contentArea);
  return container;
}
} // namespace A2ui
