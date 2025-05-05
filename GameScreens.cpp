#include "GameScreens.h"
#include <cmath>
#include "Character.h" // Include Character header for Reset()

ScreenManager::ScreenManager(Texture2D characterTexture, Texture2D deathTexture) : 
    currentScreen(TITLE), 
    titleAlpha(0.0f),
    pulseFactor(0.0f),
    fadeIn(true),
    deathAlpha(0.0f),
    deathOverlay(Color{255, 0, 0, 100}),
    deathAnimTime(0.0f),
    deathFrameCount(0),
    characterPos({0, 0}),
    characterVelocity(0.0f),
    characterJumping(false),
    characterTexture(characterTexture),
    deathTexture(deathTexture),
    player(nullptr),
    cameraTarget(nullptr)
{
    Initialize();
}

ScreenManager::~ScreenManager() {
    // No resources to unload
}

void ScreenManager::Initialize() {
    // Initialize properties
    titleAlpha = 0.0f;
    deathAlpha = 0.0f;
    pulseFactor = 0.0f;
    fadeIn = true;
    deathOverlay = Color{255, 0, 0, 100};
    deathAnimTime = 0.0f;
    deathFrameCount = 0;
    
    // Initialize character animation for title screen
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Position character on the platform
    // Platform is at screenHeight/2 + 80, feet should be on top of platform
    characterPos = {(float)screenWidth/2, (float)screenHeight/2 + 80}; // Aligned with platform top
    characterVelocity = 0.0f;
    characterJumping = false;
}

void ScreenManager::Update(float deltaTime) {
    switch (currentScreen) {
        case TITLE:
            // Animate title screen
            if (fadeIn) {
                titleAlpha += deltaTime * 1.5f;
                if (titleAlpha >= 1.0f) {
                    titleAlpha = 1.0f;
                    fadeIn = false;
                }
            }
            
            // Create a pulsing effect for "Press SPACE to start"
            pulseFactor = 0.7f + 0.3f * sinf(GetTime() * 3.0f);
            
            // Animate character
            if (!characterJumping && GetRandomValue(0, 120) == 0) {
                characterJumping = true;
                characterVelocity = -300.0f;
            }
            
            if (characterJumping) {
                characterPos.y += characterVelocity * deltaTime;
                characterVelocity += 800.0f * deltaTime;
                
                // Make character land on the platform, not at an inconsistent height
                if (characterPos.y > GetScreenHeight()/2 + 80) {
                    characterPos.y = GetScreenHeight()/2 + 80; // Match the platform position from Initialize()
                    characterVelocity = 0;
                    characterJumping = false;
                }
            }
            
            // Check for input to transition to gameplay
            if (IsKeyPressed(KEY_SPACE)) {
                SetScreen(GAMEPLAY);
                // Reset player when starting game from title
                if (player) {
                    player->Reset();
                }
                // Reset camera
                if (cameraTarget) {
                    *cameraTarget = { 0, 0 };
                }
            }
            break;
            
        case DEATH:
            // Fade in death screen
            deathAlpha += deltaTime * 2.0f;
            if (deathAlpha > 1.0f) deathAlpha = 1.0f;
            
            // Update death animation timer
            deathAnimTime += deltaTime;
            // 8 frames at 4 FPS (0.25 seconds per frame)
            deathFrameCount = static_cast<int>(deathAnimTime * 4.0f) % 8;
            
            // Check for input to restart game
            if (IsKeyPressed(KEY_R)) {
                // Reset player position before changing screen
                if (player) {
                    player->Reset();
                }
                // Reset camera to starting position
                if (cameraTarget) {
                    *cameraTarget = { 0, 0 };
                }
                SetScreen(GAMEPLAY);
            } else if (IsKeyPressed(KEY_T)) { // Changed from KEY_ESCAPE to KEY_T
                // Reset player position before changing screen 
                if (player) {
                    player->Reset();
                }
                // Reset camera to starting position
                if (cameraTarget) {
                    *cameraTarget = { 0, 0 };
                }
                SetScreen(TITLE);
            }
            break;
            
        default:
            break;
    }
}

void ScreenManager::Draw() {
    switch (currentScreen) {
        case TITLE:
            DrawTitleScreen();
            break;
        case DEATH:
            DrawDeathScreen();
            break;
        default:
            break;
    }
}

