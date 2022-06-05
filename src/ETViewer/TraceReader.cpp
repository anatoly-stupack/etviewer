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
#include "TraceReader.h"
#include "TraceProvider.h"
#include <cvconst.h>

struct STypeEnumerationData
{
    bool			b64BitPDB;
    HANDLE          hProcess;
    DWORD64         nBaseAddress;
    CTraceReader* pThis;
    STypeEnumerationData() { pThis = NULL; hProcess = NULL; b64BitPDB = false; nBaseAddress = 0; }
};

struct SSymbolEnumerationData
{
    bool			b64BitPDB;
    CTraceReader* pThis;
    SSymbolEnumerationData() { pThis = NULL; b64BitPDB = false; }
};

eTraceFormatElementType GetElementTypeFromString(TCHAR* pString, bool b64BitPDB)
{
    if (_tcscmp(pString, _T("ItemString")) == 0) { return eTraceFormatElementType_AnsiString; }
    if (_tcscmp(pString, _T("ItemWString")) == 0) { return eTraceFormatElementType_WideString; }
    if (_tcscmp(pString, _T("ItemPWString")) == 0) { return eTraceFormatElementType_CountedWideString; }
    if (_tcscmp(pString, _T("ItemLong")) == 0) { return eTraceFormatElementType_DWord; }
    if (_tcscmp(pString, _T("ItemChar")) == 0) { return eTraceFormatElementType_Byte; }
    if (_tcscmp(pString, _T("ItemDouble")) == 0) { return eTraceFormatElementType_Double; }
    if (_tcscmp(pString, _T("ItemFloat")) == 0) { return eTraceFormatElementType_Float; }
    if (_tcscmp(pString, _T("ItemLongLong")) == 0) { return eTraceFormatElementType_Quad; }
    if (_tcscmp(pString, _T("ItemULongLong")) == 0) { return eTraceFormatElementType_Quad; }
    if (_tcscmp(pString, _T("ItemLongLongX")) == 0) { return eTraceFormatElementType_Quad; }
    if (_tcscmp(pString, _T("ItemLongLongXX")) == 0) { return eTraceFormatElementType_Quad; }
    if (_tcscmp(pString, _T("ItemULongLongX")) == 0) { return eTraceFormatElementType_Quad; }
    if (_tcscmp(pString, _T("ItemULongLongXX")) == 0) { return eTraceFormatElementType_Quad; }
    if (_tcscmp(pString, _T("ItemPtr")) == 0) { return b64BitPDB ? eTraceFormatElementType_QuadPointer : eTraceFormatElementType_Pointer; }
    return eTraceFormatElementType_Unknown;

    /*
    Types that could use handlers
    ItemIPAddr
    ItemPort
    ItemNTSTATUS
    ItemWINERROR
    ItemHRESULT
    ItemNDIS_STATUS
    */
}

typedef BOOL(WINAPI* pfSymSearch)(HANDLE hProcess, ULONG64 BaseOfDll, DWORD Index, DWORD SymTag, PCTSTR Mask, DWORD64 Address, PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback, PVOID UserContext, DWORD Options);
typedef BOOL(WINAPI* pfSymEnumTypes)(HANDLE hProcess, ULONG64 BaseOfDll, PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback, PVOID UserContext);

CTraceProvider* CTraceReader::FindOrCreateProvider(const std::wstring& sProviderName, const GUID& providerGuid)
{
    std::map<std::wstring, CTraceProvider*>::iterator iProvider;

    iProvider = m_TempProviders.find(sProviderName);
    if (iProvider != m_TempProviders.end())
    {
        return iProvider->second;
    }
    else
    {
        CTraceProvider* pProvider = new CTraceProvider(sProviderName, providerGuid, m_FileName);
        m_TempProviders[sProviderName] = pProvider;
        return pProvider;
    }
}

std::shared_ptr<CTraceSourceFile> CTraceReader::FindSourceFile(const GUID& sourceFile)
{
    auto entry = m_TempSourceFiles.find(sourceFile);
    if (entry == m_TempSourceFiles.end())
    {
        return nullptr;
    }

    return entry->second;
}

