/***************************************************************************
*   Copyright (c) Juergen Riegel         (juergen.riegel@web.de) 2014     *
*                                                                         *
*   This file is part of the FreeCAD CAx development system.              *
*                                                                         *
*   This library is free software; you can redistribute it and/or         *
*   modify it under the terms of the GNU Library General Public           *
*   License as published by the Free Software Foundation; either          *
*   version 2 of the License, or (at your option) any later version.      *
*                                                                         *
*   This library  is distributed in the hope that it will be useful,      *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU Library General Public License for more details.                  *
*                                                                         *
*   You should have received a copy of the GNU Library General Public     *
*   License along with this library; see the file COPYING.LIB. If not,    *
*   write to the Free Software Foundation, Inc., 59 Temple Place,         *
*   Suite 330, Boston, MA  02111-1307, USA                                *
*                                                                         *
***************************************************************************/

#ifndef GUID_HEADER
#define GUID_HEADER

#include <istream>
#include <stdint.h>

#include "U8.h"
#include "U16.h"
#include "U32.h"



using namespace std;

#undef _C2 

struct GUID
{
	GUID(){};

	GUID(Context& cont)
	{
		read(cont);
	}

	inline void read(Context& cont)
	{
		_A1.read(cont);

		_B1.read(cont);
		_B2.read(cont);

		_C1.read(cont);
		_C2.read(cont);
		_C3.read(cont);
		_C4.read(cont);
		_C5.read(cont);
		_C6.read(cont);
		_C7.read(cont);
		_C8.read(cont);
	}

	U32 _A1;

	U16 _B1;
	U16 _B2;

	U8  _C1;
	U8  _C2;
	U8  _C3;
	U8  _C4;
	U8  _C5;
	U8  _C6;
	U8  _C7;
	U8  _C8;

};



#endif