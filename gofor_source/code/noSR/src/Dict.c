#include "../include/Dict.h"
#include "../include/utils.h"


void Dict_init(Dict_t* dic, int size)
{
    dic->paths = malloc(size * sizeof(Path_t));
    dic->actSize = 0;
    dic->maxSize = size;
    for (int i = 0 ; i < size ; i++) {
        dic->paths[i].color = IGNORED;
    }
}


void Dict_insert(Dict_t* dic, DistVector_t nextDist, DistType_t color)
{
    int ind = dic->actSize;
    if (ind >= dic->maxSize) {
        ERROR("No more space in dist %d/%d\n", ind, dic->maxSize);
        return;
    }
    dic->paths[ind].dist = nextDist;
    dic->paths[ind].color = color;
    dic->actSize++;
}


void Dict_replace(Dict_t* dic, int index, DistVector_t nextDist, DistType_t color)
{
    dic->paths[index].dist = nextDist;
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
            fprintDistVector(stdout, dic->paths[i].dist);
            printf("\n");
        }
    }
}
