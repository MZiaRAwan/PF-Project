#include "grid.h"
#include "simulation_state.h"


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
    return (tile == 'D'); //D is destination point
}


bool toggleSafetyTile(char tile)
 {
 return (tile == '#' || tile == '.');
}
