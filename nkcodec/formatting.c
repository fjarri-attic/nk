// ---------------------
// Disk format functions
// ---------------------

#include "formatting.h"

//
VOID CreateFormatting(OUT PFORMATTING formatting, UCHAR n, UCHAR k)
{
	formatting->n = n;
	formatting->k = k;

	formatting->ZonesNum = n - k + 1;
	formatting->SectorsInZone = SECTORS_ON_CD / formatting->ZonesNum;
	formatting->SectorsInBaseArea = formatting->SectorsInZone / n * k;
	formatting->BlocksInBaseArea = formatting->SectorsInZone / n;
}

//
ULONG GetBaseOffset(PFORMATTING params, ULONG BlockNumber)
{
	ULONG ZoneNumber, Offset;

	ZoneNumber = BlockNumber / params->BlocksInBaseArea;
	Offset = (BlockNumber % params->BlocksInBaseArea) * params->k;

	return ZoneNumber * params->SectorsInZone + Offset;
}

//
ULONG GetBackupOffset(PFORMATTING params, ULONG BlockNumber, UCHAR BackupNumber)
{
	ULONG BaseZoneNumber, Theta;

	BaseZoneNumber = BlockNumber / params->BlocksInBaseArea;
	Theta = BackupNumber < BaseZoneNumber ? 0 : 1;

	return	(BackupNumber + Theta) * params->SectorsInZone +
			params->SectorsInBaseArea +
			BlockNumber -
			(1 - Theta) * params->BlocksInBaseArea;
}

//
ULONG GetNumberOfBlocksToRead(ULONG StartingSector, ULONG SectorsNum, UCHAR k)
{
	ULONG ByteOffset;
	ULONG ByteLength;
	ULONG BlockLength;
	ULONG Remainder;

	ByteOffset = StartingSector * SECTOR_LEN;
	ByteLength = SectorsNum * SECTOR_LEN;

	BlockLength = RAW_LEN * k;

	Remainder = BlockLength - ByteOffset % BlockLength;

	if(ByteLength <= Remainder)
		return 1;
	else
		return (ByteLength - Remainder) / BlockLength + (Remainder ? 1 : 0) + 1;
}

//
VOID FillBlocksToRead(PENCODED_BLOCK pToRead, ULONG StartingSector,
					  ULONG SectorsNum, UCHAR n, UCHAR k)
{
	FORMATTING formatting;
	PENCODED_BLOCK pBlock = pToRead;
	ULONG CurrentBlock;
	ULONG ByteOffset;
	ULONG ByteLength = SectorsNum * SECTOR_LEN;
	UCHAR i;

	CreateFormatting(&formatting, n, k);

	ByteOffset = StartingSector * SECTOR_LEN;
	CurrentBlock = ByteOffset / (RAW_LEN * k);

	while(ByteLength)
	{
		pBlock->BaseParts = GetBaseOffset(&formatting, CurrentBlock);

		for(i = 0; i < n-k; i++)
			pBlock->BackupParts[i] = GetBackupOffset(&formatting, CurrentBlock, i);

		pBlock->k = k;
		pBlock->n = n;
		pBlock->Offset = ByteOffset - CurrentBlock * (RAW_LEN * k);
		pBlock->Length = RAW_LEN * k - pBlock->Offset;

		if(ByteLength >= pBlock->Length)
			ByteLength -= pBlock->Length;
		else
		{
            pBlock->Length = ByteLength;
			ByteLength = 0;
		}

		CurrentBlock++;
		ByteOffset = CurrentBlock * RAW_LEN * k;
		pBlock++;
	}
}
