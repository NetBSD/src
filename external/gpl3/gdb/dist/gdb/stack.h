/* Stack manipulation commands, for GDB the GNU Debugger.

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

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

#ifndef STACK_H
#define STACK_H

void select_frame_command (char *level_exp, int from_tty);

void find_frame_funname (struct frame_info *frame, char **funname,
			 enum language *funlang, struct symbol **funcp);

typedef void (*iterate_over_block_arg_local_vars_cb) (const char *print_name,
						      struct symbol *sym,
						      void *cb_data);

void iterate_over_block_arg_vars (const struct block *block,
				  iterate_over_block_arg_local_vars_cb cb,
				  void *cb_data);

void iterate_over_block_local_vars (const struct block *block,
				    iterate_over_block_arg_local_vars_cb cb,
				    void *cb_data);

/* Get or set the last displayed symtab and line, which is, e.g. where we set a
 * breakpoint when `break' is supplied with no arguments.  */
void clear_last_displayed_sal (void);
int last_displayed_sal_is_valid (void);
struct program_space* get_last_displayed_pspace (void);
CORE_ADDR get_last_displayed_addr (void);
struct symtab* get_last_displayed_symtab (void);
int get_last_displayed_line (void);
void get_last_displayed_sal (struct symtab_and_line *sal);

#endif /* #ifndef STACK_H */
