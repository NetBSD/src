/* General window behavior.

   Copyright (C) 1998-2025 Free Software Foundation, Inc.

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

#ifndef GDB_TUI_TUI_WINGENERAL_H
#define GDB_TUI_TUI_WINGENERAL_H

#include "gdb_curses.h"

struct tui_win_info;

extern void tui_unhighlight_win (struct tui_win_info *);
extern void tui_highlight_win (struct tui_win_info *);

/* An RAII class that calls doupdate on destruction (really the
   destruction of the outermost instance).  This is used to prevent
   flickering -- window implementations should only call wnoutrefresh,
   and any time rendering is needed, an object of this type should be
   instantiated.  */

class tui_batch_rendering
{
public:

  tui_batch_rendering ();
  ~tui_batch_rendering ();

  DISABLE_COPY_AND_ASSIGN (tui_batch_rendering);

private:

  /* Save the state of the suppression global.  */
  bool m_saved_suppress;
};

#endif /* GDB_TUI_TUI_WINGENERAL_H */
