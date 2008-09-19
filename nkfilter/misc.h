//
// Структуры данных и вспомогательные функции
//

#ifndef _MISC_H
#define _MISC_H

#include <wdm.h>

// Таг для выделения памяти ("nkf ")
#define POOL_TAG ' fkn'

#define Allocate(len)	ExAllocatePoolWithTag(NonPagedPool, len, POOL_TAG)
#define Free(ptr)		ExFreePoolWithTag(ptr, POOL_TAG)

// Макросы, использующиеся при заполнении CDB
#define LOWORD(l)		((USHORT)((ULONG)(l) & 0xffff))
#define HIWORD(l)		((USHORT)((ULONG)(l) >> 16))
#define LOBYTE(w)		((UCHAR)((ULONG)(w) & 0xff))
#define HIBYTE(w)		((UCHAR)((ULONG)(w) >> 8))

// Данные для объекта устройства, создаваемого драйвером
typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT	pThisDeviceObject;		// Указатель на объект устройства нашего драйвера
	PDEVICE_OBJECT	pNextDeviceObject;		// Указатель на объект устройства драйвера, идущего ниже в иерархии
	PDEVICE_OBJECT	pPhysicalDeviceObject;	// Указатель на объект физического устройства в конце цепочки драйверов
	PDRIVER_OBJECT	pDriverObject;			// Указатель на объект нашего драйвера

	IO_REMOVE_LOCK	RemoveLock;				// Объект синхронизации для защиты от несвоевременной
											// выгрузки драйвера
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Обработчик по умолчанию для IRP
NTSTATUS PassIrp			(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// Функция для обработки IRP_MJ_POWER
NTSTATUS DispatchPower		(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// Завершение IRP запроса с заданными параметрами
NTSTATUS CompleteRequest	(IN PIRP pIrp, IN NTSTATUS status, 
							 IN ULONG_PTR info, IN BOOLEAN needs_boost);

//
NTSTATUS AllocateBuffer(ULONG Length, OUT PCHAR *Address, OUT PMDL *Mdl);
VOID FreeBuffer(PCHAR pBuf, PMDL pMdl);	

// Удаление IRP
VOID	ReleaseIrp			(IN OUT PIRP pIrp);

// Сохранить буфер в IRP
NTSTATUS SaveBufferToIrp	(IN OUT PIRP pIrp, IN PCHAR SourceBuffer, IN ULONG Length);

#endif