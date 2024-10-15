#include <stdlib.h>
#include "catch_them_all.h"
#include "level.h"

#ifndef H_UTIL
#define H_UTIL

typedef struct HighScoreTable HighScoreTable;
#define malloccheck(count, type) ((type*) (_malloccheck((count)*sizeof(type), __FILE__, __LINE__)))
#define realloccheck(pointer, count, type) ((type*) (_realloccheck(pointer, (count)*sizeof(type), __FILE__, __LINE__)))
#define malloccheckarray(size, count, type) ((type**) (_malloccheckarray((size)*sizeof(type), count, __FILE__, __LINE__)))

void* _malloccheck(size_t size, const char* f, int l);
void* _realloccheck(void* p, size_t size, const char* f, int l);
void** _malloccheckarray(size_t size, size_t count, const char* f, int l);

typedef enum {EVENT_TIMER, EVENT_KEY} EVENT_TYPE;

typedef struct {
    EVENT_TYPE type;
} TimerEvent;

typedef struct {
    EVENT_TYPE type;
} DisplayCloseEvent;

typedef struct {
    EVENT_TYPE type;
    int key;
} KeyDownEvent;

typedef union {
    EVENT_TYPE type;
    TimerEvent timerEvent;
    DisplayCloseEvent displayCloseEvent;
    KeyDownEvent keyDownEvent;
} Event;

#endif