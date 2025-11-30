#include "switches.h"
#include "simulation_state.h"
#include "grid.h"
#include "io.h"

// Switch management
void updateSwitchCounters()
{
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue;
        
        int sx = switch_x[i];
        int sy = switch_y[i];
        
        for (int t = 0; t < total_trains; t++)
        {
            if (!train_active[t])
                continue;
            
            if (train_x[t] == sx && train_y[t] == sy)
            {
                int dir = train_dir[t];
                
                if (switch_mode[i] == 1)
                {
                    switch_counter_global[i]++;
                }
                else
                {
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

// Queue switches to flip
void queueSwitchFlips()
{
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue;
        
        bool should_flip = false;
        
        if (switch_mode[i] == 1)
        {
            if (switch_counter_global[i] >= switch_k_up[i])
            {
                should_flip = true;
                switch_counter_global[i] = 0;
            }
        }
        else
        {
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
            switch_flip[i] = 1;
        }
    }
}

// ----------------------------------------------------------------------------
// APPLY DEFERRED FLIPS
// Apply queued switch flips
void applyDeferredFlips()
{
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue;
        
        if (switch_flip[i] == 1)
        {
            switch_state[i] = 1 - switch_state[i];
            total_switch_flips++;
            switch_flip[i] = 0;
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
        if (switch_x[i] < 0) continue;
        
        int sx = switch_x[i];
        int sy = switch_y[i];
        
        if (emergencyHalt)
        {
            switch_signal[i] = signal_red;
            continue;
        }
        
        bool next_tile_blocked = false;
        bool train_within_two = false;
        
        for (int t = 0; t < total_trains; t++)
        {
            if (!train_active[t]) continue;
            
            if (train_next_x[t] == sx && train_next_y[t] == sy)
            {
                next_tile_blocked = true;
            }
            
            int dx = abs(train_x[t] - sx);
            int dy = abs(train_y[t] - sy);
            int dist = dx + dy;
            
            if (dist <= 2 && dist > 0)
            {
                int next_x = train_x[t];
                int next_y = train_y[t];
                if (train_dir[t] == DIR_UP) next_x--;
                else if (train_dir[t] == DIR_RIGHT) next_y++;
                else if (train_dir[t] == DIR_DOWN) next_x++;
                else if (train_dir[t] == DIR_LEFT) next_y--;
                
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
                    next_tile_blocked = true;
                }
            }
        }
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

// Toggle switch state manually
void toggleSwitchState()
{
    for (int i = 0; i < total_switches; i++)
    {
        switch_state[i] = 1 - switch_state[i];
    }
}

// Get switch state
int getSwitchStateForDirection(int index)
{
    if (index < 0 || index >= max_switches)
    {
        return 0;
    }
    
    return switch_state[index];
}