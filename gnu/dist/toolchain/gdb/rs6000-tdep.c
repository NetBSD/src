/* Target-dependent code for GDB, the GNU debugger.
   Copyright 1986, 1987, 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 2000
   Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "objfiles.h"
#include "xcoffsolib.h"

extern int errno;

/* Breakpoint shadows for the single step instructions will be kept here. */

static struct sstep_breaks
  {
    /* Address, or 0 if this is not in use.  */
    CORE_ADDR address;
    /* Shadow contents.  */
    char data[4];
  }
stepBreaks[2];

/* Hook for determining the TOC address when calling functions in the
   inferior under AIX. The initialization code in rs6000-nat.c sets
   this hook to point to find_toc_address.  */

CORE_ADDR (*find_toc_address_hook) PARAMS ((CORE_ADDR)) = NULL;

/* Static function prototypes */

     static CORE_ADDR branch_dest PARAMS ((int opcode, int instr, CORE_ADDR pc,
					   CORE_ADDR safety));

     static void frame_get_saved_regs PARAMS ((struct frame_info * fi,
					 struct rs6000_framedata * fdatap));

     static void pop_dummy_frame PARAMS ((void));

     static CORE_ADDR frame_initial_stack_address PARAMS ((struct frame_info *));

CORE_ADDR
rs6000_skip_prologue (pc)
     CORE_ADDR pc;
{
  struct rs6000_framedata frame;
  pc = skip_prologue (pc, &frame);
  return pc;
}


/* Fill in fi->saved_regs */

struct frame_extra_info
{
  /* Functions calling alloca() change the value of the stack
     pointer. We need to use initial stack pointer (which is saved in
     r31 by gcc) in such cases. If a compiler emits traceback table,
     then we should use the alloca register specified in traceback
     table. FIXME. */
  CORE_ADDR initial_sp;		/* initial stack pointer. */
};

void
rs6000_init_extra_frame_info (fromleaf, fi)
     int fromleaf;
     struct frame_info *fi;
{
  fi->extra_info = (struct frame_extra_info *)
    frame_obstack_alloc (sizeof (struct frame_extra_info));
  fi->extra_info->initial_sp = 0;
  if (fi->next != (CORE_ADDR) 0
      && fi->pc < TEXT_SEGMENT_BASE)
    /* We're in get_prev_frame */
    /* and this is a special signal frame.  */
    /* (fi->pc will be some low address in the kernel, */
    /*  to which the signal handler returns).  */
    fi->signal_handler_caller = 1;
}


void
rs6000_frame_init_saved_regs (fi)
     struct frame_info *fi;
{
  frame_get_saved_regs (fi, NULL);
}

CORE_ADDR
rs6000_frame_args_address (fi)
     struct frame_info *fi;
{
  if (fi->extra_info->initial_sp != 0)
    return fi->extra_info->initial_sp;
  else
    return frame_initial_stack_address (fi);
}


/* Calculate the destination of a branch/jump.  Return -1 if not a branch.  */

static CORE_ADDR
branch_dest (opcode, instr, pc, safety)
     int opcode;
     int instr;
     CORE_ADDR pc;
     CORE_ADDR safety;
{
  CORE_ADDR dest;
  int immediate;
  int absolute;
  int ext_op;

  absolute = (int) ((instr >> 1) & 1);

  switch (opcode)
    {
    case 18:
      immediate = ((instr & ~3) << 6) >> 6;	/* br unconditional */
      if (absolute)
	dest = immediate;
      else
	dest = pc + immediate;
      break;

    case 16:
      immediate = ((instr & ~3) << 16) >> 16;	/* br conditional */
      if (absolute)
	dest = immediate;
      else
	dest = pc + immediate;
      break;

    case 19:
      ext_op = (instr >> 1) & 0x3ff;

      if (ext_op == 16)		/* br conditional register */
	{
	  dest = read_register (LR_REGNUM) & ~3;

	  /* If we are about to return from a signal handler, dest is
	     something like 0x3c90.  The current frame is a signal handler
	     caller frame, upon completion of the sigreturn system call
	     execution will return to the saved PC in the frame.  */
	  if (dest < TEXT_SEGMENT_BASE)
	    {
	      struct frame_info *fi;

	      fi = get_current_frame ();
	      if (fi != NULL)
		dest = read_memory_integer (fi->frame + SIG_FRAME_PC_OFFSET,
					    4);
	    }
	}

      else if (ext_op == 528)	/* br cond to count reg */
	{
	  dest = read_register (CTR_REGNUM) & ~3;

	  /* If we are about to execute a system call, dest is something
	     like 0x22fc or 0x3b00.  Upon completion the system call
	     will return to the address in the link register.  */
	  if (dest < TEXT_SEGMENT_BASE)
	    dest = read_register (LR_REGNUM) & ~3;
	}
      else
	return -1;
      break;

    default:
      return -1;
    }
  return (dest < TEXT_SEGMENT_BASE) ? safety : dest;
}


/* Sequence of bytes for breakpoint instruction.  */

#define BIG_BREAKPOINT { 0x7d, 0x82, 0x10, 0x08 }
#define LITTLE_BREAKPOINT { 0x08, 0x10, 0x82, 0x7d }

unsigned char *
rs6000_breakpoint_from_pc (bp_addr, bp_size)
     CORE_ADDR *bp_addr;
     int *bp_size;
{
  static unsigned char big_breakpoint[] = BIG_BREAKPOINT;
  static unsigned char little_breakpoint[] = LITTLE_BREAKPOINT;
  *bp_size = 4;
  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
    return big_breakpoint;
  else
    return little_breakpoint;
}


/* AIX does not support PT_STEP. Simulate it. */

void
rs6000_software_single_step (signal, insert_breakpoints_p)
     unsigned int signal;
     int insert_breakpoints_p;
{
#define	INSNLEN(OPCODE)	 4

  static char le_breakp[] = LITTLE_BREAKPOINT;
  static char be_breakp[] = BIG_BREAKPOINT;
  char *breakp = TARGET_BYTE_ORDER == BIG_ENDIAN ? be_breakp : le_breakp;
  int ii, insn;
  CORE_ADDR loc;
  CORE_ADDR breaks[2];
  int opcode;

  if (insert_breakpoints_p)
    {

      loc = read_pc ();

      insn = read_memory_integer (loc, 4);

      breaks[0] = loc + INSNLEN (insn);
      opcode = insn >> 26;
      breaks[1] = branch_dest (opcode, insn, loc, breaks[0]);

      /* Don't put two breakpoints on the same address. */
      if (breaks[1] == breaks[0])
	breaks[1] = -1;

      stepBreaks[1].address = 0;

      for (ii = 0; ii < 2; ++ii)
	{

	  /* ignore invalid breakpoint. */
	  if (breaks[ii] == -1)
	    continue;

	  read_memory (breaks[ii], stepBreaks[ii].data, 4);

	  write_memory (breaks[ii], breakp, 4);
	  stepBreaks[ii].address = breaks[ii];
	}

    }
  else
    {

      /* remove step breakpoints. */
      for (ii = 0; ii < 2; ++ii)
	if (stepBreaks[ii].address != 0)
	  write_memory
	    (stepBreaks[ii].address, stepBreaks[ii].data, 4);

    }
  errno = 0;			/* FIXME, don't ignore errors! */
  /* What errors?  {read,write}_memory call error().  */
}


