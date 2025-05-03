#include "TileMap.h"

TileMap::TileMap() : 
    tileWidth(0), 
    tileHeight(0), 
    mapWidth(0), 
    mapHeight(0) {
}

TileMap::~TileMap() {
    Unload();
}

bool TileMap::Load(const std::string& mapPath) {
    // Load the map with Tileson
    tson::Tileson parser;
    map = parser.parse(mapPath);
    
    if (map->getStatus() != tson::ParseStatus::OK) {
        std::cout << "Error parsing map: " << map->getStatusMessage() << std::endl;
        return false;
    }

    // Define constants from the map
    tileWidth = map->getTileSize().x;
    tileHeight = map->getTileSize().y;
    mapWidth = map->getSize().x;
    mapHeight = map->getSize().y;
    
    std::cout << "Map dimensions: " << mapWidth << "x" << mapHeight << ", Infinite: " 
              << (map->isInfinite() ? "Yes" : "No") << std::endl;
    
    // Load tileset textures
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
    float cameraOffsetX = GetScreenWidth() / 2.0f; // Using raylib functions to get screen dimensions
    float cameraOffsetY = GetScreenHeight() / 2.0f;
    
    for (auto& layer : map->getLayers()) {
        if (layer.getType() == tson::LayerType::ObjectGroup) {
            const auto& objects = layer.getObjects();

            // Get layer color if specified (for debugging visualization)
            Color layerColor = RED;
            
            std::cout << "Loading object layer: " << layer.getName() << " with " 
                      << objects.size() << " objects" << std::endl;
                      
            for (const auto& object : objects) {
                MapObject mapObject;
                mapObject.name = object.getName();
                mapObject.type = object.getType();
                mapObject.rect = {
                    static_cast<float>(object.getPosition().x - cameraOffsetX),
                    static_cast<float>(object.getPosition().y - cameraOffsetY),
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
    
    return true;
}

void TileMap::RenderImageLayers(float cameraX, float cameraY, float cameraOffsetX, float cameraOffsetY) {
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
            int repetitions = (GetScreenWidth() / textureWidth) + 2;
            
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
}

void TileMap::RenderTileLayers(float cameraX, float cameraY, float cameraOffsetX, float cameraOffsetY) {
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
}

void TileMap::RenderObjectLayers() {
    // This function is currently commented out in main.cpp
    // Keeping it for completeness
    /*
    for (const auto& object : mapObjects) {
        // Calculate the actual position taking into account the camera position
        Rectangle drawRect = {
            object.rect.x,
            object.rect.y,
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
    */
}

void TileMap::Render(Camera2D& camera) {
    // Store camera position for parallax calculations
    float cameraX = camera.target.x;
    float cameraY = camera.target.y;
    float cameraOffsetX = camera.offset.x;
    float cameraOffsetY = camera.offset.y;
    
    RenderImageLayers(cameraX, cameraY, cameraOffsetX, cameraOffsetY);
    RenderTileLayers(cameraX, cameraY, cameraOffsetX, cameraOffsetY);
    // RenderObjectLayers();  // Uncomment if object rendering is needed
}

void TileMap::Unload() {
    // Unload textures
    for (auto& pair : tileTextures) {
        UnloadTexture(pair.second);
    }
    tileTextures.clear();
    
    for (auto& imageLayer : imageLayers) {
        UnloadTexture(imageLayer.texture);
    }
    imageLayers.clear();
    mapObjects.clear();
}

const std::vector<MapObject>& TileMap::GetMapObjects() const {
    return mapObjects;
}

int TileMap::GetTileWidth() const {
    return tileWidth;
}

int TileMap::GetTileHeight() const {
    return tileHeight;
}

int TileMap::GetMapWidth() const {
    return mapWidth;
}

int TileMap::GetMapHeight() const {
    return mapHeight;
}
