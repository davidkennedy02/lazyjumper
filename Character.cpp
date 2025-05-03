#include "Character.h"
#include <math.h>
#include <vector>
#include "TileMap.h"

// Constructor to initialize the character
Character::Character() {
    // Initialize default values
    rect = { 0, 88, 32.0f, 32.0f };
    velocity = { 0.0f, 0.0f };
    isGrounded = true;
    bool droppingThrough = false;
    color = BLUE;
    moveSpeed = 200.0f;      // Pixels per second
    jumpForce = 550.0f;      // Jump velocity
    facingRight = true;      // Start facing right
    state = IDLE;            // Start in idle state
    frameCount = 4;          // Idle animation has 4 frames
    currentFrame = 0;        // Start with first frame
    frameWidth = 32.0f;      // Width of a single frame
    frameHeight = 32.0f;     // Height of a single frame
    frameTime = 0.1f;        // 10 FPS animation
    frameTimer = 0.0f;       // Initialize timer

    jumpRiseFrames = 3;
    jumpPeakFrames = 2;
    jumpFallFrames = 3;
}

void Character::SetTextures(Texture2D idle, Texture2D walk, Texture2D jump) {
    idleTexture = idle;
    walkTexture = walk;
    jumpTexture = jump;
}

void Character::Update(float deltaTime) {
    // Update animation timer
    frameTimer += deltaTime;
    if (frameTimer >= frameTime) {
        frameTimer = 0.0f;
        currentFrame = (currentFrame + 1) % frameCount;
    }
    
    // Determine character state
    CharacterState previousState = state;
    
    if (!isGrounded) {
        // Player is jumping or falling
        state = JUMPING;
        
        // Handle jump animation phases based on vertical velocity
        if (velocity.y < -5.0f) {
            // Rising phase - frames 0-2
            int frameInPhase = (int)(frameTimer / frameTime) % jumpRiseFrames;
            currentFrame = frameInPhase;
        } 
        else if (velocity.y >= -5.0f && velocity.y <= 5.0f) {
            // Peak phase - frames 3-4
            int frameInPhase = (int)(frameTimer / frameTime) % jumpPeakFrames;
            currentFrame = jumpRiseFrames + frameInPhase;
        }
        else {
            // Falling phase - frames 5-7
            int frameInPhase = (int)(frameTimer / frameTime) % jumpFallFrames;
            currentFrame = jumpRiseFrames + jumpPeakFrames + frameInPhase;
        }
    } 
    else if (fabsf(velocity.x) > 5.0f) {
        // Player is walking (if moving faster than a threshold)
        state = WALKING;
        frameCount = 6; // Walk animation has 6 frames
    } 
    else {
        // Player is idle
        state = IDLE;
        frameCount = 4; // Idle animation has 4 frames
    }
    
    // Reset animation if state changed
    if (previousState != state) {
        currentFrame = 0;
        frameTimer = 0.0f;
    }
}

void Character::HandleInput() {
    // Left and right movement
    if (IsKeyDown(KEY_RIGHT)) {
        velocity.x = moveSpeed;
        facingRight = true;
    }
    if (IsKeyDown(KEY_LEFT)) {
        velocity.x = -moveSpeed;
        facingRight = false;
    }
    
    // Jumping (only when grounded)
    if (IsKeyPressed(KEY_UP) && isGrounded) {
        velocity.y = -jumpForce;
        isGrounded = false;
    }

    droppingThrough = IsKeyDown(KEY_DOWN) && isGrounded;
}

void Character::ApplyPhysics(float deltaTime, float gravity, float friction) {
    // Apply friction to horizontal movement
    velocity.x *= friction;
    
    // Apply gravity when not grounded
    if (!isGrounded) {
        float currentGravity = gravity;
        if (velocity.y > 0) {
            currentGravity = gravity * 1.2f;
        }
        velocity.y += currentGravity * deltaTime;
    }
    
    // Update position
    rect.x += velocity.x * deltaTime;
    rect.y += velocity.y * deltaTime;
}

