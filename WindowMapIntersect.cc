#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
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
};

// Constants
//---------------------------------------------------------------------------------------------------

// change this to change the size of the window
const IntVec2_t cScreenResolution = {1024, 768};

// this is more FYI than anything
const int cFPS = 60;

// 1/60 is 0.016666666666666666
const int cFrameDuration_ms = 16;

// imaginary, simulated window looking at our map
const IntVec2_t cWindowSize = {32, 32};

const IntVec2_t cMapOrigin = {432, 322};

// Globals
//---------------------------------------------------------------------------------------------------

// various SDL resources required for rendering.
InitSDLValues_t SDLGlobals;

SDL_Texture* MapTestTexture = nullptr;

IntVec2_t MapTextureSize;

TestTextures_t TestTextures;

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

// You really need to see this drawn out on a sheet of paper to best understand this to be honest
// see scanned png hand written pages in this folder.
//
// Returns a value that represents the window's positioning; This value tells you if the window hangs off the corner of the map, the sides, or neither.
WindowIntersectType_t GetWindowIntersectType(const IntVec2_t& mapSize, const IntVec2_t& windowNorthWestCorner, const IntVec2_t& windowSize_px)
{
    const IntVec2_t windowNorthEastCorner = {windowNorthWestCorner.X + windowSize_px.X, windowNorthWestCorner.Y + 0};
    const IntVec2_t windowSouthWestCorner = {windowNorthWestCorner.X + 0,               windowNorthWestCorner.Y + windowSize_px.Y};
    const IntVec2_t windowSouthEastCorner = {windowNorthWestCorner.X + windowSize_px.X, windowNorthWestCorner.Y + windowSize_px.Y};

    const IntVec2_t mapTopLeft = {0, 0};

    const bool northWestCornerIn = PointInRect(windowNorthWestCorner, mapTopLeft, mapSize);
    const bool northEastCornerIn = PointInRect(windowNorthEastCorner, mapTopLeft, mapSize);
    const bool southWestCornerIn = PointInRect(windowSouthWestCorner, mapTopLeft, mapSize);
    const bool southEastCornerIn = PointInRect(windowSouthEastCorner, mapTopLeft, mapSize);
    

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

void DoBasicTests()
{
    constexpr IntVec2_t cWindowSize = {50, 50};
    constexpr IntVec2_t cMapSize = {100, 100};

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

    SDL_Rect northWestArea = GetMapRenderRectangle(cMapSize, northWestPoint, cWindowSize);
    SDL_Rect northArea = GetMapRenderRectangle(cMapSize, northPoint, cWindowSize);
    SDL_Rect northEastArea = GetMapRenderRectangle(cMapSize, northEastPoint, cWindowSize);
    SDL_Rect eastArea = GetMapRenderRectangle(cMapSize, eastPoint, cWindowSize);

    SDL_Rect southEastArea = GetMapRenderRectangle(cMapSize, southEastPoint, cWindowSize);
    SDL_Rect southArea = GetMapRenderRectangle(cMapSize, southPoint, cWindowSize);
    SDL_Rect southWestArea = GetMapRenderRectangle(cMapSize, southWestPoint, cWindowSize);
    SDL_Rect westArea = GetMapRenderRectangle(cMapSize, westPoint, cWindowSize);

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

// draws a magenta outline around the area that we're using as a window over the map
void DrawWindowRegion(const IntVec2_t& testWindowSize, const IntVec2_t& windowTopLeft)
{
    SDL_Rect rect = {0};
    rect.x = windowTopLeft.X;
    rect.y = windowTopLeft.Y;
    rect.w = testWindowSize.X;
    rect.h = testWindowSize.Y;

    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 255, 0,255, 255);

    SDL_RenderDrawRect(SDLGlobals.Renderer, &rect);
}

// See scanned png hand written pages in this folder
IntVec2_t GetDrawRenderOffset(const IntVec2_t& windowTopLeft, const WindowIntersectType_t& intersectType)
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
            return {-windowTopLeft.X, -windowTopLeft.Y};
        }

        case WindowIntersectType_t::North:
        case WindowIntersectType_t::NorthEast:
        {
            return {0, -windowTopLeft.Y};
        }

        case WindowIntersectType_t::East:
        case WindowIntersectType_t::SouthEast:
        case WindowIntersectType_t::South:
        {
            return {0, 0};
        }

        case WindowIntersectType_t::SouthWest:
        case WindowIntersectType_t::West:
        {
            return {-windowTopLeft.X, 0};
        }

        default:
        {
            assert(0);
            throw std::logic_error("Impossible intersection type");
        }
        
    }
}

