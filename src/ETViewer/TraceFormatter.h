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

#include "stdafx.h"
#include "TraceController.h" // TODO: remove
#include <tdh.h>

class TraceFormatter
{
public:
    TraceFormatter(const std::wstring& tmfPath);

    bool ProcessEvent(PEVENT_RECORD event, STraceEvenTracingNormalizedData& data);

private:
    template<class Type>
    unsigned long GetProperty(PEVENT_RECORD event, const std::wstring& name, Type& value);
    unsigned long GetStringProperty(PEVENT_RECORD event, const std::wstring& name, std::wstring& value);

private:
    std::wstring m_TmfPath;
    std::vector<TDH_CONTEXT> m_Contexts;
};