//  ETViewer, an easy to use ETW / WPP trace viewer
//  Copyright (C) 2011  Javier Martin Garcia (javiermartingarcia@gmail.com)
//  
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
////////////////////////////////////////////////////////////////////////////////////////
//
// Project HomePage: http://etviewer.codeplex.com/
// For any comment or question, mail to: etviewer@gmail.com
//
////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include  <io.h>
#include  <stdio.h>
#include  <stdlib.h>
#include "ETViewer.h"
#include "MainFrm.h"

#include "ETViewerDoc.h"
#include "ETViewerView.h"
#include "ProviderTree.h"
#include ".\etviewer.h"
#include "FileMonitor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ET_VIEWER_NAME "ETViewer"

extern DWORD g_dwRegisteredMessage;

// CETViewerApp

BEGIN_MESSAGE_MAP(CETViewerApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Comandos de documento estándar basados en archivo
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_COMMAND_RANGE(RECENT_PDB_FILE_BASE_INDEX,RECENT_PDB_FILE_BASE_INDEX+RECENT_PDB_FILE_MAX,OnRecentPDBFile)
	ON_COMMAND_RANGE(RECENT_SOURCE_FILE_BASE_INDEX,RECENT_SOURCE_FILE_BASE_INDEX+RECENT_SOURCE_FILE_MAX,OnRecentSourceFile)
	ON_COMMAND_RANGE(RECENT_LOG_FILE_BASE_INDEX,RECENT_LOG_FILE_BASE_INDEX+RECENT_LOG_FILE_MAX,OnRecentLogFile)
END_MESSAGE_MAP()


// Construcción de CETViewerApp

CETViewerApp::CETViewerApp()
{	
	char tempDrive[MAX_PATH]={0},tempPath[MAX_PATH]={0};
	char currentModule[MAX_PATH]={0};
	GetModuleFileName(NULL,currentModule,sizeof(currentModule));
	_splitpath(currentModule,tempDrive,tempPath,NULL,NULL);

	m_sConfigFile=tempDrive;
	m_sConfigFile+=tempPath;
	m_sConfigFile+="\\ETViewer.cfg";

	m_bCheckingFileChangeOperations=false;
	m_hPendingFileChangesMutex=CreateMutex(NULL,FALSE,NULL);
	m_hInstantTraceMutex=CreateMutex(NULL,FALSE,NULL);
	m_InstantIncludeFilter="*";
	m_eSourceMonitoringMode=eFileMonitoringMode_AutoReload;
	m_ePDBMonitoringMode=eFileMonitoringMode_AutoReload;

	m_ConfigFile.Open(m_sConfigFile.c_str());

	m_pFileMonitor=new CFileMonitor(this);

	LoadFrom(&m_ConfigFile,"Application");
	UpdateHighLightFilters();
	UpdateInstantFilters();

}

CETViewerApp::~CETViewerApp()
{
	set<CTraceProvider *>::iterator i;
	for(i=m_sProviders.begin();i!=m_sProviders.end();i++)
	{
		CTraceProvider *pProvider=*i;
		delete pProvider;
	}
	m_sProviders.clear();
	if(m_hInstantTraceMutex){CloseHandle(m_hInstantTraceMutex);m_hInstantTraceMutex=NULL;}
	if(m_hPendingFileChangesMutex){CloseHandle(m_hPendingFileChangesMutex);m_hPendingFileChangesMutex=NULL;}

	m_pFileMonitor->Stop();
	delete m_pFileMonitor;
	m_pFileMonitor=NULL;
}

// El único objeto CETViewerApp

CETViewerApp theApp;

// Inicialización de CETViewerApp

BOOL CETViewerApp::InitInstance()
{
	CoInitialize(NULL);

	bool bFirstInstance=FALSE;
	DWORD dwLastError=ERROR_SUCCESS;
	m_hSingleInstanceMutex=CreateMutex(NULL,TRUE,"ETViewerSingleInstanceMutex");
	if(m_hSingleInstanceMutex)
	{
		dwLastError=GetLastError();
		if(dwLastError!=ERROR_ALREADY_EXISTS)
		{
			bFirstInstance=TRUE;
		}
	}

	if(!bFirstInstance)
	{
		for(int x=0;x<__argc;x++)
		{
			if(__argv[x][0]=='/' || __argv[x][0]=='-')
			{
				ATOM hAtom=GlobalAddAtom(__argv[x]);
				if(hAtom)
				{
					::PostMessage(HWND_BROADCAST,g_dwRegisteredMessage,hAtom,0);
					GlobalDeleteAtom(hAtom);
				}
			}
		}
		return FALSE;
	}

	// Windows XP requiere InitCommonControls() si un manifiesto de 
	// aplicación especifica el uso de ComCtl32.dll versión 6 o posterior para habilitar
	// estilos visuales. De lo contrario, se generará un error al crear ventanas.
	InitCommonControls();

	CWinApp::InitInstance();

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CETViewerDoc),
		RUNTIME_CLASS(CMainFrame),       // Ventana de marco MDI principal
		RUNTIME_CLASS(CProviderTree));
	if (!pDocTemplate)
		return FALSE;

	AddDocTemplate(pDocTemplate);

	this->m_nCmdShow = SW_HIDE;

	// Analizar línea de comandos para comandos Shell estándar, DDE, Archivo Abrir
	CCommandLineInfo cmdInfo;
	//ParseCommandLine(cmdInfo);
	// Enviar comandos especificados en la línea de comandos. Devolverá FALSE si
	// la aplicación se inició con los modificadores /RegServer, /Register, /Unregserver o /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	m_SourceFileContainer.Create(IDD_SOURCE_FILE_CONTAINER,m_pFrame);

	// Se ha inicializado la única ventana; mostrarla y actualizarla
	m_pFrame=dynamic_cast<CMainFrame *>(m_pMainWnd);
	// Llamar a DragAcceptFiles sólo si existe un sufijo
	//  En una aplicación SDI, esto debe ocurrir después de ProcessShellCommand

	m_SourceFileContainer.SetWindowPos(&CWnd::wndNoTopMost,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);

	RefreshRecentFilesMenus();
	ProcessSessionChange();
	BOOL bContinue=ProcessCommandLine(__argc,__argv);
	if(bContinue)
	{
		m_pMainWnd->ShowWindow(SW_SHOW);
		m_pMainWnd->UpdateWindow();
		m_pFileMonitor->Start();
	}
	return bContinue;
}

