//
// IRP_MJ_READ handling
//

#include "dispatch_read.h"

#define HIWORD(l) ((USHORT)((ULONG_PTR)(l) >> 16)) 
#define LOWORD(l) ((USHORT)((ULONG_PTR)(l) & 0xffff)) 
#define HIBYTE(w) ((UCHAR)((ULONG_PTR)(w) >> 8)) 
#define LOBYTE(w) ((UCHAR)((ULONG_PTR)(w) & 0xff)) 

#define SECTOR_LENGTH 2048
#define RAW_SECTOR_LENGTH 2352
#define ECC_LENGTH 294

typedef struct _DATA
{
	PIRP pIrp;
	ULONG Length;
	PSCSI_PASS_THROUGH pSpt;
	PIO_REMOVE_LOCK pRemoveLock;

} DATA, *PDATA;

//
NTSTATUS DispatchRead (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS stat;
	PDEVICE_EXTENSION pDeviceExtension;

	pDeviceExtension = pDeviceObject->DeviceExtension;

	if(KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		DbgPrint("Async request\n");
		return PassIrp(pDeviceObject, pIrp);
	}

	stat = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);
	if(stat != STATUS_SUCCESS)
		return CompleteRequest(pIrp, stat, 0, FALSE);

//
	{
		PIRP pNewIrp;
		PSCSI_REQUEST_BLOCK pSrb;
		PCHAR ReadBuffer;
		KEVENT event;
		IO_STATUS_BLOCK status_block;
		NTSTATUS status;
		PIO_STACK_LOCATION stack;
		PDATA pData;
		PDEVICE_OBJECT pTarget;

		stack = IoGetCurrentIrpStackLocation(pIrp);

		DbgPrint("request Offset = %X, Length = %X\n", stack->Parameters.Read.ByteOffset.LowPart,
			stack->Parameters.Read.Length);

		pTarget = stack->DeviceObject;

		pSrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK), POOL_TAG);
		ReadBuffer = ExAllocatePoolWithTag(NonPagedPool, stack->Parameters.Read.Length, POOL_TAG);

//		CreateSptiReadRequest(pspt, (ULONG)(stack->Parameters.Read.ByteOffset.QuadPart/SECTOR_LENGTH),
//			stack->Parameters.Read.Length/SECTOR_LENGTH);

		if(KeGetCurrentIrql() == PASSIVE_LEVEL)
		{
//			__asm int 3;

			KeInitializeEvent(&event, SynchronizationEvent, FALSE);

			pNewIrp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_PASS_THROUGH, 
				pDeviceExtension->pNextDeviceObject, pspt, BufferSize, pspt, BufferSize, 
				TRUE, &event, &status_block);

			if(!pNewIrp)
			{
				DbgPrint("Can't create IRP\n");
				IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);
				return CompleteRequest(pIrp, STATUS_UNSUCCESSFUL, 0, FALSE);
			}

			CreateSrb(pSrb, (ULONG)(stack->Parameters.Read.ByteOffset.QuadPart/SECTOR_LENGTH),
				stack->Parameters.Read.Length/SECTOR_LENGTH, ReadBuffer, pNewIrp);

			DbgPrint("Before calling\n");

            
//			stack =	IoGetCurrentIrpStackLocation(pNewIrp);
//			DbgPrint("Stack flags = %X\n", stack->Flags);

			status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pNewIrp);

			DbgPrint("After calling\n");

			if(status == STATUS_PENDING)
			{
				KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, 0);
				DbgPrint("Was pending\n");
			}

			DbgPrint("Passive request: status %X, info %X\n", 
				status_block.Status, status_block.Information);
			DbgPrint("IRP status %X, info %X\n", pNewIrp->IoStatus.Status, 
				pNewIrp->IoStatus.Information);

//			SaveBufferToIrp(pIrp, (PCHAR)(pspt) + sizeof(SCSI_PASS_THROUGH), 
//				BufferSize - sizeof(SCSI_PASS_THROUGH));

			ExFreePoolWithTag(pspt, POOL_TAG);

//			status = CompleteRequest(pIrp, status_block.Status, status_block.Information - 
//				sizeof(SCSI_PASS_THROUGH), TRUE);

			IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

			status = PassIrp(pDeviceObject, pIrp);

			DbgPrint("Dispatch end\n");

			return status;
		}
		else
		{
//			__asm int 3;

			IoMarkIrpPending(pIrp);

			DbgPrint("Asynchronous request\n");

			pNewIrp = IoAllocateIrp(5, FALSE);

			if(!pNewIrp)
			{
				DbgPrint("Can't create IRP\n");
				IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);
				return CompleteRequest(pIrp, STATUS_UNSUCCESSFUL, 0, FALSE);
			}

			pData = ExAllocatePoolWithTag(NonPagedPool, sizeof(DATA), POOL_TAG);

			pData->Length = stack->Parameters.Read.Length;
			pData->pIrp = pIrp;
			pData->pRemoveLock = &(pDeviceExtension->RemoveLock);
			pData->pSpt = pspt;
            
			pNewIrp->MdlAddress = NULL;
			pNewIrp->AssociatedIrp.SystemBuffer = pspt;
			
			pNewIrp->UserBuffer = pspt;

			pNewIrp->RequestorMode = KernelMode;

			stack =	IoGetCurrentIrpStackLocation(pNewIrp);

			stack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
			stack->MinorFunction = 0;
