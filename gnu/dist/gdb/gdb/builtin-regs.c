/* Builtin registers, for GDB, the GNU debugger.

   Copyright 2002 Free Software Foundation, Inc.

   Contributed by Red Hat.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "builtin-regs.h"
#include "gdbtypes.h"
#include "gdb_string.h"
#include "gdb_assert.h"

/* Implement builtin register types.  Builtin registers have regnum's
   that live above of the range [0 .. NUM_REGS + NUM_PSEUDO_REGS)
   (which is controlled by the target).  The target should never see a
   builtin register's regnum value.  */

/* An array of builtin registers.  Always append, never delete.  By
   doing this, the relative regnum (offset from NUM_REGS +
   NUM_PSEUDO_REGS) assigned to each builtin register never changes.  */

struct builtin_reg
{
  const char *name;
  struct value *(*value) (struct frame_info * frame);
};

static struct builtin_reg *builtin_regs;
int nr_builtin_regs;

void
add_builtin_reg (const char *name, struct value *(*value) (struct frame_info * frame))
{
  nr_builtin_regs++;
  builtin_regs = xrealloc (builtin_regs,
			   nr_builtin_regs * sizeof (builtin_regs[0]));
  builtin_regs[nr_builtin_regs - 1].name = name;
  builtin_regs[nr_builtin_regs - 1].value = value;
}

int
builtin_reg_map_name_to_regnum (const char *name, int len)
{
  int reg;
  for (reg = 0; reg < nr_builtin_regs; reg++)
    {
      if (len == strlen (builtin_regs[reg].name)
	  && strncmp (builtin_regs[reg].name, name, len) == 0)
	return NUM_REGS + NUM_PSEUDO_REGS + reg;
    }
  return -1;
}

const char *
builtin_reg_map_regnum_to_name (int regnum)
{
  int reg = regnum - (NUM_REGS + NUM_PSEUDO_REGS);
  if (reg < 0 || reg >= nr_builtin_regs)
    return NULL;
  return builtin_regs[reg].name;
}

struct value *
value_of_builtin_reg (int regnum, struct frame_info *frame)
{
  int reg = regnum - (NUM_REGS + NUM_PSEUDO_REGS);
  gdb_assert (reg >= 0 && reg < nr_builtin_regs);
  return builtin_regs[reg].value (frame);
}
