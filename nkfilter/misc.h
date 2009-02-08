//
// Data structures and auxiliary functions
//

#ifndef _MISC_H
#define _MISC_H

#include <wdm.h>

// Memory allocation tag ("nkf ")
#define POOL_TAG ' fkn'

#define Allocate(len)	ExAllocatePoolWithTag(NonPagedPool, len, POOL_TAG)
#define Free(ptr)		ExFreePoolWithTag(ptr, POOL_TAG)

// Macros for CDB filling
#define LOWORD(l)		((USHORT)((ULONG)(l) & 0xffff))
#define HIWORD(l)		((USHORT)((ULONG)(l) >> 16))
#define LOBYTE(w)		((UCHAR)((ULONG)(w) & 0xff))
#define HIBYTE(w)		((UCHAR)((ULONG)(w) >> 8))

// Device data
typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT	pThisDeviceObject;		// Pointer to device object of our driver
	PDEVICE_OBJECT	pNextDeviceObject;		// Pointer to child device object
	PDEVICE_OBJECT	pPhysicalDeviceObject;	// Pointer to physical device object
	PDRIVER_OBJECT	pDriverObject;			// Pointer to our driver object

	IO_REMOVE_LOCK	RemoveLock;				// Lock for preventing untimely driver unload
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Default handler for IRPs
NTSTATUS PassIrp			(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// Handler for IRP_MJ_POWER
NTSTATUS DispatchPower		(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// IRP completion with specified parameters
NTSTATUS CompleteRequest	(IN PIRP pIrp, IN NTSTATUS status,
							 IN ULONG_PTR info, IN BOOLEAN needs_boost);

//
NTSTATUS AllocateBuffer(ULONG Length, OUT PCHAR *Address, OUT PMDL *Mdl);
VOID FreeBuffer(PCHAR pBuf, PMDL pMdl);

// Release IRP
VOID	ReleaseIrp			(IN OUT PIRP pIrp);

// Save buffer to IRP
NTSTATUS SaveBufferToIrp	(IN OUT PIRP pIrp, IN PCHAR SourceBuffer, IN ULONG Length);

// Decode subchannel data
VOID DecodeSubchannelData(UCHAR *subchannel_data);

#endif
