//
// Data structures and auxiliary functions
//
#include "misc.h"
#include "nkcodec.h"

//
NTSTATUS PassIrp(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION pDeviceExtension;
	NTSTATUS status;

	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// try to acquire lock
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	// if the device is also removing - complete IRP
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// pass IRP further
	IoSkipCurrentIrpStackLocation(pIrp);
	status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// release lock
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
}

//
NTSTATUS DispatchPower(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION pDeviceExtension;
	NTSTATUS status;

	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// tell Power Manager that the driver is ready to handle next IRP_MJ_POWER
	PoStartNextPowerIrp(pIrp);

	// acquire lock
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	// if the device is also removing - complete IRP
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// pass IRP further
	IoSkipCurrentIrpStackLocation(pIrp);
	status = PoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// release lock
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
}

//
NTSTATUS CompleteRequest(IN PIRP pIrp, IN NTSTATUS status, IN ULONG_PTR info, IN BOOLEAN needs_boost)
{
	CCHAR boost = IO_NO_INCREMENT;

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;

	// If IRP handling took long, give priority boost to calling thread
	if(needs_boost)
		boost = IO_CD_ROM_INCREMENT;

	// complete IRP
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
	PMDL		pMdl, pNextMdl;			// pointers to MDLs
	PCHAR		MdlBuffer;				// pointer to MDL buffer
	ULONG		PartLength;				// MDL element length
	ULONG		LengthProcessed;		// Copied data counter

	if(pIrp->MdlAddress)
	// if DIRECT_IO is used
	{
		pMdl = pIrp->MdlAddress;
		LengthProcessed = 0;

		while(pMdl)
		// enumerate all MDL elements
		{
			// get pointer to buffer of current elements
			MdlBuffer = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);
			if(!MdlBuffer)
				return STATUS_INSUFFICIENT_RESOURCES;

			// get buffer length
			PartLength = MmGetMdlByteCount(pMdl);
			// set pointer to next MDL
			pNextMdl = pMdl->Next;

			// if current part is final and we don't need the whole data,
			// set necessary part length
			if (LengthProcessed + PartLength > Length)
				PartLength = Length - LengthProcessed;

			// copy from source buffer to MDL element
			RtlCopyMemory(MdlBuffer, SourceBuffer + LengthProcessed, PartLength);

			LengthProcessed += PartLength;

			// go to next element
			pMdl = pNextMdl;
		}

		return STATUS_SUCCESS;
	}
	else
	// if BUFFERED_IO is used
	{
		if(!pIrp->AssociatedIrp.SystemBuffer)
			return STATUS_UNSUCCESSFUL;

		RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, SourceBuffer, Length);

		return STATUS_SUCCESS;
	}
}


// The function is inplace!
VOID DecodeSubchannelData(UCHAR *subchannel_data)
{
	int subchannel, j;
	UCHAR buf[SUBCHANNELS_LEN];
	UCHAR bit;
	
	RtlZeroMemory(buf, ARRAYSIZE(buf));

	// Only P and Q subchannels are present in raw subchannel data as is.
	// R-W subchannels should be deinterleaved after extraction.
	// Since at the moment we need just P and Q, we drop R-W subchannel data.

	// for each byte of raw data
	for(j = 0; j < ARRAYSIZE(buf); j++)
		for(subchannel = 0; subchannel < 2; subchannel++)
		{
			// extract subchannel bit
			bit = ((subchannel_data[j] & (1 << (7 - subchannel))) == 0 ? 0 : 1);

			// save it to a proper place in decoded data
			buf[subchannel * ARRAYSIZE(buf) / 8 + j / 8] |= (bit << (7 - j % 8));
		}

	// save decode data in place of raw data
	RtlCopyMemory(subchannel_data, buf, ARRAYSIZE(buf));
}
