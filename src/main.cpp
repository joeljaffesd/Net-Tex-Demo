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
#include "al/graphics/al_Shader.hpp"
#include "al/io/al_File.hpp"
#include "al_ext/statedistribution/al_CuttleboneStateSimulationDomain.hpp"
#include <fstream>
#include <sstream>

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
  int textureWidth = 2048;  // 2k equirectangular width
  int textureHeight = 1024; // 2k equirectangular height
  unsigned char textureData[2048 * 1024 * 4]; // 2k equirectangular buffer
};

struct MyApp: public al::DistributedAppWithState<SharedState> {
  al::VAOMesh mesh;
  al::Texture renderTexture;  // For displaying rendered shader
  al::RBO rbo;                // Render buffer for depth
  al::FBO fbo;                // Frame buffer object for offscreen rendering
  al::Texture displayTexture; // For secondaries to display received texture
  bool displayTextureCreated = false;
  al::ShaderProgram shaderProgram; // Shader for generating texture
  al::VAOMesh quadMesh;       // Quad for rendering shader
  std::shared_ptr<al::CuttleboneStateSimulationDomain<SharedState, 8000>> cuttleboneDomain;

  void onInit() override { // Called on app start
    std::cout << "onInit() - " << (isPrimary() ? "Primary" : "Replica") << " instance" << std::endl;
    // Enable Cuttlebone with larger packet size for texture data
    cuttleboneDomain = al::CuttleboneStateSimulationDomain<SharedState, 8000>::enableCuttlebone(this);
    if (!cuttleboneDomain) {
      std::cerr << "ERROR: Could not start Cuttlebone. Quitting." << std::endl;
      quit();
    }

    // Add search path for shaders
    // al::File::searchPaths().add("../");
  }

  void onCreate() override { // Called when graphics context is available
    std::cout << "onCreate()" << std::endl;
    // Create a textured sphere mesh for equirectangular mapping
    al::addTexSphere(mesh, 1.0f, 64, false); // radius 1, 64 bands, equirectangular mode
    mesh.update();

    // Load and compile shaders
    std::string shaderPath = "shaders/";
    std::ifstream testFile(shaderPath + "standard.vert");
    if (!testFile.is_open()) {
      shaderPath = "../shaders/";
    }
    testFile.close();
    
    std::ifstream vertFile(shaderPath + "standard.vert");
    std::ifstream fragFile(shaderPath + "julia.frag");
    if (!vertFile.is_open() || !fragFile.is_open()) {
      std::cerr << "Failed to open shader files" << std::endl;
      quit();
    }
    std::stringstream vertStream, fragStream;
    vertStream << vertFile.rdbuf();
    fragStream << fragFile.rdbuf();
    std::string vertSource = vertStream.str();
    std::string fragSource = fragStream.str();
    
    if (!shaderProgram.compile(vertSource, fragSource)) {
      std::cerr << "Failed to compile shaders" << std::endl;
      shaderProgram.printLog();
      quit();
    } else {
      std::cout << "Shaders compiled successfully" << std::endl;
    }

    // Create a quad mesh for rendering the shader
    al::addTexQuad(quadMesh);
    quadMesh.update();

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

    // Position camera to view the sphere
    // nav().pos(0, 0, 5).faceToward({0, 0, 0}, {0, 1, 0});
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
      state().time += dt * 0.2f; // Slow down time to 20%
      
      // Update shader parameters
      state().onset = sin(state().time * 0.5f) * 0.5f + 0.5f;
      state().cent = cos(state().time * 0.3f) * 0.5f + 0.5f;
      state().flux = sin(state().time * 0.7f) * 0.5f + 0.5f;

      // Render shader to texture
      fbo.bind();
      glViewport(0, 0, state().textureWidth, state().textureHeight);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      graphics().pushMatrix();
      graphics().camera(al::Viewpoint::IDENTITY);
      graphics().shader(shaderProgram);
      shaderProgram.uniform("u_time", state().time);
      shaderProgram.uniform("onset", state().onset);
      shaderProgram.uniform("cent", state().cent);
      shaderProgram.uniform("flux", state().flux);
      
      graphics().draw(quadMesh);
      graphics().popMatrix();
      fbo.unbind();
      fbo.unbind();
      
      // Read texture data from GPU to CPU for transmission
      renderTexture.bind();
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, state().textureData);
      renderTexture.unbind();
      
      state().textureLoaded = true;
      std::cout << "Primary rendered shader frame " << state().frameCount << " and prepared for transmission" << std::endl;
    }
    // Replicas automatically receive the updated state
  } 

  void onDraw(al::Graphics& g) override { // Draw function  
    g.clear(state().color); 
    
    // Enable depth testing for proper 3D rendering
    al::gl::depthTesting(true);
    
    // Display texture if available
    if (state().textureLoaded) {
      g.pushMatrix();
      // For primary: display the local renderTexture
      if (cuttleboneDomain->isSender()) {
        renderTexture.bind(0);
        g.texture();
        g.draw(mesh);
        renderTexture.unbind(0);
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
        displayTexture.bind(0);
        g.texture();
        g.draw(mesh);
        displayTexture.unbind(0);
      }
      g.popMatrix();
    } else {
      // Draw a simple sphere when no texture is loaded
      g.pushMatrix();
      g.rotate(state().rotationAngle, 0, 0, 1);
      g.color(1.0f - state().color, 0.5f, state().color);
      g.draw(mesh);
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
      // io.out(0) = io.out(1) = sample;
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