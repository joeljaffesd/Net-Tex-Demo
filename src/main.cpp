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

// Define a basic state structure to demonstrate distributed functionality
struct SharedState {
  float color = 0.0f;        // Background color that changes over time
  float rotationAngle = 0.0f; // Rotation angle for visual demonstration
  int frameCount = 0;        // Frame counter
};

struct MyApp: public al::DistributedAppWithState<SharedState> {
  al::VAOMesh mesh;

  void onInit() override { // Called on app start
    std::cout << "onInit() - " << (isPrimary() ? "Primary" : "Replica") << " instance" << std::endl;
  }

  void onCreate() override { // Called when graphics context is available
    std::cout << "onCreate()" << std::endl;
    // Create a simple quad mesh
    al::addQuad(mesh, 0.6f, 0.6f);
    mesh.update();
  }

  void onAnimate(double dt) override { // Called once before drawing
    // Primary instance updates the shared state
    if (isPrimary()) {
      state().color += 0.01f;
      if (state().color > 1.f) {
        state().color -= 1.f;
      }
      state().rotationAngle += 0.02f;
      state().frameCount++;
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
    
    // Display instance type and frame count
    std::string info = isPrimary() ? "PRIMARY - Frame: " : "REPLICA - Frame: ";
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
      if (isPrimary()) {
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