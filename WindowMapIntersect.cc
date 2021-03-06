#ifdef _MSC_VER
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#endif

#include <stdexcept>
#include <cassert>


// Types
//---------------------------------------------------------------------------------------------------

typedef struct IntVec2
{
    int X; // X coordinate or column number
    int Y; // Y coordinate or row number
} IntVec2_t;

enum class WindowIntersectType_t
{
    // window is completely out of the map
    TotallyOut = 0,
    // window is completely inside the map
    TotallyIn,
    // window contains the northwest corner of the map
    NorthWest,
    // window contains the north wall of the map
    North,
    // window contains the northeast corner of the map
    NorthEast,
    // window contains the east wall of the map
    East,
    // window contains the southeast corner of the map
    SouthEast,
    // window contains the south wall of the map
    South,
    // window contains the southwest corner of the map
    SouthWest,
    // window contains the west wall of the map
    West
};

typedef struct InitSDLValues_s
{
    SDL_Window*  Window;
    SDL_Renderer* Renderer;
    
} InitSDLValues_t;

struct TestTextures_t
{
    SDL_Texture* NorthWest;
    SDL_Texture* North;
    SDL_Texture* NorthEast;
    SDL_Texture* East;
    SDL_Texture* SouthEast;
    SDL_Texture* South;
    SDL_Texture* SouthWest;
    SDL_Texture* West;
    SDL_Texture* AllIn;
    SDL_Texture* AllOut;
    SDL_Texture* Moveable;
};

struct Color_t
{
    int R, G, B;
};

// Constants
//---------------------------------------------------------------------------------------------------


constexpr Color_t cMagenta = {255, 0, 255};

// it's assumed that tiles are all cGridSize_px x cGridSize_px in dimensions
constexpr int cGridSize_px = 16;

// change this to change the size of the window
const IntVec2_t cScreenResolution = {1024, 768};

// this is more FYI than anything
const int cFPS = 60;

// 1/60 is 0.016666666666666666
const int cFrameDuration_ms = 16;

// imaginary, simulated window looking at our map
const IntVec2_t cWindowSize_px = {32, 32};

const IntVec2_t cWindowSize_Tiles = {cWindowSize_px.X / cGridSize_px, cWindowSize_px.Y / cGridSize_px};

// account for one more tile in the map render size; if a tile is partially out of view it still has to be rendered.
// in this demo's case, a 2 x 2 viewable area will need a 3 x 3 tile map render area.
const IntVec2_t cMapRenderTextureSize_Tiles = {cWindowSize_Tiles.X + 1, cWindowSize_Tiles.Y + 1};

const IntVec2_t cMapRenderTextureSize_px = {cMapRenderTextureSize_Tiles.X * cGridSize_px, cMapRenderTextureSize_Tiles.Y * cGridSize_px};

const IntVec2_t cMapOrigin = {432, 322};

constexpr IntVec2_t cTileSetSize_Tiles = {8, 8};

// DEMO: The map size would definitely NOT be the same as the tileset size in a real game
constexpr IntVec2_t cMapSize_Tiles = cTileSetSize_Tiles;


// Globals
//---------------------------------------------------------------------------------------------------

// various SDL resources required for rendering.
InitSDLValues_t SDLGlobals;

SDL_Texture* MapTestTexture = nullptr;

IntVec2_t MapTextureSize;

TestTextures_t ScreenRenderTextures;
TestTextures_t MapRenderTextures;

IntVec2_t MousePosition;

//--------------------------------------------------------------------------------------
// Misc. Utility Functions
//--------------------------------------------------------------------------------------


IntVec2_t InquireTextureSize(SDL_Texture* texture)
{
    int width, height;

    SDL_QueryTexture(texture, NULL, NULL, &width, &height);

    IntVec2_t size = {width, height};

    return size;
}

SDL_Texture* LoadImage(SDL_Renderer *renderer, const char* path)
{
    SDL_Texture *texture = NULL;

    SDL_Surface* image = IMG_Load(path);

    if (image != NULL) 
    {
        texture = SDL_CreateTextureFromSurface(renderer, image);

        SDL_FreeSurface(image);
        image = NULL;
    } 
    else 
    {
        printf("Image '%s' could not be loaded. SDL Error: %s\n", path, SDL_GetError());
    }

    return texture;

}

bool PointInRect(const IntVec2_t& point, const IntVec2_t& rectTopLeft, const IntVec2_t& rectSize)
{
    // you might be wondering what the point of IntVect2 is if SDL_Point exists,
    // well, there isn't one, I just didn't know SDL already had something.
    SDL_Rect rect;
    rect.x = rectTopLeft.X;
    rect.y = rectTopLeft.Y;
    rect.w = rectSize.X;
    rect.h = rectSize.Y;

    SDL_Point sdlPoint;
    sdlPoint.x = point.X;
    sdlPoint.y = point.Y;

    SDL_bool inRect = SDL_PointInRect(&sdlPoint, &rect);

    bool bInRect = (inRect == SDL_TRUE);

    //printf("Point (%d, %d) in rect [(%d, %d), (%d, %d)]: %d (b) %d\n", point.X, point.Y, rect.x, rect.y, rect.w, rect.h, inRect, bInRect);

    return bInRect;
}

IntVec2_t FindGridCoordinateForPoint_RoundUp(IntVec2_t point, int gridSize)
{
    int columnIndex = point.X / gridSize;
    int rowIndex = point.Y / gridSize;    

    if((point.X % gridSize) != 0)
    {
        columnIndex++;
    }

    if((point.Y % gridSize) != 0)
    {
        rowIndex++;
    }

    return {columnIndex, rowIndex};
}

IntVec2_t FindGridCoordinateForPoint(IntVec2_t point, int gridSize)
{
    int columnIndex = point.X / gridSize;
    int rowIndex = point.Y / gridSize;    

    return {columnIndex, rowIndex};
}

//--------------------------------------------------------------------------------------
// Demo / placeholder only functions
//--------------------------------------------------------------------------------------

static inline int InRange(int min, int value, int max);

