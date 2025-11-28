#include "simulation_state.h"
#include "io.h"
#include "grid.h"
#include <iostream>
#include<sstream>
using namespace std;

void printGrid()
{
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
            cout << grid[r][c];
        cout << endl;
    }
    cout << "--------------------------\n";
}

// void printSwitches()
// {
//     cout << "\n=== SWITCH DEBUG ===\n";
//     for (int i = 0; i < total_switches; i++)
//     {
//         cout << "Switch " << i
//              << " Letter: " << char('A' + i)
//              << " Row: " << switch_x[i]
//              << " Col: " << switch_y[i]
//              << " State: " << switch_state[i]
//              << endl;
//     }
//     cout << "====================\n\n";
// }
void printSwitches()
{
    cout << "\n=== SWITCH DEBUG ===\n";

    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0) continue;  // skip non-existing switches

        char letter = 'A' + i;

        cout << "\nSwitch " << i << " (" << letter << ")\n";
        cout << "Position: (" << switch_x[i] << ", " << switch_y[i] << ")\n";

        cout << "Mode: " << switch_mode[i] << "\n";
        cout << "Initial State: " << switch_init[i] << "\n";

        cout << "K-UP: " << switch_k_up[i] << "\n";
        cout << "K-RIGHT: " << switch_k_right[i] << "\n";
        cout << "K-DOWN: " << switch_k_down[i] << "\n";
        cout << "K-LEFT: " << switch_k_left[i] << "\n";

        cout << "State0: " << switch_state0[i] << "\n";
        cout << "State1: " << switch_state1[i] << "\n";

        cout << "Current State (0/1): " << switch_state[i] << "\n";
    }

    cout << "\n====================\n\n";
}


int main()
{
    initializeSimulationState();
    initializeLogFiles();

    if (!loadLevelFile())
    {
        cout << "Error loading level file\n";
        return 0;
    }

    printSwitches();
    printGrid();
    writeMetrics();
    return 0;
}
