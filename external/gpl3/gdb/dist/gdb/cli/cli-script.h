/* Header file for GDB CLI command implementation library.
   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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

#if !defined (CLI_SCRIPT_H)
#define CLI_SCRIPT_H 1

struct ui_file;
struct command_line;
struct cmd_list_element;

/* Exported to cli/cli-cmds.c */

extern void script_from_file (FILE *stream, const char *file);

extern void show_user_1 (struct cmd_list_element *c,
			 const char *prefix,
			 const char *name,
			 struct ui_file *stream);

/* Exported to gdb/breakpoint.c */

extern enum command_control_type
	execute_control_command (struct command_line *cmd);

extern enum command_control_type
	execute_control_command_untraced (struct command_line *cmd);

extern command_line_up get_command_line (enum command_control_type,
					 const char *);

extern void print_command_lines (struct ui_out *,
				 struct command_line *, unsigned int);

extern command_line_up copy_command_lines (struct command_line *cmds);

/* Exported to gdb/infrun.c */

extern void execute_user_command (struct cmd_list_element *c, char *args);

/* If we're in a user-defined command, replace any $argc/$argN
   reference found in LINE with the arguments that were passed to the
   command.  Otherwise, treat $argc/$argN as normal convenience
   variables.  */
extern std::string insert_user_defined_cmd_args (const char *line);

/* Exported to top.c */

extern void print_command_trace (const char *cmd);

/* Exported to event-top.c */

extern void reset_command_nest_depth (void);

#endif /* !defined (CLI_SCRIPT_H) */
