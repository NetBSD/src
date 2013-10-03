/* MI Command Set - information commands.
   Copyright (C) 2011-2013 Free Software Foundation, Inc.

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
#include "osdata.h"
#include "mi-cmds.h"

void
mi_cmd_info_os (char *command, char **argv, int argc)
{
  switch (argc)
    {
    case 0:
      info_osdata_command ("", 0);
      break;
    case 1:
      info_osdata_command (argv[0], 0);
      break;
    default:
      error (_("Usage: -info-os [INFOTYPE]"));
      break;
    }
}
