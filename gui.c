#include "gui.h"

int ready = 0;

ALLEGRO_DISPLAY* display = NULL;
ALLEGRO_BITMAP* tile = NULL;
ALLEGRO_FONT* font = NULL;
ALLEGRO_FONT* font2 = NULL;
ALLEGRO_TIMER* timer = NULL;
ALLEGRO_EVENT_QUEUE* queue =  NULL;

int fps = 0, fps_accum = 0;
double fps_time = 0.0;

int keys = 0;

int is_game_over = 0, is_finished = 0; int request_name = 0;

int pokeballs_left = 0, pikachus_left = 0;

int level_score = 0, total_score = 0;

int level_time = 0, tick_time = 0, walk_time = 0, step_time = 0;

int screen_w = 0, screen_h = 0;

LevelInfo* cur_level = NULL;

char* name = NULL;


HighScoreTable* highscores = NULL;




int** catch_attempts = NULL;

void gui_initialize() {
    int i;

    al_init();
    al_init_image_addon();
    al_init_primitives_addon();
    al_init_ttf_addon();
    al_install_keyboard();

    font = al_load_ttf_font("fonts/digital.ttf", 30, 0);
    font2 = al_load_ttf_font("fonts/digital.ttf", 60, 0);

    if(font == NULL) {
        fprintf(stderr, "Failed to load font!\n");
        exit(-1);
    }
 
   tile = al_load_bitmap("images/icons.png");
    if(tile == NULL) {
        fprintf(stderr, "Failed to load icons!\n");
        exit(-1);
    }
    
    al_set_new_display_option(ALLEGRO_SUPPORT_SEPARATE_ALPHA, 1, ALLEGRO_REQUIRE); 
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_REQUIRE);

    al_set_new_display_flags(ALLEGRO_WINDOWED);
    display = al_create_display(640, 480);
    if(display == NULL) {
        fprintf(stderr, "Failed to create display!\n");
        exit(-1);        
    }
    al_set_window_title(display, "Project PGM");

    al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
 
    timer = al_create_timer(1.0 / TICK_PER_S);
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);

    catch_attempts = (int**) calloc(MAX_LEVEL_WIDTH, sizeof(int*));
    for(i = 0; i < MAX_LEVEL_WIDTH; i++) {
        catch_attempts[i] = (int*)calloc(MAX_LEVEL_HEIGHT, sizeof(int));
    }

    ready = 1;
}

void gui_clean() {
    int i;

    CHECK_INIT;

    ready = 0;
    
    al_stop_timer(timer);
    al_unregister_event_source(queue, al_get_keyboard_event_source());
    al_unregister_event_source(queue, al_get_display_event_source(display));
    al_unregister_event_source(queue, al_get_timer_event_source(timer));
    al_destroy_event_queue(queue);
    queue = NULL;
    al_destroy_timer(timer);
    timer = NULL;
    
    al_destroy_display(display);
    display = NULL;

    al_destroy_bitmap(tile);
    tile = NULL;

    al_destroy_font(font);
    al_destroy_font(font2);
    font = NULL;

    al_uninstall_keyboard();
    al_shutdown_ttf_addon();
    al_shutdown_primitives_addon();
    al_shutdown_image_addon();
    al_uninstall_system();

    for(i = 0; i < MAX_LEVEL_WIDTH; i++) {
        free(catch_attempts[i]);
    }
    free(catch_attempts);
}

int orientation_offset(ORIENTATION orientation) {
    switch (orientation) {
        case NORTH: return 0;
        case EAST: return 2;
        case SOUTH: return 1;
        case WEST: return 3;
    }
    fprintf(stderr, "Bad orientation %d!\n", orientation);
    exit(-1);
}

