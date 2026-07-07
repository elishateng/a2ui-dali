#pragma once

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 */

namespace A2ui
{

/**
 * @brief Dimensional tokens (density-independent px) for the renderer.
 *
 * dali-ui has no dimension/spacing token concept yet (only colours, via the theme
 * manager), so these live locally for now — mirroring the reference renderer's ImageVariables /
 * A2UIComponentVariables / TextVariables. They are computed via dali-ui's `_dp` unit so
 * they scale with DPI exactly like the reference renderer's `.Dp()`.
 *
 * Functions (not constants) because `_dp` must NOT run during static init — it needs
 * UiConfigManager::Init() to have run, which is only guaranteed at render time.
 *
 * Image sizes match the reference renderer's values exactly. Font sizes / radii currently carry
 * a2ui-dali's existing values (which track the reference layout well) expressed in dp;
 * reconciling each with the reference renderer's exact OneUI value is a per-element review item.
 */
namespace Metrics
{
// --- Image variant sizes (catalog fixed-size variants) ---
float IconSize();      // 50
float AvatarSize();    // 100 (centred hero avatar)
float AvatarSmall();   // 40  (avatar inside a Row — chat/list item)
float SmallFeature();  // 200
float MediumFeature(); // 400
float LargeFeature();  // 800
float SquareImage();   // 280
float FeatureHeight(); // 200
float CardContentWidth(); // 400 (measured card content width = full-width-image height basis)
float FullWidthImageMaxAspect(); // 0.72 (web cover-crop height cap, h/w — measured)
float RowThumbnail();  // 160 (square media tile for an image inside a Row)

// --- Text variant font sizes (MEASURED from the deployed composer, not the spec) ---
float FontH1();        // 24 (measured; spec said 22)
float FontH2();        // 20 (measured; spec said 16) — card names/titles, BOLD
float FontH3();        // 18 (measured; spec said 14) — section titles, BOLD
float FontH4();        // 16 (sub-heading, semibold)
float FontH5();        // 14
float FontCaption();   // 12
float FontBody();      // 14
float LineHeight(float fontSize); // web size→line-height pairing

// --- Component font sizes ---
float FontButton();    // 14
float FontInput();     // 14
float FontLabel();     // 12
float FontError();     // 12
float FontStepper();   // 18 (slider +/-)
float FontChoice();    // 14 (checkbox / radio option label)

// --- Corner radii ---
float RadiusCard();    // 16
float RadiusInput();   // 8
float RadiusImage();   // 16
float RadiusCheck();   // 4
float RadiusRadio();   // 10
float RadiusTab();     // 6
float RadiusSliderThumb(); // 8
float RadiusSliderTrack(); // 2

// --- Borderline widths ---
float BorderCard();    // 1
float BorderInput();   // 1

// --- Spacing ---
// The column gap is the SOLE vertical spacer between stacked elements (text bottom-margins
// and divider margins are 0). Re-measured against the honest width-aligned diagnostic: ~20
// between elements, ~40 (2×) around a divider — see a2ui-metrics.cpp.
float PadCard();       // 24 (measured card padding)
float ColumnGap();     // 20 (sole inter-element vertical gap)
float RowGap();        // 16
float ListItemGap();   // 14 (tighter inter-row spacing for list/template runs)

// --- Form controls ---
float InputHeight();     // 38 (shared by TextField / DateTimeInput)

// --- Button ---
float ButtonHeight();    // 44 (measured: tall rounded pill; spec said ~36)
float ButtonMinWidth();  // 140
float ButtonCharWidth(); // 9  (glyph advance estimate at the button font)
float ButtonLabelPad();  // 8

// Scale an A2UI-declared logical length to device pixels by the dp factor, so inline
// width/height/padding in a message track display density (and high-DPI capture).
float Dp(float logical);
} // namespace Metrics

} // namespace A2ui
