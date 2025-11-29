#include "trains.h"
#include "simulation_state.h"
#include "grid.h"
#include "switches.h"
#include <cstdlib>
#include <iostream>
using namespace std;

// ============================================================================
// TRAINS.CPP - Train logic
// ============================================================================

// ----------------------------------------------------------------------------
// HELPER: Calculate Manhattan distance from train to its destination
// ----------------------------------------------------------------------------
int calculateDistanceToDestination(int train_id)
{
    if (train_dest_x[train_id] < 0 || train_dest_y[train_id] < 0)
        return 0;
    
    return abs(train_x[train_id] - train_dest_x[train_id]) + 
           abs(train_y[train_id] - train_dest_y[train_id]);
}

// ----------------------------------------------------------------------------
// SPAWN TRAINS FOR CURRENT TICK (Phase 1)
// ----------------------------------------------------------------------------
// Activate trains scheduled for this tick.
// If spawn tile is occupied, train waits and retries next tick.
// ----------------------------------------------------------------------------
void spawnTrainsForTick() {
    for (int i = 0; i < total_trains; i++)
    {
        // Check if this train is scheduled to spawn at current tick
        if (train_spawn_tick[i] == currentTick && !train_active[i])
        {
            int spawn_x_pos = train_x[i];
            int spawn_y_pos = train_y[i];
            
            // Check if spawn tile is occupied
            bool spawn_occupied = false;
            for (int j = 0; j < total_trains; j++)
            {
                if (train_active[j] && train_x[j] == spawn_x_pos && train_y[j] == spawn_y_pos)
                {
                    spawn_occupied = true;
                    break;
                }
            }
            
            // Spawn Rules:
            // - If spawn tile is occupied, train waits and retries next tick
            // - If spawn tile is free but next tile is invalid, train still spawns
            //   (will crash only if it attempts invalid move on movement phase)
            if (!spawn_occupied)
            {
                // Spawn the train - don't validate the tile type
                // Per spec: train spawns even if next tile is invalid (will crash on movement)
                train_active[i] = true;
                
                // Initialize next position to current position (will be updated in Phase 2)
                // This prevents crashes from uninitialized values
                train_next_x[i] = spawn_x_pos;
                train_next_y[i] = spawn_y_pos;
                train_next_dir[i] = train_dir[i];
            }
            // Otherwise, train will retry next tick (spawn_tick stays the same)
        }
    }
}