/* return pc value after skipping a function prologue and also return
   information about a function frame.

   in struct rs6000_framedata fdata:
   - frameless is TRUE, if function does not have a frame.
   - nosavedpc is TRUE, if function does not save %pc value in its frame.
   - offset is the initial size of this stack frame --- the amount by
   which we decrement the sp to allocate the frame.
   - saved_gpr is the number of the first saved gpr.
   - saved_fpr is the number of the first saved fpr.
   - alloca_reg is the number of the register used for alloca() handling.
   Otherwise -1.
   - gpr_offset is the offset of the first saved gpr from the previous frame.
   - fpr_offset is the offset of the first saved fpr from the previous frame.
   - lr_offset is the offset of the saved lr
   - cr_offset is the offset of the saved cr
 */

#define SIGNED_SHORT(x) 						\
  ((sizeof (short) == 2)						\
   ? ((int)(short)(x))							\
   : ((int)((((x) & 0xffff) ^ 0x8000) - 0x8000)))

#define GET_SRC_REG(x) (((x) >> 21) & 0x1f)

CORE_ADDR
skip_prologue (CORE_ADDR pc, struct rs6000_framedata *fdata)
{
  CORE_ADDR orig_pc = pc;
  CORE_ADDR last_prologue_pc;
  char buf[4];
  unsigned long op;
  long offset = 0;
  int lr_reg = -1;
  int cr_reg = -1;
  int reg;
  int framep = 0;
  int minimal_toc_loaded = 0;
  int prev_insn_was_prologue_insn = 1;

  memset (fdata, 0, sizeof (struct rs6000_framedata));
  fdata->saved_gpr = -1;
  fdata->saved_fpr = -1;
  fdata->alloca_reg = -1;
  fdata->frameless = 1;
  fdata->nosavedpc = 1;

  pc -= 4;
  for (;;)
    {
      pc += 4;

      /* Sometimes it isn't clear if an instruction is a prologue
         instruction or not.  When we encounter one of these ambiguous
	 cases, we'll set prev_insn_was_prologue_insn to 0 (false).
	 Otherwise, we'll assume that it really is a prologue instruction. */
      if (prev_insn_was_prologue_insn)
	last_prologue_pc = pc;
      prev_insn_was_prologue_insn = 1;

      if (target_read_memory (pc, buf, 4))
	break;
      op = extract_signed_integer (buf, 4);

      if ((op & 0xfc1fffff) == 0x7c0802a6)
	{			/* mflr Rx */
	  lr_reg = (op & 0x03e00000) | 0x90010000;
	  continue;

	}
      else if ((op & 0xfc1fffff) == 0x7c000026)
	{			/* mfcr Rx */
	  cr_reg = (op & 0x03e00000) | 0x90010000;
	  continue;

	}
      else if ((op & 0xfc1f0000) == 0xd8010000)
	{			/* stfd Rx,NUM(r1) */
	  reg = GET_SRC_REG (op);
	  if (fdata->saved_fpr == -1 || fdata->saved_fpr > reg)
	    {
	      fdata->saved_fpr = reg;
	      fdata->fpr_offset = SIGNED_SHORT (op) + offset;
	    }
	  continue;

	}
      else if (((op & 0xfc1f0000) == 0xbc010000) ||	/* stm Rx, NUM(r1) */
	       ((op & 0xfc1f0000) == 0x90010000 &&	/* st rx,NUM(r1), 
							   rx >= r13 */
		(op & 0x03e00000) >= 0x01a00000))
	{

	  reg = GET_SRC_REG (op);
	  if (fdata->saved_gpr == -1 || fdata->saved_gpr > reg)
	    {
	      fdata->saved_gpr = reg;
	      fdata->gpr_offset = SIGNED_SHORT (op) + offset;
	    }
	  continue;

	}
      else if ((op & 0xffff0000) == 0x60000000)
        {
	  			/* nop */
	  /* Allow nops in the prologue, but do not consider them to
	     be part of the prologue unless followed by other prologue
	     instructions. */
	  prev_insn_was_prologue_insn = 0;
	  continue;

	}
      else if ((op & 0xffff0000) == 0x3c000000)
	{			/* addis 0,0,NUM, used
				   for >= 32k frames */
	  fdata->offset = (op & 0x0000ffff) << 16;
	  fdata->frameless = 0;
	  continue;

	}
      else if ((op & 0xffff0000) == 0x60000000)
	{			/* ori 0,0,NUM, 2nd ha
				   lf of >= 32k frames */
	  fdata->offset |= (op & 0x0000ffff);
	  fdata->frameless = 0;
	  continue;

	}
      else if (lr_reg != -1 && (op & 0xffff0000) == lr_reg)
	{			/* st Rx,NUM(r1) 
				   where Rx == lr */
	  fdata->lr_offset = SIGNED_SHORT (op) + offset;
	  fdata->nosavedpc = 0;
	  lr_reg = 0;
	  continue;

	}
      else if (cr_reg != -1 && (op & 0xffff0000) == cr_reg)
	{			/* st Rx,NUM(r1) 
				   where Rx == cr */
	  fdata->cr_offset = SIGNED_SHORT (op) + offset;
	  cr_reg = 0;
	  continue;

	}
      else if (op == 0x48000005)
	{			/* bl .+4 used in 
				   -mrelocatable */
	  continue;

	}
      else if (op == 0x48000004)
	{			/* b .+4 (xlc) */
	  break;

	}
      else if (((op & 0xffff0000) == 0x801e0000 ||	/* lwz 0,NUM(r30), used
							   in V.4 -mrelocatable */
		op == 0x7fc0f214) &&	/* add r30,r0,r30, used
					   in V.4 -mrelocatable */
	       lr_reg == 0x901e0000)
	{
	  continue;

	}
      else if ((op & 0xffff0000) == 0x3fc00000 ||	/* addis 30,0,foo@ha, used
							   in V.4 -mminimal-toc */
	       (op & 0xffff0000) == 0x3bde0000)
	{			/* addi 30,30,foo@l */
	  continue;

	}
      else if ((op & 0xfc000001) == 0x48000001)
	{			/* bl foo, 
				   to save fprs??? */

	  fdata->frameless = 0;
	  /* Don't skip over the subroutine call if it is not within the first
	     three instructions of the prologue.  */
	  if ((pc - orig_pc) > 8)
	    break;

	  op = read_memory_integer (pc + 4, 4);

	  /* At this point, make sure this is not a trampoline function
	     (a function that simply calls another functions, and nothing else).
	     If the next is not a nop, this branch was part of the function
	     prologue. */

	  if (op == 0x4def7b82 || op == 0)	/* crorc 15, 15, 15 */
	    break;		/* don't skip over 
				   this branch */
	  continue;

	  /* update stack pointer */
	}
      else if ((op & 0xffff0000) == 0x94210000)
	{			/* stu r1,NUM(r1) */
	  fdata->frameless = 0;
	  fdata->offset = SIGNED_SHORT (op);
	  offset = fdata->offset;
	  continue;

	}
      else if (op == 0x7c21016e)
	{			/* stwux 1,1,0 */
	  fdata->frameless = 0;
	  offset = fdata->offset;
	  continue;

	  /* Load up minimal toc pointer */
	}
      else if ((op >> 22) == 0x20f
	       && !minimal_toc_loaded)
	{			/* l r31,... or l r30,... */
	  minimal_toc_loaded = 1;
	  continue;

	  /* move parameters from argument registers to local variable
             registers */
 	}
      else if ((op & 0xfc0007fe) == 0x7c000378 &&	/* mr(.)  Rx,Ry */
               (((op >> 21) & 31) >= 3) &&              /* R3 >= Ry >= R10 */
               (((op >> 21) & 31) <= 10) &&
               (((op >> 16) & 31) >= fdata->saved_gpr)) /* Rx: local var reg */
	{
	  continue;

	  /* store parameters in stack */
	}
      else if ((op & 0xfc1f0000) == 0x90010000 ||	/* st rx,NUM(r1) */
	       (op & 0xfc1f0000) == 0xd8010000 ||	/* stfd Rx,NUM(r1) */
	       (op & 0xfc1f0000) == 0xfc010000)
	{			/* frsp, fp?,NUM(r1) */
	  continue;

	  /* store parameters in stack via frame pointer */
	}
      else if (framep &&
	       ((op & 0xfc1f0000) == 0x901f0000 ||	/* st rx,NUM(r1) */
		(op & 0xfc1f0000) == 0xd81f0000 ||	/* stfd Rx,NUM(r1) */
		(op & 0xfc1f0000) == 0xfc1f0000))
	{			/* frsp, fp?,NUM(r1) */
	  continue;

	  /* Set up frame pointer */
	}
      else if (op == 0x603f0000	/* oril r31, r1, 0x0 */
	       || op == 0x7c3f0b78)
	{			/* mr r31, r1 */
	  fdata->frameless = 0;
	  framep = 1;
	  fdata->alloca_reg = 31;
	  continue;

	  /* Another way to set up the frame pointer.  */
	}
      else if ((op & 0xfc1fffff) == 0x38010000)
	{			/* addi rX, r1, 0x0 */
	  fdata->frameless = 0;
	  framep = 1;
	  fdata->alloca_reg = (op & ~0x38010000) >> 21;
	  continue;

	}
      else
	{
	  break;
	}
    }

#if 0
/* I have problems with skipping over __main() that I need to address
 * sometime. Previously, I used to use misc_function_vector which
 * didn't work as well as I wanted to be.  -MGO */

  /* If the first thing after skipping a prolog is a branch to a function,
     this might be a call to an initializer in main(), introduced by gcc2.
     We'd like to skip over it as well. Fortunately, xlc does some extra
     work before calling a function right after a prologue, thus we can
     single out such gcc2 behaviour. */


  if ((op & 0xfc000001) == 0x48000001)
    {				/* bl foo, an initializer function? */
      op = read_memory_integer (pc + 4, 4);

      if (op == 0x4def7b82)
	{			/* cror 0xf, 0xf, 0xf (nop) */

	  /* check and see if we are in main. If so, skip over this initializer
	     function as well. */

	  tmp = find_pc_misc_function (pc);
	  if (tmp >= 0 && STREQ (misc_function_vector[tmp].name, "main"))
	    return pc + 8;
	}
    }
#endif /* 0 */

  fdata->offset = -fdata->offset;
  return last_prologue_pc;
}


