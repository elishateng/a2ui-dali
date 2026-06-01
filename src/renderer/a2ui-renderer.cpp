/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "a2ui-renderer.h"
#include "a11y-bridge.h"
#include <dali/integration-api/debug.h>
#include <dali/public-api/events/touch-event.h>
#include <dali-ui-foundation/public-api/image-view.h>
#include <dali-ui-foundation/public-api/layouts/flex-layout-params.h>
#include <dali-ui-foundation/public-api/scroll-view.h>
#include <dali-ui-foundation/public-api/text/text-enumerations.h>
#include <dali-ui-foundation/public-api/text/style/underline.h>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>

using namespace Dali;
using namespace Dali::Ui;
using Dali::Ui::TreeNode;

namespace A2ui
{

namespace
{
const UiColor COLOR_TEXT_DEFAULT(0x1a1a1a);
const UiColor COLOR_TEXT_MUTED(0x6b6b6b);
const UiColor COLOR_CARD_BG(0xFFFFFF);
const UiColor COLOR_BTN_PRIMARY(0x1a1a1a);
const UiColor COLOR_BTN_SECONDARY(0xFFFFFF);
const UiColor COLOR_BTN_BORDER(0xd9d9d9);
const UiColor COLOR_DIVIDER(0xe5e5e5);
const UiColor COLOR_IMG_PLACEHOLDER(0xf0f0f0);
const UiColor COLOR_PLACEHOLDER_BG(0xfff0f0);
const UiColor COLOR_PLACEHOLDER_TEXT(0xFF4444);
const UiColor COLOR_ERROR(0xFF4444);
const UiColor COLOR_INPUT_BG(0xFFFFFF);
const UiColor COLOR_INPUT_BORDER(0xd9d9d9);
const UiColor COLOR_CHECK_ON(0x1a1a1a);
const UiColor COLOR_CHECK_OFF(0xd9d9d9);
const UiColor COLOR_TAB_ACTIVE(0x1a1a1a);
const UiColor COLOR_TAB_INACTIVE(0xf5f5f5);
const UiColor COLOR_MODAL_OVERLAY(0x00000066);
const UiColor COLOR_SLIDER_TRACK(0xe5e5e5);
const UiColor COLOR_SLIDER_FILL(0x1a1a1a);
const UiColor COLOR_SLIDER_THUMB(0x1a1a1a);

// Recursively walk a component subtree looking for the first node that
// defines an `action`. Used by RenderCard as a fallback so that the whole
// card layout can receive taps — dali-ui has no native Button, so the
// Button's own FlexLayout occasionally fails to surface Clicked events
// (e.g. when nested inside a ScrollView / List template). Having the
// surrounding Card act as a secondary hit target keeps the "Book Now"
// affordance functional from anywhere on the item.
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

} // anonymous namespace

A2uiRenderer::A2uiRenderer()
{
}

// ========================================================================
// Public API
// ========================================================================

View A2uiRenderer::Render(SurfaceModel& surface)
{
  DALI_LOG_ERROR("[A2UI] === Render() BEGIN surface=%s components=%zu ===\n",
                 surface.GetSurfaceId().c_str(),
                 surface.GetComponentsModel().GetCount());

  const ComponentModel* root = surface.GetComponentsModel().GetRoot();
  if(!root)
  {
    DALI_LOG_ERROR("[A2UI] Renderer: No 'root' component found\n");
    return View();
  }

  // Clear previous observers (from previous render)
  surface.GetDataModel().ClearObservers();

  // Release tap detectors from the previous render. The new view tree will
  // attach fresh detectors for its own Button / Card tap targets.
  mTapDetectors.clear();

  // Configure action dispatcher
  mActionDispatcher.SetSurfaceId(surface.GetSurfaceId());
  mActionDispatcher.SetSendDataModel(surface.GetSendDataModel());

  DataContext ctx(surface.GetDataModel());
  mCurrentComponents = &surface.GetComponentsModel();
  mCurrentCtx = &ctx;

  View result;
  try
  {
    result = RenderComponent("root", surface.GetComponentsModel(), ctx);
  }
  catch(const std::exception& e)
  {
    DALI_LOG_ERROR("[A2UI] *** Render() EXCEPTION: %s (surface=%s) ***\n",
                   e.what(), surface.GetSurfaceId().c_str());
    mCurrentComponents = nullptr;
    mCurrentCtx = nullptr;
    throw;
  }

  mCurrentComponents = nullptr;
  mCurrentCtx = nullptr;
  DALI_LOG_ERROR("[A2UI] === Render() END surface=%s OK ===\n",
                 surface.GetSurfaceId().c_str());
  return result;
}

// ========================================================================
// Component dispatch
// ========================================================================

View A2uiRenderer::RenderComponent(const std::string& id,
                                   const SurfaceComponentsModel& components,
                                   DataContext& ctx)
{
  const ComponentModel* comp = components.GetComponent(id);
  if(!comp)
  {
    DALI_LOG_ERROR("[A2UI] Renderer: Component '%s' not found\n", id.c_str());
    return View::New()
      .SetBackgroundColor(UiColor(0xFF0000))
      .SetRequestedWidth(MATCH_PARENT)
      .SetRequestedHeight(20.0f);
  }

  const std::string& type = comp->type;
  DALI_LOG_ERROR("[A2UI] RenderComponent: id=%s type=%s\n",
                 comp->id.c_str(), type.c_str());
  View result;

  // Phase 1 components
  if(type == "Text")         result = RenderText(*comp, ctx);
  else if(type == "Row")     result = RenderFlexContainer(*comp, components, ctx, FlexDirection::ROW);
  else if(type == "Column")  result = RenderFlexContainer(*comp, components, ctx, FlexDirection::COLUMN);
  else if(type == "Card")    result = RenderCard(*comp, components, ctx);
  else if(type == "Button")  result = RenderButton(*comp, components, ctx);
  else if(type == "Image")   result = RenderImage(*comp, ctx);
  else if(type == "Divider") result = RenderDivider(*comp);

  // Phase 2 input components
  else if(type == "TextField")     result = RenderTextField(*comp, ctx);
  else if(type == "CheckBox")      result = RenderCheckBox(*comp, ctx);
  else if(type == "ChoicePicker")  result = RenderChoicePicker(*comp, ctx);
  else if(type == "Slider")        result = RenderSlider(*comp, ctx);
  else if(type == "DateTimeInput") result = RenderDateTimeInput(*comp, ctx);

  // Phase 2 layout components
  else if(type == "Tabs")  result = RenderTabs(*comp, components, ctx);
  else if(type == "Modal") result = RenderModal(*comp, components, ctx);
  else if(type == "List")  result = RenderList(*comp, components, ctx);
  else if(type == "Icon")  result = RenderIcon(*comp, ctx);

  else
  {
    DALI_LOG_ERROR("[A2UI] Renderer: Unknown component type '%s' (id: %s)\n",
                   type.c_str(), id.c_str());
    result = RenderPlaceholder(*comp);
  }

  // Phase 4: Apply accessibility properties
  if(result)
  {
    A11yBridge::Apply(result, *comp, ctx);
    mDiffEngine.RegisterView(id, result);
  }

  return result;
}

// ========================================================================
// Text
// ========================================================================

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
    label.SetRequestedHeight(WRAP_CONTENT);
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

// ========================================================================
// Row / Column
// ========================================================================

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

