// EasyDeployer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <tchar.h>
#include <strsafe.h>

#pragma comment(lib, "advapi32.lib")

SERVICE_STATUS servicestatus ;
SERVICE_STATUS_HANDLE servicehandle  ;
HANDLE mainthread;


BOOL setstat = 0;
HANDLE stopevent;
VOID WINAPI EasyServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceControlHandler(DWORD);
DWORD WINAPI mainservicethread(LPVOID params);
DWORD WINAPI communicationpoolthread(LPVOID params);
DWORD WINAPI communicationprocthread(LPVOID params);
VOID RequestTreating(LPTSTR, LPTSTR, LPDWORD);
VOID ServiceReportEvent(LPTSTR);

#define SERVICE_NAME  _T("easydeployer2")    
#define BUFSIZE 5000  // Used with the pipe for the communication 
#define SVC_ERROR 9998






// Service Entry point
VOID _tmain(int argc, TCHAR *argv[])
{
	SERVICE_TABLE_ENTRY servicetable[] = { 
		{SERVICE_NAME,EasyServiceMain },
		{NULL,NULL}
	};



	if (StartServiceCtrlDispatcher(servicetable)== FALSE) {
		ServiceReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}

}

// Main Service

VOID WINAPI EasyServiceMain(DWORD argc ,LPTSTR *argv) {

	DWORD threadid = 0;

	servicehandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceControlHandler);

				if (!servicehandle) {
					ServiceReportEvent(TEXT("RegisterServiceCtrlHandler"));
				}
	// Inform the SCM that we are starting

				servicestatus.dwServiceType = SERVICE_WIN32;
				servicestatus.dwCurrentState = SERVICE_START_PENDING;
				servicestatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
				servicestatus.dwWin32ExitCode = 0;
				servicestatus.dwServiceSpecificExitCode = 0;
				servicestatus.dwCheckPoint = 0;
				servicestatus.dwWaitHint = 0;

			setstat =SetServiceStatus(servicehandle, &servicestatus);

				if(!setstat){
					ServiceReportEvent(TEXT("Failed to set service status to SERVICE_START_PENDING"));
					CloseHandle(servicehandle); //************* NEED TO REVIEW THIS ***************
				}

	// Create an event

				stopevent = CreateEvent(NULL, TRUE, FALSE, NULL);

				// If something goes wrong with the event we stop
				if (stopevent == NULL) {
					ServiceReportEvent(TEXT("CreateEvent"));

					servicestatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
					servicestatus.dwCurrentState = SERVICE_STOPPED;
					servicestatus.dwControlsAccepted = 0;
					servicestatus.dwWin32ExitCode = 0;
					servicestatus.dwWaitHint = 0;

					setstat = SetServiceStatus(servicehandle, &servicestatus); // We are stopped

					return ;
				}

	// Things looks good ,so we create out main thread pool 
				mainthread = CreateThread(NULL,
					0,
					mainservicethread,
					NULL,
					NULL,
					&threadid);

				

				CloseHandle(mainthread); //NEED TO REVIEW *************

				CloseHandle(stopevent); //NEED TO REVIEW ****************
				return;

}


DWORD WINAPI mainservicethread(LPVOID params) {


	servicestatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	servicestatus.dwCurrentState = SERVICE_RUNNING;
	servicestatus.dwWin32ExitCode = 0;
	servicestatus.dwServiceSpecificExitCode = 0;

	setstat = SetServiceStatus(servicehandle, &servicestatus);
	DWORD servicethreadid;

	HANDLE MainSrvHandle;

	MainSrvHandle = CreateThread(NULL, 0, communicationpoolthread, NULL, NULL, &servicethreadid);

	while (WaitForSingleObject(stopevent, 10) != WAIT_OBJECT_0) {

	}

	servicestatus.dwControlsAccepted = 0;
	servicestatus.dwCurrentState = SERVICE_STOPPED;
	servicestatus.dwWin32ExitCode = 0;
	servicestatus.dwServiceSpecificExitCode = 0;

	setstat = SetServiceStatus(servicehandle, &servicestatus);

	CloseHandle(MainSrvHandle);

	return 0;

}





