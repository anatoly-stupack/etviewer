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

#include "ColumnInfo.h"
#include "FindDialog.h"
#include "TraceController.h"

#define CAPTURE_TIMER			1
#define SHOW_LAST_TRACE_TIMER	2

struct SETViewerTrace
{
    STraceEvenTracingNormalizedData trace;

    int		iImage;

    SETViewerTrace()
    {
        iImage = 0;
    }
};

class CETViewerView
    : public CListView
    , public CFindDialogClient
    , public ITraceEvents
{
public:
    DECLARE_DYNCREATE(CETViewerView)
    CETViewerView();
    virtual ~CETViewerView();

private:
    virtual void OnInitialUpdate();
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    void UpdateFont();
    void UpdateColumns();
    void AddColumn(int id);
    void RemoveColumn(int id);

    static LRESULT CALLBACK ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void ProcessSpecialKeyStroke(WORD key);

    int FindText(const TCHAR* pTextToFind, int baseIndex, bool up, bool stopAtEnd = false);

    void SetFocusOnOwnerWindow();

    TCHAR* GetTraceText(SETViewerTrace* pTrace, CColumnInfo* pColumn, TCHAR* pAuxBuffer, unsigned nAuxLen);

public:
    bool Load();
    bool Save();

    void OnFontBigger();
    void OnFontSmaller();

    bool IsShowLastTraceEnabled();
    void EnableShowLastTrace(bool bEnable);
    void ResetShowLastTrace();

    bool FindNext();
    bool FindNext(const TCHAR* pText);
    bool FindAndMarkAll(const TCHAR* pText);
    bool FindAndDeleteAll(const TCHAR* pText);

    void Copy(bool bAllTraces);
    void Clear();
    void ClearSelected();

    void GetTransferBuffer(int* pSize, TCHAR** buffer, bool bAllTraces);
    void GetTraceColors(SETViewerTrace* pTrace, COLORREF* pTextColor, COLORREF* pBkColor, HPEN* phPen, HBRUSH* phBrush);

    // ITraceEvents
    void ProcessTrace(STraceEvenTracingNormalizedData* pTraceData);
    void ProcessUnknownTrace(STraceEvenTracingNormalizedData* pTraceData);

    void OnAddProvider(CTraceProvider* pProvider);
    void OnRemoveProvider(CTraceProvider* pProvider);
    void OnReplaceProvider(CTraceProvider* pOldProvider, CTraceProvider* pNewProvider);
    void OnProvidersModified();
    void OnSessionTypeChanged();

    void SetTraceFont(std::wstring sTraceFont, DWORD dwFontSize);
    void GetTraceFont(std::wstring* psTraceFont, DWORD* pdwFontSize);

    void SortItems(CColumnInfo* pColumn);

    DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
    afx_msg void OnDestroy();
    afx_msg void OnNMRclick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEditFind();
    afx_msg void OnFind();
    afx_msg void OnHighLightFilters();
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnNMDblclk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClear();
    afx_msg void OnCut();
    afx_msg void OnCopy();
    afx_msg void OnSave();
    afx_msg void OnBeginEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnEndEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetItemInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLvnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);

private:
    std::wstring m_LastTextToFind;
    bool m_bFindDirectionUp;

    std::deque<CColumnInfo> m_ColumnInfo;
    std::map<int, CColumnInfo*>	m_mVisibleColumns;

    WNDPROC	m_OldListEditProc;
    WNDPROC	m_OldListViewProc;

    HIMAGELIST m_hImageList;
    HICON m_hHollowIcon;
    HICON m_hMarkerIcon;

    std::deque<SETViewerTrace*>	m_lTraces;
    HANDLE m_hTracesMutex;
    bool m_bShowLastTrace;

    int m_iHollowImage;
    int m_iMarkerImage;

    int m_iEditSubItem;
    CSize m_ListEditSize;
    CPoint m_ListEditPos;
    CEdit* m_pEdit;

    CFont* m_pTraceFont;
    DWORD m_dwTraceFontSize;
    std::wstring m_sTraceFont;

    COLORREF m_cNormalTextColor;
    COLORREF m_cNormalBkColor;
    COLORREF m_cSelectedTextColor;
    COLORREF m_cSelectedBkColor;

    HBRUSH m_hNormalBrush;
    HBRUSH m_hSelectedBrush;
    HBRUSH m_hHollowBrush;

    HPEN m_hNormalPen;
    HPEN m_hSelectedPen;

    int m_SortColumn;
    int m_SortDirection;

    int m_nLastFocusedSequenceIndex;
    int m_nUnformattedTraces;
};