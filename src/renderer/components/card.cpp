#include "renderer/render-internal.h"

namespace A2ui
{

namespace
{
std::pair<const TreeNode*, std::string>
FindFirstActionInSubtree(const std::string& compId,
                         const SurfaceComponentsModel& components)
{
  const ComponentModel* comp = components.GetComponent(compId);
  if(!comp || !comp->rawNode) return {nullptr, std::string()};

  if(const TreeNode* actionNode = comp->rawNode->Find("action"))
  {
    return {actionNode, compId};
  }

  if(!comp->childId.empty())
  {
    auto r = FindFirstActionInSubtree(comp->childId, components);
    if(r.first) return r;
  }
  for(const auto& cid : comp->childIds)
  {
    auto r = FindFirstActionInSubtree(cid, components);
    if(r.first) return r;
  }
  return {nullptr, std::string()};
}
} // namespace

View A2uiRenderer::RenderCard(const ComponentModel& comp,
                              const SurfaceComponentsModel& components,
                              DataContext& ctx)
{
  FlexLayout card = FlexLayout::New();
  card.SetDirection(FlexDirection::COLUMN);
  card.SetRequestedWidth(MATCH_PARENT);
  card.SetRequestedHeight(WRAP_CONTENT);

  const char* bg = comp.rawNode ? GetNodeString(*comp.rawNode, "background", "") : "";
  card.SetBackgroundColor(bg[0] != '\0' ? ParseHexColor(bg) : COLOR_CARD_BG);
  card.SetCornerRadius(Metrics::RadiusCard());
  card.SetBorderlineWidth(Metrics::BorderCard());
  card.SetBorderlineColor(A2uiTheme::Color("OutlineLow"));  // subtle edge for a white card (web look)

  // Card padding: a message-declared `padding` (logical units) wins, else the web
  // composer's ~24 inset. Both dp-scaled so the inset tracks density / high-DPI capture
  // (a raw uint16 would be only half the intended inset under the 2x capture DPI).
  float padding = comp.rawNode ? GetNodeFloat(*comp.rawNode, "padding", -1.0f) : -1.0f;
  float padPx = (padding >= 0.0f) ? Metrics::Dp(padding) : Metrics::PadCard();
  uint16_t p = static_cast<uint16_t>(padPx);
  card.SetPadding(Extents(p, p, p, p));
  card.SetMargin(Extents(0, 0, static_cast<uint16_t>(Metrics::Dp(10)), 0));

  if(!comp.childId.empty())
  {
    View childView = RenderComponent(comp.childId, components, ctx);
    card.Add(childView);
  }

  // DALi FlexLayout drops the bottom padding of a COLUMN sized WRAP_CONTENT (the wrapped
  // height = top-padding + children, omitting the bottom inset), so every card rendered
  // ~one padding shorter than the web — worst on short cards where that's a big fraction.
  // Add an explicit bottom spacer equal to the padding to restore the symmetric inset.
  View bottomPad = View::New();
  bottomPad.SetRequestedWidth(MATCH_PARENT);
  bottomPad.SetRequestedHeight(padPx);
  card.Add(bottomPad);

  // Fallback tap target: if this card contains a descendant with an
  // `action` (typically a Button — dali-ui has no native Button, so the
  // FlexLayout-based button can fail to receive taps inside a List/Scroll
  // ancestor), attach a TapGestureDetector to the card itself and dispatch
  // that action on tap. A separate detector attached to the Button view
  // runs in parallel; DALi's gesture system routes a tap to the innermost
  // attached detector first, so tapping on the Button fires only the
  // Button handler, and tapping elsewhere on the card fires this one.
  auto actionInfo = FindFirstActionInSubtree(comp.id, components);
  if(actionInfo.first)
  {
    const TreeNode* actionNode = actionInfo.first;
    std::string sourceId = actionInfo.second;
    DataContext capturedCtx = ctx;
    Dali::TapGestureDetector detector = Dali::TapGestureDetector::New();
    detector.Attach(card);
    detector.DetectedSignal().Connect(this,
      [this, actionNode, sourceId, capturedCtx](
        Dali::Actor, const Dali::TapGesture&) mutable {
        mActionDispatcher.Dispatch(*actionNode, sourceId, capturedCtx);
      });
    mTapDetectors.push_back(detector);
  }

  return card;
}
} // namespace A2ui
