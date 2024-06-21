#include "../include/Samcra.h"



void Samcra_initialize(Dict_t **d, Heap_t *heap, int src, int nbNodes)
{
    (*d) = malloc(nbNodes * sizeof(Dict_t));
    for (int i = 0; i < nbNodes; i++)
    {
        Dict_init((*d) + i, HEAP_STEP);
    }
    DistVector_t originDist = {
        .m1 = 0,
        .m2 = 0
    };
    
    Dict_insert((*d) + src, originDist, RETRIEVABLE);

    Heap_init(heap, nbNodes);
    Heap_insert_key(heap, src, 0, 0.0);
}

// // IMPORTANT: This function assumes that distA is dominated by distB and that distA and distB are not equal!
// void Samcra_handle_dominated_distance(SamcraContext_t *ctx, DistVector_t *distA, DistVector_t *distB, DistType_t *distType)
// {
//     // since they are not equal and distA is dominated by distB, we have three cases:
//     // 1. number of segment are the same (domination is strict on the remaining distances)
//     // 2. the remaining distances are the same (domination is strict on the number of segments)
//     // 3. domination is strict both on the number of segments and the remaining distances

//     if(distA->nbSeg == distB->nbSeg)
//     {
//         // the remaining distances are different. In this case, the only way nextDist is interesting,
//         // is if it is not strongly dominated, ie, when the last seg have different sources.
//         // So we have to remove from the nextDist, the lastSegs that exists in dist_dst->paths[i].distSr.lastSegs
//         // removal is based on last segment inclusion or equality depending on whether we want to retrieve all solutions or just one
//         *distType = ONLY_EXTENDABLE; // distA may only be interesting for the next extensions
//         if(ctx->retrieveOption == SC_RETRIEVE_ONE_BEST)
//             SegmentLLSet_subtract_inclusion(&distA->lastSegs, distB->lastSegs);
//         else
//             SegmentLLSet_subtract_equality(&distA->lastSegs, distB->lastSegs);

//         if(distA->lastSegs == NULL) 
//         {
//             *distType = IGNORED; // there is no mor last segs, dist can be ignored
//         }
//         return;
//     } 

//     if(Distance_eq(distA->dist, distB->dist))
//     {
//         // the number of segments are different, so distA is strongly dominated.
//         // However, distA is still interesting if we want to retrieve all solutions or if want
//         // to retrieve the all best solution (in this case the difference in the number of segments is 1) 
//         // and when the last seg have different sources.
//         if(     ctx->retrieveOption == SC_RETRIEVE_ALL 
//             ||  (ctx->retrieveOption == SC_RETRIEVE_ALL_BEST 
//                 && distA->nbSeg == distB->nbSeg + 1)
//             )
//         {
//             if(ctx->retrieveOption == SC_RETRIEVE_ALL_BEST)
//                 *distType = ONLY_EXTENDABLE; // distA may only be interesting for the next extensions

//             // In this case, we have to remove from the distA, 
//             // the lastSegs that exists in distB->lastSegs
//             SegmentLLSet_subtract_equality(&distA->lastSegs, distB->lastSegs);
//             if(distA->lastSegs == NULL) 
//             {
//                 *distType = IGNORED; // there is no mor last segs, dist can be ignored
//             }
//             return;
//         }
//     }
//     // in the remaining case, the path is strongly dominated, and is not interesting to retrieve
//     SegmentLLSet_free(distA->lastSegs);
//     distA->lastSegs = NULL;
//     *distType = IGNORED;
// }

DistType_t Samcra_feasability(Dict_t *dist_dst, DistVector_t *nextDist)
{    

    DistType_t distType = RETRIEVABLE;

    for (int i = 0; i < dist_dst->actSize; i++)
    {
        if(dist_dst->paths[i].color == IGNORED) continue;
        DistVector_t *destDist = &dist_dst->paths[i].dist;

        if (Distance_eq(*destDist, *nextDist))
            return IGNORED;

        if (is_dominated(*nextDist, *destDist))
        {
            // Samcra_handle_dominated_distance(ctx, nextDist, destDist, &distType);
            return IGNORED;
        } 
        else if (is_dominated(*destDist, *nextDist))
        {
            dist_dst->paths[i].color = IGNORED;
            // Samcra_handle_dominated_distance(ctx, destDist, nextDist, &dist_dst->paths[i].color);
        }
    }

    return distType;
}

