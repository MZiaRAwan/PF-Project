#include "io.h"
#include "simulation_state.h"
#include "grid.h"
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
bool loadLevelFile()
{
    ifstream file(level_file);
    if (!file.is_open())
    {
        grid_loaded = 0;
        return false;
    }

    string line;

    
    while (getline(file, line))
    {
        if (line == "ROWS:") //for reading rows from file
        {
            getline(file, line);
            rows = stoi(line);
            break;
        }
    }
    while (getline(file, line))
    {
        if (line == "COLS:") // for reading columns from file
        {
            getline(file, line);
            cols = stoi(line); // stoi converts string to int because row and columns are intgers but are read as string
            break;
        }
    }
    while (getline(file, line))
    {
        if (line == "MAP:") // for reading grid from files
            break;
    }
    for (int r = 0; r < rows; r++)
    {
        getline(file, line);

        // Fill shorter lines with spaces
       while (line.length() < (size_t)cols) //size_t is alternative of int
{
    line += ' ';
}


        for (int c = 0; c < cols; c++)
        {
            char cell = line[c];
            grid[r][c] = cell;

            // Detect Spawn Points ('S')
            if (cell == 'S')
            {
                spawn_x[total_spawns] = r;
                spawn_y[total_spawns] = c;
                total_spawns++;
            }

            // Detect Destination ('D' or 'E')
            if (cell == 'D' || cell == 'E')
            {
                dest_X[total_destinations] = r;
                dest_Y[total_destinations] = c;
                total_destinations++;
            }

            // Detect Switches (A–Z)
            if (cell >= 'A' && cell <= 'Z')
            {
                switch_x[total_switches] = r;
                switch_y[total_switches] = c;
                switch_state[total_switches] = 0;  // default
                total_switches++;
            }
        }
    }

    file.close();
    grid_loaded = 1;
    return true;
}


// ----------------------------------------------------------------------------
// INITIALIZE LOG FILES
// ----------------------------------------------------------------------------
// Create/clear CSV logs with headers.
// ----------------------------------------------------------------------------
void initializeLogFiles() {
}

// ----------------------------------------------------------------------------
// LOG TRAIN TRACE
// ----------------------------------------------------------------------------
// Append tick, train id, position, direction, state to trace.csv.
// ----------------------------------------------------------------------------
void logTrainTrace() {
}

// ----------------------------------------------------------------------------
// LOG SWITCH STATE
// ----------------------------------------------------------------------------
// Append tick, switch id/mode/state to switches.csv.
// ----------------------------------------------------------------------------
void logSwitchState() {
}

// ----------------------------------------------------------------------------
// LOG SIGNAL STATE
// ----------------------------------------------------------------------------
// Append tick, switch id, signal color to signals.csv.
// ----------------------------------------------------------------------------
void logSignalState() {
}

// ----------------------------------------------------------------------------
// WRITE FINAL METRICS
// ----------------------------------------------------------------------------
// Write summary metrics to metrics.txt.
// ----------------------------------------------------------------------------
void writeMetrics() {
}
