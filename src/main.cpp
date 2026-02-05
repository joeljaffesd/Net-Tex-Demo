// Joel A. Jaffe 2025-06-13
// Basic AlloApp demonstrating how to use the App class's callbacks

// Single macro to switch between desktop and Allosphere configurations
#define DESKTOP

#ifdef DESKTOP
  // Desktop configuration
  #define SAMPLE_RATE 48000
  #define AUDIO_CONFIG SAMPLE_RATE, 128, 2, 8
  #define SPATIALIZER_TYPE al::AmbisonicsSpatializer
  #define SPEAKER_LAYOUT al::StereoSpeakerLayout()
#else
  // Allosphere configuration
  #define SAMPLE_RATE 44100
  #define AUDIO_CONFIG SAMPLE_RATE, 256, 60, 9
  #define SPATIALIZER_TYPE al::Dbap
  #define SPEAKER_LAYOUT al::AlloSphereSpeakerLayoutCompensated()
#endif

#include "al/app/al_DistributedApp.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/graphics/al_FBO.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"
#include "videoPipe/ndi_wrapping/include/al_ext/ndi/al_NDIReceiver.hpp"

// Define a basic state structure to demonstrate distributed functionality
struct SharedState {
  float color = 0.0f;        // Background color that changes over time
  float rotationAngle = 0.0f; // Rotation angle for visual demonstration
  int frameCount = 0;        // Frame counter
  float time = 0.0f;         // Time for animation
  float onset = 0.0f;        // Onset parameter for shader
  float cent = 0.0f;         // Cent parameter for shader
  float flux = 0.0f;         // Flux parameter for shader
  bool textureLoaded = false;
  int textureWidth = 512;
  int textureHeight = 512;
  unsigned char textureData[2048 * 2048 * 4]; // Larger buffer for various video resolutions
};

struct MyApp: public al::DistributedAppWithState<SharedState> {
  al::VAOMesh mesh;
  al::Texture renderTexture;  // For displaying NDI video
  al::RBO rbo;                // Render buffer for depth
  al::FBO fbo;                // Frame buffer object for offscreen rendering
  al::Texture displayTexture; // For secondaries to display received texture
  bool displayTextureCreated = false;
  al::NDIReceiver ndiReceiver; // NDI receiver for primary
  std::shared_ptr<al::CuttleboneStateSimulationDomain<SharedState, 8000>> cuttleboneDomain;

