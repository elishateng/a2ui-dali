# a2a-integration

A reference implementation of an [A2A](https://a2a-protocol.org)
(Agent-to-Agent, JSON-RPC + SSE) client that delivers A2UI v0.9 streams
to the `a2ui-dali` renderer.

This module sits in `examples/` rather than the library itself because
A2A is *one* possible transport for A2UI messages — not part of the A2UI
specification. Drop the two files into your application alongside the
library if you want a ready-made A2A client; pick a different transport
(WebSocket, gRPC, in-process) and the same renderer API still works.

## Files

| File                | Purpose                                              |
|---------------------|------------------------------------------------------|
| `a2a-transport.h`   | `A2aTransportAdapter` declaration                    |
| `a2a-transport.cpp` | libcurl-based JSON-RPC + SSE implementation          |

Built against `libcurl`. No additional `a2ui-dali` headers are required.

## Wiring

```cpp
#include "core/a2ui-message-processor.h"
#include "core/surface-model.h"
#include "renderer/a2ui-renderer.h"
#include "a2a-transport.h"   // copied from examples/a2a-integration/

A2ui::A2uiMessageProcessor processor;
A2ui::SurfaceModel         surface;
A2ui::A2uiRenderer         renderer;
A2ui::A2aTransportAdapter  transport;

transport.SetServerUrl("http://localhost:10003");
transport.SetA2uiVersion("0.9");

// Each SSE chunk is a JSONL line — feed it straight to the processor.
transport.SetOnChunk([&](const std::string& line) {
  processor.ProcessLine(line, surface);
  // Re-render whenever you've applied a meaningful batch of updates.
  // (In production, debounce or render on a frame tick.)
});

// Forward client-side actions (button taps, form submits) back to the agent.
renderer.GetActionDispatcher().SetSendCallback(
  [&](const std::string& actionJson) { transport.SendAction(actionJson); });

transport.SendMessage("Show me the weather in Seoul");
```

## Server expectations

`A2aTransportAdapter` speaks the standard A2A wire format:

- `POST /` with JSON-RPC method `message/stream` for the user's prompt
- The server responds with `text/event-stream`; each `data:` event carries
  one A2UI JSONL line in `parts[].data`
- `POST /` with `message/send` for client actions (e.g. button taps)

Any A2A-compliant agent server should work. The
[A2A reference servers](https://github.com/a2aproject) (Python ADK,
TypeScript SDK) are good starting points.
