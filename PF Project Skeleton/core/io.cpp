#include "io.h"
#include "simulation_state.h"
#include "grid.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
using namespace std;

bool loadLevelFile()
{
    // Try multiple paths to find the level file
    ifstream file;
    file.open(level_filename);
    if (!file.is_open()) {
        file.open("../" + level_filename);
    }
    
    if (!file.is_open()) {
        grid_loaded = 0;
        cerr << "Error: Could not find level file: " << level_filename << "\n";
        return false;
    }

    // Initialize counters
    total_spawns = 0;
    total_destinations = 0;
    total_switches = 0;
    total_trains = 0;
    
    string line;
    string section = "";
    
    // Read file line by line
    while (getline(file, line))
    {
        // Check for section headers
        if (line == "ROWS:")
        {
            getline(file, line);
            rows = stoi(line);
        }
        else if (line == "COLS:")
        {
            getline(file, line);
            cols = stoi(line);
        }
        else if (line == "SEED:")
        {
            getline(file, line);
            level_seed = stoi(line);
        }
        else if (line == "WEATHER:")
        {
            getline(file, line);
            if (line == "NORMAL" || line == "CLEAR")
                weather_type = weather_clear;
            else if (line == "RAIN")
                weather_type = weather_rain;
            else if (line == "FOG")
                weather_type = weather_fog;
            else
                weather_type = weather_clear;
        }
        else if (line == "MAP:")
        {
            section = "MAP";
            // Read map rows
            for (int r = 0; r < rows; r++)
            {
                if (!getline(file, line))
                    break;
                    
                // Stop if we hit SWITCHES section
                if (line == "SWITCHES:")
                {
                    section = "SWITCHES";
                    break;
                }
                
                // Pad line to required length
                while (line.length() < (size_t)cols)
                    line += ' ';
                
                // Process each cell in the row
                for (int c = 0; c < cols; c++)
                {
                    char cell = line[c];
                    grid[r][c] = cell;
                    
                    if (cell == 'S')
                    {
                        spawn_x[total_spawns] = r;
                        spawn_y[total_spawns] = c;
                        total_spawns++;
                    }
                    else if (cell == 'D')
                    {
                        dest_X[total_destinations] = r;
                        dest_Y[total_destinations] = c;
                        total_destinations++;
                    }
                    else if (cell >= 'A' && cell <= 'Z')
                    {
                        int idx = cell - 'A';
                        switch_x[idx] = r;
                        switch_y[idx] = c;
                    }
                    else if (cell == '=')
                    {
                        buffer_count++;
                    }
                }
            }
        }
        else if (line == "SWITCHES:")
        {
            section = "SWITCHES";
        }
        else if (line == "TRAINS:")
        {
            section = "TRAINS";
        }
        else if (section == "SWITCHES")
        {
            // Skip empty lines
            if (line.length() == 0)
                continue;
            
            // Parse switch line: letter mode init k_up k_right k_down k_left state0 state1
            char id;
            string modeStr;
            int init, ku, kr, kd, kl;
            string s0, s1;
            
            stringstream ss(line);
            if (ss >> id >> modeStr >> init >> ku >> kr >> kd >> kl >> s0 >> s1)
            {
                if (id >= 'A' && id <= 'Z')
                {
                    int idx = id - 'A';
                    switch_mode[idx] = (modeStr == "GLOBAL") ? 1 : 0;
                    switch_init[idx] = init;
                    switch_k_up[idx] = ku;
                    switch_k_right[idx] = kr;
                    switch_k_down[idx] = kd;
                    switch_k_left[idx] = kl;
                    switch_state0[idx] = s0;
                    switch_state1[idx] = s1;
                    switch_state[idx] = init;
                }
            }
        }
        else if (section == "TRAINS")
        {
            // Skip empty lines
            if (line.length() == 0)
                continue;
            
            // Parse train line: spawn_tick x y direction color_index
            int spawn_tick, x, y, dir, color;
            stringstream ss(line);
            if (ss >> spawn_tick >> x >> y >> dir >> color)
            {
                if (total_trains < max_trains)
                {
                    train_spawn_tick[total_trains] = spawn_tick;
                    train_x[total_trains] = x;
                    train_y[total_trains] = y;
                    train_dir[total_trains] = dir;
                    train_color_index[total_trains] = color;
                    train_active[total_trains] = false;
                    total_trains++;
                }
            }
        }
    }
    
    // Count switches found in map
    total_switches = 0;
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] != -1)
            total_switches++;
    }
    
    file.close();
    grid_loaded = 1;
    return true;
}


