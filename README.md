# OpenTraceCLI

OpenTraceCLI is a command-line interface for the OpenTrace ecosystem, providing access to signal analysis and protocol decoding capabilities.

## Versioning

This project follows semantic versioning starting from version 0.1.0. The ABI (Application Binary Interface) is tied to the minor version number, meaning:

- **Major version** (0.x.x): Breaking API/ABI changes
- **Minor version** (x.1.x): ABI changes, new features  
- **Patch version** (x.x.1): Bug fixes, no ABI changes

Current version: **0.1.0**

## Building

```bash
meson setup build --buildtype=release
meson compile -C build
meson install -C build
```

## Dependencies

- OpenTraceCapture library
- GLib >= 2.32.0
- Meson >= 0.56.0

## Usage

```bash
opentrace-cli --help
```
