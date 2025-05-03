#ifndef CHARACTER_H
#define CHARACTER_H
#include <raylib.h>
#include <vector>
#include "TileMap.h" // Include the TileMap header for collision detection

// Character animation states
enum CharacterState {
    IDLE,
    WALKING,
    JUMPING
};

class Character {
private:
    Rectangle rect;       // Position and size
    Vector2 velocity;     // Movement velocity
    bool isGrounded;      // Flag to indicate if touching an object
    bool droppingThrough; // Flag for dropping through platforms
    Color color;          // Rectangle color
    float moveSpeed;      // Horizontal movement speed
    float jumpForce;      // Force applied when jumping
    bool facingRight;     // Direction the character is facing
    
    // Animation properties
    CharacterState state;
    int frameCount;       // Total frames in current animation
    int currentFrame;     // Current frame of animation
    float frameWidth;     // Width of a single frame
    float frameHeight;    // Height of a single frame
    float frameTime;      // Time for each frame
    float frameTimer;     // Timer for current frame
    
    // Textures for different states
    Texture2D idleTexture;
    Texture2D walkTexture;
    Texture2D jumpTexture;
    
    // Jump animation configuration
    int jumpRiseFrames;
    int jumpPeakFrames;
    int jumpFallFrames;

public:
    Character();
    void Update(float deltaTime);
    void Render();
    void HandleInput();
    void ApplyPhysics(float deltaTime, float gravity, float friction);
    void CheckCollisions(const std::vector<MapObject>& objects);
    void SetTextures(Texture2D idle, Texture2D walk, Texture2D jump);
    
    // Getters for camera positioning
    Vector2 GetPosition() const { return {rect.x, rect.y}; }

    // Getter for grounded state
    bool IsGrounded() const { return isGrounded; }
};

#endif // CHARACTER_H