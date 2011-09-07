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

#include ".\traceprovider.h"
#include "Dbghelp.h"

using namespace std;

#define PARAMETER_INDEX_BASE 10


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
//							STraceFormatEntry
//
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


typedef BOOL (WINAPI *pfSymSearch)(HANDLE hProcess,ULONG64 BaseOfDll,DWORD Index,DWORD SymTag,PCTSTR Mask,DWORD64 Address,PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,PVOID UserContext,DWORD Options);
eTraceFormatElementType GetElementTypeFromString(char *pString)
{
	if(strcmp(pString,"ItemString")==0){return eTraceFormatElementType_AnsiString;}
	if(strcmp(pString,"ItemWString")==0){return eTraceFormatElementType_UnicodeString;}
	if(strcmp(pString,"ItemLong")==0){return eTraceFormatElementType_DWord;}
	if(strcmp(pString,"ItemChar")==0){return eTraceFormatElementType_Byte;}
	if(strcmp(pString,"ItemDouble")==0){return eTraceFormatElementType_Double;}
	if(strcmp(pString,"ItemFloat")==0){return eTraceFormatElementType_Float;}
	if(strcmp(pString,"ItemLongLong")==0){return eTraceFormatElementType_Quad;}
	if(strcmp(pString,"ItemULongLong")==0){return eTraceFormatElementType_Quad;}
	if(strcmp(pString,"ItemLongLongX")==0){return eTraceFormatElementType_Quad;}
	if(strcmp(pString,"ItemLongLongXX")==0){return eTraceFormatElementType_Quad;}
	if(strcmp(pString,"ItemULongLongX")==0){return eTraceFormatElementType_Quad;}
	if(strcmp(pString,"ItemULongLongXX")==0){return eTraceFormatElementType_Quad;}
	if(strcmp(pString,"ItemPtr")==0){return eTraceFormatElementType_Pointer;}
	return eTraceFormatElementType_Unknown;
}

STraceFormatEntry::STraceFormatEntry()
{
	m_dwIndex=0;
	m_SourceFileGUID=GUID_NULL;
	m_pSourceFile=NULL;
	m_pProvider=NULL;
}

STraceFormatEntry::~STraceFormatEntry()
{
	unsigned x;
	for(x=0;x<m_vElements.size();x++)
	{
		char *pFormat=m_vElements[x].pFormatString;
		delete [] pFormat;
	}
	m_vElements.clear();
	m_dwIndex=0;
}

