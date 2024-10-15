#include "util.h"

//Malloccheck
void* _malloccheck(size_t size, const char* f, int l) {
    void* pointer = malloc(size);
    if(!pointer) {
        printf("Allocation error in file %s, line %d\n", f, l);
        exit(EXIT_FAILURE);
    }
    return pointer;
}

//Realloccheck
void* _realloccheck(void* p, size_t size, const char* f, int l) {
    void* pointer = realloc(p, size);
    if(!pointer) {
        printf("Allocation error in file %s, line %d\n", f, l);
        exit(EXIT_FAILURE);
    }
    return pointer;
}

//Malloccheck, for arrays
void** _malloccheckarray(size_t size, size_t count, const char* f, int l) {
    void** pointer = malloccheck(count, void*);
    for(size_t i = 0; i < count; i++) {
        pointer[i] = _malloccheck(size, f, l);
    }
    return pointer;
}
