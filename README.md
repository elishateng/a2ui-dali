# a2ui-dali

A [DALi](https://github.com/dalihub)-based renderer for the
[A2UI](https://a2ui.org) protocol — the open generative-UI specification that lets an
agent describe a user interface as a stream of JSON messages.

`a2ui-dali` turns an A2UI message stream into native DALi views. You feed it messages as
they arrive from the agent; it builds and incrementally updates the UI, raises an event
with the root view for each surface, and reports user actions (button taps, form input)
back to you. It is a self-contained C++ library with no UI toolkit dependency beyond
DALi itself.

> ## ⚠️ DALi compatibility
>
> `dali-ui` is under active development and its UI / visual APIs change frequently.
> This revision of `a2ui-dali` is built against:
>
> | Module | Version |
> |--------|---------|
> | `dali2-core`, `dali2-adaptor` | **2.5.24** (`devel/master`) |
> | `dali2-ui-foundation`, `dali2-ui-components` | **`dali-ui` `devel`** — current typed-visual API (`Ui::VisualType`, `Ui::ImageView`, by-value signal slots) |
>
> Building against an older `dali-ui` that still exposes `Ui::Visual::Property::TYPE`
> or `Dali::FittingMode` will not compile — track a `dali-ui` revision from the same period.

## Highlights

- **One-call integration** — the `A2uiHost` facade owns the surface registry, message
  parser, and renderer. Subscribe to three callbacks and call `JsonFeed()`.
- **Multi-surface** — messages are routed by `surfaceId`; several surfaces render and
  update independently.
- **Flexible feed** — `JsonFeed()` accepts a JSON array of messages, newline-delimited
  JSONL, or a single object.
- **Full v0.9 catalog** — layout, text, media, inputs, lists, tabs, and a modal, with
  two-way data binding, expression/template evaluation, and form validation (`checks`).
- **Themable** — colors resolve through semantic tokens and dimensions use
  density-independent `dp` units, so output tracks the platform theme.
- **Distributable** — installs a static library, public headers, and a `pkg-config` file.

## Quickstart

### 1. Build

`a2ui-dali` builds against a DALi 2.x desktop environment
([dali-env](https://github.com/dalihub/dali-env)) with plain CMake:

```bash
# Source the DALi build environment (provides DESKTOP_PREFIX, PKG_CONFIG_PATH,
# and the dali2-* pkg-config entries).
. /path/to/dali-env/setenv

cmake -S . -B build
cmake --build build -j$(nproc)
```

### 2. Run the examples

Render a single A2UI stream:

```bash
./bin/a2ui-basic-renderer examples/basic-renderer/sample.jsonl
# or a sample stream (JSON array of messages):
./bin/a2ui-basic-renderer examples/samples/login-form.json
```

Or browse the whole catalog — the **gallery demo** shows one example per page; press
**Space / →** for the next, **←** for the previous, **Esc** to quit:

```bash
./bin/a2ui-gallery-demo            # defaults to examples/gallery-demo/screens
```

### 3. Integrate into your app

The whole integration is: create a host, point it at your resource directory, subscribe
to the three callbacks, and feed messages.

```cpp
#include "renderer/a2ui-host.h"

A2ui::A2uiHost host;
host.GetRenderer().SetImageDir("res/");           // where icons/images live

// The host renders each surface and hands you its root view — add it to your layout.
host.SetOnBeginRenderingSurface([&](const std::string& surfaceId, Dali::Ui::View root) {
  layout.Add(root);                                // (replace any prior view for surfaceId)
});
host.SetOnDeleteSurface([&](const std::string& surfaceId) {
  /* remove the surface's view from your layout */
});
host.SetOnUserAction([&](const std::string& surfaceId, const std::string& actionJson) {
  transport.Send(actionJson);                      // deliver the action back to the agent
});

// Feed messages as they arrive (a JSON array, JSONL, or a single object all work).
host.JsonFeed(message);
```

That is the entire surface API. The host never parents views into a window itself — you
decide where each surface's root view goes, which keeps `a2ui-dali` independent of your
app's window and navigation model.

## Feeding messages

`JsonFeed(const std::string&)` accepts any of these shapes — it splits the input into
top-level message objects by scanning brace depth, so formatting and newlines don't
matter:

```jsonc
// (a) a JSON array of messages
[ {"version":"v0.9","createSurface":{"surfaceId":"s","catalogId":"basic"}},
  {"version":"v0.9","updateComponents":{"surfaceId":"s","components":[ ... ]}} ]

// (b) newline-delimited JSONL — one message per line
{"version":"v0.9","createSurface":{"surfaceId":"s","catalogId":"basic"}}
{"version":"v0.9","updateComponents":{"surfaceId":"s","components":[ ... ]}}

// (c) a single message object
{"version":"v0.9","updateDataModel":{"surfaceId":"s","path":"/name","value":"Ada"}}
```

`JsonFeedFile(path)` reads an entire file and feeds it the same way.

### Message types (A2UI v0.9)

| Message            | Effect                                                              |
|--------------------|--------------------------------------------------------------------|
| `createSurface`    | Registers a surface; carries `catalogId`, `sendDataModel`, `sourceApp`, and a `theme` (`width`, `height`, `pattern`). |
| `updateComponents` | Sets/updates the surface's component tree; triggers a render.       |
| `updateDataModel`  | Writes a value at a JSON-pointer path; bound views update in place. |
| `deleteSurface`    | Removes the surface and raises `OnDeleteSurface`.                   |

A surface renders once its component tree contains a `root` component. Subsequent
`updateDataModel` messages update the existing views through data bindings rather than
re-rendering.

## Surfaces

Surfaces are keyed by `surfaceId` and created on demand, so a single host can drive
several independent regions at once (a main panel, a sidebar, a notification, …). Each
surface tracks its own component tree, data model, preferred size, pattern, and source
app. `OnBeginRenderingSurface` / `OnDeleteSurface` fire per surface, with the `surfaceId`
so you can place or remove each one in the right slot.

## User actions

When a component with an `action` is activated (e.g. a button tap), the host serializes
the action — including the `surfaceId`, source component id, and any bound context — and
delivers it through `OnUserAction`. Forward that JSON to your agent over whatever
transport you use; `a2ui-dali` itself is transport-agnostic.

## Component coverage

The standard A2UI v0.9 catalog is mapped onto DALi UI. Components DALi has no native
widget for are composed from views (a faithful look) rather than stubbed.

| A2UI component | DALi mapping                                          |
|----------------|------------------------------------------------------|
| Text           | `Label` (variant → font size/weight)                 |
| Image          | `ImageView` (responsive; `avatar` → circular mask)   |
| Icon           | `ImageView` + tint (bundled icon set)                |
| Divider        | `View`                                               |
| Row / Column   | `FlexLayout` (direction, justify, align, gap, weight)|
| List           | `FlexLayout` + `ScrollView` (templated via data path)|
| Card           | `View` + background + corner radius                  |
| Tabs           | `FlexLayout` + tab bar                               |
| Button         | `FlexLayout` + tap detector (pill, variant colors)   |
| TextField      | `InputField` (outlined; obscured / validation)       |
| CheckBox       | `View` + selectable trait                            |
| ChoicePicker   | `FlexLayout` chip group                              |
| Slider         | view-composition (track + fill + thumb)              |
| ProgressBar    | view-composition (track + fill)                      |
| DateTimeInput  | `InputField` / display                               |
| Modal          | view-composition (trigger reveals content)           |
| Video          | view-composition placeholder (poster + controls)     |
| AudioPlayer    | view-composition placeholder (art + transport)       |

Two-way data binding, `${...}` expression evaluation, list templating, and form
validation (`checks`: `required`, `email`, `length`, `and`, …) are supported across the
catalog.

## Extending the catalog (custom components)

The renderer dispatches each component through a **registry** — a map from
`componentType` to a handler function. The standard v0.9 catalog above is just the set of
handlers registered at construction. A *custom catalog* is therefore the standard catalog
**plus your own handlers registered onto the same renderer** — there is no renderer
subclass to write, matching the A2UI model where a catalog is a negotiated set of
component types rather than a code artifact.

A handler is a plain function `View(const ComponentModel&, RenderContext&)`. The
`RenderContext` gives it the same services the built-in handlers use — data binding
(`ResolveString` / `ResolveFloat` / `GetBoundPath`), the action dispatcher, and
`RenderChild(id)` to recurse into children — so a custom component is a first-class
citizen, not a second-class stub.

```cpp
// A custom "Badge" component — a coloured pill whose text can be a literal or a binding.
Dali::Ui::View RenderBadge(const A2ui::ComponentModel& comp, A2ui::RenderContext& rc) {
  std::string label = rc.ResolveString(comp.rawNode->Find("label"));
  // ... build and return a DALi view ...
}

host.GetRenderer().RegisterComponent("Badge", &RenderBadge);   // now "Badge" is in the catalog
```

After that, any `{"component":"Badge", ...}` node in an incoming surface renders through
your handler, interleaved with the standard components. See
[`examples/custom-component/`](examples/custom-component/main.cpp) for a complete, runnable
app. `catalogId` on `createSurface` stays a negotiation identifier — the renderer renders
whatever component types are registered, so a custom `catalogId` needs no special casing.

## Theming

`a2ui-dali` does not hard-code colors or sizes:

- **Colors** resolve through semantic tokens (e.g. `Primary`, `SurfaceContainerLow`,
  `OnSurfaceContainerHighest`) via the platform color manager, with a bundled palette as
  a fallback. The UI follows the platform theme where one is provided.
- **Dimensions** (icon/avatar sizes, font sizes, radii, spacing) use density-independent
  `dp` units, so layouts scale correctly across display densities.

## Install

To consume `a2ui-dali` from another project, install it to a prefix:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/your/prefix
cmake --build build -j$(nproc)
cmake --install build
```

This lays down `lib/liba2ui-dali.a`, the public headers under
`include/a2ui-dali/{core,renderer}/`, and `lib/pkgconfig/a2ui-dali.pc`. A consuming build
then just uses `pkg-config`:

```bash
pkg-config --cflags --libs a2ui-dali
# -> -I<prefix>/include/a2ui-dali ... -la2ui-dali -ldali2-... 
```

```cpp
#include "renderer/a2ui-host.h"
```

## Architecture

```
        message stream                         your DALi layout
              │                                       ▲
              ▼                            OnBeginRenderingSurface(id, view)
        ┌───────────┐                                 │
        │  A2uiHost │  ── routes by surfaceId ──▶ ┌────────────┐
        │  (facade) │                             │ A2uiRenderer│ ── components → DALi views,
        └─────┬─────┘ ◀── OnUserAction(id, json) ─┤ (+ bindings,│    data binding, diffing
              │                                    │  templates) │
              ▼                                    └────────────┘
   ┌────────────────────┐      ┌────────────────────────┐
   │ A2uiMessageProcessor│ ──▶ │  SurfaceGroupModel      │  one SurfaceModel
   │ (parse + dispatch)  │     │  (surfaces by id)       │  (components + data) per surface
   └────────────────────┘      └────────────────────────┘
```

| Module      | Responsibility                                                                          |
|-------------|-----------------------------------------------------------------------------------------|
| `A2uiHost`  | App-facing facade: feed messages in, get views and actions out, route by surface        |
| `core/`     | Message parsing, surface/data models, expression evaluation, diff engine, template cache|
| `renderer/` | Component-handler **registry** (one handler per component type, in `renderer/components/`), theming, accessibility bridge, view pool — extend via `RegisterComponent` |

## Examples

| Path                          | What it shows                                              |
|-------------------------------|-----------------------------------------------------------|
| `examples/basic-renderer/`    | Minimal end-to-end app built on `A2uiHost`                |
| `examples/gallery-demo/`      | Runnable catalog browser — pages through 14 example screens with the keyboard |
| `examples/custom-component/`  | Registers an app-specific `Badge` component via `RegisterComponent` — the custom-catalog extension point |
| `examples/samples/`           | Ready-to-run v0.9 message streams (login form, music player, account balance) |
| `examples/a2a-integration/`   | Wiring `JsonFeed` / actions to an [A2A](https://a2a-protocol.org) transport |

## Conformance

`a2ui-conformance-test` exercises the parser and model layers against the JSONL fixtures
in `test/` and needs no DALi UI initialization, so it is suitable for headless CI:

```bash
./bin/a2ui-conformance-test test/
```

## Contributing

Issues and pull requests are welcome. Please keep changes aligned with the published
[A2UI specification](https://a2ui.org). Application-specific component types are a
supported extension point — register them with `RegisterComponent` (see
[Extending the catalog](#extending-the-catalog-custom-components)) rather than forking the
standard catalog handlers.

## License

Apache License 2.0. See [LICENSE](LICENSE).
