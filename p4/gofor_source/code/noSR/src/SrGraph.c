#include "../include/SrGraph.h"

SrGraph_t *SrGraph_init(int nbNodes)
{
    SrGraph_t *graph = malloc(sizeof(SrGraph_t));
    ASSERT(graph, NULL);

    graph->nbNode = nbNodes;

    graph->pred = malloc(nbNodes * sizeof(Edge_t **));
    ASSERT(graph->pred, NULL);

    graph->succ = malloc(nbNodes * sizeof(Edge_t **));
    ASSERT(graph->succ, NULL);

    for (int i = 0; i < nbNodes; i++)
    {
        graph->succ[i] = malloc(nbNodes * sizeof(Edge_t *));
        ASSERT(graph->succ[i], NULL);

        graph->pred[i] = malloc(nbNodes * sizeof(Edge_t *));
        ASSERT(graph->pred[i], NULL);
        for (int j = 0; j < nbNodes; j++)
        {
            graph->succ[i][j] = NULL;
            graph->pred[i][j] = NULL;
        }
    }

    graph->m1dists = malloc(nbNodes * sizeof(my_m1 *));
    ASSERT(graph->m1dists, NULL);

    graph->m2dists = malloc(nbNodes * sizeof(my_m2 *));
    ASSERT(graph->m2dists, NULL);

    graph->has_ecmp = malloc(nbNodes * sizeof(bool *));
    ASSERT(graph->has_ecmp, NULL);

    for (int i = 0; i < nbNodes; i++)
    {
        graph->has_ecmp[i] = malloc(nbNodes * sizeof(bool));
        ASSERT(graph->has_ecmp[i], NULL);

        graph->m2dists[i] = malloc(nbNodes * sizeof(my_m2));
        ASSERT(graph->m2dists[i], NULL);

        graph->m1dists[i] = malloc(nbNodes * sizeof(my_m1));
        ASSERT(graph->m1dists[i], NULL);
    }

    graph->nonEmptySlots = malloc(nbNodes * sizeof(IntList_t *));
    for (int i = 0; i < nbNodes; i++)
        graph->nonEmptySlots[i] = NULL;
    return graph;
}

void SrGraph_free(SrGraph_t *graph)
{
    for (int i = 0; i < graph->nbNode; i++)
    {
        free(graph->m2dists[i]);
        free(graph->m1dists[i]);
        free(graph->has_ecmp[i]);

        for (int j = 0; j < graph->nbNode; j++)
        {
            Edge_free(graph->pred[i][j]);
            Edge_free(graph->succ[i][j]);
        }

        free(graph->pred[i]);
        free(graph->succ[i]);
    }

    free(graph->m2dists);
    free(graph->m1dists);
    free(graph->has_ecmp);
    free(graph->pred);
    free(graph->succ);
    for (int i = 0; i < graph->nbNode; i++)
    {
        IntListFree(graph->nonEmptySlots[i]);
    }
    free(graph->nonEmptySlots);
    // IntListFree(graph->nonEmptySlots);
    free(graph);
}

SrGraph_t *SrGraph_create_from_topology_best_m2(Topology_t *topo)
{
    SrGraph_t *graph = SrGraph_init(topo->nbNode);

    if (graph == NULL)
    {
        ERROR("Segment Routing Graph can't be initialized\n");
        return NULL;
    }

    for (int i = 0; i < topo->nbNode; i++)
    {
        //printf("Computing dijkstra for node %d\n", i);
        dikjstra_best_m2(&graph->succ, &graph->pred, topo->succ, i,
                         &(graph->m1dists[i]), &(graph->m2dists[i]), &(graph->has_ecmp[i]), topo->nbNode);
    }

    // for (int i = 0 ; i < topo->nbNode ; i++) {
    //     for (Llist_t* tmp = topo->succ[i] ; tmp != NULL ; tmp = tmp->next) {
    //         my_m1 m1 = tmp->infos.m1;
    //         my_m2 m2 = tmp->infos.m2;
    //         int dst = tmp->infos.edgeDst;
    //         if (m1 < graph->m1dists[i][dst]) {
    //             graph->succ[i][dst] = Edge_add(graph->succ[i][dst], m1, m2);
    //             graph->pred[i][dst] = Edge_add(graph->pred[i][dst], m1, m2);
    //         }
    //     }
    // }

    return graph;
}

