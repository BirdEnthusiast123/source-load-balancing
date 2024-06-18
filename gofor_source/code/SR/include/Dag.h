#pragma once
#include <stdlib.h>
#include "SegmentLLSet.h"

typedef struct Dag
{
    // an array of predecessors (lastSegs). There is a possible predecessor for each node and each
    // element of the Pareto from. a predecessor is a linked list of last segments.
    SegmentLLSet_t ***lastSegs;
    int nbNode;
    DistVector_t maxDist;
} Dag_t;

// create a new dag with n nodes
Dag_t *Dag_new(int n, DistVector_t maxDist)
{
    Dag_t *dag = malloc(sizeof(Dag_t));
    dag->lastSegs = calloc(n, sizeof(SegmentLLSet_t **));
    dag->nbNode = n;
    dag->maxDist = maxDist;
    return dag;
}

// add an edges towards dst
void Dag_addEdges(Dag_t *dag, int dst, SegmentLLSet_t *lastSegs, DistVector_t dist)
{
    if (dag->lastSegs[dst] == NULL)
    {
        dag->lastSegs[dst] = calloc(Distance_hash(dag->maxDist) + 1, sizeof(SegmentLLSet_t *));
    }
    SegmentLLSet_t *lastSegsCopy = SegmentLLSet_copy(lastSegs, dag->lastSegs[dst][Distance_hash(dist)]);
    dag->lastSegs[dst][Distance_hash(dist)] = lastSegsCopy;
    // TODO: also keep a linked list of all the set distances for easier iteration
}

// get all the last segments for a given node and distance
SegmentLLSet_t *Dag_get(Dag_t *dag, int node, DistVector_t dist)
{
    if (dag->lastSegs[node] == NULL)
    {
        return NULL;
    }
    return dag->lastSegs[node][Distance_hash(dist)];
}

// free the dag
void Dag_free(Dag_t *dag)
{
    for (int i = 0; i < dag->nbNode; i++)
    {
        if (dag->lastSegs[i] != NULL)
        {
            for (int j = 0; j < Distance_hash(dag->maxDist) + 1; j++)
            {
                if (dag->lastSegs[i][j] != NULL)
                {
                    SegmentLLSet_free(dag->lastSegs[i][j]);
                }
            }
            free(dag->lastSegs[i]);
        }
    }
    free(dag->lastSegs);
    free(dag);
}

void Dag_print(Dag_t *dag)
{
    for (int i = 0; i < dag->nbNode; i++)
    {
        if (dag->lastSegs[i] != NULL)
        {
            for (int j = 0; j < Distance_hash(dag->maxDist) + 1; j++)
            {
                if (dag->lastSegs[i][j] != NULL)
                {
                    printf("Node: %d, Distance: %d\n", i, j);
                    SegmentLLSet_fdebug(stdout, dag->lastSegs[i][j]);
                    printf("\n\n");
                }
            }
        }
    }
}

void Dag_to_dot_like(Dag_t *dag)
{
    for (int i = 0; i < dag->nbNode; i++)
    {
        if (dag->lastSegs[i] != NULL)
        {
            for (int j = 0; j < Distance_hash(dag->maxDist) + 1; j++)
            {
                if (dag->lastSegs[i][j] != NULL)
                {
                    SegmentLLSet_t *lastSegs = dag->lastSegs[i][j];
                    while (lastSegs)
                    {
                        fprintf(stdout, "%d %d %s %d\n", 
                                lastSegs->seg.src, 
                                i, 
                                lastSegs->seg.type == NODE_SEGMENT ? "Node" : "Adj", 
                                j);
                        lastSegs = lastSegs->next;
                    }
                }
            }
        }
    }
}
