
#pragma once
#ifndef H_GAME
#define H_GAME

#include "catch_them_all.h"
#include "level.h"
#include "highscores.h"
#include "stdbool.h"
#include "gui.h"
#include "graph.h"



#define CHECK_FOR_PLAYER 10 // deze waarden worden gebruikt in collision check box (zie verder) 
#define CHECK_FOR_PIKACHU 11
#define NO_PIKACHU -1 // used when collision check is not checking any pikachu (index = -1) so we use player coords

#define BOX_OFFSET_PLAYER 0  // bepaalt hoe streng collision check voor player is, hoe lager hoe strenger 
#define BOX_OFFSET_PIKACHU 2 // bepaalt hoe streng collision check voor pikachu is, hoe lager hoe strenger 

#define PIKACHU_MOVEMENT_UPDATE 32  // (64 / 2) , bepaald wanneer van richting moet worden veranderd 
#define STRONG_PIKACHU_MOVEMENT_UPDATE 16 // (64 / 4) bepaald wanneer van richting moet worden veranderd 
#define PIKACHU_AVOID_POKEBALL 16 // hoe slim een pikachu moet zijn om een pokeball te ontwijken 
#define PIKACHU_DODGE_CATCH_POKEBALL 14


typedef enum {LAUNCHED=0, FINISHED=1, GAME_OVER=2, GAME_ENDED=3} STATE;
#define LEVEL_1 1
#define LEVEL_2 2
#define LEVEL_3 3
#define LEVEL_4 4
#define LEVEL_5 5
#define LEVEL_6 6

#define LEFT 3 
#define RIGHT 2
#define UP 0
#define DOWN 1


/* controleer of entity al voor meer dan de helft op volgende tile bevind */
static inline int plafoneer_pos(int coord) {  
    return (coord % TILE_DIM) > (TILE_DIM / 2) ? 1 : 0;
}

static inline bool isValidCoord(int x, int y, int width, int height) {
    return (x < 0 || x > width - 1 || y < 0 || y > height - 1) ? 0 : 1;
}

struct graph;

typedef struct {
    int has_moved;
    int moves[4];
    int drop_pokeball;
} Input;

typedef struct Game {
    Level level;
    Player player;
    Pikachu* pikachus;
    int pikachus_left;
    STATE state;
    Input input;
    int score;
    graph* graaf;
    int pikachu_ai_timer;
    int how_smart_pikachus;
    int amt_of_tiles;
} Game;


#define entity_type(x,y,TYPE) (bool) (game->level.entities[x][y].type == TYPE)




// general game functions
void init_game(Game* game, int level_nr);
int do_game_loop(Game* game);
void update_game(Game* game);
void check_game_input(Game* game);
bool wait_for_space();
void level_transition(Game*, HighScoreTable*, int);
void destroy_game(Game* game);

// rendering functions
void render_game(Game* game);
void render_pikachus(Game* game);
void render_player(Game* game);

// main processing functions
void process_pokeballs(Game* game);
void process_bonus_items(Game* game);
void process_catchattempt(Game* game, int i, int j, int direction);
void process_game_difficulty(Game* game);


// Pikachu related functions 
void do_pikachu_ai(Game* game);
void pikachu_move(Game* game, int which_pikachu, int direction, int speed);
void move_pikachu_to_tile(Game* game, int i, int j, int which_pikachu);
void pikachus_dodge(Game* game); 
void pikachus_undodge(Game* game);
void freeze_pikachu(Game* game, int freeze_duration, int which_pikachu);
void move_pikachu_to_player(Game* game, int which_one);
void update_all_pikachu_paths(Game* game);
void update_pikachu_path(Game* game, int which_pikachu);
void set_pikachu_caught(Game* game, int which_pikachu);
void process_pikachu_collisions(Game* game, int which_pikachu);
void freeze_pikachu(Game* game, int freeze_duration, int which_pikachu);
node* pikachu_path_to_player(Game* game, int which_one);


int* coordinates_to_tile(Game* game, int x, int y);
void direction_to_coords(int* x, int* y, int offset, int direction);
void do_player_movement(Game* game);
int inverse_direction(int direction);
bool player_move(Game* game, int DIRECTION, int speed);

void board_to_graph(Game* game);
void update_board_catch_attempt(Game* game, int i, int j);
void update_board_position(Game* game, int x, int y);
void look_around_tile(Game* game, graph* g, int x, int y);
void look_direction(Game* game, graph* g, int x, int y, int direction);
void update_board(Game* game);

///////////////////// COLLISION FUNCTIONS ///////////////////////////
bool is_path_collision(Game* game, node* path, int i, int j, int which_pikachu);
bool isCollision(Game* game, int x1, int y1, int x2, int y2, int box_offset, ENTITY_TYPE collision_type);
bool collision_box_look_around(Game* game, int which_pikachu, ENTITY_TYPE collision_type);

// util functions
int random_number(int low, int high);
int random_excl(int low, int high, int excl);
float random_number_fl();
char* allocate_strlen(unsigned long len);
int max(const int a, const int b);
int sum(int* array, int length);


////////////////// SETTER FUNCTIONS //////////////////////////////////
void set_emptyspace(Game* game, int x, int y);
void set_obstacle(Game* game, int x, int y, int is_catchable);
void set_bonus(Game* game, int x, int y);
void set_catchattempt(Game* game, int x, int y, int prop_up, int prop_down, int prop_right, int prop_left, int power, int ticks_left, bool to_bonus);
void set_pokeball(Game* game, int x, int y,  int ticks_left);
void set_player_caught(Game* game);
void pikachu_set_position(Game* game, int which_pikachu, int x_coord, int y_coord);
void player_set_position(Game* game, int x_coord, int y_coord);


/////////////////// TIMER FUNCTIONS ///////////////////////////////////
void pokeball_decrease_timer(Game* game, int i, int j, bool set_negative);
void catchattempt_decrease_timer(Game* game, int i, int j, bool set_negative);
void pikachu_decrease_timer(Game* game, int which_pikachu, bool set_negative);
bool is_pokeball_timer_expired(Game* game, int i, int j);
bool is_catchattempt_timer_expired(Game* game, int i, int j);
bool is_pikachu_timer_expired(Game* game, int which_pikachu);
void pikachu_set_timer(Game* game, int which_pikachu, int value);
void gui_clear_keys();



#endif
