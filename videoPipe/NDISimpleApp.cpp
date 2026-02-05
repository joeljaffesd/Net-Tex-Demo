#include <iostream>
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al_ext/ndi/al_NDISender.hpp"

using namespace al;
using namespace std;

struct NDISimpleApp : public App {
  EasyFBO fbo;
  NDISender ndiSender;
  double time = 0.0;
  int frameCount = 0;

  void onCreate() {
    // Initialize FBO for rendering our test pattern
    fbo.init(1024, 768);

    // Initialize NDI sender with hardware acceleration enabled (we have OpenGL context)
    NDISender::VideoConfig config;
    config.width = 1024;
    config.height = 768;

    if (ndiSender.init("NDISimpleApp", config, true)) {  // Enable hardware acceleration
      cout << "NDI Sender initialized successfully" << endl;
      cout << "Stream name: 'NDISimpleApp'" << endl;
      cout << "Resolution: " << config.width << "x" << config.height << endl;
    } else {
      cout << "Failed to initialize NDI Sender" << endl;
      quit();
    }
  }

  void onDraw(Graphics& g) {
    // Render to FBO - simple animated test pattern
    g.pushFramebuffer(fbo);
    g.pushViewport(0, 0, fbo.width(), fbo.height());

    // Simple animated background color
    g.clear(
      0.5 + 0.5 * sin(time * 2.0),        // Red
      0.5 + 0.5 * sin(time * 2.0 + M_PI/2), // Green
      0.5 + 0.5 * sin(time * 2.0 + M_PI),   // Blue
      1.0  // Alpha
    );

    g.popViewport();
    g.popFramebuffer();

    // Display on screen
    g.clear(0.1, 0.1, 0.1);
    g.pushMatrix();
    g.translate(-1, -1, 0);
    g.scale(2.0/fbo.width(), 2.0/fbo.height(), 1);
    g.quad(fbo.tex(), 0, 0, fbo.width(), fbo.height());
    g.popMatrix();

    // Send over NDI
    if (ndiSender.sendDirect(fbo.tex())) {
      if (frameCount % 60 == 0) {  // Print every ~1 second at 60fps
        cout << "Sent NDI frame " << frameCount << " at " << time << "s" << endl;
      }
    } else {
      cout << "Failed to send NDI frame" << endl;
    }

    frameCount++;
    time += 1.0/60.0;  // Assume 60fps
  }

  void onExit() {
    cout << "NDI Simple App exited. Sent " << frameCount << " frames." << endl;
  }
};

int main() {
  NDISimpleApp app;
  app.dimensions(1024, 768);
  app.start();
}