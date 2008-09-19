#include "dispatch_scsi.h"

NTSTATUS DispatchScsi (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PSCSI_REQUEST_BLOCK pSrb;
	PCDB pCdb;
	PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	PREAD_REQUEST pRead;
	ULONG BlocksToRead;
	ULONG StartingSector, SectorsNum;

	pSrb = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Scsi.Srb;
	pCdb = (PCDB)pSrb->Cdb;

	if(pSrb->Function == SRB_FUNCTION_EXECUTE_SCSI
		&& pCdb->CDB10.OperationCode == SCSIOP_READ_TOC)
	{
		IoCopyCurrentIrpStackLocationToNext(pIrp); 
		IoSetCompletionRoutine(pIrp, ModifyToc, NULL, TRUE, TRUE, TRUE);
		return IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);
	}

	if(pSrb->Function == SRB_FUNCTION_EXECUTE_SCSI
		&& pCdb->CDB10.OperationCode == SCSIOP_READ)
	{
//		__asm int 3;

		IoMarkIrpPending(pIrp);

		StartingSector	=	(ULONG)pCdb->CDB10.LogicalBlockByte3 +
							((ULONG)(pCdb->CDB10.LogicalBlockByte2) << 8) +
							((ULONG)(pCdb->CDB10.LogicalBlockByte1) << 16) +
							((ULONG)(pCdb->CDB10.LogicalBlockByte0) << 24);

		SectorsNum		=	(ULONG)pCdb->CDB10.TransferBlocksLsb +
							((ULONG)(pCdb->CDB10.TransferBlocksMsb) << 8);

		BlocksToRead = GetNumberOfBlocksToRead(StartingSector, SectorsNum, 12);
			
		pRead = Allocate(sizeof(READ_REQUEST) + BlocksToRead * sizeof(ENCODED_BLOCK));
		RtlZeroMemory(pRead, sizeof(READ_REQUEST) + BlocksToRead * sizeof(ENCODED_BLOCK));

		pRead->ToRead = (PENCODED_BLOCK)(pRead + 1);
		pRead->BlocksNum = BlocksToRead;

		pRead->k = 12;
		pRead->n = 16;

		pRead->pSourceIrp = pIrp;
		pRead->pSourceSrb = pSrb;
		pRead->pSourceCdb = pCdb;

		pRead->SourceBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, HighPagePriority);

		pRead->pRemoveLock = &((PDEVICE_EXTENSION)(pDeviceObject->DeviceExtension))->RemoveLock;
		pRead->pNextDevice = ((PDEVICE_EXTENSION)(pDeviceObject->DeviceExtension))->pNextDeviceObject;
		pRead->StackSize = pRead->pNextDevice->StackSize;

		pRead->StartingSector = StartingSector;
		pRead->SectorsNum = SectorsNum;

		AllocateBuffer(pRead->k * (RAW_LEN + ECC_LEN + 2), &(pRead->ReadBuffer),
			&(pRead->ReadBufferMdl));

//		__asm int 3;
		BlocksToRead = GetNumberOfBlocksToRead(StartingSector, SectorsNum, 12);
		FillBlocksToRead(pRead->ToRead, StartingSector, SectorsNum, pRead->n, pRead->k);

		pRead->CurrentBlock = 0;
		pRead->CurrentLength = 0;

//		__asm int 3;

		ReadCurrentBase(pRead);

		return STATUS_PENDING;
	}
	else
		return PassIrp(pDeviceObject, pIrp);

}

//
VOID ReadCurrentBase(PREAD_REQUEST pRead)
{
	PIRP pNewIrp;

	pNewIrp = CreateIrp(pRead->StackSize, pRead->ToRead[pRead->CurrentBlock].BaseParts,
		pRead->ToRead[pRead->CurrentBlock].k, pRead);

	IoSetCompletionRoutine(pNewIrp, ReadCompletion, pRead, TRUE, TRUE, TRUE);
    IoCallDriver(pRead->pNextDevice, pNewIrp);
}

//
NTSTATUS ReadCompletion	(PDEVICE_OBJECT pDeviceObject, PIRP pNewIrp, PVOID context)
{
	PREAD_REQUEST pRead = (PREAD_REQUEST)context;
	UCHAR parts_num[255], i;
	PUCHAR AssemblingMatrix, TempMatrix;
	PUCHAR DecodeBuffer;
	
	PSCSI_REQUEST_BLOCK pSrb = IoGetCurrentIrpStackLocation(pNewIrp)->Parameters.Scsi.Srb;

	if(NT_SUCCESS(pNewIrp->IoStatus.Status))
	{
		AssemblingMatrix = Allocate(pRead->k * pRead->k);
		TempMatrix = Allocate(pRead->k * pRead->k);
		DecodeBuffer = Allocate(RAW_LEN * pRead->k);

		for(i=1; i<= pRead->k; i++) parts_num[i-1] = i;

		GFCreateAssemblingMatrix(AssemblingMatrix, TempMatrix, parts_num, pRead->k);
		GFAssembleSlices(DecodeBuffer, pRead->ReadBuffer, 
			RAW_LEN, pRead->k, parts_num, AssemblingMatrix);

		RtlCopyMemory(pRead->SourceBuffer + pRead->CurrentLength, 
			DecodeBuffer + pRead->ToRead[pRead->CurrentBlock].Offset,
			pRead->ToRead[pRead->CurrentBlock].Length);

		__asm int 3;

		Free(AssemblingMatrix);
		Free(TempMatrix);
		Free(DecodeBuffer);

		pRead->CurrentLength += pRead->ToRead[pRead->CurrentBlock].Length;
		pRead->CurrentBlock++;

        if(pRead->CurrentBlock < pRead->BlocksNum)		
			ReadCurrentBase(pRead);
		else
		{
			pRead->pSourceSrb->SrbStatus = 1;
            CompleteRequest(pRead->pSourceIrp, STATUS_SUCCESS, 
				pRead->SectorsNum * SECTOR_LEN, TRUE);            

			FreeBuffer(NULL, pRead->ReadBufferMdl);
			FreeIrp(pNewIrp);
			Free(pRead);
		}
	}
	else
	{
		CompleteRequest(pRead->pSourceIrp, STATUS_UNSUCCESSFUL, 0, TRUE);

		FreeBuffer(NULL, pRead->ReadBufferMdl);
		FreeIrp(pNewIrp);
		Free(pRead);
	}

	return STATUS_MORE_PROCESSING_REQUIRED;
}

