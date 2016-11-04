/* ***DEPRECATED***  The gdblib files must not be calling/using things in any
   of the possible command languages.  If necessary, a hook (that may be
   present or not) must be used and set to the appropriate routine by any
   command language that cares about it.  If you are having to include this
   file you are possibly doing things the old way.  This file will disapear.
   fnasser@redhat.com    */

/* Header file for GDB-specific command-line stuff.
   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#if !defined (GDBCMD_H)
#define GDBCMD_H 1

#include "command.h"
#include "ui-out.h"

/* Chain containing all defined commands.  */

extern struct cmd_list_element *cmdlist;

/* Chain containing all defined info subcommands.  */

extern struct cmd_list_element *infolist;

/* Chain containing all defined enable subcommands.  */

extern struct cmd_list_element *enablelist;

/* Chain containing all defined disable subcommands.  */

extern struct cmd_list_element *disablelist;

/* Chain containing all defined delete subcommands.  */

extern struct cmd_list_element *deletelist;

/* Chain containing all defined detach subcommands.  */

extern struct cmd_list_element *detachlist;

/* Chain containing all defined kill subcommands.  */

extern struct cmd_list_element *killlist;

/* Chain containing all defined stop subcommands.  */

extern struct cmd_list_element *stoplist;

/* Chain containing all defined set subcommands.  */

extern struct cmd_list_element *setlist;

/* Chain containing all defined unset subcommands.  */

extern struct cmd_list_element *unsetlist;

/* Chain containing all defined show subcommands.  */

extern struct cmd_list_element *showlist;

/* Chain containing all defined \"set history\".  */

extern struct cmd_list_element *sethistlist;

/* Chain containing all defined \"show history\".  */

extern struct cmd_list_element *showhistlist;

/* Chain containing all defined \"unset history\".  */

extern struct cmd_list_element *unsethistlist;

/* Chain containing all defined maintenance subcommands.  */

extern struct cmd_list_element *maintenancelist;

/* Chain containing all defined "maintenance info" subcommands.  */

extern struct cmd_list_element *maintenanceinfolist;

/* Chain containing all defined "maintenance print" subcommands.  */

extern struct cmd_list_element *maintenanceprintlist;

/* Chain containing all defined "maintenance set" subcommands.  */

extern struct cmd_list_element *maintenance_set_cmdlist;

/* Chain containing all defined "maintenance show" subcommands.  */

extern struct cmd_list_element *maintenance_show_cmdlist;

extern struct cmd_list_element *setprintlist;

extern struct cmd_list_element *showprintlist;

extern struct cmd_list_element *setprintrawlist;

extern struct cmd_list_element *showprintrawlist;

extern struct cmd_list_element *setprinttypelist;

extern struct cmd_list_element *showprinttypelist;

extern struct cmd_list_element *setdebuglist;

extern struct cmd_list_element *showdebuglist;

extern struct cmd_list_element *setchecklist;

extern struct cmd_list_element *showchecklist;

/* Chain containing all defined "save" subcommands.  */

extern struct cmd_list_element *save_cmdlist;

extern void execute_command (char *, int);
extern char *execute_command_to_string (char *p, int from_tty);

enum command_control_type execute_control_command (struct command_line *);

extern void print_command_line (struct command_line *, unsigned int,
				struct ui_file *);
extern void print_command_lines (struct ui_out *,
				 struct command_line *, unsigned int);

#endif /* !defined (GDBCMD_H) */
