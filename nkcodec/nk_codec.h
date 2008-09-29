// -------------------------------
// Encoding and decoding functions
// -------------------------------

#include "defines.h"

// Create slice with basis number vector_num
// src has size length*k, res has size length
VOID GFCreateSlice(OUT UCHAR *res, IN UCHAR *src, IN ULONG length, IN UCHAR k, IN UCHAR vector_num);

// Create k*k matrix for assembling
// parts_numbers - number of slices to assemble
// temp - temporary buffer
// res è temp has size of k*k
VOID GFCreateAssemblingMatrix(OUT UCHAR *res, OUT UCHAR *temp, IN const UCHAR *parts_numbers, IN UCHAR k);

// Decoding function
// src has size length*n, where slices go successively
// vector_nums has size k with slice numbers
VOID GFAssembleSlices(OUT UCHAR *res, IN UCHAR *src, ULONG part_length, UCHAR k,
					  IN UCHAR *parts_numbers, IN UCHAR *assemble_matrix);