//
PIRP CreateIrp(CCHAR StackSize, ULONG StartingSector, ULONG SectorsCount, PREAD_REQUEST pRead)
{
	PIRP pIrp;
	PSCSI_REQUEST_BLOCK pSrb;
	PCDB pCdb;
	PIO_STACK_LOCATION pStack;
	ULONG SECTOR = 2352;// + 296;

	pIrp = IoAllocateIrp(StackSize, FALSE);

//
	pIrp->RequestorMode = KernelMode;
	pIrp->MdlAddress = pRead->ReadBufferMdl;

//
	pStack = IoGetNextIrpStackLocation(pIrp);

	pStack->Parameters.Scsi.Srb = Allocate(sizeof(SCSI_REQUEST_BLOCK));
	RtlZeroMemory(pStack->Parameters.Scsi.Srb, sizeof(SCSI_REQUEST_BLOCK));

	pStack->MajorFunction = IRP_MJ_SCSI;
	pStack->Flags = SL_OVERRIDE_VERIFY_VOLUME;

//
	pSrb = pStack->Parameters.Scsi.Srb;

	pSrb->Length = sizeof(SCSI_REQUEST_BLOCK);
	pSrb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
	pSrb->CdbLength = 12;
	pSrb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
		SRB_FLAGS_ADAPTER_CACHE_ENABLE;
	pSrb->DataTransferLength = SECTOR * SectorsCount;
	pSrb->TimeOutValue = 1;

	pSrb->OriginalRequest = pIrp;
	pSrb->DataBuffer = pRead->ReadBuffer;
	
	pSrb->QueueSortKey = StartingSector;
	pSrb->InternalStatus = StartingSector;

//
	pCdb = (PCDB)pSrb->Cdb;
/*
	pCdb->READ_CD.OperationCode = 0xBE;

	pCdb->READ_CD.TransferBlocks[1] = HIBYTE(LOWORD(SectorsCount));
	pCdb->READ_CD.TransferBlocks[2] = LOBYTE(LOWORD(SectorsCount));

	pCdb->READ_CD.StartingLBA[0] = HIBYTE(HIWORD(StartingSector));
	pCdb->READ_CD.StartingLBA[1] = LOBYTE(HIWORD(StartingSector));
	pCdb->READ_CD.StartingLBA[2] = HIBYTE(LOWORD(StartingSector));
	pCdb->READ_CD.StartingLBA[3] = LOBYTE(LOWORD(StartingSector));

	pCdb->READ_CD.IncludeSyncData = 1;
	pCdb->READ_CD.HeaderCode = 3;
	pCdb->READ_CD.IncludeEDC = 1;
	pCdb->READ_CD.IncludeUserData = 1;
*/
	pCdb->AsByte[0] = 0xBE;

	pCdb->AsByte[2] = HIBYTE(HIWORD(StartingSector));
	pCdb->AsByte[3] = LOBYTE(HIWORD(StartingSector));
	pCdb->AsByte[4] = HIBYTE(LOWORD(StartingSector));
	pCdb->AsByte[5] = LOBYTE(LOWORD(StartingSector));

	pCdb->AsByte[6] = LOBYTE(HIWORD(SectorsCount));
	pCdb->AsByte[7] = HIBYTE(LOWORD(SectorsCount));
	pCdb->AsByte[8] = LOBYTE(LOWORD(SectorsCount));

	pCdb->AsByte[9] = 0xF8;

    return pIrp;
}

VOID FreeIrp(PIRP pIrp)
{
    PIO_STACK_LOCATION pStack = IoGetNextIrpStackLocation(pIrp);

//	Похоже, что эту память очищает система; надо позже разобраться
//	Free(pStack->Parameters.Scsi.Srb);
	
//	ReleaseIrp(pIrp);
	IoFreeIrp(pIrp);
}

//
NTSTATUS ModifyToc	(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	PSCSI_REQUEST_BLOCK pSrb;
	PCDB pCdb;
	PMDL pMdl;
	PUCHAR MdlBuffer;

	pSrb = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Scsi.Srb;
	pCdb = (PCDB)pSrb->Cdb;

	if(!NT_SUCCESS(pIrp->IoStatus.Status))
	{
		return STATUS_SUCCESS;
	}

	if(pIrp->MdlAddress)
	{
		pMdl = pIrp->MdlAddress;

		MmProbeAndLockPages(pMdl, KernelMode, IoModifyAccess);
		MdlBuffer = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);

		DbgPrint("!!! Writing fake TOC");
		MdlBuffer[5] = 0x14;		
	}

	return STATUS_SUCCESS;
}
