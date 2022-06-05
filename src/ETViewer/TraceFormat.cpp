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
#include "TraceFormat.h"

STraceFormatEntry::STraceFormatEntry()
{
    m_dwIndex = 0;
    m_SourceFileGUID = GUID_NULL;
    m_pSourceFile = NULL;
    m_pProvider = NULL;
}

STraceFormatEntry::~STraceFormatEntry()
{
    unsigned x;
    for (x = 0; x < m_vElements.size(); x++)
    {
        TCHAR* pFormat = m_vElements[x].pFormatString;
        delete[] pFormat;
    }
    m_vElements.clear();
    m_dwIndex = 0;
}

void STraceFormatEntry::InitializeFromBuffer(TCHAR* pBuffer)
{
    int len = (int)_tcslen(pBuffer), tempLen = 0;
    TCHAR* pTempBuffer = new TCHAR[len + 1];
    int x;
    for (x = 0; x < len;)
    {
        if (pBuffer[x] == _T('%') && pBuffer[x + 1] != _T('%'))
        {
            int nParameterIndexLen = 0;
            TCHAR sParamIndex[100] = { 0 };

            if (tempLen)
            {
                pTempBuffer[tempLen] = 0;

                STraceFormatElement element;
                element.eType = eTraceFormatElementType_ConstantString;
                element.pFormatString = new TCHAR[tempLen + 1];
                _tcscpy_s(element.pFormatString, tempLen + 1, pTempBuffer);
                m_vElements.push_back(element);
                tempLen = 0;
            }

            //Copy initial '%' TCHARacter
            pTempBuffer[tempLen] = pBuffer[x]; x++; tempLen++;
            // Read parameter index.
            while (pBuffer[x] >= _T('0') && pBuffer[x] <= _T('9'))
            {
                sParamIndex[nParameterIndexLen] = pBuffer[x];
                nParameterIndexLen++;
                x++;
            }

            x++; //Skip initial '!' TCHARacter
            while (pBuffer[x] != _T('!'))
            {
                pTempBuffer[tempLen] = pBuffer[x];
                x++; tempLen++;
            }
            x++; //Skip final '!' TCHARacter
            pTempBuffer[tempLen] = 0;
            if (tempLen)
            {
                int nParameterIndex = _ttoi(sParamIndex) - PARAMETER_INDEX_BASE;

                STraceFormatElement element;
                if (nParameterIndex >= 0 && nParameterIndex < (int)m_vParams.size())
                {
                    element.eType = m_vParams[nParameterIndex].eType;
                }
                pTempBuffer[tempLen] = 0;
                element.pFormatString = new TCHAR[tempLen + 1];
                _tcscpy_s(element.pFormatString, tempLen + 1, pTempBuffer);

                if (tempLen)
                {
                    // Modify the format buffers if needed. 

                    //ETW formats 'doubles' as %s instead of '%f'.
                    if (element.eType == eTraceFormatElementType_Float ||
                        element.eType == eTraceFormatElementType_Double)
                    {
                        if (element.pFormatString[tempLen - 1] == _T('s')) { element.pFormatString[tempLen - 1] = _T('f'); }
                    }
                    // 64 bit %p must be translated to %I64X
                    if (element.eType == eTraceFormatElementType_QuadPointer)
                    {
                        if (element.pFormatString[tempLen - 1] == _T('p'))
                        {
                            TCHAR* pOld = element.pFormatString;
                            element.pFormatString = new TCHAR[tempLen + 6 + 1];
                            _tcscpy_s(element.pFormatString, tempLen + 6 + 1, pOld);
                            _tcscpy_s(element.pFormatString + (tempLen - 1), 8, _T("016I64X"));
                            delete[] pOld;
                        }
                    }
                    if (element.eType == eTraceFormatElementType_Unknown)
                    {
                        if (element.pFormatString[tempLen - 1] == _T('s'))
                        {
                            TCHAR* pOld = element.pFormatString;
                            element.pFormatString = new TCHAR[8 + 1];
                            _tcscpy_s(element.pFormatString, 9, _T("(0x%08x)"));
                            delete[] pOld;
                        }
                    }
                }

                m_vElements.push_back(element);
                pTempBuffer[0] = 0;
                tempLen = 0;
            }
        }
        else
        {
            pTempBuffer[tempLen] = pBuffer[x];
            tempLen++;
            if (pBuffer[x] == _T('%') && pBuffer[x + 1] == _T('%'))
            {
                x++;
            }
            x++;
        }
    }
    if (tempLen)
    {
        pTempBuffer[tempLen] = 0;

        STraceFormatElement element;
        element.eType = eTraceFormatElementType_ConstantString;
        element.pFormatString = new TCHAR[tempLen + 1];
        _tcscpy_s(element.pFormatString, tempLen + 1, pTempBuffer);
        m_vElements.push_back(element);
        tempLen = 0;
    }
    delete[] pTempBuffer;
}