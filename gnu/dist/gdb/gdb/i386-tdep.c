/* Intel 386 target-dependent stuff.

   Copyright 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996,
   1997, 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "frame.h"
#include "inferior.h"
#include "gdbcore.h"
#include "objfiles.h"
#include "target.h"
#include "floatformat.h"
#include "symfile.h"
#include "symtab.h"
#include "gdbcmd.h"
#include "command.h"
#include "arch-utils.h"
#include "regcache.h"
#include "doublest.h"
#include "value.h"
#include "gdb_assert.h"

#include "i386-tdep.h"
#include "i387-tdep.h"

/* Names of the registers.  The first 10 registers match the register
   numbering scheme used by GCC for stabs and DWARF.  */
static char *i386_register_names[] =
{
  "eax",   "ecx",    "edx",   "ebx",
  "esp",   "ebp",    "esi",   "edi",
  "eip",   "eflags", "cs",    "ss",
  "ds",    "es",     "fs",    "gs",
  "st0",   "st1",    "st2",   "st3",
  "st4",   "st5",    "st6",   "st7",
  "fctrl", "fstat",  "ftag",  "fiseg",
  "fioff", "foseg",  "fooff", "fop",
  "xmm0",  "xmm1",   "xmm2",  "xmm3",
  "xmm4",  "xmm5",   "xmm6",  "xmm7",
  "mxcsr"
};

/* MMX registers.  */

static char *i386_mmx_names[] =
{
  "mm0", "mm1", "mm2", "mm3",
  "mm4", "mm5", "mm6", "mm7"
};
static const int mmx_num_regs = (sizeof (i386_mmx_names)
				 / sizeof (i386_mmx_names[0]));
#define MM0_REGNUM (NUM_REGS)

static int
mmx_regnum_p (int reg)
{
  return (reg >= MM0_REGNUM && reg < MM0_REGNUM + mmx_num_regs);
}

/* Return the name of register REG.  */

const char *
i386_register_name (int reg)
{
  if (reg < 0)
    return NULL;
  if (mmx_regnum_p (reg))
    return i386_mmx_names[reg - MM0_REGNUM];
  if (reg >= sizeof (i386_register_names) / sizeof (*i386_register_names))
    return NULL;

  return i386_register_names[reg];
}

/* Convert stabs register number REG to the appropriate register
   number used by GDB.  */

static int
i386_stab_reg_to_regnum (int reg)
{
  /* This implements what GCC calls the "default" register map.  */
  if (reg >= 0 && reg <= 7)
    {
      /* General registers.  */
      return reg;
    }
  else if (reg >= 12 && reg <= 19)
    {
      /* Floating-point registers.  */
      return reg - 12 + FP0_REGNUM;
    }
  else if (reg >= 21 && reg <= 28)
    {
      /* SSE registers.  */
      return reg - 21 + XMM0_REGNUM;
    }
  else if (reg >= 29 && reg <= 36)
    {
      /* MMX registers.  */
      return reg - 29 + MM0_REGNUM;
    }

  /* This will hopefully provoke a warning.  */
  return NUM_REGS + NUM_PSEUDO_REGS;
}

/* Convert DWARF register number REG to the appropriate register
   number used by GDB.  */

static int
i386_dwarf_reg_to_regnum (int reg)
{
  /* The DWARF register numbering includes %eip and %eflags, and
     numbers the floating point registers differently.  */
  if (reg >= 0 && reg <= 9)
    {
      /* General registers.  */
      return reg;
    }
  else if (reg >= 11 && reg <= 18)
    {
      /* Floating-point registers.  */
      return reg - 11 + FP0_REGNUM;
    }
  else if (reg >= 21)
    {
      /* The SSE and MMX registers have identical numbers as in stabs.  */
      return i386_stab_reg_to_regnum (reg);
    }

  /* This will hopefully provoke a warning.  */
  return NUM_REGS + NUM_PSEUDO_REGS;
}


/* This is the variable that is set with "set disassembly-flavor", and
   its legitimate values.  */
static const char att_flavor[] = "att";
static const char intel_flavor[] = "intel";
static const char *valid_flavors[] =
{
  att_flavor,
  intel_flavor,
  NULL
};
static const char *disassembly_flavor = att_flavor;

/* Stdio style buffering was used to minimize calls to ptrace, but
   this buffering did not take into account that the code section
   being accessed may not be an even number of buffers long (even if
   the buffer is only sizeof(int) long).  In cases where the code
   section size happened to be a non-integral number of buffers long,
   attempting to read the last buffer would fail.  Simply using
   target_read_memory and ignoring errors, rather than read_memory, is
   not the correct solution, since legitimate access errors would then
   be totally ignored.  To properly handle this situation and continue
   to use buffering would require that this code be able to determine
   the minimum code section size granularity (not the alignment of the
   section itself, since the actual failing case that pointed out this
   problem had a section alignment of 4 but was not a multiple of 4
   bytes long), on a target by target basis, and then adjust it's
   buffer size accordingly.  This is messy, but potentially feasible.
   It probably needs the bfd library's help and support.  For now, the
   buffer size is set to 1.  (FIXME -fnf) */

#define CODESTREAM_BUFSIZ 1	/* Was sizeof(int), see note above.  */
static CORE_ADDR codestream_next_addr;
static CORE_ADDR codestream_addr;
static unsigned char codestream_buf[CODESTREAM_BUFSIZ];
static int codestream_off;
static int codestream_cnt;

#define codestream_tell() (codestream_addr + codestream_off)
#define codestream_peek() \
  (codestream_cnt == 0 ? \
   codestream_fill(1) : codestream_buf[codestream_off])
#define codestream_get() \
  (codestream_cnt-- == 0 ? \
   codestream_fill(0) : codestream_buf[codestream_off++])

static unsigned char
codestream_fill (int peek_flag)
{
  codestream_addr = codestream_next_addr;
  codestream_next_addr += CODESTREAM_BUFSIZ;
  codestream_off = 0;
  codestream_cnt = CODESTREAM_BUFSIZ;
  read_memory (codestream_addr, (char *) codestream_buf, CODESTREAM_BUFSIZ);

  if (peek_flag)
    return (codestream_peek ());
  else
    return (codestream_get ());
}

static void
codestream_seek (CORE_ADDR place)
{
  codestream_next_addr = place / CODESTREAM_BUFSIZ;
  codestream_next_addr *= CODESTREAM_BUFSIZ;
  codestream_cnt = 0;
  codestream_fill (1);
  while (codestream_tell () != place)
    codestream_get ();
}

