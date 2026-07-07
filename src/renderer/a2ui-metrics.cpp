/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 */

#include "renderer/a2ui-metrics.h"
#include <dali-ui-foundation/public-api/types/unit.h>

using namespace Dali::Ui; // brings the _dp user-defined literal into scope

namespace A2ui
{
namespace Metrics
{
// Dimension tokens are derived from the A2UI web renderer's own style scale
// (renderers/web_core/src/v0_8/styles), which the composer gallery renders with:
//   grid base   = 4px      → spacing/radius tokens are multiples of it
//   type scale  = type.ts  → font sizes + line heights
// so a2ui-dali lays out at the same proportions as the web composer instead of the
// (larger) OneUI defaults it previously carried. Sizes that the web renderer leaves to
// the image's own intrinsics (full-width media) are handled in image.cpp, not here.

// Image variant sizes — explicit-size variants (icon/avatar/feature) keep the catalog's
// fixed boxes; the default (no-variant) image is full-width and sized by image.cpp.
float IconSize()      { return 50.0_dp; }
float AvatarSize()    { return 80.0_dp; }   // measured: web hero avatar ~67px vs dali's 100dp (~85) → 80
// A chat/list-item avatar sits inside a Row next to a text column; the web draws it FAR
// smaller than a centred profile/contact hero avatar. One 100dp box can't serve both, so a
// Row-context avatar uses this small box (~40px in the web gallery) — see flex-container.
float AvatarSmall()   { return 40.0_dp; }
float SmallFeature()  { return 200.0_dp; }
float MediumFeature() { return 400.0_dp; }
float LargeFeature()  { return 800.0_dp; }
float SquareImage()   { return 280.0_dp; }
float FeatureHeight() { return 200.0_dp; }

// Card content width — the MEASURED rendered width of a card's content area (card ~448 −
// 2×24 padding ≈ 400 at the capture geometry). Used to size a full-width image's height from
// its intrinsic aspect ratio before layout resolves the real width; it must match the ACTUAL
// rendered image width or the full-width image comes out too short. A layout constant (the
// cards all render at one column width), not a per-card value.
float CardContentWidth() { return 368.0_dp; }   // MEASURED: web card content ≈367px; dali was
                                                 // rendering 400 (capture 960 → 2.18×web, not 2×),
                                                 // so full-width images/cards came out ~9% wider.
                                                 // Capture now uses 900px → content 368dp = 2×web.

// The web renders a full-width image as object-fit:cover into a box capped at h/w = 0.72·
// width: a tall/portrait or square source (movie poster 200x300=1.5, album art 1.0) is
// cover-cropped down to this banner; a wide source keeps its own (shorter) ratio. So box
// aspect = min(sourceAspect, 0.72). MEASURED from the web renders: a movie poster (src 1.5)
// AND square album art (src 1.0) BOTH land at h/w=0.717 (imgH 299 in a 417px content width) —
// two different sources hitting the same value pins the cap. (0.82 was too tall; the banner
// came out ~14% taller than the web and over-zoomed the cover-crop.) NOT dp-scaled.
float FullWidthImageMaxAspect() { return 0.72f; }

// Square media tile for an image placed directly in a Row (track art, podcast/contact
// artwork, product thumbnail). The web draws these at ~150-170px; 160dp matches.
float RowThumbnail() { return 160.0_dp; }

// Text variant font sizes — MEASURED from the deployed composer's rendered glyph heights
// (cap height ≈ 0.7·font-size), NOT the published type.ts. The live gallery renders headings
// LARGER and BOLDER than the spec: a card name (h2) measures a 14px cap in a 440px card ⇒
// ~20px, restaurant/movie titles (h3) a 13px cap ⇒ ~18px, a calendar title (h1) a 17px cap ⇒
// ~24px. Body/caption matched the spec (14 / 12) and stay. Getting these right is what makes a
// title look the same WEIGHT and SIZE as the web instead of a flat near-body line.
float FontH1()        { return 24.0_dp; }  // measured ~24 (spec said 22)
float FontH2()        { return 20.0_dp; }  // measured ~20 (spec said 16) — card names/titles
float FontH3()        { return 18.0_dp; }  // measured ~18 (spec said 14) — section titles
float FontH4()        { return 16.0_dp; }  // body-large sub-heading
float FontH5()        { return 14.0_dp; }
float FontCaption()   { return 12.0_dp; }
float FontBody()      { return 14.0_dp; }

// Component font sizes.
float FontButton()    { return 14.0_dp; }
float FontInput()     { return 14.0_dp; }
float FontLabel()     { return 12.0_dp; }
float FontError()     { return 12.0_dp; }
float FontStepper()   { return 18.0_dp; }
float FontChoice()    { return 14.0_dp; }

// Line height for a given font size — mirrors the web type scale's size→line-height
// pairs (22→28, 16→24, 14→20, 12→16). Used to reserve a multi-line Label's height so
// wrapped copy occupies the same vertical space as the web renderer.
float LineHeight(float fontSize)
{
  float dp = 1.0_dp;
  float pt = fontSize / dp;            // recover the logical px size
  float lh;
  if(pt >= 23.0f)      lh = 30.0f;     // h1 24
  else if(pt >= 19.0f) lh = 26.0f;     // h2 20
  else if(pt >= 16.0f) lh = 24.0f;     // h3 18 / h4 16
  else if(pt >= 13.0f) lh = 20.0f;     // body 14
  else                 lh = 16.0f;     // caption 12
  return lh * dp;
}

// Corner radii — web composer renders cards/inputs/images with moderate radii
// (measured from the live gallery: card ~16, input ~8, image ~16).
float RadiusCard()        { return 16.0_dp; }
float RadiusInput()       { return 8.0_dp; }
float RadiusImage()       { return 16.0_dp; }
float RadiusCheck()       { return 4.0_dp; }
float RadiusRadio()       { return 10.0_dp; }
float RadiusTab()         { return 6.0_dp; }
float RadiusSliderThumb() { return 8.0_dp; }
float RadiusSliderTrack() { return 2.0_dp; }

// Borderline widths (web card/input edges are a hairline 1px).
float BorderCard()    { return 1.0_dp; }
float BorderInput()   { return 1.0_dp; }

// Spacing — calibrated to the DEPLOYED web gallery, MEASURED from the renders (the
// viewerTheme/atomic-class spec says g-2=8 but the deployed composer does not match it —
// like its radii/weights). Re-measured against the honest width-aligned diagnostic: normal
// inter-element seams are ~15-17px in a width-300 trim (web body gaps cluster 11-19), with
// ~28-29 only around a divider/section break. With text margins = 0 and divider margin = 0,
// the column gap is the SOLE vertical spacer, so ~20 reproduces both: ~20 between elements and
// ~40 (2×) around a divider. (28 was too loose — every seam read ~25% wider than the web and,
// because SSIM is unforgiving of vertical drift, shifted every line below it off its web row.)
// Rows use RowGap (16); list/template runs use the tighter ListItemGap. Card padding ≈ 24.
float PadCard()       { return 24.0_dp; }
float ColumnGap()     { return 20.0_dp; }
float RowGap()        { return 12.0_dp; }   // 16 read too wide on inline icon+text / "Vienna → NY" rows
// List / template rows are a run of similar items; the web packs them tighter than the
// general column gap (which is tuned for distinct stacked sections).
float ListItemGap()   { return 14.0_dp; }

// Form controls (web input box measures ~38px: 8+8 padding + content line; shared by
// TextField / DateTimeInput).
float InputHeight()   { return 38.0_dp; }

// Button — frame-aligned comparison put the web pill at ~36-40px (44 read a touch tall). 40.
float ButtonHeight()    { return 40.0_dp; }
float ButtonMinWidth()  { return 140.0_dp; }
float ButtonCharWidth() { return 9.0_dp; }   // glyph advance at the button font (8 clipped "Transfer")
float ButtonLabelPad()  { return 8.0_dp; }

// Generic logical->device-px scale (1.0_dp == the dp factor).
float Dp(float logical) { return logical * 1.0_dp; }
} // namespace Metrics
} // namespace A2ui
