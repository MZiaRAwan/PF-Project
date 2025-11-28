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
// Increment counters for trains entering switches.
// ----------------------------------------------------------------------------
void updateSwitchCounters()
{
    for (int i = 0; i < total_switches; i++)
    {
        int sx = switch_x[i];
        int sy = switch_y[i];
        for (int t = 0; t < total_trains; t++)
        {
            if (!train_active[t])
                continue;
            if (train_x[t] == sx && train_y[t] == sy)
            {
                switch_counter[i]++; // increase counter
            }
        }
    }
}

// ----------------------------------------------------------------------------
// QUEUE SWITCH FLIPS
// ----------------------------------------------------------------------------
// Queue flips when counters hit K.
// ----------------------------------------------------------------------------
void queueSwitchFlips()
{
    for (int i = 0; i < total_switches; i++)
    {
        if (switch_counter[i] >= switch_k[i])
        {
            // fliping switch
            switch_flip[i] = 1;
            // Reseting counter
            switch_counter[i] = 0;
        }
    }
}

// ----------------------------------------------------------------------------
// APPLY DEFERRED FLIPS
// ----------------------------------------------------------------------------
// Apply queued flips after movement.
// ----------------------------------------------------------------------------
void applyDeferredFlips()
{
    for (int i = 0; i < total_switches; i++)
    {

        // Check either it is fliping or not
        if (switch_flip[i] == 1)
        {

            // Fliping switch
            switch_state[i] = 1 - switch_state[i];
            // Reset switch_flip to zero
            switch_flip[i] = 0;
        }
    }
}

// ----------------------------------------------------------------------------
// UPDATE SIGNAL LIGHTS
// ----------------------------------------------------------------------------
// Update signal colors for switches.
// ----------------------------------------------------------------------------

void updateSignalLights()
{
    for (int i = 0; i < total_switches; i++)
    {
        if (emergencyHalt)
        {
            switch_signal[i] = signal_red;
        }
        else if (switch_flip[i] == 1)
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
