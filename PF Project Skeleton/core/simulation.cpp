#include "simulation.h"
#include "simulation_state.h"
#include "trains.h"
#include "switches.h"
#include "io.h"
#include "grid.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;

// Simulation logic and tick system
int calculateManhattanDistance(int x1, int y1, int x2, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

// Initialize simulation
void initializeSimulation() {
    srand(level_seed);
    
    bool should_reassign_spawn_ticks = (level_filename.find("complex_network") != string::npos || 
                                         level_filename.find("easy_level") != string::npos);
    int train_order[max_trains];
    for (int i = 0; i < total_trains; i++)
        train_order[i] = i;
    
    // Simple bubble sort by spawn row (x coordinate), then by original spawn tick for same row
    for (int i = 0; i < total_trains - 1; i++)
    {
        for (int j = 0; j < total_trains - i - 1; j++)
        {
            int row_j = train_x[train_order[j]];
            int row_j1 = train_x[train_order[j + 1]];
            if (row_j > row_j1 || (row_j == row_j1 && train_spawn_tick[train_order[j]] > train_spawn_tick[train_order[j + 1]]))
            {
                int temp = train_order[j];
                train_order[j] = train_order[j + 1];
                train_order[j + 1] = temp;
            }
        }
    }
    
    // Reassign spawn ticks ONLY for complex and easy levels
    // For medium and hard levels, keep original spawn ticks from level file
    if (should_reassign_spawn_ticks)
    {
        // Find unique rows
        int unique_rows[max_trains];
        int row_count = 0;
        for (int i = 0; i < total_trains; i++)
        {
            int train_id = train_order[i];
            int train_row = train_x[train_id];
            bool found = false;
            for (int j = 0; j < row_count; j++)
            {
                if (unique_rows[j] == train_row)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                unique_rows[row_count] = train_row;
                row_count++;
            }
        }
        
        // Round-robin: one train from each row, then cycle back
        int current_tick = 0;
        int train_idx_per_row[max_trains];
        for (int i = 0; i < row_count; i++)
            train_idx_per_row[i] = 0;
        
        int trains_assigned = 0;
        int round = 0;
        
        while (trains_assigned < total_trains)
        {
            for (int r = 0; r < row_count && trains_assigned < total_trains; r++)
            {
                int target_row = unique_rows[r];
                
                // Find next train from this row
                for (int i = 0; i < total_trains; i++)
                {
                    int train_id = train_order[i];
                    if (train_x[train_id] == target_row)
                    {
                        int row_idx = 0;
                        for (int j = 0; j < i; j++)
                        {
                            if (train_x[train_order[j]] == target_row)
                                row_idx++;
                        }
                        
                        if (row_idx == train_idx_per_row[r])
                        {
                            if (round == 0 && r == 0)
                            {
                                train_spawn_tick[train_id] = 0;
                                current_tick = 0;
                            }
                            else
                            {
                                current_tick += 4;
                                train_spawn_tick[train_id] = current_tick;
                            }
                            train_idx_per_row[r]++;
                            trains_assigned++;
                            break;
                        }
                    }
                }
            }
            round++;
        }
    }
    for (int i = 0; i < total_trains; i++)
    {
        int train_id = train_order[i];
        if (total_destinations > 0)
        {
            // Distribute destinations: use modulo to cycle through all destinations
            // This ensures all destinations (including bottom) are used
            int dest_idx = i % total_destinations;
            train_dest_x[train_id] = dest_X[dest_idx];
            train_dest_y[train_id] = dest_Y[dest_idx];
            
            // IMPORTANT: Ensure destination is not the same as spawn position
            // If it is, find a different destination
            if (train_dest_x[train_id] == train_x[train_id] && train_dest_y[train_id] == train_y[train_id])
            {
                // Destination matches spawn - find a different one
                for (int d = 0; d < total_destinations; d++)
                {
                    int alt_dest_idx = (dest_idx + d + 1) % total_destinations;
                    if (dest_X[alt_dest_idx] != train_x[train_id] || dest_Y[alt_dest_idx] != train_y[train_id])
                    {
                        train_dest_x[train_id] = dest_X[alt_dest_idx];
                        train_dest_y[train_id] = dest_Y[alt_dest_idx];
                        break;
                    }
                }
            }
            
            // Verify destination is valid (should always be, but double-check)
            if (train_dest_x[train_id] < 0 || train_dest_y[train_id] < 0)
            {
                // Fallback: assign first available destination that's not spawn position
                for (int d = 0; d < total_destinations; d++)
                {
                    if (dest_X[d] != train_x[train_id] || dest_Y[d] != train_y[train_id])
                    {
                        train_dest_x[train_id] = dest_X[d];
                        train_dest_y[train_id] = dest_Y[d];
                        break;
                    }
                }
                // If all destinations match spawn (shouldn't happen), use first one anyway
                if (train_dest_x[train_id] < 0 || train_dest_y[train_id] < 0)
                {
                    train_dest_x[train_id] = dest_X[0];
                    train_dest_y[train_id] = dest_Y[0];
                }
            }
        }
        else
        {
            bool found_dest = false;
            for (int r = 0; r < rows && !found_dest; r++)
            {
                for (int c = 0; c < cols && !found_dest; c++)
                {
                    if (grid[r][c] == 'D')
                    {
                        train_dest_x[train_id] = r;
                        train_dest_y[train_id] = c;
                        found_dest = true;
                    }
                }
            }
            if (!found_dest)
            {
                train_dest_x[train_id] = train_x[train_id];
                train_dest_y[train_id] = train_y[train_id];
            }
        }
    }
    
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] >= 0)
        {
            switch_state[i] = switch_init[i];
        }
    }
}

// Run one simulation tick
void simulateOneTick() {
    spawnTrainsForTick();
    determineAllRoutes();
    applyEmergencyHalt();
    updateSwitchCounters();
    queueSwitchFlips();
    moveAllTrains();
    applyDeferredFlips();
    checkArrivals();
    updateEmergencyHalt();
    printGrid();
    updateSignalLights();
    logTrainTrace();
    logSwitchState();
    logSignalState();
}

// Check if simulation is complete
bool isSimulationComplete() {
    for (int i = 0; i < total_trains; i++)
    {
        if (train_active[i])
        {
            finished = false;
            return false;
        }
        
        if (train_spawn_tick[i] >= currentTick)
        {
            finished = false;
            return false;
        }
    }
    
    finished = true;
    return true;
}