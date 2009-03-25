#include "dispatch_scsi.h"

NTSTATUS DispatchScsi (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PSCSI_REQUEST_BLOCK pSrb;
	PCDB pCdb;
	PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	pSrb = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Scsi.Srb;
	pCdb = (PCDB)pSrb->Cdb;

	// System tries to identify disk
	// Using this occasion to get N and K along with read offset
	if(pSrb->Function == SRB_FUNCTION_EXECUTE_SCSI
		&& pCdb->CDB10.OperationCode == SCSIOP_READ_TOC)
	{
		// Set our completion routine which will determine whether or not it
		// is the (n,k)-formatted disk and will modify TOC correspondingly

		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(pIrp, DetectionCompletion, NULL, TRUE, TRUE, TRUE);
		return IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);
	}

	if(pSrb->Function == SRB_FUNCTION_EXECUTE_SCSI
		&& pCdb->CDB10.OperationCode == SCSIOP_READ)
	{
		IoMarkIrpPending(pIrp);
		StartReading(pDeviceObject, pIrp);

		return STATUS_PENDING;
	}
	else
		return PassIrp(pDeviceObject, pIrp);

}

//
VOID StartReading(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	PREAD_REQUEST pRead;
	ULONG BlocksToRead;
	ULONG StartingSector, SectorsNum;
	PSCSI_REQUEST_BLOCK pSrb;
	PCDB pCdb;
	PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	pSrb = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Scsi.Srb;
	pCdb = (PCDB)pSrb->Cdb;

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

	FillBlocksToRead(pRead->ToRead, StartingSector, SectorsNum, pRead->n, pRead->k);

	pRead->CurrentBlock = 0;
	pRead->CurrentLength = 0;

	ReadCurrentBase(pRead);
}

//
VOID ReadCurrentBase(PREAD_REQUEST pRead)
{
	PIRP pNewIrp;

	pNewIrp = CreateIrp(pRead->StackSize, pRead->ToRead[pRead->CurrentBlock].BaseParts,
		pRead->ToRead[pRead->CurrentBlock].k, pRead->ReadBufferMdl, pRead->ReadBuffer,
		TRUE, FALSE);

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

	__asm int 3;

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
PIRP CreateIrp(CCHAR StackSize, ULONG StartingSector, ULONG SectorsCount,
	PMDL read_buffer_mdl, PCHAR read_buffer, BOOLEAN get_data, BOOLEAN get_subchannels)
{
	PIRP pIrp;
	PSCSI_REQUEST_BLOCK pSrb;
	PCDB pCdb;
	PIO_STACK_LOCATION pStack;
	ULONG SECTOR = (get_data ? (RAW_LEN + ECC_LEN + 2) : 0) +
		(get_subchannels ? SUBCHANNELS_LEN : 0);

	pIrp = IoAllocateIrp(StackSize, FALSE);

	// Fill IRP parameters

	pIrp->RequestorMode = KernelMode;
	pIrp->MdlAddress = read_buffer_mdl;

	// Fill stack data

	pStack = IoGetNextIrpStackLocation(pIrp);

	pStack->Parameters.Scsi.Srb = Allocate(sizeof(SCSI_REQUEST_BLOCK));
	RtlZeroMemory(pStack->Parameters.Scsi.Srb, sizeof(SCSI_REQUEST_BLOCK));

	pStack->MajorFunction = IRP_MJ_SCSI;
	pStack->Flags = SL_OVERRIDE_VERIFY_VOLUME;

	// Fill SRB

	pSrb = pStack->Parameters.Scsi.Srb;

	pSrb->Length = sizeof(SCSI_REQUEST_BLOCK);
	pSrb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
	pSrb->CdbLength = 12;
	pSrb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
		SRB_FLAGS_ADAPTER_CACHE_ENABLE;
	pSrb->DataTransferLength = SECTOR * SectorsCount;
	pSrb->TimeOutValue = 1;

	pSrb->OriginalRequest = pIrp;
	pSrb->DataBuffer = read_buffer;

	pSrb->QueueSortKey = StartingSector;
	pSrb->InternalStatus = StartingSector;

	// Fill CDB

	pCdb = (PCDB)pSrb->Cdb;

	// SCSI command
	pCdb->READ_CD.OperationCode = SCSIOP_READ_CD;

	// Number of sectors to read
	pCdb->READ_CD.TransferBlocks[1] = HIBYTE(LOWORD(SectorsCount));
	pCdb->READ_CD.TransferBlocks[2] = LOBYTE(LOWORD(SectorsCount));

	// Starting sector number
	pCdb->READ_CD.StartingLBA[0] = HIBYTE(HIWORD(StartingSector));
	pCdb->READ_CD.StartingLBA[1] = LOBYTE(HIWORD(StartingSector));
	pCdb->READ_CD.StartingLBA[2] = HIBYTE(LOWORD(StartingSector));
	pCdb->READ_CD.StartingLBA[3] = LOBYTE(LOWORD(StartingSector));

	// On Audio CDs it is the only option,
	// since the whole 2352 bytes are the user data
	pCdb->READ_CD.IncludeUserData = (get_data ? 1 : 0);

	// '2' means block error byte + 1 pad byte + 294 bytes of C2 error codes
	// 'block error byte' is all C2 codes 'and'ed
	pCdb->READ_CD.ErrorFlags = (get_data ? 2 : 0);

	// '4' means raw P-W subchannel data (96 bytes)
	// Note: R-W channels are interleaved
	pCdb->READ_CD.SubChannelSelection = (get_subchannels ? 4 : 0);

    return pIrp;
}

VOID FreeIrp(PIRP pIrp)
{
    PIO_STACK_LOCATION pStack = IoGetNextIrpStackLocation(pIrp);

//	TODO: Seems that this memory is cleaned by OS; needs investigation
//	Free(pStack->Parameters.Scsi.Srb);

//	ReleaseIrp(pIrp);
	IoFreeIrp(pIrp);
}

//
NTSTATUS DetectionCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	PSCSI_REQUEST_BLOCK pSrb;
	PCDB pCdb;
	PMDL pMdl;
	PUCHAR MdlBuffer;
	PDEVICE_EXTENSION pDeviceExtension;

	pSrb = IoGetCurrentIrpStackLocation(pIrp)->Parameters.Scsi.Srb;
	pCdb = (PCDB)pSrb->Cdb;

	// Return immediately on errors
	if(!pIrp->MdlAddress)
	{
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
		return STATUS_SUCCESS;
	}

	if(!NT_SUCCESS(pIrp->IoStatus.Status))
	{
		return STATUS_SUCCESS;
	}
	
	// TODO: check if this is a real audio CD
	// Now assume that all disks are (n,k)-formatted
	pMdl = pIrp->MdlAddress;

	MmProbeAndLockPages(pMdl, KernelMode, IoModifyAccess);
	MdlBuffer = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);

	DbgPrint("Writing fake TOC\n");
	MdlBuffer[5] = 0x14;

	// Clean N, K and ReadOffset
	// TODO: investigate the situation what will occur, if they will be in use
	// (and whether it can happen or not in reality)
	pDeviceExtension = ((PDEVICE_EXTENSION)(pDeviceObject->DeviceExtension));
	pDeviceExtension->K = 0;
	pDeviceExtension->N = 0;
	pDeviceExtension->ReadOffset = 0;
	pDeviceExtension->pDriverObject->MajorFunction[IRP_MJ_SCSI] = DispatchScsi;

	return STATUS_SUCCESS;
}
