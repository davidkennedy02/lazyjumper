#include "Enemy.h"
#include "TileMap.h" // For MapObject definition

Enemy::Enemy(Texture2D idleTex, Texture2D patrolTex, Rectangle platformRect, float margin) {
    idleTexture = idleTex;
    patrolTexture = patrolTex;
    
    // Store platform information
    platform = platformRect;
    patrolMargin = margin;
    
    // Calculate patrol boundaries with margins
    patrolStart.x = platform.x + patrolMargin;
    patrolStart.y = platform.y;
    patrolEnd.x = platform.x + platform.width - patrolMargin;
    patrolEnd.y = platform.y;
    
    // Set default size based on texture (adjust as needed)
    size = { 32, 32 };
    
    // Position the enemy on top of the platform
    position.x = patrolStart.x;
    position.y = platform.y - size.y; // Place enemy on top of platform
    
    bounds = { position.x, position.y, size.x, size.y };
    
    // Initialize animation variables
    currentFrame = 0;
    frameTimer = 0.0f;
    framesPerRowIdle = 4;
    framesPerRowPatrol = 6;
    totalFramesIdle = 4;     // Adjust based on your texture
    totalFramesPatrol = 6;
    frameSpeed = 0.1f;   // Seconds per frame
    
    // Initialize state and physics
    state = ENEMY_PATROLLING;
    facingRight = true;
    stateTimer = 0.0f;
    idleDuration = 2.0f;
    patrolSpeed = 100.0f;
    velocity = { patrolSpeed, 0 };
    isGrounded = true; // Enemy starts on platform
    
    // Add a transition cooldown to prevent rapid state changes
    stateTransitionCooldown = 0.0f;
}

void Enemy::Update(float deltaTime, const std::vector<MapObject>& mapObjects) {
    stateTimer += deltaTime;
    
    // Decrease transition cooldown if active
    if (stateTransitionCooldown > 0.0f) {
        stateTransitionCooldown -= deltaTime;
    }
    
    // State machine for enemy behavior
    switch (state) {
        case ENEMY_IDLE:
            velocity.x = 0.0f;
            
            // Return to patrolling after idle duration
            if (stateTimer >= idleDuration) {
                state = ENEMY_PATROLLING;
                stateTimer = 0.0f;
                
                // Calculate the center position of the enemy
                float centerX = position.x + size.x/2.0f;
                float platformCenterX = platform.x + platform.width/2.0f;
                
                // Determine direction based on position relative to platform center
                // This ensures enemy moves away from the boundary it was at
                facingRight = (centerX < platformCenterX);
                
                // Set velocity based on facing direction
                velocity.x = facingRight ? patrolSpeed : -patrolSpeed;
                
                // Move enemy slightly in new direction to avoid edge detection
                float safetyOffset = 8.0f;
                if (facingRight) {
                    // Ensure we don't move too close to right boundary
                    float maxRightPos = patrolEnd.x - size.x - safetyOffset;
                    position.x = fmin(position.x + safetyOffset, maxRightPos);
                } else {
                    // Ensure we don't move too close to left boundary
                    float minLeftPos = patrolStart.x + safetyOffset;
                    position.x = fmax(position.x - safetyOffset, minLeftPos);
                }
                
                // Reset transition cooldown
                stateTransitionCooldown = 0.5f;
            }
            break;
            
        case ENEMY_PATROLLING:
            // Apply velocity based on direction
            velocity.x = facingRight ? patrolSpeed : -patrolSpeed;
            
            // Check platform boundaries
            if (facingRight && position.x + size.x >= patrolEnd.x) {
                state = ENEMY_IDLE;
                stateTimer = 0.0f;
                velocity.x = 0.0f;
                facingRight = false; // Will face left next time
                // Move away from right boundary
                position.x = patrolEnd.x - size.x - 10.0f;
                stateTransitionCooldown = 0.5f;
            } 
            else if (!facingRight && position.x <= patrolStart.x) {
                state = ENEMY_IDLE;
                stateTimer = 0.0f;
                velocity.x = 0.0f;
                facingRight = true; // Will face right next time
                // Move away from left boundary
                position.x = patrolStart.x + 10.0f;
                stateTransitionCooldown = 0.5f;
            }
            
            // Only check for edge if cooldown has expired and enemy is moving
            if (stateTransitionCooldown <= 0.0f && velocity.x != 0) {
                if (DetectEdge(mapObjects)) {
                    state = ENEMY_IDLE;
                    stateTimer = 0.0f;
                    velocity.x = 0.0f;
                    facingRight = !facingRight;
                    
                    // Move away from the edge based on current position
                    float centerX = position.x + size.x/2.0f;
                    float platformCenterX = platform.x + platform.width/2.0f;
                    
                    if (centerX < platformCenterX) {
                        // If on left side of platform, move right slightly
                        position.x = fmin(position.x + 10.0f, patrolEnd.x - size.x - 10.0f);
                    } else {
                        // If on right side of platform, move left slightly
                        position.x = fmax(position.x - 10.0f, patrolStart.x + 10.0f);
                    }
                    stateTransitionCooldown = 0.5f;
                }
            }
            break;
    }
    
    // Apply gravity
    velocity.y += 800.0f * deltaTime;
    
    // Apply velocity
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    
    // Update collision bounds
    bounds = { position.x, position.y, size.x, size.y };
    
    // Check collisions with map objects
    CheckMapCollisions(mapObjects);
    
    // Update animation
    UpdateAnimation(deltaTime);
}

