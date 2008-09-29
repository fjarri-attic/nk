// ----------------------------------------------------
// Galois field operations (for GF(2^8))
// ----------------------------------------------------
// Additions and subtraction is performed by ^ (XOR)
// Zero element is 0, identity is 1
// Before using GFMul()/GFDiv() execute GFFillTables()

#include "defines.h"

// Swap two ariables
#define GFSwap(x, y) ((x)^=(y), (y)^=(x), (x)^=(y))

// Multiplication and division
// GFDiv result when y=0 is unspecified
UCHAR GFMul(IN UCHAR x, IN UCHAR y);
UCHAR GFDiv(IN UCHAR x, IN UCHAR y);