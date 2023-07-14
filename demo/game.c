#include "forces.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "text.h"
#include "vector.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CIRCLE_POINTS 40

#define G 1.67E-9
#define M 6E24
#define g 9.8
#define R 6.39E6

// screen
const vector_t SCREEN_SIZE = {1000.0, 500.0};

// fruit
const rgb_color_t DEFAULT_COLOR = (rgb_color_t){0, 0, 0};
static const double FRUIT_MASS = 10.0;
const double FRUIT_THROW_RATE = 3.7;
const double FRUIT_RADIUS = 40;
const double DOUBLE_FRUIT_RATE = 15.0;
const double FRENZY_FRUIT_THROW_RATE = 0.5;
const double FRENZY_DOUBLE_FRUIT_RATE = 0.75;
static const double INITIAL_Y_VELOCITY = 450.0;
const int32_t MAX_ANGULAR_VEL = 180;
const int32_t MAX_X_VELOCITY = 150;

// cursor
const size_t CURSOR_TICK_DELAY = 20;
const rgb_color_t CURSOR_COLOR = (rgb_color_t){0, 0, 0};
const double CURSOR_RADIUS = 10;

// general
const double DEFAULT_MASS = 1;
const double COUNTDOWN_TIMER = 60.0;
const size_t LEVEL_1 = 15;
const size_t LEVEL_2 = 20;
const size_t LEVEL_3 = 30;
const double FRENZY_TIME_LIMIT = 3.0;
const double MIN_Y_POSITION = 5.0;

// bomb
const rgb_color_t GRAY = (rgb_color_t){.5, .5, .5};
static const double BOMB_MASS = 10.0;
const double BOMB_RADIUS = 40;
const double EXPLOSION_RADIUS = 80;
const double BOMB_THROW_RATE = 6.75;
const double BOMB_THROW_RATE_LEVEL0 = 6.75;
const double BOMB_THROW_RATE_LEVEL1 = 5.0;
const double BOMB_THROW_RATE_LEVEL2 = 4.0;
const double BOMB_THROW_RATE_LEVEL3 = 3.0;
const size_t EXPLOSION_TICKS = 30;

// fruit basket
const double BASKET_RADIUS = 40;
const double BASKET_MASS = 10.0;
const double BASKET_THROW_RATE = 15.0;
const rgb_color_t BASKET_COLOR = (rgb_color_t){0, 0, 0};
static const double BASKET_INITIAL_Y_VELOCITY = 0.0;
const double BASKET_Y_OFFSET = 10;

// image paths
const size_t NUM_FRUITS = 6;
const char *APPLE_PATH = "assets/apple.png";
const char *APPLE_SLICE_PATH = "assets/apple_slice.png";
const char *ORANGE_PATH = "assets/orange.png";
const char *ORANGE_SLICE_PATH = "assets/orange_slice.png";
const char *GOLDEN_APPLE_PATH = "assets/golden_apple.png";
const char *GOLDEN_APPLE_SLICE_PATH = "assets/golden_apple_slice.png";
const char *WATERMELON_PATH = "assets/watermelon.png";
const char *WATERMELON_SLICE_PATH = "assets/watermelon_slice.png";
const char *PEACH_PATH = "assets/peach.png";
const char *PEACH_SLICE_PATH = "assets/peach_slice.png";
const char *POMEGRANATE_PATH = "assets/pomegranate.png";
const char *POMEGRANATE_SLICE_PATH = "assets/pomegranate_slice.png";
const char *BOMB_PATH = "assets/bomb.png";
const char *FRUIT_BASKET_PATH = "assets/fruitbasket.png";
const char *BASKET_EXPLOSION_PATH = "assets/fruit_burst.png";
const char *EXPLOSION_PATH = "assets/explosion.png";

