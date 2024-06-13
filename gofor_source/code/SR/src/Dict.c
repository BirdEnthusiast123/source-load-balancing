#include "../include/Dict.h"
#include "../include/utils.h"


void Dict_init(Dict_t* dic, int size)
{
    dic->paths = malloc(size * sizeof(Path_t));
    dic->actSize = 0;
    dic->maxSize = size;
    for (int i = 0 ; i < size ; i++) {
        dic->paths[i].color = IGNORED;
        //dic->paths[i].segType = NODE_SEGMENT;
    }
}


void Dict_insert(Dict_t* dic, DistVectorSR_t nextDist, DistType_t color)
{
    int ind = dic->actSize;
    if (ind >= dic->maxSize) {
        ERROR("No more space in dist %d/%d\n", ind, dic->maxSize);
        return;
    }
    dic->paths[ind].distSr = nextDist;
    dic->paths[ind].color = color;
    dic->actSize++;
}


void Dict_replace(Dict_t* dic, int index, DistVectorSR_t nextDist, DistType_t color)
{
    SegmentLLSet_free(dic->paths[index].distSr.lastSegs);
    dic->paths[index].distSr = nextDist;
    dic->paths[index].color = color;
}


void Dict_free(Dict_t* dic)
{
    free(dic->paths);
}


void Dict_print(Dict_t* dic, int dst)
{
    for (int i = 0 ; i < dic->maxSize ; i++) {
        if (dic->paths[i].color != IGNORED) {
            printf("%d ", dst);
            fprintDistVector(stdout, dic->paths[i].distSr.dist);
            printf("\n");
        }
    }
}

void SegmentLLSet_add(SegmentLLSet_t** set, Segment_t seg)
{
    SegmentLLSet_t* new = malloc(sizeof(SegmentLLSet_t));
    new->seg = seg;
    new->next = *set;
    *set = new;
}

void SegmentLLSet_free(SegmentLLSet_t* set)
{
    SegmentLLSet_t* tmp;
    while (set != NULL) {
        tmp = set;
        set = set->next;
        free(tmp);
    }
}