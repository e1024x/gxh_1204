#pragma once
#ifndef H_HIGHSCORES
#define H_HIGHSCORES

#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"
#include "util.h"
#include <time.h>

#define HIGHSCORE_FILE "highscores.dat"
#define MAX_HIGHSCORE_ENTRIES 5
#define DATETIME_LENGTH 22
#define MAX_NAME_LENGTH 20
 
typedef struct HighScore {
    char *name;
    int score;
    char *datetime;
} HighScore;

typedef struct HighScoreTable {
    HighScore* entries;
    int size;
    int changed;
} HighScoreTable;


void load_highscores(HighScoreTable* highscores);
bool check_highscore_entry(HighScoreTable* highscores, int score);
void save_highscores(HighScoreTable* highscores);
void reset_highscores();

#endif