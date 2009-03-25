#ifndef _DISPATCH_SCSI_H
#define _DISPATCH_SCSI_H

#include <wdm.h>
#include <ntddscsi.h>
#include <srb.h>
#include <scsi.h>

#include "misc.h"
#include "nkcodec.h"

typedef struct _READ_REQUEST
{
	// Source request
	PIRP pSourceIrp;
	PSCSI_REQUEST_BLOCK pSourceSrb;
	PCDB pSourceCdb;
	PCHAR SourceBuffer;
	CCHAR StackSize;
	PDEVICE_OBJECT pNextDevice;

	ULONG StartingSector;
	ULONG SectorsNum;

	PIO_REMOVE_LOCK pRemoveLock;

	// Decoding parameters
	UCHAR n;
	UCHAR k;

	// Requests to lower driver
	PENCODED_BLOCK ToRead;
	ULONG BlocksNum;
	PCHAR ReadBuffer;
	PMDL ReadBufferMdl;

	// Current status
	ULONG CurrentBlock;
	ULONG CurrentLength;
} READ_REQUEST, *PREAD_REQUEST;

typedef struct _DETECTION_REQUEST
{
	PIRP pSourceIrp;
	PDEVICE_OBJECT pNextDevice;

	PCHAR ReadBuffer;
	PMDL ReadBufferMdl;

	ULONG CurrentSector;
	ULONG AttemptCounter;
	ULONG MaxAttempts;

	UCHAR N;
	UCHAR K;
	UCHAR ReadOffset;
} DETECTION_REQUEST, *PDETECTION_REQUEST;

NTSTATUS DispatchScsi (IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
NTSTATUS ReadCompletion	(PDEVICE_OBJECT pDeviceObject, PIRP pNewIrp, PVOID context);
NTSTATUS DetectionCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pNewIrp, PVOID context);
VOID ReadCurrentBase(PREAD_REQUEST pRead);
PIRP CreateIrp(CCHAR StackSize, ULONG StartingSector, ULONG SectorsCount,
			   PMDL read_buffer_mdl, PCHAR read_buffer, BOOLEAN get_data, BOOLEAN get_subchannels);
VOID FreeIrp(PIRP pIrp);
VOID StartReading(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DeductionCompletion(PDEVICE_OBJECT pDeviceObject, PIRP pNewIrp, PVOID context);
VOID StartDeduction(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
VOID ReadNextSector(PDETECTION_REQUEST pRequest);

#endif