void CETViewerApp::SetFileAssociation(char *pExtension,char *pFileTypeName,char *pFileDescription,char *pCommandLineTag)
{
	bool bAlreadyPresent=false;
	HKEY hKey=NULL;
	DWORD dwError=RegOpenKey(HKEY_CLASSES_ROOT,pExtension,&hKey);
	if(dwError==ERROR_SUCCESS)
	{
		long dwSize=MAX_PATH;
		char  sValue[MAX_PATH]={0};
		dwError=RegQueryValue(hKey,NULL,sValue,&dwSize);
		if(dwError==ERROR_SUCCESS && strcmp(sValue,pFileTypeName)==0)
		{
			bAlreadyPresent=true;
		}
		RegCloseKey(hKey);
		hKey=NULL;
	}
	if(!bAlreadyPresent)
	{
		dwError=RegCreateKey(HKEY_CLASSES_ROOT,pExtension,&hKey);
		if(dwError==ERROR_SUCCESS)
		{
			dwError=RegSetValue(hKey,NULL,REG_SZ,pFileTypeName,strlen(pFileTypeName)+1);
		}
		RegCloseKey(hKey);
		hKey=NULL;
	}
	dwError=RegOpenKey(HKEY_CLASSES_ROOT,pFileTypeName,&hKey);
	if(hKey){RegCloseKey(hKey);hKey=NULL;}

	dwError=RegCreateKey(HKEY_CLASSES_ROOT,pFileTypeName,&hKey);
	if(dwError==ERROR_SUCCESS)
	{
		dwError=RegSetValue(hKey,NULL,REG_SZ,pFileDescription,strlen(pFileDescription)+1);
	}
	if(hKey){RegCloseKey(hKey);hKey=NULL;}

	if(dwError==ERROR_SUCCESS)
	{
		string sKeyName=pFileTypeName;
		sKeyName+="\\shell\\open\\command";
		dwError=RegCreateKey(HKEY_CLASSES_ROOT,sKeyName.c_str(),&hKey);
		if(dwError==ERROR_SUCCESS)
		{
			char pModule[MAX_PATH]={0};
			char pCommand[MAX_PATH]={0};
			GetModuleFileName(NULL,pModule,MAX_PATH);
			sprintf(pCommand,"\"%s\" %s",pModule,pCommandLineTag);
			dwError=RegSetValue(hKey,NULL,REG_SZ,pCommand,strlen(pCommand)+1);
		}
		if(hKey){RegCloseKey(hKey);hKey=NULL;}
	}
}

