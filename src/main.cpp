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
  float onset = 0.0f;        // Onset parameter for shader
  float cent = 0.0f;         // Cent parameter for shader
  float flux = 0.0f;         // Flux parameter for shader
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
out vec3 vPos;
out vec2 vUV;

void main() {
  vPos = vertexPosition;
  vUV = vertexTexCoord;
  gl_Position = al_ProjectionMatrix * al_ModelViewMatrix * vec4(vertexPosition, 1.0);
}
)";

    const char* fragmentShader = R"(
#version 330 core

in vec3 vPos; //recieve from vert
in vec2 vUV;

out vec4 fragColor;

uniform float u_time;
uniform float onset;
uniform float cent;
uniform float flux;


// *** STARTER CODE INSPIRED BY : https://www.shadertoy.com/view/4lSSRy *** //

void main() {
    vec2 uv = 0.3 * vPos.xy;
    mediump float t = (u_time * 0.01) + onset;

    mediump float k = cos(t);
    mediump float l = sin(t);
    mediump float s = 0.2 + (onset/10.0);

    // XXX simplify back to shadertoy example
    for(int i = 0; i < 32; ++i) {
        uv  = abs(uv) - s;//-onset;    // Mirror
        uv *= mat2(k,-l,l,k); // Rotate
        s  *= .95156;///(t+1);         // Scale
    }

    mediump float x = .5 + .5 * cos(6.28318 * (40.0 * length(uv)));
    fragColor = .5 + .5 * cos(6.28318 * (40.0 * length(uv)) * vec4(-1,2 + (u_time / 500.0), 3 + flux, 1));
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
      
      // Update shader parameters
      state().onset = sin(state().time * 0.5f) * 0.5f + 0.5f;
      state().cent = cos(state().time * 0.3f) * 0.5f + 0.5f;
      state().flux = sin(state().time * 0.7f) * 0.5f + 0.5f;

      // Render animated shader to texture
      if (shaderCompiled) {
        // Set up orthographic projection for FBO rendering
        al::Graphics fboG;
        fboG.framebuffer(fbo);
        fboG.viewport(0, 0, state().textureWidth, state().textureHeight);
        fboG.clear(0, 0, 0);

        // Use animated shader and render quad
        fboG.shader(animatedShader);
        animatedShader.uniform("u_time", state().time);
        animatedShader.uniform("onset", state().onset);
        animatedShader.uniform("cent", state().cent);
        animatedShader.uniform("flux", state().flux);
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