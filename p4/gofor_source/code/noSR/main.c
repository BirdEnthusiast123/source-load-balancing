#include <stdio.h>
#include "include/base.h"
#include "include/Llist.h"
#include "include/LabelTable.h"
#include "include/Topology.h"
#include "include/SrGraph.h"
#include "include/Dict.h"
#include "include/Option.h"
#include "include/Samcra.h"


//struct Options opt;

void write_execution_times(FILE *output, long int *tab, int size, struct Options opt);

void print_all_iter(FILE *output, int *tab, int size);

void write_results(FILE *output, Dict_t *dist, int nbNodes, int src);

void print_solution(SamcraContext_t *ctx, int dest, my_m1 cstr1);

int main(int argc, char **argv)
{
    struct Options opt;
    if (Option_command_parser(argc, argv, &opt) == -1)
    {
        usage(argv[0]);
        return 1;
    }

    Topology_t *topo = NULL;
    SrGraph_t *sr = NULL;

    FILE *output = stdout;
    FILE* outputRes = NULL;
    if (opt.resultsFile) {
        outputRes = fopen(opt.resultsFile, "w");
     	if(outputRes == NULL) {
     		printf("Couldnt open %s\n",opt.resultsFile);
     		return 1;
     	}
    }
    struct timeval start, stop;
    LabelTable_t labels;
    LabelTable_init(&labels);

    if (opt.output != NULL)
    {
        output = fopen(opt.output, "w");
        if (output == NULL)
        {
            output = freopen(opt.output, "w", output);
            if (output == NULL)
            {
                ERROR("Output file does not exists\n");
            }
        }
    }

    if (opt.labelsOrId == LOAD_LABELS)
    {
        sr = SrGraph_load_with_label(opt.filename, opt.accuracy, opt.biDir, &labels);
    }
    else
    {
        topo = Topology_load_from_file_with_ids(opt.filename, opt.accuracy, opt.biDir);
        sr = SrGraph_load_with_id(opt.filename, topo->nbNode, opt.accuracy, opt.biDir);
    }

    my_m1 maxSpread = SrGraph_get_max_spread(sr);

    if (maxSpread == -1)
    {
        INFO("Segment Routing graph has been transform into one connexe component\n");
        sr = SrGraph_get_biggest_connexe_coponent(sr);
        maxSpread = SrGraph_get_max_spread(sr);
    }

    maxSpread *= SEG_MAX;
    opt.cstr.m1 *= my_pow(10, opt.accuracy);

    if (opt.allNodes)
    {
        opt.allNodes = MIN(opt.allNodes, topo->nbNode);
    }

    SamcraContext_t ctx = {
        .dist = NULL,
        .heap = {},
        .topo = sr,
        .cstr = opt.cstr
    };


    if (opt.allNodes)
    {
        int init_time;
        long int *times = malloc(sr->nbNode * sizeof(long int));

        // warmup
        Samcra(&ctx, 0, opt.cstr, &init_time);
        Samcra_clean(&ctx);


        for (int i = 0; i < opt.allNodes; i++)
        {
            times[i] = 0;

            gettimeofday(&start, NULL);
            Samcra(&ctx, 0, opt.cstr, &init_time);
            gettimeofday(&stop, NULL);

            times[i] = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec - init_time;
            
            if (outputRes) {                
                write_results(outputRes, ctx.dist, sr->nbNode, i);
            }
            Samcra_clean(&ctx);
        }
        write_execution_times(output, times, opt.allNodes, opt);
        free(times);
    }
    else
    {
        int init_time = 0;
        // warmup
        Samcra(&ctx, opt.src, opt.cstr, &init_time);
        Samcra_clean(&ctx);

        //printf("params\nsrc = %d\ncstr1 = %d\ncstr2 = %d\ndict size = %d\nmaxSpread = %d\n", opt.src, opt.cstr.m1, opt.cstr.m2, max_dict_size, maxSpread);
        init_time = 0;

        gettimeofday(&start, NULL);
        Samcra(&ctx, opt.src, opt.cstr, &init_time);
        gettimeofday(&stop, NULL);

        long int time = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec - init_time;

        //time -= init_time;

        RESULTS("Execution takes %ld us\n", time);

        if (outputRes) 
        {                
            write_results(outputRes, ctx.dist, sr->nbNode, opt.src);
        }
        
        Samcra_clean(&ctx);
    }

    Topology_free(topo);
    SrGraph_free(sr);
    LabelTable_free(&labels);
    if (outputRes)
   		fclose(outputRes);
    return 0;
}

