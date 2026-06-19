#include "renderer/render-internal.h"
#include <dali/devel-api/adaptor-framework/image-loading.h>

namespace A2ui
{

View A2uiRenderer::RenderImage(const ComponentModel& comp, DataContext& ctx)
{
  if(!comp.rawNode) return View::New();

  const TreeNode* urlNode = comp.rawNode->Find("url");
  std::string url = urlNode ? ResolveString(urlNode, ctx) : "";

  const char* variant = GetNodeString(*comp.rawNode, "variant", "");

  // Web composer rule (viewerTheme Image.all = width:100%, object-fit:cover): an image with
  // no explicit size variant fills the container width and takes its height from its own
  // aspect ratio — a full-width banner, NOT a centred square. Fixed-size variants
  // (icon/avatar/feature) keep the catalog's boxes; a Row thumbnail stays small.
  bool thumbnail = mImageThumbnailHint;
  mImageThumbnailHint = false;

  float w = Metrics::SquareImage();
  float h = w;
  bool  isHeader   = false;
  bool  fullWidth  = false;   // fill container width, height from aspect ratio

  if(strcmp(variant, "icon") == 0)               { w = h = Metrics::IconSize(); }
  else if(strcmp(variant, "avatar") == 0)        { w = h = Metrics::AvatarSize(); }
  else if(strcmp(variant, "smallFeature") == 0)  { w = h = Metrics::SmallFeature(); }
  else if(strcmp(variant, "mediumFeature") == 0) { w = Metrics::MediumFeature(); h = Metrics::SmallFeature(); }
  else if(strcmp(variant, "largeFeature") == 0)  { w = Metrics::LargeFeature();  h = Metrics::FeatureHeight(); }
  else if(strcmp(variant, "square") == 0)        { w = h = Metrics::SquareImage(); }
  else if(strcmp(variant, "header") == 0)        { w = MATCH_PARENT; h = Metrics::FeatureHeight(); isHeader = true; }
  else if(thumbnail)                             { w = h = Metrics::RowThumbnail(); }  // Row media tile (~160dp square)
  else                                           { fullWidth = true; }          // default → full-width media

  // Explicit width/height on the A2UI node override the variant default (the catalog's
  // posters declare e.g. width:150, height:225). These are logical units, so dp-scale them;
  // an explicit size also opts the image out of full-width sizing.
  if(!isHeader)
  {
    float jw = GetNodeFloat(*comp.rawNode, "width", -1.0f);
    float jh = GetNodeFloat(*comp.rawNode, "height", -1.0f);
    if(jw > 0.0f) { w = Metrics::Dp(jw); fullWidth = false; }
    if(jh > 0.0f) { h = Metrics::Dp(jh); fullWidth = false; }
  }

  // Resolve a url to a loadable path: remote urls pass through (DALi loads what it can);
  // a relative local path is prefixed with the image dir and must exist on disk.
  auto resolvePath = [this](const std::string& u) -> std::string {
    if(u.empty()) return std::string();
    if(u.find("://") != std::string::npos) return u;
    std::string full = (u[0] == '/') ? u : (mImageDir + u);
    std::ifstream f(full);
    return f.is_open() ? full : std::string();
  };

  // Full-width media height. The web draws a full-width image at its intrinsic aspect ratio
  // but CAPS the height at ~0.82·width (object-fit:cover crops a tall/square source into that
  // box) — so a wide photo stays a short banner and a square cover becomes a ~5:4 banner, not
  // a full square. Takes an ALREADY-RESOLVED path (callers resolve once; re-resolving here
  // would double the image-dir prefix and silently fall back). Global rule, no per-card sizing.
  auto fullWidthHeight = [](const std::string& resolvedPath) -> float {
    float aspect = 0.66f;  // h/w fallback for an unknown/remote file (3:2 landscape)
    if(!resolvedPath.empty())
    {
      Dali::ImageDimensions dim = Dali::GetClosestImageSize(resolvedPath);
      if(dim.GetWidth() > 0 && dim.GetHeight() > 0)
        aspect = static_cast<float>(dim.GetHeight()) / static_cast<float>(dim.GetWidth());
    }
    if(aspect < 0.30f) aspect = 0.30f;                         // clamp extreme panoramas
    if(aspect > Metrics::FullWidthImageMaxAspect())            // web cover-crop cap (~0.82)
      aspect = Metrics::FullWidthImageMaxAspect();
    return aspect * Metrics::CardContentWidth();
  };

  float cornerR = Metrics::RadiusImage();
  bool isAvatar = (strcmp(variant, "avatar") == 0);
  bool isIcon   = (strcmp(variant, "icon") == 0);
  if(isAvatar && w > 0 && h > 0) cornerR = std::min(w, h) * 0.5f;  // avatar = full circle

  // A2UI fit → DALi fitting mode. "cover" (default) fills the box keeping aspect (crops
  // overflow); "contain" fits inside; "fill" stretches.
  const char* fitSpec = GetNodeString(*comp.rawNode, "fit",
                          GetNodeString(*comp.rawNode, "fittingMode", "cover"));
  auto fitMode = Ui::Image::FittingMode::OVER_FIT_KEEP_ASPECT_RATIO; // cover
  if(strcmp(fitSpec, "contain") == 0)   fitMode = Ui::Image::FittingMode::FIT_KEEP_ASPECT_RATIO;
  else if(strcmp(fitSpec, "fill") == 0) fitMode = Ui::Image::FittingMode::FILL;

  // Build the image view for a resolved path — or a grey placeholder when it's empty.
  auto buildView = [this, isAvatar, isIcon, isHeader, fullWidth, w, h, cornerR, fitMode, fullWidthHeight]
                   (const std::string& resolved) -> View {
    float vh = fullWidth ? fullWidthHeight(resolved) : h;
    if(resolved.empty())
    {
      View ph = View::New();
      ph.SetRequestedWidth((isHeader || fullWidth) ? MATCH_PARENT : w);
      ph.SetRequestedHeight(vh);
      ph.SetCornerRadius(cornerR);
      ph.SetBackgroundColor(COLOR_IMG_PLACEHOLDER);
      if(!isHeader && !fullWidth) ph.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
      return ph;
    }
    ImageView img = ImageView::New(resolved.c_str());
    img.SetFittingMode(fitMode);
    // SetCornerRadius does NOT clip an ImageView's image visual, so round the picture with
    // an alpha mask: a full circle for avatars, a rounded rectangle for everything else
    // (the web rounds every image). Falls back to SetCornerRadius if the mask is missing.
    std::string maskPath = mImageDir + (isAvatar ? "masks/circle.png" : "masks/rounded.png");
    std::ifstream maskFile(maskPath);
    if(maskFile.is_open()) { maskFile.close(); img.SetAlphaMaskUrl(maskPath.c_str()); }
    else img.SetCornerRadius(cornerR);
    img.SetDesiredWidth(static_cast<int>(w > 0.0f ? w : Metrics::CardContentWidth()));
    img.SetDesiredHeight(static_cast<int>(vh));
    if(isHeader || fullWidth)
    {
      // Fill the container width; height follows the variant box (header) or the aspect
      // ratio (full-width). align:stretch (the column default) keeps it edge-to-edge.
      img.SetRequestedWidth(MATCH_PARENT);
      img.SetRequestedHeight(vh);
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
  // updateComponents, before the updateDataModel that carries the url. Wrap the image in a
  // stable container and rebuild it when the bound value arrives (a one-shot placeholder
  // can't self-replace).
  std::string boundPath = (urlNode && urlNode->GetType() == TreeNode::OBJECT)
                          ? GetBoundPath(urlNode, ctx) : std::string();
  if(!boundPath.empty())
  {
    View container = View::New();
    container.SetRequestedWidth((isHeader || fullWidth) ? MATCH_PARENT : w);
    container.SetRequestedHeight(fullWidth ? fullWidthHeight(resolvePath(url)) : h);
    if(!isHeader && !fullWidth) container.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
    container.Add(buildView(resolvePath(url)));
    bool capFull = fullWidth;
    ctx.GetDataModel().Watch(boundPath,
      [container, buildView, resolvePath, fullWidthHeight, capFull](const std::string&, const std::string& val) mutable {
        while(container.GetChildCount() > 0) container.Remove(container.GetChildAt(0u));
        if(capFull) container.SetRequestedHeight(fullWidthHeight(resolvePath(val)));
        container.Add(buildView(resolvePath(val)));
      });
    return container;
  }

  return buildView(resolvePath(url));
}
} // namespace A2ui
