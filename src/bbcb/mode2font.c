/*
    mode2font.c: BBC 6502 Mode 2 font description
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

#include "mode2font.h"

#ifdef __cplusplus
extern "C" {
#endif

Font font = 
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00,
    0x36,0x36,0x36,0x00,0x00,0x00,0x00,0x00,
    0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00,
    0x30,0xfc,0x16,0x7c,0xd0,0x7e,0x18,0x00,
    0x06,0x66,0x30,0x18,0x0c,0x66,0x60,0x00,
    0x1c,0x36,0x36,0x1c,0xb6,0x66,0xdc,0x00,
    0x30,0x18,0x0c,0x00,0x00,0x00,0x00,0x00,
    0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00,
    0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00,
    0x00,0x18,0x7e,0x3c,0x7e,0x18,0x00,0x00,
    0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x0c,
    0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,
    0x00,0x60,0x30,0x18,0x0c,0x06,0x00,0x00,
    0x3c,0x66,0x76,0x7e,0x6e,0x66,0x3c,0x00,
    0x18,0x1c,0x18,0x18,0x18,0x18,0x7e,0x00,
    0x3c,0x66,0x60,0x30,0x18,0x0c,0x7e,0x00,
    0x3c,0x66,0x60,0x38,0x60,0x66,0x3c,0x00,
    0x30,0x38,0x3c,0x36,0x7e,0x30,0x30,0x00,
    0x7e,0x06,0x3e,0x60,0x60,0x66,0x3c,0x00,
    0x38,0x0c,0x06,0x3e,0x66,0x66,0x3c,0x00,
    0x7e,0x60,0x30,0x18,0x0c,0x0c,0x0c,0x00,
    0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00,
    0x3c,0x66,0x66,0x7c,0x60,0x30,0x1c,0x00,
    0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x00,
    0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x0c,
    0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x00,
    0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00,
    0x0c,0x18,0x30,0x60,0x30,0x18,0x0c,0x00,
    0x3c,0x66,0x30,0x18,0x18,0x00,0x18,0x00,
    0x3c,0x66,0x76,0x56,0x76,0x06,0x3c,0x00,
    0x3c,0x66,0x66,0x7e,0x66,0x66,0x66,0x00,
    0x3e,0x66,0x66,0x3e,0x66,0x66,0x3e,0x00,
    0x3c,0x66,0x06,0x06,0x06,0x66,0x3c,0x00,
    0x1e,0x36,0x66,0x66,0x66,0x36,0x1e,0x00,
    0x7e,0x06,0x06,0x3e,0x06,0x06,0x7e,0x00,
    0x7e,0x06,0x06,0x3e,0x06,0x06,0x06,0x00,
    0x3c,0x66,0x06,0x76,0x66,0x66,0x3c,0x00,
    0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00,
    0x7e,0x18,0x18,0x18,0x18,0x18,0x7e,0x00,
    0x7c,0x30,0x30,0x30,0x30,0x36,0x1c,0x00,
    0x66,0x36,0x1e,0x0e,0x1e,0x36,0x66,0x00,
    0x06,0x06,0x06,0x06,0x06,0x06,0x7e,0x00,
    0xc6,0xee,0xfe,0xd6,0xd6,0xc6,0xc6,0x00,
    0x66,0x66,0x6e,0x7e,0x76,0x66,0x66,0x00,
    0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00,
    0x3e,0x66,0x66,0x3e,0x06,0x06,0x06,0x00,
    0x3c,0x66,0x66,0x66,0x56,0x36,0x6c,0x00,
    0x3e,0x66,0x66,0x3e,0x36,0x66,0x66,0x00,
    0x3c,0x66,0x06,0x3c,0x60,0x66,0x3c,0x00,
    0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x00,
    0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00,
    0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00,
    0xc6,0xc6,0xd6,0xd6,0xfe,0xee,0xc6,0x00,
    0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00,
    0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00,
    0x7e,0x60,0x30,0x18,0x0c,0x06,0x7e,0x00,
    0x3e,0x06,0x06,0x06,0x06,0x06,0x3e,0x00,
    0x00,0x06,0x0c,0x18,0x30,0x60,0x00,0x00,
    0x7c,0x60,0x60,0x60,0x60,0x60,0x7c,0x00,
    0x18,0x3c,0x66,0x42,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,
    0x38,0x6c,0x0c,0x3e,0x0c,0x0c,0x7e,0x00,
    0x00,0x00,0x3c,0x60,0x7c,0x66,0x7c,0x00,
    0x06,0x06,0x3e,0x66,0x66,0x66,0x3e,0x00,
    0x00,0x00,0x3c,0x66,0x06,0x66,0x3c,0x00,
    0x60,0x60,0x7c,0x66,0x66,0x66,0x7c,0x00,
    0x00,0x00,0x3c,0x66,0x7e,0x06,0x3c,0x00,
    0x38,0x0c,0x0c,0x3e,0x0c,0x0c,0x0c,0x00,
    0x00,0x00,0x7c,0x66,0x66,0x7c,0x60,0x3c,
    0x06,0x06,0x3e,0x66,0x66,0x66,0x66,0x00,
    0x18,0x00,0x1c,0x18,0x18,0x18,0x3c,0x00,
    0x18,0x00,0x1c,0x18,0x18,0x18,0x18,0x0e,
    0x06,0x06,0x66,0x36,0x1e,0x36,0x66,0x00,
    0x1c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00,
    0x00,0x00,0x6c,0xfe,0xd6,0xd6,0xc6,0x00,
    0x00,0x00,0x3e,0x66,0x66,0x66,0x66,0x00,
    0x00,0x00,0x3c,0x66,0x66,0x66,0x3c,0x00,
    0x00,0x00,0x3e,0x66,0x66,0x3e,0x06,0x06,
    0x00,0x00,0x7c,0x66,0x66,0x7c,0x60,0xe0,
    0x00,0x00,0x36,0x6e,0x06,0x06,0x06,0x00,
    0x00,0x00,0x7c,0x06,0x3c,0x60,0x3e,0x00,
    0x0c,0x0c,0x3e,0x0c,0x0c,0x0c,0x38,0x00,
    0x00,0x00,0x66,0x66,0x66,0x66,0x7c,0x00,
    0x00,0x00,0x66,0x66,0x66,0x3c,0x18,0x00,
    0x00,0x00,0xc6,0xd6,0xd6,0xfe,0x6c,0x00,
    0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00,
    0x00,0x00,0x66,0x66,0x66,0x7c,0x60,0x3c,
    0x00,0x00,0x7e,0x30,0x18,0x0c,0x7e,0x00,
    0x30,0x18,0x18,0x0e,0x18,0x18,0x30,0x00,
    0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,
    0x0c,0x18,0x18,0x70,0x18,0x18,0x0c,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

#ifdef __cplusplus
}
#endif