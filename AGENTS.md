# AGENTS.md - Onboarding Guide for Net-Tex-Demo Repository

## Repository Overview

This repository is an AlloLib-kickstart project, providing a plug-n-play build environment for AlloLib-based applications. AlloLib is a C++ library for creative coding, real-time interactive multimedia, and audio-visual applications.

The current project demonstrates basic usage of the AlloApp framework with graphics and audio callbacks, configurable for both desktop and Allosphere environments.

## Project Structure

- `src/main.cpp` - Main application entry point implementing a basic AlloApp
- `allolib/` - Core AlloLib library and dependencies
- `al_ext/` - AlloLib extensions (assets, OpenVR, soundfile, spatial audio, etc.)
- `examples/` - Various example applications
- `build/` - Build artifacts (generated)
- `bin/` - Executables (generated)

## Setup Instructions

1. Clone the repository
2. Install [AlloLib dependencies](https://github.com/AlloSphere-Research-Group/allolib/blob/main/readme.md)
3. Run `./init.sh` in a Bash shell to initialize the environment
4. Use `./run.sh` to build and run the project

## Development Workflow

### Critical: Build and Test After Every Change

**MANDATORY**: After making ANY changes to source code, documentation, or configuration files, you MUST run `./run.sh` to build and test the changes.

The `./run.sh` script performs the following:
1. Configures the build using CMake with Release settings
2. Builds the project in parallel
3. If successful, executes the resulting binary (`./bin/Allolib-Kickstart`)

### Why This Is Required

- Ensures all changes compile successfully
- Verifies the application runs without runtime errors
- Catches integration issues early
- Maintains build stability across changes

### When to Run `./run.sh`

- After modifying `src/main.cpp` or any source files
- After updating CMakeLists.txt files
- After adding new dependencies or assets
- After configuration changes
- Before committing changes
- When troubleshooting build issues

## Build System

- **Build Tool**: CMake
- **Build Type**: Release (optimized)
- **Output Directory**: `build/Release/`
- **Executable**: `bin/Allolib-Kickstart`
- **Parallel Builds**: Enabled with 9 jobs

## Key Configuration

The application supports two modes via the `DESKTOP` macro in `main.cpp`:

- **Desktop Mode**: Stereo audio, basic spatialization
- **Allosphere Mode**: 60-channel audio, DBAP spatialization

## Dependencies

- AlloLib core library
- GLFW (windowing)
- OpenGL (graphics)
- RtAudio/RtMidi (audio/MIDI)
- Various external libraries in `allolib/external/`

## Making Changes

1. Edit source files as needed
2. Run `./run.sh` to build and test
3. Verify the application runs correctly
4. Commit changes only after successful build/test

## Troubleshooting

- If build fails, check CMake output for errors
- Ensure all dependencies are installed
- Verify you're using a Bash-compatible shell
- Check that `./init.sh` was run successfully

## Contact

For questions about AlloLib or this project, refer to the [AlloLib repository](https://github.com/AlloSphere-Research-Group/allolib).