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
#include <string>

// todo: remove backlinks
class CTraceProvider;

class CTraceSourceFile
{
public:
    CTraceSourceFile(const GUID& sourceFileGUID, const std::wstring& sourceFile, CTraceProvider* provider);

    const GUID& GetGUID() const;
    const std::wstring& GetFileName() const;
    const std::wstring& GetFileNameWithPath() const;
    const CTraceProvider* GetProvider() const;

private:
    const CTraceProvider* m_Provider;
    const GUID m_SourceFileGUID;
    const std::wstring m_SourceFileNameWithPath;
    const std::wstring m_SourceFileName;
};