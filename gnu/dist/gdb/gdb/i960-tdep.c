// OBSOLETE /* Target-machine dependent code for the Intel 960
// OBSOLETE 
// OBSOLETE    Copyright 1991, 1992, 1993, 1994, 1995, 1996, 1998, 1999, 2000,
// OBSOLETE    2001, 2002 Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    Contributed by Intel Corporation.
// OBSOLETE    examine_prologue and other parts contributed by Wind River Systems.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "frame.h"
// OBSOLETE #include "floatformat.h"
// OBSOLETE #include "target.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE #include "gdb_string.h"
// OBSOLETE 
// OBSOLETE static CORE_ADDR next_insn (CORE_ADDR memaddr,
// OBSOLETE 			    unsigned int *pword1, unsigned int *pword2);
// OBSOLETE 
// OBSOLETE struct type *
// OBSOLETE i960_register_type (int regnum)
// OBSOLETE {
// OBSOLETE   if (regnum < FP0_REGNUM)
// OBSOLETE     return builtin_type_int32;
// OBSOLETE   else
// OBSOLETE     return builtin_type_i960_ext;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Does the specified function use the "struct returning" convention
// OBSOLETE    or the "value returning" convention?  The "value returning" convention
// OBSOLETE    almost invariably returns the entire value in registers.  The
// OBSOLETE    "struct returning" convention often returns the entire value in
// OBSOLETE    memory, and passes a pointer (out of or into the function) saying
// OBSOLETE    where the value (is or should go).
// OBSOLETE 
// OBSOLETE    Since this sometimes depends on whether it was compiled with GCC,
// OBSOLETE    this is also an argument.  This is used in call_function to build a
// OBSOLETE    stack, and in value_being_returned to print return values.
// OBSOLETE 
// OBSOLETE    On i960, a structure is returned in registers g0-g3, if it will fit.
// OBSOLETE    If it's more than 16 bytes long, g13 pointed to it on entry.  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE i960_use_struct_convention (int gcc_p, struct type *type)
// OBSOLETE {
// OBSOLETE   return (TYPE_LENGTH (type) > 16);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* gdb960 is always running on a non-960 host.  Check its characteristics.
// OBSOLETE    This routine must be called as part of gdb initialization.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE check_host (void)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   static struct typestruct
// OBSOLETE     {
// OBSOLETE       int hostsize;		/* Size of type on host         */
// OBSOLETE       int i960size;		/* Size of type on i960         */
// OBSOLETE       char *typename;		/* Name of type, for error msg  */
// OBSOLETE     }
// OBSOLETE   types[] =
// OBSOLETE   {
// OBSOLETE     {
// OBSOLETE       sizeof (short), 2, "short"
// OBSOLETE     }
// OBSOLETE      ,
// OBSOLETE     {
// OBSOLETE       sizeof (int), 4, "int"
// OBSOLETE     }
// OBSOLETE      ,
// OBSOLETE     {
// OBSOLETE       sizeof (long), 4, "long"
// OBSOLETE     }
// OBSOLETE      ,
// OBSOLETE     {
// OBSOLETE       sizeof (float), 4, "float"
// OBSOLETE     }
// OBSOLETE      ,
// OBSOLETE     {
// OBSOLETE       sizeof (double), 8, "double"
// OBSOLETE     }
// OBSOLETE      ,
// OBSOLETE     {
// OBSOLETE       sizeof (char *), 4, "pointer"
// OBSOLETE     }
// OBSOLETE      ,
// OBSOLETE   };
// OBSOLETE #define TYPELEN	(sizeof(types) / sizeof(struct typestruct))
// OBSOLETE 
// OBSOLETE   /* Make sure that host type sizes are same as i960
// OBSOLETE    */
// OBSOLETE   for (i = 0; i < TYPELEN; i++)
// OBSOLETE     {
// OBSOLETE       if (types[i].hostsize != types[i].i960size)
// OBSOLETE 	{
// OBSOLETE 	  printf_unfiltered ("sizeof(%s) != %d:  PROCEED AT YOUR OWN RISK!\n",
// OBSOLETE 			     types[i].typename, types[i].i960size);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Is this register part of the register window system?  A yes answer
// OBSOLETE    implies that 1) The name of this register will not be the same in
// OBSOLETE    other frames, and 2) This register is automatically "saved" upon
// OBSOLETE    subroutine calls and thus there is no need to search more than one
// OBSOLETE    stack frame for it.
// OBSOLETE 
// OBSOLETE    On the i960, in fact, the name of this register in another frame is
// OBSOLETE    "mud" -- there is no overlap between the windows.  Each window is
// OBSOLETE    simply saved into the stack (true for our purposes, after having been
// OBSOLETE    flushed; normally they reside on-chip and are restored from on-chip
// OBSOLETE    without ever going to memory).  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE register_in_window_p (int regnum)
// OBSOLETE {
// OBSOLETE   return regnum <= R15_REGNUM;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* i960_find_saved_register ()
// OBSOLETE 
// OBSOLETE    Return the address in which frame FRAME's value of register REGNUM
// OBSOLETE    has been saved in memory.  Or return zero if it has not been saved.
// OBSOLETE    If REGNUM specifies the SP, the value we return is actually the SP
// OBSOLETE    value, not an address where it was saved.  */
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE i960_find_saved_register (struct frame_info *frame, int regnum)
// OBSOLETE {
// OBSOLETE   register struct frame_info *frame1 = NULL;
// OBSOLETE   register CORE_ADDR addr = 0;
// OBSOLETE 
// OBSOLETE   if (frame == NULL)		/* No regs saved if want current frame */
// OBSOLETE     return 0;
// OBSOLETE 
// OBSOLETE   /* We assume that a register in a register window will only be saved
// OBSOLETE      in one place (since the name changes and/or disappears as you go
// OBSOLETE      towards inner frames), so we only call get_frame_saved_regs on
// OBSOLETE      the current frame.  This is directly in contradiction to the
// OBSOLETE      usage below, which assumes that registers used in a frame must be
// OBSOLETE      saved in a lower (more interior) frame.  This change is a result
// OBSOLETE      of working on a register window machine; get_frame_saved_regs
// OBSOLETE      always returns the registers saved within a frame, within the
// OBSOLETE      context (register namespace) of that frame. */
// OBSOLETE 
// OBSOLETE   /* However, note that we don't want this to return anything if
// OBSOLETE      nothing is saved (if there's a frame inside of this one).  Also,
// OBSOLETE      callers to this routine asking for the stack pointer want the
// OBSOLETE      stack pointer saved for *this* frame; this is returned from the
// OBSOLETE      next frame.  */
// OBSOLETE 
// OBSOLETE   if (register_in_window_p (regnum))
// OBSOLETE     {
// OBSOLETE       frame1 = get_next_frame (frame);
// OBSOLETE       if (!frame1)
// OBSOLETE 	return 0;		/* Registers of this frame are active.  */
// OBSOLETE 
// OBSOLETE       /* Get the SP from the next frame in; it will be this
// OBSOLETE          current frame.  */
// OBSOLETE       if (regnum != SP_REGNUM)
// OBSOLETE 	frame1 = frame;
// OBSOLETE 
// OBSOLETE       FRAME_INIT_SAVED_REGS (frame1);
// OBSOLETE       return frame1->saved_regs[regnum];	/* ... which might be zero */
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Note that this next routine assumes that registers used in
// OBSOLETE      frame x will be saved only in the frame that x calls and
// OBSOLETE      frames interior to it.  This is not true on the sparc, but the
// OBSOLETE      above macro takes care of it, so we should be all right. */
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       QUIT;
// OBSOLETE       frame1 = get_next_frame (frame);
// OBSOLETE       if (frame1 == 0)
// OBSOLETE 	break;
// OBSOLETE       frame = frame1;
// OBSOLETE       FRAME_INIT_SAVED_REGS (frame1);
// OBSOLETE       if (frame1->saved_regs[regnum])
// OBSOLETE 	addr = frame1->saved_regs[regnum];
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return addr;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* i960_get_saved_register ()
// OBSOLETE 
// OBSOLETE    Find register number REGNUM relative to FRAME and put its (raw,
// OBSOLETE    target format) contents in *RAW_BUFFER.  Set *OPTIMIZED if the
// OBSOLETE    variable was optimized out (and thus can't be fetched).  Set *LVAL
// OBSOLETE    to lval_memory, lval_register, or not_lval, depending on whether
// OBSOLETE    the value was fetched from memory, from a register, or in a strange
// OBSOLETE    and non-modifiable way (e.g. a frame pointer which was calculated
// OBSOLETE    rather than fetched).  Set *ADDRP to the address, either in memory
// OBSOLETE    on as a REGISTER_BYTE offset into the registers array.
// OBSOLETE 
// OBSOLETE    Note that this implementation never sets *LVAL to not_lval.  But it
// OBSOLETE    can be replaced by defining GET_SAVED_REGISTER and supplying your
// OBSOLETE    own.
// OBSOLETE 
// OBSOLETE    The argument RAW_BUFFER must point to aligned memory.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE i960_get_saved_register (char *raw_buffer,
// OBSOLETE 			 int *optimized,
// OBSOLETE 			 CORE_ADDR *addrp,
// OBSOLETE 			 struct frame_info *frame,
// OBSOLETE 			 int regnum,
// OBSOLETE 			 enum lval_type *lval)
// OBSOLETE {
// OBSOLETE   CORE_ADDR addr;
// OBSOLETE 
// OBSOLETE   if (!target_has_registers)
// OBSOLETE     error ("No registers.");
// OBSOLETE 
// OBSOLETE   /* Normal systems don't optimize out things with register numbers.  */
// OBSOLETE   if (optimized != NULL)
// OBSOLETE     *optimized = 0;
// OBSOLETE   addr = i960_find_saved_register (frame, regnum);
// OBSOLETE   if (addr != 0)
// OBSOLETE     {
// OBSOLETE       if (lval != NULL)
// OBSOLETE 	*lval = lval_memory;
// OBSOLETE       if (regnum == SP_REGNUM)
// OBSOLETE 	{
// OBSOLETE 	  if (raw_buffer != NULL)
// OBSOLETE 	    {
// OBSOLETE 	      /* Put it back in target format.  */
// OBSOLETE 	      store_address (raw_buffer, REGISTER_RAW_SIZE (regnum),
// OBSOLETE 			     (LONGEST) addr);
// OBSOLETE 	    }
// OBSOLETE 	  if (addrp != NULL)
// OBSOLETE 	    *addrp = 0;
// OBSOLETE 	  return;
// OBSOLETE 	}
// OBSOLETE       if (raw_buffer != NULL)
// OBSOLETE 	target_read_memory (addr, raw_buffer, REGISTER_RAW_SIZE (regnum));
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       if (lval != NULL)
// OBSOLETE 	*lval = lval_register;
// OBSOLETE       addr = REGISTER_BYTE (regnum);
// OBSOLETE       if (raw_buffer != NULL)
// OBSOLETE 	read_register_gen (regnum, raw_buffer);
// OBSOLETE     }
// OBSOLETE   if (addrp != NULL)
// OBSOLETE     *addrp = addr;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Examine an i960 function prologue, recording the addresses at which
// OBSOLETE    registers are saved explicitly by the prologue code, and returning
// OBSOLETE    the address of the first instruction after the prologue (but not
// OBSOLETE    after the instruction at address LIMIT, as explained below).
// OBSOLETE 
// OBSOLETE    LIMIT places an upper bound on addresses of the instructions to be
// OBSOLETE    examined.  If the prologue code scan reaches LIMIT, the scan is
// OBSOLETE    aborted and LIMIT is returned.  This is used, when examining the
// OBSOLETE    prologue for the current frame, to keep examine_prologue () from
// OBSOLETE    claiming that a given register has been saved when in fact the
// OBSOLETE    instruction that saves it has not yet been executed.  LIMIT is used
// OBSOLETE    at other times to stop the scan when we hit code after the true
// OBSOLETE    function prologue (e.g. for the first source line) which might
// OBSOLETE    otherwise be mistaken for function prologue.
// OBSOLETE 
// OBSOLETE    The format of the function prologue matched by this routine is
// OBSOLETE    derived from examination of the source to gcc960 1.21, particularly
// OBSOLETE    the routine i960_function_prologue ().  A "regular expression" for
// OBSOLETE    the function prologue is given below:
// OBSOLETE 
// OBSOLETE    (lda LRn, g14
// OBSOLETE    mov g14, g[0-7]
// OBSOLETE    (mov 0, g14) | (lda 0, g14))?
// OBSOLETE 
// OBSOLETE    (mov[qtl]? g[0-15], r[4-15])*
// OBSOLETE    ((addo [1-31], sp, sp) | (lda n(sp), sp))?
// OBSOLETE    (st[qtl]? g[0-15], n(fp))*
// OBSOLETE 
// OBSOLETE    (cmpobne 0, g14, LFn
// OBSOLETE    mov sp, g14
// OBSOLETE    lda 0x30(sp), sp
// OBSOLETE    LFn: stq g0, (g14)
// OBSOLETE    stq g4, 0x10(g14)
// OBSOLETE    stq g8, 0x20(g14))?
// OBSOLETE 
// OBSOLETE    (st g14, n(fp))?
// OBSOLETE    (mov g13,r[4-15])?
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE /* Macros for extracting fields from i960 instructions.  */
// OBSOLETE 
// OBSOLETE #define BITMASK(pos, width) (((0x1 << (width)) - 1) << (pos))
// OBSOLETE #define EXTRACT_FIELD(val, pos, width) ((val) >> (pos) & BITMASK (0, width))
// OBSOLETE 
// OBSOLETE #define REG_SRC1(insn)    EXTRACT_FIELD (insn, 0, 5)
// OBSOLETE #define REG_SRC2(insn)    EXTRACT_FIELD (insn, 14, 5)
// OBSOLETE #define REG_SRCDST(insn)  EXTRACT_FIELD (insn, 19, 5)
// OBSOLETE #define MEM_SRCDST(insn)  EXTRACT_FIELD (insn, 19, 5)
// OBSOLETE #define MEMA_OFFSET(insn) EXTRACT_FIELD (insn, 0, 12)
// OBSOLETE 
// OBSOLETE /* Fetch the instruction at ADDR, returning 0 if ADDR is beyond LIM or
// OBSOLETE    is not the address of a valid instruction, the address of the next
// OBSOLETE    instruction beyond ADDR otherwise.  *PWORD1 receives the first word
// OBSOLETE    of the instruction, and (for two-word instructions), *PWORD2 receives
// OBSOLETE    the second.  */
// OBSOLETE 
// OBSOLETE #define NEXT_PROLOGUE_INSN(addr, lim, pword1, pword2) \
// OBSOLETE   (((addr) < (lim)) ? next_insn (addr, pword1, pword2) : 0)
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE examine_prologue (register CORE_ADDR ip, register CORE_ADDR limit,
// OBSOLETE 		  CORE_ADDR frame_addr, struct frame_saved_regs *fsr)
// OBSOLETE {
// OBSOLETE   register CORE_ADDR next_ip;
// OBSOLETE   register int src, dst;
// OBSOLETE   register unsigned int *pcode;
// OBSOLETE   unsigned int insn1, insn2;
// OBSOLETE   int size;
// OBSOLETE   int within_leaf_prologue;
// OBSOLETE   CORE_ADDR save_addr;
// OBSOLETE   static unsigned int varargs_prologue_code[] =
// OBSOLETE   {
// OBSOLETE     0x3507a00c,			/* cmpobne 0x0, g14, LFn */
// OBSOLETE     0x5cf01601,			/* mov sp, g14           */
// OBSOLETE     0x8c086030,			/* lda 0x30(sp), sp      */
// OBSOLETE     0xb2879000,			/* LFn: stq  g0, (g14)   */
// OBSOLETE     0xb2a7a010,			/* stq g4, 0x10(g14)     */
// OBSOLETE     0xb2c7a020			/* stq g8, 0x20(g14)     */
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE   /* Accept a leaf procedure prologue code fragment if present.
// OBSOLETE      Note that ip might point to either the leaf or non-leaf
// OBSOLETE      entry point; we look for the non-leaf entry point first:  */
// OBSOLETE 
// OBSOLETE   within_leaf_prologue = 0;
// OBSOLETE   if ((next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2))
// OBSOLETE       && ((insn1 & 0xfffff000) == 0x8cf00000	/* lda LRx, g14 (MEMA) */
// OBSOLETE 	  || (insn1 & 0xfffffc60) == 0x8cf03000))	/* lda LRx, g14 (MEMB) */
// OBSOLETE     {
// OBSOLETE       within_leaf_prologue = 1;
// OBSOLETE       next_ip = NEXT_PROLOGUE_INSN (next_ip, limit, &insn1, &insn2);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Now look for the prologue code at a leaf entry point:  */
// OBSOLETE 
// OBSOLETE   if (next_ip
// OBSOLETE       && (insn1 & 0xff87ffff) == 0x5c80161e	/* mov g14, gx */
// OBSOLETE       && REG_SRCDST (insn1) <= G0_REGNUM + 7)
// OBSOLETE     {
// OBSOLETE       within_leaf_prologue = 1;
// OBSOLETE       if ((next_ip = NEXT_PROLOGUE_INSN (next_ip, limit, &insn1, &insn2))
// OBSOLETE 	  && (insn1 == 0x8cf00000	/* lda 0, g14 */
// OBSOLETE 	      || insn1 == 0x5cf01e00))	/* mov 0, g14 */
// OBSOLETE 	{
// OBSOLETE 	  ip = next_ip;
// OBSOLETE 	  next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
// OBSOLETE 	  within_leaf_prologue = 0;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* If something that looks like the beginning of a leaf prologue
// OBSOLETE      has been seen, but the remainder of the prologue is missing, bail.
// OBSOLETE      We don't know what we've got.  */
// OBSOLETE 
// OBSOLETE   if (within_leaf_prologue)
// OBSOLETE     return (ip);
// OBSOLETE 
// OBSOLETE   /* Accept zero or more instances of "mov[qtl]? gx, ry", where y >= 4.
// OBSOLETE      This may cause us to mistake the moving of a register
// OBSOLETE      parameter to a local register for the saving of a callee-saved
// OBSOLETE      register, but that can't be helped, since with the
// OBSOLETE      "-fcall-saved" flag, any register can be made callee-saved.  */
// OBSOLETE 
// OBSOLETE   while (next_ip
// OBSOLETE 	 && (insn1 & 0xfc802fb0) == 0x5c000610
// OBSOLETE 	 && (dst = REG_SRCDST (insn1)) >= (R0_REGNUM + 4))
// OBSOLETE     {
// OBSOLETE       src = REG_SRC1 (insn1);
// OBSOLETE       size = EXTRACT_FIELD (insn1, 24, 2) + 1;
// OBSOLETE       save_addr = frame_addr + ((dst - R0_REGNUM) * 4);
// OBSOLETE       while (size--)
// OBSOLETE 	{
// OBSOLETE 	  fsr->regs[src++] = save_addr;
// OBSOLETE 	  save_addr += 4;
// OBSOLETE 	}
// OBSOLETE       ip = next_ip;
// OBSOLETE       next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Accept an optional "addo n, sp, sp" or "lda n(sp), sp".  */
// OBSOLETE 
// OBSOLETE   if (next_ip &&
// OBSOLETE       ((insn1 & 0xffffffe0) == 0x59084800	/* addo n, sp, sp */
// OBSOLETE        || (insn1 & 0xfffff000) == 0x8c086000	/* lda n(sp), sp (MEMA) */
// OBSOLETE        || (insn1 & 0xfffffc60) == 0x8c087400))	/* lda n(sp), sp (MEMB) */
// OBSOLETE     {
// OBSOLETE       ip = next_ip;
// OBSOLETE       next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Accept zero or more instances of "st[qtl]? gx, n(fp)".  
// OBSOLETE      This may cause us to mistake the copying of a register
// OBSOLETE      parameter to the frame for the saving of a callee-saved
// OBSOLETE      register, but that can't be helped, since with the
// OBSOLETE      "-fcall-saved" flag, any register can be made callee-saved.
// OBSOLETE      We can, however, refuse to accept a save of register g14,
// OBSOLETE      since that is matched explicitly below.  */
// OBSOLETE 
// OBSOLETE   while (next_ip &&
// OBSOLETE 	 ((insn1 & 0xf787f000) == 0x9287e000	/* stl? gx, n(fp) (MEMA) */
// OBSOLETE 	  || (insn1 & 0xf787fc60) == 0x9287f400		/* stl? gx, n(fp) (MEMB) */
// OBSOLETE 	  || (insn1 & 0xef87f000) == 0xa287e000		/* st[tq] gx, n(fp) (MEMA) */
// OBSOLETE 	  || (insn1 & 0xef87fc60) == 0xa287f400)	/* st[tq] gx, n(fp) (MEMB) */
// OBSOLETE 	 && ((src = MEM_SRCDST (insn1)) != G14_REGNUM))
// OBSOLETE     {
// OBSOLETE       save_addr = frame_addr + ((insn1 & BITMASK (12, 1))
// OBSOLETE 				? insn2 : MEMA_OFFSET (insn1));
// OBSOLETE       size = (insn1 & BITMASK (29, 1)) ? ((insn1 & BITMASK (28, 1)) ? 4 : 3)
// OBSOLETE 	: ((insn1 & BITMASK (27, 1)) ? 2 : 1);
// OBSOLETE       while (size--)
// OBSOLETE 	{
// OBSOLETE 	  fsr->regs[src++] = save_addr;
// OBSOLETE 	  save_addr += 4;
// OBSOLETE 	}
// OBSOLETE       ip = next_ip;
// OBSOLETE       next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Accept the varargs prologue code if present.  */
// OBSOLETE 
// OBSOLETE   size = sizeof (varargs_prologue_code) / sizeof (int);
// OBSOLETE   pcode = varargs_prologue_code;
// OBSOLETE   while (size-- && next_ip && *pcode++ == insn1)
// OBSOLETE     {
// OBSOLETE       ip = next_ip;
// OBSOLETE       next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Accept an optional "st g14, n(fp)".  */
// OBSOLETE 
// OBSOLETE   if (next_ip &&
// OBSOLETE       ((insn1 & 0xfffff000) == 0x92f7e000	/* st g14, n(fp) (MEMA) */
// OBSOLETE        || (insn1 & 0xfffffc60) == 0x92f7f400))	/* st g14, n(fp) (MEMB) */
// OBSOLETE     {
// OBSOLETE       fsr->regs[G14_REGNUM] = frame_addr + ((insn1 & BITMASK (12, 1))
// OBSOLETE 					    ? insn2 : MEMA_OFFSET (insn1));
// OBSOLETE       ip = next_ip;
// OBSOLETE       next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Accept zero or one instance of "mov g13, ry", where y >= 4.
// OBSOLETE      This is saving the address where a struct should be returned.  */
// OBSOLETE 
// OBSOLETE   if (next_ip
// OBSOLETE       && (insn1 & 0xff802fbf) == 0x5c00061d
// OBSOLETE       && (dst = REG_SRCDST (insn1)) >= (R0_REGNUM + 4))
// OBSOLETE     {
// OBSOLETE       save_addr = frame_addr + ((dst - R0_REGNUM) * 4);
// OBSOLETE       fsr->regs[G0_REGNUM + 13] = save_addr;
// OBSOLETE       ip = next_ip;
// OBSOLETE #if 0				/* We'll need this once there is a subsequent instruction examined. */
// OBSOLETE       next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn1, &insn2);
// OBSOLETE #endif
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return (ip);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Given an ip value corresponding to the start of a function,
// OBSOLETE    return the ip of the first instruction after the function 
// OBSOLETE    prologue.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE i960_skip_prologue (CORE_ADDR ip)
// OBSOLETE {
// OBSOLETE   struct frame_saved_regs saved_regs_dummy;
// OBSOLETE   struct symtab_and_line sal;
// OBSOLETE   CORE_ADDR limit;
// OBSOLETE 
// OBSOLETE   sal = find_pc_line (ip, 0);
// OBSOLETE   limit = (sal.end) ? sal.end : 0xffffffff;
// OBSOLETE 
// OBSOLETE   return (examine_prologue (ip, limit, (CORE_ADDR) 0, &saved_regs_dummy));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Put here the code to store, into a struct frame_saved_regs,
// OBSOLETE    the addresses of the saved registers of frame described by FRAME_INFO.
// OBSOLETE    This includes special registers such as pc and fp saved in special
// OBSOLETE    ways in the stack frame.  sp is even more special:
// OBSOLETE    the address we return for it IS the sp for the next frame.
// OBSOLETE 
// OBSOLETE    We cache the result of doing this in the frame_obstack, since it is
// OBSOLETE    fairly expensive.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE frame_find_saved_regs (struct frame_info *fi, struct frame_saved_regs *fsr)
// OBSOLETE {
// OBSOLETE   register CORE_ADDR next_addr;
// OBSOLETE   register CORE_ADDR *saved_regs;
// OBSOLETE   register int regnum;
// OBSOLETE   register struct frame_saved_regs *cache_fsr;
// OBSOLETE   CORE_ADDR ip;
// OBSOLETE   struct symtab_and_line sal;
// OBSOLETE   CORE_ADDR limit;
// OBSOLETE 
// OBSOLETE   if (!fi->fsr)
// OBSOLETE     {
// OBSOLETE       cache_fsr = (struct frame_saved_regs *)
// OBSOLETE 	frame_obstack_alloc (sizeof (struct frame_saved_regs));
// OBSOLETE       memset (cache_fsr, '\0', sizeof (struct frame_saved_regs));
// OBSOLETE       fi->fsr = cache_fsr;
// OBSOLETE 
// OBSOLETE       /* Find the start and end of the function prologue.  If the PC
// OBSOLETE          is in the function prologue, we only consider the part that
// OBSOLETE          has executed already.  */
// OBSOLETE 
// OBSOLETE       ip = get_pc_function_start (fi->pc);
// OBSOLETE       sal = find_pc_line (ip, 0);
// OBSOLETE       limit = (sal.end && sal.end < fi->pc) ? sal.end : fi->pc;
// OBSOLETE 
// OBSOLETE       examine_prologue (ip, limit, fi->frame, cache_fsr);
// OBSOLETE 
// OBSOLETE       /* Record the addresses at which the local registers are saved.
// OBSOLETE          Strictly speaking, we should only do this for non-leaf procedures,
// OBSOLETE          but no one will ever look at these values if it is a leaf procedure,
// OBSOLETE          since local registers are always caller-saved.  */
// OBSOLETE 
// OBSOLETE       next_addr = (CORE_ADDR) fi->frame;
// OBSOLETE       saved_regs = cache_fsr->regs;
// OBSOLETE       for (regnum = R0_REGNUM; regnum <= R15_REGNUM; regnum++)
// OBSOLETE 	{
// OBSOLETE 	  *saved_regs++ = next_addr;
// OBSOLETE 	  next_addr += 4;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       cache_fsr->regs[FP_REGNUM] = cache_fsr->regs[PFP_REGNUM];
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   *fsr = *fi->fsr;
// OBSOLETE 
// OBSOLETE   /* Fetch the value of the sp from memory every time, since it
// OBSOLETE      is conceivable that it has changed since the cache was flushed.  
// OBSOLETE      This unfortunately undoes much of the savings from caching the 
// OBSOLETE      saved register values.  I suggest adding an argument to 
// OBSOLETE      get_frame_saved_regs () specifying the register number we're
// OBSOLETE      interested in (or -1 for all registers).  This would be passed
// OBSOLETE      through to FRAME_FIND_SAVED_REGS (), permitting more efficient
// OBSOLETE      computation of saved register addresses (e.g., on the i960,
// OBSOLETE      we don't have to examine the prologue to find local registers). 
// OBSOLETE      -- markf@wrs.com 
// OBSOLETE      FIXME, we don't need to refetch this, since the cache is cleared
// OBSOLETE      every time the child process is restarted.  If GDB itself
// OBSOLETE      modifies SP, it has to clear the cache by hand (does it?).  -gnu */
// OBSOLETE 
// OBSOLETE   fsr->regs[SP_REGNUM] = read_memory_integer (fsr->regs[SP_REGNUM], 4);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the address of the argument block for the frame
// OBSOLETE    described by FI.  Returns 0 if the address is unknown.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE frame_args_address (struct frame_info *fi, int must_be_correct)
// OBSOLETE {
// OBSOLETE   struct frame_saved_regs fsr;
// OBSOLETE   CORE_ADDR ap;
// OBSOLETE 
// OBSOLETE   /* If g14 was saved in the frame by the function prologue code, return
// OBSOLETE      the saved value.  If the frame is current and we are being sloppy,
// OBSOLETE      return the value of g14.  Otherwise, return zero.  */
// OBSOLETE 
// OBSOLETE   get_frame_saved_regs (fi, &fsr);
// OBSOLETE   if (fsr.regs[G14_REGNUM])
// OBSOLETE     ap = read_memory_integer (fsr.regs[G14_REGNUM], 4);
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       if (must_be_correct)
// OBSOLETE 	return 0;		/* Don't cache this result */
// OBSOLETE       if (get_next_frame (fi))
// OBSOLETE 	ap = 0;
// OBSOLETE       else
// OBSOLETE 	ap = read_register (G14_REGNUM);
// OBSOLETE       if (ap == 0)
// OBSOLETE 	ap = fi->frame;
// OBSOLETE     }
// OBSOLETE   fi->arg_pointer = ap;		/* Cache it for next time */
// OBSOLETE   return ap;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the address of the return struct for the frame
// OBSOLETE    described by FI.  Returns 0 if the address is unknown.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE frame_struct_result_address (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   struct frame_saved_regs fsr;
// OBSOLETE   CORE_ADDR ap;
// OBSOLETE 
// OBSOLETE   /* If the frame is non-current, check to see if g14 was saved in the
// OBSOLETE      frame by the function prologue code; return the saved value if so,
// OBSOLETE      zero otherwise.  If the frame is current, return the value of g14.
// OBSOLETE 
// OBSOLETE      FIXME, shouldn't this use the saved value as long as we are past
// OBSOLETE      the function prologue, and only use the current value if we have
// OBSOLETE      no saved value and are at TOS?   -- gnu@cygnus.com */
// OBSOLETE 
// OBSOLETE   if (get_next_frame (fi))
// OBSOLETE     {
// OBSOLETE       get_frame_saved_regs (fi, &fsr);
// OBSOLETE       if (fsr.regs[G13_REGNUM])
// OBSOLETE 	ap = read_memory_integer (fsr.regs[G13_REGNUM], 4);
// OBSOLETE       else
// OBSOLETE 	ap = 0;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     ap = read_register (G13_REGNUM);
// OBSOLETE 
// OBSOLETE   return ap;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return address to which the currently executing leafproc will return,
// OBSOLETE    or 0 if IP, the value of the instruction pointer from the currently
// OBSOLETE    executing function, is not in a leafproc (or if we can't tell if it
// OBSOLETE    is).
// OBSOLETE 
// OBSOLETE    Do this by finding the starting address of the routine in which IP lies.
// OBSOLETE    If the instruction there is "mov g14, gx" (where x is in [0,7]), this
// OBSOLETE    is a leafproc and the return address is in register gx.  Well, this is
// OBSOLETE    true unless the return address points at a RET instruction in the current
// OBSOLETE    procedure, which indicates that we have a 'dual entry' routine that
// OBSOLETE    has been entered through the CALL entry point.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE leafproc_return (CORE_ADDR ip)
// OBSOLETE {
// OBSOLETE   register struct minimal_symbol *msymbol;
// OBSOLETE   char *p;
// OBSOLETE   int dst;
// OBSOLETE   unsigned int insn1, insn2;
// OBSOLETE   CORE_ADDR return_addr;
// OBSOLETE 
// OBSOLETE   if ((msymbol = lookup_minimal_symbol_by_pc (ip)) != NULL)
// OBSOLETE     {
// OBSOLETE       if ((p = strchr (SYMBOL_NAME (msymbol), '.')) && STREQ (p, ".lf"))
// OBSOLETE 	{
// OBSOLETE 	  if (next_insn (SYMBOL_VALUE_ADDRESS (msymbol), &insn1, &insn2)
// OBSOLETE 	      && (insn1 & 0xff87ffff) == 0x5c80161e	/* mov g14, gx */
// OBSOLETE 	      && (dst = REG_SRCDST (insn1)) <= G0_REGNUM + 7)
// OBSOLETE 	    {
// OBSOLETE 	      /* Get the return address.  If the "mov g14, gx" 
// OBSOLETE 	         instruction hasn't been executed yet, read
// OBSOLETE 	         the return address from g14; otherwise, read it
// OBSOLETE 	         from the register into which g14 was moved.  */
// OBSOLETE 
// OBSOLETE 	      return_addr =
// OBSOLETE 		read_register ((ip == SYMBOL_VALUE_ADDRESS (msymbol))
// OBSOLETE 			       ? G14_REGNUM : dst);
// OBSOLETE 
// OBSOLETE 	      /* We know we are in a leaf procedure, but we don't know
// OBSOLETE 	         whether the caller actually did a "bal" to the ".lf"
// OBSOLETE 	         entry point, or a normal "call" to the non-leaf entry
// OBSOLETE 	         point one instruction before.  In the latter case, the
// OBSOLETE 	         return address will be the address of a "ret"
// OBSOLETE 	         instruction within the procedure itself.  We test for
// OBSOLETE 	         this below.  */
// OBSOLETE 
// OBSOLETE 	      if (!next_insn (return_addr, &insn1, &insn2)
// OBSOLETE 		  || (insn1 & 0xff000000) != 0xa000000	/* ret */
// OBSOLETE 		  || lookup_minimal_symbol_by_pc (return_addr) != msymbol)
// OBSOLETE 		return (return_addr);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Immediately after a function call, return the saved pc.
// OBSOLETE    Can't go through the frames for this because on some machines
// OBSOLETE    the new frame is not set up until the new function executes
// OBSOLETE    some instructions. 
// OBSOLETE    On the i960, the frame *is* set up immediately after the call,
// OBSOLETE    unless the function is a leaf procedure.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE saved_pc_after_call (struct frame_info *frame)
// OBSOLETE {
// OBSOLETE   CORE_ADDR saved_pc;
// OBSOLETE 
// OBSOLETE   saved_pc = leafproc_return (get_frame_pc (frame));
// OBSOLETE   if (!saved_pc)
// OBSOLETE     saved_pc = FRAME_SAVED_PC (frame);
// OBSOLETE 
// OBSOLETE   return saved_pc;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Discard from the stack the innermost frame,
// OBSOLETE    restoring all saved registers.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE i960_pop_frame (void)
// OBSOLETE {
// OBSOLETE   register struct frame_info *current_fi, *prev_fi;
// OBSOLETE   register int i;
// OBSOLETE   CORE_ADDR save_addr;
// OBSOLETE   CORE_ADDR leaf_return_addr;
// OBSOLETE   struct frame_saved_regs fsr;
// OBSOLETE   char local_regs_buf[16 * 4];
// OBSOLETE 
// OBSOLETE   current_fi = get_current_frame ();
// OBSOLETE 
// OBSOLETE   /* First, undo what the hardware does when we return.
// OBSOLETE      If this is a non-leaf procedure, restore local registers from
// OBSOLETE      the save area in the calling frame.  Otherwise, load the return
// OBSOLETE      address obtained from leafproc_return () into the rip.  */
// OBSOLETE 
// OBSOLETE   leaf_return_addr = leafproc_return (current_fi->pc);
// OBSOLETE   if (!leaf_return_addr)
// OBSOLETE     {
// OBSOLETE       /* Non-leaf procedure.  Restore local registers, incl IP.  */
// OBSOLETE       prev_fi = get_prev_frame (current_fi);
// OBSOLETE       read_memory (prev_fi->frame, local_regs_buf, sizeof (local_regs_buf));
// OBSOLETE       write_register_bytes (REGISTER_BYTE (R0_REGNUM), local_regs_buf,
// OBSOLETE 			    sizeof (local_regs_buf));
// OBSOLETE 
// OBSOLETE       /* Restore frame pointer.  */
// OBSOLETE       write_register (FP_REGNUM, prev_fi->frame);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* Leaf procedure.  Just restore the return address into the IP.  */
// OBSOLETE       write_register (RIP_REGNUM, leaf_return_addr);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Now restore any global regs that the current function had saved. */
// OBSOLETE   get_frame_saved_regs (current_fi, &fsr);
// OBSOLETE   for (i = G0_REGNUM; i < G14_REGNUM; i++)
// OBSOLETE     {
// OBSOLETE       save_addr = fsr.regs[i];
// OBSOLETE       if (save_addr != 0)
// OBSOLETE 	write_register (i, read_memory_integer (save_addr, 4));
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Flush the frame cache, create a frame for the new innermost frame,
// OBSOLETE      and make it the current frame.  */
// OBSOLETE 
// OBSOLETE   flush_cached_frames ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Given a 960 stop code (fault or trace), return the signal which
// OBSOLETE    corresponds.  */
// OBSOLETE 
// OBSOLETE enum target_signal
// OBSOLETE i960_fault_to_signal (int fault)
// OBSOLETE {
// OBSOLETE   switch (fault)
// OBSOLETE     {
// OBSOLETE     case 0:
// OBSOLETE       return TARGET_SIGNAL_BUS;	/* parallel fault */
// OBSOLETE     case 1:
// OBSOLETE       return TARGET_SIGNAL_UNKNOWN;
// OBSOLETE     case 2:
// OBSOLETE       return TARGET_SIGNAL_ILL;	/* operation fault */
// OBSOLETE     case 3:
// OBSOLETE       return TARGET_SIGNAL_FPE;	/* arithmetic fault */
// OBSOLETE     case 4:
// OBSOLETE       return TARGET_SIGNAL_FPE;	/* floating point fault */
// OBSOLETE 
// OBSOLETE       /* constraint fault.  This appears not to distinguish between
// OBSOLETE          a range constraint fault (which should be SIGFPE) and a privileged
// OBSOLETE          fault (which should be SIGILL).  */
// OBSOLETE     case 5:
// OBSOLETE       return TARGET_SIGNAL_ILL;
// OBSOLETE 
// OBSOLETE     case 6:
// OBSOLETE       return TARGET_SIGNAL_SEGV;	/* virtual memory fault */
// OBSOLETE 
// OBSOLETE       /* protection fault.  This is for an out-of-range argument to
// OBSOLETE          "calls".  I guess it also could be SIGILL. */
// OBSOLETE     case 7:
// OBSOLETE       return TARGET_SIGNAL_SEGV;
// OBSOLETE 
// OBSOLETE     case 8:
// OBSOLETE       return TARGET_SIGNAL_BUS;	/* machine fault */
// OBSOLETE     case 9:
// OBSOLETE       return TARGET_SIGNAL_BUS;	/* structural fault */
// OBSOLETE     case 0xa:
// OBSOLETE       return TARGET_SIGNAL_ILL;	/* type fault */
// OBSOLETE     case 0xb:
// OBSOLETE       return TARGET_SIGNAL_UNKNOWN;	/* reserved fault */
// OBSOLETE     case 0xc:
// OBSOLETE       return TARGET_SIGNAL_BUS;	/* process fault */
// OBSOLETE     case 0xd:
// OBSOLETE       return TARGET_SIGNAL_SEGV;	/* descriptor fault */
// OBSOLETE     case 0xe:
// OBSOLETE       return TARGET_SIGNAL_BUS;	/* event fault */
// OBSOLETE     case 0xf:
// OBSOLETE       return TARGET_SIGNAL_UNKNOWN;	/* reserved fault */
// OBSOLETE     case 0x10:
// OBSOLETE       return TARGET_SIGNAL_TRAP;	/* single-step trace */
// OBSOLETE     case 0x11:
// OBSOLETE       return TARGET_SIGNAL_TRAP;	/* branch trace */
// OBSOLETE     case 0x12:
// OBSOLETE       return TARGET_SIGNAL_TRAP;	/* call trace */
// OBSOLETE     case 0x13:
// OBSOLETE       return TARGET_SIGNAL_TRAP;	/* return trace */
// OBSOLETE     case 0x14:
// OBSOLETE       return TARGET_SIGNAL_TRAP;	/* pre-return trace */
// OBSOLETE     case 0x15:
// OBSOLETE       return TARGET_SIGNAL_TRAP;	/* supervisor call trace */
// OBSOLETE     case 0x16:
// OBSOLETE       return TARGET_SIGNAL_TRAP;	/* breakpoint trace */
// OBSOLETE     default:
// OBSOLETE       return TARGET_SIGNAL_UNKNOWN;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /****************************************/
// OBSOLETE /* MEM format                           */
// OBSOLETE /****************************************/
// OBSOLETE 
// OBSOLETE struct tabent
// OBSOLETE {
// OBSOLETE   char *name;
// OBSOLETE   char numops;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Return instruction length, either 4 or 8.  When NOPRINT is non-zero
// OBSOLETE    (TRUE), don't output any text.  (Actually, as implemented, if NOPRINT
// OBSOLETE    is 0, abort() is called.) */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE mem (unsigned long memaddr, unsigned long word1, unsigned long word2,
// OBSOLETE      int noprint)
// OBSOLETE {
// OBSOLETE   int i, j;
// OBSOLETE   int len;
// OBSOLETE   int mode;
// OBSOLETE   int offset;
// OBSOLETE   const char *reg1, *reg2, *reg3;
// OBSOLETE 
// OBSOLETE   /* This lookup table is too sparse to make it worth typing in, but not
// OBSOLETE    * so large as to make a sparse array necessary.  We allocate the
// OBSOLETE    * table at runtime, initialize all entries to empty, and copy the
// OBSOLETE    * real ones in from an initialization table.
// OBSOLETE    *
// OBSOLETE    * NOTE: In this table, the meaning of 'numops' is:
// OBSOLETE    *       1: single operand
// OBSOLETE    *       2: 2 operands, load instruction
// OBSOLETE    *      -2: 2 operands, store instruction
// OBSOLETE    */
// OBSOLETE   static struct tabent *mem_tab = NULL;
// OBSOLETE /* Opcodes of 0x8X, 9X, aX, bX, and cX must be in the table.  */
// OBSOLETE #define MEM_MIN	0x80
// OBSOLETE #define MEM_MAX	0xcf
// OBSOLETE #define MEM_SIZ	((MEM_MAX-MEM_MIN+1) * sizeof(struct tabent))
// OBSOLETE 
// OBSOLETE   static struct
// OBSOLETE     {
// OBSOLETE       int opcode;
// OBSOLETE       char *name;
// OBSOLETE       char numops;
// OBSOLETE     }
// OBSOLETE   mem_init[] =
// OBSOLETE   {
// OBSOLETE     0x80, "ldob", 2,
// OBSOLETE       0x82, "stob", -2,
// OBSOLETE       0x84, "bx", 1,
// OBSOLETE       0x85, "balx", 2,
// OBSOLETE       0x86, "callx", 1,
// OBSOLETE       0x88, "ldos", 2,
// OBSOLETE       0x8a, "stos", -2,
// OBSOLETE       0x8c, "lda", 2,
// OBSOLETE       0x90, "ld", 2,
// OBSOLETE       0x92, "st", -2,
// OBSOLETE       0x98, "ldl", 2,
// OBSOLETE       0x9a, "stl", -2,
// OBSOLETE       0xa0, "ldt", 2,
// OBSOLETE       0xa2, "stt", -2,
// OBSOLETE       0xb0, "ldq", 2,
// OBSOLETE       0xb2, "stq", -2,
// OBSOLETE       0xc0, "ldib", 2,
// OBSOLETE       0xc2, "stib", -2,
// OBSOLETE       0xc8, "ldis", 2,
// OBSOLETE       0xca, "stis", -2,
// OBSOLETE       0, NULL, 0
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE   if (mem_tab == NULL)
// OBSOLETE     {
// OBSOLETE       mem_tab = (struct tabent *) xmalloc (MEM_SIZ);
// OBSOLETE       memset (mem_tab, '\0', MEM_SIZ);
// OBSOLETE       for (i = 0; mem_init[i].opcode != 0; i++)
// OBSOLETE 	{
// OBSOLETE 	  j = mem_init[i].opcode - MEM_MIN;
// OBSOLETE 	  mem_tab[j].name = mem_init[i].name;
// OBSOLETE 	  mem_tab[j].numops = mem_init[i].numops;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   i = ((word1 >> 24) & 0xff) - MEM_MIN;
// OBSOLETE   mode = (word1 >> 10) & 0xf;
// OBSOLETE 
// OBSOLETE   if ((mem_tab[i].name != NULL)	/* Valid instruction */
// OBSOLETE       && ((mode == 5) || (mode >= 12)))
// OBSOLETE     {				/* With 32-bit displacement */
// OBSOLETE       len = 8;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       len = 4;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (noprint)
// OBSOLETE     {
// OBSOLETE       return len;
// OBSOLETE     }
// OBSOLETE   internal_error (__FILE__, __LINE__, "failed internal consistency check");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Read the i960 instruction at 'memaddr' and return the address of 
// OBSOLETE    the next instruction after that, or 0 if 'memaddr' is not the
// OBSOLETE    address of a valid instruction.  The first word of the instruction
// OBSOLETE    is stored at 'pword1', and the second word, if any, is stored at
// OBSOLETE    'pword2'.  */
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE next_insn (CORE_ADDR memaddr, unsigned int *pword1, unsigned int *pword2)
// OBSOLETE {
// OBSOLETE   int len;
// OBSOLETE   char buf[8];
// OBSOLETE 
// OBSOLETE   /* Read the two (potential) words of the instruction at once,
// OBSOLETE      to eliminate the overhead of two calls to read_memory ().
// OBSOLETE      FIXME: Loses if the first one is readable but the second is not
// OBSOLETE      (e.g. last word of the segment).  */
// OBSOLETE 
// OBSOLETE   read_memory (memaddr, buf, 8);
// OBSOLETE   *pword1 = extract_unsigned_integer (buf, 4);
// OBSOLETE   *pword2 = extract_unsigned_integer (buf + 4, 4);
// OBSOLETE 
// OBSOLETE   /* Divide instruction set into classes based on high 4 bits of opcode */
// OBSOLETE 
// OBSOLETE   switch ((*pword1 >> 28) & 0xf)
// OBSOLETE     {
// OBSOLETE     case 0x0:
// OBSOLETE     case 0x1:			/* ctrl */
// OBSOLETE 
// OBSOLETE     case 0x2:
// OBSOLETE     case 0x3:			/* cobr */
// OBSOLETE 
// OBSOLETE     case 0x5:
// OBSOLETE     case 0x6:
// OBSOLETE     case 0x7:			/* reg */
// OBSOLETE       len = 4;
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case 0x8:
// OBSOLETE     case 0x9:
// OBSOLETE     case 0xa:
// OBSOLETE     case 0xb:
// OBSOLETE     case 0xc:
// OBSOLETE       len = mem (memaddr, *pword1, *pword2, 1);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     default:			/* invalid instruction */
// OBSOLETE       len = 0;
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (len)
// OBSOLETE     return memaddr + len;
// OBSOLETE   else
// OBSOLETE     return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* 'start_frame' is a variable in the MON960 runtime startup routine
// OBSOLETE    that contains the frame pointer of the 'start' routine (the routine
// OBSOLETE    that calls 'main').  By reading its contents out of remote memory,
// OBSOLETE    we can tell where the frame chain ends:  backtraces should halt before
// OBSOLETE    they display this frame.  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE mon960_frame_chain_valid (CORE_ADDR chain, struct frame_info *curframe)
// OBSOLETE {
// OBSOLETE   struct symbol *sym;
// OBSOLETE   struct minimal_symbol *msymbol;
// OBSOLETE 
// OBSOLETE   /* crtmon960.o is an assembler module that is assumed to be linked
// OBSOLETE    * first in an i80960 executable.  It contains the true entry point;
// OBSOLETE    * it performs startup up initialization and then calls 'main'.
// OBSOLETE    *
// OBSOLETE    * 'sf' is the name of a variable in crtmon960.o that is set
// OBSOLETE    *      during startup to the address of the first frame.
// OBSOLETE    *
// OBSOLETE    * 'a' is the address of that variable in 80960 memory.
// OBSOLETE    */
// OBSOLETE   static char sf[] = "start_frame";
// OBSOLETE   CORE_ADDR a;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   chain &= ~0x3f;		/* Zero low 6 bits because previous frame pointers
// OBSOLETE 				   contain return status info in them.  */
// OBSOLETE   if (chain == 0)
// OBSOLETE     {
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   sym = lookup_symbol (sf, 0, VAR_NAMESPACE, (int *) NULL,
// OBSOLETE 		       (struct symtab **) NULL);
// OBSOLETE   if (sym != 0)
// OBSOLETE     {
// OBSOLETE       a = SYMBOL_VALUE (sym);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       msymbol = lookup_minimal_symbol (sf, NULL, NULL);
// OBSOLETE       if (msymbol == NULL)
// OBSOLETE 	return 0;
// OBSOLETE       a = SYMBOL_VALUE_ADDRESS (msymbol);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return (chain != read_memory_integer (a, 4));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_i960_tdep (void)
// OBSOLETE {
// OBSOLETE   check_host ();
// OBSOLETE 
// OBSOLETE   tm_print_insn = print_insn_i960;
// OBSOLETE }