BOOL CETViewerApp::ProcessCommandLine(int argc,char **argv)
{
	unsigned x=0;
	bool bPDBSpecified=false,bSilent=false,bSourceSpecified=false;
	bool bLevelSpecified=false,bValidLevel=true,bFailedToOpenETL=false;
	DWORD dwLevel=TRACE_LEVEL_VERBOSE;
	string sETLFile;

	vector<string> vPDBsToLoad;
	vector<string> vSourcesToLoad;

	for(x=0;x<(ULONG)argc;x++)
	{
		char *sTemp=new char [strlen(argv[x])+1];
		strcpy(sTemp,argv[x]);
		strupr(sTemp);
		if(	strcmp(sTemp,"-V")==0 || 
			strcmp(sTemp,"/V")==0)
		{
			MessageBox(NULL,"ETViewer v0.9\n","ETViewer",MB_OK);
			return FALSE;
		}
		else if(strcmp(sTemp,"-H")==0 || 
			    strcmp(sTemp,"/H")==0)
		{
			MessageBox(NULL,"ETViewer v0.9\n\nCommand line options:\n\n  -v   Show version\n  -h   Show this help\n  -s   Silent: Do not warn about statup errors\n  -pdb:<file filter>  PDB files to load, for example -pdb:C:\\Sample\\*.pdb\n  -src:<file filter>   Source files to load, for example -src:C:\\Sample\\*.cpp\n  -etl:<file> Load the specified .etl file on startup\n  -l:<level> Initial tracing level:\n     FATAL\n     ERROR\n     WARNING\n     INFORMATION\n     VERBOSE\n     RESERVED6\n     RESERVED7\n     RESERVED8\n     RESERVED9","ETViewer",MB_OK);
			return FALSE;
		}
		else if(strncmp(sTemp,"-PDB:",strlen("-PDB:"))==0 || 
			    strncmp(sTemp,"/PDB:",strlen("/PDB:"))==0)
		{
			char drive[MAX_PATH]={0};
			char path[MAX_PATH]={0};
			char *pPDB=argv[x]+strlen("-PDB:");

			bPDBSpecified=true;

			_splitpath(pPDB,drive,path,NULL,NULL);

			WIN32_FIND_DATA findData={0};
			HANDLE hFind=FindFirstFile(pPDB,&findData);
			if(hFind!=INVALID_HANDLE_VALUE)
			{
				do
				{
					char tempFile[MAX_PATH]={0};
					strcpy(tempFile,drive);
					strcat(tempFile,path);
					strcat(tempFile,findData.cFileName);

					vPDBsToLoad.push_back(tempFile);
				}
				while(FindNextFile(hFind,&findData));
				FindClose(hFind);
			}
		}
		else if(strncmp(sTemp,"-SRC:",strlen("-SRC:"))==0 || 
				strncmp(sTemp,"/SRC:",strlen("/SRC:"))==0)
		{
			char drive[MAX_PATH]={0};
			char path[MAX_PATH]={0};
			char *pSource=argv[x]+strlen("-SRC:");

			bSourceSpecified=true;

			_splitpath(pSource,drive,path,NULL,NULL);

			WIN32_FIND_DATA findData={0};
			HANDLE hFind=FindFirstFile(pSource,&findData);
			if(hFind!=INVALID_HANDLE_VALUE)
			{
				do
				{
					char tempFile[MAX_PATH]={0};
					strcpy(tempFile,drive);
					strcat(tempFile,path);
					strcat(tempFile,findData.cFileName);

					vSourcesToLoad.push_back(tempFile);
				}
				while(FindNextFile(hFind,&findData));
				FindClose(hFind);
			}
		}
		else if(strncmp(sTemp,"-ETL:",strlen("-ETL:"))==0 || 
				strncmp(sTemp,"/ETL:",strlen("/ETL:"))==0)
		{
			sETLFile=(sTemp+5);

			if(!OpenETL(sETLFile.c_str()))
			{
				bFailedToOpenETL=true;
			}
		}		
		else if(strncmp(sTemp,"-L:",strlen("/L:"))==0)
		{
			if(strcmp(sTemp,"-L:NONE")==0){dwLevel=TRACE_LEVEL_NONE;}
			else if(strcmp(sTemp,"-L:FATAL")==0){dwLevel=TRACE_LEVEL_FATAL;}
			else if(strcmp(sTemp,"-L:ERROR")==0){dwLevel=TRACE_LEVEL_ERROR;}
			else if(strcmp(sTemp,"-L:WARNING")==0){dwLevel=TRACE_LEVEL_WARNING;}
			else if(strcmp(sTemp,"-L:INFORMATION")==0){dwLevel=TRACE_LEVEL_INFORMATION;}
			else if(strcmp(sTemp,"-L:VERBOSE")==0){dwLevel=TRACE_LEVEL_VERBOSE;}
			else if(strcmp(sTemp,"-L:RESERVED6")==0){dwLevel=TRACE_LEVEL_RESERVED6;}
			else if(strcmp(sTemp,"-L:RESERVED7")==0){dwLevel=TRACE_LEVEL_RESERVED7;}
			else if(strcmp(sTemp,"-L:RESERVED8")==0){dwLevel=TRACE_LEVEL_RESERVED8;}
			else if(strcmp(sTemp,"-L:RESERVED9")==0){dwLevel=TRACE_LEVEL_RESERVED9;}
			else 
			{
				bValidLevel=false;
			}
		}
		else if(strncmp(sTemp,"-S",strlen("/S"))==0)
		{
			bSilent=true;
		}

		delete [] sTemp;
	}
	for(x=0;x<vPDBsToLoad.size();x++)
	{
		OpenPDB(vPDBsToLoad[x].c_str(),!bSilent);
	}
	for(x=0;x<vSourcesToLoad.size();x++)
	{
		OpenCodeAddress(vSourcesToLoad[x].c_str(),0,!bSilent);
	}	
	if(!bSilent)
	{
		if(bPDBSpecified && vPDBsToLoad.size()==0)
		{
			MessageBox(GetActiveWindow(),"No PDB file was found in the specified paths","ETViewer",MB_ICONSTOP|MB_OK);
		}
		if(bSourceSpecified && vSourcesToLoad.size()==0)
		{
			MessageBox(GetActiveWindow(),"No source file was found in the specified paths","ETViewer",MB_ICONSTOP|MB_OK);
		}
		if(bFailedToOpenETL)
		{
			string sText="Failed to open .etl file '";
			sText+=sETLFile;
			sText+="'";
			MessageBox(GetActiveWindow(),sText.c_str(),"ETViewer",MB_ICONSTOP|MB_OK);
		}		
		if(!bValidLevel)
		{
			MessageBox(GetActiveWindow(),"The specified level is not valid, please use the following values\r\n\r\n\t-L:NONE\r\n\t-L:FATAL\r\n\t-L:ERROR\r\n\t-L:WARNING\r\n\t-L:INFORMATION\r\n\t-L:VERBOSE\r\n\t-L:RESERVED6\r\n\t-L:RESERVED7\r\n\t-L:RESERVED8\r\n\t-L:RESERVED9","ETViewer",MB_ICONSTOP|MB_OK);
		}
	}
	if(bValidLevel)
	{
		m_pFrame->GetProviderTree()->SetAllProviderLevel(dwLevel);
	}
	return TRUE;
}


// Cuadro de diálogo CAboutDlg utilizado para el comando Acerca de

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Datos del cuadro de diálogo
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Compatibilidad con DDX/DDV

// Implementación
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// Comando de la aplicación para ejecutar el cuadro de diálogo
void CETViewerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// Controladores de mensaje de CETViewerApp


