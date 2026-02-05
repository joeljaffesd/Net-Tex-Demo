# NDI Extension Notes

## Overview
This extension provides NDI (Network Device Interface) support for allolib, enabling real-time video streaming over network connections.

## Architecture
- **NDISender**: Sends video frames over NDI network streams
- **NDIReceiver**: Receives video frames from NDI network streams (not yet implemented)

## Key Components
- Uses NDI SDK for Apple (macOS)
- Integrates with allolib's graphics pipeline
- Supports OpenGL texture input
- Software-based sending (no hardware acceleration on macOS)

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

## Performance Considerations
- Current implementation uses software sending with GPU→CPU transfer
- Each frame requires a full texture readback (~4MB for 1024x1024 RGBA)
- Suitable for moderate resolutions and frame rates
- Hardware acceleration not available on macOS OpenGL

## Usage Example
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

## Testing
- NDISenderTest example demonstrates basic functionality
- Test with NDI monitoring tools to verify stream reception
- Check for memory leaks during extended operation

## Future Improvements
- Implement NDIReceiver for receiving streams
- Add audio support alongside video
- Optimize memory transfers (PBOs, async transfers)
- Add compression options
- Support for multiple simultaneous streams