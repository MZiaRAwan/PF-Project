#include "simulation_state.h"
#include <cstring>

// ============================================================================
// SIMULATION_STATE.CPP - Global state definitions
// ============================================================================
// All global variables are defined here.
// Variables are declared in simulation_state.h for use in other files.
// ============================================================================

// Variable definitions
int rows = 0;
int cols = 0;
char grid[max_rows][max_cols] = {};

int train_x[max_trains] = {};
int train_y[max_trains] = {};
int train_dir[max_trains] = {};
bool train_active[max_trains] = {};
int train_spawn_tick[max_trains] = {};
int train_next_x[max_trains] = {};
int train_next_y[max_trains] = {};
int train_next_dir[max_trains] = {};
int train_dest_x[max_trains] = {};
int train_dest_y[max_trains] = {};
bool train_waiting[max_trains] = {};
int train_rain_move_count[max_trains] = {};
int train_rain_waiting[max_trains] = {};
int train_color_index[max_trains] = {};
int total_trains = 0;
int next_train_id = 0;

int switch_x[max_switches] = {};
int switch_y[max_switches] = {};
int switch_state[max_switches] = {};
int switch_flip[max_switches] = {};
int switch_index[max_switches] = {};
int switch_mode[max_switches] = {};
int switch_init[max_switches] = {};
int switch_k_up[max_switches] = {};
int switch_k_right[max_switches] = {};
int switch_k_down[max_switches] = {};
int switch_k_left[max_switches] = {};
string switch_state0[max_switches];
string switch_state1[max_switches];
int switch_counter_up[max_switches] = {};
int switch_counter_right[max_switches] = {};
int switch_counter_down[max_switches] = {};
int switch_counter_left[max_switches] = {};
int switch_counter_global[max_switches] = {};
int switch_signal[max_switches] = {};
int total_switches = 0;

int spawn_x[max_trains] = {};
int spawn_y[max_trains] = {};
int total_spawns = 0;

int dest_X[max_trains] = {};
int dest_Y[max_trains] = {};
int total_destinations = 0;

int grid_loaded = 0;
int track_count = 0;
int spawn_count = 0;
int dest_count_grid = 0;
int currentTick = 0;
int weather_type = 0;
int emergencyHaltTimer = 0;
int level_seed = 0;
string level_filename = "data/levels/complex_network.lvl";

int arrival = 0;
int crashes = 0;
bool finished = false;
int total_wait_ticks = 0;
int signal_violations = 0;
int total_switch_flips = 0;
int total_train_ticks = 0;
int buffer_count = 0;
int train_idle_ticks[max_trains] = {};

bool emergencyHalt = false;

// ----------------------------------------------------------------------------
// GRID
// ----------------------------------------------------------------------------

void reset_grid()
{
    rows = 0;
    cols = 0;
    for (int i = 0; i < max_rows; i++)
    {
        for (int j = 0; j < max_cols; j++)
        {
            grid[i][j] = ' ';
        }
    }
}
// ----------------------------------------------------------------------------
// TRAINS
// ----------------------------------------------------------------------------

void reset_trains()
{
    total_trains = 0;
    next_train_id = 0;
    for (int i = 0; i < max_trains; i++)
    {
        train_x[i] = 0;
        train_y[i] = 0;
        train_dir[i] = 0;
        train_active[i] = false;
        train_spawn_tick[i] = 0;
        train_next_x[i] = 0;
        train_next_y[i] = 0;
        train_next_dir[i] = 0;
        train_dest_x[i] = -1;
        train_dest_y[i] = -1;
        train_waiting[i] = false;
        train_idle_ticks[i] = 0;
        train_rain_move_count[i] = 0;
        train_rain_waiting[i] = false;
        train_color_index[i] = 0;
    }
}

// ----------------------------------------------------------------------------
// SWITCHES
// ----------------------------------------------------------------------------

void reset_switches()
{
    total_switches = 0;
    for (int i = 0; i < max_switches; i++) {
        switch_x[i] = -1; // -1 = not present on map
        switch_y[i] = -1;
        switch_state[i] = 0;
        switch_flip[i] = 0;
        switch_index[i] = i;
        switch_mode[i] = 0;
        switch_init[i] = 0;
        switch_k_up[i] = 0;
        switch_k_right[i] = 0;
        switch_k_down[i] = 0;
        switch_k_left[i] = 0;
        switch_state0[i] = "";
        switch_state1[i] = "";
        switch_counter_up[i] = 0;
        switch_counter_right[i] = 0;
        switch_counter_down[i] = 0;
        switch_counter_left[i] = 0;
        switch_counter_global[i] = 0;
        switch_signal[i] = signal_green;
    }
}



// ----------------------------------------------------------------------------
// SPAWN AND DESTINATION POINTS
// ----------------------------------------------------------------------------

void reset_sd()
{
    total_spawns = 0;
    total_destinations = 0;
    for (int i = 0; i < max_trains; i++)
    {
        spawn_x[i] = 0;
        spawn_y[i] = 0;
        dest_X[i] = 0;
        dest_Y[i] = 0;
    }
}

// ----------------------------------------------------------------------------
// SIMULATION PARAMETERS
// ----------------------------------------------------------------------------

void reset_parameters()
{
    grid_loaded = 0;
    track_count = 0;
    spawn_count = 0;
    dest_count_grid = 0;
    currentTick = 0;
    weather_type = weather_clear;
    emergencyHaltTimer = 0;
    level_seed = 0;
    level_filename = "data/levels/complex_network.lvl";
}

// ----------------------------------------------------------------------------
// METRICS
// ----------------------------------------------------------------------------

void reset_metrics()
{
    arrival = 0;
    crashes = 0;
    finished = false;
    total_wait_ticks = 0;
    signal_violations = 0;
    total_switch_flips = 0;
    total_train_ticks = 0;
    buffer_count = 0;
}

// ----------------------------------------------------------------------------
// EMERGENCY HALT
// ----------------------------------------------------------------------------

void reset_emergency()
{
    emergencyHalt = false;
    emergencyHaltTimer = 0;
}

// ============================================================================
// INITIALIZE SIMULATION STATE
// ============================================================================
// ----------------------------------------------------------------------------
// Resets all global simulation state.
// ----------------------------------------------------------------------------
// Called before loading a new level.
// ----------------------------------------------------------------------------
void initializeSimulationState()
{
    reset_grid();
    reset_trains();
    reset_switches();
    reset_sd();
    reset_parameters();
    reset_metrics();
    reset_emergency();
}
