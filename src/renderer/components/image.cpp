#include "renderer/render-internal.h"

namespace A2ui
{

View A2uiRenderer::RenderImage(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  const TreeNode* urlNode = comp.rawNode->Find("url");
  std::string url = urlNode ? ResolveString(urlNode, ctx) : "";

  const char* variant = GetNodeString(*comp.rawNode, "variant", "");
  // Default to a 1:1 square so agent-generated images (confirmation, booking)
  // don't get stretched into a wide-short strip by the parent Column's default
  // alignItems: STRETCH. (dp-scaled so it tracks display density / high-DPI capture.)
  // A Row thumbnail (hinted by RenderFlexContainer) uses a small square instead.
  bool thumbnail = mImageThumbnailHint;
  mImageThumbnailHint = false;
  float w = thumbnail ? Metrics::Dp(56) : Metrics::SquareImage();
  float h = w;
  bool  isHeader = false;

  // Sizes match the reference renderer ImageVariables (dp-scaled via Metrics; see a2ui-metrics.h).
  if(strcmp(variant, "icon") == 0)               { w = h = Metrics::IconSize(); }
  else if(strcmp(variant, "avatar") == 0)        { w = h = Metrics::AvatarSize(); }
  else if(strcmp(variant, "smallFeature") == 0)  { w = h = Metrics::SmallFeature(); }
  else if(strcmp(variant, "mediumFeature") == 0) { w = Metrics::MediumFeature(); h = Metrics::SmallFeature(); }
  else if(strcmp(variant, "largeFeature") == 0)  { w = Metrics::LargeFeature();  h = Metrics::FeatureHeight(); }
  else if(strcmp(variant, "square") == 0)        { w = h = Metrics::SquareImage(); }
  else if(strcmp(variant, "header") == 0)        { w = MATCH_PARENT; h = Metrics::FeatureHeight(); isHeader = true; }

  // Explicit width/height on the A2UI node take precedence over the variant
  // default (the catalog's posters declare e.g. width:150, height:225). These are
  // logical units, so dp-scale them too.
  if(!isHeader)
  {
    float jw = GetNodeFloat(*comp.rawNode, "width", -1.0f);
    float jh = GetNodeFloat(*comp.rawNode, "height", -1.0f);
    if(jw > 0.0f) w = Metrics::Dp(jw);
    if(jh > 0.0f) h = Metrics::Dp(jh);
  }

  // Avatar images are circular (the reference renderer sets CornerRadius=1 for avatar); all
  // other variants use a 12dp rounded corner.
  float cornerR = 12.0f;
  if(strcmp(variant, "avatar") == 0 && w > 0 && h > 0) cornerR = std::min(w, h) * 0.5f;

  // A2UI fit → DALi fitting mode. "cover" (default) fills the box keeping aspect
  // (crops overflow); "contain" fits inside keeping aspect; "fill" stretches.
  const char* fitSpec = GetNodeString(*comp.rawNode, "fit",
                          GetNodeString(*comp.rawNode, "fittingMode", "cover"));
  auto fitMode = Ui::Image::FittingMode::OVER_FIT_KEEP_ASPECT_RATIO; // cover
  if(strcmp(fitSpec, "contain") == 0)   fitMode = Ui::Image::FittingMode::FIT_KEEP_ASPECT_RATIO;
  else if(strcmp(fitSpec, "fill") == 0) fitMode = Ui::Image::FittingMode::FILL;

  bool isAvatar = (strcmp(variant, "avatar") == 0);
  bool isIcon   = (strcmp(variant, "icon") == 0);

  // Resolve a url to a loadable path: remote urls pass through (DALi loads what it can);
  // a relative local path is prefixed with the image dir and must exist on disk.
  auto resolvePath = [this](const std::string& u) -> std::string {
    if(u.empty()) return std::string();
    if(u.find("://") != std::string::npos) return u;
    std::string full = (u[0] == '/') ? u : (mImageDir + u);
    std::ifstream f(full);
    return f.is_open() ? full : std::string();
  };

  // Build the image view for a resolved path — or a grey placeholder when it's empty.
  // Web-responsive sizing: width fills the container up to the declared max, height is
  // fixed, and fit:cover crops to fill (so a poster keeps its length instead of shrinking).
  auto buildView = [this, isAvatar, isIcon, isHeader, w, h, cornerR, fitMode](const std::string& resolved) -> View {
    if(resolved.empty())
    {
      View ph = View::New();
      ph.SetRequestedWidth(isHeader ? MATCH_PARENT : w);
      ph.SetRequestedHeight(h);
      ph.SetCornerRadius(cornerR);
      ph.SetBackgroundColor(COLOR_IMG_PLACEHOLDER);
      if(!isHeader) ph.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
      return ph;
    }
    ImageView img = ImageView::New(resolved.c_str());
    img.SetFittingMode(fitMode);
    // SetCornerRadius does NOT clip an ImageView's image visual (verified), so round the
    // picture itself with an alpha mask: a full circle for avatars, a rounded rectangle
    // for everything else (the web rounds every image). Falls back to SetCornerRadius if
    // the mask file is missing.
    std::string maskPath = mImageDir + (isAvatar ? "masks/circle.png" : "masks/rounded.png");
    std::ifstream maskFile(maskPath);
    if(maskFile.is_open()) { maskFile.close(); img.SetAlphaMaskUrl(maskPath.c_str()); }
    else img.SetCornerRadius(cornerR);
    img.SetDesiredWidth(static_cast<int>(w > 0 ? w : 0));
    img.SetDesiredHeight(static_cast<int>(h));
    if(isHeader)
    {
      img.SetRequestedWidth(MATCH_PARENT);
      img.SetRequestedHeight(h);
    }
    else if(isAvatar || isIcon)
    {
      img.SetRequestedWidth(w);
      img.SetRequestedHeight(h);
      img.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
    }
    else
    {
      img.SetRequestedWidth(MATCH_PARENT);
      img.SetMaximumWidth(w);
      img.SetRequestedHeight(h);
      img.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
    }
    return img;
  };

  // A data-bound url is usually empty at first paint — the surface renders on
  // updateComponents, before the updateDataModel that carries the url. Wrap the image
  // in a stable container and rebuild it when the bound value arrives, the same reactive
  // pattern RenderText uses for bound text (a one-shot placeholder can't self-replace).
  std::string boundPath = (urlNode && urlNode->GetType() == TreeNode::OBJECT)
                          ? GetBoundPath(urlNode, ctx) : std::string();
  if(!boundPath.empty())
  {
    View container = View::New();
    container.SetRequestedWidth(isHeader ? MATCH_PARENT : w);
    container.SetRequestedHeight(h);
    if(!isHeader) container.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
    container.Add(buildView(resolvePath(url)));
    ctx.GetDataModel().Watch(boundPath,
      [container, buildView, resolvePath](const std::string&, const std::string& val) mutable {
        while(container.GetChildCount() > 0) container.Remove(container.GetChildAt(0u));
        container.Add(buildView(resolvePath(val)));
      });
    return container;
  }

  return buildView(resolvePath(url));
}
} // namespace A2ui
