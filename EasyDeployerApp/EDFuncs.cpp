#include "stdafx.h"

#include "EDFuncs.h"

#define RCVBUFFER 50 
// This function create an complete installation path that will be sent to the Packageinstaller function and finaly sent throught pipe .

LPTSTR ringo::pathConstructor(std::string apppath, std::string command, int type) {

	TCHAR   finalpath[5000]; // The final path tha will be returned as TCHAR

	std::string constructed; // The constructed string 

	switch (type) {

	case 1:

		constructed = apppath + " -ms";// getback to this **** constructed = apppath + command;

		_tcscpy_s(finalpath, CA2T(constructed.c_str()));

		break;

	case 2:

		constructed = "msiexec.exe /quiet /i " + apppath + command;

		_tcscpy_s(finalpath, CA2T(constructed.c_str()));

		break;

	default:

		return NULL;

	}

	return finalpath;

}

// This Function install and start the service , that we will communicate with throught pipes.

int ringo::Easydeployerinstaller(LPTSTR computerName, LPTSTR servicepath, LPTSTR srvname) {

	// Get a handle to the scm

	SC_HANDLE scmanagerHandler = ringo::scmOpen(computerName);

	// We create the service

	ringo::ServiceCreator(scmanagerHandler, servicepath);

	SC_HANDLE svhandling = ringo::serviceOpener(scmanagerHandler, srvname);

	ringo::DoStartSvc(svhandling, scmanagerHandler);

	//clean up

	CloseServiceHandle(svhandling);

	CloseServiceHandle(scmanagerHandler);

	return 0;
}

// This function install the package in the remote poste by opening a pipe and sending the constructed path , reading the exit code response  