  void onInit() override { // Called on app start
    std::cout << "onInit() - " << (isPrimary() ? "Primary" : "Replica") << " instance" << std::endl;
    // Enable Cuttlebone with larger packet size for texture data
    cuttleboneDomain = al::CuttleboneStateSimulationDomain<SharedState, 8000>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }
  }

  void onCreate() override { // Called when graphics context is available
    std::cout << "onCreate()" << std::endl;
    // Create a simple quad mesh with texture coordinates
    al::addTexRect(mesh, -0.6f, -0.6f, 1.2f, 1.2f);
    mesh.update();

    // Initialize NDI receiver on primary
    if (isPrimary()) {
      if (!ndiReceiver.init()) {
        std::cerr << "Failed to initialize NDI receiver" << std::endl;
      } else {
        std::cout << "NDI receiver initialized" << std::endl;
        // Try to connect to first available source
        if (!ndiReceiver.connect()) {
          std::cout << "No NDI sources found or failed to connect" << std::endl;
        } else {
          std::cout << "Connected to NDI source" << std::endl;
        }
      }
    }

    // Set up FBO for primary to render animated texture
    if (isPrimary()) {
      renderTexture.create2D(state().textureWidth, state().textureHeight);
      rbo.resize(state().textureWidth, state().textureHeight);
      fbo.bind();
      fbo.attachTexture2D(renderTexture);
      fbo.attachRBO(rbo);
      fbo.unbind();
      std::cout << "FBO status: " << fbo.statusString() << std::endl;
    }
  }

  void onAnimate(double dt) override { // Called once before drawing
    // Primary instance updates the shared state
    if (cuttleboneDomain->isSender()) {
      state().color += 0.01f;
      if (state().color > 1.f) {
        state().color -= 1.f;
      }
      state().rotationAngle += 0.02f;
      state().frameCount++;
      state().time += dt; // Update time for animation
      
      // Update shader parameters (kept for compatibility)
      state().onset = sin(state().time * 0.5f) * 0.5f + 0.5f;
      state().cent = cos(state().time * 0.3f) * 0.5f + 0.5f;
      state().flux = sin(state().time * 0.7f) * 0.5f + 0.5f;

      // Update texture with NDI video
      if (ndiReceiver.update(renderTexture)) {
        // Update state dimensions to match the texture
        state().textureWidth = renderTexture.width();
        state().textureHeight = renderTexture.height();
        
        // Read texture data from GPU to CPU for transmission
        renderTexture.bind();
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, state().textureData);
        renderTexture.unbind();
        
        state().textureLoaded = true;
        std::cout << "Primary received NDI frame " << state().frameCount << " (" 
                  << state().textureWidth << "x" << state().textureHeight 
                  << ") and prepared for transmission" << std::endl;
      }
    }
    // Replicas automatically receive the updated state
  } 

  void onDraw(al::Graphics& g) override { // Draw function  
    g.clear(state().color);  
    
    // Draw a rotating square to demonstrate state synchronization
    g.pushMatrix();
    g.rotate(state().rotationAngle, 0, 0, 1);
    g.color(1.0f - state().color, 0.5f, state().color);
    g.draw(mesh);
    g.popMatrix();
    
    // Display texture if available
    if (state().textureLoaded) {
      // For primary: display the local renderTexture
      if (cuttleboneDomain->isSender()) {
        g.quadViewport(renderTexture, -0.9, -0.9, 1.8, 1.8);
      } else {
        // For secondaries: display texture from received state data
        // Recreate texture if dimensions changed
        if (!displayTextureCreated || 
            displayTexture.width() != state().textureWidth || 
            displayTexture.height() != state().textureHeight) {
          displayTexture.create2D(state().textureWidth, state().textureHeight);
          displayTextureCreated = true;
          std::cout << "Secondary display texture created/resized to " 
                    << state().textureWidth << "x" << state().textureHeight << std::endl;
        }
        displayTexture.submit(state().textureData, GL_RGBA, GL_UNSIGNED_BYTE);
        g.quadViewport(displayTexture, -0.9, -0.9, 1.8, 1.8);
      }
    }
    
    // Display instance type and frame count
    std::string info = cuttleboneDomain->isSender() ? "SENDER - Frame: " : "RECEIVER - Frame: ";
    info += std::to_string(state().frameCount);
    // Note: Text rendering would require additional setup, so we'll skip it for this basic demo
  }

  void onSound(al::AudioIOData& io) override { // Audio callback  
    static float phase = 0.0f;
    float sampleRate = io.framesPerSecond();
    while (io()) {    
      // Generate a simple tone based on the shared color value
      float freq = 220.0f + (state().color * 440.0f);
      float sample = sinf(phase) * 0.1f;
      phase += M_2PI * freq / sampleRate;
      if (phase > M_2PI) phase -= M_2PI;
      io.out(0) = io.out(1) = sample;
    }
  }

  void onMessage(al::osc::Message& m) override { // OSC message callback  
    m.print();  
  }

  bool onKeyDown(const al::Keyboard& k) override {
    if (k.key() == ' ') {
      if (cuttleboneDomain->isSender()) {
        state().color = 0.f;
        state().rotationAngle = 0.f;
        state().frameCount = 0;
      }
    }
    return true;
  }

};

int main() {
  MyApp app;
  app.title("Distributed Demo");
  app.configureAudio(AUDIO_CONFIG);
  app.start();
  return 0;
}