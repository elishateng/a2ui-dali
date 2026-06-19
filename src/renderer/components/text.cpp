#include "renderer/render-internal.h"

namespace A2ui
{

namespace
{
// dali-ui has no markdown renderer; the web composer renders Text content as markdown.
// Strip the inline/structural markers that would otherwise show as literal characters
// (a leading "# " heading prefix, **bold**/_em_ wrappers, `code` ticks) so the visible
// text matches the web instead of leaking raw markdown (e.g. a stray "#").
std::string StripMarkdown(const std::string& in)
{
  std::string s = in;
  // Leading ATX heading markers: "#", "##" … followed by space(s).
  std::size_t p = 0;
  while(p < s.size() && s[p] == '#') ++p;
  if(p > 0 && p <= 6 && p < s.size() && s[p] == ' ')
  {
    while(p < s.size() && s[p] == ' ') ++p;
    s = s.substr(p);
  }
  // Inline emphasis/code markers: remove the marker runs, keep the wrapped text.
  std::string out;
  out.reserve(s.size());
  for(std::size_t i = 0; i < s.size();)
  {
    if(s[i] == '*' || s[i] == '_')          // * ** _ __  emphasis
    {
      char m = s[i];
      while(i < s.size() && s[i] == m) ++i;  // skip the whole run
      continue;
    }
    if(s[i] == '`') { ++i; continue; }       // `code`
    out += s[i++];
  }
  return out;
}
} // namespace

View A2uiRenderer::RenderText(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  const TreeNode* textNode = comp.rawNode->Find("text");
  std::string text = textNode ? ResolveString(textNode, ctx) : "";
  text = StripMarkdown(text);

  const char* variant = GetNodeString(*comp.rawNode, "variant", "body");
  float fontSize = VariantToFontSize(variant);

  Label label = Label::New(text.c_str());
  label.SetFontSize(fontSize);
  label.SetRequestedWidth(MATCH_PARENT);
  label.SetTextColor(COLOR_TEXT_DEFAULT);
  label.SetMultiLine(true);
  label.SetLineWrapMode(Text::LineWrapMode::WORD);

  // DALi Label inside a flex-grown parent sometimes measures height=0 when the parent's
  // resolved width is only known after the layout pass, so we reserve a height estimate.
  // Estimate chars-per-line from the font size against the card content width, and use the
  // web type scale's line height per line so wrapped copy occupies the same vertical space
  // as the web renderer. All terms are dp-scaled, so the estimate holds at any capture DPI.
  float lineH = Metrics::LineHeight(fontSize);
  if(!text.empty())
  {
    int charsPerLine = static_cast<int>(Metrics::CardContentWidth() / (fontSize * 0.52f));
    if(charsPerLine < 8) charsPerLine = 8;
    int estLines = static_cast<int>((text.size() + charsPerLine - 1) / charsPerLine);
    if(estLines < 1) estLines = 1;
    if(estLines > 20) estLines = 20;
    label.SetRequestedHeight(lineH * estLines);
  }
  else
  {
    // Data-bound text is usually empty at first paint (its value arrives in a later
    // updateDataModel); reserve one line so the flex keeps the slot and the watch's
    // update becomes visible instead of the row collapsing the value away.
    bool dataBound = textNode && textNode->GetType() == TreeNode::OBJECT;
    label.SetRequestedHeight(dataBound ? lineH : WRAP_CONTENT);
  }

  // Caption/body color hints
  if(strcmp(variant, "caption") == 0)
  {
    label.SetTextColor(COLOR_TEXT_MUTED);
  }

  const char* color = GetNodeString(*comp.rawNode, "color", "");
  if(color[0] != '\0')
  {
    label.SetTextColor(ParseHexColor(color));
  }

  // Font weight (bold)
  const char* fontWeight = GetNodeString(*comp.rawNode, "fontWeight", "");
  if(fontWeight[0] != '\0')
  {
    if(strcmp(fontWeight, "bold") == 0)
      label.SetFontWeight(Text::FontWeight::BOLD);
    else if(strcmp(fontWeight, "semiBold") == 0)
      label.SetFontWeight(Text::FontWeight::SEMI_BOLD);
    else if(strcmp(fontWeight, "light") == 0)
      label.SetFontWeight(Text::FontWeight::LIGHT);
    else if(strcmp(fontWeight, "medium") == 0)
      label.SetFontWeight(Text::FontWeight::MEDIUM);
  }
  else if(strcmp(variant, "h1") == 0 || strcmp(variant, "h2") == 0 || strcmp(variant, "h3") == 0 ||
          strcmp(variant, "h4") == 0)
  {
    // Headings default to semi-bold (600)
    label.SetFontWeight(Text::FontWeight::SEMI_BOLD);
  }

  // Font slant (italic)
  const char* fontStyle = GetNodeString(*comp.rawNode, "fontStyle", "");
  if(strcmp(fontStyle, "italic") == 0)
  {
    label.SetFontSlant(Text::FontSlant::ITALIC);
  }

  // Underline
  const TreeNode* underlineNode = comp.rawNode->Find("underline");
  if(underlineNode)
  {
    Text::Underline underline;
    underline.SetColor(label.GetTextColor());
    if(underlineNode->GetType() == TreeNode::OBJECT)
    {
      const char* ulColor = GetNodeString(*underlineNode, "color", "");
      if(ulColor[0] != '\0') underline.SetColor(ParseHexColor(ulColor));
      float thickness = GetNodeFloat(*underlineNode, "thickness", 1.0f);
      underline.SetThickness(thickness);
    }
    label.SetUnderline(underline);
  }

  // Text alignment
  const char* align = GetNodeString(*comp.rawNode, "align", "");
  if(strcmp(align, "center") == 0)
    label.SetHorizontalTextAlignment(Text::Alignment::CENTER);
  else if(strcmp(align, "end") == 0)
    label.SetHorizontalTextAlignment(Text::Alignment::END);

  // Web text elements carry an 8px bottom margin (layout-mb-2) on top of the column gap, which
  // is what gives cards their vertical breathing room; headings a touch more. Captions stay
  // tight (they sit just under the value they annotate).
  bool isHeading = strcmp(variant, "h1") == 0 || strcmp(variant, "h2") == 0 ||
                   strcmp(variant, "h3") == 0 || strcmp(variant, "h4") == 0;
  bool isCaption = strcmp(variant, "caption") == 0;
  float mb = isHeading ? Metrics::Dp(12) : (isCaption ? Metrics::Dp(5) : Metrics::Dp(10));
  label.SetMargin(Extents(0, 0, 0, static_cast<uint16_t>(mb)));

  // Reactive data binding: if text is a data binding, watch for changes
  if(textNode && textNode->GetType() == TreeNode::OBJECT)
  {
    std::string boundPath = GetBoundPath(textNode, ctx);
    if(!boundPath.empty())
    {
      ctx.GetDataModel().Watch(boundPath, [label](const std::string&, const std::string& val) mutable {
        // Strip markdown on update too, matching the initial paint (a streamed value may
        // carry **bold** / "# " markers that would otherwise render literally).
        label.SetText(Dali::String(StripMarkdown(val).c_str()));
      });
    }
  }

  return label;
}
} // namespace A2ui
