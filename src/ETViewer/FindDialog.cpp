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
#include "FindDialog.h"

#define MAX_FIND_TEXTS 20

IMPLEMENT_DYNAMIC(CFindDialog, CFindReplaceDialog)

CFindDialog::CFindDialog(IFindDialogClient* findClient, HWND owner, bool extendedMode)
    : m_FindClient(findClient)
    , m_bFindDirectionUp(false)
    , m_bMatchCaseInFind(false)
    , m_bHideTracingOptions(!extendedMode)
    , m_bHideDeleteButtons(!extendedMode)
    , m_bHideMarkButtons(!extendedMode)
    , m_bFindInPIDName(false)
    , m_bFindInTraceText(true)
{
    m_fr.hwndOwner = owner;
    m_fr.Flags |= FR_HIDEWHOLEWORD | FR_ENABLETEMPLATE;
    m_fr.lpstrFindWhat = const_cast<wchar_t*>(m_LastTextToFind.c_str());
    m_fr.hInstance = AfxGetResourceHandle();
    m_fr.lpTemplateName = MAKEINTRESOURCE(IDD_FIND_DIALOG);
}

void CFindDialog::DoDataExchange(CDataExchange* pDX)
{
    CFindReplaceDialog::DoDataExchange(pDX);

    DDX_Control(pDX, 1056, m_BTUp);
    DDX_Control(pDX, 1057, m_BTDown);
    DDX_Control(pDX, IDOK, m_BTFind);
    DDX_Control(pDX, 1041, m_CBMatchCase);
    DDX_Control(pDX, IDC_BT_DELETE_ALL, m_BTDeleteAll);
    DDX_Control(pDX, IDC_BT_MARK_ALL, m_BTMarkAll);
    DDX_Control(pDX, IDC_CB_FIND_PID_NAME, m_CBFindInPIDName);
    DDX_Control(pDX, IDC_CB_FIND_IN_TRACE_TEXT, m_CBFindInTraceText);
    DDX_Control(pDX, IDC_CO_TEXT_TO_FIND, m_COTextToFind);
}

BEGIN_MESSAGE_MAP(CFindDialog, CFindReplaceDialog)
    ON_CBN_EDITCHANGE(IDC_CO_TEXT_TO_FIND, OnChangedText)
    ON_CBN_SELCHANGE(IDC_CO_TEXT_TO_FIND, OnTextSelected)
    ON_BN_CLICKED(IDOK, OnFind)
    ON_BN_CLICKED(IDCANCEL, OnCancel)
    ON_BN_CLICKED(IDC_BT_DELETE_ALL, OnDeleteAll)
    ON_BN_CLICKED(IDC_BT_MARK_ALL, OnMarkAll)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

void CFindDialog::OnDeleteAll()
{
    if (!UpdateOptions())
    {
        return;
    }

    if (m_FindClient->FindAndDeleteAll(m_LastTextToFind, m_bFindDirectionUp, m_bMatchCaseInFind))
    {
        m_FindClient->SetFocusOnOwnerWindow();
        EndDialog(IDOK);
    }
}

void CFindDialog::OnMarkAll()
{
    if (!UpdateOptions())
    {
        return;
    }

    if (m_FindClient->FindAndMarkAll(m_LastTextToFind, m_bFindDirectionUp, m_bMatchCaseInFind))
    {
        m_FindClient->SetFocusOnOwnerWindow();
        EndDialog(IDOK);
    }
}

BOOL CFindDialog::OnInitDialog()
{
    CFindReplaceDialog::OnInitDialog();

    m_CBFindInPIDName.SetCheck(m_bFindInPIDName ? BST_CHECKED : BST_UNCHECKED);
    m_CBFindInPIDName.ShowWindow(m_bHideTracingOptions ? SW_HIDE : SW_SHOW);
    m_CBFindInTraceText.SetCheck(m_bFindInTraceText ? BST_CHECKED : BST_UNCHECKED);
    m_CBFindInTraceText.ShowWindow(m_bHideTracingOptions ? SW_HIDE : SW_SHOW);

    m_BTDeleteAll.ShowWindow(m_bHideDeleteButtons ? SW_HIDE : SW_SHOW);
    m_BTMarkAll.ShowWindow(m_bHideMarkButtons ? SW_HIDE : SW_SHOW);

    m_EDTextToFind.Attach(m_COTextToFind.GetWindow(GW_CHILD)->m_hWnd);
    for (auto& text : m_TextList)
    {
        m_COTextToFind.AddString(text.c_str());
    }

    m_EDTextToFind.SetWindowText(m_LastTextToFind.c_str());
    OnChangedText();

    m_EDTextToFind.SetFocus();
    return FALSE;
}

