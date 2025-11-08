_This project demonstrates use of ClojureScript and UIx, together with React and a custom reconciler to drive a native window with ImGui via AOT compiled JavaScript with Static Hermes._  

- Based off https://github.com/tmikov/imgui-react-runtime
- Read more about [Static Hermes and imgui-react-runtime here](imgui-react-runtime/README.md)

Differences from _tmikov/imgui-react-runtime_:

- Bundles [UIx](https://github.com/pitch-io/uix) app written in ClojureScript 
- Exposes `WebSocket` interface to JS env via [libwebsockets](https://github.com/warmcat/libwebsockets), needed for REPL connection in dev
- Supports hot-reloading cljs code in dev, via custom REPL client runtime

## Development

_native dependencies + interpreted javascript for interactive development_

1. Install NPM dependencies `npm i`
1. Run cljs dev build `clojure -M -m shadow.cljs.devtools.cli watch app`
1. `cd imgui-react-runtime`
1. Configure hermes dev build `cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -G Ninja`
1. Build `cmake --build cmake-build-debug --target uix`
1. Run the binary `./cmake-build-debug/examples/uix/uix`
1. Make changes in cljs files to trigger hot-reload or/and connect to REPL server (run `(shadow/repl :app)` from Clojure REPL to hook into JavaScript env)

## Release build

_native dependencies + native AOT compiled javascript for maximum performance_

1. Build cljs code `clojure -M -m shadow.cljs.devtools.cli release app`
1. `cd imgui-react-runtime`
1. Configure hermes release build `cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -G Ninja`
1. Build `cmake --build cmake-build-release --target uix`
1. Run the binary `./cmake-build-release/examples/uix/uix`

Output binary: 8MB
- 3MB Hermes VM
- 1.8MB React library
- 3.2MB app/renderer/ImGui