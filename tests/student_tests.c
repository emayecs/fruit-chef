#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "forces.h"
#include "test_util.h"

const double E = 2.71828183;

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

// Tests that an object falling toward the Earth behaves as expected
void test_falling_gravity() {
  const double m = 10;
  const double DT = 1e-6;
  const double G = 6.6743015 * 1e-11;
  const double earth_mass = 5.97219 * 1e24;
  const double earth_radius = 6378137;
  const double elevation = 20;
  const double gravitational_acceleration = -9.80665;
  const int STEPS = 1000000;
  scene_t *scene = scene_init();
  vector_t earth_surface = (vector_t){0, earth_radius};
  body_t *body = body_init(make_shape(), m, (rgb_color_t){0, 0, 0});
  body_set_centroid(body, (vector_t){0, earth_radius + elevation});
  scene_add_body(scene, body);
  body_t *earth = body_init(make_shape(), earth_mass, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, earth);
  create_newtonian_gravity(scene, G, body, earth);
  for (int i = 0; i < STEPS; i++) {
    vector_t centroid = vec_subtract(body_get_centroid(body), earth_surface);
    double time_elapsed = i * DT;
    double predicted_y = elevation + 0.5 * gravitational_acceleration *
                                         time_elapsed * time_elapsed;
    assert(within(1e-2, centroid.y, predicted_y));
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

// Tests that an object's velocity correctly changes due to drag force
void test_drag_force() {
  const double m = 10;
  const double DT = 1e-6;
  const vector_t v0 = (vector_t){10.0, 0};
  const double gamma = 1.0;
  const int STEPS = 1000000;
  scene_t *scene = scene_init();
  body_t *body = body_init(make_shape(), m, (rgb_color_t){0, 0, 0});
  body_set_velocity(body, v0);
  scene_add_body(scene, body);
  create_drag(scene, gamma, body);
  for (int i = 1; i < STEPS; i++) {
    double curr_velocity = body_get_velocity(body).x;
    double time_elapsed = i * DT;
    double predicted_v = v0.x * pow(E, -gamma * time_elapsed / m);
    assert(within(1e-5, predicted_v, curr_velocity));
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

double spring_potential(double K, body_t *body1, body_t *body2) {
  vector_t r = vec_subtract(body_get_centroid(body2), body_get_centroid(body1));
  return 0.5 * K * pow(sqrt(vec_dot(r, r)), 2);
}
double kinetic_energy(body_t *body) {
  vector_t v = body_get_velocity(body);
  return body_get_mass(body) * vec_dot(v, v) / 2;
}

// Tests that a conservative force (spring) conserves K + U
void test_spring_energy_conservation() {
  const double M1 = 4.5, M2 = 1000.0;
  const double K = 0.5;
  const double DT = 1e-6;
  const int STEPS = 1000000;
  scene_t *scene = scene_init();
  body_t *mass1 = body_init(make_shape(), M1, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, mass1);
  body_t *mass2 = body_init(make_shape(), M2, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass1, (vector_t){10, 20});
  scene_add_body(scene, mass2);
  create_spring(scene, K, mass1, mass2);
  double initial_energy = spring_potential(K, mass1, mass2);
  // fprintf(stderr, "energy_0 %.8f \n", initial_energy);
  for (int i = 0; i < STEPS; i++) {
    double energy = spring_potential(K, mass1, mass2) + kinetic_energy(mass1) +
                    kinetic_energy(mass2);
    if (i % 100000 == 0) {
      // vector_t centroid1 = body_get_centroid(mass1);
      // vector_t centroid2 = body_get_centroid(mass2);
      // fprintf(stderr, "loc1 %.8f, %.8f \n", centroid1.x, centroid1.y);
      // fprintf(stderr, "loc2 %.8f, %.8f \n", centroid2.x, centroid2.y);
      // fprintf(stderr, "spring_energy %.8f \n", energy);
      // fprintf(stderr, "initial_energ %.8f \n", initial_energy);
      // fprintf(stderr, "ratio %.8f \n", energy / initial_energy);
    }

    assert(within(1e-4, energy / initial_energy, 1.0));
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

int main(int argc, char *argv[]) {
  // Run all tests if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_falling_gravity);
  DO_TEST(test_drag_force);
  DO_TEST(test_spring_energy_conservation);

  puts("student_tests PASS");
}
