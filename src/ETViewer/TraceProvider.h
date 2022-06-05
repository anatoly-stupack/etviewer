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

#include "TraceFormat.h"
#include "TraceSourceFile.h"
#include "TraceReader.h"

#include <windows.h>
#include <set>
#include <map>
#include <string>

#define TRACE_LEVEL_NONE        0   // Tracing is not on
#define TRACE_LEVEL_FATAL       1   // Abnormal exit or termination
#define TRACE_LEVEL_ERROR       2   // Severe errors that need logging
#define TRACE_LEVEL_WARNING     3   // Warnings such as allocation failure
#define TRACE_LEVEL_INFORMATION 4   // Includes non-error cases(for example, Entry-Exit)
#define TRACE_LEVEL_VERBOSE     5   // Detailed traces from intermediate steps
#define TRACE_LEVEL_RESERVED6   6
#define TRACE_LEVEL_RESERVED7   7
#define TRACE_LEVEL_RESERVED8   8
#define TRACE_LEVEL_RESERVED9   9

class CTraceProvider
{
public:
    CTraceProvider(const std::wstring& componentName, const GUID& providerGuid, const std::wstring& fileName);

    const GUID& GetGUID() const;
    DWORD GetAllSupportedFlagsMask() const;

    const std::map<std::wstring, DWORD>& GetSupportedFlags() const;
    DWORD GetSupportedFlagValue(const std::wstring& name) const;
    void AddSupportedFlag(const std::wstring& name, DWORD value);

    const std::wstring& GetComponentName() const;
    const std::set<std::wstring>& GetFileList() const;

    void AddFileName(const std::wstring& fileName);
    void AddSourceFile(const std::shared_ptr<CTraceSourceFile>& sourceFile);
    std::shared_ptr<CTraceSourceFile> GetSourceFile(const GUID& sourceGUID) const;

    void AddFormatEntry(const std::shared_ptr<STraceFormatEntry>& formatEntry);
    const std::vector<std::shared_ptr<STraceFormatEntry>>& GetFormatEntries() const;

private:
    const GUID m_ProviderGUID;
    DWORD m_AllSupportedFlagsMask;
    std::map<std::wstring, DWORD> m_TraceFlags;
    std::wstring m_ComponentName;
    std::set<std::wstring> m_FileList;
    std::vector<std::shared_ptr<STraceFormatEntry>> m_FormatEntries;
    std::map<GUID, std::shared_ptr<CTraceSourceFile>, CGUIDComparer> m_SourceFiles;
};