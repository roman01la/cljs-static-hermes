#pragma once

#include <hermes/VM/static_h.h>
#include <hermes/hermes.h>
#include <jsi/jsi.h>

/// Initialize WebSocket support and install the WebSocket constructor into the JS runtime.
void initializeWebSocketSupport(facebook::hermes::HermesRuntime &runtime);

/// Pump libwebsockets once, processing pending network events.
void pumpWebSocketSupport();

/// Shutdown WebSocket support and release associated resources.
void shutdownWebSocketSupport();
