#include <raylib.h>
#include <iostream>
#include "tileson.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>

// New structure to represent map objects
struct MapObject {
    std::string name;
    std::string type;
    Rectangle rect;  // Stores x, y, width, height
    Color color;     // Object color for rendering
};

int main()
{
    // Initialize window
    const int screenWidth = 1440;
    const int screenHeight = 960; // Reduced from 960 to 800
    InitWindow(screenWidth, screenHeight, "LazyJumper v3");
    SetTargetFPS(60);

    // Load the map with Tileson
    tson::Tileson parser;
    std::unique_ptr<tson::Map> map = parser.parse("tiled_project/level1.json");
    
    if (map->getStatus() != tson::ParseStatus::OK) {
        std::cout << "Error parsing map: " << map->getStatusMessage() << std::endl;
        CloseWindow();
        return -1;
    }

    // Define constants from the map
    const int tileWidth = map->getTileSize().x;
    const int tileHeight = map->getTileSize().y;
    const int mapWidth = map->getSize().x;
    const int mapHeight = map->getSize().y;
    
    std::cout << "Map dimensions: " << mapWidth << "x" << mapHeight << ", Infinite: " 
              << (map->isInfinite() ? "Yes" : "No") << std::endl;
    
    // Load tileset textures
    std::unordered_map<int, Texture2D> tileTextures;
    
    // Get the tileset
    auto& tilesets = map->getTilesets();
    for (auto& tileset : tilesets) {
        for (auto& tile : tileset.getTiles()) {
            std::string imagePath = "tiled_project/" + tile.getImage().string();  // Using string() method to convert path
            Image img = LoadImage(imagePath.c_str());
            Texture2D texture = LoadTextureFromImage(img);
            UnloadImage(img);
            tileTextures[tile.getId() + tileset.getFirstgid()-1] = texture;
        }
    }
    
    // Load image layers for parallax backgrounds
    struct ImageLayerInfo {
        Texture2D texture;
        Vector2 offset;
        Vector2 parallaxFactor;
        bool repeatX;
    };
    
    std::vector<ImageLayerInfo> imageLayers;
    
    for (auto& layer : map->getLayers()) {
        if (layer.getType() == tson::LayerType::ImageLayer) {
            // For image layers, getImage() returns a string, not a path object
            std::string imagePath = layer.getImage();
            
            // Fix the path by removing leading "../" and adding the correct prefix
            if (imagePath.substr(0, 3) == "../") {
                imagePath = imagePath.substr(3); // Remove the "../" prefix
            }
            
            std::cout << "Loading image layer: " << imagePath << std::endl;
            
            // Load the texture
            Image img = LoadImage(imagePath.c_str());
            Texture2D texture = LoadTextureFromImage(img);
            UnloadImage(img);
            
            // Store layer info
            ImageLayerInfo layerInfo;
            layerInfo.texture = texture;
            layerInfo.offset = { static_cast<float>(layer.getOffset().x), static_cast<float>(layer.getOffset().y) };
            
            // Get parallax factors
            tson::Vector2f parallax = layer.getParallax();
            layerInfo.parallaxFactor.x = parallax.x;
            layerInfo.parallaxFactor.y = parallax.y;
            
            layerInfo.repeatX = layer.hasRepeatX();
            
            imageLayers.push_back(layerInfo);
        }
    }
    
    // Load objects from object layers
    std::vector<MapObject> mapObjects;
    for (auto& layer : map->getLayers()) {
        if (layer.getType() == tson::LayerType::ObjectGroup) {
            const auto& objects = layer.getObjects();
            // Get layer color if specified (for debugging visualization)
            Color layerColor = RED;
            
            // Access the color property directly from the layer's properties
            // if (layer.hasProperty("color")) {
            //     std::string colorStr = layer.getProperty("color").getValue<std::string>();
            //     if (colorStr.length() > 0 && colorStr[0] == '#') {
            //         colorStr = colorStr.substr(1); // Remove # prefix
            //         unsigned int colorHex = std::stoul(colorStr, nullptr, 16);
            //         layerColor = {
            //             static_cast<unsigned char>((colorHex >> 16) & 0xFF),
            //             static_cast<unsigned char>((colorHex >> 8) & 0xFF),
            //             static_cast<unsigned char>(colorHex & 0xFF),
            //             255
            //         };
            //     }
            // }
            
            std::cout << "Loading object layer: " << layer.getName() << " with " 
                      << objects.size() << " objects" << std::endl;
                      
            for (const auto& object : objects) {
                MapObject mapObject;
                mapObject.name = object.getName();
                mapObject.type = object.getType();
                mapObject.rect = {
                    static_cast<float>(object.getPosition().x),
                    static_cast<float>(object.getPosition().y),
                    static_cast<float>(object.getSize().x),
                    static_cast<float>(object.getSize().y)
                };
                mapObject.color = layerColor;
                mapObjects.push_back(mapObject);
                
                std::cout << "  - Loaded object: " << mapObject.name 
                          << " at (" << mapObject.rect.x << "," << mapObject.rect.y 
                          << ") with size " << mapObject.rect.width << "x" << mapObject.rect.height 
                          << std::endl;
            }
        }
    }
    
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
    
    // Game loop
    while (!WindowShouldClose())
    {
        // Update camera controls
        float deltaTime = GetFrameTime();
        
        // Camera movement with WASD or arrow keys
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) camera.target.x += cameraSpeed * deltaTime;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) camera.target.x -= cameraSpeed * deltaTime;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) camera.target.y += cameraSpeed * deltaTime;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) camera.target.y -= cameraSpeed * deltaTime;
        
        // Store camera position for parallax calculations
        float cameraX = camera.target.x;
        float cameraY = camera.target.y; // Store Y position for Y-axis parallax
        
        // Begin drawing
        BeginDrawing();
        ClearBackground(BLACK);
        
        BeginMode2D(camera);
        
        // Render image layers (parallax backgrounds)
        for (auto& imageLayer : imageLayers) {
            // Calculate parallax position for both X and Y
            float parallaxX = cameraX * (1.0f - imageLayer.parallaxFactor.x);
            float parallaxY = cameraY * (1.0f - imageLayer.parallaxFactor.y);
            float offsetX = imageLayer.offset.x + parallaxX - cameraOffsetX;
            float offsetY = imageLayer.offset.y + parallaxY - cameraOffsetY;
            
            // Draw the image layer
            if (imageLayer.repeatX) {
                // Handle repeating backgrounds
                int textureWidth = imageLayer.texture.width;
                int repetitions = (screenWidth / textureWidth) + 2;
                
                for (int i = -1; i < repetitions; i++) {
                    DrawTexture(
                        imageLayer.texture,
                        offsetX + (i * textureWidth),
                        offsetY,
                        WHITE
                    );
                }
            } else {
                // Non-repeating background
                DrawTexture(
                    imageLayer.texture,
                    offsetX,
                    offsetY,
                    WHITE
                );
            }
        }
        
        // Render tile layers
        for (auto& layer : map->getLayers()) {
            if (layer.getType() == tson::LayerType::TileLayer) {
                // Get parallax factors for the tile layer
                tson::Vector2f parallax = layer.getParallax();
                float parallaxX = cameraX * (1.0f - parallax.x);
                float parallaxY = cameraY * (1.0f - parallax.y);
                
                // Get layer offset
                Vector2 layerOffset = { 
                    static_cast<float>(layer.getOffset().x) + parallaxX, 
                    static_cast<float>(layer.getOffset().y) + parallaxY 
                };
                
                // Check if this is an infinite map using chunks
                const auto& chunks = layer.getChunks();
                if (!chunks.empty()) {
                    // For infinite maps, iterate through the chunks
                    for (const auto& chunk : chunks) {
                        const auto& chunkData = chunk.getData();
                        int chunkWidth = chunk.getSize().x;
                        int chunkHeight = chunk.getSize().y;
                        int chunkX = chunk.getPosition().x;
                        int chunkY = chunk.getPosition().y;
                        
                        for (size_t i = 0; i < chunkData.size(); ++i) {
                            uint32_t tileId = chunkData[i];
                            if (tileId == 0) continue; // Skip empty tiles
                            
                            // Calculate position within the chunk
                            int xInChunk = i % chunkWidth;
                            int yInChunk = i / chunkWidth;
                            
                            // Calculate the world position of the tile
                            float x = (chunkX + xInChunk) * tileWidth + layerOffset.x - cameraOffsetX;
                            float y = (chunkY + yInChunk) * tileHeight + layerOffset.y - cameraOffsetY;
                            
                            // Draw the tile if texture exists
                            if (tileTextures.count(tileId) > 0) {
                                DrawTexture(tileTextures[tileId], x, y, WHITE);
                            }
                        }
                    }
                } else {
                    // Process regular tile layer data
                    const std::vector<uint32_t>& tileData = layer.getData();
                    int layerWidth = layer.getSize().x;
                    
                    for (size_t i = 0; i < tileData.size(); ++i) {
                        uint32_t tileId = tileData[i];
                        if (tileId == 0) continue; // Skip empty tiles
                        
                        // Calculate position from index with parallax offset
                        float x = (i % layerWidth) * tileWidth + layerOffset.x - cameraOffsetX;
                        float y = (i / layerWidth) * tileHeight + layerOffset.y - cameraOffsetY;
                        
                        // Draw the tile if texture exists
                        if (tileTextures.count(tileId) > 0) {
                            DrawTexture(tileTextures[tileId], x, y, WHITE);
                        }
                    }
                }
            }
        }
        
        // Render objects
        for (const auto& object : mapObjects) {
            // Calculate the actual position taking into account the camera position
            Rectangle drawRect = {
                object.rect.x - cameraOffsetX,
                object.rect.y - cameraOffsetY,
                object.rect.width,
                object.rect.height
            };
            
            // For debugging, draw rectangles with outlines and fill with semi-transparent color
            Color fillColor = object.color;
            fillColor.a = 100; // Make it semi-transparent
            DrawRectangleRec(drawRect, fillColor);
            DrawRectangleLinesEx(drawRect, 2, object.color);
            
            // Draw object name for debugging
            DrawText(object.name.c_str(), 
                    drawRect.x + 5, 
                    drawRect.y + 5, 
                    20, WHITE);
        }
        
        EndMode2D();
        
        // Draw UI or debug info here
        DrawFPS(10, 10);
        DrawText(TextFormat("Camera: %.2f, %.2f", camera.target.x, camera.target.y), 10, 30, 20, BLACK);
        DrawText("Controls: WASD/Arrows - Move", 10, screenHeight - 30, 20, DARKGRAY);
        
        EndDrawing();
    }
    
    // Unload textures
    for (auto& pair : tileTextures) {
        UnloadTexture(pair.second);
    }
    
    for (auto& imageLayer : imageLayers) {
        UnloadTexture(imageLayer.texture);
    }
    
    CloseWindow();
    return 0;
}

