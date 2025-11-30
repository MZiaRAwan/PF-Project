#include "app.h"
#include "../core/simulation_state.h"
#include "../core/simulation.h"
#include "../core/grid.h"
#include "../core/switches.h"
#include "../core/io.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <iostream>

// ============================================================================
// APP.CPP - Implementation of SFML application (NO CLASSES)
// ============================================================================

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES FOR APP STATE
// ----------------------------------------------------------------------------
static sf::RenderWindow *g_window = nullptr;
static sf::Font g_font;
static bool g_fontLoaded = false;

// View for camera (panning/zoom)
static sf::View g_camera;

// Simulation state
static bool g_isPaused = false;
static bool g_isStepMode = false;

// Mouse state
static bool g_isMiddleDragging = false;
static bool g_isLeftDragging = false;
static int g_lastMouseX = 0;
static int g_lastMouseY = 0;
static int g_dragStartX = 0;
static int g_dragStartY = 0;

// Grid rendering parameters
static float g_cellSize = 48.0f;   // 48x48 pixels per tile
static float g_gridOffsetX = 0.0f; // Will be calculated to center the grid
static float g_gridOffsetY = 0.0f; // Will be calculated to center the grid

// Sprite textures and sprites
static std::map<std::string, sf::Texture> g_textures;
static std::map<char, sf::Sprite> g_tileSprites;
static std::map<int, sf::Sprite> g_trainSprites; // Color index -> sprite

