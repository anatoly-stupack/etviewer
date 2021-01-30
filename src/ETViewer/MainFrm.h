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

#include "MainDialogBar.h"
#include "FilterDialogBar.h"

class CETViewerView;
class CProviderTree;
class CHighLightPane;
class CMainFrame : public CFrameWnd
{
protected:
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

public:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual ~CMainFrame();

    CETViewerView* GetTracePane();
    CProviderTree* GetProviderTree();
    CHighLightPane* GetHighLightPane();

    bool OpenFile(const TCHAR* pFile, bool* pbKnownFileType);
    void OnSessionTypeChanged();

protected:
    bool IsAncestorOf(HWND hWnd, HWND hAncestor);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnDestroy();
    afx_msg void OnStartStop();
    afx_msg void OnOpenFile();
    afx_msg void OnShowSourceContainer();
    afx_msg void OnClear();
    afx_msg void OnClearSelected();
    afx_msg void OnHighlightFilters();
    afx_msg void OnErrorLookup();
    afx_msg void OnEnsureVisible();
    afx_msg void OnSave();
    afx_msg void OnFind();
    afx_msg void OnOk();
    afx_msg void OnCancel();
    afx_msg void OnDropFiles(HDROP hDropInfo);
    afx_msg void OnCloseLog();
    afx_msg void OnUpdateFileCloselog(CCmdUI* pCmdUI);
    afx_msg void OnFileLogtofile();
    afx_msg void OnUpdateFileLogtofile(CCmdUI* pCmdUI);
    afx_msg void OnFileStoploggintofile();
    afx_msg void OnUpdateFileStoploggintofile(CCmdUI* pCmdUI);
    afx_msg LRESULT OnIPCCommand(WPARAM wParam, LPARAM lParam);
    afx_msg void OnEditSettings();
    afx_msg void OnTimer(UINT nTimerId);

private:
    CSplitterWnd m_wndSplitter;
    CReBar m_wndReBar;
    CMainDialogBar m_MainDialogBar;
    CFilterDialogBar m_FilterDialogBar;
};