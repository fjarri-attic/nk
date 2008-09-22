#include "galua_field.h"
#include "matrices.h"

//
UCHAR GFMulVV(IN UCHAR *x, IN UCHAR *y, UCHAR n)
{
	UCHAR i, res = 0;
	for(i=0; i<n; i++)
		res ^= GFMul(x[i], y[i]);
	return res;
}

//
VOID GFMulMV(OUT UCHAR *res, IN UCHAR *a, IN UCHAR *x, IN UCHAR n)
{
	UCHAR i, j;
	for(i=0; i<n; i++)
	{
		res[i] = 0;
		for(j=0; j<n; j++)
			res[i] ^= GFMul(a[i*n+j], x[j]);
	}     
}

//
VOID GFMulVM(OUT UCHAR *res, IN UCHAR *x, IN UCHAR *a, IN UCHAR n)
{
	UCHAR i, j;
	for(i=0; i<n; i++)
	{
		res[i] = 0;
		for(j=0; j<n; j++)
			res[i] ^= GFMul(a[j*n+i], x[j]);
	}     
}

//
VOID GFMulMM(OUT UCHAR *res, IN UCHAR *a, IN UCHAR *b, IN UCHAR n)
{
	UCHAR i, j, k;
	for(i=0; i<n; i++)
		for(j=0; j<n; j++)
		{
			res[i*n+j] = 0;
			for(k=0; k<n; k++)
				res[i*n+j] ^= GFMul(a[i*n+k], b[k*n+j]);
		}
}

//
VOID GFInvertM(OUT UCHAR *res, IN OUT UCHAR *a, IN UCHAR n)
{
	UCHAR t;
	UCHAR i, j, k;

	// Fill res with identity matrix
	for(i=0; i<n; i++)
		for(j=0; j<n; j++)
			res[i*n+j] = (i==j) ? 1 : 0; 

	// Use Gauss-Jordan method for matrix inversion

	for(i=0; i<n; i++)
	// For each column
	{
		// Search for the row, whose number is not lower than i and with non-zero i-th element
		// Such row exists, because the matrix is non-singular
		// It's number will be stored as j
		for(j=i; j<n; j++)
			if(a[j*n+i] != 0) break;

		if(j!=i)
		// Swap i-th and j-th rows for convenience
			for(k=0; k<n; k++)
			{
				GFSwap(a[i*n+k], a[j*n+k]);
				GFSwap(res[i*n+k], res[j*n+k]);
			}

		// Normalize i-th row so that its i-th element be equal to 1
		t = a[i*n+i];
		for(k=0; k<n; k++)
		{
			a[i*n+k] = GFDiv(a[i*n+k], t);
			res[i*n+k] = GFDiv(res[i*n+k], t);
		}

		for(j=0; j<n; j++)
			if(j!=i)
			// For each row except for i-th one, subtract i-th row with multiplier
			// that turns i-th element of this row to zero
			{
				t = a[j*n+i];
				for(k=0; k<n; k++)
				{
					a[j*n+k] ^=  GFMul(a[i*n+k], t);
					res[j*n+k] ^= GFMul(res[i*n+k], t);
				}
			}
	}   
}

//
VOID GFCreateBasisVector(OUT UCHAR *res, IN UCHAR number, IN UCHAR n)
{
	UCHAR i;

	for(i=0; i<n; i++)
		if(i)
			res[i] = GFMul(res[i-1], number); 
		else
			res[i] = 1;
}