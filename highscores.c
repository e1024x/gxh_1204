#include "highscores.h"
void load_highscores(HighScoreTable* highscores) {

    //Checking if file exists
    FILE* file = fopen(HIGHSCORE_FILE, "rb");
    if(!file) {
        printf("Cannot open file %s", HIGHSCORE_FILE);
        file = fopen(HIGHSCORE_FILE, "wb");
        if(!file) {exit(EXIT_FAILURE);}
        int i = 0;
        fwrite(&i, sizeof(int), 1, file);
        fclose(file);
        file = fopen(HIGHSCORE_FILE, "rb");
        if(!file) {exit(EXIT_FAILURE);}
    }
    int flen;
    fread(&flen, sizeof(int), 1, file);
    highscores->size = flen;
    highscores->changed = 0;
    if(flen == 0) {return;} //dont allocate when the length is 0
    highscores->entries = malloccheck(flen, HighScore);
    int name_len;
    for(int i = 0; i < flen; i++) {
        fread(&name_len, sizeof(int), 1, file);
        highscores->entries[i].name = malloccheck(name_len+1, char);
        fread(highscores->entries[i].name, sizeof(char), name_len, file);
        highscores->entries[i].name[name_len] = '\0';
        fread(&(highscores->entries[i].score), sizeof(int), 1, file);
        highscores->entries[i].datetime = malloccheck(DATETIME_LENGTH+1, char);
        fread(highscores->entries[i].datetime, sizeof(char), DATETIME_LENGTH, file);
        highscores->entries[i].datetime[DATETIME_LENGTH] = '\0';
    }
    fclose(file);
}

char* get_datetime() {
    time_t t;
    time(&t);
    struct tm* tm = localtime(&t);
    //wordt nog aangevuld
    char* datetime = malloccheck(DATETIME_LENGTH+1, char);
    strftime(datetime, DATETIME_LENGTH+1, "%x %X", tm);
    datetime[DATETIME_LENGTH] = '\0';
    return datetime;
}

void reset_highscores(HighScoreTable* highscores) {
    //for testing, but can be implemented if we want to
    FILE* file = fopen(HIGHSCORE_FILE, "wb");
    if(!file) {
        printf("Cannot open file %s", HIGHSCORE_FILE);
        exit(EXIT_FAILURE);
    }
    int nul = 0;
    fwrite(&nul, sizeof(int), 1, file);
    fclose(file);
    if(highscores->size) {
        for(int i = 0; i < highscores->size; i++) {
            free(highscores->entries[i].name);
            free(highscores->entries[i].datetime);
        }
        free(highscores->entries);
    }
    highscores->size = 0;
}

bool check_highscore_entry(HighScoreTable* highscores, int score) {
    //First, set gui highscores
    gui_set_game_over(highscores);
    gui_draw_buffer();
    /*
    1. Score enters highscore, set request name screen
    2. Score is too low, proceed to next level
    */
    //Dont add highscore
    if(highscores->size == MAX_HIGHSCORE_ENTRIES) {
        if(score <= highscores->entries[highscores->size-1].score) {return false;}
    }
    //Add highscore
    int enter = 0;
    while(!enter) {
        Event ev;
        gui_highscore_event(&ev);
        if(ev.keyDownEvent.key == GUI_CHAR_ENTER || ev.keyDownEvent.key == GUI_KEY_SPACE) {enter++;}
        if(ev.keyDownEvent.key == GUI_CHAR_ESC) {return true;}
    }

    //Initialise for getting name
    enter = 0;
    char* name = malloccheck(MAX_NAME_LENGTH + 1, char);
    name[MAX_NAME_LENGTH] = '\0';
    highscores->changed++;

    int i = 0; // current letter of name we insert
    bool backspace = false; //keeps track of backspace
    
    gui_set_request_name();
    gui_draw_buffer();

    while(!enter) { // while enter has not been pressed
        char new_char;
        gui_set_name(name);
        gui_draw_buffer();

        Event ev; // if Event* ev, need to dynamically allocate space
        gui_highscore_event(&ev); // processes keyboard events as chars (different from gui_game_event)

        switch (ev.keyDownEvent.key)
        {
        case GUI_CHAR_ENTER: //Enter, commit name
            enter++;
            break; 
        
        case GUI_CHAR_ESC: //Escape => shut down game
            return true;

        case GUI_CHAR_BACKSPACE:
            backspace = true;
            break; //Remove last character
        
        case GUI_CHAR_SPACE: //Add space, cannot be automatically converted
            new_char = ' ';
            break;
        
        case GUI_CHAR_MINUS: //No automated conversion
            new_char = '-';
            break;

        default:
            new_char = (char)ev.keyDownEvent.key; //Auto convert key to char
            break;
        } 
        
        if (backspace) {
            if (i > 0) {
                i--;
                name[i] = '\276'; // replace character when backspace was pressed 
            }
            backspace = false;
            continue;
        }
        
        if (i >= 0 && i < MAX_NAME_LENGTH) {    // extra out of bounds check
            name[i] = new_char;     // new_char was properly handled by gui_highscore_event 
            i++;
        }
    }
    i--;
    gui_clear_request_name();
    name[i] = '\0';

    //Add highscores if name exist
    if(i) {
        /*
        1. Highscores is empty
        2. Highscores the highscore can always be added
        3. Highscores is full, remove one highscore
        */
        int index = 0, size = highscores->size;
        //1. Empty
        if(highscores->size == 0) {highscores->size++;highscores->entries = malloccheck(1, HighScore);size++;}
        //2. Non empty
        else {
            //Increase highscores->entries
            if(size < MAX_HIGHSCORE_ENTRIES) {
                //standard, the highscore can be added
                size++;
                highscores->size++;
                highscores->entries = realloccheck(highscores->entries, size, HighScore);
                highscores->entries[size-1] = (HighScore) {NULL, -1, NULL};
            }
            //Find index to add
            for(index = size-1; index >= 0; index--) {
                if(score <= highscores->entries[index].score) {break;}
            }
            index++;
            //Move all other highscores 1 place back
            for(int i = size-1;i > index; i--) {
                highscores->entries[i] = highscores->entries[i-1];
            }
            
        }
        //Spot highscores[index] may be overwritten, add score and datetime to index
        char* datetime = get_datetime();
        highscores->entries[index] = (HighScore) {NULL, score, datetime};

        
        //Dynamisch alloceren
        
        highscores->entries[index].name = realloccheck(name, i+1, char);
    }
    gui_set_game_over(highscores);
    gui_draw_buffer();
    return 0;
}

void save_highscores(HighScoreTable* highscores) {
    if(highscores->changed) {
        //Make the file pointer
        FILE* file = fopen(HIGHSCORE_FILE, "wb");
        if(!file) {
            printf("Error whilst creating file %s", HIGHSCORE_FILE);
            exit(EXIT_FAILURE);
        }
        //Write number of highscores
        fwrite(&(highscores->size), sizeof(int), 1, file);
        //Write top 5
        for(int i = 0, name_len; i < highscores->size; i++) {
            name_len = strlen(highscores->entries[i].name);
            fwrite(&name_len, sizeof(int), 1, file);
            fwrite(highscores->entries[i].name, sizeof(char), name_len, file);
            free(highscores->entries[i].name);
            fwrite(&(highscores->entries[i].score), sizeof(int), 1, file);
            fwrite(highscores->entries[i].datetime, sizeof(char), DATETIME_LENGTH, file);
            free(highscores->entries[i].datetime);
        }
        if(highscores->entries) {free(highscores->entries);}
        fclose(file);
    }
}
