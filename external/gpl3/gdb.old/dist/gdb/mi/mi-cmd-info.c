/* MI Command Set - information commands.
   Copyright (C) 2011-2014 Free Software Foundation, Inc.

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
#include "ada-lang.h"
#include "arch-utils.h"

/* Implement the "-info-ada-exceptions" GDB/MI command.  */

void
mi_cmd_info_ada_exceptions (char *command, char **argv, int argc)
{
  struct ui_out *uiout = current_uiout;
  struct gdbarch *gdbarch = get_current_arch ();
  char *regexp;
  struct cleanup *old_chain;
  VEC(ada_exc_info) *exceptions;
  int ix;
  struct ada_exc_info *info;

  switch (argc)
    {
    case 0:
      regexp = NULL;
      break;
    case 1:
      regexp = argv[0];
      break;
    default:
      error (_("Usage: -info-ada-exceptions [REGEXP]"));
      break;
    }

  exceptions = ada_exceptions_list (regexp);
  old_chain = make_cleanup (VEC_cleanup (ada_exc_info), &exceptions);

  make_cleanup_ui_out_table_begin_end
    (uiout, 2, VEC_length (ada_exc_info, exceptions), "ada-exceptions");
  ui_out_table_header (uiout, 1, ui_left, "name", "Name");
  ui_out_table_header (uiout, 1, ui_left, "address", "Address");
  ui_out_table_body (uiout);

  for (ix = 0; VEC_iterate(ada_exc_info, exceptions, ix, info); ix++)
    {
      struct cleanup *sub_chain;

      sub_chain = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
      ui_out_field_string (uiout, "name", info->name);
      ui_out_field_core_addr (uiout, "address", gdbarch, info->addr);

      do_cleanups (sub_chain);
    }

  do_cleanups (old_chain);
}

/* Implement the "-info-gdb-mi-command" GDB/MI command.  */

void
mi_cmd_info_gdb_mi_command (char *command, char **argv, int argc)
{
  const char *cmd_name;
  struct mi_cmd *cmd;
  struct ui_out *uiout = current_uiout;
  struct cleanup *old_chain;

  /* This command takes exactly one argument.  */
  if (argc != 1)
    error (_("Usage: -info-gdb-mi-command MI_COMMAND_NAME"));
  cmd_name = argv[0];

  /* Normally, the command name (aka the "operation" in the GDB/MI
     grammar), does not include the leading '-' (dash).  But for
     the user's convenience, allow the user to specify the command
     name to be with or without that leading dash.  */
  if (cmd_name[0] == '-')
    cmd_name++;

  cmd = mi_lookup (cmd_name);

  old_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "command");
  ui_out_field_string (uiout, "exists", cmd != NULL ? "true" : "false");
  do_cleanups (old_chain);
}

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
