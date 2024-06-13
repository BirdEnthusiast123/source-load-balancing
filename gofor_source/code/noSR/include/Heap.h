#ifndef __HEAP_H__
#define __HEAP_H__

#include "base.h"
#include "BinHeap.h"

typedef struct Elem_s Elem_t;

struct Elem_s {
    int nodeId;
    int index;
    float value;
};


typedef struct Heap_s Heap_t;


#if HEAP == BIN_HEAP

struct Heap_s {
    Elem_t* keys;
    int heapSize;
    int maxSize;
    int** isPresent;
};

#endif

void Heap_init(Heap_t* bp, int size);

void swap_elem(Elem_t* x, Elem_t* y);

void Heap_insert_key(Heap_t* bp, int node, int index, float value);

void Heap_min_heapify(Heap_t* bp, int i);

Elem_t Heap_extract_min(Heap_t* bp);

void Heap_free(Heap_t* bp);

void Heap_print(Heap_t* bp);

void Heap_decrease_key(Heap_t* bp, int node, int index, float value);

#endif