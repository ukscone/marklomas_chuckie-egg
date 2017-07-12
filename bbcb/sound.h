/*
    sound.h: Approximation of BBC model B SOUND and ENVELOPE commands
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
#ifndef BBCB_SOUND_H_INCLUDED
#define BBCB_SOUND_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

int AudioInit(void);
void AudioQuit(void);
void sound(unsigned char *array);
void envelope(unsigned char *array);
void basicsound(unsigned int chan, char amplitude, unsigned char pitch, unsigned char duration);
void basicenvelope(unsigned char N,
                   unsigned char T,
                   char PI1,
                   char PI2,
                   char PI3,
                   unsigned char PN1,
                   unsigned char PN2,
                   unsigned char PN3,
                   char AA,
                   char AD,
                   char AS,
                   char AR,
                   unsigned char ALA,
                   unsigned char ALD);

#ifdef __cplusplus
}
#endif

#endif /* BBCB_SOUND_H_INCLUDED */
