#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
  auto fail(const std::string &operation) -> void
  {
    throw std::runtime_error(operation + ": " + SDL_GetError());
  }

  auto fill(SDL_Surface *surface, const SDL_Rect &rectangle,
            const std::uint8_t red, const std::uint8_t green,
            const std::uint8_t blue) -> void
  {
    if (!SDL_FillSurfaceRect(surface, &rectangle,
                             SDL_MapSurfaceRGB(surface, red, green, blue)))
      fail("Could not fill SDL window surface");
  }

  auto auto_close_milliseconds() -> std::uint64_t
  {
    const char *raw = std::getenv("SAGAN_RENDER_AUTOCLOSE_MS");
    if (!raw || *raw == '\0') return 1200;
    char *end{};
    const unsigned long value = std::strtoul(raw, &end, 10);
    if (*end != '\0' || value < 100 || value > 60000)
      throw std::runtime_error("SAGAN_RENDER_AUTOCLOSE_MS must be between 100 and 60000");
    return value;
  }
}

int main()
{
  try
  {
    if (!SDL_Init(SDL_INIT_VIDEO)) fail("Could not initialize SDL video");

    SDL_Window *window = SDL_CreateWindow(
      "Sagan Render SDL3 Foundation", 960, 540,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) fail("Could not create SDL window");

    int logical_width{};
    int logical_height{};
    int drawable_width{};
    int drawable_height{};
    if (!SDL_GetWindowSize(window, &logical_width, &logical_height) ||
        !SDL_GetWindowSizeInPixels(window, &drawable_width, &drawable_height))
      fail("Could not read SDL window dimensions");

    SDL_Surface *surface = SDL_GetWindowSurface(window);
    if (!surface) fail("Could not acquire SDL window surface");

    const SDL_Rect frame{0, 0, surface->w, surface->h};
    fill(surface, frame, 10, 18, 34);
    const int margin = std::max(18, surface->w / 24);
    const SDL_Rect header{margin, margin, surface->w - margin * 2,
                          std::max(56, surface->h / 6)};
    fill(surface, header, 34, 211, 238);
    const SDL_Rect viewport{margin, header.y + header.h + margin,
                            surface->w - margin * 2,
                            surface->h - header.h - margin * 3};
    fill(surface, viewport, 30, 41, 59);
    const SDL_Rect marker{viewport.x + viewport.w / 2 - 24,
                          viewport.y + viewport.h / 2 - 24, 48, 48};
    fill(surface, marker, 251, 191, 36);

    if (!SDL_UpdateWindowSurface(window)) fail("Could not present SDL window surface");

    const char *capture_value = std::getenv("SAGAN_RENDER_CAPTURE_BMP");
    const std::string capture = capture_value && *capture_value
                                  ? capture_value : "build/sdl-window/sdl-window.bmp";
    if (!SDL_SaveBMP(surface, capture.c_str())) fail("Could not capture SDL window surface");

    std::cout << "SDL_WINDOW_PROBE opened=1 logical=" << logical_width << 'x'
              << logical_height << " drawable=" << drawable_width << 'x'
              << drawable_height << " scale=" << SDL_GetWindowDisplayScale(window)
              << " focused="
              << ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) ? 1 : 0)
              << " capture=" << capture << '\n';

    const std::uint64_t deadline = SDL_GetTicks() + auto_close_milliseconds();
    bool running = true;
    while (running && SDL_GetTicks() < deadline)
    {
      SDL_Event event{};
      while (SDL_PollEvent(&event))
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
          running = false;
      SDL_Delay(8);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "SDL_WINDOW_PROBE closed=1 cleanup=1\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    SDL_Quit();
    return 1;
  }
}
