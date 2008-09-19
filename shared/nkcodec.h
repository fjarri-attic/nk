// -----------------------------------------------
// ������������ ���� ��� ������������� nkcodec.lib
// -----------------------------------------------

#ifndef _NKCODEC_H
#define _NKCODEC_H

// �������� �����, ���������������� �������� ������� � ������� vector_num 
// src - ������ ������� length*k, res - ������ ������� length
VOID GFCreateSlice(OUT UCHAR *res, IN UCHAR *src, IN ULONG length, IN UCHAR k, IN UCHAR vector_num);

// ���������� ������� �������� k*k ��� ������� ������ 
// parts_numbers - ������ ������ ������, ������� ������� ����� �������������
// temp - ������� ������ ��� ������������� ����������
// �� ���������� res � temp ������ ���� �������� ������� ������ �������� k*k
VOID GFCreateAssemblingMatrix(OUT UCHAR *res, OUT UCHAR *temp, IN const UCHAR *parts_numbers, IN UCHAR k);

// ������� ��� �������������
// src - ������ ������� length*n, ��� �� ������� �������� ����� ������ length � �������� 1..n
// vector_nums - ������ ������� k � �������� ������, �� ������� ������������ �������������
// assemble_matrix - ������� �������������, ����������� � ������� GFCreateAssemblingMatrix,
// ������, � �������� ������ �� ���������� �� ������ ���� ������� ��� �� ������ parts_numbers
// !!! decode_length (������ ������������� �����) ������ ���� ������ k !!
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