typedef struct state {
  scene_t *scene;
  double time_since_last_throw;
  double time_since_double_throw;
  double time_elapsed;
  double time_since_start;
  double countdown;
  bool player_exists;
  size_t points;
  size_t cursor_render_ticks;
  text_t *text;
  size_t level;
  vector_t ult_pos;
  vector_t penult_pos;
  double time_since_bomb_throw;
  double time_since_basket_throw;
  size_t ticks_since_explosion;
  bool frenzy;
  double time_since_frenzy;
  bool intro;
  bool win;
  bool lose;
} state_t;

double get_rand_angular_velocity() {
  return M_PI * ((rand() % (2 * MAX_ANGULAR_VEL)) - MAX_ANGULAR_VEL) / 180;
}

size_t get_rand_num() { return (rand() % NUM_FRUITS); }

/** Constructs a circles with the given radius centered at (0, 0) */
list_t *circle_init_angle(double radius, double max_angle) {
  list_t *circle = list_init(CIRCLE_POINTS, free);
  double arc_angle = max_angle;
  vector_t point = {.x = radius, .y = 0.0};
  for (size_t i = 0; i < CIRCLE_POINTS; i++) {
    vector_t *v = malloc(sizeof(*v));
    *v = point;
    list_add(circle, v);
    point = vec_rotate(point, arc_angle);
  }
  return circle;
}

list_t *circle_init(double radius) {
  return circle_init_angle(radius, 2 * M_PI / CIRCLE_POINTS);
}

list_t *semicircle_init(double radius) {
  return circle_init_angle(radius, M_PI / CIRCLE_POINTS);
}

bool is_fruit(body_type_t type) {
  return (type == APPLE || type == ORANGE || type == GOLDEN_APPLE ||
          type == WATERMELON || type == PEACH || type == POMEGRANATE);
}

body_t *create_slice_body(list_t *vertices, body_type_t fruit_type,
                          double angular_vel) {
  const char *image_path;
  switch (fruit_type) {
  case APPLE:
    image_path = APPLE_SLICE_PATH;
    break;
  case ORANGE:
    image_path = ORANGE_SLICE_PATH;
    break;
  case GOLDEN_APPLE:
    image_path = GOLDEN_APPLE_SLICE_PATH;
    break;
  case WATERMELON:
    image_path = WATERMELON_SLICE_PATH;
    break;
  case PEACH:
    image_path = PEACH_SLICE_PATH;
    break;
  case POMEGRANATE:
    image_path = POMEGRANATE_SLICE_PATH;
    break;
  default:
    image_path = APPLE_SLICE_PATH;
  }
  return body_init_with_info(vertices, FRUIT_MASS, DEFAULT_COLOR,
                             make_type_info(SLICE), NULL, FRUIT_RADIUS,
                             image_path, angular_vel);
}

void add_explosion(state_t *state, body_t *body, const char *image_path) {
  body_t *explosion = body_init_with_info(
      circle_init(EXPLOSION_RADIUS), DEFAULT_MASS, DEFAULT_COLOR,
      make_type_info(EXPLOSION), NULL, EXPLOSION_RADIUS, image_path, 0);
  body_set_centroid(explosion, body_get_centroid(body));
  scene_add_body(state->scene, explosion);
  state->ticks_since_explosion = EXPLOSION_TICKS;
}