  int index = 0;
  for(const auto& childId : comp.childIds)
  {
    View child = RenderComponent(childId, components, ctx);

    if(gap > 0.0f && index > 0)
    {
      uint16_t g = static_cast<uint16_t>(gap);
      if(isRow)
        child.SetMargin(Extents(g, 0, 0, 0));
      else
        child.SetMargin(Extents(0, 0, g, 0));
    }

    // Column align=center: center text alignment for Label children
    if(isColumnCentered)
    {
      Label labelChild = Label::DownCast(child);
      if(labelChild)
      {
        labelChild.SetHorizontalTextAlignment(Text::Alignment::CENTER);
      }
    }

    const ComponentModel* childComp = components.GetComponent(childId);
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
          // Containers in a Row: fill remaining space
          child.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexShrink(1.0f).SetFlexBasis(0.0f));
          child.SetRequestedWidth(0.0f);
        }
        else
        {
          // Text/other in a Row: use natural width, allow shrink
          Label labelChild = Label::DownCast(child);
          if(labelChild)
          {
            labelChild.SetRequestedWidth(WRAP_CONTENT);
          }
          else if(childType == "Image")
          {
            // A responsive image is MATCH_PARENT (fill container up to its
            // declared max-width). On a Row's MAIN axis the layout engine forces
            // MATCH_PARENT children to the full width, squeezing siblings — so
            // pin the image to its declared width (stashed as MaximumWidth);
            // flex-shrink still trims it if the row is too narrow.
            float declared = child.GetMaximumWidth();
            if(declared > 0.0f && declared < 100000.0f)
            {
              child.SetRequestedWidth(declared);
            }
          }
          child.SetLayoutParams(FlexLayoutParams::New().SetFlexShrink(1.0f));
        }
      }
    }

    flex.Add(child);
    index++;
  }

  return flex;
}

// ========================================================================
// Card
// ========================================================================

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
  card.SetCornerRadius(12.0f);
  card.SetBorderlineWidth(1.5f);
  card.SetBorderlineColor(UiColor(0xd1d5db));  // gray-300, pronounced edge

  float padding = comp.rawNode ? GetNodeFloat(*comp.rawNode, "padding", 16.0f) : 16.0f;
  uint16_t p = static_cast<uint16_t>(padding);
  card.SetPadding(Extents(p, p, p, p));
  card.SetMargin(Extents(0, 0, 10, 10));

  if(!comp.childId.empty())
  {
    View childView = RenderComponent(comp.childId, components, ctx);
    card.Add(childView);
  }

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
        DALI_LOG_ERROR("[A2UI] Card TAPPED (fallback) source=%s\n",
                       sourceId.c_str());
        mActionDispatcher.Dispatch(*actionNode, sourceId, capturedCtx);
      });
    mTapDetectors.push_back(detector);
    DALI_LOG_ERROR("[A2UI] Card fallback tap-detector attached source=%s\n",
                   sourceId.c_str());
  }

  return card;
}

// ========================================================================
// Button
// ========================================================================

View A2uiRenderer::RenderButton(const ComponentModel& comp,
                                const SurfaceComponentsModel& components,
                                DataContext& ctx)
{
  const char* variant = comp.rawNode ? GetNodeString(*comp.rawNode, "variant", "secondary") : "secondary";
  bool isPrimary = (strcmp(variant, "primary") == 0);
  bool isBorderless = (strcmp(variant, "borderless") == 0);

  // Single FlexLayout — the previous nested (borderWrap + button) structure
  // collapsed to zero under DALi's column-align:stretch when the child label
  // was WRAP_CONTENT. Fixed size + a single container avoids that entirely.
  constexpr float kBtnHeight = 40.0f;
  constexpr float kBtnMinWidth = 140.0f;
  constexpr float kCharWidth14pt = 9.0f;   // approximate glyph advance at 14pt MEDIUM
  constexpr float kLabelPadding = 8.0f;    // cushion on each side inside the button

  FlexLayout button = FlexLayout::New();
  button.SetDirection(FlexDirection::ROW);
  button.SetJustifyContent(FlexJustify::CENTER);
  button.SetAlignItems(FlexAlign::CENTER);
  button.SetRequestedHeight(kBtnHeight);
  button.SetCornerRadius(10.0f);
  button.SetPadding(Extents(16, 16, 8, 8));
  button.SetMargin(Extents(0, 0, 8, 4));
  // Don't let the parent Column's align:stretch override our fixed width.
  button.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));

  UiColor bgColor(0xf3f4f6);  // gray-100 default (secondary)
  UiColor fgColor = COLOR_TEXT_DEFAULT;
  if(isPrimary)
  {
    bgColor = COLOR_TEXT_DEFAULT;  // gray-900 fill
    fgColor = UiColor(0xFFFFFF);
  }
  else if(isBorderless)
  {
    bgColor = UiColor(0x00000000);
  }
  button.SetBackgroundColor(bgColor);

  float btnWidth = kBtnMinWidth;
  if(!comp.childId.empty())
  {
    View childView = RenderComponent(comp.childId, components, ctx);
    Label labelChild = Label::DownCast(childView);
    if(labelChild)
    {
      labelChild.SetTextColor(fgColor);
      labelChild.SetFontWeight(Text::FontWeight::MEDIUM);
      labelChild.SetFontSize(14.0f);
      labelChild.SetHorizontalTextAlignment(Text::Alignment::CENTER);
      // Grow the label box to fit the full glyph run so text like "Submit
      // Reservation" isn't ellipsized. Button width tracks the label so the
      // padding is uniform on both sides regardless of text length.
      Dali::String text = labelChild.GetText();
      std::size_t charCount = text.Size();
      float textWidth = static_cast<float>(charCount) * kCharWidth14pt + kLabelPadding * 2.0f;
      float labelWidth = std::max(100.0f, textWidth);
      labelChild.SetMultiLine(false);
      labelChild.SetRequestedWidth(labelWidth);
      labelChild.SetRequestedHeight(20.0f);
      btnWidth = std::max(kBtnMinWidth, labelWidth + 32.0f);  // 16px padding each side
    }
    button.Add(childView);
  }
  button.SetRequestedWidth(btnWidth);

  // Action handling
  //
  // Note: dali-ui has no native Button — this FlexLayout *is* the button.
  // We attach an independent TapGestureDetector instead of relying on
  // InteractiveTrait or TouchedSignal, because neither fires reliably for
  // Button layouts nested inside a ScrollView/List ancestor (the parent's
  // pan detector ends up consuming the touch sequence before our view's
  // click signal can be synthesized). The detector handle must outlive the
  // view, so we park it in mTapDetectors.
  if(comp.rawNode)
  {
    const TreeNode* actionNode = comp.rawNode->Find("action");
    if(actionNode)
    {
      std::string compId = comp.id;
      DataContext capturedCtx = ctx;
      Dali::TapGestureDetector detector = Dali::TapGestureDetector::New();
      detector.Attach(button);
      detector.DetectedSignal().Connect(this,
        [this, actionNode, compId, capturedCtx](
          Dali::Actor, const Dali::TapGesture&) mutable {
          DALI_LOG_ERROR("[A2UI] Button TAPPED: %s\n", compId.c_str());
          mActionDispatcher.Dispatch(*actionNode, compId, capturedCtx);
        });
      mTapDetectors.push_back(detector);
      DALI_LOG_ERROR("[A2UI] Button tap-detector attached: %s\n", compId.c_str());
    }
  }

  return button;
}

// ========================================================================
// Image
// ========================================================================

