#include "switches.h"
#include "simulation_state.h"
#include "grid.h"
#include "io.h"

// ============================================================================
// SWITCHES.CPP - Switch management
// ============================================================================

// ----------------------------------------------------------------------------
// UPDATE SWITCH COUNTERS
// ----------------------------------------------------------------------------
// Increment counters for trains entering switches based on entry direction.
// PER_DIR mode: increment direction-specific counter
// GLOBAL mode: increment global counter
// ----------------------------------------------------------------------------
void updateSwitchCounters()
{
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue; // Skip switches not on map
        
        int sx = switch_x[i];
        int sy = switch_y[i];
        
        for (int t = 0; t < total_trains; t++)
        {
            if (!train_active[t])
                continue;
            
            // Check if train is entering this switch (at switch position)
            if (train_x[t] == sx && train_y[t] == sy)
            {
                int dir = train_dir[t];
                
                if (switch_mode[i] == 1) // GLOBAL mode
                {
                    switch_counter_global[i]++;
                }
                else // PER_DIR mode
                {
                    // Increment counter based on entry direction
                    if (dir == DIR_UP)
                        switch_counter_up[i]++;
                    else if (dir == DIR_RIGHT)
                        switch_counter_right[i]++;
                    else if (dir == DIR_DOWN)
                        switch_counter_down[i]++;
                    else if (dir == DIR_LEFT)
                        switch_counter_left[i]++;
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// QUEUE SWITCH FLIPS
// ----------------------------------------------------------------------------
// Queue flips when counters hit K-value.
// Reset counters after queuing flip.
// ----------------------------------------------------------------------------
void queueSwitchFlips()
{
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue; // Skip switches not on map
        
        bool should_flip = false;
        
        if (switch_mode[i] == 1) // GLOBAL mode
        {
            if (switch_counter_global[i] >= switch_k_up[i]) // Use first K for GLOBAL
            {
                should_flip = true;
                switch_counter_global[i] = 0; // Reset counter
            }
        }
        else // PER_DIR mode
        {
            // Check if any direction counter reached its K-value
            if (switch_counter_up[i] >= switch_k_up[i])
            {
                should_flip = true;
                switch_counter_up[i] = 0;
            }
            else if (switch_counter_right[i] >= switch_k_right[i])
            {
                should_flip = true;
                switch_counter_right[i] = 0;
            }
            else if (switch_counter_down[i] >= switch_k_down[i])
            {
                should_flip = true;
                switch_counter_down[i] = 0;
            }
            else if (switch_counter_left[i] >= switch_k_left[i])
            {
                should_flip = true;
                switch_counter_left[i] = 0;
            }
        }
        
        if (should_flip)
        {
            switch_flip[i] = 1; // Queue the flip
        }
    }
}

// ----------------------------------------------------------------------------
// APPLY DEFERRED FLIPS
// ----------------------------------------------------------------------------
// Apply queued flips after movement (Phase 5, after trains move).
// This ensures deterministic behavior - switches flip after trains have moved,
// so the flipping train is unaffected this tick.
// Counters are reset when flip is queued (in queueSwitchFlips) to avoid
// immediate re-flip loops.
// ----------------------------------------------------------------------------
void applyDeferredFlips()
{
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue; // Skip switches not on map
        
        // Check if this switch is queued to flip
        if (switch_flip[i] == 1)
        {
            // Flip the switch state (0 -> 1, 1 -> 0)
            // The flip occurs after trains move, so trains are unaffected this tick
            switch_state[i] = 1 - switch_state[i];
            
            // Track switch flip for metrics
            total_switch_flips++;
            
            // Reset flip flag
            switch_flip[i] = 0;
            
            // Note: Counters were already reset in queueSwitchFlips() when
            // the flip was queued. This prevents immediate re-flip loops.
        }
    }
}

// ----------------------------------------------------------------------------
// UPDATE SIGNAL LIGHTS
// ----------------------------------------------------------------------------
// Update signal colors for switches based on safety conditions.
// GREEN: next tile free along planned route
// YELLOW: train within two tiles ahead
// RED: next tile blocked/occupied or would collide this tick
// ----------------------------------------------------------------------------
void updateSignalLights()
{
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue; // Skip switches not on map
        
        int sx = switch_x[i];
        int sy = switch_y[i];
        
        // Emergency halt: all signals red
        if (emergencyHalt)
        {
            switch_signal[i] = signal_red;
            continue;
        }
        
        // Check for trains that will move to this switch's next tile
        // First, determine what the "next tile" is based on switch state
        // For simplicity, we check all possible exit directions from the switch
        
        bool next_tile_blocked = false;
        bool train_within_two = false;
        
        // Check all trains to see if they're targeting tiles near this switch
        for (int t = 0; t < total_trains; t++)
        {
            if (!train_active[t]) continue;
            
            // Check if train's next position is the switch tile itself
            if (train_next_x[t] == sx && train_next_y[t] == sy)
            {
                next_tile_blocked = true; // Train will be at switch next tick
            }
            
            // Check if train is within 2 tiles ahead of switch
            int dx = abs(train_x[t] - sx);
            int dy = abs(train_y[t] - sy);
            int dist = dx + dy; // Manhattan distance
            
            if (dist <= 2 && dist > 0)
            {
                // Check if train is moving towards the switch
                int next_x = train_x[t];
                int next_y = train_y[t];
                if (train_dir[t] == DIR_UP) next_x--;
                else if (train_dir[t] == DIR_RIGHT) next_y++;
                else if (train_dir[t] == DIR_DOWN) next_x++;
                else if (train_dir[t] == DIR_LEFT) next_y--;
                
                // If train is moving towards switch or will be at switch
                if ((next_x == sx && next_y == sy) || (train_next_x[t] == sx && train_next_y[t] == sy))
                {
                    train_within_two = true;
                }
            }
            
            // Check if train's next position blocks a potential exit from switch
            // Check all 4 directions from switch
            int dirs[4][2] = {{-1,0}, {0,1}, {1,0}, {0,-1}}; // UP, RIGHT, DOWN, LEFT
            for (int d = 0; d < 4; d++)
            {
                int check_x = sx + dirs[d][0];
                int check_y = sy + dirs[d][1];
                
                if (train_next_x[t] == check_x && train_next_y[t] == check_y)
                {
                    // Check if this exit is valid based on switch state
                    // For now, mark as blocked if any train is moving there
                    next_tile_blocked = true;
                }
            }
        }
        
        // Determine signal color
        if (next_tile_blocked || switch_flip[i] == 1)
        {
            switch_signal[i] = signal_red;
        }
        else if (train_within_two)
        {
            switch_signal[i] = signal_yellow;
        }
        else
        {
            switch_signal[i] = signal_green;
        }
    }
}

// ----------------------------------------------------------------------------
// TOGGLE SWITCH STATE (Manual)
// ----------------------------------------------------------------------------
// Manually toggle a switch state.
// ----------------------------------------------------------------------------
void toggleSwitchState()
{
    for (int i = 0; i < total_switches; i++)
    {
        switch_state[i] = 1 - switch_state[i];
    }
}

// ----------------------------------------------------------------------------
// GET SWITCH STATE FOR DIRECTION
// ----------------------------------------------------------------------------
// Return the state for a given direction.
// ----------------------------------------------------------------------------
int getSwitchStateForDirection(int index)
{
    // Error Check
    if (index < 0 || index >= max_switches)
    {
        return 0;
    }

    // Return switch state
    return switch_state[index];
}