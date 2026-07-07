#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderDivider(const ComponentModel& comp)
{
  const char* axis = comp.rawNode ? GetNodeString(*comp.rawNode, "axis", "horizontal") : "horizontal";

  View divider = View::New();
  divider.SetBackgroundColor(COLOR_DIVIDER);

  if(strcmp(axis, "vertical") == 0)
  {
    divider.SetRequestedWidth(Metrics::BorderInput());
    divider.SetRequestedHeight(MATCH_PARENT);
    divider.SetMargin(Extents(static_cast<uint16_t>(Metrics::Dp(8)), static_cast<uint16_t>(Metrics::Dp(8)), 0, 0));
  }
  else
  {
    // The web Divider carries no extra margin — it sits in the parent Column's 8px gap
    // (the gap supplies ~8px above and below). A self-margin floated it in whitespace.
    divider.SetRequestedWidth(MATCH_PARENT);
    divider.SetRequestedHeight(Metrics::BorderInput());
    divider.SetMargin(Extents(0, 0, 0, 0));
  }

  return divider;
}
} // namespace A2ui
