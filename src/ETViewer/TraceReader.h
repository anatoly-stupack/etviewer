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
#include <windows.h>
#include <map>
#include <deque>
#include <string>
#include <dbghelp.h>

class CTraceProvider;

class CGUIDComparer
{
public:

    bool operator ()(const GUID& ref1, const GUID& ref2) const
    {
        return memcmp(&ref1, &ref2, sizeof(GUID)) < 0;
    }

};

enum eTraceReaderError
{
    eTraceReaderError_Success,
    eTraceReaderError_Generic,
    eTraceReaderError_PDBNotFound,
    eTraceReaderError_NoProviderFoundInPDB,
    eTraceReaderError_FailedToEnumerateSymbols,
    eTraceReaderError_DBGHelpNotFound,
    eTraceReaderError_DBGHelpWrongVersion,
    eTraceReaderError_SymEngineLoadModule,
    eTraceReaderError_SymEngineInitFailed,
    eTraceReaderError_NoMemory,
};

class CTraceReader
{
public:
    eTraceReaderError LoadFromPDB(LPCTSTR pPDB, std::vector<CTraceProvider*>* pvProviders);
    static BOOL CALLBACK TypeEnumerationCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext);
    static BOOL CALLBACK SymbolEnumerationCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext);

private:
    void AddSourceFile(const GUID& guid, const std::wstring& fileName);
    std::shared_ptr<CTraceSourceFile> FindSourceFile(const GUID& sourceFile);
    CTraceProvider* FindOrCreateProvider(const std::wstring& providerName, const GUID& providerGuid);
    CTraceProvider* FindProviderByFlag(const std::wstring& sFlagName);

private:
    std::deque<std::shared_ptr<STraceFormatEntry>> m_TempFormatEntries;
    std::map<GUID, std::shared_ptr<CTraceSourceFile>, CGUIDComparer> m_TempSourceFiles;
    std::map<std::wstring, CTraceProvider*> m_TempProviders;
    std::wstring m_FileName;
};