/*************************************************************************
  Support for creating pushing a dummy frame into the stack, and popping
  frames, etc. 
*************************************************************************/

/* The total size of dummy frame is 436, which is;

   32 gpr's           - 128 bytes
   32 fpr's           - 256 bytes
   7  the rest        -  28 bytes
   callee's link area -  24 bytes
   padding            -  12 bytes

   Note that the last 24 bytes for the link area might not be necessary,
   since it will be taken care of by push_arguments(). */

#define DUMMY_FRAME_SIZE 448

#define	DUMMY_FRAME_ADDR_SIZE 10

/* Make sure you initialize these in somewhere, in case gdb gives up what it
   was debugging and starts debugging something else. FIXMEibm */

static int dummy_frame_count = 0;
static int dummy_frame_size = 0;
static CORE_ADDR *dummy_frame_addr = 0;

extern int stop_stack_dummy;

/* push a dummy frame into stack, save all register. Currently we are saving
   only gpr's and fpr's, which is not good enough! FIXMEmgo */

void
push_dummy_frame ()
{
  /* stack pointer.  */
  CORE_ADDR sp;
  /* Same thing, target byte order.  */
  char sp_targ[4];

  /* link register.  */
  CORE_ADDR pc;
  /* Same thing, target byte order.  */
  char pc_targ[4];

  /* Needed to figure out where to save the dummy link area.
     FIXME: There should be an easier way to do this, no?  tiemann 9/9/95.  */
  struct rs6000_framedata fdata;

  int ii;

  target_fetch_registers (-1);

  if (dummy_frame_count >= dummy_frame_size)
    {
      dummy_frame_size += DUMMY_FRAME_ADDR_SIZE;
      if (dummy_frame_addr)
	dummy_frame_addr = (CORE_ADDR *) xrealloc
	  (dummy_frame_addr, sizeof (CORE_ADDR) * (dummy_frame_size));
      else
	dummy_frame_addr = (CORE_ADDR *)
	  xmalloc (sizeof (CORE_ADDR) * (dummy_frame_size));
    }

  sp = read_register (SP_REGNUM);
  pc = read_register (PC_REGNUM);
  store_address (pc_targ, 4, pc);

  skip_prologue (get_pc_function_start (pc), &fdata);

  dummy_frame_addr[dummy_frame_count++] = sp;

  /* Be careful! If the stack pointer is not decremented first, then kernel 
     thinks he is free to use the space underneath it. And kernel actually 
     uses that area for IPC purposes when executing ptrace(2) calls. So 
     before writing register values into the new frame, decrement and update
     %sp first in order to secure your frame. */

  /* FIXME: We don't check if the stack really has this much space.
     This is a problem on the ppc simulator (which only grants one page
     (4096 bytes) by default.  */

  write_register (SP_REGNUM, sp - DUMMY_FRAME_SIZE);

  /* gdb relies on the state of current_frame. We'd better update it,
     otherwise things like do_registers_info() wouldn't work properly! */

  flush_cached_frames ();

  /* save program counter in link register's space. */
  write_memory (sp + (fdata.lr_offset ? fdata.lr_offset : DEFAULT_LR_SAVE),
		pc_targ, 4);

  /* save all floating point and general purpose registers here. */

  /* fpr's, f0..f31 */
  for (ii = 0; ii < 32; ++ii)
    write_memory (sp - 8 - (ii * 8), &registers[REGISTER_BYTE (31 - ii + FP0_REGNUM)], 8);

  /* gpr's r0..r31 */
  for (ii = 1; ii <= 32; ++ii)
    write_memory (sp - 256 - (ii * 4), &registers[REGISTER_BYTE (32 - ii)], 4);

  /* so far, 32*2 + 32 words = 384 bytes have been written. 
     7 extra registers in our register set: pc, ps, cnd, lr, cnt, xer, mq */

  for (ii = 1; ii <= (LAST_UISA_SP_REGNUM - FIRST_UISA_SP_REGNUM + 1); ++ii)
    {
      write_memory (sp - 384 - (ii * 4),
		    &registers[REGISTER_BYTE (FPLAST_REGNUM + ii)], 4);
    }

  /* Save sp or so called back chain right here. */
  store_address (sp_targ, 4, sp);
  write_memory (sp - DUMMY_FRAME_SIZE, sp_targ, 4);
  sp -= DUMMY_FRAME_SIZE;

  /* And finally, this is the back chain. */
  write_memory (sp + 8, pc_targ, 4);
}


/* Pop a dummy frame.

   In rs6000 when we push a dummy frame, we save all of the registers. This
   is usually done before user calls a function explicitly.

   After a dummy frame is pushed, some instructions are copied into stack,
   and stack pointer is decremented even more.  Since we don't have a frame
   pointer to get back to the parent frame of the dummy, we start having
   trouble poping it.  Therefore, we keep a dummy frame stack, keeping
   addresses of dummy frames as such.  When poping happens and when we
   detect that was a dummy frame, we pop it back to its parent by using
   dummy frame stack (`dummy_frame_addr' array). 

   FIXME:  This whole concept is broken.  You should be able to detect
   a dummy stack frame *on the user's stack itself*.  When you do,
   then you know the format of that stack frame -- including its
   saved SP register!  There should *not* be a separate stack in the
   GDB process that keeps track of these dummy frames!  -- gnu@cygnus.com Aug92
 */

