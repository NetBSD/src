/* Copyright (C) 2012-2014 Free Software Foundation, Inc.

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

#include "server.h"
#include "tdesc.h"
#include "regdef.h"

void
init_target_desc (struct target_desc *tdesc)
{
  int offset, i;

  offset = 0;
  for (i = 0; i < tdesc->num_registers; i++)
    {
      tdesc->reg_defs[i].offset = offset;
      offset += tdesc->reg_defs[i].size;
    }

  tdesc->registers_size = offset / 8;

  /* Make sure PBUFSIZ is large enough to hold a full register
     packet.  */
  if (2 * tdesc->registers_size + 32 > PBUFSIZ)
    fatal ("Register packet size exceeds PBUFSIZ.");
}

#ifndef IN_PROCESS_AGENT

static const struct target_desc default_description;

void
copy_target_description (struct target_desc *dest,
			 const struct target_desc *src)
{
  dest->reg_defs = src->reg_defs;
  dest->num_registers = src->num_registers;
  dest->expedite_regs = src->expedite_regs;
  dest->registers_size = src->registers_size;
  dest->xmltarget = src->xmltarget;
}

const struct target_desc *
current_target_desc (void)
{
  if (current_inferior == NULL)
    return &default_description;

  return current_process ()->tdesc;
}

#endif
