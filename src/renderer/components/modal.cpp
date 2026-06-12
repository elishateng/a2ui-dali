#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderModal(const ComponentModel& comp,
                               const SurfaceComponentsModel& components,
                               DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  // dali-ui has no native modal/dialog. Compose one: the trigger is rendered visibly,
  // and tapping it reveals the content card (hidden until then), with an X to close.
  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetAlignItems(FlexAlign::CENTER);

  // Trigger — the visible element (accept "trigger" or v0.8 "entryPointChild").
  const TreeNode* triggerNode = comp.rawNode->Find("trigger");
  if(!triggerNode) triggerNode = comp.rawNode->Find("entryPointChild");
  View triggerView;
  if(triggerNode && triggerNode->GetType() == TreeNode::STRING)
  {
    triggerView = RenderComponent(triggerNode->GetString(), components, ctx);
    if(triggerView) container.Add(triggerView);
  }

  // Content card — hidden until the trigger is tapped.
  FlexLayout modalCard = FlexLayout::New();
  modalCard.SetDirection(FlexDirection::COLUMN);
  modalCard.SetRequestedWidth(MATCH_PARENT);
  modalCard.SetRequestedHeight(WRAP_CONTENT);
  modalCard.SetBackgroundColor(COLOR_CARD_BG);
  modalCard.SetCornerRadius(16.0f);
  modalCard.SetBorderlineWidth(Metrics::BorderCard());
  modalCard.SetBorderlineColor(A2uiTheme::Color("Outline"));
  modalCard.SetPadding(Extents(24, 24, 20, 20));
  modalCard.SetMargin(Extents(8, 8, 12, 8));
  modalCard.SetProperty(Actor::Property::VISIBLE, false);

  // Close (X) row.
  FlexLayout closeRow = FlexLayout::New();
  closeRow.SetDirection(FlexDirection::ROW);
  closeRow.SetJustifyContent(FlexJustify::FLEX_END);
  closeRow.SetRequestedWidth(MATCH_PARENT);
  closeRow.SetRequestedHeight(28.0f);
  Label closeLabel = Label::New("X");
  closeLabel.SetFontSize(Metrics::FontInput());
  closeLabel.SetTextColor(A2uiTheme::Color("OnSurfaceContainerLow"));
  closeLabel.SetRequestedWidth(28.0f);
  closeLabel.SetRequestedHeight(28.0f);
  closeRow.Add(closeLabel);
  {
    Dali::TapGestureDetector closeDet = Dali::TapGestureDetector::New();
    closeDet.Attach(closeRow);
    closeDet.DetectedSignal().Connect(this,
      [modalCard](Dali::Actor, const Dali::TapGesture&) mutable {
        modalCard.SetProperty(Actor::Property::VISIBLE, false);
      });
    mTapDetectors.push_back(closeDet);
  }
  modalCard.Add(closeRow);

  // Content (accept "content" or v0.8 "contentChild").
  const TreeNode* contentNode = comp.rawNode->Find("content");
  if(!contentNode) contentNode = comp.rawNode->Find("contentChild");
  if(contentNode && contentNode->GetType() == TreeNode::STRING)
  {
    View content = RenderComponent(contentNode->GetString(), components, ctx);
    if(content) modalCard.Add(content);
  }
  container.Add(modalCard);

  // Tapping the trigger reveals the card.
  if(triggerView)
  {
    Dali::TapGestureDetector openDet = Dali::TapGestureDetector::New();
    openDet.Attach(triggerView);
    openDet.DetectedSignal().Connect(this,
      [modalCard](Dali::Actor, const Dali::TapGesture&) mutable {
        modalCard.SetProperty(Actor::Property::VISIBLE, true);
      });
    mTapDetectors.push_back(openDet);
  }

  return container;
}
} // namespace A2ui
