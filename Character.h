#ifndef CHARACTER_H
#define CHARACTER_H
#include <raylib.h>
#include <vector>
#include "TileMap.h" // Include the TileMap header for collision detection

// Character animation states
enum CharacterState {
    IDLE,
    WALKING,
    RUNNING,
    JUMPING,
    DYING    // New state for death animation
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
    
    // Lazy jump mechanics
    bool jumpRequested;   // Flag indicating jump button was pressed
    float jumpRequestTime; // Time when jump was requested
    float jumpDelay;      // Delay between request and actual jump (lazy jumper)
    bool jumpBuffered;    // Flag for jump requested while in air
    
    // Running mechanics 
    float lastLeftTapTime;
    float lastRightTapTime;
    float doubleTapTimeThreshold; // Time threshold for double tap
    float runningSpeed; // Speed when running
    bool isRunning; // Flag for running state

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
    Texture2D runTexture;
    Texture2D jumpTexture;
    Texture2D deathTexture;    // Texture for death animation
    
    // Jump animation configuration
    int jumpRiseFrames;
    int jumpPeakFrames;
    int jumpFallFrames;

    bool isDead;          // Flag to indicate if character is dead
    bool isDeathAnimationComplete;  // Flag indicating if death animation finished
    int deathFrameCount;       // Number of frames in death animation

    // Sound effects
    Sound jumpSound;      // Sound played when jumping

public:
    Character();
    void Update(float deltaTime);
    void Render();
    void HandleInput();
    void ApplyPhysics(float deltaTime, float gravity, float friction);
    void CheckCollisions(const std::vector<MapObject>& objects);
    void SetTextures(Texture2D idle, Texture2D walk, Texture2D run, Texture2D jump, Texture2D death);
    void SetJumpSound(Sound sound) { jumpSound = sound; } // Set jump sound
    
    // Getters for camera positioning
    Vector2 GetPosition() const { return {rect.x, rect.y}; }

    Rectangle GetBounds() const { return rect; } // Get character bounds for collision detection

    // Getter for grounded state
    bool IsGrounded() const { return isGrounded; }

    bool IsRunning() const { return isRunning; } // Getter for running state

    // Death management
    bool IsDead() const { return isDead; }
    void SetDead(bool dead) { isDead = dead; }
    void SetDying();  // New method to start death animation
    bool IsDeathAnimationComplete() const { return isDeathAnimationComplete; }
    void Reset();  // Reset character to starting position
};

#endif // CHARACTER_H