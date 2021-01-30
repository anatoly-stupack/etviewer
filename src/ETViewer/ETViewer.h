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

#pragma once

#include "resource.h"
#include "SourceFileContainer.h"
#include "TraceController.h"
#include "MainFrm.h"
#include "FileMonitor.h"
#include "HighLightFilter.h"

#define RECENT_PDB_FILE_BASE_INDEX					3000
#define RECENT_PDB_FILE_MAX							15
#define RECENT_SOURCE_FILE_BASE_INDEX				4000
#define RECENT_SOURCE_FILE_MAX						15
#define RECENT_LOG_FILE_BASE_INDEX					5000
#define RECENT_LOG_FILE_MAX							15

#define FILE_CHANGE_TIMER_ID						1
#define FILE_CHANGE_EXPIRATION_TIME 5000
#define FILE_CHANGE_TIMER_PERIOD	500

enum
{
    eETViewerColumn_BASE = 0x0E000000,
    eETViewerColumn_Text = eETViewerColumn_BASE,
    eETViewerColumn_PIDTID,
    eETViewerColumn_Time,
    eETViewerColumn_TimeStamp,
    eETViewerColumn_Index,
    eETViewerColumn_Source,
    eETViewerColumn_Component,
    eETViewerColumn_Method,
    eETViewerColumn_Level,
    eETViewerColumn_Flag
};

enum
{
    eETViewerSortDirection_Ascending,
    eETViewerSortDirection_Descending,
};

enum eFileMonitoringMode
{
    eFileMonitoringMode_None,
    eFileMonitoringMode_AutoReload,
    eFileMonitoringMode_Ignore,
    eFileMonitoringMode_Ask
};

struct SPendingFileChangeOperation
{
    std::wstring sFileName;
    DWORD  dwChangeTime; // Tick count
};

class CETViewerApp : public CWinApp, public IFileMonitorCallback
{
public:
    CETViewerApp();
    ~CETViewerApp();

    CSourceFileContainer* GetSourceFileContainer();
    CTraceController* GetTraceController();
    CMainFrame* GetMainFrame();

    eFileMonitoringMode GetSourceMonitoringMode() const;
    eFileMonitoringMode GetPdbMonitoringMode() const;

    void SetSourceMonitoringMode(eFileMonitoringMode mode);
    void SetPdbMonitoringMode(eFileMonitoringMode mode);

    bool GetAssociatePdb() const;
    bool GetAssociateSources() const;
    bool GetAssociateEtl() const;

    void SetAssociatePdb(bool enable);
    void SetAssociateSources(bool enable);
    void SetAssociateEtl(bool enable);

    std::list<std::wstring> GetRecentSourceFiles() const;
    std::list<std::wstring> GetSourceDirectories() const;
    std::list<std::wstring> GetExcludeFilters() const;
    std::list<std::wstring> GetIncludeFilters() const;
    std::list<CHighLightFilter> GetHighLightFilters() const;

    void SetSourceDirectories(std::list<std::wstring>&& list);
    void SetExcludeFilters(std::list<std::wstring>&& list);
    void SetIncludeFilters(std::list<std::wstring>&& list);
    void SetHighLightFilters(std::list<CHighLightFilter>&& list);

    void OnClose();

    void AddFileChangeOperation(std::wstring sFileName);
    void RemoveFileChangeOperation(std::wstring sFileName);
    void RemoveExpiredFileChangeOperations();
    void CheckFileChangeOperations();

    void SetFileAssociation(TCHAR* pExtension, TCHAR* pFileTypeName, TCHAR* pFileDescription, TCHAR* pCommandLineTag);

    virtual BOOL InitInstance();
    virtual int ExitInstance();

    void RefreshRecentFilesMenus();

    void UpdateHighLightFilters();

    bool AddProvider(CTraceProvider* pProvider);
    void ReloadProvider(CTraceProvider* pProvider);
    bool ReloadPDBProviders(std::wstring sFileName);
    void ReloadAllProviders();
    void RemoveProvider(CTraceProvider* pProvider);
    void RemoveAllProviders();

    void AddRecentSourceFile(const TCHAR* pFile);
    void AddRecentPDBFile(const TCHAR* pFile);
    void AddRecentLogFile(const TCHAR* pFile);
    bool OpenCodeAddress(const TCHAR* pFile, DWORD dwLine, bool bShowErrorIfFailed);
    bool OpenPDB(const TCHAR* pFile, bool bShowErrorIfFailed);
    bool OpenETL(const TCHAR* pFile);
    bool CreateETL(const TCHAR* pFile);
    void CloseETL();
    void CloseSession();
    void UpdateFileMonitor();
    void UpdateFileAssociations();
    void ProcessSessionChange();
    BOOL ProcessCommandLine(int argc, TCHAR** argv);

    void LookupError(const std::wstring& errorText);
    bool FilterTrace(const TCHAR* pText);

    // IFileMonitorCallback
    void OnFileChanged(std::wstring sFile);

    afx_msg void OnAppAbout();
    afx_msg void OnRecentPDBFile(UINT nID);
    afx_msg void OnRecentSourceFile(UINT nID);
    afx_msg void OnRecentLogFile(UINT nID);

    DECLARE_MESSAGE_MAP()

private:
    std::set<CTraceProvider*> m_sProviders;
    CTracePDBReader m_PDBReader;

    HANDLE m_hInstantTraceMutex;
    HANDLE m_hSingleInstanceMutex;
    HANDLE m_hPendingFileChangesMutex;

    std::list<SPendingFileChangeOperation> m_lPendingFileChanges;
    bool m_bCheckingFileChangeOperations;

    std::list<std::wstring> m_RecentSourceFiles;
    std::list<std::wstring> m_RecentPDBFiles;
    std::list<std::wstring> m_RecentLogFiles;
    std::list<std::wstring> m_SourceDirectories;

    mutable std::mutex m_FiltersGuard;
    std::list<std::wstring>	m_IncludeFilters;
    std::list<std::wstring>	m_ExcludeFilters;
    std::list<CHighLightFilter> m_HighLightFilters;

    bool m_bAssociateETL;
    bool m_bAssociatePDB;
    bool m_bAssociateSources;

    eFileMonitoringMode	m_ePDBMonitoringMode;
    eFileMonitoringMode	m_eSourceMonitoringMode;

    CSourceFileContainer m_SourceFileContainer;
    CTraceController m_Controller;
    CMainFrame* m_pFrame;
    CFileMonitor* m_pFileMonitor;
};

extern CETViewerApp theApp;
