/* OBSOLETE /* Target-dependent code for Mitsubishi D30V, for GDB. */
/* OBSOLETE  */
/* OBSOLETE    Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002 Free Software */
/* OBSOLETE    Foundation, Inc. */
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
/* OBSOLETE    Boston, MA 02111-1307, USA.  */ */
/* OBSOLETE  */
/* OBSOLETE /*  Contributed by Martin Hunt, hunt@cygnus.com */ */
/* OBSOLETE  */
/* OBSOLETE #include "defs.h" */
/* OBSOLETE #include "frame.h" */
/* OBSOLETE #include "obstack.h" */
/* OBSOLETE #include "symtab.h" */
/* OBSOLETE #include "gdbtypes.h" */
/* OBSOLETE #include "gdbcmd.h" */
/* OBSOLETE #include "gdbcore.h" */
/* OBSOLETE #include "gdb_string.h" */
/* OBSOLETE #include "value.h" */
/* OBSOLETE #include "inferior.h" */
/* OBSOLETE #include "dis-asm.h" */
/* OBSOLETE #include "symfile.h" */
/* OBSOLETE #include "objfiles.h" */
/* OBSOLETE #include "regcache.h" */
/* OBSOLETE  */
/* OBSOLETE #include "language.h" /* For local_hex_string() */ */
/* OBSOLETE  */
/* OBSOLETE void d30v_frame_find_saved_regs (struct frame_info *fi, */
/* OBSOLETE 				 struct frame_saved_regs *fsr); */
/* OBSOLETE void d30v_frame_find_saved_regs_offsets (struct frame_info *fi, */
/* OBSOLETE 					 struct frame_saved_regs *fsr); */
/* OBSOLETE static void d30v_pop_dummy_frame (struct frame_info *fi); */
/* OBSOLETE static void d30v_print_flags (void); */
/* OBSOLETE static void print_flags_command (char *, int); */
/* OBSOLETE  */
/* OBSOLETE /* the following defines assume: */
/* OBSOLETE    fp is r61, lr is r62, sp is r63, and ?? is r22 */
/* OBSOLETE    if that changes, they will need to be updated */ */
/* OBSOLETE  */
/* OBSOLETE #define OP_MASK_ALL_BUT_RA	0x0ffc0fff	/* throw away Ra, keep the rest */ */
/* OBSOLETE  */
/* OBSOLETE #define OP_STW_SPM		0x054c0fc0	/* stw Ra, @(sp-) */ */
/* OBSOLETE #define OP_STW_SP_R0		0x05400fc0	/* stw Ra, @(sp,r0) */ */
/* OBSOLETE #define OP_STW_SP_IMM0		0x05480fc0	/* st Ra, @(sp, 0x0) */ */
/* OBSOLETE #define OP_STW_R22P_R0		0x05440580	/* stw Ra, @(r22+,r0) */ */
/* OBSOLETE  */
/* OBSOLETE #define OP_ST2W_SPM		0x056c0fc0	/* st2w Ra, @(sp-) */ */
/* OBSOLETE #define OP_ST2W_SP_R0		0x05600fc0	/* st2w Ra, @(sp, r0) */ */
/* OBSOLETE #define OP_ST2W_SP_IMM0		0x05680fc0	/* st2w Ra, @(sp, 0x0) */ */
/* OBSOLETE #define OP_ST2W_R22P_R0		0x05640580	/* st2w Ra, @(r22+, r0) */ */
/* OBSOLETE  */
/* OBSOLETE #define OP_MASK_OPCODE		0x0ffc0000	/* just the opcode, ign operands */ */
/* OBSOLETE #define OP_NOP			0x00f00000	/* nop */ */
/* OBSOLETE  */
/* OBSOLETE #define OP_MASK_ALL_BUT_IMM	0x0fffffc0	/* throw away imm, keep the rest */ */
/* OBSOLETE #define OP_SUB_SP_IMM		0x082bffc0	/* sub sp,sp,imm */ */
/* OBSOLETE #define OP_ADD_SP_IMM		0x080bffc0	/* add sp,sp,imm */ */
/* OBSOLETE #define OP_ADD_R22_SP_IMM	0x08096fc0	/* add r22,sp,imm */ */
/* OBSOLETE #define OP_STW_FP_SP_IMM	0x054bdfc0	/* stw fp,@(sp,imm) */ */
/* OBSOLETE #define OP_OR_SP_R0_IMM		0x03abf000	/* or sp,r0,imm */ */
/* OBSOLETE  */
/* OBSOLETE /* no mask */ */
/* OBSOLETE #define OP_OR_FP_R0_SP		0x03a3d03f	/* or fp,r0,sp */ */
/* OBSOLETE #define OP_OR_FP_SP_R0		0x03a3dfc0	/* or fp,sp,r0 */ */
/* OBSOLETE #define OP_OR_FP_IMM0_SP	0x03abd03f	/* or fp,0x0,sp */ */
/* OBSOLETE #define OP_STW_FP_R22P_R0	0x0547d580	/* stw fp,@(r22+,r0) */ */
/* OBSOLETE #define OP_STW_LR_R22P_R0	0x0547e580	/* stw lr,@(r22+,r0) */ */
/* OBSOLETE  */
/* OBSOLETE #define OP_MASK_OP_AND_RB	0x0ff80fc0	/* keep op and rb,throw away rest */ */
/* OBSOLETE #define OP_STW_SP_IMM		0x05480fc0	/* stw Ra,@(sp,imm) */ */
/* OBSOLETE #define OP_ST2W_SP_IMM		0x05680fc0	/* st2w Ra,@(sp,imm) */ */
/* OBSOLETE #define OP_STW_FP_IMM		0x05480f40	/* stw Ra,@(fp,imm) */ */
/* OBSOLETE #define OP_STW_FP_R0		0x05400f40	/* stw Ra,@(fp,r0) */ */
/* OBSOLETE  */
/* OBSOLETE #define OP_MASK_FM_BIT		0x80000000 */
/* OBSOLETE #define OP_MASK_CC_BITS		0x70000000 */
/* OBSOLETE #define OP_MASK_SUB_INST	0x0fffffff */
/* OBSOLETE  */
/* OBSOLETE #define EXTRACT_RA(op)		(((op) >> 12) & 0x3f) */
/* OBSOLETE #define EXTRACT_RB(op)		(((op) >> 6) & 0x3f) */
/* OBSOLETE #define EXTRACT_RC(op)		(((op) & 0x3f) */
/* OBSOLETE #define EXTRACT_UIMM6(op)	((op) & 0x3f) */
/* OBSOLETE #define EXTRACT_IMM6(op)	((((int)EXTRACT_UIMM6(op)) << 26) >> 26) */
/* OBSOLETE #define EXTRACT_IMM26(op)	((((op)&0x0ff00000) >> 2) | ((op)&0x0003ffff)) */
/* OBSOLETE #define EXTRACT_IMM32(opl, opr)	((EXTRACT_UIMM6(opl) << 26)|EXTRACT_IMM26(opr)) */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE d30v_frame_chain_valid (CORE_ADDR chain, struct frame_info *fi) */
/* OBSOLETE { */
/* OBSOLETE #if 0 */
/* OBSOLETE   return ((chain) != 0 && (fi) != 0 && (fi)->return_pc != 0); */
/* OBSOLETE #else */
/* OBSOLETE   return ((chain) != 0 && (fi) != 0 && (fi)->frame <= chain); */
/* OBSOLETE #endif */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Discard from the stack the innermost frame, restoring all saved */
/* OBSOLETE    registers.  */ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_pop_frame (void) */
/* OBSOLETE { */
/* OBSOLETE   struct frame_info *frame = get_current_frame (); */
/* OBSOLETE   CORE_ADDR fp; */
/* OBSOLETE   int regnum; */
/* OBSOLETE   struct frame_saved_regs fsr; */
/* OBSOLETE   char raw_buffer[8]; */
/* OBSOLETE  */
/* OBSOLETE   fp = FRAME_FP (frame); */
/* OBSOLETE   if (frame->dummy) */
/* OBSOLETE     { */
/* OBSOLETE       d30v_pop_dummy_frame (frame); */
/* OBSOLETE       return; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* fill out fsr with the address of where each */ */
/* OBSOLETE   /* register was stored in the frame */ */
/* OBSOLETE   get_frame_saved_regs (frame, &fsr); */
/* OBSOLETE  */
/* OBSOLETE   /* now update the current registers with the old values */ */
/* OBSOLETE   for (regnum = A0_REGNUM; regnum < A0_REGNUM + 2; regnum++) */
/* OBSOLETE     { */
/* OBSOLETE       if (fsr.regs[regnum]) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  read_memory (fsr.regs[regnum], raw_buffer, 8); */
/* OBSOLETE 	  write_register_bytes (REGISTER_BYTE (regnum), raw_buffer, 8); */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE   for (regnum = 0; regnum < SP_REGNUM; regnum++) */
/* OBSOLETE     { */
/* OBSOLETE       if (fsr.regs[regnum]) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  write_register (regnum, read_memory_unsigned_integer (fsr.regs[regnum], 4)); */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE   if (fsr.regs[PSW_REGNUM]) */
/* OBSOLETE     { */
/* OBSOLETE       write_register (PSW_REGNUM, read_memory_unsigned_integer (fsr.regs[PSW_REGNUM], 4)); */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   write_register (PC_REGNUM, read_register (LR_REGNUM)); */
/* OBSOLETE   write_register (SP_REGNUM, fp + frame->size); */
/* OBSOLETE   target_store_registers (-1); */
/* OBSOLETE   flush_cached_frames (); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static int */
/* OBSOLETE check_prologue (unsigned long op) */
/* OBSOLETE { */
/* OBSOLETE   /* add sp,sp,imm -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_IMM) == OP_ADD_SP_IMM) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* add r22,sp,imm -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_IMM) == OP_ADD_R22_SP_IMM) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* or  fp,r0,sp -- observed */ */
/* OBSOLETE   if (op == OP_OR_FP_R0_SP) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* nop */ */
/* OBSOLETE   if ((op & OP_MASK_OPCODE) == OP_NOP) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* stw  Ra,@(sp,r0) */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_STW_SP_R0) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* stw  Ra,@(sp,0x0) */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_STW_SP_IMM0) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* st2w  Ra,@(sp,r0) */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_ST2W_SP_R0) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* st2w  Ra,@(sp,0x0) */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_ST2W_SP_IMM0) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* stw fp, @(r22+,r0) -- observed */ */
/* OBSOLETE   if (op == OP_STW_FP_R22P_R0) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* stw r62, @(r22+,r0) -- observed */ */
/* OBSOLETE   if (op == OP_STW_LR_R22P_R0) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* stw Ra, @(fp,r0) -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_STW_FP_R0) */
/* OBSOLETE     return 1;			/* first arg */ */
/* OBSOLETE  */
/* OBSOLETE   /* stw Ra, @(fp,imm) -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_OP_AND_RB) == OP_STW_FP_IMM) */
/* OBSOLETE     return 1;			/* second and subsequent args */ */
/* OBSOLETE  */
/* OBSOLETE   /* stw fp,@(sp,imm) -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_IMM) == OP_STW_FP_SP_IMM) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* st2w Ra,@(r22+,r0) */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_ST2W_R22P_R0) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* stw  Ra, @(sp-) */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_STW_SPM) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* st2w  Ra, @(sp-) */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_ST2W_SPM) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* sub.?  sp,sp,imm */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_IMM) == OP_SUB_SP_IMM) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   return 0; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR */
/* OBSOLETE d30v_skip_prologue (CORE_ADDR pc) */
/* OBSOLETE { */
/* OBSOLETE   unsigned long op[2]; */
/* OBSOLETE   unsigned long opl, opr;	/* left / right sub operations */ */
/* OBSOLETE   unsigned long fm0, fm1;	/* left / right mode bits */ */
/* OBSOLETE   unsigned long cc0, cc1; */
/* OBSOLETE   unsigned long op1, op2; */
/* OBSOLETE   CORE_ADDR func_addr, func_end; */
/* OBSOLETE   struct symtab_and_line sal; */
/* OBSOLETE  */
/* OBSOLETE   /* If we have line debugging information, then the end of the */ */
/* OBSOLETE   /* prologue should the first assembly instruction of  the first source line */ */
/* OBSOLETE   if (find_pc_partial_function (pc, NULL, &func_addr, &func_end)) */
/* OBSOLETE     { */
/* OBSOLETE       sal = find_pc_line (func_addr, 0); */
/* OBSOLETE       if (sal.end && sal.end < func_end) */
/* OBSOLETE 	return sal.end; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   if (target_read_memory (pc, (char *) &op[0], 8)) */
/* OBSOLETE     return pc;			/* Can't access it -- assume no prologue. */ */
/* OBSOLETE  */
/* OBSOLETE   while (1) */
/* OBSOLETE     { */
/* OBSOLETE       opl = (unsigned long) read_memory_integer (pc, 4); */
/* OBSOLETE       opr = (unsigned long) read_memory_integer (pc + 4, 4); */
/* OBSOLETE  */
/* OBSOLETE       fm0 = (opl & OP_MASK_FM_BIT); */
/* OBSOLETE       fm1 = (opr & OP_MASK_FM_BIT); */
/* OBSOLETE  */
/* OBSOLETE       cc0 = (opl & OP_MASK_CC_BITS); */
/* OBSOLETE       cc1 = (opr & OP_MASK_CC_BITS); */
/* OBSOLETE  */
/* OBSOLETE       opl = (opl & OP_MASK_SUB_INST); */
/* OBSOLETE       opr = (opr & OP_MASK_SUB_INST); */
/* OBSOLETE  */
/* OBSOLETE       if (fm0 && fm1) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  /* long instruction (opl contains the opcode) */ */
/* OBSOLETE 	  if (((opl & OP_MASK_ALL_BUT_IMM) != OP_ADD_SP_IMM) &&		/* add sp,sp,imm */ */
/* OBSOLETE 	      ((opl & OP_MASK_ALL_BUT_IMM) != OP_ADD_R22_SP_IMM) &&	/* add r22,sp,imm */ */
/* OBSOLETE 	      ((opl & OP_MASK_OP_AND_RB) != OP_STW_SP_IMM) &&	/* stw Ra, @(sp,imm) */ */
/* OBSOLETE 	      ((opl & OP_MASK_OP_AND_RB) != OP_ST2W_SP_IMM))	/* st2w Ra, @(sp,imm) */ */
/* OBSOLETE 	    break; */
/* OBSOLETE 	} */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE 	  /* short instructions */ */
/* OBSOLETE 	  if (fm0 && !fm1) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      op1 = opr; */
/* OBSOLETE 	      op2 = opl; */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else */
/* OBSOLETE 	    { */
/* OBSOLETE 	      op1 = opl; */
/* OBSOLETE 	      op2 = opr; */
/* OBSOLETE 	    } */
/* OBSOLETE 	  if (check_prologue (op1)) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      if (!check_prologue (op2)) */
/* OBSOLETE 		{ */
/* OBSOLETE 		  /* if the previous opcode was really part of the prologue */ */
/* OBSOLETE 		  /* and not just a NOP, then we want to break after both instructions */ */
/* OBSOLETE 		  if ((op1 & OP_MASK_OPCODE) != OP_NOP) */
/* OBSOLETE 		    pc += 8; */
/* OBSOLETE 		  break; */
/* OBSOLETE 		} */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else */
/* OBSOLETE 	    break; */
/* OBSOLETE 	} */
/* OBSOLETE       pc += 8; */
/* OBSOLETE     } */
/* OBSOLETE   return pc; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static int end_of_stack; */
/* OBSOLETE  */
/* OBSOLETE /* Given a GDB frame, determine the address of the calling function's frame. */
/* OBSOLETE    This will be used to create a new GDB frame struct, and then */
/* OBSOLETE    INIT_EXTRA_FRAME_INFO and INIT_FRAME_PC will be called for the new frame. */
/* OBSOLETE  */ */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR */
/* OBSOLETE d30v_frame_chain (struct frame_info *frame) */
/* OBSOLETE { */
/* OBSOLETE   struct frame_saved_regs fsr; */
/* OBSOLETE  */
/* OBSOLETE   d30v_frame_find_saved_regs (frame, &fsr); */
/* OBSOLETE  */
/* OBSOLETE   if (end_of_stack) */
/* OBSOLETE     return (CORE_ADDR) 0; */
/* OBSOLETE  */
/* OBSOLETE   if (frame->return_pc == IMEM_START) */
/* OBSOLETE     return (CORE_ADDR) 0; */
/* OBSOLETE  */
/* OBSOLETE   if (!fsr.regs[FP_REGNUM]) */
/* OBSOLETE     { */
/* OBSOLETE       if (!fsr.regs[SP_REGNUM] || fsr.regs[SP_REGNUM] == STACK_START) */
/* OBSOLETE 	return (CORE_ADDR) 0; */
/* OBSOLETE  */
/* OBSOLETE       return fsr.regs[SP_REGNUM]; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   if (!read_memory_unsigned_integer (fsr.regs[FP_REGNUM], 4)) */
/* OBSOLETE     return (CORE_ADDR) 0; */
/* OBSOLETE  */
/* OBSOLETE   return read_memory_unsigned_integer (fsr.regs[FP_REGNUM], 4); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static int next_addr, uses_frame; */
/* OBSOLETE static int frame_size; */
/* OBSOLETE  */
/* OBSOLETE static int */
/* OBSOLETE prologue_find_regs (unsigned long op, struct frame_saved_regs *fsr, */
/* OBSOLETE 		    CORE_ADDR addr) */
/* OBSOLETE { */
/* OBSOLETE   int n; */
/* OBSOLETE   int offset; */
/* OBSOLETE  */
/* OBSOLETE   /* add sp,sp,imm -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_IMM) == OP_ADD_SP_IMM) */
/* OBSOLETE     { */
/* OBSOLETE       offset = EXTRACT_IMM6 (op); */
/* OBSOLETE       /*next_addr += offset; */ */
/* OBSOLETE       frame_size += -offset; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* add r22,sp,imm -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_IMM) == OP_ADD_R22_SP_IMM) */
/* OBSOLETE     { */
/* OBSOLETE       offset = EXTRACT_IMM6 (op); */
/* OBSOLETE       next_addr = (offset - frame_size); */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* stw Ra, @(fp, offset) -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_OP_AND_RB) == OP_STW_FP_IMM) */
/* OBSOLETE     { */
/* OBSOLETE       n = EXTRACT_RA (op); */
/* OBSOLETE       offset = EXTRACT_IMM6 (op); */
/* OBSOLETE       fsr->regs[n] = (offset - frame_size); */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* stw Ra, @(fp, r0) -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_STW_FP_R0) */
/* OBSOLETE     { */
/* OBSOLETE       n = EXTRACT_RA (op); */
/* OBSOLETE       fsr->regs[n] = (-frame_size); */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* or  fp,0,sp -- observed */ */
/* OBSOLETE   if ((op == OP_OR_FP_R0_SP) || */
/* OBSOLETE       (op == OP_OR_FP_SP_R0) || */
/* OBSOLETE       (op == OP_OR_FP_IMM0_SP)) */
/* OBSOLETE     { */
/* OBSOLETE       uses_frame = 1; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* nop */ */
/* OBSOLETE   if ((op & OP_MASK_OPCODE) == OP_NOP) */
/* OBSOLETE     return 1; */
/* OBSOLETE  */
/* OBSOLETE   /* stw Ra,@(r22+,r0) -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_STW_R22P_R0) */
/* OBSOLETE     { */
/* OBSOLETE       n = EXTRACT_RA (op); */
/* OBSOLETE       fsr->regs[n] = next_addr; */
/* OBSOLETE       next_addr += 4; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE #if 0				/* subsumed in pattern above */ */
/* OBSOLETE   /* stw fp,@(r22+,r0) -- observed */ */
/* OBSOLETE   if (op == OP_STW_FP_R22P_R0) */
/* OBSOLETE     { */
/* OBSOLETE       fsr->regs[FP_REGNUM] = next_addr;		/* XXX */ */
/* OBSOLETE       next_addr += 4; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* stw r62,@(r22+,r0) -- observed */ */
/* OBSOLETE   if (op == OP_STW_LR_R22P_R0) */
/* OBSOLETE     { */
/* OBSOLETE       fsr->regs[LR_REGNUM] = next_addr; */
/* OBSOLETE       next_addr += 4; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE #endif */
/* OBSOLETE   /* st2w Ra,@(r22+,r0) -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_ST2W_R22P_R0) */
/* OBSOLETE     { */
/* OBSOLETE       n = EXTRACT_RA (op); */
/* OBSOLETE       fsr->regs[n] = next_addr; */
/* OBSOLETE       fsr->regs[n + 1] = next_addr + 4; */
/* OBSOLETE       next_addr += 8; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* stw  rn, @(sp-) */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_RA) == OP_STW_SPM) */
/* OBSOLETE     { */
/* OBSOLETE       n = EXTRACT_RA (op); */
/* OBSOLETE       fsr->regs[n] = next_addr; */
/* OBSOLETE       next_addr -= 4; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* st2w  Ra, @(sp-) */ */
/* OBSOLETE   else if ((op & OP_MASK_ALL_BUT_RA) == OP_ST2W_SPM) */
/* OBSOLETE     { */
/* OBSOLETE       n = EXTRACT_RA (op); */
/* OBSOLETE       fsr->regs[n] = next_addr; */
/* OBSOLETE       fsr->regs[n + 1] = next_addr + 4; */
/* OBSOLETE       next_addr -= 8; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* sub  sp,sp,imm */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_IMM) == OP_SUB_SP_IMM) */
/* OBSOLETE     { */
/* OBSOLETE       offset = EXTRACT_IMM6 (op); */
/* OBSOLETE       frame_size += -offset; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* st  rn, @(sp,0) -- observed */ */
/* OBSOLETE   if (((op & OP_MASK_ALL_BUT_RA) == OP_STW_SP_R0) || */
/* OBSOLETE       ((op & OP_MASK_ALL_BUT_RA) == OP_STW_SP_IMM0)) */
/* OBSOLETE     { */
/* OBSOLETE       n = EXTRACT_RA (op); */
/* OBSOLETE       fsr->regs[n] = (-frame_size); */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* st2w  rn, @(sp,0) */ */
/* OBSOLETE   if (((op & OP_MASK_ALL_BUT_RA) == OP_ST2W_SP_R0) || */
/* OBSOLETE       ((op & OP_MASK_ALL_BUT_RA) == OP_ST2W_SP_IMM0)) */
/* OBSOLETE     { */
/* OBSOLETE       n = EXTRACT_RA (op); */
/* OBSOLETE       fsr->regs[n] = (-frame_size); */
/* OBSOLETE       fsr->regs[n + 1] = (-frame_size) + 4; */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* stw fp,@(sp,imm) -- observed */ */
/* OBSOLETE   if ((op & OP_MASK_ALL_BUT_IMM) == OP_STW_FP_SP_IMM) */
/* OBSOLETE     { */
/* OBSOLETE       offset = EXTRACT_IMM6 (op); */
/* OBSOLETE       fsr->regs[FP_REGNUM] = (offset - frame_size); */
/* OBSOLETE       return 1; */
/* OBSOLETE     } */
/* OBSOLETE   return 0; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Put here the code to store, into a struct frame_saved_regs, the */
/* OBSOLETE    addresses of the saved registers of frame described by FRAME_INFO. */
/* OBSOLETE    This includes special registers such as pc and fp saved in special */
/* OBSOLETE    ways in the stack frame.  sp is even more special: the address we */
/* OBSOLETE    return for it IS the sp for the next frame. */ */
/* OBSOLETE void */
/* OBSOLETE d30v_frame_find_saved_regs (struct frame_info *fi, struct frame_saved_regs *fsr) */
/* OBSOLETE { */
/* OBSOLETE   CORE_ADDR fp, pc; */
/* OBSOLETE   unsigned long opl, opr; */
/* OBSOLETE   unsigned long op1, op2; */
/* OBSOLETE   unsigned long fm0, fm1; */
/* OBSOLETE   int i; */
/* OBSOLETE  */
/* OBSOLETE   fp = fi->frame; */
/* OBSOLETE   memset (fsr, 0, sizeof (*fsr)); */
/* OBSOLETE   next_addr = 0; */
/* OBSOLETE   frame_size = 0; */
/* OBSOLETE   end_of_stack = 0; */
/* OBSOLETE  */
/* OBSOLETE   uses_frame = 0; */
/* OBSOLETE  */
/* OBSOLETE   d30v_frame_find_saved_regs_offsets (fi, fsr); */
/* OBSOLETE  */
/* OBSOLETE   fi->size = frame_size; */
/* OBSOLETE  */
/* OBSOLETE   if (!fp) */
/* OBSOLETE     fp = read_register (SP_REGNUM); */
/* OBSOLETE  */
/* OBSOLETE   for (i = 0; i < NUM_REGS - 1; i++) */
/* OBSOLETE     if (fsr->regs[i]) */
/* OBSOLETE       { */
/* OBSOLETE 	fsr->regs[i] = fsr->regs[i] + fp + frame_size; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE   if (fsr->regs[LR_REGNUM]) */
/* OBSOLETE     fi->return_pc = read_memory_unsigned_integer (fsr->regs[LR_REGNUM], 4); */
/* OBSOLETE   else */
/* OBSOLETE     fi->return_pc = read_register (LR_REGNUM); */
/* OBSOLETE  */
/* OBSOLETE   /* the SP is not normally (ever?) saved, but check anyway */ */
/* OBSOLETE   if (!fsr->regs[SP_REGNUM]) */
/* OBSOLETE     { */
/* OBSOLETE       /* if the FP was saved, that means the current FP is valid, */ */
/* OBSOLETE       /* otherwise, it isn't being used, so we use the SP instead */ */
/* OBSOLETE       if (uses_frame) */
/* OBSOLETE 	fsr->regs[SP_REGNUM] = read_register (FP_REGNUM) + fi->size; */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE 	  fsr->regs[SP_REGNUM] = fp + fi->size; */
/* OBSOLETE 	  fi->frameless = 1; */
/* OBSOLETE 	  fsr->regs[FP_REGNUM] = 0; */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_frame_find_saved_regs_offsets (struct frame_info *fi, */
/* OBSOLETE 				    struct frame_saved_regs *fsr) */
/* OBSOLETE { */
/* OBSOLETE   CORE_ADDR fp, pc; */
/* OBSOLETE   unsigned long opl, opr; */
/* OBSOLETE   unsigned long op1, op2; */
/* OBSOLETE   unsigned long fm0, fm1; */
/* OBSOLETE   int i; */
/* OBSOLETE  */
/* OBSOLETE   fp = fi->frame; */
/* OBSOLETE   memset (fsr, 0, sizeof (*fsr)); */
/* OBSOLETE   next_addr = 0; */
/* OBSOLETE   frame_size = 0; */
/* OBSOLETE   end_of_stack = 0; */
/* OBSOLETE  */
/* OBSOLETE   pc = get_pc_function_start (fi->pc); */
/* OBSOLETE  */
/* OBSOLETE   uses_frame = 0; */
/* OBSOLETE   while (pc < fi->pc) */
/* OBSOLETE     { */
/* OBSOLETE       opl = (unsigned long) read_memory_integer (pc, 4); */
/* OBSOLETE       opr = (unsigned long) read_memory_integer (pc + 4, 4); */
/* OBSOLETE  */
/* OBSOLETE       fm0 = (opl & OP_MASK_FM_BIT); */
/* OBSOLETE       fm1 = (opr & OP_MASK_FM_BIT); */
/* OBSOLETE  */
/* OBSOLETE       opl = (opl & OP_MASK_SUB_INST); */
/* OBSOLETE       opr = (opr & OP_MASK_SUB_INST); */
/* OBSOLETE  */
/* OBSOLETE       if (fm0 && fm1) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  /* long instruction */ */
/* OBSOLETE 	  if ((opl & OP_MASK_ALL_BUT_IMM) == OP_ADD_SP_IMM) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      /* add sp,sp,n */ */
/* OBSOLETE 	      long offset = EXTRACT_IMM32 (opl, opr); */
/* OBSOLETE 	      frame_size += -offset; */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else if ((opl & OP_MASK_ALL_BUT_IMM) == OP_ADD_R22_SP_IMM) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      /* add r22,sp,offset */ */
/* OBSOLETE 	      long offset = EXTRACT_IMM32 (opl, opr); */
/* OBSOLETE 	      next_addr = (offset - frame_size); */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else if ((opl & OP_MASK_OP_AND_RB) == OP_STW_SP_IMM) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      /* st Ra, @(sp,imm) */ */
/* OBSOLETE 	      long offset = EXTRACT_IMM32 (opl, opr); */
/* OBSOLETE 	      short n = EXTRACT_RA (opl); */
/* OBSOLETE 	      fsr->regs[n] = (offset - frame_size); */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else if ((opl & OP_MASK_OP_AND_RB) == OP_ST2W_SP_IMM) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      /* st2w Ra, @(sp,offset) */ */
/* OBSOLETE 	      long offset = EXTRACT_IMM32 (opl, opr); */
/* OBSOLETE 	      short n = EXTRACT_RA (opl); */
/* OBSOLETE 	      fsr->regs[n] = (offset - frame_size); */
/* OBSOLETE 	      fsr->regs[n + 1] = (offset - frame_size) + 4; */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else if ((opl & OP_MASK_ALL_BUT_IMM) == OP_OR_SP_R0_IMM) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      end_of_stack = 1; */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else */
/* OBSOLETE 	    break; */
/* OBSOLETE 	} */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE 	  /* short instructions */ */
/* OBSOLETE 	  if (fm0 && !fm1) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      op2 = opl; */
/* OBSOLETE 	      op1 = opr; */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else */
/* OBSOLETE 	    { */
/* OBSOLETE 	      op1 = opl; */
/* OBSOLETE 	      op2 = opr; */
/* OBSOLETE 	    } */
/* OBSOLETE 	  if (!prologue_find_regs (op1, fsr, pc) || !prologue_find_regs (op2, fsr, pc)) */
/* OBSOLETE 	    break; */
/* OBSOLETE 	} */
/* OBSOLETE       pc += 8; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE #if 0 */
/* OBSOLETE   fi->size = frame_size; */
/* OBSOLETE  */
/* OBSOLETE   if (!fp) */
/* OBSOLETE     fp = read_register (SP_REGNUM); */
/* OBSOLETE  */
/* OBSOLETE   for (i = 0; i < NUM_REGS - 1; i++) */
/* OBSOLETE     if (fsr->regs[i]) */
/* OBSOLETE       { */
/* OBSOLETE 	fsr->regs[i] = fsr->regs[i] + fp + frame_size; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE   if (fsr->regs[LR_REGNUM]) */
/* OBSOLETE     fi->return_pc = read_memory_unsigned_integer (fsr->regs[LR_REGNUM], 4); */
/* OBSOLETE   else */
/* OBSOLETE     fi->return_pc = read_register (LR_REGNUM); */
/* OBSOLETE  */
/* OBSOLETE   /* the SP is not normally (ever?) saved, but check anyway */ */
/* OBSOLETE   if (!fsr->regs[SP_REGNUM]) */
/* OBSOLETE     { */
/* OBSOLETE       /* if the FP was saved, that means the current FP is valid, */ */
/* OBSOLETE       /* otherwise, it isn't being used, so we use the SP instead */ */
/* OBSOLETE       if (uses_frame) */
/* OBSOLETE 	fsr->regs[SP_REGNUM] = read_register (FP_REGNUM) + fi->size; */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE 	  fsr->regs[SP_REGNUM] = fp + fi->size; */
/* OBSOLETE 	  fi->frameless = 1; */
/* OBSOLETE 	  fsr->regs[FP_REGNUM] = 0; */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE #endif */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_init_extra_frame_info (int fromleaf, struct frame_info *fi) */
/* OBSOLETE { */
/* OBSOLETE   struct frame_saved_regs dummy; */
/* OBSOLETE  */
/* OBSOLETE   if (fi->next && (fi->pc == 0)) */
/* OBSOLETE     fi->pc = fi->next->return_pc; */
/* OBSOLETE  */
/* OBSOLETE   d30v_frame_find_saved_regs_offsets (fi, &dummy); */
/* OBSOLETE  */
/* OBSOLETE   if (uses_frame == 0) */
/* OBSOLETE     fi->frameless = 1; */
/* OBSOLETE   else */
/* OBSOLETE     fi->frameless = 0; */
/* OBSOLETE  */
/* OBSOLETE   if ((fi->next == 0) && (uses_frame == 0)) */
/* OBSOLETE     /* innermost frame and it's "frameless", */
/* OBSOLETE        so the fi->frame field is wrong, fix it! */ */
/* OBSOLETE     fi->frame = read_sp (); */
/* OBSOLETE  */
/* OBSOLETE   if (dummy.regs[LR_REGNUM]) */
/* OBSOLETE     { */
/* OBSOLETE       /* it was saved, grab it! */ */
/* OBSOLETE       dummy.regs[LR_REGNUM] += (fi->frame + frame_size); */
/* OBSOLETE       fi->return_pc = read_memory_unsigned_integer (dummy.regs[LR_REGNUM], 4); */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     fi->return_pc = read_register (LR_REGNUM); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_init_frame_pc (int fromleaf, struct frame_info *prev) */
/* OBSOLETE { */
/* OBSOLETE   /* default value, put here so we can breakpoint on it and */
/* OBSOLETE      see if the default value is really the right thing to use */ */
/* OBSOLETE   prev->pc = (fromleaf ? SAVED_PC_AFTER_CALL (prev->next) : \ */
/* OBSOLETE 	      prev->next ? FRAME_SAVED_PC (prev->next) : read_pc ()); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static void d30v_print_register (int regnum, int tabular); */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE d30v_print_register (int regnum, int tabular) */
/* OBSOLETE { */
/* OBSOLETE   if (regnum < A0_REGNUM) */
/* OBSOLETE     { */
/* OBSOLETE       if (tabular) */
/* OBSOLETE 	printf_filtered ("%08lx", (long) read_register (regnum)); */
/* OBSOLETE       else */
/* OBSOLETE 	printf_filtered ("0x%lx	%ld", */
/* OBSOLETE 			 (long) read_register (regnum), */
/* OBSOLETE 			 (long) read_register (regnum)); */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     { */
/* OBSOLETE       char regbuf[MAX_REGISTER_RAW_SIZE]; */
/* OBSOLETE  */
/* OBSOLETE       frame_register_read (selected_frame, regnum, regbuf); */
/* OBSOLETE  */
/* OBSOLETE       val_print (REGISTER_VIRTUAL_TYPE (regnum), regbuf, 0, 0, */
/* OBSOLETE 		 gdb_stdout, 'x', 1, 0, Val_pretty_default); */
/* OBSOLETE  */
/* OBSOLETE       if (!tabular) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  printf_filtered ("	"); */
/* OBSOLETE 	  val_print (REGISTER_VIRTUAL_TYPE (regnum), regbuf, 0, 0, */
/* OBSOLETE 		     gdb_stdout, 'd', 1, 0, Val_pretty_default); */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE d30v_print_flags (void) */
/* OBSOLETE { */
/* OBSOLETE   long psw = read_register (PSW_REGNUM); */
/* OBSOLETE   printf_filtered ("flags #1"); */
/* OBSOLETE   printf_filtered ("   (sm) %d", (psw & PSW_SM) != 0); */
/* OBSOLETE   printf_filtered ("   (ea) %d", (psw & PSW_EA) != 0); */
/* OBSOLETE   printf_filtered ("   (db) %d", (psw & PSW_DB) != 0); */
/* OBSOLETE   printf_filtered ("   (ds) %d", (psw & PSW_DS) != 0); */
/* OBSOLETE   printf_filtered ("   (ie) %d", (psw & PSW_IE) != 0); */
/* OBSOLETE   printf_filtered ("   (rp) %d", (psw & PSW_RP) != 0); */
/* OBSOLETE   printf_filtered ("   (md) %d\n", (psw & PSW_MD) != 0); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("flags #2"); */
/* OBSOLETE   printf_filtered ("   (f0) %d", (psw & PSW_F0) != 0); */
/* OBSOLETE   printf_filtered ("   (f1) %d", (psw & PSW_F1) != 0); */
/* OBSOLETE   printf_filtered ("   (f2) %d", (psw & PSW_F2) != 0); */
/* OBSOLETE   printf_filtered ("   (f3) %d", (psw & PSW_F3) != 0); */
/* OBSOLETE   printf_filtered ("    (s) %d", (psw & PSW_S) != 0); */
/* OBSOLETE   printf_filtered ("    (v) %d", (psw & PSW_V) != 0); */
/* OBSOLETE   printf_filtered ("   (va) %d", (psw & PSW_VA) != 0); */
/* OBSOLETE   printf_filtered ("    (c) %d\n", (psw & PSW_C) != 0); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE print_flags_command (char *args, int from_tty) */
/* OBSOLETE { */
/* OBSOLETE   d30v_print_flags (); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_do_registers_info (int regnum, int fpregs) */
/* OBSOLETE { */
/* OBSOLETE   long long num1, num2; */
/* OBSOLETE   long psw; */
/* OBSOLETE  */
/* OBSOLETE   if (regnum != -1) */
/* OBSOLETE     { */
/* OBSOLETE       if (REGISTER_NAME (0) == NULL || REGISTER_NAME (0)[0] == '\000') */
/* OBSOLETE 	return; */
/* OBSOLETE  */
/* OBSOLETE       printf_filtered ("%s ", REGISTER_NAME (regnum)); */
/* OBSOLETE       d30v_print_register (regnum, 0); */
/* OBSOLETE  */
/* OBSOLETE       printf_filtered ("\n"); */
/* OBSOLETE       return; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* Have to print all the registers.  Format them nicely.  */ */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("PC="); */
/* OBSOLETE   print_address (read_pc (), gdb_stdout); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" PSW="); */
/* OBSOLETE   d30v_print_register (PSW_REGNUM, 1); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" BPC="); */
/* OBSOLETE   print_address (read_register (BPC_REGNUM), gdb_stdout); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" BPSW="); */
/* OBSOLETE   d30v_print_register (BPSW_REGNUM, 1); */
/* OBSOLETE   printf_filtered ("\n"); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("DPC="); */
/* OBSOLETE   print_address (read_register (DPC_REGNUM), gdb_stdout); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" DPSW="); */
/* OBSOLETE   d30v_print_register (DPSW_REGNUM, 1); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" IBA="); */
/* OBSOLETE   print_address (read_register (IBA_REGNUM), gdb_stdout); */
/* OBSOLETE   printf_filtered ("\n"); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("RPT_C="); */
/* OBSOLETE   d30v_print_register (RPT_C_REGNUM, 1); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" RPT_S="); */
/* OBSOLETE   print_address (read_register (RPT_S_REGNUM), gdb_stdout); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" RPT_E="); */
/* OBSOLETE   print_address (read_register (RPT_E_REGNUM), gdb_stdout); */
/* OBSOLETE   printf_filtered ("\n"); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("MOD_S="); */
/* OBSOLETE   print_address (read_register (MOD_S_REGNUM), gdb_stdout); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" MOD_E="); */
/* OBSOLETE   print_address (read_register (MOD_E_REGNUM), gdb_stdout); */
/* OBSOLETE   printf_filtered ("\n"); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("EIT_VB="); */
/* OBSOLETE   print_address (read_register (EIT_VB_REGNUM), gdb_stdout); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" INT_S="); */
/* OBSOLETE   d30v_print_register (INT_S_REGNUM, 1); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered (" INT_M="); */
/* OBSOLETE   d30v_print_register (INT_M_REGNUM, 1); */
/* OBSOLETE   printf_filtered ("\n"); */
/* OBSOLETE  */
/* OBSOLETE   d30v_print_flags (); */
/* OBSOLETE   for (regnum = 0; regnum <= 63;) */
/* OBSOLETE     { */
/* OBSOLETE       int i; */
/* OBSOLETE  */
/* OBSOLETE       printf_filtered ("R%d-R%d ", regnum, regnum + 7); */
/* OBSOLETE       if (regnum < 10) */
/* OBSOLETE 	printf_filtered (" "); */
/* OBSOLETE       if (regnum + 7 < 10) */
/* OBSOLETE 	printf_filtered (" "); */
/* OBSOLETE  */
/* OBSOLETE       for (i = 0; i < 8; i++) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  printf_filtered (" "); */
/* OBSOLETE 	  d30v_print_register (regnum++, 1); */
/* OBSOLETE 	} */
/* OBSOLETE  */
/* OBSOLETE       printf_filtered ("\n"); */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("A0-A1    "); */
/* OBSOLETE  */
/* OBSOLETE   d30v_print_register (A0_REGNUM, 1); */
/* OBSOLETE   printf_filtered ("    "); */
/* OBSOLETE   d30v_print_register (A1_REGNUM, 1); */
/* OBSOLETE   printf_filtered ("\n"); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR */
/* OBSOLETE d30v_fix_call_dummy (char *dummyname, CORE_ADDR start_sp, CORE_ADDR fun, */
/* OBSOLETE 		     int nargs, struct value **args, */
/* OBSOLETE 		     struct type *type, int gcc_p) */
/* OBSOLETE { */
/* OBSOLETE   int regnum; */
/* OBSOLETE   CORE_ADDR sp; */
/* OBSOLETE   char buffer[MAX_REGISTER_RAW_SIZE]; */
/* OBSOLETE   struct frame_info *frame = get_current_frame (); */
/* OBSOLETE   frame->dummy = start_sp; */
/* OBSOLETE   /*start_sp |= DMEM_START; */ */
/* OBSOLETE  */
/* OBSOLETE   sp = start_sp; */
/* OBSOLETE   for (regnum = 0; regnum < NUM_REGS; regnum++) */
/* OBSOLETE     { */
/* OBSOLETE       sp -= REGISTER_RAW_SIZE (regnum); */
/* OBSOLETE       store_address (buffer, REGISTER_RAW_SIZE (regnum), read_register (regnum)); */
/* OBSOLETE       write_memory (sp, buffer, REGISTER_RAW_SIZE (regnum)); */
/* OBSOLETE     } */
/* OBSOLETE   write_register (SP_REGNUM, (LONGEST) sp); */
/* OBSOLETE   /* now we need to load LR with the return address */ */
/* OBSOLETE   write_register (LR_REGNUM, (LONGEST) d30v_call_dummy_address ()); */
/* OBSOLETE   return sp; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE d30v_pop_dummy_frame (struct frame_info *fi) */
/* OBSOLETE { */
/* OBSOLETE   CORE_ADDR sp = fi->dummy; */
/* OBSOLETE   int regnum; */
/* OBSOLETE  */
/* OBSOLETE   for (regnum = 0; regnum < NUM_REGS; regnum++) */
/* OBSOLETE     { */
/* OBSOLETE       sp -= REGISTER_RAW_SIZE (regnum); */
/* OBSOLETE       write_register (regnum, read_memory_unsigned_integer (sp, REGISTER_RAW_SIZE (regnum))); */
/* OBSOLETE     } */
/* OBSOLETE   flush_cached_frames ();	/* needed? */ */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR */
/* OBSOLETE d30v_push_arguments (int nargs, struct value **args, CORE_ADDR sp, */
/* OBSOLETE 		     int struct_return, CORE_ADDR struct_addr) */
/* OBSOLETE { */
/* OBSOLETE   int i, len, index = 0, regnum = 2; */
/* OBSOLETE   char buffer[4], *contents; */
/* OBSOLETE   LONGEST val; */
/* OBSOLETE   CORE_ADDR ptrs[10]; */
/* OBSOLETE  */
/* OBSOLETE #if 0 */
/* OBSOLETE   /* Pass 1. Put all large args on stack */ */
/* OBSOLETE   for (i = 0; i < nargs; i++) */
/* OBSOLETE     { */
/* OBSOLETE       struct value *arg = args[i]; */
/* OBSOLETE       struct type *arg_type = check_typedef (VALUE_TYPE (arg)); */
/* OBSOLETE       len = TYPE_LENGTH (arg_type); */
/* OBSOLETE       contents = VALUE_CONTENTS (arg); */
/* OBSOLETE       val = extract_signed_integer (contents, len); */
/* OBSOLETE       if (len > 4) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  /* put on stack and pass pointers */ */
/* OBSOLETE 	  sp -= len; */
/* OBSOLETE 	  write_memory (sp, contents, len); */
/* OBSOLETE 	  ptrs[index++] = sp; */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE #endif */
/* OBSOLETE   index = 0; */
/* OBSOLETE  */
/* OBSOLETE   for (i = 0; i < nargs; i++) */
/* OBSOLETE     { */
/* OBSOLETE       struct value *arg = args[i]; */
/* OBSOLETE       struct type *arg_type = check_typedef (VALUE_TYPE (arg)); */
/* OBSOLETE       len = TYPE_LENGTH (arg_type); */
/* OBSOLETE       contents = VALUE_CONTENTS (arg); */
/* OBSOLETE       if (len > 4) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  /* we need multiple registers */ */
/* OBSOLETE 	  int ndx; */
/* OBSOLETE  */
/* OBSOLETE 	  for (ndx = 0; len > 0; ndx += 8, len -= 8) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      if (regnum & 1) */
/* OBSOLETE 		regnum++;	/* all args > 4 bytes start in even register */ */
/* OBSOLETE  */
/* OBSOLETE 	      if (regnum < 18) */
/* OBSOLETE 		{ */
/* OBSOLETE 		  val = extract_signed_integer (&contents[ndx], 4); */
/* OBSOLETE 		  write_register (regnum++, val); */
/* OBSOLETE  */
/* OBSOLETE 		  if (len >= 8) */
/* OBSOLETE 		    val = extract_signed_integer (&contents[ndx + 4], 4); */
/* OBSOLETE 		  else */
/* OBSOLETE 		    val = extract_signed_integer (&contents[ndx + 4], len - 4); */
/* OBSOLETE 		  write_register (regnum++, val); */
/* OBSOLETE 		} */
/* OBSOLETE 	      else */
/* OBSOLETE 		{ */
/* OBSOLETE 		  /* no more registers available.  put it on the stack */ */
/* OBSOLETE  */
/* OBSOLETE 		  /* all args > 4 bytes are padded to a multiple of 8 bytes */
/* OBSOLETE 		     and start on an 8 byte boundary */ */
/* OBSOLETE 		  if (sp & 7) */
/* OBSOLETE 		    sp -= (sp & 7);	/* align it */ */
/* OBSOLETE  */
/* OBSOLETE 		  sp -= ((len + 7) & ~7);	/* allocate space */ */
/* OBSOLETE 		  write_memory (sp, &contents[ndx], len); */
/* OBSOLETE 		  break; */
/* OBSOLETE 		} */
/* OBSOLETE 	    } */
/* OBSOLETE 	} */
/* OBSOLETE       else */
/* OBSOLETE 	{ */
/* OBSOLETE 	  if (regnum < 18) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      val = extract_signed_integer (contents, len); */
/* OBSOLETE 	      write_register (regnum++, val); */
/* OBSOLETE 	    } */
/* OBSOLETE 	  else */
/* OBSOLETE 	    { */
/* OBSOLETE 	      /* all args are padded to a multiple of 4 bytes (at least) */ */
/* OBSOLETE 	      sp -= ((len + 3) & ~3); */
/* OBSOLETE 	      write_memory (sp, contents, len); */
/* OBSOLETE 	    } */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE   if (sp & 7) */
/* OBSOLETE     /* stack pointer is not on an 8 byte boundary -- align it */ */
/* OBSOLETE     sp -= (sp & 7); */
/* OBSOLETE   return sp; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* pick an out-of-the-way place to set the return value */ */
/* OBSOLETE /* for an inferior function call.  The link register is set to this  */ */
/* OBSOLETE /* value and a momentary breakpoint is set there.  When the breakpoint */ */
/* OBSOLETE /* is hit, the dummy frame is popped and the previous environment is */ */
/* OBSOLETE /* restored. */ */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR */
/* OBSOLETE d30v_call_dummy_address (void) */
/* OBSOLETE { */
/* OBSOLETE   CORE_ADDR entry; */
/* OBSOLETE   struct minimal_symbol *sym; */
/* OBSOLETE  */
/* OBSOLETE   entry = entry_point_address (); */
/* OBSOLETE  */
/* OBSOLETE   if (entry != 0) */
/* OBSOLETE     return entry; */
/* OBSOLETE  */
/* OBSOLETE   sym = lookup_minimal_symbol ("_start", NULL, symfile_objfile); */
/* OBSOLETE  */
/* OBSOLETE   if (!sym || MSYMBOL_TYPE (sym) != mst_text) */
/* OBSOLETE     return 0; */
/* OBSOLETE   else */
/* OBSOLETE     return SYMBOL_VALUE_ADDRESS (sym); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Given a return value in `regbuf' with a type `valtype',  */
/* OBSOLETE    extract and copy its value into `valbuf'.  */ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_extract_return_value (struct type *valtype, char regbuf[REGISTER_BYTES], */
/* OBSOLETE 			   char *valbuf) */
/* OBSOLETE { */
/* OBSOLETE   memcpy (valbuf, regbuf + REGISTER_BYTE (2), TYPE_LENGTH (valtype)); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* The following code implements access to, and display of, the D30V's */
/* OBSOLETE    instruction trace buffer.  The buffer consists of 64K or more */
/* OBSOLETE    4-byte words of data, of which each words includes an 8-bit count, */
/* OBSOLETE    an 8-bit segment number, and a 16-bit instruction address. */
/* OBSOLETE  */
/* OBSOLETE    In theory, the trace buffer is continuously capturing instruction */
/* OBSOLETE    data that the CPU presents on its "debug bus", but in practice, the */
/* OBSOLETE    ROMified GDB stub only enables tracing when it continues or steps */
/* OBSOLETE    the program, and stops tracing when the program stops; so it */
/* OBSOLETE    actually works for GDB to read the buffer counter out of memory and */
/* OBSOLETE    then read each trace word.  The counter records where the tracing */
/* OBSOLETE    stops, but there is no record of where it started, so we remember */
/* OBSOLETE    the PC when we resumed and then search backwards in the trace */
/* OBSOLETE    buffer for a word that includes that address.  This is not perfect, */
/* OBSOLETE    because you will miss trace data if the resumption PC is the target */
/* OBSOLETE    of a branch.  (The value of the buffer counter is semi-random, any */
/* OBSOLETE    trace data from a previous program stop is gone.)  */ */
/* OBSOLETE  */
/* OBSOLETE /* The address of the last word recorded in the trace buffer.  */ */
/* OBSOLETE  */
/* OBSOLETE #define DBBC_ADDR (0xd80000) */
/* OBSOLETE  */
/* OBSOLETE /* The base of the trace buffer, at least for the "Board_0".  */ */
/* OBSOLETE  */
/* OBSOLETE #define TRACE_BUFFER_BASE (0xf40000) */
/* OBSOLETE  */
/* OBSOLETE static void trace_command (char *, int); */
/* OBSOLETE  */
/* OBSOLETE static void untrace_command (char *, int); */
/* OBSOLETE  */
/* OBSOLETE static void trace_info (char *, int); */
/* OBSOLETE  */
/* OBSOLETE static void tdisassemble_command (char *, int); */
/* OBSOLETE  */
/* OBSOLETE static void display_trace (int, int); */
/* OBSOLETE  */
/* OBSOLETE /* True when instruction traces are being collected.  */ */
/* OBSOLETE  */
/* OBSOLETE static int tracing; */
/* OBSOLETE  */
/* OBSOLETE /* Remembered PC.  */ */
/* OBSOLETE  */
/* OBSOLETE static CORE_ADDR last_pc; */
/* OBSOLETE  */
/* OBSOLETE /* True when trace output should be displayed whenever program stops.  */ */
/* OBSOLETE  */
/* OBSOLETE static int trace_display; */
/* OBSOLETE  */
/* OBSOLETE /* True when trace listing should include source lines.  */ */
/* OBSOLETE  */
/* OBSOLETE static int default_trace_show_source = 1; */
/* OBSOLETE  */
/* OBSOLETE struct trace_buffer */
/* OBSOLETE   { */
/* OBSOLETE     int size; */
/* OBSOLETE     short *counts; */
/* OBSOLETE     CORE_ADDR *addrs; */
/* OBSOLETE   } */
/* OBSOLETE trace_data; */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE trace_command (char *args, int from_tty) */
/* OBSOLETE { */
/* OBSOLETE   /* Clear the host-side trace buffer, allocating space if needed.  */ */
/* OBSOLETE   trace_data.size = 0; */
/* OBSOLETE   if (trace_data.counts == NULL) */
/* OBSOLETE     trace_data.counts = (short *) xmalloc (65536 * sizeof (short)); */
/* OBSOLETE   if (trace_data.addrs == NULL) */
/* OBSOLETE     trace_data.addrs = (CORE_ADDR *) xmalloc (65536 * sizeof (CORE_ADDR)); */
/* OBSOLETE  */
/* OBSOLETE   tracing = 1; */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("Tracing is now on.\n"); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE untrace_command (char *args, int from_tty) */
/* OBSOLETE { */
/* OBSOLETE   tracing = 0; */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("Tracing is now off.\n"); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE trace_info (char *args, int from_tty) */
/* OBSOLETE { */
/* OBSOLETE   int i; */
/* OBSOLETE  */
/* OBSOLETE   if (trace_data.size) */
/* OBSOLETE     { */
/* OBSOLETE       printf_filtered ("%d entries in trace buffer:\n", trace_data.size); */
/* OBSOLETE  */
/* OBSOLETE       for (i = 0; i < trace_data.size; ++i) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  printf_filtered ("%d: %d instruction%s at 0x%s\n", */
/* OBSOLETE 			   i, trace_data.counts[i], */
/* OBSOLETE 			   (trace_data.counts[i] == 1 ? "" : "s"), */
/* OBSOLETE 			   paddr_nz (trace_data.addrs[i])); */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     printf_filtered ("No entries in trace buffer.\n"); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("Tracing is currently %s.\n", (tracing ? "on" : "off")); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Print the instruction at address MEMADDR in debugged memory, */
/* OBSOLETE    on STREAM.  Returns length of the instruction, in bytes.  */ */
/* OBSOLETE  */
/* OBSOLETE static int */
/* OBSOLETE print_insn (CORE_ADDR memaddr, struct ui_file *stream) */
/* OBSOLETE { */
/* OBSOLETE   /* If there's no disassembler, something is very wrong.  */ */
/* OBSOLETE   if (tm_print_insn == NULL) */
/* OBSOLETE     internal_error (__FILE__, __LINE__, */
/* OBSOLETE 		    "print_insn: no disassembler"); */
/* OBSOLETE  */
/* OBSOLETE   if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG) */
/* OBSOLETE     tm_print_insn_info.endian = BFD_ENDIAN_BIG; */
/* OBSOLETE   else */
/* OBSOLETE     tm_print_insn_info.endian = BFD_ENDIAN_LITTLE; */
/* OBSOLETE   return TARGET_PRINT_INSN (memaddr, &tm_print_insn_info); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_eva_prepare_to_trace (void) */
/* OBSOLETE { */
/* OBSOLETE   if (!tracing) */
/* OBSOLETE     return; */
/* OBSOLETE  */
/* OBSOLETE   last_pc = read_register (PC_REGNUM); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Collect trace data from the target board and format it into a form */
/* OBSOLETE    more useful for display.  */ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE d30v_eva_get_trace_data (void) */
/* OBSOLETE { */
/* OBSOLETE   int count, i, j, oldsize; */
/* OBSOLETE   int trace_addr, trace_seg, trace_cnt, next_cnt; */
/* OBSOLETE   unsigned int last_trace, trace_word, next_word; */
/* OBSOLETE   unsigned int *tmpspace; */
/* OBSOLETE  */
/* OBSOLETE   if (!tracing) */
/* OBSOLETE     return; */
/* OBSOLETE  */
/* OBSOLETE   tmpspace = xmalloc (65536 * sizeof (unsigned int)); */
/* OBSOLETE  */
/* OBSOLETE   last_trace = read_memory_unsigned_integer (DBBC_ADDR, 2) << 2; */
/* OBSOLETE  */
/* OBSOLETE   /* Collect buffer contents from the target, stopping when we reach */
/* OBSOLETE      the word recorded when execution resumed.  */ */
/* OBSOLETE  */
/* OBSOLETE   count = 0; */
/* OBSOLETE   while (last_trace > 0) */
/* OBSOLETE     { */
/* OBSOLETE       QUIT; */
/* OBSOLETE       trace_word = */
/* OBSOLETE 	read_memory_unsigned_integer (TRACE_BUFFER_BASE + last_trace, 4); */
/* OBSOLETE       trace_addr = trace_word & 0xffff; */
/* OBSOLETE       last_trace -= 4; */
/* OBSOLETE       /* Ignore an apparently nonsensical entry.  */ */
/* OBSOLETE       if (trace_addr == 0xffd5) */
/* OBSOLETE 	continue; */
/* OBSOLETE       tmpspace[count++] = trace_word; */
/* OBSOLETE       if (trace_addr == last_pc) */
/* OBSOLETE 	break; */
/* OBSOLETE       if (count > 65535) */
/* OBSOLETE 	break; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* Move the data to the host-side trace buffer, adjusting counts to */
/* OBSOLETE      include the last instruction executed and transforming the address */
/* OBSOLETE      into something that GDB likes.  */ */
/* OBSOLETE  */
/* OBSOLETE   for (i = 0; i < count; ++i) */
/* OBSOLETE     { */
/* OBSOLETE       trace_word = tmpspace[i]; */
/* OBSOLETE       next_word = ((i == 0) ? 0 : tmpspace[i - 1]); */
/* OBSOLETE       trace_addr = trace_word & 0xffff; */
/* OBSOLETE       next_cnt = (next_word >> 24) & 0xff; */
/* OBSOLETE       j = trace_data.size + count - i - 1; */
/* OBSOLETE       trace_data.addrs[j] = (trace_addr << 2) + 0x1000000; */
/* OBSOLETE       trace_data.counts[j] = next_cnt + 1; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   oldsize = trace_data.size; */
/* OBSOLETE   trace_data.size += count; */
/* OBSOLETE  */
/* OBSOLETE   xfree (tmpspace); */
/* OBSOLETE  */
/* OBSOLETE   if (trace_display) */
/* OBSOLETE     display_trace (oldsize, trace_data.size); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE tdisassemble_command (char *arg, int from_tty) */
/* OBSOLETE { */
/* OBSOLETE   int i, count; */
/* OBSOLETE   CORE_ADDR low, high; */
/* OBSOLETE   char *space_index; */
/* OBSOLETE  */
/* OBSOLETE   if (!arg) */
/* OBSOLETE     { */
/* OBSOLETE       low = 0; */
/* OBSOLETE       high = trace_data.size; */
/* OBSOLETE     } */
/* OBSOLETE   else if (!(space_index = (char *) strchr (arg, ' '))) */
/* OBSOLETE     { */
/* OBSOLETE       low = parse_and_eval_address (arg); */
/* OBSOLETE       high = low + 5; */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     { */
/* OBSOLETE       /* Two arguments.  */ */
/* OBSOLETE       *space_index = '\0'; */
/* OBSOLETE       low = parse_and_eval_address (arg); */
/* OBSOLETE       high = parse_and_eval_address (space_index + 1); */
/* OBSOLETE       if (high < low) */
/* OBSOLETE 	high = low; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("Dump of trace from %s to %s:\n", */
/* OBSOLETE 		   paddr_u (low), */
/* OBSOLETE 		   paddr_u (high)); */
/* OBSOLETE  */
/* OBSOLETE   display_trace (low, high); */
/* OBSOLETE  */
/* OBSOLETE   printf_filtered ("End of trace dump.\n"); */
/* OBSOLETE   gdb_flush (gdb_stdout); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE display_trace (int low, int high) */
/* OBSOLETE { */
/* OBSOLETE   int i, count, trace_show_source, first, suppress; */
/* OBSOLETE   CORE_ADDR next_address; */
/* OBSOLETE  */
/* OBSOLETE   trace_show_source = default_trace_show_source; */
/* OBSOLETE   if (!have_full_symbols () && !have_partial_symbols ()) */
/* OBSOLETE     { */
/* OBSOLETE       trace_show_source = 0; */
/* OBSOLETE       printf_filtered ("No symbol table is loaded.  Use the \"file\" command.\n"); */
/* OBSOLETE       printf_filtered ("Trace will not display any source.\n"); */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   first = 1; */
/* OBSOLETE   suppress = 0; */
/* OBSOLETE   for (i = low; i < high; ++i) */
/* OBSOLETE     { */
/* OBSOLETE       next_address = trace_data.addrs[i]; */
/* OBSOLETE       count = trace_data.counts[i]; */
/* OBSOLETE       while (count-- > 0) */
/* OBSOLETE 	{ */
/* OBSOLETE 	  QUIT; */
/* OBSOLETE 	  if (trace_show_source) */
/* OBSOLETE 	    { */
/* OBSOLETE 	      struct symtab_and_line sal, sal_prev; */
/* OBSOLETE  */
/* OBSOLETE 	      sal_prev = find_pc_line (next_address - 4, 0); */
/* OBSOLETE 	      sal = find_pc_line (next_address, 0); */
/* OBSOLETE  */
/* OBSOLETE 	      if (sal.symtab) */
/* OBSOLETE 		{ */
/* OBSOLETE 		  if (first || sal.line != sal_prev.line) */
/* OBSOLETE 		    print_source_lines (sal.symtab, sal.line, sal.line + 1, 0); */
/* OBSOLETE 		  suppress = 0; */
/* OBSOLETE 		} */
/* OBSOLETE 	      else */
/* OBSOLETE 		{ */
/* OBSOLETE 		  if (!suppress) */
/* OBSOLETE 		    /* FIXME-32x64--assumes sal.pc fits in long.  */ */
/* OBSOLETE 		    printf_filtered ("No source file for address %s.\n", */
/* OBSOLETE 				 local_hex_string ((unsigned long) sal.pc)); */
/* OBSOLETE 		  suppress = 1; */
/* OBSOLETE 		} */
/* OBSOLETE 	    } */
/* OBSOLETE 	  first = 0; */
/* OBSOLETE 	  print_address (next_address, gdb_stdout); */
/* OBSOLETE 	  printf_filtered (":"); */
/* OBSOLETE 	  printf_filtered ("\t"); */
/* OBSOLETE 	  wrap_here ("    "); */
/* OBSOLETE 	  next_address = next_address + print_insn (next_address, gdb_stdout); */
/* OBSOLETE 	  printf_filtered ("\n"); */
/* OBSOLETE 	  gdb_flush (gdb_stdout); */
/* OBSOLETE 	} */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE extern void (*target_resume_hook) (void); */
/* OBSOLETE extern void (*target_wait_loop_hook) (void); */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE _initialize_d30v_tdep (void) */
/* OBSOLETE { */
/* OBSOLETE   tm_print_insn = print_insn_d30v; */
/* OBSOLETE  */
/* OBSOLETE   target_resume_hook = d30v_eva_prepare_to_trace; */
/* OBSOLETE   target_wait_loop_hook = d30v_eva_get_trace_data; */
/* OBSOLETE  */
/* OBSOLETE   add_info ("flags", print_flags_command, "Print d30v flags."); */
/* OBSOLETE  */
/* OBSOLETE   add_com ("trace", class_support, trace_command, */
/* OBSOLETE 	   "Enable tracing of instruction execution."); */
/* OBSOLETE  */
/* OBSOLETE   add_com ("untrace", class_support, untrace_command, */
/* OBSOLETE 	   "Disable tracing of instruction execution."); */
/* OBSOLETE  */
/* OBSOLETE   add_com ("tdisassemble", class_vars, tdisassemble_command, */
/* OBSOLETE 	   "Disassemble the trace buffer.\n\ */
/* OBSOLETE Two optional arguments specify a range of trace buffer entries\n\ */
/* OBSOLETE as reported by info trace (NOT addresses!)."); */
/* OBSOLETE  */
/* OBSOLETE   add_info ("trace", trace_info, */
/* OBSOLETE 	    "Display info about the trace data buffer."); */
/* OBSOLETE  */
/* OBSOLETE   add_show_from_set (add_set_cmd ("tracedisplay", no_class, */
/* OBSOLETE 				  var_integer, (char *) &trace_display, */
/* OBSOLETE 			     "Set automatic display of trace.\n", &setlist), */
/* OBSOLETE 		     &showlist); */
/* OBSOLETE   add_show_from_set (add_set_cmd ("tracesource", no_class, */
/* OBSOLETE 			   var_integer, (char *) &default_trace_show_source, */
/* OBSOLETE 		      "Set display of source code with trace.\n", &setlist), */
/* OBSOLETE 		     &showlist); */
/* OBSOLETE  */
/* OBSOLETE } */
