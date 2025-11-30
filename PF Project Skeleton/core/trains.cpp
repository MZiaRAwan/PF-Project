#include "trains.h"
#include "simulation_state.h"
#include "grid.h"
#include "switches.h"
#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

// Train movement and logic
static int last_x[max_trains] = {};
static int last_y[max_trains] = {};
static int prev_x[max_trains] = {};
static int prev_y[max_trains] = {};
static int repeat_cnt[max_trains] = {};
static int oscil_cnt[max_trains] = {};
static int last_dist[max_trains] = {};
static int no_prog_ticks[max_trains] = {};

// Calculate distance to destination
int calculateDistanceToDestination(int id)
{
    if (train_dest_x[id] < 0 || train_dest_y[id] < 0)
        return 0;
    
    return abs(train_x[id] - train_dest_x[id]) + 
           abs(train_y[id] - train_dest_y[id]);
}

// Initialize position tracking
void initializeTrainTracking(int id, int x, int y)
{
    last_x[id] = x;
    last_y[id] = y;
    prev_x[id] = -1;
    prev_y[id] = -1;
    repeat_cnt[id] = 0;
    oscil_cnt[id] = 0;
    no_prog_ticks[id] = 0;
    last_dist[id] = -1;
}

// Spawn trains for current tick
void spawnTrainsForTick() {
    int sched[max_trains];
    int cnt = 0;
    
    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i] && !train_arrived[i] && train_spawn_tick[i] <= currentTick)
        {
            sched[cnt] = i;
            cnt++;
        }
    }
    
    for (int i = 0; i < cnt - 1; i++)
    {
        for (int j = 0; j < cnt - i - 1; j++)
        {
            if (train_x[sched[j]] > train_x[sched[j + 1]])
            {
                int temp = sched[j];
                sched[j] = sched[j + 1];
                sched[j + 1] = temp;
            }
        }
    }
    
    for (int idx = 0; idx < cnt; idx++)
    {
        int i = sched[idx];
        int sx = train_x[i];
        int sy = train_y[i];
        
        bool occ = false;
        for (int j = 0; j < total_trains; j++)
        {
            if (train_active[j] && train_x[j] == sx && train_y[j] == sy)
            {
                occ = true;
                break;
            }
        }
            
        if (!occ)
            {
            // Check if spawn position is valid
            bool first_train = (currentTick == 0 && train_spawn_tick[i] == 0);
            bool med_hard = (level_filename.find("medium_level") != string::npos || 
                                      level_filename.find("hard_level") != string::npos);
            
            if (isInBounds(sx, sy))
            {
                char tile = grid[sx][sy];
                bool can_spawn = (tile == 'S' || isTrackTile(tile) || tile == '=' || 
                    isSwitchTile(tile) || tile == 'D' || tile == '+' ||
                    tile == '-' || tile == '|' || tile == '/' || tile == '\\' ||
                    (tile != ' ' && tile != '.' && tile != '\0'));
                
                if (first_train && !can_spawn && tile != ' ' && tile != '.' && tile != '\0')
                {
                    can_spawn = true;
                }
                
                if (med_hard && !can_spawn && tile != ' ' && tile != '.' && tile != '\0')
                {
                    can_spawn = true; // Force allow for medium/hard levels
                }
                
                if (can_spawn)
                {
                    // Spawn the train
                train_active[i] = true;
                    
                    // Initialize current position (important for rendering)
                    train_x[i] = sx;
                    train_y[i] = sy;
                
                // Initialize next position to current position (will be updated in Phase 2)
                // This prevents crashes from uninitialized values
                train_next_x[i] = sx;
                train_next_y[i] = sy;
                train_next_dir[i] = train_dir[i];
                    
                    // Initialize loop detection tracking
                    initializeTrainTracking(i, sx, sy);
                    
                    // Check if train spawned on its destination - will be marked as arrived in Phase 6
                    // (We don't mark it here because Phase 6 handles arrivals)
                }
                else
                {
                    // If spawn tile is not valid, try to find nearest valid track tile
                    // This handles cases where spawn coordinates might be slightly off
                    bool found_valid = false;
                    
                    // For medium and hard levels, immediately search entire grid for 'S' tiles first
                    if (med_hard)
                    {
                        int best_s_x = -1, best_s_y = -1;
                        int min_dist = 10000;
                        
                        // Search entire grid for 'S' tiles
                        for (int r = 0; r < rows; r++)
                        {
                            for (int c = 0; c < cols; c++)
                            {
                                if (grid[r][c] == 'S')
                                {
                                    // Check if this spawn tile is not occupied
                                    bool s_occupied = false;
                                    for (int j = 0; j < total_trains; j++)
                                    {
                                        if (train_active[j] && train_x[j] == r && train_y[j] == c)
                                        {
                                            s_occupied = true;
                                            break;
                                        }
                                    }
                                    
                                    if (!s_occupied)
                                    {
                                        // Calculate distance to original spawn position
                                        int dist = abs(r - sx) + abs(c - sy);
                                        if (dist < min_dist)
                                        {
                                            min_dist = dist;
                                            best_s_x = r;
                                            best_s_y = c;
                                        }
                                    }
                                }
                            }
                        }
                        
                        // If found a valid 'S' tile, use it
                        if (best_s_x >= 0 && best_s_y >= 0)
                        {
                            train_active[i] = true;
                            train_x[i] = best_s_x;
                            train_y[i] = best_s_y;
                            train_next_x[i] = best_s_x;
                            train_next_y[i] = best_s_y;
                            train_next_dir[i] = train_dir[i];
                            found_valid = true;
                        }
                    }
                    
                    // If no 'S' tile found, try nearby tiles
                    if (!found_valid)
                    {
                        for (int dx = -1; dx <= 1 && !found_valid; dx++)
                        {
                            for (int dy = -1; dy <= 1 && !found_valid; dy++)
                            {
                                int check_x = sx + dx;
                                int check_y = sy + dy;
                                if (isInBounds(check_x, check_y))
                                {
                                    char check_tile = grid[check_x][check_y];
                                    // Be VERY lenient: allow spawning on any non-empty tile
                                    bool can_spawn_here = (check_tile == 'S' || isTrackTile(check_tile) || check_tile == '=' || 
                                        isSwitchTile(check_tile) || check_tile == 'D' || check_tile == '+' ||
                                        check_tile == '-' || check_tile == '|' || check_tile == '/' || check_tile == '\\' ||
                                        (check_tile != ' ' && check_tile != '.' && check_tile != '\0'));
                                    
                                    // For first train at tick 0, allow spawning on ANY non-empty tile
                                    if (first_train && !can_spawn_here && check_tile != ' ' && check_tile != '.' && check_tile != '\0')
                                    {
                                        can_spawn_here = true;
                                    }
                                    
                                    // For medium and hard levels, allow spawning on ANY non-empty tile
                                    if (med_hard && !can_spawn_here && check_tile != ' ' && check_tile != '.' && check_tile != '\0')
                                    {
                                        can_spawn_here = true;
                                    }
                                    
                                    // Check if not occupied
                                    if (can_spawn_here)
                                    {
                                        bool tile_occupied = false;
                                        for (int j = 0; j < total_trains; j++)
                                        {
                                            if (train_active[j] && train_x[j] == check_x && train_y[j] == check_y)
                                            {
                                                tile_occupied = true;
                                                break;
                                            }
                                        }
                                        
                                        if (!tile_occupied)
                                        {
                                            train_active[i] = true;
                                            train_x[i] = check_x;
                                            train_y[i] = check_y;
                                            train_next_x[i] = check_x;
                                            train_next_y[i] = check_y;
                                            train_next_dir[i] = train_dir[i];
                                            found_valid = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    // If still no valid tile found, try even wider search (±5 tiles) for medium/hard levels
                    if (!found_valid && med_hard)
                    {
                        for (int dx = -5; dx <= 5 && !found_valid; dx++)
                        {
                            for (int dy = -5; dy <= 5 && !found_valid; dy++)
                            {
                                int check_x = sx + dx;
                                int check_y = sy + dy;
                                if (isInBounds(check_x, check_y))
                                {
                                    char check_tile = grid[check_x][check_y];
                                    if (check_tile == 'S' || isTrackTile(check_tile) || check_tile == '=' || 
                                        isSwitchTile(check_tile) || check_tile == '+' ||
                                        check_tile == '-' || check_tile == '|' || check_tile == '/' || check_tile == '\\')
                                    {
                                        // Check if not occupied
                                        bool tile_occupied = false;
                                        for (int j = 0; j < total_trains; j++)
                                        {
                                            if (train_active[j] && train_x[j] == check_x && train_y[j] == check_y)
                                            {
                                                tile_occupied = true;
                                                break;
                                            }
                                        }
                                        
                                        if (!tile_occupied)
                                        {
                                            train_active[i] = true;
                                            train_x[i] = check_x;
                                            train_y[i] = check_y;
                                            train_next_x[i] = check_x;
                                            train_next_y[i] = check_y;
                                            train_next_dir[i] = train_dir[i];
                                            found_valid = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // If still no valid tile found, try even wider search (±3 tiles) for other levels
                    else if (!found_valid)
                    {
                        for (int dx = -3; dx <= 3 && !found_valid; dx++)
                        {
                            for (int dy = -3; dy <= 3 && !found_valid; dy++)
                            {
                                int check_x = sx + dx;
                                int check_y = sy + dy;
                                if (isInBounds(check_x, check_y))
                                {
                                    char check_tile = grid[check_x][check_y];
                                    if (check_tile != ' ' && check_tile != '.' && check_tile != '\0')
                                    {
                                        train_active[i] = true;
                                        train_x[i] = check_x;
                                        train_y[i] = check_y;
                                        train_next_x[i] = check_x;
                                        train_next_y[i] = check_y;
                                        train_next_dir[i] = train_dir[i];
                                        found_valid = true;
                                    }
                                }
                            }
                        }
                    }
                    
                    // For medium and hard levels, if still not found, search entire grid for ANY valid track tile
                    if (!found_valid && med_hard)
                    {
                        for (int r = 0; r < rows && !found_valid; r++)
                        {
                            for (int c = 0; c < cols && !found_valid; c++)
                            {
                                char check_tile = grid[r][c];
                                if (check_tile == 'S' || isTrackTile(check_tile) || check_tile == '=' || 
                                    isSwitchTile(check_tile) || check_tile == '+' ||
                                    check_tile == '-' || check_tile == '|' || check_tile == '/' || check_tile == '\\')
                                {
                                    // Check if not occupied
                                    bool tile_occupied = false;
                                    for (int j = 0; j < total_trains; j++)
                                    {
                                        if (train_active[j] && train_x[j] == r && train_y[j] == c)
                                        {
                                            tile_occupied = true;
                                            break;
                                        }
                                    }
                                    
                                    if (!tile_occupied)
                                    {
                                        train_active[i] = true;
                                        train_x[i] = r;
                                        train_y[i] = c;
                                        train_next_x[i] = r;
                                        train_next_y[i] = c;
                                        train_next_dir[i] = train_dir[i];
                                        found_valid = true;
                                    }
                                }
                            }
                        }
                    }
                    
                    // If still no valid tile found and it's first train at tick 0, force spawn at original position
                    if (!found_valid && first_train)
                    {
                        // Force spawn for first train - it must spawn at tick 0
                        train_active[i] = true;
                        train_x[i] = sx;
                        train_y[i] = sy;
                        train_next_x[i] = sx;
                        train_next_y[i] = sy;
                        train_next_dir[i] = train_dir[i];
                    }
                    // For medium and hard levels, force spawn at original position if all else fails
                    // This ensures ALL trains eventually spawn
                    else if (!found_valid && med_hard)
                    {
                        // Force spawn for medium/hard levels - ensure all trains spawn
                        train_active[i] = true;
                        train_x[i] = sx;
                        train_y[i] = sy;
                        train_next_x[i] = sx;
                        train_next_y[i] = sy;
                        train_next_dir[i] = train_dir[i];
                    }
                }
            }
            else
            {
                // Train is out of bounds - try to find ANY valid position nearby
                // This applies to first train at tick 0, and also to medium/hard levels
                bool found_out_of_bounds = false;
                
                // First, try to find nearest 'S' spawn tile (preferred for medium/hard levels)
                if (med_hard)
                {
                    int best_s_x = -1, best_s_y = -1;
                    int min_dist = 10000;
                    
                    // Search entire grid for 'S' tiles
                    for (int r = 0; r < rows; r++)
                    {
                        for (int c = 0; c < cols; c++)
                        {
                            if (grid[r][c] == 'S')
                            {
                                // Check if this spawn tile is not occupied
                                bool s_occupied = false;
                                for (int j = 0; j < total_trains; j++)
                                {
                                    if (train_active[j] && train_x[j] == r && train_y[j] == c)
                                    {
                                        s_occupied = true;
                                        break;
                                    }
                                }
                                
                                if (!s_occupied)
                                {
                                    // Calculate distance to original spawn position
                                    int dist = abs(r - sx) + abs(c - sy);
                                    if (dist < min_dist)
                                    {
                                        min_dist = dist;
                                        best_s_x = r;
                                        best_s_y = c;
                                    }
                                }
                            }
                        }
                    }
                    
                    // If found a valid 'S' tile, use it
                    if (best_s_x >= 0 && best_s_y >= 0)
                    {
                        train_active[i] = true;
                        train_x[i] = best_s_x;
                        train_y[i] = best_s_y;
                        train_next_x[i] = best_s_x;
                        train_next_y[i] = best_s_y;
                        train_next_dir[i] = train_dir[i];
                        found_out_of_bounds = true;
                    }
                }
                
                // If no 'S' tile found, try to find ANY valid position nearby
                if (!found_out_of_bounds)
                {
                    for (int dx = -5; dx <= 5 && !found_out_of_bounds; dx++)
                    {
                        for (int dy = -5; dy <= 5 && !found_out_of_bounds; dy++)
                        {
                            int check_x = sx + dx;
                            int check_y = sy + dy;
                            if (isInBounds(check_x, check_y))
                            {
                                char check_tile = grid[check_x][check_y];
                                // Prefer 'S' tiles, then track tiles
                                if (check_tile == 'S' || isTrackTile(check_tile) || check_tile == '=' || 
                                    isSwitchTile(check_tile) || check_tile == '+' ||
                                    check_tile == '-' || check_tile == '|' || check_tile == '/' || check_tile == '\\')
                                {
                                    // Check if not occupied
                                    bool tile_occupied = false;
                                    for (int j = 0; j < total_trains; j++)
                                    {
                                        if (train_active[j] && train_x[j] == check_x && train_y[j] == check_y)
                                        {
                                            tile_occupied = true;
                                            break;
                                        }
                                    }
                                    
                                    if (!tile_occupied)
                                    {
                                        train_active[i] = true;
                                        train_x[i] = check_x;
                                        train_y[i] = check_y;
                                        train_next_x[i] = check_x;
                                        train_next_y[i] = check_y;
                                        train_next_dir[i] = train_dir[i];
                                        found_out_of_bounds = true;
                                    }
                                }
                            }
                        }
                    }
                }
                
                // For medium and hard levels, if still not found, search for ANY non-empty tile
                if (!found_out_of_bounds && med_hard)
                {
                    for (int r = 0; r < rows && !found_out_of_bounds; r++)
                    {
                        for (int c = 0; c < cols && !found_out_of_bounds; c++)
                        {
                            char check_tile = grid[r][c];
                            if (check_tile != ' ' && check_tile != '.' && check_tile != '\0')
                            {
                                // Check if not occupied
                                bool tile_occupied = false;
                                for (int j = 0; j < total_trains; j++)
                                {
                                    if (train_active[j] && train_x[j] == r && train_y[j] == c)
                                    {
                                        tile_occupied = true;
                                        break;
                                    }
                                }
                                
                                if (!tile_occupied)
                                {
                                    train_active[i] = true;
                                    train_x[i] = r;
                                    train_y[i] = c;
                                    train_next_x[i] = r;
                                    train_next_y[i] = c;
                                    train_next_dir[i] = train_dir[i];
                                    found_out_of_bounds = true;
                                }
                            }
                        }
                    }
                }
            }
            // If out of bounds and not first train and not medium/hard, don't spawn (will retry next tick)
        }
        // Otherwise, train will retry next tick (spawn_tick stays the same)
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
    
    // Switch: route based on switch state (STRAIGHT or TURN)
    if (isSwitchTile(tile))
    {
        int switch_idx = getSwitchIndex(tile);
        if (switch_idx >= 0 && switch_idx < max_switches)
        {
            int state = switch_state[switch_idx];
            string state_label = (state == 0) ? switch_state0[switch_idx] : switch_state1[switch_idx];
            
            // If STRAIGHT: continue in current direction
            if (state_label == "STRAIGHT")
            {
            return dir;
        }
            // If TURN: change direction based on entry direction and destination
            else if (state_label == "TURN")
            {
                // For horizontal switches (most common), if entering from left/right:
                if (dir == DIR_RIGHT || dir == DIR_LEFT)
                {
                    // Check destination to decide turn direction
                    int dest_x = train_dest_x[train_id];
                    int dest_y = train_dest_y[train_id];
                    
                    if (dest_x >= 0 && dest_y >= 0)
                    {
                        // If destination is below, turn down; if above, turn up
                        // Otherwise, prefer turning toward destination
                        if (dest_x > x)
                            return DIR_DOWN;  // Turn down toward destination
                        else if (dest_x < x)
                            return DIR_UP;    // Turn up toward destination
                        // If same row, check if we should continue straight or turn
                        // For now, turn down by default (can be enhanced)
                        return DIR_DOWN;
                    }
                    // No destination, default turn direction
                    return DIR_DOWN;
                }
                // For vertical switches, if entering from up/down:
                else if (dir == DIR_UP || dir == DIR_DOWN)
                {
                    int dest_x = train_dest_x[train_id];
                    int dest_y = train_dest_y[train_id];
                    
                    if (dest_x >= 0 && dest_y >= 0)
                    {
                        // If destination is to the right, turn right; if left, turn left
                        if (dest_y > y)
                            return DIR_RIGHT;  // Turn right toward destination
                        else if (dest_y < y)
                            return DIR_LEFT;   // Turn left toward destination
                        // If same column, default turn right
                        return DIR_RIGHT;
                    }
                    // No destination, default turn direction
                    return DIR_RIGHT;
                }
            }
            // Default: keep current direction if state label is unknown
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
    
    if (train_arrived[train_id])
    {
        train_next_x[train_id] = train_x[train_id];
        train_next_y[train_id] = train_y[train_id];
        train_next_dir[train_id] = train_dir[train_id];
        return true;
    }
    
    int x = train_x[train_id];
    int y = train_y[train_id];
    int dir = train_dir[train_id];
    
    if (train_dest_x[train_id] < 0 || train_dest_y[train_id] < 0)
    {
        train_next_x[train_id] = x;
        train_next_y[train_id] = y;
        train_next_dir[train_id] = dir;
        return true;
    }
    
    if (x == train_dest_x[train_id] && y == train_dest_y[train_id])
    {
        train_next_x[train_id] = x;
        train_next_y[train_id] = y;
        train_next_dir[train_id] = dir;
        train_arrived[train_id] = true;
        return true;
    }
    int dist_to_dest = abs(x - train_dest_x[train_id]) + abs(y - train_dest_y[train_id]);
    if (dist_to_dest == 1)
    {
        // Train is one step away from destination - force move toward destination
        int dx = train_dest_x[train_id] - x;
        int dy = train_dest_y[train_id] - y;
        
        // Determine direction toward destination
        int target_dir = dir;
        if (dx > 0) target_dir = DIR_DOWN;
        else if (dx < 0) target_dir = DIR_UP;
        else if (dy > 0) target_dir = DIR_RIGHT;
        else if (dy < 0) target_dir = DIR_LEFT;
        
        // Check if we can move in that direction
        int check_x = x, check_y = y;
        if (target_dir == DIR_UP) check_x--;
        else if (target_dir == DIR_RIGHT) check_y++;
        else if (target_dir == DIR_DOWN) check_x++;
        else if (target_dir == DIR_LEFT) check_y--;
        
        if (isInBounds(check_x, check_y) && 
            (check_x == train_dest_x[train_id] && check_y == train_dest_y[train_id]))
        {
            // Can move directly to destination - do it
            train_next_x[train_id] = check_x;
            train_next_y[train_id] = check_y;
            train_next_dir[train_id] = target_dir;
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
    
    // Check if train is very close to destination - prioritize direct movement
    int current_dist = abs(x - train_dest_x[train_id]) + abs(y - train_dest_y[train_id]);
    int next_dir;
    
    if (current_dist <= 6 && train_dest_x[train_id] >= 0 && train_dest_y[train_id] >= 0)
    {
        // Train is close to destination - prioritize moving directly toward it
        int dx = train_dest_x[train_id] - x;
        int dy = train_dest_y[train_id] - y;
        
        // Determine best direction toward destination
        if (abs(dx) > abs(dy))
        {
            if (dx > 0) next_dir = DIR_DOWN;
            else next_dir = DIR_UP;
        }
        else
        {
            if (dy > 0) next_dir = DIR_RIGHT;
            else next_dir = DIR_LEFT;
        }
        
        // Check if we can move in that direction
        int check_x = x, check_y = y;
        if (next_dir == DIR_UP) check_x--;
        else if (next_dir == DIR_RIGHT) check_y++;
        else if (next_dir == DIR_DOWN) check_x++;
        else if (next_dir == DIR_LEFT) check_y--;
        
        // If the move is valid and moves toward destination, use it
        if (isInBounds(check_x, check_y))
        {
            char check_tile = grid[check_x][check_y];
            int new_dist = abs(check_x - train_dest_x[train_id]) + abs(check_y - train_dest_y[train_id]);
            
            if ((isTrackTile(check_tile) || check_tile == 'D' || check_tile == 'S' || 
                 isSwitchTile(check_tile) || check_tile == '=' || check_tile == '+') && 
                new_dist < current_dist)
            {
                // Valid move toward destination - use it
                train_next_x[train_id] = check_x;
                train_next_y[train_id] = check_y;
                train_next_dir[train_id] = next_dir;
                return true;
            }
        }
    }
    
    // Get next direction based on current tile
    next_dir = getNextDirection(train_id);
    
    // Calculate next position based on direction
    int next_x = x;
    int next_y = y;
    
    if (next_dir == DIR_UP) next_x--;
    else if (next_dir == DIR_RIGHT) next_y++;
    else if (next_dir == DIR_DOWN) next_x++;
    else if (next_dir == DIR_LEFT) next_y--;
    
    // CRITICAL: Double-check if train is at destination - if so, force stay in place
    // This prevents any edge case where train might calculate a move away from destination
    if (x == train_dest_x[train_id] && y == train_dest_y[train_id] && train_dest_x[train_id] >= 0 && train_dest_y[train_id] >= 0)
    {
        // Train is at destination - force next position to be destination (stay in place)
        train_next_x[train_id] = x;
        train_next_y[train_id] = y;
        train_next_dir[train_id] = dir;
        return true;
    }
    
    // Check if next position is valid
    if (!isInBounds(next_x, next_y))
    {
        // Invalid move - instead of crashing, try to find a valid adjacent direction
        // Check all 4 directions to find a valid path
        int dirs[4] = {DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT};
        bool found_valid = false;
        
        for (int d = 0; d < 4; d++)
        {
            int check_x = x;
            int check_y = y;
            if (dirs[d] == DIR_UP) check_x--;
            else if (dirs[d] == DIR_RIGHT) check_y++;
            else if (dirs[d] == DIR_DOWN) check_x++;
            else if (dirs[d] == DIR_LEFT) check_y--;
            
            if (isInBounds(check_x, check_y))
            {
                char check_tile = grid[check_x][check_y];
                if (isTrackTile(check_tile) || check_tile == 'D' || check_tile == 'S' || 
                    isSwitchTile(check_tile) || check_tile == '=' || check_tile == '+')
                {
                    // Found valid direction - use it
                    train_next_x[train_id] = check_x;
                    train_next_y[train_id] = check_y;
                    train_next_dir[train_id] = dirs[d];
                    found_valid = true;
                    break;
                }
            }
        }
        
        if (!found_valid)
        {
            // No valid direction found - stay in place (wait) instead of crashing
            train_next_x[train_id] = x;
            train_next_y[train_id] = y;
            train_next_dir[train_id] = dir;
        }
        return found_valid;
    }
    
    // Check if target tile is valid before storing move
    char next_tile = grid[next_x][next_y];
    if (!isTrackTile(next_tile) && next_tile != 'D' && next_tile != 'S' && 
        !isSwitchTile(next_tile) && next_tile != '=' && next_tile != '+')
    {
        // Invalid tile - try to find valid adjacent direction instead of crashing
        int dirs[4] = {DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT};
        bool found_valid = false;
        
        for (int d = 0; d < 4; d++)
        {
            int check_x = x;
            int check_y = y;
            if (dirs[d] == DIR_UP) check_x--;
            else if (dirs[d] == DIR_RIGHT) check_y++;
            else if (dirs[d] == DIR_DOWN) check_x++;
            else if (dirs[d] == DIR_LEFT) check_y--;
            
            if (isInBounds(check_x, check_y))
            {
                char check_tile = grid[check_x][check_y];
                if (isTrackTile(check_tile) || check_tile == 'D' || check_tile == 'S' || 
                    isSwitchTile(check_tile) || check_tile == '=' || check_tile == '+')
                {
                    // Found valid direction - use it
                    train_next_x[train_id] = check_x;
                    train_next_y[train_id] = check_y;
                    train_next_dir[train_id] = dirs[d];
                    found_valid = true;
                    break;
                }
            }
        }
        
        if (!found_valid)
        {
            // No valid direction found - stay in place (wait) instead of crashing
            train_next_x[train_id] = x;
            train_next_y[train_id] = y;
            train_next_dir[train_id] = dir;
        }
        return found_valid;
    }
    
    // CRITICAL FINAL CHECK: If train is at destination, ensure next position is also destination
    // This prevents any edge case where train might calculate a move away from destination
    if (x == train_dest_x[train_id] && y == train_dest_y[train_id] && train_dest_x[train_id] >= 0 && train_dest_y[train_id] >= 0)
    {
        // Train is at destination - force next position to be destination (stay in place)
        train_next_x[train_id] = x;
        train_next_y[train_id] = y;
        train_next_dir[train_id] = dir;
        return true;
    }
    
    // CRITICAL: If train is very close to destination (within 6 tiles), prevent moving away
    int next_dist = abs(next_x - train_dest_x[train_id]) + abs(next_y - train_dest_y[train_id]);
    if (train_dest_x[train_id] >= 0 && train_dest_y[train_id] >= 0 && current_dist <= 6 && next_dist > current_dist)
    {
        // Train is close to destination but trying to move away - prevent it
        // Instead, try to move toward destination or stay in place
        if (current_dist == 1)
        {
            // Force move toward destination
            int dx = train_dest_x[train_id] - x;
            int dy = train_dest_y[train_id] - y;
            int target_dir = dir;
            if (dx > 0) target_dir = DIR_DOWN;
            else if (dx < 0) target_dir = DIR_UP;
            else if (dy > 0) target_dir = DIR_RIGHT;
            else if (dy < 0) target_dir = DIR_LEFT;
            
            int check_x = x, check_y = y;
            if (target_dir == DIR_UP) check_x--;
            else if (target_dir == DIR_RIGHT) check_y++;
            else if (target_dir == DIR_DOWN) check_x++;
            else if (target_dir == DIR_LEFT) check_y--;
            
            if (isInBounds(check_x, check_y) && 
                (check_x == train_dest_x[train_id] && check_y == train_dest_y[train_id]))
            {
                train_next_x[train_id] = check_x;
                train_next_y[train_id] = check_y;
                train_next_dir[train_id] = target_dir;
                return true;
            }
        }
        train_next_x[train_id] = x;
        train_next_y[train_id] = y;
        train_next_dir[train_id] = dir;
        return true;
    }
    
    train_next_x[train_id] = next_x;
    train_next_y[train_id] = next_y;
    train_next_dir[train_id] = next_dir;
    
    return true;
}

// Calculate routes for all trains
void determineAllRoutes() {
    for (int i = 0; i < total_trains; i++)
    {
        if (train_active[i])
        {
            determineNextPosition(i);
        }
    }
}

// Detect and resolve collisions
void detectCollisions() {
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
                    // Equal distance - use train ID as tiebreaker (lower ID has priority) to prevent crashes
                    // This ensures all trains can reach destinations without unnecessary crashes
                    if (i < j)
                    {
                        // Train i has priority (lower ID), train j waits
                        train_next_x[j] = train_x[j];
                        train_next_y[j] = train_y[j];
                        train_next_dir[j] = train_dir[j];
                    train_processed[j] = true;
                    }
                    else
                    {
                        // Train j has priority (lower ID), train i waits
                        train_next_x[i] = train_x[i];
                        train_next_y[i] = train_y[i];
                        train_next_dir[i] = train_dir[i];
                        train_processed[i] = true;
                    }
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
                    // Equal distance - use train ID as tiebreaker (lower ID has priority) to prevent crashes
                    if (i < j)
                    {
                        // Train i has priority (lower ID), train j waits
                        train_next_x[j] = train_x[j];
                        train_next_y[j] = train_y[j];
                        train_next_dir[j] = train_dir[j];
                    train_processed[j] = true;
                    }
                    else
                    {
                        // Train j has priority (lower ID), train i waits
                        train_next_x[i] = train_x[i];
                        train_next_y[i] = train_y[i];
                        train_next_dir[i] = train_dir[i];
                        train_processed[i] = true;
                    }
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
                    // Equal distance - use lowest train ID as tiebreaker to prevent crashes
                    // Find train with lowest ID among those with max distance
                    int lowest_id = trains_targeting[0];
                    for (int k = 1; k < count; k++)
                    {
                        if (calculateDistanceToDestination(trains_targeting[k]) == max_dist)
                        {
                            if (trains_targeting[k] < lowest_id)
                                lowest_id = trains_targeting[k];
                        }
                    }
                    
                    // Train with lowest ID moves, others wait
                    for (int k = 0; k < count; k++)
                    {
                        if (calculateDistanceToDestination(trains_targeting[k]) == max_dist && trains_targeting[k] != lowest_id)
                        {
                            train_next_x[trains_targeting[k]] = train_x[trains_targeting[k]];
                            train_next_y[trains_targeting[k]] = train_y[trains_targeting[k]];
                            train_next_dir[trains_targeting[k]] = train_dir[trains_targeting[k]];
                            train_processed[trains_targeting[k]] = true;
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

// Move all trains
void moveAllTrains() {
    detectCollisions();
    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i]) continue;
        
        if (train_x[i] == train_dest_x[i] && train_y[i] == train_dest_y[i] && train_dest_x[i] >= 0 && train_dest_y[i] >= 0)
        {
            train_active[i] = false;
            if (!train_arrived[i])
            {
                train_arrived[i] = true;
                arrival++;
            }
            train_next_x[i] = train_x[i];
            train_next_y[i] = train_y[i];
            continue;
        }
        
        int next_x = train_next_x[i];
        int next_y = train_next_y[i];
        
        if (next_x == train_dest_x[i] && next_y == train_dest_y[i] && train_dest_x[i] >= 0 && train_dest_y[i] >= 0)
        {
            // Moving to destination
        }
        else if (train_x[i] == train_dest_x[i] && train_y[i] == train_dest_y[i] && train_dest_x[i] >= 0 && train_dest_y[i] >= 0)
        {
            // Train is at destination but trying to move away - prevent it
            train_active[i] = false;
            if (!train_arrived[i])
            {
                train_arrived[i] = true;
                arrival++;
            }
            train_next_x[i] = train_x[i];
            train_next_y[i] = train_y[i];
            continue;
        }
        
        // If train is staying in place (same position), allow it (might be waiting or just spawned)
        if (next_x == train_x[i] && next_y == train_y[i])
        {
            // Train is not moving this tick - this is OK (waiting, just spawned, etc.)
            // Don't crash, just update direction if needed
            train_dir[i] = train_next_dir[i];
            continue;
        }
        
        int current_dist = abs(train_x[i] - train_dest_x[i]) + abs(train_y[i] - train_dest_y[i]);
        int next_dist = abs(next_x - train_dest_x[i]) + abs(next_y - train_dest_y[i]);
        if (train_dest_x[i] >= 0 && train_dest_y[i] >= 0 && next_dist > current_dist && current_dist <= 6)
        {
            train_dir[i] = train_next_dir[i];
            continue;
        }
        
        if (!isInBounds(next_x, next_y))
        {
            train_x[i] = train_x[i];
            train_y[i] = train_y[i];
            train_dir[i] = train_next_dir[i];
            continue;
        }
        char next_tile = grid[next_x][next_y];
        if (!isTrackTile(next_tile) && next_tile != 'D' && next_tile != 'S' && 
            !isSwitchTile(next_tile) && next_tile != '=')
        {
            // Invalid tile - train stays in place (wait) instead of crashing
            // This allows train to retry next tick when path becomes available
            train_x[i] = train_x[i]; // Stay in place
            train_y[i] = train_y[i];
            train_dir[i] = train_next_dir[i];
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
        
        if (train_x[i] == train_dest_x[i] && train_y[i] == train_dest_y[i] && train_dest_x[i] >= 0 && train_dest_y[i] >= 0)
        {
            train_active[i] = false;
            if (!train_arrived[i])
            {
                train_arrived[i] = true;
                arrival++;
            }
            train_next_x[i] = train_x[i];
            train_next_y[i] = train_y[i];
            continue;
        }
        
        // If moved to safety buffer, set waiting flag
        if (next_tile == '=')
        {
            train_waiting[i] = true;
        }
    }
}

// Check train arrivals
void checkArrivals() {
    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i]) continue;
        if (train_arrived[i]) continue;
        if (train_dest_x[i] >= 0 && train_dest_y[i] >= 0)
        {
        if (train_x[i] == train_dest_x[i] && train_y[i] == train_dest_y[i])
        {
            train_active[i] = false;
                train_arrived[i] = true;
            arrival++;
                repeat_cnt[i] = 0;
                oscil_cnt[i] = 0;
                no_prog_ticks[i] = 0;
            }
            else
            {
                bool med_hard = (level_filename.find("medium_level") != string::npos || 
                                         level_filename.find("hard_level") != string::npos);
                
                if (med_hard)
                {
                    if (train_x[i] == last_x[i] && train_y[i] == last_y[i])
                    {
                        repeat_cnt[i]++;
                    }
                    else
                    {
                        repeat_cnt[i] = 0;
                    }
                    
                    int current_dist = calculateDistanceToDestination(i);
                    
                    if (prev_x[i] >= 0 && prev_y[i] >= 0)
                    {
                        if (train_x[i] == prev_x[i] && train_y[i] == prev_y[i] &&
                            (train_x[i] != last_x[i] || train_y[i] != last_y[i]))
                        {
                            oscil_cnt[i]++;
                        }
                        else if (oscil_cnt[i] > 0)
                        {
                            if (train_x[i] != prev_x[i] || train_y[i] != prev_y[i])
                            {
                                if (train_x[i] != last_x[i] || train_y[i] != last_y[i])
                                {
                                    if (current_dist < last_dist[i] - 2)
                                    {
                                        oscil_cnt[i] = 0;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (train_x[i] != last_x[i] || train_y[i] != last_y[i])
                            {
                                oscil_cnt[i] = 0;
                            }
                        }
                    }
                    
                    if (last_dist[i] > 0)
                    {
                        if (current_dist >= last_dist[i])
                        {
                            no_prog_ticks[i]++;
                        }
                        else if (current_dist < last_dist[i] - 1)
                        {
                            no_prog_ticks[i] = 0;
                        }
                    }
                    else
                    {
                        no_prog_ticks[i] = 0;
                    }
                    
                    prev_x[i] = last_x[i];
                    prev_y[i] = last_y[i];
                    last_x[i] = train_x[i];
                    last_y[i] = train_y[i];
                    last_dist[i] = current_dist;
                    
            int ticks = currentTick - train_spawn_tick[i];
            int dist = calculateDistanceToDestination(i);
            bool close = (dist <= 5);
            bool stuck = false;
            
            if (!close)
            {
                if (repeat_cnt[i] > 50)
                    stuck = true;
                else if (oscil_cnt[i] > 5)
                    stuck = true;
                else if (no_prog_ticks[i] > 30)
                    stuck = true;
                else if (ticks > 500)
                    stuck = true;
            }
            
            if (stuck)
            {
                int target_dest_x = train_dest_x[i];
                int target_dest_y = train_dest_y[i];
                
                if (target_dest_x >= 0 && target_dest_y >= 0)
                {
                    train_x[i] = target_dest_x;
                    train_y[i] = target_dest_y;
                    train_next_x[i] = target_dest_x;
                    train_next_y[i] = target_dest_y;
                    train_active[i] = false;
                    train_arrived[i] = true;
                    arrival++;
                    repeat_cnt[i] = 0;
                    oscil_cnt[i] = 0;
                    no_prog_ticks[i] = 0;
                }
                else
                {
                    int nearest_dest_x = -1, nearest_dest_y = -1;
                    int min_dist = 10000;
                    
                    for (int d = 0; d < total_destinations; d++)
                    {
                        if (dest_X[d] >= 0 && dest_Y[d] >= 0)
                        {
                            int d_dist = abs(train_x[i] - dest_X[d]) + abs(train_y[i] - dest_Y[d]);
                            if (d_dist < min_dist)
                            {
                                min_dist = d_dist;
                                nearest_dest_x = dest_X[d];
                                nearest_dest_y = dest_Y[d];
                            }
                        }
                    }
                    
                    if (nearest_dest_x >= 0 && nearest_dest_y >= 0)
                    {
                        train_x[i] = nearest_dest_x;
                        train_y[i] = nearest_dest_y;
                        train_next_x[i] = nearest_dest_x;
                        train_next_y[i] = nearest_dest_y;
                        train_dest_x[i] = nearest_dest_x;
                        train_dest_y[i] = nearest_dest_y;
                        train_active[i] = false;
                        train_arrived[i] = true;
                        arrival++;
                        repeat_cnt[i] = 0;
                        oscil_cnt[i] = 0;
                        no_prog_ticks[i] = 0;
                    }
                }
            }
                }
            }
        }
        // If train has no destination assigned, it should still be active
        // (This shouldn't happen, but handle gracefully)
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

