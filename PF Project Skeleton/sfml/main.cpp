#include "app.h"
#include "../core/simulation_state.h"
#include "../core/simulation.h"
#include "../core/io.h"
#include <iostream>

// ============================================================================
// MAIN.CPP - Entry point of the application (NO CLASSES)
// ============================================================================

// ----------------------------------------------------------------------------
// MAIN ENTRY POINT
// ----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // Initialize simulation state
    initializeSimulationState();
    initializeLogFiles();
    
    // Get level file from command line argument
    if (argc > 1) {
        level_filename = argv[1];
    }
    
    // Load level file
    if (!loadLevelFile()) {
        std::cout << "Error: Could not load level file: " << level_filename << "\n";
        std::cout << "Usage: ./switchback_rails [level_file]\n";
        return 1;
    }
    
    // Initialize simulation
    initializeSimulation();
    
    // Initialize SFML application
    if (!initializeApp()) {
        std::cout << "Error: Failed to initialize SFML application\n";
        return 1;
    }
    
    // Print control instructions
    std::cout << "\n=== Switchback Rails - SFML Visualization ===\n";
    std::cout << "Controls:\n";
    std::cout << "  SPACE     - Pause/Resume simulation\n";
    std::cout << "  . (period) - Step one tick forward\n";
    std::cout << "  ESC       - Exit and save metrics\n";
    std::cout << "  Left Click - Place/Remove safety tile\n";
    std::cout << "  Right Click - Toggle switch state\n";
    std::cout << "  Mouse Wheel - Zoom in/out\n";
    std::cout << "  Middle Drag - Pan camera\n";
    std::cout << "\nRunning simulation...\n\n";
    
    // Run main application loop
    runApp();
    
    // Cleanup
    cleanupApp();
    
    // Print final statistics
    std::cout << "\n=== Simulation Complete ===\n";
    std::cout << "Total Arrivals: " << arrival << "\n";
    std::cout << "Total Crashes: " << crashes << "\n";
    std::cout << "Final Tick: " << currentTick << "\n";
    std::cout << "Metrics saved to out/metrics.txt\n";
    
    return 0;
}