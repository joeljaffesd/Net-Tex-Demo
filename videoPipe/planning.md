# NDI Video Receiver AlloApp - Planning Notes

## Current Status
- ✅ NDI SDK integrated with allolib
- ✅ NDI sender working (NDISimpleTest, NDISimpleApp)
- ✅ NDI receiver class implemented (al_NDIReceiver)
- ✅ Basic receiver test app exists (NDIReceiveTest.cpp)

## Goal
Build an alloApp that receives NDI video frames with the ability to select a video path (NDI source) and display the received frames.

## Key Components Analyzed

### NDIReceiver Class (`al_ext/ndi/al_NDIReceiver.hpp`)
- **Methods:**
  - `init()` - Initialize NDI library
  - `getAvailableSources()` - Returns vector of available NDI sources
  - `connect(const char* sourceName = nullptr)` - Connect to specific source or first available
  - `update(Texture& tex)` - Receive frame and update texture
  - `width()/height()` - Get current frame dimensions

- **Source Structure:**
  ```cpp
  struct Source {
      std::string name;  // NDI stream name
      std::string url;   // NDI URL address
  };
  ```

### Existing Receiver Test (`NDIReceiveTest.cpp`)
- Simple alloApp that:
  - Initializes NDI receiver
  - Lists available sources on startup
  - Connects to first available source
  - Displays received frames as fullscreen quad
- Limitations: No source selection, no UI controls

## Required Features for Full Implementation

### 1. Source Selection
- **UI Menu:** Dropdown/parameter menu to select from available NDI sources
- **Dynamic Refresh:** Button to refresh available sources list
- **Keyboard Shortcuts:** Number keys to quickly select sources 1-9

### 2. Connection Management
- **Connect/Disconnect Buttons:** GUI controls to connect/disconnect
- **Connection Status:** Visual feedback on connection state
- **Auto-reconnection:** Handle source disconnections gracefully

### 3. Video Display
- **Aspect Ratio Handling:** Proper scaling to fit screen while maintaining aspect ratio
- **Resolution Display:** Show current video resolution in UI
- **Performance Monitoring:** Frame rate, dropped frames, etc.

### 4. User Interface
- **Parameter GUI:** Use allolib's ParameterGUI for controls
- **Status Display:** Text overlay showing current status
- **Source List:** Display available sources with selection highlighting

### 5. Error Handling
- **Graceful Failures:** Handle NDI initialization failures
- **Source Validation:** Check if selected source still exists
- **Network Issues:** Handle network interruptions

## Implementation Plan

### Phase 1: Basic Receiver App
1. Create `NDIVideoReceiverApp.cpp` with basic structure
2. Initialize NDI receiver on startup
3. Display received frames (connect to first available source)
4. Add basic keyboard controls for connect/disconnect

### Phase 2: Source Selection
1. Add source discovery and listing
2. Implement source selection via keyboard (1-9 keys)
3. Add refresh functionality (R key)
4. Display source list in UI

### Phase 3: GUI Controls
1. Add ParameterGUI integration
2. Create source selection dropdown
3. Add connect/disconnect buttons
4. Add status display

### Phase 4: Enhanced Features
1. Aspect ratio handling
2. Resolution display
3. Connection status feedback
4. Error handling improvements

## Technical Considerations

### NDI Receiver Behavior
- `connect()` method finds sources dynamically each time
- `update()` returns true only when a new frame is received
- Texture is automatically resized if frame dimensions change
- BGRA color format used for compatibility with OpenGL

### AlloApp Integration
- Use `onCreate()` for initialization
- Use `onDraw()` for frame updates and rendering
- Use `onKeyDown()` for keyboard controls
- Use Parameter system for GUI controls

### Build System
- Add new executable to CMakeLists.txt
- Link against al_ndi library
- Set C++14 standard
- Include allolib and NDI dependencies

## Testing Strategy

### Unit Testing
- Test NDI initialization
- Test source discovery
- Test connection to valid/invalid sources
- Test frame reception

### Integration Testing
- Test with NDISimpleTest sender
- Test with external NDI sources
- Test network interruptions
- Test source disconnections

### User Experience Testing
- Test keyboard controls
- Test GUI controls
- Test different video resolutions
- Test different frame rates

## Potential Issues

### 1. NDI Library Conflicts
- Ensure only one NDI receiver instance per process
- Handle multiple apps trying to access same source

### 2. Performance
- NDI frame reception is blocking (1000ms timeout)
- May need threading for smooth UI if network is slow
- Texture uploads can be expensive for high-res video

### 3. Memory Management
- NDI frames must be freed after use
- Texture resizing on dimension changes
- Proper cleanup on app exit

### 4. Network Considerations
- NDI works over local network
- Firewall settings may block NDI traffic
- Multiple network interfaces may cause issues

## Future Enhancements

### Advanced Features
- Multiple source preview (picture-in-picture)
- Recording received video
- Video effects/filters
- Audio reception and playback
- PTZ camera control (if supported by source)

### Network Features
- Source discovery over WAN
- Authentication/security
- Bandwidth monitoring
- Quality of service controls

### UI Improvements
- Drag-and-drop source selection
- Source thumbnails/previews
- Custom layouts
- Fullscreen mode
- Multiple monitor support