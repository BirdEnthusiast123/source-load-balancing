#include "../include/Samcra.h"
#include "SegmentLLSet.h"

void Samcra_initialize(Dict_t **d, Heap_t *heap, int src, int nbNodes)
{
    (*d) = malloc(nbNodes * sizeof(Dict_t));
    for (int i = 0; i < nbNodes; i++)
    {
        Dict_init((*d) + i, HEAP_STEP);
    }
    DistVectorSR_t originDist = {
        .nbSeg = 0,
        .dist = {0, 0},
        .lastSegs = NULL};

    Dict_insert((*d) + src, originDist, RETRIEVABLE);

    Heap_init(heap, nbNodes);
    Heap_insert_key(heap, src, 0, 0.0);
}

// IMPORTANT: This function assumes that distA is dominated by distB and that distA and distB are not equal!
void Samcra_handle_dominated_distance(SamcraContext_t *ctx, DistVectorSR_t *distA, DistVectorSR_t *distB, DistType_t *distType)
{
    // since they are not equal and distA is dominated by distB, we have three cases:
    // 1. number of segment are the same (domination is strict on the remaining distances)
    // 2. the remaining distances are the same (domination is strict on the number of segments)
    // 3. domination is strict both on the number of segments and the remaining distances

    if (distA->nbSeg == distB->nbSeg)
    {
        // the remaining distances are different. In this case, the only way nextDist is interesting,
        // is if it is not strongly dominated, ie, when the last seg have different sources.
        // So we have to remove from the nextDist, the lastSegs that exists in dist_dst->paths[i].distSr.lastSegs
        // removal is based on last segment inclusion or equality depending on whether we want to retrieve all solutions or just one
        *distType = ONLY_EXTENDABLE; // distA may only be interesting for the next extensions
        if (ctx->retrieveOption == SC_RETRIEVE_ONE_BEST)
            SegmentLLSet_subtract_inclusion(&distA->lastSegs, distB->lastSegs);
        else
            SegmentLLSet_subtract_equality(&distA->lastSegs, distB->lastSegs);

        if (distA->lastSegs == NULL)
        {
            *distType = IGNORED; // there is no mor last segs, dist can be ignored
        }
        return;
    }

    if (Distance_eq(distA->dist, distB->dist))
    {
        // the number of segments are different, so distA is strongly dominated.
        // However, distA is still interesting if we want to retrieve all solutions or if want
        // to retrieve the all best solution (in this case the difference in the number of segments is 1)
        // and when the last seg have different sources.
        if (ctx->retrieveOption == SC_RETRIEVE_ALL || (ctx->retrieveOption == SC_RETRIEVE_ALL_BEST && distA->nbSeg == distB->nbSeg + 1))
        {
            if (ctx->retrieveOption == SC_RETRIEVE_ALL_BEST)
                *distType = ONLY_EXTENDABLE; // distA may only be interesting for the next extensions

            // In this case, we have to remove from the distA,
            // the lastSegs that exists in distB->lastSegs
            SegmentLLSet_subtract_equality(&distA->lastSegs, distB->lastSegs);
            if (distA->lastSegs == NULL)
            {
                *distType = IGNORED; // there is no mor last segs, dist can be ignored
            }
            return;
        }
    }
    // in the remaining case, the path is strongly dominated, and is not interesting to retrieve
    SegmentLLSet_free(distA->lastSegs);
    distA->lastSegs = NULL;
    *distType = IGNORED;
}

