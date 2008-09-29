#include "nkcontrol.h"

CHAR **Args;
ULONG ArgNum;

LONG __cdecl main(ULONG argc, CHAR *argv[])
{
	PCHAR command;
	int temp_n, temp_k;

	if(argc == 1)
	{
		PrintHelp();
		return 0;
	}

	command = argv[1];
	if(argc > 2)
		Args = argv + 2;
	else
		Args = NULL;
	ArgNum = argc - 2;

	if(!strcmp(command, "encode"))
	{
		temp_n = atoi(Args[0]);
		temp_k = atoi(Args[1]);

		if(temp_n < 1 || temp_n > 255 || temp_k < 1 || temp_k > 255)
		{
			printf("Parameters <n> and <k> must be positive integers between 1 and 255.\n");
			return 1;
		}
		if(temp_n <= temp_k)
		{
			printf("<n> must be greater then <k>.\n");
			return 1;
		}

		if(ArgNum == 2)
		{
			printf("Please specify the name of file to encode.\n");
			return 1;
		}
		if(ArgNum == 3)
		{
			printf("Please specify the name of resulting file.\n");
			return 1;
		}

        return CreateImage(Args[2], Args[3], (UCHAR)temp_n, (UCHAR)temp_k);
	}

	if(!strcmp(command, "test"))
	{
		temp_n = atoi(Args[0]);
		temp_k = atoi(Args[1]);

		printf("Testing %s with n = %d, k = %d\n", Args[2], temp_n, temp_k);
		return TestImage(Args[2], (UCHAR)temp_n, (UCHAR)temp_k);
	}

	if(!strcmp(command, "install"))
		return InstallDriver();

	if(!strcmp(command, "uninstall"))
		return UninstallDriver();

	printf("%s: unknown command. Read help:\n\n", command);
	PrintHelp();
	return 0;
}

VOID PrintHelp()
{
	printf("--- nkfilter control utility ---\n");
	printf("usage: nkcontrol [command] [parameters]\n");
	printf("For example: nkcontrol e: set 16 12\n\n");
	printf("Commands:\n");
	printf("install - install the filter\n");
	printf("uninstall - uninstall the filter\n");
	printf("enable <drive_letter> - turn the filter on\n");
	printf("disable <drive_letter> - turn the filter off\n");
	printf("getstatus <drive_letter> - get filter's current status\n");
	printf("encode <n> <k> <source image> <target image> - create the encoded image\n\n");
	printf("(be careful - foolproof is not completely implemented yet,\n");
	printf("so please follow these instructions strictly)\n");
}