View A2uiRenderer::RenderImage(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  const TreeNode* urlNode = comp.rawNode->Find("url");
  std::string url = urlNode ? ResolveString(urlNode, ctx) : "";

  const char* variant = GetNodeString(*comp.rawNode, "variant", "");
  // Default to a 1:1 square so agent-generated images (confirmation, booking)
  // don't get stretched into a wide-short strip by the parent Column's default
  // alignItems: STRETCH.
  float w = 280.0f, h = 280.0f;
  bool  isHeader = false;

  if(strcmp(variant, "icon") == 0)              { w = 24.0f;  h = 24.0f; }
  else if(strcmp(variant, "avatar") == 0)        { w = 56.0f;  h = 56.0f; }
  else if(strcmp(variant, "smallFeature") == 0)  { w = 80.0f;  h = 80.0f; }
  else if(strcmp(variant, "mediumFeature") == 0) { w = 160.0f; h = 120.0f; }
  else if(strcmp(variant, "largeFeature") == 0)  { w = 320.0f; h = 200.0f; }
  else if(strcmp(variant, "square") == 0)        { w = 280.0f; h = 280.0f; }
  else if(strcmp(variant, "header") == 0)        { w = MATCH_PARENT; h = 200.0f; isHeader = true; }

  // Explicit width/height on the A2UI node take precedence over the variant
  // default (the catalog's posters declare e.g. width:150, height:225).
  if(!isHeader)
  {
    w = GetNodeFloat(*comp.rawNode, "width", w);
    h = GetNodeFloat(*comp.rawNode, "height", h);
  }

  bool isRemoteUrl = (!url.empty() && url.find("://") != std::string::npos);
  std::string fullPath = url;
  if(!url.empty() && !isRemoteUrl && url[0] != '/')
  {
    fullPath = mImageDir + url;
  }

  std::string resolved;
  if(isRemoteUrl)
  {
    resolved = url;
  }
  else if(!fullPath.empty())
  {
    std::ifstream testFile(fullPath);
    if(testFile.is_open()) resolved = fullPath;
  }

  // A2UI fit → DALi fitting mode. "cover" (default) fills the box keeping aspect
  // (crops overflow); "contain" fits inside keeping aspect; "fill" stretches.
  const char* fitSpec = GetNodeString(*comp.rawNode, "fit",
                          GetNodeString(*comp.rawNode, "fittingMode", "cover"));
  auto fitMode = Ui::Image::FittingMode::OVER_FIT_KEEP_ASPECT_RATIO; // cover
  if(strcmp(fitSpec, "contain") == 0)   fitMode = Ui::Image::FittingMode::FIT_KEEP_ASPECT_RATIO;
  else if(strcmp(fitSpec, "fill") == 0) fitMode = Ui::Image::FittingMode::FILL;

  if(resolved.empty())
  {
    View placeholder = View::New();
    placeholder.SetRequestedWidth(isHeader ? MATCH_PARENT : w);
    placeholder.SetRequestedHeight(h);
    placeholder.SetCornerRadius(12.0f);
    placeholder.SetBackgroundColor(COLOR_IMG_PLACEHOLDER);
    if(!isHeader)
    {
      placeholder.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
    }
    return placeholder;
  }

  // Web-responsive image — mirrors CSS `img { max-width:100%; height:<declared> }`.
  // Width fills the container but never exceeds the declared width; the height
  // stays at the declared value and fit:cover crops to fill. So when a horizontal
  // flex List shrinks the cards, the poster keeps its vertical length (the image
  // just crops more) instead of shrinking via aspect ratio.
  ImageView img = ImageView::New(resolved.c_str());
  img.SetFittingMode(fitMode);
  img.SetCornerRadius(12.0f);
  img.SetDesiredWidth(static_cast<int>(w > 0 ? w : 0));
  img.SetDesiredHeight(static_cast<int>(h));
  if(isHeader)
  {
    img.SetRequestedWidth(MATCH_PARENT);
    img.SetRequestedHeight(h);
  }
  else
  {
    img.SetRequestedWidth(MATCH_PARENT);
    img.SetMaximumWidth(w);
    img.SetRequestedHeight(h);
    img.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
  }
  return img;
}

// ========================================================================
// Divider
// ========================================================================

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

// ========================================================================
// Placeholder
// ========================================================================

View A2uiRenderer::RenderPlaceholder(const ComponentModel& comp)
{
  FlexLayout placeholder = FlexLayout::New();
  placeholder.SetDirection(FlexDirection::ROW);
  placeholder.SetJustifyContent(FlexJustify::CENTER);
  placeholder.SetAlignItems(FlexAlign::CENTER);
  placeholder.SetRequestedWidth(MATCH_PARENT);
  placeholder.SetRequestedHeight(40.0f);
  placeholder.SetBackgroundColor(COLOR_PLACEHOLDER_BG);
  placeholder.SetCornerRadius(4.0f);
  placeholder.SetMargin(Extents(0, 0, 2, 2));

  std::string text = "[" + comp.type + ": " + comp.id + "]";
  Label label = Label::New(text.c_str());
  label.SetFontSize(12.0f);
  label.SetTextColor(COLOR_PLACEHOLDER_TEXT);
  label.SetRequestedWidth(WRAP_CONTENT);
  label.SetRequestedHeight(WRAP_CONTENT);
  placeholder.Add(label);

  return placeholder;
}

// ========================================================================
// TextField (Phase 2)
// ========================================================================

View A2uiRenderer::RenderTextField(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetMargin(Extents(0, 0, 4, 4));

  // Label
  const char* labelText = GetNodeString(*comp.rawNode, "label", "");
  if(labelText[0] != '\0')
  {
    Label label = Label::New(labelText);
    label.SetFontSize(12.0f);
    label.SetTextColor(UiColor(0xAAAACC));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetMargin(Extents(0, 0, 0, 4));
    container.Add(label);
  }

  // Bound path for two-way binding
  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";

  const char* placeholder = GetNodeString(*comp.rawNode, "placeholder", "");
  std::string currentValue;
  if(!boundPath.empty())
  {
    currentValue = ctx.GetDataModel().GetString(boundPath);
  }

  // Real InputField for text input
  InputField inputField = InputField::New();
  inputField.SetPlaceholder(placeholder);
  inputField.SetPlaceholderColor(UiColor(0x666688));
  inputField.SetFontSize(16.0f);
  inputField.SetTextColor(COLOR_TEXT_DEFAULT);
  inputField.SetCursorColor(COLOR_TEXT_DEFAULT);
  inputField.SetCursorWidth(2);
  inputField.SetSelectionColor(UiColor(0x4a90d9));
  inputField.SetRequestedWidth(MATCH_PARENT);
  inputField.SetRequestedHeight(WRAP_CONTENT);
  inputField.SetPadding(Extents(12, 12, 10, 10));
  inputField.SetBackgroundColor(COLOR_INPUT_BG);
  inputField.SetCornerRadius(6.0f);

  if(!currentValue.empty())
  {
    inputField.SetText(Dali::String(currentValue.c_str()));
  }

  // Two-way binding: InputField → DataModel
  if(!boundPath.empty())
  {
    inputField.TextChangedSignal().Connect(this,
      [boundPath, ctx](View changedView) mutable {
        InputField field = InputField::DownCast(changedView);
        if(field)
        {
          std::string val = field.GetText().CStr();
          ctx.GetDataModel().SetValue(boundPath, val);
        }
      });

    // DataModel → InputField (for server-side updates)
    ctx.GetDataModel().Watch(boundPath,
      [inputField](const std::string&, const std::string& val) mutable {
        std::string current = inputField.GetText().CStr();
        if(current != val)
        {
          inputField.SetText(Dali::String(val.c_str()));
        }
      });
  }

  container.Add(inputField);

  // Error label (initially hidden)
  Label errorLabel = Label::New("");
  errorLabel.SetFontSize(11.0f);
  errorLabel.SetTextColor(COLOR_ERROR);
  errorLabel.SetRequestedWidth(MATCH_PARENT);
  errorLabel.SetRequestedHeight(WRAP_CONTENT);
  errorLabel.SetProperty(Actor::Property::VISIBLE, false);
  errorLabel.SetMargin(Extents(0, 0, 2, 0));
  container.Add(errorLabel);

  // Setup checks validation
  if(!boundPath.empty())
  {
    SetupChecks(comp, ctx, errorLabel, boundPath);
  }

  return container;
}

