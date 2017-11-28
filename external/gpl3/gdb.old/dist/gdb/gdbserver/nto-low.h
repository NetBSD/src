/* Internal interfaces for the QNX Neutrino specific target code for gdbserver.
   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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

#ifndef NTO_LOW_H
#define NTO_LOW_H

struct target_desc;

enum regset_type
{
  NTO_REG_GENERAL,
  NTO_REG_FLOAT,
  NTO_REG_SYSTEM,
  NTO_REG_ALT,
  NTO_REG_END
};

struct nto_target_ops
{
  /* Architecture specific setup.  */
  void (*arch_setup) (void);
  int num_regs;
  int (*register_offset) (int gdbregno);
  const unsigned char *breakpoint;
  int breakpoint_len;
};

extern struct nto_target_ops the_low_target;

/* The inferior's target description.  This is a global because the
   LynxOS ports support neither bi-arch nor multi-process.  */
extern const struct target_desc *nto_tdesc;

#endif

