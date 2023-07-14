#include "sdl_wrapper.h"
#include "state.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <body.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

const char WINDOW_TITLE[] = "Fruit Chef";
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;
const vector_t S_SIZE = {1000.0, 500.0};
const double MS_PER_S = 1e3;
const vector_t WIDTH_HEIGHT = {1000.0, 500.0};
const char *LEVEL_1_IMAGE_PATH = "assets/level1.png";
const char *LEVEL_2_IMAGE_PATH = "assets/level2.png";
const char *LEVEL_3_IMAGE_PATH = "assets/level3.png";
const char *INTRO_IMAGE_PATH = "assets/intro.png";
const char *WIN_SCREEN = "assets/win_screen.jpeg";
const char *LOSE_SCREEN = "assets/lose_screen.jpeg";
SDL_Color WHITE = {255, 255, 255};
SDL_Color BLACK = {0, 0, 0};
const double TEXT_OFFSET = 0.1;

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;

/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
  int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
  assert(width != NULL);
  assert(height != NULL);
  SDL_GetWindowSize(window, width, height);
  vector_t dimensions = {.x = *width, .y = *height};
  free(width);
  free(height);
  return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
  // Scale scene so it fits entirely in the window
  double x_scale = window_center.x / max_diff.x,
         y_scale = window_center.y / max_diff.y;
  return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
  // Scale scene coordinates by the scaling factor
  // and map the center of the scene to the center of the window
  vector_t scene_center_offset = vec_subtract(scene_pos, center);
  double scale = get_scene_scale(window_center);
  vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
  vector_t pixel = {.x = round(window_center.x + pixel_center_offset.x),
                    // Flip y axis since positive y is down on the screen
                    .y = round(window_center.y - pixel_center_offset.y)};
  return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
  switch (key) {
  case SDLK_LEFT:
    return LEFT_ARROW;
  case SDLK_UP:
    return UP_ARROW;
  case SDLK_RIGHT:
    return RIGHT_ARROW;
  case SDLK_DOWN:
    return DOWN_ARROW;
  case SDLK_SPACE:
    return SPACE;
  default:
    // Only process 7-bit ASCII characters
    return key == (SDL_Keycode)(char)key ? key : '\0';
  }
}

void sdl_init(vector_t min, vector_t max) {
  // Check parameters
  assert(min.x < max.x);
  assert(min.y < max.y);

  center = vec_multiply(0.5, vec_add(min, max));
  max_diff = vec_subtract(max, center);
  SDL_Init(SDL_INIT_EVERYTHING);
  window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  TTF_Init();
}

bool sdl_is_done(state_t *state) {
  SDL_Event *event = malloc(sizeof(*event));
  assert(event != NULL);
  while (SDL_PollEvent(event)) {
    switch (event->type) {
    case SDL_QUIT:
      free(event);
      return true;
    case SDL_MOUSEMOTION:
      if (key_handler == NULL) {
        break;
      }
      uint32_t timestamp = event->key.timestamp;
      key_event_type_t type_mouse =
          event->type == SDL_MOUSEBUTTONDOWN ? KEY_PRESSED : KEY_RELEASED;
      type_mouse = event->type == SDL_MOUSEMOTION ? MOUSE_ENGAGED : type_mouse;

      vector_t loc = (vector_t){event->motion.x, event->motion.y};
      char key = MOUSE_MOVED;

      double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(key, type_mouse, held_time, state, loc);
      break;
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
      if (key_handler == NULL) {
        break;
      }

      // SDL_ShowCursor(false); hides cursor

      type_mouse =
          event->type == SDL_MOUSEBUTTONDOWN ? KEY_PRESSED : KEY_RELEASED;
      type_mouse = event->type == SDL_MOUSEMOTION ? MOUSE_ENGAGED : type_mouse;

      loc = (vector_t){event->motion.x, event->motion.y};
      key =
          event->type == SDL_MOUSEBUTTONDOWN ? MOUSEBUTTONDOWN : MOUSEBUTTONUP;

      held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(key, type_mouse, held_time, state, loc);
      break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // Skip the keypress if no handler is configured
      // or an unrecognized key was pressed
      if (key_handler == NULL)
        break;
      key = get_keycode(event->key.keysym.sym);
      if (key == '\0')
        break;

      timestamp = event->key.timestamp;
      if (!event->key.repeat) {
        key_start_timestamp = timestamp;
      }
      key_event_type_t type =
          event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
      held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(key, type, held_time, state, VEC_ZERO);
      break;
    }
  }
  free(event);
  return false;
}

