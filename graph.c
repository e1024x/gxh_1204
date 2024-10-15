
#include "graph.h"





/* BFS implementatie, graaf is niet gewogen en ongericht dus BFS geeft altijd kortste pad terug (als ze bestaat) */                                                                                   
stack* shortest_path(graph* g, int start_x, int start_y, int einde_x, int einde_y, int which_pikachu) {
    queue *q = malloccheck(1, queue);
    int start = label(start_x, start_y, g->width);
    int einde = label(einde_x, einde_y, g->width); 
    init_queue(q);
    queue_push(q, start);
    while (!queue_empty(q)) { /* returnt 1 wanneer queue empty is, stop dan het algoritme */
        int endofqueue = queue_pop(q);
        g->heads[endofqueue]->marked = 1; /* mark knoop als bezocht */
        node* current_head = g->heads[endofqueue];   
        while (current_head->next_node != NULL) {    /* ga over alle buurknopen van laatst gepopte knoop (label: endofqueue) */ 
            current_head = current_head->next_node;    /* schuif huidige knoop 1tje op */ 
            if (g->heads[current_head->label]->marked != 1) { /* enkel knopen die we nog niet hebben bezocht pushen */
                if (!queue_full(q)) {
                    g->heads[current_head->label]->came_from = endofqueue; /*  vorige knoop = laatst gepopte knoop (endofqueue)  */
                    queue_push(q, current_head->label);                    /* ^  wordt gebruikt om op het einde het pad vanuit eindpunt te construeren */
                }
            }
            if (g->heads[einde]->marked == 1) { /* wanneer we onze eind knoop hebben bezocht stopt het algoritme */
                int label_t = einde;  
                int i = 0;
                stack* s = malloccheck(1, stack);
                init_stack(s);
                do {           /* backtracking, vanuit het eind punt keren we telkens terug naar het punt waar het uit komt */
                    stack_push(s, label_t);
                    //g->marked_tiles[label_t + which_pikachu*(25*13)] =  1;   eerste pikachu index = 0, maar 0 is voor inactieve tile) 
                    int new_label = g->heads[label_t]->came_from; /* backtracking */
                    label_t = new_label;
                    i++;

                } while (label_t != start && (i < MAX_PATH_LENGTH));
                destroy_queue(q);
                free(q);
                clear_all_heads(g);
                return s;
            }
        }
    }
    stack* s = malloccheck(1, stack); 
    stack_push(s, NO_PATH_FOUND);
    init_stack(s);
    clear_all_heads(g);
    destroy_queue(q);
    free(q);
    /* TODO: return dat geen pad is gevonden en activeer dan random_movement */
    return s;
}


void destroy_head(node* n) {
    node* tmp;
    while (n != NULL) {
        tmp = n;
        n = n->next_node;
        free(tmp);
    }
}



void clear_all_heads(graph* g) {
    for (int i = 0; i < (g->n); i++) {
        g->heads[i]->marked = false;
        g->heads[i]->came_from = 0;
    }
}



/* initialiseer graaf met num_of_nodes knopen */
graph* init_graph(int width, int height) {    
    graph* graph_t;
    graph_t = malloccheck(1, graph);
    //init_labels(width, height);
    graph_t->n = width*height;
    graph_t->width = width;
    graph_t->height = height;
    graph_t->m = 0;
    graph_t->heads = NULL;
    graph_t->heads = malloccheck(width*height,node*);
    for (int i = 0; i < width*height; i++) {
        graph_t->heads[i] = malloccheck(1, node);
        graph_t->heads[i]->label = i;
        graph_t->heads[i]->next_node = NULL;
        graph_t->heads[i]->marked = false;
        graph_t->heads[i]->came_from = 0;
    }
    /*
    int** a_list = malloccheck(num_of_nodes, int*);
    for (int j = 0; j < num_of_nodes; j++) {
        a_list[j] = calloc(num_of_nodes, sizeof(int));
    }
    graph_t->adjadency_list = a_list;
    */
    //graph_t->marked_tiles = calloc(num_of_nodes*AMT_OF_PIKACHUS, sizeof(int));
    return graph_t;
}

void destroy_graph(graph* g) {
  /*
  for (int i = 0; i < g->n; i++) {
      free(g->adjadency_list[i]);
    }
  free(g->adjadency_list);
  */
  for (int i = 0; i < g->n; i++) {
      destroy_head(g->heads[i]);
  }
  free(g->heads);
  //free(g->marked_tiles);
  free(g);
}


/* voeg een buurknoop toe aan 1 v/d hoofdknopen */
void add_neighbour_node(graph* g, int head, int n, int undirected) {
    int label_t = head;
    node* temp = g->heads[label_t];
    while( temp->next_node != NULL) { /* doorheen de linked list gaan & er voor zorgen dat dezelfde knoop er maar 1 keer in staat */
      if (temp->next_node->label == n && (undirected == 0)) {
          goto end;
      }
      temp = temp->next_node;
    } 
    temp->next_node = malloccheck(1, node);
    temp->next_node->next_node = NULL;
    temp->next_node->label = n;
    end:;
}