void Enemy::UpdateAnimation(float deltaTime) {
    frameTimer += deltaTime;
    
    if (frameTimer >= frameSpeed) {
        int frameCount;
        
        if (state == ENEMY_IDLE) {
            frameCount = totalFramesIdle;
        } else {
            frameCount = totalFramesPatrol;
        }
        currentFrame = (currentFrame + 1) % frameCount;
        frameTimer = 0.0f;
    }
}

// Modify DetectEdge to be more reliable
bool Enemy::DetectEdge(const std::vector<MapObject>& mapObjects) {
    // Create a wider rectangle to check ahead for edge detection
    float edgeCheckDistance = 10.0f;
    float checkWidth = 8.0f; // Wider check to be more stable
    
    Rectangle edgeCheck;
    
    if (facingRight) {
        edgeCheck = { position.x + size.x - 2.0f, position.y + size.y + 1.0f, edgeCheckDistance, 10.0f };
    } else {
        edgeCheck = { position.x - edgeCheckDistance + 2.0f, position.y + size.y + 1.0f, edgeCheckDistance, 10.0f };
    }
    
    // Check if the edge detector is still over the platform
    if (CheckCollisionRecs(edgeCheck, platform)) {
        return false; // Still on platform, no edge detected
    }
    
    // Also check against other map objects that might be walkable
    for (const auto& obj : mapObjects) {
        if (obj.type == "ground" && CheckCollisionRecs(edgeCheck, obj.rect)) {
            return false; // Found another surface, no edge detected
        }
    }
    
    return true; // Off the platform, edge detected
}

void Enemy::CheckMapCollisions(const std::vector<MapObject>& mapObjects) {
    isGrounded = false;
    
    // First check if still on patrol platform
    if (position.y + size.y >= platform.y && 
        position.y + size.y <= platform.y + 10 &&
        position.x + size.x > platform.x && 
        position.x < platform.x + platform.width) {
        
        position.y = platform.y - size.y;
        velocity.y = 0;
        isGrounded = true;
    }
    
    // Then check other collisions
    for (const auto& obj : mapObjects) {
        if (obj.type == "ground") {
            Rectangle intersection;
            
            if (CheckCollisionRecs(bounds, obj.rect)) {
                // Check collision from top (landing)
                if (velocity.y > 0 && position.y + size.y - 10 <= obj.rect.y) {
                    position.y = obj.rect.y - size.y;
                    velocity.y = 0;
                    isGrounded = true;
                }
                // Check collision from sides - only if not in transition cooldown
                else if (stateTransitionCooldown <= 0.0f) {
                    if (velocity.x > 0) { // Right collision
                        position.x = obj.rect.x - size.x - 5.0f; // Increased margin
                        facingRight = false;
                        velocity.x = -patrolSpeed;
                        state = ENEMY_IDLE;
                        stateTimer = 0.0f;
                        stateTransitionCooldown = 0.5f; // Add cooldown
                    }
                    else if (velocity.x < 0) { // Left collision
                        position.x = obj.rect.x + obj.rect.width + 5.0f; // Increased margin
                        facingRight = true;
                        velocity.x = patrolSpeed;
                        state = ENEMY_IDLE;
                        stateTimer = 0.0f;
                        stateTransitionCooldown = 0.5f; // Add cooldown
                    }
                }
                
                // Update bounds after collision response
                bounds = { position.x, position.y, size.x, size.y };
            }
        }
    }
}

void Enemy::Render() {
    Texture2D currentTexture = (state == ENEMY_IDLE) ? idleTexture : patrolTexture;
    
    int framesPerRow = (state == ENEMY_IDLE) ? framesPerRowIdle : framesPerRowPatrol;

    // Calculate frame width and height
    float frameWidth = currentTexture.width / framesPerRow;
    float frameHeight = currentTexture.height;
    
    // Source rectangle from sprite sheet
    Rectangle source = {
        currentFrame * frameWidth,
        0.0f,
        facingRight ? frameWidth : -frameWidth,
        frameHeight
    };
    
    // Destination rectangle
    Rectangle dest = {
        position.x,
        position.y,
        size.x,
        size.y
    };
    
    // Draw the enemy
    DrawTexturePro(currentTexture, source, dest, { 0, 0 }, 0.0f, WHITE);
    
    // Uncomment for debugging
    /*
    // Debug visualization
    DrawRectangleLines(bounds.x, bounds.y, bounds.width, bounds.height, RED);
    DrawRectangleLines(platform.x, platform.y, platform.width, platform.height, GREEN);
    DrawText(state == ENEMY_IDLE ? "IDLE" : "PATROL", position.x, position.y - 20, 10, RED);
    DrawText(facingRight ? "RIGHT" : "LEFT", position.x, position.y - 10, 10, BLUE);
    
    // Draw patrol boundaries
    DrawLine(patrolStart.x, platform.y - 5, patrolStart.x, platform.y + 5, YELLOW);
    DrawLine(patrolEnd.x, platform.y - 5, patrolEnd.x, platform.y + 5, YELLOW);
    */
}

bool Enemy::CheckCollisionWithPlayer(Rectangle playerBounds) {
    return CheckCollisionRecs(bounds, playerBounds);
}