void draw_base_level() {
    al_draw_filled_rectangle(
        0, 0,
        screen_w + GAME_OFFSET_X + GAME_EXTRA_W, screen_h + GAME_OFFSET_Y + GAME_EXTRA_H,
        al_map_rgb(0, 0, 0));

    int x, y;
    for(x = 0; x < cur_level->width; x++) {
        for(y = 0; y < cur_level->height; y++) {
            int sx, sy;

            if((x == 0) || (x == cur_level->width-1) || (y == 0) || (y == cur_level->height-1) ) { 
                sx = OBSTACLE_X * TILE_W;
                sy = OBSTACLE_UNCATCHABLE_Y * TILE_H;
            } else {
                sx = EMPTY_X * TILE_W;
                sy = EMPTY_Y * TILE_H;
            }

            al_draw_tinted_scaled_rotated_bitmap_region(
                tile,
                sx, sy, TILE_W, TILE_H,
                al_map_rgba_f(1, 1, 1, 1),
                0, 0,
                (x * TILE_DIM) + GAME_OFFSET_X, (y * TILE_DIM) + GAME_OFFSET_Y,
                SCALE, SCALE,
                0, 0);                

            if(catch_attempts[x][y] & CATCH_ATTEMPT) {
                int neighbors_x = 0;
                int neighbors_y = 0;
                if(x > 0 && catch_attempts[x-1][y] & CATCH_ATTEMPT) { neighbors_x++; }
                if(x < cur_level->width-1 && catch_attempts[x+1][y] & CATCH_ATTEMPT) { neighbors_x++; }
                if(y > 0 && catch_attempts[x][y-1] & CATCH_ATTEMPT) { neighbors_y++; }
                if(y < cur_level->height-1 && catch_attempts[x][y+1] & CATCH_ATTEMPT) { neighbors_y++; }

                sx = CATCH_CORE_X * TILE_W;
                sy = CATCH_CORE_Y * TILE_H;
                if(catch_attempts[x][y] & CATCH_ATTEMPT_POKEBALL) {
                    sx = CATCH_X * TILE_W;
                    sy = CATCH_CENTER_Y * TILE_H;
                } else {
                    if(neighbors_x >= 1 && neighbors_y >= 1) {
                        sx = CATCH_X * TILE_W;
                        sy = CATCH_CENTER_Y * TILE_H;
                    } else if(neighbors_x >= 1) {
                        sx = CATCH_X * TILE_W;
                        sy = CATCH_HORIZONTAL_Y * TILE_H;
                    } else if(neighbors_y >= 1) {
                        sx = CATCH_X * TILE_W;
                        sy = CATCH_VERTICAL_Y * TILE_H;
                    }

                    if(catch_attempts[x][y] & CATCH_ATTEMPT_OBSTACLE) {
                        sx = CATCH_CORE_X * TILE_W;
                        sy = CATCH_CORE_Y * TILE_H;
                    }
                }

                al_draw_tinted_scaled_rotated_bitmap_region(
                    tile,
                    sx, sy, TILE_W, TILE_H,
                    al_map_rgba_f(1, 1, 1, 1),
                    0, 0,
                    (x * TILE_DIM) + GAME_OFFSET_X, (y * TILE_DIM) + GAME_OFFSET_Y,
                    SCALE, SCALE,
                    0, 0);    

                if(catch_attempts[x][y] & CATCH_ATTEMPT_PLAYER || catch_attempts[x][y] & CATCH_ATTEMPT_PIKACHU) {
                    sx = CATCH_CORE_X * TILE_W;
                    sy = CATCH_CORE_Y * TILE_H;

                    al_draw_tinted_scaled_rotated_bitmap_region(
                        tile,
                        sx, sy, TILE_W, TILE_H,
                        al_map_rgba_f(1, 1, 1, 1),
                        0, 0,
                        (x * TILE_DIM) + GAME_OFFSET_X, (y * TILE_DIM) + GAME_OFFSET_Y,
                        SCALE, SCALE,
                        0, 0);    
                }
            }
        }
    }
        
    for(x = 0; x < cur_level->width; x++) {
        for(y = 0; y < cur_level->height; y++) {
            if(catch_attempts[x][y] & CATCH_ATTEMPT) {
                catch_attempts[x][y] &= ~CATCH_ATTEMPT;
            }
            else {
                catch_attempts[x][y] = 0;
            }
        }
    }
}

