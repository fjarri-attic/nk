// 
// ����� ����� � �������
// 

#include "driver_entry.h"

// ���� � ������� � ������ � �������� 
UNICODE_STRING RegistryPath;

// 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	ULONG index;

	// ������������� � �������� ����������� ���� IRP ��������� ������� 
    for (index = 0; index <= IRP_MJ_MAXIMUM_FUNCTION; index++) 
        pDriverObject->MajorFunction[index] = PassIrp;
    
	// ��� ������ ��� IRP ������������� ���� �����������
	pDriverObject->MajorFunction[IRP_MJ_POWER]			= DispatchPower;
	pDriverObject->MajorFunction[IRP_MJ_PNP]			= DispatchPnP;
	pDriverObject->MajorFunction[IRP_MJ_SCSI]			= DispatchScsi;
	
	// ����������� AddDevice � Unload
	pDriverObject->DriverExtension->AddDevice	= AddDevice;
	pDriverObject->DriverUnload					= DriverUnload;

	// ���������� ���������� ��� ���� �������
	RegistryPath.MaximumLength = pRegistryPath->Length + sizeof(UNICODE_NULL);
	RegistryPath.Buffer = ExAllocatePool(PagedPool,	RegistryPath.MaximumLength);
	if (RegistryPath.Buffer != NULL)
		RtlCopyUnicodeString(&RegistryPath, pRegistryPath);
	else
		return STATUS_INSUFFICIENT_RESOURCES;

	return STATUS_SUCCESS;
} 

// 
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	RtlFreeUnicodeString(&RegistryPath);
}