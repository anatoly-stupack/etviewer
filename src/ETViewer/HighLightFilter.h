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

class CHighLightFilter
{
public:
    CHighLightFilter(); 
    CHighLightFilter(const std::wstring& serializedObject);
    CHighLightFilter(const CHighLightFilter& otherFilter);
    CHighLightFilter& operator = (CHighLightFilter& otherFilter);
    ~CHighLightFilter();

    HPEN GetPen() const;
    HBRUSH GetBrush() const;
    COLORREF GetTextColor() const;
    const std::wstring& GetText() const;
    DWORD GetTextLen() const;
    COLORREF GetBkColor() const;
    bool GetEnabled() const;

    void SetBkColor(COLORREF bkColor);
    void SetEnabled(bool bEnable);
    void SetText(std::wstring sText);
    void SetTextColor(COLORREF textColor);

    void UpdateObjects();

    std::wstring ToString();

private:
    std::wstring m_Text;
    DWORD m_dwTextLen;
    DWORD m_TextColor;
    DWORD m_BkColor;
    bool m_bEnabled;
    HPEN m_hPen;
    HBRUSH m_hBrush;

    /*
        BEGIN_PERSIST_MAP(CHighLightFilter)
            PERSIST(m_Text, _T("Text"))
            PERSIST(m_dwTextLen, _T("TextLength"))
            PERSIST(m_TextColor, _T("TextColor"))
            PERSIST(m_BkColor, _T("BkColor"))
            PERSIST(m_bEnabled, _T("Enabled"))
        END_PERSIST_MAP()
    */
};