# NDI Extension Notes

## Overview
This extension provides complete NDI (Network Device Interface) support for allolib, enabling real-time video streaming over network connections. Includes both sending and receiving capabilities.

## Architecture
- **NDISender**: Sends video frames over NDI network streams (WORKING)
- **NDIReceiver**: Receives video frames from NDI network streams (WORKING)

## Key Components
- Uses NDI SDK for Apple (macOS)
- Integrates with allolib's graphics pipeline
- Supports OpenGL texture input/output
- Software-based sending with GPU↔CPU transfer
- Dynamic source discovery and selection

## Critical Bug Fix (Feb 2026)
### Problem
The original implementation had a critical bug where `NDIlib_video_frame_v2_t.p_data` was incorrectly set to point to an OpenGL texture ID instead of actual pixel data in CPU memory. This caused:
- Segmentation faults when sending frames
- Invalid memory access during NDI transmission

### Root Cause
```cpp
// WRONG: Setting p_data to texture ID
mHardwareCtx.videoFrame.p_data = (uint8_t*)mHardwareCtx.sharedTexture;

// CORRECT: Allocating CPU memory and setting p_data to pixel buffer
mHardwareCtx.pPixelData = new uint8_t[dataSize];
mHardwareCtx.videoFrame.p_data = mHardwareCtx.pPixelData;
```

### Solution
1. **Allocate CPU Memory**: Create a pixel buffer in system RAM
2. **GPU-to-CPU Transfer**: Use `glReadPixels()` to copy rendered texture data to CPU memory
3. **Proper Pointer Assignment**: Set `p_data` to the CPU pixel buffer, not texture ID
4. **Memory Management**: Properly allocate/deallocate pixel buffers during resize operations

### Code Changes
- Modified `initHardwareContext()` to allocate CPU pixel buffer
- Updated `resizeHardwareContext()` to reallocate buffer on dimension changes
- Added `glReadPixels()` call in `sendDirect()` to transfer GPU→CPU
- Updated `HardwareContext` struct to include `uint8_t* pPixelData`

## Receiver Implementation (Feb 2026)

### Features
- **Dynamic Source Discovery**: Automatically finds available NDI streams on network
- **Source Selection**: Connect to specific streams by name
- **Texture Integration**: Direct rendering to allolib Texture objects
- **Automatic Resizing**: Adapts to incoming video dimensions
- **Color Space Handling**: BGRA→RGBA conversion for OpenGL

### Usage
```cpp
NDIReceiver receiver;
receiver.init();

// Discover sources
auto sources = receiver.getAvailableSources();

// Connect to specific source
receiver.connect("MyNDIStream");

// Receive frames
Texture tex;
if (receiver.update(tex)) {
    // Render texture to screen
    g.quad(tex, -1, -1, 2, 2);
}
```

## Performance Considerations
- **Sender**: GPU→CPU transfer (~4MB/frame for 1024×768 RGBA)
- **Receiver**: Minimal CPU overhead, efficient GPU texture uploads
- **Network**: Uncompressed video, optimized for local networks
- **Hardware Acceleration**: Not available on macOS OpenGL (software path)

## Working Applications

### Sender Applications
```cpp
// Console test
NDISimpleTest  // Basic functionality verification

// GUI app with animated patterns
NDISimpleApp   // Visual sender testing
```

### Receiver Applications
```cpp
// Basic receiver (auto-connect)
NDIReceiveTest // Simple fullscreen display

// Advanced receiver with source selection
NDIVideoReceiverApp // Keyboard-controlled source selection
```

## Build Configuration
- Requires NDI SDK path in root CMakeLists.txt
- Links with NDI libraries and allolib graphics
- Include paths must cover allolib external dependencies
- C++14 standard required

## Testing and Validation

### Test Setup
1. Run sender application (e.g., `./NDISimpleTest`)
2. Run receiver application (e.g., `./NDIVideoReceiverApp`)
3. Use NDI Studio Monitor to verify streams
4. Test source selection and connection management

### NDI Monitoring Tools
- **NDI Studio Monitor**: Official monitoring application
- **NDI Tools**: Free suite with monitoring capabilities
- **SDK Examples**: Basic receiver examples for testing

### Network Requirements
- Local network connectivity
- Firewall configuration for NDI ports
- Wired Ethernet recommended for best performance

## Future Improvements
- [x] Implement NDIReceiver for receiving streams (COMPLETED)
- [ ] Add audio support alongside video
- [ ] Optimize memory transfers (PBOs, async transfers)
- [ ] Add compression options
- [ ] Support for multiple simultaneous streams
- [ ] GUI parameter controls for receiver
- [ ] Hardware-accelerated receiving

## File Organization
```
videoPipe/
├── DEVELOPER.md              # Complete developer documentation
├── NDI-Notes.md              # Technical implementation notes
├── planning.md               # Development planning
├── NDISimpleTest.cpp         # Console sender test
├── NDISimpleApp.cpp          # GUI sender app
├── NDIVideoReceiverApp.cpp   # GUI receiver with source selection
└── ndi_wrapping/
    ├── CMakeLists.txt
    ├── examples/
    └── include/al_ext/ndi/
        ├── al_NDIReceiver.*   # Receiver implementation
        └── al_NDISender.*     # Sender implementation
```
```cpp
NDISender sender;
sender.init("MyStream", NDISender::VideoConfig(1920, 1080));

// In render loop:
sender.sendDirect(fbo.tex());  // Send FBO texture over NDI
```

## Build Configuration
- Requires NDI SDK path in root CMakeLists.txt
- Links with NDI libraries and allolib graphics
- Include paths must cover allolib external dependencies
- C++14 standard required for allolib compatibility

## Testing and Validation
- **Sender Tests**: NDISimpleTest, NDISimpleApp demonstrate functionality
- **Receiver Tests**: NDIReceiveTest, NDIVideoReceiverApp for receiving
- **Cross-testing**: Run sender and receiver simultaneously
- **Monitoring**: Use NDI Studio Monitor to verify streams
- **Network**: Test on local network with proper firewall configuration

## Future Improvements
- [x] Implement NDIReceiver for receiving streams (COMPLETED)
- [ ] Add audio support alongside video
- [ ] Optimize memory transfers (PBOs, async transfers)
- [ ] Add compression options
- [ ] Support for multiple simultaneous streams
- [ ] GUI parameter controls for receiver
- [ ] Hardware-accelerated receiving