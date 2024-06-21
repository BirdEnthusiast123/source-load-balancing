#ifndef __DICT_H__
#define __DICT_H__

#include "base.h"
#include "utils.h"

typedef struct Dict_s Dict_t;

typedef struct Path_s Path_t;



struct Path_s {
    DistVector_t dist;
    DistType_t color;
};

struct Dict_s {
    Path_t* paths;
    int maxSize;
    int actSize;
};

void Dict_init(Dict_t* dic, int size);

void Dict_insert(Dict_t* dic, DistVector_t nextDist, DistType_t color);

void Dict_replace(Dict_t* dic, int index, DistVector_t nextDist, DistType_t color);

void Dict_free(Dict_t* dic);

void Dict_print(Dict_t* dic, int dst);

#endif