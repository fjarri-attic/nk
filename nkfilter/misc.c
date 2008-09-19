//
// Структуры данных и вспомогательные функции
//

#include "misc.h"

//
NTSTATUS PassIrp(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION pDeviceExtension;
	NTSTATUS status;

	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// Попытка входа в защищенную секцию
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);
    
	// Если устройство уже удаляется - завершаем IRP
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// Передаем IRP следующему в стеке драйверу
	IoSkipCurrentIrpStackLocation(pIrp); 
	status = IoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// Выходим из защищенной секции
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
} 

//
NTSTATUS DispatchPower(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION pDeviceExtension;
	NTSTATUS status;
	
	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	// Сигнализируем Power Manager'у о том, что драйвер готов обрабатывать
	// следующую IRP_MJ_POWER
	PoStartNextPowerIrp(pIrp);

	// Попытка входа в защищенную секцию
	status = IoAcquireRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	// Если устройство уже удаляется - завершаем IRP
	if (status != STATUS_SUCCESS)
		return CompleteRequest(pIrp, status, 0, FALSE);

	// Передаем IRP следующему в стеке драйверу
	IoSkipCurrentIrpStackLocation(pIrp);
	status = PoCallDriver(pDeviceExtension->pNextDeviceObject, pIrp);

	// Выходим из защищенной секции
	IoReleaseRemoveLock(&pDeviceExtension->RemoveLock, pIrp);

	return status;
} 

//
NTSTATUS CompleteRequest(IN PIRP pIrp, IN NTSTATUS status, IN ULONG_PTR info, IN BOOLEAN needs_boost)
{							
	CCHAR boost = IO_NO_INCREMENT;

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;

	// Если выполнение запроса было долговременным, 
	// дадим прирост вызывающему потоку
	if(needs_boost) 
		boost = IO_CD_ROM_INCREMENT;

	// Завершаем IRP
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
	PMDL		pMdl, pNextMdl;			// Указатели на MDL
	PCHAR		MdlBuffer;				// Указатель на буфер из элемента MDL
	ULONG		PartLength;				// Длина элемента MDL
	ULONG		LengthProcessed;		// Счетчик скопированной информации

	if(pIrp->MdlAddress)
	// Если используется DIRECT_IO
	{
		pMdl = pIrp->MdlAddress;
		LengthProcessed = 0;

		while(pMdl)
		// Проходим по всем элементам MDL
		{
			// Получаем указатель на буфер, описанный в текущем элементе
			MdlBuffer = MmGetSystemAddressForMdlSafe(pMdl, HighPagePriority);
			if(!MdlBuffer)
				return STATUS_INSUFFICIENT_RESOURCES;

			// Получаем длину буфера
			PartLength = MmGetMdlByteCount(pMdl);
			// Устанавливаем указатель на следующий элемент MDL
			pNextMdl = pMdl->Next;

			// Если текущий кусок последний и не нужен нам целиком,
			// устанавливаем длину нужной части
			if (LengthProcessed + PartLength > Length) 
				PartLength = Length - LengthProcessed;

			// Копируем часть исходного буфера в элемент MDL
			RtlCopyMemory(MdlBuffer, SourceBuffer + LengthProcessed, PartLength);

			LengthProcessed += PartLength;

			// Переходим к следующему элемену
			pMdl = pNextMdl;
		}

		return STATUS_SUCCESS;
	}
	else
	// Если используется BUFFERED_IO
	{
		if(!pIrp->AssociatedIrp.SystemBuffer)
			return STATUS_UNSUCCESSFUL;

		RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, SourceBuffer, Length);

		return STATUS_SUCCESS;
	}
}