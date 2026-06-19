/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 */

#include "renderer/a2ui-metrics.h"
#include <dali-ui-foundation/public-api/unit.h>

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
float AvatarSize()    { return 100.0_dp; }
float SmallFeature()  { return 200.0_dp; }
float MediumFeature() { return 400.0_dp; }
float LargeFeature()  { return 800.0_dp; }
float SquareImage()   { return 280.0_dp; }
float FeatureHeight() { return 200.0_dp; }

// Nominal card content width (card ~417 − 2×24 padding ≈ 369, rounded). Used to size a
// full-width image's height from its intrinsic aspect ratio before layout resolves the
// real width — the cards all render at one column width, so this is a layout constant,
// not a per-card value.
float CardContentWidth() { return 360.0_dp; }

// The web caps a full-width image's height at ~0.82·width (object-fit:cover crops a tall or
// square source into that banner box). Aspect ratio, so it is NOT dp-scaled.
float FullWidthImageMaxAspect() { return 0.82f; }

// Square media tile for an image placed directly in a Row (track art, podcast/contact
// artwork, product thumbnail). The web draws these at ~150-170px; 160dp matches.
float RowThumbnail() { return 160.0_dp; }

// Text variant font sizes (web type.ts: h1=title-large 22, h2=title-medium 16,
// h3=title-small 14, h4=body-large 16, h5/body=body-medium 14, caption=body-small 12).
float FontH1()        { return 24.0_dp; }
float FontH2()        { return 16.0_dp; }
float FontH3()        { return 14.0_dp; }
float FontH4()        { return 16.0_dp; }
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
  if(pt >= 22.0f)      lh = 28.0f;
  else if(pt >= 16.0f) lh = 24.0f;
  else if(pt >= 13.0f) lh = 20.0f;
  else                 lh = 16.0f;
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

// Spacing — the renderer's vertical rhythm is the SUM of two owners:
//   inter-element gap ≈ ColumnGap (this base) + the prior element's own bottom margin.
// ColumnGap (24) is the base spacing between distinct stacked sections; text blocks add
// 5-12 of their own bottom margin in text.cpp (matching the web's per-element mb), and form
// controls add a small block margin. Card padding ~28 (PadCard, restored top+bottom via the
// card spacer). Tuning vertical spacing means adjusting BOTH ColumnGap and the text margins
// together. Rows use the tighter RowGap (16); list/template runs use ListItemGap (14).
float PadCard()       { return 28.0_dp; }
float ColumnGap()     { return 24.0_dp; }
float RowGap()        { return 16.0_dp; }
// List / template rows are a run of similar items; the web packs them tighter than the
// general column gap (which is tuned for distinct stacked sections).
float ListItemGap()   { return 14.0_dp; }

// Form controls (web inputs are a fixed ~44px box; shared by TextField / DateTimeInput).
float InputHeight()   { return 44.0_dp; }

// Button.
float ButtonHeight()    { return 40.0_dp; }
float ButtonMinWidth()  { return 140.0_dp; }
float ButtonCharWidth() { return 9.0_dp; }
float ButtonLabelPad()  { return 8.0_dp; }

// Generic logical->device-px scale (1.0_dp == the dp factor).
float Dp(float logical) { return logical * 1.0_dp; }
} // namespace Metrics
} // namespace A2ui