static void
codestream_read (unsigned char *buf, int count)
{
  unsigned char *p;
  int i;
  p = buf;
  for (i = 0; i < count; i++)
    *p++ = codestream_get ();
}


/* If the next instruction is a jump, move to its target.  */

static void
i386_follow_jump (void)
{
  unsigned char buf[4];
  long delta;

  int data16;
  CORE_ADDR pos;

  pos = codestream_tell ();

  data16 = 0;
  if (codestream_peek () == 0x66)
    {
      codestream_get ();
      data16 = 1;
    }

  switch (codestream_get ())
    {
    case 0xe9:
      /* Relative jump: if data16 == 0, disp32, else disp16.  */
      if (data16)
	{
	  codestream_read (buf, 2);
	  delta = extract_signed_integer (buf, 2);

	  /* Include the size of the jmp instruction (including the
             0x66 prefix).  */
	  pos += delta + 4;
	}
      else
	{
	  codestream_read (buf, 4);
	  delta = extract_signed_integer (buf, 4);

	  pos += delta + 5;
	}
      break;
    case 0xeb:
      /* Relative jump, disp8 (ignore data16).  */
      codestream_read (buf, 1);
      /* Sign-extend it.  */
      delta = extract_signed_integer (buf, 1);

      pos += delta + 2;
      break;
    }
  codestream_seek (pos);
}

/* Find & return the amount a local space allocated, and advance the
   codestream to the first register push (if any).

   If the entry sequence doesn't make sense, return -1, and leave
   codestream pointer at a random spot.  */

static long
i386_get_frame_setup (CORE_ADDR pc)
{
  unsigned char op;

  codestream_seek (pc);

  i386_follow_jump ();

  op = codestream_get ();

  if (op == 0x58)		/* popl %eax */
    {
      /* This function must start with

	    popl %eax             0x58
            xchgl %eax, (%esp)    0x87 0x04 0x24
         or xchgl %eax, 0(%esp)   0x87 0x44 0x24 0x00

	 (the System V compiler puts out the second `xchg'
	 instruction, and the assembler doesn't try to optimize it, so
	 the 'sib' form gets generated).  This sequence is used to get
	 the address of the return buffer for a function that returns
	 a structure.  */
      int pos;
      unsigned char buf[4];
      static unsigned char proto1[3] = { 0x87, 0x04, 0x24 };
      static unsigned char proto2[4] = { 0x87, 0x44, 0x24, 0x00 };

      pos = codestream_tell ();
      codestream_read (buf, 4);
      if (memcmp (buf, proto1, 3) == 0)
	pos += 3;
      else if (memcmp (buf, proto2, 4) == 0)
	pos += 4;

      codestream_seek (pos);
      op = codestream_get ();	/* Update next opcode.  */
    }

  if (op == 0x68 || op == 0x6a)
    {
      /* This function may start with

            pushl constant
            call _probe
	    addl $4, %esp
	   
	 followed by

            pushl %ebp

	 etc.  */
      int pos;
      unsigned char buf[8];

      /* Skip past the `pushl' instruction; it has either a one-byte 
         or a four-byte operand, depending on the opcode.  */
      pos = codestream_tell ();
      if (op == 0x68)
	pos += 4;
      else
	pos += 1;
      codestream_seek (pos);

      /* Read the following 8 bytes, which should be "call _probe" (6
         bytes) followed by "addl $4,%esp" (2 bytes).  */
      codestream_read (buf, sizeof (buf));
      if (buf[0] == 0xe8 && buf[6] == 0xc4 && buf[7] == 0x4)
	pos += sizeof (buf);
      codestream_seek (pos);
      op = codestream_get ();	/* Update next opcode.  */
    }

  if (op == 0x55)		/* pushl %ebp */
    {
      /* Check for "movl %esp, %ebp" -- can be written in two ways.  */
      switch (codestream_get ())
	{
	case 0x8b:
	  if (codestream_get () != 0xec)
	    return -1;
	  break;
	case 0x89:
	  if (codestream_get () != 0xe5)
	    return -1;
	  break;
	default:
	  return -1;
	}
      /* Check for stack adjustment 

           subl $XXX, %esp

	 NOTE: You can't subtract a 16 bit immediate from a 32 bit
	 reg, so we don't have to worry about a data16 prefix.  */
      op = codestream_peek ();
      if (op == 0x83)
	{
	  /* `subl' with 8 bit immediate.  */
	  codestream_get ();
	  if (codestream_get () != 0xec)
	    /* Some instruction starting with 0x83 other than `subl'.  */
	    {
	      codestream_seek (codestream_tell () - 2);
	      return 0;
	    }
	  /* `subl' with signed byte immediate (though it wouldn't
	     make sense to be negative).  */
	  return (codestream_get ());
	}
      else if (op == 0x81)
	{
	  char buf[4];
	  /* Maybe it is `subl' with a 32 bit immedediate.  */
	  codestream_get ();
	  if (codestream_get () != 0xec)
	    /* Some instruction starting with 0x81 other than `subl'.  */
	    {
	      codestream_seek (codestream_tell () - 2);
	      return 0;
	    }
	  /* It is `subl' with a 32 bit immediate.  */
	  codestream_read ((unsigned char *) buf, 4);
	  return extract_signed_integer (buf, 4);
	}
      else
	{
	  return 0;
	}
    }
  else if (op == 0xc8)
    {
      char buf[2];
      /* `enter' with 16 bit unsigned immediate.  */
      codestream_read ((unsigned char *) buf, 2);
      codestream_get ();	/* Flush final byte of enter instruction.  */
      return extract_unsigned_integer (buf, 2);
    }
  return (-1);
}

/* Signal trampolines don't have a meaningful frame.  The frame
   pointer value we use is actually the frame pointer of the calling
   frame -- that is, the frame which was in progress when the signal
   trampoline was entered.  GDB mostly treats this frame pointer value
   as a magic cookie.  We detect the case of a signal trampoline by
   looking at the SIGNAL_HANDLER_CALLER field, which is set based on
   PC_IN_SIGTRAMP.

   When a signal trampoline is invoked from a frameless function, we
   essentially have two frameless functions in a row.  In this case,
   we use the same magic cookie for three frames in a row.  We detect
   this case by seeing whether the next frame has
   SIGNAL_HANDLER_CALLER set, and, if it does, checking whether the
   current frame is actually frameless.  In this case, we need to get
   the PC by looking at the SP register value stored in the signal
   context.

   This should work in most cases except in horrible situations where
   a signal occurs just as we enter a function but before the frame
   has been set up.  Incidentally, that's just what happens when we
   call a function from GDB with a signal pending (there's a test in
   the testsuite that makes this happen).  Therefore we pretend that
   we have a frameless function if we're stopped at the start of a
   function.  */

