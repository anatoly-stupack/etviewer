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

class CConfigFile;
class CConfigFileNode;

class CConfigFileNode : public IPersistencyNode
{
    friend CConfigFile;

    std::wstring                                    m_sName;
    std::map<std::wstring, std::wstring>             m_mData;
    std::multimap<std::wstring, CConfigFileNode*>    m_mNodes;

    CConfigFileNode* GetAddNode_Internal(std::wstring sNodePath, bool bAdd);

public:

    bool                HasValue(std::wstring sValue);
    std::wstring        GetValue(std::wstring sValue);

    // IPersistencyNode
    virtual void Clear();
    virtual bool AddProperty(SPersistencyProperty);
    virtual bool GetProperty(SPersistencyProperty*);
    virtual bool RemoveProperty(SPersistencyProperty);

    virtual IPersistencyNode* AddNode(std::wstring id);
    virtual IPersistencyNode* GetNode(std::wstring id);
    virtual void DeleteNode(std::wstring id);

    ~CConfigFileNode();
};

class CConfigFile
{
    DWORD			m_dwSaveTabCount;
    HANDLE  	    m_hFile;
    TCHAR* m_pBuffer;

    CConfigFileNode m_RootNode;

    void SaveLine(std::wstring sValue);
    void SaveValue(std::wstring sName, std::wstring sValue);
    void SaveBeginSection(std::wstring sName);
    void SaveEndSection();
    void SaveNode(CConfigFileNode* pNode);

public:

    bool Open(std::wstring sFileName);
    bool Save(std::wstring sFileName);

    IPersistencyNode* AddNode(std::wstring  sNodePath);
    IPersistencyNode* GetNode(std::wstring  sNodePath);
    IPersistencyNode* GetRoot();

    CConfigFile(void);
    ~CConfigFile(void);
};

#define DECLARE_CONFIG_FILE_MEDIA()\
    bool SaveTo(CConfigFile *pFile,std::wstring sNode)\
    {\
        IPersistencyNode *piNode=pFile->AddNode(sNode);\
        if(piNode){return SUCCEEDED(PersistencySave(piNode));}else{return false;}\
    }\
    bool LoadFrom(CConfigFile *pFile,std::wstring sNode)\
    {\
        IPersistencyNode *piNode=pFile->GetNode(sNode);\
        if(piNode){return SUCCEEDED(PersistencyLoad(piNode));}else{return false;}\
    }
