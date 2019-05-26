/* UI_FILE - a generic STDIO like output stream.
   Copyright (C) 1999-2017 Free Software Foundation, Inc.

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

#include "defs.h"
#include "tui/tui-file.h"
#include "tui/tui-io.h"
#include "tui/tui-command.h"
#include "tui.h"

tui_file::tui_file (FILE *stream)
  : stdio_file (stream)
{}

/* All TUI I/O sent to the *_filtered and *_unfiltered functions
   eventually ends up here.  The fputs_unfiltered_hook is primarily
   used by GUIs to collect all output and send it to the GUI, instead
   of the controlling terminal.  Only output to gdb_stdout and
   gdb_stderr are sent to the hook.  Everything else is sent on to
   fputs to allow file I/O to be handled appropriately.  */

void
tui_file::puts (const char *linebuffer)
{
  tui_puts (linebuffer);
  /* gdb_stdout is buffered, and the caller must gdb_flush it at
     appropriate times.  Other streams are not so buffered.  */
  if (this != gdb_stdout)
    tui_refresh_cmd_win ();
}

void
tui_file::write (const char *buf, long length_buf)
{
  tui_write (buf, length_buf);
  /* gdb_stdout is buffered, and the caller must gdb_flush it at
     appropriate times.  Other streams are not so buffered.  */
  if (this != gdb_stdout)
    tui_refresh_cmd_win ();
}

void
tui_file::flush ()
{
  /* gdb_stdout is buffered.  Other files are always flushed on
     every write.  */
  if (this == gdb_stdout)
    tui_refresh_cmd_win ();
  stdio_file::flush ();
}
