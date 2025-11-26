#include "simulation_state.h"
#include "io.h"
#include "grid.h"
#include <iostream>
using namespace std;

// Function to print the grid
void printGrid()
{
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            cout << grid[r][c];
        }
        cout << endl;
    }
    cout << "--------------------------\n";
}

int main()
{
    initializeSimulationState();   // reset everything
    initializeLogFiles();          // create empty CSV logs

    if (!loadLevelFile())
    {
        cout << "Error loading level file!\n";
        return 0;
    }

    // SIMPLE LOOP (run 20 ticks)
    for (int t = 0; t < 20; t++)
    {
        currentTick = t;

        // CALL YOUR LOGGING FUNCTIONS
        logTrainTrace();
        logSwitchState();
        logSignalState();

        // PRINT GRID ON SCREEN
        printGrid();
    }

    writeMetrics();  // write final summary

    return 0;
}
