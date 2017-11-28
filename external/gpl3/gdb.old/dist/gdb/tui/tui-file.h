/* UI_FILE - a generic STDIO like output stream.
   Copyright (C) 1999-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef TUI_FILE_H
#define TUI_FILE_H

extern struct ui_file *tui_fileopen (FILE *);
extern struct ui_file *tui_sfileopen (int);
extern char *tui_file_get_strbuf (struct ui_file *);
extern void tui_file_adjust_strbuf (int, struct ui_file *);

#endif
