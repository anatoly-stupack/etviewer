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
#include "SourceFileViewer.h"
#include "SourceFileContainer.h"

#define TAB_SIZE 4

std::wstring g_Keywords[] = {
    /*C++ Keywords*/
    L"asm1", L"auto", L"bad_cast", L"bad_typeid", L"bool", L"break", L"case", L"catch", L"TCHAR", L"class", L"const", L"const_cast",
    L"continue", L"default", L"delete", L"do", L"double", L"dynamic_cast", L"else", L"enum", L"except", L"explicit", L"extern",
    L"false", L"finally", L"float", L"for", L"friend", L"goto", L"if", L"inline", L"int", L"long", L"mutable", L"namespace", L"new",
    L"operator", L"private", L"protected", L"public", L"register", L"reinterpret_cast", L"return", L"short", L"signed", L"sizeof",
    L"static", L"static_cast", L"struct", L"switch", L"template", L"this", L"throw", L"true", L"try", L"type_info", L"typedef",
    L"typeid", L"typename", L"union", L"unsigned", L"using", L"virtual", L"void", L"volatile", L"while", L"this", L"nullptr"
    /*Preprocessor Directives*/
    L"#define", L"#error", L"#import", L"#undef", L"#elif", L"#if", L"#include", L"#else", L"#ifdef", L"#line", L"#endif", L"#ifndef",
    L"#pragma", L"BOOL", L"HANDLE", L"DWORD", L"FALSE", L"TRUE", L"boolean", L"LRESULT", L"hyper", L"BYTE", L"LONG", L"ULONG", L"UINT",
    L"UIN16", L"set", L"map", L"list", L"deque", L"vector", L"BEGIN_DBTABLE", L"DBTABLE_COMP", L"END_DBTABLE", L"BEGIN_COM_MAP",
    L"COM_INTERFACE_ENTRY", L"COM_INTERFACE_ENTRY_CHAIN", L"END_COM_MAP", L"DECLARE_GUIOBJECT", L"DECLARE_PPAGE", L"DECLARE_COMPOSITE",
    L"BEGIN_MSG_MAP", L"CHAIN_MSG_MAP", L"MESSAGE_HANDLER", L"COMMAND_HANDLER", L"COMMAND_ID_HANDLER", L"COMMAND_CODE_HANDLER",
    L"COMMAND_RANGE_HANDLER", L"NOTIFY_HANDLER", L"NOTIFY_ID_HANDLER", L"NOTIFY_CODE_HANDLER", L"NOTIFY_RANGE_HANDLER", L"END_MSG_MAP",
    L"VAA2W", L"VAW2A", L"_CSA2W", L"_GRS", L"LS", L"NULL", L"CLSID_NULL", L"GUID_NULL", L"IID_NULL", L"SUCCEEDED", L"FAILED",
    L"VA_SLAVE_DB", L"VA_MASTER_DB"};

std::wstring g_Separators = L" ,.:;'\\\"+-*/%=!?<>[](){}\t\n\r&|~^";

void SetRichEditTextColor(CRichEditCtrl* pEdit, DWORD begin, DWORD end, COLORREF color)
{
    CHARFORMAT format = { 0 };
    format.cbSize = sizeof(format);
    format.crTextColor = color;
    format.dwMask = CFM_COLOR;
    format.dwEffects &= ~CFE_AUTOCOLOR;

    pEdit->SetSel(begin, end);
    pEdit->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (DWORD)&format);
    pEdit->SetSel(0, 0);
}

CSourceFileViewer::CSourceFileViewer(CSourceFileContainer* parent)
    : CDialog(CSourceFileViewer::IDD, parent)
    , m_pContainer(parent)
    , m_hFileFont(nullptr)
    , m_OldEditProc(nullptr)
    , m_SourceLine(0)
    , m_FindDialog(nullptr)
    , m_FindDirectionUp(false)
    , m_FindMatchCase(false)
{
    LoadLibrary(L"RICHED32.DLL");
}

