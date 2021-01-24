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

class CSourceFileContainer;

#include "FindDialog.h"

class CSourceFileViewer : public CDialog, public CFindDialogClient
{
public:
    CSourceFileViewer(CSourceFileContainer* pParent = NULL);

    void ShowLine(int line);
    std::wstring GetFile();

    //{{AFX_DATA(CSourceFileViewer)
    enum { IDD = IDD_SOURCE_FILE_VIEWER };
    //}}AFX_DATA

    DWORD OpenFile(const TCHAR* pFile, int line, bool bShowErrorIfFailed = true);
    void Reload();
    bool FindNext(const TCHAR* pText);
    void Copy();
    void ShowFindDialog();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    bool FindAndDeleteAll(const TCHAR* pText);
    bool FindAndMarkAll(const TCHAR* pText);
    void SetFocusOnOwnerWindow();

    static LRESULT CALLBACK FileEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SetMetrics();
    void UpdateLine();
    void OnFind();

protected:
    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnUpdateSelectedLine();

    DECLARE_MESSAGE_MAP()

// TODO: make private
public:
    DWORD m_OldEditProc;
    HFONT m_hFileFont;
    CHAR* m_pFileBuffer;
    CHAR* m_pFileBufferUpper;
    TCHAR* m_pFileBufferWide;
    DWORD m_FileBufferLength;

    TCHAR m_SourceFile[MAX_PATH];
    int m_SourceLine;
    CSourceFileContainer* m_pContainer;

    CEdit m_EDLine;
    CEdit m_EDFullPath;
    CRichEditCtrl m_EDFile;
};