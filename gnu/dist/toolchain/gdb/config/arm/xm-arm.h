/* OBSOLETE /* Definitions to make GDB run on an ARM under RISCiX (4.3bsd). */
/* OBSOLETE    Copyright (C) 1986, 1987, 1989 Free Software Foundation, Inc. */
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
/* OBSOLETE #define HOST_BYTE_ORDER LITTLE_ENDIAN */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE #if 0 */
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
/* OBSOLETE #define PUSH_FRAME_PTR        \ */
/* OBSOLETE   asm ("pushl fp"); */
/* OBSOLETE  */
/* OBSOLETE /* Copy the top-of-stack to the frame pointer register.  *x/ */
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
/* OBSOLETE   asm ("pushal 0x14(sp)");    \ */
/* OBSOLETE   asm ("pushr $037777"); } */
/* OBSOLETE  */
/* OBSOLETE /* Assuming the registers (including processor status) have been */
/* OBSOLETE    pushed on the stack in order of ascending GDB register number, */
/* OBSOLETE    restore them and return to the address in the saved PC register.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define POP_REGISTERS      \ */
/* OBSOLETE { asm ("popr $037777");    \ */
/* OBSOLETE   asm ("subl2 $8,(sp)");   \ */
/* OBSOLETE   asm ("movl (sp),sp");    \ */
/* OBSOLETE   asm ("rei"); } */
/* OBSOLETE #endif /* 0 *x/ */