// ----------------------------------------------------------------------------
// GET NEXT DIRECTION based on current tile and direction
// ----------------------------------------------------------------------------
// Return new direction after entering the tile.
// Handles: -, |, /, \, +, switches, safety buffers
// ----------------------------------------------------------------------------
int getNextDirection(int train_id)
{
    int x = train_x[train_id];
    int y = train_y[train_id];
    int dir = train_dir[train_id];
    
    if (!isInBounds(x, y))
        return dir; // Keep current direction if out of bounds
    
    char tile = grid[x][y];
    
    // Spawn point 'S' - allow train to continue in its initial direction
    if (tile == 'S')
    {
        return dir; // Keep initial direction when on spawn point
    }
    
    // Safety buffer (=): doesn't change direction, just delays
    if (tile == '=')
    {
        return dir; // Keep direction
    }
    
    // Horizontal track
    if (tile == '-')
    {
        if (dir == DIR_LEFT || dir == DIR_RIGHT)
            return dir; // Continue in same direction
        // If direction doesn't match horizontal track, default to RIGHT
        // This prevents crashes when train spawns with wrong direction
        return DIR_RIGHT;
    }
    
    // Vertical track
    if (tile == '|')
    {
        if (dir == DIR_UP || dir == DIR_DOWN)
            return dir; // Continue in same direction
        // If direction doesn't match vertical track, default to DOWN
        // This prevents crashes when train spawns with wrong direction
        return DIR_DOWN;
    }
    
    // Handle empty spaces or invalid tiles - try to find a valid direction
    if (tile == ' ' || tile == '.')
    {
        // Try to find adjacent valid track tiles
        // Check right
        if (isInBounds(x, y + 1) && (grid[x][y + 1] == '-' || grid[x][y + 1] == '+' || 
            grid[x][y + 1] == '=' || isSwitchTile(grid[x][y + 1]) || grid[x][y + 1] == 'D'))
            return DIR_RIGHT;
        // Check left
        if (isInBounds(x, y - 1) && (grid[x][y - 1] == '-' || grid[x][y - 1] == '+' || 
            grid[x][y - 1] == '=' || isSwitchTile(grid[x][y - 1]) || grid[x][y - 1] == 'D'))
            return DIR_LEFT;
        // Check down
        if (isInBounds(x + 1, y) && (grid[x + 1][y] == '|' || grid[x + 1][y] == '+' || 
            grid[x + 1][y] == '=' || isSwitchTile(grid[x + 1][y]) || grid[x + 1][y] == 'D'))
            return DIR_DOWN;
        // Check up
        if (isInBounds(x - 1, y) && (grid[x - 1][y] == '|' || grid[x - 1][y] == '+' || 
            grid[x - 1][y] == '=' || isSwitchTile(grid[x - 1][y]) || grid[x - 1][y] == 'D'))
            return DIR_UP;
        // Default to original direction if no valid adjacent tile found
        return dir;
    }
    
    // Curve: / (forward slash)
    if (tile == '/')
    {
        if (dir == DIR_RIGHT) return DIR_UP;
        if (dir == DIR_DOWN) return DIR_LEFT;
        if (dir == DIR_UP) return DIR_RIGHT;
        if (dir == DIR_LEFT) return DIR_DOWN;
    }
    
    // Curve: \ (backslash)
    if (tile == '\\')
    {
        if (dir == DIR_RIGHT) return DIR_DOWN;
        if (dir == DIR_UP) return DIR_LEFT;
        if (dir == DIR_DOWN) return DIR_RIGHT;
        if (dir == DIR_LEFT) return DIR_UP;
    }
    
    // Crossing: choose direction toward destination
    if (tile == '+')
    {
        return getSmartDirectionAtCrossing(train_id);
    }
    
    // Switch: route based on switch state
    if (isSwitchTile(tile))
    {
        int switch_idx = getSwitchIndex(tile);
        if (switch_idx >= 0 && switch_idx < max_switches)
        {
            // For now, keep current direction (can be enhanced with switch state routing)
            return dir;
        }
    }
    
    // Handle empty spaces or invalid tiles - try to find a valid direction
    if (tile == ' ' || tile == '.')
    {
        // Try to find adjacent valid track tiles based on current direction preference
        // First try the current direction
        int check_x = x, check_y = y;
        if (dir == DIR_RIGHT) check_y++;
        else if (dir == DIR_LEFT) check_y--;
        else if (dir == DIR_DOWN) check_x++;
        else if (dir == DIR_UP) check_x--;
        
        if (isInBounds(check_x, check_y))
        {
            char adj_tile = grid[check_x][check_y];
            if (isTrackTile(adj_tile) || adj_tile == 'D' || adj_tile == 'S' || 
                isSwitchTile(adj_tile) || adj_tile == '=' || adj_tile == '+')
                return dir; // Can move in current direction
        }
        
        // If current direction doesn't work, try to find any valid adjacent tile
        // Check right
        if (isInBounds(x, y + 1))
        {
            char adj_tile = grid[x][y + 1];
            if (isTrackTile(adj_tile) || adj_tile == 'D' || adj_tile == 'S' || 
                isSwitchTile(adj_tile) || adj_tile == '=' || adj_tile == '+')
                return DIR_RIGHT;
        }
        // Check left
        if (isInBounds(x, y - 1))
        {
            char adj_tile = grid[x][y - 1];
            if (isTrackTile(adj_tile) || adj_tile == 'D' || adj_tile == 'S' || 
                isSwitchTile(adj_tile) || adj_tile == '=' || adj_tile == '+')
                return DIR_LEFT;
        }
        // Check down
        if (isInBounds(x + 1, y))
        {
            char adj_tile = grid[x + 1][y];
            if (isTrackTile(adj_tile) || adj_tile == 'D' || adj_tile == 'S' || 
                isSwitchTile(adj_tile) || adj_tile == '=' || adj_tile == '+')
                return DIR_DOWN;
        }
        // Check up
        if (isInBounds(x - 1, y))
        {
            char adj_tile = grid[x - 1][y];
            if (isTrackTile(adj_tile) || adj_tile == 'D' || adj_tile == 'S' || 
                isSwitchTile(adj_tile) || adj_tile == '=' || adj_tile == '+')
                return DIR_UP;
        }
    }
    
    // Default: keep current direction
    return dir;
}