/* Return non-zero if we're dealing with a frameless signal, that is,
   a signal trampoline invoked from a frameless function.  */

static int
i386_frameless_signal_p (struct frame_info *frame)
{
  return (frame->next && frame->next->signal_handler_caller
	  && (frameless_look_for_prologue (frame)
	      || frame->pc == get_pc_function_start (frame->pc)));
}

/* Return the chain-pointer for FRAME.  In the case of the i386, the
   frame's nominal address is the address of a 4-byte word containing
   the calling frame's address.  */

static CORE_ADDR
i386_frame_chain (struct frame_info *frame)
{
  if (PC_IN_CALL_DUMMY (frame->pc, 0, 0))
    return frame->frame;

  if (frame->signal_handler_caller
      || i386_frameless_signal_p (frame))
    return frame->frame;

  if (! inside_entry_file (frame->pc))
    return read_memory_unsigned_integer (frame->frame, 4);

  return 0;
}

/* Determine whether the function invocation represented by FRAME does
   not have a from on the stack associated with it.  If it does not,
   return non-zero, otherwise return zero.  */

static int
i386_frameless_function_invocation (struct frame_info *frame)
{
  if (frame->signal_handler_caller)
    return 0;

  return frameless_look_for_prologue (frame);
}

/* Assuming FRAME is for a sigtramp routine, return the saved program
   counter.  */

static CORE_ADDR
i386_sigtramp_saved_pc (struct frame_info *frame)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  CORE_ADDR addr;

  addr = tdep->sigcontext_addr (frame);
  return read_memory_unsigned_integer (addr + tdep->sc_pc_offset, 4);
}

/* Assuming FRAME is for a sigtramp routine, return the saved stack
   pointer.  */

static CORE_ADDR
i386_sigtramp_saved_sp (struct frame_info *frame)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  CORE_ADDR addr;

  addr = tdep->sigcontext_addr (frame);
  return read_memory_unsigned_integer (addr + tdep->sc_sp_offset, 4);
}

/* Return the saved program counter for FRAME.  */

static CORE_ADDR
i386_frame_saved_pc (struct frame_info *frame)
{
  if (PC_IN_CALL_DUMMY (frame->pc, 0, 0))
    return generic_read_register_dummy (frame->pc, frame->frame,
					PC_REGNUM);

  if (frame->signal_handler_caller)
    return i386_sigtramp_saved_pc (frame);

  if (i386_frameless_signal_p (frame))
    {
      CORE_ADDR sp = i386_sigtramp_saved_sp (frame->next);
      return read_memory_unsigned_integer (sp, 4);
    }

  return read_memory_unsigned_integer (frame->frame + 4, 4);
}

/* Immediately after a function call, return the saved pc.  */

static CORE_ADDR
i386_saved_pc_after_call (struct frame_info *frame)
{
  if (frame->signal_handler_caller)
    return i386_sigtramp_saved_pc (frame);

  return read_memory_unsigned_integer (read_register (SP_REGNUM), 4);
}

/* Return number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

static int
i386_frame_num_args (struct frame_info *fi)
{
#if 1
  return -1;
#else
  /* This loses because not only might the compiler not be popping the
     args right after the function call, it might be popping args from
     both this call and a previous one, and we would say there are
     more args than there really are.  */

  int retpc;
  unsigned char op;
  struct frame_info *pfi;

  /* On the i386, the instruction following the call could be:
     popl %ecx        -  one arg
     addl $imm, %esp  -  imm/4 args; imm may be 8 or 32 bits
     anything else    -  zero args.  */

  int frameless;

  frameless = FRAMELESS_FUNCTION_INVOCATION (fi);
  if (frameless)
    /* In the absence of a frame pointer, GDB doesn't get correct
       values for nameless arguments.  Return -1, so it doesn't print
       any nameless arguments.  */
    return -1;

  pfi = get_prev_frame (fi);
  if (pfi == 0)
    {
      /* NOTE: This can happen if we are looking at the frame for
         main, because FRAME_CHAIN_VALID won't let us go into start.
         If we have debugging symbols, that's not really a big deal;
         it just means it will only show as many arguments to main as
         are declared.  */
      return -1;
    }
  else
    {
      retpc = pfi->pc;
      op = read_memory_integer (retpc, 1);
      if (op == 0x59)		/* pop %ecx */
	return 1;
      else if (op == 0x83)
	{
	  op = read_memory_integer (retpc + 1, 1);
	  if (op == 0xc4)
	    /* addl $<signed imm 8 bits>, %esp */
	    return (read_memory_integer (retpc + 2, 1) & 0xff) / 4;
	  else
	    return 0;
	}
      else if (op == 0x81)	/* `add' with 32 bit immediate.  */
	{
	  op = read_memory_integer (retpc + 1, 1);
	  if (op == 0xc4)
	    /* addl $<imm 32>, %esp */
	    return read_memory_integer (retpc + 2, 4) / 4;
	  else
	    return 0;
	}
      else
	{
	  return 0;
	}
    }
#endif
}

/* Parse the first few instructions the function to see what registers
   were stored.
   
   We handle these cases:

   The startup sequence can be at the start of the function, or the
   function can start with a branch to startup code at the end.

   %ebp can be set up with either the 'enter' instruction, or "pushl
   %ebp, movl %esp, %ebp" (`enter' is too slow to be useful, but was
   once used in the System V compiler).

   Local space is allocated just below the saved %ebp by either the
   'enter' instruction, or by "subl $<size>, %esp".  'enter' has a 16
   bit unsigned argument for space to allocate, and the 'addl'
   instruction could have either a signed byte, or 32 bit immediate.

   Next, the registers used by this function are pushed.  With the
   System V compiler they will always be in the order: %edi, %esi,
   %ebx (and sometimes a harmless bug causes it to also save but not
   restore %eax); however, the code below is willing to see the pushes
   in any order, and will handle up to 8 of them.
 
   If the setup sequence is at the end of the function, then the next
   instruction will be a branch back to the start.  */

