// -----------------------------
// Matrix operations for GF(2^8)
// -----------------------------
// Memory is allocated by user

#include "defines.h"

// Dot product
UCHAR GFMulVV(IN UCHAR *x, IN UCHAR *y, UCHAR n);

// Matrix-vector multiplication
// a - pointer to n*n matrix
// res - pointer to n buffer
VOID GFMulMV(OUT UCHAR *res, IN UCHAR *a, IN UCHAR *x, IN UCHAR n);

// Vector-matrix multiplication
// a - pointer to n*n matrix
// res - pointer to n*n buffer
VOID GFMulVM(OUT UCHAR *res, IN UCHAR *x, IN UCHAR *a, IN UCHAR n);

// Matrix-matrix multiplication
// a and b - pointers to n*n matrix
// res - pointer to n*n buffer
VOID GFMulMM(OUT UCHAR *res, IN UCHAR *a, IN UCHAR *b, IN UCHAR n);

// Get inversed matrix for a
// a - pointer to n*n matrix
// res - pointer to n*n buffer
// ! a is assumed to be non-singular
// ! functions spoils contents of a
VOID GFInvertM(OUT UCHAR *res, IN OUT UCHAR *a, IN UCHAR n);

// Create basis vector for encoding
// number - vector number
// res - pointer to n buffer
VOID GFCreateBasisVector(OUT UCHAR *res, IN UCHAR number, IN UCHAR n);