// ----------------------------------------------------------------------------
// HELPER: Load sprite texture
// ----------------------------------------------------------------------------
bool loadTexture(const std::string &name, const std::string &filename)
{
    sf::Texture texture;
    if (texture.loadFromFile(filename))
    {
        g_textures[name] = texture;
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// HELPER: Draw text (with font fallback)
// ----------------------------------------------------------------------------
void drawText(sf::RenderWindow *window, const std::string &text, float x, float y,
              sf::Color color = sf::Color::White, int size = 12)
{
    if (g_fontLoaded)
    {
        sf::Text sfText;
        sfText.setFont(g_font);
        sfText.setString(text);
        sfText.setCharacterSize(size);
        sfText.setFillColor(color);
        sfText.setPosition(x, y);
        window->draw(sfText);
    }
    else
    {
        // Fallback: draw simple indicator for text (small colored square)
        // This is a placeholder - in production you'd want proper font rendering
        sf::RectangleShape indicator(sf::Vector2f(size * 0.6f, size * 0.6f));
        indicator.setPosition(x, y);
        indicator.setFillColor(color);
        window->draw(indicator);
    }
}

// ----------------------------------------------------------------------------
// HELPER: Draw track segment
// ----------------------------------------------------------------------------
void drawTrack(sf::RenderWindow *window, char tile, float x, float y)
{
    if (tile == '-')
    {
        // Horizontal track - yellow dotted line
        for (int i = 0; i < 4; i++)
        {
            sf::RectangleShape dot(sf::Vector2f(g_cellSize * 0.15f, 2));
            dot.setPosition(x + i * g_cellSize * 0.25f, y + g_cellSize * 0.5f);
            dot.setFillColor(sf::Color(255, 255, 0)); // Yellow
            window->draw(dot);
        }
    }
    else if (tile == '|')
    {
        // Vertical track - white solid line
        sf::RectangleShape line(sf::Vector2f(2, g_cellSize));
        line.setPosition(x + g_cellSize * 0.5f, y);
        line.setFillColor(sf::Color::White);
        window->draw(line);
    }
    else if (tile == '+' || tile == '/')
    {
        // Track crossing or curve - draw both
        sf::RectangleShape hLine(sf::Vector2f(g_cellSize, 2));
        hLine.setPosition(x, y + g_cellSize * 0.5f);
        hLine.setFillColor(sf::Color::White);
        window->draw(hLine);

        sf::RectangleShape vLine(sf::Vector2f(2, g_cellSize));
        vLine.setPosition(x + g_cellSize * 0.5f, y);
        vLine.setFillColor(sf::Color::White);
        window->draw(vLine);
    }
}

// ----------------------------------------------------------------------------
// HELPER: Get train sprite for color index
// ----------------------------------------------------------------------------
sf::Sprite getTrainSprite(int colorIndex)
{
    // Use sprite 1 for trains, color them based on index
    std::string texName = "sprite1";
    if (g_textures.find(texName) != g_textures.end())
    {
        sf::Sprite sprite(g_textures[texName]);
        sprite.setScale(g_cellSize / 32.0f, g_cellSize / 32.0f);

        // Apply color tint based on color index
        sf::Color colors[] = {
            sf::Color::Red,
            sf::Color::Blue,
            sf::Color::Green,
            sf::Color::Yellow,
            sf::Color::Magenta,
            sf::Color::Cyan,
            sf::Color::White,
            sf::Color(255, 165, 0) // Orange
        };
        sprite.setColor(colors[colorIndex % 8]);
        return sprite;
    }

    sf::Sprite sprite;
    return sprite;
}

// ----------------------------------------------------------------------------
// INITIALIZATION
// ----------------------------------------------------------------------------
bool initializeApp()
{
    // Create window
    g_window = new sf::RenderWindow(sf::VideoMode(1200, 800), "Switchback Rails - Railway Simulation");
    g_window->setFramerateLimit(60);

    // Initialize camera view
    g_camera = g_window->getDefaultView();

    // Load sprites (try multiple paths)
    std::vector<std::string> spritePaths = {
        "Sprites/",
        "../Sprites/",
        "PF Project Skeleton/Sprites/",
        "../PF Project Skeleton/Sprites/"};

    bool spritesLoaded = false;
    for (const auto &path : spritePaths)
    {
        if (loadTexture("sprite1", path + "1.png") &&
            loadTexture("sprite2", path + "2.png") &&
            loadTexture("sprite3", path + "3.png") &&
            loadTexture("sprite4", path + "4.png") &&
            loadTexture("sprite5", path + "5.png"))
        {
            spritesLoaded = true;
            break;
        }
    }

    if (!spritesLoaded)
    {
        std::cerr << "Warning: Could not load sprite files. Using colored rectangles instead.\n";
    }

    // Try to load font (optional)
    // Try common system fonts
    std::vector<std::string> fontPaths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "C:/Windows/Fonts/arial.ttf"};

    for (const auto &path : fontPaths)
    {
        if (g_font.loadFromFile(path))
        {
            g_fontLoaded = true;
            break;
        }
    }

    // If no font loaded, we'll use simple shapes for text representation
    if (!g_fontLoaded)
    {
        std::cerr << "Warning: Could not load font. Text rendering will be limited.\n";
    }

    return g_window != nullptr;
}

// ----------------------------------------------------------------------------
// HELPER: Center camera on the grid
// ----------------------------------------------------------------------------
void centerCameraOnGrid()
{
    if (grid_loaded == 0 || rows == 0 || cols == 0)
        return;

    // Calculate grid dimensions in world coordinates
    float gridWidth = cols * g_cellSize;
    float gridHeight = rows * g_cellSize;

    // Get window size
    float windowWidth = g_window->getSize().x;
    float windowHeight = g_window->getSize().y;

    // Calculate zoom to fit grid with padding
    // We want to show the entire grid with some padding
    // If grid is larger than window, we need to zoom out (larger view size)
    // If grid is smaller than window, we can zoom in (smaller view size)
    // Use 1.2f multiplier to add 20% padding around the grid
    float padding = 1.2f;
    float zoomX = (gridWidth * padding) / windowWidth;
    float zoomY = (gridHeight * padding) / windowHeight;
    // Use the larger zoom to ensure entire grid is visible
    float zoom = (zoomX > zoomY) ? zoomX : zoomY;

    // Calculate the view size after zoom
    // Larger view size = more zoomed out = shows more of the world
    float viewWidth = windowWidth * zoom;
    float viewHeight = windowHeight * zoom;

    // Calculate grid center position
    float gridCenterX = gridWidth / 2.0f;
    float gridCenterY = gridHeight / 2.0f;

    // Set grid offset so grid starts at (0, 0) in world coordinates
    // This means grid center will be at (gridCenterX, gridCenterY)
    g_gridOffsetX = 0.0f;
    g_gridOffsetY = 0.0f;

    // Create a new view centered on the grid center
    // The view center should be at the grid center in world coordinates
    g_camera = sf::View();
    g_camera.setCenter(gridCenterX, gridCenterY);
    g_camera.setSize(viewWidth, viewHeight);

    // Apply the view to the window immediately
    g_window->setView(g_camera);
}

// ----------------------------------------------------------------------------
// HELPER: Convert grid coordinates to screen coordinates
// ----------------------------------------------------------------------------
sf::Vector2f gridToScreen(int row, int col)
{
    float x = g_gridOffsetX + col * g_cellSize;
    float y = g_gridOffsetY + row * g_cellSize;
    return sf::Vector2f(x, y);
}

// ----------------------------------------------------------------------------
// HELPER: Convert screen coordinates to grid coordinates
// ----------------------------------------------------------------------------
sf::Vector2i screenToGrid(float screenX, float screenY)
{
    // Account for camera view
    sf::Vector2f worldPos = g_window->mapPixelToCoords(sf::Vector2i(screenX, screenY), g_camera);

    int col = (int)((worldPos.x - g_gridOffsetX) / g_cellSize);
    int row = (int)((worldPos.y - g_gridOffsetY) / g_cellSize);
    return sf::Vector2i(row, col);
}

// ----------------------------------------------------------------------------
// RENDER GRID
// ----------------------------------------------------------------------------
void renderGrid()
{
    if (!g_window || grid_loaded == 0)
        return;

    // Grid lines removed - no background grid visible

    // Render each tile
    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            char tile = grid[r][c];
            if (tile == '.' || tile == ' ')
                continue; // Skip empty tiles

            sf::Vector2f pos = gridToScreen(r, c);

            // Draw tracks first (with background)
            if (tile == '-' || tile == '|' || tile == '+' || tile == '/' || tile == '\\')
            {
                // Draw track background rectangle for better visibility
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40)); // Dark background
                g_window->draw(bgRect);

                drawTrack(g_window, tile, pos.x, pos.y);
            }

            // Draw spawn point - Green circle with 'S'
            if (tile == 'S')
            {
                // Draw background
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40));
                g_window->draw(bgRect);

                sf::CircleShape circle(g_cellSize * 0.4f);
                circle.setPosition(pos.x + g_cellSize * 0.1f, pos.y + g_cellSize * 0.1f);
                circle.setFillColor(sf::Color::Green);
                g_window->draw(circle);
                drawText(g_window, "S", pos.x + g_cellSize * 0.35f, pos.y + g_cellSize * 0.3f, sf::Color::White, 16);
            }

            // Draw destination - Colored circle with 'D'
            if (tile == 'D')
            {
                // Draw background
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40));
                g_window->draw(bgRect);

                sf::CircleShape circle(g_cellSize * 0.4f);
                circle.setPosition(pos.x + g_cellSize * 0.1f, pos.y + g_cellSize * 0.1f);
                circle.setFillColor(sf::Color(255, 165, 0)); // Orange
                g_window->draw(circle);
                drawText(g_window, "D", pos.x + g_cellSize * 0.35f, pos.y + g_cellSize * 0.3f, sf::Color::White, 16);
            }

            // Draw safety buffer - Colored rectangle
            if (tile == '=')
            {
                // Draw background
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40));
                g_window->draw(bgRect);

                // Draw track line first (safety buffer is on a track)
                drawTrack(g_window, '-', pos.x, pos.y);

                sf::RectangleShape rect(sf::Vector2f(g_cellSize * 0.8f, g_cellSize * 0.3f));
                rect.setPosition(pos.x + g_cellSize * 0.1f, pos.y + g_cellSize * 0.35f);
                rect.setFillColor(sf::Color(128, 128, 128)); // Grey
                g_window->draw(rect);
            }

            // Draw switch - Diamond shape with letter
            if (tile >= 'A' && tile <= 'Z')
            {
                // Draw background
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40));
                g_window->draw(bgRect);

                // Draw track lines for switch (switches are on tracks)
                drawTrack(g_window, '+', pos.x, pos.y);

                // Draw diamond shape
                sf::ConvexShape diamond;
                diamond.setPointCount(4);
                diamond.setPoint(0, sf::Vector2f(g_cellSize * 0.5f, 0));
                diamond.setPoint(1, sf::Vector2f(g_cellSize, g_cellSize * 0.5f));
                diamond.setPoint(2, sf::Vector2f(g_cellSize * 0.5f, g_cellSize));
                diamond.setPoint(3, sf::Vector2f(0, g_cellSize * 0.5f));
                diamond.setPosition(pos);
                diamond.setFillColor(sf::Color(100, 100, 255)); // Lighter blue for visibility
                diamond.setOutlineThickness(2);
                diamond.setOutlineColor(sf::Color::White);
                g_window->draw(diamond);

                // Draw switch letter
                std::string letter(1, tile);
                drawText(g_window, letter, pos.x + g_cellSize * 0.35f, pos.y + g_cellSize * 0.3f, sf::Color::White, 18);
            }

            // Draw track crossing - White '+' sign (already handled in track drawing above)
            // The '+' is drawn as part of the track rendering
        }
    }
}