CTraceProvider* CTraceReader::FindProviderByFlag(const std::wstring& flagName)
{
    std::map<std::wstring, CTraceProvider*>::iterator iProvider;

    for (iProvider = m_TempProviders.begin(); iProvider != m_TempProviders.end(); iProvider++)
    {
        CTraceProvider* pProvider = iProvider->second;
        if (pProvider->GetSupportedFlagValue(flagName))
        {
            return pProvider;
        }
    }
    return NULL;
}

void CTraceReader::AddSourceFile(const GUID& guid, const std::wstring& fileName)
{
    auto entry = m_TempSourceFiles.find(guid);
    if (entry != m_TempSourceFiles.end())
    {
        return;
    }

    m_TempSourceFiles[guid] = std::make_shared<CTraceSourceFile>(guid, fileName, nullptr);
}

eTraceReaderError CTraceReader::LoadFromPDB(LPCTSTR pPDB, std::vector<CTraceProvider*>* pvProviders)
{
    eTraceReaderError eResult = eTraceReaderError_Generic;

    m_FileName = pPDB;

    pfSymSearch pSymSearch = NULL;
    pfSymEnumTypes pSymEnumTypes = NULL;

    HANDLE hFile = CreateFile(pPDB, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return eTraceReaderError_PDBNotFound;
    }
    DWORD dwFileSize = GetFileSize(hFile, NULL);
    CloseHandle(hFile);
    hFile = NULL;

    HMODULE hDbgHelp = LoadLibrary(_T("dbghelp.dll"));
    if (hDbgHelp)
    {
#ifdef UNICODE
        pSymSearch = (pfSymSearch)GetProcAddress(hDbgHelp, "SymSearchW");
        pSymEnumTypes = (pfSymEnumTypes)GetProcAddress(hDbgHelp, "SymEnumTypesW");
#else
        pSymSearch = (pfSymSearch)GetProcAddress(hDbgHelp, "SymSearch");
        pSymEnumTypes = (pfSymEnumTypes)GetProcAddress(hDbgHelp, "SymEnumTypes");
#endif

        if (pSymSearch && pSymEnumTypes)
        {
            VOID* pVirtual = VirtualAlloc(NULL, dwFileSize, MEM_RESERVE, PAGE_READWRITE);
            if (pVirtual)
            {
                HANDLE hProcess = GetCurrentProcess();
                BOOL bOk = SymInitialize(hProcess, _T(""), FALSE);
                if (bOk)
                {
                    DWORD64 dwModuleBase = SymLoadModuleEx(hProcess, NULL, pPDB, const_cast<LPCTSTR>(pPDB), (DWORD64)pVirtual, dwFileSize, NULL, 0);
                    if (dwModuleBase)
                    {

                        STypeEnumerationData typeData;
                        typeData.b64BitPDB = false;
                        typeData.hProcess = hProcess;
                        typeData.nBaseAddress = dwModuleBase;
                        typeData.pThis = this;

                        // Temporal solution to 64 bit %p parameters.
                        // %p parameters are identified as ItemPtr in both 32 and 64 bit PDBs
                        // We need to distinguish this two cases.
                        // For the time being we have no clean solution.
                        // The current solution is to find some well known pointer 
                        // types such as PVOID and check their sizes.

                        // Enum types to identify 64 bit PDBs
                        SymEnumTypes(hProcess, dwModuleBase, TypeEnumerationCallback, &typeData);

                        SSymbolEnumerationData symbolData;
                        symbolData.b64BitPDB = typeData.b64BitPDB;
                        symbolData.pThis = this;

                        // 8 = SymTagEnum::SymTagAnnotation
                        if (!pSymSearch(hProcess, dwModuleBase, 0, 8, 0, 0, SymbolEnumerationCallback, &symbolData, 2))
                        {
                            eResult = eTraceReaderError_FailedToEnumerateSymbols;
                        }
                        else if (m_TempProviders.size())
                        {
                            eResult = eTraceReaderError_Success;
                        }
                        else
                        {
                            eResult = eTraceReaderError_NoProviderFoundInPDB;
                        }
                        SymUnloadModule64(hProcess, dwModuleBase);
                    }
                    else
                    {
                        eResult = eTraceReaderError_SymEngineLoadModule;
                    }
                    SymCleanup(hProcess);
                }
                else
                {
                    eResult = eTraceReaderError_SymEngineInitFailed;
                }

                VirtualFree(pVirtual, 0, MEM_RELEASE);
                pVirtual = NULL;
            }
            else
            {
                eResult = eTraceReaderError_NoMemory;
            }
        }
        else
        {
            eResult = eTraceReaderError_DBGHelpWrongVersion;
        }
        FreeLibrary(hDbgHelp);
        hDbgHelp = NULL;
    }
    else
    {
        eResult = eTraceReaderError_DBGHelpNotFound;
    }

    if (eResult != eTraceReaderError_Success)
    {
        std::map<std::wstring, CTraceProvider*>::iterator iProvider;

        for (iProvider = m_TempProviders.begin(); iProvider != m_TempProviders.end(); iProvider++)
        {
            CTraceProvider* pProvider = iProvider->second;
            delete pProvider;
        }
        m_TempProviders.clear();
    }
    else
    {
        std::map<std::wstring, CTraceProvider*>::iterator iSource;

        for (iSource = m_TempProviders.begin(); iSource != m_TempProviders.end(); iSource++)
        {
            pvProviders->push_back(iSource->second);
        }
    }

    unsigned x;

    // Classification and initialization of all read format entries

    for (x = 0; x < m_TempFormatEntries.size(); x++)
    {
        auto pFormatEntry = m_TempFormatEntries[x];

        // Search the provider by flag string.

        pFormatEntry->m_pProvider = FindProviderByFlag(pFormatEntry->m_sFlag);
        if (!pFormatEntry->m_pProvider)
        {
            continue;
        }

        auto pSourceFile = pFormatEntry->m_pProvider->GetSourceFile(pFormatEntry->m_SourceFileGUID);
        if (!pSourceFile)
        {
            // Find the read source and use it as a template to create a new CTraceSource instance,
            // this is done this way because each provider will hace its own source file instance, and 
            // two different providers can trace from the same source file.

            auto pSourceFileTemplate = FindSourceFile(pFormatEntry->m_SourceFileGUID);
            if (!pSourceFileTemplate)
            {
                continue;
            }

            pSourceFile = std::make_shared<CTraceSourceFile>(pSourceFileTemplate->GetGUID(), pSourceFileTemplate->GetFileNameWithPath(), pFormatEntry->m_pProvider);
            pFormatEntry->m_pProvider->AddSourceFile(pSourceFile);
        }

        pFormatEntry->m_pSourceFile = pSourceFile;
        pFormatEntry->m_pProvider->AddFormatEntry(pFormatEntry);

    }

    // Free the source file instances used as templates
    m_TempSourceFiles.clear();
    m_TempFormatEntries.clear();
    m_TempProviders.clear();

    return eResult;
}

