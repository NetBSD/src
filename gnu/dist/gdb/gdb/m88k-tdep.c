// OBSOLETE /* Target-machine dependent code for Motorola 88000 series, for GDB.
// OBSOLETE 
// OBSOLETE    Copyright 1988, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1998,
// OBSOLETE    2000, 2001, 2002 Free Software Foundation, Inc.
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
// OBSOLETE #include "frame.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "setjmp.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE /* Size of an instruction */
// OBSOLETE #define	BYTES_PER_88K_INSN	4
// OBSOLETE 
// OBSOLETE void frame_find_saved_regs ();
// OBSOLETE 
// OBSOLETE /* Is this target an m88110?  Otherwise assume m88100.  This has
// OBSOLETE    relevance for the ways in which we screw with instruction pointers.  */
// OBSOLETE 
// OBSOLETE int target_is_m88110 = 0;
// OBSOLETE 
// OBSOLETE void
// OBSOLETE m88k_target_write_pc (CORE_ADDR pc, ptid_t ptid)
// OBSOLETE {
// OBSOLETE   /* According to the MC88100 RISC Microprocessor User's Manual,
// OBSOLETE      section 6.4.3.1.2:
// OBSOLETE 
// OBSOLETE      ... can be made to return to a particular instruction by placing
// OBSOLETE      a valid instruction address in the SNIP and the next sequential
// OBSOLETE      instruction address in the SFIP (with V bits set and E bits
// OBSOLETE      clear).  The rte resumes execution at the instruction pointed to
// OBSOLETE      by the SNIP, then the SFIP.
// OBSOLETE 
// OBSOLETE      The E bit is the least significant bit (bit 0).  The V (valid)
// OBSOLETE      bit is bit 1.  This is why we logical or 2 into the values we are
// OBSOLETE      writing below.  It turns out that SXIP plays no role when
// OBSOLETE      returning from an exception so nothing special has to be done
// OBSOLETE      with it.  We could even (presumably) give it a totally bogus
// OBSOLETE      value.
// OBSOLETE 
// OBSOLETE      -- Kevin Buettner */
// OBSOLETE 
// OBSOLETE   write_register_pid (SXIP_REGNUM, pc, ptid);
// OBSOLETE   write_register_pid (SNIP_REGNUM, (pc | 2), ptid);
// OBSOLETE   write_register_pid (SFIP_REGNUM, (pc | 2) + 4, ptid);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* The type of a register.  */
// OBSOLETE struct type *
// OBSOLETE m88k_register_type (int regnum)
// OBSOLETE {
// OBSOLETE   if (regnum >= XFP_REGNUM)
// OBSOLETE     return builtin_type_m88110_ext;
// OBSOLETE   else if (regnum == PC_REGNUM || regnum == FP_REGNUM || regnum == SP_REGNUM)
// OBSOLETE     return builtin_type_void_func_ptr;
// OBSOLETE   else
// OBSOLETE     return builtin_type_int32;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* The m88k kernel aligns all instructions on 4-byte boundaries.  The
// OBSOLETE    kernel also uses the least significant two bits for its own hocus
// OBSOLETE    pocus.  When gdb receives an address from the kernel, it needs to
// OBSOLETE    preserve those right-most two bits, but gdb also needs to be careful
// OBSOLETE    to realize that those two bits are not really a part of the address
// OBSOLETE    of an instruction.  Shrug.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE m88k_addr_bits_remove (CORE_ADDR addr)
// OBSOLETE {
// OBSOLETE   return ((addr) & ~3);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Given a GDB frame, determine the address of the calling function's frame.
// OBSOLETE    This will be used to create a new GDB frame struct, and then
// OBSOLETE    INIT_EXTRA_FRAME_INFO and INIT_FRAME_PC will be called for the new frame.
// OBSOLETE 
// OBSOLETE    For us, the frame address is its stack pointer value, so we look up
// OBSOLETE    the function prologue to determine the caller's sp value, and return it.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE frame_chain (struct frame_info *thisframe)
// OBSOLETE {
// OBSOLETE 
// OBSOLETE   frame_find_saved_regs (thisframe, (struct frame_saved_regs *) 0);
// OBSOLETE   /* NOTE:  this depends on frame_find_saved_regs returning the VALUE, not
// OBSOLETE      the ADDRESS, of SP_REGNUM.  It also depends on the cache of
// OBSOLETE      frame_find_saved_regs results.  */
// OBSOLETE   if (thisframe->fsr->regs[SP_REGNUM])
// OBSOLETE     return thisframe->fsr->regs[SP_REGNUM];
// OBSOLETE   else
// OBSOLETE     return thisframe->frame;	/* Leaf fn -- next frame up has same SP. */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE frameless_function_invocation (struct frame_info *frame)
// OBSOLETE {
// OBSOLETE 
// OBSOLETE   frame_find_saved_regs (frame, (struct frame_saved_regs *) 0);
// OBSOLETE   /* NOTE:  this depends on frame_find_saved_regs returning the VALUE, not
// OBSOLETE      the ADDRESS, of SP_REGNUM.  It also depends on the cache of
// OBSOLETE      frame_find_saved_regs results.  */
// OBSOLETE   if (frame->fsr->regs[SP_REGNUM])
// OBSOLETE     return 0;			/* Frameful -- return addr saved somewhere */
// OBSOLETE   else
// OBSOLETE     return 1;			/* Frameless -- no saved return address */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE init_extra_frame_info (int fromleaf, struct frame_info *frame)
// OBSOLETE {
// OBSOLETE   frame->fsr = 0;		/* Not yet allocated */
// OBSOLETE   frame->args_pointer = 0;	/* Unknown */
// OBSOLETE   frame->locals_pointer = 0;	/* Unknown */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Examine an m88k function prologue, recording the addresses at which
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
// OBSOLETE    derived from examination of the source to gcc 1.95, particularly
// OBSOLETE    the routine output_prologue () in config/out-m88k.c.
// OBSOLETE 
// OBSOLETE    subu r31,r31,n                       # stack pointer update
// OBSOLETE 
// OBSOLETE    (st rn,r31,offset)?                  # save incoming regs
// OBSOLETE    (st.d rn,r31,offset)?
// OBSOLETE 
// OBSOLETE    (addu r30,r31,n)?                    # frame pointer update
// OBSOLETE 
// OBSOLETE    (pic sequence)?                      # PIC code prologue
// OBSOLETE 
// OBSOLETE    (or   rn,rm,0)?                      # Move parameters to other regs
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE /* Macros for extracting fields from instructions.  */
// OBSOLETE 
// OBSOLETE #define BITMASK(pos, width) (((0x1 << (width)) - 1) << (pos))
// OBSOLETE #define EXTRACT_FIELD(val, pos, width) ((val) >> (pos) & BITMASK (0, width))
// OBSOLETE #define	SUBU_OFFSET(x)	((unsigned)(x & 0xFFFF))
// OBSOLETE #define	ST_OFFSET(x)	((unsigned)((x) & 0xFFFF))
// OBSOLETE #define	ST_SRC(x)	EXTRACT_FIELD ((x), 21, 5)
// OBSOLETE #define	ADDU_OFFSET(x)	((unsigned)(x & 0xFFFF))
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * prologue_insn_tbl is a table of instructions which may comprise a
// OBSOLETE  * function prologue.  Associated with each table entry (corresponding
// OBSOLETE  * to a single instruction or group of instructions), is an action.
// OBSOLETE  * This action is used by examine_prologue (below) to determine
// OBSOLETE  * the state of certain machine registers and where the stack frame lives.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE enum prologue_insn_action
// OBSOLETE {
// OBSOLETE   PIA_SKIP,			/* don't care what the instruction does */
// OBSOLETE   PIA_NOTE_ST,			/* note register stored and where */
// OBSOLETE   PIA_NOTE_STD,			/* note pair of registers stored and where */
// OBSOLETE   PIA_NOTE_SP_ADJUSTMENT,	/* note stack pointer adjustment */
// OBSOLETE   PIA_NOTE_FP_ASSIGNMENT,	/* note frame pointer assignment */
// OBSOLETE   PIA_NOTE_PROLOGUE_END,	/* no more prologue */
// OBSOLETE };
// OBSOLETE 
// OBSOLETE struct prologue_insns
// OBSOLETE   {
// OBSOLETE     unsigned long insn;
// OBSOLETE     unsigned long mask;
// OBSOLETE     enum prologue_insn_action action;
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE struct prologue_insns prologue_insn_tbl[] =
// OBSOLETE {
// OBSOLETE   /* Various register move instructions */
// OBSOLETE   {0x58000000, 0xf800ffff, PIA_SKIP},	/* or/or.u with immed of 0 */
// OBSOLETE   {0xf4005800, 0xfc1fffe0, PIA_SKIP},	/* or rd, r0, rs */
// OBSOLETE   {0xf4005800, 0xfc00ffff, PIA_SKIP},	/* or rd, rs, r0 */
// OBSOLETE 
// OBSOLETE   /* Stack pointer setup: "subu sp, sp, n" where n is a multiple of 8 */
// OBSOLETE   {0x67ff0000, 0xffff0007, PIA_NOTE_SP_ADJUSTMENT},
// OBSOLETE 
// OBSOLETE   /* Frame pointer assignment: "addu r30, r31, n" */
// OBSOLETE   {0x63df0000, 0xffff0000, PIA_NOTE_FP_ASSIGNMENT},
// OBSOLETE 
// OBSOLETE   /* Store to stack instructions; either "st rx, sp, n" or "st.d rx, sp, n" */
// OBSOLETE   {0x241f0000, 0xfc1f0000, PIA_NOTE_ST},	/* st rx, sp, n */
// OBSOLETE   {0x201f0000, 0xfc1f0000, PIA_NOTE_STD},	/* st.d rs, sp, n */
// OBSOLETE 
// OBSOLETE   /* Instructions needed for setting up r25 for pic code. */
// OBSOLETE   {0x5f200000, 0xffff0000, PIA_SKIP},	/* or.u r25, r0, offset_high */
// OBSOLETE   {0xcc000002, 0xffffffff, PIA_SKIP},	/* bsr.n Lab */
// OBSOLETE   {0x5b390000, 0xffff0000, PIA_SKIP},	/* or r25, r25, offset_low */
// OBSOLETE   {0xf7396001, 0xffffffff, PIA_SKIP},	/* Lab: addu r25, r25, r1 */
// OBSOLETE 
// OBSOLETE   /* Various branch or jump instructions which have a delay slot -- these
// OBSOLETE      do not form part of the prologue, but the instruction in the delay
// OBSOLETE      slot might be a store instruction which should be noted. */
// OBSOLETE   {0xc4000000, 0xe4000000, PIA_NOTE_PROLOGUE_END},
// OBSOLETE 					/* br.n, bsr.n, bb0.n, or bb1.n */
// OBSOLETE   {0xec000000, 0xfc000000, PIA_NOTE_PROLOGUE_END},	/* bcnd.n */
// OBSOLETE   {0xf400c400, 0xfffff7e0, PIA_NOTE_PROLOGUE_END}	/* jmp.n or jsr.n */
// OBSOLETE 
// OBSOLETE };
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Fetch the instruction at ADDR, returning 0 if ADDR is beyond LIM or
// OBSOLETE    is not the address of a valid instruction, the address of the next
// OBSOLETE    instruction beyond ADDR otherwise.  *PWORD1 receives the first word
// OBSOLETE    of the instruction. */
// OBSOLETE 
// OBSOLETE #define NEXT_PROLOGUE_INSN(addr, lim, pword1) \
// OBSOLETE   (((addr) < (lim)) ? next_insn (addr, pword1) : 0)
// OBSOLETE 
// OBSOLETE /* Read the m88k instruction at 'memaddr' and return the address of 
// OBSOLETE    the next instruction after that, or 0 if 'memaddr' is not the
// OBSOLETE    address of a valid instruction.  The instruction
// OBSOLETE    is stored at 'pword1'.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE next_insn (CORE_ADDR memaddr, unsigned long *pword1)
// OBSOLETE {
// OBSOLETE   *pword1 = read_memory_integer (memaddr, BYTES_PER_88K_INSN);
// OBSOLETE   return memaddr + BYTES_PER_88K_INSN;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Read a register from frames called by us (or from the hardware regs).  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE read_next_frame_reg (struct frame_info *frame, int regno)
// OBSOLETE {
// OBSOLETE   for (; frame; frame = frame->next)
// OBSOLETE     {
// OBSOLETE       if (regno == SP_REGNUM)
// OBSOLETE 	return FRAME_FP (frame);
// OBSOLETE       else if (frame->fsr->regs[regno])
// OBSOLETE 	return read_memory_integer (frame->fsr->regs[regno], 4);
// OBSOLETE     }
// OBSOLETE   return read_register (regno);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Examine the prologue of a function.  `ip' points to the first instruction.
// OBSOLETE    `limit' is the limit of the prologue (e.g. the addr of the first 
// OBSOLETE    linenumber, or perhaps the program counter if we're stepping through).
// OBSOLETE    `frame_sp' is the stack pointer value in use in this frame.  
// OBSOLETE    `fsr' is a pointer to a frame_saved_regs structure into which we put
// OBSOLETE    info about the registers saved by this frame.  
// OBSOLETE    `fi' is a struct frame_info pointer; we fill in various fields in it
// OBSOLETE    to reflect the offsets of the arg pointer and the locals pointer.  */
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE examine_prologue (register CORE_ADDR ip, register CORE_ADDR limit,
// OBSOLETE 		  CORE_ADDR frame_sp, struct frame_saved_regs *fsr,
// OBSOLETE 		  struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   register CORE_ADDR next_ip;
// OBSOLETE   register int src;
// OBSOLETE   unsigned long insn;
// OBSOLETE   int size, offset;
// OBSOLETE   char must_adjust[32];		/* If set, must adjust offsets in fsr */
// OBSOLETE   int sp_offset = -1;		/* -1 means not set (valid must be mult of 8) */
// OBSOLETE   int fp_offset = -1;		/* -1 means not set */
// OBSOLETE   CORE_ADDR frame_fp;
// OBSOLETE   CORE_ADDR prologue_end = 0;
// OBSOLETE 
// OBSOLETE   memset (must_adjust, '\0', sizeof (must_adjust));
// OBSOLETE   next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn);
// OBSOLETE 
// OBSOLETE   while (next_ip)
// OBSOLETE     {
// OBSOLETE       struct prologue_insns *pip;
// OBSOLETE 
// OBSOLETE       for (pip = prologue_insn_tbl; (insn & pip->mask) != pip->insn;)
// OBSOLETE 	if (++pip >= prologue_insn_tbl + sizeof prologue_insn_tbl)
// OBSOLETE 	  goto end_of_prologue_found;	/* not a prologue insn */
// OBSOLETE 
// OBSOLETE       switch (pip->action)
// OBSOLETE 	{
// OBSOLETE 	case PIA_NOTE_ST:
// OBSOLETE 	case PIA_NOTE_STD:
// OBSOLETE 	  if (sp_offset != -1)
// OBSOLETE 	    {
// OBSOLETE 	      src = ST_SRC (insn);
// OBSOLETE 	      offset = ST_OFFSET (insn);
// OBSOLETE 	      must_adjust[src] = 1;
// OBSOLETE 	      fsr->regs[src++] = offset;	/* Will be adjusted later */
// OBSOLETE 	      if (pip->action == PIA_NOTE_STD && src < 32)
// OBSOLETE 		{
// OBSOLETE 		  offset += 4;
// OBSOLETE 		  must_adjust[src] = 1;
// OBSOLETE 		  fsr->regs[src++] = offset;
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    goto end_of_prologue_found;
// OBSOLETE 	  break;
// OBSOLETE 	case PIA_NOTE_SP_ADJUSTMENT:
// OBSOLETE 	  if (sp_offset == -1)
// OBSOLETE 	    sp_offset = -SUBU_OFFSET (insn);
// OBSOLETE 	  else
// OBSOLETE 	    goto end_of_prologue_found;
// OBSOLETE 	  break;
// OBSOLETE 	case PIA_NOTE_FP_ASSIGNMENT:
// OBSOLETE 	  if (fp_offset == -1)
// OBSOLETE 	    fp_offset = ADDU_OFFSET (insn);
// OBSOLETE 	  else
// OBSOLETE 	    goto end_of_prologue_found;
// OBSOLETE 	  break;
// OBSOLETE 	case PIA_NOTE_PROLOGUE_END:
// OBSOLETE 	  if (!prologue_end)
// OBSOLETE 	    prologue_end = ip;
// OBSOLETE 	  break;
// OBSOLETE 	case PIA_SKIP:
// OBSOLETE 	default:
// OBSOLETE 	  /* Do nothing */
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       ip = next_ip;
// OBSOLETE       next_ip = NEXT_PROLOGUE_INSN (ip, limit, &insn);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE end_of_prologue_found:
// OBSOLETE 
// OBSOLETE   if (prologue_end)
// OBSOLETE     ip = prologue_end;
// OBSOLETE 
// OBSOLETE   /* We're done with the prologue.  If we don't care about the stack
// OBSOLETE      frame itself, just return.  (Note that fsr->regs has been trashed,
// OBSOLETE      but the one caller who calls with fi==0 passes a dummy there.)  */
// OBSOLETE 
// OBSOLETE   if (fi == 0)
// OBSOLETE     return ip;
// OBSOLETE 
// OBSOLETE   /*
// OBSOLETE      OK, now we have:
// OBSOLETE 
// OBSOLETE      sp_offset  original (before any alloca calls) displacement of SP
// OBSOLETE      (will be negative).
// OBSOLETE 
// OBSOLETE      fp_offset  displacement from original SP to the FP for this frame
// OBSOLETE      or -1.
// OBSOLETE 
// OBSOLETE      fsr->regs[0..31]   displacement from original SP to the stack
// OBSOLETE      location where reg[0..31] is stored.
// OBSOLETE 
// OBSOLETE      must_adjust[0..31] set if corresponding offset was set.
// OBSOLETE 
// OBSOLETE      If alloca has been called between the function prologue and the current
// OBSOLETE      IP, then the current SP (frame_sp) will not be the original SP as set by
// OBSOLETE      the function prologue.  If the current SP is not the original SP, then the
// OBSOLETE      compiler will have allocated an FP for this frame, fp_offset will be set,
// OBSOLETE      and we can use it to calculate the original SP.
// OBSOLETE 
// OBSOLETE      Then, we figure out where the arguments and locals are, and relocate the
// OBSOLETE      offsets in fsr->regs to absolute addresses.  */
// OBSOLETE 
// OBSOLETE   if (fp_offset != -1)
// OBSOLETE     {
// OBSOLETE       /* We have a frame pointer, so get it, and base our calc's on it.  */
// OBSOLETE       frame_fp = (CORE_ADDR) read_next_frame_reg (fi->next, ACTUAL_FP_REGNUM);
// OBSOLETE       frame_sp = frame_fp - fp_offset;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* We have no frame pointer, therefore frame_sp is still the same value
// OBSOLETE          as set by prologue.  But where is the frame itself?  */
// OBSOLETE       if (must_adjust[SRP_REGNUM])
// OBSOLETE 	{
// OBSOLETE 	  /* Function header saved SRP (r1), the return address.  Frame starts
// OBSOLETE 	     4 bytes down from where it was saved.  */
// OBSOLETE 	  frame_fp = frame_sp + fsr->regs[SRP_REGNUM] - 4;
// OBSOLETE 	  fi->locals_pointer = frame_fp;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* Function header didn't save SRP (r1), so we are in a leaf fn or
// OBSOLETE 	     are otherwise confused.  */
// OBSOLETE 	  frame_fp = -1;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* The locals are relative to the FP (whether it exists as an allocated
// OBSOLETE      register, or just as an assumed offset from the SP) */
// OBSOLETE   fi->locals_pointer = frame_fp;
// OBSOLETE 
// OBSOLETE   /* The arguments are just above the SP as it was before we adjusted it
// OBSOLETE      on entry.  */
// OBSOLETE   fi->args_pointer = frame_sp - sp_offset;
// OBSOLETE 
// OBSOLETE   /* Now that we know the SP value used by the prologue, we know where
// OBSOLETE      it saved all the registers.  */
// OBSOLETE   for (src = 0; src < 32; src++)
// OBSOLETE     if (must_adjust[src])
// OBSOLETE       fsr->regs[src] += frame_sp;
// OBSOLETE 
// OBSOLETE   /* The saved value of the SP is always known.  */
// OBSOLETE   /* (we hope...) */
// OBSOLETE   if (fsr->regs[SP_REGNUM] != 0
// OBSOLETE       && fsr->regs[SP_REGNUM] != frame_sp - sp_offset)
// OBSOLETE     fprintf_unfiltered (gdb_stderr, "Bad saved SP value %lx != %lx, offset %x!\n",
// OBSOLETE 			fsr->regs[SP_REGNUM],
// OBSOLETE 			frame_sp - sp_offset, sp_offset);
// OBSOLETE 
// OBSOLETE   fsr->regs[SP_REGNUM] = frame_sp - sp_offset;
// OBSOLETE 
// OBSOLETE   return (ip);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Given an ip value corresponding to the start of a function,
// OBSOLETE    return the ip of the first instruction after the function 
// OBSOLETE    prologue.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE m88k_skip_prologue (CORE_ADDR ip)
// OBSOLETE {
// OBSOLETE   struct frame_saved_regs saved_regs_dummy;
// OBSOLETE   struct symtab_and_line sal;
// OBSOLETE   CORE_ADDR limit;
// OBSOLETE 
// OBSOLETE   sal = find_pc_line (ip, 0);
// OBSOLETE   limit = (sal.end) ? sal.end : 0xffffffff;
// OBSOLETE 
// OBSOLETE   return (examine_prologue (ip, limit, (CORE_ADDR) 0, &saved_regs_dummy,
// OBSOLETE 			    (struct frame_info *) 0));
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
// OBSOLETE          has executed already.  In the case where the PC is not in
// OBSOLETE          the function prologue, we set limit to two instructions beyond
// OBSOLETE          where the prologue ends in case if any of the prologue instructions
// OBSOLETE          were moved into a delay slot of a branch instruction. */
// OBSOLETE 
// OBSOLETE       ip = get_pc_function_start (fi->pc);
// OBSOLETE       sal = find_pc_line (ip, 0);
// OBSOLETE       limit = (sal.end && sal.end < fi->pc) ? sal.end + 2 * BYTES_PER_88K_INSN
// OBSOLETE 	: fi->pc;
// OBSOLETE 
// OBSOLETE       /* This will fill in fields in *fi as well as in cache_fsr.  */
// OBSOLETE #ifdef SIGTRAMP_FRAME_FIXUP
// OBSOLETE       if (fi->signal_handler_caller)
// OBSOLETE 	SIGTRAMP_FRAME_FIXUP (fi->frame);
// OBSOLETE #endif
// OBSOLETE       examine_prologue (ip, limit, fi->frame, cache_fsr, fi);
// OBSOLETE #ifdef SIGTRAMP_SP_FIXUP
// OBSOLETE       if (fi->signal_handler_caller && fi->fsr->regs[SP_REGNUM])
// OBSOLETE 	SIGTRAMP_SP_FIXUP (fi->fsr->regs[SP_REGNUM]);
// OBSOLETE #endif
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (fsr)
// OBSOLETE     *fsr = *fi->fsr;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the address of the locals block for the frame
// OBSOLETE    described by FI.  Returns 0 if the address is unknown.
// OBSOLETE    NOTE!  Frame locals are referred to by negative offsets from the
// OBSOLETE    argument pointer, so this is the same as frame_args_address().  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE frame_locals_address (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   struct frame_saved_regs fsr;
// OBSOLETE 
// OBSOLETE   if (fi->args_pointer)		/* Cached value is likely there.  */
// OBSOLETE     return fi->args_pointer;
// OBSOLETE 
// OBSOLETE   /* Nope, generate it.  */
// OBSOLETE 
// OBSOLETE   get_frame_saved_regs (fi, &fsr);
// OBSOLETE 
// OBSOLETE   return fi->args_pointer;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the address of the argument block for the frame
// OBSOLETE    described by FI.  Returns 0 if the address is unknown.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE frame_args_address (struct frame_info *fi)
// OBSOLETE {
// OBSOLETE   struct frame_saved_regs fsr;
// OBSOLETE 
// OBSOLETE   if (fi->args_pointer)		/* Cached value is likely there.  */
// OBSOLETE     return fi->args_pointer;
// OBSOLETE 
// OBSOLETE   /* Nope, generate it.  */
// OBSOLETE 
// OBSOLETE   get_frame_saved_regs (fi, &fsr);
// OBSOLETE 
// OBSOLETE   return fi->args_pointer;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the saved PC from this frame.
// OBSOLETE 
// OBSOLETE    If the frame has a memory copy of SRP_REGNUM, use that.  If not,
// OBSOLETE    just use the register SRP_REGNUM itself.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE frame_saved_pc (struct frame_info *frame)
// OBSOLETE {
// OBSOLETE   return read_next_frame_reg (frame, SRP_REGNUM);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE #define DUMMY_FRAME_SIZE 192
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE write_word (CORE_ADDR sp, ULONGEST word)
// OBSOLETE {
// OBSOLETE   register int len = REGISTER_SIZE;
// OBSOLETE   char buffer[MAX_REGISTER_RAW_SIZE];
// OBSOLETE 
// OBSOLETE   store_unsigned_integer (buffer, len, word);
// OBSOLETE   write_memory (sp, buffer, len);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE m88k_push_dummy_frame (void)
// OBSOLETE {
// OBSOLETE   register CORE_ADDR sp = read_register (SP_REGNUM);
// OBSOLETE   register int rn;
// OBSOLETE   int offset;
// OBSOLETE 
// OBSOLETE   sp -= DUMMY_FRAME_SIZE;	/* allocate a bunch of space */
// OBSOLETE 
// OBSOLETE   for (rn = 0, offset = 0; rn <= SP_REGNUM; rn++, offset += 4)
// OBSOLETE     write_word (sp + offset, read_register (rn));
// OBSOLETE 
// OBSOLETE   write_word (sp + offset, read_register (SXIP_REGNUM));
// OBSOLETE   offset += 4;
// OBSOLETE 
// OBSOLETE   write_word (sp + offset, read_register (SNIP_REGNUM));
// OBSOLETE   offset += 4;
// OBSOLETE 
// OBSOLETE   write_word (sp + offset, read_register (SFIP_REGNUM));
// OBSOLETE   offset += 4;
// OBSOLETE 
// OBSOLETE   write_word (sp + offset, read_register (PSR_REGNUM));
// OBSOLETE   offset += 4;
// OBSOLETE 
// OBSOLETE   write_word (sp + offset, read_register (FPSR_REGNUM));
// OBSOLETE   offset += 4;
// OBSOLETE 
// OBSOLETE   write_word (sp + offset, read_register (FPCR_REGNUM));
// OBSOLETE   offset += 4;
// OBSOLETE 
// OBSOLETE   write_register (SP_REGNUM, sp);
// OBSOLETE   write_register (ACTUAL_FP_REGNUM, sp);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE pop_frame (void)
// OBSOLETE {
// OBSOLETE   register struct frame_info *frame = get_current_frame ();
// OBSOLETE   register int regnum;
// OBSOLETE   struct frame_saved_regs fsr;
// OBSOLETE 
// OBSOLETE   get_frame_saved_regs (frame, &fsr);
// OBSOLETE 
// OBSOLETE   if (PC_IN_CALL_DUMMY (read_pc (), read_register (SP_REGNUM), frame->frame))
// OBSOLETE     {
// OBSOLETE       /* FIXME: I think get_frame_saved_regs should be handling this so
// OBSOLETE          that we can deal with the saved registers properly (e.g. frame
// OBSOLETE          1 is a call dummy, the user types "frame 2" and then "print $ps").  */
// OBSOLETE       register CORE_ADDR sp = read_register (ACTUAL_FP_REGNUM);
// OBSOLETE       int offset;
// OBSOLETE 
// OBSOLETE       for (regnum = 0, offset = 0; regnum <= SP_REGNUM; regnum++, offset += 4)
// OBSOLETE 	(void) write_register (regnum, read_memory_integer (sp + offset, 4));
// OBSOLETE 
// OBSOLETE       write_register (SXIP_REGNUM, read_memory_integer (sp + offset, 4));
// OBSOLETE       offset += 4;
// OBSOLETE 
// OBSOLETE       write_register (SNIP_REGNUM, read_memory_integer (sp + offset, 4));
// OBSOLETE       offset += 4;
// OBSOLETE 
// OBSOLETE       write_register (SFIP_REGNUM, read_memory_integer (sp + offset, 4));
// OBSOLETE       offset += 4;
// OBSOLETE 
// OBSOLETE       write_register (PSR_REGNUM, read_memory_integer (sp + offset, 4));
// OBSOLETE       offset += 4;
// OBSOLETE 
// OBSOLETE       write_register (FPSR_REGNUM, read_memory_integer (sp + offset, 4));
// OBSOLETE       offset += 4;
// OBSOLETE 
// OBSOLETE       write_register (FPCR_REGNUM, read_memory_integer (sp + offset, 4));
// OBSOLETE       offset += 4;
// OBSOLETE 
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       for (regnum = FP_REGNUM; regnum > 0; regnum--)
// OBSOLETE 	if (fsr.regs[regnum])
// OBSOLETE 	  write_register (regnum,
// OBSOLETE 			  read_memory_integer (fsr.regs[regnum], 4));
// OBSOLETE       write_pc (frame_saved_pc (frame));
// OBSOLETE     }
// OBSOLETE   reinit_frame_cache ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_m88k_tdep (void)
// OBSOLETE {
// OBSOLETE   tm_print_insn = print_insn_m88k;
// OBSOLETE }
