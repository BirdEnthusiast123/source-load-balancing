#ifndef __DICT_H__
#define __DICT_H__

#include "base.h"

typedef struct Dict_s Dict_t;

typedef struct Path_s Path_t;

struct Path_s {
    my_m1 m1;
    my_m2 m2;
    char color;
    char nbSeg;
};

struct Dict_s {
    Path_t* paths;
    int maxSize;
    int actSize;
};

void Dict_init(Dict_t* dic, int size);

void Dict_insert(Dict_t* dic, my_m1 m1, my_m2 m2, char nbSeg);

void Dict_replace(Dict_t* dic, my_m1 m1, my_m2 m2, int index, char nbSeg);

void Dict_free(Dict_t* dic);

void Dict_print(Dict_t* dic, int dst);

#endif