void add_slices(state_t *state, body_t *fruit, double angle) {
  vector_t fruit_centroid = body_get_centroid(fruit);
  list_t *top_slice_vertices = semicircle_init(FRUIT_RADIUS);
  list_t *bottom_slice_vertices = semicircle_init(FRUIT_RADIUS);

  polygon_rotate(bottom_slice_vertices, M_PI,
                 polygon_centroid(bottom_slice_vertices));
  vector_t semi_center = vec_multiply(
      0.5, vec_add(*(vector_t *)list_get(top_slice_vertices, 0),
                   *(vector_t *)list_get(top_slice_vertices,
                                         list_size(top_slice_vertices) - 1)));
  vector_t translation = vec_subtract(fruit_centroid, semi_center);
  polygon_translate(top_slice_vertices, translation);
  polygon_translate(bottom_slice_vertices,
                    vec_add(translation, (vector_t){0, -FRUIT_RADIUS}));
  polygon_rotate(top_slice_vertices, angle, fruit_centroid);
  polygon_rotate(bottom_slice_vertices, angle, fruit_centroid);

  body_type_t fruit_type = get_type(fruit);
  double angular_vel = body_get_angular_velocity(fruit);

  body_t *top_slice =
      create_slice_body(top_slice_vertices, fruit_type, angular_vel);
  body_t *bottom_slice =
      create_slice_body(bottom_slice_vertices, fruit_type, -angular_vel);

  body_set_init_angle(top_slice, angle);
  body_set_init_angle(bottom_slice, M_PI + angle);

  body_set_init_centroid(top_slice, polygon_centroid(top_slice_vertices));
  body_set_init_centroid(bottom_slice, polygon_centroid(bottom_slice_vertices));

  vector_t fruit_velocity = body_get_velocity(fruit);
  body_set_velocity(top_slice, fruit_velocity);
  fruit_velocity.x *= -1;
  body_set_velocity(bottom_slice, fruit_velocity);

  scene_t *scene = state->scene;

  scene_add_body(scene, top_slice);
  scene_add_body(scene, bottom_slice);

  size_t body_count = scene_bodies(scene);
  bool gravity_done = false;
  for (size_t i = 0; i < body_count; i++) {
    body_t *other_body = scene_get_body(scene, i);
    switch (get_type(other_body)) {
    case GRAVITY:
      create_newtonian_gravity(scene, G, other_body, top_slice);
      create_newtonian_gravity(scene, G, other_body, bottom_slice);
      gravity_done = true;
      break;
    default:
      break;
    }
    if (gravity_done) {
      return;
    }
  }
}

void flying_obj_collision_handler(body_t *cursor, body_t *body, vector_t axis,
                                  state_t *state) {
  vector_t cursor_vec;
  double angle;
  switch (get_type(body)) {
  case APPLE:
  case GOLDEN_APPLE:
  case WATERMELON:
  case PEACH:
  case POMEGRANATE:
  case ORANGE:
    cursor_vec = vec_subtract(state->ult_pos, state->penult_pos);
    angle = -1 * vec_angle_btwn(cursor_vec, (vector_t){-1, 0});
    add_slices(state, body, angle);
    state->points++;
    break;
  case BOMB:
    if (state->points < 5) {
      state->points = 0;
    } else {
      state->points = state->points - 5;
    }
    add_explosion(state, body, EXPLOSION_PATH);
    break;
  case POWERUP:
    add_explosion(state, body, BASKET_EXPLOSION_PATH);
    state->frenzy = true;
    break;
  default:
    break;
  }
  body_remove(body);
}

double rand_x_position() {
  double num = (double)(rand() % (int)SCREEN_SIZE.x + 1);
  return num;
}

double rand_x_velocity(double x_position) {
  double halfway = SCREEN_SIZE.x / 2;
  double num = (double)(rand() % MAX_X_VELOCITY + 1);
  if (x_position > halfway) {
    return -num;
  }
  return num;
}

void apply_forces(state_t *state, body_t *body1) {
  scene_t *scene = state->scene;
  size_t body_count = scene_bodies(scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body2 = scene_get_body(scene, i);
    switch (get_type(body2)) {
    case PLAYER:
      create_collision(scene, body2, body1,
                       (collision_handler_t)flying_obj_collision_handler, state,
                       NULL);
      break;
    case GRAVITY:
      create_newtonian_gravity(scene, G, body2, body1);
      break;
    default:
      break;
    }
  }
}

