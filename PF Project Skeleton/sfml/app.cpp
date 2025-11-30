#include "app.h"
#include "../core/simulation_state.h"
#include "../core/simulation.h"
#include "../core/grid.h"
#include "../core/switches.h"
#include "../core/io.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdio>
#include <string>
#include <iostream>
#include <cstdio>

// SFML application implementation
static sf::RenderWindow* g_window = nullptr;
static sf::Font g_font;
static bool g_fontLoaded = false;

static sf::View g_camera;
static bool g_isPaused = false;
static bool g_isStepMode = false;
static bool g_isMiddleDragging = false;
static bool g_isLeftDragging = false;
static int g_lastMouseX = 0;
static int g_lastMouseY = 0;
static int g_dragStartX = 0;
static int g_dragStartY = 0;
static float g_cellSize = 48.0f;
static float g_gridOffsetX = 0.0f;
static float g_gridOffsetY = 0.0f;

// Sprite textures - simple array
static sf::Texture g_textures[10];
static bool g_textureLoaded[10] = {false};

// Load sprite texture
bool loadTexture(int index, const char* filename) {
    if (index < 0 || index >= 10) return false;
    if (g_textures[index].loadFromFile(filename)) {
        g_textureLoaded[index] = true;
        return true;
    }
    return false;
}

// Draw text
void drawText(sf::RenderWindow* window, const std::string& text, float x, float y, 
              sf::Color color = sf::Color::White, int size = 12) {
    if (g_fontLoaded) {
        sf::Text sfText;
        sfText.setFont(g_font);
        sfText.setString(text);
        sfText.setCharacterSize(size);
        sfText.setFillColor(color);
        sfText.setPosition(x, y);
        window->draw(sfText);
    } else {
        sf::RectangleShape indicator(sf::Vector2f(size * 0.6f, size * 0.6f));
        indicator.setPosition(x, y);
        indicator.setFillColor(color);
        window->draw(indicator);
    }
}

