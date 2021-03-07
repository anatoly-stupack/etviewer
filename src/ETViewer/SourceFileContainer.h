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

#include "SourceFileViewer.h"

class CSourceFileContainer : public CDialog
{
public:
    CSourceFileContainer(CWnd* pParent = NULL);

    //{{AFX_DATA(CSourceFileContainer)
    enum { IDD = IDD_SOURCE_FILE_CONTAINER };
    //}}AFX_DATA

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    CSourceFileViewer* GetViewerAt(int index);

public:
    void GetFiles(std::set<std::wstring>* psFiles);
    void ReloadFile(const TCHAR* sFile);

    bool ShowFile(const TCHAR* pFile, int line, bool bShowErrorIfFailed);
    void BrowseForAndShowFile();
    void ShowSelectedFile();
    void SetMetrics();

    void SelectNext();
    void SelectPrevious();

protected:
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnCloseFile();
    afx_msg void OnSourceFileChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnOpenFile();
    afx_msg void OnCopy();
    afx_msg void OnFind();
    afx_msg void OnRecentFile();

    DECLARE_MESSAGE_MAP()
public:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

private:
    CButton	m_BTRecentFiles;
    CButton	m_BTFind;
    CButton	m_BTCopy;
    CButton	m_BTOpenFile;
    CTabCtrl m_TCSourceFiles;
    CButton	m_BTCloseFile;
};