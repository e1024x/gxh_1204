#pragma once
#ifndef H_GRAPH
#define H_GRAPH

#include "catch_them_all.h"
#include "level.h"
#include "highscores.h"
#include "stdbool.h"
#include "gui.h"


#define MAX_PATH_LENGTH 80
#define MAX_QUEUE_SIZE 1400
#define MAX_STACK_SIZE 1000

#define UNDIRECTED 1
#define DIRECTED 0
#define NO_PATH_FOUND -10


typedef struct node {
    int label; 
    int came_from;
    /*
    ENTITY_TYPE type;
    */
    bool marked;
    struct node* next_node;
} node;


typedef struct stack {
    int size;
    node* top;
} stack;


typedef struct graph { /* niet gerichte graaf */
    int n;
    int m; 
    int width; 
    int height;
    node** heads;
    //int** adjadency_list;
    int* marked_tiles; /* gewoon voor te testen, is overbodig */
} graph;

typedef struct queue {
    int size;
    node* head;
    node* tail;
} queue;

static inline int label(int x, int y, int width) {
    return width*y + x;
}

bool queue_empty(queue* q);
bool queue_full(queue* q);
void queue_push(queue* q, int label);
int queue_pop(queue* q);
stack* shortest_path(graph* g, int start_x, int start_y, int einde_x, int einde_y, int which_pikachu);
void print_all_neighbours(node* n);


void init_stack(stack* s);
bool stack_empty(stack* s);
bool stack_full(stack* s);
void stack_push(stack* s, int number);
int stack_pop(stack* s);
int* stack_pop_coords(graph* g, stack* s);

void destroy_stack(stack* s);
void destroy_queue(queue* q);
void destroy_head(node* n);
void destroy_graph(graph*);
void clear_all_heads(graph* g);

void init_queue(queue* q);
graph* init_graph(int width, int height);

void add_neighbour_node(graph*, int, int, int undirected);
void add_edge(graph*, int src_x, int src_y, int dest_x, int dest_y, int undirected);
void remove_edge(graph* g, int head, int dest);
void remove_edge_undirected(graph* g, int head_x, int head_y, int dest_x, int dest_y);

void print_a_list(graph*);
void print_graph(graph*);


#endif