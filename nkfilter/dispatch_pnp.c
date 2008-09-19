//
// Обработка Plug'n'Play запросов
//

#include "dispatch_pnp.h"

//
NTSTATUS AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pPhysicalDeviceObject)
{
	NTSTATUS			status;
	PDEVICE_OBJECT		pThisDeviceObject;
	PDEVICE_EXTENSION	pDeviceExtension;

	// Создать объект устройства
	status = IoCreateDevice(pDriverObject,				// указатель на объект драйвера
							sizeof(DEVICE_EXTENSION),	// размер структуры DEVICE_EXTENSION
							NULL,						// указатель на имя драйвера
							FILE_DEVICE_CD_ROM,			// тип устройства
							0,							// характеристики устройства
							FALSE,						// параметр зарезервирован
							&pThisDeviceObject);		// сюда запишется указатель на объект созданного устройства

	if(!NT_SUCCESS(status))
	{
		return status;
	}

	// Заполним структуру DEVICE_EXTENSION
	pDeviceExtension = (PDEVICE_EXTENSION)pThisDeviceObject->DeviceExtension;

	IoInitializeRemoveLock(&pDeviceExtension->RemoveLock, 0, 0, 0);

	pDeviceExtension->pThisDeviceObject = pThisDeviceObject;			
	pDeviceExtension->pPhysicalDeviceObject = pPhysicalDeviceObject;	 
	pDeviceExtension->pDriverObject = pDriverObject;

	// Прикрепим наш фильтр к стеку драйверов
	pDeviceExtension->pNextDeviceObject = IoAttachDeviceToDeviceStack(pThisDeviceObject, pPhysicalDeviceObject);
	if(pDeviceExtension->pNextDeviceObject == NULL)
	{
		IoDeleteDevice(pThisDeviceObject);
		return STATUS_NO_SUCH_DEVICE;
	}

	// Передадим по наследству от предыдущего устройства в стеке некоторые флаги 
	pThisDeviceObject->Flags |= pDeviceExtension->pNextDeviceObject->Flags &
		(DO_POWER_PAGABLE | DO_DIRECT_IO | DO_BUFFERED_IO);

	// Инициализация фильтра закончена, снимаем соответствующий флаг 
	pThisDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
} 

//
VOID RemoveDevice(IN PDEVICE_OBJECT pDeviceObject)
{
	PDEVICE_EXTENSION pDeviceExtension;

	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// Отсоединяем устройство от стека
	if (pDeviceExtension->pNextDeviceObject)
		IoDetachDevice(pDeviceExtension->pNextDeviceObject);

	// Удаляем устройство
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

	// Попытка входа в защищенную секцию
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	// Если устройство уже удаляется - завершаем IRP
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// Уведомление о наличии специальных файлов (подкачки и т.п.) на этом устройстве
	if (minor_func == IRP_MN_DEVICE_USAGE_NOTIFICATION)
	{
		// Если наш драйвер находится на вершине стека, или вышележащий драйвер имеет флаг
		// DO_POWER_PAGABLE, то мы устанавливаем этот флаг у себя
		if (!pDeviceObject->AttachedDevice || 
			(pDeviceObject->AttachedDevice->Flags & DO_POWER_PAGABLE))
			pDeviceObject->Flags |= DO_POWER_PAGABLE;

		// Установим функцию для завершения запроса и передадим его вниз по стеку
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(	pIrp, UsageNotificationCompletion,
								(PVOID)pDeviceExtension, TRUE, TRUE, TRUE );

		return IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);
	}

	// Уведомление о запуске устройства
	if (minor_func == IRP_MN_START_DEVICE)
	{
		// Установим функцию для завершения запроса и передадим его вниз по стеку
		IoCopyCurrentIrpStackLocationToNext(pIrp);
		IoSetCompletionRoutine(	pIrp, StartDeviceCompletion,
								(PVOID)pDeviceExtension, TRUE, TRUE, TRUE );
		return IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);
	}

	// Уведомление об удалении устройства
	if (minor_func == IRP_MN_REMOVE_DEVICE)
	{
		// Передадим запрос вниз по стеку
		IoSkipCurrentIrpStackLocation(pIrp);
		status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

		// Выходим из защищенной секции и ждем завершения всех IRP
		IoReleaseRemoveLockAndWait(&pDeviceExtension->RemoveLock, pIrp);

		// Удалим устройство
		RemoveDevice(pDeviceObject);
		return status;
	}

	// Передадим запрос вниз по стеку
	IoSkipCurrentIrpStackLocation(pIrp);
	status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// Выходим из защищенной секции
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
}

//
NTSTATUS UsageNotificationCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	PDEVICE_EXTENSION pDeviceExtension;
	
	pDeviceExtension = (PDEVICE_EXTENSION)context;

	// Если для IRP требуется дальнейшая обработка, пометим его соответствующим образом
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);
	
	// Если нижележащее устройство очистило флаг DO_POWER_PAGABLE, очистим его и мы
	if (!(pDeviceExtension->pNextDeviceObject->Flags & DO_POWER_PAGABLE))
		pDeviceObject->Flags &= ~DO_POWER_PAGABLE;

	// Выходим из защищенной секции, в которую мы вошли в DispatchPnP
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return STATUS_SUCCESS;
}

//
NTSTATUS StartDeviceCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context)
{
	PDEVICE_EXTENSION pDeviceExtension;

	pDeviceExtension = (PDEVICE_EXTENSION)context;

	// Если для IRP требуется дальнейшая обработка, пометим его соответствующим образом
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	// Если у нижележащего устройства установлен флаг FILE_REMOVABLE_MEDIA, установим его и мы
	if (pDeviceExtension->pNextDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		pDeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
	
	// Выходим из защищенной секции, в которую мы вошли в DispatchPnP
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return STATUS_SUCCESS;
}