// ----------------------------------------------------------------------------
// RENDER TRAINS
// ----------------------------------------------------------------------------
void renderTrains()
{
    if (!g_window)
        return;

    // Train colors based on color index
    sf::Color trainColors[] = {
        sf::Color::Red,
        sf::Color::Blue,
        sf::Color::Green,
        sf::Color::Yellow,
        sf::Color::Magenta,
        sf::Color::Cyan,
        sf::Color(255, 165, 0),  // Orange
        sf::Color(255, 192, 203) // Pink
    };

    for (int i = 0; i < total_trains; i++)
    {
        if (!train_active[i])
            continue;

        // Check bounds - train_x is row, train_y is col
        int row = train_x[i];
        int col = train_y[i];

        if (!isInBounds(row, col))
            continue;

        sf::Vector2f pos = gridToScreen(row, col);

        // Draw train as colored circle - make it larger and more visible
        sf::Color trainColor = trainColors[train_color_index[i] % 8];

        // Draw a larger, more visible train circle with white outline
        sf::CircleShape trainCircle(g_cellSize * 0.45f);
        trainCircle.setPosition(pos.x + g_cellSize * 0.05f, pos.y + g_cellSize * 0.05f);
        trainCircle.setFillColor(trainColor);
        trainCircle.setOutlineThickness(3.0f);
        trainCircle.setOutlineColor(sf::Color::White);
        g_window->draw(trainCircle);

        // Draw train ID/letter on the circle
        char trainChar = (i < 26) ? ('A' + i) : ('0' + (i - 26));
        std::string trainLabel(1, trainChar);
        drawText(g_window, trainLabel, pos.x + g_cellSize * 0.35f, pos.y + g_cellSize * 0.3f, sf::Color::White, 18);
    }
}

