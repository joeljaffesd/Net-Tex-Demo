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
#include "al/graphics/al_Image.hpp"
#include "al/io/al_File.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

// Define a basic state structure to demonstrate distributed functionality
struct SharedState {
  float color = 0.0f;        // Background color that changes over time
  float rotationAngle = 0.0f; // Rotation angle for visual demonstration
  int frameCount = 0;        // Frame counter
  bool textureLoaded = false;
  int textureWidth = 0;
  int textureHeight = 0;
  unsigned char textureData[2048 * 2048 * 4]; // Fixed size for demo, 2048x2048 RGBA
};

struct MyApp: public al::DistributedAppWithState<SharedState> {
  al::VAOMesh mesh;
  al::Texture texture;
  bool textureCreated = false;
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
    // Create a simple quad mesh
    al::addQuad(mesh, 0.6f, 0.6f);
    mesh.update();

    // Primary loads the image and puts data into state
    if (isPrimary()) {
      const std::string filename = al::File::currentPath() + "../allolib/examples/graphics/bin/data/hubble.jpg";
      auto imageData = al::Image(filename);

      if (imageData.array().size() == 0) {
        std::cout << "Failed to load image " << filename << std::endl;
      } else {
        std::cout << "Loaded image size: " << imageData.width() << ", " << imageData.height() << std::endl;
        size_t dataSize = imageData.array().size();
        size_t expectedSize = imageData.width() * imageData.height() * 4;
        size_t maxSize = sizeof(state().textureData);
        if (dataSize == expectedSize && dataSize <= maxSize) {
          std::memcpy(state().textureData, imageData.array().data(), dataSize);
          state().textureWidth = imageData.width();
          state().textureHeight = imageData.height();
          state().textureLoaded = true;
          std::cout << "Texture data loaded into state" << std::endl;
        } else {
          std::cout << "Image size mismatch or too large: dataSize=" << dataSize << ", expected=" << expectedSize << ", max=" << maxSize << std::endl;
        }
      }
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
    }
    // Replicas automatically receive the updated state

    // Create texture from state data if available and not created yet
    if (state().textureLoaded && !textureCreated) {
      texture.create2D(state().textureWidth, state().textureHeight);
      texture.submit(state().textureData, GL_RGBA, GL_UNSIGNED_BYTE);
      texture.filter(al::Texture::LINEAR);
      textureCreated = true;
      std::cout << "Texture created from state data on " << (cuttleboneDomain->isSender() ? "sender" : "receiver") << std::endl;
    }
  } 

  void onDraw(al::Graphics& g) override { // Draw function  
    g.clear(state().color);  
    
    // Draw a rotating square to demonstrate state synchronization
    g.pushMatrix();
    g.rotate(state().rotationAngle, 0, 0, 1);
    g.color(1.0f - state().color, 0.5f, state().color);
    g.draw(mesh);
    g.popMatrix();
    
    // Display texture if created
    if (textureCreated) {
      g.pushMatrix();
      g.translate(0, 0, -5);
      g.quad(texture, -1, -1, 2, 2);
      g.popMatrix();
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