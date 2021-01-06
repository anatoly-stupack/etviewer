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

TEST(Persistency, BooleanValue)
{
    PersistentSettings settings;

    settings.WriteBoolValue(L"BooleanValue", true);

    ASSERT_EQ(settings.ReadBoolValue(L"BooleanValue", false), true);
}

TEST(Persistency, BooleanValueUseDefault)
{
    PersistentSettings settings;

    ASSERT_EQ(settings.ReadBoolValue(L"NonExistentBooleanValue", false), false);

    ASSERT_EQ(settings.ReadBoolValue(L"NonExistentBooleanValue", true), true);
}

TEST(Persistency, DwordValue)
{
    PersistentSettings settings;

    settings.WriteDwordValue(L"DwordValue", 42);

    ASSERT_EQ(settings.ReadDwordValue(L"DwordValue", 0), 42);
}

TEST(Persistency, DwordValueUseDefault)
{
    PersistentSettings settings;

    ASSERT_EQ(settings.ReadDwordValue(L"NonExistentDwordValue", 0), 0);

    ASSERT_EQ(settings.ReadDwordValue(L"NonExistentDwordValue", 42), 42);
}

TEST(Persistency, StringValue)
{
    PersistentSettings settings;

    settings.WriteStringValue(L"StringValue", L"string");

    ASSERT_EQ(settings.ReadStringValue(L"StringValue", L""), L"string");
}

TEST(Persistency, StringValueUseDefault)
{
    PersistentSettings settings;

    ASSERT_EQ(settings.ReadStringValue(L"NonExistentStringValue", L""), L"");

    ASSERT_EQ(settings.ReadStringValue(L"NonExistentStringValue", L"defaultString"), L"defaultString");
}

TEST(Persistency, MultiStringValue)
{
    PersistentSettings settings;

    std::list<std::wstring> list = { L"string1", L"string2" };

    settings.WriteMultiStringValue(L"MultiStringValue", list);

    ASSERT_EQ(settings.ReadMultiStringValue(L"MultiStringValue", { }), list);
}

TEST(Persistency, MultiStringValueUseDefault)
{
    PersistentSettings settings;

    std::list<std::wstring> emptyList;
    ASSERT_EQ(settings.ReadMultiStringValue(L"NonExistentMultiStringValue", {}), emptyList);

    std::list<std::wstring> notEmptyList = { L"string1", L"string2" };
    ASSERT_EQ(settings.ReadMultiStringValue(L"NonExistentMultiStringValue", { L"string1", L"string2" }), notEmptyList);
}
