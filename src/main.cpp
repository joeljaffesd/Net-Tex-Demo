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
#include "al/graphics/al_Shader.hpp"
#include "al/graphics/al_FBO.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"

// Define a basic state structure to demonstrate distributed functionality
struct SharedState {
  float color = 0.0f;        // Background color that changes over time
  float rotationAngle = 0.0f; // Rotation angle for visual demonstration
  int frameCount = 0;        // Frame counter
  float time = 0.0f;         // Time for animation
  bool textureLoaded = false;
  int textureWidth = 512;
  int textureHeight = 512;
  unsigned char textureData[512 * 512 * 4]; // Fixed size for demo, 512x512 RGBA
};

struct MyApp: public al::DistributedAppWithState<SharedState> {
  al::VAOMesh mesh;
  al::ShaderProgram animatedShader;
  bool shaderCompiled = false;
  al::Texture renderTexture;  // For primary to render animated shader to
  al::RBO rbo;                // Render buffer for depth
  al::FBO fbo;                // Frame buffer object for offscreen rendering
  al::Texture displayTexture; // For secondaries to display received texture
  bool displayTextureCreated = false;
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

    // Create animated shader
    const char* vertexShader = R"(
#version 330
uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;
in vec3 vertexPosition;
in vec2 vertexTexCoord;
out vec2 vTexcoord;

void main() {
  vTexcoord = vertexTexCoord;
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * vec4(vertexPosition, 1.0);
}
)";

    const char* fragmentShader = R"(
#version 330
uniform float uTime;
in vec2 vTexcoord;
out vec4 fragColor;

void main() {
  // Create animated pattern based on time and texture coordinates
  vec2 uv = vTexcoord;
  
  // Create moving waves
  float wave1 = sin(uv.x * 10.0 + uTime * 2.0) * 0.5 + 0.5;
  float wave2 = sin(uv.y * 8.0 + uTime * 1.5) * 0.5 + 0.5;
  float wave3 = sin((uv.x + uv.y) * 6.0 + uTime * 3.0) * 0.5 + 0.5;
  
  // Combine waves for RGB channels
  float r = wave1;
  float g = wave2; 
  float b = wave3;
  
  // Add some color variation
  r += sin(uTime + uv.x * 20.0) * 0.2;
  g += cos(uTime * 0.7 + uv.y * 15.0) * 0.2;
  b += sin(uTime * 1.3 + (uv.x + uv.y) * 12.0) * 0.2;
  
  // Make sure colors are in valid range
  // r = clamp(r, 0.0, 1.0);
  // g = clamp(g, 0.0, 1.0);
  // b = clamp(b, 0.0, 1.0);
  
  fragColor = vec4(r, g, b, 1.0);
}
)";

    if (animatedShader.compile(vertexShader, fragmentShader)) {
      shaderCompiled = true;
      std::cout << "Animated shader compiled successfully" << std::endl;
    } else {
      std::cout << "Failed to compile animated shader" << std::endl;
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

      // Render animated shader to texture
      if (shaderCompiled) {
        // Set up orthographic projection for FBO rendering
        al::Graphics fboG;
        fboG.framebuffer(fbo);
        fboG.viewport(0, 0, state().textureWidth, state().textureHeight);
        fboG.clear(0, 0, 0);

        // Use animated shader and render quad
        fboG.shader(animatedShader);
        animatedShader.uniform("uTime", state().time);
        fboG.draw(mesh);

        // Read texture data from FBO
        renderTexture.bind();
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, state().textureData);
        renderTexture.unbind();

        state().textureLoaded = true;
        std::cout << "Primary rendered frame " << state().frameCount << " to texture" << std::endl;
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
    if (state().textureLoaded && !displayTextureCreated) {
      displayTexture.create2D(state().textureWidth, state().textureHeight);
      displayTexture.submit(state().textureData, GL_RGBA, GL_UNSIGNED_BYTE);
      displayTexture.filter(al::Texture::LINEAR);
      displayTextureCreated = true;
      std::cout << "Display texture created from state data" << std::endl;
    } else if (state().textureLoaded && displayTextureCreated) {
      // Update texture with new data
      displayTexture.submit(state().textureData, GL_RGBA, GL_UNSIGNED_BYTE);
      if (state().frameCount % 60 == 0) { // Log every 60 frames to avoid spam
        std::cout << "Updated display texture for frame " << state().frameCount << std::endl;
      }
    }
    
    if (displayTextureCreated) {
      g.pushMatrix();
      g.translate(0, 0, -5);
      g.quad(displayTexture, -1, -1, 2, 2);
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