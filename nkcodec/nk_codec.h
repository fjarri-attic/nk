// --------------------------------------------
// Функции кодирования и декодирования массивов
// --------------------------------------------

#include "defines.h"

// Создание куска, соответствующего базовому вектору с номером vector_num 
// src - массив размера length*k, res - массив размера length
VOID GFCreateSlice(OUT UCHAR *res, IN UCHAR *src, IN ULONG length, IN UCHAR k, IN UCHAR vector_num);

// Построение матрицы размером k*k для склейки файлов 
// parts_numbers - номера частей файлов, склейка которых будет производиться
// temp - область памяти для промежуточных вычислений
// По указателям res и temp должны быть выделены области памяти размером k*k
VOID GFCreateAssemblingMatrix(OUT UCHAR *res, OUT UCHAR *temp, IN const UCHAR *parts_numbers, IN UCHAR k);

// Функция для декодирования
// src - массив размера length*n, где по порядку записаны куски длиной length с номерами 1..n
// vector_nums - массив размера k с номерами кусков, по которым производится декодирование
// assemble_matrix - матрица декодирования, построенная с помощью GFCreateAssemblingMatrix,
// причем, в качестве одного из параметров ей должен быть передан тот же массив vector_nums
VOID GFAssembleSlices(OUT UCHAR *res, IN UCHAR *src, ULONG part_length, UCHAR k, 
					  IN UCHAR *parts_numbers, IN UCHAR *assemble_matrix);