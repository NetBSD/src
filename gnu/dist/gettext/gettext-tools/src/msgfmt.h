/* msgfmt specifics
   Copyright (C) 1995-1998, 2000, 2001 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, April 1995.

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

#ifndef _MSGFMT_H
#define _MSGFMT_H

/* Be more verbose.  Use only 'fprintf' and 'multiline_warning' but not
   'error' or 'multiline_error' to emit verbosity messages, because 'error'
   and 'multiline_error' during PO file parsing cause the program to exit
   with EXIT_FAILURE.  See function lex_end().  */
extern bool verbose;

#endif /* _MSGFMT_H */
