/* External/Public TUI Header File.

   Copyright (C) 1998-2020 Free Software Foundation, Inc.

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

#ifndef TUI_TUI_H
#define TUI_TUI_H

struct ui_file;

/* Types of error returns.  */
enum tui_status
{
  TUI_SUCCESS,
  TUI_FAILURE
};

/* Types of windows.  */
enum tui_win_type
{
  SRC_WIN = 0,
  DISASSEM_WIN,
  DATA_WIN,
  CMD_WIN,
  /* This must ALWAYS be AFTER the major windows last.  */
  MAX_MAJOR_WINDOWS,
};

extern CORE_ADDR tui_get_low_disassembly_address (struct gdbarch *,
						  CORE_ADDR, CORE_ADDR);
extern void tui_show_assembly (struct gdbarch *gdbarch, CORE_ADDR addr);
extern bool tui_is_window_visible (enum tui_win_type type);
extern bool tui_get_command_dimension (unsigned int *width,
				       unsigned int *height);

/* Initialize readline and configure the keymap for the switching key
   shortcut.  May be called more than once without issue.  */
extern void tui_ensure_readline_initialized ();

/* Enter in the tui mode (curses).  */
extern void tui_enable (void);

/* Leave the tui mode.  */
extern void tui_disable (void);

enum tui_key_mode
{
  /* Plain command mode to enter gdb commands.  */
  TUI_COMMAND_MODE,

  /* SingleKey mode with some keys bound to gdb commands.  */
  TUI_SINGLE_KEY_MODE,

  /* Read/edit one command and return to SingleKey after it's
     processed.  */
  TUI_ONE_COMMAND_MODE
};

extern enum tui_key_mode tui_current_key_mode;

/* Change the TUI key mode by installing the appropriate readline
   keymap.  */
extern void tui_set_key_mode (enum tui_key_mode mode);

extern bool tui_active;

#endif /* TUI_TUI_H */
