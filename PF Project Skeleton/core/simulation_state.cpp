#include "simulation_state.h"
#include <cstring>


// ============================================================================
// SIMULATION_STATE.CPP - Global state definitions
// ============================================================================

int rows;
int cols;
char grid[max_rows][max_cols];

int train_x[max_trains];
int train_y[max_trains];
int train_dir[max_trains];
bool train_active[max_trains];
int total_trains;
int next_train_id;

int switch_x[max_switches];
int switch_y[max_switches];
int switch_state[max_switches];
int switch_flip[max_switches];
int switch_index[max_switches];
int total_switches = 0;
int switch_mode[max_switches];
int switch_init[max_switches];
int switch_k_up[max_switches];
int switch_k_right[max_switches];
int switch_k_down[max_switches];
int switch_k_left[max_switches];
string switch_state0[max_switches];
string switch_state1[max_switches];

int spawn_x[max_trains];
int spawn_y[max_trains];
int total_spawns;

int dest_X[max_trains];
int dest_Y[max_trains];
int total_destinations;

int grid_loaded;               
int track_count;          
int spawn_count;         
int dest_count_grid;
int currentTick;

int arrival;
int crashes;
bool finished;

bool emergencyHalt;
int emergencyHaltTimer = 0;
int emergencyHaltDuration = 0; 

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
}

// ----------------------------------------------------------------------------
// METRICS
// ----------------------------------------------------------------------------

void reset_metrics()
{
    arrival = 0;
    crashes = 0;
    finished = false;
}

// ----------------------------------------------------------------------------
// EMERGENCY HALT
// ----------------------------------------------------------------------------

void reset_emergency()
{
    emergencyHalt = false;
    emergencyHaltTimer = 0;
    emergencyHaltDuration = 0; 

    
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