// this is only for the sake of the demo, in a real game you would look up the image 
// you need to draw from the map
void DEMO_DrawTile(SDL_Texture* mapRenderTexture, SDL_Texture* tileSetTexture, const IntVec2_t& sourceTileCoordinate_tiles, const IntVec2_t& textureDestCoordinate_tiles)
{
    

    assert(InRange(0, sourceTileCoordinate_tiles.X, cTileSetSize_Tiles.X));
    assert(InRange(0, sourceTileCoordinate_tiles.Y, cTileSetSize_Tiles.Y));
    
    assert(InRange(0, textureDestCoordinate_tiles.X, cMapRenderTextureSize_Tiles.X));
    assert(InRange(0, textureDestCoordinate_tiles.Y, cMapRenderTextureSize_Tiles.Y));

    // in this demo's case, the tileset is the same as the map size, this will certainly NOT be the case in a real game

    const IntVec2_t sourceTilesetCoord_px = {sourceTileCoordinate_tiles.X * cGridSize_px, sourceTileCoordinate_tiles.Y * cGridSize_px};

    // For this partiuclar demo we could reduce the amount of calls to SDL_RenderCopy by copying the entire contiguous area at once,
    // but this might not be a useful optimization in a real game unless it just so happened that the tileset exactly contained
    // what was in the player's fov (unlikely unless you rendered the map itself into the tileset, which is not what I have in mind)
    SDL_Rect srcRect = {0};
    srcRect.x = sourceTilesetCoord_px.X;
    srcRect.y = sourceTilesetCoord_px.Y;
    srcRect.w = cGridSize_px;
    srcRect.h = cGridSize_px;

    SDL_Rect destRect = {0};
    destRect.x = textureDestCoordinate_tiles.X * cGridSize_px;
    destRect.y = textureDestCoordinate_tiles.Y * cGridSize_px;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;
    
    SDL_SetRenderTarget(SDLGlobals.Renderer, mapRenderTexture);
    SDL_RenderCopy(SDLGlobals.Renderer, tileSetTexture, &srcRect, &destRect);
    SDL_SetRenderTarget(SDLGlobals.Renderer, nullptr);
}



//--------------------------------------------------------------------------------------
// Tile rendering functions
//--------------------------------------------------------------------------------------

static inline void CheckArea(const SDL_Rect& offset, const IntVec2_t& windowSize)
{
    assert(offset.x >= 0);
    assert(offset.y >= 0);
    assert(offset.w >= 0);
    assert(offset.h >= 0);

    assert(offset.x < windowSize.X);
    assert(offset.y < windowSize.Y);
    assert(offset.w <= windowSize.X);
    assert(offset.h <= windowSize.Y);

}

