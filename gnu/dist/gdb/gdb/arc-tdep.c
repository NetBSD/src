// OBSOLETE /* ARC target-dependent stuff.
// OBSOLETE    Copyright 1995, 1996, 1999, 2000, 2001 Free Software Foundation, Inc.
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
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "target.h"
// OBSOLETE #include "floatformat.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "gdbcmd.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE #include "gdb_string.h"
// OBSOLETE 
// OBSOLETE /* Local functions */
// OBSOLETE 
// OBSOLETE static int arc_set_cpu_type (char *str);
// OBSOLETE 
// OBSOLETE /* Current CPU, set with the "set cpu" command.  */
// OBSOLETE static int arc_bfd_mach_type;
// OBSOLETE char *arc_cpu_type;
// OBSOLETE char *tmp_arc_cpu_type;
// OBSOLETE 
// OBSOLETE /* Table of cpu names.  */
// OBSOLETE struct
// OBSOLETE   {
// OBSOLETE     char *name;
// OBSOLETE     int value;
// OBSOLETE   }
// OBSOLETE arc_cpu_type_table[] =
// OBSOLETE {
// OBSOLETE   { "arc5", bfd_mach_arc_5 },
// OBSOLETE   { "arc6", bfd_mach_arc_6 },
// OBSOLETE   { "arc7", bfd_mach_arc_7 },
// OBSOLETE   { "arc8", bfd_mach_arc_8 },
// OBSOLETE   {  NULL,  0 }
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Used by simulator.  */
// OBSOLETE int display_pipeline_p;
// OBSOLETE int cpu_timer;
// OBSOLETE /* This one must have the same type as used in the emulator.
// OBSOLETE    It's currently an enum so this should be ok for now.  */
// OBSOLETE int debug_pipeline_p;
// OBSOLETE 
// OBSOLETE #define ARC_CALL_SAVED_REG(r) ((r) >= 16 && (r) < 24)
// OBSOLETE 
// OBSOLETE #define OPMASK	0xf8000000
// OBSOLETE 
// OBSOLETE /* Instruction field accessor macros.
// OBSOLETE    See the Programmer's Reference Manual.  */
// OBSOLETE #define X_OP(i) (((i) >> 27) & 0x1f)
// OBSOLETE #define X_A(i) (((i) >> 21) & 0x3f)
// OBSOLETE #define X_B(i) (((i) >> 15) & 0x3f)
// OBSOLETE #define X_C(i) (((i) >> 9) & 0x3f)
// OBSOLETE #define X_D(i) ((((i) & 0x1ff) ^ 0x100) - 0x100)
// OBSOLETE #define X_L(i) (((((i) >> 5) & 0x3ffffc) ^ 0x200000) - 0x200000)
// OBSOLETE #define X_N(i) (((i) >> 5) & 3)
// OBSOLETE #define X_Q(i) ((i) & 0x1f)
// OBSOLETE 
// OBSOLETE /* Return non-zero if X is a short immediate data indicator.  */
// OBSOLETE #define SHIMM_P(x) ((x) == 61 || (x) == 63)
// OBSOLETE 
// OBSOLETE /* Return non-zero if X is a "long" (32 bit) immediate data indicator.  */
// OBSOLETE #define LIMM_P(x) ((x) == 62)
// OBSOLETE 
// OBSOLETE /* Build a simple instruction.  */
// OBSOLETE #define BUILD_INSN(op, a, b, c, d) \
// OBSOLETE   ((((op) & 31) << 27) \
// OBSOLETE    | (((a) & 63) << 21) \
// OBSOLETE    | (((b) & 63) << 15) \
// OBSOLETE    | (((c) & 63) << 9) \
// OBSOLETE    | ((d) & 511))
// OBSOLETE 
// OBSOLETE /* Codestream stuff.  */
// OBSOLETE static void codestream_read (unsigned int *, int);
// OBSOLETE static void codestream_seek (CORE_ADDR);
// OBSOLETE static unsigned int codestream_fill (int);
// OBSOLETE 
// OBSOLETE #define CODESTREAM_BUFSIZ 16
// OBSOLETE static CORE_ADDR codestream_next_addr;
// OBSOLETE static CORE_ADDR codestream_addr;
// OBSOLETE /* FIXME assumes sizeof (int) == 32? */
// OBSOLETE static unsigned int codestream_buf[CODESTREAM_BUFSIZ];
// OBSOLETE static int codestream_off;
// OBSOLETE static int codestream_cnt;
// OBSOLETE 
// OBSOLETE #define codestream_tell() \
// OBSOLETE   (codestream_addr + codestream_off * sizeof (codestream_buf[0]))
// OBSOLETE #define codestream_peek() \
// OBSOLETE   (codestream_cnt == 0 \
// OBSOLETE    ? codestream_fill (1) \
// OBSOLETE    : codestream_buf[codestream_off])
// OBSOLETE #define codestream_get() \
// OBSOLETE   (codestream_cnt-- == 0 \
// OBSOLETE    ? codestream_fill (0) \
// OBSOLETE    : codestream_buf[codestream_off++])
// OBSOLETE 
// OBSOLETE static unsigned int
// OBSOLETE codestream_fill (int peek_flag)
// OBSOLETE {
// OBSOLETE   codestream_addr = codestream_next_addr;
// OBSOLETE   codestream_next_addr += CODESTREAM_BUFSIZ * sizeof (codestream_buf[0]);
// OBSOLETE   codestream_off = 0;
// OBSOLETE   codestream_cnt = CODESTREAM_BUFSIZ;
// OBSOLETE   read_memory (codestream_addr, (char *) codestream_buf,
// OBSOLETE 	       CODESTREAM_BUFSIZ * sizeof (codestream_buf[0]));
// OBSOLETE   /* FIXME: check return code?  */
// OBSOLETE 
// OBSOLETE 
// OBSOLETE   /* Handle byte order differences -> convert to host byte ordering.  */
// OBSOLETE   {
// OBSOLETE     int i;
// OBSOLETE     for (i = 0; i < CODESTREAM_BUFSIZ; i++)
// OBSOLETE       codestream_buf[i] =
// OBSOLETE 	extract_unsigned_integer (&codestream_buf[i],
// OBSOLETE 				  sizeof (codestream_buf[i]));
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   if (peek_flag)
// OBSOLETE     return codestream_peek ();
// OBSOLETE   else
// OBSOLETE     return codestream_get ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE codestream_seek (CORE_ADDR place)
// OBSOLETE {
// OBSOLETE   codestream_next_addr = place / CODESTREAM_BUFSIZ;
// OBSOLETE   codestream_next_addr *= CODESTREAM_BUFSIZ;
// OBSOLETE   codestream_cnt = 0;
// OBSOLETE   codestream_fill (1);
// OBSOLETE   while (codestream_tell () != place)
// OBSOLETE     codestream_get ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* This function is currently unused but leave in for now.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE codestream_read (unsigned int *buf, int count)
// OBSOLETE {
// OBSOLETE   unsigned int *p;
// OBSOLETE   int i;
// OBSOLETE   p = buf;
// OBSOLETE   for (i = 0; i < count; i++)
// OBSOLETE     *p++ = codestream_get ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Set up prologue scanning and return the first insn.  */
// OBSOLETE 
// OBSOLETE static unsigned int
// OBSOLETE setup_prologue_scan (CORE_ADDR pc)
// OBSOLETE {
// OBSOLETE   unsigned int insn;
// OBSOLETE 
// OBSOLETE   codestream_seek (pc);
// OBSOLETE   insn = codestream_get ();
// OBSOLETE 
// OBSOLETE   return insn;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Find & return amount a local space allocated, and advance codestream to
// OBSOLETE  * first register push (if any).
// OBSOLETE  * If entry sequence doesn't make sense, return -1, and leave 
// OBSOLETE  * codestream pointer random.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static long
// OBSOLETE arc_get_frame_setup (CORE_ADDR pc)
// OBSOLETE {
// OBSOLETE   unsigned int insn;
// OBSOLETE   /* Size of frame or -1 if unrecognizable prologue.  */
// OBSOLETE   int frame_size = -1;
// OBSOLETE   /* An initial "sub sp,sp,N" may or may not be for a stdarg fn.  */
// OBSOLETE   int maybe_stdarg_decr = -1;
// OBSOLETE 
// OBSOLETE   insn = setup_prologue_scan (pc);
// OBSOLETE 
// OBSOLETE   /* The authority for what appears here is the home-grown ABI.
// OBSOLETE      The most recent version is 1.2.  */
// OBSOLETE 
// OBSOLETE   /* First insn may be "sub sp,sp,N" if stdarg fn.  */
// OBSOLETE   if ((insn & BUILD_INSN (-1, -1, -1, -1, 0))
// OBSOLETE       == BUILD_INSN (10, SP_REGNUM, SP_REGNUM, SHIMM_REGNUM, 0))
// OBSOLETE     {
// OBSOLETE       maybe_stdarg_decr = X_D (insn);
// OBSOLETE       insn = codestream_get ();
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if ((insn & BUILD_INSN (-1, 0, -1, -1, -1))	/* st blink,[sp,4] */
// OBSOLETE       == BUILD_INSN (2, 0, SP_REGNUM, BLINK_REGNUM, 4))
// OBSOLETE     {
// OBSOLETE       insn = codestream_get ();
// OBSOLETE       /* Frame may not be necessary, even though blink is saved.
// OBSOLETE          At least this is something we recognize.  */
// OBSOLETE       frame_size = 0;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if ((insn & BUILD_INSN (-1, 0, -1, -1, -1))	/* st fp,[sp] */
// OBSOLETE       == BUILD_INSN (2, 0, SP_REGNUM, FP_REGNUM, 0))
// OBSOLETE     {
// OBSOLETE       insn = codestream_get ();
// OBSOLETE       if ((insn & BUILD_INSN (-1, -1, -1, -1, 0))
// OBSOLETE 	  != BUILD_INSN (12, FP_REGNUM, SP_REGNUM, SP_REGNUM, 0))
// OBSOLETE 	return -1;
// OBSOLETE 
// OBSOLETE       /* Check for stack adjustment sub sp,sp,N.  */
// OBSOLETE       insn = codestream_peek ();
// OBSOLETE       if ((insn & BUILD_INSN (-1, -1, -1, 0, 0))
// OBSOLETE 	  == BUILD_INSN (10, SP_REGNUM, SP_REGNUM, 0, 0))
// OBSOLETE 	{
// OBSOLETE 	  if (LIMM_P (X_C (insn)))
// OBSOLETE 	    frame_size = codestream_get ();
// OBSOLETE 	  else if (SHIMM_P (X_C (insn)))
// OBSOLETE 	    frame_size = X_D (insn);
// OBSOLETE 	  else
// OBSOLETE 	    return -1;
// OBSOLETE 	  if (frame_size < 0)
// OBSOLETE 	    return -1;
// OBSOLETE 
// OBSOLETE 	  codestream_get ();
// OBSOLETE 
// OBSOLETE 	  /* This sequence is used to get the address of the return
// OBSOLETE 	     buffer for a function that returns a structure.  */
// OBSOLETE 	  insn = codestream_peek ();
// OBSOLETE 	  if ((insn & OPMASK) == 0x60000000)
// OBSOLETE 	    codestream_get ();
// OBSOLETE 	}
// OBSOLETE       /* Frameless fn.  */
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  frame_size = 0;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* If we found a "sub sp,sp,N" and nothing else, it may or may not be a
// OBSOLETE      stdarg fn.  The stdarg decrement is not treated as part of the frame size,
// OBSOLETE      so we have a dilemma: what do we return?  For now, if we get a
// OBSOLETE      "sub sp,sp,N" and nothing else assume this isn't a stdarg fn.  One way
// OBSOLETE      to fix this completely would be to add a bit to the function descriptor
// OBSOLETE      that says the function is a stdarg function.  */
// OBSOLETE 
// OBSOLETE   if (frame_size < 0 && maybe_stdarg_decr > 0)
// OBSOLETE     return maybe_stdarg_decr;
// OBSOLETE   return frame_size;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Given a pc value, skip it forward past the function prologue by
// OBSOLETE    disassembling instructions that appear to be a prologue.
// OBSOLETE 
// OBSOLETE    If FRAMELESS_P is set, we are only testing to see if the function
// OBSOLETE    is frameless.  If it is a frameless function, return PC unchanged.
// OBSOLETE    This allows a quicker answer.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE arc_skip_prologue (CORE_ADDR pc, int frameless_p)
// OBSOLETE {
// OBSOLETE   unsigned int insn;
// OBSOLETE   int i, frame_size;
// OBSOLETE 
// OBSOLETE   if ((frame_size = arc_get_frame_setup (pc)) < 0)
// OBSOLETE     return (pc);
// OBSOLETE 
// OBSOLETE   if (frameless_p)
// OBSOLETE     return frame_size == 0 ? pc : codestream_tell ();
// OBSOLETE 
// OBSOLETE   /* Skip over register saves.  */
// OBSOLETE   for (i = 0; i < 8; i++)
// OBSOLETE     {
// OBSOLETE       insn = codestream_peek ();
// OBSOLETE       if ((insn & BUILD_INSN (-1, 0, -1, 0, 0))
// OBSOLETE 	  != BUILD_INSN (2, 0, SP_REGNUM, 0, 0))
// OBSOLETE 	break;			/* not st insn */
// OBSOLETE       if (!ARC_CALL_SAVED_REG (X_C (insn)))
// OBSOLETE 	break;
// OBSOLETE       codestream_get ();
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return codestream_tell ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Is the prologue at PC frameless?  */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE arc_prologue_frameless_p (CORE_ADDR pc)
// OBSOLETE {
// OBSOLETE   return (pc == arc_skip_prologue (pc, 1));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the return address for a frame.
// OBSOLETE    This is used to implement FRAME_SAVED_PC.
// OBSOLETE    This is taken from frameless_look_for_prologue.  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE arc_frame_saved_pc (struct frame_info *frame)
// OBSOLETE {
// OBSOLETE   CORE_ADDR func_start;
// OBSOLETE   unsigned int insn;
// OBSOLETE 
// OBSOLETE   func_start = get_pc_function_start (frame->pc) + FUNCTION_START_OFFSET;
// OBSOLETE   if (func_start == 0)
// OBSOLETE     {
// OBSOLETE       /* Best guess.  */
// OBSOLETE       return ARC_PC_TO_REAL_ADDRESS (read_memory_integer (FRAME_FP (frame) + 4, 4));
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* The authority for what appears here is the home-grown ABI.
// OBSOLETE      The most recent version is 1.2.  */
// OBSOLETE 
// OBSOLETE   insn = setup_prologue_scan (func_start);
// OBSOLETE 
// OBSOLETE   /* First insn may be "sub sp,sp,N" if stdarg fn.  */
// OBSOLETE   if ((insn & BUILD_INSN (-1, -1, -1, -1, 0))
// OBSOLETE       == BUILD_INSN (10, SP_REGNUM, SP_REGNUM, SHIMM_REGNUM, 0))
// OBSOLETE     insn = codestream_get ();
// OBSOLETE 
// OBSOLETE   /* If the next insn is "st blink,[sp,4]" we can get blink from there.
// OBSOLETE      Otherwise this is a leaf function and we can use blink.  Note that
// OBSOLETE      this still allows for the case where a leaf function saves/clobbers/
// OBSOLETE      restores blink.  */
// OBSOLETE 
// OBSOLETE   if ((insn & BUILD_INSN (-1, 0, -1, -1, -1))	/* st blink,[sp,4] */
// OBSOLETE       != BUILD_INSN (2, 0, SP_REGNUM, BLINK_REGNUM, 4))
// OBSOLETE     return ARC_PC_TO_REAL_ADDRESS (read_register (BLINK_REGNUM));
// OBSOLETE   else
// OBSOLETE     return ARC_PC_TO_REAL_ADDRESS (read_memory_integer (FRAME_FP (frame) + 4, 4));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Parse the first few instructions of the function to see
// OBSOLETE  * what registers were stored.
// OBSOLETE  *
// OBSOLETE  * The startup sequence can be at the start of the function.
// OBSOLETE  * 'st blink,[sp+4], st fp,[sp], mov fp,sp' 
// OBSOLETE  *
// OBSOLETE  * Local space is allocated just below by sub sp,sp,nnn.
// OBSOLETE  * Next, the registers used by this function are stored (as offsets from sp).
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE frame_find_saved_regs (struct frame_info *fip, struct frame_saved_regs *fsrp)
// OBSOLETE {
// OBSOLETE   long locals;
// OBSOLETE   unsigned int insn;
// OBSOLETE   CORE_ADDR dummy_bottom;
// OBSOLETE   CORE_ADDR adr;
// OBSOLETE   int i, regnum, offset;
// OBSOLETE 
// OBSOLETE   memset (fsrp, 0, sizeof *fsrp);
// OBSOLETE 
// OBSOLETE   /* If frame is the end of a dummy, compute where the beginning would be.  */
// OBSOLETE   dummy_bottom = fip->frame - 4 - REGISTER_BYTES - CALL_DUMMY_LENGTH;
// OBSOLETE 
// OBSOLETE   /* Check if the PC is in the stack, in a dummy frame.  */
// OBSOLETE   if (dummy_bottom <= fip->pc && fip->pc <= fip->frame)
// OBSOLETE     {
// OBSOLETE       /* all regs were saved by push_call_dummy () */
// OBSOLETE       adr = fip->frame;
// OBSOLETE       for (i = 0; i < NUM_REGS; i++)
// OBSOLETE 	{
// OBSOLETE 	  adr -= REGISTER_RAW_SIZE (i);
// OBSOLETE 	  fsrp->regs[i] = adr;
// OBSOLETE 	}
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   locals = arc_get_frame_setup (get_pc_function_start (fip->pc));
// OBSOLETE 
// OBSOLETE   if (locals >= 0)
// OBSOLETE     {
// OBSOLETE       /* Set `adr' to the value of `sp'.  */
// OBSOLETE       adr = fip->frame - locals;
// OBSOLETE       for (i = 0; i < 8; i++)
// OBSOLETE 	{
// OBSOLETE 	  insn = codestream_get ();
// OBSOLETE 	  if ((insn & BUILD_INSN (-1, 0, -1, 0, 0))
// OBSOLETE 	      != BUILD_INSN (2, 0, SP_REGNUM, 0, 0))
// OBSOLETE 	    break;
// OBSOLETE 	  regnum = X_C (insn);
// OBSOLETE 	  offset = X_D (insn);
// OBSOLETE 	  fsrp->regs[regnum] = adr + offset;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   fsrp->regs[PC_REGNUM] = fip->frame + 4;
// OBSOLETE   fsrp->regs[FP_REGNUM] = fip->frame;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE arc_push_dummy_frame (void)
// OBSOLETE {
// OBSOLETE   CORE_ADDR sp = read_register (SP_REGNUM);
// OBSOLETE   int regnum;
// OBSOLETE   char regbuf[MAX_REGISTER_RAW_SIZE];
// OBSOLETE 
// OBSOLETE   read_register_gen (PC_REGNUM, regbuf);
// OBSOLETE   write_memory (sp + 4, regbuf, REGISTER_SIZE);
// OBSOLETE   read_register_gen (FP_REGNUM, regbuf);
// OBSOLETE   write_memory (sp, regbuf, REGISTER_SIZE);
// OBSOLETE   write_register (FP_REGNUM, sp);
// OBSOLETE   for (regnum = 0; regnum < NUM_REGS; regnum++)
// OBSOLETE     {
// OBSOLETE       read_register_gen (regnum, regbuf);
// OBSOLETE       sp = push_bytes (sp, regbuf, REGISTER_RAW_SIZE (regnum));
// OBSOLETE     }
// OBSOLETE   sp += (2 * REGISTER_SIZE);
// OBSOLETE   write_register (SP_REGNUM, sp);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE arc_pop_frame (void)
// OBSOLETE {
// OBSOLETE   struct frame_info *frame = get_current_frame ();
// OBSOLETE   CORE_ADDR fp;
// OBSOLETE   int regnum;
// OBSOLETE   struct frame_saved_regs fsr;
// OBSOLETE   char regbuf[MAX_REGISTER_RAW_SIZE];
// OBSOLETE 
// OBSOLETE   fp = FRAME_FP (frame);
// OBSOLETE   get_frame_saved_regs (frame, &fsr);
// OBSOLETE   for (regnum = 0; regnum < NUM_REGS; regnum++)
// OBSOLETE     {
// OBSOLETE       CORE_ADDR adr;
// OBSOLETE       adr = fsr.regs[regnum];
// OBSOLETE       if (adr)
// OBSOLETE 	{
// OBSOLETE 	  read_memory (adr, regbuf, REGISTER_RAW_SIZE (regnum));
// OBSOLETE 	  write_register_bytes (REGISTER_BYTE (regnum), regbuf,
// OBSOLETE 				REGISTER_RAW_SIZE (regnum));
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   write_register (FP_REGNUM, read_memory_integer (fp, 4));
// OBSOLETE   write_register (PC_REGNUM, read_memory_integer (fp + 4, 4));
// OBSOLETE   write_register (SP_REGNUM, fp + 8);
// OBSOLETE   flush_cached_frames ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Simulate single-step.  */
// OBSOLETE 
// OBSOLETE typedef enum
// OBSOLETE {
// OBSOLETE   NORMAL4,			/* a normal 4 byte insn */
// OBSOLETE   NORMAL8,			/* a normal 8 byte insn */
// OBSOLETE   BRANCH4,			/* a 4 byte branch insn, including ones without delay slots */
// OBSOLETE   BRANCH8,			/* an 8 byte branch insn, including ones with delay slots */
// OBSOLETE }
// OBSOLETE insn_type;
// OBSOLETE 
// OBSOLETE /* Return the type of INSN and store in TARGET the destination address of a
// OBSOLETE    branch if this is one.  */
// OBSOLETE /* ??? Need to verify all cases are properly handled.  */
// OBSOLETE 
// OBSOLETE static insn_type
// OBSOLETE get_insn_type (unsigned long insn, CORE_ADDR pc, CORE_ADDR *target)
// OBSOLETE {
// OBSOLETE   unsigned long limm;
// OBSOLETE 
// OBSOLETE   switch (insn >> 27)
// OBSOLETE     {
// OBSOLETE     case 0:
// OBSOLETE     case 1:
// OBSOLETE     case 2:			/* load/store insns */
// OBSOLETE       if (LIMM_P (X_A (insn))
// OBSOLETE 	  || LIMM_P (X_B (insn))
// OBSOLETE 	  || LIMM_P (X_C (insn)))
// OBSOLETE 	return NORMAL8;
// OBSOLETE       return NORMAL4;
// OBSOLETE     case 4:
// OBSOLETE     case 5:
// OBSOLETE     case 6:			/* branch insns */
// OBSOLETE       *target = pc + 4 + X_L (insn);
// OBSOLETE       /* ??? It isn't clear that this is always the right answer.
// OBSOLETE          The problem occurs when the next insn is an 8 byte insn.  If the
// OBSOLETE          branch is conditional there's no worry as there shouldn't be an 8
// OBSOLETE          byte insn following.  The programmer may be cheating if s/he knows
// OBSOLETE          the branch will never be taken, but we don't deal with that.
// OBSOLETE          Note that the programmer is also allowed to play games by putting
// OBSOLETE          an insn with long immediate data in the delay slot and then duplicate
// OBSOLETE          the long immediate data at the branch target.  Ugh!  */
// OBSOLETE       if (X_N (insn) == 0)
// OBSOLETE 	return BRANCH4;
// OBSOLETE       return BRANCH8;
// OBSOLETE     case 7:			/* jump insns */
// OBSOLETE       if (LIMM_P (X_B (insn)))
// OBSOLETE 	{
// OBSOLETE 	  limm = read_memory_integer (pc + 4, 4);
// OBSOLETE 	  *target = ARC_PC_TO_REAL_ADDRESS (limm);
// OBSOLETE 	  return BRANCH8;
// OBSOLETE 	}
// OBSOLETE       if (SHIMM_P (X_B (insn)))
// OBSOLETE 	*target = ARC_PC_TO_REAL_ADDRESS (X_D (insn));
// OBSOLETE       else
// OBSOLETE 	*target = ARC_PC_TO_REAL_ADDRESS (read_register (X_B (insn)));
// OBSOLETE       if (X_Q (insn) == 0 && X_N (insn) == 0)
// OBSOLETE 	return BRANCH4;
// OBSOLETE       return BRANCH8;
// OBSOLETE     default:			/* arithmetic insns, etc. */
// OBSOLETE       if (LIMM_P (X_A (insn))
// OBSOLETE 	  || LIMM_P (X_B (insn))
// OBSOLETE 	  || LIMM_P (X_C (insn)))
// OBSOLETE 	return NORMAL8;
// OBSOLETE       return NORMAL4;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* single_step() is called just before we want to resume the inferior, if we
// OBSOLETE    want to single-step it but there is no hardware or kernel single-step
// OBSOLETE    support.  We find all the possible targets of the coming instruction and
// OBSOLETE    breakpoint them.
// OBSOLETE 
// OBSOLETE    single_step is also called just after the inferior stops.  If we had
// OBSOLETE    set up a simulated single-step, we undo our damage.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE arc_software_single_step (enum target_signal ignore,	/* sig but we don't need it */
// OBSOLETE 			  int insert_breakpoints_p)
// OBSOLETE {
// OBSOLETE   static CORE_ADDR next_pc, target;
// OBSOLETE   static int brktrg_p;
// OBSOLETE   typedef char binsn_quantum[BREAKPOINT_MAX];
// OBSOLETE   static binsn_quantum break_mem[2];
// OBSOLETE 
// OBSOLETE   if (insert_breakpoints_p)
// OBSOLETE     {
// OBSOLETE       insn_type type;
// OBSOLETE       CORE_ADDR pc;
// OBSOLETE       unsigned long insn;
// OBSOLETE 
// OBSOLETE       pc = read_register (PC_REGNUM);
// OBSOLETE       insn = read_memory_integer (pc, 4);
// OBSOLETE       type = get_insn_type (insn, pc, &target);
// OBSOLETE 
// OBSOLETE       /* Always set a breakpoint for the insn after the branch.  */
// OBSOLETE       next_pc = pc + ((type == NORMAL8 || type == BRANCH8) ? 8 : 4);
// OBSOLETE       target_insert_breakpoint (next_pc, break_mem[0]);
// OBSOLETE 
// OBSOLETE       brktrg_p = 0;
// OBSOLETE 
// OBSOLETE       if ((type == BRANCH4 || type == BRANCH8)
// OBSOLETE       /* Watch out for branches to the following location.
// OBSOLETE          We just stored a breakpoint there and another call to
// OBSOLETE          target_insert_breakpoint will think the real insn is the
// OBSOLETE          breakpoint we just stored there.  */
// OBSOLETE 	  && target != next_pc)
// OBSOLETE 	{
// OBSOLETE 	  brktrg_p = 1;
// OBSOLETE 	  target_insert_breakpoint (target, break_mem[1]);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* Remove breakpoints.  */
// OBSOLETE       target_remove_breakpoint (next_pc, break_mem[0]);
// OBSOLETE 
// OBSOLETE       if (brktrg_p)
// OBSOLETE 	target_remove_breakpoint (target, break_mem[1]);
// OBSOLETE 
// OBSOLETE       /* Fix the pc.  */
// OBSOLETE       stop_pc -= DECR_PC_AFTER_BREAK;
// OBSOLETE       write_pc (stop_pc);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Because of Multi-arch, GET_LONGJMP_TARGET is always defined.  So test
// OBSOLETE    for a definition of JB_PC.  */
// OBSOLETE #ifdef JB_PC
// OBSOLETE /* Figure out where the longjmp will land.  Slurp the args out of the stack.
// OBSOLETE    We expect the first arg to be a pointer to the jmp_buf structure from which
// OBSOLETE    we extract the pc (JB_PC) that we will land at.  The pc is copied into PC.
// OBSOLETE    This routine returns true on success. */
// OBSOLETE 
// OBSOLETE int
// OBSOLETE get_longjmp_target (CORE_ADDR *pc)
// OBSOLETE {
// OBSOLETE   char buf[TARGET_PTR_BIT / TARGET_CHAR_BIT];
// OBSOLETE   CORE_ADDR sp, jb_addr;
// OBSOLETE 
// OBSOLETE   sp = read_register (SP_REGNUM);
// OBSOLETE 
// OBSOLETE   if (target_read_memory (sp + SP_ARG0,		/* Offset of first arg on stack */
// OBSOLETE 			  buf,
// OBSOLETE 			  TARGET_PTR_BIT / TARGET_CHAR_BIT))
// OBSOLETE     return 0;
// OBSOLETE 
// OBSOLETE   jb_addr = extract_address (buf, TARGET_PTR_BIT / TARGET_CHAR_BIT);
// OBSOLETE 
// OBSOLETE   if (target_read_memory (jb_addr + JB_PC * JB_ELEMENT_SIZE, buf,
// OBSOLETE 			  TARGET_PTR_BIT / TARGET_CHAR_BIT))
// OBSOLETE     return 0;
// OBSOLETE 
// OBSOLETE   *pc = extract_address (buf, TARGET_PTR_BIT / TARGET_CHAR_BIT);
// OBSOLETE 
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE #endif /* GET_LONGJMP_TARGET */
// OBSOLETE 
// OBSOLETE /* Disassemble one instruction.  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE arc_print_insn (bfd_vma vma, disassemble_info *info)
// OBSOLETE {
// OBSOLETE   static int current_mach;
// OBSOLETE   static int current_endian;
// OBSOLETE   static disassembler_ftype current_disasm;
// OBSOLETE 
// OBSOLETE   if (current_disasm == NULL
// OBSOLETE       || arc_bfd_mach_type != current_mach
// OBSOLETE       || TARGET_BYTE_ORDER != current_endian)
// OBSOLETE     {
// OBSOLETE       current_mach = arc_bfd_mach_type;
// OBSOLETE       current_endian = TARGET_BYTE_ORDER;
// OBSOLETE       current_disasm = arc_get_disassembler (NULL);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return (*current_disasm) (vma, info);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Command to set cpu type.  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE arc_set_cpu_type_command (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   if (tmp_arc_cpu_type == NULL || *tmp_arc_cpu_type == '\0')
// OBSOLETE     {
// OBSOLETE       printf_unfiltered ("The known ARC cpu types are as follows:\n");
// OBSOLETE       for (i = 0; arc_cpu_type_table[i].name != NULL; ++i)
// OBSOLETE 	printf_unfiltered ("%s\n", arc_cpu_type_table[i].name);
// OBSOLETE 
// OBSOLETE       /* Restore the value.  */
// OBSOLETE       tmp_arc_cpu_type = xstrdup (arc_cpu_type);
// OBSOLETE 
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (!arc_set_cpu_type (tmp_arc_cpu_type))
// OBSOLETE     {
// OBSOLETE       error ("Unknown cpu type `%s'.", tmp_arc_cpu_type);
// OBSOLETE       /* Restore its value.  */
// OBSOLETE       tmp_arc_cpu_type = xstrdup (arc_cpu_type);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE arc_show_cpu_type_command (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Modify the actual cpu type.
// OBSOLETE    Result is a boolean indicating success.  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE arc_set_cpu_type (char *str)
// OBSOLETE {
// OBSOLETE   int i, j;
// OBSOLETE 
// OBSOLETE   if (str == NULL)
// OBSOLETE     return 0;
// OBSOLETE 
// OBSOLETE   for (i = 0; arc_cpu_type_table[i].name != NULL; ++i)
// OBSOLETE     {
// OBSOLETE       if (strcasecmp (str, arc_cpu_type_table[i].name) == 0)
// OBSOLETE 	{
// OBSOLETE 	  arc_cpu_type = str;
// OBSOLETE 	  arc_bfd_mach_type = arc_cpu_type_table[i].value;
// OBSOLETE 	  return 1;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_arc_tdep (void)
// OBSOLETE {
// OBSOLETE   struct cmd_list_element *c;
// OBSOLETE 
// OBSOLETE   c = add_set_cmd ("cpu", class_support, var_string_noescape,
// OBSOLETE 		   (char *) &tmp_arc_cpu_type,
// OBSOLETE 		   "Set the type of ARC cpu in use.\n\
// OBSOLETE This command has two purposes.  In a multi-cpu system it lets one\n\
// OBSOLETE change the cpu being debugged.  It also gives one access to\n\
// OBSOLETE cpu-type-specific registers and recognize cpu-type-specific instructions.\
// OBSOLETE ",
// OBSOLETE 		   &setlist);
// OBSOLETE   set_cmd_cfunc (c, arc_set_cpu_type_command);
// OBSOLETE   c = add_show_from_set (c, &showlist);
// OBSOLETE   set_cmd_cfunc (c, arc_show_cpu_type_command);
// OBSOLETE 
// OBSOLETE   /* We have to use xstrdup() here because the `set' command frees it
// OBSOLETE      before setting a new value.  */
// OBSOLETE   tmp_arc_cpu_type = xstrdup (DEFAULT_ARC_CPU_TYPE);
// OBSOLETE   arc_set_cpu_type (tmp_arc_cpu_type);
// OBSOLETE 
// OBSOLETE   c = add_set_cmd ("displaypipeline", class_support, var_zinteger,
// OBSOLETE 		   (char *) &display_pipeline_p,
// OBSOLETE 		   "Set pipeline display (simulator only).\n\
// OBSOLETE When enabled, the state of the pipeline after each cycle is displayed.",
// OBSOLETE 		   &setlist);
// OBSOLETE   c = add_show_from_set (c, &showlist);
// OBSOLETE 
// OBSOLETE   c = add_set_cmd ("debugpipeline", class_support, var_zinteger,
// OBSOLETE 		   (char *) &debug_pipeline_p,
// OBSOLETE 		   "Set pipeline debug display (simulator only).\n\
// OBSOLETE When enabled, debugging information about the pipeline is displayed.",
// OBSOLETE 		   &setlist);
// OBSOLETE   c = add_show_from_set (c, &showlist);
// OBSOLETE 
// OBSOLETE   c = add_set_cmd ("cputimer", class_support, var_zinteger,
// OBSOLETE 		   (char *) &cpu_timer,
// OBSOLETE 		   "Set maximum cycle count (simulator only).\n\
// OBSOLETE Control will return to gdb if the timer expires.\n\
// OBSOLETE A negative value disables the timer.",
// OBSOLETE 		   &setlist);
// OBSOLETE   c = add_show_from_set (c, &showlist);
// OBSOLETE 
// OBSOLETE   tm_print_insn = arc_print_insn;
// OBSOLETE }