int ringo::Packageinstaller(LPTSTR finalpipename, LPTSTR aplicationpath) {

	HANDLE hPipe;
	//LPTSTR lpvMessage = TEXT("msiexec.exe /quiet /i \\\\IT-P1\\installations\\chrome.msi");
	//LPTSTR lpvMessage = TEXT("c:\\windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe /c Get-ItemProperty HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* |  Select-Object DisplayName, DisplayVersion| Format-Table \> \\\\IT-P1\\installations\\InstalledPrograms-PS.txt");
	//LPTSTR lpvMessage = TEXT("msiexec.exe /quiet /i \\\\IT-P1\\installations\\chrome.msi");
	TCHAR  chBuf[sizeof(aplicationpath)];
	BOOL   fSuccess = FALSE;
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;
	LPTSTR lpszPipename = finalpipename;



	// Try to open a named pipe; wait for it, if necessary. 

	while (1)
	{
		hPipe = CreateFile(
			lpszPipename,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

							// Break if the pipe handle is valid. 

		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			_tprintf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
			return -1;
		}

		// All pipe instances are busy, so wait for 20 seconds. 

		if (!WaitNamedPipe(lpszPipename, 20000))
		{
			printf("Could not open pipe: 20 second wait timed out.");
			return GetLastError();
		}
	}

	// The pipe connected; change to message-read mode. 

	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if (!fSuccess)
	{
		_tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
		return GetLastError();
	}

	// Send a message to the pipe server. 

	cbToWrite = (lstrlen(aplicationpath) + 1)*sizeof(TCHAR);
	_tprintf(TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, aplicationpath);

	fSuccess = WriteFile(
		hPipe,                  // pipe handle 
		aplicationpath,             // message 
		cbToWrite,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		_tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
		return GetLastError();
	}

	printf("\nMessage sent to server, receiving reply as follows:\n");

	do
	{
		// Read from the pipe. 

		fSuccess = ReadFile(
			hPipe,    // pipe handle 
			chBuf,    // buffer to receive reply 
			RCVBUFFER,  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 

		if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
			break;
		_tprintf(TEXT("Process exited with code %S \n"), chBuf);
	} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

	if (!fSuccess)
	{
		_tprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
		return GetLastError();
	}
	//Log what happened
	logger(lpszPipename, aplicationpath, chBuf);

	//printf("\n<Thread ended click enter>");
	//_getch();
	Sleep(5000);

	CloseHandle(hPipe);
	printf("\n<Thread ended>\n");
	Sleep(5000);
	return 0;

}

// Return an SCM handler

SC_HANDLE ringo::scmOpen(LPTSTR rango) {

	SC_HANDLE handler = OpenSCManager(rango, NULL, SC_MANAGER_ALL_ACCESS);
	if (handler != NULL) {
		std::cout << "HANDLE IS GOOD--> " << handler << std::endl;
	}
	else
	{
		std::cout << "HANDLE IS NOT GOOD-->" << handler << std::endl;
	}

	return handler;
}

//return a service Handler

SC_HANDLE ringo::serviceOpener(SC_HANDLE handler, std::wstring svname) {

	SC_HANDLE svhandle = OpenService(handler, svname.c_str(), SERVICE_ALL_ACCESS);
	if (svhandle != NULL) {
		std::cout << "SERVICE HANDLE IS GOOD --> " << svhandle << std::endl;
	}
	else
	{
		std::cout << "SERVICE HANDLE IS NOT GOOD -->" << svhandle << std::endl;
	}


	return svhandle;
}

// This function is cool :) , it create the service the service in the remote machine , so we can do our deployment job , it's used by the function Easydeployerinstaller.

SC_HANDLE ringo::ServiceCreator(SC_HANDLE scmanager, LPTSTR servicepath) {
	SC_HANDLE creserv = CreateService(scmanager,
		L"easydeployer",
		L"easydeployer",
		SC_MANAGER_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL,
		servicepath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	if (creserv == NULL) {
		printf("problem with the create service handle");

		CloseServiceHandle(scmanager);
		return 0;
	}

	return creserv;
};

// This function stop the services

VOID __stdcall ringo::DoStopSvc(SC_HANDLE svhandler, SC_HANDLE handler)
{
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwStartTime = GetTickCount();
	DWORD dwBytesNeeded;
	DWORD dwTimeout = 30000; // 30-second time-out
	DWORD dwWaitTime;


	// Make sure the service is not already stopped.

	if (!QueryServiceStatusEx(
		svhandler,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&ssp,
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded))
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		goto stop_cleanup;
	}

	if (ssp.dwCurrentState == SERVICE_STOPPED)
	{
		printf("Service is already stopped.\n");
		goto stop_cleanup;
	}

	// If a stop is pending, wait for it.

	while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
	{
		printf("Service stop pending...\n");

		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssp.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(
			svhandler,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded))
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
		{
			printf("Service stopped successfully.\n");
			goto stop_cleanup;
		}

		if (GetTickCount() - dwStartTime > dwTimeout)
		{
			printf("Service stop timed out.\n");
			goto stop_cleanup;
		}
	}

	// If the service is running, dependencies must be stopped first.

	StopDependentServices(svhandler, handler);

	// Send a stop code to the service.

	if (!ControlService(
		svhandler,
		SERVICE_CONTROL_STOP,
		(LPSERVICE_STATUS)&ssp))
	{
		printf("ControlService failed (%d)\n", GetLastError());
		goto stop_cleanup;
	}

	// Wait for the service to stop.

	while (ssp.dwCurrentState != SERVICE_STOPPED)
	{
		Sleep(ssp.dwWaitHint);
		if (!QueryServiceStatusEx(
			svhandler,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssp,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded))
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			goto stop_cleanup;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
			break;

		if (GetTickCount() - dwStartTime > dwTimeout)
		{
			printf("Wait timed out\n");
			goto stop_cleanup;
		}
	}
	printf("Service stopped successfully\n");

stop_cleanup:
	CloseServiceHandle(svhandler);
	CloseServiceHandle(handler);
}

// This function stop the dependent services if there is any , it's used by DoStopSvc

