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

#ifndef __AFXWIN_H__
	#error incluye 'stdafx.h' antes de incluir este archivo para PCH
#endif

#include "resource.h"       // Símbolos principales
#include "SourceFileContainer.h"       // Símbolos principales
#include "TraceController.h"      
#include "MainFrm.h"      
#include "FileMonitor.h"      

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
	eETViewerColumn_BASE=0x0E000000,
	eETViewerColumn_Text=eETViewerColumn_BASE,
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

class CHightLightFilter
{
	string				m_Text;
	DWORD				m_dwTextLen;
	DWORD				m_TextColor;
	DWORD				m_BkColor;
	bool				m_bEnabled;
	HPEN				m_hPen;
	HBRUSH				m_hBrush;

public:


	HPEN		GetPen(){return m_hPen;}
	HBRUSH		GetBrush(){return m_hBrush;}

	COLORREF	GetTextColor(){return m_TextColor;}
	void		SetTextColor(COLORREF textColor){m_TextColor=textColor;UpdateObjects();}
	COLORREF	GetBkColor(){return m_BkColor;}
	void		SetBkColor(COLORREF bkColor){m_BkColor=bkColor;UpdateObjects();}
	void		SetEnabled(bool bEnable){m_bEnabled=bEnable;}
	bool		GetEnabled(){return m_bEnabled;}

	void		SetText(string sText){m_Text=sText;m_dwTextLen=(DWORD)strlen(m_Text.c_str());}
	string		&GetText(){return m_Text;}
	DWORD		GetTextLen(){return m_dwTextLen;}

	CHightLightFilter(const CHightLightFilter &otherFilter)
	{
		m_Text=otherFilter.m_Text;
		m_TextColor=otherFilter.m_TextColor;
		m_BkColor=otherFilter.m_BkColor;
		m_bEnabled=otherFilter.m_bEnabled;
		m_dwTextLen=otherFilter.m_dwTextLen;
		UpdateObjects();
	}
	CHightLightFilter &operator = (CHightLightFilter &otherFilter)
	{
		m_Text=otherFilter.m_Text;
		m_TextColor=otherFilter.m_TextColor;
		m_BkColor=otherFilter.m_BkColor;
		m_bEnabled=otherFilter.m_bEnabled;
		m_dwTextLen=otherFilter.m_dwTextLen;
		UpdateObjects();
		return *this;
	}

	CHightLightFilter()
	{
		m_Text[0]=0;
		m_bEnabled=true;
		m_dwTextLen=0;
		m_hPen=NULL;
		m_hBrush=NULL;
		SetTextColor(RGB(0,0,0));
		SetBkColor(RGB(255,255,255));
	}
	~CHightLightFilter()
	{
		if(m_hPen){DeleteObject(m_hPen);m_hPen=NULL;}
		if(m_hBrush){DeleteObject(m_hBrush);m_hBrush=NULL;}
	}

	void UpdateObjects()
	{
		if(m_hPen){DeleteObject(m_hPen);m_hPen=NULL;}
		if(m_hBrush){DeleteObject(m_hBrush);m_hBrush=NULL;}

		POINT	 P={1,1};
		LOGPEN   LP={0};
		LP.lopnColor=m_BkColor;
		LP.lopnWidth=P;
		LP.lopnStyle=PS_SOLID;

		LOGBRUSH LB={0};
		LB.lbColor=m_BkColor;
		LB.lbStyle=BS_SOLID;

		m_hBrush=CreateBrushIndirect(&LB);
		m_hPen=CreatePenIndirect(&LP);
	}

	BEGIN_PERSIST_MAP(CHightLightFilter)
		PERSIST(m_Text		,"Text")
		PERSIST(m_dwTextLen	,"TextLenght")
		PERSIST(m_TextColor	,"TextColor")
		PERSIST(m_BkColor	,"BkColor")
		PERSIST(m_bEnabled	,"Enabled")
	END_PERSIST_MAP()

};

DECLARE_SERIALIZABLE(CHightLightFilter)

class CFilter
{
public:

	string		m_Text;
	DWORD		m_dwTextLen;
	bool		m_bInclusionFilter;

	CFilter()
	{
		m_Text[0]=0;
		m_bInclusionFilter=true;
		m_dwTextLen=0;
	}
	~CFilter(){}
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
	string sFileName;
	DWORD  dwChangeTime; // Tick count
};

DECLARE_SERIALIZABLE_ENUMERATION(eFileMonitoringMode);

class CETViewerApp : public CWinApp, public IFileMonitorCallback
{
public:
	CETViewerApp();
	~CETViewerApp();

	set<CTraceProvider *> m_sProviders;
	CTracePDBReader		  m_PDBReader;

	HANDLE m_hInstantTraceMutex;
	HANDLE m_hSingleInstanceMutex;

	list<SPendingFileChangeOperation> m_lPendingFileChanges;
	HANDLE							  m_hPendingFileChangesMutex;
	bool							  m_bCheckingFileChangeOperations;

