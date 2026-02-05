# NDI Video Pipeline - Developer Documentation

## Overview

This project implements a complete NDI (Network Device Interface) video streaming solution using allolib. It provides both sending and receiving capabilities for real-time video over IP networks.

## Architecture

### Core Components

#### 1. NDI Sender (`al_NDISender`)
- **Location**: `videoPipe/ndi_wrapping/include/al_ext/ndi/al_NDISender.hpp`
- **Purpose**: Send OpenGL textures over NDI network streams
- **Key Features**:
  - Hardware-accelerated GPU-to-CPU transfer using `glReadPixels()`
  - Automatic texture resizing
  - BGRA pixel format for OpenGL compatibility
  - Memory-managed pixel buffers

#### 2. NDI Receiver (`al_NDIReceiver`)
- **Location**: `videoPipe/ndi_wrapping/include/al_ext/ndi/al_NDIReceiver.hpp`
- **Purpose**: Receive NDI video streams and render to OpenGL textures
- **Key Features**:
  - Dynamic source discovery
  - Automatic texture resizing
  - BGRA to RGBA color space handling
  - Connection management

#### 3. AlloApps
- **NDISimpleTest**: Console-based NDI sender test
- **NDISimpleApp**: GUI-based NDI sender with animated patterns
- **NDIVideoReceiverApp**: GUI-based NDI receiver with source selection

## Build System

### CMake Configuration
```cmake
# Root CMakeLists.txt
set(NDI_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/ndi/NDI SDK for Apple" CACHE PATH "Path to NDI SDK")

# Add NDI executables
add_executable(NDISimpleTest videoPipe/NDISimpleTest.cpp)
add_executable(NDISimpleApp videoPipe/NDISimpleApp.cpp)
add_executable(NDIVideoReceiverApp videoPipe/NDIVideoReceiverApp.cpp)

# Link with NDI and allolib
target_link_libraries(NDISimpleTest PRIVATE al al_ndi)
```

### Dependencies
- **NDI SDK for Apple**: Network video streaming library
- **allolib**: Graphics and application framework
- **OpenGL**: GPU rendering and texture operations
- **CMake 3.29+**: Build system

## Critical Implementation Details

### Memory Management Bug Fix (Feb 2026)

**Problem**: Original NDI sender incorrectly set `p_data` to OpenGL texture ID instead of CPU pixel data, causing crashes.

**Solution**:
```cpp
// Allocate CPU pixel buffer
mHardwareCtx.pPixelData = new uint8_t[dataSize];
mHardwareCtx.videoFrame.p_data = mHardwareCtx.pPixelData;

// Transfer GPU → CPU
glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, mHardwareCtx.pPixelData);
```

### Texture Format Handling

**Sender**: OpenGL textures (RGBA) → BGRA pixel buffer → NDI
**Receiver**: NDI frames (BGRA) → OpenGL texture (RGBA)

### Performance Characteristics

- **Sender**: ~4MB transfer per frame (1024×768 RGBA)
- **Receiver**: Minimal CPU overhead, GPU texture uploads
- **Network**: Uncompressed video, local network recommended

## Usage Examples

### Basic NDI Sender
```cpp
#include "al_ext/ndi/al_NDISender.hpp"

NDISender sender;
NDISender::VideoConfig config(1920, 1080);
sender.init("MyStream", config, true);  // Enable hardware accel

// In render loop
sender.sendDirect(fbo.tex());
```

### Basic NDI Receiver
```cpp
#include "al_ext/ndi/al_NDIReceiver.hpp"

NDIReceiver receiver;
receiver.init();

// Discover sources
auto sources = receiver.getAvailableSources();

// Connect and receive
receiver.connect("SourceName");
Texture tex;
if (receiver.update(tex)) {
    // Render texture
}
```

