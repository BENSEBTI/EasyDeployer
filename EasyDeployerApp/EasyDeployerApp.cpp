// EasyDeployerApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "EDFuncs.h"

SC_HANDLE svhandling;
SC_HANDLE scmanagerHandler;
HANDLE  hThreadArray[12];
DWORD threadid;
ringo Helloo;
ENUM_SERVICE_STATUS * lpservicing = NULL;
int choice = 0;
std::string computerName;
std::string servicelocation;
std::string servicename;
std::string servicepath;
std::string networkaccess = "\\\\";
std::string pipe = "\\pipe\\easydeployer";
std::string apppath;
std::string command = "";  
std::string pipename;
std::string line;
std::ifstream computers;
std::string computerfilepath;


int installtype;
LPTSTR finalpath;


TCHAR finalname[sizeof(computerName)];
TCHAR ServiceLocation[500];
TCHAR finalSvcname[sizeof(servicename)];
TCHAR finalapppath[5000];
TCHAR finalpipename[500];

//#define PATHING L"\\\\*\\installations\\easydeployer.exe"
//#define PATHING L"\\\\*\\install\\easydeployer.exe"
#define SRVNAME L"easydeployer"
//#define	APPPATH L"c:\\windows\\system32\\WindowsPowerShell\\v1.0\\powershell.exe /c Get-ItemProperty HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\* |  Select-Object DisplayName, DisplayVersion| Format-Table \> \\\\*\\installations\\InstalledPrograms-PS.txt"
#define BUFSIZE 5000

DWORD WINAPI InstallThread(LPVOID params);

int main() {

	std::unique_ptr<std::vector<std::string>> CompArray (new std::vector<std::string>);

	while (1) {

		std::cout << "\n \n  What do you want to do \n \n";

		std::cout << "1. INSTALL EASYDEPLOYER SERVICE AND START IT \n";

		std::cout << "2. ENNUMERATE SERVICES \n";

		std::cout << "3. DELETE A SERVICE \n";

		std::cout << "4. DEPLOY A SOFTWARE \n";

		std::cout << "5. START A SERVICE \n";

		std::cout << "6. STOP A SERVICE \n";

		std::cin >> choice;


		switch (choice) {

		case 1:

			std::cout << "The computer name" << std::endl;

			std::cin >> computerName;

			std::cout << "The Service location" << std::endl;

			std::cin >> servicelocation;

			//std::getline(std::cin >> std::ws, servicelocation);

			_tcscpy_s(finalname, CA2T(computerName.c_str()));
			_tcscpy_s(ServiceLocation, CA2T(servicelocation.c_str()));

			Helloo.Easydeployerinstaller(finalname, ServiceLocation, SRVNAME);

			break;

		case 2:

			std::cout << "The computer name" << std::endl;

			std::cin >> computerName;

			_tcscpy_s(finalname, CA2T(computerName.c_str()));

			// Get a handle to the scm

			scmanagerHandler = Helloo.scmOpen(finalname);

			Helloo.enummeration(scmanagerHandler);

			//clean up 

			CloseServiceHandle(scmanagerHandler);

			break;

		case 3:

			std::cout << "The computer name" << std::endl;

			std::cin >> computerName;

			_tcscpy_s(finalname, CA2T(computerName.c_str()));

			std::cout << "The Service name" << std::endl;

			std::getline(std::cin >> std::ws, servicename);

			_tcscpy_s(finalSvcname, CA2T(servicename.c_str()));

			// Get a handle to the scm

			scmanagerHandler = Helloo.scmOpen(finalname);

			Helloo.DoDeleteSvc(scmanagerHandler, finalSvcname);

			CloseServiceHandle(scmanagerHandler);

			break;

		case 4:

			std::cout << "The computers file path" << std::endl;

			std::cin >> computerfilepath;

			std::cout << "Application Path" << std::endl;

			std::getline(std::cin >> std::ws, apppath);

			std::cout << "Is it An \n1-EXE or \n2-MSI" << std::endl;

			std::cin >> installtype;
			
			computers.open(computerfilepath);

			if (computers.is_open()) {
				while (std::getline(computers, line)) {

					(*CompArray).push_back(line);

				}
			}
			else {
				printf("Couldn't open file with error %d", GetLastError());
				return 5;
			}

			for (std::size_t i = 0; i < (*CompArray).size(); i++) {

				hThreadArray[i] = CreateThread(NULL,
					0,
					InstallThread,
					&(*CompArray)[i],
					0,
					&threadid);

				if (hThreadArray[i] == NULL) {

					printf("Failed to create thread %d", GetLastError());

					return 15;

				}
				//Sleep(3000);
			}

			WaitForMultipleObjects((*CompArray).size(), hThreadArray, TRUE, INFINITE);
			return 12;

			break;
			/*
			pipename = networkaccess + computerName + pipe;

			_tcscpy_s(finalpipename, CA2T(pipename.c_str()));

			finalpath = Helloo.pathConstructor(apppath, command, installtype);



			if (finalpath == NULL) {

				printf("something is wrong");

				return 1;
			}

			Helloo.Packageinstaller(finalpipename, finalpath);

			break;
			*/
		case 5:

			std::cout << "The computer name" << std::endl;

			std::cin >> computerName;

			std::cout << "The service name" << std::endl;

			std::cin >> servicename;

			_tcscpy_s(finalname, CA2T(computerName.c_str()));

			_tcscpy_s(finalSvcname, CA2T(servicename.c_str()));

			// Get a handle to the scm

			scmanagerHandler = Helloo.scmOpen(finalname);

			svhandling = Helloo.serviceOpener(scmanagerHandler, finalSvcname);

			Helloo.DoStartSvc(svhandling, scmanagerHandler);

			//clean up

			CloseServiceHandle(svhandling);

			CloseServiceHandle(scmanagerHandler);

			break;

		case 6:
			std::cout << "The computer name" << std::endl;

			std::cin >> computerName;

			std::cout << "The service name" << std::endl;

			std::cin >> servicename;

			_tcscpy_s(finalname, CA2T(computerName.c_str()));

			_tcscpy_s(finalSvcname, CA2T(servicename.c_str()));

			// Get a handle to the scm

			scmanagerHandler = Helloo.scmOpen(finalname);

			// Service Handle

			svhandling = Helloo.serviceOpener(scmanagerHandler, finalSvcname);

			//Paasing all SCM handle and the Service Handle that we want to stop to DoStopSvc

			Helloo.DoStopSvc(svhandling, scmanagerHandler);

			// Clean up

			CloseServiceHandle(svhandling);

			CloseServiceHandle(scmanagerHandler);
			
			break;

		default:

			std::cout << " I didn't understand your request. \n" << std::endl;
			break;
		}


	}

	return 0;
}

    
DWORD WINAPI InstallThread(LPVOID params) {

	
	std::string * compuname = (std::string*) params;

	pipename = networkaccess + *compuname + pipe;

	//std::cout << pipename << std::endl;

	_tcscpy_s(finalpipename, CA2T(pipename.c_str()));

	finalpath = Helloo.pathConstructor(apppath, command, installtype);

	Helloo.Packageinstaller(finalpipename, finalpath);

	
	return 45 ;

};