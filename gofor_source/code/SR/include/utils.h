#ifndef __UTILS_H__
#define __UTILS_H__

#include "base.h"
#include "use-case.h"
#include "SegmentLLSet.h"

typedef struct DistVectorSR {
    int nbSeg;
    DistVector_t dist;
    // last segments
    SegmentLLSet_t *lastSegs;
} DistVectorSR_t;


float Slength(int m0, DistVector_t dist, DistVector_t cstr, Mode_t mode);

char* distTypeStr(DistType_t distType);

/*
bool is_later_dominated_u_v(int m0u, my_m1 m1u, my_m2 m2u, int m0v, my_m1 m1v, my_m2 m2v, int vSegType, int currDagu, int currDagv);

bool is_dominated_u_v(int m0u, my_m1 m1u, my_m2 m2u, int m0v, my_m1 m1v, my_m2 m2v, int vSegType, int currDagu, int currDagv);
*/

bool is_dominated_SR(
    int m0v, DistVector_t distv, 
    int m0u, DistVector_t distu, Mode_t mode);


bool is_dominated_0(DistVector_t distv, DistVector_t distu, Mode_t mode);
bool is_dominated_0_neq(DistVector_t distv, DistVector_t distu, Mode_t mode);

// First condition to be strongly dominated
bool is_strongly_dominated_i(
    int m0v, DistVector_t distv, int vSegType, int currDagv, 
    int m0u, DistVector_t distu, int uSegType, int currDagu, Mode_t mode);

// Second condition to be strongly dominated
bool is_strongly_dominated_ii(
    int m0v, DistVector_t distv, 
    int m0u, DistVector_t distu, Mode_t mode);


bool is_strongly_dominated(
    int m0v, DistVector_t distv, int vSegType, int currDagv, 
    int m0u, DistVector_t distu, int uSegType, int currDagu, Mode_t mode);

static inline bool DistanceSr_eq(DistVectorSR_t *distv, DistVectorSR_t *distu)
{
    return ((distv->nbSeg == distu->nbSeg) && Distance_eq(distv->dist, distu->dist));
}

#endif
