#include "../include/Option.h"
#include "Samcra.h"

int Option_command_parser(int argc, char **argv, struct Options *opt)
{
    int optValue = 0;
    const char *optString = "f:a:o:P:L:b:1:2:r:E:e::R:B:tsSlui";
    const struct option long_options[] = {
        {"file", required_argument, NULL, 'f'},
        {"topo", no_argument, NULL, 't'},
        {"sr", no_argument, NULL, 's'},
        {"labels", no_argument, NULL, 'l'},
        {"id", no_argument, NULL, 'i'},
        {"accu", required_argument, NULL, 'a'},
        {"output", required_argument, NULL, 'o'},
        {"bi-dir", no_argument, NULL, 'b'},
        {"cstr1", required_argument, NULL, '1'},
        {"cstr2", required_argument, NULL, '2'},
        {"src", required_argument, NULL, 'n'},
        {"all-nodes", optional_argument, NULL, 'e'},
        {"help", no_argument, NULL, 'h'},
        {"sr-conv", required_argument, NULL, 'c'},
        {"sr-bin", no_argument, NULL, 'S'},
        {"results", required_argument, NULL, 'r'},
        {"encoding", required_argument, NULL, 'E'},
        {"mode", required_argument, NULL, 'm'},
        {"retrieve", required_argument, NULL, 'R'},
        {"print-solution", required_argument, NULL, 'P'},
        {"save-sr-bin", required_argument, NULL, 'B'},
        {"print-segment-list", required_argument, NULL, 'L'},
        {0, 0, 0, 0}};

    opt->output = NULL;
    opt->biDir = UNI_DIRECTIANNAL;
    opt->filename = NULL;
    Distance_default_cstr(&opt->cstr);
    opt->loadingMode = -1;
    opt->accuracy = 1;
    opt->labelsOrId = -1;
    opt->src = -1;
    opt->allNodes = 0;
    opt->srConv = NULL;
    opt->srBin = 0;
    opt->resultsFile = NULL;
    opt->encoding = LOOSE_ENCODING;
    opt->retrieve = SC_RETRIEVE_ONE_BEST;
    opt->printSolution = NULL;
    opt->printSegList = NULL;
    opt->saveSrGraphBin = NULL;
    opt->mode = PARETO;

    while ((optValue = getopt_long(argc, argv, optString, long_options, NULL)) != -1)
    {
        switch (optValue)
        {
        case 'h':
            return -1;
            break;

        case 'e':
            // the computation will be done on all nodes
            if (optarg != NULL)
            {
                opt->allNodes = atoi(optarg);
            }
            else
            {
                opt->allNodes = 9999;
            }
            break;

        case 'n':
            // set the source node
            opt->src = atoi(optarg);
            break;

        case 'P':
            opt->printSolution = optarg;
            break;

        case 'L':
            opt->printSegList = optarg;
            break;

        case 'r':
            opt->resultsFile = optarg;
            break;

        case 't':
            // we are going to load a topology
            opt->loadingMode = LOAD_TOPO;
            break;

        case 's':
            // we are going to load directly a SR Graph
            opt->loadingMode = LOAD_SR;
            break;

        case 'f':
            // set the input file name
            opt->filename = optarg;
            break;
        case 'c':
            opt->srConv = optarg;
            break;

        case 'l':
            // the nodes in the file are identified by labels
            opt->labelsOrId = LOAD_LABELS;
            break;
        case 'S':
            opt->srBin = 1;
            break;
        case '1':
            // change the delay cstr value
            opt->cstr.m1 = atoi(optarg); // TODO: generalize this
            break;

        case '2':
            // change the IGP ctsr value
            opt->cstr.m1 = atoi(optarg); // TODO: generalize this
            break;

        case 'i':
            // the nodes in the file are identified by IDs
            opt->labelsOrId = LOAD_IDS;
            break;

        case 'o':
            // set the output file name
            opt->output = optarg;
            break;

        case 'a':
            // set the trueness
            opt->accuracy = atoi(optarg);
            break;

        case 'b':
            opt->biDir = BI_DIRECTIONNAL;
            break;

        case 'E':
            if (strcmp(optarg, "strict") == 0)
            {
                opt->encoding = STRICT_ENCODING;
            }
            else if (strcmp(optarg, "loose") == 0)
            {
                opt->encoding = LOOSE_ENCODING;
            }
            else
            {
                printf("Encoding must be loose or strict\n");
                exit(1);
            }
            break;
        case 'R':
            if (strcasecmp(optarg, "all") == 0)
            {
                opt->retrieve = SC_RETRIEVE_ALL;
            }
            else if (strcasecmp(optarg, "allbest") == 0)
            {
                opt->retrieve = SC_RETRIEVE_ALL_BEST;
            }
            else if (strcasecmp(optarg, "onebest") == 0)
            {
                opt->retrieve = SC_RETRIEVE_ONE_BEST;
            }
            else
            {
                printf("Retrieve option must be onebest, allbest or all\n");
                exit(1);
            }
            break;
        
        case 'm': 
            if (strcasecmp(optarg, "pareto") == 0)
            {
                opt->mode = PARETO;
            }
            else if (strcasecmp(optarg, "lex") == 0)
            {
                opt->mode = LEX;
            }
            else
            {
                printf("Mode must be either pareto or lex\n");
                exit(1);
            }
            break;

        case 'B':
            opt->saveSrGraphBin = optarg;
            break;
        


        case ':':

        


        default:
            printf("Unrecognized argument : %s", optarg);
            return -1;
            break;
        }
    }



    if ((!opt->allNodes && opt->src == -1) || opt->filename == NULL || opt->labelsOrId == -1 || opt->loadingMode == -1)
    {
        printf("Missing a mandatory argument\n");
        return -1;
    }

    return 0;
}

void usage(char *name)
{
    printf("Usage:\n");
    printf("\t%s [options] --file input_file --topo/--sr --labels/--id --all-nodes/--src [source node]\n", name);

    printf("\nMandatory parameters :\n"
           "You need to use --file, --topo or --sr, --labels or --id and --all-nodes or --src\n"
           "\t--file [filename]\t\tUse the given file as graph file\n"
           "\t--topo/--sr \t\t\tdefine which type of loading will be choosen\n"
           "\t--labels/--id \t\t\tdefine if src and dst in the file are represented by ids or labels\n"
           "\t--all-nodes/-src [source]\tdefine if the main function will be lauch on all nodes or only one\n");

    printf("\nOptions :\n"
           "\t--encoding [strict/loose]\t\t\tDefine the encoding of the paths. By default it is loose\n"
           "\t--retrieve [onebest/allbest/all]\t\tWhat path do you want to retrieve. By default it is onebest\n"
           "\t--accu [value]\t\t\tSet the accuracy of delay to value. By default this parameter is 1 (0.1ms)\n"
           "\t--output [output_file]\t\tThe results are printed in output_file. The file is created if it doesn't exists\n"
           "\t--bi-dir\t\t\tIf this option is activated, the link represented in input file are bi-directionals\n"
           "\t--cstr1 [value]\t\t\tPut the first constraint to value\n"
           "\t--cstr2 [value]\t\t\tPut the second constraint to value\n"
           "\t--full\t\t\t\tPut SEGMAX to infinity and count the iterations instead of the execution time\n"
           "\t--interface\t\t\tLauch the interactive mode\n"
           "\t--print-solution dest=ID,m1=VALUE\t\tPrint the solution for the given destination and the given value of m1\n");
}