static void
i386_frame_init_saved_regs (struct frame_info *fip)
{
  long locals = -1;
  unsigned char op;
  CORE_ADDR addr;
  CORE_ADDR pc;
  int i;

  if (fip->saved_regs)
    return;

  frame_saved_regs_zalloc (fip);

  pc = get_pc_function_start (fip->pc);
  if (pc != 0)
    locals = i386_get_frame_setup (pc);

  if (locals >= 0)
    {
      addr = fip->frame - 4 - locals;
      for (i = 0; i < 8; i++)
	{
	  op = codestream_get ();
	  if (op < 0x50 || op > 0x57)
	    break;
#ifdef I386_REGNO_TO_SYMMETRY
	  /* Dynix uses different internal numbering.  Ick.  */
	  fip->saved_regs[I386_REGNO_TO_SYMMETRY (op - 0x50)] = addr;
#else
	  fip->saved_regs[op - 0x50] = addr;
#endif
	  addr -= 4;
	}
    }

  fip->saved_regs[PC_REGNUM] = fip->frame + 4;
  fip->saved_regs[FP_REGNUM] = fip->frame;
}

/* Return PC of first real instruction.  */

static CORE_ADDR
i386_skip_prologue (CORE_ADDR pc)
{
  unsigned char op;
  int i;
  static unsigned char pic_pat[6] =
  { 0xe8, 0, 0, 0, 0,		/* call   0x0 */
    0x5b,			/* popl   %ebx */
  };
  CORE_ADDR pos;

  if (i386_get_frame_setup (pc) < 0)
    return (pc);

  /* Found valid frame setup -- codestream now points to start of push
     instructions for saving registers.  */

  /* Skip over register saves.  */
  for (i = 0; i < 8; i++)
    {
      op = codestream_peek ();
      /* Break if not `pushl' instrunction.  */
      if (op < 0x50 || op > 0x57)
	break;
      codestream_get ();
    }

  /* The native cc on SVR4 in -K PIC mode inserts the following code
     to get the address of the global offset table (GOT) into register
     %ebx
     
        call	0x0
	popl    %ebx
        movl    %ebx,x(%ebp)    (optional)
        addl    y,%ebx

     This code is with the rest of the prologue (at the end of the
     function), so we have to skip it to get to the first real
     instruction at the start of the function.  */

  pos = codestream_tell ();
  for (i = 0; i < 6; i++)
    {
      op = codestream_get ();
      if (pic_pat[i] != op)
	break;
    }
  if (i == 6)
    {
      unsigned char buf[4];
      long delta = 6;

      op = codestream_get ();
      if (op == 0x89)		/* movl %ebx, x(%ebp) */
	{
	  op = codestream_get ();
	  if (op == 0x5d)	/* One byte offset from %ebp.  */
	    {
	      delta += 3;
	      codestream_read (buf, 1);
	    }
	  else if (op == 0x9d)	/* Four byte offset from %ebp.  */
	    {
	      delta += 6;
	      codestream_read (buf, 4);
	    }
	  else			/* Unexpected instruction.  */
	    delta = -1;
	  op = codestream_get ();
	}
      /* addl y,%ebx */
      if (delta > 0 && op == 0x81 && codestream_get () == 0xc3)
	{
	  pos += delta + 6;
	}
    }
  codestream_seek (pos);

  i386_follow_jump ();

  return (codestream_tell ());
}

/* Use the program counter to determine the contents and size of a
   breakpoint instruction.  Return a pointer to a string of bytes that
   encode a breakpoint instruction, store the length of the string in
   *LEN and optionally adjust *PC to point to the correct memory
   location for inserting the breakpoint.

   On the i386 we have a single breakpoint that fits in a single byte
   and can be inserted anywhere.  */
   
static const unsigned char *
i386_breakpoint_from_pc (CORE_ADDR *pc, int *len)
{
  static unsigned char break_insn[] = { 0xcc };	/* int 3 */
  
  *len = sizeof (break_insn);
  return break_insn;
}

/* Push the return address (pointing to the call dummy) onto the stack
   and return the new value for the stack pointer.  */

static CORE_ADDR
i386_push_return_address (CORE_ADDR pc, CORE_ADDR sp)
{
  char buf[4];

  store_unsigned_integer (buf, 4, CALL_DUMMY_ADDRESS ());
  write_memory (sp - 4, buf, 4);
  return sp - 4;
}

static void
i386_do_pop_frame (struct frame_info *frame)
{
  CORE_ADDR fp;
  int regnum;
  char regbuf[I386_MAX_REGISTER_SIZE];

  fp = FRAME_FP (frame);
  i386_frame_init_saved_regs (frame);

  for (regnum = 0; regnum < NUM_REGS; regnum++)
    {
      CORE_ADDR addr;
      addr = frame->saved_regs[regnum];
      if (addr)
	{
	  read_memory (addr, regbuf, REGISTER_RAW_SIZE (regnum));
	  write_register_gen (regnum, regbuf);
	}
    }
  write_register (FP_REGNUM, read_memory_integer (fp, 4));
  write_register (PC_REGNUM, read_memory_integer (fp + 4, 4));
  write_register (SP_REGNUM, fp + 8);
  flush_cached_frames ();
}

static void
i386_pop_frame (void)
{
  generic_pop_current_frame (i386_do_pop_frame);
}


/* Figure out where the longjmp will land.  Slurp the args out of the
   stack.  We expect the first arg to be a pointer to the jmp_buf
   structure from which we extract the address that we will land at.
   This address is copied into PC.  This routine returns true on
   success.  */

static int
i386_get_longjmp_target (CORE_ADDR *pc)
{
  char buf[4];
  CORE_ADDR sp, jb_addr;
  int jb_pc_offset = gdbarch_tdep (current_gdbarch)->jb_pc_offset;

  /* If JB_PC_OFFSET is -1, we have no way to find out where the
     longjmp will land.  */
  if (jb_pc_offset == -1)
    return 0;

  sp = read_register (SP_REGNUM);
  if (target_read_memory (sp + 4, buf, 4))
    return 0;

  jb_addr = extract_address (buf, 4);
  if (target_read_memory (jb_addr + jb_pc_offset, buf, 4))
    return 0;

  *pc = extract_address (buf, 4);
  return 1;
}


static CORE_ADDR
i386_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
		     int struct_return, CORE_ADDR struct_addr)
{
  sp = default_push_arguments (nargs, args, sp, struct_return, struct_addr);
  
  if (struct_return)
    {
      char buf[4];

      sp -= 4;
      store_address (buf, 4, struct_addr);
      write_memory (sp, buf, 4);
    }

  return sp;
}

static void
i386_store_struct_return (CORE_ADDR addr, CORE_ADDR sp)
{
  /* Do nothing.  Everything was already done by i386_push_arguments.  */
}

/* These registers are used for returning integers (and on some
   targets also for returning `struct' and `union' values when their
   size and alignment match an integer type).  */
