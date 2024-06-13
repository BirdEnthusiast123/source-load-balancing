#include "../include/Samcra.h"

void Samcra_initialize(Dict_t **d, Heap_t *heap, int src, int nbNodes)
{
    (*d) = malloc(nbNodes * sizeof(Dict_t));
    for (int i = 0; i < nbNodes; i++)
    {
        Dict_init((*d) + i, HEAP_STEP);
    }

    Dict_insert((*d) + src, 0, 0, 0);

    Heap_init(heap, nbNodes);
    Heap_insert_key(heap, src, 0, 0.0);
}

bool Samcra_feasability(Edge_t *edge, Dict_t *dist_v, Dict_t *dist_u, int ind)
{
    bool dominated = false;
    my_m1 d1v = dist_u->paths[ind].m1 + edge->m1;
    my_m2 d2v = dist_u->paths[ind].m2 + edge->m2;
    int d0v = dist_u->paths[ind].nbSeg + 1;
    for (int i = 0; i < dist_v->actSize; i++)
    {
        if (dist_v->paths[i].nbSeg == d0v && dist_v->paths[i].m1 == d1v && dist_v->paths[i].m2 == d2v)
        {
            dominated = true;
            break;
        }
        else if (is_dominated_u_v(dist_v->paths[i].nbSeg, dist_v->paths[i].m1, dist_v->paths[i].m2, d0v, d1v, d2v))
        {
            dominated = true;
            break;
        }
        else if (is_dominated_u_v(d0v, d1v, d2v,
                                  dist_v->paths[i].nbSeg, dist_v->paths[i].m1, dist_v->paths[i].m2))
        {
            dist_v->paths[i].color = BLACK;
        }
    }
    return dominated;
}

void Samcra_update_queue(Heap_t *heap, Dict_t *dist_v, Dict_t *dist_u, int ind, float predLength, int v, Edge_t *edge)
{
    my_m1 d1v = dist_u->paths[ind].m1 + edge->m1;
    my_m2 d2v = dist_u->paths[ind].m2 + edge->m2;
    for (int i = 0; i < dist_v->actSize; i++)
    {
        if (dist_v->paths[i].color == BLACK)
        {
            Heap_insert_key(heap, v, i, predLength);
            Dict_replace(dist_v, d1v, d2v, i, dist_u->paths[ind].nbSeg + 1);
            return;
        }
    }

    if (dist_v->actSize < dist_v->maxSize)
    {
        Dict_insert(dist_v, d1v, d2v, dist_u->paths[ind].nbSeg + 1);
        Heap_insert_key(heap, v, dist_v->actSize - 1, predLength);
    }
}

void Samcra(Dict_t **dist, SrGraph_t *sr, int src, my_m1 c1, my_m2 c2, int* init_time)
{
    struct timeval start, stop;
    gettimeofday(&start, NULL);

    Heap_t heap;
    Samcra_initialize(dist, &heap, src, sr->nbNode);
    Elem_t u_i;
    bool dominated;
    float predLength;
    int actNode;
    int actIndex;

    gettimeofday(&stop, NULL);
    *init_time = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;

    while (heap.heapSize > 0)
    {
        u_i = Heap_extract_min(&heap);
        actNode = u_i.nodeId;
        actIndex = u_i.index;
        if ((*dist)[actNode].paths[actIndex].color == BLACK)
        {
            continue;
        }

        for (int dst = 0; dst < sr->nbNode; dst++)
        {
            if (dst == actNode)
            {
                continue;
            }

            for (Edge_t *tmp = sr->succ[actNode][dst]; tmp != NULL; tmp = tmp->next)
            {
                my_m1 d1v = (*dist)[actNode].paths[actIndex].m1 + tmp->m1;
                my_m2 d2v = (*dist)[actNode].paths[actIndex].m2 + tmp->m2;
                int d0v = (*dist)[actNode].paths[actIndex].nbSeg + 1;
                predLength = Slength(d0v, d1v, d2v, c1, c2);
                if (predLength >= 1.0)
                {
                    continue;
                }
                dominated = Samcra_feasability(tmp, (*dist) + dst, (*dist) + actNode, actIndex);
                if (!dominated)
                {
                    Samcra_update_queue(&heap, (*dist) + dst, (*dist) + actNode, actIndex,
                                        predLength, dst, tmp);
                }
            }
        }
    }
    gettimeofday(&start, NULL);
    Heap_free(&heap);
    gettimeofday(&stop, NULL);
    *init_time += (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
}
