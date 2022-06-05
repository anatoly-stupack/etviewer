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

#include "TraceSourceFile.h"

#include <windows.h>
#include <vector>
#include <string>

#define PARAMETER_INDEX_BASE 10

enum eTraceFormatElementType
{
    eTraceFormatElementType_Unknown,
    eTraceFormatElementType_ConstantString,
    eTraceFormatElementType_Byte,
    eTraceFormatElementType_Word,
    eTraceFormatElementType_DWord,
    eTraceFormatElementType_Float,
    eTraceFormatElementType_Double,
    eTraceFormatElementType_Quad,
    eTraceFormatElementType_Pointer,
    eTraceFormatElementType_QuadPointer,
    eTraceFormatElementType_AnsiString,
    eTraceFormatElementType_WideString,
    eTraceFormatElementType_CountedWideString
};

struct STraceFormatElement
{
    TCHAR* pFormatString;
    eTraceFormatElementType	 eType;

    STraceFormatElement() :eType(eTraceFormatElementType_Unknown), pFormatString(NULL) {}
};

struct STraceFormatParam
{
    eTraceFormatElementType	 eType;

    STraceFormatParam() :eType(eTraceFormatElementType_Unknown) {}
};

struct STraceFormatEntry
{
    GUID						        m_SourceFileGUID;
    DWORD						        m_dwIndex;
    DWORD						        m_dwLine;
    std::vector<STraceFormatElement>    m_vElements;
    std::vector<STraceFormatParam>	    m_vParams;
    std::wstring						m_sFunction;
    std::wstring						m_sLevel;
    std::wstring						m_sFlag;
    std::shared_ptr<CTraceSourceFile>   m_pSourceFile;
    CTraceProvider* m_pProvider;

    void InitializeFromBuffer(TCHAR* pBuffer);

    STraceFormatEntry();
    ~STraceFormatEntry();
};

struct STraceFormatEntryKey
{
    GUID	m_SourceFileGUID;
    DWORD	m_dwIndex;

    bool operator <(const STraceFormatEntryKey& other) const
    {
        int nComp = memcmp(&m_SourceFileGUID, &other.m_SourceFileGUID, sizeof(GUID));
        if (nComp < 0) { return true; }
        if (nComp > 0) { return false; }
        return m_dwIndex < other.m_dwIndex;
    }

    STraceFormatEntryKey(STraceFormatEntry* pValue) { m_SourceFileGUID = pValue->m_SourceFileGUID; m_dwIndex = pValue->m_dwIndex; }
    STraceFormatEntryKey(GUID& sourceFileGUID, DWORD dwIndex) { m_SourceFileGUID = sourceFileGUID; m_dwIndex = dwIndex; }
};