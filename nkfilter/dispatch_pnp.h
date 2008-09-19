//
// Обработка Plug'n'Play запросов
//

#ifndef _DISPATCH_PNP_H
#define _DISPATCH_PNP_H

#include <wdm.h>
#include "misc.h"

// Инициализация нового устройства
NTSTATUS AddDevice		(IN PDRIVER_OBJECT pDriverObject, 
						 IN PDEVICE_OBJECT pPhysicalDeviceObject);

// Удаление устройства
VOID	RemoveDevice	(IN PDEVICE_OBJECT pDeviceObject);

// Обработка IRP_MJ_PNP
NTSTATUS DispatchPnP	(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// Функции завершения для различных минорных кодов IRP_MJ_PNP
NTSTATUS UsageNotificationCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context);
NTSTATUS StartDeviceCompletion		(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context);

#endif