// Draw track segment
void drawTrack(sf::RenderWindow* window, char tile, float x, float y) {
    if (tile == '-') {
        // Horizontal track - yellow dotted line
        for (int i = 0; i < 4; i++) {
            sf::RectangleShape dot(sf::Vector2f(g_cellSize * 0.15f, 2));
            dot.setPosition(x + i * g_cellSize * 0.25f, y + g_cellSize * 0.5f);
            dot.setFillColor(sf::Color(255, 255, 0));  // Yellow
            window->draw(dot);
        }
    } else if (tile == '|') {
        // Vertical track - white solid line
        sf::RectangleShape line(sf::Vector2f(2, g_cellSize));
        line.setPosition(x + g_cellSize * 0.5f, y);
        line.setFillColor(sf::Color::White);
        window->draw(line);
    } else if (tile == '+' || tile == '/') {
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

// Get train sprite
sf::Sprite getTrainSprite(int colorIndex) {
    sf::Sprite sprite;
    if (g_textureLoaded[0]) {
        sprite.setTexture(g_textures[0]);
        sprite.setScale(g_cellSize / 32.0f, g_cellSize / 32.0f);
        
        sf::Color colors[] = {
            sf::Color::Red,
            sf::Color::Blue,
            sf::Color::Green,
            sf::Color::Yellow,
            sf::Color::Magenta,
            sf::Color::Cyan,
            sf::Color::White,
            sf::Color(255, 165, 0)
        };
        sprite.setColor(colors[colorIndex % 8]);
    }
    return sprite;
}

// Initialize application
bool initializeApp() {
    // Create window
    g_window = new sf::RenderWindow(sf::VideoMode(1200, 800), "Switchback Rails - Railway Simulation");
    g_window->setFramerateLimit(60);
    
    // Initialize camera view
    g_camera = g_window->getDefaultView();
    
    // Load sprites (try multiple paths)
    const char* spritePaths[] = {
        "Sprites/",
        "../Sprites/",
        "PF Project Skeleton/Sprites/",
        "../PF Project Skeleton/Sprites/"
    };
    int numPaths = 4;
    
    bool spritesLoaded = false;
    for (int p = 0; p < numPaths; p++) {
        char path1[200], path2[200], path3[200], path4[200], path5[200];
        sprintf(path1, "%s1.png", spritePaths[p]);
        sprintf(path2, "%s2.png", spritePaths[p]);
        sprintf(path3, "%s3.png", spritePaths[p]);
        sprintf(path4, "%s4.png", spritePaths[p]);
        sprintf(path5, "%s5.png", spritePaths[p]);
        
        if (loadTexture(0, path1) && loadTexture(1, path2) && 
            loadTexture(2, path3) && loadTexture(3, path4) && 
            loadTexture(4, path5)) {
            spritesLoaded = true;
            break;
        }
    }
    
    if (!spritesLoaded) {
        std::cout << "Warning: Could not load sprite files. Using colored rectangles instead.\n";
    }
    
    // Try to load font (optional)
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "C:/Windows/Fonts/arial.ttf"
    };
    int numFontPaths = 4;
    
    for (int i = 0; i < numFontPaths; i++) {
        if (g_font.loadFromFile(fontPaths[i])) {
            g_fontLoaded = true;
            break;
        }
    }
    
    if (!g_fontLoaded) {
        std::cout << "Warning: Could not load font. Text rendering will be limited.\n";
    }
    
    return g_window != nullptr;
}

// Center camera on grid
void centerCameraOnGrid() {
    if (grid_loaded == 0 || rows == 0 || cols == 0) return;
    
    // Calculate grid dimensions in world coordinates
    float gridWidth = cols * g_cellSize;
    float gridHeight = rows * g_cellSize;
    
    // Get window size
    float windowWidth = g_window->getSize().x;
    float windowHeight = g_window->getSize().y;
    
    float padding = 1.2f;
    float zoomX = (gridWidth * padding) / windowWidth;
    float zoomY = (gridHeight * padding) / windowHeight;
    float zoom = (zoomX > zoomY) ? zoomX : zoomY;
    
    float viewWidth = windowWidth * zoom;
    float viewHeight = windowHeight * zoom;
    
    float gridCenterX = gridWidth / 2.0f;
    float gridCenterY = gridHeight / 2.0f;
    
    g_gridOffsetX = 0.0f;
    g_gridOffsetY = 0.0f;
    g_camera = sf::View();
    g_camera.setCenter(gridCenterX, gridCenterY);
    g_camera.setSize(viewWidth, viewHeight);
    
    g_window->setView(g_camera);
}

// Convert grid to screen coordinates
sf::Vector2f gridToScreen(int row, int col) {
    float x = g_gridOffsetX + col * g_cellSize;
    float y = g_gridOffsetY + row * g_cellSize;
    return sf::Vector2f(x, y);
}

// Convert screen to grid coordinates
sf::Vector2i screenToGrid(float screenX, float screenY) {
    sf::Vector2f worldPos = g_window->mapPixelToCoords(sf::Vector2i(screenX, screenY), g_camera);
    
    int col = (int)((worldPos.x - g_gridOffsetX) / g_cellSize);
    int row = (int)((worldPos.y - g_gridOffsetY) / g_cellSize);
    return sf::Vector2i(row, col);
}

// Render grid
void renderGrid() {
    if (!g_window || grid_loaded == 0) return;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            char tile = grid[r][c];
            if (tile == '.' || tile == ' ') continue;
            
            sf::Vector2f pos = gridToScreen(r, c);
            
            if (tile == '-' || tile == '|' || tile == '+' || tile == '/' || tile == '\\') {
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40));
                g_window->draw(bgRect);
                
                drawTrack(g_window, tile, pos.x, pos.y);
            }
            
            if (tile == 'S') {
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
            
            if (tile == 'D') {
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40));
                g_window->draw(bgRect);
                
                sf::CircleShape circle(g_cellSize * 0.4f);
                circle.setPosition(pos.x + g_cellSize * 0.1f, pos.y + g_cellSize * 0.1f);
                circle.setFillColor(sf::Color(255, 165, 0));
                g_window->draw(circle);
                drawText(g_window, "D", pos.x + g_cellSize * 0.35f, pos.y + g_cellSize * 0.3f, sf::Color::White, 16);
            }
            
            if (tile == '=') {
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40));
                g_window->draw(bgRect);
                
                drawTrack(g_window, '-', pos.x, pos.y);
                
                sf::RectangleShape rect(sf::Vector2f(g_cellSize * 0.8f, g_cellSize * 0.3f));
                rect.setPosition(pos.x + g_cellSize * 0.1f, pos.y + g_cellSize * 0.35f);
                rect.setFillColor(sf::Color(128, 128, 128));
                g_window->draw(rect);
            }
            
            if (tile >= 'A' && tile <= 'Z') {
                sf::RectangleShape bgRect(sf::Vector2f(g_cellSize - 2, g_cellSize - 2));
                bgRect.setPosition(pos.x + 1, pos.y + 1);
                bgRect.setFillColor(sf::Color(40, 40, 40));
                g_window->draw(bgRect);
                
                drawTrack(g_window, '+', pos.x, pos.y);
                
                sf::ConvexShape diamond;
                diamond.setPointCount(4);
                diamond.setPoint(0, sf::Vector2f(g_cellSize * 0.5f, 0));
                diamond.setPoint(1, sf::Vector2f(g_cellSize, g_cellSize * 0.5f));
                diamond.setPoint(2, sf::Vector2f(g_cellSize * 0.5f, g_cellSize));
                diamond.setPoint(3, sf::Vector2f(0, g_cellSize * 0.5f));
                diamond.setPosition(pos);
                diamond.setFillColor(sf::Color(100, 100, 255));
                diamond.setOutlineThickness(2);
                diamond.setOutlineColor(sf::Color::White);
                g_window->draw(diamond);
                
                std::string letter(1, tile);
                drawText(g_window, letter, pos.x + g_cellSize * 0.35f, pos.y + g_cellSize * 0.3f, sf::Color::White, 18);
            }
        }
    }
}

