#include "../include/utils.h"
#include "../include/params.h"

float Slength(DistVector_t dist, DistVector_t cstr)
{
    (void) cstr;
    return Distance_SLength(dist, cstr);
}

char* distTypeStr(DistType_t distType)
{
    switch (distType)
    {
    case RETRIEVABLE:
        return "RETRIEVABLE";
    case ONLY_EXTENDABLE:
        return "ONLY_EXTENDABLE";
    case IGNORED:
        return "IGNORED";
    }
    return "UNKNOWN";
}