void CETViewerApp::UpdateHighLightFilters()
{
	m_SplittedHighLightFilters.clear();
	for(DWORD x=0;x<m_HighLightFilters.size();x++)
	{
		m_HighLightFilters[x].UpdateObjects();

		if(m_HighLightFilters[x].GetEnabled())
		{
			char temp[1024];
			strcpy(temp,m_HighLightFilters[x].GetText().c_str());
			strupr(temp);

			char* token=strtok(temp,";");
			while(token!=NULL)
			{
				CHightLightFilter filter=m_HighLightFilters[x];
				filter.SetText(token);
				m_SplittedHighLightFilters.push_back(filter);
				token=strtok(NULL,";");
			}
		}
	}
	if(m_pFrame && m_pFrame->GetTracePane())
	{
		m_pFrame->GetTracePane()->InvalidateRect(NULL);
	}
		
}	
void CETViewerApp::UpdateInstantFilters()
{
	char temp[2048]={0};
	char* token=NULL;

	WaitForSingleObject(m_hInstantTraceMutex,INFINITE);

	m_dSplittedInstantFilters.clear();

	strcpy(temp,m_InstantExcludeFilter.c_str());
	strupr(temp);

	token=strtok(temp,";");
	while(token!=NULL)
	{
		CFilter filter;
		filter.m_Text=token;
		filter.m_bInclusionFilter=false;
		filter.m_dwTextLen=(DWORD)strlen(token);
		m_dSplittedInstantFilters.push_back(filter);
		token=strtok(NULL,";");
	}

	strcpy(temp,m_InstantIncludeFilter.c_str());
	strupr(temp);

	token=strtok(temp,";");
	while(token!=NULL)
	{
		CFilter filter;
		filter.m_Text=token;
		filter.m_bInclusionFilter=true;
		filter.m_dwTextLen=(DWORD)strlen(token);
		m_dSplittedInstantFilters.push_back(filter);
		token=strtok(NULL,";");
	}

	ReleaseMutex(m_hInstantTraceMutex);
}	

void CETViewerApp::LookupError(const char *pText)
{
	m_pFrame->LookupError(pText);
}


void CETViewerApp::AddRecentSourceFile(const char *pFile)
{
	string file;
	file=pFile;
	deque<string>::iterator i;
	for(i=m_RecentSourceFiles.begin();i!=m_RecentSourceFiles.end();i++)
	{
		if(strcmp(i->c_str(),pFile)==0)
		{
			m_RecentSourceFiles.erase(i);
			break;
		}
	}
	m_RecentSourceFiles.push_front(file);
	RefreshRecentFilesMenus();
	if(m_RecentSourceFiles.size()>=RECENT_SOURCE_FILE_MAX)
	{m_RecentSourceFiles.pop_back();}
}

void CETViewerApp::AddRecentPDBFile(const char *pFile)
{
	string file;
	file=pFile;
	deque<string>::iterator i;
	for(i=m_RecentPDBFiles.begin();i!=m_RecentPDBFiles.end();i++)
	{
		if(strcmp(i->c_str(),pFile)==0)
		{
			m_RecentPDBFiles.erase(i);
			break;
		}
	}
	m_RecentPDBFiles.push_front(file);
	RefreshRecentFilesMenus();
	if(m_RecentPDBFiles.size()>=RECENT_PDB_FILE_MAX)
	{m_RecentPDBFiles.pop_back();}
}

void CETViewerApp::AddRecentLogFile(const char *pFile)
{
	string file;
	file=pFile;
	deque<string>::iterator i;
	for(i=m_RecentLogFiles.begin();i!=m_RecentLogFiles.end();i++)
	{
		if(strcmp(i->c_str(),pFile)==0)
		{
			m_RecentLogFiles.erase(i);
			break;
		}
	}
	m_RecentLogFiles.push_front(file);
	RefreshRecentFilesMenus();
	if(m_RecentLogFiles.size()>=RECENT_LOG_FILE_MAX)
	{m_RecentLogFiles.pop_back();}
}

bool CETViewerApp::OpenPDB(const char *pFile,bool bShowErrorIfFailed)
{
	vector<CTraceProvider *> vProviders;
	eTracePDBReaderError eErrorCode=m_PDBReader.LoadFromPDB(pFile,&vProviders);
	if(eErrorCode==eTracePDBReaderError_Success)
	{
		AddRecentPDBFile(pFile);

		for(unsigned x=0;x<vProviders.size();x++)
		{
			CTraceProvider  *pProvider=vProviders[x];
			if(!AddProvider(pProvider))
			{
				if(bShowErrorIfFailed)
				{
					string sTemp="The provider '";
					sTemp+=pProvider->GetComponentName().c_str();
					sTemp+=" has already been loaded";
					MessageBox(GetActiveWindow(),sTemp.c_str(),"ETViewer",MB_ICONSTOP|MB_OK);
				}
				delete pProvider;
			}
		}
	}
	else
	{
		if(bShowErrorIfFailed)
		{
			string sTemp="Failed to load pdb file '";
			sTemp+=pFile;
			sTemp+="'.\r\n\r\n";
			switch(eErrorCode)
			{
			case eTracePDBReaderError_NoMemory:
				sTemp+="Not enought memory.";
				break;
			case eTracePDBReaderError_SymEngineInitFailed:
				sTemp+="Failed to initialize the symbol engine.";
				break;
			case eTracePDBReaderError_FailedToEnumerateSymbols:
				sTemp+="Failed to enumerate PDB symbols.";
				break;			
			case eTracePDBReaderError_SymEngineLoadModule:
				sTemp+="Failed to load module by the symbol engine.";
				break;			
			case eTracePDBReaderError_NoProviderFoundInPDB:
				sTemp+="No provider was found in the PDB file.";
				break;			
			case eTracePDBReaderError_DBGHelpNotFound:
				sTemp+="DBGHelp.dll was not found.";
				break;
			case eTracePDBReaderError_DBGHelpWrongVersion:
				sTemp+="Wrong version of DBGHelp.dll found (does not have the expected exports).";
				break;
			case eTracePDBReaderError_PDBNotFound:
				sTemp+="Cannot open the PDB file.";
				break;
			case eTracePDBReaderError_Generic:
				sTemp+="Generic error.";
				break;			
			default:
				sTemp+="Unknown error";
				break;
			}
			MessageBox(GetActiveWindow(),sTemp.c_str(),"ETViewer",MB_ICONSTOP|MB_OK);
		}
	}
	UpdateFileMonitor();
	return (eErrorCode==eTracePDBReaderError_Success);
}

