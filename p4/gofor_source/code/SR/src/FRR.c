

#ifdef USE_CASE_FRR

#include "base.h"
#include "Samcra.h"


bool TopologicalProperties(SamcraContext_t *ctx, Segment_t seg, int dest) {
    (void)seg;
    (void)dest;
    (void)ctx;

    if(seg.type == ADJACENCY_SEGMENT) {
        return seg.src != ctx->failedSrc || dest != ctx->failedDst || seg.adjIndex != ctx->failedEdgeIndex;
    }
    if (ctx->srGraph->m2dists[seg.src][dest] == 
              ctx->srGraph->m2dists[seg.src][ctx->failedSrc] 
            + ctx->failedEdgeIGP
            + ctx->srGraph->m2dists[ctx->failedDst][dest]) {
        return false;
    }
    /*
    for (int i = 0; i < ctx->topo->nbNodes; i++) {
        for(int j = 0; j < ctx->topo->nbNodes; j++) {
            if (i == j) continue;
            
        }
    }*/
    return true; // TODO
}

#endif