#define LOW_RETURN_REGNUM 0	/* %eax */
#define HIGH_RETURN_REGNUM 2	/* %edx */

/* Extract from an array REGBUF containing the (raw) register state, a
   function return value of TYPE, and copy that, in virtual format,
   into VALBUF.  */

static void
i386_extract_return_value (struct type *type, struct regcache *regcache,
			   void *dst)
{
  bfd_byte *valbuf = dst;
  int len = TYPE_LENGTH (type);
  char buf[I386_MAX_REGISTER_SIZE];

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      && TYPE_NFIELDS (type) == 1)
    {
      i386_extract_return_value (TYPE_FIELD_TYPE (type, 0), regcache, valbuf);
      return;
    }

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      if (FP0_REGNUM == 0)
	{
	  warning ("Cannot find floating-point return value.");
	  memset (valbuf, 0, len);
	  return;
	}

      /* Floating-point return values can be found in %st(0).  Convert
	 its contents to the desired type.  This is probably not
	 exactly how it would happen on the target itself, but it is
	 the best we can do.  */
      regcache_raw_read (regcache, FP0_REGNUM, buf);
      convert_typed_floating (buf, builtin_type_i387_ext, valbuf, type);
    }
  else
    {
      int low_size = REGISTER_RAW_SIZE (LOW_RETURN_REGNUM);
      int high_size = REGISTER_RAW_SIZE (HIGH_RETURN_REGNUM);

      if (len <= low_size)
	{
	  regcache_raw_read (regcache, LOW_RETURN_REGNUM, buf);
	  memcpy (valbuf, buf, len);
	}
      else if (len <= (low_size + high_size))
	{
	  regcache_raw_read (regcache, LOW_RETURN_REGNUM, buf);
	  memcpy (valbuf, buf, low_size);
	  regcache_raw_read (regcache, HIGH_RETURN_REGNUM, buf);
	  memcpy (valbuf + low_size, buf, len - low_size);
	}
      else
	internal_error (__FILE__, __LINE__,
			"Cannot extract return value of %d bytes long.", len);
    }
}

/* Write into the appropriate registers a function return value stored
   in VALBUF of type TYPE, given in virtual format.  */

static void
i386_store_return_value (struct type *type, struct regcache *regcache,
			 const void *valbuf)
{
  int len = TYPE_LENGTH (type);

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      && TYPE_NFIELDS (type) == 1)
    {
      i386_store_return_value (TYPE_FIELD_TYPE (type, 0), regcache, valbuf);
      return;
    }

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      ULONGEST fstat;
      char buf[FPU_REG_RAW_SIZE];

      if (FP0_REGNUM == 0)
	{
	  warning ("Cannot set floating-point return value.");
	  return;
	}

      /* Returning floating-point values is a bit tricky.  Apart from
         storing the return value in %st(0), we have to simulate the
         state of the FPU at function return point.  */

      /* Convert the value found in VALBUF to the extended
	 floating-point format used by the FPU.  This is probably
	 not exactly how it would happen on the target itself, but
	 it is the best we can do.  */
      convert_typed_floating (valbuf, type, buf, builtin_type_i387_ext);
      regcache_raw_write (regcache, FP0_REGNUM, buf);

      /* Set the top of the floating-point register stack to 7.  The
         actual value doesn't really matter, but 7 is what a normal
         function return would end up with if the program started out
         with a freshly initialized FPU.  */
      regcache_raw_read_unsigned (regcache, FSTAT_REGNUM, &fstat);
      fstat |= (7 << 11);
      regcache_raw_write_unsigned (regcache, FSTAT_REGNUM, fstat);

      /* Mark %st(1) through %st(7) as empty.  Since we set the top of
         the floating-point register stack to 7, the appropriate value
         for the tag word is 0x3fff.  */
      regcache_raw_write_unsigned (regcache, FTAG_REGNUM, 0x3fff);
    }
  else
    {
      int low_size = REGISTER_RAW_SIZE (LOW_RETURN_REGNUM);
      int high_size = REGISTER_RAW_SIZE (HIGH_RETURN_REGNUM);

      if (len <= low_size)
	regcache_raw_write_part (regcache, LOW_RETURN_REGNUM, 0, len, valbuf);
      else if (len <= (low_size + high_size))
	{
	  regcache_raw_write (regcache, LOW_RETURN_REGNUM, valbuf);
	  regcache_raw_write_part (regcache, HIGH_RETURN_REGNUM, 0,
				   len - low_size, (char *) valbuf + low_size);
	}
      else
	internal_error (__FILE__, __LINE__,
			"Cannot store return value of %d bytes long.", len);
    }
}

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR.  */

static CORE_ADDR
i386_extract_struct_value_address (struct regcache *regcache)
{
  /* NOTE: cagney/2002-08-12: Replaced a call to
     regcache_raw_read_as_address() with a call to
     regcache_cooked_read_unsigned().  The old, ...as_address function
     was eventually calling extract_unsigned_integer (via
     extract_address) to unpack the registers value.  The below is
     doing an unsigned extract so that it is functionally equivalent.
     The read needs to be cooked as, otherwise, it will never
     correctly return the value of a register in the [NUM_REGS
     .. NUM_REGS+NUM_PSEUDO_REGS) range.  */
  ULONGEST val;
  regcache_cooked_read_unsigned (regcache, LOW_RETURN_REGNUM, &val);
  return val;
}


/* This is the variable that is set with "set struct-convention", and
   its legitimate values.  */
static const char default_struct_convention[] = "default";
static const char pcc_struct_convention[] = "pcc";
static const char reg_struct_convention[] = "reg";
static const char *valid_conventions[] =
{
  default_struct_convention,
  pcc_struct_convention,
  reg_struct_convention,
  NULL
};
static const char *struct_convention = default_struct_convention;

static int
i386_use_struct_convention (int gcc_p, struct type *type)
{
  enum struct_return struct_return;

  if (struct_convention == default_struct_convention)
    struct_return = gdbarch_tdep (current_gdbarch)->struct_return;
  else if (struct_convention == pcc_struct_convention)
    struct_return = pcc_struct_return;
  else
    struct_return = reg_struct_return;

  return generic_use_struct_convention (struct_return == reg_struct_return,
					type);
}


/* Return the GDB type object for the "standard" data type of data in
   register REGNUM.  Perhaps %esi and %edi should go here, but
   potentially they could be used for things other than address.  */

