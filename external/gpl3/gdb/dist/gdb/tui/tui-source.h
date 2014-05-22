/* TUI display source window.

   Copyright (C) 1998-2013 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

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

#ifndef TUI_SOURCE_H
#define TUI_SOURCE_H

#include "tui/tui-data.h"

struct symtab;
struct tui_win_info;

extern void tui_set_source_content_nil (struct tui_win_info *, 
					char *);

extern enum tui_status tui_set_source_content (struct symtab *, 
					       int, int);
extern void tui_show_symtab_source (struct gdbarch *, struct symtab *,
				    struct tui_line_or_address,
				    int);
extern int tui_source_is_displayed (const char *);
extern void tui_vertical_source_scroll (enum tui_scroll_direction,
					int);

#endif
