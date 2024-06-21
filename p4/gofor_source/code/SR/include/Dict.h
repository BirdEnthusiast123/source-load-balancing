#ifndef __DICT_H__
#define __DICT_H__

#include "base.h"
#include "utils.h"

typedef struct Dict_s Dict_t;

typedef struct Path_s Path_t;



struct Path_s {
    DistVectorSR_t distSr;
    DistType_t color;
};

struct Dict_s {
    Path_t* paths;
    int maxSize;
    int actSize;
};

void Dict_init(Dict_t* dic, int size);

void Dict_insert(Dict_t* dic, DistVectorSR_t nextDist, DistType_t color);

void Dict_replace(Dict_t* dic, int index, DistVectorSR_t nextDist, DistType_t color);

void Dict_free(Dict_t* dic);

void Dict_print(Dict_t* dic, int dst);

void SegmentLLSet_add(SegmentLLSet_t** set, Segment_t seg);

void SegmentLLSet_free(SegmentLLSet_t* set);

#endif