DistType_t Samcra_feasability(SamcraContext_t *ctx, Dict_t *dist_dst, DistVectorSR_t *nextDist)
{

    DistType_t distType = RETRIEVABLE;
    int merge_path_index = -1; // used only if retrieveOption == SC_RETRIEVE_ALL

    for (int i = 0; i < dist_dst->actSize; i++)
    {
        if (dist_dst->paths[i].color == IGNORED)
            continue;

        // if we found a distance with exactly the same distance vector,
        // then we have to merge the lastSegs, because each last seg could be extended in different ways.
        // Merging is either done now, if we are only interested in the
        // best solutions, or later if we want to retrieve all solutions
        // (indeed in the latter case we should first remove from the lastSegs
        // the one for which a better solution has been found)
        DistVectorSR_t *destDist = &dist_dst->paths[i].distSr;
        if (DistanceSr_eq(nextDist, destDist))
        {
            if (ctx->retrieveOption == SC_RETRIEVE_ONE_BEST)
            {
                SegmentLLSet_merge_inclusion(&destDist->lastSegs, &nextDist->lastSegs);
                return IGNORED;
            }
            else if (ctx->retrieveOption == SC_RETRIEVE_ALL_BEST)
            {
                SegmentLLSet_merge_equality(&destDist->lastSegs, &nextDist->lastSegs);
                return IGNORED;
            }
            else
            {
                merge_path_index = i;
            }
        }
        else if (is_dominated_SR(nextDist->nbSeg, nextDist->dist, destDist->nbSeg, destDist->dist, ctx->mode))
        {
            Samcra_handle_dominated_distance(ctx, nextDist, destDist, &distType);
            if (distType == IGNORED)
                return IGNORED;
        }
        else if (is_dominated_SR(destDist->nbSeg, destDist->dist, nextDist->nbSeg, nextDist->dist, ctx->mode))
        {
            Samcra_handle_dominated_distance(ctx, destDist, nextDist, &dist_dst->paths[i].color);
        }
    }

    if (merge_path_index != -1) // now that we remove the useless lastSegs, we can merge
    {
        SegmentLLSet_merge_equality(&dist_dst->paths[merge_path_index].distSr.lastSegs, &nextDist->lastSegs);
        return IGNORED;
    }
    return distType;
}

char isNotOnDAG(SrGraph_t *sr_conv, int currDag, int edge_src, int edge_dst, Edge_t *edge_weight)
{
    return ((sr_conv->m1dists[currDag][edge_src] + edge_weight->dist.m1 != sr_conv->m1dists[currDag][edge_dst]) || (sr_conv->m2dists[currDag][edge_src] + edge_weight->dist.m2 != sr_conv->m2dists[currDag][edge_dst]));
}

int addNecessarySegmentsLoose(
    SamcraContext_t *ctx,
    int edge_src,
    int edge_dst,
    Edge_t *edge_weight,
    unsigned char edge_index,
    Segment_t lastSeg,
    Segment_t *newSeg)
{
    SrGraph_t *sr_conv = ctx->srGraph;
    // By default, the new segment is the same as the last one, extended by the edge
    *newSeg = lastSeg; // For efficiency, the destination is not stored in the segment

    char edgeIsNotOnDag = isNotOnDAG(sr_conv, lastSeg.src, edge_src, edge_dst, edge_weight);

    if (edgeIsNotOnDag || lastSeg.type == ADJACENCY_SEGMENT || !TopologicalProperties(ctx, lastSeg, edge_dst))
    {
        // printf("%d %d %d\n", edgeIsNotOnDag, lastSeg.type == ADJACENCY_SEGMENT, !TopologicalProperties(ctx, lastSeg, edge_dst));
        // A new segment is required, by default it is a node segment
        newSeg->type = NODE_SEGMENT;
        newSeg->src = edge_src;

        if (sr_conv->m1dists[edge_src][edge_dst] != edge_weight->dist.m1 || sr_conv->m2dists[edge_src][edge_dst] != edge_weight->dist.m2 || !TopologicalProperties(ctx, *newSeg, edge_dst))
        {
            newSeg->type = ADJACENCY_SEGMENT;
            newSeg->adjIndex = edge_index;
        }
        return 1;
    }
    return 0;
}

// Check if the extension of the last segment with the given edge require a new segment or not.
// If yes, the function returns 1 and the new segment is stored in newSeg.
int addNecessarySegmentsStrict(
    SamcraContext_t *ctx,
    int edge_src,
    int edge_dst,
    Edge_t *edge_weight,
    unsigned char edge_index,
    Segment_t lastSeg,
    Segment_t *newSeg)
{
    SrGraph_t *sr_conv = ctx->srGraph;
    // By default, the new segment is the same as the last one, extended by the edge
    *newSeg = lastSeg; // For efficiency, the destination is not stored in the segment

    char edgeIsNotOnDag = isNotOnDAG(sr_conv, lastSeg.src, edge_src, edge_dst, edge_weight);
    char segmentHasECMP = sr_conv->has_ecmp[lastSeg.src][edge_dst];

    // no need to check the topological properties because only the path is in the DAG
    if (edgeIsNotOnDag || lastSeg.type == ADJACENCY_SEGMENT || segmentHasECMP || !TopologicalProperties(ctx, lastSeg, edge_dst))
    {
        // A new segment is required, by default it is a node segment
        newSeg->type = NODE_SEGMENT;
        newSeg->src = edge_src;

        char edgeHasECMP = sr_conv->has_ecmp[edge_src][edge_dst];

        if (sr_conv->m1dists[edge_src][edge_dst] != edge_weight->dist.m1 || sr_conv->m2dists[edge_src][edge_dst] != edge_weight->dist.m2 || edgeHasECMP || !TopologicalProperties(ctx, *newSeg, edge_dst))
        {
            newSeg->type = ADJACENCY_SEGMENT;
            newSeg->adjIndex = edge_index;
        }
        return 1;
    }

    return 0;
}