// Render trains
void renderTrains() {
    if (!g_window) return;
    
    sf::Color trainColors[] = {
        sf::Color::Red,
        sf::Color::Blue,
        sf::Color::Green,
        sf::Color::Yellow,
        sf::Color::Magenta,
        sf::Color::Cyan,
        sf::Color(255, 165, 0),
        sf::Color(255, 192, 203)
    };
    
    for (int i = 0; i < total_trains; i++) {
        if (!train_active[i]) continue;
        
        int row = train_x[i];
        int col = train_y[i];
        
        if (!isInBounds(row, col)) continue;
        
        sf::Vector2f pos = gridToScreen(row, col);
        
        sf::Color trainColor = trainColors[train_color_index[i] % 8];
        
        sf::CircleShape trainCircle(g_cellSize * 0.45f);
        trainCircle.setPosition(pos.x + g_cellSize * 0.05f, pos.y + g_cellSize * 0.05f);
        trainCircle.setFillColor(trainColor);
        trainCircle.setOutlineThickness(3.0f);
        trainCircle.setOutlineColor(sf::Color::White);
        g_window->draw(trainCircle);
        
        char trainChar = (i < 26) ? ('A' + i) : ('0' + (i - 26));
        std::string trainLabel(1, trainChar);
        drawText(g_window, trainLabel, pos.x + g_cellSize * 0.35f, pos.y + g_cellSize * 0.3f, sf::Color::White, 18);
    }
}

// Render signal lights
void renderSignals() {
    if (!g_window) return;
    
    for (int i = 0; i < max_switches; i++) {
        if (switch_x[i] < 0) continue;
        
        sf::Vector2f pos = gridToScreen(switch_x[i], switch_y[i]);
        sf::CircleShape light(g_cellSize * 0.15f);
        light.setPosition(pos.x + g_cellSize * 0.7f, pos.y + g_cellSize * 0.1f);
        
        if (switch_signal[i] == signal_green)
            light.setFillColor(sf::Color::Green);
        else if (switch_signal[i] == signal_yellow)
            light.setFillColor(sf::Color::Yellow);
        else
            light.setFillColor(sf::Color::Red);
        
        g_window->draw(light);
    }
}

