/* Native-dependent code for BSD Unix running on ns32k's, for GDB.
   Copyright 1988, 1989, 1991, 1992 Free Software Foundation, Inc.

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
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Id: ns32k-nat.c,v 1.1 1994/06/09 14:46:19 phil Exp $
*/

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/user.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <sys/ptrace.h>

#include "defs.h"

/* Incomplete -- PAN */

void
fetch_kcore_registers (pcb)
struct pcb *pcb;
{
	return;
}

void
clear_regs()
{
	return;
}

