/* Process record and replay target for GDB, the GNU debugger.

   Copyright (C) 2008-2014 Free Software Foundation, Inc.

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

#ifndef _RECORD_H_
#define _RECORD_H_

struct cmd_list_element;

#define RECORD_IS_USED	(current_target.to_stratum == record_stratum)

extern unsigned int record_debug;

/* Allow record targets to add their own sub-commands.  */
extern struct cmd_list_element *record_cmdlist;
extern struct cmd_list_element *set_record_cmdlist;
extern struct cmd_list_element *show_record_cmdlist;
extern struct cmd_list_element *info_record_cmdlist;

/* A list of flags specifying what record target methods should print.  */
enum record_print_flag
{
  /* Print the source file and line (if applicable).  */
  RECORD_PRINT_SRC_LINE = (1 << 0),

  /* Print the instruction number range (if applicable).  */
  RECORD_PRINT_INSN_RANGE = (1 << 1),
};

/* Wrapper for target_read_memory that prints a debug message if
   reading memory fails.  */
extern int record_read_memory (struct gdbarch *gdbarch,
			       CORE_ADDR memaddr, gdb_byte *myaddr,
			       ssize_t len);

/* The "record goto" command.  */
extern void cmd_record_goto (char *arg, int from_tty);

/* The default "to_disconnect" target method for record targets.  */
extern void record_disconnect (struct target_ops *, char *, int);

/* The default "to_detach" target method for record targets.  */
extern void record_detach (struct target_ops *, const char *, int);

/* The default "to_mourn_inferior" target method for record targets.  */
extern void record_mourn_inferior (struct target_ops *);

/* The default "to_kill" target method for record targets.  */
extern void record_kill (struct target_ops *);

#endif /* _RECORD_H_ */
