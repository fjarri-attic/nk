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

	// «аписываем в res единичную матрицу
	for(i=0; i<n; i++)
		for(j=0; j<n; j++)
			res[i*n+j] = (i==j) ? 1 : 0; 

	// ¬ычисл€ем обратную матрицу методом √аусса
	// «аписываем в res единичную матрицу и совершаем одинаковые линейные
	// преобразовани€ над строками res и a, пока a не перейдет в единичную
	// тогда в res будет находитьс€ обратна€ матрица

	for(i=0; i<n; i++)
	// ƒл€ каждого столбца
	{
		// »щем строку не выше строки i с ненулевым i-тым элементом
		// “ака€ строка существует, т.к. матрица a невырождена
		// ≈е номер запишетс€ в j
		for(j=i; j<n; j++)
			if(a[j*n+i] != 0) break;

		if(j!=i)
		// ƒл€ удобства нужно переставить i-тую и j-тую строки
			for(k=0; k<n; k++)
			{
				GFSwap(a[i*n+k], a[j*n+k]);
				GFSwap(res[i*n+k], res[j*n+k]);
			}

		// Ќормируем i-тую строку так, чтобы ее i-тый элемент был равен 1
		t = a[i*n+i];
		for(k=0; k<n; k++)
		{
			a[i*n+k] = GFDiv(a[i*n+k], t);
			res[i*n+k] = GFDiv(res[i*n+k], t);
		}

		for(j=0; j<n; j++)
			if(j!=i)
			// ƒл€ каждой строки кроме i-той вычитаем из нее i-тую строку с таким 
			// множителем, чтобы обнулить i-тый элемент этой строки
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