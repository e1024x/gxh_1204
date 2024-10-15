#ifndef H_CATCH_THEM_ALL
#define H_CATCH_THEM_ALL



#define MAX_LEVEL_WIDTH 90
#define MAX_LEVEL_HEIGHT 60

#define TILE_SIZE 64

#define PLAYER_MOVEMENT_INCREMENT 4
#define PIKACHU_MOVEMENT_INCREMENT 2
#define STRONG_PIKACHU_MOVEMENT_INCREMENT 4


#define POKEBALL_TICKS 60 // 30     set to 60 instead of 30 as otherwise pokeball explodes too quick
#define CATCH_ATTEMPT_TICKS 60 // 60
#define POKEBALL_MAX_POWER 5

#define FREEZE_DURATION 100

#define PIKACHU_SCORE 60
#define STRONG_PIKACHU_SCORE 150

#define STRONG_PIKACHU_LIVES 3

#define REPLACE_BY_EMPTYSPACE false
#define REPLACE_BY_BONUS true

typedef struct stack stack;
typedef enum {POKEBALL, CATCH_ATTEMPT, BONUS, OBSTACLE, EMPTY_SPACE} ENTITY_TYPE;
typedef enum {EXTRA_POWER, EXTRA_POKEBALL, FREEZE_PIKACHUS} BONUS_TYPE;
typedef enum {NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3} ORIENTATION;
typedef enum {MOVEMENT_RANDOM = 0, MOVEMENT_HUNTER = 1} MOVEMENT_TYPE;

typedef struct {
    int width;
    int height;
    int level_nr;
    double fill_ratio;
    int nr_of_pikachus;
    int spawn_strong_pikachu;
    double bonus_spawn_ratio;
} LevelInfo;

typedef struct {
    int x;
    int y;
    ORIENTATION orientation;
    int pokeball_power;
    int remaining_pokeballs;
    int is_catched;
    int auto_walking_tick;
} Player;

typedef struct {
    ENTITY_TYPE type;
    int x;
    int y;
    int spread[4];
    int power;
    int ticks_left;
    int to_bonus;
} CatchAttempt;

typedef struct {
    stack* path;
    int pathLength;
    int speed;
} Movement;

typedef struct {
    int x;
    int y;
    ORIENTATION orientation;
    int is_strong;
    int remaining_attempts;
    int is_catched;
    int frozen;
    int is_invincible;
    int num_attempts;
    Movement movement;
    int path_timer;
    int dodge;
} Pikachu;



typedef struct {
    ENTITY_TYPE type;
    int x;
    int y;
    int ticks_left;
} Pokeball;

typedef struct {
    ENTITY_TYPE type;
    int x;
    int y;
    BONUS_TYPE bonus_type;
} Bonus;

typedef struct {
    ENTITY_TYPE type;
    int x;
    int y;
    int is_catchable;
} Obstacle;

typedef struct {
    ENTITY_TYPE type;
    int x;
    int y;
} EmptySpace;

typedef union {
    ENTITY_TYPE type;
    Pokeball pokeball;
    CatchAttempt catch_attempt;
    Bonus bonus;
    Obstacle obstacle;
    EmptySpace empty_space;
} Entity;

#endif