// Insert the path in the dict (it may replace an existing one that is marked black)
// return the index of the inserted path
int Samcra_dict_insert_path(Dict_t *dist_v, DistVector_t nextDist, DistType_t distType)
{
    for (int i = 0; i < dist_v->actSize; i++)
    {
        if (dist_v->paths[i].color == IGNORED)
        {
            Dict_replace(dist_v, i, nextDist, distType);
            return i;
        }
    }

    ASSERT_EXIT(dist_v->actSize < dist_v->maxSize, "Dict is full");

    Dict_insert(dist_v, nextDist, distType);
    return dist_v->actSize - 1;
}

void Samcra_update_queue(
    Heap_t *heap, 
    Dict_t *dist_v, 
    int v,
    float samcraLength, 
    DistVector_t nextDist, 
    DistType_t color)
{
    
    int path_index = Samcra_dict_insert_path(dist_v, nextDist, color);
    Heap_insert_key(heap, v, path_index, samcraLength);
}



void Samcra(SamcraContext_t *ctx, int src, DistVector_t cstr, int *init_time)
{
    struct timeval start, stop;
    gettimeofday(&start, NULL);

    
    Samcra_initialize(&ctx->dist, &ctx->heap, src, ctx->topo->nbNode);
    Elem_t u_i;
    int edge_src, edge_dst, path_index;

    gettimeofday(&stop, NULL);
    *init_time = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;

    // While a path could be extended
    while (ctx->heap.heapSize > 0)
    {
        // extract from the list the node that has a potential path to extend 
        // It is called edge_src because it will be the source of the edge used to extend the path
        // path_index is the index of the path in the dictionary that will be extended
        u_i = Heap_extract_min(&ctx->heap);
        edge_src = u_i.nodeId;
        path_index = u_i.index;

        // An ignored path should not be extended
        if ((ctx->dist)[edge_src].paths[path_index].color == IGNORED)
        {
            continue;
        }
        // For all its neighbors
        IntList_t *succs = ctx->topo->nonEmptySlots[edge_src];
        while (succs != NULL)
        {
            edge_dst = succs->value;
            if (edge_dst == edge_src)
            {
                continue;
            }
            // For all the edges with this neighbor
            unsigned char edge_index = 0;
            for (
                Edge_t *edge_weight = ctx->topo->succ[edge_src][edge_dst]; 
                edge_weight != NULL; 
                edge_weight = edge_weight->next, edge_index++)
            {
                // Try to extend the path with this edge, and retrieve the  
                // distance of the extension. This includes all the last segments used by the
                // extension

                DistVector_t nextDist = Distance_add(ctx->dist[edge_src].paths[path_index].dist, edge_weight->dist);
                
                if (cstr.m1 > 0 && nextDist.m1 > cstr.m1) {
                    continue;
                }

                float samcraLength = Slength(nextDist, cstr);

                // We now want to check if the obtained distance is dominated or not. 
                // If not dominated, we add it to the priority queue
                DistType_t distType = Samcra_feasability((ctx->dist) + edge_dst, &nextDist);

                if(distType != IGNORED)
                {
                    Samcra_update_queue(
                        &ctx->heap, (ctx->dist) + edge_dst, edge_dst,
                        samcraLength, nextDist, distType
                    );
                } 
            }
            succs = succs->next;
        }
    }
    
}


void Samcra_clean(SamcraContext_t *ctx)
{
    
    Heap_free(&ctx->heap);

    for (int i = 0; i < ctx->topo->nbNode; i++)
    {
        Dict_free(&ctx->dist[i]);
    }
    free(ctx->dist);
    ctx->dist = NULL;
}