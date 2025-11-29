#include "grid.h"
#include "simulation_state.h"
#include <iostream>
#include <string>
using namespace std;


bool isInBounds(int r, int c) {
    return r >= 0 && r < rows && c >= 0 && c < cols; //to check whether train is inside the defined grid or not
}




bool isTrackTile(char tile) {
    return (tile == '-' || tile == '|' || tile == '/' || tile == '\\' || tile == '+');// symbols k lye true return kry ga like +,/
}


bool isSwitchTile(char tile) {
    return (tile >= 'A' && tile <= 'Z');// because switch is represented by capital letters
}
int getSwitchIndex(char tile) {
    return tile - 'A';
}

 
bool isSpawnPoint(char tile) {
    return (tile == '>' || tile == '<' || tile == '^' || tile == 'v');// cz spawn is represented by <,>,^ etc, that is starting point of the train
}


bool isDestinationPoint(char tile)
{
    return (tile == 'D' || tile == 'E');  // E is also a destination in your map
}



bool toggleSafetyTile(char tile)
 {
 return (tile == '#' || tile == '.');
 }

// ----------------------------------------------------------------------------
// PRINT GRID WITH TRAINS (Terminal Output - Phase 7)
// ----------------------------------------------------------------------------
// Print complete grid state showing all tiles, train positions, and switch states.
// Required: Terminal output at each tick showing grid state.
// ----------------------------------------------------------------------------
void printGrid()
{
    // Create a display grid that includes trains
    char display_grid[max_rows][max_cols];
    
    // Copy base grid
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            display_grid[r][c] = grid[r][c];
        }
    }
    
    // Overlay trains on the grid (use train ID as character)
    for (int i = 0; i < total_trains; i++)
    {
        if (train_active[i] && isInBounds(train_x[i], train_y[i]))
        {
            // Use train ID (0-9) or letter for display
            if (i < 10)
                display_grid[train_x[i]][train_y[i]] = '0' + i;
            else
                display_grid[train_x[i]][train_y[i]] = 'A' + (i - 10);
        }
    }
    
    // Print tick number
    cout << "\nTick: " << currentTick << "\n";
    
    // Print grid
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            cout << display_grid[r][c];
        }
        cout << endl;
    }
    
    // Print train information
    for (int i = 0; i < total_trains; i++)
    {
        if (train_active[i])
        {
            string dir_str = "UNKNOWN";
            if (train_dir[i] == DIR_UP) dir_str = "UP";
            else if (train_dir[i] == DIR_RIGHT) dir_str = "RIGHT";
            else if (train_dir[i] == DIR_DOWN) dir_str = "DOWN";
            else if (train_dir[i] == DIR_LEFT) dir_str = "LEFT";
            
            cout << "Train " << i << " at (" << train_x[i] << "," << train_y[i] << ") moving " << dir_str << "\n";
        }
    }
    
    cout << "--------------------------\n";
}