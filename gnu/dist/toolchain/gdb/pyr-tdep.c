/* OBSOLETE /* Pyramid target-dependent code for GDB. */
/* OBSOLETE    Copyright (C) 1988, 1989, 1991, 2000 Free Software Foundation, Inc. */
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
/* OBSOLETE #include "defs.h" */
/* OBSOLETE  */
/* OBSOLETE /*** Prettier register printing. ***x/ */
/* OBSOLETE  */
/* OBSOLETE /* Print registers in the same format as pyramid's dbx, adb, sdb.  *x/ */
/* OBSOLETE pyr_print_registers(reg_buf, regnum) */
/* OBSOLETE     long *reg_buf[]; */
/* OBSOLETE { */
/* OBSOLETE   register int regno; */
/* OBSOLETE   int usp, ksp; */
/* OBSOLETE   struct user u; */
/* OBSOLETE  */
/* OBSOLETE   for (regno = 0; regno < 16; regno++) { */
/* OBSOLETE     printf_unfiltered/*_filtered*x/ ("%6.6s: %8x  %6.6s: %8x  %6s: %8x  %6s: %8x\n", */
/* OBSOLETE                  REGISTER_NAME (regno), reg_buf[regno], */
/* OBSOLETE                  REGISTER_NAME (regno+16), reg_buf[regno+16], */
/* OBSOLETE                  REGISTER_NAME (regno+32), reg_buf[regno+32], */
/* OBSOLETE                  REGISTER_NAME (regno+48), reg_buf[regno+48]); */
/* OBSOLETE   } */
/* OBSOLETE   usp = ptrace (3, inferior_pid, */
/* OBSOLETE             (PTRACE_ARG3_TYPE) ((char *)&u.u_pcb.pcb_usp) - */
/* OBSOLETE             ((char *)&u), 0); */
/* OBSOLETE   ksp = ptrace (3, inferior_pid, */
/* OBSOLETE             (PTRACE_ARG3_TYPE) ((char *)&u.u_pcb.pcb_ksp) - */
/* OBSOLETE             ((char *)&u), 0); */
/* OBSOLETE   printf_unfiltered/*_filtered*x/ ("\n%6.6s: %8x  %6.6s: %8x (%08x) %6.6s %8x\n", */
/* OBSOLETE                REGISTER_NAME (CSP_REGNUM),reg_buf[CSP_REGNUM], */
/* OBSOLETE                REGISTER_NAME (KSP_REGNUM), reg_buf[KSP_REGNUM], ksp, */
/* OBSOLETE                "usp", usp); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Print the register regnum, or all registers if regnum is -1. */
/* OBSOLETE    fpregs is currently ignored.  *x/ */
/* OBSOLETE  */
/* OBSOLETE pyr_do_registers_info (regnum, fpregs) */
/* OBSOLETE     int regnum; */
/* OBSOLETE     int fpregs; */
/* OBSOLETE { */
/* OBSOLETE   /* On a pyr, we know a virtual register can always fit in an long. */
/* OBSOLETE      Here (and elsewhere) we take advantage of that.  Yuk.  *x/ */
/* OBSOLETE   long raw_regs[MAX_REGISTER_RAW_SIZE*NUM_REGS]; */
/* OBSOLETE   register int i; */
/* OBSOLETE    */
/* OBSOLETE   for (i = 0 ; i < 64 ; i++) { */
/* OBSOLETE     read_relative_register_raw_bytes(i, raw_regs+i); */
/* OBSOLETE   } */
/* OBSOLETE   if (regnum == -1) */
/* OBSOLETE     pyr_print_registers (raw_regs, regnum); */
/* OBSOLETE   else */
/* OBSOLETE     for (i = 0; i < NUM_REGS; i++) */
/* OBSOLETE       if (i == regnum) { */
/* OBSOLETE     long val = raw_regs[i]; */
/* OBSOLETE      */
/* OBSOLETE     fputs_filtered (REGISTER_NAME (i), gdb_stdout); */
/* OBSOLETE     printf_filtered(":"); */
/* OBSOLETE     print_spaces_filtered (6 - strlen (REGISTER_NAME (i)), gdb_stdout); */
/* OBSOLETE     if (val == 0) */
/* OBSOLETE       printf_filtered ("0"); */
/* OBSOLETE     else */
/* OBSOLETE       printf_filtered ("%s  %d", local_hex_string_custom(val,"08"), val); */
/* OBSOLETE     printf_filtered("\n"); */
/* OBSOLETE       } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /*** Debugging editions of various macros from m-pyr.h ****x/ */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR frame_locals_address (frame) */
/* OBSOLETE     struct frame_info *frame; */
/* OBSOLETE { */
/* OBSOLETE   register int addr = find_saved_register (frame,CFP_REGNUM); */
/* OBSOLETE   register int result = read_memory_integer (addr, 4); */
/* OBSOLETE #ifdef PYRAMID_CONTROL_FRAME_DEBUGGING */
/* OBSOLETE   fprintf_unfiltered (gdb_stderr, */
/* OBSOLETE        "\t[[..frame_locals:%8x, %s= %x @%x fcfp= %x foo= %x\n\t gr13=%x pr13=%x tr13=%x @%x]]\n", */
/* OBSOLETE        frame->frame, */
/* OBSOLETE        REGISTER_NAME (CFP_REGNUM), */
/* OBSOLETE        result, addr, */
/* OBSOLETE        frame->frame_cfp, (CFP_REGNUM), */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE        read_register(13), read_register(29), read_register(61), */
/* OBSOLETE        find_saved_register(frame, 61)); */
/* OBSOLETE #endif /* PYRAMID_CONTROL_FRAME_DEBUGGING *x/ */
/* OBSOLETE  */
/* OBSOLETE   /* FIXME: I thought read_register (CFP_REGNUM) should be the right answer; */
/* OBSOLETE      or at least CFP_REGNUM relative to FRAME (ie, result). */
/* OBSOLETE      There seems to be a bug in the way the innermost frame is set up.  *x/ */
/* OBSOLETE  */
/* OBSOLETE     return ((frame->next) ? result: frame->frame_cfp); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR frame_args_addr (frame) */
/* OBSOLETE     struct frame_info *frame; */
/* OBSOLETE { */
/* OBSOLETE   register int addr = find_saved_register (frame,CFP_REGNUM); */
/* OBSOLETE   register int result = read_memory_integer (addr, 4); */
/* OBSOLETE  */
/* OBSOLETE #ifdef PYRAMID_CONTROL_FRAME_DEBUGGING */
/* OBSOLETE   fprintf_unfiltered (gdb_stderr, */
/* OBSOLETE        "\t[[..frame_args:%8x, %s= %x @%x fcfp= %x r_r= %x\n\t gr13=%x pr13=%x tr13=%x @%x]]\n", */
/* OBSOLETE        frame->frame, */
/* OBSOLETE        REGISTER_NAME (CFP_REGNUM), */
/* OBSOLETE        result, addr, */
/* OBSOLETE        frame->frame_cfp, read_register(CFP_REGNUM), */
/* OBSOLETE  */
/* OBSOLETE        read_register(13), read_register(29), read_register(61), */
/* OBSOLETE        find_saved_register(frame, 61)); */
/* OBSOLETE #endif /*  PYRAMID_CONTROL_FRAME_DEBUGGING *x/ */
/* OBSOLETE  */
/* OBSOLETE   /* FIXME: I thought read_register (CFP_REGNUM) should be the right answer; */
/* OBSOLETE      or at least CFP_REGNUM relative to FRAME (ie, result). */
/* OBSOLETE      There seems to be a bug in the way the innermost frame is set up.  *x/ */
/* OBSOLETE     return ((frame->next) ? result: frame->frame_cfp); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #include "symtab.h" */
/* OBSOLETE #include "opcode/pyr.h" */
/* OBSOLETE #include "gdbcore.h" */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /*  A couple of functions used for debugging frame-handling on */
/* OBSOLETE     Pyramids. (The Pyramid-dependent handling of register values for */
/* OBSOLETE     windowed registers is known to be buggy.) */
/* OBSOLETE  */
/* OBSOLETE     When debugging, these functions can supplant the normal definitions of some */
/* OBSOLETE     of the macros in tm-pyramid.h  The quantity of information produced */
/* OBSOLETE     when these functions are used makes the gdb  unusable as a */
/* OBSOLETE     debugger for user programs.  *x/ */
/* OBSOLETE      */
/* OBSOLETE extern unsigned pyr_saved_pc(), pyr_frame_chain(); */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR pyr_frame_chain(frame) */
/* OBSOLETE     CORE_ADDR frame; */
/* OBSOLETE { */
/* OBSOLETE     int foo=frame - CONTROL_STACK_FRAME_SIZE; */
/* OBSOLETE     /* printf_unfiltered ("...following chain from %x: got %x\n", frame, foo);*x/ */
/* OBSOLETE     return foo; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR pyr_saved_pc(frame) */
/* OBSOLETE     CORE_ADDR frame; */
/* OBSOLETE { */
/* OBSOLETE     int foo=0; */
/* OBSOLETE     foo = read_memory_integer (((CORE_ADDR)(frame))+60, 4); */
/* OBSOLETE     printf_unfiltered ("..reading pc from frame 0x%0x+%d regs: got %0x\n", */
/* OBSOLETE         frame, 60/4, foo); */
/* OBSOLETE     return foo; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Pyramid instructions are never longer than this many bytes.  *x/ */
/* OBSOLETE #define MAXLEN 24 */
/* OBSOLETE  */
/* OBSOLETE /* Number of elements in the opcode table.  *x/ */
/* OBSOLETE /*const*x/ static int nopcodes = (sizeof (pyr_opcodes) / sizeof( pyr_opcodes[0])); */
/* OBSOLETE #define NOPCODES (nopcodes) */
/* OBSOLETE  */
/* OBSOLETE /* Let's be byte-independent so we can use this as a cross-assembler.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define NEXTLONG(p)  \ */
/* OBSOLETE   (p += 4, (((((p[-4] << 8) + p[-3]) << 8) + p[-2]) << 8) + p[-1]) */
/* OBSOLETE  */
/* OBSOLETE /* Print one instruction at address MEMADDR in debugged memory, */
/* OBSOLETE    on STREAM.  Returns length of the instruction, in bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE pyr_print_insn (memaddr, stream) */
/* OBSOLETE      CORE_ADDR memaddr; */
/* OBSOLETE      struct ui_file *stream; */
/* OBSOLETE { */
/* OBSOLETE   unsigned char buffer[MAXLEN]; */
/* OBSOLETE   register int i, nargs, insn_size =4; */
/* OBSOLETE   register unsigned char *p; */
/* OBSOLETE   register char *d; */
/* OBSOLETE   register int insn_opcode, operand_mode; */
/* OBSOLETE   register int index_multiplier, index_reg_regno, op_1_regno, op_2_regno ; */
/* OBSOLETE   long insn;                        /* first word of the insn, not broken down. *x/ */
/* OBSOLETE   pyr_insn_format insn_decode;      /* the same, broken out into op{code,erands} *x/ */
/* OBSOLETE   long extra_1, extra_2; */
/* OBSOLETE  */
/* OBSOLETE   read_memory (memaddr, buffer, MAXLEN); */
/* OBSOLETE   insn_decode = *((pyr_insn_format *) buffer); */
/* OBSOLETE   insn = * ((int *) buffer); */
/* OBSOLETE   insn_opcode = insn_decode.operator; */
/* OBSOLETE   operand_mode = insn_decode.mode; */
/* OBSOLETE   index_multiplier = insn_decode.index_scale; */
/* OBSOLETE   index_reg_regno = insn_decode.index_reg; */
/* OBSOLETE   op_1_regno = insn_decode.operand_1; */
/* OBSOLETE   op_2_regno = insn_decode.operand_2; */
/* OBSOLETE    */
/* OBSOLETE    */
/* OBSOLETE   if (*((int *)buffer) == 0x0) { */
/* OBSOLETE     /* "halt" looks just like an invalid "jump" to the insn decoder, */
/* OBSOLETE        so is dealt with as a special case *x/ */
/* OBSOLETE     fprintf_unfiltered (stream, "halt"); */
/* OBSOLETE     return (4); */
/* OBSOLETE   } */
/* OBSOLETE  */
/* OBSOLETE   for (i = 0; i < NOPCODES; i++) */
/* OBSOLETE       if (pyr_opcodes[i].datum.code == insn_opcode) */
/* OBSOLETE               break; */
/* OBSOLETE  */
/* OBSOLETE   if (i == NOPCODES) */
/* OBSOLETE       /* FIXME: Handle unrecognised instructions better.  *x/ */
/* OBSOLETE       fprintf_unfiltered (stream, "???\t#%08x\t(op=%x mode =%x)", */
/* OBSOLETE                insn, insn_decode.operator, insn_decode.mode); */
/* OBSOLETE   else */
/* OBSOLETE     { */
/* OBSOLETE       /* Print the mnemonic for the instruction.  Pyramid insn operands */
/* OBSOLETE          are so regular that we can deal with almost all of them */
/* OBSOLETE          separately. */
/* OBSOLETE      Unconditional branches are an exception: they are encoded as */
/* OBSOLETE      conditional branches (branch if false condition, I think) */
/* OBSOLETE      with no condition specified. The average user will not be */
/* OBSOLETE      aware of this. To maintain their illusion that an */
/* OBSOLETE      unconditional branch insn exists, we will have to FIXME to */
/* OBSOLETE      treat the insn mnemnonic of all branch instructions here as a */
/* OBSOLETE      special case: check the operands of branch insn and print an */
/* OBSOLETE      appropriate mnemonic. *x/  */
/* OBSOLETE  */
/* OBSOLETE       fprintf_unfiltered (stream, "%s\t", pyr_opcodes[i].name); */
/* OBSOLETE  */
/* OBSOLETE     /* Print the operands of the insn (as specified in */
/* OBSOLETE        insn.operand_mode).  */
/* OBSOLETE        Branch operands of branches are a special case: they are a word */
/* OBSOLETE        offset, not a byte offset. *x/ */
/* OBSOLETE    */
/* OBSOLETE     if (insn_decode.operator == 0x01 || insn_decode.operator == 0x02) { */
/* OBSOLETE       register int bit_codes=(insn >> 16)&0xf; */
/* OBSOLETE       register int i; */
/* OBSOLETE       register int displacement = (insn & 0x0000ffff) << 2; */
/* OBSOLETE  */
/* OBSOLETE       static char cc_bit_names[] = "cvzn";  /* z,n,c,v: strange order? *x/ */
/* OBSOLETE  */
/* OBSOLETE       /* Is bfc and no bits specified an unconditional branch?*x/ */
/* OBSOLETE       for (i=0;i<4;i++) { */
/* OBSOLETE     if ((bit_codes) & 0x1) */
/* OBSOLETE             fputc_unfiltered (cc_bit_names[i], stream); */
/* OBSOLETE     bit_codes >>= 1; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE       fprintf_unfiltered (stream, ",%0x", */
/* OBSOLETE            displacement + memaddr); */
/* OBSOLETE       return (insn_size); */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE       switch (operand_mode) { */
/* OBSOLETE       case 0: */
/* OBSOLETE     fprintf_unfiltered (stream, "%s,%s", */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              REGISTER_NAME (op_2_regno)); */
/* OBSOLETE     break; */
/* OBSOLETE          */
/* OBSOLETE       case 1: */
/* OBSOLETE     fprintf_unfiltered (stream, " 0x%0x,%s", */
/* OBSOLETE              op_1_regno, */
/* OBSOLETE              REGISTER_NAME (op_2_regno)); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE       case 2: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, " $0x%0x,%s", */
/* OBSOLETE              extra_1, */
/* OBSOLETE              REGISTER_NAME (op_2_regno)); */
/* OBSOLETE     break; */
/* OBSOLETE       case 3: */
/* OBSOLETE     fprintf_unfiltered (stream, " (%s),%s", */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              REGISTER_NAME (op_2_regno)); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE       case 4: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, " 0x%0x(%s),%s", */
/* OBSOLETE              extra_1, */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              REGISTER_NAME (op_2_regno)); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE     /* S1 destination mode *x/ */
/* OBSOLETE       case 5: */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? "%s,(%s)[%s*%1d]" : "%s,(%s)"), */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE       case 6: */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? " $%#0x,(%s)[%s*%1d]" */
/* OBSOLETE               : " $%#0x,(%s)"), */
/* OBSOLETE              op_1_regno, */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE       case 7: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? " $%#0x,(%s)[%s*%1d]" */
/* OBSOLETE               : " $%#0x,(%s)"), */
/* OBSOLETE              extra_1, */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE       case 8: */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? " (%s),(%s)[%s*%1d]" : " (%s),(%s)"), */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE       case 9: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) */
/* OBSOLETE               ? "%#0x(%s),(%s)[%s*%1d]" */
/* OBSOLETE               : "%#0x(%s),(%s)"), */
/* OBSOLETE              extra_1, */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE     /* S2 destination mode *x/ */
/* OBSOLETE       case 10: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? "%s,%#0x(%s)[%s*%1d]" : "%s,%#0x(%s)"), */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              extra_1, */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE       case 11: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? */
/* OBSOLETE               " $%#0x,%#0x(%s)[%s*%1d]" : " $%#0x,%#0x(%s)"), */
/* OBSOLETE              op_1_regno, */
/* OBSOLETE              extra_1, */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE       case 12: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     read_memory (memaddr+8, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_2 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? */
/* OBSOLETE               " $%#0x,%#0x(%s)[%s*%1d]" : " $%#0x,%#0x(%s)"), */
/* OBSOLETE              extra_1, */
/* OBSOLETE              extra_2, */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE       case 13: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) */
/* OBSOLETE               ? " (%s),%#0x(%s)[%s*%1d]"  */
/* OBSOLETE               : " (%s),%#0x(%s)"), */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              extra_1, */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE       case 14: */
/* OBSOLETE     read_memory (memaddr+4, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_1 = * ((int *) buffer); */
/* OBSOLETE     read_memory (memaddr+8, buffer, MAXLEN); */
/* OBSOLETE     insn_size += 4; */
/* OBSOLETE     extra_2 = * ((int *) buffer); */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? "%#0x(%s),%#0x(%s)[%s*%1d]" */
/* OBSOLETE               : "%#0x(%s),%#0x(%s) "), */
/* OBSOLETE              extra_1, */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              extra_2, */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     break; */
/* OBSOLETE      */
/* OBSOLETE       default: */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              ((index_reg_regno) ? "%s,%s [%s*%1d]" : "%s,%s"), */
/* OBSOLETE              REGISTER_NAME (op_1_regno), */
/* OBSOLETE              REGISTER_NAME (op_2_regno), */
/* OBSOLETE              REGISTER_NAME (index_reg_regno), */
/* OBSOLETE              index_multiplier); */
/* OBSOLETE     fprintf_unfiltered (stream, */
/* OBSOLETE              "\t\t# unknown mode in %08x", */
/* OBSOLETE              insn); */
/* OBSOLETE     break; */
/* OBSOLETE       } /* switch *x/ */
/* OBSOLETE     } */
/* OBSOLETE    */
/* OBSOLETE   { */
/* OBSOLETE     return insn_size; */
/* OBSOLETE   } */
/* OBSOLETE   abort (); */
/* OBSOLETE } */