void CSourceFileViewer::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ED_LINE, m_EDLine);
    DDX_Control(pDX, IDC_ED_FULL_PATH, m_EDFullPath);
    DDX_Control(pDX, IDC_ED_FILE, m_EDFile);
}

BEGIN_MESSAGE_MAP(CSourceFileViewer, CDialog)
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDCANCEL, OnCancel)
    ON_BN_CLICKED(IDOK, OnOK)
END_MESSAGE_MAP()

void CSourceFileViewer::OnOK()
{
}

void CSourceFileViewer::OnCancel()
{
}

DWORD CSourceFileViewer::OpenFile(const std::wstring& filePath, int line, bool bShowErrorIfFailed)
{
    DWORD result = ERROR_SUCCESS;

    std::wifstream file(filePath);
    if (!file.is_open())
    {
        if (bShowErrorIfFailed)
        {
            std::wstring message = L"Cannot open source file '" + filePath;
            MessageBox(message.c_str(), L"ETViewer", MB_ICONSTOP | MB_OK);
        }
        return ERROR_FILE_NOT_FOUND;
    }

    m_SourceFile = filePath;
    m_SourceLine = line;

    theApp.AddRecentSourceFile(m_SourceFile.c_str());

    std::wstring contentLine;
    while (std::getline(file, contentLine))
    {
        m_FileContent += contentLine + L"\r\n";
    }
    file.close();

    m_FileContentUpperCase = m_FileContent;

    std::transform(
        m_FileContentUpperCase.begin(),
        m_FileContentUpperCase.end(),
        m_FileContentUpperCase.begin(), towupper);

    m_EDFile.SetWindowText(m_FileContent.c_str());

    SetRichEditTextColor(&m_EDFile, 0, (DWORD)-1, RGB(0, 0, 0));

    // Highlight C++ keywords and preprocessor directives
    for (auto& keyword : g_Keywords)
    {
        size_t position = 0;
        do
        {
            position = m_FileContent.find(keyword, position);
            if (position == std::wstring::npos)
            {
                break;
            }

            // Ensure keyword is not in the middle of another word
            if (position > 0 && g_Separators.find(m_FileContent[position - 1]) == std::wstring::npos)
            {
                break;
            }
            if ((position + keyword.size() + 1) < m_FileContent.size() &&
                g_Separators.find(m_FileContent[position + keyword.size() + 1]) == std::wstring::npos)
            {
                break;
            }

            SetRichEditTextColor(&m_EDFile, position, position + keyword.size(), RGB(0, 0, 255));

            position++;
        }
        while (true);
    }

    // Highlight strings
    size_t position = 0;
    size_t startPosition = (size_t)-1;
    do
    {
        position = m_FileContent.find(L"\"", position);
        if (position == std::wstring::npos)
        {
            break;
        }

        // Ignore quotes prepended with '\\' sign
        if (position > 0 && m_FileContent[position - 1] == L'\\')
        {
            position++;
            continue;
        }

        if (startPosition == -1)
        {
            startPosition = position;
        }
        else
        {
            SetRichEditTextColor(&m_EDFile, startPosition, position, RGB(128, 0, 0));
            startPosition = (size_t)-1;
        }

        position++;
    }
    while (true);

    // Highlight one line comments
    position = 0;
    startPosition = (size_t)-1;
    do
    {
        if (startPosition == -1)
        {
            position = m_FileContent.find(L"//", position);
            if (position == std::wstring::npos)
            {
                break;
            }

            startPosition = position;
        }
        else
        {
            position = m_FileContent.find(L"\r\n", position);
            if (position == std::wstring::npos)
            {
                break;
            }

            SetRichEditTextColor(&m_EDFile, startPosition, position, RGB(0, 128, 0));
            startPosition = (size_t)-1;
        }

        position++;
    }
    while (true);

    // Highlight multiline comments
    position = 0;
    startPosition = (size_t)-1;
    do
    {
        if (startPosition == -1)
        {
            position = m_FileContent.find(L"/*", position);
            if (position == std::wstring::npos)
            {
                break;
            }

            startPosition = position;
        }
        else
        {
            position = m_FileContent.find(L"*/", position);
            if (position == std::wstring::npos)
            {
                break;
            }

            SetRichEditTextColor(&m_EDFile, startPosition, position + 2, RGB(0, 128, 0));
            startPosition = (size_t)-1;
        }

        position++;
    }
    while (true);

    SetMetrics();

    UpdateLine();

    PostMessage(WM_USER + 1);

    ShowLine(m_SourceLine);

    std::wstring caption = m_SourceFile.c_str();
    if (m_SourceLine)
    {
        caption += L" " + std::to_wstring(m_SourceLine);
    }

    SetWindowText(caption.c_str());

    m_EDFullPath.SetWindowText(m_SourceFile.c_str());

    return result;
}