// ----------------------------------------------------------------------------
// RENDER SIGNAL LIGHTS
// ----------------------------------------------------------------------------
void renderSignals()
{
    if (!g_window)
        return;

    for (int i = 0; i < max_switches; i++)
    {
        if (switch_x[i] < 0)
            continue;

        sf::Vector2f pos = gridToScreen(switch_x[i], switch_y[i]);
        sf::CircleShape light(g_cellSize * 0.15f);
        light.setPosition(pos.x + g_cellSize * 0.7f, pos.y + g_cellSize * 0.1f);

        // Color based on signal state
        if (switch_signal[i] == signal_green)
            light.setFillColor(sf::Color::Green);
        else if (switch_signal[i] == signal_yellow)
            light.setFillColor(sf::Color::Yellow);
        else
            light.setFillColor(sf::Color::Red);

        g_window->draw(light);
    }
}

// ----------------------------------------------------------------------------
// RENDER STATISTICS PANEL (Top Left) - Small Box
// ----------------------------------------------------------------------------
void renderStatistics()
{
    if (!g_window)
        return;

    float panelX = 10;
    float panelY = 10;
    float lineHeight = 18;
    float fontSize = 12;

    // Smaller background panel
    sf::RectangleShape panel(sf::Vector2f(180, 140));
    panel.setPosition(panelX, panelY);
    panel.setFillColor(sf::Color(0, 0, 0, 200)); // Semi-transparent black
    panel.setOutlineColor(sf::Color::White);
    panel.setOutlineThickness(2);
    g_window->draw(panel);

    // Count active trains
    int activeCount = 0;
    for (int i = 0; i < total_trains; i++)
    {
        if (train_active[i])
            activeCount++;
    }

    // Weather string
    std::string weatherStr = "NORMAL";
    if (weather_type == weather_rain)
        weatherStr = "RAIN";
    else if (weather_type == weather_fog)
        weatherStr = "FOG";

    // Status string
    std::string statusStr = g_isPaused ? "PAUSED" : "RUNNING";

    // Draw statistics text (smaller font)
    float y = panelY + 8;
    drawText(g_window, "Tick: " + std::to_string(currentTick), panelX + 8, y, sf::Color::White, fontSize);
    y += lineHeight;
    drawText(g_window, "Active: " + std::to_string(activeCount), panelX + 8, y, sf::Color::White, fontSize);
    y += lineHeight;
    drawText(g_window, "Delivered: " + std::to_string(arrival), panelX + 8, y, sf::Color::White, fontSize);
    y += lineHeight;
    drawText(g_window, "Crashed: " + std::to_string(crashes), panelX + 8, y, sf::Color::White, fontSize);
    y += lineHeight;
    drawText(g_window, "Status: " + statusStr, panelX + 8, y, g_isPaused ? sf::Color::Red : sf::Color::Green, fontSize);
    y += lineHeight;
    drawText(g_window, "Weather: " + weatherStr, panelX + 8, y, sf::Color::White, fontSize);
    y += lineHeight;
    drawText(g_window, "SPACE: Pause | .: Step", panelX + 8, y, sf::Color(200, 200, 200), fontSize - 2);
}