void write_execution_times(FILE *output, long int *tab, int size, struct Options opt)
{
    fprintf(output, "NODE_ID C2 TIME\n");
    for (int i = 0; i < size; i++)
    {
        fprintf(output, "%d %d %ld\n", i, opt.cstr.m1, tab[i]);
    }
}

void write_results(FILE *output, Dict_t *dist, int nbNodes, int src)
{
    fprintf(output, "SRC DST COST DELAY\n");
    for (int i = 0; i < nbNodes; i++)
    {
        for (int j = 0; j < dist[i].actSize; j++)
        {
            //if (dist[i].paths[j].color != IGNORED)
            if (dist[i].paths[j].color == RETRIEVABLE)
            {
                fprintf(output, "%d ", src);
                fprintf(output, "%d ", i);
                fprintDistVector(output, dist[i].paths[j].dist);
                fprintf(output, "\n");
            }
        }
    }
}


// DistVector_t find_src_dist(SamcraContext_t *ctx, int dst, DistVector_t dstDist, Segment_t seg)
// {
//     int src = seg.src;
//     for(int i = 0; i < ctx->dist[src].actSize; i++)
//     {
//         DistVector_t srcDist = ctx->dist[src].paths[i].dist;
//         if(seg.type == NODE_SEGMENT 
//             &&  ctx->srGraph->m2dists[src][dst] + srcDist.m2 
//                 == dstDist.m2
//             && ctx->srGraph->m1dists[src][dst] + srcDist.m1 
//                 == dstDist.m1
//             //&& nbSeg == dist[src].paths[j].distSr.nbSeg + 1 //no need to check because if many solution exists with
//             // different number of segments, it means the user want to retrieve all of them
//             && ctx->dist[src].paths[i].color == RETRIEVABLE
//         )
//         {
//             return srcDist;
//         }
//         else if(seg.type == ADJACENCY_SEGMENT)
//         {
//             int edge_index = 0;
//             Edge_t *edge = ctx->topo->succ[src][dst];
//             while(edge) 
//             {
//                 if(edge_index == seg.adjIndex)
//                 {
//                     if(edge->dist.m2 + srcDist.m2 
//                             == dstDist.m2
//                         && edge->dist.m1 + srcDist.m1 
//                             == dstDist.m1
//                         //&& nbSeg == dist[src].paths[j].distSr.nbSeg + 1
//                         && ctx->dist[src].paths[i].color == RETRIEVABLE
//                     )
//                     {
//                         return srcDist;
//                     }
//                 }
//                 edge = edge->next;
//                 edge_index++;
//             }
//         }
//     }
//     fprintf(stderr, "Error: no path found from %d to %d with dest distance ", src, dst);
//     fprintDistVector(stderr, dstDist);
//     fprintf(stderr, " and segtype %s\n", seg.type == NODE_SEGMENT ? "Node" : "Adj");
//     exit(1);
// }


// void recursive_solution(SamcraContext_t *ctx, Dag_t *dag, int dst, DistVector_t dstDist, int level, DistVector_t *ignoredPred)
// {    
//     // TODO: check that the number of seg is ok (based on the level)

//     // ignored if already computed
//     if(Dag_get(dag, dst, dstDist) != NULL)
//     {
//         //for(int l = 0; l < level; l++) printf("| ");
//         //printf("Already computed for dst %d and dist ", dst);
//         //fprintDistVector(stdout, dstDist);
//         //printf("\n");
//         return;
//     }
//     //for(int l = 0; l < level; l++) printf("| ");
//     //printf("Computing for dst %d and dist ", dst);
//     //fprintDistVector(stdout, dstDist);
//     //printf("\n");

//     // find the index of the path that leads to the current path
//     for (int j = 0; j < ctx->dist[dst].actSize; j++)
//     {
//         DistVector_t jDist = ctx->dist[dst].paths[j].distSr.dist;
//         if(ctx->dist[dst].paths[j].color == RETRIEVABLE && Distance_eq(jDist, dstDist))
//         {
//             Dag_addEdges(dag, dst, ctx->dist[dst].paths[j].distSr.lastSegs, dstDist);
//         }
        
//     }

