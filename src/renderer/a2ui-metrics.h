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
// --- Image variant sizes (match the reference renderer ImageVariables) ---
float IconSize();      // 50
float AvatarSize();    // 100
float SmallFeature();  // 200
float MediumFeature(); // 400
float LargeFeature();  // 800
float SquareImage();   // 280
float FeatureHeight(); // 200

// --- Text variant font sizes ---
float FontH1();        // 28
float FontH2();        // 22
float FontH3();        // 18
float FontH4();        // 16
float FontH5();        // 15
float FontCaption();   // 13
float FontBody();      // 16

// --- Component font sizes ---
float FontButton();    // 14
float FontInput();     // 16
float FontLabel();     // 12
float FontError();     // 11
float FontStepper();   // 18 (slider +/-)
float FontChoice();    // 15 (checkbox / radio option label)

// --- Corner radii ---
float RadiusCard();    // 12
float RadiusInput();   // 6
float RadiusCheck();   // 4
float RadiusRadio();   // 10
float RadiusTab();     // 6
float RadiusSliderThumb(); // 8
float RadiusSliderTrack(); // 2

// --- Borderline widths ---
float BorderCard();    // 1.5
float BorderInput();   // 1

// --- Spacing ---
float PadCard();       // 16
float ColumnGap();     // 15
float RowGap();        // 15

// --- Button ---
float ButtonHeight();    // 40
float ButtonMinWidth();  // 140
float ButtonCharWidth(); // 9  (glyph advance estimate at the button font)
float ButtonLabelPad();  // 8

// Scale an A2UI-declared logical length to device pixels by the dp factor, so inline
// width/height/padding in a message track display density (and high-DPI capture).
float Dp(float logical);
} // namespace Metrics

} // namespace A2ui
