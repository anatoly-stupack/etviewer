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
#include <windows.h>
#include <set>
#include <map>
#include <string>
#include <vector>
#include <deque>
#include <dbghelp.h>

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

class CGUIDComparer
{
public :

	bool operator ()(const GUID &ref1, const GUID &ref2) const 
	{
		return memcmp(&ref1,&ref2,sizeof(GUID))<0;
	}

};

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
	eTraceFormatElementType_AnsiString,
	eTraceFormatElementType_UnicodeString
};

struct STraceFormatElement
{
	CHAR					*pFormatString;
	eTraceFormatElementType	 eType;

	STraceFormatElement():eType(eTraceFormatElementType_Unknown),pFormatString(NULL){}
};

struct STraceFormatParam
{
	eTraceFormatElementType	 eType;

	STraceFormatParam():eType(eTraceFormatElementType_Unknown){}
};
class CTraceSourceFile;
class CTraceProvider;

struct STraceFormatEntry
{
	GUID						m_SourceFileGUID;
	DWORD						m_dwIndex;
	DWORD						m_dwLine;
	std::vector<STraceFormatElement> m_vElements;
	std::vector<STraceFormatParam>	m_vParams;
	std::string						m_sFunction;
	std::string						m_sLevel;
	std::string						m_sFlag;
	CTraceSourceFile			*m_pSourceFile;
	CTraceProvider				*m_pProvider;

	void InitializeFromBuffer(char *pBuffer);

	STraceFormatEntry();
	~STraceFormatEntry();
};

struct STraceFormatEntryKey
{
	GUID	m_SourceFileGUID;
	DWORD	m_dwIndex;

	bool operator <(const STraceFormatEntryKey& other) const
	{
		int nComp=memcmp(&m_SourceFileGUID,&other.m_SourceFileGUID,sizeof(GUID));
		if(nComp<0){return true;}
		if(nComp>0){return false;}
		return m_dwIndex<other.m_dwIndex;
	}

	STraceFormatEntryKey(STraceFormatEntry *pValue){m_SourceFileGUID=pValue->m_SourceFileGUID;m_dwIndex=pValue->m_dwIndex;}
	STraceFormatEntryKey(GUID &sourceFileGUID,DWORD dwIndex){m_SourceFileGUID=sourceFileGUID;m_dwIndex=dwIndex;}
};

class CTraceProvider;

class CTraceSourceFile
{
	GUID	m_SourceFileGUID;
	CHAR	m_SourceFileName[MAX_PATH];

	CTraceProvider *m_pProvider;

public:

	GUID			GetGUID();
	std::string			GetFileName();
	CTraceProvider *GetProvider();
	void			SetProvider(CTraceProvider *pProvider);

	CTraceSourceFile(GUID sourceFileGUID,const char *pSourceFile);
	~CTraceSourceFile(void);
};


class CTraceProvider
{
	GUID						m_ProviderGUID;
	std::map<std::string,DWORD>	m_TraceFlags;
	std::string					m_sComponentName;
	std::string					m_sFileName;
	DWORD						m_dwAllSupportedFlagsMask;

	std::vector<STraceFormatEntry *>				m_FormatEntries;
	std::map<GUID,CTraceSourceFile *,CGUIDComparer>	m_sSourceFiles;

	void FreeAll();

public:

	GUID	GetGUID();
	void	SetGUID(GUID guid);

	DWORD	GetAllSupportedFlagsMask();
	void	GetSupportedFlags(std::map<std::string,DWORD> *pmFlags);
	void	AddSupportedFlag(std::string sName,DWORD dwValue);
	DWORD	GetSupportedFlagValue(std::string sName);

	std::string	GetComponentName();
	std::string	GetFileName();

	void				AddSourceFile(CTraceSourceFile *pSourceFile);
	void 				GetSourceFiles(std::vector<CTraceSourceFile*> *pvSources);
	CTraceSourceFile*	GetSourceFile(GUID sourceGUID);

	void				AddFormatEntry(STraceFormatEntry *pFormatEntry);
	void 				GetFormatEntries(std::vector<STraceFormatEntry*> *pvFormatEntries);


	CTraceProvider(std::string sComponentName,std::string sFileName);
	~CTraceProvider(void);
};

enum eTracePDBReaderError
{
	eTracePDBReaderError_Success,
	eTracePDBReaderError_Generic,
	eTracePDBReaderError_PDBNotFound,
	eTracePDBReaderError_NoProviderFoundInPDB,
	eTracePDBReaderError_FailedToEnumerateSymbols,
	eTracePDBReaderError_DBGHelpNotFound,
	eTracePDBReaderError_DBGHelpWrongVersion,
	eTracePDBReaderError_SymEngineLoadModule,
	eTracePDBReaderError_SymEngineInitFailed,
	eTracePDBReaderError_NoMemory,
};

class CTracePDBReader
{
	std::deque<STraceFormatEntry *>					m_sTempFormatEntries;
	std::map<GUID,CTraceSourceFile*,CGUIDComparer>	m_sTempSourceFiles;
	std::map<std::string,CTraceProvider*>			m_sTempProviders;
	std::string										m_sFileName;

	void				 AddSourceFile(GUID guid,std::string sFileName);
	CTraceSourceFile	*FindSourceFile(GUID sourceFile);
	CTraceProvider		*FindOrCreateProvider(std::string sProviderName);
	CTraceProvider		*FindProviderByFlag(std::string sFlagName);

	static BOOL CALLBACK SymbolEnumerationCallback(PSYMBOL_INFO pSymInfo,ULONG SymbolSize,PVOID UserContext);

public:

	 eTracePDBReaderError LoadFromPDB(const char *pPDB,std::vector<CTraceProvider *> *pvProviders);
};