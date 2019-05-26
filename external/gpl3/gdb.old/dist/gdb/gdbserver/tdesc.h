/* Target description definitions for remote server for GDB.
   Copyright (C) 2012-2017 Free Software Foundation, Inc.

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

#ifndef TDESC_H
#define TDESC_H

struct reg;

/* A target description.  */

struct target_desc
{
  /* An array of NUM_REGISTERS elements of register definitions that
     describe the inferior's register set.  */
  struct reg *reg_defs;

  /* The number of registers in inferior's register set (and thus in
     the regcache).  */
  int num_registers;

  /* The register cache size, in bytes.  */
  int registers_size;

  /* An array of register names.  These are the "expedite" registers:
     registers whose values are sent along with stop replies.  */
  const char **expedite_regs;

  /* Defines what to return when looking for the "target.xml" file in
     response to qXfer:features:read.  Its contents can either be
     verbatim XML code (prefixed with a '@') or else the name of the
     actual XML file to be used in place of "target.xml".  */
  const char *xmltarget;
};

/* Copy target description SRC to DEST.  */

void copy_target_description (struct target_desc *dest,
			      const struct target_desc *src);

/* Initialize TDESC.  */

void init_target_desc (struct target_desc *tdesc);

/* Return the current inferior's target description.  Never returns
   NULL.  */

const struct target_desc *current_target_desc (void);

#endif /* TDESC_H */
