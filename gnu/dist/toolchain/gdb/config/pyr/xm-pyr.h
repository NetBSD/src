/* OBSOLETE /* Definitions to make GDB run on a Pyramidax under OSx 4.0 (4.2bsd). */
/* OBSOLETE    Copyright 1988, 1989, 1992  Free Software Foundation, Inc. */
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
/* OBSOLETE #define HOST_BYTE_ORDER BIG_ENDIAN */
/* OBSOLETE  */
/* OBSOLETE /* Define PYRAMID_CONTROL_FRAME_DEBUGGING to get copious messages */
/* OBSOLETE    about reading the control stack on standard output. This */
/* OBSOLETE    makes gdb unusable as a debugger. *x/ */
/* OBSOLETE  */
/* OBSOLETE /* #define PYRAMID_CONTROL_FRAME_DEBUGGING *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Define PYRAMID_FRAME_DEBUGGING for ? *x/ */
/* OBSOLETE  */
/* OBSOLETE /* use Pyramid's slightly strange ptrace *x/ */
/* OBSOLETE #define PYRAMID_PTRACE */
/* OBSOLETE  */
/* OBSOLETE /* Traditional Unix virtual address spaces have thre regions: text, */
/* OBSOLETE    data and stack.  The text, initialised data, and uninitialised data */
/* OBSOLETE    are represented in separate segments of the a.out file. */
/* OBSOLETE    When a process dumps core, the data and stack regions are written */
/* OBSOLETE    to a core file.  This gives a debugger enough information to */
/* OBSOLETE    reconstruct (and debug) the virtual address space at the time of */
/* OBSOLETE    the coredump. */
/* OBSOLETE    Pyramids have an distinct fourth region of the virtual address */
/* OBSOLETE    space, in which the contents of the windowed registers are stacked */
/* OBSOLETE    in fixed-size frames.  Pyramid refer to this region as the control */
/* OBSOLETE    stack.  Each call (or trap) automatically allocates a new register */
/* OBSOLETE    frame; each return deallocates the current frame and restores the */
/* OBSOLETE    windowed registers to their values before the call. */
/* OBSOLETE  */
/* OBSOLETE    When dumping core, the control stack is written to a core files as */
/* OBSOLETE    a third segment. The core-handling functions need to know to deal */
/* OBSOLETE    with it. *x/  */
/* OBSOLETE  */
/* OBSOLETE /* Tell dep.c what the extra segment is.  *x/ */
/* OBSOLETE #define PYRAMID_CORE */
/* OBSOLETE  */
/* OBSOLETE #define NO_SIGINTERRUPT */
/* OBSOLETE  */
/* OBSOLETE #define HAVE_WAIT_STRUCT */
/* OBSOLETE  */
/* OBSOLETE /* This is the amount to subtract from u.u_ar0 */
/* OBSOLETE    to get the offset in the core file of the register values.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define KERNEL_U_ADDR (0x80000000 - (UPAGES * NBPG)) */
/* OBSOLETE  */
/* OBSOLETE /* Define offsets of registers in the core file (or maybe u area) *x/ */
/* OBSOLETE #define REGISTER_U_ADDR(addr, blockend, regno)      \ */
/* OBSOLETE { struct user __u;                                  \ */
/* OBSOLETE   addr = blockend  + (regno - 16 ) * 4;                     \ */
/* OBSOLETE   if (regno == 67) {                                        \ */
/* OBSOLETE       printf("\\geting reg 67\\");                  \ */
/* OBSOLETE       addr = (int)(&__u.u_pcb.pcb_csp) - (int) &__u;        \ */
/* OBSOLETE   } else if (regno == KSP_REGNUM) {                 \ */
/* OBSOLETE       printf("\\geting KSP (reg %d)\\", KSP_REGNUM);        \ */
/* OBSOLETE       addr = (int)(&__u.u_pcb.pcb_ksp) - (int) &__u;        \ */
/* OBSOLETE   } else if (regno == CSP_REGNUM) {                 \ */
/* OBSOLETE       printf("\\geting CSP (reg %d\\",CSP_REGNUM);  \ */
/* OBSOLETE       addr = (int)(&__u.u_pcb.pcb_csp) - (int) &__u;        \ */
/* OBSOLETE   } else if (regno == 64) {                         \ */
/* OBSOLETE       printf("\\geting reg 64\\");                  \ */
/* OBSOLETE       addr = (int)(&__u.u_pcb.pcb_csp) - (int) &__u;        \ */
/* OBSOLETE    } else if (regno == PS_REGNUM)                   \ */
/* OBSOLETE       addr = blockend - 4;                          \ */
/* OBSOLETE   else if (1 && ((16 > regno) && (regno > 11)))             \ */
/* OBSOLETE       addr = last_frame_offset + (4 *(regno+32));   \ */
/* OBSOLETE   else if (0 && (12 > regno))                               \ */
/* OBSOLETE       addr = global_reg_offset + (4 *regno);                \ */
/* OBSOLETE   else if (16 > regno)                                      \ */
/* OBSOLETE       addr = global_reg_offset + (4 *regno);                \ */
/* OBSOLETE  else                                                       \ */
/* OBSOLETE       addr = blockend  + (regno - 16 ) * 4;         \ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Override copies of {fetch,store}_inferior_registers in infptrace.c.  *x/ */
/* OBSOLETE #define FETCH_INFERIOR_REGISTERS */
