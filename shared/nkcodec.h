// -----------------------------------------------
// Заголовочный файл для использования nkcodec.lib
// -----------------------------------------------

#ifndef _NKCODEC_H
#define _NKCODEC_H

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
// причем, в качестве одного из параметров ей должен быть передан тот же массив parts_numbers
// !!! decode_length (размер декодируемого блока) должен быть кратен k !!
VOID GFAssembleSlices(	OUT UCHAR *res, IN UCHAR *src, ULONG part_length, UCHAR k, 
						IN UCHAR *parts_numbers, IN UCHAR *assemble_matrix );

#define SECTOR_LEN 2048
#define RAW_LEN 2352
#define ECC_LEN (RAW_LEN/8)
#define SECTORS_ON_CD 350000

typedef struct _FORMATTING
{
	UCHAR n;
	UCHAR k;

	ULONG ZonesNum;
	ULONG SectorsInZone;
	ULONG BlocksInBaseArea;
	ULONG SectorsInBaseArea;

} FORMATTING, *PFORMATTING;

typedef struct _ENCODED_BLOCK
{
	ULONG BaseParts;
	ULONG BackupParts[254];
	ULONG Offset;
	ULONG Length;

	UCHAR n;
	UCHAR k;
} ENCODED_BLOCK, *PENCODED_BLOCK;

VOID CreateFormatting(OUT PFORMATTING formatting, UCHAR n, UCHAR k);
ULONG GetBaseOffset(PFORMATTING params, ULONG BlockNumber);
ULONG GetBackupOffset(PFORMATTING params, ULONG BlockNumber, UCHAR BackupNumber);
ULONG GetNumberOfBlocksToRead(ULONG StartingSector, ULONG SectorsNum, UCHAR k);
VOID FillBlocksToRead(PENCODED_BLOCK pToRead, ULONG StartingSector, 
					  ULONG SectorsNum, UCHAR n, UCHAR k);

#endif