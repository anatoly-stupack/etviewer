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
#include "PersistentSettings.h"

PersistentSettings::PersistentSettings()
    : m_BasePath(L"Software\\Etviewer\\")
{
}

PersistentSettings::~PersistentSettings()
{
}

bool PersistentSettings::ReadBoolValue(const std::wstring& name, bool default)
{
    return (ReadDwordValue(name, (DWORD)default) != FALSE);
}

DWORD PersistentSettings::ReadDwordValue(const std::wstring& name, DWORD default)
{
    DWORD bufferSize = sizeof(DWORD);
    DWORD value = 0;

    auto result = RegGetValue(HKEY_CURRENT_USER, m_BasePath.c_str(), name.c_str(), RRF_RT_REG_DWORD, nullptr, &value, &bufferSize);
    if (result != ERROR_SUCCESS)
    {
        return default;
    }

    return value;
}

std::wstring PersistentSettings::ReadStringValue(const std::wstring& name, const std::wstring& default)
{
    DWORD bufferSize = 0;
    std::wstring buffer;

    auto result = RegGetValue(HKEY_CURRENT_USER, m_BasePath.c_str(), name.c_str(), RRF_RT_REG_SZ, nullptr, nullptr, &bufferSize);
    if (result != ERROR_SUCCESS || bufferSize == 0)
    {
        return default;
    }

    buffer.resize(bufferSize / sizeof(wchar_t));

    result = RegGetValue(
        HKEY_CURRENT_USER,
        m_BasePath.c_str(),
        name.c_str(),
        RRF_RT_REG_SZ,
        nullptr,
        const_cast<wchar_t*>(buffer.data()),
        &bufferSize);

    if (result != ERROR_SUCCESS)
    {
        return default;
    }

    // Remove null terminator
    buffer.resize(bufferSize / sizeof(wchar_t) - 1);

    return buffer;
}

std::list<std::wstring> PersistentSettings::ReadMultiStringValue(const std::wstring& name, const std::list<std::wstring>& default)
{
    DWORD bufferSize = 0;
    std::wstring buffer;
    std::list<std::wstring> list;

    auto result = RegGetValue(
        HKEY_CURRENT_USER,
        m_BasePath.c_str(),
        name.c_str(),
        RRF_RT_REG_MULTI_SZ,
        nullptr,
        nullptr,
        &bufferSize);

    if (result != ERROR_SUCCESS || bufferSize == 0)
    {
        return default;
    }

    buffer.resize(bufferSize / sizeof(wchar_t));

    result = RegGetValue(
        HKEY_CURRENT_USER,
        m_BasePath.c_str(),
        name.c_str(),
        RRF_RT_REG_MULTI_SZ,
        nullptr,
        &buffer[0],
        &bufferSize);

    if (result != ERROR_SUCCESS)
    {
        return default;
    }

    buffer.resize(bufferSize / sizeof(wchar_t) - 1);

    std::wstring token;
    std::wistringstream parser(buffer);
    while (std::getline(parser, token, L'\0'))
    {
        list.push_back(token);
    }

    return list;
}

void PersistentSettings::WriteBoolValue(const std::wstring& name, bool value)
{
    WriteDwordValue(name, (DWORD)value);
}

void PersistentSettings::WriteDwordValue(const std::wstring& name, DWORD value)
{
    auto result = CreateBaseKey();
    if (result != ERROR_SUCCESS)
    {
        return;
    }

    result = RegSetKeyValue(
        HKEY_CURRENT_USER,
        m_BasePath.c_str(),
        name.c_str(),
        REG_DWORD,
        &value,
        sizeof(DWORD));

    if (result != ERROR_SUCCESS)
    {
        return;
    }
}

void PersistentSettings::WriteStringValue(const std::wstring& name, const std::wstring& value)
{
    auto result = CreateBaseKey();
    if (result != ERROR_SUCCESS)
    {
        return;
    }

    result = RegSetKeyValue(
        HKEY_CURRENT_USER,
        m_BasePath.c_str(),
        name.c_str(),
        REG_SZ,
        value.c_str(),
        (value.size() + 1) * sizeof(wchar_t));

    if (result != ERROR_SUCCESS)
    {
        return;
    }
}

void PersistentSettings::WriteMultiStringValue(const std::wstring& name, const std::list<std::wstring>& list)
{
    auto result = CreateBaseKey();
    if (result != ERROR_SUCCESS)
    {
        return;
    }

    std::wstring buffer;

    for (auto& item : list)
    {
        buffer += item;
        buffer.push_back(L'\0');
    }

    if (buffer.size() == 0)
    {
        return;
    }

    buffer.push_back(L'\0');

    result = RegSetKeyValue(
        HKEY_CURRENT_USER,
        m_BasePath.c_str(),
        name.c_str(),
        REG_MULTI_SZ,
        buffer.data(),
        buffer.size() * sizeof(wchar_t));

    if (result != ERROR_SUCCESS)
    {
        return;
    }
}

DWORD PersistentSettings::CreateBaseKey()
{
    HKEY key = nullptr;
    auto result = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        m_BasePath.c_str(),
        0,
        nullptr,
        0,
        KEY_QUERY_VALUE,
        nullptr,
        &key,
        nullptr);

    if (result != ERROR_SUCCESS)
    {
        return result;
    }

    RegCloseKey(key);

    return result;
}