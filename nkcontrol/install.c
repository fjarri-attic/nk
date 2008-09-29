#include "install.h"

#include <setupapi.h>
#include <malloc.h>
#include <stdio.h>

ULONG InstallDriver()
{
	HINF inf;
	HSPFILEQ queue;
	UINT ErrorLine;
	PVOID Context;

	queue = SetupOpenFileQueue();

	inf = SetupOpenInfFile(".\\nkfilter.inf", NULL, INF_STYLE_WIN4, &ErrorLine);

	SetupInstallFilesFromInfSection(inf, NULL, queue,  "DefaultInstall.NT", NULL,
		SP_COPY_IN_USE_NEEDS_REBOOT | SP_COPY_NOSKIP);
	SetupInstallServicesFromInfSection(inf, "DefaultInstall.NT.Services", 0);
	SetupInstallFromInfSection(NULL, inf, "DefaultInstall.NT", SPINST_REGISTRY,
		NULL, NULL, 0, NULL, NULL, NULL, NULL);

	Context = SetupInitDefaultQueueCallback(NULL);
	SetupCommitFileQueue(NULL, queue, SetupDefaultQueueCallback, Context);
	SetupTermDefaultQueueCallback(Context);

	SetupCloseFileQueue(queue);
	SetupCloseInfFile(inf);

	printf("Installing successful.\n");

	return 0;
}

DWORD DeleteStringFromMultiSZ(char *str, DWORD str_size, const char *search_for)
{
	char *substr = str;
	int search_len = strlen(search_for);

	while(substr[0] != 0)
		if(!strcmp(search_for, substr))
			break;
		else
			substr += strlen(substr) + 1;

	if(str_size == search_len + 2)
	{
		str[0] = 0;
		return 1;
	}
	else
	{
		memmove(substr, substr + search_len + 1,
			str_size - (substr - str) - search_len - 1);
		return str_size - search_len - 1;
	}
};

ULONG UninstallDriver()
{
	SC_HANDLE ServiceManager;
	SC_HANDLE nkfilter;
	HKEY RegKey;
	LONG Result;

	PBYTE Buffer;
	DWORD BufferSize;

	// Removing from filters list
	printf("Removing from CDROM filters list...\n");

	Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"System\\CurrentControlSet\\Control\\Class\\{4d36e965-e325-11ce-bfc1-08002be10318}",
		0, KEY_READ | KEY_WRITE, &RegKey);

	Result = RegQueryValueEx(RegKey, "UpperFilters", 0, NULL, NULL, &BufferSize);

	Buffer = malloc(BufferSize);

	Result = RegQueryValueEx(RegKey, "UpperFilters", 0, NULL, Buffer, &BufferSize);

	BufferSize = DeleteStringFromMultiSZ(Buffer, BufferSize, "nkfilter");

	Result = RegSetValueEx(RegKey, "UpperFilters", 0, REG_MULTI_SZ, Buffer, BufferSize);

	free(Buffer);

	Result = RegCloseKey(RegKey);

	// Queueing nkfilter.sys deletion
/*	printf("Queueing driver deletion...\n");
	Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
		0, KEY_WRITE, &RegKey);

#define DELSTR "del /F /Q %windir%\\system32\\drivers\\nkfilter.sys"

	Result = RegSetValueEx(RegKey, "nkfilter.sys removal", 0, REG_EXPAND_SZ,
		DELSTR, strlen(DELSTR) + 1);

	Result = RegCloseKey(RegKey);*/

	// Deleting Service
	printf("Queueing service deletion...\n");
	ServiceManager = OpenSCManager(NULL, NULL, GENERIC_WRITE);

	nkfilter = OpenService(ServiceManager, "nkfilter", DELETE);

	DeleteService(nkfilter);
	CloseServiceHandle(nkfilter);
	CloseServiceHandle(ServiceManager);

	printf("Uninstallation successful.\n");

	return 0;
}
