#pragma once

#include "base.h"
#include <stdio.h>

struct SegmentLLSet_t;
typedef struct SegmentLLSet_t SegmentLLSet_t;

struct SegmentLLSet_t {
    Segment_t seg;
    SegmentLLSet_t* next;
};

void SegmentLLSet_fdebug(FILE*file, SegmentLLSet_t *set);

void SegmentLLSet_merge_equality(SegmentLLSet_t **dest, SegmentLLSet_t **other);

void SegmentLLSet_merge_inclusion(SegmentLLSet_t **dest, SegmentLLSet_t **other);


void SegmentLLSet_subtract_equality(SegmentLLSet_t **dest, SegmentLLSet_t *other);

void SegmentLLSet_subtract_inclusion(SegmentLLSet_t **dest, SegmentLLSet_t *other);

// Copy the src list. Put the end list at the end of the copy (set to NULL if you don't want to append anything).
SegmentLLSet_t * SegmentLLSet_copy(SegmentLLSet_t *src, SegmentLLSet_t *end);