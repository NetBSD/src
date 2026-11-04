/* Copyright (C) 2023-2025 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/remote-args.h"
#include "gdbsupport/common-inferior.h"
#include "gdbsupport/buildargv.h"

/* See remote-args.h.  */

std::vector<std::string>
gdb::remote_args::split (const std::string &args)
{
  std::vector<std::string> results;

  gdb_argv argv (args.c_str ());
  for (int i = 0; argv[i] != nullptr; i++)
    results.emplace_back (argv[i]);

  return results;
}

/* See remote-args.h.  */

std::string
gdb::remote_args::join (const std::vector<char *> &args)
{
  return construct_inferior_arguments (args, true);
}