static void
pop_dummy_frame ()
{
  CORE_ADDR sp, pc;
  int ii;
  sp = dummy_frame_addr[--dummy_frame_count];

  /* restore all fpr's. */
  for (ii = 1; ii <= 32; ++ii)
    read_memory (sp - (ii * 8), &registers[REGISTER_BYTE (32 - ii + FP0_REGNUM)], 8);

  /* restore all gpr's */
  for (ii = 1; ii <= 32; ++ii)
    {
      read_memory (sp - 256 - (ii * 4), &registers[REGISTER_BYTE (32 - ii)], 4);
    }

  /* restore the rest of the registers. */
  for (ii = 1; ii <= (LAST_UISA_SP_REGNUM - FIRST_UISA_SP_REGNUM + 1); ++ii)
    read_memory (sp - 384 - (ii * 4),
		 &registers[REGISTER_BYTE (FPLAST_REGNUM + ii)], 4);

  read_memory (sp - (DUMMY_FRAME_SIZE - 8),
	       &registers[REGISTER_BYTE (PC_REGNUM)], 4);

  /* when a dummy frame was being pushed, we had to decrement %sp first, in 
     order to secure astack space. Thus, saved %sp (or %r1) value, is not the
     one we should restore. Change it with the one we need. */

  memcpy (&registers[REGISTER_BYTE (FP_REGNUM)], (char *) &sp, sizeof (int));

  /* Now we can restore all registers. */

  target_store_registers (-1);
  pc = read_pc ();
  flush_cached_frames ();
}


/* pop the innermost frame, go back to the caller. */

void
pop_frame ()
{
  CORE_ADDR pc, lr, sp, prev_sp;	/* %pc, %lr, %sp */
  struct rs6000_framedata fdata;
  struct frame_info *frame = get_current_frame ();
  int addr, ii;

  pc = read_pc ();
  sp = FRAME_FP (frame);

  if (stop_stack_dummy)
    {
      if (USE_GENERIC_DUMMY_FRAMES)
	{
	  generic_pop_dummy_frame ();
	  flush_cached_frames ();
	  return;
	}
      else
	{
	  if (dummy_frame_count)
	    pop_dummy_frame ();
	  return;
	}
    }

  /* Make sure that all registers are valid.  */
  read_register_bytes (0, NULL, REGISTER_BYTES);

  /* figure out previous %pc value. If the function is frameless, it is 
     still in the link register, otherwise walk the frames and retrieve the
     saved %pc value in the previous frame. */

  addr = get_pc_function_start (frame->pc);
  (void) skip_prologue (addr, &fdata);

  if (fdata.frameless)
    prev_sp = sp;
  else
    prev_sp = read_memory_integer (sp, 4);
  if (fdata.lr_offset == 0)
    lr = read_register (LR_REGNUM);
  else
    lr = read_memory_integer (prev_sp + fdata.lr_offset, 4);

  /* reset %pc value. */
  write_register (PC_REGNUM, lr);

  /* reset register values if any was saved earlier. */

  if (fdata.saved_gpr != -1)
    {
      addr = prev_sp + fdata.gpr_offset;
      for (ii = fdata.saved_gpr; ii <= 31; ++ii)
	{
	  read_memory (addr, &registers[REGISTER_BYTE (ii)], 4);
	  addr += 4;
	}
    }

  if (fdata.saved_fpr != -1)
    {
      addr = prev_sp + fdata.fpr_offset;
      for (ii = fdata.saved_fpr; ii <= 31; ++ii)
	{
	  read_memory (addr, &registers[REGISTER_BYTE (ii + FP0_REGNUM)], 8);
	  addr += 8;
	}
    }

  write_register (SP_REGNUM, prev_sp);
  target_store_registers (-1);
  flush_cached_frames ();
}

/* fixup the call sequence of a dummy function, with the real function address.
   its argumets will be passed by gdb. */

void
rs6000_fix_call_dummy (dummyname, pc, fun, nargs, args, type, gcc_p)
     char *dummyname;
     CORE_ADDR pc;
     CORE_ADDR fun;
     int nargs;
     value_ptr *args;
     struct type *type;
     int gcc_p;
{
#define	TOC_ADDR_OFFSET		20
#define	TARGET_ADDR_OFFSET	28

  int ii;
  CORE_ADDR target_addr;

  if (USE_GENERIC_DUMMY_FRAMES)
    {
      if (find_toc_address_hook != NULL)
	{
	  CORE_ADDR tocvalue = (*find_toc_address_hook) (fun);
	  write_register (TOC_REGNUM, tocvalue);
	}
    }
  else
    {
      if (find_toc_address_hook != NULL)
	{
	  CORE_ADDR tocvalue;

	  tocvalue = (*find_toc_address_hook) (fun);
	  ii = *(int *) ((char *) dummyname + TOC_ADDR_OFFSET);
	  ii = (ii & 0xffff0000) | (tocvalue >> 16);
	  *(int *) ((char *) dummyname + TOC_ADDR_OFFSET) = ii;

	  ii = *(int *) ((char *) dummyname + TOC_ADDR_OFFSET + 4);
	  ii = (ii & 0xffff0000) | (tocvalue & 0x0000ffff);
	  *(int *) ((char *) dummyname + TOC_ADDR_OFFSET + 4) = ii;
	}

      target_addr = fun;
      ii = *(int *) ((char *) dummyname + TARGET_ADDR_OFFSET);
      ii = (ii & 0xffff0000) | (target_addr >> 16);
      *(int *) ((char *) dummyname + TARGET_ADDR_OFFSET) = ii;

      ii = *(int *) ((char *) dummyname + TARGET_ADDR_OFFSET + 4);
      ii = (ii & 0xffff0000) | (target_addr & 0x0000ffff);
      *(int *) ((char *) dummyname + TARGET_ADDR_OFFSET + 4) = ii;
    }
}

/* Pass the arguments in either registers, or in the stack. In RS6000,
   the first eight words of the argument list (that might be less than
   eight parameters if some parameters occupy more than one word) are
   passed in r3..r11 registers.  float and double parameters are
   passed in fpr's, in addition to that. Rest of the parameters if any
   are passed in user stack. There might be cases in which half of the
   parameter is copied into registers, the other half is pushed into
   stack.

   If the function is returning a structure, then the return address is passed
   in r3, then the first 7 words of the parameters can be passed in registers,
   starting from r4. */