static inline int max(int a, int b)
{
    if(a > b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

static inline int min(int a, int b)
{
    if(a < b)
    {
        return a;
    }
    else
    {
        return b;        
    }
    
}

static inline int InRange(int min, int value, int max)
{
    if(value < min)
    {
        return false;
    }

    if(value > max)
    {
        return false;
    }

    return true;
}

// You really need to see this drawn out on a sheet of paper to best understand this to be honest
// see scanned png hand written pages in this folder.
//
// Returns a value that represents the window's positioning; This value tells you if the window hangs off the corner of the map, the sides, or neither.
WindowIntersectType_t GetWindowIntersectType(const IntVec2_t& mapSize_px, const IntVec2_t& windowNorthWestCorner, const IntVec2_t& windowSize_px)
{
    const IntVec2_t windowNorthEastCorner = {windowNorthWestCorner.X + windowSize_px.X, windowNorthWestCorner.Y + 0};
    const IntVec2_t windowSouthWestCorner = {windowNorthWestCorner.X + 0,               windowNorthWestCorner.Y + windowSize_px.Y};
    const IntVec2_t windowSouthEastCorner = {windowNorthWestCorner.X + windowSize_px.X, windowNorthWestCorner.Y + windowSize_px.Y};

    const IntVec2_t mapTopLeft = {0, 0};

    const bool northWestCornerIn = PointInRect(windowNorthWestCorner, mapTopLeft, mapSize_px);
    const bool northEastCornerIn = PointInRect(windowNorthEastCorner, mapTopLeft, mapSize_px);
    const bool southWestCornerIn = PointInRect(windowSouthWestCorner, mapTopLeft, mapSize_px);
    const bool southEastCornerIn = PointInRect(windowSouthEastCorner, mapTopLeft, mapSize_px);
    

    if(northWestCornerIn && northEastCornerIn && southWestCornerIn && southEastCornerIn)
    {
        return WindowIntersectType_t::TotallyIn;
    }

    if(!northWestCornerIn && !northEastCornerIn && !southWestCornerIn && !southEastCornerIn)
    {
        return WindowIntersectType_t::TotallyOut;
    }

    if(southWestCornerIn && southEastCornerIn)
    {
        return WindowIntersectType_t::North;
    }
    else if(northWestCornerIn && southWestCornerIn)
    {
        return WindowIntersectType_t::East;
    }
    else if(northWestCornerIn && northEastCornerIn)
    {
        return WindowIntersectType_t::South;
    }
    else if(northEastCornerIn && southEastCornerIn)
    {
        return WindowIntersectType_t::West;
    }
    else if(southEastCornerIn)
    {
        return WindowIntersectType_t::NorthWest;
    }
    else if(southWestCornerIn)
    {
        return WindowIntersectType_t::NorthEast;
    }
    else if(northWestCornerIn)
    {
        return WindowIntersectType_t::SouthEast;
    }
    else if(northEastCornerIn)
    {
        return WindowIntersectType_t::SouthWest;
    } 
    else
    {
        assert(0);
        throw std::logic_error("Impossible intersect type detected.");
    }

}

// See scanned png hand written pages in this folder.
// Returns a rectangle (top left corner in map coordinates) containing the dimensions of the map to copy into the window.
SDL_Rect GetMapRenderRectangle(const IntVec2_t& mapSize_px, const IntVec2_t& windowNorthWestCorner_px, const IntVec2_t& windowSize_px)
{

    WindowIntersectType_t intersectType = GetWindowIntersectType(mapSize_px, windowNorthWestCorner_px, windowSize_px);

    if(intersectType == WindowIntersectType_t::TotallyOut)
    {
        // nothing to draw from the map
        return {0,0,0,0};
    }

    if(intersectType == WindowIntersectType_t::TotallyIn)
    {
        SDL_Rect mapRenderRect{0};
        mapRenderRect.x = windowNorthWestCorner_px.X;
        mapRenderRect.y = windowNorthWestCorner_px.Y;

        mapRenderRect.w = windowSize_px.X;
        mapRenderRect.h = windowSize_px.Y;

        return mapRenderRect;
    }

    // abbreviations
    const IntVec2_t& winP   = windowNorthWestCorner_px; 
    const IntVec2_t& winSiz = windowSize_px;
    const IntVec2_t& mSiz   = mapSize_px;

    const int northHeight   = winP.Y + winSiz.Y;
    const int southHeight   = mSiz.Y - winP.Y;

    const int westWidth     = winP.X + winSiz.X;
    const int eastWidth     = mSiz.X - winP.X;

    switch(intersectType)
    {
        case WindowIntersectType_t::NorthWest:
        {
            SDL_Rect mapRenderRect{0};    
            mapRenderRect.x = 0;
            mapRenderRect.y = 0;

            mapRenderRect.w = westWidth;
            mapRenderRect.h = northHeight;

            return mapRenderRect;
        }

        case WindowIntersectType_t::North:
        {
            SDL_Rect mapRenderRect{0};
            mapRenderRect.x = winP.X;
            mapRenderRect.y = 0;

            mapRenderRect.w = winSiz.X;
            mapRenderRect.h = northHeight;

            return mapRenderRect;
        }

        case WindowIntersectType_t::NorthEast:
        {
            SDL_Rect mapRenderRect{0};
            mapRenderRect.x = winP.X;
            mapRenderRect.y = 0;

            mapRenderRect.w = eastWidth;
            mapRenderRect.h = northHeight;

            return mapRenderRect;
        }

        case WindowIntersectType_t::East:
        {
            SDL_Rect mapRenderRect{0};
            mapRenderRect.x = winP.X;
            mapRenderRect.y = winP.Y;

            mapRenderRect.w = eastWidth;
            mapRenderRect.h = winSiz.Y;

            return mapRenderRect;
        }

        case WindowIntersectType_t::SouthEast:
        {
            SDL_Rect mapRenderRect{0};
            mapRenderRect.x = winP.X;
            mapRenderRect.y = winP.Y;

            mapRenderRect.w = eastWidth;
            mapRenderRect.h = southHeight;

            return mapRenderRect;
        }

        case WindowIntersectType_t::South:
        {
            SDL_Rect mapRenderRect{0};
            mapRenderRect.x = winP.X;
            mapRenderRect.y = winP.Y;

            mapRenderRect.w = winSiz.X;
            mapRenderRect.h = southHeight;

            return mapRenderRect;
        }

        case WindowIntersectType_t::SouthWest:
        {
            SDL_Rect mapRenderRect{0};
            mapRenderRect.x = 0;
            mapRenderRect.y = winP.Y;

            mapRenderRect.w = westWidth;
            mapRenderRect.h = southHeight;

            return mapRenderRect;
        }

        case WindowIntersectType_t::West:
        {
            SDL_Rect mapRenderRect{0};
            mapRenderRect.x = 0;
            mapRenderRect.y = winP.Y;

            mapRenderRect.w = westWidth;
            mapRenderRect.h = winSiz.Y;

            return mapRenderRect;
        }

        default:
        {
            assert(0);
            throw std::logic_error("Impossible intersect type dected.");
        }
    }
    

}

// try and draw a window, return the rectangle that it drew
// draw the tileset underneath it in red, draw the area it rendered in white maybe
SDL_Rect DrawTiles(SDL_Texture* mapRenderTexture, SDL_Texture* tileSetTexture, const IntVec2_t& topLeftTile, const IntVec2_t& topLeftOfTileToWindow_px, const IntVec2_t& windowSize_Tiles, const IntVec2_t& mapSize_Tiles)
{ 
    // if the window is shifted right or down in the tile it's in, you'll have to render one extra tile to the east / south
    IntVec2_t renderNextOffset = {0,0};

    if(topLeftOfTileToWindow_px.X > 0)
    {
        renderNextOffset.X = 1;
    }

    if(topLeftOfTileToWindow_px.Y > 0)
    {
        renderNextOffset.Y = 1;
    }


    int maxEast = min(mapSize_Tiles.X - 1, topLeftTile.X + windowSize_Tiles.X + renderNextOffset.X);
    int maxSouth = min(mapSize_Tiles.Y - 1, topLeftTile.Y + windowSize_Tiles.Y + renderNextOffset.Y);

    int minWest = max(0, topLeftTile.X);
    int minNorth = max(0, topLeftTile.Y);

    int validRows = 0;
    int validColumns = 0;

    
    for(int rowIndex = topLeftTile.Y; rowIndex <= (topLeftTile.Y + windowSize_Tiles.Y); rowIndex++)
    {
        if(!InRange(minNorth, rowIndex, maxSouth))
        {
            continue; 
        }

        int oldValidColumns = validColumns;
        
        if(oldValidColumns != 0)
        {
            // should not change shape from one row to the next
            assert(oldValidColumns == validColumns);
        }

        validColumns = 0;
        // look 1 past the size, otherwise if you're in the middle of tile 1,
        // 1 + 2,  < 3, stop at tile 2
        for(int columnIndex = topLeftTile.X; columnIndex <= (topLeftTile.X + windowSize_Tiles.X); columnIndex++)
        {
            if(!InRange(minWest, columnIndex, maxEast))
            {
                continue;
            }

            IntVec2_t mapCoordinate_Tiles = {columnIndex, rowIndex};
            IntVec2_t textureCoordinate_Tiles = {validColumns, validRows};
            DEMO_DrawTile(mapRenderTexture, tileSetTexture, mapCoordinate_Tiles, textureCoordinate_Tiles);

            validColumns++;
        }

        // don't count the row if it contained zero valid columns
        if(validColumns != 0)
        {
            validRows++;
        }
    }

    SDL_Rect resultRect = {0};
    resultRect.w = validColumns * cGridSize_px;
    resultRect.h = validRows * cGridSize_px;
    resultRect.x = minWest * cGridSize_px;
    resultRect.y = minNorth * cGridSize_px;

    return resultRect;
}

// Returns a 2D point giving the top left corner of a rectangle that serves as the destination of where the map pixels will be copied to the screen.
// This is needed because we don't copy the map pixels to the bottom left of the map render texture
// (and it wouldn't help anyway because the map render texture is 1 tile bigger than the screen in width and height)
//
// This might be a bit hard to visualize, but if you stood on the northwest corner of the map, you would see the map in the bottom right corner,
// and the sky would fill the rest of the top left
IntVec2_t GetDrawRenderOffset(const SDL_Rect& srcFromRenderRect, const IntVec2_t& windowSize, const WindowIntersectType_t& intersectType)
{
    switch(intersectType)
    {
        case WindowIntersectType_t::TotallyOut:
        case WindowIntersectType_t::TotallyIn:
        {
            return {0, 0};
        }

        case WindowIntersectType_t::NorthWest:
        {
            // put it in the bottom right corner
            IntVec2_t result = {windowSize.X - srcFromRenderRect.w, windowSize.Y - srcFromRenderRect.h};
            return result;
        }

        case WindowIntersectType_t::North:
        {
            // put it on the bottom
            IntVec2_t result = {0, windowSize.Y - srcFromRenderRect.h};
            return result;
        }
        case WindowIntersectType_t::NorthEast:
        {
            // put it on the bottom
            IntVec2_t result = {0, windowSize.Y - srcFromRenderRect.h};
            return result;
        }

        case WindowIntersectType_t::East:
        {
            // leave as is
            IntVec2_t result = {0,0};
            return result;
        }

        case WindowIntersectType_t::SouthEast:
        {
            // leave as is
            IntVec2_t result = {0,0};
            return result;
        }

        case WindowIntersectType_t::South:
        {
            // leave as is
            IntVec2_t result = {0,0};
            return result;
        }


        case WindowIntersectType_t::SouthWest:
        {
            // offset to right
            IntVec2_t result = {windowSize.X - srcFromRenderRect.w,0};
            return result;
        }
        
        case WindowIntersectType_t::West:
        {
            //offset to right
            IntVec2_t result = {windowSize.X - srcFromRenderRect.w,0};
            return result;
        }

        default:
        {
            assert(0);
            throw std::logic_error("Impossible intersection type");
        }
        
    }
}

// Returns a rectangle showing the area to copy from, to get all of the useful pixels rendered from the map that would be in the player's view
SDL_Rect GetTextureReadArea(const IntVec2_t& windowTopLeft_RelToTextureTopLeft, const IntVec2_t& windowSize, const WindowIntersectType_t& intersectType, const SDL_Rect& renderedRectangle)
{
    // yeah, we do need the offset to be not clamped at 0, because otherwise hit doesn't truncate the northeast case
    const IntVec2_t textureOffset = {max(0, windowTopLeft_RelToTextureTopLeft.X), max(0, windowTopLeft_RelToTextureTopLeft.Y)};

    switch(intersectType)
    {
        case WindowIntersectType_t::TotallyOut:
        {
            SDL_Rect rect = {0,0,0,0};

            // there's really no reason for CheckArea to be in this case, but it's a handy breakpoint location
            CheckArea(rect, windowSize);
            return rect;
        }
        case WindowIntersectType_t::TotallyIn:
        {
            SDL_Rect rect = {0};
            rect.x = windowTopLeft_RelToTextureTopLeft.X;
            rect.y = windowTopLeft_RelToTextureTopLeft.Y;
            rect.w = windowSize.X;
            rect.h = windowSize.Y;

            CheckArea(rect, windowSize);

            return rect;
        }

        // maybe I can reduce some of this by doing some kind of absolute value for some of the width and height calculations
        // I'm not sure it's worth it, I thought I had a more elegant method about 3 times now.

        // very special case, can't be merged with anything
        case WindowIntersectType_t::NorthWest:
        {
            SDL_Rect rect = {0};
            rect.x = 0; // is NOT windowTopLeft_RelToTextureTopLeft.X, which is NOT zero
            rect.y = 0; // is NOT windowTopLeft_RelToTextureTopLeft.Y, which is NOT zero
            rect.w = windowSize.X + windowTopLeft_RelToTextureTopLeft.X; // really is plus! windowTopLeft is negative!
            rect.h = windowSize.Y + windowTopLeft_RelToTextureTopLeft.Y; // really is plus! windowTopLeft is negative!

            CheckArea(rect, windowSize);

            return rect;
        }

        // possible merge with N/NE
        case WindowIntersectType_t::North:
        {
            SDL_Rect rect = {0};
            rect.x = windowTopLeft_RelToTextureTopLeft.X;
            rect.y = 0; // is NOT windowTopLeft_RelToTextureTopLeft.Y, which is NOT zero
            rect.w = windowSize.X;
            rect.h = windowSize.Y + windowTopLeft_RelToTextureTopLeft.Y; // really is plus! windowTopLeft is negative!

            CheckArea(rect, windowSize);

            return rect;
        }
        case WindowIntersectType_t::East:
        {
            SDL_Rect rect = {0};
            rect.x = windowTopLeft_RelToTextureTopLeft.X;
            rect.y = windowTopLeft_RelToTextureTopLeft.Y;
            rect.w = renderedRectangle.w - windowTopLeft_RelToTextureTopLeft.X;
            rect.h = windowSize.Y;

            CheckArea(rect, windowSize);

            return rect;
        }
        
        case WindowIntersectType_t::NorthEast:
        {
            SDL_Rect rect = {0};
            rect.x = windowTopLeft_RelToTextureTopLeft.X;
            rect.y = 0; // is NOT windowTopLeft_RelToTextureTopLeft.Y, which is NOT zero
            rect.w = renderedRectangle.w - windowTopLeft_RelToTextureTopLeft.X;
            rect.h = windowSize.Y + windowTopLeft_RelToTextureTopLeft.Y; // for east, windowTopLeft_RelToTexturetopLeft will be 0; really is plus, Y is negative for NE

            CheckArea(rect, windowSize);

            return rect;
        }
        case WindowIntersectType_t::SouthEast:
        {
            SDL_Rect rect = {0};
            rect.x = windowTopLeft_RelToTextureTopLeft.X;
            rect.y = windowTopLeft_RelToTextureTopLeft.Y;
            rect.w = renderedRectangle.w - windowTopLeft_RelToTextureTopLeft.X;
            rect.h = renderedRectangle.h - windowTopLeft_RelToTextureTopLeft.Y;

            CheckArea(rect, windowSize);

            return rect;
        }
        case WindowIntersectType_t::South:
        {
            SDL_Rect rect = {0};

            rect.x = windowTopLeft_RelToTextureTopLeft.X;
            rect.y = windowTopLeft_RelToTextureTopLeft.Y;
            rect.w = windowSize.X;
            rect.h = renderedRectangle.h - windowTopLeft_RelToTextureTopLeft.Y;

            CheckArea(rect, windowSize);

            return rect;
        }

        case WindowIntersectType_t::SouthWest:
        {
            SDL_Rect rect = {0};
            rect.x = 0; // is NOT windowTopLeft_RelToTextureTopLeft, which is NOT zero
            rect.y = windowTopLeft_RelToTextureTopLeft.Y;
            rect.w = windowSize.X + windowTopLeft_RelToTextureTopLeft.X; // is NOT minus, X is negative, we want to truncate off the part to the left of the texture's bounds
            rect.h = renderedRectangle.h - windowTopLeft_RelToTextureTopLeft.Y;

            CheckArea(rect, windowSize);

            return rect;
        }

        case WindowIntersectType_t::West:
        {
            SDL_Rect rect = {0};
            rect.x = 0; // is NOT windowTopLeft_RelToTextureTopLeft, which is NOT zero
            rect.y = windowTopLeft_RelToTextureTopLeft.Y;
            rect.w = windowSize.X + windowTopLeft_RelToTextureTopLeft.X; // is NOT minus, X is negative, we want to truncate off the part to the left of the texture's bounds
            rect.h = windowSize.Y;

            CheckArea(rect, windowSize);

            return rect;
        }

        default:
        {
            assert(0);
            throw std::logic_error("Impossible intersection type");
        }
        
    }
}

SDL_Rect RenderMapRegion(SDL_Texture* mapRenderTexture, SDL_Texture* tileSetTexture, const IntVec2_t& northWestTile, const IntVec2_t& topLeftOfTileToWindow_px, const IntVec2_t& windowSize_Tiles, const IntVec2_t& mapSize_tiles)
{
    SDL_SetRenderTarget(SDLGlobals.Renderer, mapRenderTexture);

    // you should never see this cyan color in this example, because the map has no transparent pixels.
    // in a real game you may want transparent pixels in the middle of the map to show some background.
    // 
    // If you do want that, use
    //     SDL_SetTextureBlendMode(mapRenderTexture, SDL_BLENDMODE_BLEND);
    //     SDL_SetRenderDrawColor(SDLGlobals.Renderer, 0, 0, 0, 0);
    // instead.
    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 0, 255, 255, 255);
    SDL_RenderClear(SDLGlobals.Renderer);

    // The non-demo code would draw tiles spanning between the northWestTile and southEastTile to your mapRenderTexture
    // I'm not going to do that in this demo, I'm just going to use a pre-rendered map texture. In this demo the map is already "rendered" in full.
    // I just want to focus on the geometry of what's visible, so this example does not show the code for tiles and their tile pictures.

    SDL_Rect renderedArea = DrawTiles(mapRenderTexture, tileSetTexture, northWestTile, topLeftOfTileToWindow_px, windowSize_Tiles, mapSize_tiles);

    return renderedArea;

}

SDL_Rect RenderMapToTexture(SDL_Texture* mapRenderTexture, SDL_Texture* tileSetTexture, const IntVec2_t& windowSize_Tiles, const IntVec2_t& relToMap_WindowTopLeft)
{
    const IntVec2_t gridCoordOfWindow_TopLeft = FindGridCoordinateForPoint(relToMap_WindowTopLeft, cGridSize_px);

    const IntVec2_t coordOfTopLeftOfEnclosingGrid_px = {gridCoordOfWindow_TopLeft.X * cGridSize_px, gridCoordOfWindow_TopLeft.Y * cGridSize_px};

    const IntVec2_t topLeftOfTileToWindow_px = {relToMap_WindowTopLeft.X - coordOfTopLeftOfEnclosingGrid_px.X, relToMap_WindowTopLeft.Y - coordOfTopLeftOfEnclosingGrid_px.Y};

    // render the part of the map the player can see to a texture
    SDL_Rect renderedRectangle = RenderMapRegion(mapRenderTexture, tileSetTexture, gridCoordOfWindow_TopLeft, topLeftOfTileToWindow_px, windowSize_Tiles, cMapSize_Tiles);

    return renderedRectangle;

}

void CopyRenderedMapToScreen(SDL_Texture* screenRenderTexture, SDL_Texture* mapRenderTexture, const IntVec2_t& relToMap_WindowTopLeft, const IntVec2_t& windowSize, const SDL_Rect& renderedRectangle)
{
    const IntVec2_t gridCoordOfWindow_TopLeft = FindGridCoordinateForPoint(relToMap_WindowTopLeft, cGridSize_px);

    // This is the northwest most tile coordinate that our region touches.
    const IntVec2_t topLeftValidTile = {max(0, gridCoordOfWindow_TopLeft.X), max(0, gridCoordOfWindow_TopLeft.Y)};

    // DON'T use relToRenderTexture for the intersect type! It needs to be relative to the map!
    const WindowIntersectType_t intersectType = GetWindowIntersectType(MapTextureSize, relToMap_WindowTopLeft, windowSize);

    SDL_SetRenderTarget(SDLGlobals.Renderer, screenRenderTexture);

    // I'm using this orangish color to simulate a sky texture or background color.
    // in a real game you will probably want this to be set to transparent instead, 
    // If you do want that, use
    //     SDL_SetTextureBlendMode(screenRenderTexture, SDL_BLENDMODE_BLEND);
    //     SDL_SetRenderDrawColor(SDLGlobals.Renderer, 0, 0, 0, 0);
    // instead.
    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 255, 180, 0, 255);
    SDL_RenderClear(SDLGlobals.Renderer);

    const IntVec2_t topLeftValidTileTopLeft_px = {topLeftValidTile.X * cGridSize_px, topLeftValidTile.Y * cGridSize_px};
    const IntVec2_t validTopLeftTileToRegionTopLeft = {relToMap_WindowTopLeft.X - topLeftValidTileTopLeft_px.X, relToMap_WindowTopLeft.Y - topLeftValidTileTopLeft_px.Y};

    const SDL_Rect srcRect = GetTextureReadArea(validTopLeftTileToRegionTopLeft , windowSize, intersectType, renderedRectangle);

    const IntVec2_t screenDestOrigin = GetDrawRenderOffset(srcRect, windowSize, intersectType);

    SDL_Rect destRect = {0};
    destRect.x = screenDestOrigin.X;
    destRect.y = screenDestOrigin.Y;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;

    SDL_RenderCopy(SDLGlobals.Renderer, mapRenderTexture, &srcRect, &destRect);
}