void STraceFormatEntry::InitializeFromBuffer(char *pBuffer)
{
	int len=(int)strlen(pBuffer),tempLen=0;
	char *pTempBuffer=new char [len+1];
	int x;
	for(x=0;x<len;)
	{
		if(pBuffer[x]=='%' && pBuffer[x+1]!='%')
		{
			int nParameterIndexLen=0;
			char sParamIndex[100]={0};

			if(tempLen)
			{
				pTempBuffer[tempLen]=0;

				STraceFormatElement element;
				element.eType=eTraceFormatElementType_ConstantString;
				element.pFormatString=new char [tempLen+1];
				strcpy(element.pFormatString,pTempBuffer);	
				m_vElements.push_back(element);
				tempLen=0;
			}

			STraceFormatElement element;

			//Copy initial '%' character
			pTempBuffer[tempLen]=pBuffer[x];x++;tempLen++;
			// Read parameter index.
			while(pBuffer[x]>='0' && pBuffer[x]<='9')
			{
				sParamIndex[nParameterIndexLen]=pBuffer[x];
				nParameterIndexLen++;
				x++;
			}

			x++; //Skip initial '!' character
			while(pBuffer[x]!='!')
			{
				pTempBuffer[tempLen]=pBuffer[x];
				x++;tempLen++;
			}
			x++; //Skip final '!' character
			pTempBuffer[tempLen]=0;
			if(tempLen)
			{
				int nParameterIndex=atoi(sParamIndex)-PARAMETER_INDEX_BASE;

				STraceFormatElement element;
				if(nParameterIndex>=0 && nParameterIndex<(int)m_vParams.size())
				{
					element.eType=m_vParams[nParameterIndex].eType;
				}
				pTempBuffer[tempLen]=0;
				element.pFormatString=new char [tempLen+1];
				strcpy(element.pFormatString,pTempBuffer);	

				if(tempLen)
				{
					// Modify the format buffer. ETW formats 'doubles' as %s instead of '%f'.
					if(element.eType==eTraceFormatElementType_Float || 
						element.eType==eTraceFormatElementType_Double)
					{
						if(element.pFormatString[tempLen-1]=='s'){element.pFormatString[tempLen-1]='f';}
					}
				}

				m_vElements.push_back(element);
				pTempBuffer[0]=0;
				tempLen=0;
			}
		}
		else
		{
			pTempBuffer[tempLen]=pBuffer[x];		
			tempLen++;
			x++;
		}
	}
	if(tempLen)
	{
		pTempBuffer[tempLen]=0;

		STraceFormatElement element;
		element.eType=eTraceFormatElementType_ConstantString;
		element.pFormatString=new char [tempLen+1];
		strcpy(element.pFormatString,pTempBuffer);	
		m_vElements.push_back(element);
		tempLen=0;
	}
	delete [] pTempBuffer;
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
//							CTraceSourceFile
//
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


CTraceSourceFile::CTraceSourceFile(GUID sourceFileGUID, const char *pSourceFile)
{
	m_pProvider=NULL;
	m_SourceFileGUID=sourceFileGUID;
	strcpy(m_SourceFileName,pSourceFile);
}

CTraceSourceFile::~CTraceSourceFile(void)
{
}

GUID CTraceSourceFile::GetGUID()
{
	return m_SourceFileGUID;
}

string CTraceSourceFile::GetFileName()
{
	return m_SourceFileName;
}

CTraceProvider* CTraceSourceFile::GetProvider()
{
	return m_pProvider;
}

void CTraceSourceFile::SetProvider(CTraceProvider *pProvider)
{
	m_pProvider=pProvider;
}


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
//							CTraceProvider
//
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


CTraceProvider::CTraceProvider(string sComponentName,string sFileName)
{
	m_sComponentName=sComponentName;
	m_sFileName=sFileName;
	m_dwAllSupportedFlagsMask=0;
	m_ProviderGUID=GUID_NULL;
}

CTraceProvider::~CTraceProvider(void)
{
	FreeAll();
}

void CTraceProvider::FreeAll(void)
{
	unsigned x;

	map<GUID,CTraceSourceFile *,CGUIDComparer>::iterator i;
	for(i=m_sSourceFiles.begin();i!=m_sSourceFiles.end();i++)
	{
		CTraceSourceFile *pSource=i->second;
		delete pSource;
	}
	m_sSourceFiles.clear();

	for(x=0;x<m_FormatEntries.size();x++)
	{
		STraceFormatEntry *pFormatEntry=m_FormatEntries[x];
		delete pFormatEntry;
	}
	m_FormatEntries.clear();
}

GUID CTraceProvider::GetGUID()
{
	return m_ProviderGUID;
}

void CTraceProvider::SetGUID(GUID guid)
{
	m_ProviderGUID=guid;
}

string CTraceProvider::GetComponentName()
{
	return m_sComponentName;
}

string CTraceProvider::GetFileName()
{
	return m_sFileName;
}

void CTraceProvider::AddSourceFile(CTraceSourceFile *pSourceFile)
{
	m_sSourceFiles[pSourceFile->GetGUID()]=pSourceFile;
}

void CTraceProvider::GetSourceFiles(vector<CTraceSourceFile*> *pvSources)
{
	map<GUID,CTraceSourceFile *,CGUIDComparer>::iterator i;
	for(i=m_sSourceFiles.begin();i!=m_sSourceFiles.end();i++)
	{
		pvSources->push_back(i->second);
	}
}

CTraceSourceFile* CTraceProvider::GetSourceFile(GUID sourceGUID)
{
	map<GUID,CTraceSourceFile *,CGUIDComparer>::iterator i;
	i=m_sSourceFiles.find(sourceGUID);
	if(i!=m_sSourceFiles.end()){return i->second;}
	return 0;
}

void CTraceProvider::AddFormatEntry(STraceFormatEntry *pFormatEntry)
{
	m_FormatEntries.push_back(pFormatEntry);
}

void CTraceProvider::GetFormatEntries(vector<STraceFormatEntry*> *pvFormatEntries)
{
	unsigned x;
	for(x=0;x<m_FormatEntries.size();x++)
	{
		STraceFormatEntry *pFormatEntry=m_FormatEntries[x];
		pvFormatEntries->push_back(pFormatEntry);
	}
}

DWORD CTraceProvider::GetAllSupportedFlagsMask(){return m_dwAllSupportedFlagsMask;}
void  CTraceProvider::GetSupportedFlags(map<string,DWORD> *pmFlags){*pmFlags=m_TraceFlags;}
void  CTraceProvider::AddSupportedFlag(string sName,DWORD dwValue)
{
	m_TraceFlags[sName]=dwValue;
	m_dwAllSupportedFlagsMask|=dwValue;
}
DWORD CTraceProvider::GetSupportedFlagValue(string sName)
{
	map<string,DWORD>::iterator i;
	i=m_TraceFlags.find(sName);
	if(i!=m_TraceFlags.end()){return i->second;}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
//
//							CTracePDBReader
//
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

CTraceProvider *CTracePDBReader::FindOrCreateProvider(string sProviderName)
{
	map<string,CTraceProvider*>::iterator iProvider;

	iProvider=m_sTempProviders.find(sProviderName);
	if(iProvider!=m_sTempProviders.end())
	{
		return iProvider->second;
	}
	else
	{
		CTraceProvider *pProvider=new CTraceProvider(sProviderName,m_sFileName);
		m_sTempProviders[sProviderName]=pProvider;
		return pProvider;
	}
}
CTraceSourceFile *CTracePDBReader::FindSourceFile(GUID sourceFile)
{
	map<GUID,CTraceSourceFile*,CGUIDComparer>::iterator iSourceFile;

	iSourceFile=m_sTempSourceFiles.find(sourceFile);
	if(iSourceFile!=m_sTempSourceFiles.end())
	{
		return iSourceFile->second;
	}
	return NULL;
}

CTraceProvider *CTracePDBReader::FindProviderByFlag(string sFlagName)
{
	map<string,CTraceProvider*>::iterator iProvider;

	for(iProvider=m_sTempProviders.begin();iProvider!=m_sTempProviders.end();iProvider++)
	{
		CTraceProvider *pProvider=iProvider->second;
		if(pProvider->GetSupportedFlagValue(sFlagName))
		{
			return pProvider;
		}
	}
	return NULL;
}

void CTracePDBReader::AddSourceFile(GUID guid,string sFileName)
{
	map<GUID,CTraceSourceFile*,CGUIDComparer>::iterator iSourceFile;

	iSourceFile=m_sTempSourceFiles.find(guid);
	if(iSourceFile==m_sTempSourceFiles.end())
	{
		CTraceSourceFile *pSourceFile=new CTraceSourceFile(guid,sFileName.c_str());
		m_sTempSourceFiles[guid]=pSourceFile;
	}
}

eTracePDBReaderError CTracePDBReader::LoadFromPDB(const char *pPDB,vector<CTraceProvider *> *pvProviders)
{
	eTracePDBReaderError eResult=eTracePDBReaderError_Generic;

	m_sFileName=pPDB;

	pfSymSearch pSymSearch=NULL;

	HANDLE hFile=CreateFile(pPDB,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		DWORD dwResult=GetLastError();
		return eTracePDBReaderError_PDBNotFound;
	}
	DWORD dwFileSize=GetFileSize(hFile,NULL);
	CloseHandle(hFile);
	hFile=NULL;

	HMODULE hDbgHelp=LoadLibrary("dbghelp.dll");
	if(hDbgHelp)
	{
		pSymSearch=(pfSymSearch)GetProcAddress(hDbgHelp,"SymSearch");
		if(pSymSearch)
		{
			VOID *pVirtual=VirtualAlloc(NULL,dwFileSize,MEM_RESERVE,PAGE_READWRITE);
			if(pVirtual)
			{
				HANDLE hProcess=GetCurrentProcess();
				BOOL bOk=SymInitialize(hProcess,"",FALSE);
				if(bOk)
				{
					DWORD64 dwModuleBase=SymLoadModuleEx(hProcess,NULL,const_cast<char*>(pPDB),const_cast<char*>(pPDB),(DWORD64)pVirtual,dwFileSize,NULL,0);
					if(dwModuleBase)
					{
						// 8 = SymTagAnnotation
						if(!pSymSearch(hProcess,dwModuleBase,0,8,0,0,SymbolEnumerationCallback,this,2))
						{
							eResult=eTracePDBReaderError_FailedToEnumerateSymbols;
						}
						else if(m_sTempProviders.size())
						{
							eResult=eTracePDBReaderError_Success;
						}
						else
						{
							eResult=eTracePDBReaderError_NoProviderFoundInPDB;
						}
						SymUnloadModule64(hProcess,dwModuleBase);
					}
					else
					{
						eResult=eTracePDBReaderError_SymEngineLoadModule;
					}
					SymCleanup(hProcess);
				}
				else
				{
					eResult=eTracePDBReaderError_SymEngineInitFailed;
				}

				VirtualFree(pVirtual,dwFileSize,MEM_RELEASE);
				pVirtual=NULL;
			}
			else
			{
				eResult=eTracePDBReaderError_NoMemory;
			}
		}
		else
		{
			eResult=eTracePDBReaderError_DBGHelpWrongVersion;
		}
		FreeLibrary(hDbgHelp);
		hDbgHelp=NULL;
	}
	else
	{
		eResult=eTracePDBReaderError_DBGHelpNotFound;
	}

	if(eResult!=eTracePDBReaderError_Success)
	{
		map<string,CTraceProvider*>::iterator iProvider;

		for(iProvider=m_sTempProviders.begin();iProvider!=m_sTempProviders.end();iProvider++)
		{
			CTraceProvider *pProvider=iProvider->second;
			delete pProvider;
		}
		m_sTempProviders.clear();
	}
	else
	{
		map<string,CTraceProvider*>::iterator iSource;

		for(iSource=m_sTempProviders.begin();iSource!=m_sTempProviders.end();iSource++)
		{
			pvProviders->push_back(iSource->second);
		}		
	}

	unsigned x;

	// Classification and initialization of all read format entries

	for(x=0;x<m_sTempFormatEntries.size();x++)
	{
		STraceFormatEntry *pFormatEntry=m_sTempFormatEntries[x];

		// Search the provider by flag string.

		pFormatEntry->m_pProvider=FindProviderByFlag(pFormatEntry->m_sFlag);
		if(!pFormatEntry->m_pProvider)
		{
			delete pFormatEntry;
			continue;
		}
	
		CTraceSourceFile *pSourceFile=pFormatEntry->m_pProvider->GetSourceFile(pFormatEntry->m_SourceFileGUID);
		if(!pSourceFile)
		{
			// Find the read source and use it as a template to create a new CTraceSource instance,
			// this is done this way because each provider will hace its own source file instance, and 
			// two different providers can trace from the same source file.

			CTraceSourceFile *pSourceFileTemplate=FindSourceFile(pFormatEntry->m_SourceFileGUID);
			if(!pSourceFileTemplate)
			{
				delete pFormatEntry;
				continue;
			}

			pSourceFile=new CTraceSourceFile(pSourceFileTemplate->GetGUID(),pSourceFileTemplate->GetFileName().c_str());
			pFormatEntry->m_pProvider->AddSourceFile(pSourceFile);
			pSourceFile->SetProvider(pFormatEntry->m_pProvider);
		}

		pFormatEntry->m_pSourceFile=pSourceFile;
		pFormatEntry->m_pProvider->AddFormatEntry(pFormatEntry);

	}

	// Free the source file instances used as templates

	map<GUID,CTraceSourceFile*,CGUIDComparer>::iterator iProvider;
	for(iProvider=m_sTempSourceFiles.begin();iProvider!=m_sTempSourceFiles.end();iProvider++)
	{
		CTraceSourceFile *pSource=iProvider->second;
		delete pSource;
	}
	m_sTempSourceFiles.clear();
	m_sTempFormatEntries.clear();
	m_sTempProviders.clear();
	return eResult;
}

BOOL CALLBACK CTracePDBReader::SymbolEnumerationCallback(PSYMBOL_INFO pSymInfo,ULONG SymbolSize,PVOID UserContext)
{
	CTracePDBReader *pThis=(CTracePDBReader*)UserContext;

	char *pTemp=pSymInfo->Name;
	char *pCompName=NULL;
	char *pGUIDName=NULL;
	char *pFlagName=NULL;
	BOOL bOK=FALSE;
	// 8 = SymTagAnnotation
	if(pSymInfo->Tag==8)
	{
		if(strncmp(pSymInfo->Name,"TMC:",4)==0)
		{
			GUID providerGUID=GUID_NULL;
			pTemp=pTemp+strlen(pTemp)+1;
			pGUIDName=pTemp;
			pTemp=pTemp+strlen(pTemp)+1;
			pCompName=pTemp;
			pTemp=pTemp+strlen(pTemp)+1;

			WCHAR wsCLSID[100]={0};
			swprintf(wsCLSID,L"{%S}",pGUIDName);
			CLSIDFromString(wsCLSID,&providerGUID);
			

			CTraceProvider *pProvider=pThis->FindOrCreateProvider(pCompName);

			pProvider->SetGUID(providerGUID);

			DWORD dwTraceValue=1;

			while(((ULONG)(pTemp-pSymInfo->Name))<pSymInfo->NameLen)
			{
				pProvider->AddSupportedFlag(pTemp,dwTraceValue);
				pTemp=pTemp+strlen(pTemp)+1;
				dwTraceValue<<=1;
			}

		}
		if(strncmp(pSymInfo->Name,"TMF:",4)==0 && pSymInfo->NameLen)
		{
			string sCompName;
			char sTraceLevel[MAX_PATH]={0};
			char sTraceFlag[MAX_PATH]={0};
			GUID sSourceFileGUID=GUID_NULL;
			DWORD dwTraceId=0;
			char *pFormatString=new char [pSymInfo->NameLen];
			pFormatString[0]=0;

			SYMBOL_INFO *pFunctionSymbol=(SYMBOL_INFO *)new char [sizeof(SYMBOL_INFO)+1024];
			memset(pFunctionSymbol,0,sizeof(SYMBOL_INFO)+1024);
			pFunctionSymbol->SizeOfStruct=sizeof(SYMBOL_INFO);
			pFunctionSymbol->MaxNameLen=1024;

			DWORD64 dwFunctionDisplacement=0;
			BOOL bFuncionOk=SymFromAddr(GetCurrentProcess(),pSymInfo->Address,&dwFunctionDisplacement,pFunctionSymbol);

			DWORD dwLineDisplacement=0;
			IMAGEHLP_LINE64 lineInfo={0};
			BOOL bLineOk=SymGetLineFromAddr64(GetCurrentProcess(),pSymInfo->Address,&dwLineDisplacement,&lineInfo);

			STraceFormatEntry *pFormatEntry=new STraceFormatEntry;

			int stringIndex=0;
			pTemp=pTemp+strlen(pTemp)+1;
			while(((ULONG)(pTemp-pSymInfo->Name))<pSymInfo->NameLen)
			{
				int nTempLen=(int)strlen(pTemp);
				char *pFullString=new char [nTempLen+1];
				strcpy(pFullString,pTemp);

		//		OutputDebugString(pTemp);
				if(stringIndex==0) // <GUID> <module> // SRC=<source>
				{
					int tokenindex=0;
					char *pToken=strtok(pFullString," ");
					if(pToken)
					{
						WCHAR wsCLSID[100]={0};
						swprintf(wsCLSID,L"{%S}",pToken);
						CLSIDFromString(wsCLSID,&sSourceFileGUID);
						pToken=strtok(NULL," ");
					}
					if(pToken)
					{
						sCompName=pToken;
					}
				}
				else if(stringIndex==1) // #typev <source:line> index "<format>" // LEVEL=<level> FLAGS=<flag>
				{	
					int startIndex=-1,endIndex=-1;

					// Search format string limits
					for(int x=0;pFullString[x]!=0;x++)
					{
						if(pFullString[x]=='"')
						{
							if(startIndex==-1)
							{
								startIndex=x+1;
							}
							endIndex=x;
						}					
					}
					// Format string is copied to another buffer and cleaned from the original buffer (through memset).
					// This is done to simplify string parsing.

					if(startIndex<endIndex)
					{
						if((endIndex-startIndex)>=2 && pFullString[startIndex]=='%' && pFullString[startIndex+1]=='0')
						{
							startIndex+=2;
						}
						memcpy(pFormatString,pFullString+startIndex,endIndex-startIndex);
						pFormatString[endIndex-startIndex]=0;
						memset(pFullString+startIndex+1,' ',endIndex-startIndex+2);
					}

					bool bFlagFound=false;

					// Search for the remaining useful tokens.
					int tokenIndex=0;
					char *pToken=strtok(pFullString," ");
					while(pToken)
					{
						if(tokenIndex==2)
						{
							dwTraceId=atoi(pToken);
						}
						else if(strncmp(pToken,"LEVEL=",6)==0)
						{
							strcpy(sTraceLevel,pToken+6);
						}
						else if(strncmp(pToken,"FLAGS=",6)==0)
						{
							bFlagFound=true;
							strcpy(sTraceFlag,pToken+6);
						}
						pToken=strtok(NULL," ");
						tokenIndex++;
					}
					if(!bFlagFound)
					{
						strcpy(sTraceFlag,sTraceLevel);
						strcpy(sTraceLevel,"TRACE_LEVEL_NONE");
					}
				}
				else if(pTemp[0]!='{' && pTemp[0]!='}')
				{
					int x;
					for(x=nTempLen-(int)strlen(", Item");x>0;x--)
					{
						if(strncmp(pFullString+x,", Item",strlen(", Item"))==0)
						{
							char sParamType[MAX_PATH]={0};
							char sParamIndex[MAX_PATH]={0};

							char *pToken=strtok(pFullString+x," ,");
							if(pToken){strcpy(sParamType,pToken);pToken=strtok(NULL," ,");}
							if(pToken){pToken=strtok(NULL," ,");}
							if(pToken){strcpy(sParamIndex,pToken);}
							STraceFormatParam parameter;

							int nParamIndex=atoi(sParamIndex)-PARAMETER_INDEX_BASE;
							if(nParamIndex>=0)
							{
								if((int)pFormatEntry->m_vParams.size()<nParamIndex+1)
								{
									pFormatEntry->m_vParams.resize(nParamIndex+1);
								}
								parameter.eType=GetElementTypeFromString(sParamType);
								pFormatEntry->m_vParams[nParamIndex]=parameter;
							}
						}
					}
				}
				pTemp=pTemp+nTempLen+1;
				stringIndex++;
				delete [] pFullString;
			}

			pFormatEntry->InitializeFromBuffer(pFormatString);
			pFormatEntry->m_SourceFileGUID=sSourceFileGUID;
			pFormatEntry->m_dwIndex=dwTraceId;
			pFormatEntry->m_dwLine=lineInfo.LineNumber;
			pFormatEntry->m_sFunction=pFunctionSymbol->Name;
			pFormatEntry->m_sLevel=sTraceLevel;
			pFormatEntry->m_sFlag=sTraceFlag;

			pThis->m_sTempFormatEntries.push_back(pFormatEntry);

			pThis->AddSourceFile(sSourceFileGUID,lineInfo.FileName);

			delete [] pFormatString;
			delete pFunctionSymbol;
		}
	}


	return TRUE;
}	
