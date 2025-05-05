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
    runningSpeed = 350.0f; // Speed when running
    jumpForce = 550.0f;      // Jump velocity
    facingRight = true;      // Start facing right
    
    // Initialize lazy jump mechanics
    jumpRequested = false;
    jumpRequestTime = 0.0f;
    jumpDelay = 0.5f;        // 0.5 second delay for lazy jumper
    jumpBuffered = false;    // Initialize buffered jump flag
    
    state = IDLE;            // Start in idle state
    frameCount = 4;          // Idle animation has 4 frames
    currentFrame = 0;        // Start with first frame
    frameWidth = 32.0f;      // Width of a single frame
    frameHeight = 32.0f;     // Height of a single frame
    frameTime = 0.1f;        // 10 FPS animation
    frameTimer = 0.0f;       // Initialize timer

    // Initialise running mechanics
    lastLeftTapTime = 0.0f;
    lastRightTapTime = 0.0f;
    doubleTapTimeThreshold = 0.2f; // Time threshold for double tap
    isRunning = false; // Start not running

    jumpRiseFrames = 3;
    jumpPeakFrames = 2;
    jumpFallFrames = 3;

    // Death animation properties
    isDeathAnimationComplete = false;
    deathFrameCount = 8;  // Death animation has 8 frames
    isDead = false;       // Start alive
}

void Character::SetTextures(Texture2D idle, Texture2D walk, Texture2D run, Texture2D jump, Texture2D death) {
    idleTexture = idle;
    walkTexture = walk;
    runTexture = run;
    jumpTexture = jump;
    deathTexture = death;
}

void Character::Update(float deltaTime) {
    // Update animation timer
    frameTimer += deltaTime;
    
    // Process lazy jump if requested
    if (jumpRequested && isGrounded) {
        float currentTime = GetTime();
        if (currentTime - jumpRequestTime >= jumpDelay) {
            // Time to execute the jump after delay
            velocity.y = -jumpForce;
            isGrounded = false;
            jumpRequested = false; // Reset jump request
            jumpBuffered = false;  // Reset buffered jump flag
            
            // Play jump sound when jump is executed
            PlaySound(jumpSound);
        }
    }
    
    // Handle death animation separately
    if (state == DYING) {
        if (frameTimer >= frameTime) {
            frameTimer = 0.0f;
            currentFrame++;
            
            // Check if death animation is complete
            if (currentFrame >= deathFrameCount) {
                isDeathAnimationComplete = true;
                currentFrame = deathFrameCount - 1; // Stay on last frame
            }
        }
        return; // Skip other state updates if dying
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
        if (isRunning) {
            state = RUNNING;
        } else {
            state = WALKING;
        }
        frameCount = 6; // Walk and run animations have 6 frames
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

void Character::SetDying() {
    if (!isDead) {
        state = DYING;
        isDead = true;
        isDeathAnimationComplete = false;
        currentFrame = 0;
        frameTimer = 0.0f;
        velocity = { 0, 0 }; // Stop movement
    }
}

void Character::HandleInput() {
    // Skip input handling if character is dead
    if (isDead) return;

    float currentTime = GetTime();

    // Check for right key double-tap
    if (IsKeyPressed(KEY_RIGHT)) {
        if (currentTime - lastRightTapTime < doubleTapTimeThreshold) {
            // Double tap detected
            isRunning = true;
        }
        lastRightTapTime = currentTime;
    }
    
    // Check for left key double-tap
    if (IsKeyPressed(KEY_LEFT)) {
        if (currentTime - lastLeftTapTime < doubleTapTimeThreshold) {
            // Double tap detected
            isRunning = true;
        }
        lastLeftTapTime = currentTime;
    }

    // Handle movement based on current input and running state
    if (IsKeyDown(KEY_RIGHT)) {
        velocity.x = isRunning ? runningSpeed : moveSpeed;
        facingRight = true;
    }
    else if (IsKeyDown(KEY_LEFT)) {
        velocity.x = isRunning ? -runningSpeed : -moveSpeed;
        facingRight = false;
    }
    else {
        // Reset running state if no movement keys are pressed
        isRunning = false;
    }
    
    // Request jumping (allow request even when in air)
    if (IsKeyPressed(KEY_UP) && !jumpRequested) {
        jumpRequested = true;
        jumpRequestTime = currentTime; // Record when jump was requested
        if (!isGrounded) {
            jumpBuffered = true; // Mark this as a buffered jump (requested while in air)
        }
    }

    droppingThrough = IsKeyDown(KEY_DOWN) && isGrounded;
}

void Character::ApplyPhysics(float deltaTime, float gravity, float friction) {
    // Skip physics if dying
    if (state == DYING) return;

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

    const float leftBound = -GetScreenWidth() / 2.0f; // Left boundary

    if (rect.x < leftBound) {
        rect.x = leftBound; // Prevent going off-screen to the left
        velocity.x = 0; // Stop horizontal movement
    }
}

void Character::CheckCollisions(const std::vector<MapObject>& objects) {
    bool wasGrounded = isGrounded; // Store previous grounded state
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

    // Check if we just landed and have a buffered jump
    if (!wasGrounded && isGrounded && jumpBuffered) {
        // We've landed with a buffered jump request
        // Keep the jumpRequested flag true, but reset the timer to now
        // This ensures we honor the jump but still have the lazy delay
        jumpRequestTime = GetTime();
        jumpBuffered = false; // No longer a buffered jump
    }

    // Don't cancel jump requests when airborne anymore
    // The old code was:
    // if (!isGrounded && jumpRequested) {
    //     jumpRequested = false;
    // }

    if (droppingThrough && isGrounded) {
        // Drop through the platform
        velocity.y = 100.0f; // Small downward velocity to drop through
        isGrounded = false; // Reset grounded state
    }
}

void Character::Render() {
    // Draw the player character using sprites
    Texture2D currentTexture;
    switch (state) {
        case IDLE: currentTexture = idleTexture; break;
        case WALKING: currentTexture = walkTexture; break;
        case RUNNING: currentTexture = runTexture; break;
        case JUMPING: currentTexture = jumpTexture; break;
        case DYING: currentTexture = deathTexture; break;
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

// Reset character to starting position
void Character::Reset() {
    rect = { 0, 88, 32.0f, 32.0f };
    velocity = { 0.0f, 0.0f };
    isGrounded = true;
    isDead = false;
    isDeathAnimationComplete = false;
    state = IDLE;
    facingRight = true;
    isRunning = false;
    currentFrame = 0;
    frameTimer = 0.0f;
    jumpRequested = false;
    jumpBuffered = false;
}