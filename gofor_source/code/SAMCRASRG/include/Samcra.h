#ifndef __SAMCRA_H__
#define __SAMCRA_H__

#include "base.h"
#include "Dict.h"
#include "Heap.h"
#include "SrGraph.h"
#include "utils.h"
#include "Llist.h"


void Samcra_initialize(Dict_t** d, Heap_t* heap, int src, int nbNodes);

bool Samcra_feasability(Edge_t* edge, Dict_t* dist_v, Dict_t* dist_u, int ind);

void Samcra_update_queue(Heap_t* heap, Dict_t* dist_v, Dict_t* dist_u, int ind, float predLength, int v, Edge_t* edge);

void Samcra(Dict_t** dist, SrGraph_t* sr, int src, my_m1 c1, my_m2 c2, int* init_time);

SrGraph_t* SrGraph_load_bin(char* filename);

#endif