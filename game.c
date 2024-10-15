
#include "game.h"


#define INVINCIBLE 0





int do_game_loop(Game* game) {
    int stop = 0;
    gui_start_timer();
    while(!stop) {
        Event ev;
        gui_game_event(&ev); 
        update_game(game);
        if (ev.type == EVENT_KEY) 
        {
            /* 0: UP, 1: DOWN, 2: LEFT, 3: RIGHT
               vb:  stap 0:  0101>>0 = 0101  ->   0101 & 0001 = 1 
                    stap 1:  0101>>1 = 0010  ->   0010 & 0001 = 0 
                    stap 2:  0101>>2 = 0001  ->   0001 & 0001 = 1 
                    stap 3:  0101>>3 = 0000  ->   0000 & 0001 = 0 
                                                                ^----- dit zijn de waarden die we zoeken */ 
            for (int i = 0; i < 4; i++) { 
                game->input.moves[i] = (ev.keyDownEvent.key >> i) & 1;    
            } 
            if (16 & ev.keyDownEvent.key) { game->input.drop_pokeball = true;} // space pressed                      
            if (32 & ev.keyDownEvent.key) { game->state = GAME_ENDED;}   // esc pressed                                        
        }           
        else if (ev.type == EVENT_TIMER) 
        {
            render_game(game);
        }             
        check_game_input(game);
        switch(game->state) 
        {
            case LAUNCHED: // most occuring, do nothing
                break; 

            case FINISHED:
                gui_draw_buffer();
                stop = 1;
                break;

            default:    // GAME_OVER or GAME_ENDED
                stop = 1;
                break;
        }                                                   
    }
    destroy_level(&game->level);
    return game->score;
}

void init_pikachu(Game* game, bool is_strong, int i) {
    int x,y;
    //get valid coord
    do {
        // generate 2 random coord with border not included
        x = random_number(1, game->level.level_info.width-3), y = random_number(1, game->level.level_info.height-3);
    } while (entity_type(x,y, OBSTACLE) || (!(x>8) && !(y>2)));

    //spawn pikachu
    stack* s = malloccheck(1, stack); // allocate empty path for pikachu 
    init_stack(s);
   
    Pikachu pikachu = { x * TILE_W,                              // x coord
                        y * TILE_W,                              // y coord
                        (ORIENTATION) rand() % 4,                // orientation
                        is_strong,                               // is_strong
                        0,                                       // remaining_attempts
                        0,                                       // is_caught
                        0,                                       // frozen
                        0,                                       // is invincible
                        (int) is_strong*2,                       // num_attempts
                        (Movement) {s, 0, 0}, // movement (type, path, length, speed)
                        0,                                       // path_timer, has to be a multiple of pathlength (to be changed)
                        0,     // dodge
                    };
    int speed, pathupdate = 0;
    game->pikachus[i] = pikachu;
    if (game->pikachus[i].is_strong) {   // apply appropriate arg depending on whether pikachu is strong or not
        speed = STRONG_PIKACHU_MOVEMENT_INCREMENT; 
        pathupdate = STRONG_PIKACHU_MOVEMENT_UPDATE; // strong pikachu moves faster, so has to update its path to player
    }                                                // quicker (TILE_DIM / strong_mov_speed) = 16 
    else {
        speed = PIKACHU_MOVEMENT_INCREMENT;
        pathupdate = PIKACHU_MOVEMENT_UPDATE; // moves slower than strong pikachu, (TILE_DIM / mov_speed (64/4) = 32
    }
    // allocate movement struct for pikachu 
    Movement movement = {s, pathupdate, speed}; 
    game->pikachus[i].movement = movement;  
    game->pikachus[i].path_timer = game->pikachus[i].movement.pathLength; // set path timer to start on path_length initially
    update_pikachu_path(game, i); // update pikachus path to player  
}

void init_game(Game* game, int level_nr) {
    LevelInfo level_info = generate_level_info(level_nr);
    *game = (Game)  { (Level) {},                // level
                      (Player) {TILE_W, TILE_H, SOUTH, 1, 4, 0, 0},  // player
                      (Pikachu*) NULL,           // pikachus
                      0, // pikachus_left
                      LAUNCHED,                  // state
                      (Input) {0, {0, 0, 0, 0}, 0},        // input
                      0,                         // score
                      (graph*) NULL,             // graaf
                      0,                        // pikachu_ai_timer
                      8,                         // how_smart_pikachus
                      1                         // amount of tiles pikachus should maintain their current movement 
                    };

    // initialises the level (entities), handles obstacles
    init_level(&(game->level), level_info);    

    // turn board into graph where each tile represents a node 
    board_to_graph(game);   

    int pikachus_amount = game->level.level_info.nr_of_pikachus;    
    game->pikachus_left = pikachus_amount + game->level.level_info.spawn_strong_pikachu;  
    game->pikachus = malloccheck(pikachus_amount + game->level.level_info.spawn_strong_pikachu, Pikachu);

    int i = 0;
    for(; i < pikachus_amount; i++) {
        init_pikachu(game, false, i);
    }
    pikachus_amount += game->level.level_info.spawn_strong_pikachu;
    for(; i < pikachus_amount; i++) {
        init_pikachu(game, true, i);
    }

    // initialize text drawn by gui
    gui_set_pikachus_left(game->pikachus_left);
    gui_set_pokeballs_left(game->player.remaining_pokeballs);
    gui_set_level_score(0);
    gui_clear_keys();
}

void destroy_game(Game* game) {
    // destroy (and free) all paths to player , allocated in update_pikachu_path -> shortest_path
    for(int i = 0; i < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu; i++) 
    {
        destroy_stack(game->pikachus[i].movement.path); 
    }
    free(game->pikachus); // free pikachus, allocated in init_game
    destroy_graph(game->graaf); // destroy (and free) graph, allocated in board_to_graph
    game->how_smart_pikachus = 0;
    game->pikachu_ai_timer = 0;
    game->pikachus_left = 0;
}

