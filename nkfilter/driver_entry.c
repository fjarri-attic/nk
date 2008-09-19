// 
// Точка входа в драйвер
// 

#include "driver_entry.h"

// Путь в реестре к данным о драйвере 
UNICODE_STRING RegistryPath;

// 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	ULONG index;

	// Устанавливаем в качестве обработчика всех IRP дефолтную функцию 
    for (index = 0; index <= IRP_MJ_MAXIMUM_FUNCTION; index++) 
        pDriverObject->MajorFunction[index] = PassIrp;
    
	// Для нужных нам IRP устанавливаем свои обработчики
	pDriverObject->MajorFunction[IRP_MJ_POWER]			= DispatchPower;
	pDriverObject->MajorFunction[IRP_MJ_PNP]			= DispatchPnP;
	pDriverObject->MajorFunction[IRP_MJ_SCSI]			= DispatchScsi;
	
	// Обработчики AddDevice и Unload
	pDriverObject->DriverExtension->AddDevice	= AddDevice;
	pDriverObject->DriverUnload					= DriverUnload;

	// Запоминаем переданный нам ключ реестра
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