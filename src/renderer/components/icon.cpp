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
  // Inline icons (the bullet glyph beside a contact phone/email row, a status check, a
  // weather/trend arrow) measure ~16-18px in the web gallery; the catalog default of 24 made
  // every icon+text row ~6px taller than web and over-weighted the glyph. 18 matches; a
  // message can still request an explicit size.
  float size = GetNodeFloat(*comp.rawNode, "size", 18.0f);

  // Tint: default to text color; override via "color". Icons are shipped as WHITE
  // glyphs on transparent (see tools/build_icons.py) so SetImageColor multiplies to
  // the desired colour — mirrors the reference renderer's ImageView.MultipliedColor approach.
  // Default to the MUTED/secondary colour — the web renders these inline & header icons (bullet,
  // status, calendar/pin, trend arrow) as light-grey Material outline glyphs, NOT near-black.
  // Tinting to the primary text colour made every icon read as a heavy dark blob. A message can
  // still set an explicit `color`.
  UiColor tint = COLOR_TEXT_MUTED;
  const char* color = GetNodeString(*comp.rawNode, "color", "");
  if(color[0] != '\0') tint = ParseHexColor(color);

  std::string iconPath = mImageDir + "icons/" + name + ".png";
  std::ifstream testFile(iconPath);
  if(testFile.is_open())
  {
    testFile.close();
    ImageView icon = ImageView::New(iconPath.c_str());
    // dp-scale the size — every other component sizes via Metrics::Dp(), but this used the raw
    // logical value, so at the 2x capture DPI every icon rendered at HALF its intended size and
    // read as a faint tiny speck next to the (dp-scaled) text (the contact bullets, header
    // payment/run/calendar/star glyphs, the trend arrow). dp-scaling matches the web glyph size.
    icon.SetRequestedWidth(Metrics::Dp(size));
    icon.SetRequestedHeight(Metrics::Dp(size));
    icon.SetImageColor(tint);
    return icon;
  }

  // Graceful fallback for an unknown icon name: a transparent box of the right size
  // (keeps layout) instead of a jarring solid grey square.
  View fallback = View::New();
  fallback.SetRequestedWidth(Metrics::Dp(size));
  fallback.SetRequestedHeight(Metrics::Dp(size));
  return fallback;
}
} // namespace A2ui