void remove_edge_undirected(graph* g, int head_x, int head_y, int dest_x, int dest_y) {
    int head = label(head_x, head_y, g->width);
    int dest = label(dest_x, dest_y, g->width);
    remove_edge(g, head, dest);
    remove_edge(g, dest, head);
}
void remove_edge(graph* g, int head, int dest) {
    node* temp = g->heads[head];
    node* new_head = malloccheck(1, node);
    new_head->label = head;
    new_head->next_node = NULL;
    new_head->marked = false;
    new_head->came_from = 0;
    while (temp->next_node != NULL) {
        temp = temp->next_node;
        if (temp->label != dest) {
            node* t_2 = new_head;
            while( t_2->next_node != NULL) { /* doorheen de linked list gaan & er voor zorgen dat dezelfde knoop er maar 1 keer in staat */
                t_2 = t_2->next_node;
            }
            t_2->next_node = malloccheck(1, node);
            t_2->next_node->next_node = NULL;
            t_2->next_node->label = temp->label;
        } 
    }
    destroy_head(g->heads[head]);
    g->heads[head] = new_head;
}

void add_edge(graph* g, int src_x, int src_y, int dest_x, int dest_y, int undirected) {
    //g->adjadency_list[src][dest] = 1;
    int dest = label(dest_x, dest_y, g->width);
    int src = label(src_x, src_y, g->width);
    g->m++;
    add_neighbour_node(g, src, dest, 1);
    if (undirected) {
        //g->adjadency_list[dest][src] = 1;
        add_neighbour_node(g, dest, src, 1);  
        g->m++;
    }
    
}


node* create_node(int label, int type) {
  node* n = malloc(1 * sizeof(node));
  n->label = label;
  n->next_node = NULL;
  n->marked = false;
  n->came_from = 0;
  return n;
}
/*
void print_a_list(graph* g) {
  for(int i = 0; i < g->n; i++) {
    for(int j = 0; j < g->n; j++) {
      printf("|  %d  ",g->adjadency_list[i][j]);
    }
    printf("|\n");
  } 
}
*/
void print_graph(graph* g) {
  for (int i = 0; i < g->n; i++) {
    node* temp = g->heads[i];
    while( temp->next_node != NULL ) {
        printf("%d ->\t", temp->label);
        temp = temp->next_node;
    }
    printf("%d ->\t", temp->label);
    printf("\n---------\n"); 
  }
}

void print_all_neighbours(node* n) {
  node* temp = n;
  while( temp->next_node != NULL ) {
      printf("%d ->\t", temp->label);
      temp = temp->next_node;
    }
  printf("\n");
}


/**********************************************/
/******************* STACK ********************/
/**********************************************/

void init_stack(stack* s) {
    s->size = 0;
    s->top = NULL;
}

bool stack_empty(stack* s) {
    if (s->size == 0) {
        return true;
    }
    else {
        return false;
    }
}

bool stack_full(stack* s) {
    if (s->size == MAX_STACK_SIZE) {
        return true;
    }
    else {
        return false;
    }
}

void stack_push(stack* s, int number) {
    if (!stack_full(s)) {
        s->size++;
        node* new_top;
        new_top = malloccheck(1, node);
        new_top->label = number;
        new_top->next_node = s->top;
        s->top = new_top;
    }
}

int stack_pop(stack* s) {
    int waarde;
    node* vorige_top_p;
    if (s->size != 0) {
        s->size--;
        vorige_top_p = s->top;
        waarde = s->top->label;
        s->top = s->top->next_node;
        free(vorige_top_p);
        return waarde;
    }
    return 0;
}

int* stack_pop_coords(graph* g, stack* s) {
    int label = stack_pop(s);
    int* coords = malloc(sizeof(int)*2);
    int x = label % g->width; /* inverse operatie van labels_x_y */
    int y = (label - x) / g->width;
    coords[0] = x; coords[1] = y;
    return coords;
}

void destroy_stack(stack* s) {
    while (s->size != 0) {
        stack_pop(s);
    }
    free(s);
}

/**********************************************/
/******************* QUEUE ********************/
/**********************************************/

/* initialiseer queue */
void init_queue(queue* q) {
    q->size = 0;
    q->tail = NULL;
    q->head = NULL;
}

void destroy_queue(queue* q) {
    while (!queue_empty(q)) {
        queue_pop(q);
    }
}

/* voeg een knoop toe aan queue */
void queue_push(queue* q, int label) {
    node* t = malloccheck(1,node);
    t->label = label;
    t->next_node = NULL;
    if (q->size > 0) {
        q->tail->next_node = t;
        q->tail = t;
    }
    else {
        q->head = t;
        q->tail = t;
    }
    q->size++;
}

/* pop een element en return het */
int queue_pop(queue* q) {
    int ret_label;
    node *n;
    ret_label = q->head->label;
    n = q->head; /* gealloceerd in queue_push */
    q->head = q->head->next_node;
    free(n);
    q->size--;
    return ret_label;
}


bool queue_full(queue* q) {
    if (q->size == MAX_QUEUE_SIZE) {
        return true;
    }
    else {
        return false;
    }
}
bool queue_empty(queue* q) {
  if (q->size == 0) {
    return true;
  }
  else {
    return false;
  }
}

