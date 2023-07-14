#ifndef __COLLISION_H__
#define __COLLISION_H__

#include "list.h"
#include "vector.h"
#include <stdbool.h>

/**
 * Represents the status of a collision between two shapes.
 * The shapes are either not colliding, or they are colliding along some axis.
 */
typedef struct {
  /** Whether the two shapes are colliding */
  bool collided;
  /**
   * If the shapes are colliding, the axis they are colliding on.
   * This is a unit vector pointing from the first shape towards the second.
   * Normal impulses are applied along this axis.
   * If collided is false, this value is undefined.
   */
  vector_t axis;
} collision_info_t;

/**
 * Computes the status of the collision between two convex polygons.
 * The shapes are given as lists of vertices in counterclockwise order.
 * There is an edge between each pair of consecutive vertices,
 * and one between the first vertex and the last vertex.
 *
 * @param shape1 the first shape
 * @param shape2 the second shape
 * @return whether the shapes are colliding, and if so, the collision axis.
 * The axis should be a unit vector pointing from shape1 towards shape2.
 */
collision_info_t find_collision(list_t *shape1, list_t *shape2);

void remove_and_free(list_t *list);

size_t find_min_x(list_t *list);

size_t find_min_y(list_t *list);

/**
 * Goes through all axes that are perpendicular to all of the edges of shape1.
 * For each axis, checks if the projection of shape1 onto the axis intersects
 * with the proejction of shape2 onto the same axis. If any pair doesn't
 * intersect, return true. Otherwise, return false.
 *
 * @param shape1 a pointer to the list of vertices of shape1
 * @param shape2 a pointer to the list of vertices of shape2
 * @return if all of the projections of shape1 and shape2 on every
 * perpendicular axis of shape1 intersect
 */
collision_info_t check_shape_sides(list_t *overlaps, list_t *shape1,
                                   list_t *shape2);

/**
 * Returns the vector that is closer to the origin.
 *
 * @param a the first vector
 * @param b the second vector
 * @return the vector with the least magnitude.
 */
vector_t closer_to_origin(vector_t a, vector_t b);

/**
 * TODO
 */
double find_overlap(list_t *proj1, list_t *proj2);

/**
 * Finds the projection of a polygon onto a vector.
 * Adds the start point and end point of the projection as two vectors to a
 * given list.
 *
 * @param points a list to add the start and end point to.
 * @param shape a list of the vertices of the polygon.
 * @param line the vector that the polygon will be projected onto.
 */
void projection_of_polygon(list_t *points, list_t *shape, vector_t line);

#endif // #ifndef __COLLISION_H__