// =============================================
// LOGGING FUNCTIONS
// =============================================
void initializeLogFiles()
{
    // Create output directory if it doesn't exist (simplified - assumes it exists)
    // Write to out/ directory as per requirements
    // Try to create files in out/ directory, fallback to current directory
    ofstream traceLog("out/trace.csv");
    if (!traceLog.is_open()) traceLog.open("trace.csv");
    traceLog << "Tick,TrainID,X,Y,Direction,State\n";
    traceLog.close();
    
    ofstream switchLog("out/switches.csv");
    if (!switchLog.is_open()) switchLog.open("switches.csv");
    switchLog << "Tick,Switch,Mode,State\n";
    switchLog.close();
    
    ofstream signalLog("out/signals.csv");
    if (!signalLog.is_open()) signalLog.open("signals.csv");
    signalLog << "Tick,Switch,Signal\n";
    signalLog.close();
}

void logTrainTrace()
{
    ofstream file("out/trace.csv", ios::app);
    if (!file.is_open()) file.open("trace.csv", ios::app);
    if (!file.is_open()) return;

    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i]) continue;

        // Format: Tick,TrainID,X,Y,Direction,State
        // State: 0=active, 1=arrived, 2=crashed
        int state = 0;
        if (train_x[i] == train_dest_x[i] && train_y[i] == train_dest_y[i])
            state = 1;
        
        file << currentTick << ","
             << i << ","
             << train_x[i] << ","
             << train_y[i] << ","
             << train_dir[i] << ","
             << state << "\n";
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

    ofstream file("out/switches.csv", ios::app);
    if (!file.is_open()) file.open("switches.csv", ios::app);
    if (!file.is_open()) return;

    // Format: Tick,Switch,Mode,State
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue; // Skip switches not on map
        
        if (switch_state[i] != prev[i])
        {
            file << currentTick << ","
                 << char('A' + i) << ","
                 << (switch_mode[i] == 1 ? "GLOBAL" : "PER_DIR") << ","
                 << switch_state[i] << "\n";

            prev[i] = switch_state[i];
        }
    }
}

void logSignalState()
{
    ofstream file("out/signals.csv", ios::app);
    if (!file.is_open()) file.open("signals.csv", ios::app);
    if (!file.is_open()) return;

    // Apply fog delay: display previous tick's signal (logic remains correct)
    static int prev_signal[max_switches];
    static bool first = true;
    
    if (first)
    {
        for (int i = 0; i < max_switches; i++)
            prev_signal[i] = switch_signal[i];
        first = false;
    }
    
    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue; // Skip switches not on map
        
        int s = switch_signal[i];
        
        // Fog effect: delay signal display by 1 tick (but logic uses current signal)
        if (weather_type == weather_fog)
        {
            s = prev_signal[i]; // Display previous tick's signal
        }
        
        string color =
            (s == signal_green)  ? "GREEN" :
            (s == signal_yellow) ? "YELLOW" :
                                   "RED";

        file << currentTick << "," << char('A' + i) << "," << color << "\n";
        
        // Update previous signal for next tick
        prev_signal[i] = switch_signal[i];
    }
}

void writeMetrics()
{
    // Ensure out directory exists
    ofstream out("out/metrics.txt");
    if (!out.is_open()) 
    {
        // Fallback to current directory if out/ doesn't exist
        out.open("metrics.txt");
        if (!out.is_open()) return;
    }

    out << "TOTAL_ARRIVALS: " << arrival << "\n";
    out << "TOTAL_CRASHES: " << crashes << "\n";
    out << "FINISHED: " << (finished ? "YES" : "NO") << "\n";
    out << "TOTAL_TRAINS: " << total_trains << "\n";
    out << "TOTAL_SWITCHES: " << total_switches << "\n";
    out << "TOTAL_SPAWNS: " << total_spawns << "\n";
    out << "TOTAL_DESTINATIONS: " << total_destinations << "\n";
    out << "FINAL_TICK: " << currentTick << "\n";
    
    // Calculate required metrics
    // Throughput: trains delivered per 100 ticks
    double throughput = 0.0;
    if (currentTick > 0)
    {
        throughput = (arrival * 100.0) / currentTick;
    }
    out << "THROUGHPUT: " << throughput << " trains per 100 ticks\n";
    
    // Average Wait: mean idle ticks
    double avg_wait = 0.0;
    if (total_trains > 0)
    {
        avg_wait = (double)total_wait_ticks / total_trains;
    }
    out << "AVERAGE_WAIT: " << avg_wait << " ticks\n";
    
    // Signal Violations: entries against red
    out << "SIGNAL_VIOLATIONS: " << signal_violations << "\n";
    
    // Energy Efficiency: (trains × ticks)/buffers
    double energy_efficiency = 0.0;
    if (buffer_count > 0)
    {
        energy_efficiency = (double)total_train_ticks / buffer_count;
    }
    out << "ENERGY_EFFICIENCY: " << energy_efficiency << "\n";
    
    // Switch Flips: total number of switch state changes
    out << "SWITCH_FLIPS: " << total_switch_flips << "\n";
    
    // Calculate additional metrics
    if (total_trains > 0)
    {
        out << "SUCCESS_RATE: " << (arrival * 100.0 / total_trains) << "%\n";
    }
    
    out.close();
}