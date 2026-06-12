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
    divider.SetRequestedWidth(1.0f);
    divider.SetRequestedHeight(MATCH_PARENT);
    divider.SetMargin(Extents(4, 4, 0, 0));
  }
  else
  {
    divider.SetRequestedWidth(MATCH_PARENT);
    divider.SetRequestedHeight(1.0f);
    divider.SetMargin(Extents(0, 0, 8, 8));
  }

  return divider;
}
} // namespace A2ui