bool CETViewerApp::OpenCodeAddress(const char *pFile,DWORD dwLine,bool bShowErrorIfFailed)
{	
	bool result=false;

	if(strcmp(pFile,"")==0){return false;}

	string sFile=pFile;

	bool bAccesible=false;
	if(_taccess(sFile.c_str(),0))
	{
		for(unsigned x=0;x<theApp.m_SourceDirectories.size();x++)
		{
			const char *pRemainder=pFile;

			do
			{
				pRemainder=strchr(pRemainder,'\\');
				if(pRemainder)
				{
					string sTempFile=theApp.m_SourceDirectories[x];
					sTempFile+=pRemainder;

					if(_taccess(sTempFile.c_str(),0)==0)
					{
						sFile=sTempFile;
						bAccesible=true;
						break;
					}
					// Skip \\ for the next iteration
					pRemainder=(pRemainder+1); 
				}
			}while(pRemainder);

			if(bAccesible){break;}
		}
	}
	else
	{
		bAccesible=true;
	}
	if(!bAccesible){return false;}
	result=m_SourceFileContainer.ShowFile(sFile.c_str(),dwLine,bShowErrorIfFailed);
	m_SourceFileContainer.BringWindowToTop();
	UpdateFileMonitor();
	return result;
}

bool CETViewerApp::OpenETL(const char *pFile)
{
	if(strcmp(pFile,"")==0){return false;}

	m_pFrame->GetTracePane()->Clear();
    m_Controller.Stop();
	ProcessSessionChange();

	if(m_Controller.OpenLog(pFile,m_pFrame->GetTracePane())!=ERROR_SUCCESS)
	{
		m_Controller.StartRealTime("ETVIEWER_SESSION",m_pFrame->GetTracePane());
		ProcessSessionChange();
		return false;
	}
	else
	{
		theApp.AddRecentLogFile(pFile);
		ProcessSessionChange();
		return true;
	}
}

bool CETViewerApp::CreateETL(const char *pFile)
{
	if(strcmp(pFile,"")==0){return false;}

	m_pFrame->GetTracePane()->Clear();
	m_Controller.Stop();
	ProcessSessionChange();

	if(m_Controller.CreateLog(pFile,m_pFrame->GetTracePane())!=ERROR_SUCCESS)
	{
		m_Controller.StartRealTime("ETVIEWER_SESSION",m_pFrame->GetTracePane());
		ProcessSessionChange();
		return false;
	}
	else
	{
		theApp.AddRecentLogFile(pFile);
		ProcessSessionChange();
		return true;
	}
}

void CETViewerApp::CloseETL()
{
	m_Controller.Stop();
	ProcessSessionChange();

	m_Controller.StartRealTime("ETVIEWER_SESSION",m_pFrame->GetTracePane());
	ProcessSessionChange();
}

void CETViewerApp::CloseSession()
{
	m_Controller.Stop();
	ProcessSessionChange();
}

void CETViewerApp::ProcessSessionChange()
{
	if(m_pFrame)
	{
		CProviderTree *pTree=m_pFrame->GetProviderTree();
		CETViewerView *pTraceViewer=m_pFrame->GetTracePane();
		m_pFrame->OnSessionTypeChanged();
		pTree->OnSessionTypeChanged();
		pTraceViewer->OnSessionTypeChanged();
	}
}

bool CETViewerApp::AddProvider(CTraceProvider *pProvider)
{
	if(!theApp.m_Controller.AddProvider(pProvider,pProvider->GetAllSupportedFlagsMask(),TRACE_LEVEL_VERBOSE))
	{
		return false;
	}

	CProviderTree *pTree=m_pFrame->GetProviderTree();
	CETViewerView *pTraceViewer=m_pFrame->GetTracePane();
	pTree->OnAddProvider(pProvider);
	pTraceViewer->OnAddProvider(pProvider);
	pTree->OnProvidersModified();
	pTraceViewer->OnProvidersModified();
	m_sProviders.insert(pProvider);
	return true;
}

void CETViewerApp::RemoveProvider(CTraceProvider *pProvider)
{
	CProviderTree *pTree=m_pFrame->GetProviderTree();
	CETViewerView *pTraceViewer=m_pFrame->GetTracePane();
	theApp.m_Controller.RemoveProvider(pProvider);
	pTree->OnRemoveProvider(pProvider);
	pTraceViewer->OnRemoveProvider(pProvider);
	delete pProvider;
	m_sProviders.erase(pProvider);
	UpdateFileMonitor();
}

void CETViewerApp::RemoveAllProviders()
{
	CETViewerView *pTraceViewer=m_pFrame->GetTracePane();
	CProviderTree *pTree=m_pFrame->GetProviderTree();

	while(m_sProviders.size())
	{
		CTraceProvider *pProvider=*m_sProviders.begin();	
		theApp.m_Controller.RemoveProvider(pProvider);
		pTree->OnRemoveProvider(pProvider);
		pTraceViewer->OnRemoveProvider(pProvider);
		m_sProviders.erase(pProvider);
	}
	UpdateFileMonitor();
}

