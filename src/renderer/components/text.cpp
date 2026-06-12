#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderText(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  const TreeNode* textNode = comp.rawNode->Find("text");
  std::string text = textNode ? ResolveString(textNode, ctx) : "";

  const char* variant = GetNodeString(*comp.rawNode, "variant", "body");
  float fontSize = VariantToFontSize(variant);

  Label label = Label::New(text.c_str());
  label.SetFontSize(fontSize);
  label.SetRequestedWidth(MATCH_PARENT);
  label.SetTextColor(COLOR_TEXT_DEFAULT);
  label.SetMultiLine(true);
  label.SetLineWrapMode(Text::LineWrapMode::WORD);

  // DALi Label inside a flex-grown parent sometimes measures height=0 when
  // the parent's resolved width is only known after the layout pass. Provide
  // a rough fallback so text is never invisible. Heuristic: assume ~40 chars
  // per line; upper-bound at 5 lines to avoid runaway heights on long text.
  if(!text.empty())
  {
    int charsPerLine = 40;
    int estLines = static_cast<int>((text.size() + charsPerLine - 1) / charsPerLine);
    if(estLines < 1) estLines = 1;
    if(estLines > 5) estLines = 5;
    label.SetRequestedHeight(fontSize * 1.5f * estLines);
  }
  else
  {
    // Data-bound text is usually empty at first paint (its value arrives in a later
    // updateDataModel); reserve one line so the flex keeps the slot and the watch's
    // update becomes visible instead of the row collapsing the value away.
    bool dataBound = textNode && textNode->GetType() == TreeNode::OBJECT;
    label.SetRequestedHeight(dataBound ? fontSize * 1.5f : WRAP_CONTENT);
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

  if(strcmp(variant, "h1") == 0 || strcmp(variant, "h2") == 0)
  {
    label.SetMargin(Extents(0, 0, 0, 4));
  }

  // Reactive data binding: if text is a data binding, watch for changes
  if(textNode && textNode->GetType() == TreeNode::OBJECT)
  {
    std::string boundPath = GetBoundPath(textNode, ctx);
    if(!boundPath.empty())
    {
      ctx.GetDataModel().Watch(boundPath, [label](const std::string&, const std::string& val) mutable {
        label.SetText(Dali::String(val.c_str()));
      });
    }
  }

  return label;
}
} // namespace A2ui