// in progress
void process_game_difficulty(Game* game) {
    int elapsed_time = gui_get_timer_score();

    // refresh board every minute, incase bugs have occured / too many things have happened
    if ((elapsed_time % 60) == 0 && (elapsed_time != 0)) {
        update_board(game);
    }

    // when pikachus are in hunting mode the difficulty will start increasing after 3 minutes
    // to be made exponential ..
    for (int minute = 3; minute < 7; minute++) 
    {
        if (elapsed_time > minute*60)  
        {
            // increase rate at which pikachus path to player is updated in case pikachu is in hunter mode
            game->how_smart_pikachus -= (minute-2) * game->how_smart_pikachus;
            if (game->how_smart_pikachus < 0) {
                game->how_smart_pikachus = 0;
            }
            
            // increase rate at which pikachus movement will be updated
            game->amt_of_tiles -= (minute-2);
            if (game->amt_of_tiles < 1) {
                game->amt_of_tiles = 1;
            } 

            goto next;
        }
    }
    next:;

    
}

void update_game(Game* game) {
    do_player_movement(game); 
    do_pikachu_ai(game);
    process_bonus_items(game);
    process_pokeballs(game);
    process_game_difficulty(game);
}


void render_player(Game* game) {
    gui_add_player(&(game->player)); // render player
    if (collision_box_look_around(game, NO_PIKACHU, CATCH_ATTEMPT)) { // checks whether player collides with catch_attempt
        set_player_caught(game); // set player as caught so game can be ended 
    }
}
void render_pikachus(Game* game) {
    for (int j = 0; j < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu; j++) {
        if (!game->pikachus[j].is_catched) {         // only render pikachu when pikachu isn't caught 
            if(!(game->pikachus[j].is_invincible % 2)) {
                gui_add_pikachu(&(game->pikachus[j]));  // render the j'th pikachu
            }
        }
    } 
}
void render_game(Game* game) {
    render_level(&(game->level)); // render_level handles render bonus, catchattempts, obstacles and pokeballs
    render_player(game); 
    render_pikachus(game); 
}

void check_game_input(Game* game) {
    // check whether one of the keys has been pressed
    if (sum(game->input.moves, 4) == 0) // if none of the keys have been pressed 
    { 
        game->input.has_moved = 0; // clear has_moved when player hasn't pressed any movement key
        game->player.auto_walking_tick = 0; 
    }
    else {
        game->input.has_moved = 1; // set has_moved when player has pressed atleast 1 movement key
    }    
    if (game->input.drop_pokeball == true)  // check whether drop_pokeball was set in game_loop
    {  
        if (game->player.remaining_pokeballs > 0) // make sure player has pokeballs available
        { 
            int* positie = coordinates_to_tile(game, game->player.x, game->player.y);  // get corrected tile position 
            set_pokeball(game, positie[0], positie[1], POKEBALL_TICKS); 
            update_board_position(game, positie[0], positie[1]);
            //update_board_catch_attempt(game, positie[0], positie[1]);  // update graph (to be changed, currently inefficient)
            if (game->how_smart_pikachus <= PIKACHU_AVOID_POKEBALL) // check whether pikachus their path to player should be updated 
            { 
                game->pikachu_ai_timer = 0;
            }                                 
          
            gui_set_pokeballs_left(game->player.remaining_pokeballs); 
            free(positie);
        }
        game->input.drop_pokeball = false;  // clear drop_pokeball, allow next to be processed (regardless of whether there are enough avail or not)
    }
}

