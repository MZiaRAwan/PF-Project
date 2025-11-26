#ifndef GRID_H
#define GRID_H

void printgrid();

// Check if a position is within grid bounds
bool isInBounds(int r, int c);

// Check if a tile is a track (can trains move on it?)
bool isTrackTile(char tile);

// Check if a tile is a switch (A-Z)
bool isSwitchTile(char tile);

// Get the switch index (0-25) from a switch character (A-Z)
int getSwitchIndex(char tile);

// Check if a position is a spawn point
bool isSpawnPoint(char tile);

// Check if a position is a destination point
bool isDestinationPoint(char tile);

// Place or remove a safety tile at a position (for mouse editing)
// Returns true if successful
bool toggleSafetyTile(char tile);

#endif