CORE_ADDR
rs6000_push_arguments (nargs, args, sp, struct_return, struct_addr)
     int nargs;
     value_ptr *args;
     CORE_ADDR sp;
     int struct_return;
     CORE_ADDR struct_addr;
{
  int ii;
  int len = 0;
  int argno;			/* current argument number */
  int argbytes;			/* current argument byte */
  char tmp_buffer[50];
  int f_argno = 0;		/* current floating point argno */

  value_ptr arg = 0;
  struct type *type;

  CORE_ADDR saved_sp;

  if (!USE_GENERIC_DUMMY_FRAMES)
    {
      if (dummy_frame_count <= 0)
	printf_unfiltered ("FATAL ERROR -push_arguments()! frame not found!!\n");
    }

  /* The first eight words of ther arguments are passed in registers. Copy
     them appropriately.

     If the function is returning a `struct', then the first word (which 
     will be passed in r3) is used for struct return address. In that
     case we should advance one word and start from r4 register to copy 
     parameters. */

  ii = struct_return ? 1 : 0;

/* 
   effectively indirect call... gcc does...

   return_val example( float, int);

   eabi: 
   float in fp0, int in r3
   offset of stack on overflow 8/16
   for varargs, must go by type.
   power open:
   float in r3&r4, int in r5
   offset of stack on overflow different 
   both: 
   return in r3 or f0.  If no float, must study how gcc emulates floats;
   pay attention to arg promotion.  
   User may have to cast\args to handle promotion correctly 
   since gdb won't know if prototype supplied or not.
 */

  for (argno = 0, argbytes = 0; argno < nargs && ii < 8; ++ii)
    {
      int reg_size = REGISTER_RAW_SIZE (ii + 3);

      arg = args[argno];
      type = check_typedef (VALUE_TYPE (arg));
      len = TYPE_LENGTH (type);

      if (TYPE_CODE (type) == TYPE_CODE_FLT)
	{

	  /* floating point arguments are passed in fpr's, as well as gpr's.
	     There are 13 fpr's reserved for passing parameters. At this point
	     there is no way we would run out of them. */

	  if (len > 8)
	    printf_unfiltered (
				"Fatal Error: a floating point parameter #%d with a size > 8 is found!\n", argno);

	  memcpy (&registers[REGISTER_BYTE (FP0_REGNUM + 1 + f_argno)],
		  VALUE_CONTENTS (arg),
		  len);
	  ++f_argno;
	}

      if (len > reg_size)
	{

	  /* Argument takes more than one register. */
	  while (argbytes < len)
	    {
	      memset (&registers[REGISTER_BYTE (ii + 3)], 0, reg_size);
	      memcpy (&registers[REGISTER_BYTE (ii + 3)],
		      ((char *) VALUE_CONTENTS (arg)) + argbytes,
		      (len - argbytes) > reg_size
		        ? reg_size : len - argbytes);
	      ++ii, argbytes += reg_size;

	      if (ii >= 8)
		goto ran_out_of_registers_for_arguments;
	    }
	  argbytes = 0;
	  --ii;
	}
      else
	{			/* Argument can fit in one register. No problem. */
	  int adj = TARGET_BYTE_ORDER == BIG_ENDIAN ? reg_size - len : 0;
	  memset (&registers[REGISTER_BYTE (ii + 3)], 0, reg_size);
	  memcpy ((char *)&registers[REGISTER_BYTE (ii + 3)] + adj, 
	          VALUE_CONTENTS (arg), len);
	}
      ++argno;
    }

ran_out_of_registers_for_arguments:

  if (USE_GENERIC_DUMMY_FRAMES)
    {
      saved_sp = read_sp ();
#ifndef ELF_OBJECT_FORMAT
      /* location for 8 parameters are always reserved. */
      sp -= 4 * 8;

      /* another six words for back chain, TOC register, link register, etc. */
      sp -= 24;

      /* stack pointer must be quadword aligned */
      sp &= -16;
#endif
    }
  else
    {
      /* location for 8 parameters are always reserved. */
      sp -= 4 * 8;

      /* another six words for back chain, TOC register, link register, etc. */
      sp -= 24;

      /* stack pointer must be quadword aligned */
      sp &= -16;
    }

  /* if there are more arguments, allocate space for them in 
     the stack, then push them starting from the ninth one. */

  if ((argno < nargs) || argbytes)
    {
      int space = 0, jj;

      if (argbytes)
	{
	  space += ((len - argbytes + 3) & -4);
	  jj = argno + 1;
	}
      else
	jj = argno;

      for (; jj < nargs; ++jj)
	{
	  value_ptr val = args[jj];
	  space += ((TYPE_LENGTH (VALUE_TYPE (val))) + 3) & -4;
	}

      /* add location required for the rest of the parameters */
      space = (space + 15) & -16;
      sp -= space;

      /* This is another instance we need to be concerned about securing our
         stack space. If we write anything underneath %sp (r1), we might conflict
         with the kernel who thinks he is free to use this area. So, update %sp
         first before doing anything else. */

      write_register (SP_REGNUM, sp);

      /* if the last argument copied into the registers didn't fit there 
         completely, push the rest of it into stack. */

      if (argbytes)
	{
	  write_memory (sp + 24 + (ii * 4),
			((char *) VALUE_CONTENTS (arg)) + argbytes,
			len - argbytes);
	  ++argno;
	  ii += ((len - argbytes + 3) & -4) / 4;
	}

      /* push the rest of the arguments into stack. */
      for (; argno < nargs; ++argno)
	{

	  arg = args[argno];
	  type = check_typedef (VALUE_TYPE (arg));
	  len = TYPE_LENGTH (type);


	  /* float types should be passed in fpr's, as well as in the stack. */
	  if (TYPE_CODE (type) == TYPE_CODE_FLT && f_argno < 13)
	    {

	      if (len > 8)
		printf_unfiltered (
				    "Fatal Error: a floating point parameter #%d with a size > 8 is found!\n", argno);

	      memcpy (&registers[REGISTER_BYTE (FP0_REGNUM + 1 + f_argno)],
		      VALUE_CONTENTS (arg),
		      len);
	      ++f_argno;
	    }

	  write_memory (sp + 24 + (ii * 4), (char *) VALUE_CONTENTS (arg), len);
	  ii += ((len + 3) & -4) / 4;
	}
    }
  else
    /* Secure stack areas first, before doing anything else. */
    write_register (SP_REGNUM, sp);

  if (!USE_GENERIC_DUMMY_FRAMES)
    {
      /* we want to copy 24 bytes of target's frame to dummy's frame,
         then set back chain to point to new frame. */

      saved_sp = dummy_frame_addr[dummy_frame_count - 1];
      read_memory (saved_sp, tmp_buffer, 24);
      write_memory (sp, tmp_buffer, 24);
    }

  /* set back chain properly */
  store_address (tmp_buffer, 4, saved_sp);
  write_memory (sp, tmp_buffer, 4);

  target_store_registers (-1);
  return sp;
}
/* #ifdef ELF_OBJECT_FORMAT */

/* Function: ppc_push_return_address (pc, sp)
   Set up the return address for the inferior function call. */

CORE_ADDR
ppc_push_return_address (pc, sp)
     CORE_ADDR pc;
     CORE_ADDR sp;
{
  write_register (LR_REGNUM, CALL_DUMMY_ADDRESS ());
  return sp;
}

/* #endif */

/* a given return value in `regbuf' with a type `valtype', extract and copy its
   value into `valbuf' */

void
extract_return_value (valtype, regbuf, valbuf)
     struct type *valtype;
     char regbuf[REGISTER_BYTES];
     char *valbuf;
{
  int offset = 0;

  if (TYPE_CODE (valtype) == TYPE_CODE_FLT)
    {

      double dd;
      float ff;
      /* floats and doubles are returned in fpr1. fpr's have a size of 8 bytes.
         We need to truncate the return value into float size (4 byte) if
         necessary. */

      if (TYPE_LENGTH (valtype) > 4)	/* this is a double */
	memcpy (valbuf,
		&regbuf[REGISTER_BYTE (FP0_REGNUM + 1)],
		TYPE_LENGTH (valtype));
      else
	{			/* float */
	  memcpy (&dd, &regbuf[REGISTER_BYTE (FP0_REGNUM + 1)], 8);
	  ff = (float) dd;
	  memcpy (valbuf, &ff, sizeof (float));
	}
    }
  else
    {
      /* return value is copied starting from r3. */
      if (TARGET_BYTE_ORDER == BIG_ENDIAN
	  && TYPE_LENGTH (valtype) < REGISTER_RAW_SIZE (3))
	offset = REGISTER_RAW_SIZE (3) - TYPE_LENGTH (valtype);

      memcpy (valbuf,
	      regbuf + REGISTER_BYTE (3) + offset,
	      TYPE_LENGTH (valtype));
    }
}


