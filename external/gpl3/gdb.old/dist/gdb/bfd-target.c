/* Very simple "bfd" target, for GDB, the GNU debugger.

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

#include "defs.h"
#include "target.h"
#include "bfd-target.h"
#include "exec.h"
#include "gdb_bfd.h"

/* The object that is stored in the target_ops->to_data field has this
   type.  */
struct target_bfd_data
{
  /* The BFD we're wrapping.  */
  struct bfd *bfd;

  /* The section table build from the ALLOC sections in BFD.  Note
     that we can't rely on extracting the BFD from a random section in
     the table, since the table can be legitimately empty.  */
  struct target_section_table table;
};

static enum target_xfer_status
target_bfd_xfer_partial (struct target_ops *ops,
			 enum target_object object,
			 const char *annex, gdb_byte *readbuf,
			 const gdb_byte *writebuf,
			 ULONGEST offset, ULONGEST len,
			 ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      {
	struct target_bfd_data *data = (struct target_bfd_data *) ops->to_data;
	return section_table_xfer_memory_partial (readbuf, writebuf,
						  offset, len, xfered_len,
						  data->table.sections,
						  data->table.sections_end,
						  NULL);
      }
    default:
      return TARGET_XFER_E_IO;
    }
}

static struct target_section_table *
target_bfd_get_section_table (struct target_ops *ops)
{
  struct target_bfd_data *data = (struct target_bfd_data *) ops->to_data;
  return &data->table;
}

static void
target_bfd_xclose (struct target_ops *t)
{
  struct target_bfd_data *data = (struct target_bfd_data *) t->to_data;

  gdb_bfd_unref (data->bfd);
  xfree (data->table.sections);
  xfree (data);
  xfree (t);
}

struct target_ops *
target_bfd_reopen (struct bfd *abfd)
{
  struct target_ops *t;
  struct target_bfd_data *data;

  data = XCNEW (struct target_bfd_data);
  data->bfd = abfd;
  gdb_bfd_ref (abfd);
  build_section_table (abfd, &data->table.sections, &data->table.sections_end);

  t = XCNEW (struct target_ops);
  t->to_shortname = "bfd";
  t->to_longname = _("BFD backed target");
  t->to_doc = _("You should never see this");
  t->to_get_section_table = target_bfd_get_section_table;
  t->to_xfer_partial = target_bfd_xfer_partial;
  t->to_xclose = target_bfd_xclose;
  t->to_data = data;
  t->to_magic = OPS_MAGIC;

  return t;
}
