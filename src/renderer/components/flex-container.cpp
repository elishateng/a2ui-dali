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
      uint16_t p = static_cast<uint16_t>(padding);
      flex.SetPadding(Extents(p, p, p, p));
    }
  }

  bool isRow = (direction == FlexDirection::ROW || direction == FlexDirection::ROW_REVERSE);
  bool isColumnCentered = !isRow && comp.rawNode &&
    (strcmp(GetNodeString(*comp.rawNode, "align", "start"), "center") == 0);

  float defaultGap = isRow ? 6.0f : 8.0f;
  float gap = comp.rawNode ? GetNodeFloat(*comp.rawNode, "gap", defaultGap) : defaultGap;

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
        bool isContainer = (childType == "Column" || childType == "Row" || childType == "Card" ||
                            childType == "List");
        if(isContainer)
        {
          if(allRowChildrenContainers)
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
          Label labelChild = Label::DownCast(child);
          if(labelChild)
          {
            labelChild.SetRequestedWidth(WRAP_CONTENT);
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
          // NOTE: a small trailing value (track duration, line-item price) that sits AFTER
          // a flex-grow container can still be clipped — DALi FlexLayout gives the grow
          // sibling all remaining space without reserving the trailing item's width, and
          // RequestedWidth/MinimumWidth/FlexShrink on it don't override that. Left as a
          // known minor limitation; the primary content renders.
          child.SetLayoutParams(FlexLayoutParams::New().SetFlexShrink(1.0f));
        }
      }
    }

    flex.Add(child);
    index++;
  }

  return flex;
}
} // namespace A2ui