void CFindDialog::OnFind()
{
    CString text;
    m_COTextToFind.GetWindowText(text);

    if (!text.IsEmpty())
    {
        for (auto index = 0; index < m_COTextToFind.GetCount(); index++)
        {
            CString label;
            m_COTextToFind.GetLBText(index, label);

            if (label.CompareNoCase(text) == 0)
            {
                m_COTextToFind.DeleteString(index);
                break;
            }
        }

        m_COTextToFind.InsertString(0, text);

        if (m_COTextToFind.GetCount() > MAX_FIND_TEXTS)
        {
            m_COTextToFind.DeleteString(m_COTextToFind.GetCount() - 1);
        }
    }

    m_EDTextToFind.SetWindowText(text);

    Save();

    if (!UpdateOptions())
    {
        return;
    }

    if (m_FindClient->FindNext(text.GetBuffer(), m_bFindDirectionUp, m_bMatchCaseInFind))
    {
        m_FindClient->SetFocusOnOwnerWindow();
        EndDialog(IDOK);
    }
}

bool CFindDialog::UpdateOptions()
{
    CString text;
    m_EDTextToFind.GetWindowText(text);

    m_LastTextToFind = text.GetBuffer();
    m_bFindDirectionUp = (m_BTUp.GetCheck() == BST_CHECKED);
    m_bMatchCaseInFind = (m_CBMatchCase.GetCheck() == BST_CHECKED);
    m_bFindInPIDName = (m_CBFindInPIDName.GetCheck() == BST_CHECKED);
    m_bFindInTraceText = (m_CBFindInTraceText.GetCheck() == BST_CHECKED);

    if (!m_bFindInPIDName && !m_bFindInTraceText)
    {
        MessageBox(
            L"At least one search option must be selected\r\n\r\n\"Find in Process Id / Name\" \r\n\"Find in Trace Text\"",
            L"ETViewer Search",
            MB_ICONSTOP | MB_OK);

        return false;
    }

    return true;
}

void CFindDialog::Save()
{
    m_TextList.clear();

    for (auto index = 0; index < m_COTextToFind.GetCount(); index++)
    {
        CString label;
        m_COTextToFind.GetLBText(index, label);
        m_TextList.emplace_back(label.GetBuffer());
    }
}

void CFindDialog::OnDestroy()
{
    m_EDTextToFind.Detach();
    CFindReplaceDialog::OnDestroy();
    m_FindClient->SetFocusOnOwnerWindow();
}

void CFindDialog::OnCancel()
{
    Save();
    CFindReplaceDialog::EndDialog(IDCANCEL);
}

void CFindDialog::OnChangedText()
{
    if (m_EDTextToFind.m_hWnd == NULL)
    {
        return;
    }

    CString text;
    m_EDTextToFind.GetWindowText(text);
    bool anyText = !text.IsEmpty();

    m_BTDeleteAll.EnableWindow(anyText);
    m_BTMarkAll.EnableWindow(anyText);
    m_BTFind.EnableWindow(anyText);
}

void CFindDialog::OnTextSelected()
{
    CString text;
    int index = m_COTextToFind.GetCurSel();
    if (index != -1)
    {
        m_COTextToFind.GetLBText(index, text);
        m_EDTextToFind.SetWindowText(text);
    }
    OnChangedText();
}

void CFindDialog::SetText(const TCHAR* pTextToFind)
{
    m_EDTextToFind.SetWindowText(pTextToFind);
    OnChangedText();
    m_EDTextToFind.SetSel(0, m_EDTextToFind.GetWindowTextLength());
    m_COTextToFind.SetFocus();
}