BOOL __stdcall ringo::StopDependentServices(SC_HANDLE svhandler, SC_HANDLE handler)
{
	DWORD i;
	DWORD dwBytesNeeded;
	DWORD dwCount;

	LPENUM_SERVICE_STATUS   lpDependencies = NULL;
	ENUM_SERVICE_STATUS     ess;
	SC_HANDLE               hDepService;
	SERVICE_STATUS_PROCESS  ssp;

	DWORD dwStartTime = GetTickCount();
	DWORD dwTimeout = 30000; // 30-second time-out

							 // Pass a zero-length buffer to get the required buffer size.
	if (EnumDependentServices(svhandler, SERVICE_ACTIVE,
		lpDependencies, 0, &dwBytesNeeded, &dwCount))
	{
		// If the Enum call succeeds, then there are no dependent
		// services, so do nothing.
		return TRUE;
	}
	else
	{
		if (GetLastError() != ERROR_MORE_DATA)
			return FALSE; // Unexpected error

						  // Allocate a buffer for the dependencies.
		lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(
			GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);

		if (!lpDependencies)
			return FALSE;

		__try {
			// Enumerate the dependencies.
			if (!EnumDependentServices(svhandler, SERVICE_ACTIVE,
				lpDependencies, dwBytesNeeded, &dwBytesNeeded,
				&dwCount))
				return FALSE;

			for (i = 0; i < dwCount; i++)
			{
				ess = *(lpDependencies + i);
				// Open the service.
				hDepService = OpenService(handler,
					ess.lpServiceName,
					SERVICE_STOP | SERVICE_QUERY_STATUS);

				if (!hDepService)
					return FALSE;

				__try {
					// Send a stop code.
					if (!ControlService(hDepService,
						SERVICE_CONTROL_STOP,
						(LPSERVICE_STATUS)&ssp))
						return FALSE;

					// Wait for the service to stop.
					while (ssp.dwCurrentState != SERVICE_STOPPED)
					{
						Sleep(ssp.dwWaitHint);
						if (!QueryServiceStatusEx(
							hDepService,
							SC_STATUS_PROCESS_INFO,
							(LPBYTE)&ssp,
							sizeof(SERVICE_STATUS_PROCESS),
							&dwBytesNeeded))
							return FALSE;

						if (ssp.dwCurrentState == SERVICE_STOPPED)
							break;

						if (GetTickCount() - dwStartTime > dwTimeout)
							return FALSE;
					}
				}
				__finally
				{
					// Always release the service handle.
					CloseServiceHandle(hDepService);
				}
			}
		}
		__finally
		{
			// Always free the enumeration buffer.
			HeapFree(GetProcessHeap(), 0, lpDependencies);
		}
	}
	return TRUE;
}

// This function start services

VOID __stdcall ringo::DoStartSvc(SC_HANDLE svhandler, SC_HANDLE handler)
{
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwOldCheckPoint;
	DWORD dwStartTickCount;
	DWORD dwWaitTime;
	DWORD dwBytesNeeded;

	// Check the status in case the service is not stopped. 

	if (!QueryServiceStatusEx(
		svhandler,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // information level
		(LPBYTE)&ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded))              // size needed if buffer is too small
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(svhandler);
		CloseServiceHandle(handler);
		return;
	}

	// Check if the service is already running. It would be possible 
	// to stop the service here, but for simplicity this example just returns. 

	if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		printf("Cannot start the service because it is already running\n");
		CloseServiceHandle(svhandler);
		CloseServiceHandle(handler);
		return;
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	// Wait for the service to stop before attempting to start it.

	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status until the service is no longer stop pending. 

		if (!QueryServiceStatusEx(
			svhandler,                     // handle to service 
			SC_STATUS_PROCESS_INFO,         // information level
			(LPBYTE)&ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded))              // size needed if buffer is too small
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			CloseServiceHandle(svhandler);
			CloseServiceHandle(handler);
			return;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				printf("Timeout waiting for service to stop\n");
				CloseServiceHandle(svhandler);
				CloseServiceHandle(handler);
				return;
			}
		}
	}

	// Attempt to start the service.

	if (!StartService(
		svhandler,  // handle to service 
		0,           // number of arguments 
		NULL))      // no arguments 
	{
		printf("StartService failed (%d)\n", GetLastError());
		CloseServiceHandle(svhandler);
		CloseServiceHandle(handler);
		return;
	}
	else printf("Service start pending...\n");

	// Check the status until the service is no longer start pending. 

	if (!QueryServiceStatusEx(
		svhandler,                     // handle to service 
		SC_STATUS_PROCESS_INFO,         // info level
		(LPBYTE)&ssStatus,             // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded))              // if buffer too small
	{
		printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
		CloseServiceHandle(svhandler);
		CloseServiceHandle(handler);
		return;
	}

	// Save the tick count and initial checkpoint.

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth the wait hint, but no less than 1 second and no 
		// more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		// Check the status again. 

		if (!QueryServiceStatusEx(
			svhandler,             // handle to service 
			SC_STATUS_PROCESS_INFO, // info level
			(LPBYTE)&ssStatus,             // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded))              // if buffer too small
		{
			printf("QueryServiceStatusEx failed (%d)\n", GetLastError());
			break;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// Continue to wait and check.

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				// No progress made within the wait hint.
				break;
			}
		}
	}

	// Determine whether the service is running.

	if (ssStatus.dwCurrentState == SERVICE_RUNNING)
	{
		printf("Service started successfully.\n");
	}
	else
	{
		printf("Service not started. \n");
		printf("  Current State: %d\n", ssStatus.dwCurrentState);
		printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
		printf("  Check Point: %d\n", ssStatus.dwCheckPoint);
		printf("  Wait Hint: %d\n", ssStatus.dwWaitHint);
	}
	/*
	CloseServiceHandle(svhandler);
	CloseServiceHandle(handler);
	*/
}

VOID __stdcall ringo::DoDeleteSvc(SC_HANDLE handler, std::wstring svname)
{
	SC_HANDLE schService;
	SERVICE_STATUS ssStatus;


	// Get a handle to the service.

	schService = OpenService(
		handler,       // SCM database 
		svname.c_str(),          // name of service 
		DELETE);            // need delete access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(handler);
		return;
	}

	// Delete the service.

	if (!DeleteService(schService))
	{
		printf("DeleteService failed (%d)\n", GetLastError());
	}
	else printf("Service deleted successfully\n");

	CloseServiceHandle(schService);
}


