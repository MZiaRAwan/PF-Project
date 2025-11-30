#include "simulation.h"
#include "simulation_state.h"
#include "trains.h"
#include "switches.h"
#include "io.h"
#include "grid.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

// ============================================================================
// SIMULATION.CPP - Implementation of main simulation logic
// ============================================================================
// This file contains the 7-phase tick system and simulation control functions.
// These functions should ONLY be defined here, NOT in simulation_state.cpp
// ============================================================================

// ----------------------------------------------------------------------------
// HELPER: Calculate Manhattan distance between two points
// ----------------------------------------------------------------------------
int calculateManhattanDistance(int x1, int y1, int x2, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

// ----------------------------------------------------------------------------
// INITIALIZE SIMULATION
// ----------------------------------------------------------------------------
// Initialize simulation after loading level file.
// Assign destinations to trains, initialize switch states.
// ----------------------------------------------------------------------------
void initializeSimulation() {
    // Initialize random seed for deterministic behavior
    // Identical inputs (level file + seed) => identical outputs
    srand(level_seed);
    
    // Assign destinations to trains (simple: assign first available destination)
    int dest_idx = 0;
    for (int i = 0; i < total_trains; i++)
    {
        if (dest_idx < total_destinations)
        {
            train_dest_x[i] = dest_X[dest_idx];
            train_dest_y[i] = dest_Y[dest_idx];
            dest_idx++;
        }
        else
        {
            // Wrap around if more trains than destinations
            train_dest_x[i] = dest_X[i % total_destinations];
            train_dest_y[i] = dest_Y[i % total_destinations];
        }
    }
    
    // Initialize switch states from switch_init
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] >= 0)
        {
            switch_state[i] = switch_init[i];
        }
    }
}

void simulateOneTick() {
    // ========================================================================
    // PHASE 1: SPAWN
    // ========================================================================
    // Align trains scheduled for this tick (see spawn rules)
    spawnTrainsForTick();
    
    // ========================================================================
    // PHASE 2: ROUTE DETERMINATION
    // ========================================================================
    // Each train computes its next tile from current heading & switch's current state
    determineAllRoutes();
    
    // Apply emergency halt (prevents trains in 3x3 zone from moving)
    applyEmergencyHalt();
    
    // ========================================================================
    // PHASE 3: COUNTER UPDATE
    // ========================================================================
    // Entering a switch increments its counter(s)
    updateSwitchCounters();
    
    // ========================================================================
    // PHASE 4: FLIP QUEUE
    // ========================================================================
    // Switches at K are flagged to toggle
    queueSwitchFlips();
    
    // ========================================================================
    // PHASE 5: MOVEMENT
    // ========================================================================
    // All trains advance simultaneously; detect invalid moves/collisions 
    // using distance-based priority
    moveAllTrains();
    
    // Apply deferred flips after movement (ensures deterministic behavior)
    // This happens as part of Phase 5 but after trains have moved
    applyDeferredFlips();
    
    // ========================================================================
    // PHASE 6: ARRIVALS
    // ========================================================================
    // Record station arrivals
    checkArrivals();
    
    // Update emergency halt timer
    updateEmergencyHalt();
    
    // ========================================================================
    // PHASE 7: TERMINAL OUTPUT
    // ========================================================================
    // Print complete grid state to terminal showing all tiles and train positions
    printGrid();
    
    // Update signal lights for next tick
    updateSignalLights();
    
    // Log data for evidence files
    logTrainTrace();
    logSwitchState();
    logSignalState();
}

// ----------------------------------------------------------------------------
// CHECK IF SIMULATION IS COMPLETE
// ----------------------------------------------------------------------------
// Returns true if all trains are delivered or crashed AND no more trains
// are scheduled to spawn (current tick or future).
// ----------------------------------------------------------------------------
bool isSimulationComplete() {
    // Check if there are any active trains or trains scheduled to spawn
    for (int i = 0; i < total_trains; i++)
    {
        // If train is active, simulation continues
        if (train_active[i])
        {
            finished = false;
            return false;
        }
        
        // If train is scheduled to spawn at current tick or later, simulation continues
        if (train_spawn_tick[i] >= currentTick)
        {
            finished = false;
            return false;
        }
    }
    
    // All trains have either arrived/crashed and no more are scheduled to spawn
    finished = true;
    return true;
}