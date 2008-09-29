//
// IRP_MJ_READ handling
//

#ifndef _DISPATCH_READ_H
#define _DISPATCH_READ_H

#include <wdm.h>
#include <ntddscsi.h>
#include "misc.h"

NTSTATUS DispatchRead	(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS ReadCompletion	(PDEVICE_OBJECT pDeviceObject, PIRP pNewIrp, PVOID context);
VOID CreateSptiReadRequest(SCSI_PASS_THROUGH *spt, ULONG StartSector, ULONG SectorCount);
VOID CreateSrb(PSCSI_REQUEST_BLOCK pSrb, ULONG StartSector, ULONG SectorCount, PCHAR ReadBuffer, PIRP pIrp);

NTKERNELAPI BOOLEAN IoIsOperationSynchronous(PIRP pIrp);

#endif