void do_player_movement(Game* game) {
    int speed = PLAYER_MOVEMENT_INCREMENT; //Bonus speed?
    /*
    Massive if statement:
        If player moves in a direction, try moving perpendicular to this direction first
        If not, move parallel to this direction
        If player cant move, keep moving 2 ticks longer in the current direction
    */
    if(game->input.has_moved == 1) { 
        switch (game->player.orientation)
        {
        case NORTH:
            if (game->input.moves[LEFT]) {
                if(player_move(game, LEFT, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[RIGHT]) {
                if(player_move(game, RIGHT, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[UP]) {
                if(player_move(game, UP, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[DOWN]) {
                if(player_move(game, DOWN, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->player.auto_walking_tick++ < 1 + game->input.has_moved) {
                player_move(game, game->player.orientation, speed);
            } 
            break;

        case SOUTH:
            if (game->input.moves[RIGHT]) {
                if(player_move(game, RIGHT, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[LEFT]) {
                if(player_move(game, LEFT, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[DOWN]) {
                if(player_move(game, DOWN, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[UP]) {
                if(player_move(game, UP, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->player.auto_walking_tick++ < 1 + game->input.has_moved) {
                player_move(game, game->player.orientation, speed);
            } 
            break;

        case WEST:
            if (game->input.moves[DOWN]) {
                if(player_move(game, DOWN, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[UP]) {
                if(player_move(game, UP, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[LEFT]) {
                if(player_move(game, LEFT, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[RIGHT]) {
                if(player_move(game, RIGHT, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->player.auto_walking_tick++ < 1 + game->input.has_moved) {
                player_move(game, game->player.orientation, speed);
            } 
            break;

        case EAST:
            if (game->input.moves[UP]) {
                if(player_move(game, UP, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[DOWN]) {
                if(player_move(game, DOWN, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[RIGHT]) {
                if(player_move(game, RIGHT, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->input.moves[LEFT]) {
                if(player_move(game, LEFT, speed)){game->player.auto_walking_tick = 0;break;}
            }
            if (game->player.auto_walking_tick++ < 1 + game->input.has_moved) {
                player_move(game, game->player.orientation, speed);
            } 
            break;
        
        }
    }
}


void process_pikachu_collisions(Game* game, int which_pikachu)  {
    // checks whether current pikachu collides with player 
    bool collide_with_catch = collision_box_look_around(game, which_pikachu, CATCH_ATTEMPT); 
    
    // checks whether current pikachu collides with player        
    bool collide_with_player = collision_box_look_around(game, which_pikachu, CHECK_FOR_PLAYER); 
    
    if(!game->pikachus[which_pikachu].frozen) {
        if (collide_with_player) {
            set_player_caught(game);
        }
    }
    
    
    //add for strong pikachu - easy fix
    if(game->pikachus[which_pikachu].is_invincible) {
        game->pikachus[which_pikachu].is_invincible--;
    } else {
        if(collide_with_catch) {
            if(!game->pikachus[which_pikachu].num_attempts) {
                set_pikachu_caught(game, which_pikachu);
            } else {
                game->pikachus[which_pikachu].is_invincible = 64;
                game->pikachus[which_pikachu].num_attempts--;
            }
        }
    }
}


void do_pikachu_ai(Game* game) { 
    if (game->pikachu_ai_timer == 0) {  // check whether all pikachus their paths to player should be updated
        update_all_pikachu_paths(game); // to be rewritten so paths to player are updated individually
    }
    else {
        game->pikachu_ai_timer--;
    }
    int amt_of_pikachus = game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu;
    for (int which_pikachu = 0; which_pikachu < amt_of_pikachus; which_pikachu++) // iterate over all pikachus
    { 
        if (game->pikachus[which_pikachu].is_catched == 0) // only process pikachu movement when pikachu isn't caught
        {   
            process_pikachu_collisions(game, which_pikachu); // check collisions before checking whether pikachu is frozen to allow it to get caught
            if (game->pikachus[which_pikachu].frozen == 0)  // only process movement when pikachu isn't frozen
            {   
                // check whether pikachu collides with catchattempt and player
                if (is_pikachu_timer_expired(game, which_pikachu)) 
                {
                    if (game->level.level_info.level_nr < 20) {
                        int num_of_tiles = random_number(3,5);
                        int new_direction = random_number(0, 3);
                        game->pikachus[which_pikachu].orientation = new_direction;
                        pikachu_set_timer(game, which_pikachu, num_of_tiles * game->pikachus[which_pikachu].movement.pathLength); 
                    }
                    else { 
                        int num_of_tiles = 1; // how many tiles until current pikachus orientation will be updated 
                        int* current_pos = coordinates_to_tile(game, game->pikachus[which_pikachu].x, game->pikachus[which_pikachu].y);
                        if (game->pikachus[which_pikachu].dodge == 2) {
                            freeze_pikachu(game, (CATCH_ATTEMPT_TICKS/(3))*4, which_pikachu);
                            game->pikachus[which_pikachu].dodge = 0;
                        }

                        // only process dodge when pikachu hasn't found new path to player
                        if (game->pikachus[which_pikachu].dodge == 1 && game->pikachus[which_pikachu].movement.path->size == 0) 
                        { 
                            game->pikachus[which_pikachu].orientation = inverse_direction(game->pikachus[which_pikachu].orientation);
                            num_of_tiles = 2; // make sure pikachu is dodging for long enough
                            game->pikachus[which_pikachu].dodge = 2; 
                        }
                        else 
                        {
                            // if pikachu shouldn't dodge, move pikachu to next tile along path to player 
                            move_pikachu_to_player(game, which_pikachu); 
                        }

                        // update pikachus movement in num_of_tiles tiles (pathlength * num_of_tiles ticks)
                        pikachu_set_timer(game, which_pikachu, num_of_tiles * game->pikachus[which_pikachu].movement.pathLength); 

                        free(current_pos);
                    }
                }
                else // when pikachu shouldn't update movement yet decrease timer by 1 tick and move according to current orientation 
                { 
                    pikachu_decrease_timer(game, which_pikachu, false); 
                    pikachu_move(game, which_pikachu, (int)game->pikachus[which_pikachu].orientation, game->pikachus[which_pikachu].movement.speed); // move pikachu (handles collision)
                }
            }
            else // when pikachu is frozen decrease a tick
            { 
                game->pikachus[which_pikachu].frozen--;
            }
        }
    }
    if (game->pikachus_left == 0) {
        game->state = FINISHED;
    }
}
/* freeze pikachus for freeze_duration ticks */
void freeze_pikachu(Game* game, int freeze_duration, int which_pikachu) {
    if (which_pikachu >= 0 && which_pikachu < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu) { // check whether index is valid 
        game->pikachus[which_pikachu].frozen = freeze_duration;
    }
}

/* move pikachu to next tile on its path to player */
void move_pikachu_to_player(Game* game, int which_one) {
    // get correct coordinates of pikachu
    int* current_pos = coordinates_to_tile(game, game->pikachus[which_one].x, game->pikachus[which_one].y); 

    // returns latest coordinates of next tile pikachu should move to (of path to player)
    int* new_coord = stack_pop_coords(game->graaf, game->pikachus[which_one].movement.path); 

    // if the next tile pikachu should move to == current tile, get a new tile 
    if (new_coord[0] == current_pos[0] && new_coord[1] == current_pos[1]) {  
        free(new_coord); 
        new_coord = stack_pop_coords(game->graaf, game->pikachus[which_one].movement.path);
    }
    // move pikachu to new tile
    move_pikachu_to_tile(game, new_coord[0], new_coord[1], which_one); 
    free(new_coord);
    free(current_pos);
}

// in progress 
void process_pokeballs(Game *game) { 
    for (int i = 1; i < game->level.level_info.width-1; i++) {
        for (int j = 1; j < game->level.level_info.height-1; j++) {
            if (entity_type(i, j, POKEBALL)) 
            {                        
                // when pokeball timer is expired replace it by catch attempt                
                if (is_pokeball_timer_expired(game, i, j)) 
                {     
                    set_catchattempt(game, 
                                            i * TILE_DIM,       // x coord
                                            j * TILE_DIM,       // y coord
                                            1, 1, 1, 1,         // spread, uniform in all directions
                                            game->player.pokeball_power, // how far the catchattempt will go 
                                            CATCH_ATTEMPT_TICKS, // how long before the catch attempt expires
                                            REPLACE_BY_EMPTYSPACE);  // replace by empty space as center of catch attempt can only start on empty space
                                                                     // so no bonus can be spawned (unless a pikachu is caught, which is handled by pikachu ai)

                    // process catch attempts in all directions (UP, DOWN, LEFT, RIGHT)
                    for (int direction = UP; direction < 4; direction++) { 
                        process_catchattempt(game, i, j, direction);
                    }

                    update_board_position(game, i, j);
                    if (game->how_smart_pikachus <= PIKACHU_AVOID_POKEBALL) { 
                        game->pikachu_ai_timer = 0;
                    }    
                }   
                else 
                {
                    pokeball_decrease_timer(game, i, j, false);       
                }
        
            }
            else if (entity_type(i, j, CATCH_ATTEMPT)) 
            {                                
                if (is_catchattempt_timer_expired(game, i, j)) // catchattempt timer has expired so now we can process it
                { 
                    /*
                     make sure we only give a pokeball back when catchattempt is at the  
                     center coord (initial place it was put) otherwise we give back 
                    (spread[UP] + spread[DOWN] + spread[LEFT] + spread[RIGHT]) * power too many pokeballs back..
                    */
                    if (game->level.entities[i][j].catch_attempt.power != 0) 
                    {  
                        game->player.remaining_pokeballs++;             // increase #pokeballs player has
                        gui_set_pokeballs_left(game->player.remaining_pokeballs);   // update #pokeballs on GUI
                    }
                    if (game->how_smart_pikachus <= PIKACHU_AVOID_POKEBALL) 
                    { 
                        game->pikachu_ai_timer = 0;
                    }
                    // decided in process_catchattempt when catch was put down initially)
                    if (game->level.entities[i][j].catch_attempt.to_bonus == REPLACE_BY_BONUS) { 
                        set_bonus(game, i * TILE_SIZE, j * TILE_SIZE); // set bonus, which type is decided randomly in set_bonus
                    }
                    else {
                        set_emptyspace(game, i*TILE_SIZE, j*TILE_SIZE);   // otherwise replace the catchattempt by empty space 
                    }
                    
                    look_around_tile(game, game->graaf, i, j);  
                }
                else // catchattempt hasn't expired yet so we can continue
                {                                                         
                    catchattempt_decrease_timer(game, i, j, false);  
                }  
            }
        }
    }
}
void process_catchattempt(Game* game, int i, int j, int direction) {

    // whether we should process any spread in appropriate direction, assumed to be 0 or 1
    int spread_direction = game->level.entities[i][j].catch_attempt.spread[direction];

    // decides how far in appropriate direction the spread will go (if spread is not 0)
    
    int power = game->level.entities[i][j].catch_attempt.power;
    int amt_of_tiles = 0;
    if (spread_direction != 0) 
    {
        amt_of_tiles = power * spread_direction; // spread direction always 1 or 0
        int x = i; int y = j; int tile_nr = 0;
        bool keep_going = true; 
        while (tile_nr < amt_of_tiles) // iterate over all tiles in the appropriate direction
        {    
            bool replace_by_bonus = REPLACE_BY_EMPTYSPACE;  // default case is to replace attempt by empty space when expired
            direction_to_coords(&x, &y, 1, direction);      // move one tile in the right direction

            // when we are out of bounds stop going any further
            if (!isValidCoord(x, y, game->level.level_info.width - 1, game->level.level_info.height - 1)) 
            {
                return;
            }
            // (these checks have to be separate or else we get a bug from checking is_catchable on catchattempt (garbage value)
            if (entity_type(x, y, EMPTY_SPACE)) 
            {
                keep_going = true; 
            }
            else if (entity_type(x, y, OBSTACLE)) 
            {        
                if (game->level.entities[x][y].obstacle.is_catchable)   // make sure we aren't on an uncatchable obstacle 
                {
                    keep_going = false;
                    // check whether we should replace obstacle later on by bonus when catch attempt expires
                    if (random_number_fl() > (game->level.level_info.bonus_spawn_ratio)) { 
                        replace_by_bonus = REPLACE_BY_BONUS;
                    } 
                }  
                else {return;}                     
            }
            set_catchattempt(game, 
                                x * TILE_DIM,   // x pos    
                                y * TILE_DIM,   // y pos
                                0, 0, 0, 0,     // as this is a catch_attempt which is part of the blast no further spread is required
                                0,              // no power either
                                CATCH_ATTEMPT_TICKS,
                                replace_by_bonus
                            );
            update_board_position(game, x, y);
            if (keep_going == false) {return;}
            tile_nr++;
        }
    }
}

void pikachus_dodge(Game* game) {
    for (int i = 0; i < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu; i++) {
        game->pikachus[i].dodge = 1;
    }  
}

bool player_move(Game* game, int direction, int speed) {
    int x_old = game->player.x;
    int y_old = game->player.y;
    int x_new = x_old;
    int y_new = y_old;
    direction_to_coords(&x_new, &y_new, speed, direction);
    player_set_position(game, x_new, y_new);
    if (collision_box_look_around(game, NO_PIKACHU, OBSTACLE) ) { /* als er collision is gedetecteerd, zet oude positie terug */
        player_set_position(game, x_old, y_old);
        return false;
    }
    game->player.orientation = direction;
    return true;
}



void update_pikachu_path(Game* game, int which_pikachu) {
    int* player_pos = coordinates_to_tile(game, game->player.x, game->player.y);
    int* pikachu_pos = coordinates_to_tile(game, game->pikachus[which_pikachu].x, game->pikachus[which_pikachu].y);
    if (player_pos[0] != -1 && pikachu_pos[0] != -1) {
        stack* new_path;
        new_path = shortest_path(game->graaf, 
                                 pikachu_pos[0], pikachu_pos[1], 
                                 player_pos[0], player_pos[1], 
                                 which_pikachu
                                ); 
        destroy_stack(game->pikachus[which_pikachu].movement.path);
        game->pikachus[which_pikachu].movement.path = new_path; /* update huidige pikachu path */
    }
    free(player_pos);
    free(pikachu_pos);  
    
}

/* bereken op welke tile entity/player/pikachu werkelijk zit, zie plafoneer_pos(), returnt -1 wanneer coordinaten niet valid zijn */
int* coordinates_to_tile(Game* game, int x, int y) {
    int* positie;
    if (isValidCoord(x, y, game->level.level_info.width * TILE_DIM, game->level.level_info.height * TILE_DIM)) {
        positie = malloccheck(2, int);
        int f_x = plafoneer_pos(x); 
        int f_y = plafoneer_pos(y);  
        int index_x = (x / TILE_DIM) + f_x; 
        int index_y = (y / TILE_DIM) + f_y;
        positie[0] = index_x; positie[1] = index_y;
    }
    else {
        positie = malloccheck(1, int);
        positie[0] = -1;
    }
    return positie;
} 

/* assuming tile is next to current tile (left, right, up, down) and changes direction accordingly */
void move_pikachu_to_tile(Game* game, int i, int j, int which_pikachu) {
    int* pikachu_now = coordinates_to_tile(game, game->pikachus[which_pikachu].x, game->pikachus[which_pikachu].y);
    int pikachu_x = pikachu_now[0]; int pikachu_y = pikachu_now[1];
    if (pikachu_x != -1) {
        if (pikachu_x == i) {       // if x coords are equal the next tile is either above or below 
            if (pikachu_y > j) {        // if current pikachu y coord is greater than next tile, new tile is above 
                game->pikachus[which_pikachu].orientation = UP;
            }
            else {                      // if new tile isn't above then it is below
                game->pikachus[which_pikachu].orientation = DOWN;   
            }
        }   
        else {                     // not above or below so can only be left or right
            if (pikachu_x > i) {        // if current pikachu x coord is greater than next tile, new tile is to the left 
                game->pikachus[which_pikachu].orientation = LEFT;
            }
            else {                      // if new tile isn't above then it is to the right
                game->pikachus[which_pikachu].orientation = RIGHT; 
            }
        }
    }
    free(pikachu_now);
}
int inverse_direction(int direction) {
    switch(direction)  
    {  
        case UP: return DOWN; break;
        case DOWN: return UP; break;
        case RIGHT: return LEFT; break;
        case LEFT: return RIGHT; break;
        default: return -1; break;
    }
}

// in progress
void pikachu_move(Game* game, int which_pikachu, int direction, int speed) {
    int x_old = game->pikachus[which_pikachu].x;
    int y_old = game->pikachus[which_pikachu].y;
    int x_new = x_old;
    int y_new = y_old;
    int x_new_forward = x_old; 
    int y_new_forward = y_old;
    direction_to_coords(&x_new, &y_new, speed, direction);  // update coordinates based on pikachus orientation and speed 
    direction_to_coords(&x_new_forward, &y_new_forward, speed*3, direction); 

    pikachu_set_position(game, which_pikachu, x_new, y_new); 
    bool collide_with_obstacle = collision_box_look_around(game, which_pikachu, OBSTACLE);
    bool collide_with_pokeball = collision_box_look_around(game, which_pikachu, POKEBALL);

    pikachu_set_position(game, which_pikachu, x_new_forward, y_new_forward); 
    bool collide_with_catchattempt = collision_box_look_around(game, which_pikachu, CATCH_ATTEMPT);

    int num_of_tiles = 1; // amt of tiles to maintain path after colliding 
    int new_direction = 0;
    int inv_dir = inverse_direction(direction);
    if (collide_with_obstacle ) {
        num_of_tiles = 2;
        new_direction = random_excl(0, 3, direction);
        if (game->pikachus[which_pikachu].dodge == 2) { // if collision is detected while a pikachu is trying to dodge: freeze pikachu in place
            //freeze_pikachu(game, (CATCH_ATTEMPT_TICKS / 3) * 4, which_pikachu);  // to avoid pikachu from going into catch attempt
            new_direction = random_excl(0, 3, inv_dir);
        }                                                                      
    }
    else if (collide_with_pokeball) {
        game->pikachus[which_pikachu].dodge = 2;
        num_of_tiles = 2;
        new_direction = inv_dir;
    }
    if (collide_with_catchattempt && game->how_smart_pikachus < 14) { 
        pikachu_set_position(game, which_pikachu, x_old, y_old);
        freeze_pikachu(game, CATCH_ATTEMPT_TICKS, which_pikachu);
    }
    else if (collide_with_obstacle || collide_with_pokeball) {
        pikachu_set_position(game, which_pikachu, x_old, y_old);  /* when collision is detected set position back */
        game->pikachus[which_pikachu].orientation = new_direction;
        pikachu_set_timer(game, which_pikachu, num_of_tiles*game->pikachus[which_pikachu].movement.pathLength); // maintain random movement for num_of_tiles tiles   
    }
    else {
        pikachu_set_position(game, which_pikachu, x_new, y_new);
    }
}                                                               

void update_all_pikachu_paths(Game* game) {
    for (int i = 0; i < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu; i++) {
        if (game->pikachus[i].is_catched == 0)  { // only update pikachus path when pikachu isn't caught
            update_pikachu_path(game, i);        
        }
    } 
    if (game->how_smart_pikachus > 0) { // make sure how_smart_pikachus is positive number
        game->pikachu_ai_timer = game->how_smart_pikachus; // reset timer to initial value
    }
}

bool wait_for_space() {
    while(true) {
        Event ev;
        gui_highscore_event(&ev);
        switch (ev.keyDownEvent.key)
        {
        case GUI_CHAR_ENTER:
        case GUI_CHAR_SPACE:
            return 0;
        
        case GUI_CHAR_ESC:
            return 1;

        default:
            break;
        }
    }
}

// handles the transition from current level to next level dpending on game state
void level_transition(Game* game, HighScoreTable* highscores, int total_score) {
    int stop = 0;
    int level_nr = game->level.level_info.level_nr;
    
    while (stop == 0) {
        Event ev;
        gui_game_event(&ev);
        if (game->state == FINISHED) { // game finished, increase lvl and wait for space
            gui_set_finished_level(total_score, game->score);
            gui_draw_buffer();
            level_nr += 1;
            stop = 1;
        }
        else if (game->state == GAME_OVER) {
            gui_set_game_over(highscores);
            gui_draw_buffer();
            if(check_highscore_entry(highscores, total_score)) {
                //Game closed
                game->state = GAME_ENDED;
                return;
            }
            level_nr = 1;
            stop = 1;
        }
    }
    if(game->state == GAME_ENDED) {return;} //return if window closed
    if(wait_for_space()) {game->state = GAME_ENDED;return;} //return if window closed
    init_game(game, level_nr);
}

void process_bonus_items(Game* game) {

    //You can only enter new tile if you are in the middle of current tile
    Entity* tile;
    int x = game->player.x, y = game->player.y;
    //restore old position
    switch (game->player.orientation)
    {
    case NORTH:y -= 31;break;
    case SOUTH:y += 32;break;
    case EAST:x += 32;break;
    case WEST:x -= 31;break;
    }
    //old position + tile direction to check if player enters new tile
    int* player_coord = coordinates_to_tile(game, x, y); // get tile player is on right now
    if (player_coord[0] != -1) { 
        x = player_coord[0]; y = player_coord[1];
    } else {return;}
    free(player_coord);

    //Get new tile
    switch(game->player.orientation) 
    {
        case SOUTH:tile = &(game->level.entities[x][y]);break;
        case WEST:tile = &(game->level.entities[x][y]);break;
        case NORTH:tile = &(game->level.entities[x][y]);break;
        case EAST:tile = &(game->level.entities[x][y]);break;
    }
    if(tile->type != BONUS) {return;}

    //Process bonus
    switch (tile->bonus.bonus_type)
    {
    case EXTRA_POWER: 
        if (game->player.pokeball_power < POKEBALL_MAX_POWER) {   // make sure we power isn't too big 
            game->player.pokeball_power++;
        }
        break;
    
    case EXTRA_POKEBALL:
        game->player.remaining_pokeballs++;
        gui_set_pokeballs_left(game->player.remaining_pokeballs); 
        break;
    
    case FREEZE_PIKACHUS:
        for(int i = 0; i < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu; i++) {
            freeze_pikachu(game, FREEZE_DURATION, i);
        }
        break;
    
    default:
        break;
    }
    set_emptyspace(game, tile->bonus.x, tile->bonus.y);
}


/****************** algemene hulp / board functies **************/


bool is_path_collision(Game* game, node* path, int i, int j, int which_pikachu) {
    if (!game->pikachus[which_pikachu].is_catched) 
    {
        node* temp = path;
        while (temp->next_node != NULL) {
            if (temp->label == label(i, j, game->level.level_info.width)) {
                return true;
            }
            temp = temp->next_node;
        }
    }
    return false;
}




void direction_to_coords(int* x, int* y, int offset, int direction) {
    switch(direction) {
        case(UP):
            *y = *y - offset;
            break;
        case(DOWN): 
            *y = *y + offset;
            break;
        case(LEFT):
            *x = *x - offset;
            break;
        case(RIGHT):
            *x = *x + offset;
            break;
    }
}

/* this function generates a bigger square consisting of boxes around entity/player/pikachu, makes sure that */
/* you can't glitch through objects by using certain movement combinations,              */
/*  while still being more efficient than iterating over every single tile on the board  */
/*  it does so like this:                   x x x                                        */  
/*  where x represents a box                x P x                                        */
/*  and P the entity/player/pikachu         x x x                                        */


bool collision_box_look_around(Game* game, int which_pikachu, ENTITY_TYPE collision_type) {    
    ALLEGRO_COLOR color_pink = al_map_rgba_f(1, 0, 1, 1); // testing purposes
    ALLEGRO_COLOR color_blue = al_map_rgba_f(0, 0, 1, 1);      
    ALLEGRO_COLOR color_red = al_map_rgba_f(1, 0, 0, 1);          
    int box_offset_strength = 0;
    int x; int y;
    if (which_pikachu == NO_PIKACHU) {  /* als which_pikachu op -1 staat bekijken we player ipv een pikachu */
        x = game->player.x;
        y = game->player.y;
        box_offset_strength = BOX_OFFSET_PLAYER;  /* deze waarde bepaalt hoe streng de collision is */
    }
    else {
        x = game->pikachus[which_pikachu].x;
        y = game->pikachus[which_pikachu].y;
        box_offset_strength = BOX_OFFSET_PIKACHU; 
    }
    int* coord = coordinates_to_tile(game, x, y);
    int player_i = coord[0]; int player_j = coord[1];
    if (coord[0] != -1 && coord[1] != -1) {
        if (collision_type != CHECK_FOR_PLAYER) {       /* wanneer niet we niet vergelijken met player wordt oftewel met object, catch tile of pokeball gecheckt */
            for (int i = player_i - 1; i < player_i + 2; i++) {       /* itereer enkel over de boxes rond positie player, veel efficienter dan over alle tiles te itereren en */
                for (int j = player_j - 1; j < player_j + 2; j++) {   /* zo telkens voor collision te checken */
                    if( isCollision(game, i * TILE_SIZE,  j * TILE_SIZE,  x, y, box_offset_strength , collision_type)) {
                        free(coord);
                        return true; /* return true wanneer eerste keer collision wordt gedetecteerd */ 
                    }
                }
            }
        }
        else {
            if(isCollision(game, game->player.x, game->player.y, x, y, box_offset_strength , -1)) {  /* dit wordt gebruikt voor de pikachus om de detecteren of ze colliden met player */
                free(coord);
                return true;   /* return true wanneer eerste keer collision wordt gedetecteerd */  
            }
        }    
        free(coord);                           
     
        return false; /* wanneer geen enkele keer collision wordt gedetecteerd return false */
    } 
    free(coord);
    return false;
}

int sum(int* array, int length) {
    int i = 0;
    int sum = 0;
    for (i = 0; i < length; i++) {
        sum += array[i];
    }
    return sum;
}
/* generate random number between [low, high) which can't include number excl */
int random_excl(int low, int high, int excl) {
    int number;
    bool flag = false;
    do {
        number = random_number(low, high);
        if (number != excl) {
            flag = true;
        }
    } while (flag == false);
    return number;
}

int random_number(int low, int high) {
    return (rand()%(high+1)) + low;
}

/* random number between 0 and 1 */
float random_number_fl() { 
    return (float)((float)(rand()%100) / 100);
}

int max(const int a, const int b) {
    if (a > b) {
        return a;
    }
    else {
        return b;
    }
}

/*  generates a graph for current game board. turns the every tile on the board into a node and processes        */
/*  edges appropriately. when processing edges it suffices to only check positions whose sum (of row and column) */
/*  are even considering we look in all 4 direction from the node we check                                  */
/*  example:    x o x o x o     here every o and x are a tile on the board which are turnt into a node      */
/*              o x o x o x     however we only iterate over the x's to process edges                       */
/*              x o x o x o                                                                                 */

void board_to_graph(Game* game) {                   
  int lvl_width = game->level.level_info.width;
  int lvl_height = game->level.level_info.height;
  graph* graph_t; 
  graph_t = init_graph((lvl_width), (lvl_height));      // properly initialises graph and allocates memory 
  for(int x = 1; x < game->level.level_info.width-1; x++) {     // iterate over all tiles except the border
    for(int y = 1; y < game->level.level_info.height-1; y++) {
        if (!entity_type(x, y, OBSTACLE) && ( (( (x+y) % 2) == 0))) {  // when current tile is not an object and has an even position
            look_around_tile(game, graph_t, x, y);  // looks in all 4 directions from (x,y) coord and adds edges to graph accordingly
        }
    }
  }
  game->graaf = graph_t;  
}

/* this is inefficient as we regenerate entire board. */
void update_board(Game* game) {   
    destroy_graph(game->graaf);   // clean up current graph & destroy current graph
    board_to_graph(game);         // turn board into new graph 
}

/* IN PROGRESS, comments to be added                                      */
/* update graph of the board at (x,y) coordinates, used to remove edges.  */
void update_board_position(Game* game, int x, int y) {
    if (isValidCoord(x, y, game->level.level_info.width, game->level.level_info.height)) {
        if (entity_type(x, y, CATCH_ATTEMPT) || entity_type(x, y, POKEBALL)) {
            if (isValidCoord(x+1, y, game->level.level_info.width, game->level.level_info.height)) {
                remove_edge_undirected(game->graaf, 
                                       x, y,
                                       x + 1, y
                                       );
            }
            if (isValidCoord(x-1, y, game->level.level_info.width, game->level.level_info.height)) {
                remove_edge_undirected(game->graaf, 
                                       x, y,
                                       x - 1, y
                                       );
            }
            if (isValidCoord(x, y+1, game->level.level_info.width, game->level.level_info.height)) {
                remove_edge_undirected(game->graaf, 
                                       x, y,
                                       x, y + 1
                                       );
            }
            if (isValidCoord(x, y-1, game->level.level_info.width, game->level.level_info.height)) {
                remove_edge_undirected(game->graaf, 
                                       x, y,
                                       x, y - 1
                                       );
            }
        }
    }  
}

/* look in every direction of a node at coordinates (x,y) */
void look_around_tile(Game* game, graph* g, int x, int y) {
  for (int i = 0; i < 4; i++) {
    look_direction(game, g, x, y, i);
  }
}

/* used in look_around_tile, adds edge when no obstacle is detected at new coordinate */

void look_direction(Game* game, graph* g, int x, int y, int direction) {
  int x_old = x; int y_old = y; // save current x,y coordinates
  direction_to_coords(&x, &y, 1, direction); // change x,y coordinates to new ones according to direction, 1 up, 1 down, 1 left or 1 right
    if (isValidCoord(x, y, game->level.level_info.width, game->level.level_info.height)) { // make sure the coordinate is valid
        if (game->how_smart_pikachus > PIKACHU_AVOID_POKEBALL) {  // if pikachus don't have to avoid pokeballs, only check against obstacles
            if ( (!entity_type(x, y, OBSTACLE)) && (!entity_type(x, y, -1)) ) {
                add_edge(g, 
                         x_old, y_old, 
                         x, y, 
                         UNDIRECTED
                         );
            } 
       }
       else {   // if pikachus do avoid pokeballs, also check against pokeballs and if there are pokeballs detected don't add edges 
            if ( (!entity_type(x, y, OBSTACLE)) && (!entity_type(x, y, -1)) && (!entity_type(x, y, POKEBALL))) {
                add_edge(g, 
                         x_old, y_old, 
                         x, y, 
                         UNDIRECTED
                         );
            } 
        }
    }
}

/* IN PROGRESS */
void update_board_catch_attempt(Game* game, int i, int j) {
    update_board_position(game, i, j+1);
    update_board_position(game, i, j-1);
    update_board_position(game, i+1, j);
    update_board_position(game, i-1, j);
    update_board_position(game, i, j);
}

/* checks when 2 squares centered around (x1, y1) ; (x2, y2) with box_offset as length from center           */ 
/* to either sides collision_type set to -1 when no type check has to be performed (generic collision check) */

bool isCollision(Game* game, int x1, int y1, int x2, int y2, int box_offset, ENTITY_TYPE collision_type) { 
    if (box_offset > 64) { // make sure length is valid 
        return false;
    }
    int boxsize = TILE_DIM - box_offset; 
    //               UP               DOWN            RIGHT             LEFT
    int box1[4] = {y1 - boxsize/2,  y1 + boxsize/2,  x1 + boxsize/2,  x1 - boxsize/2}; // first box
    int box2[4] = {y2 - boxsize/2,  y2 + boxsize/2,  x2 + boxsize/2,  x2 - boxsize/2}; // second box
    // each condition is one for which the boxes don't overlap. the boxes overlap only when none of these conditions are met 
    // IMPORTANT: UP and ABOVE are "inverted" in the sense that a coordinate "below" another coordinate has a greater y coordinate 
    if ( !( (box1[UP] >= box2[DOWN]) ||       // upper  corner  BOX1  above            bottom corner   BOX 2
            (box1[LEFT] >= box2[RIGHT]) ||    // left   corner  BOX1  to the right of  right  corner   BOX 2
            (box1[RIGHT] <= box2[LEFT]) ||    // right  corner  BOX1  to the left of   left   corner   BOX 2
            (box1[DOWN] <= box2[UP])) ) {     // bottom corner  BOX1  below            upper  corner   BOX 2
        if (collision_type == -1) {  // check whether generic generic collision check is required or type check
            return true;
        }
        // coordinates already verified by caller
        else if (entity_type(x1 / TILE_DIM, y1 / TILE_DIM, collision_type)) {
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}




/************ entities/pikachu/player hulp functies *******************************/
void pikachu_set_position(Game* game, int which_pikachu, int x_coord, int y_coord) {
    if (isValidCoord(x_coord, y_coord, game->level.level_info.width * TILE_DIM, game->level.level_info.height*TILE_DIM)) {
        game->pikachus[which_pikachu].x = x_coord;
        game->pikachus[which_pikachu].y = y_coord;
    }
    
}
void pikachu_set_timer(Game* game, int which_pikachu, int value) {
    game->pikachus[which_pikachu].path_timer = value;
}

void set_player_caught(Game* game) {
    if (INVINCIBLE == 0) {
        game->player.is_catched = 1;
        game->state = GAME_OVER;
    }
}

void set_pikachu_caught(Game* game, int which_pikachu) {
    if (which_pikachu >= 0 && which_pikachu < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu) {
        if (game->pikachus_left >= 0) {
            int* current_position = coordinates_to_tile(game, game->pikachus[which_pikachu].x, game->pikachus[which_pikachu].y);
            if (current_position[0] != -1) {
                float rand_number = random_number_fl();
                if (rand_number > (1 - game->level.level_info.bonus_spawn_ratio)) {  // check if we should replace tile pikachu died on with bonus
                    set_bonus(game, current_position[0]*TILE_DIM, current_position[1]*TILE_DIM);
                }   
            } 
            if (game->pikachus[which_pikachu].is_strong) 
            {
                if (game->pikachus[which_pikachu].num_attempts == 0) {
                    game->pikachus[which_pikachu].is_catched = 1;
                    game->score += STRONG_PIKACHU_SCORE;
                }
                else {
                    game->pikachus[which_pikachu].num_attempts--;
                }
            }     
            else {
                game->score += PIKACHU_SCORE;
            }
            game->pikachus_left--; 
            gui_set_pikachus_left(game->pikachus_left);
            gui_set_level_score(game->score);
            game->pikachus[which_pikachu].is_catched = 1;
            free(current_position); 
        }
    }   
}

void set_emptyspace(Game* game, int x, int y) {
    if (isValidCoord(x, y, game->level.level_info.width * TILE_DIM, game->level.level_info.height*TILE_DIM)) {
        game->level.entities[x / (TILE_SIZE) ][y / (TILE_SIZE)].empty_space = (EmptySpace) {EMPTY_SPACE, x, y};
    }
}

void set_obstacle(Game* game, int x, int y, int is_catchable) {
    if (isValidCoord(x, y, game->level.level_info.width * TILE_DIM, game->level.level_info.height*TILE_DIM)) {
        game->level.entities[x / (TILE_SIZE) ][y / (TILE_SIZE)].obstacle = (Obstacle) {OBSTACLE, x, y, is_catchable};
    }
}

void set_bonus(Game* game, int x, int y) {
    BONUS_TYPE bonus_type = (BONUS_TYPE) random_number(0, 2);
    if (isValidCoord(x, y, game->level.level_info.width * TILE_DIM, game->level.level_info.height*TILE_DIM)) {
        game->level.entities[x / (TILE_SIZE) ][y / (TILE_SIZE)].bonus = (Bonus) {BONUS, x, y, bonus_type};
    }
}

void set_catchattempt(Game* game, int x, int y, int prop_up, int prop_down, int prop_right, int prop_left, int power, int ticks_left, bool to_bonus) {
    if (isValidCoord(x, y, game->level.level_info.width * TILE_DIM, game->level.level_info.height*TILE_DIM)) {
        game->level.entities[x / (TILE_SIZE) ][y / (TILE_SIZE)].catch_attempt = (CatchAttempt) {CATCH_ATTEMPT, x, y, {prop_up, prop_down, prop_right, prop_left}, power, ticks_left, to_bonus}; 
    }
}

void set_pokeball(Game* game, int x, int y, int ticks_left) {
    if (isValidCoord(x, y, game->level.level_info.width, game->level.level_info.height)) {
        if(game->level.entities[x][y].type != POKEBALL) {
            game->level.entities[x][y].pokeball = (Pokeball) {POKEBALL, x,y, ticks_left};
            game->player.remaining_pokeballs--;
        }
    }
}

/* dec entity timer, gebruikt in pikachu_ai en process pokeball, laatste arg genegeerd wanneer geen pikachu (out of bounds wordt gecheckt) */
/* als set_negative true is wordt timer direct op -1 gezet */
void pokeball_decrease_timer(Game* game, int i, int j, bool set_negative) {
    if (set_negative) {
        game->level.entities[i][j].pokeball.ticks_left = 0;
    }
    else {
        game->level.entities[i][j].pokeball.ticks_left--;
    }
}

void catchattempt_decrease_timer(Game* game, int i, int j, bool set_negative) {
    if (set_negative) {
        game->level.entities[i][j].catch_attempt.ticks_left = 0;
    }
    else {
        game->level.entities[i][j].catch_attempt.ticks_left--;
    }
}

void pikachu_decrease_timer(Game* game, int which_pikachu, bool set_negative) {
    if (which_pikachu >= 0 && which_pikachu < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu) {
        if (set_negative) {
            game->pikachus[which_pikachu].path_timer = 0;
        }
        else {
            game->pikachus[which_pikachu].path_timer--;
        }  
    }
}

bool is_pikachu_timer_expired(Game* game, int which_pikachu) {
    if (which_pikachu >= 0 && which_pikachu < game->level.level_info.nr_of_pikachus + game->level.level_info.spawn_strong_pikachu) {
        if (game->pikachus[which_pikachu].path_timer == 0) {
            return true;
        }
    }
    return false;
}

bool is_pokeball_timer_expired(Game* game, int i, int j) {
    if (game->level.entities[i][j].pokeball.ticks_left == 0) {
        return true;
    }
    return false;
}

bool is_catchattempt_timer_expired(Game* game, int i, int j) {
    if (game->level.entities[i][j].catch_attempt.ticks_left == 0) {
        return true;
    }
    return false;
}

void player_set_position(Game* game, int x_coord, int y_coord) {
    if (isValidCoord(x_coord, y_coord, game->level.level_info.width*TILE_DIM, game->level.level_info.height*TILE_DIM)) {
        game->player.x = x_coord;
        game->player.y = y_coord;
    }
}