### Complete AlloApp Receiver
```cpp
struct MyNDIApp : public App {
    NDIReceiver receiver;
    Texture videoTexture;

    void onCreate() {
        receiver.init();
        // Auto-connect to first source
        receiver.connect();
    }

    void onDraw(Graphics& g) {
        if (receiver.update(videoTexture)) {
            g.quad(videoTexture, -1, -1, 2, 2);
        }
    }
};
```

## Testing and Validation

### Test Applications
1. **NDISimpleTest**: Console sender, verify basic functionality
2. **NDIVideoReceiverApp**: GUI receiver, test source selection
3. **Cross-testing**: Run sender and receiver simultaneously

### NDI Monitoring Tools
- **NDI Studio Monitor** (recommended)
- **NDI Tools suite** (free download)
- **SDK examples** for basic testing

### Network Setup
- Ensure sender and receiver on same local network
- Check firewall settings
- Use wired Ethernet for best performance

## File Structure

```
videoPipe/
├── NDI-Notes.md              # Technical implementation notes
├── planning.md               # Development planning
├── DEVELOPER.md              # This file
├── NDISimpleTest.cpp         # Console NDI sender test
├── NDISimpleApp.cpp          # GUI NDI sender app
├── NDIVideoReceiverApp.cpp   # GUI NDI receiver app
└── ndi_wrapping/
    ├── CMakeLists.txt
    ├── examples/
    │   ├── NDIReceiveTest.cpp
    │   └── NDISenderTest.cpp
    └── include/al_ext/ndi/
        ├── al_NDIReceiver.hpp
        ├── al_NDIReceiver.cpp
        ├── al_NDISender.hpp
        └── al_NDISender.cpp
```

## Key Classes Reference

### NDISender::VideoConfig
```cpp
struct VideoConfig {
    int width = 1920;
    int height = 1080;
    int frameRateN = 60000;  // Numerator
    int frameRateD = 1000;   // Denominator
};
```

### Source (Receiver)
```cpp
struct Source {
    std::string name;  // Human-readable name
    std::string url;   // NDI URL address
};
```

## Troubleshooting

### Common Issues

1. **"No NDI sources found"**
   - Check if sender is running
   - Verify network connectivity
   - Ensure firewall allows NDI traffic

2. **Receiver crashes on connect**
   - Verify NDI SDK path in CMake
   - Check library linking
   - Ensure compatible NDI versions

3. **Poor performance**
   - Use wired network connection
   - Reduce resolution/frame rate
   - Check CPU/GPU usage

### Debug Output
Enable verbose logging by modifying receiver connection code:
```cpp
if (!receiver.connect(sourceName)) {
    std::cerr << "Failed to connect to: " << sourceName << std::endl;
}
```

## Future Enhancements

### High Priority
- [ ] GUI parameter controls for receiver
- [ ] Multiple simultaneous streams
- [ ] Audio support alongside video
- [ ] Connection status indicators

### Medium Priority
- [ ] Hardware-accelerated receiving
- [ ] Compressed video options
- [ ] PTZ camera control
- [ ] Recording capabilities

### Low Priority
- [ ] WebRTC integration
- [ ] Cloud streaming support
- [ ] Advanced codec options

## Development Workflow

### Adding New Features
1. Update planning.md with feature requirements
2. Implement in appropriate class (sender/receiver)
3. Add test in examples/ directory
4. Update CMakeLists.txt if needed
5. Test with existing applications
6. Update documentation

### Code Standards
- Use allolib naming conventions
- Include comprehensive error handling
- Document public APIs
- Test memory management
- Follow RAII principles

## Version History

- **Feb 2026**: Complete NDI receiver implementation with source selection
- **Feb 2026**: Fixed critical memory management bug in sender
- **Feb 2026**: Initial NDI integration with allolib
- **Feb 2026**: Project restructuring to videoPipe/

## Contributing

When adding NDI features:
1. Test with both sender and receiver
2. Verify memory safety
3. Update this documentation
4. Add examples demonstrating usage
5. Test network edge cases

## License and Credits

- NDI SDK: Licensed from NewTek
- allolib: BSD-style license
- Implementation: Custom integration for real-time video streaming