void sdl_clear(void) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
  // Check parameters
  size_t n = list_size(points);
  assert(n >= 3);
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);
  assert(0 <= color.a && color.a <= 1);

  // if only a 3-value rgb value is given, automatically assume a to be 1.
  if (color.a == 0) {
    color.a = 1;
  }

  vector_t window_center = get_window_center();

  // Convert each vertex to a point on screen
  int16_t *x_points = malloc(sizeof(*x_points) * n),
          *y_points = malloc(sizeof(*y_points) * n);
  assert(x_points != NULL);
  assert(y_points != NULL);
  for (size_t i = 0; i < n; i++) {
    vector_t *vertex = list_get(points, i);
    vector_t pixel = get_window_position(*vertex, window_center);
    x_points[i] = pixel.x;
    y_points[i] = pixel.y;
  }

  // Draw polygon with the given color
  filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255,
                    color.g * 255, color.b * 255, color.a * 255);
  free(x_points);
  free(y_points);
}

void sdl_show(void) {
  // Draw boundary lines
  vector_t window_center = get_window_center();
  vector_t max = vec_add(center, max_diff),
           min = vec_subtract(center, max_diff);
  vector_t max_pixel = get_window_position(max, window_center),
           min_pixel = get_window_position(min, window_center);
  SDL_Rect *boundary = malloc(sizeof(*boundary));
  boundary->x = min_pixel.x;
  boundary->y = max_pixel.y;
  boundary->w = max_pixel.x - min_pixel.x;
  boundary->h = min_pixel.y - max_pixel.y;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, boundary);
  free(boundary);

  SDL_RenderPresent(renderer);
}

void render_image(vector_t origin, vector_t rotation_center,
                  const char *image_path, double angle) {
  SDL_Texture *img = IMG_LoadTexture(renderer, image_path);
  int32_t w, h;
  SDL_QueryTexture(img, NULL, NULL, &w, &h);
  SDL_Rect texr;
  SDL_Point center = {rotation_center.x, rotation_center.y};
  texr.x = origin.x;
  texr.y = origin.y;
  texr.w = w;
  texr.h = h;
  SDL_RendererFlip flip = SDL_FLIP_NONE;
  SDL_RenderCopyEx(renderer, img, NULL, &texr, angle, &center, flip);
  SDL_RenderPresent(renderer);
}

// remove all these magic numbers
void render_background_image(const char *image_path) {
  SDL_Texture *img = IMG_LoadTexture(renderer, image_path);
  int w, h;
  SDL_QueryTexture(img, NULL, NULL, &w, &h);
  SDL_Rect texr;
  texr.x = 0;
  texr.y = 0;
  texr.w = WIDTH_HEIGHT.x;
  texr.h = WIDTH_HEIGHT.y;
  SDL_RenderCopy(renderer, img, NULL, &texr);
  SDL_RenderPresent(renderer);
}

// MAGIC NUMBERS
void sdl_render_text(scene_t *scene, text_t *text, double time, size_t points,
                     size_t level) {
  TTF_Font *font = text_get_font(text);

  int trunc_time = trunc(time);
  char str_time[100];
  snprintf(str_time, 100, "Time: %d", trunc_time);

  SDL_Color text_color;
  if (level < 3) {
    text_color = WHITE;
  } else {
    text_color = BLACK;
  }
  SDL_Surface *Surface_Message_Time =
      TTF_RenderText_Solid(font, str_time, text_color);
  SDL_Texture *Message_Time =
      SDL_CreateTextureFromSurface(renderer, Surface_Message_Time);

  SDL_Rect Message_rect; // create a rect
  Message_rect.x =
      Surface_Message_Time->w * TEXT_OFFSET; // controls the rect's x coordinate
  Message_rect.y = 0;                        // controls the rect's y coordinte
  Message_rect.w = Surface_Message_Time->w;  // controls the width of the rect
  Message_rect.h = Surface_Message_Time->h;  // controls the height of the rect

  SDL_RenderCopy(renderer, Message_Time, NULL, &Message_rect);

  char str_points[100];
  snprintf(str_points, 100, "Points: %zu", points);

  SDL_Surface *Surface_Message_Points =
      TTF_RenderText_Solid(font, str_points, text_color);
  SDL_Texture *Message_Points =
      SDL_CreateTextureFromSurface(renderer, Surface_Message_Points);

  Message_rect.x = (S_SIZE.x / 2) - (Surface_Message_Points->w /
                                     2); // controls the rect's x coordinate
  Message_rect.y = 0;                    // controls the rect's y coordinte
  Message_rect.w = Surface_Message_Points->w; // controls the width of the rect
  Message_rect.h = Surface_Message_Points->h; // controls the height of the rect

  SDL_RenderCopy(renderer, Message_Points, NULL, &Message_rect);

  char str_level[100];
  snprintf(str_level, 100, "Level: %zu", level);

  SDL_Surface *Surface_Message_Level =
      TTF_RenderText_Solid(font, str_level, text_color);
  SDL_Texture *Message_Level =
      SDL_CreateTextureFromSurface(renderer, Surface_Message_Level);

  Message_rect.x =
      S_SIZE.x - Surface_Message_Level->w *
                     (1.0 + TEXT_OFFSET);    // controls the rect's x coordinate
  Message_rect.y = 0;                        // controls the rect's y coordinte
  Message_rect.w = Surface_Message_Level->w; // controls the width of the rect
  Message_rect.h = Surface_Message_Level->h; // controls the height of the rect

  SDL_RenderCopy(renderer, Message_Level, NULL, &Message_rect);
  SDL_RenderPresent(renderer);

  SDL_FreeSurface(Surface_Message_Time);
  SDL_FreeSurface(Surface_Message_Points);
  SDL_FreeSurface(Surface_Message_Level);
  SDL_DestroyTexture(Message_Time);
  SDL_DestroyTexture(Message_Points);
  SDL_DestroyTexture(Message_Level);
}

