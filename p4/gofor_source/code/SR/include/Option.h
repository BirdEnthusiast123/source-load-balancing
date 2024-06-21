#ifndef __OPTION_H__
#define __OPTION_H__

#include "base.h"
#include "SrGraph.h"
#include "Dict.h"
#include "Samcra.h"



/**
 * @brief This structure represents all the availables Options
 */

struct Options {
    char loadingMode;       /**< define if you load a topology or directly a SR Graph */
    char labelsOrId;        /**< define if the nodes are identified by a label or an id */
    DistVector_t cstr;            /**< constraint on second component (IGP weight) */
    char* filename;         /**< file to load */
    char* output;           /**< output file */
    int accuracy;           /**< delay trueness (1 means that accuracy is about 0.1ms) */
    char biDir;             /**< define if the links are uni-directionals or bi */
    int src;                /**< source node */
    int allNodes;          /**< define if we need to mesure on all nodes */
    char* srConv;
    char srBin;
    char* resultsFile;
    EncodingType_t encoding;
    SamcraRetrieveOption_t retrieve;
    Mode_t mode;
    char *printSolution;
    char* saveSrGraphBin; 
    char* printDAG;
    char* printDAGWeights;
};



/**
 * @brief fill the corrects Options. Use ./bellmanFork --help
 * to see the availables Options
 *
 * @param argc          number of arguments on the command line
 * @param argv          arguments
 * @return              return 1 if all the Options are available, -1 if not
 **/

int Option_command_parser(int argc, char** argv, struct Options* opt);


/**
 * @brief display user manual
 * @param name          programme name (argv[0])
 **/

void usage(char* name);


#endif
