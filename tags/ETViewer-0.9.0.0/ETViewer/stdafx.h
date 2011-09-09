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

// stdafx.h: archivo de inclusión para archivos de inclusión estándar del sistema,
// o archivos de inclusión específicos del proyecto utilizados frecuentemente,
// pero cambiados rara vez

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Excluir material rara vez utilizado de encabezados de Windows
#endif

// Modificar las siguientes secciones define si su objetivo es una plataforma distinta a las especificadas a continuación.
// Consulte la referencia MSDN para obtener la información más reciente sobre los valores correspondientes a diferentes plataformas.
#ifndef WINVER				// Permitir el uso de características específicas de Windows 95 y Windows NT 4 o posterior.
#define WINVER 0x0501		// Cambiar para establecer el valor apropiado para Windows 98 y Windows 2000 o posterior.
#endif

#ifndef _WIN32_WINNT		// Permitir el uso de características específicas de Windows NT 4 o posterior.
#define _WIN32_WINNT 0x0500		// Cambiar para establecer el valor apropiado para Windows 98 y Windows 2000 o posterior.
#endif						

#ifndef _WIN32_WINDOWS		// Permitir el uso de características específicas de Windows 98 o posterior.
#define _WIN32_WINDOWS 0x0510 // Cambiar para establecer el valor apropiado para Windows Me o posterior.
#endif

#ifndef _WIN32_IE			// Permitir el uso de características específicas de Internet Explorer 4.0 o posterior.
#define _WIN32_IE 0x0400	// Cambiar para establecer el valor apropiado para IE 5.0 o posterior.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// Algunos constructores CString serán explícitos

// Desactiva la ocultación de MFC para algunos mensajes de advertencia comunes y, muchas veces, omitidos de forma consciente
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // Componentes principales y estándar de MFC
#include <afxext.h>         // Extensiones de MFC
#include <afxcview.h>

#include <afxdtctl.h>		// Compatibilidad MFC para controles comunes de Internet Explorer 4
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// Compatibilidad MFC para controles comunes de Windows
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <deque>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <afxdhtml.h>

#include "persistency\persistency.h"
#include "persistency\persistentTypes.h"
#include "persistency\configfile.h"

using namespace std;

SIZE  GetRectSize(RECT &rect);
POINT GetRectPos(RECT &rect);