void CSourceFileViewer::Reload()
{
    OpenFile(m_SourceFile, m_SourceLine, false);
}

BOOL CSourceFileViewer::OnInitDialog()
{
    CDialog::OnInitDialog();

    LOGFONT logFont = { 0 };
    logFont.lfHeight = 10;

    StringCbPrintf(logFont.lfFaceName, sizeof(logFont.lfFaceName), L"Courier");

    m_hFileFont = CreateFontIndirect(&logFont);
    m_EDFile.SendMessage(WM_SETFONT, (DWORD)m_hFileFont, true);

    m_OldEditProc = (WNDPROC)GetWindowLong(m_EDFile.m_hWnd, GWL_WNDPROC);
    SetWindowLong(m_EDFile.m_hWnd, GWL_WNDPROC, (DWORD)FileEditProc);
    SetWindowLong(m_EDFile.m_hWnd, GWL_USERDATA, (DWORD)this);

    return FALSE;
}

void CSourceFileViewer::OnDestroy()
{
    CDialog::OnDestroy();

    m_FileContent.clear();

    if (m_hFileFont)
    {
        DeleteObject((HGDIOBJ)m_hFileFont);
    }

    m_FindDialog = nullptr;
}

void CSourceFileViewer::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);

    if (m_EDFile.m_hWnd)
    {
        SetMetrics();
    }
}

void CSourceFileViewer::SetMetrics()
{
    RECT clientRect, fileRect, pathRect, lineRect;
    GetClientRect(&clientRect);
    m_EDFile.GetWindowRect(&fileRect);
    m_EDFullPath.GetWindowRect(&pathRect);
    m_EDLine.GetWindowRect(&lineRect);

    SIZE clientSize = GetRectSize(clientRect);
    SIZE fileSize = GetRectSize(fileRect);
    SIZE pathSize = GetRectSize(pathRect);
    SIZE lineSize = GetRectSize(lineRect);
    POINT filePos, pathPos, linePos;

    filePos.x = 0;
    filePos.y = 0;
    fileSize.cx = clientSize.cx;
    fileSize.cy = clientSize.cy - (pathSize.cy + 2);

    pathPos.x = 0;
    pathPos.y = clientSize.cy - pathSize.cy;
    pathSize.cx = clientSize.cx - (lineSize.cx + 1);

    linePos.x = clientSize.cx - lineSize.cx;
    linePos.y = clientSize.cy - lineSize.cy;
    m_EDFile.SetWindowPos(NULL, filePos.x, filePos.y, fileSize.cx, fileSize.cy, SWP_NOZORDER);
    m_EDFullPath.SetWindowPos(NULL, pathPos.x, pathPos.y, pathSize.cx, pathSize.cy, SWP_NOZORDER);
    m_EDLine.SetWindowPos(NULL, linePos.x, linePos.y, lineSize.cx, lineSize.cy, SWP_NOZORDER);
}

