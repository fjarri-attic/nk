// ------------------
// nkcodec.lib header
// ------------------

#ifndef _NKCODEC_H
#define _NKCODEC_H

// Create slice, corresponding to basis vector with given number
// src has size (length*k), res has size (length)
VOID GFCreateSlice(OUT UCHAR *res, IN UCHAR *src, IN ULONG length, IN UCHAR k, IN UCHAR vector_num);

// Create assembling matrix with size k*k
// parts_numbers - array of slice numbers
// temp, res - k*k buffers
VOID GFCreateAssemblingMatrix(OUT UCHAR *res, OUT UCHAR *temp, IN const UCHAR *parts_numbers, IN UCHAR k);

// Decoding function
// src has size (length*n), and contains n consecutive slices
// parts_numbers - has size (k) and contains numbers of slices
// assemble_matrix - created with the same parts_numbers
// !!! part_length must be multiple of k !!
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
