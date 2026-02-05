#include <iostream>
#include <vector>
#include <string>
#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/ui/al_ParameterGUI.hpp"
#include "al_ext/ndi/al_NDIReceiver.hpp"

using namespace al;
using namespace std;

struct NDIVideoReceiverApp : public App {
    NDIReceiver ndiReceiver;
    Texture receivedTexture;
    vector<Source> availableSources;
    int selectedSourceIndex = -1;
    bool connected = false;
    string statusMessage = "Initializing...";

    // UI parameters - simplified for now
    // ParameterMenu sourceMenu{"NDI Source", "", 0};
    // ParameterBool connectButton{"Connect", "", false};
    // ParameterBool disconnectButton{"Disconnect", "", false};

    void onCreate() override {
        // Initialize NDI
        if (!ndiReceiver.init()) {
            statusMessage = "Failed to initialize NDI";
            cout << statusMessage << endl;
            return;
        }

        // Initialize texture
        receivedTexture.filter(Texture::LINEAR);
        receivedTexture.wrap(Texture::CLAMP_TO_EDGE);

        // Get available sources
        refreshSources();

        // Setup UI parameters - simplified for now
        // TODO: Add proper GUI integration later

        statusMessage = "Ready - Select NDI source and click Connect";
    }

    void onDraw(Graphics& g) override {
        g.clear(0.1, 0.1, 0.1);

        // Handle connection/disconnection - simplified for keyboard only
        // TODO: Add GUI controls later

        // Try to receive and display frames
        if (connected && ndiReceiver.update(receivedTexture)) {
            // Draw the received texture
            g.pushMatrix();
            g.translate(0, 0, -4);

            // Calculate aspect ratio to fit screen
            float screenAspect = width() / (float)height();
            float textureAspect = receivedTexture.width() / (float)receivedTexture.height();

            if (textureAspect > screenAspect) {
                // Texture is wider, fit to width
                float scale = 2.0f / textureAspect;
                g.scale(textureAspect * scale, scale, 1);
            } else {
                // Texture is taller, fit to height
                float scale = 2.0f / textureAspect;
                g.scale(scale, textureAspect * scale, 1);
            }

            g.quad(receivedTexture, -1, -1, 2, 2);
            g.popMatrix();

            statusMessage = "Receiving: " + to_string(receivedTexture.width()) + "x" +
                          to_string(receivedTexture.height());
        }

        // Draw UI
        drawUI(g);
    }

    void drawUI(Graphics& g) {
        // Simplified UI - just show status in console for now
        // TODO: Add proper text rendering later
    }

    bool onKeyDown(Keyboard const& k) override {
        if (k.key() == 'r' || k.key() == 'R') {
            refreshSources();
        } else if (k.key() == 'c' || k.key() == 'C') {
            if (!connected) {
                connectToSelectedSource();
            }
        } else if (k.key() == 'd' || k.key() == 'D') {
            if (connected) {
                disconnectFromSource();
            }
        } else if (k.key() >= '1' && k.key() <= '9') {
            int index = k.key() - '1';
            if (index >= 0 && index < (int)availableSources.size()) {
                selectedSourceIndex = index;
                cout << "Selected source: " << availableSources[index].name << endl;
            }
        }
        return true; // Continue processing other key events
    }

    void refreshSources() {
        availableSources = ndiReceiver.getAvailableSources();
        cout << "Found " << availableSources.size() << " NDI sources:" << endl;
        for (size_t i = 0; i < availableSources.size(); ++i) {
            cout << "  " << (i + 1) << ": " << availableSources[i].name << endl;
        }

        if (!availableSources.empty()) {
            selectedSourceIndex = 0;
            statusMessage = "Sources refreshed. Press C to connect or use keyboard 1-9.";
        } else {
            statusMessage = "No NDI sources found. Press R to refresh.";
        }
    }

    void connectToSelectedSource() {
        if (availableSources.empty()) {
            statusMessage = "No sources available";
            return;
        }

        int sourceIndex = selectedSourceIndex;
        if (sourceIndex < 0 || sourceIndex >= (int)availableSources.size()) {
            sourceIndex = 0;
        }

        const string& sourceName = availableSources[sourceIndex].name;
        cout << "Connecting to: " << sourceName << endl;

        if (ndiReceiver.connect(sourceName.c_str())) {
            connected = true;
            selectedSourceIndex = sourceIndex;
            statusMessage = "Connected to: " + sourceName;
            cout << "Successfully connected to NDI source: " << sourceName << endl;
        } else {
            statusMessage = "Failed to connect to: " + sourceName;
            cout << "Failed to connect to NDI source: " << sourceName << endl;
        }
    }

    void disconnectFromSource() {
        // Note: The NDIReceiver doesn't have a disconnect method,
        // but we can recreate the receiver
        connected = false;
        selectedSourceIndex = -1;
        statusMessage = "Disconnected";
        cout << "Disconnected from NDI source" << endl;
    }

    void onExit() override {
        cout << "NDI Video Receiver App exited." << endl;
    }
};

int main() {
    NDIVideoReceiverApp app;
    app.dimensions(1280, 720);

    // Add some instructions
    cout << "NDI Video Receiver Controls:" << endl;
    cout << "  R - Refresh available sources" << endl;
    cout << "  C - Connect to selected source" << endl;
    cout << "  D - Disconnect" << endl;
    cout << "  1-9 - Select source by number" << endl;
    cout << "  Or use the GUI controls" << endl;
    cout << endl;

    app.start();
    return 0;
}