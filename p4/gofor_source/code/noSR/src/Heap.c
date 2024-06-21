#include "../include/Heap.h"

void Heap_init(Heap_t *bp, int size)
{
    bp->heapSize = 0;
    bp->keys = malloc(size * HEAP_STEP * sizeof(Elem_t));
    ASSERT_VOID(bp->keys);
    memset(bp->keys, 0, size * HEAP_STEP * sizeof(Elem_t));
    bp->isPresent = malloc(size * sizeof(int *));
    ASSERT_VOID(bp->isPresent);
    for (int i = 0; i < size; i++)
    {
        bp->isPresent[i] = malloc(HEAP_STEP * sizeof(int));
        for (int j = 0; j < HEAP_STEP; j++)
        {
            bp->isPresent[i][j] = -1;
        }
    }
    bp->maxSize = size * HEAP_STEP;
}

void swap_elem(Elem_t *x, Elem_t *y)
{
    Elem_t temp = *x;
    *x = *y;
    *y = temp;
}

void Heap_insert_key(Heap_t *bp, int node, int index, float value)
{
    if (bp->heapSize >= bp->maxSize)
    {
        ERROR("Cannot insert key : no more space");
        return;
    }

    if (bp->isPresent[node][index] != -1)
    {
        Heap_decrease_key(bp, node, index, value);
        return;
    }

    bp->heapSize++;
    int i = bp->heapSize - 1;
    bp->keys[i].nodeId = node;
    bp->keys[i].index = index;
    bp->keys[i].value = value;
    bp->isPresent[node][index] = i;
    while (i != 0 && bp->keys[parent(i)].value > bp->keys[i].value)
    {
        //printf("Swap act\n");
        swap_elem(&bp->keys[i], &bp->keys[parent(i)]);
        swap_int(&bp->isPresent[bp->keys[i].nodeId][bp->keys[i].index], &bp->isPresent[bp->keys[parent(i)].nodeId][bp->keys[parent(i)].index]);
        i = parent(i);
    }
}

void Heap_min_heapify(Heap_t *bp, int i)
{
    int l = left(i);
    int r = right(i);
    int smallest = i;
    if (l < bp->heapSize && bp->keys[l].value < bp->keys[i].value)
    {
        smallest = l;
    }
    if (r < bp->heapSize && bp->keys[r].value < bp->keys[smallest].value)
    {
        smallest = r;
    }
    if (smallest != i)
    {
        swap_elem(&bp->keys[i], &bp->keys[smallest]);
        swap_int(&bp->isPresent[bp->keys[i].nodeId][bp->keys[i].index], &bp->isPresent[bp->keys[smallest].nodeId][bp->keys[smallest].index]);
        Heap_min_heapify(bp, smallest);
    }
}

Elem_t Heap_extract_min(Heap_t *bp)
{
    Elem_t ret;

    if (bp->heapSize <= 0)
    {
        ret.index = -1;
        ret.nodeId = -1;
        ret.value = -1.0;
        return ret;
    }
    if (bp->heapSize == 1)
    {
        bp->heapSize--;
        ret.index = bp->keys[0].index;
        ret.nodeId = bp->keys[0].nodeId;
        ret.value = bp->keys[0].value;
        return ret;
    }

    ret.index = bp->keys[0].index;
    ret.nodeId = bp->keys[0].nodeId;
    ret.value = bp->keys[0].value;
    bp->keys[0] = bp->keys[bp->heapSize - 1];
    bp->isPresent[bp->keys[0].nodeId][bp->keys[0].index] = 0;
    bp->isPresent[ret.nodeId][ret.index] = -1;
    bp->heapSize--;
    Heap_min_heapify(bp, 0);

    return ret;
}


void Heap_free(Heap_t* bp)
{
    for (int i = 0 ; i < bp->maxSize / HEAP_STEP; i++) {
        free(bp->isPresent[i]);
    }
	free(bp->isPresent);
    free(bp->keys);
}


void Heap_print(Heap_t* bp)
{
    for (int i = 0 ; i < bp->heapSize ; i++) {
        printf(" (%d ; %f ; %d) ", bp->keys[i].nodeId, bp->keys[i].value, bp->keys[i].index);
    }
    printf("\n");
}


void Heap_decrease_key(Heap_t* bp, int node, int index, float value)
{
    int ind = bp->isPresent[node][index];
    //printf("IN DECREASE : %d\n", ind);

    if (ind == -1) {
        ERROR("Node is not in the heap anymore\n");
        return ;
    }

    bp->keys[ind].value = value;
    bp->keys[ind].index = index;

    while (ind != 0 && bp->keys[parent(ind)].value > bp->keys[ind].value) {
        swap_elem(&bp->keys[parent(ind)], &bp->keys[ind]);
        swap_int(&bp->isPresent[bp->keys[parent(ind)].nodeId][bp->keys[parent(ind)].index], &bp->isPresent[bp->keys[ind].nodeId][bp->keys[ind].index]);
        ind = parent(ind);
    }
}


