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
#include "HighLightFilter.h"

CHighLightFilter::CHighLightFilter()
    : m_bEnabled(true)
    , m_dwTextLen(0)
    , m_hPen(nullptr)
    , m_hBrush(nullptr)
{
    SetTextColor(RGB(0, 0, 0));
    SetBkColor(RGB(255, 255, 255));
}

CHighLightFilter::CHighLightFilter(const std::wstring& serializedObject)
    : CHighLightFilter()
{
    std::wstring token;
    std::vector<std::wstring> tokens;
    std::wistringstream parser(serializedObject);

    while (std::getline(parser, token, L';'))
    {
        tokens.emplace_back(token);
    }

    if (tokens.size() < 5)
    {
        return;
    }

    try
    {
        m_bEnabled = (tokens[0] == L"1");
        m_BkColor = _wtoi(tokens[1].c_str());
        m_TextColor = _wtoi(tokens[2].c_str());
        m_dwTextLen = _wtoi(tokens[3].c_str());
        m_Text = tokens[4];
    }
    catch (...)
    {
        return;
    }
}

CHighLightFilter::CHighLightFilter(const CHighLightFilter& otherFilter)
{
    m_Text = otherFilter.m_Text;
    m_TextColor = otherFilter.m_TextColor;
    m_BkColor = otherFilter.m_BkColor;
    m_bEnabled = otherFilter.m_bEnabled;
    m_dwTextLen = otherFilter.m_dwTextLen;
    UpdateObjects();
}

CHighLightFilter& CHighLightFilter::operator = (CHighLightFilter& otherFilter)
{
    m_Text = otherFilter.m_Text;
    m_TextColor = otherFilter.m_TextColor;
    m_BkColor = otherFilter.m_BkColor;
    m_bEnabled = otherFilter.m_bEnabled;
    m_dwTextLen = otherFilter.m_dwTextLen;
    UpdateObjects();
    return *this;
}

CHighLightFilter::~CHighLightFilter()
{
    if (m_hPen)
    {
        DeleteObject(m_hPen);
        m_hPen = NULL;
    }

    if (m_hBrush)
    {
        DeleteObject(m_hBrush);
        m_hBrush = NULL;
    }
}

HPEN CHighLightFilter::GetPen() const
{
    return m_hPen;
}

HBRUSH CHighLightFilter::GetBrush() const
{
    return m_hBrush;
}

COLORREF CHighLightFilter::GetTextColor() const
{
    return m_TextColor;
}

COLORREF CHighLightFilter::GetBkColor() const
{
    return m_BkColor;
}

bool CHighLightFilter::GetEnabled() const
{
    return m_bEnabled;
}

const std::wstring& CHighLightFilter::GetText() const
{
    return m_Text;
}

DWORD CHighLightFilter::GetTextLen() const
{
    return m_dwTextLen;
}

void CHighLightFilter::SetTextColor(COLORREF textColor)
{
    m_TextColor = textColor;
    UpdateObjects();
}

void CHighLightFilter::SetBkColor(COLORREF bkColor)
{
    m_BkColor = bkColor; UpdateObjects();
}

void CHighLightFilter::SetEnabled(bool bEnable)
{
    m_bEnabled = bEnable;
}

void CHighLightFilter::SetText(std::wstring sText)
{
    m_Text = sText;
    m_dwTextLen = (DWORD)_tcslen(m_Text.c_str());
}

void CHighLightFilter::UpdateObjects()
{
    if (m_hPen)
    {
        DeleteObject(m_hPen);
        m_hPen = nullptr;
    }
    
    if (m_hBrush)
    {
        DeleteObject(m_hBrush);
        m_hBrush = NULL;
    }

    POINT P = { 1,1 };
    LOGPEN LP = { 0 };
    LP.lopnColor = m_BkColor;
    LP.lopnWidth = P;
    LP.lopnStyle = PS_SOLID;

    LOGBRUSH LB = { 0 };
    LB.lbColor = m_BkColor;
    LB.lbStyle = BS_SOLID;

    m_hBrush = CreateBrushIndirect(&LB);
    m_hPen = CreatePenIndirect(&LP);
}

std::wstring CHighLightFilter::ToString()
{
    std::wstringstream result;

    result << m_bEnabled << L';';
    result << m_BkColor << L';';
    result << m_TextColor << L';';
    result << m_dwTextLen << L';';
    result << m_Text << L';';

    return result.str();
}