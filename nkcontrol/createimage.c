#include "createimage.h"

void CreateEmptyFile(const char *path, ULONG SectorsNum)
{
    ULONG i;
	FILE *file;
	UCHAR buf[RAW_LEN];

	memset(buf, 0, RAW_LEN);
    file = fopen(path, "wb");

	for(i=0; i<SectorsNum; i++)
		fwrite(buf, 1, RAW_LEN, file);

	fclose(file);
}

ULONG CreateImage(const char *source_path, const char *res_path, UCHAR n, UCHAR k)
{
	ULONG BytesInBlock = k * RAW_LEN;

	ULONG SourceFileBytesNum;
	ULONG SourceFileBlocksNum, Remainder;

	ULONG j;
	UCHAR i;

	PUCHAR ReadBuffer, WriteBuffer;

	FILE *source;
	FILE *res;
	FORMATTING formatting;

	CreateFormatting(&formatting, n, k);

	printf("Creating empty file...");
	CreateEmptyFile(res_path, formatting.ZonesNum * formatting.SectorsInZone);
	printf(" done.\n");

	source = fopen(source_path, "rb");
	res = fopen(res_path, "r+b");

	ReadBuffer = malloc(BytesInBlock);
	WriteBuffer = malloc(RAW_LEN);

    fseek(source, 0, SEEK_END);
	SourceFileBytesNum = ftell(source);
	fseek(source, 0, SEEK_SET);

	SourceFileBlocksNum = (SourceFileBytesNum / BytesInBlock) + 1;
	Remainder = SourceFileBytesNum % BytesInBlock;

	for(j=0; j<SourceFileBlocksNum; j++)
	{
		if(j % 100 == 0)
			printf("Block %d/%d\n", j, SourceFileBlocksNum);

		if(j == SourceFileBlocksNum - 1)
		{
			fread(ReadBuffer, 1, BytesInBlock - Remainder, source);
			memset(ReadBuffer + BytesInBlock - Remainder, 0, Remainder);
		}
		else
			fread(ReadBuffer, 1, BytesInBlock, source);

		fseek(res, GetBaseOffset(&formatting, j)*RAW_LEN, SEEK_SET);
        for(i=1; i<=k; i++)
		{
			GFCreateSlice(WriteBuffer, ReadBuffer, RAW_LEN, k, i);
			fwrite(WriteBuffer, 1, RAW_LEN, res);
		}

		for(i=0; i<n-k; i++)
		{
			GFCreateSlice(WriteBuffer, ReadBuffer, RAW_LEN, k, k + i + 1);
			fseek(res, GetBackupOffset(&formatting, j, i)*RAW_LEN, SEEK_SET);
			fwrite(WriteBuffer, 1, RAW_LEN, res);
		}
	}

	fclose(source);
	fclose(res);
	free(ReadBuffer);
	free(WriteBuffer);

	return 0;
}

ULONG TestImage(const char *source_path, UCHAR n, UCHAR k)
{
	ULONG BytesInBlock = k * RAW_LEN;

	PUCHAR ReadBuffer, WriteBuffer;

	FILE *source;
	FILE *res;

	PUCHAR AsmM, TmpM;
	UCHAR nums[12] = {1,2,3,4,5,6,7,8,9,10,11,12};

	source = fopen(source_path, "rb");
	res = fopen("test.iso", "wb");

	ReadBuffer = malloc(BytesInBlock);
	WriteBuffer = malloc(BytesInBlock);
	AsmM = malloc(12*12);
	TmpM = malloc(12*12);

	fread(ReadBuffer, 1, BytesInBlock, source);
//	GFCreateAssemblingMatrix(AsmM, TmpM, nums, 12);
//	GFAssembleSlices(WriteBuffer, ReadBuffer, RAW_LEN, 12, nums, AsmM);
//	fwrite(WriteBuffer, 1, RAW_LEN*12, res);

	fread(ReadBuffer, 1, BytesInBlock, source);
	GFCreateAssemblingMatrix(AsmM, TmpM, nums, 12);
	printf("%d %d %d\n", AsmM[0], AsmM[11], AsmM[143]);
	GFAssembleSlices(WriteBuffer, ReadBuffer, RAW_LEN, 12, nums, AsmM);
	fwrite(WriteBuffer, 1, RAW_LEN*12, res);

	fclose(source);
	fclose(res);

	return 0;
}