	void OnClose();

// Reemplazos
public:

	void AddFileChangeOperation(string sFileName);
	void RemoveFileChangeOperation(string sFileName);
	void RemoveExpiredFileChangeOperations();
	void CheckFileChangeOperations();

	void SetFileAssociation(char *pExtension,char *pFileTypeName,char *pFileDescription,char *pCommandLineTag);

	virtual BOOL InitInstance();

	string					 m_sConfigFile;
	CConfigFile				 m_ConfigFile;

	deque<CHightLightFilter> m_HighLightFilters;
	deque<CHightLightFilter> m_SplittedHighLightFilters;
	deque<std::string>		 m_RecentSourceFiles;
	deque<std::string>		 m_RecentPDBFiles;
	deque<std::string>		 m_RecentLogFiles;
	deque<std::string>		 m_SourceDirectories;

	deque<CFilter>			m_dSplittedInstantFilters;

	string 					m_InstantIncludeFilter;
	string 					m_InstantExcludeFilter;
	deque<std::string>		m_InstantIncludeFilterList;
	deque<std::string>		m_InstantExcludeFilterList;
	bool					m_bAssociateETL;
	bool					m_bAssociatePDB;
	bool					m_bAssociateSources;

	eFileMonitoringMode		m_ePDBMonitoringMode;
	eFileMonitoringMode		m_eSourceMonitoringMode;

	CSourceFileContainer m_SourceFileContainer;
	CTraceController	 m_Controller;
	CMainFrame			*m_pFrame;
	CFileMonitor		*m_pFileMonitor;

	void RefreshRecentFilesMenus();

	void UpdateHighLightFilters();
	void UpdateInstantFilters();

	bool AddProvider(CTraceProvider *pProvider);
	void ReloadProvider(CTraceProvider *pProvider);
	bool ReloadPDBProviders(string sFileName);
	void ReloadAllProviders();
	void RemoveProvider(CTraceProvider *pProvider);
	void RemoveAllProviders();

	void AddRecentSourceFile(const char *pFile);
	void AddRecentPDBFile(const char *pFile);
	void AddRecentLogFile(const char *pFile);
	bool OpenCodeAddress(const char *pFile,DWORD dwLine,bool bShowErrorIfFailed);
	bool OpenPDB(const char *pFile,bool bShowErrorIfFailed);
	bool OpenETL(const char *pFile);
	bool CreateETL(const char *pFile);
	void CloseETL();
	void CloseSession();
	void UpdateFileMonitor();
	void UpdateFileAssociations();
	void ProcessSessionChange();
	BOOL ProcessCommandLine(int argc,char **argv);

	void LookupError(const char *pText);
	bool FilterTrace(const char *pText);

	// IFileMonitorCallback
	void OnFileChanged(string sFile);

	BEGIN_PERSIST_MAP(CETViewerApp)
		PERSIST(m_InstantIncludeFilter,"IncludeFilter")
		PERSIST(m_InstantExcludeFilter,"ExcludeFilter")
		PERSIST(m_InstantIncludeFilterList,"IncludeFilterList")
		PERSIST(m_InstantExcludeFilterList,"ExcludeFilterList")
		PERSIST(m_HighLightFilters,"HighLightFilters")
		PERSIST(m_RecentSourceFiles,"RecentSourceFiles")
		PERSIST(m_RecentPDBFiles,"RecentPDBFiles")
		PERSIST_FLAGS(m_RecentLogFiles,"m_RecentLogFiles",PF_NORMAL|PF_OPTIONAL)
		PERSIST_FLAGS(m_SourceDirectories,"SourceDirectories",PF_NORMAL|PF_OPTIONAL)
		PERSIST_VALUE_FLAGS(m_bAssociateETL,"AssociateETL",false,PF_NORMAL|PF_OPTIONAL)
		PERSIST_VALUE_FLAGS(m_bAssociatePDB,"AssociatePDB",false,PF_NORMAL|PF_OPTIONAL)
		PERSIST_VALUE_FLAGS(m_bAssociateSources,"AssociateSources",false,PF_NORMAL|PF_OPTIONAL)
		PERSIST_VALUE_FLAGS(m_ePDBMonitoringMode,"PDBMonitoringMode",eFileMonitoringMode_AutoReload,PF_NORMAL|PF_OPTIONAL)
		PERSIST_VALUE_FLAGS(m_eSourceMonitoringMode,"SourceMonitoringMode",eFileMonitoringMode_AutoReload,PF_NORMAL|PF_OPTIONAL)
	END_PERSIST_MAP();

	DECLARE_CONFIG_FILE_MEDIA();
// Implementación
	afx_msg void OnAppAbout();
	afx_msg void OnRecentPDBFile(UINT nID);
	afx_msg void OnRecentSourceFile(UINT nID);
	afx_msg void OnRecentLogFile(UINT nID);
	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern CETViewerApp theApp;