static struct type *
i386_register_virtual_type (int regnum)
{
  if (regnum == PC_REGNUM || regnum == FP_REGNUM || regnum == SP_REGNUM)
    return lookup_pointer_type (builtin_type_void);

  if (IS_FP_REGNUM (regnum))
    return builtin_type_i387_ext;

  if (IS_SSE_REGNUM (regnum))
    return builtin_type_vec128i;

  if (mmx_regnum_p (regnum))
    return builtin_type_vec64i;

  return builtin_type_int;
}

/* Map a cooked register onto a raw register or memory.  For the i386,
   the MMX registers need to be mapped onto floating point registers.  */

static int
mmx_regnum_to_fp_regnum (struct regcache *regcache, int regnum)
{
  int mmxi;
  ULONGEST fstat;
  int tos;
  int fpi;
  mmxi = regnum - MM0_REGNUM;
  regcache_raw_read_unsigned (regcache, FSTAT_REGNUM, &fstat);
  tos = (fstat >> 11) & 0x7;
  fpi = (mmxi + tos) % 8;
  return (FP0_REGNUM + fpi);
}

static void
i386_pseudo_register_read (struct gdbarch *gdbarch, struct regcache *regcache,
			   int regnum, void *buf)
{
  if (mmx_regnum_p (regnum))
    {
      char *mmx_buf = alloca (MAX_REGISTER_RAW_SIZE);
      int fpnum = mmx_regnum_to_fp_regnum (regcache, regnum);
      regcache_raw_read (regcache, fpnum, mmx_buf);
      /* Extract (always little endian).  */
      memcpy (buf, mmx_buf, REGISTER_RAW_SIZE (regnum));
    }
  else
    regcache_raw_read (regcache, regnum, buf);
}

static void
i386_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int regnum, const void *buf)
{
  if (mmx_regnum_p (regnum))
    {
      char *mmx_buf = alloca (MAX_REGISTER_RAW_SIZE);
      int fpnum = mmx_regnum_to_fp_regnum (regcache, regnum);
      /* Read ...  */
      regcache_raw_read (regcache, fpnum, mmx_buf);
      /* ... Modify ... (always little endian).  */
      memcpy (mmx_buf, buf, REGISTER_RAW_SIZE (regnum));
      /* ... Write.  */
      regcache_raw_write (regcache, fpnum, mmx_buf);
    }
  else
    regcache_raw_write (regcache, regnum, buf);
}

/* Return true iff register REGNUM's virtual format is different from
   its raw format.  Note that this definition assumes that the host
   supports IEEE 32-bit floats, since it doesn't say that SSE
   registers need conversion.  Even if we can't find a counterexample,
   this is still sloppy.  */

static int
i386_register_convertible (int regnum)
{
  return IS_FP_REGNUM (regnum);
}

/* Convert data from raw format for register REGNUM in buffer FROM to
   virtual format with type TYPE in buffer TO.  */

static void
i386_register_convert_to_virtual (int regnum, struct type *type,
				  char *from, char *to)
{
  gdb_assert (IS_FP_REGNUM (regnum));

  /* We only support floating-point values.  */
  if (TYPE_CODE (type) != TYPE_CODE_FLT)
    {
      warning ("Cannot convert floating-point register value "
	       "to non-floating-point type.");
      memset (to, 0, TYPE_LENGTH (type));
      return;
    }

  /* Convert to TYPE.  This should be a no-op if TYPE is equivalent to
     the extended floating-point format used by the FPU.  */
  convert_typed_floating (from, builtin_type_i387_ext, to, type);
}

/* Convert data from virtual format with type TYPE in buffer FROM to
   raw format for register REGNUM in buffer TO.  */

static void
i386_register_convert_to_raw (struct type *type, int regnum,
			      char *from, char *to)
{
  gdb_assert (IS_FP_REGNUM (regnum));

  /* We only support floating-point values.  */
  if (TYPE_CODE (type) != TYPE_CODE_FLT)
    {
      warning ("Cannot convert non-floating-point type "
	       "to floating-point register value.");
      memset (to, 0, TYPE_LENGTH (type));
      return;
    }

  /* Convert from TYPE.  This should be a no-op if TYPE is equivalent
     to the extended floating-point format used by the FPU.  */
  convert_typed_floating (from, type, to, builtin_type_i387_ext);
}
     

#ifdef STATIC_TRANSFORM_NAME
/* SunPRO encodes the static variables.  This is not related to C++
   mangling, it is done for C too.  */

char *
sunpro_static_transform_name (char *name)
{
  char *p;
  if (IS_STATIC_TRANSFORM_NAME (name))
    {
      /* For file-local statics there will be a period, a bunch of
         junk (the contents of which match a string given in the
         N_OPT), a period and the name.  For function-local statics
         there will be a bunch of junk (which seems to change the
         second character from 'A' to 'B'), a period, the name of the
         function, and the name.  So just skip everything before the
         last period.  */
      p = strrchr (name, '.');
      if (p != NULL)
	name = p + 1;
    }
  return name;
}
#endif /* STATIC_TRANSFORM_NAME */


/* Stuff for WIN32 PE style DLL's but is pretty generic really.  */

CORE_ADDR
i386_pe_skip_trampoline_code (CORE_ADDR pc, char *name)
{
  if (pc && read_memory_unsigned_integer (pc, 2) == 0x25ff) /* jmp *(dest) */
    {
      unsigned long indirect = read_memory_unsigned_integer (pc + 2, 4);
      struct minimal_symbol *indsym =
	indirect ? lookup_minimal_symbol_by_pc (indirect) : 0;
      char *symname = indsym ? SYMBOL_NAME (indsym) : 0;

      if (symname)
	{
	  if (strncmp (symname, "__imp_", 6) == 0
	      || strncmp (symname, "_imp_", 5) == 0)
	    return name ? 1 : read_memory_unsigned_integer (indirect, 4);
	}
    }
  return 0;			/* Not a trampoline.  */
}


/* Return non-zero if PC and NAME show that we are in a signal
   trampoline.  */

static int
i386_pc_in_sigtramp (CORE_ADDR pc, char *name)
{
  return (name && strcmp ("_sigtramp", name) == 0);
}


/* We have two flavours of disassembly.  The machinery on this page
   deals with switching between those.  */

static int
gdb_print_insn_i386 (bfd_vma memaddr, disassemble_info *info)
{
  if (disassembly_flavor == att_flavor)
    return print_insn_i386_att (memaddr, info);
  else if (disassembly_flavor == intel_flavor)
    return print_insn_i386_intel (memaddr, info);
  /* Never reached -- disassembly_flavour is always either att_flavor
     or intel_flavor.  */
  internal_error (__FILE__, __LINE__, "failed internal consistency check");
}


