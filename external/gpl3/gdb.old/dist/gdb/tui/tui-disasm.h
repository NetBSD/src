/* Disassembly display.

   Copyright (C) 1998-2015 Free Software Foundation, Inc.
   
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

#ifndef TUI_DISASM_H
#define TUI_DISASM_H

#include "tui/tui.h"		/* For enum tui_status.  */
#include "tui/tui-data.h"	/* For enum tui_scroll_direction.  */

extern enum tui_status tui_set_disassem_content (struct gdbarch *, CORE_ADDR);
extern void tui_show_disassem (struct gdbarch *, CORE_ADDR);
extern void tui_show_disassem_and_update_source (struct gdbarch *, CORE_ADDR);
extern void tui_vertical_disassem_scroll (enum tui_scroll_direction, 
					  int);
extern void tui_get_begin_asm_address (struct gdbarch **, CORE_ADDR *);

#endif
