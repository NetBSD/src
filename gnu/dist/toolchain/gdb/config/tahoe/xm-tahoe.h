/* OBSOLETE /* Definitions to make GDB hosted on a tahoe running 4.3-Reno */
/* OBSOLETE    Copyright 1986, 1987, 1989, 1991, 1992 Free Software Foundation, Inc. */
/* OBSOLETE    Contributed by the State University of New York at Buffalo, by the */
/* OBSOLETE    Distributed Computer Systems Lab, Department of Computer Science, 1991. */
/* OBSOLETE  */
/* OBSOLETE    This file is part of GDB. */
/* OBSOLETE  */
/* OBSOLETE    This program is free software; you can redistribute it and/or modify */
/* OBSOLETE    it under the terms of the GNU General Public License as published by */
/* OBSOLETE    the Free Software Foundation; either version 2 of the License, or */
/* OBSOLETE    (at your option) any later version. */
/* OBSOLETE  */
/* OBSOLETE    This program is distributed in the hope that it will be useful, */
/* OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE    GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE    You should have received a copy of the GNU General Public License */
/* OBSOLETE    along with this program; if not, write to the Free Software */
/* OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330, */
/* OBSOLETE    Boston, MA 02111-1307, USA.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* Make sure the system include files define BIG_ENDIAN, UINT_MAX, const, */
/* OBSOLETE    etc, rather than GDB's files.  *x/ */
/* OBSOLETE #include <stdio.h> */
/* OBSOLETE #include <sys/param.h> */
/* OBSOLETE  */
/* OBSOLETE /* Host is big-endian *x/ */
/* OBSOLETE  */
/* OBSOLETE #define	HOST_BYTE_ORDER	BIG_ENDIAN */
/* OBSOLETE  */
/* OBSOLETE /* This is the amount to subtract from u.u_ar0 */
/* OBSOLETE    to get the offset in the core file of the register values.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define KERNEL_U_ADDR (0xc0000000 - (TARGET_UPAGES * TARGET_NBPG)) */
/* OBSOLETE  */
/* OBSOLETE #define REGISTER_U_ADDR(addr, blockend, regno)		\ */
/* OBSOLETE { addr = blockend - 100 + regno * 4;			\ */
/* OBSOLETE   if (regno == PC_REGNUM) addr = blockend - 8;		\ */
/* OBSOLETE   if (regno == PS_REGNUM) addr = blockend - 4;		\ */
/* OBSOLETE   if (regno == FP_REGNUM) addr = blockend - 40;	        \ */
/* OBSOLETE   if (regno == SP_REGNUM) addr = blockend - 36;         \ */
/* OBSOLETE   if (regno == AL_REGNUM) addr = blockend - 20;       \ */
/* OBSOLETE   if (regno == AH_REGNUM) addr = blockend - 24;} */
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
/* OBSOLETE { 0, SIGKILL, SIGSEGV, 0, 0, 0, 0, 0, \ */
/* OBSOLETE   0, 0, SIGTRAP, SIGTRAP, 0, 0, 0, 0, \ */
/* OBSOLETE   0, 0, 0, 0, 0, 0, 0, 0} */
/* OBSOLETE  */
/* OBSOLETE /* Start running with a stack stretching from BEG to END. */
/* OBSOLETE    BEG and END should be symbols meaningful to the assembler. */
/* OBSOLETE    This is used only for kdb.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define INIT_STACK(beg, end)  \ */
/* OBSOLETE { asm (".globl end");         \ */
/* OBSOLETE   asm ("movl $ end, sp");      \ */
/* OBSOLETE   asm ("clrl fp"); } */
/* OBSOLETE  */
/* OBSOLETE /* Push the frame pointer register on the stack.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define PUSH_FRAME_PTR        \ */
/* OBSOLETE   asm ("pushl fp"); */
/* OBSOLETE  */
/* OBSOLETE /* Copy the top-of-stack to the frame pointer register.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define POP_FRAME_PTR  \ */
/* OBSOLETE   asm ("movl (sp), fp"); */
/* OBSOLETE  */
/* OBSOLETE /* After KDB is entered by a fault, push all registers */
/* OBSOLETE    that GDB thinks about (all NUM_REGS of them), */
/* OBSOLETE    so that they appear in order of ascending GDB register number. */
/* OBSOLETE    The fault code will be on the stack beyond the last register.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define PUSH_REGISTERS        \ */
/* OBSOLETE { asm ("pushl 8(sp)");        \ */
/* OBSOLETE   asm ("pushl 8(sp)");        \ */
/* OBSOLETE   asm ("pushal 0x41(sp)");    \ */
/* OBSOLETE   asm ("pushl r0" );       \ */
/* OBSOLETE   asm ("pushl r1" );       \ */
/* OBSOLETE   asm ("pushl r2" );       \ */
/* OBSOLETE   asm ("pushl r3" );       \ */
/* OBSOLETE   asm ("pushl r4" );       \ */
/* OBSOLETE   asm ("pushl r5" );       \ */
/* OBSOLETE   asm ("pushl r6" );       \ */
/* OBSOLETE   asm ("pushl r7" );       \ */
/* OBSOLETE   asm ("pushl r8" );       \ */
/* OBSOLETE   asm ("pushl r9" );       \ */
/* OBSOLETE   asm ("pushl r10" );       \ */
/* OBSOLETE   asm ("pushl r11" );       \ */
/* OBSOLETE   asm ("pushl r12" );       \ */
/* OBSOLETE   asm ("pushl fp" );       \ */
/* OBSOLETE   asm ("pushl sp" );       \ */
/* OBSOLETE   asm ("pushl pc" );       \ */
/* OBSOLETE   asm ("pushl ps" );       \ */
/* OBSOLETE   asm ("pushl aclo" );       \ */
/* OBSOLETE   asm ("pushl achi" );       \ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Assuming the registers (including processor status) have been */
/* OBSOLETE    pushed on the stack in order of ascending GDB register number, */
/* OBSOLETE    restore them and return to the address in the saved PC register.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define POP_REGISTERS      \ */
/* OBSOLETE {                          \ */
/* OBSOLETE   asm ("movl (sp)+, achi");   \ */
/* OBSOLETE   asm ("movl (sp)+, aclo");   \ */
/* OBSOLETE   asm ("movl (sp)+, ps");   \ */
/* OBSOLETE   asm ("movl (sp)+, pc");   \ */
/* OBSOLETE   asm ("movl (sp)+, sp");   \ */
/* OBSOLETE   asm ("movl (sp)+, fp");   \ */
/* OBSOLETE   asm ("movl (sp)+, r12");   \ */
/* OBSOLETE   asm ("movl (sp)+, r11");   \ */
/* OBSOLETE   asm ("movl (sp)+, r10");   \ */
/* OBSOLETE   asm ("movl (sp)+, r9");   \ */
/* OBSOLETE   asm ("movl (sp)+, r8");   \ */
/* OBSOLETE   asm ("movl (sp)+, r7");   \ */
/* OBSOLETE   asm ("movl (sp)+, r6");   \ */
/* OBSOLETE   asm ("movl (sp)+, r5");   \ */
/* OBSOLETE   asm ("movl (sp)+, r4");   \ */
/* OBSOLETE   asm ("movl (sp)+, r3");   \ */
/* OBSOLETE   asm ("movl (sp)+, r2");   \ */
/* OBSOLETE   asm ("movl (sp)+, r1");   \ */
/* OBSOLETE   asm ("movl (sp)+, r0");   \ */
/* OBSOLETE   asm ("subl2 $8,(sp)");   \ */
/* OBSOLETE   asm ("movl (sp),sp");    \ */
/* OBSOLETE   asm ("rei"); } */
