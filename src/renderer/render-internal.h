#pragma once

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0.
 *
 * Internal implementation header shared by the per-component renderer files
 * (renderer/components/*.cpp). It bundles the DALi + renderer includes and the usings
 * each A2uiRenderer::Render* definition needs, so each component file stays small.
 * Internal only — do not include from public headers.
 */

#include "renderer/a2ui-renderer.h"
#include "renderer/render-style.h"
#include "renderer/a2ui-theme.h"
#include "renderer/a2ui-metrics.h"
#include "renderer/a11y-bridge.h"

#include <dali/integration-api/debug.h>
#include <dali/public-api/events/tap-gesture-detector.h>
#include <dali/public-api/events/touch-event.h>
#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/image-view.h>
#include <dali-ui-foundation/public-api/layouts/flex-layout-params.h>
#include <dali-ui-foundation/public-api/scroll-view.h>
#include <dali-ui-foundation/public-api/text/text-enumerations.h>
#include <dali-ui-foundation/public-api/text/style/underline.h>

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>

using namespace Dali;       // intentional in this internal-only implementation header
using namespace Dali::Ui;
using Dali::Ui::TreeNode;