void SrGraph_check_m1(SrGraph_t *sr)
{
    int nbEdge = 0;
    for (int i = 0; i < sr->nbNode; i++)
    {
        for (int j = 0; j < sr->nbNode; j++)
        {
            nbEdge = 0;
            for (Edge_t *tmp = sr->succ[i][j]; tmp != NULL; tmp = tmp->next)
            {
                nbEdge++;
            }
        }
    }
}

SrGraph_t *SrGraph_create_from_topology_best_m1(Topology_t *topo)
{
    SrGraph_t *graph = SrGraph_init(topo->nbNode);

    if (graph == NULL)
    {
        ERROR("Segment Routing Graph can't be initialized\n");
        return NULL;
    }

    for (int i = 0; i < topo->nbNode; i++)
    {
        dikjstra_best_m1(&graph->succ, &graph->pred, topo->succ, i,
                         &(graph->m1dists[i]), &(graph->m2dists[i]), topo->nbNode);
    }

    for (int i = 0; i < topo->nbNode; i++)
    {
        for (Llist_t *tmp = topo->succ[i]; tmp != NULL; tmp = tmp->next)
        {
            my_m1 m1 = tmp->infos.m1;
            my_m2 m2 = tmp->infos.m2;
            int dst = tmp->infos.edgeDst;
            if (m1 < graph->m1dists[i][dst])
            {
                graph->succ[i][dst] = Edge_add(graph->succ[i][dst], m1, m2);
                graph->pred[i][dst] = Edge_add(graph->pred[i][dst], m1, m2);
            }
        }
    }

    return graph;
}

SrGraph_t *SrGraph_merge_sr_graph(SrGraph_t *best_m1, SrGraph_t *best_m2, int size)
{
    SrGraph_t *flex = SrGraph_init(size);

    int nbNode = size;
    for (int src = 0; src < nbNode; src++)
    {
        for (int dst = 0; dst < nbNode; dst++)
        {
            if (src == dst)
            {
                continue;
            }
            flex->succ[src][dst] = Edge_merge_flex(best_m1->succ[src][dst], best_m2->succ[src][dst]);
            flex->pred[dst][src] = Edge_merge_flex(best_m1->pred[dst][src], best_m2->pred[dst][src]);
        }
    }
    return flex;
}

SrGraph_t *SrGraph_create_flex_algo(Topology_t *topo)
{
    if (topo == NULL)
    {
        ERROR("Can't compute with an empty topology\n");
        return NULL;
    }
    SrGraph_t *best_m1 = SrGraph_create_from_topology_best_m1(topo);
    SrGraph_t *best_m2 = SrGraph_create_from_topology_best_m2(topo);
    SrGraph_t *flex = SrGraph_merge_sr_graph(best_m1, best_m2, topo->nbNode);

    SrGraph_free(best_m1);
    SrGraph_free(best_m2);

    return flex;
}

SrGraph_t *SrGraph_add_adjacencies(SrGraph_t *graph, Topology_t *topo)
{
    for (int i = 0; i < graph->nbNode; i++)
    {
        for (Llist_t *edge = topo->succ[i]; edge != NULL; edge = edge->next)
        {
            my_m1 m1 = edge->infos.m1;
            my_m2 m2 = edge->infos.m2;
            int dst = edge->infos.edgeDst;

            if (graph->succ[i][dst] != NULL)
            {

                if (graph->succ[i][dst]->next == NULL)
                {
                    if (m1 < graph->succ[i][dst]->dist.m1)
                    {
                        graph->succ[i][dst] = Edge_add(graph->succ[i][dst], m1, m2);
                    }
                }
                else
                {

                    if (m1 < graph->succ[i][dst]->dist.m1 && m2 < graph->succ[i][dst]->next->dist.m2)
                    {
                        graph->succ[i][dst] = Edge_add(graph->succ[i][dst], m1, m2);
                    }
                }
            }
        }
    }

    return graph;
}