void gui_set_level_info(LevelInfo* level_info) {
    CHECK_INIT;

    if(level_info->width <= MAX_LEVEL_WIDTH && level_info->height <= MAX_LEVEL_HEIGHT) {
        screen_w = level_info->width * TILE_DIM;
        screen_h = level_info->height * TILE_DIM;

        al_resize_display(display, screen_w + GAME_OFFSET_X + GAME_EXTRA_W, screen_h + GAME_OFFSET_Y + GAME_EXTRA_H);

        cur_level = level_info;

        is_finished = 0;
        is_game_over = 0;

        draw_base_level();
    } else {
        fprintf(stderr, "Wrong level size %dx%d!\n", level_info->width, level_info->height);
        exit(-1);
    }
}

void gui_game_event(Event* ev) {
    int has_event = 0;

    while (!has_event) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
 
        switch (event.type) {
            case ALLEGRO_EVENT_DISPLAY_CLOSE: {
                keys |= GUI_KEY_ESC;
                ev->keyDownEvent.type = EVENT_KEY;
                ev->keyDownEvent.key = keys;
                has_event = 1;
                break;
            }
            /*  Support following key combinations:
                    LEFT, UP, RIGHT, DOWN
                    A, W, D, S (qwerty)
                    Q, Z, D, S (azerty)
                    4, 8, 6, 5
            */
            case ALLEGRO_EVENT_KEY_UP: {
                 switch(event.keyboard.keycode) {
                    case ALLEGRO_KEY_PAD_8:
                    case ALLEGRO_KEY_W:
                    case ALLEGRO_KEY_Z:
                    case ALLEGRO_KEY_UP:
                       keys &= ~GUI_KEY_UP;
                       break;
 
                    case ALLEGRO_KEY_PAD_5:
                    case ALLEGRO_KEY_S:
                    case ALLEGRO_KEY_DOWN:
                       keys &= ~GUI_KEY_DOWN;
                       break;
 
                    case ALLEGRO_KEY_PAD_4:
                    case ALLEGRO_KEY_A:
                    case ALLEGRO_KEY_Q:
                    case ALLEGRO_KEY_LEFT: 
                       keys &= ~GUI_KEY_LEFT;
                       break;
 
                    case ALLEGRO_KEY_PAD_6:
                    case ALLEGRO_KEY_D:
                    case ALLEGRO_KEY_RIGHT:
                       keys &= ~GUI_KEY_RIGHT;
                       break;
 
                    case ALLEGRO_KEY_ENTER:
                    case ALLEGRO_KEY_SPACE:
                        keys &= ~GUI_KEY_SPACE;
                       break;
                 }
                 ev->keyDownEvent.type = EVENT_KEY;
                 ev->keyDownEvent.key = keys;
                 has_event = 1;
                 break;
            }
            case ALLEGRO_EVENT_KEY_DOWN: {
                 switch(event.keyboard.keycode) {
                    case ALLEGRO_KEY_ESCAPE:
                        keys |= GUI_KEY_ESC;
                        break;

                    case ALLEGRO_KEY_PAD_8:
                    case ALLEGRO_KEY_W:
                    case ALLEGRO_KEY_Z:
                    case ALLEGRO_KEY_UP:
                       keys |= GUI_KEY_UP;
                       break;
 
                    case ALLEGRO_KEY_PAD_5:
                    case ALLEGRO_KEY_S:
                    case ALLEGRO_KEY_DOWN:
                       keys |= GUI_KEY_DOWN;
                       break;
 
                    case ALLEGRO_KEY_PAD_4:
                    case ALLEGRO_KEY_A:
                    case ALLEGRO_KEY_Q:
                    case ALLEGRO_KEY_LEFT: 
                       keys |= GUI_KEY_LEFT;
                       break;
 
                    case ALLEGRO_KEY_PAD_6:
                    case ALLEGRO_KEY_D:
                    case ALLEGRO_KEY_RIGHT:
                       keys |= GUI_KEY_RIGHT;
                       break;
 
                    case ALLEGRO_KEY_ENTER:
                    case ALLEGRO_KEY_SPACE:
                        keys |= GUI_KEY_SPACE;
                       break;
                 }
                 ev->keyDownEvent.type = EVENT_KEY;
                 ev->keyDownEvent.key = keys;
                 has_event = 1;
                 break;
            }
            case ALLEGRO_EVENT_TIMER: {
                if(++tick_time >= TICK_PER_S) {
                    level_time++;
                    tick_time = 0;
                }

                if(++step_time >= 5) {
                    step_time = 0;

                    if(++walk_time >= 3) {
                        walk_time = 1;
                    }
                }
                ev->timerEvent.type = EVENT_TIMER;
                has_event = 1;
                break;
            }
            case ALLEGRO_EVENT_DISPLAY_RESIZE: {
                al_acknowledge_resize(display);
                has_event = 0;
                break;
            }
            default: {
                has_event = 0;
                break;
            }
        }
    }
}
void gui_highscore_event(Event* ev) {
    int has_event = 0;
    while (!has_event) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_KEY_CHAR) { 
            ev->keyDownEvent.type = EVENT_KEY;
            has_event = 0;
            switch (event.keyboard.keycode)
                {
                case ALLEGRO_KEY_ESCAPE:
                    ev->keyDownEvent.key = GUI_CHAR_ESC;
                    has_event = 1;
                    break;
                
                case ALLEGRO_KEY_ENTER:
                    ev->keyDownEvent.key = GUI_CHAR_ENTER;
                    has_event = 1;
                    break;
                
                case ALLEGRO_KEY_BACKSPACE:
                    ev->keyDownEvent.key = GUI_CHAR_BACKSPACE;
                    has_event = 1;
                    break;

                case ALLEGRO_KEY_SPACE:
                    ev->keyDownEvent.key = GUI_CHAR_SPACE;
                    has_event = 1;
                    break;

                case ALLEGRO_KEY_MINUS:
                    ev->keyDownEvent.key = GUI_CHAR_MINUS;
                    has_event = 1;
                    break;
                }
            if(event.keyboard.keycode < 28) { //valid character
                ev->keyDownEvent.key = event.keyboard.unichar;
                if(event.keyboard.modifiers & 1 || event.keyboard.modifiers & 1024) {ev->keyDownEvent.key |= 64;} //shift or capslock
                has_event = 1;
            }
        }
        else if(event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            ev->keyDownEvent.type = EVENT_KEY;
            ev->keyDownEvent.key = GUI_CHAR_ESC;
            has_event = 1;    
        }
        else {
            has_event = 0; // ignore all other events, don't think this approach is correct as esc won't be processed
                           // and if we do process esc it results in code duplication
        }
    }
}