void CETViewerApp::ReloadProvider(CTraceProvider *pProvider)
{
	CETViewerView *pTraceViewer=m_pFrame->GetTracePane();
	CProviderTree *pTree=m_pFrame->GetProviderTree();

	vector<CTraceProvider *> loadedProviders;
	CTracePDBReader reader;
	reader.LoadFromPDB(pProvider->GetFileName().c_str(),&loadedProviders);
	unsigned x;
	bool bReplaced=false;
	for(x=0;x<loadedProviders.size();x++)
	{
		CTraceProvider *pLoadedProvider=loadedProviders[x];
		if(!bReplaced && pLoadedProvider->GetGUID()==pProvider->GetGUID())
		{
			bReplaced=true;
			m_sProviders.erase(pProvider);
			m_Controller.ReplaceProvider(pProvider,pLoadedProvider);
			pTree->OnReplaceProvider(pProvider,pLoadedProvider);
			pTraceViewer->OnReplaceProvider(pProvider,pLoadedProvider);
			m_sProviders.insert(pLoadedProvider);
			delete pProvider;
		}
		else
		{
			delete pLoadedProvider;
		}
	}
	if(bReplaced)
	{
		pTree->OnProvidersModified();
		pTraceViewer->OnProvidersModified();
	}
}

void CETViewerApp::ReloadAllProviders()
{
	CETViewerView *pTraceViewer=m_pFrame->GetTracePane();
	CProviderTree *pTree=m_pFrame->GetProviderTree();

	set<string> sPDBsToLoad;
	set<string>::iterator iPDB; 
	set<CTraceProvider *>::iterator iProvider; 
	for(iProvider=m_sProviders.begin();iProvider!=m_sProviders.end();iProvider++)
	{
		CTraceProvider *pProvider=*iProvider;
		sPDBsToLoad.insert(pProvider->GetFileName());
	}
	bool bAnyReplaced=false;

	for(iPDB=sPDBsToLoad.begin();iPDB!=sPDBsToLoad.end();iPDB++)
	{
		string sPDB=*iPDB;
		vector<CTraceProvider *> loadedProviders;
		CTracePDBReader reader;
		reader.LoadFromPDB(sPDB.c_str(),&loadedProviders);
		unsigned x;
		for(x=0;x<loadedProviders.size();x++)
		{
			CTraceProvider *pLoadedProvider=loadedProviders[x];
			CTraceProvider *pExistingProvider=NULL;
			for(iProvider=m_sProviders.begin();iProvider!=m_sProviders.end();iProvider++)
			{
				CTraceProvider *pProvider=*iProvider;				
				if(pProvider->GetGUID()==pLoadedProvider->GetGUID())
				{
					pExistingProvider=pProvider;
					break;
				}
			}
			if(pExistingProvider)
			{
				bAnyReplaced=true;
				m_sProviders.erase(pExistingProvider);
				m_Controller.ReplaceProvider(pExistingProvider,pLoadedProvider);
				pTree->OnReplaceProvider(pExistingProvider,pLoadedProvider);
				pTraceViewer->OnReplaceProvider(pExistingProvider,pLoadedProvider);
				m_sProviders.insert(pLoadedProvider);
				delete pExistingProvider;
			}
			else
			{
				delete pLoadedProvider;
			}
		}
	}
	if(bAnyReplaced)
	{
		pTree->OnProvidersModified();
		pTraceViewer->OnProvidersModified();
	}
}

bool CETViewerApp::ReloadPDBProviders(string sFileName)
{
	CETViewerView *pTraceViewer=m_pFrame->GetTracePane();
	CProviderTree *pTree=m_pFrame->GetProviderTree();

	bool bModified=false;
	vector<CTraceProvider *> loadedProviders;
	set<CTraceProvider *>::iterator iProvider; 
	CTracePDBReader reader;
	if(reader.LoadFromPDB(sFileName.c_str(),&loadedProviders)!=eTracePDBReaderError_Success)
	{
		return false;
	}
	unsigned x;
	for(x=0;x<loadedProviders.size();x++)
	{
		CTraceProvider *pLoadedProvider=loadedProviders[x];
		CTraceProvider *pExistingProvider=NULL;
		for(iProvider=m_sProviders.begin();iProvider!=m_sProviders.end();iProvider++)
		{
			CTraceProvider *pProvider=*iProvider;				
			if(pProvider->GetGUID()==pLoadedProvider->GetGUID())
			{
				pExistingProvider=pProvider;
				break;
			}
		}
		if(pExistingProvider)
		{
			bModified=true;
			m_sProviders.erase(pExistingProvider);
			m_Controller.ReplaceProvider(pExistingProvider,pLoadedProvider);
			pTree->OnReplaceProvider(pExistingProvider,pLoadedProvider);
			pTraceViewer->OnReplaceProvider(pExistingProvider,pLoadedProvider);
			m_sProviders.insert(pLoadedProvider);
			delete pExistingProvider;
		}
		else
		{
			bModified=true;
			m_Controller.AddProvider(pLoadedProvider,pLoadedProvider->GetAllSupportedFlagsMask(),TRACE_LEVEL_VERBOSE);
			pTree->OnAddProvider(pLoadedProvider);
			pTraceViewer->OnAddProvider(pLoadedProvider);
			m_sProviders.insert(pLoadedProvider);
		}
	}

	if(bModified)
	{
		pTree->OnProvidersModified();
		pTraceViewer->OnProvidersModified();
	}
	return true;
}

bool CETViewerApp::FilterTrace(const char *pText)
{
	WaitForSingleObject(m_hInstantTraceMutex,INFINITE);

	bool res=false;

	char tempText[2048];
	tempText[0]=0;
	unsigned textLen=0;
	while(pText[textLen]!=0)
	{
		if(pText[textLen]>='a' && pText[textLen]<='z')
		{
			tempText[textLen]=pText[textLen]-'a'+'A';
		}
		else
		{
			tempText[textLen]=pText[textLen];
		}
		textLen++;
	}
	tempText[textLen]=0;
        
	// Text Filters must always be ordered by relevance, the first filter that matches the criteria
	// is the effective filter 

	bool bPassed=true;
	unsigned x;
	for(x=0;x<m_dSplittedInstantFilters.size();x++)
	{
		int index=0,maxTextSearchSize=textLen-m_dSplittedInstantFilters[x].m_dwTextLen;
		if(maxTextSearchSize>0)
		{
			CFilter *pFilter=&m_dSplittedInstantFilters[x];
			const char *pFilterText=m_dSplittedInstantFilters[x].m_Text.c_str();
			if(pFilterText[0]=='*')
			{
				bPassed=(pFilter->m_bInclusionFilter)?true:false;
				ReleaseMutex(m_hInstantTraceMutex);
				return bPassed;
			}

			while(index<=maxTextSearchSize)
			{
				if(memcmp(tempText+index,pFilterText,pFilter->m_dwTextLen)==0)
				{
					bPassed=(pFilter->m_bInclusionFilter)?true:false;
					ReleaseMutex(m_hInstantTraceMutex);
					return bPassed;
				}
				index++;
			}
		}
	}

	ReleaseMutex(m_hInstantTraceMutex);

	return false;
}

