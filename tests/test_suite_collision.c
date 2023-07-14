#include "collision.h"
#include "forces.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "test_util.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

list_t *make_shape() {
  list_t *shape = list_init(4, free);
  vector_t *v = malloc(sizeof(*v));
  *v = (vector_t){-1, -1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){+1, -1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){+1, +1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){-1, +1};
  list_add(shape, v);
  return shape;
}

list_t *make_quad(double x0, double y0, double x1, double y1, double x2,
                  double y2, double x3, double y3) {
  list_t *sq = list_init(4, free);
  vector_t *v = malloc(sizeof(*v));
  *v = (vector_t){x0, y0};
  list_add(sq, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){x1, y1};
  list_add(sq, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){x2, y2};
  list_add(sq, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){x3, y3};
  list_add(sq, v);
  return sq;
}

void test_dynamic_collision() {
  const double m = 10;
  const double DT = 1e-4;
  const double G = 6.6743015 * 1e-11;
  const double earth_mass = 5.97219 * 1e24;
  const double earth_radius = 6378137;
  const double elevation = 20;
  const double gravitational_acceleration = 9.8;
  scene_t *scene = scene_init();
  body_t *body = body_init(make_shape(), m, (rgb_color_t){0, 0, 0});
  body_set_centroid(body, (vector_t){0, earth_radius + elevation + 1});
  scene_add_body(scene, body);
  body_t *earth = body_init(make_shape(), earth_mass, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, earth);
  body_t *ground = body_init(make_shape(), m, (rgb_color_t){0, 0, 0});
  body_set_centroid(ground, (vector_t){0, earth_radius - 1});
  scene_add_body(scene, ground);
  create_newtonian_gravity(scene, G, body, earth);
  create_destructive_collision(scene, body, ground);
  double t = 0;
  while (scene_bodies(scene) > 1) {
    t += DT;
    /* fprintf(stderr, "--------------------------------NEW TIME: %f \n", t);
    list_t *vertices1 = body_get_shape(body);
    list_t *vertices2 = body_get_shape(ground);
    for (size_t i = 0; i < list_size(vertices1); i++){
      vector_t v = *(vector_t *)list_get(vertices1, i);
      fprintf(stderr, "vertex: %f, %f \n", v.x, v.y);
    }
    for (size_t i = 0; i < list_size(vertices2); i++){
      vector_t v = *(vector_t *)list_get(vertices2, i);
      fprintf(stderr, "vertex: %f, %f \n", v.x, v.y);
    } */
    scene_tick(scene, DT);
  }
  double predicted_time = sqrt(elevation * 2.0 / gravitational_acceleration);
  assert(within(1e-3, t, predicted_time));
  scene_free(scene);
}

void assert_static_collisions(list_t *sq1, list_t *sq2, list_t *sq3,
                              list_t *sq4, list_t *sq5, list_t *sq6) {
  assert(find_collision(sq1, sq2).collided);
  assert(find_collision(sq1, sq3).collided);
  assert(find_collision(sq1, sq4).collided);
  assert(find_collision(sq1, sq5).collided);
  assert(!find_collision(sq1, sq6).collided);

  assert(!find_collision(sq2, sq3).collided);
  assert(find_collision(sq2, sq4).collided);
  assert(!find_collision(sq2, sq5).collided);
  assert(!find_collision(sq2, sq6).collided);

  assert(find_collision(sq3, sq4).collided);
  assert(!find_collision(sq3, sq5).collided);
  assert(!find_collision(sq3, sq6).collided);

  assert(!find_collision(sq4, sq5).collided);
  assert(!find_collision(sq4, sq6).collided);

  assert(!find_collision(sq5, sq6).collided);
}

void test_static_collision() {
  list_t *sq1 = make_quad(-1, -1, -1, 1, 1, 1, 1, -1);
  list_t *sq2 = make_quad(0, 0, 0, 1, 1, 1, 1, 0);
  list_t *sq3 = make_quad(-0.5, -0.5, -1, 0, -1, -1, 0, -1);
  list_t *sq4 = make_quad(0.5, 0.5, -1, 0, -1, -1, 0, -1);
  list_t *sq5 = make_quad(0, -1, 1, -1, 1, 0, 0.5, -0.5);

  list_t *sq6 = make_quad(0, 5, 0, 6, 1, 6, 1, 5);

  // test collision code in different angles
  for (size_t i = 0; i < 4; i++) {
    assert_static_collisions(sq1, sq2, sq3, sq4, sq5, sq6);

    polygon_rotate(sq1, 1.5707, VEC_ZERO);
    polygon_rotate(sq2, 1.5707, VEC_ZERO);
    polygon_rotate(sq3, 1.5707, VEC_ZERO);
    polygon_rotate(sq4, 1.5707, VEC_ZERO);
    polygon_rotate(sq5, 1.5707, VEC_ZERO);
    polygon_rotate(sq6, 1.5707, VEC_ZERO);
  }

  vector_t t = (vector_t){2, 2};
  // test collision code in different axes
  for (size_t i = 0; i < 4; i++) {
    assert_static_collisions(sq1, sq2, sq3, sq4, sq5, sq6);

    polygon_translate(sq1, t);
    polygon_translate(sq2, t);
    polygon_translate(sq3, t);
    polygon_translate(sq4, t);
    polygon_translate(sq5, t);
    t = vec_multiply(2, vec_rotate(t, 1.5707));
  }

  list_free(sq1);
  list_free(sq2);
  list_free(sq3);
  list_free(sq4);
  list_free(sq5);
  list_free(sq6);
}

int main(int argc, char *argv[]) {
  // Run all tests if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_static_collision)
  DO_TEST(test_dynamic_collision)

  puts("collision_test PASS");
}