/* keep structure return address in this variable.
   FIXME:  This is a horrid kludge which should not be allowed to continue
   living.  This only allows a single nested call to a structure-returning
   function.  Come on, guys!  -- gnu@cygnus.com, Aug 92  */

CORE_ADDR rs6000_struct_return_address;


/* Indirect function calls use a piece of trampoline code to do context
   switching, i.e. to set the new TOC table. Skip such code if we are on
   its first instruction (as when we have single-stepped to here). 
   Also skip shared library trampoline code (which is different from
   indirect function call trampolines).
   Result is desired PC to step until, or NULL if we are not in
   trampoline code.  */

CORE_ADDR
skip_trampoline_code (pc)
     CORE_ADDR pc;
{
  register unsigned int ii, op;
  CORE_ADDR solib_target_pc;

  static unsigned trampoline_code[] =
  {
    0x800b0000,			/*     l   r0,0x0(r11)  */
    0x90410014,			/*    st   r2,0x14(r1)  */
    0x7c0903a6,			/* mtctr   r0           */
    0x804b0004,			/*     l   r2,0x4(r11)  */
    0x816b0008,			/*     l  r11,0x8(r11)  */
    0x4e800420,			/*  bctr                */
    0x4e800020,			/*    br                */
    0
  };

  /* If pc is in a shared library trampoline, return its target.  */
  solib_target_pc = find_solib_trampoline_target (pc);
  if (solib_target_pc)
    return solib_target_pc;

  for (ii = 0; trampoline_code[ii]; ++ii)
    {
      op = read_memory_integer (pc + (ii * 4), 4);
      if (op != trampoline_code[ii])
	return 0;
    }
  ii = read_register (11);	/* r11 holds destination addr   */
  pc = read_memory_integer (ii, 4);	/* (r11) value                  */
  return pc;
}

/* Determines whether the function FI has a frame on the stack or not.  */

int
rs6000_frameless_function_invocation (struct frame_info *fi)
{
  CORE_ADDR func_start;
  struct rs6000_framedata fdata;

  /* Don't even think about framelessness except on the innermost frame
     or if the function was interrupted by a signal.  */
  if (fi->next != NULL && !fi->next->signal_handler_caller)
    return 0;

  func_start = get_pc_function_start (fi->pc);

  /* If we failed to find the start of the function, it is a mistake
     to inspect the instructions. */

  if (!func_start)
    {
      /* A frame with a zero PC is usually created by dereferencing a NULL
         function pointer, normally causing an immediate core dump of the
         inferior. Mark function as frameless, as the inferior has no chance
         of setting up a stack frame.  */
      if (fi->pc == 0)
	return 1;
      else
	return 0;
    }

  (void) skip_prologue (func_start, &fdata);
  return fdata.frameless;
}

/* Return the PC saved in a frame */

unsigned long
rs6000_frame_saved_pc (struct frame_info *fi)
{
  CORE_ADDR func_start;
  struct rs6000_framedata fdata;

  if (fi->signal_handler_caller)
    return read_memory_integer (fi->frame + SIG_FRAME_PC_OFFSET, 4);

  if (USE_GENERIC_DUMMY_FRAMES)
    {
      if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
	return generic_read_register_dummy (fi->pc, fi->frame, PC_REGNUM);
    }

  func_start = get_pc_function_start (fi->pc);

  /* If we failed to find the start of the function, it is a mistake
     to inspect the instructions. */
  if (!func_start)
    return 0;

  (void) skip_prologue (func_start, &fdata);

  if (fdata.lr_offset == 0 && fi->next != NULL)
    {
      if (fi->next->signal_handler_caller)
	return read_memory_integer (fi->next->frame + SIG_FRAME_LR_OFFSET, 4);
      else
	return read_memory_integer (FRAME_CHAIN (fi) + DEFAULT_LR_SAVE, 4);
    }

  if (fdata.lr_offset == 0)
    return read_register (LR_REGNUM);

  return read_memory_integer (FRAME_CHAIN (fi) + fdata.lr_offset, 4);
}

/* If saved registers of frame FI are not known yet, read and cache them.
   &FDATAP contains rs6000_framedata; TDATAP can be NULL,
   in which case the framedata are read.  */

static void
frame_get_saved_regs (fi, fdatap)
     struct frame_info *fi;
     struct rs6000_framedata *fdatap;
{
  CORE_ADDR frame_addr;
  struct rs6000_framedata work_fdata;

  if (fi->saved_regs)
    return;

  if (fdatap == NULL)
    {
      fdatap = &work_fdata;
      (void) skip_prologue (get_pc_function_start (fi->pc), fdatap);
    }

  frame_saved_regs_zalloc (fi);

  /* If there were any saved registers, figure out parent's stack
     pointer. */
  /* The following is true only if the frame doesn't have a call to
     alloca(), FIXME. */

  if (fdatap->saved_fpr == 0 && fdatap->saved_gpr == 0
      && fdatap->lr_offset == 0 && fdatap->cr_offset == 0)
    frame_addr = 0;
  else if (fi->prev && fi->prev->frame)
    frame_addr = fi->prev->frame;
  else
    frame_addr = read_memory_integer (fi->frame, 4);

  /* if != -1, fdatap->saved_fpr is the smallest number of saved_fpr.
     All fpr's from saved_fpr to fp31 are saved.  */

  if (fdatap->saved_fpr >= 0)
    {
      int i;
      int fpr_offset = frame_addr + fdatap->fpr_offset;
      for (i = fdatap->saved_fpr; i < 32; i++)
	{
	  fi->saved_regs[FP0_REGNUM + i] = fpr_offset;
	  fpr_offset += 8;
	}
    }

  /* if != -1, fdatap->saved_gpr is the smallest number of saved_gpr.
     All gpr's from saved_gpr to gpr31 are saved.  */

  if (fdatap->saved_gpr >= 0)
    {
      int i;
      int gpr_offset = frame_addr + fdatap->gpr_offset;
      for (i = fdatap->saved_gpr; i < 32; i++)
	{
	  fi->saved_regs[i] = gpr_offset;
	  gpr_offset += 4;
	}
    }

  /* If != 0, fdatap->cr_offset is the offset from the frame that holds
     the CR.  */
  if (fdatap->cr_offset != 0)
    fi->saved_regs[CR_REGNUM] = frame_addr + fdatap->cr_offset;

  /* If != 0, fdatap->lr_offset is the offset from the frame that holds
     the LR.  */
  if (fdatap->lr_offset != 0)
    fi->saved_regs[LR_REGNUM] = frame_addr + fdatap->lr_offset;
}

/* Return the address of a frame. This is the inital %sp value when the frame
   was first allocated. For functions calling alloca(), it might be saved in
   an alloca register. */

static CORE_ADDR
frame_initial_stack_address (fi)
     struct frame_info *fi;
{
  CORE_ADDR tmpaddr;
  struct rs6000_framedata fdata;
  struct frame_info *callee_fi;

  /* if the initial stack pointer (frame address) of this frame is known,
     just return it. */

  if (fi->extra_info->initial_sp)
    return fi->extra_info->initial_sp;

  /* find out if this function is using an alloca register.. */

  (void) skip_prologue (get_pc_function_start (fi->pc), &fdata);

  /* if saved registers of this frame are not known yet, read and cache them. */

  if (!fi->saved_regs)
    frame_get_saved_regs (fi, &fdata);

  /* If no alloca register used, then fi->frame is the value of the %sp for
     this frame, and it is good enough. */

  if (fdata.alloca_reg < 0)
    {
      fi->extra_info->initial_sp = fi->frame;
      return fi->extra_info->initial_sp;
    }

  /* This function has an alloca register. If this is the top-most frame
     (with the lowest address), the value in alloca register is good. */

