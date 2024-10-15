#ifndef H_LEVEL
#define H_LEVEL

#include "stdlib.h"
#include "catch_them_all.h"
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include "gui.h"
#include <math.h>

typedef struct {
    LevelInfo level_info;
    Entity** entities;
} Level;

LevelInfo generate_level_info(int level_nr);
void init_level(Level* level, LevelInfo level_info);
void render_level(Level* level);
void destroy_level(Level* level);
void init_entities(Entity** entities, int width, int height);

#endif