void gui_add_bonus(Bonus* bonus) {
    CHECK_INIT;

    if(bonus->x < MAX_LEVEL_WIDTH * TILE_DIM && bonus->y < MAX_LEVEL_HEIGHT * TILE_DIM) {
        int sx = BONUS_X * TILE_W;
        int sy = 0;

        if(bonus->bonus_type == EXTRA_POWER) {
            sy = BONUS_POKEBALL_POWER * TILE_H;
        } else if(bonus->bonus_type == EXTRA_POKEBALL) {
            sy = BONUS_POKEBALL * TILE_H;
        } else if(bonus->bonus_type == FREEZE_PIKACHUS) {
            sy = BONUS_POKEBALL_FREEZE * TILE_H;
        }

        al_draw_tinted_scaled_rotated_bitmap_region(
            tile,
            sx, sy, TILE_W, TILE_H,
            al_map_rgba_f(1, 1, 1, 1),
            0, 0,
            bonus->x + GAME_OFFSET_X, bonus->y + GAME_OFFSET_Y,
            SCALE, SCALE,
            0, 0);
    }
}

void gui_add_pikachu(Pikachu* pikachu) {
    CHECK_INIT;

    if(pikachu->x < MAX_LEVEL_WIDTH * TILE_DIM && pikachu->y < MAX_LEVEL_HEIGHT * TILE_DIM) {
        int sx = (PIKACHU_NORTH_X + orientation_offset(pikachu->orientation)) * TILE_W;
        int sy = 0;

        if(!pikachu->frozen) { 
            if(pikachu->orientation == NORTH || pikachu->orientation == SOUTH) {
                sy = walk_time * TILE_H;
            } else {
                if(walk_time <= 1) {
                    sy = walk_time * TILE_H;
                }
            }
        }

        ALLEGRO_COLOR color = al_map_rgba_f(1, 1, 1, 1);
        if(pikachu->is_strong) {
            color = al_map_rgba_f(1, 0, 0, 1);
        }

        al_draw_tinted_scaled_rotated_bitmap_region(
            tile,
            sx, sy, TILE_W, TILE_H,
            color,
            0, 0,
            pikachu->x + GAME_OFFSET_X, pikachu->y + GAME_OFFSET_Y,
            SCALE, SCALE,
            0, 0);

        if(pikachu->is_catched) {
            catch_attempts[pikachu->x / TILE_DIM][pikachu->y / TILE_DIM] |= CATCH_ATTEMPT_PIKACHU;
        }
    }
}