// ----------------------------------------------------------------------------
// RENDER LEGEND (Bottom Right) - Small Box
// ----------------------------------------------------------------------------
void renderLegend()
{
    if (!g_window)
        return;

    sf::Vector2u windowSize = g_window->getSize();
    float panelX = windowSize.x - 180;
    float panelY = windowSize.y - 150;
    float lineHeight = 20;
    float fontSize = 11;

    // Smaller background panel
    sf::RectangleShape panel(sf::Vector2f(170, 140));
    panel.setPosition(panelX, panelY);
    panel.setFillColor(sf::Color(0, 0, 0, 200)); // Semi-transparent black
    panel.setOutlineColor(sf::Color::White);
    panel.setOutlineThickness(2);
    g_window->draw(panel);

    float y = panelY + 8;
    float iconX = panelX + 8;
    float textX = panelX + 35;

    // S = Spawn Point
    sf::CircleShape spawnIcon(6);
    spawnIcon.setPosition(iconX, y);
    spawnIcon.setFillColor(sf::Color::Green);
    g_window->draw(spawnIcon);
    drawText(g_window, "S = Spawn", textX, y, sf::Color::White, fontSize);
    y += lineHeight;

    // D = Destination
    sf::CircleShape destIcon(6);
    destIcon.setPosition(iconX, y);
    destIcon.setFillColor(sf::Color(255, 165, 0)); // Orange
    g_window->draw(destIcon);
    drawText(g_window, "D = Dest", textX, y, sf::Color::White, fontSize);
    y += lineHeight;

    // = = Safety Buffer
    sf::RectangleShape bufferIcon(sf::Vector2f(12, 6));
    bufferIcon.setPosition(iconX, y + 3);
    bufferIcon.setFillColor(sf::Color(128, 128, 128)); // Grey
    g_window->draw(bufferIcon);
    drawText(g_window, "= = Buffer", textX, y, sf::Color::White, fontSize);
    y += lineHeight;

    // + = Track Crossing
    sf::RectangleShape crossH(sf::Vector2f(10, 2));
    crossH.setPosition(iconX + 1, y + 5);
    crossH.setFillColor(sf::Color::White);
    g_window->draw(crossH);
    sf::RectangleShape crossV(sf::Vector2f(2, 10));
    crossV.setPosition(iconX + 5, y + 1);
    crossV.setFillColor(sf::Color::White);
    g_window->draw(crossV);
    drawText(g_window, "+ = Cross", textX, y, sf::Color::White, fontSize);
    y += lineHeight;

    // Diamond = Switch
    sf::ConvexShape switchIcon;
    switchIcon.setPointCount(4);
    switchIcon.setPoint(0, sf::Vector2f(6, 0));
    switchIcon.setPoint(1, sf::Vector2f(12, 6));
    switchIcon.setPoint(2, sf::Vector2f(6, 12));
    switchIcon.setPoint(3, sf::Vector2f(0, 6));
    switchIcon.setPosition(iconX, y);
    switchIcon.setFillColor(sf::Color::Blue);
    g_window->draw(switchIcon);
    drawText(g_window, "Diamond = Switch", textX, y, sf::Color::White, fontSize);
    y += lineHeight;

    // Circle = Train
    sf::CircleShape trainIcon(6);
    trainIcon.setPosition(iconX, y);
    trainIcon.setFillColor(sf::Color::Red);
    g_window->draw(trainIcon);
    drawText(g_window, "Circle = Train", textX, y, sf::Color::White, fontSize);
}

