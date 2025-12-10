# Switchback Rails - Railway Simulation Game

[![C++](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/)
[![SFML](https://img.shields.io/badge/SFML-2.x-green.svg)](https://www.sfml-dev.org/)
[![License](https://img.shields.io/badge/License-Academic-orange.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://www.linux.org/)

A deterministic railway simulation game implementing deferred switch mechanics, dynamic train scheduling, and collision detection. Built with C++ and SFML graphics library.

## 🚂 Overview

Switchback Rails is a sophisticated railway simulation that models train movement, switch operations, and traffic management on a grid-based railway network. The simulation features:

- **Deterministic 7-phase tick execution** for predictable behavior
- **Deferred switch mechanics** with counter-based logic
- **Dynamic train scheduling** with spawn queues
- **Advanced collision detection** with distance-based priority system
- **Visual interface** using SFML graphics
- **Weather effects** (NORMAL, RAIN, FOG)
- **Signal lights** (GREEN, YELLOW, RED)
- **Multiple difficulty levels** from easy to complex networks

## 📋 Table of Contents

- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [Features](#features)
- [Controls](#controls)
- [Levels](#levels)
- [Output Files](#output-files)
- [Requirements](#requirements)
- [Building the Project](#building-the-project)
- [Documentation](#documentation)

## 🚀 Quick Start

### 1. Install Dependencies

```bash
cd "PF Project Skeleton"
bash libraries.sh
```

This will install:
- GCC/G++ compiler
- SFML development libraries
- Build tools (make, cmake)
- Required system libraries

### 2. Build the Project

```bash
cd "PF Project Skeleton"
make clean
make
```

### 3. Run the Simulation

```bash
# Run with default level (complex network)
make run

# Or run specific levels
./switchback_rails data/levels/easy_level.lvl
./switchback_rails data/levels/medium_level.lvl
./switchback_rails data/levels/hard_level.lvl
./switchback_rails data/levels/complex_network.lvl
```

## 📁 Project Structure

```
PF-Project-main/
├── PF Project Skeleton/          # Main project directory
│   ├── core/                      # Core simulation logic
│   │   ├── simulation.*          # Main tick loop (7-phase execution)
│   │   ├── trains.*              # Train movement, routing, collision detection
│   │   ├── switches.*            # Switch counter logic and deferred flips
│   │   ├── grid.*                # Grid utilities and track validation
│   │   ├── io.*                  # Level file parsing and CSV output
│   │   └── simulation_state.*    # Global simulation state
│   ├── sfml/                      # SFML visual interface
│   │   ├── app.*                 # Application window and rendering
│   │   └── main.cpp              # Entry point
│   ├── data/levels/               # Level files (.lvl)
│   │   ├── easy_level.lvl
│   │   ├── medium_level.lvl
│   │   ├── hard_level.lvl
│   │   └── complex_network.lvl
│   ├── Sprites/                   # Train sprite images
│   ├── Makefile                   # Build configuration
│   ├── libraries.sh               # Dependency installation script
│   └── README.md                  # Detailed documentation
├── Sample Board and Sprites run/  # Sample implementation
└── README.md                       # This file
```

## ✨ Features

### Core Simulation Features

- ✅ **7-Phase Tick Execution**
  - Phase 1: Spawn trains
  - Phase 2: Determine movements
  - Phase 3: Update switch counters
  - Phase 4: Process flip queue
  - Phase 5: Execute movements
  - Phase 6: Apply deferred flips
  - Phase 7: Check arrivals

- ✅ **Deferred Switch Flips** - Switches flip after movement phase
- ✅ **Direction-Conditioned Switches** - PER_DIR & GLOBAL modes
- ✅ **Spawn Queue** - Trains wait if spawn tile is occupied
- ✅ **Distance-Based Collision Priority** - Higher distance = higher priority
- ✅ **3 Collision Types** - Same-destination, head-on swap, crossing
- ✅ **Signal Lights** - GREEN/YELLOW/RED based on train proximity
- ✅ **Weather Effects** - NORMAL/RAIN/FOG with different behaviors
- ✅ **Safety Tiles** - 1-tick delay mechanism
- ✅ **Emergency Halt** - 3×3 zone detection
- ✅ **Deterministic Simulation** - SEED-based reproducible runs
- ✅ **Fast Spawn Timing** - New trains every 4 ticks

### Visual Features

- Real-time grid visualization
- Train sprite rendering
- Switch state indicators
- Signal light visualization
- Interactive camera controls
- Zoom and pan functionality

## 🎮 Controls

| Key/Action | Function |
|------------|----------|
| **SPACE** | Pause/Resume simulation |
| **. (period)** | Step forward one tick |
| **Left-click** | Toggle safety tile (=) |
| **Right-click** | Toggle switch state |
| **Middle-drag** | Pan camera |
| **Mouse wheel** | Zoom in/out |
| **ESC** | Exit and save metrics |

## 🗺️ Levels

The project includes four difficulty levels:

1. **easy_level.lvl** - 2 trains, simple railway with minimal switches (NORMAL weather)
2. **medium_level.lvl** - 5 trains, intersecting routes with crossings (NORMAL weather)
3. **hard_level.lvl** - 8 trains, complex network challenge with multiple intersections (NORMAL weather)
4. **complex_network.lvl** - 10 trains, complex interconnected network (NORMAL weather)

All trains spawn from 'S' (source) tiles and navigate to 'D' (destination) tiles.

### Changing Weather

Edit any `.lvl` file and change the `WEATHER:` line:
- `NORMAL` - Constant speed, standard behavior
- `RAIN` - Occasional slowdowns every 5 moves
- `FOG` - Signal lights delayed by 1 tick (visual challenge)

### Collision Priority System 🚂

When two trains would collide, instead of crashing both, the system uses **distance-based priority**:

- **Higher Distance = Higher Priority**: The train further from its destination gets to move
- **Lower Distance = Waits**: The closer train waits for the next tick
- **Equal Distance = Both Crash**: Tie-breaker scenario

**Example**: Train A (20 tiles away) meets Train B (8 tiles away) at a crossing
→ Train A continues, Train B waits

This creates more realistic and efficient train traffic flow!

## 📊 Output Files

After simulation, check the `out/` directory (created automatically):

- **trace.csv** - Complete train movement history with positions per tick
- **switches.csv** - Switch state changes per tick
- **signals.csv** - Signal light states (GREEN/YELLOW/RED) per tick
- **metrics.txt** - Final statistics and efficiency metrics

## 🔧 Requirements

### System Requirements

- **OS**: Linux (tested on Ubuntu/Debian-based systems)
- **Compiler**: GCC/G++ with C++11 support
- **Graphics**: SFML 2.x library
- **Build Tools**: Make, CMake

### Dependencies

All dependencies are automatically installed by `libraries.sh`:
- `gcc`, `g++`
- `libsfml-dev`
- `make`, `cmake`
- `build-essential`
- `libfreetype6-dev`
- `libx11-dev`, `libxrandr-dev`
- `libgl1-mesa-dev`, `libglu1-mesa-dev`
- `libopenal-dev`, `libflac-dev`, `libvorbis-dev`, `libogg-dev`

## 🛠️ Building the Project

### Standard Build

```bash
cd "PF Project Skeleton"
make clean    # Remove previous build artifacts
make          # Compile the project
```

### Build Targets

```bash
make          # Build the executable
make run      # Build and run with default level
make clean    # Clean build files and output
make help     # Show help message
```

### Manual Compilation

If you prefer manual compilation:

```bash
g++ -std=c++11 -Wall -Wextra -g \
    core/simulation_state.cpp core/grid.cpp core/trains.cpp \
    core/switches.cpp core/simulation.cpp core/io.cpp \
    sfml/app.cpp sfml/main.cpp \
    -o switchback_rails \
    -lsfml-graphics -lsfml-window -lsfml-system
```

## 📚 Documentation

For detailed documentation, see:
- **[PF Project Skeleton/README.md](PF%20Project%20Skeleton/README.md)** - Complete technical documentation
- **[Project_Fall_2025_AI_and_DS.pdf](Project_Fall_2025_AI_and_DS.pdf)** - Project specification document

## 🎯 Specification Compliance

Fully compliant with the Switchback Rails specification:
- ✅ 7-phase tick execution (spawn → determination → counters → flip queue → movement → deferred flip → arrivals)
- ✅ Deterministic spawn rules with queue retry
- ✅ All collision detection types implemented
- ✅ Deferred switch flips with counter reset
- ✅ Signal lights per specification
- ✅ Complete weather system
- ✅ Proper evidence file generation

Built using only Programming Fundamentals concepts (no classes/structs).

## 🐛 Troubleshooting

### Build Issues

**Problem**: `SFML libraries not found`
```bash
# Solution: Run the installation script
bash libraries.sh
```

**Problem**: `Permission denied` when running `libraries.sh`
```bash
# Solution: Make the script executable
chmod +x libraries.sh
```

**Problem**: Compilation errors
```bash
# Solution: Clean and rebuild
make clean
make
```

### Runtime Issues

**Problem**: Executable not found
```bash
# Solution: Ensure you're in the correct directory
cd "PF Project Skeleton"
./switchback_rails data/levels/easy_level.lvl
```

**Problem**: Level file not found
```bash
# Solution: Check the level file path
ls data/levels/
```

## 📝 License

This project is part of a Programming Fundamentals course assignment. 

**Note**: This is an academic project. Please respect academic integrity policies. For questions or issues, please refer to the course materials or contact the course instructor.

## 👥 Contributing

This is an academic project. While contributions are welcome for educational purposes, please note:

- This project is primarily for learning and demonstration
- For questions or issues, please refer to the course materials or contact the course instructor
- If you fork this project, please maintain academic integrity standards

## 📸 Screenshots

*Add screenshots of the simulation in action here!*

## 🎓 Academic Context

This project was developed as part of a Programming Fundamentals course, demonstrating:
- C++ programming fundamentals (no classes/structs)
- Algorithm design and implementation
- State management and simulation logic
- File I/O and data processing
- Graphics programming with SFML

---

**Happy Railroading! 🚂🚂🚂**