// ========================================================================
// CheckBox (Phase 2)
// ========================================================================

View A2uiRenderer::RenderCheckBox(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout row = FlexLayout::New();
  row.SetDirection(FlexDirection::ROW);
  row.SetAlignItems(FlexAlign::CENTER);
  row.SetRequestedWidth(MATCH_PARENT);
  row.SetRequestedHeight(WRAP_CONTENT);
  row.SetMargin(Extents(0, 0, 4, 4));

  // Check icon
  View checkIcon = View::New();
  checkIcon.SetRequestedWidth(24.0f);
  checkIcon.SetRequestedHeight(24.0f);
  checkIcon.SetCornerRadius(4.0f);

  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";

  bool checked = !boundPath.empty() ? ctx.GetDataModel().GetBool(boundPath, false) : false;
  checkIcon.SetBackgroundColor(checked ? COLOR_CHECK_ON : COLOR_CHECK_OFF);

  // Label
  const char* labelText = GetNodeString(*comp.rawNode, "label", "");
  Label label = Label::New(labelText);
  label.SetFontSize(15.0f);
  label.SetTextColor(COLOR_TEXT_DEFAULT);
  label.SetRequestedWidth(WRAP_CONTENT);
  label.SetRequestedHeight(WRAP_CONTENT);
  label.SetMargin(Extents(8, 0, 0, 0));

  row.Add(checkIcon);
  row.Add(label);

  // Click toggle
  if(!boundPath.empty())
  {
    row.AsInteractive([this, boundPath, ctx](InteractiveTrait& trait) mutable {
      trait.ClickedSignal().Connect(this,
        [boundPath, ctx](View, const InputEvent&) mutable -> bool {
          bool current = ctx.GetDataModel().GetBool(boundPath, false);
          ctx.GetDataModel().SetBoolValue(boundPath, !current);
          return true;
        });
    });

    // Watch for data changes → update icon
    ctx.GetDataModel().Watch(boundPath,
      [checkIcon](const std::string&, const std::string& val) mutable {
        bool on = (val == "true");
        checkIcon.SetBackgroundColor(on ? COLOR_CHECK_ON : COLOR_CHECK_OFF);
      });
  }

  return row;
}

// ========================================================================
// ChoicePicker (Phase 2)
// ========================================================================

View A2uiRenderer::RenderChoicePicker(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetMargin(Extents(0, 0, 4, 4));

  // Label (optional)
  const char* labelText = GetNodeString(*comp.rawNode, "label", "");
  if(labelText[0] != '\0')
  {
    Label label = Label::New(labelText);
    label.SetFontSize(12.0f);
    label.SetTextColor(UiColor(0xAAAACC));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    label.SetMargin(Extents(0, 0, 0, 4));
    container.Add(label);
  }

  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";
  std::string currentValue = !boundPath.empty() ? ctx.GetDataModel().GetString(boundPath) : "";

  const TreeNode* optionsNode = comp.rawNode->Find("options");
  if(!optionsNode || optionsNode->GetType() != TreeNode::ARRAY)
  {
    return container;
  }

  // Collect option views for reactive updates
  std::vector<std::pair<View, std::string>> optionIcons;

  for(auto it = optionsNode->CBegin(); it != optionsNode->CEnd(); ++it)
  {
    const TreeNode& optNode = (*it).second;
    const TreeNode* optLabelNode = optNode.Find("label");
    const TreeNode* optValueNode = optNode.Find("value");

    std::string optLabel = (optLabelNode && optLabelNode->GetType() == TreeNode::STRING)
                           ? optLabelNode->GetString() : "";
    std::string optValue = (optValueNode && optValueNode->GetType() == TreeNode::STRING)
                           ? optValueNode->GetString() : "";

    FlexLayout optRow = FlexLayout::New();
    optRow.SetDirection(FlexDirection::ROW);
    optRow.SetAlignItems(FlexAlign::CENTER);
    optRow.SetRequestedWidth(MATCH_PARENT);
    optRow.SetRequestedHeight(WRAP_CONTENT);
    optRow.SetPadding(Extents(8, 8, 10, 10));

    // Radio circle
    View radioIcon = View::New();
    radioIcon.SetRequestedWidth(20.0f);
    radioIcon.SetRequestedHeight(20.0f);
    radioIcon.SetCornerRadius(10.0f);
    bool selected = (optValue == currentValue);
    radioIcon.SetBackgroundColor(selected ? COLOR_CHECK_ON : COLOR_CHECK_OFF);
    optRow.Add(radioIcon);

    optionIcons.push_back({radioIcon, optValue});

    // Option label
    Label optLbl = Label::New(optLabel.c_str());
    optLbl.SetFontSize(15.0f);
    optLbl.SetTextColor(COLOR_TEXT_DEFAULT);
    optLbl.SetRequestedWidth(WRAP_CONTENT);
    optLbl.SetRequestedHeight(WRAP_CONTENT);
    optLbl.SetMargin(Extents(8, 0, 0, 0));
    optRow.Add(optLbl);

    // Click handler
    if(!boundPath.empty())
    {
      optRow.AsInteractive([this, optValue, boundPath, ctx](InteractiveTrait& trait) mutable {
        trait.ClickedSignal().Connect(this,
          [optValue, boundPath, ctx](View, const InputEvent&) mutable -> bool {
            ctx.GetDataModel().SetValue(boundPath, optValue);
            return true;
          });
      });
    }

    container.Add(optRow);
  }

  // Watch for value changes → update all radio icons
  if(!boundPath.empty())
  {
    ctx.GetDataModel().Watch(boundPath,
      [optionIcons](const std::string&, const std::string& val) mutable {
        for(auto& pair : optionIcons)
        {
          bool selected = (pair.second == val);
          pair.first.SetBackgroundColor(selected ? COLOR_CHECK_ON : COLOR_CHECK_OFF);
        }
      });
  }

  return container;
}

// ========================================================================
// Slider (Phase 2)
// ========================================================================

