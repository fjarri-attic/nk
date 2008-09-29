//
// Plug'n'Play requests processing
//

#include "dispatch_pnp.h"

//
NTSTATUS AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhysicalDeviceObject)
{
	NTSTATUS			status;
	PDEVICE_OBJECT		pThisDeviceObject;
	PDEVICE_EXTENSION	pDeviceExtension;

	// create device object
	status = IoCreateDevice(pDriverObject,				// pointer to driver object
							sizeof(DEVICE_EXTENSION),	//
							NULL,						// pointer to driver name
							FILE_DEVICE_CD_ROM,			// device type
							0,							// device characteristics
							FALSE,						// reserved
							&pThisDeviceObject);		// pointer to device will be saved here

	if(!NT_SUCCESS(status))
	{
		return status;
	}

	// fill DEVICE_EXTENSION
	pDeviceExtension = (PDEVICE_EXTENSION)pThisDeviceObject->DeviceExtension;

	IoInitializeRemoveLock(&pDeviceExtension->RemoveLock, 0, 0, 0);

	pDeviceExtension->pThisDeviceObject = pThisDeviceObject;
	pDeviceExtension->pPhysicalDeviceObject = pPhysicalDeviceObject;
	pDeviceExtension->pDriverObject = pDriverObject;

	// attach our filter to driver stack
	pDeviceExtension->pNextDeviceObject = IoAttachDeviceToDeviceStack(pThisDeviceObject, pPhysicalDeviceObject);
	if(pDeviceExtension->pNextDeviceObject == NULL)
	{
		IoDeleteDevice(pThisDeviceObject);
		return STATUS_NO_SUCH_DEVICE;
	}

	// inherit some flags
	pThisDeviceObject->Flags |= pDeviceExtension->pNextDeviceObject->Flags &
		(DO_POWER_PAGABLE | DO_DIRECT_IO | DO_BUFFERED_IO);

	// finish initialization
	pThisDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

//
VOID RemoveDevice(IN PDEVICE_OBJECT pDeviceObject)
{
	PDEVICE_EXTENSION pDeviceExtension;

	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// detach device from stack
	if (pDeviceExtension->pNextDeviceObject)
		IoDetachDevice(pDeviceExtension->pNextDeviceObject);

	// delete device
	IoDeleteDevice(pDeviceObject);
}

//
NTSTATUS DispatchPnP(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PIO_STACK_LOCATION pStack;
	ULONG minor_func;
	NTSTATUS status;
	PDEVICE_EXTENSION pDeviceExtension;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	minor_func = pStack->MinorFunction;
	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// enter critical section
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	// if device is already removing - complete IRP
	// FIXME: we should release lock here
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// notification that tells there are some special files on this device (swap files etc)
	if (minor_func == IRP_MN_DEVICE_USAGE_NOTIFICATION)
	{
		// if our driver is on the top of the stack or parent driver has DO_POWER_PAGABLE,
		// set this flag too
		if (!pDeviceObject->AttachedDevice ||
			(pDeviceObject->AttachedDevice->Flags & DO_POWER_PAGABLE))
			pDeviceObject->Flags |= DO_POWER_PAGABLE;

		// set completion function and pass request further
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(	pIrp, UsageNotificationCompletion,
								(PVOID)pDeviceExtension, TRUE, TRUE, TRUE );

		return IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);
	}

	// starting notification
	if (minor_func == IRP_MN_START_DEVICE)
	{
		// set completion function and pass request further
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(	pIrp, StartDeviceCompletion,
								(PVOID)pDeviceExtension, TRUE, TRUE, TRUE );
		return IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);
	}

	// device deletion notification
	if (minor_func == IRP_MN_REMOVE_DEVICE)
	{
		// pass request further
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

		// release lock and wait for all IRPs completion
		IoReleaseRemoveLockAndWait(&pDeviceExtension->RemoveLock, pIrp);

		// remove device
		RemoveDevice(pDeviceObject);
		return status;
	}

	// pass request further
	IoSkipCurrentIrpStackLocation(pIrp);
	status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// release lock
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
}

//
NTSTATUS UsageNotificationCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	PDEVICE_EXTENSION pDeviceExtension;

	pDeviceExtension = (PDEVICE_EXTENSION)context;

	// If IRP requires further handling, mark it accordingly
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	// If child device cleared DO_POWER_PAGABLE, clear it too
	if (!(pDeviceExtension->pNextDeviceObject->Flags & DO_POWER_PAGABLE))
		pDeviceObject->Flags &= ~DO_POWER_PAGABLE;

	// release lock, which we acquired in DispatchPnP()
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return STATUS_SUCCESS;
}

//
NTSTATUS StartDeviceCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	PDEVICE_EXTENSION pDeviceExtension;

	pDeviceExtension = (PDEVICE_EXTENSION)context;

	// If IRP requires further handling, mark it accordingly
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	// If child device has FILE_REMOVABLE_MEDIA set, set it too
	if (pDeviceExtension->pNextDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		pDeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;

	// release lock, which we acquired in DispatchPnP()
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return STATUS_SUCCESS;
}
