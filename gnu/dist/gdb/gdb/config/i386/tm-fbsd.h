/* Target-dependent definitions for FreeBSD/i386.
   Copyright 1997, 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef TM_FBSD_H
#define TM_FBSD_H

#include "i386/tm-i386.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/* These defines allow the recognition of sigtramps as a function name
   <sigtramp>.

   FIXME: kettenis/2002-05-12: Of course these defines will have to go
   if we go truly "multi-arch", but I don't know yet how to get rid of
   them.  */

#define SIGTRAMP_START(pc) i386bsd_sigtramp_start (pc)
#define SIGTRAMP_END(pc) i386bsd_sigtramp_end (pc)
extern CORE_ADDR i386bsd_sigtramp_start (CORE_ADDR pc);
extern CORE_ADDR i386bsd_sigtramp_end (CORE_ADDR pc);

#endif /* TM_FBSD_H */