void Character::CheckCollisions(const std::vector<MapObject>& objects) {
    isGrounded = false; // Reset grounded state

    const float groundCheckDistance = 2.0f;
    
    for (const auto& object : objects) {

        if (object.name != "floor") {

            // Skip one-way platforms if the player is dropping through
            if (droppingThrough) {
                continue;
            }

            // Create a ground sensor for one-way platforms
            Rectangle groundSensor = {
                rect.x + 2.0f,
                rect.y + rect.height,
                rect.width - 4.0f,
                groundCheckDistance
            };
            
            // Only check collision if:
            // 1. The character is falling (velocity.y > 0) OR
            // 2. The character is already standing on the platform (feet just above platform)
            if ((velocity.y > 0 || rect.y + rect.height <= object.rect.y + 2.0f) && 
                CheckCollisionRecs(groundSensor, object.rect)) {
                
                // Make sure the character was above the platform in the previous frame
                // (to prevent collision when jumping up through the platform)
                if (rect.y + rect.height - velocity.y * GetFrameTime() <= object.rect.y) {
                    rect.y = object.rect.y - rect.height;
                    velocity.y = 0;
                    isGrounded = true;
                }
            }
            // Skip other collision checks for one-way platforms
            continue;
        }

        if (CheckCollisionRecs(rect, object.rect)) {
            // Bottom collision (landing)
            if (velocity.y > 0 && // Only when falling downward
                rect.y + rect.height > object.rect.y &&
                rect.y < object.rect.y &&
                // Check horizontal overlap to ensure player is above the platform
                rect.x + rect.width > object.rect.x + 2.0f &&
                rect.x < object.rect.x + object.rect.width - 2.0f) {
                
                // Position player slightly above the ground to prevent sinking
                rect.y = object.rect.y - rect.height;
                velocity.y = 0;
                isGrounded = true;
            }
            
            // // Basic horizontal collision
            // if (velocity.x > 0 && 
            //     rect.x + rect.width > object.rect.x &&
            //     rect.x < object.rect.x) {
            //     rect.x = object.rect.x - rect.width;
            //     velocity.x = 0;
            // }
            
            // if (velocity.x < 0 && 
            //     rect.x < object.rect.x + object.rect.width &&
            //     rect.x + rect.width > object.rect.x + object.rect.width) {
            //     rect.x = object.rect.x + object.rect.width;
            //     velocity.x = 0;
            // }
        }
        else {
            // Ground check - detect if we're very close to the ground
            // Create a slightly wider rectangle just below the player's feet
            Rectangle groundSensor = {
                rect.x + 2.0f,                      // Slightly narrower than player
                rect.y + rect.height,               // At player's feet
                rect.width - 4.0f,                 // Slightly narrower than player
                groundCheckDistance                // Small distance below feet
            };
            
            if (CheckCollisionRecs(groundSensor, object.rect)) {
                rect.y = object.rect.y - rect.height;
                velocity.y = 0;
                isGrounded = true;
            }
        }
    }

    if (droppingThrough && isGrounded) {
        // Drop through the platform
        velocity.y = 50.0f; // Small downward velocity to drop through
        isGrounded = false; // Reset grounded state
    }
}

void Character::Render() {
    // Draw the player character using sprites
    Texture2D currentTexture;
    switch (state) {
        case IDLE: currentTexture = idleTexture; break;
        case WALKING: currentTexture = walkTexture; break;
        case JUMPING: currentTexture = jumpTexture; break;
    }
    
    // Rectangle for source (which part of the texture to draw)
    Rectangle source = {
        currentFrame * frameWidth,
        0,
        facingRight ? frameWidth : -frameWidth,
        frameHeight
    };
    
    // Rectangle for destination (where to draw it in the world)
    Rectangle dest = {
        rect.x,
        rect.y,
        rect.width,
        rect.height
    };
    
    // Draw the sprite
    Vector2 origin = { 0, 0 };
    DrawTexturePro(currentTexture, source, dest, origin, 0.0f, WHITE);
}