#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>
#include <string.h>
#include "malloc.h"
#define dp 14
#define shifter (1 << dp)
/*
what are we trying to do here you might be asking

we want to represent doubles as integers,
but how to do such task i will give you an example 
take the following

        3.50
        2.25 x
    --------------
        0.00
      112.50 +
      675.00 +
    --------------
      787.50
as you can see we did normal multiplication and we made the decimal point fixed in the result
meaning we shifted the decimal point two places to the right and multiplied 350 x 250
so what you are asking, we can them shift the decimal point two places to the left and get the desired answer
this gives us the answer as 7.8750 and this is what we will be doing

we will represent our numbers as n * f where f is just the shifting in the decimal point we want to make
so f will be  1 lsh / << q , where q is the number of bits we represent our fraction in
lets illustrate more
we will represent any number as n * f
and when me add we nnormally make x + y same for subtracting
but for multiplication and division we need to do some stuff
we say that we will do the following
(n * f) * (m * f) and since f = q if numbers are large enough we might overflow
so how can we fix that we perform our operation in larger int representation to prevent overflow then we set back the decimal point to the specified position
for example (int64_t)(int32_t) * (int64_t)(int32_t) >> / rsh q ; note that we do right shift as due to the representation we shifted the decimal point 2q

in division  we can do the following (int64_t) / (int64_t) * f but this isnt enough
as the division will be rounded down as it is integer division but since division and multiplication are associative we can say
(int64_t) / (int64_t) * f == ((int64_t) * f) / (int64_t) 

this is all we need to know
*/



typedef int real;


static inline real convert_fixed_point(int n){

    
    real real_num = (n *shifter);
    return real_num;
}
static inline int convert_round_zero(int x){
    // we shift back the value to normall
    return (x / (shifter));
}
static inline int convert_round_nearest(int x){
    if(x >= 0){
        // adding a value to make a bias for rounding to the nearest
        return ((x + (shifter/2)) / (shifter));
    } 
    
    return ((x - (shifter/2)) / (shifter));
}

static inline real add(int  x, int y){

    real real_num = (x + y);
    return real_num;
}
static inline real  sub(int  x,int y){

    real real_num = x - y;
    return real_num;
}
static inline real  add_to_int(int x,int n){
    real real_num = (x + (n * shifter));
    return real_num;
}
static inline   real sub_to_int(int  x,int n){
    real real_num = (x - (n * shifter));
    return real_num;
}
static inline real mult_by_fixed(int  x, int y){

    real real_num = 
        (int32_t)((int64_t)(x) * (int64_t)(y) / (shifter));
    
    return real_num;
}
static inline real mult_by_int(int  x, int n){
    real real_num = x * n;
    return real_num;
}
 
static inline real divide_by_fixed(int  x,int y){
    ASSERT( y != 0);
    real real_num = (int32_t)(((int64_t)(x) * (shifter)) / (int64_t)(y));

    return real_num;
}
static inline real divide_by_int(int  x,int n){
    real real_num = (x / n);
    return real_num;
}

#endif