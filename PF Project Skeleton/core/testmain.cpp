#include<iostream>
#include "grid.h"
#include "simulation_state.h"
using namespace std;

int main() {
    FetchgridFromFile("level1.txt"); 
    printgrid();                      

    return 0;
}