void gui_add_catch_attempt_tile(int x, int y) {    
    int tx, ty;

    CHECK_INIT;
    
    if(x < MAX_LEVEL_WIDTH * TILE_DIM && y < MAX_LEVEL_HEIGHT * TILE_DIM) {
        if(x % TILE_DIM != 0 || y % TILE_DIM != 0) {
            fprintf(stderr, "Coordinate must be at tile edge: x=%d y=%d!\n", x, y);
            exit(-1);
        }
        tx = x / TILE_DIM;
        ty = y / TILE_DIM;

        catch_attempts[tx][ty] |= CATCH_ATTEMPT;        
    }
}

void gui_add_player(Player* player) {
    CHECK_INIT;

    if(player->x < MAX_LEVEL_WIDTH * TILE_DIM && player->y < MAX_LEVEL_HEIGHT * TILE_DIM) {
        int sx = (PLAYER_NORTH_X + orientation_offset(player->orientation)) * TILE_W;
        int sy = 0;
        if(keys != 0) {
            sy = walk_time * TILE_H;
        }

        if(is_game_over || player->is_catched) {
            sx = PLAYER_CATCHED_X * TILE_W;
            sy = PLAYER_CATCHED_Y * TILE_H;
        }

        al_draw_tinted_scaled_rotated_bitmap_region(
            tile,
            sx, sy, TILE_W, TILE_H,
            al_map_rgba_f(1, 1, 1, 1),
            0, 0,
            player->x + GAME_OFFSET_X, player->y + GAME_OFFSET_Y,
            SCALE, SCALE,
            0, 0);
        
        if(player->is_catched) {
            catch_attempts[player->x / TILE_DIM][player->y / TILE_DIM] |= CATCH_ATTEMPT_PLAYER;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////

void gui_add_obstacle(Obstacle* obstacle) {
    CHECK_INIT;

    if(obstacle->x < MAX_LEVEL_WIDTH * TILE_DIM && obstacle->y < MAX_LEVEL_HEIGHT * TILE_DIM) {
        int sx = OBSTACLE_X * TILE_W;
        int sy = 0;

        if(obstacle->is_catchable == 0) {
            sy = OBSTACLE_UNCATCHABLE_Y * TILE_H;
        }
        else {
            sy = OBSTACLE_CATCHABLE_Y * TILE_H;
        }
        al_draw_tinted_scaled_rotated_bitmap_region(
            tile,
            sx, sy, TILE_W, TILE_H,
            al_map_rgba_f(1, 1, 1, 1),
            0, 0,
            (obstacle->x) + GAME_OFFSET_X, (obstacle->y) + GAME_OFFSET_Y,
            SCALE, SCALE,
            0, 0);
    }
}

void gui_draw_text(char* text, int x, int y) {
    CHECK_INIT;
    ALLEGRO_COLOR color = al_map_rgba_f(1, 1, 1, 1);
    al_draw_text(   
                    font, 
                    color, 
                    x, 
                    y, 
                    0, 
                    text
                    );
}
void gui_draw_text_number(int number, int x, int y) {
    CHECK_INIT;
    ALLEGRO_COLOR color = al_map_rgba_f(1, 1, 1, 1);
    al_draw_textf(font, color, x, y, 0, "%d", number); 
}

void gui_draw_tile(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color) {
    CHECK_INIT;
    int offset = GAME_EXTRA_H + GAME_EXTRA_W + GAME_OFFSET_X;
    al_draw_rectangle(
                        offset + GAME_OFFSET_X  +  x1 - TILE_SIZE/2 , 
                        offset + GAME_OFFSET_Y  + y1 - TILE_SIZE/2, 
                        offset + GAME_OFFSET_X  + x2 + TILE_SIZE/2, 
                        offset + GAME_OFFSET_Y  + y2 + TILE_SIZE/2 , 
                        color, 
                        1
                    );
}

void gui_draw_circle(float cx, float cy, ALLEGRO_COLOR color, int size, int number) {
    CHECK_INIT; 
    int offset = GAME_EXTRA_H + GAME_EXTRA_W + GAME_OFFSET_X;
    al_draw_circle( 
                       offset + GAME_OFFSET_X + cx,
                       offset + GAME_OFFSET_Y + cy,
                       ((TILE_SIZE/2)-size),
                       color,
                       (1-((float)number/10))
                  );

}

/* draw_tile maar met size kun je het vierkant van grootte veranderen, test functie */
void gui_draw_rectangle_size(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, int size) {
    CHECK_INIT;
    int offset = GAME_EXTRA_H + GAME_EXTRA_W + GAME_OFFSET_X;
    al_draw_rectangle(
                        offset + GAME_OFFSET_X  +  x1 - ((TILE_SIZE/2)-size) , 
                        offset + GAME_OFFSET_Y  + y1 - ((TILE_SIZE/2)-size), 
                        offset + GAME_OFFSET_X  + x2 + ((TILE_SIZE/2)-size), 
                        offset + GAME_OFFSET_Y  + y2 + ((TILE_SIZE/2)-size) , 
                        color, 
                        1
                    );
}

void gui_draw_rectangle_filled(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color) {
    CHECK_INIT;
    int offset = GAME_EXTRA_H + GAME_EXTRA_W + GAME_OFFSET_X;
    al_draw_filled_rectangle(
                        offset + GAME_OFFSET_X  +  x1 - TILE_SIZE/2 , 
                        offset + GAME_OFFSET_Y  + y1 - TILE_SIZE/2, 
                        offset + GAME_OFFSET_X  + x2 + TILE_SIZE/2, 
                        offset + GAME_OFFSET_Y  + y2 + TILE_SIZE/2 , 
                        color
                    );
}
void gui_draw_input_box(float x1, float y1) {
    CHECK_INIT;
    int offset = GAME_EXTRA_H + GAME_EXTRA_W + GAME_OFFSET_X;
    al_draw_filled_rectangle(
                        offset + GAME_OFFSET_X  +  x1 - TILE_SIZE*3 , 
                        offset + GAME_OFFSET_Y  + y1 - TILE_SIZE/2, 
                        offset + GAME_OFFSET_X  + x1 + TILE_SIZE*3, 
                        offset + GAME_OFFSET_Y  + y1 + TILE_SIZE/2 , 
                        al_map_rgba_f(0.150, 0.150, 0.150, 0.8)
                    );
}



void gui_add_pokeball(Pokeball* pokeball) {
    CHECK_INIT;

    if(pokeball->x < MAX_LEVEL_WIDTH * TILE_DIM && pokeball->y < MAX_LEVEL_HEIGHT * TILE_DIM) {
        int sx = POKEBALL_X * TILE_W;
        int sy = POKEBALL_Y * TILE_H;

        al_draw_tinted_scaled_rotated_bitmap_region(
            tile,
            sx, sy, TILE_W, TILE_H,
            al_map_rgba_f(1, 1, 1, 1),
            0, 0,
            (pokeball->x * TILE_W) + GAME_OFFSET_X, (pokeball->y * TILE_H) + GAME_OFFSET_Y,
            SCALE, SCALE,
            0, 0);
    }
    
}


void draw_game_over_overlay() {
    CHECK_INIT;
    ALLEGRO_COLOR white = al_map_rgba_f(1, 1, 1, 1);
    ALLEGRO_COLOR red = al_map_rgba_f(1, 0, 0, 1);
    al_draw_filled_rectangle(
        0, 0,
        screen_w + GAME_OFFSET_X + GAME_EXTRA_W, screen_h + GAME_OFFSET_Y + GAME_EXTRA_H,
        al_map_rgba_f(0.090, 0.090, 0.090, 0.7));
    
    al_draw_textf(font2, red, GAME_OFFSET_X + (screen_w + GAME_OFFSET_X) / 2, GAME_OFFSET_Y + ((screen_h + GAME_OFFSET_Y) / 8), ALLEGRO_ALIGN_CENTER, "GAME OVER!");
    if (request_name == 1) {
        //Draw message
        al_draw_textf(font, white, 
                        GAME_OFFSET_X + (screen_w + GAME_OFFSET_X) / 2,
                        GAME_OFFSET_Y + (screen_h + GAME_OFFSET_Y) / 8, 
                        ALLEGRO_ALIGN_CENTER, 
                        "Your score is: %d",
                        total_score
                     );
        al_draw_textf(font, white, 
                      GAME_OFFSET_X + (screen_w + GAME_OFFSET_X) / 2, 
                      GAME_OFFSET_Y + ((screen_h + GAME_OFFSET_Y) / 8) + 70, 
                      ALLEGRO_ALIGN_CENTER, 
                      "Please enter your name"
                      );
        if(name) {
            al_draw_textf(font, white, 
            GAME_OFFSET_X + (screen_w + GAME_OFFSET_X) / 2, 
            GAME_OFFSET_Y + ((screen_h + GAME_OFFSET_Y) / 8) + 140, 
            ALLEGRO_ALIGN_CENTRE, 
            "%s", name
            );
        }
    }
    else {
        //Draw message
        al_draw_textf(font, white, 
                    GAME_OFFSET_X + (screen_w + GAME_OFFSET_X) / 2, 
                    GAME_OFFSET_Y + ((screen_h + GAME_OFFSET_Y) / 8) + 80, 
                    ALLEGRO_ALIGN_CENTER, 
                    "Press enter to continue"
                    );
        //Draw highscores
        for (int i = 0; i < highscores->size; i++) {
            al_draw_textf(font, white, 
            GAME_OFFSET_X + (screen_w + GAME_OFFSET_X) / 2, 
            GAME_OFFSET_Y + ((screen_h + GAME_OFFSET_Y) / 8) + 35*(i+1) + 80, 
            ALLEGRO_ALIGN_CENTER, 
            "%d. %s  %d  %s", 
            i+1,
            highscores->entries[i].name, 
            highscores->entries[i].score, 
            highscores->entries[i].datetime
            );
        }
    }
}

void draw_finished_overlay() {
    CHECK_INIT;
    ALLEGRO_COLOR green = al_map_rgba_f(0, 1, 0, 1);
    al_draw_filled_rectangle(
        0, 0,
        screen_w + GAME_OFFSET_X + GAME_EXTRA_W, screen_h + GAME_OFFSET_Y + GAME_EXTRA_H,
        al_map_rgba_f(0.090, 0.090, 0.090, 0.7));
    al_draw_textf(font2, green, (screen_w + GAME_OFFSET_X) / 2, (screen_h + GAME_OFFSET_Y) / 2 - 120, ALLEGRO_ALIGN_CENTER, "YOU WON!");
    al_draw_textf(font, green, (screen_w + GAME_OFFSET_X) / 2, ((screen_h + GAME_OFFSET_Y) / 2) - 10, ALLEGRO_ALIGN_CENTER, "Press space to go to the next level");
    al_draw_textf(font, green, (screen_w + GAME_OFFSET_X) / 2, ((screen_h + GAME_OFFSET_Y) / 2) + 40, ALLEGRO_ALIGN_CENTER, "Your level score: %d", level_score);
    al_draw_textf(font, green, (screen_w + GAME_OFFSET_X) / 2, ((screen_h + GAME_OFFSET_Y) / 2) + 90, ALLEGRO_ALIGN_CENTER, "Your current score: %d", total_score);
}


/* debug hulp functie, wordt gebruikt om voor elke pikachu een verschillend kleur te hebben */
/* om hun pad te kunnen visualiseren, niet belangrijk */
ALLEGRO_COLOR number_to_color(int number) {  
    //float alpha = 1;
    float alpha = 1-((float)number/10);
    switch(number) {
        case(5):
            return al_map_rgba_f(0.9, 0.91, 0.91, 0.81);
            break;
        case(6):
            return al_map_rgba_f(0, 1, 0, alpha); /* green*/
            break;
        case(3):
            return al_map_rgba_f(1, 0, 0, alpha); /* blue */
            break;
        case(4):
            return al_map_rgba_f(0, 0, 1, alpha); /* red */
            break;
        case(1):
            return al_map_rgba_f(0.9, 0, 0.8, alpha); /* pink */
            break;
        case(2):
            return al_map_rgba_f(0.2, 0.9, 1, alpha); /* light blue */
            break;
    }  
    return al_map_rgba_f(0,0,0,alpha); /* no idea whut colour tho, probs black or something*/                 
}

void gui_draw_textf(char* text) {
    al_draw_textf(font, al_map_rgba_f(1, 1, 1, 1), GAME_OFFSET_X +((screen_w + GAME_OFFSET_X) / 2) - 20, GAME_OFFSET_Y + ((screen_h + GAME_OFFSET_Y) / 8) + 150, ALLEGRO_ALIGN_CENTER, "%s", text);
}
void gui_draw_buffer() {
    double t = al_get_time();

    CHECK_INIT;
    ALLEGRO_COLOR white = al_map_rgba_f(1, 1, 1, 1);
    al_draw_textf(font, white, 10, 5, ALLEGRO_ALIGN_LEFT, "POKEBALLS: %d", pokeballs_left);
    al_draw_textf(font, white, 10, 40, ALLEGRO_ALIGN_LEFT, "PIKACHUS: %d", pikachus_left);
    al_draw_textf(font, white, 10, 75, ALLEGRO_ALIGN_LEFT, "POINTS: %d", level_score);
    al_draw_textf(font, white, screen_w, 5, ALLEGRO_ALIGN_RIGHT, "LEVEL: %d", cur_level->level_nr);
    al_draw_textf(font, white, screen_w, 40, ALLEGRO_ALIGN_RIGHT, "ELAPSED SEC: %d", level_time);
    al_draw_textf(font, white, screen_w, 75, ALLEGRO_ALIGN_RIGHT, "FPS: %d", fps);
    if(is_game_over) {

        draw_game_over_overlay();
    }
    if(is_finished) {
        draw_finished_overlay(cur_level->level_nr, level_score, total_score);
    }
    al_flip_display();
    fps_accum++;
    if(t - fps_time >= 1) {
        fps = fps_accum;
        fps_accum = 0;
        fps_time = t;
    }

    draw_base_level();
}

////////////////////////////////////////////////////////////////////////////////////

void gui_start_timer() {
    CHECK_INIT;

    level_time = 0;
    tick_time = 0;
}

int gui_get_timer_score() {
    CHECK_INIT;

    return level_time;
}

void gui_set_game_over(HighScoreTable* highscores_) {
    CHECK_INIT;

    is_game_over = 1;
    highscores = highscores_;
}

void gui_set_request_name() {
    CHECK_INIT;
    request_name = 1;
}

void gui_set_name(char* _name) {
    CHECK_INIT;
    name = _name;
}

void gui_clear_request_name() {
    request_name = 0;
    name = NULL;
}

void gui_set_finished_level(int total_score_, int level_score_) {
    CHECK_INIT;

    is_finished = 1;
    total_score = total_score_;
    level_score = level_score_;
}

void gui_set_pikachus_left(int pikachus_left_) {
    CHECK_INIT;

    pikachus_left = pikachus_left_;
}

void gui_set_pokeballs_left(int pokeballs_left_) {
    CHECK_INIT;

    pokeballs_left = pokeballs_left_;
}

void gui_set_level_score(int level_score_) {
    CHECK_INIT;

    level_score = level_score_;
}

void gui_clear_keys() {
    keys = 0;
}