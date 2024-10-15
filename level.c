#include "level.h"


//Specs of given level
LevelInfo generate_level_info(int level_nr) {
    LevelInfo level_info = (LevelInfo) {
    /*Width             */  15,
    /*Height            */  9, 
    /*level nr          */  21,
    /*fill_ratio        */  0.1, 
    /*#pikachu's        */  (int) ((double) (5 / (exp(-(level_nr - 10) * 0.2) + 1)) + (double) pow((level_nr * 0.05),(1 / 3)) + (double) 0.55),
    /*#strong pikachu's */  (int) ((double) (((5 / (exp(-(level_nr - 13) * 0.5) + 1)) + (double) pow((level_nr * 0.05), (1 / 3)) + (double) 0.55) / 3) + 0.8), 
    /*bonus_spawn_ratio */  (double) (0.01 + 0.29*exp(-(level_nr-1)*0.01)) 
    };
    return level_info;
}

//Handle uncatchable obstacles
void init_entities(Entity** entities, int width, int height) {
    for(int j = 0; j < height; j++) {
        entities[0][j].obstacle = (Obstacle) {OBSTACLE, 0, j* TILE_SIZE, false};
        entities[width-1][j].obstacle = (Obstacle) {OBSTACLE, (width-1)* TILE_SIZE, j* TILE_SIZE, false};
    }
    for(int i = 1; i < width-1; i++) {
        entities[i][0].obstacle = (Obstacle) {OBSTACLE, i* TILE_SIZE, 0, false};
        entities[i][height-1].obstacle = (Obstacle) {OBSTACLE, i* TILE_SIZE, (height-1)* TILE_SIZE, false};

        for(int j = 1; j < height-1; j++) {
            if(i%2 == 0 && j%2 == 0) {
                entities[i][j].obstacle = (Obstacle) {OBSTACLE, i * TILE_SIZE, j* TILE_SIZE, false};
            } else {
                entities[i][j].empty_space = (EmptySpace) {EMPTY_SPACE, i, j};
            }
        }
    }
}


//Create a grid of uncatchable obstacles, add catchable obstacles, init levelinfo
void init_level(Level* level, LevelInfo level_info) {
    //Generating the board
    level->entities = malloccheck(level_info.width, Entity*);
    for(int i = 0; i < level_info.width; i++) {
        level->entities[i] = malloccheck(level_info.height, Entity);
    }

    //Adding levelinfo to level
    level->level_info = level_info;

    //Initialize grid and edge of the board
    init_entities(level->entities, level_info.width, level_info.height);

    //Number of uncatchable obstacles
    int num_obs = (level_info.width-2) * (level_info.height-2) * level_info.fill_ratio - ((int) ((level_info.width-3)/2)) * ((int) ((level_info.height-3)/2));
    
    //Coords
    int x, y;

    //Place remaining obstacles
    while(num_obs > 0) {
        x = rand()%(level_info.width-2), y = rand()%(level_info.height-2);
        if(level->entities[x][y].type == OBSTACLE) {
            continue;
        }
        if(x == 1 && y == 1) {
            continue;
        }
        if(x == 1 && y == 2) {
            continue;
        }
        if(x == 2 && y == 1) {
            continue;
        }
        level->entities[x][y].obstacle = (Obstacle) {OBSTACLE, x*TILE_SIZE, y*TILE_SIZE, true};
        num_obs--;
    }
    gui_set_level_info(&level->level_info);
}

void render_level(Level* level) {
    gui_draw_buffer();
    for(int i = 0; i < level->level_info.width; i++) {
        for(int j = 0; j < level->level_info.height; j++) {
            switch (level->entities[i][j].type)
            {
            case POKEBALL:
                gui_add_pokeball(&level->entities[i][j].pokeball);
                break;
            
            case CATCH_ATTEMPT:
                gui_add_catch_attempt_tile(i*TILE_DIM, j*TILE_DIM);
                break;
            
            case BONUS:
                gui_add_bonus(&level->entities[i][j].bonus);
                break;
            
            case OBSTACLE:
                gui_add_obstacle(&level->entities[i][j].obstacle);
                break;
            
            default:
                break;
            }
        }
    }
}

void destroy_level(Level* level) {
    for(int i = 0; i < level->level_info.height; i++) {
        free(level->entities[i]);
    }
    free(level->entities);
}
