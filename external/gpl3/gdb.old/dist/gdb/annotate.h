/* Annotation routines for GDB.
   Copyright (C) 1986-2015 Free Software Foundation, Inc.

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

#include "symtab.h"
#include "gdbtypes.h"

extern void annotate_breakpoint (int);
extern void annotate_catchpoint (int);
extern void annotate_watchpoint (int);
extern void annotate_starting (void);
extern void annotate_stopped (void);
extern void annotate_exited (int);
extern void annotate_signalled (void);
extern void annotate_signal_name (void);
extern void annotate_signal_name_end (void);
extern void annotate_signal_string (void);
extern void annotate_signal_string_end (void);
extern void annotate_signal (void);

extern void annotate_breakpoints_headers (void);
extern void annotate_field (int);
extern void annotate_breakpoints_table (void);
extern void annotate_record (void);
extern void annotate_breakpoints_table_end (void);

extern void annotate_frames_invalid (void);
extern void annotate_new_thread (void);
extern void annotate_thread_changed (void);

extern void annotate_display_prompt (void);

struct type;

extern void annotate_field_begin (struct type *);
extern void annotate_field_name_end (void);
extern void annotate_field_value (void);
extern void annotate_field_end (void);

extern void annotate_quit (void);
extern void annotate_error (void);
extern void annotate_error_begin (void);

extern void annotate_value_history_begin (int, struct type *);
extern void annotate_value_begin (struct type *);
extern void annotate_value_history_value (void);
extern void annotate_value_history_end (void);
extern void annotate_value_end (void);

extern void annotate_display_begin (void);
extern void annotate_display_number_end (void);
extern void annotate_display_format (void);
extern void annotate_display_expression (void);
extern void annotate_display_expression_end (void);
extern void annotate_display_value (void);
extern void annotate_display_end (void);

extern void annotate_arg_begin (void);
extern void annotate_arg_name_end (void);
extern void annotate_arg_value (struct type *);
extern void annotate_arg_end (void);

extern void annotate_source (char *, int, int, int,
			     struct gdbarch *, CORE_ADDR);

extern void annotate_frame_begin (int, struct gdbarch *, CORE_ADDR);
extern void annotate_function_call (void);
extern void annotate_signal_handler_caller (void);
extern void annotate_frame_address (void);
extern void annotate_frame_address_end (void);
extern void annotate_frame_function_name (void);
extern void annotate_frame_args (void);
extern void annotate_frame_source_begin (void);
extern void annotate_frame_source_file (void);
extern void annotate_frame_source_file_end (void);
extern void annotate_frame_source_line (void);
extern void annotate_frame_source_end (void);
extern void annotate_frame_where (void);
extern void annotate_frame_end (void);

extern void annotate_array_section_begin (int, struct type *);
extern void annotate_elt_rep (unsigned int);
extern void annotate_elt_rep_end (void);
extern void annotate_elt (void);
extern void annotate_array_section_end (void);

extern void (*deprecated_annotate_signalled_hook) (void);
extern void (*deprecated_annotate_signal_hook) (void);
