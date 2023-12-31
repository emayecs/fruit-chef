#ifndef __COLOR_H__
#define __COLOR_H__

/**
 * A color to display on the screen.
 * The color is represented by its red, green, and blue components.
 * Each component must be between 0 (black) and 1 (white).
 */
typedef struct {
  float r;
  float g;
  float b;
  float a;
} rgb_color_t;

/**
 * Return a random color.
 * @returns a random color
 */
rgb_color_t get_random_color();

rgb_color_t set_yellow();

rgb_color_t set_blank();

#endif // #ifndef __COLOR_H__