void CETViewerApp::RefreshRecentFilesMenus()
{
	CMenu *pLogFilesMenu=m_pFrame->GetMenu()->GetSubMenu(0)->GetSubMenu(9);
	CMenu *pSourceFilesMenu=m_pFrame->GetMenu()->GetSubMenu(0)->GetSubMenu(8);
	CMenu *pPDBFilesMenu=m_pFrame->GetMenu()->GetSubMenu(0)->GetSubMenu(7);

	if(pSourceFilesMenu==NULL || pPDBFilesMenu==NULL || pLogFilesMenu==NULL){return;}
	unsigned x;
	while(pPDBFilesMenu->GetMenuItemCount()){pPDBFilesMenu->RemoveMenu(0,MF_BYPOSITION);}
	while(pSourceFilesMenu->GetMenuItemCount()){pSourceFilesMenu->RemoveMenu(0,MF_BYPOSITION);}
	while(pLogFilesMenu->GetMenuItemCount()){pLogFilesMenu->RemoveMenu(0,MF_BYPOSITION);}

	for(x=0;x<m_RecentPDBFiles.size();x++)
	{
		char *pText=(char*)m_RecentPDBFiles[x].c_str();
		pPDBFilesMenu->InsertMenu(x,MF_BYPOSITION,RECENT_PDB_FILE_BASE_INDEX+x,pText);
	}
	for(x=0;x<m_RecentSourceFiles.size();x++)
	{
		char *pText=(char*)m_RecentSourceFiles[x].c_str();
		pSourceFilesMenu->InsertMenu(x,MF_BYPOSITION,RECENT_SOURCE_FILE_BASE_INDEX+x,pText);
	}
	for(x=0;x<m_RecentLogFiles.size();x++)
	{
		char *pText=(char*)m_RecentLogFiles[x].c_str();
		pLogFilesMenu->InsertMenu(x,MF_BYPOSITION,RECENT_LOG_FILE_BASE_INDEX+x,pText);
	}

	if(m_RecentPDBFiles.size()==0){pPDBFilesMenu->InsertMenu(x,MF_BYPOSITION,RECENT_PDB_FILE_BASE_INDEX,"<No Recent File>");}
	if(m_RecentSourceFiles.size()==0){pSourceFilesMenu->InsertMenu(x,MF_BYPOSITION,RECENT_SOURCE_FILE_BASE_INDEX,"<No Recent File>");}
	if(m_RecentLogFiles.size()==0){pLogFilesMenu->InsertMenu(x,MF_BYPOSITION,RECENT_LOG_FILE_BASE_INDEX,"<No Recent File>");}
}

void CETViewerApp::OnRecentPDBFile(UINT nID)
{
	if(m_RecentPDBFiles.size()==0){return;}
	string file=m_RecentPDBFiles[nID-RECENT_PDB_FILE_BASE_INDEX];
	m_pFrame->OpenFile(file.c_str(),NULL);
}

void CETViewerApp::OnRecentSourceFile(UINT nID)
{
	if(m_RecentSourceFiles.size()==0){return;}
	string file=m_RecentSourceFiles[nID-RECENT_SOURCE_FILE_BASE_INDEX];
	OpenCodeAddress(file.c_str(),0,true);
}

void CETViewerApp::OnRecentLogFile(UINT nID)
{
	if(m_RecentLogFiles.size()==0){return;}
	string file=m_RecentLogFiles[nID-RECENT_LOG_FILE_BASE_INDEX];
	m_pFrame->OpenFile(file.c_str(),NULL);
}

void CETViewerApp::OnClose()
{
	SaveTo(&m_ConfigFile,"Application");
	m_pFrame->GetTracePane()->Save(&theApp.m_ConfigFile);
	m_ConfigFile.Save(m_sConfigFile.c_str());
	if(m_hSingleInstanceMutex){CloseHandle(m_hSingleInstanceMutex);m_hSingleInstanceMutex=NULL;}

	CloseSession();
}

int CETViewerApp::ExitInstance()
{
	int res=CWinApp::ExitInstance();
	CoUninitialize();
	return res;
}

void CETViewerApp::UpdateFileAssociations()
{
	if(m_bAssociatePDB){SetFileAssociation(".pdb","pdbfile","Program Database","-pdb:\"%1\"");}
	if(m_bAssociateETL){SetFileAssociation(".etl","etlfile","Event Tracing for Windows (ETW) Log","-etl:\"%1\"");}
	if(m_bAssociateSources)
	{
		SetFileAssociation(".cpp","cppfile","C++ Source File","-src:\"%1\"");
		SetFileAssociation(".c","cfile","C Source File","-src:\"%1\"");
		SetFileAssociation(".h","hfile","Header File","-src:\"%1\"");
		SetFileAssociation(".inl","inlfile","Inline File","-src:\"%1\"");
	}
}

void CETViewerApp::OnFileChanged(string sFile)
{
	AddFileChangeOperation(sFile);
}