void throw_fruit(state_t *state) {
  // generate body
  list_t *fruit = circle_init(FRUIT_RADIUS);
  double x_pos = rand_x_position();
  polygon_translate(fruit, (vector_t){.x = x_pos, .y = MIN_Y_POSITION});
  body_t *fruit_body;
  const char *image_path;
  body_type_t body_type;
  switch (get_rand_num()) {
  case 0:
    image_path = ORANGE_PATH;
    body_type = ORANGE;
    break;
  case 1:
    image_path = APPLE_PATH;
    body_type = APPLE;
    break;
  case 2:
    image_path = GOLDEN_APPLE_PATH;
    body_type = GOLDEN_APPLE;
    break;
  case 3:
    image_path = WATERMELON_PATH;
    body_type = WATERMELON;
    break;
  case 4:
    image_path = PEACH_PATH;
    body_type = PEACH;
    break;
  case 5:
    image_path = POMEGRANATE_PATH;
    body_type = POMEGRANATE;
    break;
  default:
    image_path = ORANGE_PATH;
    body_type = ORANGE;
    break;
  }
  fruit_body = body_init_with_info(
      fruit, FRUIT_MASS, DEFAULT_COLOR, make_type_info(body_type), NULL,
      FRUIT_RADIUS, image_path, get_rand_angular_velocity());
  double x_vel = rand_x_velocity(x_pos);
  body_set_velocity(fruit_body, (vector_t){x_vel, INITIAL_Y_VELOCITY});
  scene_add_body(state->scene, fruit_body);
  apply_forces(state, fruit_body);
}

void throw_bomb(state_t *state) {
  list_t *bomb = circle_init(BOMB_RADIUS);
  double x_pos = rand_x_position();
  polygon_translate(bomb, (vector_t){.x = x_pos, .y = MIN_Y_POSITION});
  body_t *bomb_body =
      body_init_with_info(bomb, BOMB_MASS, GRAY, make_type_info(BOMB), NULL,
                          BOMB_RADIUS, BOMB_PATH, get_rand_angular_velocity());
  double x_vel = rand_x_velocity(x_pos);
  body_set_velocity(bomb_body, (vector_t){x_vel, INITIAL_Y_VELOCITY});
  scene_add_body(state->scene, bomb_body);
  apply_forces(state, bomb_body);
}

void throw_basket(state_t *state) {
  list_t *basket = circle_init(BASKET_RADIUS);
  double x_pos = rand_x_position();
  polygon_translate(
      basket, (vector_t){.x = x_pos, .y = SCREEN_SIZE.y - BASKET_Y_OFFSET});
  body_t *basket_body = body_init_with_info(
      basket, BASKET_MASS, BASKET_COLOR, make_type_info(POWERUP), NULL,
      BASKET_RADIUS, FRUIT_BASKET_PATH, get_rand_angular_velocity());
  double x_vel = rand_x_velocity(x_pos);
  body_set_velocity(basket_body, (vector_t){x_vel, BASKET_INITIAL_Y_VELOCITY});
  scene_add_body(state->scene, basket_body);
  apply_forces(state, basket_body);
}

void add_gravity_body(scene_t *scene) {
  // Will be offscreen, so shape is irrelevant
  list_t *gravity_ball = circle_init(1);
  body_t *body = body_init_with_info(gravity_ball, M, DEFAULT_COLOR,
                                     make_type_info(GRAVITY), free, 1, NULL, 0);

  // Move a distnace R below the scene
  vector_t gravity_center = {.x = SCREEN_SIZE.x / 2, .y = -R};
  body_set_centroid(body, gravity_center);
  scene_add_body(scene, body);
}

void add_cursor_body(state_t *state) {
  scene_t *scene = state->scene;
  list_t *cursor = circle_init(CURSOR_RADIUS);
  body_t *body =
      body_init_with_info(cursor, DEFAULT_MASS, CURSOR_COLOR,
                          make_type_info(PLAYER), free, CURSOR_RADIUS, NULL, 0);
  size_t body_count = scene_bodies(scene);
  // in case cursor was generated after fruit
  for (size_t i = 0; i < body_count; i++) {
    body_t *body2 = scene_get_body(scene, i);
    if (is_fruit(get_type(body2)) || get_type(body2) == BOMB ||
        get_type(body2) == POWERUP) {
      create_collision(scene, body, body2,
                       (collision_handler_t)flying_obj_collision_handler, state,
                       NULL);
    }
  }
  scene_add_body(scene, body);
}

