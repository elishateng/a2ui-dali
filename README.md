# a2ui-dali

A [DALi](https://github.com/dalihub)-based renderer for the
[A2UI](https://a2ui.dev) protocol — the open generative UI specification.

`a2ui-dali` lets a DALi/Tizen application receive A2UI v0.9 messages from
an agent (typically over the [A2A](https://a2a-protocol.org) protocol) and
render them as native DALi views in real time.

## Status

Early preview. The standard A2UI v0.9 catalog is mapped to DALi UI
components; the surface, data, action, and template-cache models are
implemented. APIs may evolve before a 1.0 release.

## Component coverage

The renderer maps the standard A2UI v0.9 catalog onto DALi UI components.

| A2UI component | DALi mapping                          |
|----------------|---------------------------------------|
| Text           | `Label`                               |
| Image          | `View` + image visual                 |
| Icon           | `View` + image visual                 |
| Divider        | `View`                                |
| Row            | `FlexLayout` (row)                    |
| Column         | `FlexLayout` (column)                 |
| List           | `FlexLayout` + `ScrollView`           |
| Card           | `View` + background + corner radius   |
| Tabs           | `FlexLayout` + tab bar                |
| Modal          | `View` + overlay layer                |
| Button         | `FlexLayout` + tap detector           |
| TextField      | `InputField`                          |
| CheckBox       | `View` + selectable trait             |
| ChoicePicker   | `FlexLayout` chip group               |
| DateTimeInput  | `InputField` + validation             |
| Video          | not yet implemented                   |
| AudioPlayer    | not yet implemented                   |
| Slider         | not yet implemented                   |

## Build

`a2ui-dali` is built and tested against the DALi 2.x desktop environment
([dali-env](https://github.com/dalihub/dali-env)). Once the environment is
sourced, the project builds with plain CMake:

```bash
# 1. Source the DALi build environment (provides DESKTOP_PREFIX,
#    PKG_CONFIG_PATH, and the dali2-* pkg-config entries used below).
. /path/to/dali-env/setenv

# 2. Configure and build
cmake -S . -B build
cmake --build build -j$(nproc)
```

This produces three artifacts:

| Artifact                | Purpose                                |
|-------------------------|----------------------------------------|
| `libaa2ui-dali.a`       | Static renderer library                |
| `bin/a2ui-conformance-test` | Spec conformance test (no GUI)     |
| `bin/a2ui-basic-renderer`   | Minimal end-to-end example         |

### Dependencies

| pkg-config name      | Source                                            |
|----------------------|---------------------------------------------------|
| `dali2-core`         | <https://github.com/dalihub/dali-core>            |
| `dali2-adaptor`      | <https://github.com/dalihub/dali-adaptor>         |
| `dali2-ui-foundation`| <https://github.com/dalihub/dali-ui>              |
| `dali2-ui-components`| <https://github.com/dalihub/dali-ui>              |
| `libcurl`            | system                                            |

## Quickstart

The example loads an A2UI v0.9 JSONL stream from a file and renders it
into a 720x1080 window:

```bash
./bin/a2ui-basic-renderer examples/basic-renderer/sample.jsonl
```

`sample.jsonl` contains a minimal three-message stream — `createSurface`,
`updateComponents`, `updateDataModel` — that draws a card with a heading,
data-bound subtitle, and a tappable button.

To integrate `a2ui-dali` into your own DALi application, link against the
static library and feed messages to `A2uiMessageProcessor`:

```cpp
#include "core/a2ui-message-processor.h"
#include "core/surface-model.h"
#include "renderer/a2ui-renderer.h"

A2ui::A2uiMessageProcessor processor;
A2ui::SurfaceModel         surface;
A2ui::A2uiRenderer         renderer;

// Feed JSONL lines as they arrive from the agent.
processor.ProcessLine(jsonlLine, surface);

// After each batch of updates, ask the renderer for the latest View tree.
Dali::Ui::View view = renderer.Render(surface);
window.Add(view);
```

For live agents, `src/transport/a2a-transport.h` provides a JSON-RPC +
SSE transport that speaks the A2A protocol.

## Architecture

```
         JSONL stream                       DALi window
              │                                  ▲
              ▼                                  │
   ┌──────────────────────┐         ┌────────────────────────┐
   │ A2uiMessageProcessor │ ─────▶  │      A2uiRenderer      │
   │  (parses messages,   │         │  (maps components to   │
   │   updates models)    │         │   DALi views, applies  │
   └──────────┬───────────┘         │   data binding, diffs) │
              │                     └────────────────────────┘
              ▼                                  ▲
      ┌───────────────┐                 ┌────────┴────────┐
      │  SurfaceModel │ ──── reads ────▶│  ExpressionParser│
      │  + DataModel  │                 │  + ActionDispatch│
      └───────────────┘                 └─────────────────┘
```

| Module                 | Responsibility                                |
|------------------------|-----------------------------------------------|
| `core/`                | Message parsing, surface/data models, expression evaluation, diff engine, template cache |
| `renderer/`            | A2UI component → DALi view mapping, accessibility bridge, view pool |
| `transport/`           | A2A (JSON-RPC + SSE) client                   |

## Conformance

`a2ui-conformance-test` runs `test/conformance-test.cpp` against the
JSONL fixtures in `test/` to verify that the spec-mandated message shapes
parse and apply correctly:

```bash
./bin/a2ui-conformance-test test/
```

The test does not require DALi UI to be initialised — it exercises the
parser and model layers only — so it is suitable for CI on headless hosts.

## Contributing

Issues and pull requests are welcome. Please keep changes aligned with
the published [A2UI specification](https://a2ui.dev); custom catalogs and
DALi-specific extensions belong on a separate branch.

## License

Apache License 2.0. See [LICENSE](LICENSE).