//---------------------------------------------------------------------------------------------------------------------------
// Demo main functions
//---------------------------------------------------------------------------------------------------------------------------


const InitSDLValues_t InitSDL(IntVec2_t windowSize_px)
{
    InitSDLValues_t sdlInitResult = {NULL, NULL};

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) 
    {
        return sdlInitResult;
    }

    // Init the window
    SDL_Window* window = SDL_CreateWindow("A wild map intersect test program appears!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowSize_px.X, windowSize_px.Y, SDL_WINDOW_SHOWN);
    if (!window) 
    {
        printf("An error occured while trying to create window : %s\n", SDL_GetError());
        return sdlInitResult;
    }

    // Init the renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) 
    {
        printf("An error occured while trying to create renderer : %s\n", SDL_GetError());
        return sdlInitResult;
    }

    sdlInitResult.Window = window;
    sdlInitResult.Renderer = renderer;
    return sdlInitResult;
}

// just checks for a quit signal. You can put more keypresses and mouse button handlers here if you want.
// returns 1 on quit, returns 0 otherwise
int HandleInput()
{
    SDL_Event event = {0};

    while (SDL_PollEvent(&event)) 
    {
        // E.g., from hitting the close window button
        if (event.type == SDL_QUIT) 
        {
            return 1;
        }
        else if(event.type == SDL_MOUSEMOTION)
        {
            MousePosition.X = event.motion.x;
            MousePosition.Y = event.motion.y;
        }
    }

    return 0;
}

