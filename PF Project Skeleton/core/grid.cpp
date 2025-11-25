#include "grid.h"
#include "simulation_state.h"


void FetchgridFromFile(const char* levelfile)
{
    ifstream file(levelfile);

    if (!file.is_open())
     {
        cout << "Error: cannot open level file!\n";
        grid_loaded = 0;
        return;
    }
     file >> rows >> cols; //for reading rows and columns from file

       string line;

    getline(file, line); //to remove the leftover lines

     for (int r = 0; r < rows; r++) {   //grid file ma sa fetch krny k lye
        getline(file, line);//file ma sa line by line read krny k lye
        
     while (line.length() < cols) { // If the line is shorter than the number of columnsit fills the rest with spaces
         line += ' ';
         }
         if (line.length() > cols) {   //for trimming long lines
        line = line.substr(0, cols);
}

      
        for (int c = 0; c < cols; c++) {
            grid[r][c] = line[c];
        }
     }

    file.close();
    grid_loaded = 1;
    }
void printgrid() //grid print krny k lye
{
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            cout << grid[r][c];
        }
        cout << endl;
    }
}
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
