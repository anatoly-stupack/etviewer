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
#include "TraceSourceFile.h"

CTraceSourceFile::CTraceSourceFile(const GUID& sourceFileGUID, const std::wstring& sourceFile, CTraceProvider* provider)
    : m_SourceFileGUID(sourceFileGUID)
    , m_SourceFileNameWithPath(sourceFile)
    , m_SourceFileName(wcsrchr(m_SourceFileNameWithPath.c_str(), L'\\') + 1) // TODO: use C++ function
    , m_Provider(provider)
{
}

const GUID& CTraceSourceFile::GetGUID() const
{
    return m_SourceFileGUID;
}

const std::wstring& CTraceSourceFile::GetFileName() const
{
    return m_SourceFileName;
}

const std::wstring& CTraceSourceFile::GetFileNameWithPath() const
{
    return m_SourceFileNameWithPath;
}

const CTraceProvider* CTraceSourceFile::GetProvider() const
{
    return m_Provider;
}