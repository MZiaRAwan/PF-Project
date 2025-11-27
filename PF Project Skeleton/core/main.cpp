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

void printSwitches()
{
    cout << "\n=== SWITCH DEBUG ===\n";
    for (int i = 0; i < total_switches; i++)
    {
        cout << "Switch " << i
             << " Letter: " << char('A' + i)
             << " Row: " << switch_x[i]
             << " Col: " << switch_y[i]
             << " State: " << switch_state[i]
             << endl;
    }
    cout << "====================\n\n";
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