void DrawTexture(SDL_Texture *texture, const IntVec2_t& textureSize, const IntVec2_t& screenCoord)
{
    SDL_Rect textureRectangle;
    textureRectangle.x = 0;
    textureRectangle.y = 0;
    textureRectangle.w = textureSize.X;
    textureRectangle.h = textureSize.Y;

    SDL_Rect screenRectangle;
    screenRectangle.x = screenCoord.X;
    screenRectangle.y = screenCoord.Y;

    screenRectangle.w = textureSize.X;
    screenRectangle.h = textureSize.Y;

    SDL_RenderCopy(SDLGlobals.Renderer, texture, &textureRectangle, &screenRectangle);
}



IntVec2_t DEMO_TextureWindowRegion_RelToTexture(const IntVec2_t& relToMap_WindowTopLeft)
{
    const IntVec2_t gridCoordOfWindow_TopLeft = FindGridCoordinateForPoint(relToMap_WindowTopLeft, cGridSize_px);

    // This is the northwest most tile coordinate that our region touches.
    const IntVec2_t topLeftValidTile = {max(0, gridCoordOfWindow_TopLeft.X), max(0, gridCoordOfWindow_TopLeft.Y)};

    const IntVec2_t topLeftOfNorthWestTile_RelToMap_px = {topLeftValidTile.X * cGridSize_px, topLeftValidTile.Y * cGridSize_px};

    const IntVec2_t topLeftOfTextureToRegionTopLeft = {relToMap_WindowTopLeft.X - topLeftOfNorthWestTile_RelToMap_px.X, relToMap_WindowTopLeft.Y - topLeftOfNorthWestTile_RelToMap_px.Y}; 

    return topLeftOfTextureToRegionTopLeft;
}