/* There are a few i386 architecture variants that differ only
   slightly from the generic i386 target.  For now, we don't give them
   their own source file, but include them here.  As a consequence,
   they'll always be included.  */

/* System V Release 4 (SVR4).  */

static int
i386_svr4_pc_in_sigtramp (CORE_ADDR pc, char *name)
{
  return (name && (strcmp ("_sigreturn", name) == 0
		   || strcmp ("_sigacthandler", name) == 0
		   || strcmp ("sigvechandler", name) == 0));
}

/* Get address of the pushed ucontext (sigcontext) on the stack for
   all three variants of SVR4 sigtramps.  */

static CORE_ADDR
i386_svr4_sigcontext_addr (struct frame_info *frame)
{
  int sigcontext_offset = -1;
  char *name = NULL;

  find_pc_partial_function (frame->pc, &name, NULL, NULL);
  if (name)
    {
      if (strcmp (name, "_sigreturn") == 0)
	sigcontext_offset = 132;
      else if (strcmp (name, "_sigacthandler") == 0)
	sigcontext_offset = 80;
      else if (strcmp (name, "sigvechandler") == 0)
	sigcontext_offset = 120;
    }

  gdb_assert (sigcontext_offset != -1);

  if (frame->next)
    return frame->next->frame + sigcontext_offset;
  return read_register (SP_REGNUM) + sigcontext_offset;
}


/* DJGPP.  */

static int
i386_go32_pc_in_sigtramp (CORE_ADDR pc, char *name)
{
  /* DJGPP doesn't have any special frames for signal handlers.  */
  return 0;
}


/* Generic ELF.  */

void
i386_elf_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* We typically use stabs-in-ELF with the DWARF register numbering.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, i386_dwarf_reg_to_regnum);
}

/* System V Release 4 (SVR4).  */

void
i386_svr4_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* System V Release 4 uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  /* System V Release 4 has shared libraries.  */
  set_gdbarch_in_solib_call_trampoline (gdbarch, in_plt_section);
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* FIXME: kettenis/20020511: Why do we override this function here?  */
  set_gdbarch_frame_chain_valid (gdbarch, generic_func_frame_chain_valid);

  set_gdbarch_pc_in_sigtramp (gdbarch, i386_svr4_pc_in_sigtramp);
  tdep->sigcontext_addr = i386_svr4_sigcontext_addr;
  tdep->sc_pc_offset = 14 * 4;
  tdep->sc_sp_offset = 7 * 4;

  tdep->jb_pc_offset = 20;
}

/* DJGPP.  */

static void
i386_go32_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  set_gdbarch_pc_in_sigtramp (gdbarch, i386_go32_pc_in_sigtramp);

  tdep->jb_pc_offset = 36;
}

/* NetWare.  */

static void
i386_nw_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* FIXME: kettenis/20020511: Why do we override this function here?  */
  set_gdbarch_frame_chain_valid (gdbarch, generic_func_frame_chain_valid);

  tdep->jb_pc_offset = 24;
}