void reset_state_variables(state_t *state) {
  state->cursor_render_ticks = false;
  state->time_since_last_throw = 0;
  state->points = 0;
  state->ult_pos = VEC_ZERO;
  state->penult_pos = VEC_ZERO;
  state->time_since_bomb_throw = 0;
  state->time_since_basket_throw = 0;
  state->ticks_since_explosion = 0;
  state->countdown = COUNTDOWN_TIMER;
  state->frenzy = false;
  state->time_since_frenzy = 0;
}

void remove_sprites(state_t *state) {
  scene_t *scene = state->scene;
  size_t num_bodies = scene_bodies(scene);
  for (size_t i = 0; i < num_bodies; i++) {
    body_t *body = scene_get_body(scene, i);
    if (get_type(body) != GRAVITY) {
      body_remove(body);
    }
  }
  state->player_exists = false;
  reset_state_variables(state);
}

void scene_update(state_t *state, char key, double held_time,
                  vector_t mouse_loc) {
  scene_t *scene = state->scene;
  double time_elapsed = state->time_elapsed;

  switch (key) {
  case MOUSEBUTTONDOWN:
    state->cursor_render_ticks = CURSOR_TICK_DELAY;
  case MOUSEBUTTONUP:
  case MOUSE_CLICK:
  case MOUSE_MOVED:
  case MOUSE_ENGAGED:
    if (state->cursor_render_ticks >= 1) {
      state->cursor_render_ticks = CURSOR_TICK_DELAY;
    }
    break;
  case LEFT_ARROW:
    break;
  case RIGHT_ARROW:
    break;
  case SPACE:
    state->intro = false;
    break;
  default:
    if (state->cursor_render_ticks > 0) {
      state->cursor_render_ticks--;
    }
    break;
  }

  // flips mouse position to sdl position
  mouse_loc.y = SCREEN_SIZE.y - mouse_loc.y;
  if (state->cursor_render_ticks) {
    if (!state->player_exists) {
      state->player_exists = true;
      add_cursor_body(state);
    }
  }

  if (state->frenzy) {
    if (state->time_since_double_throw >
        FRENZY_DOUBLE_FRUIT_RATE) { // could move this to a scene_update
                                    // function
      throw_fruit(state);
      throw_fruit(state);
      state->time_since_last_throw = 0.0;
      state->time_since_double_throw = 0.0;
    }
    if (state->time_since_last_throw > FRENZY_FRUIT_THROW_RATE) {
      throw_fruit(state);
      state->time_since_last_throw = 0.0;
    }
  } else {
    if (state->time_since_double_throw >
        DOUBLE_FRUIT_RATE) { // could move this to a scene_update function
      throw_fruit(state);
      throw_fruit(state);
      state->time_since_last_throw = 0.0;
      state->time_since_double_throw = 0.0;
    }
    if (state->time_since_last_throw > FRUIT_THROW_RATE) {
      throw_fruit(state);
      state->time_since_last_throw = 0.0;
    }
  }

  switch (state->level) {
  case 1:
    if (state->time_since_bomb_throw > BOMB_THROW_RATE_LEVEL1) {
      throw_bomb(state);
      state->time_since_bomb_throw = 0.0;
    }
  case 2:
    if (state->time_since_bomb_throw > BOMB_THROW_RATE_LEVEL2) {
      throw_bomb(state);
      state->time_since_bomb_throw = 0.0;
    }
  case 3:
    if (state->time_since_bomb_throw > BOMB_THROW_RATE_LEVEL3) {
      throw_bomb(state);
      state->time_since_bomb_throw = 0.0;
    }
  default:
    if (state->time_since_bomb_throw > BOMB_THROW_RATE) {
      throw_bomb(state);
      state->time_since_bomb_throw = 0.0;
    }
  }

  if (state->time_since_basket_throw > BASKET_THROW_RATE) {
    throw_basket(state);
    state->time_since_basket_throw = 0.0;
  }

  size_t body_count = scene_bodies(scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    double y_pos = body_get_centroid(body).y;
    switch (get_type(body)) {
    case SLICE:
    case ORANGE:
    case GOLDEN_APPLE:
    case WATERMELON:
    case PEACH:
    case POMEGRANATE:
    case APPLE:
    case BOMB:
    case POWERUP:
      if (y_pos < 0.0) {
        body_remove(body);
      }
      break;
    case PLAYER:
      if (state->cursor_render_ticks <= 0 && state->player_exists) {
        state->player_exists = false;
        body_remove(body);
      }
      if (state->cursor_render_ticks >= 1) {
        state->penult_pos = state->ult_pos;
        if (mouse_loc.x == 0 && mouse_loc.y == SCREEN_SIZE.y) {
          body_set_centroid(body, body_get_centroid(body));
          state->ult_pos = body_get_centroid(body);
        } else {
          body_set_centroid(body, mouse_loc);
          state->ult_pos = mouse_loc;
        }
      }
      break;
    case EXPLOSION:
      if (state->ticks_since_explosion > 0) {
        state->ticks_since_explosion -= 1;
      } else {
        body_remove(body);
      }
      break;
    default:
      break;
    }
  }
  scene_tick(scene, time_elapsed);
}

