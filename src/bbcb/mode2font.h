/*
    mode2font.h: BBC 6502 Mode 2 font description
    Copyright (C) 2007  Mark Lomas

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef BBC_MODE2FONT_H_INCLUDED
#define BBC_MODE2FONT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char Font[];

extern Font font;

#ifdef __cplusplus
}
#endif

#endif // BBC_MODE2FONT_H_INCLUDED
