/* Simulator breakpoint support.
   Copyright (C) 1997 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifndef SIM_BREAK_H
#define SIM_BREAK_H

/* Call this to install the resume and suspend handlers for the breakpoint
   module.  */

MODULE_INSTALL_FN sim_break_install;

/* Call this inside the simulator when we execute the potential
   breakpoint insn.  If the breakpoint system knows about it, the
   breakpoint is handled, and this routine never returns.  If this
   isn't really a breakpoint, then it returns to allow the caller to
   handle things.  */

void sim_handle_breakpoint PARAMS ((SIM_DESC sd, sim_cpu *cpu, sim_cia cia));

#endif /* SIM_BREAK_H */
