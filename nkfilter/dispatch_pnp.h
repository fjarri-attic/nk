//
// ��������� Plug'n'Play ��������
//

#ifndef _DISPATCH_PNP_H
#define _DISPATCH_PNP_H

#include <wdm.h>
#include "misc.h"

// ������������� ������ ����������
NTSTATUS AddDevice		(IN PDRIVER_OBJECT pDriverObject, 
						 IN PDEVICE_OBJECT pPhysicalDeviceObject);

// �������� ����������
VOID	RemoveDevice	(IN PDEVICE_OBJECT pDeviceObject);

// ��������� IRP_MJ_PNP
NTSTATUS DispatchPnP	(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

// ������� ���������� ��� ��������� �������� ����� IRP_MJ_PNP
NTSTATUS UsageNotificationCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context);
NTSTATUS StartDeviceCompletion		(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, PVOID context);

#endif