void on_key(char key, key_event_type_t type, double held_time, state_t *state,
            vector_t loc) {
  if (type == KEY_PRESSED || type == MOUSE_ENGAGED) {
    scene_update(state, key, held_time, loc);
  }
}

state_t *emscripten_init(void) {
  sdl_on_key(on_key);
  srand(time(NULL));
  // Initialize scene
  sdl_init(VEC_ZERO, SCREEN_SIZE);
  scene_t *scene = scene_init();
  add_gravity_body(scene);
  // Repeatedly render scene
  state_t *state = malloc(sizeof(state_t));
  state->scene = scene;
  state->player_exists = true;
  state->time_since_start = 0;
  state->intro = true;
  state->win = false;
  state->lose = false;
  state->level = 1;
  reset_state_variables(state);

  TTF_Font *font = TTF_OpenFont("assets/Roboto-Regular.ttf", 50);
  text_t *text = text_init(font, free);
  state->text = text;

  add_cursor_body(state);
  return state;
}

void emscripten_main(state_t *state) {
  sdl_render_scene(state->scene, SCREEN_SIZE, state->intro, state->win,
                   state->lose, state->level);
  if (!state->intro) {
    double time_elapsed = time_since_last_tick();
    state->time_since_last_throw += time_elapsed;
    state->time_since_double_throw += time_elapsed;
    state->time_since_bomb_throw += time_elapsed;
    state->time_since_basket_throw += time_elapsed;
    state->time_elapsed = time_elapsed;
    state->time_since_start += time_elapsed;
    state->countdown -= time_elapsed;
    if (state->frenzy) {
      state->time_since_frenzy += time_elapsed;
      if (state->time_since_frenzy > FRENZY_TIME_LIMIT) {
        state->frenzy = false;
        state->time_since_frenzy = 0;
      }
    }

    if (state->countdown < 0) {
      // fprintf(stderr, "%s\n", "game over");
      state->lose = true;
    }

    if (state->level == 1 && state->points >= LEVEL_1) {
      state->level = 2;
      state->countdown = COUNTDOWN_TIMER;
      state->points = 0;
      remove_sprites(state);
    } else if (state->level == 2 && state->points >= LEVEL_2) {
      state->level = 3;
      state->countdown = COUNTDOWN_TIMER;
      state->points = 0;
      remove_sprites(state);
    } else if (state->level == 3 && state->points >= LEVEL_3) {
      state->countdown = COUNTDOWN_TIMER;
      state->points = 0;
      remove_sprites(state);
      // fprintf(stderr, "%s\n", "win");
      state->win = true;
    }
    scene_update(state, '\0', 0.0, VEC_ZERO);
    if (!state->win && !state->lose) {
      sdl_render_text(state->scene, state->text, state->countdown,
                      state->points, state->level);
    }
  }
}

void emscripten_free(state_t *state) {
  text_free(state->text);
  scene_free(state->scene);
  free(state);
}