void CSourceFileViewer::ShowLine(int line)
{
    m_SourceLine = line;
    OnUpdateSelectedLine();
    UpdateLine();
}

void CSourceFileViewer::OnUpdateSelectedLine()
{
    m_EDFile.SetSel(0, 0);
    if (m_SourceLine)
    {
        int lineToFocus = m_SourceLine - 2;
        int firstChar = m_EDFile.LineIndex(m_SourceLine - 1);
        int lastChar = m_EDFile.LineIndex(m_SourceLine);
        m_EDFile.SetSel(firstChar, lastChar);
        m_EDFile.LineScroll(-m_EDFile.GetLineCount(), 0);
        m_EDFile.LineScroll(lineToFocus - 1);
    }
}

void CSourceFileViewer::UpdateLine()
{
    long selBegin = 0;
    long selEnd = 0;
    m_EDFile.GetSel(selBegin, selEnd);

    auto line = m_EDFile.LineFromChar(selBegin);
    auto column = selBegin - m_EDFile.LineIndex(line);

    std::wstring text = L"Ln " + std::to_wstring(line + 1) + L", Col " + std::to_wstring(column + 1);
    m_EDLine.SetWindowText(text.c_str());
}

void CSourceFileViewer::OnFind()
{
    ShowFindDialog();
}

void CSourceFileViewer::ShowFindDialog()
{
    long begin = 0;
    long end = 0;
    m_EDFile.GetSel(begin, end);

    m_LastTextToFind = m_FileContent.substr(begin, end - begin);

    if (m_FindDialog == nullptr)
    {
        m_FindDialog = new CFindDialog(this, m_EDFile.m_hWnd, false);
        m_FindDialog->Create(true, L"", nullptr, m_FindDirectionUp ? 0 : FR_DOWN, this);
    }
    m_FindDialog->ShowWindow(SW_SHOW);
    m_FindDialog->SetText(m_LastTextToFind.c_str());
}

