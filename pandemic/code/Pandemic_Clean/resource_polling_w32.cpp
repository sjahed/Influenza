#include "stdafx.h"
#include <windows.h>
#include <Psapi.h>



size_t get_memory_usage()
{

	DWORD processID = GetCurrentProcessId();
	HANDLE hProcess = OpenProcess(
		PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE, processID);

	if(hProcess == NULL)
	{
		printf("failed to get handle\n");
		return 0;
	}

	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));

	size_t peakWorkingSet = pmc.PeakWorkingSetSize;
	CloseHandle(hProcess);

	return peakWorkingSet;
}