// ----------------------------------------------------------------------------
// RENDER UI
// ----------------------------------------------------------------------------
void renderUI()
{
    if (!g_window)
        return;

    renderStatistics();
    renderLegend();
}

// ----------------------------------------------------------------------------
// HANDLE MOUSE CLICK
// ----------------------------------------------------------------------------
void handleMouseClick(int x, int y, bool leftButton, bool rightButton)
{
    sf::Vector2i gridPos = screenToGrid(x, y);
    int row = gridPos.x;
    int col = gridPos.y;

    if (!isInBounds(row, col))
        return;

    if (leftButton)
    {
        // Left-click: place/remove safety tile
        char currentTile = grid[row][col];
        if (currentTile == '=')
        {
            // Remove safety tile - replace with appropriate track
            // Try to determine track type from neighbors
            char replacement = '-';
            if (row > 0 && (grid[row - 1][col] == '|' || grid[row - 1][col] == '+'))
                replacement = '|';
            if (row < rows - 1 && (grid[row + 1][col] == '|' || grid[row + 1][col] == '+'))
                replacement = '|';
            grid[row][col] = replacement;
        }
        else if (isTrackTile(currentTile) || currentTile == '.' || currentTile == ' ')
        {
            grid[row][col] = '='; // Place safety tile
        }
    }
    else if (rightButton)
    {
        // Right-click: toggle switch state
        char tile = grid[row][col];
        if (isSwitchTile(tile))
        {
            int switchIdx = getSwitchIndex(tile);
            if (switchIdx >= 0 && switchIdx < max_switches)
            {
                switch_state[switchIdx] = 1 - switch_state[switchIdx];
            }
        }
    }
}

// ----------------------------------------------------------------------------
// HANDLE EMERGENCY HALT TRIGGER
// ----------------------------------------------------------------------------
// Trigger emergency halt on a switch (middle-click or special key)
// Pauses trains in 3x3 neighborhood for 3 ticks
// ----------------------------------------------------------------------------
void handleEmergencyHaltTrigger(int x, int y)
{
    sf::Vector2i gridPos = screenToGrid(x, y);
    int row = gridPos.x;
    int col = gridPos.y;

    if (!isInBounds(row, col))
        return;

    char tile = grid[row][col];
    if (isSwitchTile(tile))
    {
        int switchIdx = getSwitchIndex(tile);
        if (switchIdx >= 0 && switchIdx < max_switches)
        {
            // Trigger emergency halt: pause trains in 3x3 zone for 3 ticks
            emergencyHalt = true;
            emergencyHaltTimer = 3; // 3 ticks duration
            // Store which switch triggered it (for visualization if needed)
            // The applyEmergencyHalt() function will check all switches
        }
    }
}