LRESULT CALLBACK CSourceFileViewer::FileEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSourceFileViewer* pThis = (CSourceFileViewer*)GetWindowLong(hwnd, GWL_USERDATA);
    if (uMsg == WM_RBUTTONDOWN)
    {
        long begin = 0, end = 0;
        TCHAR A[MAX_PATH] = { 0 };
        TCHAR fileToOpen[MAX_PATH] = { 0 };
        pThis->m_EDFile.GetSel(begin, end);
        POINTL P = { LOWORD(lParam),HIWORD(lParam) };
        int TCHARacter = pThis->m_EDFile.SendMessage(EM_CHARFROMPOS, 0, (DWORD)&P);

        if (TCHARacter<begin || TCHARacter>end || begin == end || begin > end)
        {
            begin = end = TCHARacter;
            int line = pThis->m_EDFile.LineFromChar(TCHARacter);
            if (line >= 0)
            {
                long TCHARIndex = TCHARacter - pThis->m_EDFile.LineIndex(line);
                pThis->m_EDFile.GetLine(line, A, _countof(A) - 1);
                if (TCHARIndex >= (long)_tcslen(A)) { TCHARIndex = 0; }

                TCHAR beginCharToFind = _T('"');
                TCHAR* pBegin = NULL, * pEnd = _tcschr(A + TCHARIndex, _T('"'));
                if (pEnd == NULL) { pEnd = _tcschr(A + TCHARIndex, _T('>')); beginCharToFind = _T('<'); }
                if (pEnd != NULL)
                {
                    pEnd[0] = 0;
                    pBegin = _tcsrchr(A, beginCharToFind);
                    if (pBegin)
                    {
                        pBegin++; //skip " TCHARacter
                        (*pEnd) = 0;
                        _tcsncpy_s(fileToOpen, MAX_PATH, pBegin, _countof(fileToOpen) - 1);
                    }
                }
            }

        }
        else
        {
            CString sSelText = pThis->m_EDFile.GetSelText();
            _tcsncpy_s(fileToOpen, MAX_PATH, sSelText, _countof(fileToOpen) - 1);
        }
        if (fileToOpen[0] != 0)
        {
            POINT p = { 0,0 };
            ::GetCursorPos(&p);
            HMENU hMenu = GetSubMenu(LoadMenu(AfxGetResourceHandle(), MAKEINTRESOURCE(IDM_FILE_VIEWER)), 0);

            std::wstring menuItemName = std::wstring(L"Open '") + fileToOpen + L"'";
            MENUITEMINFO itemInfo = { 0 };
            itemInfo.cbSize = sizeof(itemInfo);
            itemInfo.cch = menuItemName.size();
            itemInfo.dwTypeData = const_cast<LPWSTR>(menuItemName.c_str());
            itemInfo.fMask = MIIM_STRING;

            SetMenuItemInfo(hMenu, ID_OPEN_AS_SOURCE_FILE, MF_BYCOMMAND, &itemInfo);
            DWORD command = ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD, p.x, p.y, 0, pThis->m_hWnd, NULL);
            if (command == ID_OPEN_AS_SOURCE_FILE)
            {
                std::deque<std::wstring> dirs;

                // include the same directory as the file showed by this viewer
                std::wstring temp1;
                TCHAR drive[MAX_PATH] = { 0 }, path[MAX_PATH] = { 0 };
                _tsplitpath_s(pThis->m_SourceFile.c_str(), drive, MAX_PATH, path, MAX_PATH, NULL, 0, NULL, 0);
                temp1 = drive;
                temp1 += path;
                if (_tcscmp(temp1.c_str(), _T("")) != 0) { dirs.push_back(temp1); }

                auto directories = theApp.GetSourceDirectories();
                for (auto& directory : directories)
                {
                    dirs.push_back(directory.c_str());
                }

                int x = 0;
                if (!theApp.OpenCodeAddress(fileToOpen, 0, false))
                {
                    for (x = 0; x < (int)dirs.size(); x++)
                    {
                        TCHAR fullPath[MAX_PATH] = { 0 };
                        _tcsncpy_s(fullPath, MAX_PATH, dirs[x].c_str(), _countof(fullPath) - 2); //-2 for the additional slash
                        DWORD len = (DWORD)_tcslen(fullPath);
                        if (len)
                        {
                            if (fullPath[len - 1] != _T('\\'))
                            {
                                fullPath[len] = _T('\\');
                                fullPath[len + 1] = 0;
                                len++;
                            }
                        }

                        _tcsncat_s(fullPath, fileToOpen[0] == _T('\\') ? fileToOpen + 1 : fileToOpen, _countof(fullPath) - 1);

                        if (theApp.OpenCodeAddress(fullPath, 0, false))
                        {
                            break;
                        }
                    }
                }
            }
        }

        return 0;
    }

    if (uMsg == WM_SETFOCUS)
    {
        long begin = 0, end = 0;
        pThis->m_EDFile.GetSel(begin, end);
        LRESULT res = CallWindowProc(pThis->m_OldEditProc, hwnd, uMsg, wParam, lParam);
        pThis->m_EDFile.SetSel(begin, end);
        return res;
    }

    if (uMsg == WM_KEYDOWN)
    {
        bool pushedLControl = (GetKeyState(VK_LCONTROL) >> 15) ? true : false;
        bool pushedRControl = (GetKeyState(VK_RCONTROL) >> 15) ? true : false;
        bool pushedLShift = (GetKeyState(VK_LSHIFT) >> 15) ? true : false;
        bool pushedRShift = (GetKeyState(VK_RSHIFT) >> 15) ? true : false;
        bool pushedControl = (pushedLControl || pushedRControl);
        bool pushedShift = (pushedLShift || pushedRShift);
        if (wParam == 'F' && (pushedControl))
        {
            pThis->OnFind();
            return 0;
        }
        if (wParam == VK_TAB && (pushedControl))
        {
            if (pushedShift)
            {
                pThis->m_pContainer->SelectPrevious();
            }
            else
            {
                pThis->m_pContainer->SelectNext();
            }
            return 0;
        }
        if (wParam == VK_NEXT && (pushedControl))
        {
            pThis->m_pContainer->SelectNext();
            return 0;
        }
        if (wParam == VK_PRIOR && (pushedControl))
        {
            pThis->m_pContainer->SelectPrevious();
            return 0;
        }

        if (wParam == VK_F3 && !(pushedControl))
        {
            bool dir = pThis->m_FindDirectionUp;
            pThis->m_FindDirectionUp = pushedShift;
            pThis->FindNext(pThis->m_LastTextToFind.c_str(), pThis->m_FindDirectionUp, pThis->m_FindMatchCase);
            pThis->m_FindDirectionUp = dir;
            return 0;
        }
    }

    if ((uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) || (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) && uMsg != WM_MOUSEMOVE)
    {
        LRESULT res = CallWindowProc(pThis->m_OldEditProc, hwnd, uMsg, wParam, lParam);
        pThis->UpdateLine();
        return res;
    }

    return CallWindowProc(pThis->m_OldEditProc, hwnd, uMsg, wParam, lParam);
}