void ScreenManager::DrawTitleScreen() {
    ClearBackground(Color{230, 230, 250, 255}); // Light lavender background
    
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Draw clouds in the background
    for (int i = 0; i < 5; i++) {
        float xPos = sin(GetTime() * 0.2f + i * 1.0f) * 50.0f + screenWidth/2 + i * 100 - 200;
        DrawCircle(xPos, screenHeight/5 + i * 15, 40 + i * 5, ColorAlpha(WHITE, titleAlpha * 0.8f));
        DrawCircle(xPos + 30, screenHeight/5 + i * 15 - 10, 30 + i * 3, ColorAlpha(WHITE, titleAlpha * 0.8f));
        DrawCircle(xPos - 30, screenHeight/5 + i * 15 - 5, 35 + i * 4, ColorAlpha(WHITE, titleAlpha * 0.8f));
    }
    
    // Draw title with shadow effect
    const char* titleText = "LAZY JUMPER";
    int titleFontSize = 80;
    DrawText(
        titleText,
        screenWidth/2 - MeasureText(titleText, titleFontSize)/2 + 4,
        screenHeight/4 - titleFontSize/2 + 4,
        titleFontSize,
        ColorAlpha(DARKGRAY, titleAlpha * 0.6f)
    );
    DrawText(
        titleText,
        screenWidth/2 - MeasureText(titleText, titleFontSize)/2,
        screenHeight/4 - titleFontSize/2,
        titleFontSize,
        ColorAlpha(DARKBLUE, titleAlpha)
    );
    
    // Draw character using the sprite instead of shapes
    if (characterTexture.id > 0) {
        // Source rectangle - first 32x32 pixels of the spritesheet
        Rectangle sourceRect = {0, 0, 32, 32};
        
        // Destination rectangle - scale up the sprite for better visibility
        float scale = 2.5f;
        
        // Calculate the y-position to make character stand on platform
        float feetPosition = characterPos.y; // The y-coordinate where feet should touch
        float characterHeight = 32 * scale;  // Total height of character sprite when scaled
        
        Rectangle destRect = {
            characterPos.x - (32 * scale) / 2,  // Center horizontally
            feetPosition - characterHeight,     // Position sprite so feet touch platform
            32 * scale,                         // Scaled width
            characterHeight                     // Scaled height
        };
        
        // Draw the sprite with alpha based on titleAlpha
        DrawTexturePro(
            characterTexture,
            sourceRect,
            destRect,
            {0, 0},                           // Origin (0,0)
            0.0f,                             // Rotation
            ColorAlpha(WHITE, titleAlpha)     // Tint with alpha
        );
    }
    
    // Draw platform
    DrawRectangleRounded(Rectangle{(float)screenWidth/2 - 150, (float)screenHeight/2 + 80, 300, 20}, 0.5f, 8, ColorAlpha(DARKGREEN, titleAlpha));
    
    // Draw "lazy" visual effect - ZZZs coming from character
    for (int i = 1; i <= 3; i++) {
        float offsetX = sin(GetTime() * 2.0f + i * 0.5f) * 5.0f;
        DrawText(
            "z",
            characterPos.x + 25 + i * 15 + offsetX,
            characterPos.y - (32 * 2.5f) - 15 - i * 15, // Position ZZZs relative to character head
            20 + i * 5,
            ColorAlpha(SKYBLUE, titleAlpha * (0.3f + i * 0.2f))
        );
    }
    
    // Draw start instruction with pulsing effect
    const char* startText = "PRESS SPACE TO START";
    DrawText(
        startText,
        screenWidth/2 - MeasureText(startText, 30)/2,
        screenHeight*2/3 + 40,
        30,
        ColorAlpha(DARKGRAY, titleAlpha * pulseFactor)
    );
    
    // Draw version info
    DrawText(
        "v3.0",
        screenWidth - 60,
        screenHeight - 30,
        20,
        ColorAlpha(GRAY, titleAlpha)
    );
}