//			stack->DeviceObject = pTarget;
			
			stack->Parameters.DeviceIoControl.InputBufferLength = BufferSize;
			stack->Parameters.DeviceIoControl.OutputBufferLength = BufferSize;
			stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SCSI_PASS_THROUGH;

			IoSetCompletionRoutine(pNewIrp, ReadCompletion, pData, TRUE, TRUE, TRUE);

			status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pNewIrp);

			return STATUS_PENDING;
		}
	}
}

NTSTATUS ReadCompletion (PDEVICE_OBJECT pDeviceObject, PIRP pNewIrp, PVOID context)
{
	PDATA pData;
	IO_STATUS_BLOCK status_block;

	pData = (PDATA)context;

//	__asm int 3;

	DbgPrint("Nonpassive request: status %X, info %X\n", 
		pNewIrp->IoStatus.Status, pNewIrp->IoStatus.Information);

	SaveBufferToIrp(pData->pIrp, (PCHAR)(pData->pSpt + 1), pData->Length);
	status_block = pData->pIrp->IoStatus;

	ReleaseIrp(pNewIrp);

	CompleteRequest(pData->pIrp, status_block.Status, status_block.Information, TRUE);

    IoReleaseRemoveLock(pData->pRemoveLock, pData->pIrp);

	ExFreePoolWithTag(pData->pSpt, POOL_TAG);
	ExFreePoolWithTag(pData, POOL_TAG);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

//
VOID CreateSptiReadRequest(SCSI_PASS_THROUGH *spt, ULONG StartSector, ULONG SectorCount)
{
	RtlZeroMemory(spt, sizeof(SCSI_PASS_THROUGH));

	spt->Length              = sizeof(SCSI_PASS_THROUGH);
	spt->CdbLength           = 12;							// CDB packet length
	spt->DataIn              = SCSI_IOCTL_DATA_IN;			// we will read
	//	spt->DataTransferLength  = (RAW_SECTOR_LENGTH + ECC_LENGTH) * SectorCount;	// bytes to read
	spt->DataTransferLength  = SECTOR_LENGTH * SectorCount;
	spt->SenseInfoLength     = 0;
	spt->TimeOutValue        = 1;							// time to wait for operation to finish
	spt->DataBufferOffset    = sizeof(SCSI_PASS_THROUGH);
	spt->SenseInfoOffset     = 0;								

	// Заполняем CDB
	spt->Cdb[0]              =  0xBE;						// (SPTI READ_CD)

	// number of the first sector to be read
	spt->Cdb[2]              = HIBYTE(HIWORD(StartSector));
	spt->Cdb[3]              = LOBYTE(HIWORD(StartSector));
	spt->Cdb[4]              = HIBYTE(LOWORD(StartSector));
	spt->Cdb[5]              = LOBYTE(LOWORD(StartSector));

	// number of sectors to read
	spt->Cdb[6]              = LOBYTE(HIWORD(SectorCount));
	spt->Cdb[7]              = HIBYTE(LOWORD(SectorCount));
	spt->Cdb[8]              = LOBYTE(LOWORD(SectorCount));

	//	spt->Cdb[9]              = 0xFA;						// Read 2352 + 294 bytes
	spt->Cdb[9]              = 0x10;						// Read 2048 bytes
}

VOID CreateSrb(PSCSI_REQUEST_BLOCK pSrb, ULONG StartSector, ULONG SectorCount, PCHAR ReadBuffer, PIRP pIrp)
{
	RtlZeroMemory(pSrb, sizeof(SCSI_REQUEST_BLOCK));

	pSrb->Length = sizeof(SCSI_REQUEST_BLOCK);
	pSrb->Function = SRB_FUNCTION_EXECUTE_SCSI;
	pSrb->CdbLength = 12;
	pSrb->SrbFlags = SRB_FLAGS_DATA_IN;
	pSrb->DataTransferLength = SECTOR_LENGTH * SectorCount;
	pSrb->DataBuffer = ReadBuffer;
	pSrb->TimeOutValue = 1;
	pSrb->OriginalRequest = pIrp;

	// fill CDB
	pSrb->Cdb[0]              =  0xBE;						// (SPTI READ_CD)

	// number of the first sector to be read
	pSrb->Cdb[2]              = HIBYTE(HIWORD(StartSector));
	pSrb->Cdb[3]              = LOBYTE(HIWORD(StartSector));
	pSrb->Cdb[4]              = HIBYTE(LOWORD(StartSector));
	pSrb->Cdb[5]              = LOBYTE(LOWORD(StartSector));

	// number of sectors to read
	pSrb->Cdb[6]              = LOBYTE(HIWORD(SectorCount));
	pSrb->Cdb[7]              = HIBYTE(LOWORD(SectorCount));
	pSrb->Cdb[8]              = LOBYTE(LOWORD(SectorCount));

	//	spt->Cdb[9]              = 0xFA;						// Read 2352 + 294 bytes
	pSrb->Cdb[9]              = 0x10;						// Read 2048 bytes
}