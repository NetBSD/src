/* Copying of files.
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


/* Copy a regular file: from src_filename to dest_filename.
   The destination file is assumed to be a backup file.
   Modification times, owner, group and access permissions are preserved as
   far as possible.
   Exit upon failure.  */
extern void copy_file_preserving (const char *src_filename, const char *dest_filename);


#ifdef __cplusplus
}
#endif
