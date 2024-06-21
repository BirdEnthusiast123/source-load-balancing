#pragma once


//number max of segments
#define SEG_MAX                 90


typedef int my_m2;
typedef int my_m1;



#define MAX_SR_GRAPH_SIZE       1500


#define BI_DIRECTIONNAL         1
#define UNI_DIRECTIANNAL        0

#define LOAD_TOPO               0
#define LOAD_SR                 1
#define LOAD_LABELS             0
#define LOAD_IDS                1


#define NB_THREADS              4

#define BIN_HEAP                1
#define FIBO_HEAP               2

#define HEAP    BIN_HEAP

#define HEAP_STEP               10000

typedef enum DistType_t {
    IGNORED,
    RETRIEVABLE,
    ONLY_EXTENDABLE
} DistType_t;
