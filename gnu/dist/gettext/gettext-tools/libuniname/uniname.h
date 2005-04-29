/* Association between Unicode characters and their names.
   Copyright (C) 2000-2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _UNINAME_H
#define _UNINAME_H

/* Required size of buffer for a Unicode character name.  */
#define UNINAME_MAX 256

/* Looks up the name of a Unicode character, in uppercase ASCII.
   Returns the filled buf, or NULL if the character does not have a name.  */
extern char * unicode_character_name (unsigned int uc, char *buf);

/* Looks up the Unicode character with a given name, in upper- or lowercase
   ASCII.  Returns the character if found, or UNINAME_INVALID if not found.  */
extern unsigned int unicode_name_character (const char *name);
#define UNINAME_INVALID ((unsigned int) 0xFFFF)

#endif /* _UNINAME_H */
