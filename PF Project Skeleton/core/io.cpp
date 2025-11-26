#include "io.h"
#include "simulation_state.h"
#include "grid.h"
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using namespace std;
bool loadLevelFile()
{
    ifstream file("/home/ibrahim/Documents/GitHub/PF-Project/PF Project Skeleton/data/levels/complex_network.lvl");
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
                switch_state[total_switches] = 0;  
                total_switches++;
            }
        }
    }

    file.close();
    grid_loaded = 1;
    return true;
}
void initializeLogFiles() {// these lines creates the file if it doesn’t exist, or clears it if it already exists.
ofstream trainLog("train_log.csv");  //trainLog file where train positions will be saved
 ofstream switchLog("switch_log.csv");// switchLog file where switch events will be saved
ofstream crashLog("crash_log.csv");// crashLog file where crash data will be saved
ofstream arrivalLog("arrival_log.csv");// arrivalLog  file where arrivals will be saved


trainLog << "tick,id,x,y,dir\n";
switchLog << "tick,id,row,col,state\n"; // writing headers to csv files 
crashLog << "tick,x,y\n";  // it means that these csv files are going to store these things inside them
arrivalLog << "tick,trainID\n";

trainLog.close();  // We only opened the files to create/clear them and add headers.
switchLog.close();   //We close them now.
crashLog.close();  //Later, other functions will open them again  to add more rows.
arrivalLog.close();
}                    
// ----------------------------------------------------------------------------
// LOG TRAIN TRACE
// ----------------------------------------------------------------------------
// Append tick, train id, position, direction, state to trace.csv.
// ----------------------------------------------------------------------------
// this function opens the log train.csv file and writes inside tick, id, x,y ,direction and tick number of train

    void logTrainTrace()
{
    ofstream file("trace.csv", ios::app);
    if (!file.is_open()) 
    return;  //  here return; returns nothing but only ends the functions early so to protect from crash

    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i])
            continue;  // skips those trains that are not active

        file << currentTick << ","   // reading all the information about train
             << i << ","
             << train_x[i] << ","
             << train_y[i] << ","
             << train_dir[i] << "\n";
    }

    file.close();
}



// ----------------------------------------------------------------------------
// LOG SWITCH STATE
// ----------------------------------------------------------------------------
// Append tick, switch id/mode/state to switches.csv.
// ----------------------------------------------------------------------------

void logSwitchState()
{
    static int prev_state[max_switches];
    static bool firstTime = true;

    if (firstTime)
    {
        for (int i = 0; i < total_switches; i++)
        {
            prev_state[i] = switch_state[i];
        }
        firstTime = false;
    }

    // Open file in append mode
    ofstream file("switches.csv", ios::app);
    if (!file.is_open()) return;

    // Check every switch
    for (int i = 0; i < total_switches; i++)
    {
        int cur = switch_state[i];

        // Only log if the switch changed
        if (cur != prev_state[i])
        {
            file << currentTick << ","   
                 << i << ","        
                 << switch_x[i] << ","   
                 << switch_y[i] << ","   
                 << cur << "\n";      

            prev_state[i] = cur;         
        }
    }

    file.close();
}


// ----------------------------------------------------------------------------
// LOG SIGNAL STATE
// ----------------------------------------------------------------------------
// Append tick, switch id, signal color to signals.csv.
// ----------------------------------------------------------------------------

void logSignalState()
{
    ofstream file("signals.csv", ios::app);  //ios::app means append mode ,new data is added at the end.



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
    ofstream out("metrics.txt"); 
    if (!out.is_open()) 
        return;
    out << "TOTAL_ARRIVALS: " << arrival << "\n";
    out << "TOTAL_CRASHES: " << crashes << "\n";
    out << "FINISHED: " << (finished ? "YES" : "NO") << "\n";

    out << "TOTAL_TRAINS: " << total_trains << "\n";
    out << "TOTAL_SWITCHES: " << total_switches << "\n";
    out << "TOTAL_SPAWNS: " << total_spawns << "\n";
    out << "TOTAL_DESTINATIONS: " << total_destinations << "\n";

    out.close();
}