SrGraph_t *SrGraph_load_with_id(char *filename, int nbNodes, int accuracy, char bi_dir)
{
    SrGraph_t *sr = SrGraph_init(nbNodes);

    if (sr == NULL)
    {
        ERROR("Segment Routing Graph can't be initialized\n");
        return NULL;
    }

    sr->nbNode = 0;

    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        ERROR("File %s can't be found in the curent directory\n", filename);
        return NULL;
    }

    ASSERT(file, NULL);

    int src, dst;
    my_m2 m2;
    char line[1024];
    double m1;
    int nbLine = 1;
    int type;

    while (fgets(line, 1024, file))
    {
        if (sscanf(line, "%d %d %lf %d\n", &src, &dst, &m1, &m2))
        {
            m1 *= my_pow(10, accuracy);
            m2 = MAX(m2, 1);
            sr->succ[src][dst] = Edge_add(sr->succ[src][dst], m1, m2);
            //sr->pred[dst][src] = Edge_add(sr->pred[dst][src], m1, m2);
            sr->nbNode = MAX(sr->nbNode, src + 1);
            sr->nbNode = MAX(sr->nbNode, dst + 1);
            IntListAdd(dst, &(sr->nonEmptySlots[src]));

            if (bi_dir)
            {
                sr->succ[dst][src] = Edge_add(sr->succ[dst][src], m1, m2);
             //   sr->pred[src][dst] = Edge_add(sr->pred[src][dst], m1, m2);
                IntListAdd(src, &(sr->nonEmptySlots[dst]));
            }
        }
        else if (sscanf(line, "%d %d %lf %d %d\n", &src, &dst, &m1, &m2, &type))
        {
            if (type == ADJACENCY_SEGMENT) continue;
            m1 *= my_pow(10, accuracy);
            m2 = MAX(m2, 1);
            sr->succ[src][dst] = Edge_add(sr->succ[src][dst], m1, m2);
            sr->nbNode = MAX(sr->nbNode, src + 1);
            sr->nbNode = MAX(sr->nbNode, dst + 1);
            IntListAdd(dst, &(sr->nonEmptySlots[src]));

            if (bi_dir)
            {
                sr->succ[dst][src] = Edge_add(sr->succ[dst][src], m1, m2);
                IntListAdd(src, &(sr->nonEmptySlots[dst]));
            }
        }
        else
        {
            ERROR("Can't load line %d : your file might not have the good format : \n\t[source_node]  [destination_node]  [delay]  [IGP_weight]\n", nbLine);
            SrGraph_free(sr);
            return NULL;
        }
        nbLine++;
    }

    fclose(file);
    return sr;
}


void SrGraph_save_bin(SrGraph_t* sr, char* filename)
{
    FILE *out = fopen(filename, "wb");
    if(out == NULL){
        ERROR("Unable to open the Binary file <%s>\n", filename);
    }
    fwrite(&(sr->nbNode), sizeof(sr->nbNode), 1, out);

    // const Edge_t emptyEdge = {.dist.m1=-1};
    my_m1 separator = -1;
    for (int i = 0 ; i < sr->nbNode ; i++) {
        for (int j = 0 ; j < sr->nbNode ; j++) {
            if (i == j) {
                continue;
            }
            fwrite(&(sr->m1dists[i][j]), sizeof(sr->m1dists[i][j]), 1, out);
            fwrite(&(sr->m2dists[i][j]), sizeof(sr->m2dists[i][j]), 1, out);
            fwrite(&(sr->has_ecmp[i][j]), sizeof(sr->has_ecmp[i][j]), 1, out);
            fwrite(&separator, sizeof(separator), 1, out);
        }
    }
    fclose(out);
}

SrGraph_t* SrGraph_load_bin(char* filename)
{
    FILE *in = fopen(filename, "rb");
    if(in == NULL){
        ERROR("Unable to open the Binary file <%s>\n", filename);
        return NULL;
    }
    int nbNode = 0;
    if(fread(&(nbNode), sizeof(nbNode), 1, in) == 0)
    {
        ERROR("Unable to read nbNode\n");
    }
    
    SrGraph_t* sr = SrGraph_init(nbNode);
    if(sr == NULL) return NULL;

    for (int i = 0 ; i < sr->nbNode ; i++) {
        for (int j = 0 ; j < sr->nbNode ; j++) {
            if (i == j) {
                continue;
            }
            // Edge_t edge = {.id=1, .dist = {.m1=0, .m2=0}};
            bool ecmp;
            my_m1 m1;
            my_m2 m2;

            if(fread(&m1, sizeof(m1), 1, in) == 0)
            {
                ERROR("Unable to read edge %d %d\n",i,j);
            }
            while(m1 != -1)
            {
                
                if(fread(&m2, sizeof(m2), 1, in) == 0)
                {
                    ERROR("Unable to read edge %d %d\n",i,j);
                }
                if(fread(&ecmp, sizeof(ecmp), 1, in) == 0)
                {
                    ERROR("Unable to read edge %d %d\n",i,j);
                }
                
                sr->m1dists[i][j] = m1;
                sr->m2dists[i][j] = m2;
                sr->has_ecmp[i][j] = ecmp;

                if(fread(&m1, sizeof(m1), 1, in) == 0)
                {
                    ERROR("Unable to read edge %d %d\n",i,j);
                }
            }
        }
    }
    fclose(in);
    return sr;
}


