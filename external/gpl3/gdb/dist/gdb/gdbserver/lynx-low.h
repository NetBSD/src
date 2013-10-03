/* Copyright (C) 2010-2013 Free Software Foundation, Inc.

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

struct regcache;

/*  Some information relative to a given register set.   */

struct lynx_regset_info
{
  /* The ptrace request needed to get/set registers of this set.  */
  int get_request, set_request;
  /* The size of the register set.  */
  int size;
  /* Fill the buffer BUF from the contents of the given REGCACHE.  */
  void (*fill_function) (struct regcache *regcache, char *buf);
  /* Store the register value in BUF in the given REGCACHE.  */
  void (*store_function) (struct regcache *regcache, const char *buf);
};

/* A list of regsets for the target being debugged, terminated by an entry
   where the size is negative.

   This list should be created by the target-specific code.  */

extern struct lynx_regset_info lynx_target_regsets[];

/* The target-specific operations for LynxOS support.  */

struct lynx_target_ops
{
  /* Architecture-specific setup.  */
  void (*arch_setup) (void);
};

extern struct lynx_target_ops the_low_target;

