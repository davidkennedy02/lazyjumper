#ifndef TILEMAP_H
#define TILEMAP_H

#include <raylib.h>
#include <iostream>
#include "tileson.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>

// Structure to represent map objects
struct MapObject {
    std::string name;
    std::string type;
    Rectangle rect;  // Stores x, y, width, height
    Color color;     // Object color for rendering
};

// Structure for image layers
struct ImageLayerInfo {
    Texture2D texture;
    Vector2 offset;
    Vector2 parallaxFactor;
    bool repeatX;
};

class TileMap {
public:
    TileMap();
    ~TileMap();

    bool Load(const std::string& mapPath);
    void Render(Camera2D& camera);
    void Unload();

    const std::vector<MapObject>& GetMapObjects() const;
    int GetTileWidth() const;
    int GetTileHeight() const;
    int GetMapWidth() const;
    int GetMapHeight() const;

private:
    std::unique_ptr<tson::Map> map;
    std::unordered_map<int, Texture2D> tileTextures;
    std::vector<ImageLayerInfo> imageLayers;
    std::vector<MapObject> mapObjects;
    
    int tileWidth;
    int tileHeight;
    int mapWidth;
    int mapHeight;
    
    // Helper functions for rendering different layer types
    void RenderImageLayers(float cameraX, float cameraY, float cameraOffsetX, float cameraOffsetY);
    void RenderTileLayers(float cameraX, float cameraY, float cameraOffsetX, float cameraOffsetY);
    void RenderObjectLayers();
};

#endif // TILEMAP_H
