//
// ��������� Plug'n'Play ��������
//

#include "dispatch_pnp.h"

//
NTSTATUS AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhysicalDeviceObject)
{
	NTSTATUS			status;
	PDEVICE_OBJECT		pThisDeviceObject;
	PDEVICE_EXTENSION	pDeviceExtension;

	// ������� ������ ����������
	status = IoCreateDevice(pDriverObject,				// ��������� �� ������ ��������
							sizeof(DEVICE_EXTENSION),	// ������ ��������� DEVICE_EXTENSION
							NULL,						// ��������� �� ��� ��������
							FILE_DEVICE_CD_ROM,			// ��� ����������
							0,							// �������������� ����������
							FALSE,						// �������� ��������������
							&pThisDeviceObject);		// ���� ��������� ��������� �� ������ ���������� ����������

	if(!NT_SUCCESS(status))
	{
		return status;
	}

	// �������� ��������� DEVICE_EXTENSION
	pDeviceExtension = (PDEVICE_EXTENSION)pThisDeviceObject->DeviceExtension;

	IoInitializeRemoveLock(&pDeviceExtension->RemoveLock, 0, 0, 0);

	pDeviceExtension->pThisDeviceObject = pThisDeviceObject;			
	pDeviceExtension->pPhysicalDeviceObject = pPhysicalDeviceObject;	 
	pDeviceExtension->pDriverObject = pDriverObject;

	// ��������� ��� ������ � ����� ���������
	pDeviceExtension->pNextDeviceObject = IoAttachDeviceToDeviceStack(pThisDeviceObject, pPhysicalDeviceObject);
	if(pDeviceExtension->pNextDeviceObject == NULL)
	{
		IoDeleteDevice(pThisDeviceObject);
		return STATUS_NO_SUCH_DEVICE;
	}

	// ��������� �� ���������� �� ����������� ���������� � ����� ��������� ����� 
	pThisDeviceObject->Flags |= pDeviceExtension->pNextDeviceObject->Flags &
		(DO_POWER_PAGABLE | DO_DIRECT_IO | DO_BUFFERED_IO);

	// ������������� ������� ���������, ������� ��������������� ���� 
	pThisDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
} 

//
VOID RemoveDevice(IN PDEVICE_OBJECT pDeviceObject)
{
	PDEVICE_EXTENSION pDeviceExtension;

	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// ����������� ���������� �� �����
	if (pDeviceExtension->pNextDeviceObject)
		IoDetachDevice(pDeviceExtension->pNextDeviceObject);

	// ������� ����������
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

	// ������� ����� � ���������� ������
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	// ���� ���������� ��� ��������� - ��������� IRP
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// ����������� � ������� ����������� ������ (�������� � �.�.) �� ���� ����������
	if (minor_func == IRP_MN_DEVICE_USAGE_NOTIFICATION)
	{
		// ���� ��� ������� ��������� �� ������� �����, ��� ����������� ������� ����� ����
		// DO_POWER_PAGABLE, �� �� ������������� ���� ���� � ����
		if (!pDeviceObject->AttachedDevice || 
			(pDeviceObject->AttachedDevice->Flags & DO_POWER_PAGABLE))
			pDeviceObject->Flags |= DO_POWER_PAGABLE;

		// ��������� ������� ��� ���������� ������� � ��������� ��� ���� �� �����
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(	pIrp, UsageNotificationCompletion,
								(PVOID)pDeviceExtension, TRUE, TRUE, TRUE );

		return IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);
	}

	// ����������� � ������� ����������
	if (minor_func == IRP_MN_START_DEVICE)
	{
		// ��������� ������� ��� ���������� ������� � ��������� ��� ���� �� �����
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(	pIrp, StartDeviceCompletion,
								(PVOID)pDeviceExtension, TRUE, TRUE, TRUE );
		return IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);
	}

	// ����������� �� �������� ����������
	if (minor_func == IRP_MN_REMOVE_DEVICE)
	{
		// ��������� ������ ���� �� �����
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

		// ������� �� ���������� ������ � ���� ���������� ���� IRP
		IoReleaseRemoveLockAndWait(&pDeviceExtension->RemoveLock, pIrp);

		// ������ ����������
		RemoveDevice(pDeviceObject);
		return status;
	}

	// ��������� ������ ���� �� �����
	IoSkipCurrentIrpStackLocation(pIrp);
	status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// ������� �� ���������� ������
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
}

//
NTSTATUS UsageNotificationCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	PDEVICE_EXTENSION pDeviceExtension;
	
	pDeviceExtension = (PDEVICE_EXTENSION)context;

	// ���� ��� IRP ��������� ���������� ���������, ������� ��� ��������������� �������
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);
	
	// ���� ����������� ���������� �������� ���� DO_POWER_PAGABLE, ������� ��� � ��
	if (!(pDeviceExtension->pNextDeviceObject->Flags & DO_POWER_PAGABLE))
		pDeviceObject->Flags &= ~DO_POWER_PAGABLE;

	// ������� �� ���������� ������, � ������� �� ����� � DispatchPnP
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return STATUS_SUCCESS;
}

//
NTSTATUS StartDeviceCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	PDEVICE_EXTENSION pDeviceExtension;

	pDeviceExtension = (PDEVICE_EXTENSION)context;

	// ���� ��� IRP ��������� ���������� ���������, ������� ��� ��������������� �������
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	// ���� � ������������ ���������� ���������� ���� FILE_REMOVABLE_MEDIA, ��������� ��� � ��
	if (pDeviceExtension->pNextDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		pDeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
	
	// ������� �� ���������� ������, � ������� �� ����� � DispatchPnP
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return STATUS_SUCCESS;
}


