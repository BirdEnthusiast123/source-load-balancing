#include "../include/Dict.h"


void Dict_init(Dict_t* dic, int size)
{
    dic->paths = malloc(size * sizeof(Path_t));
    dic->actSize = 0;
    dic->maxSize = size;
    for (int i = 0 ; i < size ; i++) {
        dic->paths[i].color = BLACK;
    }
}


void Dict_insert(Dict_t* dic, my_m1 m1, my_m2 m2, char nbSeg)
{
    int ind = dic->actSize;
    if (ind >= dic->maxSize) {
        //ERROR("No more space in dist %d/%d\n", ind, dic->maxSize);
        return;
    }
    dic->paths[ind].m1 = m1;
    dic->paths[ind].m2 = m2;
    dic->paths[ind].color = WHITE;
    dic->paths[ind].nbSeg = nbSeg;
    dic->actSize++;
}


void Dict_replace(Dict_t* dic, my_m1 m1, my_m2 m2, int index, char nbSeg)
{
    dic->paths[index].m1 = m1;
    dic->paths[index].m2 = m2;
    dic->paths[index].nbSeg = nbSeg;
    dic->paths[index].color = WHITE;
}


void Dict_free(Dict_t* dic)
{
    free(dic->paths);
}


void Dict_print(Dict_t* dic, int dst)
{
    for (int i = 0 ; i < dic->maxSize ; i++) {
        if (dic->paths[i].color != BLACK) {
            printf("%d %d %d\n", dst, dic->paths[i].m1, dic->paths[i].m2);
        }
    }
}