// ----------------------------------------------------------------------------
// SMART ROUTING AT CROSSING - Route train to its matched destination
// ----------------------------------------------------------------------------
// Choose best direction at '+' toward destination using Manhattan distance.
// ----------------------------------------------------------------------------
int getSmartDirectionAtCrossing(int train_id)
{
    int x = train_x[train_id];
    int y = train_y[train_id];
    int dest_x = train_dest_x[train_id];
    int dest_y = train_dest_y[train_id];
    
    if (dest_x < 0 || dest_y < 0)
        return train_dir[train_id]; // No destination, keep current direction
    
    // Calculate distances for each possible direction
    int dist_up = abs(x - 1 - dest_x) + abs(y - dest_y);
    int dist_right = abs(x - dest_x) + abs(y + 1 - dest_y);
    int dist_down = abs(x + 1 - dest_x) + abs(y - dest_y);
    int dist_left = abs(x - dest_x) + abs(y - 1 - dest_y);
    
    // Find direction with minimum distance
    int min_dist = dist_up;
    int best_dir = DIR_UP;
    
    if (dist_right < min_dist) { min_dist = dist_right; best_dir = DIR_RIGHT; }
    if (dist_down < min_dist) { min_dist = dist_down; best_dir = DIR_DOWN; }
    if (dist_left < min_dist) { min_dist = dist_left; best_dir = DIR_LEFT; }
    
    return best_dir;
}