bool CSourceFileViewer::FindAndDeleteAll(const std::wstring& text, bool findDirectionUp, bool matchCase)
{
    UNREFERENCED_PARAMETER(text);
    UNREFERENCED_PARAMETER(findDirectionUp);
    UNREFERENCED_PARAMETER(matchCase);
    return false;
}

bool CSourceFileViewer::FindAndMarkAll(const std::wstring& text, bool findDirectionUp, bool matchCase)
{
    UNREFERENCED_PARAMETER(text);
    UNREFERENCED_PARAMETER(findDirectionUp);
    UNREFERENCED_PARAMETER(matchCase);
    return false;
}

bool CSourceFileViewer::FindNext(const std::wstring& text, bool findDirectionUp, bool matchCase)
{
    m_LastTextToFind = text;
    m_FindDirectionUp = findDirectionUp;
    m_FindMatchCase = matchCase;

    if (m_FileContent.empty())
    {
        return false;
    }

    long begin = 0;
    long end = 0;
    m_EDFile.GetSel(begin, end);
    if (begin < 0)
    {
        begin = 0;
    }
    if (end < 0)
    {
        end = 0;
    }

    std::wstring* content = &m_FileContent;
    std::wstring textToSearch = text;

    if (matchCase == false)
    {
        content = &m_FileContentUpperCase;

        std::transform(textToSearch.begin(), textToSearch.end(), textToSearch.begin(), towupper);
    }

    size_t position = 0;
    if (m_FindDirectionUp)
    {
        position = content->rfind(textToSearch,  begin - 1);
        if (position == std::wstring::npos)
        {
            position = content->rfind(textToSearch, content->size());
        }
    }
    else
    {
        position = content->find(textToSearch, end);
        if (position == std::wstring::npos)
        {
            position = content->find(textToSearch, 0);
        }
    }

    if (position == std::wstring::npos)
    {
        std::wstring errorText = L"'" + m_LastTextToFind + L"' was not found";
        MessageBox(errorText.c_str(), L"ETViewer Search", MB_OK);
        return false;
    }

    m_EDFile.SetSel(position, position + text.size());
    
    return true;
}

void CSourceFileViewer::SetFocusOnOwnerWindow()
{
}

void CSourceFileViewer::SetFocusOnEditor()
{
    m_EDFile.SetFocus();
}

void CSourceFileViewer::Copy()
{
    m_EDFile.Copy();
}

std::wstring CSourceFileViewer::GetFile()
{
    return m_SourceFile;
}