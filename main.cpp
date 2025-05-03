#include <raylib.h>
#include <iostream>
#include "tileson.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include "TileMap.h"  // Include the new TileMap header
#include "Character.h" // Include the Character class header

int main()
{
    // Initialize window
    const int screenWidth = 1440;
    const int screenHeight = 960; 
    InitWindow(screenWidth, screenHeight, "LazyJumper v3");
    SetTargetFPS(60);

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
    if (!tileMap.Load("tiled_project/level1.json")) {
        CloseWindow();
        return -1;
    }

    // Load character sprite sheets
    Texture2D idleTexture = LoadTexture("main_character/idle.png");
    Texture2D walkTexture = LoadTexture("main_character/walk.png");
    Texture2D jumpTexture = LoadTexture("main_character/jump.png");
    
    // Initialize the character
    Character* player = new Character();
    player->SetTextures(idleTexture, walkTexture, jumpTexture);
    
    // Physics constants
    const float gravity = 900.0f; // Pixels per second squared
    const float friction = 0.8f; // Horizontal movement friction
    
    // Game loop
    while (!WindowShouldClose())
    {
        // Update camera controls
        float deltaTime = GetFrameTime();
        
        player->Update(deltaTime);
        
        player->HandleInput();

        player->ApplyPhysics(deltaTime, gravity, friction);
        
        player->CheckCollisions(tileMap.GetMapObjects());
        
        // Make camera follow player horizontally only
        camera.target.x = player->GetPosition().x;
        // Camera y position stays independent of player jumps
        
        // Begin drawing
        BeginDrawing();
        ClearBackground(BLACK);
        
        BeginMode2D(camera);
        
        // Render the map
        tileMap.Render(camera);
        
        // Render the player character
        player->Render();
        
        EndMode2D();
        
        // Draw UI or debug info here
        DrawFPS(10, 10);
        DrawText(TextFormat("Camera: %.2f, %.2f", camera.target.x, camera.target.y), 10, 30, 20, BLACK);
        DrawText("Controls: LEFT/RIGHT - Move, UP - Jump", 10, screenHeight - 30, 20, DARKGRAY);
        DrawText(TextFormat("Player: %.2f, %.2f %s", player->GetPosition().x, player->GetPosition().y, 
                player->IsGrounded() ? "(Grounded)" : "(In air)"), 10, 50, 20, BLACK);
        
        EndDrawing();
    }
    
    // Unload textures and resources
    tileMap.Unload();
    
    // Unload character textures
    UnloadTexture(idleTexture);
    UnloadTexture(walkTexture);
    UnloadTexture(jumpTexture);
    
    CloseWindow();
    return 0;
}