// ----------------------------------------------------------------------------
// DETERMINE NEXT POSITION for a train
// ----------------------------------------------------------------------------
// Compute next position/direction from current tile and rules.
// Returns true if move is valid.
// ----------------------------------------------------------------------------
bool determineNextPosition(int train_id)
{
    if (!train_active[train_id])
        return false;
    
    int x = train_x[train_id];
    int y = train_y[train_id];
    int dir = train_dir[train_id];
    
    // Check if train is already at its destination - don't move, will be marked as arrived in Phase 6
    if (train_dest_x[train_id] >= 0 && train_dest_y[train_id] >= 0)
    {
        if (x == train_dest_x[train_id] && y == train_dest_y[train_id])
        {
            // Train is at destination - stay in place, will be marked as arrived
            train_next_x[train_id] = x;
            train_next_y[train_id] = y;
            train_next_dir[train_id] = dir;
            return true;
        }
    }
    
    // Check if on safety buffer - train waits one tick
    if (isInBounds(x, y) && grid[x][y] == '=')
    {
        if (train_waiting[train_id])
        {
            // Wait is over, can move now
            train_waiting[train_id] = false;
        }
        else
        {
            // Start waiting
            train_waiting[train_id] = true;
            train_next_x[train_id] = x;
            train_next_y[train_id] = y;
            train_next_dir[train_id] = dir;
            return true;
        }
    }
    
    // If still waiting, don't move
    if (train_waiting[train_id])
    {
        train_next_x[train_id] = x;
        train_next_y[train_id] = y;
        train_next_dir[train_id] = dir;
        return true;
    }
    
    // Weather effect: RAIN - occasional slowdowns (extra wait tick after n moves)
    if (weather_type == weather_rain)
    {
        if (train_rain_waiting[train_id])
        {
            // Currently waiting due to rain - skip this move
            train_rain_waiting[train_id] = false;
            train_next_x[train_id] = x;
            train_next_y[train_id] = y;
            train_next_dir[train_id] = dir;
            return true;
        }
        
        // Increment move counter
        train_rain_move_count[train_id]++;
        
        // Every 5 moves, trigger a slowdown (pseudo-random based on seed)
        if (train_rain_move_count[train_id] >= 5)
        {
            // 30% chance of slowdown (deterministic based on seed + tick + train_id)
            int rand_val = (level_seed + currentTick * 1000 + train_id * 100) % 100;
            if (rand_val < 30)
            {
                train_rain_waiting[train_id] = true;
                train_rain_move_count[train_id] = 0; // Reset counter
                train_next_x[train_id] = x;
                train_next_y[train_id] = y;
                train_next_dir[train_id] = dir;
                return true;
            }
            train_rain_move_count[train_id] = 0; // Reset counter
        }
    }
    
    // Get next direction based on current tile
    int next_dir = getNextDirection(train_id);
    
    // Calculate next position based on direction
    int next_x = x;
    int next_y = y;
    
    if (next_dir == DIR_UP) next_x--;
    else if (next_dir == DIR_RIGHT) next_y++;
    else if (next_dir == DIR_DOWN) next_x++;
    else if (next_dir == DIR_LEFT) next_y--;
    
    // Check if next position is valid
    if (!isInBounds(next_x, next_y))
    {
        // Invalid move - train will crash
        train_next_x[train_id] = next_x;
        train_next_y[train_id] = next_y;
        train_next_dir[train_id] = next_dir;
        return false;
    }
    
    // Store planned move
    train_next_x[train_id] = next_x;
    train_next_y[train_id] = next_y;
    train_next_dir[train_id] = next_dir;
    
    return true;
}

// ----------------------------------------------------------------------------
// DETERMINE ALL ROUTES (PHASE 2)
// ----------------------------------------------------------------------------
// Fill next positions/directions for all active trains.
// ----------------------------------------------------------------------------
void determineAllRoutes() {
    for (int i = 0; i < total_trains; i++)
    {
        if (train_active[i])
        {
            determineNextPosition(i);
        }
    }
}