View A2uiRenderer::RenderSlider(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetMargin(Extents(0, 0, 4, 4));

  // Label (optional)
  const char* labelText = GetNodeString(*comp.rawNode, "label", "");
  if(labelText[0] != '\0')
  {
    Label label = Label::New(labelText);
    label.SetFontSize(12.0f);
    label.SetTextColor(UiColor(0xAAAACC));
    label.SetRequestedWidth(MATCH_PARENT);
    label.SetRequestedHeight(WRAP_CONTENT);
    container.Add(label);
  }

  float minVal = GetNodeFloat(*comp.rawNode, "min", 0.0f);
  float maxVal = GetNodeFloat(*comp.rawNode, "max", 100.0f);

  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";

  float currentVal = !boundPath.empty()
    ? ctx.GetDataModel().GetFloat(boundPath, minVal)
    : minVal;

  // Value display
  std::ostringstream valStr;
  valStr << static_cast<int>(currentVal);
  Label valueLabel = Label::New(valStr.str().c_str());
  valueLabel.SetFontSize(14.0f);
  valueLabel.SetTextColor(COLOR_TEXT_DEFAULT);
  valueLabel.SetRequestedWidth(WRAP_CONTENT);
  valueLabel.SetRequestedHeight(WRAP_CONTENT);
  valueLabel.SetMargin(Extents(0, 0, 2, 4));

  // Track container (visual slider representation)
  FlexLayout trackRow = FlexLayout::New();
  trackRow.SetDirection(FlexDirection::ROW);
  trackRow.SetAlignItems(FlexAlign::CENTER);
  trackRow.SetRequestedWidth(MATCH_PARENT);
  trackRow.SetRequestedHeight(24.0f);

  // Filled portion
  float ratio = (maxVal > minVal) ? (currentVal - minVal) / (maxVal - minVal) : 0.0f;

  View fillTrack = View::New();
  fillTrack.SetRequestedHeight(4.0f);
  fillTrack.SetBackgroundColor(COLOR_SLIDER_FILL);
  fillTrack.SetCornerRadius(2.0f);
  fillTrack.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(ratio > 0.01f ? ratio : 0.01f).SetFlexBasis(0.0f));

  View thumb = View::New();
  thumb.SetRequestedWidth(16.0f);
  thumb.SetRequestedHeight(16.0f);
  thumb.SetCornerRadius(8.0f);
  thumb.SetBackgroundColor(COLOR_SLIDER_THUMB);

  View emptyTrack = View::New();
  emptyTrack.SetRequestedHeight(4.0f);
  emptyTrack.SetBackgroundColor(COLOR_SLIDER_TRACK);
  emptyTrack.SetCornerRadius(2.0f);
  float emptyRatio = 1.0f - ratio;
  emptyTrack.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(emptyRatio > 0.01f ? emptyRatio : 0.01f).SetFlexBasis(0.0f));

  trackRow.Add(fillTrack);
  trackRow.Add(thumb);
  trackRow.Add(emptyTrack);

  // Click on track to change value (simplified interaction)
  if(!boundPath.empty())
  {
    // +/- buttons for keyboard-friendly slider control
    FlexLayout btnRow = FlexLayout::New();
    btnRow.SetDirection(FlexDirection::ROW);
    btnRow.SetJustifyContent(FlexJustify::SPACE_BETWEEN);
    btnRow.SetRequestedWidth(MATCH_PARENT);
    btnRow.SetRequestedHeight(32.0f);

    float step = GetNodeFloat(*comp.rawNode, "step", 1.0f);

    FlexLayout minusBtn = FlexLayout::New();
    minusBtn.SetDirection(FlexDirection::ROW);
    minusBtn.SetJustifyContent(FlexJustify::CENTER);
    minusBtn.SetAlignItems(FlexAlign::CENTER);
    minusBtn.SetRequestedWidth(48.0f);
    minusBtn.SetRequestedHeight(32.0f);
    minusBtn.SetBackgroundColor(COLOR_BTN_SECONDARY);
    minusBtn.SetCornerRadius(6.0f);
    Label minusLabel = Label::New("-");
    minusLabel.SetFontSize(18.0f);
    minusLabel.SetTextColor(UiColor(0xFFFFFF));
    minusLabel.SetRequestedWidth(WRAP_CONTENT);
    minusLabel.SetRequestedHeight(WRAP_CONTENT);
    minusBtn.Add(minusLabel);

    minusBtn.AsInteractive([this, boundPath, step, minVal, ctx](InteractiveTrait& trait) mutable {
      trait.ClickedSignal().Connect(this,
        [boundPath, step, minVal, ctx](View, const InputEvent&) mutable -> bool {
          float cur = ctx.GetDataModel().GetFloat(boundPath, 0.0f);
          float newVal = cur - step;
          if(newVal < minVal) newVal = minVal;
          ctx.GetDataModel().SetFloatValue(boundPath, newVal);
          return true;
        });
    });

    FlexLayout plusBtn = FlexLayout::New();
    plusBtn.SetDirection(FlexDirection::ROW);
    plusBtn.SetJustifyContent(FlexJustify::CENTER);
    plusBtn.SetAlignItems(FlexAlign::CENTER);
    plusBtn.SetRequestedWidth(48.0f);
    plusBtn.SetRequestedHeight(32.0f);
    plusBtn.SetBackgroundColor(COLOR_BTN_SECONDARY);
    plusBtn.SetCornerRadius(6.0f);
    Label plusLabel = Label::New("+");
    plusLabel.SetFontSize(18.0f);
    plusLabel.SetTextColor(UiColor(0xFFFFFF));
    plusLabel.SetRequestedWidth(WRAP_CONTENT);
    plusLabel.SetRequestedHeight(WRAP_CONTENT);
    plusBtn.Add(plusLabel);

    plusBtn.AsInteractive([this, boundPath, step, maxVal, ctx](InteractiveTrait& trait) mutable {
      trait.ClickedSignal().Connect(this,
        [boundPath, step, maxVal, ctx](View, const InputEvent&) mutable -> bool {
          float cur = ctx.GetDataModel().GetFloat(boundPath, 0.0f);
          float newVal = cur + step;
          if(newVal > maxVal) newVal = maxVal;
          ctx.GetDataModel().SetFloatValue(boundPath, newVal);
          return true;
        });
    });

    btnRow.Add(minusBtn);
    btnRow.Add(plusBtn);

    container.Add(valueLabel);
    container.Add(trackRow);
    container.Add(btnRow);

    // Watch for value changes
    ctx.GetDataModel().Watch(boundPath,
      [valueLabel, fillTrack, emptyTrack, minVal, maxVal](const std::string&, const std::string& val) mutable {
        float v = 0.0f;
        try { v = std::stof(val); } catch(...) {}
        std::ostringstream oss;
        oss << static_cast<int>(v);
        valueLabel.SetText(Dali::String(oss.str().c_str()));

        float r = (maxVal > minVal) ? (v - minVal) / (maxVal - minVal) : 0.0f;
        if(r < 0.01f) r = 0.01f;
        if(r > 0.99f) r = 0.99f;
        fillTrack.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(r).SetFlexBasis(0.0f));
        emptyTrack.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f - r).SetFlexBasis(0.0f));
      });
  }
  else
  {
    container.Add(valueLabel);
    container.Add(trackRow);
  }

  return container;
}

// ========================================================================
// DateTimeInput (Phase 2) — simplified as formatted TextField
// ========================================================================