// draws a magenta outline around the area that we're using as a window over the map
void DEMO_DrawWindowRegion(const IntVec2_t& testWindowSize, const IntVec2_t& windowTopLeft, Color_t color = cMagenta)
{
    SDL_Rect rect = {0};
    rect.x = windowTopLeft.X;
    rect.y = windowTopLeft.Y;
    rect.w = testWindowSize.X;
    rect.h = testWindowSize.Y;

    SDL_SetRenderDrawColor(SDLGlobals.Renderer, color.R, color.G, color.B, 255);

    SDL_RenderDrawRect(SDLGlobals.Renderer, &rect);
}

// Render what a simulated window would see, if its top left corner were placed at a certain position in the map
void RenderWindow(SDL_Texture* screenRenderTexture, SDL_Texture* mapRenderTexture, SDL_Texture* tileSetTexture, const IntVec2_t& windowSize_Tiles, const IntVec2_t& windowTopLeft_px, const IntVec2_t& mapTexRenderPoint, const IntVec2_t& screenRenderPoint)
{
    
    const IntVec2_t windowSize_px = {windowSize_Tiles.X * cGridSize_px, windowSize_Tiles.Y * cGridSize_px};

    // This variable is probably only important for the sake of this demo, if this were in a real game, you would pass in windowTopLeft
    // that was already relative to the top of the map, but since this demo contains more than one render window case, we have to do this offset.
    //
    // Though maybe this would be useful outside of this demo, if you wanted to offset where the map was drawn
    const IntVec2_t relToMap_WindowTopLeft = {windowTopLeft_px.X - cMapOrigin.X, windowTopLeft_px.Y- cMapOrigin.Y };

    SDL_Rect mapTextureRenderedRectangle = RenderMapToTexture(mapRenderTexture, tileSetTexture, windowSize_Tiles, relToMap_WindowTopLeft);

    // DEMO ONLY: for the sake of visualization, render the contents of the rendered map texture to the screen, this would not be done in a real game
    {
        SDL_Rect mapRenderRect = {0};
        mapRenderRect.x = mapTexRenderPoint.X;
        mapRenderRect.y = mapTexRenderPoint.Y;
        mapRenderRect.w = cMapRenderTextureSize_px.X;
        mapRenderRect.h = cMapRenderTextureSize_px.Y;

        // Now set the render target back to the screen
        SDL_SetRenderTarget(SDLGlobals.Renderer, nullptr);
        SDL_RenderCopy(SDLGlobals.Renderer, mapRenderTexture, nullptr, &mapRenderRect);
    }

    // DEMO ONLY: draw the player's simulated screen in the render texture, this would not be done in a real game, this is just for illustrative purposes
    {
        const IntVec2_t topLeftOfTextureToRegionTopLeft = DEMO_TextureWindowRegion_RelToTexture(relToMap_WindowTopLeft);        
        const IntVec2_t windowTopLeft_InMapTexture = {mapTexRenderPoint.X + topLeftOfTextureToRegionTopLeft.X, mapTexRenderPoint.Y + topLeftOfTextureToRegionTopLeft.Y};

        const WindowIntersectType_t intersectType = GetWindowIntersectType(MapTextureSize, relToMap_WindowTopLeft, windowSize_px);

        // but don't draw the region if the region's completely outside of the map, the offset won't make any sense
        if(intersectType != WindowIntersectType_t::TotallyOut)
        {
            DEMO_DrawWindowRegion(windowSize_px, windowTopLeft_InMapTexture);
        }
    }

    CopyRenderedMapToScreen(screenRenderTexture, mapRenderTexture, relToMap_WindowTopLeft, windowSize_px, mapTextureRenderedRectangle);

    // DEMO ONLY: now copy the part of the mapRenderTexture that contains the map onto the screen (with an orangish background behind it)
    {
        SDL_SetRenderTarget(SDLGlobals.Renderer, nullptr);
        // Then copy the window texture to the screen
        SDL_Rect screenRenderRect = {0};
        screenRenderRect.x = screenRenderPoint.X;
        screenRenderRect.y = screenRenderPoint.Y;
        screenRenderRect.w = windowSize_px.X;
        screenRenderRect.h = windowSize_px.Y;

        SDL_RenderCopy(SDLGlobals.Renderer, screenRenderTexture, nullptr, &screenRenderRect);
    
    }


}