// ----------------------------------------------------------------------------
// MAIN RUN LOOP
// ----------------------------------------------------------------------------
void runApp()
{
    if (!g_window)
        return;

    // Center camera on grid after level is loaded
    // Wait a frame to ensure grid is loaded
    bool cameraCentered = false;

    sf::Clock simClock;
    float simInterval = 0.5f; // 2 ticks per second (0.5 seconds per tick)

    while (g_window->isOpen())
    {
        sf::Event event;
        while (g_window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                g_window->close();
            }

            // Keyboard events
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    // Save metrics and exit
                    writeMetrics();
                    g_window->close();
                }
                else if (event.key.code == sf::Keyboard::Space)
                {
                    // Toggle pause
                    g_isPaused = !g_isPaused;
                    g_isStepMode = false;
                }
                else if (event.key.code == sf::Keyboard::Period)
                {
                    // Step one tick forward
                    g_isStepMode = true;
                    g_isPaused = false;
                }
            }

            // Mouse wheel: zoom
            if (event.type == sf::Event::MouseWheelScrolled)
            {
                float zoomFactor = 1.0f + (event.mouseWheelScroll.delta * 0.1f);
                g_camera.zoom(zoomFactor);
                g_window->setView(g_camera);
            }

            // Mouse button press
            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    // Start left-click drag for panning
                    g_isLeftDragging = true;
                    g_lastMouseX = event.mouseButton.x;
                    g_lastMouseY = event.mouseButton.y;
                    // Don't handle click immediately - wait to see if it's a drag
                }
                else if (event.mouseButton.button == sf::Mouse::Right)
                {
                    handleMouseClick(event.mouseButton.x, event.mouseButton.y, false, true);
                }
                else if (event.mouseButton.button == sf::Mouse::Middle)
                {
                    // Middle-click: trigger emergency halt on switch, or start drag for panning
                    g_isMiddleDragging = true;
                    g_lastMouseX = event.mouseButton.x;
                    g_lastMouseY = event.mouseButton.y;
                    g_dragStartX = event.mouseButton.x; // Track start for click vs drag
                    g_dragStartY = event.mouseButton.y;
                }
            }

            // Mouse button release
            if (event.type == sf::Event::MouseButtonReleased)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    // If it was a short click (not a drag), handle it as a click
                    if (g_isLeftDragging)
                    {
                        int dx = abs(event.mouseButton.x - g_lastMouseX);
                        int dy = abs(event.mouseButton.y - g_lastMouseY);
                        // If movement was small, treat as click, not drag
                        if (dx < 5 && dy < 5)
                        {
                            handleMouseClick(event.mouseButton.x, event.mouseButton.y, true, false);
                        }
                        g_isLeftDragging = false;
                    }
                }
                else if (event.mouseButton.button == sf::Mouse::Middle)
                {
                    // Check if it was a click (small movement) or a drag
                    if (g_isMiddleDragging)
                    {
                        int dx = abs(event.mouseButton.x - g_dragStartX);
                        int dy = abs(event.mouseButton.y - g_dragStartY);
                        // If movement was small, treat as click for emergency halt
                        if (dx < 5 && dy < 5)
                        {
                            handleEmergencyHaltTrigger(event.mouseButton.x, event.mouseButton.y);
                        }
                        g_isMiddleDragging = false;
                    }
                }
            }

            // Mouse move: pan with left button or middle button
            if (event.type == sf::Event::MouseMoved)
            {
                if (g_isLeftDragging || g_isMiddleDragging)
                {
                    int dx = event.mouseMove.x - g_lastMouseX;
                    int dy = event.mouseMove.y - g_lastMouseY;

                    // Convert screen delta to world delta for smooth panning
                    // The camera move is in world coordinates, so we need to account for zoom
                    float zoomFactor = g_camera.getSize().x / g_window->getSize().x;
                    float worldDx = -dx * zoomFactor;
                    float worldDy = -dy * zoomFactor;

                    g_camera.move(worldDx, worldDy);
                    g_window->setView(g_camera);

                    g_lastMouseX = event.mouseMove.x;
                    g_lastMouseY = event.mouseMove.y;
                }
            }
        }

        // Update simulation
        if (!g_isPaused || g_isStepMode)
        {
            if (simClock.getElapsedTime().asSeconds() >= simInterval || g_isStepMode)
            {
                if (!isSimulationComplete())
                {
                    currentTick++;
                    simulateOneTick();
                }
                simClock.restart();
                g_isStepMode = false;
            }
        }

        // Center camera on first frame (after grid is loaded)
        if (!cameraCentered && grid_loaded != 0 && rows > 0 && cols > 0)
        {
            centerCameraOnGrid();
            cameraCentered = true;
        }

        // Render
        g_window->clear(sf::Color(30, 30, 30)); // Dark gray background

        // Always apply the camera view before rendering
        g_window->setView(g_camera);
        renderGrid();
        renderTrains(); // Render trains AFTER grid so they appear on top
        renderSignals();

        g_window->setView(g_window->getDefaultView());
        renderUI();

        g_window->display();
    }
}

// ----------------------------------------------------------------------------
// CLEANUP
// ----------------------------------------------------------------------------
void cleanupApp()
{
    if (g_window)
    {
        delete g_window;
        g_window = nullptr;
    }
    g_textures.clear();
    g_tileSprites.clear();
    g_trainSprites.clear();
}