View A2uiRenderer::RenderDateTimeInput(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);
  container.SetMargin(Extents(0, 0, 4, 4));

  const char* labelText = GetNodeString(*comp.rawNode, "label", "Date/Time");
  Label label = Label::New(labelText);
  label.SetFontSize(12.0f);
  label.SetTextColor(UiColor(0xAAAACC));
  label.SetRequestedWidth(MATCH_PARENT);
  label.SetRequestedHeight(WRAP_CONTENT);
  label.SetMargin(Extents(0, 0, 0, 4));
  container.Add(label);

  // Same Label-based workaround as TextField (InputField OnChildAdd crash)
  FlexLayout inputBox = FlexLayout::New();
  inputBox.SetDirection(FlexDirection::ROW);
  inputBox.SetAlignItems(FlexAlign::CENTER);
  inputBox.SetRequestedWidth(MATCH_PARENT);
  inputBox.SetRequestedHeight(44.0f);
  inputBox.SetBackgroundColor(COLOR_INPUT_BG);
  inputBox.SetCornerRadius(6.0f);
  inputBox.SetPadding(Extents(12, 12, 0, 0));

  const TreeNode* valueNode = comp.rawNode->Find("value");
  std::string boundPath = valueNode ? GetBoundPath(valueNode, ctx) : "";

  std::string displayText;
  if(!boundPath.empty())
  {
    displayText = ctx.GetDataModel().GetString(boundPath);
  }
  if(displayText.empty()) displayText = "YYYY-MM-DD";

  Label inputLabel = Label::New(displayText.c_str());
  inputLabel.SetFontSize(16.0f);
  inputLabel.SetTextColor(displayText == "YYYY-MM-DD" ? UiColor(0x666688) : COLOR_TEXT_DEFAULT);
  inputLabel.SetRequestedWidth(MATCH_PARENT);
  inputLabel.SetRequestedHeight(WRAP_CONTENT);
  inputBox.Add(inputLabel);

  if(!boundPath.empty())
  {
    ctx.GetDataModel().Watch(boundPath,
      [inputLabel](const std::string&, const std::string& val) mutable {
        inputLabel.SetText(Dali::String(val.c_str()));
        inputLabel.SetTextColor(val.empty() ? UiColor(0x666688) : COLOR_TEXT_DEFAULT);
      });
  }

  container.Add(inputBox);

  // Error label
  Label errorLabel = Label::New("");
  errorLabel.SetFontSize(11.0f);
  errorLabel.SetTextColor(COLOR_ERROR);
  errorLabel.SetRequestedWidth(MATCH_PARENT);
  errorLabel.SetRequestedHeight(WRAP_CONTENT);
  errorLabel.SetProperty(Actor::Property::VISIBLE, false);
  container.Add(errorLabel);

  if(!boundPath.empty())
  {
    SetupChecks(comp, ctx, errorLabel, boundPath);
  }

  return container;
}

// ========================================================================
// Tabs (Phase 2)
// ========================================================================

View A2uiRenderer::RenderTabs(const ComponentModel& comp,
                              const SurfaceComponentsModel& components,
                              DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  FlexLayout container = FlexLayout::New();
  container.SetDirection(FlexDirection::COLUMN);
  container.SetRequestedWidth(MATCH_PARENT);
  container.SetRequestedHeight(WRAP_CONTENT);

  const TreeNode* tabsNode = comp.rawNode->Find("tabs");
  if(!tabsNode || tabsNode->GetType() != TreeNode::ARRAY)
  {
    return container;
  }

  // Tab header row
  FlexLayout tabHeader = FlexLayout::New();
  tabHeader.SetDirection(FlexDirection::ROW);
  tabHeader.SetRequestedWidth(MATCH_PARENT);
  tabHeader.SetRequestedHeight(44.0f);

  // Content area
  FlexLayout contentArea = FlexLayout::New();
  contentArea.SetDirection(FlexDirection::COLUMN);
  contentArea.SetRequestedWidth(MATCH_PARENT);
  contentArea.SetRequestedHeight(WRAP_CONTENT);

  // Collect tab content views
  std::vector<View> tabContentViews;
  std::vector<FlexLayout> tabButtons;

  int tabIndex = 0;
  for(auto it = tabsNode->CBegin(); it != tabsNode->CEnd(); ++it)
  {
    const TreeNode& tabDef = (*it).second;
    const TreeNode* titleNode = tabDef.Find("title");
    const TreeNode* childNode = tabDef.Find("child");

    std::string title = (titleNode && titleNode->GetType() == TreeNode::STRING)
                        ? titleNode->GetString() : "Tab";
    std::string childId = (childNode && childNode->GetType() == TreeNode::STRING)
                          ? childNode->GetString() : "";

    // Tab button
    FlexLayout tabBtn = FlexLayout::New();
    tabBtn.SetDirection(FlexDirection::ROW);
    tabBtn.SetJustifyContent(FlexJustify::CENTER);
    tabBtn.SetAlignItems(FlexAlign::CENTER);
    tabBtn.SetRequestedHeight(44.0f);
    tabBtn.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexBasis(0.0f));
    tabBtn.SetBackgroundColor(tabIndex == 0 ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE);
    tabBtn.SetCornerRadius(6.0f, 6.0f, 0.0f, 0.0f);

    Label tabLabel = Label::New(title.c_str());
    tabLabel.SetFontSize(14.0f);
    tabLabel.SetTextColor(UiColor(0xFFFFFF));
    tabLabel.SetRequestedWidth(WRAP_CONTENT);
    tabLabel.SetRequestedHeight(WRAP_CONTENT);
    tabBtn.Add(tabLabel);

    tabButtons.push_back(tabBtn);
    tabHeader.Add(tabBtn);

    // Tab content
    View tabContent;
    if(!childId.empty())
    {
      tabContent = RenderComponent(childId, components, ctx);
    }
    else
    {
      tabContent = View::New();
    }
    tabContent.SetRequestedWidth(MATCH_PARENT);
    tabContent.SetProperty(Actor::Property::VISIBLE, tabIndex == 0);
    tabContentViews.push_back(tabContent);
    contentArea.Add(tabContent);

    tabIndex++;
  }

  // Tab click handlers
  for(int i = 0; i < static_cast<int>(tabButtons.size()); ++i)
  {
    tabButtons[i].AsInteractive(
      [this, i, tabButtons, tabContentViews](InteractiveTrait& trait) mutable {
        trait.ClickedSignal().Connect(this,
          [i, tabButtons, tabContentViews](View, const InputEvent&) mutable -> bool {
            for(int j = 0; j < static_cast<int>(tabButtons.size()); ++j)
            {
              tabButtons[j].SetBackgroundColor(j == i ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE);
              tabContentViews[j].SetProperty(Actor::Property::VISIBLE, j == i);
            }
            return true;
          });
      });
  }

  container.Add(tabHeader);
  container.Add(contentArea);
  return container;
}

// ========================================================================
// Modal (Phase 2)
// ========================================================================

