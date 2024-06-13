#pragma once

#include "base.h"

typedef struct DistVector {
    my_m1 m1;
    my_m2 m2;
} DistVector_t;

static inline bool is_dominated(
    int m0v, DistVector_t distv, 
    int m0u, DistVector_t distu)
{
    if ((m0u <= m0v) && (distu.m2 <= distv.m2))
    {
        return true;
    }
    return false;
}

static inline bool Distance_eq(DistVector_t distv, DistVector_t distu)
{
    return (distv.m1 == distu.m1) && (distv.m2 == distu.m2);
}

static inline bool TopologicalProperties(Segment_t seg, int dest) {
    return true; // TODO
}

#define Distance_SLength(dist, cstr) ((float)(dist).m2 / (float)(cstr).m2)

#define fprintDistVector(output, dist) fprintf(output, "%d %d", dist.m1, dist.m2)


static inline DistVector_t Distance_add(DistVector_t a, DistVector_t b) {
    DistVector_t res;
    res.m1 = a.m1 + b.m1;
    res.m2 = a.m2 + b.m2;
    return res;
}

static inline void Distance_default_cstr(DistVector_t* dist)
{
    dist->m1 = 100;
    dist->m2 = INT_MAX;
}

