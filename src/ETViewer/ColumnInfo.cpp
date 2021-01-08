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
#include "ColumnInfo.h"

CColumnInfo::CColumnInfo(int _id, TCHAR* n, int fmt, int w, bool v, int o)
{
    id = _id;
    format = fmt;
    width = w;
    visible = v;
    name = n;
    iSubItem = -1;
    iOrder = o;
}

CColumnInfo::CColumnInfo(const std::wstring& serialized)
{
    std::wstring token;
    std::wistringstream parser(serialized);
    std::vector<std::wstring> tokens;

    while (std::getline(parser, token, L';'))
    {
        tokens.emplace_back(token);
    }

    if (tokens.size() != 6)
    {
        return;
    }

    width = _wtoi(tokens[0].c_str());
    visible = _wtoi(tokens[1].c_str());
    name = tokens[2];
    format = _wtoi(tokens[3].c_str());
    id = _wtoi(tokens[4].c_str());
    iOrder = _wtoi(tokens[5].c_str());
}

std::wstring CColumnInfo::ToString() const
{
    std::wstringstream serializer;
    serializer << width << L";";
    serializer << visible << L";";
    serializer << name << L";";
    serializer << format << L";";
    serializer << id << L";";
    serializer << iOrder << L";";

    return serializer.str();
}