View A2uiRenderer::RenderModal(const ComponentModel& comp,
                               const SurfaceComponentsModel& components,
                               DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  // Modal is initially hidden. It shows when a trigger component is clicked.
  // The overlay covers the full parent area.

  FlexLayout overlay = FlexLayout::New();
  overlay.SetDirection(FlexDirection::COLUMN);
  overlay.SetJustifyContent(FlexJustify::CENTER);
  overlay.SetAlignItems(FlexAlign::CENTER);
  overlay.SetRequestedWidth(MATCH_PARENT);
  overlay.SetRequestedHeight(400.0f);
  overlay.SetBackgroundColor(UiColor(0x000000AA));
  overlay.SetProperty(Actor::Property::VISIBLE, false);

  // Modal content card
  FlexLayout modalCard = FlexLayout::New();
  modalCard.SetDirection(FlexDirection::COLUMN);
  modalCard.SetRequestedWidth(MATCH_PARENT);
  modalCard.SetRequestedHeight(WRAP_CONTENT);
  modalCard.SetBackgroundColor(COLOR_CARD_BG);
  modalCard.SetCornerRadius(16.0f);
  modalCard.SetPadding(Extents(24, 24, 20, 20));
  modalCard.SetMargin(Extents(20, 20, 0, 0));

  // Close button
  FlexLayout closeRow = FlexLayout::New();
  closeRow.SetDirection(FlexDirection::ROW);
  closeRow.SetJustifyContent(FlexJustify::FLEX_END);
  closeRow.SetRequestedWidth(MATCH_PARENT);
  closeRow.SetRequestedHeight(28.0f);

  Label closeLabel = Label::New("X");
  closeLabel.SetFontSize(16.0f);
  closeLabel.SetTextColor(UiColor(0xCCCCCC));
  closeLabel.SetRequestedWidth(28.0f);
  closeLabel.SetRequestedHeight(28.0f);
  closeRow.Add(closeLabel);

  closeRow.AsInteractive([this, overlay](InteractiveTrait& trait) mutable {
    trait.ClickedSignal().Connect(this,
      [overlay](View, const InputEvent&) mutable -> bool {
        overlay.SetProperty(Actor::Property::VISIBLE, false);
        return true;
      });
  });

  modalCard.Add(closeRow);

  // Content
  const TreeNode* contentNode = comp.rawNode->Find("content");
  if(contentNode && contentNode->GetType() == TreeNode::STRING)
  {
    std::string contentId = contentNode->GetString();
    View content = RenderComponent(contentId, components, ctx);
    modalCard.Add(content);
  }

  overlay.Add(modalCard);

  // Also click outside to close
  overlay.AsInteractive([this, overlay](InteractiveTrait& trait) mutable {
    trait.ClickedSignal().Connect(this,
      [overlay](View, const InputEvent&) mutable -> bool {
        overlay.SetProperty(Actor::Property::VISIBLE, false);
        return true;
      });
  });

  // Store overlay reference for trigger component lookup
  // The trigger button will be found by the playground controller
  // or we can use a data-binding approach. For now, we use a simple
  // "trigger" field pointing to a component ID.
  const TreeNode* triggerNode = comp.rawNode->Find("trigger");
  if(triggerNode && triggerNode->GetType() == TreeNode::STRING)
  {
    // We'll set a flag in data model so the trigger button can check
    std::string triggerPath = "/__modal_" + comp.id;
    ctx.GetDataModel().Watch(triggerPath,
      [overlay](const std::string&, const std::string& val) mutable {
        overlay.SetProperty(Actor::Property::VISIBLE, val == "true");
      });
  }

  return overlay;
}

// ========================================================================
// List (Phase 2)
// ========================================================================

View A2uiRenderer::RenderList(const ComponentModel& comp,
                              const SurfaceComponentsModel& components,
                              DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  ScrollView scrollView = ScrollView::New();
  scrollView.SetRequestedWidth(MATCH_PARENT);
  scrollView.SetRequestedHeight(WRAP_CONTENT);

  // Direction: "vertical" (default) → COLUMN, "horizontal" → ROW
  std::string direction = GetNodeString(*comp.rawNode, "direction", "vertical");
  bool horizontal = (direction == "horizontal");

  // Web CSS-flexbox model: a horizontal-list item must not keep MATCH_PARENT
  // width (the first item would fill the row and push the rest off-screen).
  // Make each item a flex item (CSS `flex: 1 1 0`) so items share the row and
  // shrink to fit — all lay out side-by-side and fill the width. The vertical
  // list is unchanged (there MATCH_PARENT width is the cross axis = stretch).
  auto sizeItemMainAxis = [horizontal](View item) {
    if(horizontal && item && item.GetRequestedWidth() == MATCH_PARENT)
    {
      item.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f).SetFlexShrink(1.0f).SetFlexBasis(0.0f));
      item.SetRequestedWidth(0.0f);
    }
  };

  FlexLayout listContainer = FlexLayout::New();
  listContainer.SetDirection(horizontal ? FlexDirection::ROW : FlexDirection::COLUMN);
  listContainer.SetRequestedWidth(MATCH_PARENT);
  listContainer.SetRequestedHeight(WRAP_CONTENT);

  float gap = GetNodeFloat(*comp.rawNode, "gap", 4.0f);

  // Check for template children (data-driven list)
  const TreeNode* childrenNode = comp.rawNode->Find("children");

  if(childrenNode && childrenNode->GetType() == TreeNode::OBJECT)
  {
    // Official A2UI v0.9 format: children = { componentId, path }
    // componentId references another component in the surface-level components
    // map; path resolves to an ARRAY in the DataModel whose each element
    // becomes a child scope.
    const TreeNode* cidNode  = childrenNode->Find("componentId");
    const TreeNode* pathNode = childrenNode->Find("path");
    if(cidNode  && cidNode->GetType()  == TreeNode::STRING
       && pathNode && pathNode->GetType() == TreeNode::STRING)
    {
      // Official v0.9 payload recognized. Always return a list container from
      // this branch — even if the data path resolves to empty/non-ARRAY — so
      // we don't accidentally fall through to the template or childIds paths.
      std::string tmplId = cidNode->GetString();
      std::string dataBindingPath = ctx.Resolve(pathNode->GetString());
      const Dali::Ui::TreeNode* arrayNode =
        ctx.GetDataModel().ResolvePath(dataBindingPath);

      if(arrayNode && arrayNode->GetType() == TreeNode::ARRAY)
      {
        int itemIndex = 0;
        for(auto arrIt = arrayNode->CBegin(); arrIt != arrayNode->CEnd(); ++arrIt)
        {
          DataContext childCtx =
            ctx.CreateChildContext(dataBindingPath + "/" + std::to_string(itemIndex));

          View itemView = RenderComponent(tmplId, components, childCtx);
          sizeItemMainAxis(itemView);
          if(gap > 0.0f && itemIndex > 0)
          {
            uint16_t g = static_cast<uint16_t>(gap);
            itemView.SetMargin(
              horizontal ? Extents(g, 0, 0, 0) : Extents(0, 0, g, 0));
          }
          listContainer.Add(itemView);
          itemIndex++;
        }
      }

      scrollView.Add(listContainer);
      return scrollView;
    }

    const TreeNode* templateNode = childrenNode->Find("template");
    if(templateNode && templateNode->GetType() == TreeNode::OBJECT)
    {
      const TreeNode* tmplComponents = templateNode->Find("components");
      const TreeNode* tmplBinding = templateNode->Find("dataBinding");

      if(tmplComponents && tmplBinding && tmplBinding->GetType() == TreeNode::STRING)
      {
        std::string dataBindingPath = ctx.Resolve(tmplBinding->GetString());
        const Dali::Ui::TreeNode* arrayNode = ctx.GetDataModel().ResolvePath(dataBindingPath);

        if(arrayNode && arrayNode->GetType() == TreeNode::ARRAY)
        {
          SurfaceComponentsModel tmplCompModel;
          tmplCompModel.AddComponents(*tmplComponents);

          // Find the root of the template (first component)
          std::string tmplRootId;
          if(tmplComponents->GetType() == TreeNode::ARRAY)
          {
            auto firstIt = tmplComponents->CBegin();
            if(firstIt != tmplComponents->CEnd())
            {
              const TreeNode* idNode = (*firstIt).second.Find("id");
              if(idNode && idNode->GetType() == TreeNode::STRING)
              {
                tmplRootId = idNode->GetString();
              }
            }
          }

          int itemIndex = 0;
          for(auto arrIt = arrayNode->CBegin(); arrIt != arrayNode->CEnd(); ++arrIt)
          {
            DataContext childCtx = ctx.CreateChildContext(dataBindingPath + "/" + std::to_string(itemIndex));

            if(!tmplRootId.empty())
            {
              View itemView = RenderComponent(tmplRootId, tmplCompModel, childCtx);
              sizeItemMainAxis(itemView);

              if(gap > 0.0f && itemIndex > 0)
              {
                uint16_t g = static_cast<uint16_t>(gap);
                itemView.SetMargin(
                  horizontal ? Extents(g, 0, 0, 0) : Extents(0, 0, g, 0));
              }
              listContainer.Add(itemView);
            }

            itemIndex++;
          }

          scrollView.Add(listContainer);
          return scrollView;
        }
      }
    }
  }

  // Simple list: children is an array of component IDs
  int index = 0;
  for(const auto& childId : comp.childIds)
  {
    View child = RenderComponent(childId, components, ctx);
    sizeItemMainAxis(child);

    if(gap > 0.0f && index > 0)
    {
      uint16_t g = static_cast<uint16_t>(gap);
      child.SetMargin(
        horizontal ? Extents(g, 0, 0, 0) : Extents(0, 0, g, 0));
    }

    listContainer.Add(child);
    index++;
  }

  scrollView.Add(listContainer);
  return scrollView;
}

