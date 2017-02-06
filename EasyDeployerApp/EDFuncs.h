#pragma once
#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <Windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <atlstr.h> 


class ringo {

public:


	LPTSTR pathConstructor(std::string apppath, std::string command, int type);

	int Packageinstaller(LPTSTR finalpipename, LPTSTR aplicationpath);

	int Easydeployerinstaller(LPTSTR computerName, LPTSTR servicepath, LPTSTR srvname);

	SC_HANDLE scmOpen(LPTSTR rango);

	ENUM_SERVICE_STATUS * enummeration(SC_HANDLE handler);

	SC_HANDLE serviceOpener(SC_HANDLE handler, std::wstring svname);

	SC_HANDLE ServiceCreator(SC_HANDLE scmanager, LPTSTR servicepath);

	VOID __stdcall DoStopSvc(SC_HANDLE svhandler, SC_HANDLE handler);

	VOID __stdcall DoStartSvc(SC_HANDLE svhandler, SC_HANDLE handler);

	VOID __stdcall ringo::DoDeleteSvc(SC_HANDLE handler, std::wstring svname);

	BOOL __stdcall StopDependentServices(SC_HANDLE svhandler, SC_HANDLE handler);

};
