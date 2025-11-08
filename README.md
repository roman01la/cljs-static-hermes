## Development

1. Install NPM dependencies `npm i`
1. Run cljs dev build `clojure -M -m shadow.cljs.devtools.cli watch app`
1. `cd imgui-react-runtime`
1. Configure hermes dev build `cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -G Ninja`
1. Build `cmake --build cmake-build-debug --target uix`
1. Run the binary `./cmake-build-debug/examples/uix/uix`

## Release build
1. Build cljs code `clojure -M -m shadow.cljs.devtools.cli release app`
1. `cd imgui-react-runtime`
1. Configure hermes release build `cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -G Ninja`
1. Build `cmake --build cmake-build-release --target uix`
1. Run the binary `./cmake-build-release/examples/uix/uix`