static struct gdbarch *
i386_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch_tdep *tdep;
  struct gdbarch *gdbarch;
  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;

  /* Try to determine the OS ABI of the object we're loading.  */
  if (info.abfd != NULL)
    osabi = gdbarch_lookup_osabi (info.abfd);

  /* Find a candidate among extant architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      /* Make sure the OS ABI selection matches.  */
      tdep = gdbarch_tdep (arches->gdbarch);
      if (tdep && tdep->osabi == osabi)
        return arches->gdbarch;
    }

  /* Allocate space for the new architecture.  */
  tdep = XMALLOC (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  tdep->osabi = osabi;

  /* The i386 default settings don't include the SSE registers.
     FIXME: kettenis/20020614: They do include the FPU registers for
     now, which probably is not quite right.  */
  tdep->num_xmm_regs = 0;

  tdep->jb_pc_offset = -1;
  tdep->struct_return = pcc_struct_return;
  tdep->sigtramp_start = 0;
  tdep->sigtramp_end = 0;
  tdep->sigcontext_addr = NULL;
  tdep->sc_pc_offset = -1;
  tdep->sc_sp_offset = -1;

  /* The format used for `long double' on almost all i386 targets is
     the i387 extended floating-point format.  In fact, of all targets
     in the GCC 2.95 tree, only OSF/1 does it different, and insists
     on having a `long double' that's not `long' at all.  */
  set_gdbarch_long_double_format (gdbarch, &floatformat_i387_ext);

  /* Although the i386 extended floating-point has only 80 significant
     bits, a `long double' actually takes up 96, probably to enforce
     alignment.  */
  set_gdbarch_long_double_bit (gdbarch, 96);

  /* NOTE: tm-i386aix.h, tm-i386bsd.h, tm-i386os9k.h, tm-ptx.h,
     tm-symmetry.h currently override this.  Sigh.  */
  set_gdbarch_num_regs (gdbarch, I386_NUM_GREGS + I386_NUM_FREGS);

  set_gdbarch_sp_regnum (gdbarch, 4);
  set_gdbarch_fp_regnum (gdbarch, 5);
  set_gdbarch_pc_regnum (gdbarch, 8);
  set_gdbarch_ps_regnum (gdbarch, 9);
  set_gdbarch_fp0_regnum (gdbarch, 16);

  /* Use the "default" register numbering scheme for stabs and COFF.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, i386_stab_reg_to_regnum);
  set_gdbarch_sdb_reg_to_regnum (gdbarch, i386_stab_reg_to_regnum);

  /* Use the DWARF register numbering scheme for DWARF and DWARF 2.  */
  set_gdbarch_dwarf_reg_to_regnum (gdbarch, i386_dwarf_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, i386_dwarf_reg_to_regnum);

  /* We don't define ECOFF_REG_TO_REGNUM, since ECOFF doesn't seem to
     be in use on any of the supported i386 targets.  */

  set_gdbarch_register_name (gdbarch, i386_register_name);
  set_gdbarch_register_size (gdbarch, 4);
  set_gdbarch_register_bytes (gdbarch, I386_SIZEOF_GREGS + I386_SIZEOF_FREGS);
  set_gdbarch_max_register_raw_size (gdbarch, I386_MAX_REGISTER_SIZE);
  set_gdbarch_max_register_virtual_size (gdbarch, I386_MAX_REGISTER_SIZE);
  set_gdbarch_register_virtual_type (gdbarch, i386_register_virtual_type);

  set_gdbarch_print_float_info (gdbarch, i387_print_float_info);

  set_gdbarch_get_longjmp_target (gdbarch, i386_get_longjmp_target);

  set_gdbarch_use_generic_dummy_frames (gdbarch, 1);

  /* Call dummy code.  */
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_call_dummy_address (gdbarch, entry_point_address);
  set_gdbarch_call_dummy_start_offset (gdbarch, 0);
  set_gdbarch_call_dummy_breakpoint_offset (gdbarch, 0);
  set_gdbarch_call_dummy_breakpoint_offset_p (gdbarch, 1);
  set_gdbarch_call_dummy_length (gdbarch, 0);
  set_gdbarch_call_dummy_p (gdbarch, 1);
  set_gdbarch_call_dummy_words (gdbarch, NULL);
  set_gdbarch_sizeof_call_dummy_words (gdbarch, 0);
  set_gdbarch_call_dummy_stack_adjust_p (gdbarch, 0);
  set_gdbarch_fix_call_dummy (gdbarch, generic_fix_call_dummy);

  set_gdbarch_register_convertible (gdbarch, i386_register_convertible);
  set_gdbarch_register_convert_to_virtual (gdbarch,
					   i386_register_convert_to_virtual);
  set_gdbarch_register_convert_to_raw (gdbarch, i386_register_convert_to_raw);

  set_gdbarch_get_saved_register (gdbarch, generic_unwind_get_saved_register);
  set_gdbarch_push_arguments (gdbarch, i386_push_arguments);

  set_gdbarch_pc_in_call_dummy (gdbarch, pc_in_call_dummy_at_entry_point);

  /* "An argument's size is increased, if necessary, to make it a
     multiple of [32-bit] words.  This may require tail padding,
     depending on the size of the argument" -- from the x86 ABI.  */
  set_gdbarch_parm_boundary (gdbarch, 32);

  set_gdbarch_extract_return_value (gdbarch, i386_extract_return_value);
  set_gdbarch_push_arguments (gdbarch, i386_push_arguments);
  set_gdbarch_push_dummy_frame (gdbarch, generic_push_dummy_frame);
  set_gdbarch_push_return_address (gdbarch, i386_push_return_address);
  set_gdbarch_pop_frame (gdbarch, i386_pop_frame);
  set_gdbarch_store_struct_return (gdbarch, i386_store_struct_return);
  set_gdbarch_store_return_value (gdbarch, i386_store_return_value);
  set_gdbarch_extract_struct_value_address (gdbarch,
					    i386_extract_struct_value_address);
  set_gdbarch_use_struct_convention (gdbarch, i386_use_struct_convention);

  set_gdbarch_frame_init_saved_regs (gdbarch, i386_frame_init_saved_regs);
  set_gdbarch_skip_prologue (gdbarch, i386_skip_prologue);

  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_from_pc (gdbarch, i386_breakpoint_from_pc);
  set_gdbarch_decr_pc_after_break (gdbarch, 1);
  set_gdbarch_function_start_offset (gdbarch, 0);

  /* The following redefines make backtracing through sigtramp work.
     They manufacture a fake sigtramp frame and obtain the saved pc in
     sigtramp from the sigcontext structure which is pushed by the
     kernel on the user stack, along with a pointer to it.  */

  set_gdbarch_frame_args_skip (gdbarch, 8);
  set_gdbarch_frameless_function_invocation (gdbarch,
                                           i386_frameless_function_invocation);
  set_gdbarch_frame_chain (gdbarch, i386_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, generic_file_frame_chain_valid);
  set_gdbarch_frame_saved_pc (gdbarch, i386_frame_saved_pc);
  set_gdbarch_frame_args_address (gdbarch, default_frame_address);
  set_gdbarch_frame_locals_address (gdbarch, default_frame_address);
  set_gdbarch_saved_pc_after_call (gdbarch, i386_saved_pc_after_call);
  set_gdbarch_frame_num_args (gdbarch, i386_frame_num_args);
  set_gdbarch_pc_in_sigtramp (gdbarch, i386_pc_in_sigtramp);

  /* Wire in the MMX registers.  */
  set_gdbarch_num_pseudo_regs (gdbarch, mmx_num_regs);
  set_gdbarch_pseudo_register_read (gdbarch, i386_pseudo_register_read);
  set_gdbarch_pseudo_register_write (gdbarch, i386_pseudo_register_write);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch, osabi);

  return gdbarch;
}

static enum gdb_osabi
i386_coff_osabi_sniffer (bfd *abfd)
{
  if (strcmp (bfd_get_target (abfd), "coff-go32-exe") == 0
      || strcmp (bfd_get_target (abfd), "coff-go32") == 0)
    return GDB_OSABI_GO32;

  return GDB_OSABI_UNKNOWN;
}

static enum gdb_osabi
i386_nlm_osabi_sniffer (bfd *abfd)
{
  return GDB_OSABI_NETWARE;
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386_tdep (void);

void
_initialize_i386_tdep (void)
{
  register_gdbarch_init (bfd_arch_i386, i386_gdbarch_init);

  tm_print_insn = gdb_print_insn_i386;
  tm_print_insn_info.mach = bfd_lookup_arch (bfd_arch_i386, 0)->mach;

  /* Add the variable that controls the disassembly flavor.  */
  {
    struct cmd_list_element *new_cmd;

    new_cmd = add_set_enum_cmd ("disassembly-flavor", no_class,
				valid_flavors,
				&disassembly_flavor,
				"\
Set the disassembly flavor, the valid values are \"att\" and \"intel\", \
and the default value is \"att\".",
				&setlist);
    add_show_from_set (new_cmd, &showlist);
  }

  /* Add the variable that controls the convention for returning
     structs.  */
  {
    struct cmd_list_element *new_cmd;

    new_cmd = add_set_enum_cmd ("struct-convention", no_class,
				 valid_conventions,
				&struct_convention, "\
Set the convention for returning small structs, valid values \
are \"default\", \"pcc\" and \"reg\", and the default value is \"default\".",
                                &setlist);
    add_show_from_set (new_cmd, &showlist);
  }

  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_coff_flavour,
				  i386_coff_osabi_sniffer);
  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_nlm_flavour,
				  i386_nlm_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_i386, GDB_OSABI_SVR4,
			  i386_svr4_init_abi);
  gdbarch_register_osabi (bfd_arch_i386, GDB_OSABI_GO32,
			  i386_go32_init_abi);
  gdbarch_register_osabi (bfd_arch_i386, GDB_OSABI_NETWARE,
			  i386_nw_init_abi);
}
