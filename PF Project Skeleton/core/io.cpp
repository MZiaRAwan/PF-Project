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
    ifstream file;
    file.open(level_filename);
    if (!file.is_open()) {
        file.open("../" + level_filename);
    }
    
    if (!file.is_open()) {
        grid_loaded = 0;
        cout << "Error: Could not find level file: " << level_filename << "\n";
        return false;
    }

    total_spawns = 0;
    total_destinations = 0;
    total_switches = 0;
    total_trains = 0;
    
    string line;
    string section = "";
    
    while (getline(file, line))
    {
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
            for (int r = 0; r < rows; r++)
            {
                if (!getline(file, line))
                    break;
                    
                if (line == "SWITCHES:")
                {
                    section = "SWITCHES";
                    break;
                }
                
                while (line.length() < (size_t)cols)
                    line += ' ';
                
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


// Initialize log files
void initializeLogFiles()
{
    ofstream traceLog("out/trace.csv", ios::trunc);
    if (!traceLog.is_open()) {
        traceLog.open("trace.csv", ios::trunc);
    }
    if (traceLog.is_open()) {
        traceLog << "Tick,TrainID,X,Y,Direction,State\n";
        traceLog.close();
    }
    
    ofstream switchLog("out/switches.csv", ios::trunc);
    if (!switchLog.is_open()) {
        switchLog.open("switches.csv", ios::trunc);
    }
    if (switchLog.is_open()) {
        switchLog << "Tick,Switch,Mode,State\n";
        switchLog.close();
    }
    
    ofstream signalLog("out/signals.csv", ios::trunc);
    if (!signalLog.is_open()) {
        signalLog.open("signals.csv", ios::trunc);
    }
    if (signalLog.is_open()) {
        signalLog << "Tick,Switch,Signal\n";
        signalLog.close();
    }
}

void logTrainTrace()
{
    ofstream file("out/trace.csv", ios::app);
    if (!file.is_open()) {
        file.open("trace.csv", ios::app);
    }
    if (!file.is_open()) return;

    for (int i = 0; i < total_trains; i++)
    {
        if (train_x[i] < 0 || train_y[i] < 0) continue;
        
        int state = 0;
        if (train_arrived[i] || (train_x[i] == train_dest_x[i] && train_y[i] == train_dest_y[i] && train_dest_x[i] >= 0))
        {
            state = 1;
        }
        else if (train_active[i])
        {
            state = 0;
        }
        
        file << currentTick << ","
             << i << ","
             << train_x[i] << ","
             << train_y[i] << ","
             << train_dir[i] << ","
             << state << "\n";
    }
    
    file.close();
}

void logSwitchState()
{
    static int prev[max_switches];
    static bool first = true;
    static bool initial_logged = false;

    if (first)
    {
        for (int i = 0; i < max_switches; i++)
            prev[i] = switch_state[i];
        first = false;
    }

    ofstream file("out/switches.csv", ios::app);
    if (!file.is_open()) {
        file.open("switches.csv", ios::app);
    }
    if (!file.is_open()) return;

    if (!initial_logged && currentTick == 0)
    {
        for (int i = 0; i < max_switches; i++)
        {
            if (switch_x[i] < 0) continue;
            
            file << currentTick << ","
                 << char('A' + i) << ","
                 << (switch_mode[i] == 1 ? "GLOBAL" : "PER_DIR") << ","
                 << switch_state[i] << "\n";
        }
        initial_logged = true;
    }
    else
    {
        for (int i = 0; i < max_switches; i++)
        {
            if (switch_x[i] < 0) continue;
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
    
    file.close();
}

void logSignalState()
{
    ofstream file("out/signals.csv", ios::app);
    if (!file.is_open()) {
        file.open("signals.csv", ios::app);
    }
    if (!file.is_open()) return;

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
        if (switch_x[i] < 0) continue;
        
        int s = switch_signal[i];
        
        if (weather_type == weather_fog)
        {
            s = prev_signal[i];
        }
        
        string color =
            (s == signal_green)  ? "GREEN" :
            (s == signal_yellow) ? "YELLOW" :
                                   "RED";

        file << currentTick << "," << char('A' + i) << "," << color << "\n";
        
        prev_signal[i] = switch_signal[i];
    }
    
    file.close();
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