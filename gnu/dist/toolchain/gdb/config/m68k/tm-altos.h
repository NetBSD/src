/* OBSOLETE /* Target definitions for GDB on an Altos 3068 (m68k running SVR2) */
/* OBSOLETE    Copyright 1987, 1989, 1991, 1993 Free Software Foundation, Inc. */
/* OBSOLETE  */
/* OBSOLETE This file is part of GDB. */
/* OBSOLETE  */
/* OBSOLETE This program is free software; you can redistribute it and/or modify */
/* OBSOLETE it under the terms of the GNU General Public License as published by */
/* OBSOLETE the Free Software Foundation; either version 2 of the License, or */
/* OBSOLETE (at your option) any later version. */
/* OBSOLETE  */
/* OBSOLETE This program is distributed in the hope that it will be useful, */
/* OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE You should have received a copy of the GNU General Public License */
/* OBSOLETE along with this program; if not, write to the Free Software */
/* OBSOLETE Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* The child target can't deal with floating registers.  *x/ */
/* OBSOLETE #define CANNOT_STORE_REGISTER(regno) ((regno) >= FP0_REGNUM) */
/* OBSOLETE  */
/* OBSOLETE /* Define BPT_VECTOR if it is different than the default. */
/* OBSOLETE    This is the vector number used by traps to indicate a breakpoint. *x/ */
/* OBSOLETE  */
/* OBSOLETE #define BPT_VECTOR 0xe */
/* OBSOLETE  */
/* OBSOLETE /* Address of end of stack space.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /*#define STACK_END_ADDR (0xffffff)*x/ */
/* OBSOLETE #define STACK_END_ADDR (0x1000000) */
/* OBSOLETE  */
/* OBSOLETE /* Amount PC must be decremented by after a breakpoint. */
/* OBSOLETE    On the Altos, the kernel resets the pc to the trap instr *x/ */
/* OBSOLETE  */
/* OBSOLETE #define DECR_PC_AFTER_BREAK 0 */
/* OBSOLETE  */
/* OBSOLETE /* The only reason this is here is the tm-altos.h reference below.  It */
/* OBSOLETE    was moved back here from tm-m68k.h.  FIXME? *x/ */
/* OBSOLETE  */
/* OBSOLETE extern CORE_ADDR altos_skip_prologue PARAMS ((CORE_ADDR)); */
/* OBSOLETE #define SKIP_PROLOGUE(pc) (altos_skip_prologue (pc)) */
/* OBSOLETE  */
/* OBSOLETE #include "m68k/tm-m68k.h" */