  if (!fi->next)
    return fi->extra_info->initial_sp = read_register (fdata.alloca_reg);

  /* Otherwise, this is a caller frame. Callee has usually already saved
     registers, but there are exceptions (such as when the callee
     has no parameters). Find the address in which caller's alloca
     register is saved. */

  for (callee_fi = fi->next; callee_fi; callee_fi = callee_fi->next)
    {

      if (!callee_fi->saved_regs)
	frame_get_saved_regs (callee_fi, NULL);

      /* this is the address in which alloca register is saved. */

      tmpaddr = callee_fi->saved_regs[fdata.alloca_reg];
      if (tmpaddr)
	{
	  fi->extra_info->initial_sp = read_memory_integer (tmpaddr, 4);
	  return fi->extra_info->initial_sp;
	}

      /* Go look into deeper levels of the frame chain to see if any one of
         the callees has saved alloca register. */
    }

  /* If alloca register was not saved, by the callee (or any of its callees)
     then the value in the register is still good. */

  fi->extra_info->initial_sp = read_register (fdata.alloca_reg);
  return fi->extra_info->initial_sp;
}

CORE_ADDR
rs6000_frame_chain (thisframe)
     struct frame_info *thisframe;
{
  CORE_ADDR fp;

  if (USE_GENERIC_DUMMY_FRAMES)
    {
      if (PC_IN_CALL_DUMMY (thisframe->pc, thisframe->frame, thisframe->frame))
	return thisframe->frame;	/* dummy frame same as caller's frame */
    }

  if (inside_entry_file (thisframe->pc) ||
      thisframe->pc == entry_point_address ())
    return 0;

  if (thisframe->signal_handler_caller)
    fp = read_memory_integer (thisframe->frame + SIG_FRAME_FP_OFFSET, 4);
  else if (thisframe->next != NULL
	   && thisframe->next->signal_handler_caller
	   && FRAMELESS_FUNCTION_INVOCATION (thisframe))
    /* A frameless function interrupted by a signal did not change the
       frame pointer.  */
    fp = FRAME_FP (thisframe);
  else
    fp = read_memory_integer ((thisframe)->frame, 4);

  if (USE_GENERIC_DUMMY_FRAMES)
    {
      CORE_ADDR fpp, lr;

      lr = read_register (LR_REGNUM);
      if (lr == entry_point_address ())
	if (fp != 0 && (fpp = read_memory_integer (fp, 4)) != 0)
	  if (PC_IN_CALL_DUMMY (lr, fpp, fpp))
	    return fpp;
    }

  return fp;
}

/* Return nonzero if ADDR (a function pointer) is in the data space and
   is therefore a special function pointer.  */

int
is_magic_function_pointer (addr)
     CORE_ADDR addr;
{
  struct obj_section *s;

  s = find_pc_section (addr);
  if (s && s->the_bfd_section->flags & SEC_CODE)
    return 0;
  else
    return 1;
}

#ifdef GDB_TARGET_POWERPC
int
gdb_print_insn_powerpc (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
    return print_insn_big_powerpc (memaddr, info);
  else
    return print_insn_little_powerpc (memaddr, info);
}
#endif


/* Handling the various PowerPC/RS6000 variants.  */


/* The arrays here called register_names_MUMBLE hold names that 
   the rs6000_register_name function returns.

   For each family of PPC variants, I've tried to isolate out the
   common registers and put them up front, so that as long as you get
   the general family right, GDB will correctly identify the registers
   common to that family.  The common register sets are:

   For the 60x family: hid0 hid1 iabr dabr pir

   For the 505 and 860 family: eie eid nri

   For the 403 and 403GC: icdbdr esr dear evpr cdbcr tsr tcr pit tbhi
   tblo srr2 srr3 dbsr dbcr iac1 iac2 dac1 dac2 dccr iccr pbl1
   pbu1 pbl2 pbu2

   Most of these register groups aren't anything formal.  I arrived at
   them by looking at the registers that occurred in more than one
   processor.  */

/* UISA register names common across all architectures, including POWER.  */

#define COMMON_UISA_REG_NAMES \
  /*  0 */ "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",  \
  /*  8 */ "r8", "r9", "r10","r11","r12","r13","r14","r15", \
  /* 16 */ "r16","r17","r18","r19","r20","r21","r22","r23", \
  /* 24 */ "r24","r25","r26","r27","r28","r29","r30","r31", \
  /* 32 */ "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",  \
  /* 40 */ "f8", "f9", "f10","f11","f12","f13","f14","f15", \
  /* 48 */ "f16","f17","f18","f19","f20","f21","f22","f23", \
  /* 56 */ "f24","f25","f26","f27","f28","f29","f30","f31", \
  /* 64 */ "pc", "ps"

/* UISA-level SPR names for PowerPC.  */
#define PPC_UISA_SPR_NAMES \
  /* 66 */ "cr",  "lr", "ctr", "xer", ""

/* Segment register names, for PowerPC.  */
#define PPC_SEGMENT_REG_NAMES \
  /* 71 */ "sr0", "sr1", "sr2",  "sr3",  "sr4",  "sr5",  "sr6",  "sr7", \
  /* 79 */ "sr8", "sr9", "sr10", "sr11", "sr12", "sr13", "sr14", "sr15"

/* OEA SPR names for 32-bit PowerPC implementations.
   The blank space is for "asr", which is only present on 64-bit
   implementations.  */
#define PPC_32_OEA_SPR_NAMES \
  /*  87 */ "pvr", \
  /*  88 */ "ibat0u", "ibat0l", "ibat1u", "ibat1l", \
  /*  92 */ "ibat2u", "ibat2l", "ibat3u", "ibat3l", \
  /*  96 */ "dbat0u", "dbat0l", "dbat1u", "dbat1l", \
  /* 100 */ "dbat2u", "dbat2l", "dbat3u", "dbat3l", \
  /* 104 */ "sdr1", "", "dar", "dsisr", "sprg0", "sprg1", "sprg2", "sprg3",\
  /* 112 */ "srr0", "srr1", "tbl", "tbu", "dec", "dabr", "ear"

/* For the RS6000, we only cover user-level SPR's.  */
char *register_names_rs6000[] =
{
  COMMON_UISA_REG_NAMES,
  /* 66 */ "cnd", "lr", "cnt", "xer", "mq"
};

/* a UISA-only view of the PowerPC.  */
char *register_names_uisa[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES
};

char *register_names_403[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "icdbdr", "esr", "dear", "evpr", "cdbcr", "tsr", "tcr", "pit",
  /* 127 */ "tbhi", "tblo", "srr2", "srr3", "dbsr", "dbcr", "iac1", "iac2",
  /* 135 */ "dac1", "dac2", "dccr", "iccr", "pbl1", "pbu1", "pbl2", "pbu2"
};

char *register_names_403GC[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "icdbdr", "esr", "dear", "evpr", "cdbcr", "tsr", "tcr", "pit",
  /* 127 */ "tbhi", "tblo", "srr2", "srr3", "dbsr", "dbcr", "iac1", "iac2",
  /* 135 */ "dac1", "dac2", "dccr", "iccr", "pbl1", "pbu1", "pbl2", "pbu2",
  /* 143 */ "zpr", "pid", "sgr", "dcwr", "tbhu", "tblu"
};

char *register_names_505[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "eie", "eid", "nri"
};

