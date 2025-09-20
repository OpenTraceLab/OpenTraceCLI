# OpenTraceCLI Transformation Summary

This document summarizes the transformation of sigrok-cli to OpenTraceCLI.

## What Was Done

### 1. Repository Setup
- Cloned sigrok-cli master branch and latest release (0.7.2)
- Removed connection to upstream repository
- Created two branches: `master` and `release-0.7.2`

### 2. Naming Changes
- **Project name**: `sigrok-cli` → `OpenTraceCLI`
- **Binary name**: `sigrok-cli` → `opentrace-cli`
- **Header file**: `sigrok-cli.h` → `opentrace-cli.h`
- **Library references**: `libsigrok` → `libopentrace`, `libsigrokdecode` → `libopentracedecode`

### 3. Code Transformations
- **Prefixes**: All `SR_` → `OTC_`, `sr_` → `otc_`
- **Constants**: Updated `SIGROK_CLI` → `OPENTRACE_CLI`
- **File formats**: `srzip` → `otczip`
- **Include guards**: Updated to use `OPENTRACE_CLI` namespace

### 4. File Renames
- `sigrok-cli.h` → `opentrace-cli.h`
- `doc/sigrok-cli.1` → `doc/opentrace-cli.1`
- `contrib/sigrok-cli.svg` → `contrib/opentrace-cli.svg`
- `contrib/org.sigrok.sigrok-cli.desktop` → `contrib/org.opentrace.opentrace-cli.desktop`
- `contrib/sigrok-logo-notext.ico` → `contrib/opentrace-logo-notext.ico`

### 5. Build System Migration
- **Removed**: autotools (configure.ac, Makefile.am, autogen.sh, m4/)
- **Added**: meson build system (meson.build)
- **Created**: build.sh script for easy building with ninja
- **Updated**: .gitignore for meson build artifacts

### 6. Copyright and Attribution
- Updated copyright headers to reference "OpenTraceLab Contributors"
- Preserved original copyright attribution
- Maintained GPL-3.0-or-later license compliance
- Added "Originally derived from sigrok-cli project" attribution

### 7. Documentation Updates
- Updated README with new project information
- Updated AUTHORS file with proper attribution
- Updated man page references
- Updated desktop file for proper application registration

## Build Instructions

### Prerequisites
- meson (>= 0.56.0)
- ninja
- libopentrace (>= 0.6.0)
- libopentracedecode (>= 0.6.0, optional)
- glib-2.0 (>= 2.32.0)

### Building
```bash
./build.sh
```

Or manually:
```bash
meson setup build
ninja -C build
```

### Installing
```bash
ninja -C build install
```

## Branches
- `master`: Latest development version based on sigrok-cli master
- `release-0.7.2`: Stable version based on sigrok-cli 0.7.2

## License Compliance
This transformation maintains full GPL compliance by:
- Preserving original copyright notices
- Adding clear attribution to the original sigrok project
- Maintaining the GPL-3.0-or-later license
- Documenting the derivation in all modified files

## Dependencies Note
This transformation assumes the existence of corresponding OpenTrace libraries:
- `libopentrace` (equivalent to libsigrok)
- `libopentracedecode` (equivalent to libsigrokdecode)

These libraries would need to be available for the project to build successfully.