// ========================================================================
// Icon (Phase 2)
// ========================================================================

View A2uiRenderer::RenderIcon(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  const char* name = GetNodeString(*comp.rawNode, "name", "");
  float size = GetNodeFloat(*comp.rawNode, "size", 24.0f);

  View iconView = View::New();
  iconView.SetRequestedWidth(size);
  iconView.SetRequestedHeight(size);

  // Try loading icon image from icons/ directory
  std::string iconPath = mImageDir + "icons/" + name + ".png";
  std::ifstream testFile(iconPath);
  if(testFile.is_open())
  {
    testFile.close();
    iconView.SetProperty(Ui::View::Property::BACKGROUND, Property::Value(Dali::String(iconPath.c_str())));
  }
  else
  {
    // Fallback: colored square with initial letter
    iconView.SetBackgroundColor(COLOR_IMG_PLACEHOLDER);
    // No label overlay for simple icon — just a colored placeholder
  }

  // Optional color tint
  const char* color = GetNodeString(*comp.rawNode, "color", "");
  if(color[0] != '\0')
  {
    iconView.SetBackgroundColor(ParseHexColor(color));
  }

  return iconView;
}

// ========================================================================
// Checks validation (Phase 2)
// ========================================================================

void A2uiRenderer::SetupChecks(const ComponentModel& comp, DataContext& ctx,
                                Label errorLabel, const std::string& boundPath)
{
  const TreeNode* checksNode = comp.rawNode ? comp.rawNode->Find("checks") : nullptr;
  if(!checksNode || checksNode->GetType() != TreeNode::ARRAY)
  {
    return;
  }

  // Watch the bound path and run checks on every change
  ctx.GetDataModel().Watch(boundPath,
    [this, checksNode, errorLabel, ctx](const std::string&, const std::string&) mutable {
      // Run checks in order, stop at first failure
      for(auto it = checksNode->CBegin(); it != checksNode->CEnd(); ++it)
      {
        const TreeNode& check = (*it).second;
        const TreeNode* conditionNode = check.Find("condition");
        const TreeNode* messageNode = check.Find("message");

        if(!conditionNode) continue;

        std::string result = mExprParser.Evaluate(*conditionNode, ctx);
        if(result == "false")
        {
          // Validation failed
          std::string msg = (messageNode && messageNode->GetType() == TreeNode::STRING)
                            ? messageNode->GetString() : "Validation failed";
          errorLabel.SetText(Dali::String(msg.c_str()));
          errorLabel.SetProperty(Actor::Property::VISIBLE, true);
          return;
        }
      }

      // All checks passed
      errorLabel.SetProperty(Actor::Property::VISIBLE, false);
    });
}

// ========================================================================
// Data binding helpers
// ========================================================================

std::string A2uiRenderer::ResolveString(const TreeNode* propNode, const DataContext& ctx) const
{
  if(!propNode)
  {
    return "";
  }

  if(propNode->GetType() == TreeNode::STRING)
  {
    return propNode->GetString();
  }

  if(propNode->GetType() == TreeNode::OBJECT)
  {
    // Check "call" first — Find() may recurse into nested args.value.path
    const TreeNode* callNode = propNode->Find("call");
    if(callNode)
    {
      return mExprParser.Evaluate(*propNode, ctx);
    }

    const TreeNode* pathNode = propNode->Find("path");
    if(pathNode && pathNode->GetType() == TreeNode::STRING)
    {
      return ctx.GetString(pathNode->GetString());
    }
  }

  if(propNode->GetType() == TreeNode::INTEGER)
  {
    return std::to_string(propNode->GetInteger());
  }
  if(propNode->GetType() == TreeNode::FLOAT)
  {
    std::ostringstream oss;
    oss << propNode->GetFloat();
    return oss.str();
  }

  return "";
}

float A2uiRenderer::ResolveFloat(const TreeNode* propNode, const DataContext& ctx,
                                 float fallback) const
{
  if(!propNode) return fallback;

  if(propNode->GetType() == TreeNode::FLOAT) return propNode->GetFloat();
  if(propNode->GetType() == TreeNode::INTEGER) return static_cast<float>(propNode->GetInteger());

  if(propNode->GetType() == TreeNode::OBJECT)
  {
    const TreeNode* pathNode = propNode->Find("path");
    if(pathNode && pathNode->GetType() == TreeNode::STRING)
    {
      return ctx.GetFloat(pathNode->GetString(), fallback);
    }
  }

  return fallback;
}

std::string A2uiRenderer::GetBoundPath(const TreeNode* propNode, const DataContext& ctx) const
{
  if(!propNode || propNode->GetType() != TreeNode::OBJECT)
  {
    return "";
  }
  const TreeNode* pathNode = propNode->Find("path");
  if(!pathNode || pathNode->GetType() != TreeNode::STRING)
  {
    return "";
  }
  return ctx.Resolve(pathNode->GetString());
}

// ========================================================================
// Helpers
// ========================================================================

const char* A2uiRenderer::GetNodeString(const TreeNode& node, const char* key, const char* fallback)
{
  const TreeNode* child = node.Find(key);
  if(child && child->GetType() == TreeNode::STRING)
  {
    return child->GetString();
  }
  return fallback;
}

float A2uiRenderer::GetNodeFloat(const TreeNode& node, const char* key, float fallback)
{
  const TreeNode* child = node.Find(key);
  if(child)
  {
    if(child->GetType() == TreeNode::FLOAT) return child->GetFloat();
    if(child->GetType() == TreeNode::INTEGER) return static_cast<float>(child->GetInteger());
  }
  return fallback;
}

float A2uiRenderer::VariantToFontSize(const char* variant)
{
  if(strcmp(variant, "h1") == 0) return 28.0f;
  if(strcmp(variant, "h2") == 0) return 22.0f;
  if(strcmp(variant, "h3") == 0) return 18.0f;
  if(strcmp(variant, "h4") == 0) return 16.0f;
  if(strcmp(variant, "h5") == 0) return 15.0f;
  if(strcmp(variant, "caption") == 0) return 13.0f;
  return 16.0f; // body (default)
}

UiColor A2uiRenderer::ParseHexColor(const char* hex)
{
  if(!hex || hex[0] == '\0') return UiColor(0x000000);
  const char* p = (hex[0] == '#') ? hex + 1 : hex;
  uint32_t val = static_cast<uint32_t>(strtoul(p, nullptr, 16));
  return UiColor(val);
}

} // namespace A2ui
