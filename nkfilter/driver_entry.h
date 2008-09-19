//
// ����� ����� � �������
// 

#ifndef _DRIVER_ENTRY_H
#define _DRIVER_ENTRY_H

#include <wdm.h>
#include "dispatch_pnp.h"
#include "dispatch_scsi.h"
#include "misc.h"

// ����� ����� � �������
NTSTATUS	DriverEntry		(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath);

// �������, ���������� ��� �������� ��������
VOID		DriverUnload	(IN PDRIVER_OBJECT pDriverObject);

#endif