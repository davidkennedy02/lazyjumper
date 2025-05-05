#pragma once
#include <raylib.h>

// Forward declaration of Character class
class Character;

// Enum to track which screen we're currently on
enum GameScreen {
    TITLE,
    GAMEPLAY,
    DEATH
};

class ScreenManager {
private:
    GameScreen currentScreen;
    
    // Title screen properties
    float titleAlpha;
    float pulseFactor;
    bool fadeIn;
    
    // Death screen properties
    float deathAlpha;
    Color deathOverlay;
    float deathAnimTime;  // Animation timer for death sequence
    int deathFrameCount;  // To track current animation frame
    
    // Common properties
    Vector2 characterPos;
    float characterVelocity;
    bool characterJumping;
    
    // Character sprite
    Texture2D characterTexture;
    Texture2D deathTexture;  // Added death texture
    
    // Character reference for reset
    Character* player;
    Vector2* cameraTarget;
    
public:
    ScreenManager(Texture2D characterTexture = {0}, Texture2D deathTexture = {0});
    ~ScreenManager();
    
    // Main methods
    void Initialize();
    void Update(float deltaTime);
    void Draw();
    
    // Screen specific rendering
    void DrawTitleScreen();
    void DrawDeathScreen();
    
    // State changes
    void SetScreen(GameScreen screen);
    GameScreen GetCurrentScreen() const { return currentScreen; }
    
    // Set player reference
    void SetPlayerReference(Character* playerRef, Vector2* camTarget) { 
        player = playerRef; 
        cameraTarget = camTarget;
    }
};
