#include "list.h"
#include "math.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct text text_t;

text_t *text_init(TTF_Font *font, free_func_t info_free);

void text_free(text_t *text);

TTF_Font *text_get_font(text_t *text);