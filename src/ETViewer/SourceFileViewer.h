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

class CSourceFileViewer : public CDialog, public IFindDialogClient
{
public:
    CSourceFileViewer(CSourceFileContainer* parent);

    void ShowLine(int line);
    std::wstring GetFile();

    DWORD OpenFile(const std::wstring& filePath, int line, bool bShowErrorIfFailed);
    void Reload();

    void Copy();
    void ShowFindDialog();
    void SetFocusOnEditor();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

    // IFindDialogClient
    virtual bool FindNext(const std::wstring& text, bool findDirectionUp, bool matchCase);
    virtual bool FindAndDeleteAll(const std::wstring& text, bool findDirectionUp, bool matchCase);
    virtual bool FindAndMarkAll(const std::wstring& text, bool findDirectionUp, bool matchCase);
    virtual void SetFocusOnOwnerWindow();

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

private:
    enum { IDD = IDD_SOURCE_FILE_VIEWER };

    WNDPROC m_OldEditProc;
    HFONT m_hFileFont;

    std::wstring m_FileContent;
    std::wstring m_FileContentUpperCase;

    std::wstring m_SourceFile;
    int m_SourceLine;
    
    CSourceFileContainer* m_pContainer;

    CEdit m_EDLine;
    CEdit m_EDFullPath;
    CRichEditCtrl m_EDFile;

    CFindDialog* m_FindDialog;
    bool m_FindDirectionUp;
    bool m_FindMatchCase;
    std::wstring m_LastTextToFind;
};