BOOL CALLBACK CTraceReader::TypeEnumerationCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    UNREFERENCED_PARAMETER(SymbolSize);

    if (pSymInfo->Name == NULL) { return TRUE; }
    if (pSymInfo->Tag != 17 || pSymInfo->Size != 8) { return TRUE; }

    STypeEnumerationData* pData = (STypeEnumerationData*)UserContext;

    if (_tcscmp(pSymInfo->Name, _T("HANDLE")) == 0 ||
        _tcscmp(pSymInfo->Name, _T("PBYTE")) == 0 ||
        _tcscmp(pSymInfo->Name, _T("PVOID")) == 0 ||
        _tcscmp(pSymInfo->Name, _T("UINT_PTR")) == 0 ||
        _tcscmp(pSymInfo->Name, _T("ULONG_PTR")) == 0)
    {
        pData->b64BitPDB = true;
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK CTraceReader::SymbolEnumerationCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext)
{
    UNREFERENCED_PARAMETER(SymbolSize);

    SSymbolEnumerationData* pData = (SSymbolEnumerationData*)UserContext;
    CTraceReader* pThis = pData->pThis;

    LPTSTR pTemp = pSymInfo->Name;
    LPTSTR pCompName = NULL;
    LPTSTR pGUIDName = NULL;

    // 8 = SymTagAnnotation
    if (pSymInfo->Tag == SymTagAnnotation)
    {
        if (_tcsncmp(pSymInfo->Name, _T("TMC:"), 4) == 0)
        {
            GUID providerGUID = GUID_NULL;
            pTemp = pTemp + _tcslen(pTemp) + 1;
            pGUIDName = pTemp;
            pTemp = pTemp + _tcslen(pTemp) + 1;
            pCompName = pTemp;
            pTemp = pTemp + _tcslen(pTemp) + 1;

            WCHAR wsCLSID[100] = { 0 };
#ifdef UNICODE
            StringCbPrintfW(wsCLSID, 100, L"{%s}", pGUIDName);
#else
            StringCbPrintfW(wsCLSID, 100, L"{%S}", pGUIDName);
#endif
            CLSIDFromString(wsCLSID, &providerGUID);

            CTraceProvider* pProvider = pThis->FindOrCreateProvider(pCompName, providerGUID);

            DWORD dwTraceValue = 1;

            while (((ULONG)(pTemp - pSymInfo->Name)) < pSymInfo->NameLen)
            {
                pProvider->AddSupportedFlag(pTemp, dwTraceValue);
                pTemp = pTemp + _tcslen(pTemp) + 1;
                dwTraceValue <<= 1;
            }

        }
        if (_tcsncmp(pSymInfo->Name, _T("TMF:"), 4) == 0 && pSymInfo->NameLen)
        {
            std::wstring sCompName;
            TCHAR sTraceLevel[MAX_PATH] = { 0 };
            TCHAR sTraceFlag[MAX_PATH] = { 0 };
            GUID sSourceFileGUID = GUID_NULL;
            DWORD dwTraceId = 0;
            LPTSTR pFormatString = new TCHAR[pSymInfo->NameLen];
            pFormatString[0] = 0;

            SYMBOL_INFO* pFunctionSymbol = (SYMBOL_INFO*)new char[sizeof(SYMBOL_INFO) + (1024 * sizeof(TCHAR))];
            memset(pFunctionSymbol, 0, sizeof(SYMBOL_INFO) + (1024 * sizeof(TCHAR)));
            pFunctionSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pFunctionSymbol->MaxNameLen = 1024;

            DWORD64 dwFunctionDisplacement = 0;
            SymFromAddr(GetCurrentProcess(), pSymInfo->Address, &dwFunctionDisplacement, pFunctionSymbol);

            DWORD dwLineDisplacement = 0;
            IMAGEHLP_LINE64 lineInfo = { 0 };
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            SymGetLineFromAddr64(GetCurrentProcess(), pSymInfo->Address, &dwLineDisplacement, &lineInfo);

            auto pFormatEntry = std::make_shared<STraceFormatEntry>();

            int stringIndex = 0;
            pTemp = pTemp + _tcslen(pTemp) + 1;
            while (((ULONG)(pTemp - pSymInfo->Name)) < pSymInfo->NameLen)
            {
                int nTempLen = (int)_tcslen(pTemp);
                TCHAR* pFullString = new TCHAR[nTempLen + 1];
                _tcscpy_s(pFullString, nTempLen + 1, pTemp);

                //OutputDebugString(pTemp);
                //OutputDebugString("\n");

                if (stringIndex == 0) // <GUID> <module> // SRC=<source>
                {
                    TCHAR* nextToken = NULL;
                    TCHAR* pToken = _tcstok_s(pFullString, _T(" "), &nextToken);
                    if (pToken)
                    {
                        WCHAR wsCLSID[100] = { 0 };
                        StringCbPrintfW(wsCLSID, 100, L"{%s}", pToken);
                        CLSIDFromString(wsCLSID, &sSourceFileGUID);
                        pToken = _tcstok_s(NULL, _T(" "), &nextToken);
                    }
                    if (pToken)
                    {
                        sCompName = pToken;
                    }
                }
                else if (stringIndex == 1) // #typev <source:line> index "<format>" // LEVEL=<level> FLAGS=<flag>
                {
                    int startIndex = -1, endIndex = -1;

                    // Search format string limits
                    for (int x = 0; pFullString[x] != 0; x++)
                    {
                        if (pFullString[x] == _T('"'))
                        {
                            if (startIndex == -1)
                            {
                                startIndex = x + 1;
                            }
                            endIndex = x;
                        }
                    }
                    // Format string is copied to another buffer and cleaned from the original buffer (through memset).
                    // This is done to simplify string parsing.

                    if (startIndex < endIndex)
                    {
                        if ((endIndex - startIndex) >= 2 && pFullString[startIndex] == _T('%') && pFullString[startIndex + 1] == _T('0'))
                        {
                            startIndex += 2;
                        }
                        _tcsncpy_s(pFormatString, pSymInfo->NameLen, pFullString + startIndex, endIndex - startIndex);
                        //memcpy(pFormatString,pFullString+startIndex,(endIndex-startIndex)*sizeof(TCHAR));
                        //pFormatString[endIndex-startIndex]=0;

                        wmemset(pFullString + startIndex - 2, _T(' '), ((endIndex - startIndex) + 2));
                    }

                    bool bFlagFound = false;

                    // Search for the remaining useful tokens.
                    int tokenIndex = 0;
                    TCHAR* nextToken = NULL;
                    TCHAR* pToken = _tcstok_s(pFullString, _T(" "), &nextToken);
                    while (pToken)
                    {
                        if (tokenIndex == 2)
                        {
                            dwTraceId = _ttoi(pToken);
                        }
                        else if (_tcsncmp(pToken, _T("LEVEL="), 6) == 0)
                        {
                            _tcscpy_s(sTraceLevel, pToken + 6);
                        }
                        else if (_tcsncmp(pToken, _T("FLAGS="), 6) == 0)
                        {
                            bFlagFound = true;
                            _tcscpy_s(sTraceFlag, pToken + 6);
                        }
                        pToken = _tcstok_s(NULL, _T(" "), &nextToken);
                        tokenIndex++;
                    }
                    if (!bFlagFound)
                    {
                        _tcscpy_s(sTraceFlag, sTraceLevel);
                        _tcscpy_s(sTraceLevel, _T("TRACE_LEVEL_NONE"));
                    }
                }
                else if (pTemp[0] != _T('{') && pTemp[0] != _T('}'))
                {
                    int x;
                    for (x = nTempLen - (int)_tcslen(_T(", Item")); x > 0; x--)
                    {
                        if (_tcsncmp(pFullString + x, _T(", Item"), _tcslen(_T(", Item"))) == 0)
                        {
                            TCHAR sParamType[MAX_PATH] = { 0 };
                            TCHAR sParamIndex[MAX_PATH] = { 0 };

                            LPTSTR nextToken = NULL;
                            LPTSTR pToken = _tcstok_s(pFullString + x, _T(" ,"), &nextToken);
                            if (pToken) { _tcscpy_s(sParamType, pToken); pToken = _tcstok_s(NULL, _T(" ,"), &nextToken); }
                            if (pToken) { pToken = _tcstok_s(NULL, _T(" ,"), &nextToken); }
                            if (pToken) { _tcscpy_s(sParamIndex, pToken); }
                            STraceFormatParam parameter;

                            int nParamIndex = _ttoi(sParamIndex) - PARAMETER_INDEX_BASE;
                            if (nParamIndex >= 0)
                            {
                                if ((int)pFormatEntry->m_vParams.size() < nParamIndex + 1)
                                {
                                    pFormatEntry->m_vParams.resize(nParamIndex + 1);
                                }
                                parameter.eType = GetElementTypeFromString(sParamType, pData->b64BitPDB);
                                pFormatEntry->m_vParams[nParamIndex] = parameter;
                            }
                        }
                    }
                }
                pTemp = pTemp + nTempLen + 1;
                stringIndex++;
                delete[] pFullString;
            }

            pFormatEntry->InitializeFromBuffer(pFormatString);
            pFormatEntry->m_SourceFileGUID = sSourceFileGUID;
            pFormatEntry->m_dwIndex = dwTraceId;
            pFormatEntry->m_dwLine = lineInfo.LineNumber;
            pFormatEntry->m_sFunction = pFunctionSymbol->Name;
            pFormatEntry->m_sLevel = sTraceLevel;
            pFormatEntry->m_sFlag = sTraceFlag;

            pThis->m_TempFormatEntries.push_back(pFormatEntry);

            pThis->AddSourceFile(sSourceFileGUID, lineInfo.FileName);

            delete[] pFormatString;
            delete[] pFunctionSymbol;
        }
    }
    return TRUE;
}