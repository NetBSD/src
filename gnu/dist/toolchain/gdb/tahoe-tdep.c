/* OBSOLETE /* Print instructions for Tahoe target machines, for GDB. */
/* OBSOLETE    Copyright 1986, 1989, 1991, 1992, 2000 Free Software Foundation, Inc. */
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
/* OBSOLETE #include "defs.h" */
/* OBSOLETE #include "symtab.h" */
/* OBSOLETE #include "opcode/tahoe.h" */
/* OBSOLETE  */
/* OBSOLETE /* Tahoe instructions are never longer than this.  *x/ */
/* OBSOLETE #define MAXLEN 62 */
/* OBSOLETE  */
/* OBSOLETE /* Number of elements in the opcode table.  *x/ */
/* OBSOLETE #define NOPCODES (sizeof votstrs / sizeof votstrs[0]) */
/* OBSOLETE  */
/* OBSOLETE static unsigned char *print_insn_arg (); */
/* OBSOLETE  */
/* OBSOLETE /* Advance PC across any function entry prologue instructions */
/* OBSOLETE    to reach some "real" code.  *x/ */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR */
/* OBSOLETE tahoe_skip_prologue (pc) */
/* OBSOLETE      CORE_ADDR pc; */
/* OBSOLETE { */
/* OBSOLETE   register int op = (unsigned char) read_memory_integer (pc, 1); */
/* OBSOLETE   if (op == 0x11) */
/* OBSOLETE     pc += 2;			/* skip brb *x/ */
/* OBSOLETE   if (op == 0x13) */
/* OBSOLETE     pc += 3;			/* skip brw *x/ */
/* OBSOLETE   if (op == 0x2c */
/* OBSOLETE       && ((unsigned char) read_memory_integer (pc + 2, 1)) == 0x5e) */
/* OBSOLETE     pc += 3;			/* skip subl2 *x/ */
/* OBSOLETE   if (op == 0xe9 */
/* OBSOLETE       && ((unsigned char) read_memory_integer (pc + 1, 1)) == 0xae */
/* OBSOLETE       && ((unsigned char) read_memory_integer (pc + 3, 1)) == 0x5e) */
/* OBSOLETE     pc += 4;			/* skip movab *x/ */
/* OBSOLETE   if (op == 0xe9 */
/* OBSOLETE       && ((unsigned char) read_memory_integer (pc + 1, 1)) == 0xce */
/* OBSOLETE       && ((unsigned char) read_memory_integer (pc + 4, 1)) == 0x5e) */
/* OBSOLETE     pc += 5;			/* skip movab *x/ */
/* OBSOLETE   if (op == 0xe9 */
/* OBSOLETE       && ((unsigned char) read_memory_integer (pc + 1, 1)) == 0xee */
/* OBSOLETE       && ((unsigned char) read_memory_integer (pc + 6, 1)) == 0x5e) */
/* OBSOLETE     pc += 7;			/* skip movab *x/ */
/* OBSOLETE   return pc; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Return number of args passed to a frame. */
/* OBSOLETE    Can return -1, meaning no way to tell.  *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE tahoe_frame_num_args (fi) */
/* OBSOLETE      struct frame_info *fi; */
/* OBSOLETE { */
/* OBSOLETE   return (((0xffff & read_memory_integer (((fi)->frame - 4), 4)) - 4) >> 2); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Print the Tahoe instruction at address MEMADDR in debugged memory, */
/* OBSOLETE    on STREAM.  Returns length of the instruction, in bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE tahoe_print_insn (memaddr, stream) */
/* OBSOLETE      CORE_ADDR memaddr; */
/* OBSOLETE      struct ui_file *stream; */
/* OBSOLETE { */
/* OBSOLETE   unsigned char buffer[MAXLEN]; */
/* OBSOLETE   register int i; */
/* OBSOLETE   register unsigned char *p; */
/* OBSOLETE   register char *d; */
/* OBSOLETE  */
/* OBSOLETE   read_memory (memaddr, buffer, MAXLEN); */
/* OBSOLETE  */
/* OBSOLETE   for (i = 0; i < NOPCODES; i++) */
/* OBSOLETE     if (votstrs[i].detail.code == buffer[0] */
/* OBSOLETE 	|| votstrs[i].detail.code == *(unsigned short *) buffer) */
/* OBSOLETE       break; */
/* OBSOLETE  */
/* OBSOLETE   /* Handle undefined instructions.  *x/ */
/* OBSOLETE   if (i == NOPCODES) */
/* OBSOLETE     { */
/* OBSOLETE       fprintf_unfiltered (stream, "0%o", buffer[0]); */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   fprintf_unfiltered (stream, "%s", votstrs[i].name); */
/* OBSOLETE  */
/* OBSOLETE   /* Point at first byte of argument data, */
/* OBSOLETE      and at descriptor for first argument.  *x/ */
/* OBSOLETE   p = buffer + 1 + (votstrs[i].detail.code >= 0x100); */
/* OBSOLETE   d = votstrs[i].detail.args; */
/* OBSOLETE  */
/* OBSOLETE   if (*d) */
/* OBSOLETE     fputc_unfiltered ('\t', stream); */
/* OBSOLETE  */
/* OBSOLETE   while (*d) */
/* OBSOLETE     { */
/* OBSOLETE       p = print_insn_arg (d, p, memaddr + (p - buffer), stream); */
/* OBSOLETE       d += 2; */
/* OBSOLETE       if (*d) */
/* OBSOLETE 	fprintf_unfiltered (stream, ","); */
/* OBSOLETE     } */
/* OBSOLETE   return p - buffer; */
/* OBSOLETE } */
/* OBSOLETE /*******************************************************************x/ */
/* OBSOLETE static unsigned char * */
/* OBSOLETE print_insn_arg (d, p, addr, stream) */
/* OBSOLETE      char *d; */
/* OBSOLETE      register char *p; */
/* OBSOLETE      CORE_ADDR addr; */
/* OBSOLETE      struct ui_file *stream; */
/* OBSOLETE { */
/* OBSOLETE   int temp1 = 0; */
/* OBSOLETE   register int regnum = *p & 0xf; */
/* OBSOLETE   float floatlitbuf; */
/* OBSOLETE  */
/* OBSOLETE   if (*d == 'b') */
/* OBSOLETE     { */
/* OBSOLETE       if (d[1] == 'b') */
/* OBSOLETE 	fprintf_unfiltered (stream, "0x%x", addr + *p++ + 1); */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE  */
/* OBSOLETE 	  temp1 = *p; */
/* OBSOLETE 	  temp1 <<= 8; */
/* OBSOLETE 	  temp1 |= *(p + 1); */
/* OBSOLETE 	  fprintf_unfiltered (stream, "0x%x", addr + temp1 + 2); */
/* OBSOLETE 	  p += 2; */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     switch ((*p++ >> 4) & 0xf) */
/* OBSOLETE       { */
/* OBSOLETE       case 0: */
/* OBSOLETE       case 1: */
/* OBSOLETE       case 2: */
/* OBSOLETE       case 3:			/* Literal (short immediate byte) mode *x/ */
/* OBSOLETE 	if (d[1] == 'd' || d[1] == 'f' || d[1] == 'g' || d[1] == 'h') */
/* OBSOLETE 	  { */
/* OBSOLETE 	    *(int *) &floatlitbuf = 0x4000 + ((p[-1] & 0x3f) << 4); */
/* OBSOLETE 	    fprintf_unfiltered (stream, "$%f", floatlitbuf); */
/* OBSOLETE 	  } */
/* OBSOLETE 	else */
/* OBSOLETE 	  fprintf_unfiltered (stream, "$%d", p[-1] & 0x3f); */
/* OBSOLETE 	break; */
/* OBSOLETE  */
/* OBSOLETE       case 4:			/* Indexed *x/ */
/* OBSOLETE 	p = (char *) print_insn_arg (d, p, addr + 1, stream); */
/* OBSOLETE 	fprintf_unfiltered (stream, "[%s]", REGISTER_NAME (regnum)); */
/* OBSOLETE 	break; */
/* OBSOLETE  */
/* OBSOLETE       case 5:			/* Register *x/ */
/* OBSOLETE 	fprintf_unfiltered (stream, REGISTER_NAME (regnum)); */
/* OBSOLETE 	break; */
/* OBSOLETE  */
/* OBSOLETE       case 7:			/* Autodecrement *x/ */
/* OBSOLETE 	fputc_unfiltered ('-', stream); */
/* OBSOLETE       case 6:			/* Register deferred *x/ */
/* OBSOLETE 	fprintf_unfiltered (stream, "(%s)", REGISTER_NAME (regnum)); */
/* OBSOLETE 	break; */
/* OBSOLETE  */
/* OBSOLETE       case 9:			/* Absolute Address & Autoincrement deferred *x/ */
/* OBSOLETE 	fputc_unfiltered ('*', stream); */
/* OBSOLETE 	if (regnum == PC_REGNUM) */
/* OBSOLETE 	  { */
/* OBSOLETE 	    temp1 = *p; */
/* OBSOLETE 	    temp1 <<= 8; */
/* OBSOLETE 	    temp1 |= *(p + 1); */
/* OBSOLETE  */
/* OBSOLETE 	    fputc_unfiltered ('$', stream); */
/* OBSOLETE 	    print_address (temp1, stream); */
/* OBSOLETE 	    p += 4; */
/* OBSOLETE 	    break; */
/* OBSOLETE 	  } */
/* OBSOLETE       case 8:			/*Immediate & Autoincrement SP *x/ */
/* OBSOLETE 	if (regnum == 8)	/*88 is Immediate Byte Mode *x/ */
/* OBSOLETE 	  fprintf_unfiltered (stream, "$%d", *p++); */
/* OBSOLETE  */
/* OBSOLETE 	else if (regnum == 9)	/*89 is Immediate Word Mode *x/ */
/* OBSOLETE 	  { */
/* OBSOLETE 	    temp1 = *p; */
/* OBSOLETE 	    temp1 <<= 8; */
/* OBSOLETE 	    temp1 |= *(p + 1); */
/* OBSOLETE 	    fprintf_unfiltered (stream, "$%d", temp1); */
/* OBSOLETE 	    p += 2; */
/* OBSOLETE 	  } */
/* OBSOLETE  */
/* OBSOLETE 	else if (regnum == PC_REGNUM)	/*8F is Immediate Long Mode *x/ */
/* OBSOLETE 	  { */
/* OBSOLETE 	    temp1 = *p; */
/* OBSOLETE 	    temp1 <<= 8; */
/* OBSOLETE 	    temp1 |= *(p + 1); */
/* OBSOLETE 	    temp1 <<= 8; */
/* OBSOLETE 	    temp1 |= *(p + 2); */
/* OBSOLETE 	    temp1 <<= 8; */
/* OBSOLETE 	    temp1 |= *(p + 3); */
/* OBSOLETE 	    fprintf_unfiltered (stream, "$%d", temp1); */
/* OBSOLETE 	    p += 4; */
/* OBSOLETE 	  } */
/* OBSOLETE  */
/* OBSOLETE 	else			/*8E is Autoincrement SP Mode *x/ */
/* OBSOLETE 	  fprintf_unfiltered (stream, "(%s)+", REGISTER_NAME (regnum)); */
/* OBSOLETE 	break; */
/* OBSOLETE  */
/* OBSOLETE       case 11:			/* Register + Byte Displacement Deferred Mode *x/ */
/* OBSOLETE 	fputc_unfiltered ('*', stream); */
/* OBSOLETE       case 10:			/* Register + Byte Displacement Mode *x/ */
/* OBSOLETE 	if (regnum == PC_REGNUM) */
/* OBSOLETE 	  print_address (addr + *p + 2, stream); */
/* OBSOLETE 	else */
/* OBSOLETE 	  fprintf_unfiltered (stream, "%d(%s)", *p, REGISTER_NAME (regnum)); */
/* OBSOLETE 	p += 1; */
/* OBSOLETE 	break; */
/* OBSOLETE  */
/* OBSOLETE       case 13:			/* Register + Word Displacement Deferred Mode *x/ */
/* OBSOLETE 	fputc_unfiltered ('*', stream); */
/* OBSOLETE       case 12:			/* Register + Word Displacement Mode *x/ */
/* OBSOLETE 	temp1 = *p; */
/* OBSOLETE 	temp1 <<= 8; */
/* OBSOLETE 	temp1 |= *(p + 1); */
/* OBSOLETE 	if (regnum == PC_REGNUM) */
/* OBSOLETE 	  print_address (addr + temp1 + 3, stream); */
/* OBSOLETE 	else */
/* OBSOLETE 	  fprintf_unfiltered (stream, "%d(%s)", temp1, REGISTER_NAME (regnum)); */
/* OBSOLETE 	p += 2; */
/* OBSOLETE 	break; */
/* OBSOLETE  */
/* OBSOLETE       case 15:			/* Register + Long Displacement Deferred Mode *x/ */
/* OBSOLETE 	fputc_unfiltered ('*', stream); */
/* OBSOLETE       case 14:			/* Register + Long Displacement Mode *x/ */
/* OBSOLETE 	temp1 = *p; */
/* OBSOLETE 	temp1 <<= 8; */
/* OBSOLETE 	temp1 |= *(p + 1); */
/* OBSOLETE 	temp1 <<= 8; */
/* OBSOLETE 	temp1 |= *(p + 2); */
/* OBSOLETE 	temp1 <<= 8; */
/* OBSOLETE 	temp1 |= *(p + 3); */
/* OBSOLETE 	if (regnum == PC_REGNUM) */
/* OBSOLETE 	  print_address (addr + temp1 + 5, stream); */
/* OBSOLETE 	else */
/* OBSOLETE 	  fprintf_unfiltered (stream, "%d(%s)", temp1, REGISTER_NAME (regnum)); */
/* OBSOLETE 	p += 4; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE   return (unsigned char *) p; */
/* OBSOLETE } */
