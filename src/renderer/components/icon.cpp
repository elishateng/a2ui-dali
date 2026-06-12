#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderIcon(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  // Name may be a literal string or a {path} data binding (e.g. statsCard icon comes
  // from the data model) — resolve through the data context so both work.
  const TreeNode* nameNode = comp.rawNode->Find("name");
  std::string name = nameNode ? ResolveString(nameNode, ctx) : "";
  float size = GetNodeFloat(*comp.rawNode, "size", 24.0f);

  // Tint: default to text color; override via "color". Icons are shipped as WHITE
  // glyphs on transparent (see tools/build_icons.py) so SetImageColor multiplies to
  // the desired colour — mirrors the reference renderer's ImageView.MultipliedColor approach.
  UiColor tint = COLOR_TEXT_DEFAULT;
  const char* color = GetNodeString(*comp.rawNode, "color", "");
  if(color[0] != '\0') tint = ParseHexColor(color);

  std::string iconPath = mImageDir + "icons/" + name + ".png";
  std::ifstream testFile(iconPath);
  if(testFile.is_open())
  {
    testFile.close();
    ImageView icon = ImageView::New(iconPath.c_str());
    icon.SetRequestedWidth(size);
    icon.SetRequestedHeight(size);
    icon.SetImageColor(tint);
    return icon;
  }

  // Graceful fallback for an unknown icon name: a transparent box of the right size
  // (keeps layout) instead of a jarring solid grey square.
  View fallback = View::New();
  fallback.SetRequestedWidth(size);
  fallback.SetRequestedHeight(size);
  return fallback;
}
} // namespace A2ui
