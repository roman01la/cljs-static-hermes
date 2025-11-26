# Copilot Instructions for cljs-static-hermes

This repository demonstrates ClojureScript and UIx with React and a custom reconciler to drive a native window with ImGui via AOT compiled JavaScript with Static Hermes.

## Project Overview

- **Primary Language**: ClojureScript
- **UI Framework**: UIx (React-based library for ClojureScript)
- **Runtime**: Static Hermes (AOT compiled JavaScript)
- **Native UI**: ImGui with custom React reconciler
- **Build Tools**: shadow-cljs, CMake, Ninja

## Key Technologies

- **UIx**: React wrapper for ClojureScript - uses `$` macro for creating elements and `defui` for components
- **Static Hermes**: Facebook's JavaScript engine with ahead-of-time compilation
- **ImGui**: Immediate mode GUI library accessed through a custom React reconciler

## Development Commands

### ClojureScript Development
```bash
# Install dependencies
npm install

# Watch mode for development (starts REPL)
clojure -M -m shadow.cljs.devtools.cli watch app

# Release build
clojure -M -m shadow.cljs.devtools.cli release app
```

### Native Build (after ClojureScript build)
```bash
cd imgui-react-runtime

# Debug build
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build cmake-build-debug --target uix

# Release build
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build cmake-build-release --target uix
```

## Code Style and Conventions

### ClojureScript/UIx Patterns

- Use `defui` macro for defining React components
- Use `$` macro for creating elements: `($ :text "Hello")`
- Use `:<>` for React fragments: `($ :<> ...children)`
- Prefer functional components with hooks (`uix/use-state`, `uix/use-effect`, `uix/use-ref`)
- Component props are passed as maps: `($ my-component {:prop1 value1})`

### Available ImGui Elements

The project uses ImGui elements through a custom React reconciler:

**Layout & Containers:**
- `:window` - Main window container with title, position, and size props
- `:child` - Scrollable child region
- `:group`, `:indent` - Layout grouping
- `:sameline` - Place next element on same line

**Widgets:**
- `:button` - Clickable button with `on-click` handler
- `:text` - Text display with optional color
- `:separator` - Horizontal separator line
- `:collapsingheader` - Collapsible section header

**Tables:**
- `:table`, `:tablerow`, `:tablecell`, `:tablecolumn`, `:tableheader`

**Drawing Primitives:**
- `:rect` - Rectangle with position, size, color, and fill options
- `:circle` - Circle with position, radius, color, and segments

**Custom Widgets:**
- `:radialmenu` - Custom radial menu widget

### File Structure

- `src/hermes/app.cljs` - Main application with UI components
- `src/hermes/client.cljs` - Client setup and registration
- `src/hermes/repl/` - REPL client runtime for hot-reloading
- `imgui-react-runtime/` - Native runtime submodule

## Best Practices

1. **Hot Reloading**: The project supports hot-reloading via shadow-cljs and react-refresh
2. **State Management**: Use UIx hooks for local state, component state is preserved during hot-reload
3. **Performance**: Release builds use AOT compilation with Static Hermes for maximum performance
4. **Testing**: Run the native binary to visually verify UI changes

## Dependencies

- Clojure dependencies defined in `deps.edn`
- JavaScript dependencies defined in `package.json`
- Native dependencies managed through CMake in `imgui-react-runtime/`