void sdl_render_scene(scene_t *scene, vector_t screen_size, bool intro,
                      bool win, bool lose, size_t level) {
  sdl_clear();
  if (intro) {
    render_background_image(INTRO_IMAGE_PATH);
  } else if (win) {
    render_background_image(WIN_SCREEN);
  } else if (lose) {
    render_background_image(LOSE_SCREEN);
  } else {
    if (level == 1) {
      render_background_image(LEVEL_1_IMAGE_PATH);
    } else if (level == 2) {
      render_background_image(LEVEL_2_IMAGE_PATH);
    } else {
      render_background_image(LEVEL_3_IMAGE_PATH);
    }
    size_t body_count = scene_bodies(scene);
    body_t *player = NULL;
    for (size_t i = 0; i < body_count; i++) {
      body_t *body = scene_get_body(scene, i);

      list_t *shape = body_get_shape(body);
      // draws the colors for bodies that don't have images
      if (get_type(body) == PLAYER) {
        player = body;
        list_free(shape);
        continue;
      }

      const char *image_path = body_get_image_path(body);
      if (image_path == NULL) {
        continue;
      }
      vector_t centroid = body_get_centroid(body);
      vector_t origin = centroid;
      double radius = body_get_radius(body);
      vector_t rotation_center = (vector_t){radius, radius};

      // convert counterclockwise to clockwise
      double angle = 2 * M_PI - (body_get_angle(body));
      // double angle = 0;
      switch (get_type(body)) {
      case BOMB:
      case ORANGE:
      case GOLDEN_APPLE:
      case WATERMELON:
      case PEACH:
      case POMEGRANATE:
      case EXPLOSION:
      case POWERUP:
      case APPLE:
        origin.y += radius;
        break;
      case SLICE:
        origin = vec_multiply(
            0.5, vec_add(*(vector_t *)list_get(shape, 0),
                         *(vector_t *)list_get(shape, list_size(shape) - 1)));
        rotation_center.y = 0;
        angle += M_PI;
        break;
      default:
        break;
      }
      origin.x -= radius;
      angle = angle * 180 / M_PI;
      // flip the y to render properly
      origin.y = screen_size.y - origin.y;
      render_image(origin, rotation_center, image_path, angle);
      list_free(shape);
    }

    // need to draw cursor afterward or else it gets overwritten
    if (player != NULL) {
      list_t *player_vertices = body_get_shape(player);
      sdl_draw_polygon(player_vertices, body_get_color(player));
      list_free(player_vertices);
    }
  }
  sdl_show();
}

void sdl_on_key(key_handler_t handler) { key_handler = handler; }

double time_since_last_tick(void) {
  clock_t now = clock();
  double difference = last_clock
                          ? (double)(now - last_clock) / CLOCKS_PER_SEC
                          : 0.0; // return 0 the first time this is called
  last_clock = now;
  return difference;
}

list_t *create_star(size_t num_star_points, double outer_radius,
                    double inner_radius) {
  size_t num_vertices = num_star_points * 2;
  list_t *star = list_init(num_vertices, free);
  double angle = 0.0;
  for (size_t i = 0; i < num_vertices; i++) {
    double radius;
    if (i % 2 == 0) {
      radius = outer_radius;
    } else {
      radius = inner_radius;
    }
    vector_t *vertex = malloc(sizeof(vector_t));
    vertex->x = cos(angle) * radius;
    vertex->y = sin(angle) * radius;
    list_add(star, vertex);
    angle += 2.0 * M_PI / num_vertices;
  }
  return star;
}