// Render statistics panel
void renderStatistics() {
    if (!g_window) return;
    
    float panelX = 10;
    float panelY = 10;
    float lineHeight = 18;
    float fontSize = 12;
    
    sf::RectangleShape panel(sf::Vector2f(180, 140));
    panel.setPosition(panelX, panelY);
    panel.setFillColor(sf::Color(0, 0, 0, 200));
    panel.setOutlineColor(sf::Color::White);
    panel.setOutlineThickness(2);
    g_window->draw(panel);
    
    int activeCount = 0;
    for (int i = 0; i < total_trains; i++) {
        if (train_active[i]) activeCount++;
    }
    
    std::string weatherStr = "NORMAL";
    if (weather_type == weather_rain) weatherStr = "RAIN";
    else if (weather_type == weather_fog) weatherStr = "FOG";
    
    std::string statusStr = g_isPaused ? "PAUSED" : "RUNNING";
    
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

// Render legend
void renderLegend() {
    if (!g_window) return;
    
    sf::Vector2u windowSize = g_window->getSize();
    float panelX = windowSize.x - 180;
    float panelY = windowSize.y - 150;
    float lineHeight = 20;
    float fontSize = 11;
    
    sf::RectangleShape panel(sf::Vector2f(170, 140));
    panel.setPosition(panelX, panelY);
    panel.setFillColor(sf::Color(0, 0, 0, 200));
    panel.setOutlineColor(sf::Color::White);
    panel.setOutlineThickness(2);
    g_window->draw(panel);
    
    float y = panelY + 8;
    float iconX = panelX + 8;
    float textX = panelX + 35;
    
    sf::CircleShape spawnIcon(6);
    spawnIcon.setPosition(iconX, y);
    spawnIcon.setFillColor(sf::Color::Green);
    g_window->draw(spawnIcon);
    drawText(g_window, "S = Spawn", textX, y, sf::Color::White, fontSize);
    y += lineHeight;
    
    sf::CircleShape destIcon(6);
    destIcon.setPosition(iconX, y);
    destIcon.setFillColor(sf::Color(255, 165, 0));
    g_window->draw(destIcon);
    drawText(g_window, "D = Dest", textX, y, sf::Color::White, fontSize);
    y += lineHeight;
    
    sf::RectangleShape bufferIcon(sf::Vector2f(12, 6));
    bufferIcon.setPosition(iconX, y + 3);
    bufferIcon.setFillColor(sf::Color(128, 128, 128));
    g_window->draw(bufferIcon);
    drawText(g_window, "= = Buffer", textX, y, sf::Color::White, fontSize);
    y += lineHeight;
    
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
    
    sf::CircleShape trainIcon(6);
    trainIcon.setPosition(iconX, y);
    trainIcon.setFillColor(sf::Color::Red);
    g_window->draw(trainIcon);
    drawText(g_window, "Circle = Train", textX, y, sf::Color::White, fontSize);
}

// Render UI
void renderUI() {
    if (!g_window) return;
    
    renderStatistics();
    renderLegend();
}

// Handle mouse click
void handleMouseClick(int x, int y, bool leftButton, bool rightButton) {
    sf::Vector2i gridPos = screenToGrid(x, y);
    int row = gridPos.x;
    int col = gridPos.y;
    
    if (!isInBounds(row, col)) return;
    
    if (leftButton) {
        char currentTile = grid[row][col];
        if (currentTile == '=') {
            char replacement = '-';
            if (row > 0 && (grid[row-1][col] == '|' || grid[row-1][col] == '+')) replacement = '|';
            if (row < rows-1 && (grid[row+1][col] == '|' || grid[row+1][col] == '+')) replacement = '|';
            grid[row][col] = replacement;
        } else if (isTrackTile(currentTile) || currentTile == '.' || currentTile == ' ') {
            grid[row][col] = '=';
        }
    } else if (rightButton) {
        char tile = grid[row][col];
        if (isSwitchTile(tile)) {
            int switchIdx = getSwitchIndex(tile);
            if (switchIdx >= 0 && switchIdx < max_switches) {
                switch_state[switchIdx] = 1 - switch_state[switchIdx];
            }
        }
    }
}

// Handle emergency halt trigger
void handleEmergencyHaltTrigger(int x, int y) {
    sf::Vector2i gridPos = screenToGrid(x, y);
    int row = gridPos.x;
    int col = gridPos.y;
    
    if (!isInBounds(row, col)) return;
    
    char tile = grid[row][col];
    if (isSwitchTile(tile)) {
        int switchIdx = getSwitchIndex(tile);
        if (switchIdx >= 0 && switchIdx < max_switches) {
            emergencyHalt = true;
            emergencyHaltTimer = 3;
        }
    }
}

// Main run loop
void runApp() {
    if (!g_window) return;
    
    bool cameraCentered = false;
    sf::Clock simClock;
    float simInterval = 0.5f;
    
    while (g_window->isOpen()) {
        sf::Event event;
        while (g_window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                g_window->close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    writeMetrics();
                    g_window->close();
                } else if (event.key.code == sf::Keyboard::Space) {
                    g_isPaused = !g_isPaused;
                    g_isStepMode = false;
                } else if (event.key.code == sf::Keyboard::Period) {
                    g_isStepMode = true;
                    g_isPaused = false;
                }
            }
            
            if (event.type == sf::Event::MouseWheelScrolled) {
                float zoomFactor = 1.0f + (event.mouseWheelScroll.delta * 0.1f);
                g_camera.zoom(zoomFactor);
                g_window->setView(g_camera);
            }
            
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    g_isLeftDragging = true;
                    g_lastMouseX = event.mouseButton.x;
                    g_lastMouseY = event.mouseButton.y;
                } else if (event.mouseButton.button == sf::Mouse::Right) {
                    handleMouseClick(event.mouseButton.x, event.mouseButton.y, false, true);
                } else if (event.mouseButton.button == sf::Mouse::Middle) {
                    g_isMiddleDragging = true;
                    g_lastMouseX = event.mouseButton.x;
                    g_lastMouseY = event.mouseButton.y;
                    g_dragStartX = event.mouseButton.x;
                    g_dragStartY = event.mouseButton.y;
                }
            }
            
            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (g_isLeftDragging) {
                        int dx = abs(event.mouseButton.x - g_lastMouseX);
                        int dy = abs(event.mouseButton.y - g_lastMouseY);
                        if (dx < 5 && dy < 5) {
                            handleMouseClick(event.mouseButton.x, event.mouseButton.y, true, false);
                        }
                        g_isLeftDragging = false;
                    }
                } else if (event.mouseButton.button == sf::Mouse::Middle) {
                    if (g_isMiddleDragging) {
                        int dx = abs(event.mouseButton.x - g_dragStartX);
                        int dy = abs(event.mouseButton.y - g_dragStartY);
                        if (dx < 5 && dy < 5) {
                            handleEmergencyHaltTrigger(event.mouseButton.x, event.mouseButton.y);
                        }
                    g_isMiddleDragging = false;
                    }
                }
            }
            
            if (event.type == sf::Event::MouseMoved) {
                if (g_isLeftDragging || g_isMiddleDragging) {
                    int dx = event.mouseMove.x - g_lastMouseX;
                    int dy = event.mouseMove.y - g_lastMouseY;
                    
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
        
        if (!g_isPaused || g_isStepMode) {
            if (simClock.getElapsedTime().asSeconds() >= simInterval || g_isStepMode) {
                if (!isSimulationComplete()) {
                    currentTick++;
                    simulateOneTick();
                }
                simClock.restart();
                g_isStepMode = false;
            }
        }
        
        if (!cameraCentered && grid_loaded != 0 && rows > 0 && cols > 0) {
            centerCameraOnGrid();
            cameraCentered = true;
        }
        
        g_window->clear(sf::Color(30, 30, 30));
        
        g_window->setView(g_camera);
        renderGrid();
        renderTrains();
        renderSignals();
        
        g_window->setView(g_window->getDefaultView());
        renderUI();
        
        g_window->display();
    }
}

// Cleanup
void cleanupApp() {
    if (g_window) {
        delete g_window;
        g_window = nullptr;
    }
    for (int i = 0; i < 10; i++) {
        g_textureLoaded[i] = false;
    }
}