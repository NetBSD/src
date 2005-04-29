/* Locating a program in PATH.
   Copyright (C) 2001-2003 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

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


#ifdef __cplusplus
extern "C" {
#endif


/* Look up a program in the PATH.
   Attempt to determine the pathname that would be called by execlp/execvp
   of PROGNAME.  If successful, return a pathname containing a slash
   (either absolute or relative to the current directory).  Otherwise,
   return PROGNAME unmodified.
   Because of the latter case, callers should use execlp/execvp, not
   execl/execv on the returned pathname.
   The returned string is freshly malloc()ed if it is != PROGNAME.  */
extern const char *find_in_path (const char *progname);


#ifdef __cplusplus
}
#endif