void Render(void)
{
    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 0, 40, 60, 255);
    SDL_RenderClear(SDLGlobals.Renderer);

    // Draw the whole map (would not be used in a real game)
    DrawTexture(MapTestTexture, MapTextureSize, cMapOrigin);

    // in absolute pixels from the top left of our real 1024x768 screen
    IntVec2_t northWestRegion ={416, 306};
    IntVec2_t northRegion = {480, 306};
    IntVec2_t northEastRegion = {533, 316};
    IntVec2_t eastRegion = {532, 370};
    IntVec2_t southEastRegion = {534, 426};
    IntVec2_t southRegion = {476, 427};
    IntVec2_t southWestRegion = {426, 420};
    IntVec2_t westRegion = {413, 361};

    IntVec2_t allInRegion = {482, 356};
    IntVec2_t allOutRegion = {364, 308};

    // Draw our simulated window regions
    DEMO_DrawWindowRegion(cWindowSize_px, northWestRegion);
    DEMO_DrawWindowRegion(cWindowSize_px, northRegion);
    DEMO_DrawWindowRegion(cWindowSize_px, northEastRegion);
    DEMO_DrawWindowRegion(cWindowSize_px, eastRegion);

    DEMO_DrawWindowRegion(cWindowSize_px, southEastRegion);
    DEMO_DrawWindowRegion(cWindowSize_px, southRegion);
    DEMO_DrawWindowRegion(cWindowSize_px, southWestRegion);
    DEMO_DrawWindowRegion(cWindowSize_px, westRegion);

    DEMO_DrawWindowRegion(cWindowSize_px, allInRegion);
    DEMO_DrawWindowRegion(cWindowSize_px, allOutRegion);
    DEMO_DrawWindowRegion(cWindowSize_px, MousePosition); // moveable region

    // Draw what these windows would see
    // note: I had trouble getting the exact coordinates of the upper left hand corners of these regions, may be off by +/- 1 px from what's in layout.xcf

    //          screen texture (orange)             map render texture (cyan)       tileset         window size        region position     map texture render position     screen texture render position
    RenderWindow(ScreenRenderTextures.NorthWest,    MapRenderTextures.NorthWest,    MapTestTexture, cWindowSize_Tiles, northWestRegion,    {356, 244},                     {301, 192});
    RenderWindow(ScreenRenderTextures.North,        MapRenderTextures.North,        MapTestTexture, cWindowSize_Tiles, northRegion,        {476, 245},                     {474, 170});
    RenderWindow(ScreenRenderTextures.NorthEast,    MapRenderTextures.NorthEast,    MapTestTexture, cWindowSize_Tiles, northEastRegion,    {580, 265},                     {649, 208});
    RenderWindow(ScreenRenderTextures.East,         MapRenderTextures.East,         MapTestTexture, cWindowSize_Tiles, eastRegion,         {606, 359},                     {686, 357});

    RenderWindow(ScreenRenderTextures.SouthEast,    MapRenderTextures.SouthEast,    MapTestTexture, cWindowSize_Tiles, southEastRegion,    {595, 481},                     {651, 537});
    RenderWindow(ScreenRenderTextures.South,        MapRenderTextures.South,        MapTestTexture, cWindowSize_Tiles, southRegion,        {468, 491},                     {469, 592});
    RenderWindow(ScreenRenderTextures.SouthWest,    MapRenderTextures.SouthWest,    MapTestTexture, cWindowSize_Tiles, southWestRegion,    {361, 464},                     {316, 525});
    RenderWindow(ScreenRenderTextures.West,         MapRenderTextures.West,         MapTestTexture, cWindowSize_Tiles, westRegion,         {323, 358},                     {271, 410});

    RenderWindow(ScreenRenderTextures.AllIn,        MapRenderTextures.AllIn,        MapTestTexture, cWindowSize_Tiles, allInRegion,        {164, 278},                     {82, 294});
    RenderWindow(ScreenRenderTextures.AllOut,       MapRenderTextures.AllOut,       MapTestTexture, cWindowSize_Tiles, allOutRegion,       {164, 337},                     {81, 334});

    RenderWindow(ScreenRenderTextures.AllOut,       MapRenderTextures.AllOut,       MapTestTexture, cWindowSize_Tiles, MousePosition,      {770, 255},                     {777, 323});

    SDL_RenderPresent(SDLGlobals.Renderer);
}

