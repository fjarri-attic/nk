//
// ��������� ������ � ��������������� �������
//

#ifndef _MISC_H
#define _MISC_H

#include <wdm.h>

// ��� ��� ��������� ������ ("nkf ")
#define POOL_TAG ' fkn'

#define Allocate(len)	ExAllocatePoolWithTag(NonPagedPool, len, POOL_TAG)
#define Free(ptr)		ExFreePoolWithTag(ptr, POOL_TAG)

// �������, �������������� ��� ���������� CDB
#define LOWORD(l)		((USHORT)((ULONG)(l) & 0xffff))
#define HIWORD(l)		((USHORT)((ULONG)(l) >> 16))
#define LOBYTE(w)		((UCHAR)((ULONG)(w) & 0xff))
#define HIBYTE(w)		((UCHAR)((ULONG)(w) >> 8))

// ������ ��� ������� ����������, ������������ ���������
typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT	pThisDeviceObject;		// ��������� �� ������ ���������� ������ ��������
	PDEVICE_OBJECT	pNextDeviceObject;		// ��������� �� ������ ���������� ��������, ������� ���� � ��������
	PDEVICE_OBJECT	pPhysicalDeviceObject;	// ��������� �� ������ ����������� ���������� � ����� ������� ���������
	PDRIVER_OBJECT	pDriverObject;			// ��������� �� ������ ������ ��������

	IO_REMOVE_LOCK	RemoveLock;				// ������ ������������� ��� ������ �� ���������������
											// �������� ��������
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// ���������� �� ��������� ��� IRP
NTSTATUS PassIrp			(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// ������� ��� ��������� IRP_MJ_POWER
NTSTATUS DispatchPower		(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// ���������� IRP ������� � ��������� �����������
NTSTATUS CompleteRequest	(IN PIRP pIrp, IN NTSTATUS status, 
							 IN ULONG_PTR info, IN BOOLEAN needs_boost);

//
NTSTATUS AllocateBuffer(ULONG Length, OUT PCHAR *Address, OUT PMDL *Mdl);
VOID FreeBuffer(PCHAR pBuf, PMDL pMdl);	

// �������� IRP
VOID	ReleaseIrp			(IN OUT PIRP pIrp);

// ��������� ����� � IRP
NTSTATUS SaveBufferToIrp	(IN OUT PIRP pIrp, IN PCHAR SourceBuffer, IN ULONG Length);

#endif