SrGraph_t *SrGraph_load_with_label(char *filename, int accuracy, char bi_dir, LabelTable_t *labels)
{

    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        ERROR("File %s can't be found in the current directory\n", filename);
        return NULL;
    }

    char srcLabel[128], dstLabel[128];
    my_m2 m2;
    char line[1024];
    double m1;
    int src, dst;
    int nbNode = 0;
    int nbLine = 1;

    while (fgets(line, 1024, file))
    {
        if (sscanf(line, "%s %s %lf %d\n", &srcLabel[0], &dstLabel[0], &m1, &m2) == 4)
        {
            src = LabelTable_add_node(labels, srcLabel);
            dst = LabelTable_add_node(labels, dstLabel);
            nbNode = MAX(nbNode, labels->nextNodeId);
        }
        else
        {
            ERROR("Can't load line %d : your file might not have the good format : \n\t[source_node]  [destination_node]  [delay]  [IGP_weight]\n", nbLine);
            return NULL;
        }
        nbLine++;
    }
    fclose(file);

    LabelTable_sort(labels);

    SrGraph_t *sr = SrGraph_init(nbNode);

    if (sr == NULL)
    {
        ERROR("Segment Routing Graph can't be initialized\n");
        return NULL;
    }

    file = fopen(filename, "r");

    if (file == NULL)
    {
        ERROR("File %s can't be found in the current directory\n", filename);
        return NULL;
    }

    nbLine = 1;

    while (fgets(line, 1024, file))
    {
        if (sscanf(line, "%s %s %lf %d\n", &srcLabel[0], &dstLabel[0], &m1, &m2) == 4)
        {
            src = LabelTable_get_id(labels, srcLabel);
            dst = LabelTable_get_id(labels, dstLabel);
            m2 = MAX(m2, 1);
            m1 *= my_pow(10, accuracy);
            sr->succ[src][dst] = Edge_add(sr->succ[src][dst], m1, m2);
            sr->pred[dst][src] = Edge_add(sr->pred[dst][src], m1, m2);
            IntListAdd(dst, &(sr->nonEmptySlots[src]));
            if (bi_dir)
            {
                sr->succ[dst][src] = Edge_add(sr->succ[dst][src], m1, m2);
                sr->pred[src][dst] = Edge_add(sr->pred[src][dst], m1, m2);
                IntListAdd(src, &(sr->nonEmptySlots[dst]));
            }
        }
        else
        {
            ERROR("Can't load line %d : your file might not have the good format : \n\t[source_node]  [destination_node]  [delay]  [IGP_weight]\n", nbLine);
            SrGraph_free(sr);
            return NULL;
        }
        nbLine++;
    }

    fclose(file);

    return sr;
}

SrGraph_t *SrGraph_create_crash_test(int nbNode, int nbPlinks)
{
    SrGraph_t *graph = SrGraph_init(nbNode);

    if (graph == NULL)
    {
        ERROR("Segment Routing Graph can't be initialized\n");
    }

    for (int i = 0; i < nbNode; i++)
    {
        for (int j = 0; j < nbNode; j++)
        {
            if (i == j)
            {
                continue;
            }
            my_m1 m1 = 1;
            my_m2 m2 = 1;
            for (int k = 0; k < nbPlinks; k++)
            {
                graph->succ[i][j] = Edge_new(graph->succ[i][j], m1, m2);
                graph->pred[j][i] = Edge_new(graph->pred[j][i], m1, m2);
            }
        }
    }
    return graph;
}


// void SrGraph_print(SrGraph_t *sr)
// {
//     for (int i = 0; i < sr->nbNode; i++)
//     {
//         for (int j = 0; j < sr->nbNode; j++)
//         {
//             if (i == j)
//             {
//                 continue;
//             }
//             for (Edge_t *edge = sr->succ[i][j]; edge != NULL; edge = edge->next)
//             {
//                 printf("%d %d %d %d %d %d\n", i, j, edge->m1, edge->m2, sr->has_ecmp[i][j], sr->has_ecmp[i][j]);
//             }
//             // fprintf(output, "%d -> %d : ", i, j);
//             // Edge_print_list(sr->succ[i][j], output);
//         }
//     }
// }


