#pragma once

#include "base.h"

typedef struct DistVector {
    my_m1 m1;
    my_m2 m2;
} DistVector_t;



static inline bool is_dominated(
    DistVector_t distv, 
    DistVector_t distu)
{
    return distu.m1 < distv.m1 
        || (distu.m1 == distv.m1 && distu.m2 <= distv.m2);
}

static inline bool Distance_eq(DistVector_t distv, DistVector_t distu)
{
    return (distv.m1 == distu.m1) && (distv.m2 == distu.m2);
}
static inline DistVector_t Distance_maxValue()
{
    DistVector_t res;
    res.m1 = INT_MAX;
    res.m2 = INT_MAX;
    return res;
}

static inline int Distance_hash(DistVector_t dist)
{
    (void) dist;
    return 0; // all distance are totally ordered so there is only one best possible distance
}

static inline bool TopologicalProperties(SamcraContext_t *ctx, Segment_t seg, int dest) {
    (void)ctx;
    (void)seg;
    (void)dest;
    return true;
}

#define Distance_SLength(dist, cstr) ((float)(dist).m1 / (float)(cstr).m1)

#define fprintDistVector(output, dist) fprintf(output, "%d %d", dist.m2, dist.m1)


static inline DistVector_t Distance_add(DistVector_t a, DistVector_t b) {
    DistVector_t res;
    res.m1 = a.m1 + b.m1;
    res.m2 = a.m2 + b.m2;
    return res;
}
static inline DistVector_t Distance_sub(DistVector_t a, DistVector_t b) {
    DistVector_t res;
    res.m1 = a.m1 - b.m1;
    res.m2 = a.m2 - b.m2;
    return res;
}

static inline void Distance_default_cstr(DistVector_t* dist)
{
    dist->m1 = 100;
    dist->m2 = INT_MAX;
}

