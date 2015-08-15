/* Output generating routines for GDB CLI.
   Copyright (C) 1999-2014 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

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

#ifndef CLI_OUT_H
#define CLI_OUT_H

#include "ui-out.h"
#include "vec.h"

/* Used for cli_ui_out_data->streams.  */

typedef struct ui_file *ui_filep;
DEF_VEC_P (ui_filep);

/* These are exported so that they can be extended by other `ui_out'
   implementations, like TUI's.  */

struct cli_ui_out_data
  {
    VEC (ui_filep) *streams;
    int suppress_output;
  };

extern struct ui_out_impl cli_ui_out_impl;


extern struct ui_out *cli_out_new (struct ui_file *stream);

extern void cli_out_data_ctor (struct cli_ui_out_data *data,
			       struct ui_file *stream);

extern struct ui_file *cli_out_set_stream (struct ui_out *uiout,
					   struct ui_file *stream);

#endif
