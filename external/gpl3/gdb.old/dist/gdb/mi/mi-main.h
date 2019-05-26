/* MI Internal Functions for GDB, the GNU debugger.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

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

#ifndef MI_MAIN_H
#define MI_MAIN_H

struct ui_file;

extern void mi_load_progress (const char *section_name,
			      unsigned long sent_so_far,
			      unsigned long total_section,
			      unsigned long total_sent,
			      unsigned long grand_total);

extern void mi_print_timing_maybe (struct ui_file *file);

/* Whether MI is in async mode.  */

extern int mi_async_p (void);

extern char *current_token;

extern int running_result_record_printed;
extern int mi_proceeded;

struct mi_suppress_notification
{
  /* Breakpoint notification suppressed?  */
  int breakpoint;
  /* Command param changed notification suppressed?  */
  int cmd_param_changed;
  /* Traceframe changed notification suppressed?  */
  int traceframe;
  /* Memory changed notification suppressed?  */
  int memory;
  /* User selected context changed notification suppressed?  */
  int user_selected_context;
};
extern struct mi_suppress_notification mi_suppress_notification;

#endif

