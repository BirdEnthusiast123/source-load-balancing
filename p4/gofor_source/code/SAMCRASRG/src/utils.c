#include "../include/utils.h"
#include "../include/params.h"

float Slength(int m0, my_m1 m1, my_m2 m2, my_m1 c1, my_m2 c2)
{
    return MAX((float)m0 / (float)SEG_MAX, MAX((float)m1 / (float)c1, (float)m2 / (float)c2));
}



bool is_dominated_u_v(int m0u, my_m1 m1u, my_m2 m2u, int m0v, my_m1 m1v, my_m2 m2v)
{
    if ((m0u <= m0v) && (m1u <= m1v) && (m2u <= m2v)) { 
        return true;
    }




    return false;
}
