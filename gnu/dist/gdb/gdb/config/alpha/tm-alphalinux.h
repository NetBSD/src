/* Definitions to make GDB run on an Alpha box under GNU/Linux.  The
   definitions here are used when the _target_ system is running
   GNU/Linux.

   Copyright 1996, 1998, 1999, 2000, 2002 Free Software Foundation,
   Inc.

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

#ifndef TM_LINUXALPHA_H
#define TM_LINUXALPHA_H

#include "alpha/tm-alpha.h"

/* Get start and end address of sigtramp handler.  */

extern LONGEST alpha_linux_sigtramp_offset (CORE_ADDR);
#define SIGTRAMP_START(pc)	(pc - alpha_linux_sigtramp_offset (pc))
#define SIGTRAMP_END(pc)	(SIGTRAMP_START(pc) + 3*4)


/* Number of traps that happen between exec'ing the shell to run an
   inferior, and when we finally get to the inferior code.  This is 2
   on GNU/Linux and most implementations.  */
#undef START_INFERIOR_TRAPS_EXPECTED
#define START_INFERIOR_TRAPS_EXPECTED 2

#include "config/tm-linux.h"

#endif /* TM_LINUXALPHA_H */
