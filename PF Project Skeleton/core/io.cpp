#include "io.h"
#include "simulation_state.h"
#include "grid.h"
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

bool loadLevelFile()
{
    ifstream file("/home/zia/Documents/GitHub/PF-Project/PF Project Skeleton/data/levels/complex_network.lvl");
    if (!file.is_open()) {
        grid_loaded = 0;
        return false;
    }

    string line;
    total_spawns = 0;
    total_destinations = 0;
    total_switches = 0;

    for (int i = 0; i < max_switches; i++)
    {
        switch_x[i] = -1;
        switch_y[i] = -1;

        switch_mode[i] = 0;
        switch_init[i] = 0;

        switch_k_up[i] = 0;
        switch_k_right[i] = 0;
        switch_k_down[i] = 0;
        switch_k_left[i] = 0;

        switch_state0[i] = "";
        switch_state1[i] = "";

        switch_state[i] = 0;
        switch_index[i] = i;
    }

    // ============================================
    // READ ROWS
    // ============================================
    while (getline(file, line))
    {
        if (line == "ROWS:")
        {
            getline(file, line);
            rows = stoi(line);
            break;
        }
    }

    // ============================================
    // READ COLS
    // ============================================
    while (getline(file, line))
    {
        if (line == "COLS:")
        {
            getline(file, line);
            cols = stoi(line);
            break;
        }
    }

    // ============================================
    // FIND MAP HEADER
    // ============================================
    while (getline(file, line))
        if (line == "MAP:")
            break;

    // ============================================
    // READ MAP CONTENT
    // ============================================
    for (int r = 0; r < rows; r++)
    {
        getline(file, line);
        while (line.length() < (size_t)cols)
            line += ' ';

        for (int c = 0; c < cols; c++)
        {
            char cell = line[c];
            grid[r][c] = cell;

            // SPAWNS
            if (cell == 'S')
            {
                spawn_x[total_spawns] = r;
                spawn_y[total_spawns] = c;
                total_spawns++;
            }

            // DESTINATIONS
            if (cell == 'D' || cell == 'E')
            {
                dest_X[total_destinations] = r;
                dest_Y[total_destinations] = c;
                total_destinations++;
            }

            // SWITCH LETTERS
            if (cell >= 'A' && cell <= 'Z')
            {
                int idx = cell - 'A';
                switch_x[idx] = r;
                switch_y[idx] = c;
            }
        }
    }

    // ============================================
    // FIND SWITCHES:
    // ============================================
// ============================================
// READ SWITCHES ONLY (NO TRAINS AT ALL)
// ============================================

while (getline(file, line)) {
    if (line == "SWITCHES:")
        break;
}

// ------- READ SWITCH DEFINITIONS -------
while (getline(file, line)) {

    // Stop BEFORE TRAINS SECTION
    if (line == "TRAINS:")
        break;

    // Skip empty lines
    if (line.find_first_not_of(" \t\r\n") == string::npos)
        continue;

    char id;
    string modeStr;
    int init, ku, kr, kd, kl;
    string s0, s1;

    stringstream ss(line);

    // If parsing fails → skip line
    if (!(ss >> id >> modeStr >> init >> ku >> kr >> kd >> kl >> s0 >> s1))
        continue;

    // Valid switch ID must be A–Z
    if (id < 'A' || id > 'Z')
        continue;

    int idx = id - 'A';

    // STORE SWITCH DATA
    switch_mode[idx]  = (modeStr == "GLOBAL") ? 1 : 0;
    switch_init[idx]  = init;

    switch_k_up[idx]    = ku;
    switch_k_right[idx] = kr;
    switch_k_down[idx]  = kd;
    switch_k_left[idx]  = kl;

    switch_state0[idx] = s0;
    switch_state1[idx] = s1;

    switch_state[idx]  = init;
}

// ============================================
// COUNT SWITCHES FOUND IN MAP ONLY
// ============================================
total_switches = 0;
for (int i = 0; i < max_switches; i++) {
    if (switch_x[i] != -1)
        total_switches++;
}

grid_loaded = 1;
return true;

}


// =============================================
// LOGGING FUNCTIONS
// =============================================
void initializeLogFiles()
{
    ofstream trainLog("train_log.csv");
    ofstream switchLog("switch_log.csv");
    ofstream crashLog("crash_log.csv");
    ofstream arrivalLog("arrival_log.csv");

    trainLog  << "tick,id,x,y,dir\n";
    switchLog << "tick,id,row,col,state\n";
    crashLog  << "tick,x,y\n";
    arrivalLog<< "tick,trainID\n";
}

void logTrainTrace()
{
    ofstream file("trace.csv", ios::app);
    if (!file.is_open()) return;

    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i]) continue;

        file   << currentTick << ","
               << i << ","
               << train_x[i] << ","
               << train_y[i] << ","
               << train_dir[i] << "\n";
    }
}

void logSwitchState()
{
    static int prev[max_switches];
    static bool first = true;

    if (first)
    {
        for (int i = 0; i < max_switches; i++)
            prev[i] = switch_state[i];
        first = false;
    }

    ofstream file("switches.csv", ios::app);
    if (!file.is_open()) return;

    for (int i = 0; i < total_switches; i++)
    {
        if (switch_state[i] != prev[i])
        {
            file << currentTick << ","
                 << i << ","
                 << switch_x[i] << ","
                 << switch_y[i] << ","
                 << switch_state[i] << "\n";

            prev[i] = switch_state[i];
        }
    }
}

void logSignalState()
{
    ofstream file("signals.csv", ios::app);
    if (!file.is_open()) return;

    for (int i = 0; i < total_switches; i++)
    {
        int s = switch_state[i];
        string color =
            (s == signal_green)  ? "GREEN" :
            (s == signal_yellow) ? "YELLOW" :
                                   "RED";

        file << currentTick << "," << i << "," << color << "\n";
    }
}

void writeMetrics()
{
    ofstream out("metrics.txt");
    if (!out.is_open()) return;

    out << "TOTAL_ARRIVALS: " << arrival << "\n";
    out << "TOTAL_CRASHES: " << crashes << "\n";
    out << "FINISHED: " << (finished ? "YES" : "NO") << "\n";
    out << "TOTAL_TRAINS: " << total_trains << "\n";
    out << "TOTAL_SWITCHES: " << total_switches << "\n";
    out << "TOTAL_SPAWNS: " << total_spawns << "\n";
    out << "TOTAL_DESTINATIONS: " << total_destinations << "\n";
}
