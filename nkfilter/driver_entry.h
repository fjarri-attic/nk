//
// Точка входа в драйвер
// 

#ifndef _DRIVER_ENTRY_H
#define _DRIVER_ENTRY_H

#include <wdm.h>
#include "dispatch_pnp.h"
#include "dispatch_scsi.h"
#include "misc.h"

// Точка входа в драйвер
NTSTATUS	DriverEntry		(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath);

// Функция, вызываемая при выгрузке драйвера
VOID		DriverUnload	(IN PDRIVER_OBJECT pDriverObject);

#endif