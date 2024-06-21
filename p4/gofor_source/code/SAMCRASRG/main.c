#include <stdio.h>
#include "include/base.h"
#include "include/Llist.h"
#include "include/LabelTable.h"
#include "include/Topology.h"
#include "include/SrGraph.h"
#include "include/Dict.h"
#include "include/Option.h"
#include "include/Samcra.h"
#include "include/LabelTable.h"
//struct Options opt;

void max_of_tab(FILE *output, long int *tab, int size, struct Options opt);

void print_all_iter(FILE *output, int *tab, int size);

void Main_display_all_paths(FILE *output, Dict_t *dist, int nbNodes);

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
    FILE *fileResults = NULL;
    LabelTable_t labels;
    LabelTable_init(&labels);


    if (opt.resFile)
    {
        fileResults = fopen(opt.resFile, "w");
    }
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

    struct timeval start, stop;

    if (opt.loadingMode == LOAD_TOPO)
    {

        if (opt.labelsOrId == LOAD_LABELS)
        {
            topo = Topology_load_from_file_with_labels(opt.filename, opt.accuracy, opt.biDir, &labels);
        }
        else
        {
            topo = Topology_load_from_file_with_ids(opt.filename, opt.accuracy, opt.biDir);
        }

        if (topo == NULL)
        {
            ERROR("Topology can't be load\n");
            return EXIT_FAILURE;
        }

        INFO("Topology succesfully loaded\n");

        gettimeofday(&start, NULL);
        sr = SrGraph_create_from_topology_best_m2(topo);
        gettimeofday(&stop, NULL);

        if (sr == NULL)
        {
            ERROR("The Segment Routing Graph can't be computed\n");
            Topology_free(topo);
            return EXIT_FAILURE;
        }

        INFO("Segment Routing Graph succesfully computed\n");
        INFO("Tranformation takes %ld us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
    }
    else if (opt.loadingMode == LOAD_SR)
    {
        if (opt.srBin)
        {
            sr = SrGraph_load_bin(opt.filename);
        }
        else if (opt.labelsOrId == LOAD_IDS)
        {
            sr = SrGraph_load_with_id(opt.filename, MAX_SR_GRAPH_SIZE, opt.accuracy, opt.biDir);

            if (sr == NULL)
            {
                ERROR("The Segment Routing Graph can't be loaded\n");
                return EXIT_FAILURE;
            }

            INFO("Segment Routing Graph succesfully loaded\n");
        }
        else if (opt.labelsOrId == LOAD_LABELS)
        {
            sr = SrGraph_load_with_label(opt.filename, opt.accuracy, opt.biDir);

            if (sr == NULL)
            {
                ERROR("The Segment Routing Graph can't be loaded\n");
                return EXIT_FAILURE;
            }

            INFO("Segment Routing Graph succesfully loaded\n");
        }
    }
    else
    {
        ERROR("Please choose an available loading mode\n");
    }

    ASSERT(sr, 1, "Segment Routing Graph hasn't been loaded yet so we kill the program\n");

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

    my_m1 maxSpread = SrGraph_get_max_spread(sr);

    if (maxSpread == -1)
    {
        INFO("Segment Routing graph has been transform into one connexe component\n");
        sr = SrGraph_get_biggest_connexe_coponent(sr);
        maxSpread = SrGraph_get_max_spread(sr);
    }

    Dict_t *dist = NULL;
    maxSpread *= SEG_MAX;
    opt.cstr1 *= my_pow(10, opt.accuracy);

    if (opt.allNodes)
    {
        opt.allNodes = MIN(opt.allNodes, sr->nbNode);
        long int *times = malloc(sr->nbNode * sizeof(long int));
        int init_time = 0;
        Samcra(&dist, sr, 0, opt.cstr1, opt.cstr2, &init_time);
        for (int k = 0; k < sr->nbNode; k++)
            {
                Dict_free(dist + k);
            }
            free(dist);

        for (int i = 0; i < opt.allNodes; i++)
        {
            //printf("Iter %d\n", i);
            times[i] = 0;
            dist = NULL;
            int init_time = 0; 
        
            //printf("Iter %d\n", i);
            gettimeofday(&start, NULL);

            Samcra(&dist, sr, i, opt.cstr1, opt.cstr2, &init_time);

            gettimeofday(&stop, NULL);

            //printf("Fin Iter %d\n", i);
            times[i] = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec - init_time;
            //RESULTS("Iter max : %d\n", iterMax[i]);
            if (fileResults)
                Main_display_all_paths(fileResults, dist, sr->nbNode);
            //printf("\r");

            for (int k = 0; k < sr->nbNode; k++)
            {
                Dict_free(dist + k);
            }
            free(dist);
        }

        max_of_tab(output, times, opt.allNodes, opt);
        free(times);
    }
    else
    {
        dist = NULL;
        Samcra(&dist, sr, opt.src, opt.cstr1, opt.cstr2, NULL);
        dist = NULL;
        //printf("params\nsrc = %d\ncstr1 = %d\ncstr2 = %d\ndict size = %d\nmaxSpread = %d\n", opt.src, opt.cstr1, opt.cstr2, max_dict_size, maxSpread);
        gettimeofday(&start, NULL);

        Samcra(&dist, sr, opt.src, opt.cstr1, opt.cstr2, NULL);
        gettimeofday(&stop, NULL);
        long int time = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;

        RESULTS("Execution takes %ld us\n", time);
        if (fileResults)
            Main_display_all_paths(fileResults, dist, sr->nbNode);

        // Main_display_results(output, dist, sr->nbNode, pfront, iter);
        // Main_display_all_paths(output, dist, sr->nbNode);

        for (int k = 0; k < sr->nbNode; k++)
        {
            Dict_free(dist + k);
        }

        free(dist);
    }

    Topology_free(topo);
    SrGraph_free(sr);
    LabelTable_free(&labels);
    
    if (output) fclose(output);
    if (fileResults) fclose(fileResults);
    return 0;
}

void max_of_tab(FILE *output, long int *tab, int size, struct Options opt)
{
    fprintf(output, "NODE_ID C2 NB_THREADS TIME\n");
    for (int i = 0; i < size; i++)
    {
        fprintf(output, "%d %d 1 %ld\n", i, opt.cstr1, tab[i]);
    }
}

void Main_display_all_paths(FILE *output, Dict_t *dist, int nbNodes)
{
    for (int i = 0; i < nbNodes; i++)
    {
        for (int j = 0; j < dist[i].actSize; j++)
        {
            if (dist[i].paths[j].color != BLACK)
            {
                fprintf(output, "%d %d %d %d\n", i, dist[i].paths[j].m1, dist[i].paths[j].m2, dist[i].paths[j].nbSeg);
            }
        }
    }
}