// Render what a simulated window would see, if its top left corner were placed at a certain position in the map
void RenderWindow(SDL_Texture* renderToTexture, const IntVec2_t& testWindowSize, const IntVec2_t& windowTopLeft, const IntVec2_t& screenRenderPoint)
{
    const IntVec2_t relToMap_WindowTopLeft = {windowTopLeft.X - cMapOrigin.X, windowTopLeft.Y- cMapOrigin.Y };
    const SDL_Rect textureRect = GetMapRenderRectangle(MapTextureSize, relToMap_WindowTopLeft, testWindowSize);

    SDL_SetRenderTarget(SDLGlobals.Renderer, renderToTexture);

    // I'm using this orangish color to simulate a sky texture or background color.
    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 255, 180, 0, 255);
    SDL_RenderClear(SDLGlobals.Renderer);

    const WindowIntersectType_t intersectType = GetWindowIntersectType(MapTextureSize, relToMap_WindowTopLeft, testWindowSize);

    if(WindowIntersectType_t::TotallyOut != intersectType)
    {
        const IntVec2_t renderDrawOffset = GetDrawRenderOffset(relToMap_WindowTopLeft, intersectType);

        SDL_Rect destRect = {0};
        destRect.x = renderDrawOffset.X;
        destRect.y = renderDrawOffset.Y;
        destRect.w = textureRect.w;
        destRect.h = textureRect.h;

        SDL_RenderCopy(SDLGlobals.Renderer, MapTestTexture, &textureRect, &destRect);
    }

    // Now set the render target back to the screen
    SDL_SetRenderTarget(SDLGlobals.Renderer, nullptr);

    // Then copy the texture to the screen
    SDL_Rect screenRenderRect = {0};
    screenRenderRect.x = screenRenderPoint.X;
    screenRenderRect.y = screenRenderPoint.Y;
    screenRenderRect.w = testWindowSize.X;
    screenRenderRect.h = testWindowSize.Y;


    SDL_RenderCopy(SDLGlobals.Renderer, renderToTexture, nullptr, &screenRenderRect);
}

void Render(void)
{
    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 0, 40, 60, 255);
    SDL_RenderClear(SDLGlobals.Renderer);

    // Draw the "map"
    DrawTexture(MapTestTexture, MapTextureSize, cMapOrigin);

    // in absolute pixels from the top left of our real 1024x768 screen
    IntVec2_t northWestRegion ={416, 306};
    IntVec2_t northRegion = {480, 306};
    IntVec2_t northEastRegion = {533, 316};
    IntVec2_t eastRegion = {532, 370};
    IntVec2_t southEastRegion = {555, 445};
    IntVec2_t southRegion = {476, 446};
    IntVec2_t southWestRegion = {426, 420};
    IntVec2_t westRegion = {413, 361};

    IntVec2_t allInRegion = {482, 356};
    IntVec2_t allOutRegion = {364, 308};

    // Draw our simulated window regions
    DrawWindowRegion(cWindowSize, northWestRegion);
    DrawWindowRegion(cWindowSize, northRegion);
    DrawWindowRegion(cWindowSize, northEastRegion);
    DrawWindowRegion(cWindowSize, eastRegion);

    DrawWindowRegion(cWindowSize, southEastRegion);
    DrawWindowRegion(cWindowSize, southRegion);
    DrawWindowRegion(cWindowSize, southWestRegion);
    DrawWindowRegion(cWindowSize, westRegion);

    DrawWindowRegion(cWindowSize, allInRegion);
    DrawWindowRegion(cWindowSize, allOutRegion);

    // Draw what these windows would see
    RenderWindow(TestTextures.NorthWest,    cWindowSize, northWestRegion,   {356, 243});
    RenderWindow(TestTextures.North,        cWindowSize, northRegion,       {475, 243});
    RenderWindow(TestTextures.NorthEast,    cWindowSize, northEastRegion,   {579, 263});
    RenderWindow(TestTextures.East,         cWindowSize, eastRegion,        {606, 358});

    RenderWindow(TestTextures.SouthEast,    cWindowSize, southEastRegion,   {595, 480});
    RenderWindow(TestTextures.South,        cWindowSize, southRegion,       {468, 490});
    RenderWindow(TestTextures.SouthWest,    cWindowSize, southWestRegion,   {361, 463});
    RenderWindow(TestTextures.West,         cWindowSize, westRegion,        {323, 356});

    RenderWindow(TestTextures.AllIn,        cWindowSize, allInRegion,       {253, 300});
    RenderWindow(TestTextures.AllOut,       cWindowSize, allOutRegion,      {253, 343});

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

void GameRenderLoop()
{
    // initialization
    SDLGlobals = InitSDL(cScreenResolution);

    MapTestTexture = LoadImage(SDLGlobals.Renderer, "Debug16.png");
    MapTextureSize = InquireTextureSize(MapTestTexture);

    TestTextures.NorthWest =  SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);
    TestTextures.North = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);
    TestTextures.NorthEast = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);
    TestTextures.East = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);

    TestTextures.SouthEast = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);;
    TestTextures.South = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);
    TestTextures.SouthWest = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);
    TestTextures.West = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);

    TestTextures.AllIn = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);
    TestTextures.AllOut = SDL_CreateTexture(SDLGlobals.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cWindowSize.X, cWindowSize.Y);


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

    SDL_DestroyTexture(TestTextures.NorthWest);
    SDL_DestroyTexture(TestTextures.North);
    SDL_DestroyTexture(TestTextures.NorthEast);
    SDL_DestroyTexture(TestTextures.East);

    SDL_DestroyTexture(TestTextures.SouthEast);
    SDL_DestroyTexture(TestTextures.South);
    SDL_DestroyTexture(TestTextures.SouthWest);
    SDL_DestroyTexture(TestTextures.West);

    SDL_DestroyTexture(TestTextures.AllIn);
    SDL_DestroyTexture(TestTextures.AllOut);

    TestTextures = {0};

}

int main()
{
    // For testing whether the core functions are working properly
    DoBasicTests();

    GameRenderLoop();


    return 0;
}