#ifndef SIMULATION_STATE_H
#define SIMULATION_STATE_H
#include <string>         
using namespace std;

// ============================================================================
// SIMULATION_STATE.H - Global constants and state
// ============================================================================
// Global constants and arrays used by the game.
// ============================================================================

// ----------------------------------------------------------------------------
// GRID CONSTANTS
// ----------------------------------------------------------------------------

#define max_rows 200
#define max_cols 200

// ----------------------------------------------------------------------------
// TRAIN CONSTANTS
// ----------------------------------------------------------------------------

#define max_trains 400

// ----------------------------------------------------------------------------
// SWITCH CONSTANTS
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// WEATHER CONSTANTS
// ----------------------------------------------------------------------------

#define weather_clear 0
#define weather_rain 1
#define weather_fog 2

// ----------------------------------------------------------------------------
// SIGNAL CONSTANTS
// ----------------------------------------------------------------------------

#define signal_green 0
#define signal_yellow 1
#define signal_red 2

// ----------------------------------------------------------------------------
// GLOBAL STATE: GRID
// ----------------------------------------------------------------------------

extern int rows;
extern int cols;
extern char grid[max_rows][max_cols];

// ----------------------------------------------------------------------------
// GLOBAL STATE: TRAINS
// ----------------------------------------------------------------------------

extern int train_x[max_trains];
extern int train_y[max_trains];
extern int train_dir[max_trains];
extern bool train_active[max_trains];
extern int total_trains;
extern int next_train_id;

// ----------------------------------------------------------------------------
// GLOBAL STATE: SWITCHES (A-Z mapped to 0-25)
// ----------------------------------------------------------------------------
//const int max_switches = 26;
 
const int max_switches = 26;

extern int switch_x[max_switches];
extern int switch_y[max_switches];
extern int switch_state[max_switches];
extern int switch_flip[max_switches];
extern int switch_index[max_switches];
extern int switch_mode[max_switches];
extern int switch_init[max_switches];
extern int switch_k_up[max_switches];
extern int switch_k_right[max_switches];
extern int switch_k_down[max_switches];
extern int switch_k_left[max_switches];
extern string switch_state0[max_switches];
extern string switch_state1[max_switches];
extern int total_switches;



// ----------------------------------------------------------------------------
// GLOBAL STATE: SPAWN POINTS
// ----------------------------------------------------------------------------

extern int spawn_x[max_trains];
extern int spawn_y[max_trains];
extern int total_spawns;

// ----------------------------------------------------------------------------
// GLOBAL STATE: DESTINATION POINTS
// ----------------------------------------------------------------------------

extern int dest_X[max_trains];
extern int dest_Y[max_trains];
extern int total_destinations;

// ----------------------------------------------------------------------------
// GLOBAL STATE: SIMULATION PARAMETERS
// ----------------------------------------------------------------------------

extern int grid_loaded;
extern int track_count;
extern int spawn_count;
extern int dest_count_grid;
extern int currentTick;

// ----------------------------------------------------------------------------
// GLOBAL STATE: METRICS
// ----------------------------------------------------------------------------

extern int arrival;
extern int crashes;
extern bool finished;

// ----------------------------------------------------------------------------
// GLOBAL STATE: EMERGENCY HALT
// ----------------------------------------------------------------------------

extern bool emergencyHalt;
extern int emergencyHaltTimer; 
extern int emergencyHaltDuration;

// ----------------------------------------------------------------------------
// INITIALIZATION FUNCTION
// ----------------------------------------------------------------------------
// Resets all state before loading a new level.
void initializeSimulationState();

#endif
