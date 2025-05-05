#include <raylib.h>
#include <iostream>
#include "tileson.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include "TileMap.h"  // Include the new TileMap header
#include "Character.h" // Include the Character class header
#include "GameScreens.h" // Include the new GameScreens header
#include "Enemy.h" // Include the Enemy class header

int main()
{
    // Initialize window
    const int screenWidth = 1440;
    const int screenHeight = 960; 
    InitWindow(screenWidth, screenHeight, "LazyJumper v3");
    SetTargetFPS(60);
    
    // Initialize audio
    InitAudioDevice();

    // Load soundtrack
    Music soundtrack = LoadMusicStream("sounds/soundtrack.mp3");
    SetMusicVolume(soundtrack, 0.5f); // Set volume to 50%
    
    // Load lullaby soundtrack for title and death screens
    Music lullabyTrack = LoadMusicStream("sounds/lullaby.mp3");
    SetMusicVolume(lullabyTrack, 0.5f); // Set volume to 50%
    
    // Load death sound effect
    Sound deathSound = LoadSound("sounds/death.mp3");
    
    // Load jump sound effect
    Sound jumpSound = LoadSound("sounds/jump.mp3");
    
    // Load character sprite sheets
    Texture2D idleTexture = LoadTexture("main_character/idle.png");
    Texture2D walkTexture = LoadTexture("main_character/walk.png");
    Texture2D runTexture = LoadTexture("main_character/run.png");
    Texture2D jumpTexture = LoadTexture("main_character/jump.png");
    Texture2D deathTexture = LoadTexture("main_character/death.png");

    // Load enemy sprite sheets
    Texture2D enemyIdleTexture = LoadTexture("enemies/bird/Idle.png");
    Texture2D enemyWalkTexture = LoadTexture("enemies/bird/Walk.png");
    
    // Initialize the screen manager with the idle texture
    ScreenManager screenManager(idleTexture, deathTexture);

    // Camera setup for following player (later)
    const float cameraOffsetX = screenWidth / 2.0f;
    const float cameraOffsetY = screenHeight / 2.0f;
    Camera2D camera = { 0 };
    camera.target = { 0, 0 };
    camera.offset = { cameraOffsetX, cameraOffsetY };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    
    // Camera control parameters
    float cameraSpeed = 500.0f;      // Speed of camera movement

    // Load the map with TileMap class
    TileMap tileMap;
    if (!tileMap.Load("tiled_project/level2.json")) {
        CloseWindow();
        return -1;
    }

    // Initialize the character
    Character* player = new Character();
    player->SetTextures(idleTexture, walkTexture, runTexture, jumpTexture, deathTexture);
    player->SetJumpSound(jumpSound); // Set the jump sound
    
    // Set player reference in screen manager
    screenManager.SetPlayerReference(player, &camera.target);
    
    // Create a vector to store enemy instances
    std::vector<Enemy*> enemies;
    
    // Find suitable platforms for enemies
    const std::vector<MapObject>& mapObjects = tileMap.GetMapObjects();
    
    // For the first enemy
    Rectangle platformRect1 = {0, 0, 0, 0};
    bool foundPlatform1 = false;
    
    // For the second enemy
    Rectangle platformRect2 = {0, 0, 0, 0};
    bool foundPlatform2 = false;
    
    // Look for the first suitable platform (ground object with sufficient width)
    for (const auto& obj : mapObjects) {
        if (obj.rect.width >= 200) {
            // Use this platform
            platformRect1 = obj.rect;
            foundPlatform1 = true;
            
            // Don't place the enemy too close to the start
            if (obj.rect.x > 300) {
                break;
            }
        }
    }
    
    // Look for a second suitable platform (different from the first one)
    for (const auto& obj : mapObjects) {
        if (obj.rect.width >= 200 && 
            (obj.rect.x != platformRect1.x || obj.rect.y != platformRect1.y)) {
            // Use a different platform than the first one
            platformRect2 = obj.rect;
            foundPlatform2 = true;
            
            // Preferably place the second enemy further into the level
            if (obj.rect.x > platformRect1.x + 300) {
                break;
            }
        }
    }
    
    // Create enemies if we found suitable platforms
    if (foundPlatform1) {
        Enemy* enemy1 = new Enemy(enemyIdleTexture, enemyWalkTexture, platformRect1);
        enemies.push_back(enemy1);
    }
    
    if (foundPlatform2) {
        Enemy* enemy2 = new Enemy(enemyIdleTexture, enemyWalkTexture, platformRect2);
        enemies.push_back(enemy2);
    }
    
    // Physics constants
    const float gravity = 900.0f; // Pixels per second squared
    const float friction = 0.8f; // Horizontal movement friction
    
    // Game loop
    while (!WindowShouldClose())
    {
        // Update camera controls
        float deltaTime = GetFrameTime();

        screenManager.Update(deltaTime);
        
        // Update music streams
        UpdateMusicStream(soundtrack);
        UpdateMusicStream(lullabyTrack);

        // Begin drawing - moved outside the if-else
        BeginDrawing();
        ClearBackground(BLACK);

        if (screenManager.GetCurrentScreen() == GAMEPLAY){
            // Play gameplay soundtrack and stop lullaby
            if (!IsMusicStreamPlaying(soundtrack) && player->IsDead() == false) {
                StopMusicStream(lullabyTrack);
                PlayMusicStream(soundtrack);
            }
            
            player->Update(deltaTime);
            
            player->HandleInput();

            player->ApplyPhysics(deltaTime, gravity, friction);
            
            player->CheckCollisions(tileMap.GetMapObjects());
            
            // Add a centralized check for death animation completion
            if (player->IsDead() && player->IsDeathAnimationComplete()) {
                screenManager.SetScreen(DEATH);
            }
            
            // Update enemies
            for (auto& enemy : enemies) {
                enemy->Update(deltaTime, tileMap.GetMapObjects());
                
                // Check if player collides with enemy
                if (enemy->CheckCollisionWithPlayer(player->GetBounds())) {
                    if (!player->IsDead()) {
                        player->SetDying(); // Start death animation instead of immediate death
                        StopMusicStream(soundtrack); // Stop music when player dies
                        PlaySound(deathSound); // Play death sound effect
                    }
                }
            }

            // Check if player has fallen off the screen
            if (player->GetPosition().y > screenHeight && !player->IsDead()) {
                player->SetDying();
                StopMusicStream(soundtrack); // Stop music when player dies
                PlaySound(deathSound); // Play death sound effect
            }
            
            // Make camera follow player horizontally only
            camera.target.x = std::max(player->GetPosition().x, 0.0f); // Prevent camera from going left
            // Camera y position stays independent of player jumps
            
            BeginMode2D(camera);
            
            // Render the map
            tileMap.Render(camera);
            
            // Render the player character
            player->Render();
            
            // Render enemies
            for (auto& enemy : enemies) {
                enemy->Render();
            }
            
            EndMode2D();
            
            // Draw UI or debug info here
            DrawFPS(10, 10);
            DrawText(TextFormat("Camera: %.2f, %.2f", camera.target.x, camera.target.y), 10, 30, 20, BLACK);
            DrawText("Controls: LEFT/RIGHT - Move, UP - Jump", 10, screenHeight - 30, 20, DARKGRAY);
            DrawText(TextFormat("Player: %.2f, %.2f %s", player->GetPosition().x, player->GetPosition().y, 
                    player->IsGrounded() ? "(Grounded)" : "(In air)"), 10, 50, 20, BLACK);
        }
        else if (screenManager.GetCurrentScreen() == DEATH) {
            // Stop gameplay music and play lullaby in death screen
            if (IsMusicStreamPlaying(soundtrack)) {
                StopMusicStream(soundtrack);
            }
            
            if (!IsMusicStreamPlaying(lullabyTrack)) {
                PlayMusicStream(lullabyTrack);
            }
            
            // Handle death screen
            screenManager.Draw();
            
            // No need for key handling here anymore, it's managed in screenManager.Update()
        }
        else {
            // Title screen or any other screen - stop gameplay music and play lullaby
            if (IsMusicStreamPlaying(soundtrack)) {
                StopMusicStream(soundtrack);
            }
            
            if (!IsMusicStreamPlaying(lullabyTrack)) {
                PlayMusicStream(lullabyTrack);
            }
            
            screenManager.Draw();
        }
        
        // End drawing - moved outside the if-else
        EndDrawing();
    }
    
    // Unload textures and resources
    tileMap.Unload();
    
    // Unload character textures
    UnloadTexture(idleTexture);
    UnloadTexture(walkTexture);
    UnloadTexture(runTexture);
    UnloadTexture(jumpTexture);
    UnloadTexture(deathTexture);
    
    // Unload enemy textures
    UnloadTexture(enemyIdleTexture);
    UnloadTexture(enemyWalkTexture);
    
    // Unload audio
    UnloadMusicStream(soundtrack);
    UnloadMusicStream(lullabyTrack); // Unload lullaby soundtrack
    UnloadSound(deathSound); // Unload death sound
    UnloadSound(jumpSound); // Unload jump sound
    CloseAudioDevice();
    
    // Delete enemies
    for (auto& enemy : enemies) {
        delete enemy;
    }
    enemies.clear();
    
    // Delete player
    delete player;
    
    CloseWindow();
    return 0;
}

