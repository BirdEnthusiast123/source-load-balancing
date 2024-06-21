#include "SegmentLLSet.h"

void SegmentLLSet_fdebug(FILE*file, SegmentLLSet_t *set) {
    while(set) {
        fprintf(file, "(%d, %s) ", set->seg.src, set->seg.type == NODE_SEGMENT ? "Node" : "Adj");
        set = set->next;
    }
}

void SegmentLLSet_merge_equality(SegmentLLSet_t **dest, SegmentLLSet_t **other) {
    //printf("SegmentLLSet_merge_equality ");
    //SegmentLLSet_fdebug(stdout, *dest);
    //printf(" with ");
    //SegmentLLSet_fdebug(stdout, *other);
    //printf("\n");

    SegmentLLSet_t *currLastSegs = *dest;
    SegmentLLSet_t **currLastSegsPtr = dest;
    SegmentLLSet_t *nextLastSegs = *other;
    while(nextLastSegs) 
    {
        if(currLastSegs != NULL 
            && nextLastSegs->seg.src == currLastSegs->seg.src
            && nextLastSegs->seg.type == currLastSegs->seg.type)
        {
            SegmentLLSet_t * tmpNextLastSegs = nextLastSegs->next;
            free(nextLastSegs);
            nextLastSegs = tmpNextLastSegs;
            continue;
        }
        if (currLastSegs == NULL 
            || nextLastSegs->seg.src < currLastSegs->seg.src
            || (nextLastSegs->seg.src == currLastSegs->seg.src
                && nextLastSegs->seg.type < currLastSegs->seg.type)
            )
        {
            SegmentLLSet_t * tmpNextLastSegs = nextLastSegs->next;
            nextLastSegs->next = currLastSegs;
            *currLastSegsPtr = nextLastSegs;
            currLastSegs = nextLastSegs;
            nextLastSegs = tmpNextLastSegs;
        }
        currLastSegsPtr = &(currLastSegs->next);
        currLastSegs = currLastSegs->next;
    }
    *other = NULL;
    //SegmentLLSet_fdebug(stdout, *dest);
    //printf("\n");
}

void SegmentLLSet_merge_inclusion(SegmentLLSet_t **dest, SegmentLLSet_t **other) {
    SegmentLLSet_merge_equality(dest, other);
}


void SegmentLLSet_subtract_equality(SegmentLLSet_t **dest, SegmentLLSet_t *other) {

    //printf("SegmentLLSet_subtract_equality ");
    //SegmentLLSet_fdebug(stdout, *dest);
    //printf(" by ");
    //SegmentLLSet_fdebug(stdout, other);
    //printf("\n");

    SegmentLLSet_t *currLastSegs = other;
    SegmentLLSet_t *nextLastSegs = *dest;
    SegmentLLSet_t **nextLastSegsPtr = dest;
    while(currLastSegs && nextLastSegs) 
    {
        if(nextLastSegs->seg.src == currLastSegs->seg.src
            && nextLastSegs->seg.type == currLastSegs->seg.type)
        {
            *nextLastSegsPtr = nextLastSegs->next;
            free(nextLastSegs);
            nextLastSegs = *nextLastSegsPtr;
            continue;
        }
        if (    nextLastSegs->seg.src < currLastSegs->seg.src
            || (nextLastSegs->seg.src == currLastSegs->seg.src
                && nextLastSegs->seg.type < currLastSegs->seg.type)
            )
        {
            nextLastSegsPtr = &(nextLastSegs->next);
            nextLastSegs = nextLastSegs->next;
        }
        else {
            currLastSegs = currLastSegs->next;
        }
    }
    //SegmentLLSet_fdebug(stdout, *dest);
    //printf("\n");
}


void SegmentLLSet_subtract_inclusion(SegmentLLSet_t **dest, SegmentLLSet_t *other) {
    SegmentLLSet_subtract_equality(dest, other);
}


SegmentLLSet_t * SegmentLLSet_copy(SegmentLLSet_t *src, SegmentLLSet_t *end)
{
    if(src == NULL) return end;

    SegmentLLSet_t *newSet = NULL;
    SegmentLLSet_t **newSetPtr = &newSet;
    while(src) {
        *newSetPtr = malloc(sizeof(SegmentLLSet_t));
        (*newSetPtr)->seg = src->seg;
        (*newSetPtr)->next = end;
        newSetPtr = &((*newSetPtr)->next);
        src = src->next;
    }
    return newSet;
}