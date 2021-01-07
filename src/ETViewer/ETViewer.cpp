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
#include "ETViewer.h"
#include "MainFrm.h"
#include "ETViewerDoc.h"
#include "ETViewerView.h"
#include "ProviderTree.h"
#include "FileMonitor.h"
#include "PersistentSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define ET_VIEWER_NAME "ETViewer"

extern DWORD g_dwRegisteredMessage;

BEGIN_MESSAGE_MAP(CETViewerApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
    ON_COMMAND_RANGE(RECENT_PDB_FILE_BASE_INDEX, RECENT_PDB_FILE_BASE_INDEX + RECENT_PDB_FILE_MAX, OnRecentPDBFile)
    ON_COMMAND_RANGE(RECENT_SOURCE_FILE_BASE_INDEX, RECENT_SOURCE_FILE_BASE_INDEX + RECENT_SOURCE_FILE_MAX, OnRecentSourceFile)
    ON_COMMAND_RANGE(RECENT_LOG_FILE_BASE_INDEX, RECENT_LOG_FILE_BASE_INDEX + RECENT_LOG_FILE_MAX, OnRecentLogFile)
END_MESSAGE_MAP()

CETViewerApp::CETViewerApp()
    : m_bCheckingFileChangeOperations(false)
    , m_hPendingFileChangesMutex(CreateMutex(nullptr, false, nullptr))
    , m_hInstantTraceMutex(CreateMutex(nullptr, false, nullptr))
    , m_hSingleInstanceMutex(nullptr)
    , m_eSourceMonitoringMode(eFileMonitoringMode_AutoReload)
    , m_ePDBMonitoringMode(eFileMonitoringMode_AutoReload)
    , m_pFileMonitor(new CFileMonitor(this))
{
    PersistentSettings settings;

    m_ExcludeFilters = settings.ReadMultiStringValue(L"ExcludeFilter", {});
    m_IncludeFilters = settings.ReadMultiStringValue(L"IncludeFilter", {L"*"});

    m_RecentLogFiles = settings.ReadMultiStringValue(L"RecentLogFiles", {});
    m_RecentSourceFiles = settings.ReadMultiStringValue(L"RecentSourceFiles", {});
    m_RecentPDBFiles = settings.ReadMultiStringValue(L"RecentPDBFiles", {});

    m_SourceDirectories = settings.ReadMultiStringValue(L"SourceDirectories", {});

    m_bAssociateETL = settings.ReadBoolValue(L"AssociateETL", false);
    m_bAssociatePDB = settings.ReadBoolValue(L"AssociatePDB", false);
    m_bAssociateSources = settings.ReadBoolValue(L"AssociateSources", false);

    m_ePDBMonitoringMode = (eFileMonitoringMode)settings.ReadDwordValue(L"PDBMonitoringMode", eFileMonitoringMode_AutoReload);
    m_eSourceMonitoringMode = (eFileMonitoringMode)settings.ReadDwordValue(L"SourceMonitoringMode", eFileMonitoringMode_AutoReload);

    auto highlightFilters = settings.ReadMultiStringValue(L"HightlightFilter", {});
    for (auto& entry : highlightFilters)
    {
        m_HighLightFilters.emplace_back(entry);
    }

    UpdateHighLightFilters();
}

CETViewerApp::~CETViewerApp()
{
    std::set<CTraceProvider*>::iterator i;
    for (i = m_sProviders.begin(); i != m_sProviders.end(); i++)
    {
        CTraceProvider* pProvider = *i;
        delete pProvider;
    }

    m_sProviders.clear();

    if (m_hInstantTraceMutex)
    {
        CloseHandle(m_hInstantTraceMutex);
        m_hInstantTraceMutex = NULL;
    }

    if (m_hPendingFileChangesMutex)
    {
        CloseHandle(m_hPendingFileChangesMutex);
        m_hPendingFileChangesMutex = NULL;
    }

    m_pFileMonitor->Stop();
    delete m_pFileMonitor;
    m_pFileMonitor = NULL;
}

CETViewerApp theApp;

BOOL CETViewerApp::InitInstance()
{
    CoInitialize(NULL);

    bool bFirstInstance = FALSE;
    DWORD dwLastError = ERROR_SUCCESS;
    m_hSingleInstanceMutex = CreateMutex(NULL, TRUE, _T("ETViewerSingleInstanceMutex"));
    if (m_hSingleInstanceMutex)
    {
        dwLastError = GetLastError();
        if (dwLastError != ERROR_ALREADY_EXISTS)
        {
            bFirstInstance = TRUE;
        }
    }

    //If UAC is enabled we need to open up some messages so drag and drop will work if we are admin and explorer isn't

    typedef BOOL(WINAPI* PFN_CHANGEWINDOWMESSAGEFILTER) (UINT, DWORD);
    HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
    if (0 != hUser32)
    {
        PFN_CHANGEWINDOWMESSAGEFILTER pfnChangeWindowMessageFilter = (PFN_CHANGEWINDOWMESSAGEFILTER) ::GetProcAddress(hUser32, "ChangeWindowMessageFilter");
        if (pfnChangeWindowMessageFilter)
        {
            (*pfnChangeWindowMessageFilter)(WM_DROPFILES, MSGFLT_ADD);
            (*pfnChangeWindowMessageFilter)(WM_COPYDATA, MSGFLT_ADD);
            (*pfnChangeWindowMessageFilter)(0x0049, MSGFLT_ADD);
        }
    }

    // If this is not the first instance, send the command line files to
    // to the first instance, this way, a user can double click an element
    // on a folder to add it to ETViewer.
    if (!bFirstInstance)
    {
        for (int x = 0; x < __argc; x++)
        {
            if (__wargv[x][0] == _T('/') || __wargv[x][0] == _T('-'))
            {
                ATOM hAtom = GlobalAddAtom(__wargv[x]);
                if (hAtom)
                {
                    // PostMessage is not an option because the Atom can be deleted before the message is received
                    // SendMessage does not return in both Vista and Windows7 with UAC on
                    ::SendMessageTimeout(HWND_BROADCAST, g_dwRegisteredMessage, hAtom, 0, SMTO_BLOCK, 1000, NULL);
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

    m_SourceFileContainer.Create(IDD_SOURCE_FILE_CONTAINER, m_pFrame);

    // Se ha inicializado la única ventana; mostrarla y actualizarla
    m_pFrame = dynamic_cast<CMainFrame*>(m_pMainWnd);
    // Llamar a DragAcceptFiles sólo si existe un sufijo
    //  En una aplicación SDI, esto debe ocurrir después de ProcessShellCommand

    m_SourceFileContainer.SetWindowPos(&CWnd::wndNoTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    RefreshRecentFilesMenus();
    ProcessSessionChange();
    BOOL bContinue = ProcessCommandLine(__argc, __wargv);
    if (bContinue)
    {
        m_pMainWnd->ShowWindow(SW_SHOW);
        m_pMainWnd->UpdateWindow();
        m_pFileMonitor->Start();
    }
    return bContinue;
}

void CETViewerApp::SetFileAssociation(TCHAR* pExtension, TCHAR* pFileTypeName, TCHAR* pFileDescription, TCHAR* pCommandLineTag)
{
    bool bAlreadyPresent = false;
    HKEY hKey = NULL;
    DWORD dwError = RegOpenKey(HKEY_CLASSES_ROOT, pExtension, &hKey);
    if (dwError == ERROR_SUCCESS)
    {
        long dwSize = MAX_PATH;
        TCHAR  sValue[MAX_PATH] = { 0 };
        dwError = RegQueryValue(hKey, NULL, sValue, &dwSize);
        if (dwError == ERROR_SUCCESS && _tcscmp(sValue, pFileTypeName) == 0)
        {
            bAlreadyPresent = true;
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }
    if (!bAlreadyPresent)
    {
        dwError = RegCreateKey(HKEY_CLASSES_ROOT, pExtension, &hKey);
        if (dwError == ERROR_SUCCESS)
        {
            dwError = RegSetValue(hKey, NULL, REG_SZ, pFileTypeName, _tcslen(pFileTypeName) + 1);
        }
        RegCloseKey(hKey);
        hKey = NULL;
    }
    dwError = RegOpenKey(HKEY_CLASSES_ROOT, pFileTypeName, &hKey);
    if (hKey) { RegCloseKey(hKey); hKey = NULL; }

    dwError = RegCreateKey(HKEY_CLASSES_ROOT, pFileTypeName, &hKey);
    if (dwError == ERROR_SUCCESS)
    {
        dwError = RegSetValue(hKey, NULL, REG_SZ, pFileDescription, _tcslen(pFileDescription) + 1);
    }
    if (hKey) { RegCloseKey(hKey); hKey = NULL; }

    if (dwError == ERROR_SUCCESS)
    {
        std::wstring sKeyName = pFileTypeName;
        sKeyName += _T("\\shell\\open\\command");
        dwError = RegCreateKey(HKEY_CLASSES_ROOT, sKeyName.c_str(), &hKey);
        if (dwError == ERROR_SUCCESS)
        {
            TCHAR pModule[MAX_PATH] = { 0 };
            TCHAR pCommand[MAX_PATH] = { 0 };
            GetModuleFileName(NULL, pModule, MAX_PATH);
            _stprintf_s(pCommand, _T("\"%s\" %s"), pModule, pCommandLineTag);
            dwError = RegSetValue(hKey, NULL, REG_SZ, pCommand, _tcslen(pCommand) + 1);
        }
        if (hKey) { RegCloseKey(hKey); hKey = NULL; }
    }
}

BOOL CETViewerApp::ProcessCommandLine(int argc, TCHAR** argw)
{
    unsigned x = 0;
    bool bPDBSpecified = false, bSilent = false, bSourceSpecified = false;
    bool bLevelSpecified = false, bValidLevel = true, bFailedToOpenETL = false;
    DWORD dwLevel = TRACE_LEVEL_VERBOSE;
    std::wstring sETLFile;

    std::vector<std::wstring> vPDBsToLoad;
    std::vector<std::wstring> vSourcesToLoad;

    for (x = 0; x < (ULONG)argc; x++)
    {
        DWORD dwTmpLen = _tcslen(argw[x]) + 1;
        TCHAR* sTemp = new TCHAR[dwTmpLen];
        _tcscpy_s(sTemp, dwTmpLen, argw[x]);
        _tcsupr_s(sTemp, dwTmpLen);
        if (_tcscmp(sTemp, _T("-V")) == 0 ||
            _tcscmp(sTemp, _T("/V")) == 0)
        {
            MessageBox(NULL, _T("ETViewer v0.9\n"), _T("ETViewer"), MB_OK);
            return FALSE;
        }
        else if (_tcscmp(sTemp, _T("-H")) == 0 ||
            _tcscmp(sTemp, _T("/H")) == 0)
        {
            MessageBox(NULL, _T("ETViewer v1.1\n\n")
                _T("Command line options:\n\n")
                _T("  -v   Show version\n")
                _T("  -h   Show this help\n")
                _T("  -s   Silent: Do not warn about statup errors\n")
                _T("  -pdb:<file filter>  PDB files to load, for example -pdb:C:\\Sample\\*.pdb\n")
                _T("  -src:<file filter>   Source files to load, for example -src:C:\\Sample\\*.cpp\n")
                _T("  -etl:<file> Load the specified .etl file on startup\n ")
                _T("  -l:<level> Initial tracing level:\n")
                _T("     FATAL\n")
                _T("     ERROR\n")
                _T("     WARNING\n")
                _T("     INFORMATION\n")
                _T("     VERBOSE\n")
                _T("     RESERVED6\n")
                _T("     RESERVED7\n")
                _T("     RESERVED8\n")
                _T("     RESERVED9"),
                _T("ETViewer"), MB_OK);
            return FALSE;
        }
        else if (_tcsncmp(sTemp, _T("-PDB:"), _tcslen(_T("-PDB:"))) == 0 ||
            _tcsncmp(sTemp, _T("/PDB:"), _tcslen(_T("/PDB:"))) == 0)
        {
            TCHAR drive[MAX_PATH] = { 0 };
            TCHAR path[MAX_PATH] = { 0 };
            TCHAR* pPDB = argw[x] + _tcslen(_T("-PDB:"));

            bPDBSpecified = true;

            _tsplitpath_s(pPDB, drive, MAX_PATH, path, MAX_PATH, NULL, 0, NULL, 0);

            WIN32_FIND_DATA findData = { 0 };
            HANDLE hFind = FindFirstFile(pPDB, &findData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    TCHAR tempFile[MAX_PATH] = { 0 };
                    _tcscpy_s(tempFile, drive);
                    _tcscpy_s(tempFile, path);
                    _tcscpy_s(tempFile, findData.cFileName);

                    vPDBsToLoad.push_back(tempFile);
                }
                while (FindNextFile(hFind, &findData));
                FindClose(hFind);
            }
        }
        else if (_tcsncmp(sTemp, _T("-SRC:"), _tcslen(_T("-SRC:"))) == 0 ||
            _tcsncmp(sTemp, _T("/SRC:"), _tcslen(_T("/SRC:"))) == 0)
        {
            TCHAR drive[MAX_PATH] = { 0 };
            TCHAR path[MAX_PATH] = { 0 };
            TCHAR* pSource = argw[x] + _tcslen(_T("-SRC:"));

            bSourceSpecified = true;

            _tsplitpath_s(pSource, drive, MAX_PATH, path, MAX_PATH, NULL, 0, NULL, 0);

            WIN32_FIND_DATA findData = { 0 };
            HANDLE hFind = FindFirstFile(pSource, &findData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    TCHAR tempFile[MAX_PATH] = { 0 };
                    _tcscpy_s(tempFile, drive);
                    _tcscpy_s(tempFile, path);
                    _tcscpy_s(tempFile, findData.cFileName);

                    vSourcesToLoad.push_back(tempFile);
                }
                while (FindNextFile(hFind, &findData));
                FindClose(hFind);
            }
        }
        else if (_tcsncmp(sTemp, _T("-ETL:"), _tcslen(_T("-ETL:"))) == 0 ||
            _tcsncmp(sTemp, _T("/ETL:"), _tcslen(_T("/ETL:"))) == 0)
        {
            sETLFile = (sTemp + 5);

            if (!OpenETL(sETLFile.c_str()))
            {
                bFailedToOpenETL = true;
            }
        }
        else if (_tcsncmp(sTemp, _T("-L:"), _tcslen(_T("/L:"))) == 0)
        {
            if (_tcscmp(sTemp, _T("-L:NONE")) == 0) { dwLevel = TRACE_LEVEL_NONE; }
            else if (_tcscmp(sTemp, _T("-L:FATAL")) == 0) { dwLevel = TRACE_LEVEL_FATAL; }
            else if (_tcscmp(sTemp, _T("-L:ERROR")) == 0) { dwLevel = TRACE_LEVEL_ERROR; }
            else if (_tcscmp(sTemp, _T("-L:WARNING")) == 0) { dwLevel = TRACE_LEVEL_WARNING; }
            else if (_tcscmp(sTemp, _T("-L:INFORMATION")) == 0) { dwLevel = TRACE_LEVEL_INFORMATION; }
            else if (_tcscmp(sTemp, _T("-L:VERBOSE")) == 0) { dwLevel = TRACE_LEVEL_VERBOSE; }
            else if (_tcscmp(sTemp, _T("-L:RESERVED6")) == 0) { dwLevel = TRACE_LEVEL_RESERVED6; }
            else if (_tcscmp(sTemp, _T("-L:RESERVED7")) == 0) { dwLevel = TRACE_LEVEL_RESERVED7; }
            else if (_tcscmp(sTemp, _T("-L:RESERVED8")) == 0) { dwLevel = TRACE_LEVEL_RESERVED8; }
            else if (_tcscmp(sTemp, _T("-L:RESERVED9")) == 0) { dwLevel = TRACE_LEVEL_RESERVED9; }
            else
            {
                bValidLevel = false;
            }
        }
        else if (_tcsncmp(sTemp, _T("-S"), _tcslen(_T("/S"))) == 0)
        {
            bSilent = true;
        }

        delete[] sTemp;
    }
    for (x = 0; x < vPDBsToLoad.size(); x++)
    {
        OpenPDB(vPDBsToLoad[x].c_str(), !bSilent);
    }
    for (x = 0; x < vSourcesToLoad.size(); x++)
    {
        OpenCodeAddress(vSourcesToLoad[x].c_str(), 0, !bSilent);
    }
    if (!bSilent)
    {
        if (bPDBSpecified && vPDBsToLoad.size() == 0)
        {
            MessageBox(GetActiveWindow(), _T("No PDB file was found in the specified paths"), _T("ETViewer"), MB_ICONSTOP | MB_OK);
        }
        if (bSourceSpecified && vSourcesToLoad.size() == 0)
        {
            MessageBox(GetActiveWindow(), _T("No source file was found in the specified paths"), _T("ETViewer"), MB_ICONSTOP | MB_OK);
        }
        if (bFailedToOpenETL)
        {
            std::wstring sText = _T("Failed to open .etl file '");
            sText += sETLFile;
            sText += _T("'");
            MessageBox(GetActiveWindow(), sText.c_str(), _T("ETViewer"), MB_ICONSTOP | MB_OK);
        }
        if (!bValidLevel)
        {
            MessageBox(GetActiveWindow(), _T("The specified level is not valid, please use the following values\r\n\r\n\t-L:NONE\r\n\t-L:FATAL\r\n\t-L:ERROR\r\n\t-L:WARNING\r\n\t-L:INFORMATION\r\n\t-L:VERBOSE\r\n\t-L:RESERVED6\r\n\t-L:RESERVED7\r\n\t-L:RESERVED8\r\n\t-L:RESERVED9"), _T("ETViewer"), MB_ICONSTOP | MB_OK);
        }
    }
    if (bValidLevel)
    {
        m_pFrame->GetProviderTree()->SetAllProviderLevel(dwLevel);
    }
    return TRUE;
}

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

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

void CETViewerApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

void CETViewerApp::UpdateHighLightFilters()
{
    for (auto& entry : m_HighLightFilters)
    {
        entry.UpdateObjects();
    }
    if (m_pFrame && m_pFrame->GetTracePane())
    {
        m_pFrame->GetTracePane()->InvalidateRect(nullptr);
    }
}

void CETViewerApp::LookupError(const TCHAR* pText)
{
    m_pFrame->LookupError(pText);
}

void CETViewerApp::AddRecentSourceFile(const TCHAR* file)
{
    auto existingFile = std::find(m_RecentSourceFiles.begin(), m_RecentSourceFiles.end(), file);
    if (existingFile != m_RecentSourceFiles.end())
    {
        m_RecentSourceFiles.erase(existingFile);
    }

    m_RecentSourceFiles.push_front(file);
    RefreshRecentFilesMenus();
    if (m_RecentSourceFiles.size() >= RECENT_SOURCE_FILE_MAX)
    {
        m_RecentSourceFiles.pop_back();
    }
}

void CETViewerApp::AddRecentPDBFile(const TCHAR* file)
{
    auto existingFile = std::find(m_RecentPDBFiles.begin(), m_RecentPDBFiles.end(), file);
    if (existingFile != m_RecentPDBFiles.end())
    {
        m_RecentPDBFiles.erase(existingFile);
    }

    m_RecentPDBFiles.push_front(file);
    RefreshRecentFilesMenus();
    if (m_RecentPDBFiles.size() >= RECENT_PDB_FILE_MAX)
    {
        m_RecentPDBFiles.pop_back();
    }
}

void CETViewerApp::AddRecentLogFile(const TCHAR* file)
{
    auto existingFile = std::find(m_RecentLogFiles.begin(), m_RecentLogFiles.end(), file);
    if (existingFile != m_RecentLogFiles.end())
    {
        m_RecentLogFiles.erase(existingFile);
    }

    m_RecentLogFiles.push_front(file);
    RefreshRecentFilesMenus();
    if (m_RecentLogFiles.size() >= RECENT_LOG_FILE_MAX)
    {
        m_RecentLogFiles.pop_back();
    }
}

bool CETViewerApp::OpenPDB(const TCHAR* pFile, bool bShowErrorIfFailed)
{
    std::vector<CTraceProvider*> vProviders;
    eTraceReaderError eErrorCode = m_PDBReader.LoadFromPDB(pFile, &vProviders);
    if (eErrorCode == eTraceReaderError_Success)
    {
        AddRecentPDBFile(pFile);

        for (unsigned x = 0; x < vProviders.size(); x++)
        {
            CTraceProvider* pProvider = vProviders[x];
            if (pProvider)
            {
                if (!AddProvider(pProvider))
                {
                    if (bShowErrorIfFailed)
                    {
                        std::wstring sTemp = _T("The provider '");
                        sTemp += pProvider->GetComponentName().c_str();
                        sTemp += _T(" has already been loaded");
                        MessageBox(GetActiveWindow(), sTemp.c_str(), _T("ETViewer"), MB_ICONSTOP | MB_OK);
                    }
                    delete pProvider;
                }
            }
        }
    }
    else
    {
        if (bShowErrorIfFailed)
        {
            std::wstring sTemp = _T("Failed to load pdb file '");
            sTemp += pFile;
            sTemp += _T("'.\r\n\r\n");
            switch (eErrorCode)
            {
            case eTraceReaderError_NoMemory:
                sTemp += _T("Not enought memory.");
                break;
            case eTraceReaderError_SymEngineInitFailed:
                sTemp += _T("Failed to initialize the symbol engine.");
                break;
            case eTraceReaderError_FailedToEnumerateSymbols:
                sTemp += _T("Failed to enumerate PDB symbols.");
                break;
            case eTraceReaderError_SymEngineLoadModule:
                sTemp += _T("Failed to load module by the symbol engine.");
                break;
            case eTraceReaderError_NoProviderFoundInPDB:
                sTemp += _T("No provider was found in the PDB file.");
                break;
            case eTraceReaderError_DBGHelpNotFound:
                sTemp += _T("DBGHelp.dll was not found.");
                break;
            case eTraceReaderError_DBGHelpWrongVersion:
                sTemp += _T("Wrong version of DBGHelp.dll found (does not have the expected exports).");
                break;
            case eTraceReaderError_PDBNotFound:
                sTemp += _T("Cannot open the PDB file.");
                break;
            case eTraceReaderError_Generic:
                sTemp += _T("Generic error.");
                break;
            default:
                sTemp += _T("Unknown error");
                break;
            }
            MessageBox(GetActiveWindow(), sTemp.c_str(), _T("ETViewer"), MB_ICONSTOP | MB_OK);
        }
    }
    UpdateFileMonitor();
    return (eErrorCode == eTraceReaderError_Success);
}

bool CETViewerApp::OpenCodeAddress(const TCHAR* pFile, DWORD dwLine, bool bShowErrorIfFailed)
{
    bool result = false;
    std::wstring sFile = pFile;

    bool bAccesible = false;

    if (_taccess(sFile.c_str(), 0))
    {
        for (auto& directory : m_SourceDirectories)
        {
            const TCHAR* pRemainder = pFile;

            do
            {
                pRemainder = _tcschr(pRemainder, _T('\\'));
                if (pRemainder)
                {
                    std::wstring sTempFile = directory;
                    sTempFile += pRemainder;

                    if (_taccess(sTempFile.c_str(), 0) == 0)
                    {
                        sFile = sTempFile;
                        bAccesible = true;
                        break;
                    }
                    // Skip \\ for the next iteration
                    pRemainder = (pRemainder + 1);
                }
            }
            while (pRemainder);

            if (bAccesible)
            {
                break;
            }
        }
    }
    else
    {
        bAccesible = true;
    }

    if (!bAccesible)
    {
        return false;
    }

    result = m_SourceFileContainer.ShowFile(sFile.c_str(), dwLine, bShowErrorIfFailed);
    m_SourceFileContainer.BringWindowToTop();
    UpdateFileMonitor();
    return result;
}

bool CETViewerApp::OpenETL(const TCHAR* pFile)
{
    if (_tcscmp(pFile, _T("")) == 0) { return false; }

    m_pFrame->GetTracePane()->Clear();
    m_Controller.Stop();
    ProcessSessionChange();

    if (m_Controller.OpenLog(pFile, m_pFrame->GetTracePane()) != ERROR_SUCCESS)
    {
        m_Controller.StartRealTime(_T("ETVIEWER_SESSION"), m_pFrame->GetTracePane());
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

bool CETViewerApp::CreateETL(const TCHAR* pFile)
{
    if (_tcscmp(pFile, _T("")) == 0) { return false; }

    m_pFrame->GetTracePane()->Clear();
    m_Controller.Stop();
    ProcessSessionChange();

    if (m_Controller.CreateLog(pFile, m_pFrame->GetTracePane()) != ERROR_SUCCESS)
    {
        m_Controller.StartRealTime(_T("ETVIEWER_SESSION"), m_pFrame->GetTracePane());
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

    m_Controller.StartRealTime(_T("ETVIEWER_SESSION"), m_pFrame->GetTracePane());
    ProcessSessionChange();
}

void CETViewerApp::CloseSession()
{
    m_Controller.Stop();
    ProcessSessionChange();
}

void CETViewerApp::ProcessSessionChange()
{
    if (m_pFrame)
    {
        CProviderTree* pTree = m_pFrame->GetProviderTree();
        CETViewerView* pTraceViewer = m_pFrame->GetTracePane();
        m_pFrame->OnSessionTypeChanged();
        pTree->OnSessionTypeChanged();
        pTraceViewer->OnSessionTypeChanged();
    }
}

bool CETViewerApp::AddProvider(CTraceProvider* pProvider)
{
    CETViewerView* pTraceViewer = m_pFrame->GetTracePane();
    CProviderTree* pTree = m_pFrame->GetProviderTree();
    if (theApp.m_Controller.AddProvider(pProvider, pProvider->GetAllSupportedFlagsMask(), TRACE_LEVEL_VERBOSE))
    {
        //provider already exists, just added to that one.
        pTree->OnAddProvider(pProvider);
        pTraceViewer->OnAddProvider(pProvider);
        m_sProviders.insert(pProvider);
    }

    pTree->OnProvidersModified();
    pTraceViewer->OnProvidersModified();
    return true;
}

void CETViewerApp::RemoveProvider(CTraceProvider* pProvider)
{
    CProviderTree* pTree = m_pFrame->GetProviderTree();
    CETViewerView* pTraceViewer = m_pFrame->GetTracePane();
    theApp.m_Controller.RemoveProvider(pProvider);
    pTree->OnRemoveProvider(pProvider);
    pTraceViewer->OnRemoveProvider(pProvider);
    delete pProvider;
    m_sProviders.erase(pProvider);
    UpdateFileMonitor();
}

void CETViewerApp::RemoveAllProviders()
{
    CETViewerView* pTraceViewer = m_pFrame->GetTracePane();
    CProviderTree* pTree = m_pFrame->GetProviderTree();

    while (m_sProviders.size())
    {
        CTraceProvider* pProvider = *m_sProviders.begin();
        theApp.m_Controller.RemoveProvider(pProvider);
        pTree->OnRemoveProvider(pProvider);
        pTraceViewer->OnRemoveProvider(pProvider);
        m_sProviders.erase(pProvider);
    }
    UpdateFileMonitor();
}

void CETViewerApp::ReloadProvider(CTraceProvider* pProvider)
{
    CETViewerView* pTraceViewer = m_pFrame->GetTracePane();
    CProviderTree* pTree = m_pFrame->GetProviderTree();

    std::vector<CTraceProvider*> loadedProviders;
    CTracePDBReader reader;

    for (const auto sCurFileName : pProvider->GetFileList())
    {
        reader.LoadFromPDB(sCurFileName.c_str(), &loadedProviders);
    }

    unsigned x;
    bool bReplaced = false;
    for (x = 0; x < loadedProviders.size(); x++)
    {
        CTraceProvider* pLoadedProvider = loadedProviders[x];
        if (!pLoadedProvider)
        {
            continue;
        }
        if (!bReplaced && pLoadedProvider->GetGUID() == pProvider->GetGUID())
        {
            bReplaced = true;
            m_sProviders.erase(pProvider);
            m_Controller.ReplaceProvider(pProvider, pLoadedProvider);
            pTree->OnReplaceProvider(pProvider, pLoadedProvider);
            pTraceViewer->OnReplaceProvider(pProvider, pLoadedProvider);
            m_sProviders.insert(pLoadedProvider);
            delete pProvider;
        }
        else
        {
            delete pLoadedProvider;
        }
    }
    if (bReplaced)
    {
        pTree->OnProvidersModified();
        pTraceViewer->OnProvidersModified();
    }
}

void CETViewerApp::ReloadAllProviders()
{
    CETViewerView* pTraceViewer = m_pFrame->GetTracePane();
    CProviderTree* pTree = m_pFrame->GetProviderTree();

    std::set<std::wstring> sPDBsToLoad;
    std::set<std::wstring>::iterator iPDB;
    std::set<CTraceProvider*>::iterator iProvider;
    for (iProvider = m_sProviders.begin(); iProvider != m_sProviders.end(); iProvider++)
    {
        CTraceProvider* pProvider = *iProvider;
        for (const auto sCurFileName : pProvider->GetFileList())
        {
            sPDBsToLoad.insert(sCurFileName);
        }
    }
    bool bAnyReplaced = false;

    for (iPDB = sPDBsToLoad.begin(); iPDB != sPDBsToLoad.end(); iPDB++)
    {
        std::wstring sPDB = *iPDB;
        std::vector<CTraceProvider*> loadedProviders;
        CTracePDBReader reader;
        reader.LoadFromPDB(sPDB.c_str(), &loadedProviders);
        unsigned x;
        for (x = 0; x < loadedProviders.size(); x++)
        {
            CTraceProvider* pLoadedProvider = loadedProviders[x];
            if (!pLoadedProvider)
            {
                continue;
            }

            CTraceProvider* pExistingProvider = NULL;
            for (iProvider = m_sProviders.begin(); iProvider != m_sProviders.end(); iProvider++)
            {
                CTraceProvider* pProvider = *iProvider;
                if (pProvider->GetGUID() == pLoadedProvider->GetGUID())
                {
                    pExistingProvider = pProvider;
                    break;
                }
            }
            if (pExistingProvider)
            {
                bAnyReplaced = true;
                m_sProviders.erase(pExistingProvider);
                m_Controller.ReplaceProvider(pExistingProvider, pLoadedProvider);
                pTree->OnReplaceProvider(pExistingProvider, pLoadedProvider);
                pTraceViewer->OnReplaceProvider(pExistingProvider, pLoadedProvider);
                m_sProviders.insert(pLoadedProvider);
                delete pExistingProvider;
            }
            else
            {
                delete pLoadedProvider;
            }
        }
    }
    if (bAnyReplaced)
    {
        pTree->OnProvidersModified();
        pTraceViewer->OnProvidersModified();
    }
}

bool CETViewerApp::ReloadPDBProviders(std::wstring sFileName)
{
    CETViewerView* pTraceViewer = m_pFrame->GetTracePane();
    CProviderTree* pTree = m_pFrame->GetProviderTree();

    bool bModified = false;
    std::vector<CTraceProvider*> loadedProviders;
    std::set<CTraceProvider*>::iterator iProvider;
    CTracePDBReader reader;
    if (reader.LoadFromPDB(sFileName.c_str(), &loadedProviders) != eTraceReaderError_Success)
    {
        return false;
    }
    unsigned x;
    for (x = 0; x < loadedProviders.size(); x++)
    {
        CTraceProvider* pLoadedProvider = loadedProviders[x];
        if (!pLoadedProvider)
        {
            continue;
        }
        CTraceProvider* pExistingProvider = NULL;
        for (iProvider = m_sProviders.begin(); iProvider != m_sProviders.end(); iProvider++)
        {
            CTraceProvider* pProvider = *iProvider;
            if (pProvider && pProvider->GetGUID() == pLoadedProvider->GetGUID())
            {
                pExistingProvider = pProvider;
                break;
            }
        }
        if (pExistingProvider)
        {
            bModified = true;
            m_sProviders.erase(pExistingProvider);
            m_Controller.ReplaceProvider(pExistingProvider, pLoadedProvider);
            pTree->OnReplaceProvider(pExistingProvider, pLoadedProvider);
            pTraceViewer->OnReplaceProvider(pExistingProvider, pLoadedProvider);
            m_sProviders.insert(pLoadedProvider);
            delete pExistingProvider;
        }
        else
        {
            bModified = true;
            m_Controller.AddProvider(pLoadedProvider, pLoadedProvider->GetAllSupportedFlagsMask(), TRACE_LEVEL_VERBOSE);
            pTree->OnAddProvider(pLoadedProvider);
            pTraceViewer->OnAddProvider(pLoadedProvider);
            m_sProviders.insert(pLoadedProvider);
        }
    }

    if (bModified)
    {
        pTree->OnProvidersModified();
        pTraceViewer->OnProvidersModified();
    }
    return true;
}

bool CETViewerApp::FilterTrace(const TCHAR* pText)
{
    std::wstring text = pText;

    WaitForSingleObject(m_hInstantTraceMutex, INFINITE);

    // Text Filters must always be ordered by relevance, the first filter that matches the criteria
    // is the effective filter 

    for (auto& filter : m_ExcludeFilters)
    {
        if (filter == L"*")
        {
            ReleaseMutex(m_hInstantTraceMutex);
            return false;
        }

        if (text.find(filter) != std::wstring::npos)
        {
            ReleaseMutex(m_hInstantTraceMutex);
            return false;
        }
    }

    for (auto& filter : m_IncludeFilters)
    {
        if (filter == L"*")
        {
            ReleaseMutex(m_hInstantTraceMutex);
            return true;
        }

        if (text.find(filter) != std::wstring::npos)
        {
            ReleaseMutex(m_hInstantTraceMutex);
            return true;
        }
    }

    ReleaseMutex(m_hInstantTraceMutex);

    return false;
}

void CETViewerApp::RefreshRecentFilesMenus()
{
    CMenu* pLogFilesMenu = m_pFrame->GetMenu()->GetSubMenu(0)->GetSubMenu(9);
    CMenu* pSourceFilesMenu = m_pFrame->GetMenu()->GetSubMenu(0)->GetSubMenu(8);
    CMenu* pPDBFilesMenu = m_pFrame->GetMenu()->GetSubMenu(0)->GetSubMenu(7);

    if (pSourceFilesMenu == NULL || pPDBFilesMenu == NULL || pLogFilesMenu == NULL)
    {
        return;
    }
    
    while (pPDBFilesMenu->GetMenuItemCount())
    {
        pPDBFilesMenu->RemoveMenu(0, MF_BYPOSITION);
    }
    
    while (pSourceFilesMenu->GetMenuItemCount())
    {
        pSourceFilesMenu->RemoveMenu(0, MF_BYPOSITION);
    }
    
    while (pLogFilesMenu->GetMenuItemCount())
    {
        pLogFilesMenu->RemoveMenu(0, MF_BYPOSITION);
    }

    auto menuIndex = 0;
    for (auto& file : m_RecentPDBFiles)
    {
        pPDBFilesMenu->InsertMenu(menuIndex, MF_BYPOSITION, RECENT_PDB_FILE_BASE_INDEX + menuIndex, file.c_str());
        menuIndex++;
    }

    menuIndex = 0;
    for (auto& file : m_RecentSourceFiles)
    {
        pSourceFilesMenu->InsertMenu(menuIndex, MF_BYPOSITION, RECENT_SOURCE_FILE_BASE_INDEX + menuIndex, file.c_str());
        menuIndex++;
    }

    menuIndex = 0;
    for (auto& file : m_RecentLogFiles)
    {
        pLogFilesMenu->InsertMenu(menuIndex, MF_BYPOSITION, RECENT_LOG_FILE_BASE_INDEX + menuIndex, file.c_str());
        menuIndex++;
    }

    if (m_RecentPDBFiles.size() == 0)
    {
        pPDBFilesMenu->InsertMenu(0, MF_BYPOSITION, RECENT_PDB_FILE_BASE_INDEX, _T("<No Recent File>"));
    }
    if (m_RecentSourceFiles.size() == 0)
    {
        pSourceFilesMenu->InsertMenu(0, MF_BYPOSITION, RECENT_SOURCE_FILE_BASE_INDEX, _T("<No Recent File>"));
    }
    if (m_RecentLogFiles.size() == 0)
    {
        pLogFilesMenu->InsertMenu(0, MF_BYPOSITION, RECENT_LOG_FILE_BASE_INDEX, _T("<No Recent File>"));
    }
}

void CETViewerApp::OnRecentPDBFile(UINT nID)
{
    auto fileId = nID - RECENT_PDB_FILE_BASE_INDEX;
    if (m_RecentPDBFiles.size() < fileId)
    {
        return;
    }

    auto file = m_RecentPDBFiles.begin();

    // TODO: use random access container
    std::advance(file, fileId);

    m_pFrame->OpenFile(file->c_str(), NULL);
}

void CETViewerApp::OnRecentSourceFile(UINT nID)
{
    auto fileId = nID - RECENT_SOURCE_FILE_BASE_INDEX;
    if (m_RecentSourceFiles.size() < fileId)
    {
        return;
    }

    auto file = m_RecentSourceFiles.begin();

    // TODO: use random access container
    std::advance(file, fileId);

    OpenCodeAddress(file->c_str(), 0, true);
}

void CETViewerApp::OnRecentLogFile(UINT nID)
{
    auto fileId = nID - RECENT_LOG_FILE_BASE_INDEX;
    if (m_RecentLogFiles.size() < fileId)
    {
        return;
    }

    auto file = m_RecentLogFiles.begin();
    
    // TODO: use random access container
    std::advance(file, fileId);

    m_pFrame->OpenFile(file->c_str(), NULL);
}

void CETViewerApp::OnClose()
{
    PersistentSettings settings;

    settings.WriteMultiStringValue(L"ExcludeFilter", m_ExcludeFilters);
    settings.WriteMultiStringValue(L"IncludeFilter", m_IncludeFilters);

    settings.WriteMultiStringValue(L"RecentLogFiles", m_RecentLogFiles);
    settings.WriteMultiStringValue(L"RecentSourceFiles", m_RecentSourceFiles);
    settings.WriteMultiStringValue(L"RecentPDBFiles", m_RecentPDBFiles);
    settings.WriteMultiStringValue(L"SourceDirectories", m_SourceDirectories);

    settings.WriteBoolValue(L"AssociateETL", m_bAssociateETL);
    settings.WriteBoolValue(L"AssociatePDB", m_bAssociatePDB);
    settings.WriteBoolValue(L"AssociateSources", m_bAssociateSources);

    settings.WriteDwordValue(L"PDBMonitoringMode", m_ePDBMonitoringMode);
    settings.WriteDwordValue(L"SourceMonitoringMode", m_eSourceMonitoringMode);

    std::list<std::wstring> highLightFilters;
    for (auto& filter : m_HighLightFilters)
    {
        highLightFilters.push_back(filter.ToString());
    }
    settings.WriteMultiStringValue(L"HightlightFilter", highLightFilters);

    if (m_hSingleInstanceMutex)
    {
        CloseHandle(m_hSingleInstanceMutex);
        m_hSingleInstanceMutex = NULL;
    }

    CloseSession();
}

int CETViewerApp::ExitInstance()
{
    int res = CWinApp::ExitInstance();
    CoUninitialize();
    return res;
}

void CETViewerApp::UpdateFileAssociations()
{
    if (m_bAssociatePDB)
    {
        SetFileAssociation(L".pdb", L"pdbfile", L"Program Database", L"-pdb:\"%1\"");
    }

    if (m_bAssociateETL)
    {
        SetFileAssociation(L".etl", L"etlfile", L"Event Tracing for Windows (ETW) Log", L"-etl:\"%1\"");
    }
    
    if (m_bAssociateSources)
    {
        SetFileAssociation(L".cpp", L"cppfile", L"C++ Source File", L"-src:\"%1\"");
        SetFileAssociation(L".c", L"cfile", L"C Source File", L"-src:\"%1\"");
        SetFileAssociation(L".h", L"hfile", L"Header File", L"-src:\"%1\"");
        SetFileAssociation(L".inl", L"inlfile", L"Inline File", L"-src:\"%1\"");
    }
}

void CETViewerApp::OnFileChanged(std::wstring sFile)
{
    AddFileChangeOperation(sFile);
}

void CETViewerApp::AddFileChangeOperation(std::wstring sFileName)
{
    WaitForSingleObject(m_hPendingFileChangesMutex, INFINITE);
    bool bFound = false;
    std::list<SPendingFileChangeOperation>::iterator i;
    for (i = m_lPendingFileChanges.begin(); i != m_lPendingFileChanges.end(); i++)
    {
        SPendingFileChangeOperation op = *i;
        if (op.sFileName == sFileName)
        {
            bFound = true;
            break;
        }
    }
    if (!bFound)
    {
        SPendingFileChangeOperation op;
        op.sFileName = sFileName;
        op.dwChangeTime = GetTickCount();
        m_lPendingFileChanges.push_back(op);
    }
    ReleaseMutex(m_hPendingFileChangesMutex);
}

void CETViewerApp::RemoveFileChangeOperation(std::wstring sFileName)
{
    WaitForSingleObject(m_hPendingFileChangesMutex, INFINITE);

    std::list<SPendingFileChangeOperation>::iterator i;
    for (i = m_lPendingFileChanges.begin(); i != m_lPendingFileChanges.end();)
    {
        SPendingFileChangeOperation op = *i;
        if (op.sFileName == sFileName)
        {
            i = m_lPendingFileChanges.erase(i);
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
    WaitForSingleObject(m_hPendingFileChangesMutex, INFINITE);

    std::list<SPendingFileChangeOperation>::iterator i;
    for (i = m_lPendingFileChanges.begin(); i != m_lPendingFileChanges.end();)
    {
        SPendingFileChangeOperation op = *i;
        if ((op.dwChangeTime + FILE_CHANGE_EXPIRATION_TIME) < GetTickCount())
        {
            i = m_lPendingFileChanges.erase(i);
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
    if (m_bCheckingFileChangeOperations) { return; }
    m_bCheckingFileChangeOperations = true;

    if (m_eSourceMonitoringMode == eFileMonitoringMode_AutoReload ||
        m_ePDBMonitoringMode == eFileMonitoringMode_AutoReload)
    {
        RemoveExpiredFileChangeOperations();
    }

    WaitForSingleObject(m_hPendingFileChangesMutex, INFINITE);

    bool bRetryOperation = false;

    std::list<SPendingFileChangeOperation>::iterator i;
    for (i = m_lPendingFileChanges.begin(); i != m_lPendingFileChanges.end();)
    {
        SPendingFileChangeOperation op = *i;
        TCHAR sExt[MAX_PATH] = { 0 };
        _tsplitpath_s(op.sFileName.c_str(), NULL, 0, NULL, 0, NULL, 0, sExt, MAX_PATH);
        bool bRemoveOperation = false;
        bool bIsPDB = (_tcsicmp(sExt, _T(".PDB")) == 0);
        bool bMustAsk = (bIsPDB && m_ePDBMonitoringMode == eFileMonitoringMode_Ask) ||
            (!bIsPDB && m_eSourceMonitoringMode == eFileMonitoringMode_Ask);

        if (!bRetryOperation)
        {
            if (bMustAsk)
            {
                std::wstring sQuestion;
                sQuestion = _T("File '");
                sQuestion += op.sFileName;
                sQuestion += _T("' has changed.\r\n\r\nDo you want to reload it?");

                if (MessageBox(m_pFrame->m_hWnd, sQuestion.c_str(), _T("ETViewer"), MB_ICONQUESTION | MB_YESNO) == IDNO)
                {
                    bRemoveOperation = true;
                }
            }
        }

        bRetryOperation = false;

        if (!bRemoveOperation)
        {
            bool bReloadOk = true;

            if (bIsPDB)
            {
                bReloadOk = ReloadPDBProviders(op.sFileName.c_str());
            }
            else
            {
                m_SourceFileContainer.ReloadFile(op.sFileName.c_str());
            }

            if (bReloadOk)
            {
                bRemoveOperation = true;
            }
            else
            {
                if (bMustAsk)
                {
                    std::wstring sQuestion;
                    sQuestion = _T("File '");
                    sQuestion += op.sFileName;
                    sQuestion += _T("' cannot be reloaded.\r\n\r\nDo you want to retry?");

                    if (MessageBox(m_pFrame->m_hWnd, sQuestion.c_str(), _T("ETViewer"), MB_ICONEXCLAMATION | MB_RETRYCANCEL) == IDRETRY)
                    {
                        bRetryOperation = true;
                    }
                    else
                    {
                        bRemoveOperation = true;
                    }
                }
            }
        }

        if (bRemoveOperation)
        {
            i = m_lPendingFileChanges.erase(i);
        }
        else if (!bRetryOperation)
        {
            i++;
        }
    }

    ReleaseMutex(m_hPendingFileChangesMutex);

    m_bCheckingFileChangeOperations = false;
};

void CETViewerApp::UpdateFileMonitor()
{
    std::set<std::wstring> sFiles;

    if (m_eSourceMonitoringMode != eFileMonitoringMode_None &&
        m_eSourceMonitoringMode != eFileMonitoringMode_Ignore)
    {
        m_SourceFileContainer.GetFiles(&sFiles);
    }

    if (m_ePDBMonitoringMode != eFileMonitoringMode_None &&
        m_ePDBMonitoringMode != eFileMonitoringMode_Ignore)
    {
        std::set<CTraceProvider*>::iterator i;
        for (auto curProvider : m_sProviders)
        {
            for (const auto curFileName : curProvider->GetFileList())
            {
                sFiles.insert(curFileName);
            }
        }
    }

    m_pFileMonitor->SetFiles(&sFiles);
}
