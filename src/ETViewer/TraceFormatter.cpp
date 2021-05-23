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
#include "TraceFormatter.h"

// TODO: use common definition
const GUID GUID_EventTraceEvent1 = { 0x68fdd900, 0x4a3e, 0x11d1, { 0x84, 0xf4, 0, 0, 0xf8, 0x04, 0x64, 0xe3 } };

TraceFormatter::TraceFormatter(const std::wstring& tmfPath)
    : m_TmfPath(tmfPath)
{
    TDH_CONTEXT context = { 0 };

    context.ParameterType = TDH_CONTEXT_WPP_TMFSEARCHPATH;
    context.ParameterValue = (ULONGLONG)m_TmfPath.c_str();
    m_Contexts.push_back(context);

    context.ParameterType = TDH_CONTEXT_POINTERSIZE;
    context.ParameterValue = sizeof(void*);
    m_Contexts.push_back(context);
}

bool TraceFormatter::ProcessEvent(PEVENT_RECORD event, STraceEvenTracingNormalizedData& data)
{
    // Process header
    if ((event->EventHeader.ProviderId == GUID_EventTraceEvent1) &&
        (event->EventHeader.EventDescriptor.Opcode == EVENT_TRACE_TYPE_INFO))
    {
        unsigned long pointerSize = sizeof(void*);
        GetProperty(event, L"PointerSize", pointerSize);
        return false;
    }

    // Only trace messages are supported
    if ((event->EventHeader.Flags & EVENT_HEADER_FLAG_TRACE_MESSAGE) == 0)
    {
        return false;
    }

    GetProperty(event, L"SequenceNum", data.dwSequenceIndex);
    GetProperty(event, L"SystemTime", data.systemTime);

    GetStringProperty(event, L"FunctionName", data.sFunction);
    GetStringProperty(event, L"FormattedString", data.sText);
    GetStringProperty(event, L"ComponentName", data.sComponent);

    GetStringProperty(event, L"LevelName", data.sLevel);

    data.dwProcessId = event->EventHeader.ProcessId;
    data.dwThreadId = event->EventHeader.ThreadId;
    data.timeStamp = event->EventHeader.TimeStamp;
    data.dwLine = 0;

    return true;
}

template <class Type>
unsigned long TraceFormatter::GetProperty(PEVENT_RECORD event, const std::wstring& name, Type& value)
{
    PROPERTY_DATA_DESCRIPTOR propertyData = { 0 };
    propertyData.ArrayIndex = ULONG_MAX;
    propertyData.PropertyName = (ULONGLONG)name.c_str();

    return TdhGetProperty(
        event,
        m_Contexts.size(),
        m_Contexts.data(),
        1,
        &propertyData,
        sizeof(value),
        (PBYTE)&value);
}

unsigned long TraceFormatter::GetStringProperty(PEVENT_RECORD event, const std::wstring& name, std::wstring& value)
{
    BYTE buffer[4096] = { 0 };

    PROPERTY_DATA_DESCRIPTOR propertyData = { 0 };
    propertyData.ArrayIndex = ULONG_MAX;
    propertyData.PropertyName = (ULONGLONG)name.c_str();

    auto result = TdhGetProperty(
        event,
        m_Contexts.size(),
        m_Contexts.data(),
        1,
        &propertyData,
        4096,
        buffer);

    if (result != ERROR_SUCCESS)
    {
        return result;
    }

    value = (wchar_t*)buffer;

    return ERROR_SUCCESS;
}