// ----------------------------------------------------------------------------
// DETECT COLLISIONS WITH PRIORITY SYSTEM
// ----------------------------------------------------------------------------
// Resolve same-tile, swap, and crossing conflicts using distance-based priority.
// Distance Calculation: Manhattan distance (sum of absolute differences in x and y)
// from train's current position to its assigned destination point.
// Priority Rule: Train with higher distance to destination gets priority and proceeds,
// while trains with lower distance wait for the next tick.
// ----------------------------------------------------------------------------
void detectCollisions() {
    // Track which trains have been marked to wait or crash (to avoid double-processing)
    bool train_processed[max_trains];
    for (int i = 0; i < total_trains; i++)
        train_processed[i] = false;
    
    // First pass: Handle same-destination and head-on swap collisions (pairwise)
    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i] || train_processed[i]) continue;
        
        int next_x_i = train_next_x[i];
        int next_y_i = train_next_y[i];
        int dist_i = calculateDistanceToDestination(i);
        
        // Check for collisions with other trains
        for (int j = i + 1; j < total_trains; j++)
        {
            if (!train_active[j] || train_processed[j]) continue;
            
            int next_x_j = train_next_x[j];
            int next_y_j = train_next_y[j];
            int dist_j = calculateDistanceToDestination(j);
            
            // Same-destination collision: multiple trains targeting same tile
            // (This includes crossing '+' collisions)
            if (next_x_i == next_x_j && next_y_i == next_y_j)
            {
                if (dist_i > dist_j)
                {
                    // Train i has priority (higher distance), train j waits
                    train_next_x[j] = train_x[j];
                    train_next_y[j] = train_y[j];
                    train_next_dir[j] = train_dir[j];
                    train_processed[j] = true;
                }
                else if (dist_j > dist_i)
                {
                    // Train j has priority (higher distance), train i waits
                    train_next_x[i] = train_x[i];
                    train_next_y[i] = train_y[i];
                    train_next_dir[i] = train_dir[i];
                    train_processed[i] = true;
                }
                else
                {
                    // Equal distance - all involved trains crash
                    train_active[i] = false;
                    train_active[j] = false;
                    train_processed[i] = true;
                    train_processed[j] = true;
                    crashes += 2;
                }
            }
            
            // Head-on swap collision: trains swapping positions
            else if (next_x_i == train_x[j] && next_y_i == train_y[j] &&
                     next_x_j == train_x[i] && next_y_j == train_y[i])
            {
                if (dist_i > dist_j)
                {
                    // Train i has priority (higher distance), train j waits
                    train_next_x[j] = train_x[j];
                    train_next_y[j] = train_y[j];
                    train_next_dir[j] = train_dir[j];
                    train_processed[j] = true;
                }
                else if (dist_j > dist_i)
                {
                    // Train j has priority (higher distance), train i waits
                    train_next_x[i] = train_x[i];
                    train_next_y[i] = train_y[i];
                    train_next_dir[i] = train_dir[i];
                    train_processed[i] = true;
                }
                else
                {
                    // Equal distance - both crash
                    train_active[i] = false;
                    train_active[j] = false;
                    train_processed[i] = true;
                    train_processed[j] = true;
                    crashes += 2;
                }
            }
        }
    }
    
    // Second pass: Handle crossing '+' collisions with 3+ trains
    // (Pairwise check might miss some cases, so we do a comprehensive check)
    for (int target_x = 0; target_x < rows; target_x++)
    {
        for (int target_y = 0; target_y < cols; target_y++)
        {
            if (!isInBounds(target_x, target_y) || grid[target_x][target_y] != '+')
                continue;
            
            // Find all active trains targeting this crossing that haven't been processed
            int trains_targeting[max_trains];
            int count = 0;
            
            for (int i = 0; i < total_trains; i++)
            {
                if (!train_active[i] || train_processed[i]) continue;
                if (train_next_x[i] == target_x && train_next_y[i] == target_y)
                {
                    trains_targeting[count++] = i;
                }
            }
            
            // If multiple trains target this crossing, resolve using priority
            if (count > 1)
            {
                // Find train with highest distance
                int max_dist = -1;
                int priority_train = -1;
                int same_max_count = 0;
                
                for (int k = 0; k < count; k++)
                {
                    int dist = calculateDistanceToDestination(trains_targeting[k]);
                    if (dist > max_dist)
                    {
                        max_dist = dist;
                        priority_train = trains_targeting[k];
                        same_max_count = 1;
                    }
                    else if (dist == max_dist)
                    {
                        same_max_count++;
                    }
                }
                
                if (same_max_count > 1)
                {
                    // Equal distance - all involved trains crash
                    for (int k = 0; k < count; k++)
                    {
                        if (calculateDistanceToDestination(trains_targeting[k]) == max_dist)
                        {
                            train_active[trains_targeting[k]] = false;
                            train_processed[trains_targeting[k]] = true;
                            crashes++;
                        }
                    }
                }
                else
                {
                    // Train with highest distance moves, others wait
                    for (int k = 0; k < count; k++)
                    {
                        if (trains_targeting[k] != priority_train)
                        {
                            train_next_x[trains_targeting[k]] = train_x[trains_targeting[k]];
                            train_next_y[trains_targeting[k]] = train_y[trains_targeting[k]];
                            train_next_dir[trains_targeting[k]] = train_dir[trains_targeting[k]];
                            train_processed[trains_targeting[k]] = true;
                        }
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// MOVE ALL TRAINS (PHASE 5)
// ----------------------------------------------------------------------------
// Move trains; resolve collisions and apply effects.
// ----------------------------------------------------------------------------
void moveAllTrains() {
    // First, detect and resolve collisions using distance-based priority
    detectCollisions();
    
    // Now move all trains to their planned positions
    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i]) continue;
        
        // Check if move is valid (within bounds)
        int next_x = train_next_x[i];
        int next_y = train_next_y[i];
        
        // If train is staying in place (same position), allow it (might be waiting or just spawned)
        if (next_x == train_x[i] && next_y == train_y[i])
        {
            // Train is not moving this tick - this is OK (waiting, just spawned, etc.)
            // Don't crash, just update direction if needed
            train_dir[i] = train_next_dir[i];
            continue;
        }
        
        if (!isInBounds(next_x, next_y))
        {
            // Invalid move - train crashes
            train_active[i] = false;
            crashes++;
            continue;
        }
        
        // Check if target tile is a valid track tile
        char next_tile = grid[next_x][next_y];
        if (!isTrackTile(next_tile) && next_tile != 'D' && next_tile != 'S' && 
            !isSwitchTile(next_tile) && next_tile != '=')
        {
            // Invalid tile - train crashes
            train_active[i] = false;
            crashes++;
            continue;
        }
        
        // Check for signal violation: train entering switch on red signal
        if (isSwitchTile(next_tile))
        {
            int switch_idx = getSwitchIndex(next_tile);
            if (switch_idx >= 0 && switch_signal[switch_idx] == signal_red)
            {
                signal_violations++;
            }
        }
        
        // Track idle ticks (train not moving)
        if (train_x[i] == next_x && train_y[i] == next_y)
        {
            train_idle_ticks[i]++;
            total_wait_ticks++;
        }
        
        // Track total train ticks for energy efficiency
        total_train_ticks++;
        
        // Move train to next position
        train_x[i] = next_x;
        train_y[i] = next_y;
        train_dir[i] = train_next_dir[i];
        
        // If moved to safety buffer, set waiting flag
        if (next_tile == '=')
        {
            train_waiting[i] = true;
        }
    }
}

// ----------------------------------------------------------------------------
// CHECK ARRIVALS (Phase 7)
// ----------------------------------------------------------------------------
// Mark trains that reached destinations.
// ----------------------------------------------------------------------------
void checkArrivals() {
    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i]) continue;
        
        // Check if train reached its destination
        if (train_x[i] == train_dest_x[i] && train_y[i] == train_dest_y[i])
        {
            train_active[i] = false;
            arrival++;
        }
    }
}

