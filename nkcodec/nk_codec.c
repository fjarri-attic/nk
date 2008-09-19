// --------------------------------------------
// Функции кодирования и декодирования массивов
// --------------------------------------------

#include "galua_field.h"
#include "matrices.h"
#include "nk_codec.h"

//
VOID GFCreateSlice(OUT UCHAR *res, IN UCHAR *src, IN ULONG length, IN UCHAR k, IN UCHAR vector_num)
{
	ULONG i;
	UCHAR vec[256];

	GFCreateBasisVector(vec, vector_num, k);
	for(i=0; i<length; i++)
		res[i] = GFMulVV(src + i*k, vec, k);
}

//
VOID GFCreateAssemblingMatrix(OUT UCHAR *res, OUT UCHAR *temp, IN const UCHAR *parts_numbers, IN UCHAR k)
{
	UCHAR i, j;

	for(i=0; i<k; i++)
		for(j=0; j<k; j++)
			if(j)
				temp[j*k+i] = GFMul(temp[(j-1)*k+i], parts_numbers[i]); 
			else
				temp[i] = 1;

	GFInvertM(res, temp, k);  
}

//
VOID GFAssembleSlices(OUT UCHAR *res, IN UCHAR *src, ULONG part_length, UCHAR k, 
					  IN UCHAR *parts_numbers, IN UCHAR *assemble_matrix)
{
	ULONG i, j;
	UCHAR vec[256];

	for(i=0; i<part_length; i++)
	{
		for(j=0; j<k; j++)
			vec[j] = src[(parts_numbers[j]-1)*part_length + i];

		GFMulVM(res + i*k, vec, assemble_matrix, k);
	}
}

