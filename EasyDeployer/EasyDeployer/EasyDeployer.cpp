// EasyDeployer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <tchar.h>
#include <strsafe.h>

// All this belong to the service 
SERVICE_STATUS servicestatus ;
SERVICE_STATUS_HANDLE servicehandle ;
HANDLE mainthread;


BOOL setstat = 0;
HANDLE stopevent;
VOID WINAPI EasyServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceControlHandler(DWORD);
DWORD WINAPI mainservicethread(LPVOID params);
DWORD WINAPI communicationpoolthread();
DWORD WINAPI communicationprocthread(LPVOID params);
VOID ServiceReportEvent(LPTSTR);

#define SERVICE_NAME L"easydeployer"
#define BUFSIZE 5000  // Used with the pipe for the communication 


// End of service  





// Service Entry point
VOID _tmain(int argc , TCHAR *argv)
{
	SERVICE_TABLE_ENTRY servicetable[] = { {SERVICE_NAME,EasyServiceMain }, {NULL,NULL} };

	BOOL ServiceDispatcher;

	ServiceDispatcher = StartServiceCtrlDispatcher(servicetable);

	if (!ServiceDispatcher) {
		ServiceReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}

}

// Main Service

VOID WINAPI EasyServiceMain(DWORD argc ,LPTSTR *argv[]) {

	DWORD threadid = 0;

	servicehandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceControlHandler);

				if (!servicehandle) {
					ServiceReportEvent(TEXT("RegisterServiceCtrlHandler"));
				}
	// Inform the SCM that we are starting

				servicestatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
				servicestatus.dwCurrentState = SERVICE_START_PENDING;
				servicestatus.dwControlsAccepted = 0;
				servicestatus.dwWin32ExitCode = 0;
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

		if (!fconnected) {
			ServiceReportEvent(TEXT("ConnectNamedPipe"));
			CloseHandle(hpipe);
			return 1;


			pipethread = CreateThread(NULL,
				0,
				communicationprocthread,
				hpipe,
				NULL,
				&pipethreadid);



			if (pipethread == NULL)
			{

				ServiceReportEvent(TEXT("ConnectNamedPipe"));

				return -1;
			}
			else
			{
				CloseHandle(pipethread);
			}

		}
		else { CloseHandle(hpipe); }
	}
}

DWORD WINAPI communicationprocthread(LPVOID params) {



};

VOID WINAPI ServiceControlHandler(DWORD ctrlcode) {

};
