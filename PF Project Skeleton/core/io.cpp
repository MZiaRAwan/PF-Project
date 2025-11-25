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
        if (line == "ROWS:") // for reading rows from file
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
        while (line.length() < (size_t)cols) // size_t is alternative of int
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
                switch_state[total_switches] = 0; // default
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
void initializeLogFiles()
{
}

// ----------------------------------------------------------------------------
// LOG TRAIN TRACE
// ----------------------------------------------------------------------------
// Append tick, train id, position, direction, state to trace.csv.
// ----------------------------------------------------------------------------
void logTrainTrace()
{
}

// ----------------------------------------------------------------------------
// LOG SWITCH STATE
// ----------------------------------------------------------------------------
// Append tick, switch id/mode/state to switches.csv.
// ----------------------------------------------------------------------------

void logSwitchState()
{
    // using static so memory doesnot delete after use
    static int prev_state[max_switches];
    static bool initialized = false;

    if (!initialized)
    {
        for (int i = 0; i < total_switches; ++i)
        {
            prev_state[i] = switch_state[i];
        }
        initialized = true;
    }

    // Check every switch each tick
    for (int i = 0; i < total_switches; ++i)
    {
        int sx = switch_x[i];          // switch row
        int sy = switch_y[i];          // switch column
        int current = switch_state[i]; // 0 = straight, 1 = turn

        // If the switch changed since last tick
        if (current != prev_state[i])
        {
            // Tell the logger (replace with your actual logging function)
            // Format: (tick, switchID, row, col, state)
            recordSwitchEvent(currentTick, i, sx, sy, current);

            // Update previous state memory
            prev_state[i] = current;
        }
    }
}

// ----------------------------------------------------------------------------
// LOG SIGNAL STATE
// ----------------------------------------------------------------------------
// Append tick, switch id, signal color to signals.csv.
// ----------------------------------------------------------------------------

void logSignalState()
{
    ofstream file("signals.csv", ios::app);

    if (!file.is_open())
        return;

    for (int i = 0; i < total_switches; i++)
    {
        int state = switch_state[i];

        // Convert number → text
        string color;
        if (state == signal_green)
            color = "GREEN";
        else if (state == signal_yellow)
            color = "YELLOW";
        else
            color = "RED";

        // Write to file
        file << currentTick << "," << i << "," << color << "\n";
    }

    file.close();
}

// ----------------------------------------------------------------------------
// WRITE FINAL METRICS
// ----------------------------------------------------------------------------
// Write summary metrics to metrics.txt.
// ----------------------------------------------------------------------------

void writeMetrics()
{
    ofstream out("metrics.txt") if (!out.is_open()) return;
    out << "TOTAL_ARRIVALS: " << arrival << "\n";
    out << "TOTAL_CRASHES: " << crashes << "\n";
    out << "FINISHED: " << (finished ? "YES" : "NO") << "\n";

    out << "TOTAL_TRAINS: " << total_trains << "\n";
    out << "TOTAL_SWITCHES: " << total_switches << "\n";
    out << "TOTAL_SPAWNS: " << total_spawns << "\n";
    out << "TOTAL_DESTINATIONS: " << total_destinations << "\n";

    out.close();
}