DWORD WINAPI communicationpoolthread(LPVOID params) {

	BOOL fconnected;
	HANDLE hpipe = NULL;
	HANDLE pipethread;
	DWORD pipethreadid;
	LPTSTR pipename = TEXT("\\\\.\\pipe\\easydeployer");

	for (;;) {

		hpipe = CreateNamedPipe(pipename,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE |
			PIPE_READMODE_MESSAGE |
			PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			BUFSIZE,
			BUFSIZE,
			0,
			NULL);

		fconnected = ConnectNamedPipe(hpipe, 0);

		if (fconnected) {

			pipethread = CreateThread(NULL,
				0,
				communicationprocthread,
				(LPVOID)hpipe,
				NULL,
				&pipethreadid);



			if (pipethread == NULL)
			{

				ServiceReportEvent(TEXT("ConnectNamedPipe not connected"));

				return -1;
			}
			else
			{
				CloseHandle(pipethread);
			}
		}
		else {
			CloseHandle(hpipe);
		}
	}
}

DWORD WINAPI communicationprocthread(LPVOID param) {
	//allocate memory for us 
	HANDLE heap = GetProcessHeap(); 
	TCHAR * receivedrequest = (TCHAR*)HeapAlloc(heap, 0, BUFSIZE*sizeof(TCHAR));
	TCHAR * sentreply = (TCHAR*)HeapAlloc(heap, 0, BUFSIZE*sizeof(TCHAR));



	DWORD bytesreplyed = 0, bytesread = 0, written = 0;


	HANDLE pipehandle = NULL;
	BOOL success =FALSE;
	pipehandle = param;

	// Read client request until done 
	while (1) {

		success = ReadFile(pipehandle,
			receivedrequest,
			BUFSIZE*sizeof(TCHAR),
			&bytesread,
			NULL);
		

			if (!success) {
				ServiceReportEvent(TEXT("ReadFile"));
				break;
			}

			
			// Process the incoming request
			RequestTreating(receivedrequest, sentreply, &bytesreplyed);
			
		// Reply to the client the exit code 

			success = WriteFile(pipehandle, sentreply, bytesreplyed, &written, NULL);

			if (!success || bytesread == 0) {
				ServiceReportEvent(TEXT("WriteFile"));
				break;
			}

	}

	FlushFileBuffers(pipehandle);
	DisconnectNamedPipe(pipehandle);
	CloseHandle(pipehandle);

	HeapFree(heap, 0, receivedrequest);
	HeapFree(heap, 0, sentreply);

	return 1;

};






VOID WINAPI ServiceControlHandler(DWORD ctrlcode) {

	switch (ctrlcode) {
	case SERVICE_CONTROL_STOP:
		servicestatus.dwWaitHint = 0;
		servicestatus.dwCurrentState = SERVICE_STOPPED;
		servicestatus.dwCheckPoint = 0;
		servicestatus.dwWin32ExitCode = 0;
		

		setstat = SetServiceStatus(servicehandle, &servicestatus);
		SetEvent(stopevent);
		break;

	case SERVICE_CONTROL_INTERROGATE:
		break;
	default :
		break;
	}

};

VOID RequestTreating(LPTSTR receivedrequest, LPTSTR sentreply, LPDWORD bytesreplyed) {


	BOOL process = FALSE;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD exit_code; // Process exit code that will be sent to client .
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	process = CreateProcess(NULL,
		receivedrequest,
		NULL,
		NULL,
		TRUE,
		NULL,
		NULL,
		NULL,
		&si,
		&pi);
	if (!process) {
		ServiceReportEvent(TEXT("CreateProcess"));
		return;
	}
	// We will wait for the process to terminate 
	WaitForSingleObject(pi.hProcess, INFINITE);

	//We get the Process exit Code
	GetExitCodeProcess(pi.hProcess, &exit_code);

	wchar_t  wcsSomeStr[64];

	_itow(exit_code, wcsSomeStr, 10);
	StringCchCopy(sentreply, BUFSIZE, wcsSomeStr);
	*bytesreplyed = (lstrlen(sentreply) + 1)*sizeof(TCHAR);

	//We clean up and go home :)
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);


}


VOID ServiceReportEvent(LPTSTR methode) {
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SERVICE_NAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), methode, GetLastError());

		lpszStrings[0] = SERVICE_NAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}