void CETViewerApp::AddFileChangeOperation(string sFileName)
{
	WaitForSingleObject(m_hPendingFileChangesMutex,INFINITE);
	bool bFound=false;
	list<SPendingFileChangeOperation>::iterator i;
	for(i=m_lPendingFileChanges.begin();i!=m_lPendingFileChanges.end();i++)
	{
		SPendingFileChangeOperation op=*i;
		if(op.sFileName==sFileName)
		{
			bFound=true;
			break;
		}
	}
	if(!bFound)
	{
		SPendingFileChangeOperation op;
		op.sFileName=sFileName;
		op.dwChangeTime=GetTickCount();
		m_lPendingFileChanges.push_back(op);
	}
	ReleaseMutex(m_hPendingFileChangesMutex);
}

void CETViewerApp::RemoveFileChangeOperation(string sFileName)
{
	WaitForSingleObject(m_hPendingFileChangesMutex,INFINITE);

	list<SPendingFileChangeOperation>::iterator i;
	for(i=m_lPendingFileChanges.begin();i!=m_lPendingFileChanges.end();)
	{
		SPendingFileChangeOperation op=*i;
		if(op.sFileName==sFileName)
		{
			i=m_lPendingFileChanges.erase(i);
		}
		else
		{
			i++;
		}
	}

	ReleaseMutex(m_hPendingFileChangesMutex);
}

void CETViewerApp::RemoveExpiredFileChangeOperations()
{
	WaitForSingleObject(m_hPendingFileChangesMutex,INFINITE);

	list<SPendingFileChangeOperation>::iterator i;
	for(i=m_lPendingFileChanges.begin();i!=m_lPendingFileChanges.end();)
	{
		SPendingFileChangeOperation op=*i;
		if((op.dwChangeTime+FILE_CHANGE_EXPIRATION_TIME)<GetTickCount())
		{
			i=m_lPendingFileChanges.erase(i);
		}
		else
		{
			i++;
		}
	}

	ReleaseMutex(m_hPendingFileChangesMutex);
}

void CETViewerApp::CheckFileChangeOperations()
{
	if(m_bCheckingFileChangeOperations){return;}
	m_bCheckingFileChangeOperations=true;

	if( m_eSourceMonitoringMode==eFileMonitoringMode_AutoReload ||
		m_ePDBMonitoringMode==eFileMonitoringMode_AutoReload)
	{
		RemoveExpiredFileChangeOperations();
	}

	WaitForSingleObject(m_hPendingFileChangesMutex,INFINITE);

	bool bRetryOperation=false;

	list<SPendingFileChangeOperation>::iterator i;
	for(i=m_lPendingFileChanges.begin();i!=m_lPendingFileChanges.end();)
	{
		SPendingFileChangeOperation op=*i;
		char sExt[MAX_PATH]={0};
		_splitpath(op.sFileName.c_str(),NULL,NULL,NULL,sExt);
		bool bRemoveOperation=false;
		bool bIsPDB=(stricmp(sExt,".PDB")==0);
		bool bMustAsk=(bIsPDB && m_ePDBMonitoringMode==eFileMonitoringMode_Ask) ||	
		              (!bIsPDB && m_eSourceMonitoringMode==eFileMonitoringMode_Ask);

		if(!bRetryOperation)
		{
			if(bMustAsk)
			{
				string sQuestion;
				sQuestion="File '";
				sQuestion+=op.sFileName;
				sQuestion+="' has changed.\r\n\r\nDo you want to reload it?";

				if(MessageBox(m_pFrame->m_hWnd,sQuestion.c_str(),"ETViewer",MB_ICONQUESTION|MB_YESNO)==IDNO)
				{
					bRemoveOperation=true;
				}
			}
		}

		bRetryOperation=false;

		if(!bRemoveOperation)
		{
			bool bReloadOk=true;

			if(bIsPDB)
			{
				bReloadOk=ReloadPDBProviders(op.sFileName.c_str());
			}
			else 
			{
				m_SourceFileContainer.ReloadFile(op.sFileName.c_str());
			}

			if(bReloadOk)
			{
				bRemoveOperation=true;
			}
			else
			{
				if(bMustAsk)
				{
					string sQuestion;
					sQuestion="File '";
					sQuestion+=op.sFileName;
					sQuestion+="' cannot be reloaded.\r\n\r\nDo you want to retry?";

					if(MessageBox(m_pFrame->m_hWnd,sQuestion.c_str(),"ETViewer",MB_ICONEXCLAMATION|MB_RETRYCANCEL)==IDRETRY)
					{
						bRetryOperation=true;
					}
					else
					{
						bRemoveOperation=true;
					}
				}
			}
		}

		if(bRemoveOperation)
		{
			i=m_lPendingFileChanges.erase(i);
		}
		else if(!bRetryOperation)
		{
			i++;
		}
	}

	ReleaseMutex(m_hPendingFileChangesMutex);

	m_bCheckingFileChangeOperations=false;
};

void CETViewerApp::UpdateFileMonitor()
{
	set<string> sFiles;
	
	if(m_eSourceMonitoringMode!=eFileMonitoringMode_None &&
		m_eSourceMonitoringMode!=eFileMonitoringMode_Ignore )
	{
		m_SourceFileContainer.GetFiles(&sFiles);
	}

	if(m_ePDBMonitoringMode!=eFileMonitoringMode_None &&
		m_ePDBMonitoringMode!=eFileMonitoringMode_Ignore )
	{
		set<CTraceProvider *>::iterator i;
		for(i=m_sProviders.begin();i!=m_sProviders.end();i++)
		{
			CTraceProvider *pProvider=*i;
			sFiles.insert(pProvider->GetFileName());
		}
	}

	m_pFileMonitor->SetFiles(&sFiles);
}
