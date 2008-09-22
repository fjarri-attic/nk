// ---------------------
// Disk format functions
// ---------------------

#include "defines.h"

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