//     SegmentLLSet_t *lastSegs = Dag_get(dag, dst, dstDist);
//     if(lastSegs == NULL) 
//     {
//         // this is the source of the DAG
//         return;
//     }
//     // create new ignoredPred array
//     DistVector_t *newIgnoredPred = malloc(ctx->topo->nbNode * sizeof(*newIgnoredPred));
//     for(int i = 0; i < ctx->topo->nbNode; i++)
//     {
//         newIgnoredPred[i] = Distance_maxValue();
//     }
    
//     while(lastSegs)
//     {
//         int src = lastSegs->seg.src;
//         if(lastSegs->seg.type == NODE_SEGMENT)
//             newIgnoredPred[src] = find_src_dist(ctx, dst, dstDist, lastSegs->seg);
//         lastSegs = lastSegs->next;
//     }


//     lastSegs = Dag_get(dag, dst, dstDist);
//     while(lastSegs)
//     {
//         int src = lastSegs->seg.src;
//         DistVector_t srcDist = find_src_dist(ctx, dst, dstDist, lastSegs->seg);

//         if(lastSegs->seg.type == NODE_SEGMENT && Distance_eq(ignoredPred[src], srcDist))
//         {
//             // ignore this path because the predecessor has already been 
//             // handled by our parent

//             for(int l = 0; l < level; l++) printf("| ");
//             printf("%d<-%d %s(", dst, src, lastSegs->seg.type == NODE_SEGMENT ? "Node" : "Adj");
//             DistVector_t diff = Distance_sub(dstDist, srcDist);
//             fprintDistVector(stdout, diff);
//             printf(")");
//             printf(" <- (");
//             fprintDistVector(stdout, srcDist);
//             printf(") IGNORED TRIANGLE\n");



//             lastSegs = lastSegs->next;
//             continue;
//         }

        
        
//         for(int l = 0; l < level; l++) printf("| ");
//         printf("%d<-%d %s(", dst, src, lastSegs->seg.type == NODE_SEGMENT ? "Node" : "Adj");
//         DistVector_t diff = Distance_sub(dstDist, srcDist);
//         fprintDistVector(stdout, diff);
//         printf(")");
//         printf(" <- (");
//         fprintDistVector(stdout, srcDist);
//         printf(")\n");


//         recursive_solution(ctx, dag, src, srcDist, level + 1, newIgnoredPred);
        
//         if(ctx->retrieveOption == SC_RETRIEVE_ONE_BEST)
//         {
//             // only one is enough
//             break;
//         }
//         lastSegs = lastSegs->next;
//     }
//     free(newIgnoredPred);
// }

// void print_solution(SamcraContext_t *ctx, int dst, my_m1 cstr1)
// {
    
//     Dag_t *dag = Dag_new(ctx->topo->nbNode, ctx->cstr);

//     bool found = false;
//     DistVector_t finalDist;
//     for (int j = 0; j < ctx->dist[dst].actSize; j++)
//     {
//         //if (dist[dst].paths[j].color == NON_DOMINATED)
//         {
//             if(ctx->dist[dst].paths[j].distSr.dist.m1 == cstr1)
//             { 
//                 //Dag_addEdges(dag, dst, ctx->dist[dst].paths[j].distSr.lastSegs, ctx->dist[dst].paths[j].distSr.dist);   
//                 finalDist = ctx->dist[dst].paths[j].distSr.dist;
//                 found = true;
//             }
//             else
//             {
//                 //printf("other solution #%d ", dist[dst].paths[j].distSr.nbSeg);
//                 //fprintDistVector(stdout, dist[dst].paths[j].distSr.dist);
//                 //printf(" %s\n", distTypeStr(dist[dst].paths[j].color));
//             }
//         }
//     }

//     if(!found) 
//     {
//         printf("No solution found with m1 = %d towards %d\n", cstr1, dst);
//     }
//     else {
//         printf("== found solution == -> %d with distance ", dst);
//         fprintDistVector(stdout, finalDist);
//         printf("\n");

//         DistVector_t *ignoredPred = calloc(ctx->topo->nbNode, sizeof(*ignoredPred));
//         for(int i = 0; i < ctx->topo->nbNode; i++)
//         {
//             ignoredPred[i] = Distance_maxValue();
//         }   
//         recursive_solution(ctx, dag, dst, finalDist, 0, ignoredPred);
//         free(ignoredPred);
//     }
// }