// ----------------------------------------------------------------------------
// APPLY EMERGENCY HALT
// ----------------------------------------------------------------------------
// Apply halt to trains in the active 3x3 zone.
// ----------------------------------------------------------------------------
void applyEmergencyHalt() {
    if (!emergencyHalt) return;
    
    // Find switch with emergency halt (simplified - can be enhanced)
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue;
        
        // Check trains in 3x3 zone around switch
        for (int t = 0; t < total_trains; t++)
        {
            if (!train_active[t]) continue;
            
            int dx = abs(train_x[t] - switch_x[i]);
            int dy = abs(train_y[t] - switch_y[i]);
            
            if (dx <= 1 && dy <= 1)
            {
                // Train is in halt zone - prevent movement
                train_next_x[t] = train_x[t];
                train_next_y[t] = train_y[t];
                train_next_dir[t] = train_dir[t];
            }
        }
    }
}

// ----------------------------------------------------------------------------
// UPDATE EMERGENCY HALT
// ----------------------------------------------------------------------------
// Decrement timer and disable when done.
// ----------------------------------------------------------------------------
void updateEmergencyHalt() {
    if (emergencyHalt && emergencyHaltTimer > 0)
    {
        emergencyHaltTimer--;
        if (emergencyHaltTimer <= 0)
        {
            emergencyHalt = false;
        }
    }
}
