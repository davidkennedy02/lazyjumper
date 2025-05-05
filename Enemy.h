#ifndef ENEMY_H
#define ENEMY_H

#include <raylib.h>
#include <vector>
#include <cmath>

// Forward declaration
struct MapObject;

enum EnemyState {
    ENEMY_IDLE,
    ENEMY_PATROLLING
};

class Enemy {
private:
    Vector2 position;
    Vector2 velocity;
    Vector2 size;
    Rectangle bounds;
    
    Texture2D idleTexture;
    Texture2D patrolTexture;
    
    // Animation properties
    int currentFrame;
    float frameTimer;
    int framesPerRowIdle;
    int framesPerRowPatrol;
    int totalFramesIdle;
    int totalFramesPatrol;
    float frameSpeed;

    // State variables
    EnemyState state;
    bool facingRight;
    float stateTimer;
    float idleDuration;
    float patrolSpeed;
    float stateTransitionCooldown; // Prevents rapid state transitions
    
    // Platform and patrol information
    Rectangle platform;
    Vector2 patrolStart;
    Vector2 patrolEnd;
    float patrolMargin;  // Small margin from platform edges
    
    // Physics state
    bool isGrounded;
    
    void UpdateAnimation(float deltaTime);
    bool DetectEdge(const std::vector<MapObject>& mapObjects);

public:
    Enemy(Texture2D idleTex, Texture2D patrolTex, Rectangle platformRect, float margin = 10.0f);
    ~Enemy() {}
    
    void Update(float deltaTime, const std::vector<MapObject>& mapObjects);
    void Render();
    bool CheckCollisionWithPlayer(Rectangle playerBounds);
    void CheckMapCollisions(const std::vector<MapObject>& mapObjects);
    
    // Getters
    Vector2 GetPosition() const { return position; }
    Rectangle GetBounds() const { return bounds; }
    Rectangle GetPlatform() const { return platform; }
};

#endif // ENEMY_H
