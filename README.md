# Qt 6.8.3 for macOS 10.9 Mavericks

Drop-in replacement for [Qt 6.8.3](https://github.com/qt/qtbase/tree/v6.8.3) with macOS 10.9 (Mavericks) support. Works on all macOS versions from 10.9 to the latest.

## Download

Grab the latest build from [Releases](https://github.com/mavericksforever/qtbase/releases).

- **qt6-mavericks-x86_64.tar.gz** — Qt frameworks and plugins (x86_64, deployment target 10.9)
- **qt6-host-arm64.tar.gz** — Host tools (moc, rcc, uic) for cross-compilation from Apple Silicon

## Usage

```bash
# Extract
tar xzf qt6-mavericks-x86_64.tar.gz
tar xzf qt6-host-arm64.tar.gz

# Configure your project
cmake .. \
  -DCMAKE_PREFIX_PATH=$PWD/qt6-mavericks \
  -DQT_HOST_PATH=$PWD/qt6-host \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9
```

## Included modules

| Module | Description |
|--------|-------------|
| QtBase | Core, Gui, Widgets, Network, DBus, OpenGL, PrintSupport |
| QtSvg | SVG rendering |
| QtImageFormats | WebP, TIFF, JP2, HEIF |
| QtNetworkAuth | OAuth support |

## Building from source

### Prerequisites

- macOS with Xcode (Apple Silicon or Intel)
- CMake 3.22+
- Ninja (recommended) or Make
- [macports-legacy-support](https://github.com/macports/macports-legacy-support) (built for x86_64)

### Build

```bash
# 1. Build macports-legacy-support
git clone https://github.com/macports/macports-legacy-support.git
cd macports-legacy-support
make PREFIX=/path/to/legacy \
  CFLAGS="-arch x86_64 -mmacosx-version-min=10.9" \
  LDFLAGS="-arch x86_64 -mmacosx-version-min=10.9" \
  install

# 2. Build host Qt (runs on your Mac)
cd qtbase
mkdir build-host && cd build-host
cmake .. -GNinja -DCMAKE_INSTALL_PREFIX=/path/to/qt6-host -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
cmake --build . --parallel && cmake --install .

# 3. Build target Qt (runs on 10.9)
cd .. && mkdir build && cd build
cmake .. -GNinja \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
  -DCMAKE_INSTALL_PREFIX=/path/to/qt6-mavericks \
  -DQT_HOST_PATH=/path/to/qt6-host
cmake --build . --parallel && cmake --install .
```

## What was changed

All changes are backward-compatible — this Qt works on macOS 10.9 through the latest macOS.

### Build system
- Lowered minimum macOS deployment target to 10.9
- Auto weak-link Metal and UniformTypeIdentifiers frameworks when targeting < 10.11
- Disabled ObjC runtime stubs (`-fno-objc-msgsend-selector-stubs`) for < 10.14
- Disabled aligned allocation (`-fno-aligned-allocation`) for < 10.13
- Link with [macports-legacy-support](https://github.com/macports/macports-legacy-support) for POSIX polyfills

### Runtime polyfills (src/corelib/global/)
- `__availability_version_check` — enables `@available()` on < 10.15
- Aligned `new`/`delete` — C++17 aligned allocation for < 10.13
- `std::bad_optional_access` / `std::bad_variant_access` vtable/typeinfo — C++17 exceptions for < 10.14
- `__ulock_wait2` / `__ulock_wake` — libc++ mutex primitives
- `renameatx_np` stub

### API compatibility guards
- NSAppearance / Dark Mode APIs (10.14+) — `@available` guards with fallbacks
- NSColor APIs (10.10+) — fallback to legacy color methods
- UniformTypeIdentifiers (11.0+) — `@available` guards with legacy UTI string fallbacks
- NSGraphicsContext (10.10+) — `graphicsContextWithGraphicsPort` fallback
- IOKit `kIOMainPortDefault` → `kIOMasterPortDefault`
- NSAlert `setControlSize:` — runtime category for NSButton on 10.9
- Various NSWindow/NSView APIs — `@available` and `respondsToSelector` guards
- `NSEvent.deviceID` — `@try/@catch` wrapper for 10.9 compatibility
- CoreText font weight constants — hardcoded values replacing 10.11+ symbols
- Qt `qt_mac_toQBrush` / `qt_mac_toQColor` — safe fallbacks for < 10.13

## License

Same as Qt — LGPL-3.0 / GPL-2.0 / GPL-3.0 / Commercial.