//This Function Ennumerate services

ENUM_SERVICE_STATUS * ringo::enummeration(SC_HANDLE handler) {
	DWORD pcbBytesNeeded = 0, lpServicesReturned = 0, ResumeHandle = 0;
	ENUM_SERVICE_STATUS * lpservices = NULL;


	BOOL miration = EnumServicesStatus(handler, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &pcbBytesNeeded, &lpServicesReturned, &ResumeHandle);


	if (!miration) {
		DWORD dw = GetLastError();
		switch (dw) {
		case ERROR_MORE_DATA: {
			DWORD needed = pcbBytesNeeded;
			lpservices = new ENUM_SERVICE_STATUS[needed];
			miration = EnumServicesStatus(handler, SERVICE_WIN32, SERVICE_STATE_ALL, lpservices, needed, &pcbBytesNeeded, &lpServicesReturned, &ResumeHandle);
			for (int i = 0; i < lpServicesReturned; i++) {
				printf("service name : %S \n", lpservices[i].lpServiceName);
			}
			std::cout << "Services number : " << lpServicesReturned << std::endl;
			return lpservices;
			break;
		}
		case ERROR_ACCESS_DENIED:

			printf("ERROR_ACCESS_DENIED\n");
			CloseServiceHandle(handler);
			break;
		case ERROR_INVALID_HANDLE:

			printf("ERROR_INVALID_HANDLE\n");
			CloseServiceHandle(handler);
			break;
		case ERROR_INVALID_PARAMETER:

			printf("ERROR_INVALID_PARAMETER\n");
			CloseServiceHandle(handler);
			break;
		case ERROR_INSUFFICIENT_BUFFER:

			printf("ERROR_INSUFFICIENT_BUFFER");
			CloseServiceHandle(handler);
			break;

		default:
			//printf("SOMETHING ELSE %S ",GetLastError());
			std::cout << "\n" << GetLastError() << std::endl;
			CloseServiceHandle(handler);
			break;
		}
	}

}

void ringo::logger(LPTSTR pipenamepath, LPTSTR finalpath, TCHAR * returncode) {

	std::mutex mtx;

	mtx.lock();

	std::wstring pipen = pipenamepath;

	std::wstring finalp = finalpath;

	time_t t = time(0);

	tm* localtm = localtime(&t);

	std::wofstream myfile;

	myfile.open("Easylogging.txt", std::ios::app);

	myfile << pipen << ";" << finalp << ";" << *returncode << ";" << asctime(localtm) <<"\n";

	mtx.unlock();

	return;

}