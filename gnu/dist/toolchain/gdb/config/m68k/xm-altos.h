/* OBSOLETE /* Definitions to make GDB run on an Altos 3068 (m68k running SVR2) */
/* OBSOLETE    Copyright (C) 1987,1989 Free Software Foundation, Inc. */
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
/* OBSOLETE /* The altos support would make a good base for a port to other USGR2 systems */
/* OBSOLETE    (like the 3b1 and the Convergent miniframe).  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* This is only needed in one file, but it's cleaner to put it here than */
/* OBSOLETE    putting in more #ifdef's.  *x/ */
/* OBSOLETE #include <sys/page.h> */
/* OBSOLETE #include <sys/net.h> */
/* OBSOLETE  */
/* OBSOLETE #define USG */
/* OBSOLETE  */
/* OBSOLETE #define HAVE_TERMIO */
/* OBSOLETE  */
/* OBSOLETE #define CBREAK XTABS        /* It takes all kinds... *x/ */
/* OBSOLETE  */
/* OBSOLETE #ifndef R_OK */
/* OBSOLETE #define R_OK 4 */
/* OBSOLETE #define W_OK 2 */
/* OBSOLETE #define X_OK 1 */
/* OBSOLETE #define F_OK 0 */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* Get sys/wait.h ie. from a Sun and edit it a little (mc68000 to m68k) *x/ */
/* OBSOLETE /* Why bother?  *x/ */
/* OBSOLETE #if 0 */
/* OBSOLETE #define HAVE_WAIT_STRUCT */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* This is the amount to subtract from u.u_ar0 */
/* OBSOLETE    to get the offset in the core file of the register values. *x/ */
/* OBSOLETE  */
/* OBSOLETE #define KERNEL_U_ADDR 0x1fbf000 */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_U_ADDR(addr, blockend, regno)              \ */
/* OBSOLETE {   if (regno <= SP_REGNUM) \ */
/* OBSOLETE       addr = blockend + regno * 4; \ */
/* OBSOLETE     else if (regno == PS_REGNUM) \ */
/* OBSOLETE       addr = blockend + regno * 4 + 4; \ */
/* OBSOLETE     else if (regno == PC_REGNUM) \ */
/* OBSOLETE       addr = blockend + regno * 4 + 2; \ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_ADDR(u_ar0, regno)                                 \ */
/* OBSOLETE   (((regno) < PS_REGNUM)                                            \ */
/* OBSOLETE    ? (&((struct exception_stack *) (u_ar0))->e_regs[(regno + R0)])  \ */
/* OBSOLETE    : (((regno) == PS_REGNUM)                                                \ */
/* OBSOLETE       ? ((int *) (&((struct exception_stack *) (u_ar0))->e_PS))             \ */
/* OBSOLETE       : (&((struct exception_stack *) (u_ar0))->e_PC))) */
/* OBSOLETE  */
/* OBSOLETE #define FP_REGISTER_ADDR(u, regno)                                  \ */
/* OBSOLETE   (((char *)                                                                \ */
/* OBSOLETE     (((regno) < FPC_REGNUM)                                         \ */
/* OBSOLETE      ? (&u.u_pcb.pcb_mc68881[FMC68881_R0 + (((regno) - FP0_REGNUM) * 3)]) \ */
/* OBSOLETE      : (&u.u_pcb.pcb_mc68881[FMC68881_C + ((regno) - FPC_REGNUM)])))        \ */
/* OBSOLETE    - ((char *) (& u))) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE #ifndef __GNUC__ */
/* OBSOLETE #undef USE_GAS */
/* OBSOLETE #define ALTOS_AS */
/* OBSOLETE #else */
/* OBSOLETE #define USE_GAS */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* Motorola assembly format *x/ */
/* OBSOLETE #if !defined(USE_GAS) && !defined(ALTOS) */
/* OBSOLETE #define MOTOROLA */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* Interface definitions for kernel debugger KDB.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Map machine fault codes into signal numbers. */
/* OBSOLETE    First subtract 0, divide by 4, then index in a table. */
/* OBSOLETE    Faults for which the entry in this table is 0 */
/* OBSOLETE    are not handled by KDB; the program's own trap handler */
/* OBSOLETE    gets to handle then.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define FAULT_CODE_ORIGIN 0 */
/* OBSOLETE #define FAULT_CODE_UNITS 4 */
/* OBSOLETE #define FAULT_TABLE    \ */
/* OBSOLETE { 0, 0, 0, 0, SIGTRAP, 0, 0, 0, \ */
/* OBSOLETE   0, SIGTRAP, 0, 0, 0, 0, 0, SIGKILL, \ */
/* OBSOLETE   0, 0, 0, 0, 0, 0, 0, 0, \ */
/* OBSOLETE   SIGILL } */
/* OBSOLETE  */
/* OBSOLETE /* Start running with a stack stretching from BEG to END. */
/* OBSOLETE    BEG and END should be symbols meaningful to the assembler. */
/* OBSOLETE    This is used only for kdb.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #ifdef MOTOROLA */
/* OBSOLETE #define INIT_STACK(beg, end)  \ */
/* OBSOLETE { asm (".globl end");         \ */
/* OBSOLETE   asm ("move.l $ end, sp");      \ */
/* OBSOLETE   asm ("clr.l fp"); } */
/* OBSOLETE #else */
/* OBSOLETE #ifdef ALTOS_AS */
/* OBSOLETE #define INIT_STACK(beg, end)  \ */
/* OBSOLETE { asm ("global end");         \ */
/* OBSOLETE   asm ("mov.l &end,%sp");      \ */
/* OBSOLETE   asm ("clr.l %fp"); } */
/* OBSOLETE #else */
/* OBSOLETE #define INIT_STACK(beg, end)  \ */
/* OBSOLETE { asm (".globl end");         \ */
/* OBSOLETE   asm ("movel $ end, sp");      \ */
/* OBSOLETE   asm ("clrl fp"); } */
/* OBSOLETE #endif */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* Push the frame pointer register on the stack.  *x/ */
/* OBSOLETE #ifdef MOTOROLA */
/* OBSOLETE #define PUSH_FRAME_PTR        \ */
/* OBSOLETE   asm ("move.l fp, -(sp)"); */
/* OBSOLETE #else */
/* OBSOLETE #ifdef ALTOS_AS */
/* OBSOLETE #define PUSH_FRAME_PTR        \ */
/* OBSOLETE   asm ("mov.l %fp, -(%sp)"); */
/* OBSOLETE #else */
/* OBSOLETE #define PUSH_FRAME_PTR        \ */
/* OBSOLETE   asm ("movel fp, -(sp)"); */
/* OBSOLETE #endif */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* Copy the top-of-stack to the frame pointer register.  *x/ */
/* OBSOLETE #ifdef MOTOROLA */
/* OBSOLETE #define POP_FRAME_PTR  \ */
/* OBSOLETE   asm ("move.l (sp), fp"); */
/* OBSOLETE #else */
/* OBSOLETE #ifdef ALTOS_AS */
/* OBSOLETE #define POP_FRAME_PTR  \ */
/* OBSOLETE   asm ("mov.l (%sp), %fp"); */
/* OBSOLETE #else */
/* OBSOLETE #define POP_FRAME_PTR  \ */
/* OBSOLETE   asm ("movl (sp), fp"); */
/* OBSOLETE #endif */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* After KDB is entered by a fault, push all registers */
/* OBSOLETE    that GDB thinks about (all NUM_REGS of them), */
/* OBSOLETE    so that they appear in order of ascending GDB register number. */
/* OBSOLETE    The fault code will be on the stack beyond the last register.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #ifdef MOTOROLA */
/* OBSOLETE #define PUSH_REGISTERS        \ */
/* OBSOLETE { asm ("clr.w -(sp)");            \ */
/* OBSOLETE   asm ("pea (10,sp)");            \ */
/* OBSOLETE   asm ("movem $ 0xfffe,-(sp)"); } */
/* OBSOLETE #else */
/* OBSOLETE #ifdef ALTOS_AS */
/* OBSOLETE #define PUSH_REGISTERS        \ */
/* OBSOLETE { asm ("clr.w -(%sp)");           \ */
/* OBSOLETE   asm ("pea (10,%sp)");           \ */
/* OBSOLETE   asm ("movm.l &0xfffe,-(%sp)"); } */
/* OBSOLETE #else */
/* OBSOLETE #define PUSH_REGISTERS        \ */
/* OBSOLETE { asm ("clrw -(sp)");             \ */
/* OBSOLETE   asm ("pea 10(sp)");             \ */
/* OBSOLETE   asm ("movem $ 0xfffe,-(sp)"); } */
/* OBSOLETE #endif */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* Assuming the registers (including processor status) have been */
/* OBSOLETE    pushed on the stack in order of ascending GDB register number, */
/* OBSOLETE    restore them and return to the address in the saved PC register.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #ifdef MOTOROLA */
/* OBSOLETE #define POP_REGISTERS          \ */
/* OBSOLETE { asm ("subi.l $8,28(sp)");     \ */
/* OBSOLETE   asm ("movem (sp),$ 0xffff"); \ */
/* OBSOLETE   asm ("rte"); } */
/* OBSOLETE #else */
/* OBSOLETE #ifdef ALTOS_AS */
/* OBSOLETE #define POP_REGISTERS          \ */
/* OBSOLETE { asm ("sub.l &8,28(%sp)");     \ */
/* OBSOLETE   asm ("movem (%sp),&0xffff"); \ */
/* OBSOLETE   asm ("rte"); } */
/* OBSOLETE #else */
/* OBSOLETE #define POP_REGISTERS          \ */
/* OBSOLETE { asm ("subil $8,28(sp)");     \ */
/* OBSOLETE   asm ("movem (sp),$ 0xffff"); \ */
/* OBSOLETE   asm ("rte"); } */
/* OBSOLETE #endif */
/* OBSOLETE #endif */