void FrameDelay(unsigned int targetTicks)
{
    // Block at 60 fps

    // ticks is in ms
    unsigned int ticks = SDL_GetTicks();

    if (targetTicks < ticks) 
    {
        return;
    }

    if (targetTicks > ticks + cFrameDuration_ms) 
    {
        SDL_Delay(cFrameDuration_ms);
    } 
    else 
    {
        SDL_Delay(targetTicks - ticks);
    }
}


// convenience functions to reduce typing and typos
static SDL_Texture* AllocateTexture(const IntVec2_t& size)
{
    return SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, size.X, size.Y);
}

static TestTextures_t AllocateTestTextures(const IntVec2_t& size)
{
    TestTextures_t textures = {0};

    textures.NorthWest  = AllocateTexture(size);
    textures.North      = AllocateTexture(size);
    textures.NorthEast  = AllocateTexture(size);
    textures.East       = AllocateTexture(size);

    textures.SouthEast  = AllocateTexture(size);
    textures.South      = AllocateTexture(size);
    textures.SouthWest  = AllocateTexture(size);
    textures.West       = AllocateTexture(size);

    textures.AllIn      = AllocateTexture(size);
    textures.AllOut     = AllocateTexture(size);
    textures.Moveable   = AllocateTexture(size);

    return textures;
}

static void FreeTextures(TestTextures_t& textures)
{
    SDL_DestroyTexture(textures.NorthWest);
    SDL_DestroyTexture(textures.North);
    SDL_DestroyTexture(textures.NorthEast);
    SDL_DestroyTexture(textures.East);

    SDL_DestroyTexture(textures.SouthEast);
    SDL_DestroyTexture(textures.South);
    SDL_DestroyTexture(textures.SouthWest);
    SDL_DestroyTexture(textures.West);

    SDL_DestroyTexture(textures.AllIn);
    SDL_DestroyTexture(textures.AllOut);
    SDL_DestroyTexture(textures.Moveable);

    textures = {0};
}


void GameRenderLoop()
{
    // initialization
    SDLGlobals = InitSDL(cScreenResolution);

    MapTestTexture = LoadImage(SDLGlobals.Renderer, "Debug16.png");
    MapTextureSize = InquireTextureSize(MapTestTexture);

    ScreenRenderTextures    = AllocateTestTextures(cWindowSize_px);
    MapRenderTextures       = AllocateTestTextures(cMapRenderTextureSize_px);

    // main loop
    unsigned int targetTicks = SDL_GetTicks() + cFrameDuration_ms;
    while(1)
    {
        int quitSignal = HandleInput();

        if(quitSignal)
        {
            break;
        }

        Render();
        FrameDelay(targetTicks);
        targetTicks = SDL_GetTicks() + cFrameDuration_ms;
    }

    FreeTextures(ScreenRenderTextures);
    FreeTextures(MapRenderTextures);

}

void DoBasicTests()
{
    constexpr IntVec2_t cWindowSize_px = {50, 50};
    constexpr IntVec2_t cMapSize_Tiles = {100, 100};

    constexpr IntVec2_t allInPoint = {20, 20};
    constexpr IntVec2_t allOutPoint = {200, 200};

    // these points are all the coordinate of the top left point of the window,
    // relative to the top left point of the map
    constexpr IntVec2_t northWestPoint  = {-20, -20};
    constexpr IntVec2_t northPoint      = {20, -20};
    constexpr IntVec2_t northEastPoint  = {80, -20};
    constexpr IntVec2_t eastPoint       = {80, 20};

    constexpr IntVec2_t southEastPoint  = {80, 80};
    constexpr IntVec2_t southPoint      = {25, 80};
    constexpr IntVec2_t southWestPoint  = {-20, 80};
    constexpr IntVec2_t westPoint       = {-20, 25};

    SDL_Rect northWestArea = GetMapRenderRectangle(cMapSize_Tiles, northWestPoint, cWindowSize_px);
    SDL_Rect northArea = GetMapRenderRectangle(cMapSize_Tiles, northPoint, cWindowSize_px);
    SDL_Rect northEastArea = GetMapRenderRectangle(cMapSize_Tiles, northEastPoint, cWindowSize_px);
    SDL_Rect eastArea = GetMapRenderRectangle(cMapSize_Tiles, eastPoint, cWindowSize_px);

    SDL_Rect southEastArea = GetMapRenderRectangle(cMapSize_Tiles, southEastPoint, cWindowSize_px);
    SDL_Rect southArea = GetMapRenderRectangle(cMapSize_Tiles, southPoint, cWindowSize_px);
    SDL_Rect southWestArea = GetMapRenderRectangle(cMapSize_Tiles, southWestPoint, cWindowSize_px);
    SDL_Rect westArea = GetMapRenderRectangle(cMapSize_Tiles, westPoint, cWindowSize_px);

    /*
        (gdb) call northWestArea
        $1 = {x = 0, y = 0, w = 30, h = 30}
        (gdb) call northArea
        $2 = {x = 20, y = 0, w = 50, h = 30}
        (gdb) call northEastArea
        $3 = {x = 80, y = 0, w = 20, h = 30}
        (gdb) call eastArea
        $4 = {x = 80, y = 20, w = 20, h = 50}
        (gdb) call southEastArea
        $5 = {x = 80, y = 80, w = 20, h = 20}
        (gdb) call southArea
        $6 = {x = 25, y = 80, w = 50, h = 20}
        (gdb) call southWestArea
        $7 = {x = 0, y = 80, w = 30, h = 20}
        (gdb) call westArea
        $8 = {x = 0, y = 25, w = 30, h = 50}
    */

   
}

int main()
{
    // For testing whether the core functions are working properly
    DoBasicTests();

    GameRenderLoop();


    return 0;
}