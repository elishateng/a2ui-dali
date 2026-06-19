#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderFlexContainer(const ComponentModel& comp,
                                       const SurfaceComponentsModel& components,
                                       DataContext& ctx,
                                       FlexDirection direction)
{
  FlexLayout flex = FlexLayout::New();
  flex.SetDirection(direction);
  flex.SetRequestedHeight(WRAP_CONTENT);

  float selfWeight = comp.rawNode ? GetNodeFloat(*comp.rawNode, "weight", 0.0f) : 0.0f;
  if(selfWeight <= 0.0f)
  {
    flex.SetRequestedWidth(MATCH_PARENT);
  }

  if(comp.rawNode)
  {
    const char* justify = GetNodeString(*comp.rawNode, "justify", "start");
    if(strcmp(justify, "center") == 0)
      flex.SetJustifyContent(FlexJustify::CENTER);
    else if(strcmp(justify, "end") == 0)
      flex.SetJustifyContent(FlexJustify::FLEX_END);
    else if(strcmp(justify, "spaceBetween") == 0)
      flex.SetJustifyContent(FlexJustify::SPACE_BETWEEN);
    else if(strcmp(justify, "spaceAround") == 0)
      flex.SetJustifyContent(FlexJustify::SPACE_AROUND);
    else if(strcmp(justify, "spaceEvenly") == 0)
      flex.SetJustifyContent(FlexJustify::SPACE_EVENLY);

    const char* align = GetNodeString(*comp.rawNode, "align", "stretch");
    if(strcmp(align, "center") == 0)
      flex.SetAlignItems(FlexAlign::CENTER);
    else if(strcmp(align, "end") == 0)
      flex.SetAlignItems(FlexAlign::FLEX_END);
    else if(strcmp(align, "start") == 0)
      flex.SetAlignItems(FlexAlign::FLEX_START);
    else
      flex.SetAlignItems(FlexAlign::STRETCH);

    const char* bg = GetNodeString(*comp.rawNode, "background", "");
    if(bg[0] != '\0')
    {
      flex.SetBackgroundColor(ParseHexColor(bg));
    }

    float padding = GetNodeFloat(*comp.rawNode, "padding", 0.0f);
    if(padding > 0.0f)
    {
      uint16_t p = static_cast<uint16_t>(Metrics::Dp(padding));
      flex.SetPadding(Extents(p, p, p, p));
    }
  }

  bool isRow = (direction == FlexDirection::ROW || direction == FlexDirection::ROW_REVERSE);
  bool isColumnCentered = !isRow && comp.rawNode &&
    (strcmp(GetNodeString(*comp.rawNode, "align", "start"), "center") == 0);

  // Default gap = the web composer's flex gaps (Row g-4 = 16, Column g-2 = 8); a
  // message-declared `gap` (logical) wins and is dp-scaled to match.
  float defaultGap = isRow ? Metrics::RowGap() : Metrics::ColumnGap();
  float gap = (comp.rawNode && comp.rawNode->Find("gap"))
                ? Metrics::Dp(GetNodeFloat(*comp.rawNode, "gap", 0.0f))
                : defaultGap;

  // Data-driven child list (children: {path, componentId}) — instantiate the template per
  // array element. Must run before the static childIds loop, which would render nothing
  // (the parser leaves childIds empty for the OBJECT form). Template runs are list rows, so
  // they pack with the tighter list gap (unless the message set an explicit gap).
  float tmplGap = (comp.rawNode && comp.rawNode->Find("gap")) ? gap : Metrics::ListItemGap();
  if(RenderTemplateChildren(comp, components, ctx, flex, isRow, tmplGap))
  {
    return flex;
  }

  // A justify of spaceBetween/Around/Evenly distributes children itself — in that case a
  // child must NOT flex-grow, or the grown child eats the space the distribution would have
  // given the trailing item (e.g. a line-item price "$6.45" clipped to "$6").
  const char* rowJustify = comp.rawNode ? GetNodeString(*comp.rawNode, "justify", "start") : "start";
  bool distributed = isRow && (strcmp(rowJustify, "spaceBetween") == 0 ||
                               strcmp(rowJustify, "spaceAround") == 0 ||
                               strcmp(rowJustify, "spaceEvenly") == 0);

  // A Row whose children are ALL containers (e.g. Departs/Status/Arrives columns) wants
  // them split evenly. A Row with MIXED children (text + image + one info column) wants
  // the container to fill the *remaining* space. These need different flex seeding, so
  // detect the all-container case up front.
  bool allRowChildrenContainers = isRow && !comp.childIds.empty();
  if(allRowChildrenContainers)
  {
    for(const auto& cid : comp.childIds)
    {
      const ComponentModel* cc = components.GetComponent(cid);
      const std::string t = cc ? cc->type : std::string();
      if(t != "Column" && t != "Row" && t != "Card" && t != "List") { allRowChildrenContainers = false; break; }
    }
  }

  int index = 0;
  for(const auto& childId : comp.childIds)
  {
    const ComponentModel* childComp = components.GetComponent(childId);
    // A default-sized Image placed directly in a Row is a list thumbnail — hint
    // RenderImage to build it small so it doesn't overflow the row. (Shrinking the
    // wrapper after the fact doesn't work: the data-binding container rebuilds the
    // full-size inner image when its url arrives.)
    if(isRow && childComp && childComp->type == "Image" && childComp->rawNode &&
       childComp->rawNode->Find("width") == nullptr &&
       childComp->rawNode->Find("height") == nullptr &&
       GetNodeString(*childComp->rawNode, "variant", "")[0] == '\0')
    {
      mImageThumbnailHint = true;
    }
    View child = RenderComponent(childId, components, ctx);
    mImageThumbnailHint = false;

    if(gap > 0.0f && index > 0)
    {
      uint16_t g = static_cast<uint16_t>(gap);
      if(isRow)
        child.SetMargin(Extents(g, 0, 0, 0));
      else
        child.SetMargin(Extents(0, 0, g, 0));
    }

    // Column align=center: centre Label text, and STRETCH full-width children so they
    // don't collapse to zero width — DALi FlexLayout collapses a MATCH_PARENT child
    // under align-items:center (which silently blanked e.g. a Slider in a centred
    // Column). Narrow children (Button/Image) keep their own AlignSelf:CENTER.
    if(isColumnCentered)
    {
      Label labelChild = Label::DownCast(child);
      if(labelChild)
      {
        labelChild.SetHorizontalTextAlignment(Text::Alignment::CENTER);
      }
      std::string ct = childComp ? childComp->type : std::string();
      bool fullWidth = labelChild || ct == "TextField" || ct == "Slider" ||
                       ct == "ProgressBar" || ct == "List" || ct == "Divider" ||
                       ct == "Video" || ct == "AudioPlayer";
      if(fullWidth)
      {
        child.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::STRETCH));
      }
    }

    if(childComp && childComp->rawNode)
    {
      float weight = GetNodeFloat(*childComp->rawNode, "weight", 0.0f);
      if(weight > 0.0f)
      {
        child.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(weight).SetFlexBasis(0.0f));
        child.SetRequestedWidth(0.0f);
      }
      else if(isRow)
      {
        const std::string& childType = childComp->type;
        // Full-width controls (inputs/sliders) behave like containers in a Row: they should
        // flex-grow to fill, not collapse to content width (a Row's "Due" DateTimeInput
        // otherwise vanishes). CheckBox stays a non-container so it keeps its small size.
        bool isContainer = (childType == "Column" || childType == "Row" || childType == "Card" ||
                            childType == "List" || childType == "DateTimeInput" ||
                            childType == "TextField" || childType == "Slider" ||
                            childType == "ChoicePicker");
        if(isContainer)
        {
          if(distributed)
          {
            // justify spaceBetween/Around/Evenly: grow the container from a 0 basis (and a 0
            // requested width, or a MATCH_PARENT child would fill the whole row and hide its
            // siblings — e.g. flight Departs/Status/Arrives showing only Departs). This keeps
            // every column visible AND leaves room for a pinned trailing item (price/value).
            child.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexShrink(1.0f).SetFlexBasis(0.0f));
            child.SetRequestedWidth(0.0f);
          }
          else if(allRowChildrenContainers)
          {
            // All siblings are containers → split the row evenly. basis 0 + a 0 requested
            // width seed equal grow distribution (e.g. flightStatus Departs/Status/Arrives).
            child.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexShrink(1.0f).SetFlexBasis(0.0f));
            child.SetRequestedWidth(0.0f);
          }
          else
          {
            // Mixed row (text/image + an info column) → the container fills the remaining
            // space. Do NOT pin RequestedWidth to 0 here: that collapses the column's
            // MATCH_PARENT text to nothing (a track row's title/artist rendered blank).
            child.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexShrink(1.0f));
          }
        }
        else
        {
          // Text/other in a Row: use natural width, allow shrink.
          bool pinned = false;
          Label labelChild = Label::DownCast(child);
          if(labelChild)
          {
            labelChild.SetRequestedWidth(WRAP_CONTENT);
            // A short trailing Label (line-item price, value, duration) sitting AFTER a
            // flex-grow sibling gets its space consumed and clips (e.g. "$6.45" → "$6").
            // Reserve a width from the glyph count and stop it shrinking so it stays intact.
            Dali::String t = labelChild.GetText();
            if(t.Size() > 0 && t.Size() <= 16)
            {
              float fs = (childComp && childComp->rawNode)
                ? VariantToFontSize(GetNodeString(*childComp->rawNode, "variant", "body")) : Metrics::FontBody();
              labelChild.SetRequestedWidth(static_cast<float>(t.Size()) * fs * 0.6f + fs);
              labelChild.SetLayoutParams(FlexLayoutParams::New().SetFlexShrink(0.0f));
              pinned = true;
            }
          }
          else if(childType == "Image")
          {
            // A responsive image is MATCH_PARENT (fill container up to its declared
            // max-width). On a Row's MAIN axis the layout engine forces MATCH_PARENT
            // children to the full width, squeezing siblings — so pin the image to its
            // declared width (stashed as MaximumWidth). Default-sized row images were
            // already built thumbnail-small by the hint above, so this pins that too.
            float declared = child.GetMaximumWidth();
            if(declared > 0.0f && declared < 100000.0f)
            {
              child.SetRequestedWidth(declared);
            }
          }
          else if(child.GetRequestedWidth() == MATCH_PARENT)
          {
            // A non-container that declares full width (e.g. a CheckBox's row) would grab the
            // whole Row on the main axis and push its siblings off — size it to content.
            child.SetRequestedWidth(WRAP_CONTENT);
          }
          if(!pinned)
          {
            child.SetLayoutParams(FlexLayoutParams::New().SetFlexShrink(1.0f));
          }
        }
      }
    }

    flex.Add(child);
    index++;
  }

  return flex;
}
} // namespace A2ui