char *register_names_860[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "eie", "eid", "nri", "cmpa", "cmpb", "cmpc", "cmpd", "icr",
  /* 127 */ "der", "counta", "countb", "cmpe", "cmpf", "cmpg", "cmph",
  /* 134 */ "lctrl1", "lctrl2", "ictrl", "bar", "ic_cst", "ic_adr", "ic_dat",
  /* 141 */ "dc_cst", "dc_adr", "dc_dat", "dpdr", "dpir", "immr", "mi_ctr",
  /* 148 */ "mi_ap", "mi_epn", "mi_twc", "mi_rpn", "md_ctr", "m_casid",
  /* 154 */ "md_ap", "md_epn", "md_twb", "md_twc", "md_rpn", "m_tw",
  /* 160 */ "mi_dbcam", "mi_dbram0", "mi_dbram1", "md_dbcam", "md_dbram0",
  /* 165 */ "md_dbram1"
};

/* Note that the 601 has different register numbers for reading and
   writing RTCU and RTCL.  However, how one reads and writes a
   register is the stub's problem.  */
char *register_names_601[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "hid0", "hid1", "iabr", "dabr", "pir", "mq", "rtcu",
  /* 126 */ "rtcl"
};

char *register_names_602[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "hid0", "hid1", "iabr", "", "", "tcr", "ibr", "esassr", "sebr",
  /* 128 */ "ser", "sp", "lt"
};

char *register_names_603[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "hid0", "hid1", "iabr", "", "", "dmiss", "dcmp", "hash1",
  /* 127 */ "hash2", "imiss", "icmp", "rpa"
};

char *register_names_604[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "hid0", "hid1", "iabr", "dabr", "pir", "mmcr0", "pmc1", "pmc2",
  /* 127 */ "sia", "sda"
};

char *register_names_750[] =
{
  COMMON_UISA_REG_NAMES,
  PPC_UISA_SPR_NAMES,
  PPC_SEGMENT_REG_NAMES,
  PPC_32_OEA_SPR_NAMES,
  /* 119 */ "hid0", "hid1", "iabr", "dabr", "", "ummcr0", "upmc1", "upmc2",
  /* 127 */ "usia", "ummcr1", "upmc3", "upmc4", "mmcr0", "pmc1", "pmc2",
  /* 134 */ "sia", "mmcr1", "pmc3", "pmc4", "l2cr", "ictc", "thrm1", "thrm2",
  /* 142 */ "thrm3"
};


/* Information about a particular processor variant.  */
struct variant
  {
    /* Name of this variant.  */
    char *name;

    /* English description of the variant.  */
    char *description;

    /* Table of register names; registers[R] is the name of the register
       number R.  */
    int num_registers;
    char **registers;
  };

#define num_registers(list) (sizeof (list) / sizeof((list)[0]))


/* Information in this table comes from the following web sites:
   IBM:       http://www.chips.ibm.com:80/products/embedded/
   Motorola:  http://www.mot.com/SPS/PowerPC/

   I'm sure I've got some of the variant descriptions not quite right.
   Please report any inaccuracies you find to GDB's maintainer.

   If you add entries to this table, please be sure to allow the new
   value as an argument to the --with-cpu flag, in configure.in.  */

static struct variant
  variants[] =
{
  {"ppc-uisa", "PowerPC UISA - a PPC processor as viewed by user-level code",
   num_registers (register_names_uisa), register_names_uisa},
  {"rs6000", "IBM RS6000 (\"POWER\") architecture, user-level view",
   num_registers (register_names_rs6000), register_names_rs6000},
  {"403", "IBM PowerPC 403",
   num_registers (register_names_403), register_names_403},
  {"403GC", "IBM PowerPC 403GC",
   num_registers (register_names_403GC), register_names_403GC},
  {"505", "Motorola PowerPC 505",
   num_registers (register_names_505), register_names_505},
  {"860", "Motorola PowerPC 860 or 850",
   num_registers (register_names_860), register_names_860},
  {"601", "Motorola PowerPC 601",
   num_registers (register_names_601), register_names_601},
  {"602", "Motorola PowerPC 602",
   num_registers (register_names_602), register_names_602},
  {"603", "Motorola/IBM PowerPC 603 or 603e",
   num_registers (register_names_603), register_names_603},
  {"604", "Motorola PowerPC 604 or 604e",
   num_registers (register_names_604), register_names_604},
  {"750", "Motorola/IBM PowerPC 750 or 740",
   num_registers (register_names_750), register_names_750},
  {0, 0, 0, 0}
};


static struct variant *current_variant;

char *
rs6000_register_name (int i)
{
  if (i < 0 || i >= NUM_REGS)
    error ("GDB bug: rs6000-tdep.c (rs6000_register_name): strange register number");

  return ((i < current_variant->num_registers)
	  ? current_variant->registers[i]
	  : "");
}


static void
install_variant (struct variant *v)
{
  current_variant = v;
}


/* Look up the variant named NAME in the `variants' table.  Return a
   pointer to the struct variant, or null if we couldn't find it.  */
static struct variant *
find_variant_by_name (char *name)
{
  int i;

  for (i = 0; variants[i].name; i++)
    if (!strcmp (name, variants[i].name))
      return &variants[i];

  return 0;
}


/* Install the PPC/RS6000 variant named NAME in the `variants' table.
   Return zero if we installed it successfully, or a non-zero value if
   we couldn't do it.

   This might be useful to code outside this file, which doesn't want
   to depend on the exact indices of the entries in the `variants'
   table.  Just make it non-static if you want that.  */
static int
install_variant_by_name (char *name)
{
  struct variant *v = find_variant_by_name (name);

  if (v)
    {
      install_variant (v);
      return 0;
    }
  else
    return 1;
}


static void
list_variants ()
{
  int i;

  printf_filtered ("GDB knows about the following PowerPC and RS6000 variants:\n");

  for (i = 0; variants[i].name; i++)
    printf_filtered ("  %-8s  %s\n",
		     variants[i].name, variants[i].description);
}


static void
show_current_variant ()
{
  printf_filtered ("PowerPC / RS6000 processor variant is set to `%s'.\n",
		   current_variant->name);
}


static void
set_processor (char *arg, int from_tty)
{
  if (!arg || arg[0] == '\0')
    {
      list_variants ();
      return;
    }

  if (install_variant_by_name (arg))
    {
      error_begin ();
      fprintf_filtered (gdb_stderr,
	"`%s' is not a recognized PowerPC / RS6000 variant name.\n\n", arg);
      list_variants ();
      return_to_top_level (RETURN_ERROR);
    }

  show_current_variant ();
}

static void
show_processor (char *arg, int from_tty)
{
  show_current_variant ();
}




/* Initialization code.  */

void
_initialize_rs6000_tdep ()
{
  /* FIXME, this should not be decided via ifdef. */
#ifdef GDB_TARGET_POWERPC
  tm_print_insn = gdb_print_insn_powerpc;
#else
  tm_print_insn = print_insn_rs6000;
#endif

  /* I don't think we should use the set/show command arrangement
     here, because the way that's implemented makes it hard to do the
     error checking we want in a reasonable way.  So we just add them
     as two separate commands.  */
  add_cmd ("processor", class_support, set_processor,
	   "`set processor NAME' sets the PowerPC/RS6000 variant to NAME.\n\
If you set this, GDB will know about the special-purpose registers that are\n\
available on the given variant.\n\
Type `set processor' alone for a list of recognized variant names.",
	   &setlist);
  add_cmd ("processor", class_support, show_processor,
	   "Show the variant of the PowerPC or RS6000 processor in use.\n\
Use `set processor' to change this.",
	   &showlist);

  /* Set the current PPC processor variant.  */
  {
    int status = 1;

#ifdef TARGET_CPU_DEFAULT
    status = install_variant_by_name (TARGET_CPU_DEFAULT);
#endif

    if (status)
      {
#ifdef GDB_TARGET_POWERPC
	install_variant_by_name ("ppc-uisa");
#else
	install_variant_by_name ("rs6000");
#endif
      }
  }
}
