/* TUI_FILE - a STDIO-like output stream for the TUI.
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

#ifndef TUI_FILE_H
#define TUI_FILE_H

#include "ui-file.h"

/* A STDIO-like output stream for the TUI.  */

class tui_file : public stdio_file
{
public:
  explicit tui_file (FILE *stream);

  void write (const char *buf, long length_buf) override;
  void puts (const char *) override;
  void flush () override;
};

#endif