void ScreenManager::DrawDeathScreen() {
    // Change to a peaceful background similar to title screen
    ClearBackground(Color{230, 230, 250, 255}); // Light lavender background

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Draw clouds in the background similar to title screen
    for (int i = 0; i < 5; i++) {
        float xPos = sin(GetTime() * 0.1f + i * 1.0f) * 30.0f + screenWidth/2 + i * 100 - 200;
        DrawCircle(xPos, screenHeight/5 + i * 15, 40 + i * 5, ColorAlpha(WHITE, deathAlpha * 0.8f));
        DrawCircle(xPos + 30, screenHeight/5 + i * 15 - 10, 30 + i * 3, ColorAlpha(WHITE, deathAlpha * 0.8f));
        DrawCircle(xPos - 30, screenHeight/5 + i * 15 - 5, 35 + i * 4, ColorAlpha(WHITE, deathAlpha * 0.8f));
    }

    // Draw death animation
    if (deathTexture.id > 0) {
        // Source rectangle - get current frame from spritesheet (8 frames, each 32x32)
        Rectangle sourceRect = {
            static_cast<float>(deathFrameCount * 32), // x position based on current frame
            0,                                        // y position (single row spritesheet)
            32,                                       // width of a single frame
            32                                        // height of a single frame
        };
        
        // Destination rectangle - scale up the sprite for better visibility
        float scale = 5.0f; // Larger scale for more visibility
        
        Rectangle destRect = {
            static_cast<float>(screenWidth/2 - 32*scale/2),  // Center horizontally
            static_cast<float>(screenHeight/3 - 32*scale/2), // Position 
            32 * scale,                                      // Scaled width
            32 * scale                                       // Scaled height
        };
        
        // Draw the sprite with alpha based on deathAlpha
        DrawTexturePro(
            deathTexture,
            sourceRect,
            destRect,
            {0, 0},                          // Origin (0,0)
            0.0f,                            // Rotation
            ColorAlpha(WHITE, deathAlpha)    // Tint with alpha
        );
    }
    
    // Draw "ZZZs" coming from the character but larger and more prominent
    for (int i = 1; i <= 5; i++) {
        float offsetX = sin(GetTime() * 1.5f + i * 0.3f) * 3.0f;
        float offsetY = cos(GetTime() * 0.8f + i * 0.4f) * 2.0f;
        DrawText(
            "Z",
            screenWidth/2 + 30 + i * 25 + offsetX,
            screenHeight/3 - 20 - i * 20 + offsetY, 
            30 + i * 8,
            ColorAlpha(SKYBLUE, deathAlpha * (0.4f + i * 0.12f))
        );
    }

    const char* deathText = "GAME OVER";
    int deathFontSize = 60;
    DrawText(
        deathText,
        screenWidth/2 - MeasureText(deathText, deathFontSize)/2,
        screenHeight/2 + 20,
        deathFontSize,
        ColorAlpha(DARKBLUE, deathAlpha) // Changed from RED to DARKBLUE like title
    );

    // Add a platform like in title screen
    DrawRectangleRounded(Rectangle{(float)screenWidth/2 - 200, (float)screenHeight/2 + 100, 400, 20}, 0.5f, 8, ColorAlpha(DARKGREEN, deathAlpha));

    // Draw restart instructions with a more peaceful color
    const char* restartText = "PRESS R TO WAKE UP";  // Changed text to fit the theme
    DrawText(
        restartText,
        screenWidth/2 - MeasureText(restartText, 30)/2,
        screenHeight*2/3,
        30,
        ColorAlpha(DARKGRAY, deathAlpha * (0.7f + 0.3f * sin(GetTime() * 3.0f))) // Added pulsing effect like title
    );
    
    // Draw menu instructions
    const char* menuText = "PRESS T FOR TITLE SCREEN";
    DrawText(
        menuText,
        screenWidth/2 - MeasureText(menuText, 20)/2,
        screenHeight*2/3 + 40,
        20,
        ColorAlpha(GRAY, deathAlpha)
    );
}

void ScreenManager::SetScreen(GameScreen screen) {
    currentScreen = screen;
    
    // Reset animations for the specific screen
    switch (screen) {
        case TITLE:
            titleAlpha = 0.0f;
            fadeIn = true;
            break;
        case DEATH:
            deathAlpha = 0.0f;
            deathAnimTime = 0.0f;  // Reset death animation timer
            deathFrameCount = 0;    // Reset frame counter
            break;
        default:
            break;
    }
}
