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
// Image variant sizes — mirror the reference renderer ImageVariables exactly.
float IconSize()      { return 50.0_dp; }
float AvatarSize()    { return 100.0_dp; }
float SmallFeature()  { return 200.0_dp; }
float MediumFeature() { return 400.0_dp; }
float LargeFeature()  { return 800.0_dp; }
float SquareImage()   { return 280.0_dp; }
float FeatureHeight() { return 200.0_dp; }

// Text variant font sizes.
float FontH1()        { return 28.0_dp; }
float FontH2()        { return 22.0_dp; }
float FontH3()        { return 18.0_dp; }
float FontH4()        { return 16.0_dp; }
float FontH5()        { return 15.0_dp; }
float FontCaption()   { return 13.0_dp; }
float FontBody()      { return 16.0_dp; }

// Component font sizes.
float FontButton()    { return 14.0_dp; }
float FontInput()     { return 16.0_dp; }
float FontLabel()     { return 12.0_dp; }
float FontError()     { return 11.0_dp; }
float FontStepper()   { return 18.0_dp; }
float FontChoice()    { return 15.0_dp; }

// Corner radii.
float RadiusCard()        { return 12.0_dp; }
float RadiusInput()       { return 6.0_dp; }
float RadiusCheck()       { return 4.0_dp; }
float RadiusRadio()       { return 10.0_dp; }
float RadiusTab()         { return 6.0_dp; }
float RadiusSliderThumb() { return 8.0_dp; }
float RadiusSliderTrack() { return 2.0_dp; }

// Borderline widths.
float BorderCard()    { return 1.5_dp; }
float BorderInput()   { return 1.0_dp; }

// Spacing.
float PadCard()       { return 16.0_dp; }
float ColumnGap()     { return 15.0_dp; }
float RowGap()        { return 15.0_dp; }

// Button.
float ButtonHeight()    { return 40.0_dp; }
float ButtonMinWidth()  { return 140.0_dp; }
float ButtonCharWidth() { return 9.0_dp; }
float ButtonLabelPad()  { return 8.0_dp; }

// Generic logical->device-px scale (1.0_dp == the dp factor).
float Dp(float logical) { return logical * 1.0_dp; }
} // namespace Metrics
} // namespace A2ui
