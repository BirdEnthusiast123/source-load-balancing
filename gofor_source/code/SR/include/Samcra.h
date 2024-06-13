#ifndef __SAMCRA_H__
#define __SAMCRA_H__

#include "base.h"
#include "Dict.h"
#include "Heap.h"
#include "SrGraph.h"
#include "utils.h"
#include "Llist.h"
#include "IntList.h"

typedef enum SamcraRetrieveOption_t 
{
    SC_RETRIEVE_ONE_BEST,
    SC_RETRIEVE_ALL_BEST,
    SC_RETRIEVE_ALL,
} SamcraRetrieveOption_t;

typedef struct SamcraContext_t {
    SrGraph_t *topo;
    SrGraph_t *srGraph;
    Heap_t heap;
    Dict_t *dist;
    DistVector_t cstr;
    SamcraRetrieveOption_t retrieveOption;
    EncodingType_t encodingType;
    Mode_t mode;
    int failedSrc;
    int failedDst;
    char failedEdgeIndex;
    my_m1 failedEdgeIGP;
} SamcraContext_t;


void Samcra_initialize(Dict_t** d, Heap_t* heap, int src, int nbNodes);

DistType_t Samcra_feasability(SamcraContext_t *ctx, Dict_t *dist_dst, DistVectorSR_t *nextDist);

void Samcra_update_queue(
    Heap_t *heap, 
    Dict_t *dist_v, 
    int v,
    float predLength, 
    DistVectorSR_t nextDist, 
    DistType_t color);

void Samcra(SamcraContext_t *ctx, int src, DistVector_t cstr, int *init_time);

void Samcra_clean(SamcraContext_t *ctx);

int addNecessarySegmentsStrict(
    SamcraContext_t *ctx, 
    int edge_src, 
    int edge_dst, 
    Edge_t *edge_weight, 
    unsigned char edge_index,
    Segment_t lastSeg,
    Segment_t *newSeg);

int addNecessarySegmentsLoose(
    SamcraContext_t *ctx, 
    int edge_src, 
    int edge_dst, 
    Edge_t *edge_weight, 
    unsigned char edge_index,
    Segment_t lastSeg,
    Segment_t *newSeg);
;
#endif