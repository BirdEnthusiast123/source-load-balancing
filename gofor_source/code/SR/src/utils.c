#include "../include/utils.h"
#include "../include/params.h"
#include "../include/use-case.h"

float Slength(int m0, DistVector_t dist, DistVector_t cstr, Mode_t mode)
{
    (void)cstr;
    (void)mode;
    (void)dist;
    float length = Distance_SLength(dist, cstr);
    
    length += m0*0.001; // acount for SR when ordering distances in the PQ
    
    return length;
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


bool is_dominated_SR(
    int m0v, DistVector_t distv, 
    int m0u, DistVector_t distu, Mode_t mode)
{
    if (mode == PARETO) {
        if ((m0u <= m0v) && is_dominated(distv, distu))
        {
            return true;
        }
    } 

    if (mode == LEX) {
        if (   (!Distance_eq(distv, distu) && is_dominated(distv, distu))
            || (Distance_eq(distv, distu) && (m0u < m0v))) 
        {
            return true;
        }
    }
    return false;

}




bool is_dominated_0_neq(DistVector_t distv, DistVector_t distu, Mode_t mode)
{
    return is_dominated_0(distv, distu, mode) && !Distance_eq(distv, distu);
}

bool is_dominated_0(DistVector_t distv, DistVector_t distu, Mode_t mode)
{
    return is_dominated_SR(0, distv, 0, distu, mode);
}

// First condition to be strongly dominated
bool is_strongly_dominated_i(
    int m0v, DistVector_t distv, int vSegType, int currDagv, 
    int m0u, DistVector_t distu, int uSegType, int currDagu, Mode_t mode)
{
    return (vSegType == uSegType) 
        && (currDagv != currDagu) // TODO: actually check dag inclusion
        && is_dominated_SR(m0v, distv, m0u, distu, mode);
}

// Second condition to be strongly dominated
bool is_strongly_dominated_ii(
    int m0v, DistVector_t distv, 
    int m0u, DistVector_t distu, Mode_t mode)
{
    return is_dominated_SR(m0v-1, distv, m0u, distu, mode);
}


bool is_strongly_dominated(
    int m0v, DistVector_t distv, int vSegType, int currDagv, 
    int m0u, DistVector_t distu, int uSegType, int currDagu, Mode_t mode)
{
    return is_strongly_dominated_ii(m0v, distv, m0u, distu, mode)
    || is_strongly_dominated_i(m0v, distv, vSegType, currDagv, m0u, distu, uSegType, currDagu, mode);
}