// Insert the path in the dict (it may replace an existing one that is marked black)
// return the index of the inserted path
int Samcra_dict_insert_path(Dict_t *dist_v, DistVectorSR_t nextDist, DistType_t distType)
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
    DistVectorSR_t nextDist,
    DistType_t color)
{
    int path_index = Samcra_dict_insert_path(dist_v, nextDist, color);
    Heap_insert_key(heap, v, path_index, samcraLength);
}

void Samcra_distance_to_extend(
    SamcraContext_t *ctx,
    int path_index,
    int edge_src,
    int edge_dst,
    Edge_t *edge_weight,
    unsigned char edge_index,
    DistVectorSR_t *nextDist,
    DistVectorSR_t *nextDistPlusOneSeg)
{
    // Copy the distance, but add the edge weight and remove the last Segs (computed after)
    *nextDist = ctx->dist[edge_src].paths[path_index].distSr;
    nextDist->dist = Distance_add(nextDist->dist, edge_weight->dist);
    nextDist->lastSegs = NULL;
    SegmentLLSet_t **nextDistLastSegsPtr = &nextDist->lastSegs;

    *nextDistPlusOneSeg = ctx->dist[edge_src].paths[path_index].distSr;
    nextDistPlusOneSeg->nbSeg++;
    nextDistPlusOneSeg->dist = Distance_add(nextDistPlusOneSeg->dist, edge_weight->dist);
    nextDistPlusOneSeg->lastSegs = NULL;

    // store the conversion function in a function pointer
    int (*addNecessarySegments)(SamcraContext_t *, int, int, Edge_t *, unsigned char, Segment_t, Segment_t *);
    if (ctx->encodingType == STRICT_ENCODING)
    {
        addNecessarySegments = &addNecessarySegmentsStrict;
    }
    else
    {
        addNecessarySegments = &addNecessarySegmentsLoose;
    }

    // For each possible last segment, check if we new a new segment or not, and store the results in the two "last segment sets"

    SegmentLLSet_t *lastSegs = ctx->dist[edge_src].paths[path_index].distSr.lastSegs;

    // if there is no previous segments (ie, this is the origin of the path)
    // then we simply add a segment and return
    if (lastSegs == NULL)
    {
        Segment_t prevSeg = {
            .type = NODE_SEGMENT,
            .src = edge_src};
        Segment_t newSeg;
        addNecessarySegments(ctx, edge_src, edge_dst, edge_weight, edge_index, prevSeg, &newSeg);
        SegmentLLSet_add(&nextDistPlusOneSeg->lastSegs, newSeg);
    }

    // otherwise we try to extend each previous segment
    Segment_t nextSeg;
    while (lastSegs != NULL)
    {
        if (addNecessarySegments(ctx, edge_src, edge_dst, edge_weight, edge_index, lastSegs->seg, &nextSeg))
        {
            // if a new segment is required, we only add it to the list if it is not already in it (because it is necessarily the same segment)
            if (nextDistPlusOneSeg->lastSegs == NULL)
            {
                SegmentLLSet_add(&nextDistPlusOneSeg->lastSegs, nextSeg);
            }
        }
        else
        {
            SegmentLLSet_add(nextDistLastSegsPtr, lastSegs->seg);
            nextDistLastSegsPtr = &(*nextDistLastSegsPtr)->next;
        }
        lastSegs = lastSegs->next;
    }
    // printf("extending segments from %d to %d: ", edge_src, edge_dst);
    // SegmentLLSet_fdebug(stdout, ctx->dist[edge_src].paths[path_index].distSr.lastSegs);
    // printf("\n+0: ");
    // SegmentLLSet_fdebug(stdout, nextDist->lastSegs);
    // printf("\n+1: ");
    // SegmentLLSet_fdebug(stdout, nextDistPlusOneSeg->lastSegs);
    // printf("\n");
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
                succs = succs->next;
                continue;
            }

            // For all the edges with this neighbor
            unsigned char edge_index = 0;
            for (
                Edge_t *edge_weight = ctx->topo->succ[edge_src][edge_dst];
                edge_weight != NULL;
                edge_weight = edge_weight->next, edge_index++)
            {

                // test if the adjacency is valid topology-wise

                Segment_t defaultAdjSeg = {
                    .type = ADJACENCY_SEGMENT,
                    .src = edge_src,
                    .adjIndex = edge_index};

                if (!TopologicalProperties(ctx, defaultAdjSeg, edge_dst))
                {
                    continue;
                }

                // Try to extend the path with this edge, and retrieve the
                // distance of the extension. This includes all the last segments used by the
                // extension
                DistVectorSR_t nextDist[2]; // 0 is the distance of the extension, 1 is the distance of the extension + 1 segment
                Samcra_distance_to_extend(ctx, path_index, edge_src, edge_dst, edge_weight, edge_index,
                                          &(nextDist[0]), &(nextDist[1]));

                // if the Samcra "length" > 1.0 then one metric is above the constraint,
                // so ignore the path
                for (int i = 0; i < 2; i++)
                {
                    if (nextDist[i].lastSegs == NULL)
                    {
                        continue;
                    }
                    float samcraLength = Slength(nextDist[i].nbSeg, nextDist[i].dist, cstr, ctx->mode);

                    /*if (edge_dst == 8) {
                         printf("%d ; %d ; %f \n", nextDist[i].dist.m1, nextDist[i].dist.m2, samcraLength);
                         printf("%d, %d\n", ctx->cstr.m1, ctx->cstr.m2);
                     }*/

                    if (nextDist[i].nbSeg > SEG_MAX || nextDist[i].dist.m1 > ctx->cstr.m1)
                    {
                        SegmentLLSet_free(nextDist[i].lastSegs);
                        continue;
                    }

                    // if (samcraLength > 1.0)
                    // {
                    //     SegmentLLSet_free(nextDist[i].lastSegs);
                    //     continue;
                    // }

                    /*
                    if(edge_src == 11 && edge_dst == 10 && nextDist[i].dist.m1 == 97) {
                        printf("extending %d -> %d distance #%d ", edge_src, edge_dst, nextDist[i].nbSeg);
                        fprintDistVector(stdout, nextDist[i].dist);
                        printf("\n");
                        SegmentLLSet_fdebug(stdout, nextDist[i].lastSegs);
                        printf("\n");
                    }
                    //*/
                    // We now want to check if the obtained distance is dominated or not.
                    // If not dominated, we add it to the priority queue
                    DistType_t distType = Samcra_feasability(ctx, (ctx->dist) + edge_dst, &(nextDist[i]));

                    /*
                    if(edge_src == 11 && edge_dst == 10 && nextDist[i].dist.m1 == 97) {
                        printf("Obtained type %s\n", distTypeStr(distType));
                        SegmentLLSet_fdebug(stdout, nextDist[i].lastSegs);
                        printf("\n");
                    }//*/
                    if (distType != IGNORED)
                    {
                        Samcra_update_queue(
                            &ctx->heap, (ctx->dist) + edge_dst, edge_dst,
                            samcraLength, nextDist[i], distType);
                    }
                }
            }

            succs = succs->next;
        }
    }
}

void Samcra_clean(SamcraContext_t *ctx)
{
    for (int i = 0; i < ctx->topo->nbNode; i++)
    {
        for (int j = 0; j < (ctx->dist)[i].actSize; j++)
        {
            SegmentLLSet_free((ctx->dist)[i].paths[j].distSr.lastSegs);
            (ctx->dist)[i].paths[j].distSr.lastSegs = NULL;
        }
    }
    Heap_free(&ctx->heap);

    for (int i = 0; i < ctx->topo->nbNode; i++)
    {
        Dict_free(&ctx->dist[i]);
    }
    free(ctx->dist);
    ctx->dist = NULL;
}