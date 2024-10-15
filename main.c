#include <stdlib.h>
#include <time.h>
#include "game.h"
#include "gui.h"
#include "highscores.h"

int main(int argc, char* argv[]) 
{
    int total_score = 0;    
    Game game = {};
    HighScoreTable highscores;
    load_highscores(&highscores);
    /* Specifiy level number */
    int level = argc >= 2 ? atoi(argv[1]) : 1;
    
    /* Or use random seed */
    int seed = argc == 3 ? atoi(argv[2]) : (int)time(NULL);
    
    srand(seed);

    /* Initialize the gui */
    gui_initialize();

    /* Initialize the first game instance */
    init_game(&game, level);

    while(game.state != GAME_ENDED) {
        
        /*Init game is currently in level transition, 
        this will allow us to generate the new game whilst waiting for the players approvel to start*/

        /* Start the actual game loop */
        total_score += do_game_loop(&game);

        /* Clean up */
        destroy_game(&game);

        /* Press space to continue to next game, handles transition to next level*/
        if(game.state == GAME_OVER || game.state == FINISHED) {
            level_transition(&game, &highscores, total_score);
        }
    }
    save_highscores(&highscores);
    gui_clean();

    return 0;
}