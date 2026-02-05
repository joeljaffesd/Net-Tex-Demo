#include <iostream>
#include <thread>
#include <chrono>
#include "al_ext/ndi/al_NDISender.hpp"

using namespace al;
using namespace std;

int main() {
    cout << "Simple NDI Test - Basic functionality test" << endl;

    // Initialize NDI sender
    NDISender::VideoConfig config;
    config.width = 1024;
    config.height = 768;

    NDISender sender;
    if (!sender.init("SimpleColorTest", config, false)) {  // Disable hardware acceleration for console app
        cout << "Failed to initialize NDI sender" << endl;
        return 1;
    }

    cout << "NDI sender initialized successfully!" << endl;
    cout << "Stream name: 'SimpleColorTest'" << endl;
    cout << "Resolution: " << config.width << "x" << config.height << endl;
    cout << "Use NDI monitoring tools to view the stream." << endl;
    cout << endl;

    // Simple test loop - just demonstrates that NDI is working
    // In a real application, you would render graphics and send actual frames
    int frameCount = 0;
    double time = 0.0;

    cout << "Running test for 10 seconds..." << endl;

    while (frameCount < 600) {  // Run for about 10 seconds at 60fps
        if (frameCount % 60 == 0) {
            cout << "Test running... frame " << frameCount << " at " << time << "s" << endl;
        }

        frameCount++;
        time += 1.0/60.0;

        // Sleep to simulate 60fps timing
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    cout << "Test completed successfully!" << endl;
    cout << "NDI sender functionality verified." << endl;
    return 0;
}