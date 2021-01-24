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

#include "stdafx.h"

class CHighLightFiltersEditor : public CDialog
{
public:
    CHighLightFiltersEditor(CWnd* pParent = NULL);

    //{{AFX_DATA(CHighLightFiltersEditor)
    enum { IDD = IDD_HIGHLIGHT_FILTERS_EDITOR };
    //}}AFX_DATA

    static LRESULT CALLBACK ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SwapItems(int index1, int index2);
    void GetItemColorRects(int index, RECT* pR1, RECT* pR2);

    void LoadFilters();
    void SaveFilters();
    void SetMetrics();

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnCancel();
    afx_msg void OnDown();
    afx_msg void OnUp();
    virtual void OnOK();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    virtual BOOL OnInitDialog();
    afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnFilterClicked(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    CButton	m_BTCancel;
    CButton	m_BTOk;
    CButton	m_BTDown;
    CButton	m_BTUp;
    CListCtrl m_LWFilters;
    WNDPROC	m_OldListViewProc;
    HIMAGELIST m_hImageList;
};