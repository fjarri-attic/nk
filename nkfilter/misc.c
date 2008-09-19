//
// ��������� ������ � ��������������� �������
//

#include "misc.h"

//
NTSTATUS PassIrp(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION pDeviceExtension;
	NTSTATUS status;

	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// ������� ����� � ���������� ������
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);
    
	// ���� ���������� ��� ��������� - ��������� IRP
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// �������� IRP ���������� � ����� ��������
	IoSkipCurrentIrpStackLocation(pIrp); 
	status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// ������� �� ���������� ������
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
} 

//
NTSTATUS DispatchPower(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION pDeviceExtension;
	NTSTATUS status;
	
	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// ������������� Power Manager'� � ���, ��� ������� ����� ������������
	// ��������� IRP_MJ_POWER
	PoStartNextPowerIrp(pIrp);

	// ������� ����� � ���������� ������
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	// ���� ���������� ��� ��������� - ��������� IRP
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// �������� IRP ���������� � ����� ��������
	IoSkipCurrentIrpStackLocation(pIrp);
	status = PoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// ������� �� ���������� ������
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
} 

//
NTSTATUS CompleteRequest(IN PIRP pIrp, IN NTSTATUS status, IN ULONG_PTR info, IN BOOLEAN needs_boost)
{							
	CCHAR boost = IO_NO_INCREMENT;

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;

	// ���� ���������� ������� ���� ��������������, 
	// ����� ������� ����������� ������
	if(needs_boost) 
		boost = IO_CD_ROM_INCREMENT;

	// ��������� IRP
	IoCompleteRequest(pIrp, boost);
	return status;
}				

//
NTSTATUS AllocateBuffer(ULONG Length, OUT PCHAR *Address, OUT PMDL *Mdl)
{
	PMDL pMdl;
	PCHAR pBuf;

	pBuf = Allocate(Length);

	if(!pBuf)
		return STATUS_INSUFFICIENT_RESOURCES;

	*Address = pBuf;

	if(!Mdl)
		return STATUS_SUCCESS;

	pMdl = IoAllocateMdl(pBuf, Length, FALSE, FALSE, FALSE);
	MmBuildMdlForNonPagedPool(pMdl);

	if(!pMdl)
	{
		Free(pBuf);
		*Address = NULL;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	*Mdl = pMdl;

	return STATUS_SUCCESS;
}

//
VOID FreeBuffer(PCHAR pBuf, PMDL pMdl)
{
	PCHAR buf;

	if(pBuf && !pMdl)
		Free(pBuf);

	if(pMdl)
	{
		buf = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);
		IoFreeMdl(pMdl);
		Free(buf);
	}
}

//
VOID ReleaseIrp(IN OUT PIRP pIrp)
{
	PMDL pMdl, pNextMdl;

	pMdl = pIrp->MdlAddress;

	while(pMdl)
	{
		pNextMdl = pMdl->Next;

		MmUnlockPages(pMdl);
		IoFreeMdl(pMdl);

		pMdl = pNextMdl;
	}

	IoFreeIrp(pIrp);
}

//
NTSTATUS SaveBufferToIrp(IN OUT PIRP pIrp, IN PCHAR SourceBuffer, IN ULONG Length)
{
	PMDL		pMdl, pNextMdl;			// ��������� �� MDL
	PCHAR		MdlBuffer;				// ��������� �� ����� �� �������� MDL
	ULONG		PartLength;				// ����� �������� MDL
	ULONG		LengthProcessed;		// ������� ������������� ����������

	if(pIrp->MdlAddress)
	// ���� ������������ DIRECT_IO
	{
		pMdl = pIrp->MdlAddress;
		LengthProcessed = 0;

		while(pMdl)
		// �������� �� ���� ��������� MDL
		{
			// �������� ��������� �� �����, ��������� � ������� ��������
			MdlBuffer = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);
			if(!MdlBuffer)
				return STATUS_INSUFFICIENT_RESOURCES;

			// �������� ����� ������
			PartLength = MmGetMdlByteCount(pMdl);
			// ������������� ��������� �� ��������� ������� MDL
			pNextMdl = pMdl->Next;

			// ���� ������� ����� ��������� � �� ����� ��� �������,
			// ������������� ����� ������ �����
			if (LengthProcessed + PartLength > Length) 
				PartLength = Length - LengthProcessed;

			// �������� ����� ��������� ������ � ������� MDL
			RtlCopyMemory(MdlBuffer, SourceBuffer + LengthProcessed, PartLength);

			LengthProcessed += PartLength;

			// ��������� � ���������� �������
			pMdl = pNextMdl;
		}

		return STATUS_SUCCESS;
	}
	else
	// ���� ������������ BUFFERED_IO
	{
		if(!pIrp->AssociatedIrp.SystemBuffer)
			return STATUS_UNSUCCESSFUL;

		RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, SourceBuffer, Length);

		return STATUS_SUCCESS;
	}
}