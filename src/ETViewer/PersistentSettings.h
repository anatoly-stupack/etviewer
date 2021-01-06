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

class PersistentSettings final
{
public:
    PersistentSettings();
    ~PersistentSettings();

    bool ReadBoolValue(const std::wstring& name, bool default);

    DWORD ReadDwordValue(const std::wstring& name, DWORD default);

    std::wstring ReadStringValue(const std::wstring& name, const std::wstring& default);

    std::list<std::wstring> ReadMultiStringValue(const std::wstring& name, const std::list<std::wstring>& default);

    void WriteBoolValue(const std::wstring& name, bool value);

    void WriteDwordValue(const std::wstring& name, DWORD value);

    void WriteStringValue(const std::wstring& name, const std::wstring& value);

    void WriteMultiStringValue(const std::wstring& name, const std::list<std::wstring>& value);

private:
    const std::wstring m_BasePath;
};