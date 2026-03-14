/* TUI_FILE - a STDIO-like output stream for the TUI.
   Copyright (C) 1999-2025 Free Software Foundation, Inc.

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

#ifndef GDB_TUI_TUI_FILE_H
#define GDB_TUI_TUI_FILE_H

#include "ui-file.h"

/* A STDIO-like output stream for the TUI.  */

class tui_file : public escape_buffering_file
{
public:
  tui_file (FILE *stream, bool buffered)
    : escape_buffering_file (stream),
      m_buffered (buffered)
  {}

  void flush () override;

protected:

  void do_write (const char *buf, long length_buf) override;
  void do_puts (const char *) override;

private:

  /* True if this stream is buffered.  */
  bool m_buffered;
};

#endif /* GDB_TUI_TUI_FILE_H */
