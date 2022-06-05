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
#include "TraceProvider.h"

CTraceProvider::CTraceProvider(const std::wstring& componentName, const GUID& providerGuid, const std::wstring& fileName)
    : m_ComponentName(componentName)
    , m_AllSupportedFlagsMask(0)
    , m_ProviderGUID(providerGuid)
{
    m_FileList.insert(fileName);
}

const GUID& CTraceProvider::GetGUID() const
{
    return m_ProviderGUID;
}

const std::wstring& CTraceProvider::GetComponentName() const
{
    return m_ComponentName;
}

const std::set<std::wstring>& CTraceProvider::GetFileList() const
{
    return m_FileList;
}

void CTraceProvider::AddFileName(const std::wstring& fileName)
{
    m_FileList.insert(fileName);
}

void CTraceProvider::AddSourceFile(const std::shared_ptr<CTraceSourceFile>& sourceFile)
{
    m_SourceFiles.emplace(sourceFile->GetGUID(), sourceFile);
}

std::shared_ptr<CTraceSourceFile> CTraceProvider::GetSourceFile(const GUID& sourceGUID) const
{
    auto entry = m_SourceFiles.find(sourceGUID);
    if (entry == m_SourceFiles.end())
    {
        return nullptr;
    }

    return entry->second;
}

void CTraceProvider::AddFormatEntry(const std::shared_ptr<STraceFormatEntry>& formatEntry)
{
    m_FormatEntries.emplace_back(formatEntry);
}

const std::vector<std::shared_ptr<STraceFormatEntry>>& CTraceProvider::GetFormatEntries() const
{
    return m_FormatEntries;
}

DWORD CTraceProvider::GetAllSupportedFlagsMask() const
{
    return m_AllSupportedFlagsMask;
}

const std::map<std::wstring, DWORD>& CTraceProvider::GetSupportedFlags() const
{
    return m_TraceFlags;
}

void CTraceProvider::AddSupportedFlag(const std::wstring& name, DWORD dwValue)
{
    m_TraceFlags[name] = dwValue;
    m_AllSupportedFlagsMask |= dwValue;
}

DWORD CTraceProvider::GetSupportedFlagValue(const std::wstring& name) const
{
    auto entry = m_TraceFlags.find(name);
    if (entry == m_TraceFlags.end())
    {
        return 0;
    }

    return entry->second;
}