void SrGraph_print_in_file(SrGraph_t *sr, FILE *output)
{
    for (int i = 0; i < sr->nbNode; i++)
    {
        for (int j = 0; j < sr->nbNode; j++)
        {
            if (i == j)
            {
                continue;
            }
            for (Edge_t *edge = sr->succ[i][j]; edge != NULL; edge = edge->next)
            {
                fprintf(output, "%d %d %d %d %d\n", i, j, edge->dist.m1, edge->dist.m2, sr->has_ecmp[i][j]);
            }
            // fprintf(output, "%d -> %d : ", i, j);
            // Edge_print_list(sr->succ[i][j], output);
        }
    }
}

my_m1 SrGraph_get_max_spread(SrGraph_t *sr)
{
    my_m1 max = 0;
    my_m1 minM1 = INF;
    my_m2 minM2 = INF;
    my_m2 maxM2 = 0;
    int nbzero = 0;
    int nbEdge = 0;
    for (int i = 0; i < sr->nbNode; i++)
    {
        for (int j = 0; j < sr->nbNode; j++)
        {
            //printf("debut (%d ; %d)\n", i, j);
            for (Edge_t *tmp = sr->pred[i][j]; tmp != NULL; tmp = tmp->next)
            {
                // if (tmp->m1 == INF) {
                //     return -1;
                // }
                minM1 = MIN(minM1, tmp->dist.m1);
                minM2 = MIN(minM2, tmp->dist.m2);
                maxM2 = MAX(maxM2, tmp->dist.m2);
                max = MAX(max, tmp->dist.m1);
                if (!tmp->dist.m1)
                {
                    nbzero++;
                }
                nbEdge++;
            }
            //printf("fin (%d ; %d)\n", i, j);
        }
    }

    INFO("There are %d adjacencies\n", nbEdge - (sr->nbNode) * (sr->nbNode - 1));
    RESULTS("Max delay : %d\n", max);
    RESULTS("Min delay : %d\n", minM1);
    RESULTS("Max igp : %d\n", maxM2);
    RESULTS("Min igp : %d\n", minM2);
    RESULTS("NB edges with 0 : %d\n", nbzero);
    return max;
}

bool SrGraph_is_connex(SrGraph_t *sr)
{
    for (int i = 0; i < sr->nbNode; i++)
    {
        for (int j = 0; j < sr->nbNode; j++)
        {
            //printf("debut (%d ; %d)\n", i, j);
            for (Edge_t *tmp = sr->pred[i][j]; tmp != NULL; tmp = tmp->next)
            {
                if (tmp->dist.m1 == INF)
                {
                    return false;
                }
            }
            //printf("fin (%d ; %d)\n", i, j);
        }
    }
    return true;
}

SrGraph_t *SrGraph_get_biggest_connexe_coponent(SrGraph_t *sr)
{
    int *nbNeighbors = malloc(sr->nbNode * sizeof(int));
    ASSERT(nbNeighbors, NULL);

    memset(nbNeighbors, 0, sr->nbNode * sizeof(int));

    for (int i = 0; i < sr->nbNode; i++)
    {
        for (int j = 0; j < sr->nbNode; j++)
        {
            for (Edge_t *edge = sr->succ[i][j]; edge != NULL; edge = edge->next)
            {
                if (edge->id == 1 && edge->dist.m1 != INF)
                {
                    nbNeighbors[i]++;
                }
            }
        }
    }

    int maxNeighbor = 0;
    for (int i = 0; i < sr->nbNode; i++)
    {
        maxNeighbor = MAX(maxNeighbor, nbNeighbors[i]);
    }

    int *nodes = malloc(sr->nbNode * sizeof(int));
    ASSERT(nodes, NULL);

    int nbNodes = 0;

    memset(nodes, 0, sr->nbNode * sizeof(int));

    for (int i = 0; i < sr->nbNode; i++)
    {
        if (nbNeighbors[i] == maxNeighbor)
        {
            nodes[nbNodes++] = i;
        }
    }

    SrGraph_t *newSr = SrGraph_init(nbNodes);

    for (int i = 0; i < nbNodes; i++)
    {
        for (int j = 0; j < nbNodes; j++)
        {
            newSr->succ[i][j] = Edge_copy(sr->succ[nodes[i]][nodes[j]]);
            newSr->pred[i][j] = Edge_copy(sr->pred[nodes[i]][nodes[j]]);
        }
    }

    SrGraph_free(sr);
    free(nbNeighbors);
    free(nodes);

    return newSr;
}
