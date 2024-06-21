#ifndef __BASE_H__
#define __BASE_H__

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <sys/time.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "params.h"


typedef struct Segment_t
{
    int src;
    char type;
    // If the segment is an adjacency segment, then the exact edge is part of the type, so we need to store its index
    unsigned char adjIndex;
} Segment_t;


#define DEFAULT "\x1B[0m"
#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE    "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN    "\x1B[36m"


#define ANALYSE_DCLC            1
#define ANALYSE_2COP            2



#define ADJACENCY_SEGMENT       1
#define NODE_SEGMENT            2
#define OSEF_SEGMENT            3

typedef enum EncodingType_t {
    LOOSE_ENCODING,
    STRICT_ENCODING,
} EncodingType_t;

#define MIN(a,b) ((a < b) ? a : b)
#define MAX(a,b) ((a > b) ? a : b)

#define INF     INT_MAX


#define RESULTS(...)\
    fprintf(stderr,"[" BLUE "RESULTS" DEFAULT "] : " __VA_ARGS__); \

#define INFO(...)\
    fprintf(stderr,"[" GREEN "INFO" DEFAULT "] : " __VA_ARGS__); \

#define WARNING(...)\
    fprintf(stderr,YELLOW "\nWARNING : " DEFAULT __VA_ARGS__); \

#define DEBUG(file, ...)\
    fprintf(file,"[DEBUG] " __VA_ARGS__); \

#define ERROR(...)\
    fprintf(stderr, "[" RED "ERROR" DEFAULT "] : " MAGENTA __VA_ARGS__ ); \
    fprintf(stderr, DEFAULT); \

#define ASSERT(value, code,...)\
    if(!(value)) {\
        fprintf(stderr,RED "ASSERTION FAILED - file : \"%s\", line : %d, function \"%s\". " MAGENTA  __VA_ARGS__ "\n",__FILE__,__LINE__,__func__); \
        return code;\
    }

#define ASSERT_VOID(value,...)\
    if(!(value)) {\
        fprintf(stderr,RED "ASSERTION FAILED - file : \"%s\", line : %d, function \"%s\". " MAGENTA  __VA_ARGS__ "\n",__FILE__,__LINE__,__func__); \
        return;\
    }

#define ASSERT_EXIT(value,...)\
    if(!(value)) {\
        fprintf(stderr,RED "ASSERTION FAILED - file : \"%s\", line : %d, function \"%s\". " MAGENTA  __VA_ARGS__ "\n",__FILE__,__LINE__,__func__); \
        exit(42);\
    }

#define MY_RAND(max, min) \
    rand()%(max - min) + min;

#define TEST(value,...)\
    if(!(value)) { \
        fprintf(stderr,RED "TEST FAILED - file : \"%s\", line : %d, function \"%s\". " MAGENTA  __VA_ARGS__ "\n" ,__FILE__,__LINE__,__func__); \
    }\
    else{\
        fprintf(stdout,GREEN "TEST PASS - file : \"%s\", line : %d, function \"%s\". " NON_DOMINATED  __VA_ARGS__ "\n",__FILE__,__LINE__,__func__); \
    }

# define RAND(min, max) \
    ((rand()%(int)(((max))-(min)))+ (min))


#endif