/* Target-dependent code for the MIPS architecture, for GDB, the GNU Debugger.
   Copyright 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996
   Free Software Foundation, Inc.
   Contributed by Alessandro Forin(af@cs.cmu.edu) at CMU
   and by Per Bothner(bothner@cs.wisc.edu) at U.Wisconsin.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "gdb_string.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "value.h"
#include "gdbcmd.h"
#include "language.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "target.h"

#include "opcode/mips.h"

#define VM_MIN_ADDRESS (CORE_ADDR)0x400000

/* FIXME: Put this declaration in frame.h.  */
extern struct obstack frame_cache_obstack;

#if 0
static int mips_in_lenient_prologue PARAMS ((CORE_ADDR, CORE_ADDR));
#endif

static int gdb_print_insn_mips PARAMS ((bfd_vma, disassemble_info *));

static void mips_print_register PARAMS ((int, int));

static mips_extra_func_info_t
heuristic_proc_desc PARAMS ((CORE_ADDR, CORE_ADDR, struct frame_info *));

static CORE_ADDR heuristic_proc_start PARAMS ((CORE_ADDR));

static CORE_ADDR read_next_frame_reg PARAMS ((struct frame_info *, int));

static void mips_set_fpu_command PARAMS ((char *, int,
					  struct cmd_list_element *));

static void mips_show_fpu_command PARAMS ((char *, int,
					   struct cmd_list_element *));

void mips_set_processor_type_command PARAMS ((char *, int));

int mips_set_processor_type PARAMS ((char *));

static void mips_show_processor_type_command PARAMS ((char *, int));

static void reinit_frame_cache_sfunc PARAMS ((char *, int,
					      struct cmd_list_element *));

static mips_extra_func_info_t
  find_proc_desc PARAMS ((CORE_ADDR pc, struct frame_info *next_frame));

static CORE_ADDR after_prologue PARAMS ((CORE_ADDR pc,
					 mips_extra_func_info_t proc_desc));

/* This value is the model of MIPS in use.  It is derived from the value
   of the PrID register.  */

char *mips_processor_type;

char *tmp_mips_processor_type;

/* Some MIPS boards don't support floating point, so we permit the
   user to turn it off.  */

enum mips_fpu_type mips_fpu;

static char *mips_fpu_string;

/* A set of original names, to be used when restoring back to generic
   registers from a specific set.  */

char *mips_generic_reg_names[] = REGISTER_NAMES;

/* Names of IDT R3041 registers.  */

char *mips_r3041_reg_names[] = {
	"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3",
	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7",
	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7",
	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra",
	"sr",	"lo",	"hi",	"bad",	"cause","pc",
	"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
	"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
	"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
	"f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",
	"fsr",  "fir",  "fp",	"",
	"",	"",	"bus",	"ccfg",	"",	"",	"",	"",
	"",	"",	"port",	"cmp",	"",	"",	"epc",	"prid",
};

/* Names of IDT R3051 registers.  */

char *mips_r3051_reg_names[] = {
	"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3",
	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7",
	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7",
	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra",
	"sr",	"lo",	"hi",	"bad",	"cause","pc",
	"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
	"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
	"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
	"f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",
	"fsr",  "fir",  "fp",	"",
	"inx",	"rand",	"elo",	"",	"ctxt",	"",	"",	"",
	"",	"",	"ehi",	"",	"",	"",	"epc",	"prid",
};

/* Names of IDT R3081 registers.  */

char *mips_r3081_reg_names[] = {
	"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3",
	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7",
	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7",
	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra",
	"sr",	"lo",	"hi",	"bad",	"cause","pc",
	"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
	"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
	"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
	"f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",
	"fsr",  "fir",  "fp",	"",
	"inx",	"rand",	"elo",	"cfg",	"ctxt",	"",	"",	"",
	"",	"",	"ehi",	"",	"",	"",	"epc",	"prid",
};

/* Names of LSI 33k registers.  */

char *mips_lsi33k_reg_names[] = {
	"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3",
	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7",
	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7",
	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra",
	"epc",	"hi",	"lo",	"sr",	"cause","badvaddr",
	"dcic", "bpc",  "bda",  "",     "",     "",     "",      "",
	"",     "",     "",     "",     "",     "",     "",      "",
	"",     "",     "",     "",     "",     "",     "",      "",
	"",     "",     "",     "",     "",     "",     "",      "",
	"",     "",     "",	"",
	"",	"",	"",	"",	"",	"",	"",	 "",
	"",	"",	"",	"",	"",	"",	"",	 "",
};

struct {
  char *name;
  char **regnames;
} mips_processor_type_table[] = {
  { "generic", mips_generic_reg_names },
  { "r3041", mips_r3041_reg_names },
  { "r3051", mips_r3051_reg_names },
  { "r3071", mips_r3081_reg_names },
  { "r3081", mips_r3081_reg_names },
  { "lsi33k", mips_lsi33k_reg_names },
  { NULL, NULL }
};

/* Table to translate MIPS16 register field to actual register number.  */
static int mips16_to_32_reg[8] = { 16, 17, 2, 3, 4, 5, 6, 7 };

/* Heuristic_proc_start may hunt through the text section for a long
   time across a 2400 baud serial line.  Allows the user to limit this
   search.  */

static unsigned int heuristic_fence_post = 0;

#define PROC_LOW_ADDR(proc) ((proc)->pdr.adr) /* least address */
#define PROC_HIGH_ADDR(proc) ((proc)->high_addr) /* upper address bound */
#define PROC_FRAME_OFFSET(proc) ((proc)->pdr.frameoffset)
#define PROC_FRAME_REG(proc) ((proc)->pdr.framereg)
#define PROC_FRAME_ADJUST(proc)  ((proc)->frame_adjust)
#define PROC_REG_MASK(proc) ((proc)->pdr.regmask)
#define PROC_FREG_MASK(proc) ((proc)->pdr.fregmask)
#define PROC_REG_OFFSET(proc) ((proc)->pdr.regoffset)
#define PROC_FREG_OFFSET(proc) ((proc)->pdr.fregoffset)
#define PROC_PC_REG(proc) ((proc)->pdr.pcreg)
#define PROC_SYMBOL(proc) (*(struct symbol**)&(proc)->pdr.isym)
#define _PROC_MAGIC_ 0x0F0F0F0F
#define PROC_DESC_IS_DUMMY(proc) ((proc)->pdr.isym == _PROC_MAGIC_)
#define SET_PROC_DESC_IS_DUMMY(proc) ((proc)->pdr.isym = _PROC_MAGIC_)

struct linked_proc_info
{
  struct mips_extra_func_info info;
  struct linked_proc_info *next;
} *linked_proc_desc_table = NULL;


/* Tell if the program counter value in MEMADDR is in a MIPS16 function.  */

static int
pc_is_mips16 (bfd_vma memaddr)
{
  struct minimal_symbol *sym;

  /* If bit 0 of the address is set, assume this is a MIPS16 address. */
  if (IS_MIPS16_ADDR (memaddr))
    return 1;

  /* A flag indicating that this is a MIPS16 function is stored by elfread.c in
     the high bit of the info field.  Use this to decide if the function is
     MIPS16 or normal MIPS.  */
  sym = lookup_minimal_symbol_by_pc (memaddr);
  if (sym)
    return MSYMBOL_IS_SPECIAL (sym);
  else
    return 0;
}


/* This returns the PC of the first inst after the prologue.  If we can't
   find the prologue, then return 0.  */

static CORE_ADDR
after_prologue (pc, proc_desc)
     CORE_ADDR pc;
     mips_extra_func_info_t proc_desc;
{
  struct symtab_and_line sal;
  CORE_ADDR func_addr, func_end;

  if (!proc_desc)
    proc_desc = find_proc_desc (pc, NULL);

  if (proc_desc)
    {
      /* If function is frameless, then we need to do it the hard way.  I
	 strongly suspect that frameless always means prologueless... */
      if (PROC_FRAME_REG (proc_desc) == SP_REGNUM
	  && PROC_FRAME_OFFSET (proc_desc) == 0)
	return 0;
    }

  if (!find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    return 0;			/* Unknown */

  sal = find_pc_line (func_addr, 0);

  if (sal.end < func_end)
    return sal.end;

  /* The line after the prologue is after the end of the function.  In this
     case, tell the caller to find the prologue the hard way.  */

  return 0;
}

/* Decode a MIPS32 instruction that saves a register in the stack, and
   set the appropriate bit in the general register mask or float register mask
   to indicate which register is saved.  This is a helper function
   for mips_find_saved_regs.  */

static void
mips32_decode_reg_save (inst, gen_mask, float_mask)
     t_inst inst;
     unsigned long *gen_mask;
     unsigned long *float_mask;
{
  int reg;

  if ((inst & 0xffe00000) == 0xafa00000		/* sw reg,n($sp) */
      || (inst & 0xffe00000) == 0xafc00000	/* sw reg,n($r30) */
      || (inst & 0xffe00000) == 0xffa00000)	/* sd reg,n($sp) */
    {
      /* It might be possible to use the instruction to
	 find the offset, rather than the code below which
	 is based on things being in a certain order in the
	 frame, but figuring out what the instruction's offset
	 is relative to might be a little tricky.  */
      reg = (inst & 0x001f0000) >> 16;
      *gen_mask |= (1 << reg);
    }
  else if ((inst & 0xffe00000) == 0xe7a00000	/* swc1 freg,n($sp) */
	   || (inst & 0xffe00000) == 0xe7c00000	/* swc1 freg,n($r30) */
	   || (inst & 0xffe00000) == 0xf7a00000)/* sdc1 freg,n($sp) */

    {
      reg = ((inst & 0x001f0000) >> 16);
      *float_mask |= (1 << reg);
    }
}

/* Decode a MIPS16 instruction that saves a register in the stack, and
   set the appropriate bit in the general register or float register mask
   to indicate which register is saved.  This is a helper function
   for mips_find_saved_regs.  */

static void
mips16_decode_reg_save (inst, gen_mask)
     t_inst inst;
     unsigned long *gen_mask;
{
  if ((inst & 0xf800) == 0xd000)		/* sw reg,n($sp) */
    {
      int reg = mips16_to_32_reg[(inst & 0x700) >> 8];
      *gen_mask |= (1 << reg);
    }
  else if ((inst & 0xff00) == 0xf900)		/* sd reg,n($sp) */
    {
      int reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
      *gen_mask |= (1 << reg);
    }
  else if ((inst & 0xff00) == 0x6200		/* sw $ra,n($sp) */
	   || (inst & 0xff00) == 0xfa00)	/* sd $ra,n($sp) */
    *gen_mask |= (1 << RA_REGNUM);
}


/* Fetch and return instruction from the specified location.  If the PC
   is odd, assume it's a MIPS16 instruction; otherwise MIPS32.  */

static t_inst
mips_fetch_instruction (addr)
    CORE_ADDR addr;
{
  char buf[MIPS_INSTLEN];
  int instlen;
  int status;

  if (pc_is_mips16 (addr))
    {
      instlen = MIPS16_INSTLEN;
      addr = UNMAKE_MIPS16_ADDR (addr);
    }
  else
      instlen = MIPS_INSTLEN;
  status = read_memory_nobpt (addr, buf, instlen);
  if (status)
    memory_error (status, addr);
  return extract_unsigned_integer (buf, instlen);
}


/* Guaranteed to set fci->saved_regs to some values (it never leaves it
   NULL).  */

void
mips_find_saved_regs (fci)
     struct frame_info *fci;
{
  int ireg;
  CORE_ADDR reg_position;
  /* r0 bit means kernel trap */
  int kernel_trap;
  /* What registers have been saved?  Bitmasks.  */
  unsigned long gen_mask, float_mask;
  mips_extra_func_info_t proc_desc;
  t_inst inst;

  fci->saved_regs = (struct frame_saved_regs *)
    obstack_alloc (&frame_cache_obstack, sizeof(struct frame_saved_regs));
  memset (fci->saved_regs, 0, sizeof (struct frame_saved_regs));

  /* If it is the frame for sigtramp, the saved registers are located
     in a sigcontext structure somewhere on the stack.
     If the stack layout for sigtramp changes we might have to change these
     constants and the companion fixup_sigtramp in mdebugread.c  */
#ifndef SIGFRAME_BASE
/* To satisfy alignment restrictions, sigcontext is located 4 bytes
   above the sigtramp frame.  */
#define SIGFRAME_BASE		MIPS_REGSIZE
/* FIXME!  Are these correct?? */
#define SIGFRAME_PC_OFF		(SIGFRAME_BASE + 2 * MIPS_REGSIZE)
#define SIGFRAME_REGSAVE_OFF	(SIGFRAME_BASE + 3 * MIPS_REGSIZE)
#define SIGFRAME_FPREGSAVE_OFF	\
        (SIGFRAME_REGSAVE_OFF + MIPS_NUMREGS * MIPS_REGSIZE + 3 * MIPS_REGSIZE)
#endif
#ifndef SIGFRAME_REG_SIZE
/* FIXME!  Is this correct?? */
#define SIGFRAME_REG_SIZE	MIPS_REGSIZE
#endif
  if (fci->signal_handler_caller)
    {
      for (ireg = 0; ireg < MIPS_NUMREGS; ireg++)
	{
 	  reg_position = fci->frame + SIGFRAME_REGSAVE_OFF
			 + ireg * SIGFRAME_REG_SIZE;
 	  fci->saved_regs->regs[ireg] = reg_position;
	}
      for (ireg = 0; ireg < MIPS_NUMREGS; ireg++)
	{
 	  reg_position = fci->frame + SIGFRAME_FPREGSAVE_OFF
			 + ireg * SIGFRAME_REG_SIZE;
 	  fci->saved_regs->regs[FP0_REGNUM + ireg] = reg_position;
	}
      fci->saved_regs->regs[PC_REGNUM] = fci->frame + SIGFRAME_PC_OFF;
      return;
    }

  proc_desc = fci->proc_desc;
  if (proc_desc == NULL)
    /* I'm not sure how/whether this can happen.  Normally when we can't
       find a proc_desc, we "synthesize" one using heuristic_proc_desc
       and set the saved_regs right away.  */
    return;

  kernel_trap = PROC_REG_MASK(proc_desc) & 1;
  gen_mask = kernel_trap ? 0xFFFFFFFF : PROC_REG_MASK(proc_desc);
  float_mask = kernel_trap ? 0xFFFFFFFF : PROC_FREG_MASK(proc_desc);

  if (/* In any frame other than the innermost or a frame interrupted by
	 a signal, we assume that all registers have been saved.
	 This assumes that all register saves in a function happen before
	 the first function call.  */
      (fci->next == NULL || fci->next->signal_handler_caller)

      /* In a dummy frame we know exactly where things are saved.  */
      && !PROC_DESC_IS_DUMMY (proc_desc)

      /* Don't bother unless we are inside a function prologue.  Outside the
	 prologue, we know where everything is. */

      && in_prologue (fci->pc, PROC_LOW_ADDR (proc_desc))

      /* Not sure exactly what kernel_trap means, but if it means
	 the kernel saves the registers without a prologue doing it,
	 we better not examine the prologue to see whether registers
	 have been saved yet.  */
      && !kernel_trap)
    {
      /* We need to figure out whether the registers that the proc_desc
	 claims are saved have been saved yet.  */

      CORE_ADDR addr;

      /* Bitmasks; set if we have found a save for the register.  */
      unsigned long gen_save_found = 0;
      unsigned long float_save_found = 0;
      int instlen;

      /* If the address is odd, assume this is MIPS16 code.  */
      addr = PROC_LOW_ADDR (proc_desc);
      instlen = pc_is_mips16 (addr) ? MIPS16_INSTLEN : MIPS_INSTLEN;

      /* Scan through this function's instructions preceding the current
         PC, and look for those that save registers.  */
      while (addr < fci->pc)
	{
	  inst = mips_fetch_instruction (addr);
	  if (pc_is_mips16 (addr))
	    mips16_decode_reg_save (inst, &gen_save_found);
	  else
	    mips32_decode_reg_save (inst, &gen_save_found, &float_save_found);
	  addr += instlen;
	}
      gen_mask = gen_save_found;
      float_mask = float_save_found;
    }

  /* Fill in the offsets for the registers which gen_mask says
     were saved.  */
  reg_position = fci->frame + PROC_REG_OFFSET (proc_desc);
  for (ireg= MIPS_NUMREGS-1; gen_mask; --ireg, gen_mask <<= 1)
    if (gen_mask & 0x80000000)
      {
	fci->saved_regs->regs[ireg] = reg_position;
	reg_position -= MIPS_REGSIZE;
      }

  /* The MIPS16 entry instruction saves $s0 and $s1 in the reverse order
     of that normally used by gcc.  Therefore, we have to fetch the first
     instruction of the function, and if it's an entry instruction that
     saves $s0 or $s1, correct their saved addresses.  */
  if (pc_is_mips16 (PROC_LOW_ADDR (proc_desc)))
    {
      inst = mips_fetch_instruction (PROC_LOW_ADDR (proc_desc));
      if ((inst & 0xf81f) == 0xe809 && (inst & 0x700) != 0x700) /* entry */
	{
	  int reg;
	  int sreg_count = (inst >> 6) & 3;
	  
	  /* Check if the ra register was pushed on the stack.  */
	  reg_position = fci->frame + PROC_REG_OFFSET (proc_desc);
	  if (inst & 0x20)
	    reg_position -= MIPS_REGSIZE;

	  /* Check if the s0 and s1 registers were pushed on the stack.  */
	  for (reg = 16; reg < sreg_count+16; reg++)
	    {
	      fci->saved_regs->regs[reg] = reg_position;
	      reg_position -= MIPS_REGSIZE;
	    }
	}
    }

  /* Fill in the offsets for the registers which float_mask says
     were saved.  */
  reg_position = fci->frame + PROC_FREG_OFFSET (proc_desc);

  /* The freg_offset points to where the first *double* register
     is saved.  So skip to the high-order word. */
  if (! GDB_TARGET_IS_MIPS64)
    reg_position += MIPS_REGSIZE;

  /* Fill in the offsets for the float registers which float_mask says
     were saved.  */
  for (ireg = MIPS_NUMREGS-1; float_mask; --ireg, float_mask <<= 1)
    if (float_mask & 0x80000000)
      {
	fci->saved_regs->regs[FP0_REGNUM+ireg] = reg_position;
	reg_position -= MIPS_REGSIZE;
      }

  fci->saved_regs->regs[PC_REGNUM] = fci->saved_regs->regs[RA_REGNUM];
}

static CORE_ADDR
read_next_frame_reg(fi, regno)
     struct frame_info *fi;
     int regno;
{
  for (; fi; fi = fi->next)
    {
      /* We have to get the saved sp from the sigcontext
	 if it is a signal handler frame.  */
      if (regno == SP_REGNUM && !fi->signal_handler_caller)
	return fi->frame;
      else
	{
	  if (fi->saved_regs == NULL)
	    mips_find_saved_regs (fi);
	  if (fi->saved_regs->regs[regno])
	    return read_memory_integer(fi->saved_regs->regs[regno], MIPS_REGSIZE);
	}
    }
  return read_register (regno);
}

/* mips_addr_bits_remove - remove useless address bits  */

CORE_ADDR
mips_addr_bits_remove (addr)
    CORE_ADDR addr;
{
#if GDB_TARGET_IS_MIPS64
  if ((addr >> 32 == (CORE_ADDR)0xffffffff)
      && (strcmp (target_shortname,"pmon")==0
	 || strcmp (target_shortname,"ddb")==0
	 || strcmp (target_shortname,"sim")==0))
    {
      /* This hack is a work-around for existing boards using PMON,
	 the simulator, and any other 64-bit targets that doesn't have
	 true 64-bit addressing.  On these targets, the upper 32 bits
	 of addresses are ignored by the hardware.  Thus, the PC or SP
	 are likely to have been sign extended to all 1s by instruction
	 sequences that load 32-bit addresses.  For example, a typical
	 piece of code that loads an address is this:
		lui $r2, <upper 16 bits>
		ori $r2, <lower 16 bits>
	 But the lui sign-extends the value such that the upper 32 bits
	 may be all 1s.  The workaround is simply to mask off these bits.
	 In the future, gcc may be changed to support true 64-bit
	 addressing, and this masking will have to be disabled.  */
        addr &= (CORE_ADDR)0xffffffff;
    }
#else
  /* Even when GDB is configured for some 32-bit targets (e.g. mips-elf),
     BFD is configured to handle 64-bit targets, so CORE_ADDR is 64 bits.
     So we still have to mask off useless bits from addresses.  */
  addr &= (CORE_ADDR)0xffffffff;
#endif

  return addr;
}

void
mips_init_frame_pc_first (fromleaf, prev)
     int fromleaf;
     struct frame_info *prev;
{
  CORE_ADDR pc, tmp;

  pc = ((fromleaf) ? SAVED_PC_AFTER_CALL (prev->next) :
        prev->next ? FRAME_SAVED_PC (prev->next) : read_pc ());
  tmp = mips_skip_stub (pc);
  prev->pc =  tmp ? tmp : pc;
}


CORE_ADDR
mips_frame_saved_pc(frame)
     struct frame_info *frame;
{
  CORE_ADDR saved_pc;
  mips_extra_func_info_t proc_desc = frame->proc_desc;
  /* We have to get the saved pc from the sigcontext
     if it is a signal handler frame.  */
  int pcreg = frame->signal_handler_caller ? PC_REGNUM
	      : (proc_desc ? PROC_PC_REG(proc_desc) : RA_REGNUM);

  if (proc_desc && PROC_DESC_IS_DUMMY(proc_desc))
    saved_pc = read_memory_integer(frame->frame - MIPS_REGSIZE, MIPS_REGSIZE);
  else
    saved_pc = read_next_frame_reg(frame, pcreg);

  return ADDR_BITS_REMOVE (saved_pc);
}

static struct mips_extra_func_info temp_proc_desc;
static struct frame_saved_regs temp_saved_regs;

/* Set a register's saved stack address in temp_saved_regs.  If an address
   has already been set for this register, do nothing; this way we will
   only recognize the first save of a given register in a function prologue.
   This is a helper function for mips{16,32}_heuristic_proc_desc.  */

static void
set_reg_offset (regno, offset)
     int regno;
     CORE_ADDR offset;
{
  if (temp_saved_regs.regs[regno] == 0)
    temp_saved_regs.regs[regno] = offset;
}


/* This fencepost looks highly suspicious to me.  Removing it also
   seems suspicious as it could affect remote debugging across serial
   lines.  */

static CORE_ADDR
heuristic_proc_start(pc)
    CORE_ADDR pc;
{
    CORE_ADDR start_pc;
    CORE_ADDR fence;
    int instlen;
    int seen_adjsp = 0;

    pc = ADDR_BITS_REMOVE (pc);
    start_pc = pc;
    fence = start_pc - heuristic_fence_post;
    if (start_pc == 0)	return 0;

    if (heuristic_fence_post == UINT_MAX
	|| fence < VM_MIN_ADDRESS)
      fence = VM_MIN_ADDRESS;

    instlen = pc_is_mips16 (pc) ? MIPS16_INSTLEN : MIPS_INSTLEN;

    /* search back for previous return */
    for (start_pc -= instlen; ; start_pc -= instlen)
	if (start_pc < fence)
	  {
	    /* It's not clear to me why we reach this point when
	       stop_soon_quietly, but with this test, at least we
	       don't print out warnings for every child forked (eg, on
	       decstation).  22apr93 rich@cygnus.com.  */
	    if (!stop_soon_quietly)
	      {
		static int blurb_printed = 0;

		if (fence == VM_MIN_ADDRESS)
		  warning("Hit beginning of text section without finding");
		else
		  warning("Hit heuristic-fence-post without finding");
		
		warning("enclosing function for address 0x%s", paddr_nz (pc));
		if (!blurb_printed)
		  {
		    printf_filtered ("\
This warning occurs if you are debugging a function without any symbols\n\
(for example, in a stripped executable).  In that case, you may wish to\n\
increase the size of the search with the `set heuristic-fence-post' command.\n\
\n\
Otherwise, you told GDB there was a function where there isn't one, or\n\
(more likely) you have encountered a bug in GDB.\n");
		    blurb_printed = 1;
		  }
	      }

	    return 0; 
	  }
	else if (pc_is_mips16 (start_pc))
	  {
	    unsigned short inst;

	    /* On MIPS16, any one of the following is likely to be the
	       start of a function:
		 entry
		 addiu sp,-n
		 daddiu sp,-n
		 extend -n followed by 'addiu sp,+n' or 'daddiu sp,+n'  */
	    inst = mips_fetch_instruction (start_pc);
	    if (((inst & 0xf81f) == 0xe809 && (inst & 0x700) != 0x700) /* entry */
		|| (inst & 0xff80) == 0x6380	/* addiu sp,-n */
		|| (inst & 0xff80) == 0xfb80	/* daddiu sp,-n */
		|| ((inst & 0xf810) == 0xf010 && seen_adjsp))	/* extend -n */
	      break;
	    else if ((inst & 0xff00) == 0x6300		/* addiu sp */
		     || (inst & 0xff00) == 0xfb00)	/* daddiu sp */
	      seen_adjsp = 1;
	    else
	      seen_adjsp = 0;
	  }
	else if (ABOUT_TO_RETURN(start_pc))
	  {
	    start_pc += 2 * MIPS_INSTLEN; /* skip return, and its delay slot */
	    break;
	  }

#if 0
    /* skip nops (usually 1) 0 - is this */
    while (start_pc < pc && read_memory_integer (start_pc, MIPS_INSTLEN) == 0)
	start_pc += MIPS_INSTLEN;
#endif
    return start_pc;
}

/* Fetch the immediate value from a MIPS16 instruction.
   If the previous instruction was an EXTEND, use it to extend
   the upper bits of the immediate value.  This is a helper function
   for mips16_heuristic_proc_desc.  */

static int
mips16_get_imm (prev_inst, inst, nbits, scale, is_signed)
    unsigned short prev_inst;	/* previous instruction */
    unsigned short inst;	/* current instruction */
    int nbits;			/* number of bits in imm field */
    int scale;			/* scale factor to be applied to imm */
    int is_signed;		/* is the imm field signed? */
{
  int offset;

  if ((prev_inst & 0xf800) == 0xf000)	/* prev instruction was EXTEND? */
    {
      offset = ((prev_inst & 0x1f) << 11) | (prev_inst & 0x7e0);
      if (offset & 0x8000)		/* check for negative extend */
	offset = 0 - (0x10000 - (offset & 0xffff));
      return offset | (inst & 0x1f);
    }
  else
    {
      int max_imm = 1 << nbits;
      int mask = max_imm - 1;
      int sign_bit = max_imm >> 1;

      offset = inst & mask;
      if (is_signed && (offset & sign_bit))
	offset = 0 - (max_imm - offset);
      return offset * scale;
    }
}


/* Fill in values in temp_proc_desc based on the MIPS16 instruction
   stream from start_pc to limit_pc.  */

static void
mips16_heuristic_proc_desc(start_pc, limit_pc, next_frame, sp)
    CORE_ADDR start_pc, limit_pc;
    struct frame_info *next_frame;
    CORE_ADDR sp;
{
  CORE_ADDR cur_pc;
  CORE_ADDR frame_addr = 0;	/* Value of $r17, used as frame pointer */
  unsigned short prev_inst = 0;	/* saved copy of previous instruction */
  unsigned inst = 0;		/* current instruction */
  unsigned entry_inst = 0;	/* the entry instruction */
  int reg, offset;

  PROC_FRAME_OFFSET(&temp_proc_desc) = 0;	/* size of stack frame */
  PROC_FRAME_ADJUST(&temp_proc_desc) = 0;	/* offset of FP from SP */

  for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += MIPS16_INSTLEN)
    {
      /* Save the previous instruction.  If it's an EXTEND, we'll extract
         the immediate offset extension from it in mips16_get_imm.  */
      prev_inst = inst;

      /* Fetch and decode the instruction.   */
      inst = (unsigned short) mips_fetch_instruction (cur_pc);
      if ((inst & 0xff00) == 0x6300		/* addiu sp */
	  || (inst & 0xff00) == 0xfb00)		/* daddiu sp */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 8, 1);
	  if (offset < 0)			/* negative stack adjustment? */
	    PROC_FRAME_OFFSET(&temp_proc_desc) -= offset;
	  else
	    /* Exit loop if a positive stack adjustment is found, which
	       usually means that the stack cleanup code in the function
	       epilogue is reached.  */
	    break;
	}
      else if ((inst & 0xf800) == 0xd000)	/* sw reg,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  reg = mips16_to_32_reg[(inst & 0x700) >> 8];
	  PROC_REG_MASK(&temp_proc_desc) |= (1 << reg);
	  set_reg_offset (reg, sp + offset);
	}
      else if ((inst & 0xff00) == 0xf900)	/* sd reg,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 8, 0);
	  reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
	  PROC_REG_MASK(&temp_proc_desc) |= (1 << reg);
	  set_reg_offset (reg, sp + offset);
	}
      else if ((inst & 0xff00) == 0x6200)	/* sw $ra,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  PROC_REG_MASK(&temp_proc_desc) |= (1 << RA_REGNUM);
	  set_reg_offset (RA_REGNUM, sp + offset);
	}
      else if ((inst & 0xff00) == 0xfa00)	/* sd $ra,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 8, 0);
	  PROC_REG_MASK(&temp_proc_desc) |= (1 << RA_REGNUM);
	  set_reg_offset (RA_REGNUM, sp + offset);
	}
      else if (inst == 0x673d)			/* move $s1, $sp */
	{
	  frame_addr = sp;
	  PROC_FRAME_REG (&temp_proc_desc) = 17;
	}
      else if ((inst & 0xff00) == 0x0100)	/* addiu $s1,sp,n */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  frame_addr = sp + offset;
	  PROC_FRAME_REG (&temp_proc_desc) = 17;
	  PROC_FRAME_ADJUST (&temp_proc_desc) = offset;
	}
      else if ((inst & 0xFF00) == 0xd900)	/* sw reg,offset($s1) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 4, 0);
	  reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
	  PROC_REG_MASK(&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, frame_addr + offset);
	}
      else if ((inst & 0xFF00) == 0x7900)	/* sd reg,offset($s1) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 8, 0);
	  reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
	  PROC_REG_MASK(&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, frame_addr + offset);
	}
      else if ((inst & 0xf81f) == 0xe809 && (inst & 0x700) != 0x700) /* entry */
	entry_inst = inst;			/* save for later processing */
      else if ((inst & 0xf800) == 0x1800)	/* jal(x) */
	cur_pc += MIPS16_INSTLEN;		/* 32-bit instruction */
    }

    /* The entry instruction is typically the first instruction in a function,
       and it stores registers at offsets relative to the value of the old SP
       (before the prologue).  But the value of the sp parameter to this
       function is the new SP (after the prologue has been executed).  So we
       can't calculate those offsets until we've seen the entire prologue,
       and can calculate what the old SP must have been. */
    if (entry_inst != 0)
      {
	int areg_count = (entry_inst >> 8) & 7;
	int sreg_count = (entry_inst >> 6) & 3;

	/* The entry instruction always subtracts 32 from the SP.  */
	PROC_FRAME_OFFSET(&temp_proc_desc) += 32;

	/* Now we can calculate what the SP must have been at the
	   start of the function prologue.  */
	sp += PROC_FRAME_OFFSET(&temp_proc_desc);

	/* Check if a0-a3 were saved in the caller's argument save area.  */
	for (reg = 4, offset = 0; reg < areg_count+4; reg++)
	  {
	    PROC_REG_MASK(&temp_proc_desc) |= 1 << reg;
	    set_reg_offset (reg, sp + offset);
	    offset += MIPS_REGSIZE;
	  }

	/* Check if the ra register was pushed on the stack.  */
	offset = -4;
	if (entry_inst & 0x20)
	  {
	    PROC_REG_MASK(&temp_proc_desc) |= 1 << RA_REGNUM;
	    set_reg_offset (RA_REGNUM, sp + offset);
	    offset -= MIPS_REGSIZE;
	  }

	/* Check if the s0 and s1 registers were pushed on the stack.  */
	for (reg = 16; reg < sreg_count+16; reg++)
	  {
	    PROC_REG_MASK(&temp_proc_desc) |= 1 << reg;
	    set_reg_offset (reg, sp + offset);
	    offset -= MIPS_REGSIZE;
	  }
      }
}

static void
mips32_heuristic_proc_desc(start_pc, limit_pc, next_frame, sp)
    CORE_ADDR start_pc, limit_pc;
    struct frame_info *next_frame;
    CORE_ADDR sp;
{
  CORE_ADDR cur_pc;
  CORE_ADDR frame_addr = 0; /* Value of $r30. Used by gcc for frame-pointer */
restart:
  PROC_FRAME_OFFSET(&temp_proc_desc) = 0;
  PROC_FRAME_ADJUST (&temp_proc_desc) = 0;	/* offset of FP from SP */
  for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += MIPS_INSTLEN)
    {
      unsigned long inst, high_word, low_word;
      int reg;

      /* Fetch the instruction.   */
      inst = (unsigned long) mips_fetch_instruction (cur_pc);

      /* Save some code by pre-extracting some useful fields.  */
      high_word = (inst >> 16) & 0xffff;
      low_word = inst & 0xffff;
      reg = high_word & 0x1f;

      if (high_word == 0x27bd		/* addiu $sp,$sp,-i */
	  || high_word == 0x23bd	/* addi $sp,$sp,-i */
	  || high_word == 0x67bd)	/* daddiu $sp,$sp,-i */
	{
	  if (low_word & 0x8000)	/* negative stack adjustment? */
	    PROC_FRAME_OFFSET(&temp_proc_desc) += 0x10000 - low_word;
	  else
	    /* Exit loop if a positive stack adjustment is found, which
	       usually means that the stack cleanup code in the function
	       epilogue is reached.  */
	    break;
	}
      else if ((high_word & 0xFFE0) == 0xafa0)	/* sw reg,offset($sp) */
	{
	  PROC_REG_MASK(&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, sp + low_word);
	}
      else if ((high_word & 0xFFE0) == 0xffa0)	/* sd reg,offset($sp) */
	{
	  /* Irix 6.2 N32 ABI uses sd instructions for saving $gp and $ra,
	     but the register size used is only 32 bits. Make the address
	     for the saved register point to the lower 32 bits.  */
	  PROC_REG_MASK(&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, sp + low_word + 8 - MIPS_REGSIZE);
	}
      else if (high_word == 0x27be)			/* addiu $30,$sp,size */
	{
	  /* Old gcc frame, r30 is virtual frame pointer.  */
	  if ((long)low_word != PROC_FRAME_OFFSET(&temp_proc_desc))
	      frame_addr = sp + low_word;
	  else if (PROC_FRAME_REG (&temp_proc_desc) == SP_REGNUM)
	    {
	      unsigned alloca_adjust;
	      PROC_FRAME_REG (&temp_proc_desc) = 30;
	      frame_addr = read_next_frame_reg(next_frame, 30);
	      alloca_adjust = (unsigned)(frame_addr - (sp + low_word));
	      if (alloca_adjust > 0)
		{
		  /* FP > SP + frame_size. This may be because
		   * of an alloca or somethings similar.
		   * Fix sp to "pre-alloca" value, and try again.
		   */
		  sp += alloca_adjust;
		  goto restart;
		}
	    }
	}
     /* move $30,$sp.  With different versions of gas this will be either
       `addu $30,$sp,$zero' or `or $30,$sp,$zero' or `daddu 30,sp,$0'.
	Accept any one of these.  */
      else if (inst == 0x03A0F021 || inst == 0x03a0f025 || inst == 0x03a0f02d)
	{
	  /* New gcc frame, virtual frame pointer is at r30 + frame_size.  */
	  if (PROC_FRAME_REG (&temp_proc_desc) == SP_REGNUM)
	    {
	      unsigned alloca_adjust;
	      PROC_FRAME_REG (&temp_proc_desc) = 30;
	      frame_addr = read_next_frame_reg(next_frame, 30);
	      alloca_adjust = (unsigned)(frame_addr - sp);
	      if (alloca_adjust > 0)
		{
		  /* FP > SP + frame_size. This may be because
		   * of an alloca or somethings similar.
		   * Fix sp to "pre-alloca" value, and try again.
		   */
		  sp += alloca_adjust;
		  goto restart;
		}
	    }
	}
      else if ((high_word & 0xFFE0) == 0xafc0)		/* sw reg,offset($30) */
	{
	  PROC_REG_MASK(&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, frame_addr + low_word);
	}
    }
}

static mips_extra_func_info_t
heuristic_proc_desc(start_pc, limit_pc, next_frame)
    CORE_ADDR start_pc, limit_pc;
    struct frame_info *next_frame;
{
  CORE_ADDR sp = read_next_frame_reg (next_frame, SP_REGNUM);

  if (start_pc == 0) return NULL;
  memset (&temp_proc_desc, '\0', sizeof(temp_proc_desc));
  memset (&temp_saved_regs, '\0', sizeof(struct frame_saved_regs));
  PROC_LOW_ADDR (&temp_proc_desc) = start_pc;
  PROC_FRAME_REG (&temp_proc_desc) = SP_REGNUM;
  PROC_PC_REG (&temp_proc_desc) = RA_REGNUM;

  if (start_pc + 200 < limit_pc)
    limit_pc = start_pc + 200;
  if (pc_is_mips16 (start_pc))
    mips16_heuristic_proc_desc (start_pc, limit_pc, next_frame, sp);
  else
    mips32_heuristic_proc_desc (start_pc, limit_pc, next_frame, sp);
  return &temp_proc_desc;
}

static mips_extra_func_info_t
non_heuristic_proc_desc (pc, addrptr)
     CORE_ADDR pc;
     CORE_ADDR *addrptr;
{
  CORE_ADDR startaddr;
  mips_extra_func_info_t proc_desc;
  struct block *b = block_for_pc(pc);
  struct symbol *sym;

  find_pc_partial_function (pc, NULL, &startaddr, NULL);
  if (addrptr)
    *addrptr = startaddr;
  if (b == NULL || PC_IN_CALL_DUMMY (pc, 0, 0))
    sym = NULL;
  else
    {
      if (startaddr > BLOCK_START (b))
	/* This is the "pathological" case referred to in a comment in
	   print_frame_info.  It might be better to move this check into
	   symbol reading.  */
	sym = NULL;
      else
	sym = lookup_symbol (MIPS_EFI_SYMBOL_NAME, b, LABEL_NAMESPACE, 0, NULL);
    }

  /* If we never found a PDR for this function in symbol reading, then
     examine prologues to find the information.  */
  if (sym)
    {
      proc_desc = (mips_extra_func_info_t) SYMBOL_VALUE (sym);
      if (PROC_FRAME_REG (proc_desc) == -1)
	return NULL;
      else
	return proc_desc;
    }
  else
    return NULL;
}


static mips_extra_func_info_t
find_proc_desc (pc, next_frame)
     CORE_ADDR pc;
     struct frame_info *next_frame;
{
  mips_extra_func_info_t proc_desc;
  CORE_ADDR startaddr;

  proc_desc = non_heuristic_proc_desc (pc, &startaddr);

  if (proc_desc)
    {
      /* IF this is the topmost frame AND
       * (this proc does not have debugging information OR
       * the PC is in the procedure prologue)
       * THEN create a "heuristic" proc_desc (by analyzing
       * the actual code) to replace the "official" proc_desc.
       */
      if (next_frame == NULL)
	{
	  struct symtab_and_line val;
	  struct symbol *proc_symbol =
	      PROC_DESC_IS_DUMMY(proc_desc) ? 0 : PROC_SYMBOL(proc_desc);

	  if (proc_symbol)
	    {
	      val = find_pc_line (BLOCK_START
				  (SYMBOL_BLOCK_VALUE(proc_symbol)),
				  0);
	      val.pc = val.end ? val.end : pc;
	    }
	  if (!proc_symbol || pc < val.pc)
	    {
	      mips_extra_func_info_t found_heuristic =
		heuristic_proc_desc (PROC_LOW_ADDR (proc_desc),
				     pc, next_frame);
	      if (found_heuristic)
		proc_desc = found_heuristic;
	    }
	}
    }
  else
    {
      /* Is linked_proc_desc_table really necessary?  It only seems to be used
	 by procedure call dummys.  However, the procedures being called ought
	 to have their own proc_descs, and even if they don't,
	 heuristic_proc_desc knows how to create them! */

      register struct linked_proc_info *link;

      for (link = linked_proc_desc_table; link; link = link->next)
	if (PROC_LOW_ADDR(&link->info) <= pc
	    && PROC_HIGH_ADDR(&link->info) > pc)
	  return &link->info;

      if (startaddr == 0)
	startaddr = heuristic_proc_start (pc);

      proc_desc =
	heuristic_proc_desc (startaddr, pc, next_frame);
    }
  return proc_desc;
}

static CORE_ADDR
get_frame_pointer(frame, proc_desc)
    struct frame_info *frame;
    mips_extra_func_info_t proc_desc;
{
  return ADDR_BITS_REMOVE (
    read_next_frame_reg (frame, PROC_FRAME_REG (proc_desc)) +
      PROC_FRAME_OFFSET (proc_desc) - PROC_FRAME_ADJUST (proc_desc));
}

mips_extra_func_info_t cached_proc_desc;

CORE_ADDR
mips_frame_chain(frame)
    struct frame_info *frame;
{
  mips_extra_func_info_t proc_desc;
  CORE_ADDR tmp;
  CORE_ADDR saved_pc = FRAME_SAVED_PC(frame);

  if (saved_pc == 0 || inside_entry_file (saved_pc))
    return 0;

  /* Check if the PC is inside a call stub.  If it is, fetch the
     PC of the caller of that stub.  */
  if ((tmp = mips_skip_stub (saved_pc)) != 0)
    saved_pc = tmp;

  /* Look up the procedure descriptor for this PC.  */
  proc_desc = find_proc_desc(saved_pc, frame);
  if (!proc_desc)
    return 0;

  cached_proc_desc = proc_desc;

  /* If no frame pointer and frame size is zero, we must be at end
     of stack (or otherwise hosed).  If we don't check frame size,
     we loop forever if we see a zero size frame.  */
  if (PROC_FRAME_REG (proc_desc) == SP_REGNUM
      && PROC_FRAME_OFFSET (proc_desc) == 0
      /* The previous frame from a sigtramp frame might be frameless
	 and have frame size zero.  */
      && !frame->signal_handler_caller)
    return 0;
  else
    return get_frame_pointer (frame, proc_desc);
}

void
init_extra_frame_info(fci)
     struct frame_info *fci;
{
  int regnum;

  /* Use proc_desc calculated in frame_chain */
  mips_extra_func_info_t proc_desc =
    fci->next ? cached_proc_desc : find_proc_desc(fci->pc, fci->next);

  fci->saved_regs = NULL;
  fci->proc_desc =
    proc_desc == &temp_proc_desc ? 0 : proc_desc;
  if (proc_desc)
    {
      /* Fixup frame-pointer - only needed for top frame */
      /* This may not be quite right, if proc has a real frame register.
	 Get the value of the frame relative sp, procedure might have been
	 interrupted by a signal at it's very start.  */
      if (fci->pc == PROC_LOW_ADDR (proc_desc)
	  && !PROC_DESC_IS_DUMMY (proc_desc))
	fci->frame = read_next_frame_reg (fci->next, SP_REGNUM);
      else
	fci->frame = get_frame_pointer (fci->next, proc_desc);

      if (proc_desc == &temp_proc_desc)
	{
	  char *name;

	  /* Do not set the saved registers for a sigtramp frame,
	     mips_find_saved_registers will do that for us.
	     We can't use fci->signal_handler_caller, it is not yet set.  */
	  find_pc_partial_function (fci->pc, &name,
				    (CORE_ADDR *)NULL,(CORE_ADDR *)NULL);
	  if (!IN_SIGTRAMP (fci->pc, name))
	    {
	      fci->saved_regs = (struct frame_saved_regs*)
		obstack_alloc (&frame_cache_obstack,
			       sizeof (struct frame_saved_regs));
	      *fci->saved_regs = temp_saved_regs;
	      fci->saved_regs->regs[PC_REGNUM]
		= fci->saved_regs->regs[RA_REGNUM];
	    }
	}

      /* hack: if argument regs are saved, guess these contain args */
      fci->num_args = -1;	/* assume we can't tell how many args for now */
      for (regnum = MIPS_LAST_ARG_REGNUM; regnum >= A0_REGNUM; regnum--)
	{
	  if (PROC_REG_MASK(proc_desc) & (1 << regnum))
	    {
	      fci->num_args = regnum - A0_REGNUM + 1;
	      break;
	    }
	} 
    }
}

/* MIPS stack frames are almost impenetrable.  When execution stops,
   we basically have to look at symbol information for the function
   that we stopped in, which tells us *which* register (if any) is
   the base of the frame pointer, and what offset from that register
   the frame itself is at.  

   This presents a problem when trying to examine a stack in memory
   (that isn't executing at the moment), using the "frame" command.  We
   don't have a PC, nor do we have any registers except SP.

   This routine takes two arguments, SP and PC, and tries to make the
   cached frames look as if these two arguments defined a frame on the
   cache.  This allows the rest of info frame to extract the important
   arguments without difficulty.  */

struct frame_info *
setup_arbitrary_frame (argc, argv)
     int argc;
     CORE_ADDR *argv;
{
  if (argc != 2)
    error ("MIPS frame specifications require two arguments: sp and pc");

  return create_new_frame (argv[0], argv[1]);
}

CORE_ADDR
mips_push_arguments(nargs, args, sp, struct_return, struct_addr)
     int nargs;
     value_ptr *args;
     CORE_ADDR sp;
     int struct_return;
     CORE_ADDR struct_addr;
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;

  /* Macros to round N up or down to the next A boundary; A must be
     a power of two. */
#define ROUND_DOWN(n,a) ((n) & ~((a)-1))
#define ROUND_UP(n,a) (((n)+(a)-1) & ~((a)-1))
  
  /* First ensure that the stack and structure return address (if any)
     are properly aligned. The stack has to be 64-bit aligned even
     on 32-bit machines, because doubles must be 64-bit aligned. */
  sp = ROUND_DOWN (sp, 8);
  struct_addr = ROUND_DOWN (struct_addr, MIPS_REGSIZE);
      
  /* Now make space on the stack for the args. We allocate more
     than necessary for EABI, because the first few arguments are
     passed in registers, but that's OK. */
  for (argnum = 0; argnum < nargs; argnum++)
    len += ROUND_UP (TYPE_LENGTH(VALUE_TYPE(args[argnum])), MIPS_REGSIZE);
  sp -= ROUND_UP (len, 8);

  /* Initialize the integer and float register pointers.  */
  argreg = A0_REGNUM;
  float_argreg = FPA0_REGNUM;

  /* the struct_return pointer occupies the first parameter-passing reg */
  if (struct_return)
      write_register (argreg++, struct_addr);

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      char *val;
      char valbuf[REGISTER_RAW_SIZE(A0_REGNUM)];
      value_ptr arg = args[argnum];
      struct type *arg_type = check_typedef (VALUE_TYPE (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      /* The EABI passes structures that do not fit in a register by
	 reference. In all other cases, pass the structure by value.  */
      if (MIPS_EABI && len > MIPS_REGSIZE &&
	  (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION))
	{
	  store_address (valbuf, MIPS_REGSIZE, VALUE_ADDRESS (arg));
	  typecode = TYPE_CODE_PTR;
	  len = MIPS_REGSIZE;
	  val = valbuf;
	}
      else
	val = (char *)VALUE_CONTENTS (arg);

      /* 32-bit ABIs always start floating point arguments in an
         even-numbered floating point register.   */
      if (!GDB_TARGET_IS_MIPS64 && typecode == TYPE_CODE_FLT
          && (float_argreg & 1))
	float_argreg++;

      /* Floating point arguments passed in registers have to be
         treated specially.  On 32-bit architectures, doubles
	 are passed in register pairs; the even register gets
	 the low word, and the odd register gets the high word.
	 On non-EABI processors, the first two floating point arguments are
	 also copied to general registers, because MIPS16 functions
	 don't use float registers for arguments.  This duplication of
	 arguments in general registers can't hurt non-MIPS16 functions
	 because those registers are normally skipped.  */
      if (typecode == TYPE_CODE_FLT
	  && float_argreg <= MIPS_LAST_FP_ARG_REGNUM
	  && mips_fpu != MIPS_FPU_NONE)
	{
	  if (!GDB_TARGET_IS_MIPS64 && len == 8)
	    {
	      int low_offset = TARGET_BYTE_ORDER == BIG_ENDIAN ? 4 : 0;
	      unsigned long regval;

	      /* Write the low word of the double to the even register(s).  */
	      regval = extract_unsigned_integer (val+low_offset, 4);
	      write_register (float_argreg++, regval);
	      if (!MIPS_EABI)
		write_register (argreg+1, regval);

	      /* Write the high word of the double to the odd register(s).  */
	      regval = extract_unsigned_integer (val+4-low_offset, 4);
	      write_register (float_argreg++, regval);
	      if (!MIPS_EABI)
	        {
		  write_register (argreg, regval);
		  argreg += 2;
		}

	    }
	  else
	    {
	      /* This is a floating point value that fits entirely
	         in a single register.  */
	      CORE_ADDR regval = extract_address (val, len);
	      write_register (float_argreg++, regval);
	      if (!MIPS_EABI)
	        {
		  write_register (argreg, regval);
		  argreg += GDB_TARGET_IS_MIPS64 ? 1 : 2;
		}
	    }
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* Note: structs whose size is not a multiple of MIPS_REGSIZE
	     are treated specially: Irix cc passes them in registers
	     where gcc sometimes puts them on the stack.  For maximum
	     compatibility, we will put them in both places.  */

	  int odd_sized_struct = ((len > MIPS_REGSIZE) && 
				  (len % MIPS_REGSIZE != 0));
	  while (len > 0)
	    {
	      int partial_len = len < MIPS_REGSIZE ? len : MIPS_REGSIZE;

	      if (argreg > MIPS_LAST_ARG_REGNUM || odd_sized_struct)
		{
		  /* Write this portion of the argument to the stack.  */
		  /* Should shorter than int integer values be
		     promoted to int before being stored? */

		  int longword_offset = 0;
		  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
		    if (MIPS_REGSIZE == 8 &&
			(typecode == TYPE_CODE_INT ||
			 typecode == TYPE_CODE_PTR ||
			 typecode == TYPE_CODE_FLT) && len <= 4)
		      longword_offset = MIPS_REGSIZE - len;
		    else if ((typecode == TYPE_CODE_STRUCT ||
			      typecode == TYPE_CODE_UNION) &&
			     TYPE_LENGTH (arg_type) < MIPS_REGSIZE)
		      longword_offset = MIPS_REGSIZE - len;

		  write_memory (sp + stack_offset + longword_offset, 
				val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.
		 Odd sized structs may go thru BOTH paths.  */
	      if (argreg <= MIPS_LAST_ARG_REGNUM)
		{
		  CORE_ADDR regval = extract_address (val, partial_len);

		  /* A non-floating-point argument being passed in a 
		     general register.  If a struct or union, and if
		     the remaining length is smaller than the register
		     size, we have to adjust the register value on
		     big endian targets.

		     It does not seem to be necessary to do the
		     same for integral types.

		     Also don't do this adjustment on EABI targets.  */

		  if (!MIPS_EABI &&
		      TARGET_BYTE_ORDER == BIG_ENDIAN &&
		      partial_len < MIPS_REGSIZE &&
		      (typecode == TYPE_CODE_STRUCT ||
		       typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS_REGSIZE - partial_len) * 
				TARGET_CHAR_BIT);

		  write_register (argreg, regval);
		  argreg++;
    
		  /* If this is the old ABI, prevent subsequent floating
		     point arguments from being passed in floating point
		     registers.  */
		  if (!MIPS_EABI)
		    float_argreg = MIPS_LAST_FP_ARG_REGNUM + 1;
		}
    
	      len -= partial_len;
	      val += partial_len;

	      /* The offset onto the stack at which we will start
		 copying parameters (after the registers are used up) 
		 begins at (4 * MIPS_REGSIZE) in the old ABI.  This 
		 leaves room for the "home" area for register parameters.

		 In the new EABI, the 8 register parameters do not 
		 have "home" stack space reserved for them, so the
		 stack offset does not get incremented until after
		 we have used up the 8 parameter registers.  */
	      if (!(MIPS_EABI && argnum < 8))
		stack_offset += ROUND_UP (partial_len, MIPS_REGSIZE);
	    }
	}
    }

  /* Set the return address register to point to the entry
     point of the program, where a breakpoint lies in wait.  */
  write_register (RA_REGNUM, CALL_DUMMY_ADDRESS());

  /* Return adjusted stack pointer.  */
  return sp;
}

static void
mips_push_register(CORE_ADDR *sp, int regno)
{
  char buffer[MAX_REGISTER_RAW_SIZE];
  int regsize = REGISTER_RAW_SIZE (regno);

  *sp -= regsize;
  read_register_gen (regno, buffer);
  write_memory (*sp, buffer, regsize);
}

/* MASK(i,j) == (1<<i) + (1<<(i+1)) + ... + (1<<j)). Assume i<=j<(MIPS_NUMREGS-1). */
#define MASK(i,j) (((1 << ((j)+1))-1) ^ ((1 << (i))-1))

void
mips_push_dummy_frame()
{
  int ireg;
  struct linked_proc_info *link = (struct linked_proc_info*)
      xmalloc(sizeof(struct linked_proc_info));
  mips_extra_func_info_t proc_desc = &link->info;
  CORE_ADDR sp = ADDR_BITS_REMOVE (read_register (SP_REGNUM));
  CORE_ADDR old_sp = sp;
  link->next = linked_proc_desc_table;
  linked_proc_desc_table = link;

/* FIXME!   are these correct ? */
#define PUSH_FP_REGNUM 16 /* must be a register preserved across calls */
#define GEN_REG_SAVE_MASK MASK(1,16)|MASK(24,28)|(1<<(MIPS_NUMREGS-1))
#define FLOAT_REG_SAVE_MASK MASK(0,19)
#define FLOAT_SINGLE_REG_SAVE_MASK \
  ((1<<18)|(1<<16)|(1<<14)|(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0))
  /*
   * The registers we must save are all those not preserved across
   * procedure calls. Dest_Reg (see tm-mips.h) must also be saved.
   * In addition, we must save the PC, PUSH_FP_REGNUM, MMLO/-HI
   * and FP Control/Status registers.
   * 
   *
   * Dummy frame layout:
   *  (high memory)
   * 	Saved PC
   *	Saved MMHI, MMLO, FPC_CSR
   *	Saved R31
   *	Saved R28
   *	...
   *	Saved R1
   *    Saved D18 (i.e. F19, F18)
   *    ...
   *    Saved D0 (i.e. F1, F0)
   *	Argument build area and stack arguments written via mips_push_arguments
   *  (low memory)
   */

  /* Save special registers (PC, MMHI, MMLO, FPC_CSR) */
  PROC_FRAME_REG(proc_desc) = PUSH_FP_REGNUM;
  PROC_FRAME_OFFSET(proc_desc) = 0;
  PROC_FRAME_ADJUST(proc_desc) = 0;
  mips_push_register (&sp, PC_REGNUM);
  mips_push_register (&sp, HI_REGNUM);
  mips_push_register (&sp, LO_REGNUM);
  mips_push_register (&sp, mips_fpu == MIPS_FPU_NONE ? 0 : FCRCS_REGNUM);

  /* Save general CPU registers */
  PROC_REG_MASK(proc_desc) = GEN_REG_SAVE_MASK;
  PROC_REG_OFFSET(proc_desc) = sp - old_sp; /* offset of (Saved R31) from FP */
  for (ireg = 32; --ireg >= 0; )
    if (PROC_REG_MASK(proc_desc) & (1 << ireg))
      mips_push_register (&sp, ireg);

  /* Save floating point registers starting with high order word */
  PROC_FREG_MASK(proc_desc) = 
    mips_fpu == MIPS_FPU_DOUBLE ? FLOAT_REG_SAVE_MASK
    : mips_fpu == MIPS_FPU_SINGLE ? FLOAT_SINGLE_REG_SAVE_MASK : 0;
  PROC_FREG_OFFSET(proc_desc) = sp - old_sp; /* offset of (Saved D18) from FP */
  for (ireg = 32; --ireg >= 0; )
    if (PROC_FREG_MASK(proc_desc) & (1 << ireg))
      mips_push_register (&sp, ireg + FP0_REGNUM);

  /* Update the frame pointer for the call dummy and the stack pointer.
     Set the procedure's starting and ending addresses to point to the
     call dummy address at the entry point.  */
  write_register (PUSH_FP_REGNUM, old_sp);
  write_register (SP_REGNUM, sp);
  PROC_LOW_ADDR(proc_desc) = CALL_DUMMY_ADDRESS();
  PROC_HIGH_ADDR(proc_desc) =  CALL_DUMMY_ADDRESS() + 4;
  SET_PROC_DESC_IS_DUMMY(proc_desc);
  PROC_PC_REG(proc_desc) = RA_REGNUM;
}

void
mips_pop_frame()
{
  register int regnum;
  struct frame_info *frame = get_current_frame ();
  CORE_ADDR new_sp = FRAME_FP (frame);

  mips_extra_func_info_t proc_desc = frame->proc_desc;

  write_register (PC_REGNUM, FRAME_SAVED_PC(frame));
  if (frame->saved_regs == NULL)
    mips_find_saved_regs (frame);
  for (regnum = 0; regnum < NUM_REGS; regnum++)
    {
      if (regnum != SP_REGNUM && regnum != PC_REGNUM
	  && frame->saved_regs->regs[regnum])
	write_register (regnum,
			read_memory_integer (frame->saved_regs->regs[regnum],
					     MIPS_REGSIZE)); 
    }
  write_register (SP_REGNUM, new_sp);
  flush_cached_frames ();

  if (proc_desc && PROC_DESC_IS_DUMMY(proc_desc))
    {
      struct linked_proc_info *pi_ptr, *prev_ptr;

      for (pi_ptr = linked_proc_desc_table, prev_ptr = NULL;
	   pi_ptr != NULL;
	   prev_ptr = pi_ptr, pi_ptr = pi_ptr->next)
	{
	  if (&pi_ptr->info == proc_desc)
	    break;
	}

      if (pi_ptr == NULL)
	error ("Can't locate dummy extra frame info\n");

      if (prev_ptr != NULL)
	prev_ptr->next = pi_ptr->next;
      else
	linked_proc_desc_table = pi_ptr->next;

      free (pi_ptr);

      write_register (HI_REGNUM,
	        read_memory_integer (new_sp - 2*MIPS_REGSIZE, MIPS_REGSIZE));
      write_register (LO_REGNUM,
	        read_memory_integer (new_sp - 3*MIPS_REGSIZE, MIPS_REGSIZE));
      if (mips_fpu != MIPS_FPU_NONE)
	write_register (FCRCS_REGNUM,
	        read_memory_integer (new_sp - 4*MIPS_REGSIZE, MIPS_REGSIZE));
    }
}

static void
mips_print_register (regnum, all)
     int regnum, all;
{
  char raw_buffer[MAX_REGISTER_RAW_SIZE];

  /* Get the data in raw format.  */
  if (read_relative_register_raw_bytes (regnum, raw_buffer))
    {
      printf_filtered ("%s: [Invalid]", reg_names[regnum]);
      return;
    }

  /* If an even floating point register, also print as double. */
  if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT
      && !((regnum-FP0_REGNUM) & 1))
    if (REGISTER_RAW_SIZE(regnum) == 4)	/* this would be silly on MIPS64 */
      {
	char dbuffer[2 * MAX_REGISTER_RAW_SIZE]; 

	read_relative_register_raw_bytes (regnum, dbuffer);
	read_relative_register_raw_bytes (regnum+1, dbuffer+MIPS_REGSIZE);
	REGISTER_CONVERT_TO_TYPE (regnum, builtin_type_double, dbuffer);

	printf_filtered ("(d%d: ", regnum-FP0_REGNUM);
	val_print (builtin_type_double, dbuffer, 0,
		   gdb_stdout, 0, 1, 0, Val_pretty_default);
	printf_filtered ("); ");
      }
  fputs_filtered (reg_names[regnum], gdb_stdout);

  /* The problem with printing numeric register names (r26, etc.) is that
     the user can't use them on input.  Probably the best solution is to
     fix it so that either the numeric or the funky (a2, etc.) names
     are accepted on input.  */
  if (regnum < MIPS_NUMREGS)
    printf_filtered ("(r%d): ", regnum);
  else
    printf_filtered (": ");

  /* If virtual format is floating, print it that way.  */
  if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
    if (REGISTER_RAW_SIZE(regnum) == 8)
      { /* show 8-byte floats as float AND double: */
	int offset = 4 * (TARGET_BYTE_ORDER == BIG_ENDIAN);

	printf_filtered (" (float) ");
	val_print (builtin_type_float, raw_buffer + offset, 0,
		   gdb_stdout, 0, 1, 0, Val_pretty_default);
	printf_filtered (", (double) ");
	val_print (builtin_type_double, raw_buffer, 0,
		   gdb_stdout, 0, 1, 0, Val_pretty_default);
      }
    else
      val_print (REGISTER_VIRTUAL_TYPE (regnum), raw_buffer, 0,
		 gdb_stdout, 0, 1, 0, Val_pretty_default);
  /* Else print as integer in hex.  */
  else
    print_scalar_formatted (raw_buffer, REGISTER_VIRTUAL_TYPE (regnum),
			    'x', 0, gdb_stdout);
}

/* Replacement for generic do_registers_info.  
   Print regs in pretty columns.  */

static int
do_fp_register_row (regnum)
     int regnum;
{ /* do values for FP (float) regs */
  char raw_buffer[2] [REGISTER_RAW_SIZE(FP0_REGNUM)];
  char dbl_buffer[2 * REGISTER_RAW_SIZE(FP0_REGNUM)];
  /* use HI and LO to control the order of combining two flt regs */
  int HI = (TARGET_BYTE_ORDER == BIG_ENDIAN);
  int LO = (TARGET_BYTE_ORDER != BIG_ENDIAN);
  double doub, flt1, flt2;	/* doubles extracted from raw hex data */
  int inv1, inv2, inv3;
   
  /* Get the data in raw format.  */
  if (read_relative_register_raw_bytes (regnum, raw_buffer[HI]))
    error ("can't read register %d (%s)", regnum, reg_names[regnum]);
  if (REGISTER_RAW_SIZE(regnum) == 4)
    {
      /* 4-byte registers: we can fit two registers per row. */
      /* Also print every pair of 4-byte regs as an 8-byte double. */
      if (read_relative_register_raw_bytes (regnum + 1, raw_buffer[LO]))
	error ("can't read register %d (%s)", 
	       regnum + 1, reg_names[regnum + 1]);

      /* copy the two floats into one double, and unpack both */
      memcpy (dbl_buffer, raw_buffer, sizeof(dbl_buffer));
      flt1 = unpack_double (builtin_type_float,  raw_buffer[HI], &inv1);
      flt2 = unpack_double (builtin_type_float,  raw_buffer[LO], &inv2);
      doub = unpack_double (builtin_type_double, dbl_buffer,     &inv3);

      printf_filtered (inv1 ? " %-5s: <invalid float>" : 
		       " %-5s%-17.9g", reg_names[regnum],     flt1);
      printf_filtered (inv2 ? " %-5s: <invalid float>" : 
		       " %-5s%-17.9g", reg_names[regnum + 1], flt2);
      printf_filtered (inv3 ? " dbl: <invalid double>\n" : 
		       " dbl: %-24.17g\n", doub);
      /* may want to do hex display here (future enhancement) */
      regnum +=2;
    }
  else
    { /* eight byte registers: print each one as float AND as double. */
      int offset = 4 * (TARGET_BYTE_ORDER == BIG_ENDIAN);

      memcpy (dbl_buffer, raw_buffer[HI], sizeof(dbl_buffer));
      flt1 = unpack_double (builtin_type_float, 
			    &raw_buffer[HI][offset], &inv1);
      doub = unpack_double (builtin_type_double, dbl_buffer,    &inv3);

      printf_filtered (inv1 ? " %-5s: <invalid float>" : 
		       " %-5s flt: %-17.9g", reg_names[regnum], flt1);
      printf_filtered (inv3 ? " dbl: <invalid double>\n" : 
		       " dbl: %-24.17g\n", doub);
      /* may want to do hex display here (future enhancement) */
      regnum++;
    }
  return regnum;
}

/* Print a row's worth of GP (int) registers, with name labels above */

static int
do_gp_register_row (regnum)
     int regnum;
{ /* do values for GP (int) regs */
  char raw_buffer[REGISTER_RAW_SIZE(0)];
  int ncols = MIPS_REGSIZE == 8 ? 4 : 8;	/* display cols per row */
  int col, byte, start_regnum = regnum;

  /* For GP registers, we print a separate row of names above the vals */
  printf_filtered ("     ");
  for (col = 0; col < ncols && regnum < NUM_REGS; regnum++)
    {
      if (*reg_names[regnum] == '\0')
	continue;	/* unused register */
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
	break;	/* end the row: reached FP register */
      printf_filtered (MIPS_REGSIZE == 8 ? "%17s" : "%9s", 
		       reg_names[regnum]);
      col++;
    }
  printf_filtered (start_regnum < MIPS_NUMREGS ? "\n R%-4d" : "\n      ", 
		   start_regnum);	/* print the R0 to R31 names */

  regnum = start_regnum;	/* go back to start of row */
  /* now print the values in hex, 4 or 8 to the row */
  for (col = 0; col < ncols && regnum < NUM_REGS; regnum++)
    {
      if (*reg_names[regnum] == '\0')
	continue;	/* unused register */
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
	break;	/* end row: reached FP register */
      /* OK: get the data in raw format.  */
      if (read_relative_register_raw_bytes (regnum, raw_buffer))
	error ("can't read register %d (%s)", regnum, reg_names[regnum]);
      /* Now print the register value in hex, endian order. */
      if (TARGET_BYTE_ORDER == BIG_ENDIAN)
	for (byte = 0; byte < REGISTER_RAW_SIZE (regnum); byte++)
	  printf_filtered ("%02x", (unsigned char) raw_buffer[byte]);
      else
	for (byte = REGISTER_RAW_SIZE (regnum) - 1; byte >= 0; byte--)
	  printf_filtered ("%02x", (unsigned char) raw_buffer[byte]);
      printf_filtered (" ");
      col++;
    }
  if (col > 0)	/* ie. if we actually printed anything... */
    printf_filtered ("\n");

  return regnum;
}

/* MIPS_DO_REGISTERS_INFO(): called by "info register" command */

void
mips_do_registers_info (regnum, fpregs)
     int regnum;
     int fpregs;
{
  if (regnum != -1)	/* do one specified register */
    {
      if (*(reg_names[regnum]) == '\0')
	error ("Not a valid register for the current processor type");

      mips_print_register (regnum, 0);
      printf_filtered ("\n");
    }
  else			/* do all (or most) registers */
    {
      regnum = 0;
      while (regnum < NUM_REGS)
	if (TYPE_CODE(REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
	  if (fpregs)	/* true for "INFO ALL-REGISTERS" command */
	    regnum = do_fp_register_row (regnum);	/* FP regs */
	  else
	    regnum += MIPS_NUMREGS;	/* skip floating point regs */
	else
	  regnum = do_gp_register_row (regnum);		/* GP (int) regs */
    }
}

/* Return number of args passed to a frame. described by FIP.
   Can return -1, meaning no way to tell.  */

int
mips_frame_num_args (frame)
	struct frame_info *frame;
{
#if 0 /* FIXME Use or lose this! */
  struct chain_info_t *p;

  p = mips_find_cached_frame (FRAME_FP (frame));
  if (p->valid)
    return p->the_info.numargs;
#endif
  return -1;
}

/* Is this a branch with a delay slot?  */

static int is_delayed PARAMS ((unsigned long));

static int
is_delayed (insn)
     unsigned long insn;
{
  int i;
  for (i = 0; i < NUMOPCODES; ++i)
    if (mips_opcodes[i].pinfo != INSN_MACRO
	&& (insn & mips_opcodes[i].mask) == mips_opcodes[i].match)
      break;
  return (i < NUMOPCODES
	  && (mips_opcodes[i].pinfo & (INSN_UNCOND_BRANCH_DELAY
				       | INSN_COND_BRANCH_DELAY
				       | INSN_COND_BRANCH_LIKELY)));
}

int
mips_step_skips_delay (pc)
     CORE_ADDR pc;
{
  char buf[MIPS_INSTLEN];

  /* There is no branch delay slot on MIPS16.  */
  if (pc_is_mips16 (pc))
    return 0;

  if (target_read_memory (pc, buf, MIPS_INSTLEN) != 0)
    /* If error reading memory, guess that it is not a delayed branch.  */
    return 0;
  return is_delayed ((unsigned long)extract_unsigned_integer (buf, MIPS_INSTLEN));
}


/* Skip the PC past function prologue instructions (32-bit version).
   This is a helper function for mips_skip_prologue.  */

static CORE_ADDR
mips32_skip_prologue (pc, lenient)
     CORE_ADDR pc;	/* starting PC to search from */
     int lenient;
{
    t_inst inst;
    CORE_ADDR end_pc;
    int seen_sp_adjust = 0;
    int load_immediate_bytes = 0;

    /* Skip the typical prologue instructions. These are the stack adjustment
       instruction and the instructions that save registers on the stack
       or in the gcc frame.  */
    for (end_pc = pc + 100; pc < end_pc; pc += MIPS_INSTLEN)
      {
	unsigned long high_word;

	inst = mips_fetch_instruction (pc);
	high_word = (inst >> 16) & 0xffff;

#if 0
	if (lenient && is_delayed (inst))
	  continue;
#endif

	if (high_word == 0x27bd			/* addiu $sp,$sp,offset */
	    || high_word == 0x67bd)		/* daddiu $sp,$sp,offset */
	    seen_sp_adjust = 1;
	else if (inst == 0x03a1e823 || 	        /* subu $sp,$sp,$at */
		 inst == 0x03a8e823)   	        /* subu $sp,$sp,$t0 */
	    seen_sp_adjust = 1;
	else if (((inst & 0xFFE00000) == 0xAFA00000 /* sw reg,n($sp) */
		  || (inst & 0xFFE00000) == 0xFFA00000) /* sd reg,n($sp) */
		 && (inst & 0x001F0000))	/* reg != $zero */
	    continue;
						
	else if ((inst & 0xFFE00000) == 0xE7A00000) /* swc1 freg,n($sp) */
	    continue;
	else if ((inst & 0xF3E00000) == 0xA3C00000 && (inst & 0x001F0000))
						/* sx reg,n($s8) */
	    continue;				/* reg != $zero */
 
        /* move $s8,$sp.  With different versions of gas this will be either
           `addu $s8,$sp,$zero' or `or $s8,$sp,$zero' or `daddu s8,sp,$0'.
	    Accept any one of these.  */
        else if (inst == 0x03A0F021 || inst == 0x03a0f025 || inst == 0x03a0f02d)
	    continue;

	else if ((inst & 0xFF9F07FF) == 0x00800021) /* move reg,$a0-$a3 */
	    continue;
	else if (high_word == 0x3c1c) 		/* lui $gp,n */
	    continue;
	else if (high_word == 0x279c) 		/* addiu $gp,$gp,n */
	    continue;
	else if (inst == 0x0399e021		/* addu $gp,$gp,$t9 */
		 || inst == 0x033ce021)		/* addu $gp,$t9,$gp */
	  continue;
	/* The following instructions load $at or $t0 with an immediate
	   value in preparation for a stack adjustment via
	   subu $sp,$sp,[$at,$t0]. These instructions could also initialize
	   a local variable, so we accept them only before a stack adjustment
	   instruction was seen.  */
	else if (!seen_sp_adjust)
	  {
	    if (high_word == 0x3c01 ||	 	/* lui $at,n */
		high_word == 0x3c08)	  	/* lui $t0,n */
	      {
		load_immediate_bytes += MIPS_INSTLEN; /* FIXME!! */
		continue;
	      }
	    else if (high_word == 0x3421 || 	/* ori $at,$at,n */
		     high_word == 0x3508 || 	/* ori $t0,$t0,n */
		     high_word == 0x3401 || 	/* ori $at,$zero,n */
		     high_word == 0x3408)   	/* ori $t0,$zero,n */
	      {
		load_immediate_bytes += MIPS_INSTLEN; /* FIXME!! */
		continue;
	      }
	    else
	      break;
	  }
	else
	  break;
    }

    /* In a frameless function, we might have incorrectly
       skipped some load immediate instructions. Undo the skipping
       if the load immediate was not followed by a stack adjustment.  */
    if (load_immediate_bytes && !seen_sp_adjust)
      pc -= load_immediate_bytes;
    return pc;
}

/* Skip the PC past function prologue instructions (16-bit version).
   This is a helper function for mips_skip_prologue.  */

static CORE_ADDR
mips16_skip_prologue (pc, lenient)
     CORE_ADDR pc;	/* starting PC to search from */
     int lenient;
{
    CORE_ADDR end_pc;
    int extend_bytes = 0;
    int prev_extend_bytes;

    /* Table of instructions likely to be found in a function prologue.  */
    static struct
    {
      unsigned short inst;
      unsigned short mask;
    } table[] =
    {
      { 0x6300, 0xff00 },	/* addiu $sp,offset */
      { 0xfb00, 0xff00 },	/* daddiu $sp,offset */
      { 0xd000, 0xf800 },	/* sw reg,n($sp) */
      { 0xf900, 0xff00 },	/* sd reg,n($sp) */
      { 0x6200, 0xff00 },	/* sw $ra,n($sp) */
      { 0xfa00, 0xff00 },	/* sd $ra,n($sp) */
      { 0x673d, 0xffff },	/* move $s1,sp */
      { 0xd980, 0xff80 },	/* sw $a0-$a3,n($s1) */
      { 0x6704, 0xff1c },	/* move reg,$a0-$a3 */
      { 0xe809, 0xf81f },	/* entry pseudo-op */
      { 0x0100, 0xff00 },	/* addiu $s1,$sp,n */
      { 0, 0 }			/* end of table marker */
    };

    /* Skip the typical prologue instructions. These are the stack adjustment
       instruction and the instructions that save registers on the stack
       or in the gcc frame.  */
    for (end_pc = pc + 100; pc < end_pc; pc += MIPS16_INSTLEN)
      {
	unsigned short inst;
	int i;

	inst = mips_fetch_instruction (pc);

	/* Normally we ignore an extend instruction.  However, if it is
	   not followed by a valid prologue instruction, we must adjust
	   the pc back over the extend so that it won't be considered
	   part of the prologue.  */
	if ((inst & 0xf800) == 0xf000)		/* extend */
	  {
	    extend_bytes = MIPS16_INSTLEN;
	    continue;
	  }
	prev_extend_bytes = extend_bytes;
	extend_bytes = 0;

	/* Check for other valid prologue instructions besides extend.  */
	for (i = 0; table[i].mask != 0; i++)
	  if ((inst & table[i].mask) == table[i].inst)	/* found, get out */
	    break;
	if (table[i].mask != 0)			/* it was in table? */
	  continue;				/* ignore it */
	else					/* non-prologue */
	  {
	    /* Return the current pc, adjusted backwards by 2 if
	       the previous instruction was an extend.  */
	    return pc - prev_extend_bytes;
	  }
    }
  return pc;
}

/* To skip prologues, I use this predicate.  Returns either PC itself
   if the code at PC does not look like a function prologue; otherwise
   returns an address that (if we're lucky) follows the prologue.  If
   LENIENT, then we must skip everything which is involved in setting
   up the frame (it's OK to skip more, just so long as we don't skip
   anything which might clobber the registers which are being saved.
   We must skip more in the case where part of the prologue is in the
   delay slot of a non-prologue instruction).  */

CORE_ADDR
mips_skip_prologue (pc, lenient)
     CORE_ADDR pc;
     int lenient;
{
  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */

  CORE_ADDR post_prologue_pc = after_prologue (pc, NULL);

  if (post_prologue_pc != 0)
    return max (pc, post_prologue_pc);

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  if (pc_is_mips16 (pc))
    return mips16_skip_prologue (pc, lenient);
  else
    return mips32_skip_prologue (pc, lenient);
}

#if 0
/* The lenient prologue stuff should be superseded by the code in
   init_extra_frame_info which looks to see whether the stores mentioned
   in the proc_desc have actually taken place.  */

/* Is address PC in the prologue (loosely defined) for function at
   STARTADDR?  */

static int
mips_in_lenient_prologue (startaddr, pc)
     CORE_ADDR startaddr;
     CORE_ADDR pc;
{
  CORE_ADDR end_prologue = mips_skip_prologue (startaddr, 1);
  return pc >= startaddr && pc < end_prologue;
}
#endif

/* Given a return value in `regbuf' with a type `valtype', 
   extract and copy its value into `valbuf'.  */
void
mips_extract_return_value (valtype, regbuf, valbuf)
    struct type *valtype;
    char regbuf[REGISTER_BYTES];
    char *valbuf;
{
  int regnum;
  int offset = 0;
  int len = TYPE_LENGTH (valtype);
  
  regnum = 2;
  if (TYPE_CODE (valtype) == TYPE_CODE_FLT
      && (mips_fpu == MIPS_FPU_DOUBLE
	  || (mips_fpu == MIPS_FPU_SINGLE && len <= MIPS_FPU_SINGLE_REGSIZE)))
    regnum = FP0_REGNUM;

  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
    { /* "un-left-justify" the value from the register */
      if (len < REGISTER_RAW_SIZE (regnum))
	offset = REGISTER_RAW_SIZE (regnum) - len;
      if (len > REGISTER_RAW_SIZE (regnum)     &&	/* odd-size structs */
	  len < REGISTER_RAW_SIZE (regnum) * 2 &&
	  (TYPE_CODE (valtype) == TYPE_CODE_STRUCT ||
	   TYPE_CODE (valtype) == TYPE_CODE_UNION))
	offset = 2 * REGISTER_RAW_SIZE (regnum) - len;
    }
  memcpy (valbuf, regbuf + REGISTER_BYTE (regnum) + offset, len);
  REGISTER_CONVERT_TO_TYPE (regnum, valtype, valbuf);
}

/* Given a return value in `regbuf' with a type `valtype', 
   write it's value into the appropriate register.  */
void
mips_store_return_value (valtype, valbuf)
    struct type *valtype;
    char *valbuf;
{
  int regnum;
  int offset = 0;
  int len = TYPE_LENGTH (valtype);
  char raw_buffer[MAX_REGISTER_RAW_SIZE];
  
  regnum = 2;
  if (TYPE_CODE (valtype) == TYPE_CODE_FLT
      && (mips_fpu == MIPS_FPU_DOUBLE
	  || (mips_fpu == MIPS_FPU_SINGLE && len <= MIPS_REGSIZE)))
    regnum = FP0_REGNUM;

  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
    { /* "left-justify" the value in the register */
      if (len < REGISTER_RAW_SIZE (regnum))
	offset = REGISTER_RAW_SIZE (regnum) - len;
      if (len > REGISTER_RAW_SIZE (regnum)     &&	/* odd-size structs */
	  len < REGISTER_RAW_SIZE (regnum) * 2 &&
	  (TYPE_CODE (valtype) == TYPE_CODE_STRUCT ||
	   TYPE_CODE (valtype) == TYPE_CODE_UNION))
	offset = 2 * REGISTER_RAW_SIZE (regnum) - len;
    }
  memcpy(raw_buffer + offset, valbuf, len);
  REGISTER_CONVERT_FROM_TYPE(regnum, valtype, raw_buffer);
  write_register_bytes(REGISTER_BYTE (regnum), raw_buffer, 
		       len > REGISTER_RAW_SIZE (regnum) ? 
		       len : REGISTER_RAW_SIZE (regnum));
}

/* Exported procedure: Is PC in the signal trampoline code */

int
in_sigtramp (pc, ignore)
     CORE_ADDR pc;
     char *ignore;		/* function name */
{
  if (sigtramp_address == 0)
    fixup_sigtramp ();
  return (pc >= sigtramp_address && pc < sigtramp_end);
}

/* Command to set FPU type.  mips_fpu_string will have been set to the
   user's argument.  Set mips_fpu based on mips_fpu_string, and then
   canonicalize mips_fpu_string.  */

/*ARGSUSED*/
static void
mips_set_fpu_command (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
  char *err = NULL;

  if (mips_fpu_string == NULL || *mips_fpu_string == '\0')
    mips_fpu = MIPS_FPU_DOUBLE;
  else if (strcasecmp (mips_fpu_string, "double") == 0
	   || strcasecmp (mips_fpu_string, "on") == 0
	   || strcasecmp (mips_fpu_string, "1") == 0
	   || strcasecmp (mips_fpu_string, "yes") == 0)
    mips_fpu = MIPS_FPU_DOUBLE;
  else if (strcasecmp (mips_fpu_string, "none") == 0
	   || strcasecmp (mips_fpu_string, "off") == 0
	   || strcasecmp (mips_fpu_string, "0") == 0
	   || strcasecmp (mips_fpu_string, "no") == 0)
    mips_fpu = MIPS_FPU_NONE;
  else if (strcasecmp (mips_fpu_string, "single") == 0)
    mips_fpu = MIPS_FPU_SINGLE;
  else
    err = strsave (mips_fpu_string);

  if (mips_fpu_string != NULL)
    free (mips_fpu_string);

  switch (mips_fpu)
    {
    case MIPS_FPU_DOUBLE:
      mips_fpu_string = strsave ("double");
      break;
    case MIPS_FPU_SINGLE:
      mips_fpu_string = strsave ("single");
      break;
    case MIPS_FPU_NONE:
      mips_fpu_string = strsave ("none");
      break;
    }

  if (err != NULL)
    {
      struct cleanup *cleanups = make_cleanup (free, err);
      error ("Unknown FPU type `%s'.  Use `double', `none', or `single'.",
	     err);
      do_cleanups (cleanups);
    }
}

static void
mips_show_fpu_command (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
}

/* Command to set the processor type.  */

void
mips_set_processor_type_command (args, from_tty)
     char *args;
     int from_tty;
{
  int i;

  if (tmp_mips_processor_type == NULL || *tmp_mips_processor_type == '\0')
    {
      printf_unfiltered ("The known MIPS processor types are as follows:\n\n");
      for (i = 0; mips_processor_type_table[i].name != NULL; ++i)
	printf_unfiltered ("%s\n", mips_processor_type_table[i].name);

      /* Restore the value.  */
      tmp_mips_processor_type = strsave (mips_processor_type);

      return;
    }
  
  if (!mips_set_processor_type (tmp_mips_processor_type))
    {
      error ("Unknown processor type `%s'.", tmp_mips_processor_type);
      /* Restore its value.  */
      tmp_mips_processor_type = strsave (mips_processor_type);
    }
}

static void
mips_show_processor_type_command (args, from_tty)
     char *args;
     int from_tty;
{
}

/* Modify the actual processor type. */

int
mips_set_processor_type (str)
     char *str;
{
  int i, j;

  if (str == NULL)
    return 0;

  for (i = 0; mips_processor_type_table[i].name != NULL; ++i)
    {
      if (strcasecmp (str, mips_processor_type_table[i].name) == 0)
	{
	  mips_processor_type = str;

	  for (j = 0; j < NUM_REGS; ++j)
	    reg_names[j] = mips_processor_type_table[i].regnames[j];

	  return 1;

	  /* FIXME tweak fpu flag too */
	}
    }

  return 0;
}

/* Attempt to identify the particular processor model by reading the
   processor id.  */

char *
mips_read_processor_type ()
{
  CORE_ADDR prid;

  prid = read_register (PRID_REGNUM);

  if ((prid & ~0xf) == 0x700)
    return savestring ("r3041", strlen("r3041"));

  return NULL;
}

/* Just like reinit_frame_cache, but with the right arguments to be
   callable as an sfunc.  */

static void
reinit_frame_cache_sfunc (args, from_tty, c)
     char *args;
     int from_tty;
     struct cmd_list_element *c;
{
  reinit_frame_cache ();
}

static int
gdb_print_insn_mips (memaddr, info)
     bfd_vma memaddr;
     disassemble_info *info;
{
  mips_extra_func_info_t proc_desc;

  /* Search for the function containing this address.  Set the low bit
     of the address when searching, in case we were given an even address
     that is the start of a 16-bit function.  If we didn't do this,
     the search would fail because the symbol table says the function
     starts at an odd address, i.e. 1 byte past the given address.  */
  memaddr = ADDR_BITS_REMOVE (memaddr);
  proc_desc = non_heuristic_proc_desc (MAKE_MIPS16_ADDR (memaddr), NULL);

  /* Make an attempt to determine if this is a 16-bit function.  If
     the procedure descriptor exists and the address therein is odd,
     it's definitely a 16-bit function.  Otherwise, we have to just
     guess that if the address passed in is odd, it's 16-bits.  */
  if (proc_desc)
    info->mach = pc_is_mips16 (PROC_LOW_ADDR (proc_desc)) ? 16 : 0;
  else
    info->mach = pc_is_mips16 (memaddr) ? 16 : 0;

  /* Round down the instruction address to the appropriate boundary.  */
  memaddr &= (info->mach == 16 ? ~1 : ~3);
      
  /* Call the appropriate disassembler based on the target endian-ness.  */
  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
    return print_insn_big_mips (memaddr, info);
  else
    return print_insn_little_mips (memaddr, info);
}

/* This function implements the BREAKPOINT_FROM_PC macro.  It uses the program
   counter value to determine whether a 16- or 32-bit breakpoint should be
   used.  It returns a pointer to a string of bytes that encode a breakpoint
   instruction, stores the length of the string to *lenptr, and adjusts pc
   (if necessary) to point to the actual memory location where the
   breakpoint should be inserted.  */

unsigned char *mips_breakpoint_from_pc (pcptr, lenptr)
     CORE_ADDR *pcptr;
     int *lenptr;
{
  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
    {
      if (pc_is_mips16 (*pcptr))
	{
	  static char mips16_big_breakpoint[] = MIPS16_BIG_BREAKPOINT;
	  *pcptr = UNMAKE_MIPS16_ADDR (*pcptr);
	  *lenptr = sizeof(mips16_big_breakpoint);
	  return mips16_big_breakpoint;
	}
      else
	{
	  static char big_breakpoint[] = BIG_BREAKPOINT;
	  static char pmon_big_breakpoint[] = PMON_BIG_BREAKPOINT;
	  static char idt_big_breakpoint[] = IDT_BIG_BREAKPOINT;

	  *lenptr = sizeof(big_breakpoint);

	  if (strcmp (target_shortname, "mips") == 0)
	    return idt_big_breakpoint;
	  else if (strcmp (target_shortname, "ddb") == 0
		   || strcmp (target_shortname, "pmon") == 0
		   || strcmp (target_shortname, "lsi") == 0)
	    return pmon_big_breakpoint;
	  else
	    return big_breakpoint;
	}
    }
  else
    {
      if (pc_is_mips16 (*pcptr))
	{
	  static char mips16_little_breakpoint[] = MIPS16_LITTLE_BREAKPOINT;
	  *pcptr = UNMAKE_MIPS16_ADDR (*pcptr);
	  *lenptr = sizeof(mips16_little_breakpoint);
	  return mips16_little_breakpoint;
	}
      else
	{
	  static char little_breakpoint[] = LITTLE_BREAKPOINT;
	  static char pmon_little_breakpoint[] = PMON_LITTLE_BREAKPOINT;
	  static char idt_little_breakpoint[] = IDT_LITTLE_BREAKPOINT;

	  *lenptr = sizeof(little_breakpoint);

	  if (strcmp (target_shortname, "mips") == 0)
	    return idt_little_breakpoint;
	  else if (strcmp (target_shortname, "ddb") == 0
		   || strcmp (target_shortname, "pmon") == 0
		   || strcmp (target_shortname, "lsi") == 0)
	    return pmon_little_breakpoint;
	  else
	    return little_breakpoint;
	}
    }
}

/* Test whether the PC points to the return instruction at the
   end of a function.  This implements the ABOUT_TO_RETURN macro.  */

int 
mips_about_to_return (pc)
     CORE_ADDR pc;
{
  if (pc_is_mips16 (pc))
    /* This mips16 case isn't necessarily reliable.  Sometimes the compiler
       generates a "jr $ra"; other times it generates code to load
       the return address from the stack to an accessible register (such
       as $a3), then a "jr" using that register.  This second case
       is almost impossible to distinguish from an indirect jump
       used for switch statements, so we don't even try.  */
    return mips_fetch_instruction (pc) == 0xe820;	/* jr $ra */
  else
    return mips_fetch_instruction (pc) == 0x3e00008;	/* jr $ra */
}


/* If PC is in a mips16 call or return stub, return the address of the target
   PC, which is either the callee or the caller.  There are several
   cases which must be handled:

   * If the PC is in __mips16_ret_{d,s}f, this is a return stub and the
     target PC is in $31 ($ra).
   * If the PC is in __mips16_call_stub_{1..10}, this is a call stub
     and the target PC is in $2.
   * If the PC at the start of __mips16_call_stub_{s,d}f_{0..10}, i.e.
     before the jal instruction, this is effectively a call stub
     and the the target PC is in $2.  Otherwise this is effectively
     a return stub and the target PC is in $18.

   See the source code for the stubs in gcc/config/mips/mips16.S for
   gory details.

   This function implements the SKIP_TRAMPOLINE_CODE macro.
*/

CORE_ADDR
mips_skip_stub (pc)
     CORE_ADDR pc;
{
  char *name;
  CORE_ADDR start_addr;

  /* Find the starting address and name of the function containing the PC.  */
  if (find_pc_partial_function (pc, &name, &start_addr, NULL) == 0)
    return 0;

  /* If the PC is in __mips16_ret_{d,s}f, this is a return stub and the
     target PC is in $31 ($ra).  */
  if (strcmp (name, "__mips16_ret_sf") == 0
      || strcmp (name, "__mips16_ret_df") == 0)
    return read_register (RA_REGNUM);

  if (strncmp (name, "__mips16_call_stub_", 19) == 0)
    {
      /* If the PC is in __mips16_call_stub_{1..10}, this is a call stub
         and the target PC is in $2.  */
      if (name[19] >= '0' && name[19] <= '9')
	return read_register (2);

      /* If the PC at the start of __mips16_call_stub_{s,d}f_{0..10}, i.e.
	 before the jal instruction, this is effectively a call stub
	 and the the target PC is in $2.  Otherwise this is effectively
	 a return stub and the target PC is in $18.  */
      else if (name[19] == 's' || name[19] == 'd')
	{
	  if (pc == start_addr)
	    {
	      /* Check if the target of the stub is a compiler-generated
		 stub.  Such a stub for a function bar might have a name
		 like __fn_stub_bar, and might look like this:
		      mfc1    $4,$f13
		      mfc1    $5,$f12
		      mfc1    $6,$f15
		      mfc1    $7,$f14
		      la      $1,bar   (becomes a lui/addiu pair)
		      jr      $1
		 So scan down to the lui/addi and extract the target
		 address from those two instructions.  */

	      CORE_ADDR target_pc = read_register (2);
	      t_inst inst;
	      int i;

	      /* See if the name of the target function is  __fn_stub_*.  */
	      if (find_pc_partial_function (target_pc, &name, NULL, NULL) == 0)
		return target_pc;
	      if (strncmp (name, "__fn_stub_", 10) != 0
		  && strcmp (name, "etext") != 0
		  && strcmp (name, "_etext") != 0)
		return target_pc;

	      /* Scan through this _fn_stub_ code for the lui/addiu pair.
		 The limit on the search is arbitrarily set to 20
		 instructions.  FIXME.  */
	      for (i = 0, pc = 0; i < 20; i++, target_pc += MIPS_INSTLEN)
		{
		   inst = mips_fetch_instruction (target_pc);
		   if ((inst & 0xffff0000) == 0x3c010000)	/* lui $at */
		      pc = (inst << 16) & 0xffff0000;		/* high word */
		   else if ((inst & 0xffff0000) == 0x24210000)	/* addiu $at */
		      return pc | (inst & 0xffff);		/* low word */
		}

	      /* Couldn't find the lui/addui pair, so return stub address.  */
	      return target_pc;
	    }
	  else
	    /* This is the 'return' part of a call stub.  The return
	       address is in $r18.  */
	    return read_register (18);
	}
    }
  return 0;	/* not a stub */
}


/* Return non-zero if the PC is inside a call thunk (aka stub or trampoline).
   This implements the IN_SOLIB_CALL_TRAMPOLINE macro.  */

int
mips_in_call_stub (pc, name)
     CORE_ADDR pc;
     char *name;
{
  CORE_ADDR start_addr;

  /* Find the starting address of the function containing the PC.  If the
     caller didn't give us a name, look it up at the same time.  */
  if (find_pc_partial_function (pc, name ? NULL : &name, &start_addr, NULL) == 0)
    return 0;

  if (strncmp (name, "__mips16_call_stub_", 19) == 0)
    {
      /* If the PC is in __mips16_call_stub_{1..10}, this is a call stub.  */
      if (name[19] >= '0' && name[19] <= '9')
	return 1;
      /* If the PC at the start of __mips16_call_stub_{s,d}f_{0..10}, i.e.
	 before the jal instruction, this is effectively a call stub.  */
      else if (name[19] == 's' || name[19] == 'd')
	return pc == start_addr;
    }

  return 0;	/* not a stub */
}


/* Return non-zero if the PC is inside a return thunk (aka stub or trampoline).
   This implements the IN_SOLIB_RETURN_TRAMPOLINE macro.  */

int
mips_in_return_stub (pc, name)
     CORE_ADDR pc;
     char *name;
{
  CORE_ADDR start_addr;

  /* Find the starting address of the function containing the PC.  */
  if (find_pc_partial_function (pc, NULL, &start_addr, NULL) == 0)
    return 0;

  /* If the PC is in __mips16_ret_{d,s}f, this is a return stub.  */
  if (strcmp (name, "__mips16_ret_sf") == 0
      || strcmp (name, "__mips16_ret_df") == 0)
    return 1;

  /* If the PC is in __mips16_call_stub_{s,d}f_{0..10} but not at the start,
      i.e. after the jal instruction, this is effectively a return stub.  */
  if (strncmp (name, "__mips16_call_stub_", 19) == 0
      && (name[19] == 's' || name[19] == 'd')
      && pc != start_addr)
    return 1;

  return 0;	/* not a stub */
}


/* Return non-zero if the PC is in a library helper function that should
   be ignored.  This implements the IGNORE_HELPER_CALL macro.  */

int
mips_ignore_helper (pc)
     CORE_ADDR pc;
{
  char *name;

  /* Find the starting address and name of the function containing the PC.  */
  if (find_pc_partial_function (pc, &name, NULL, NULL) == 0)
    return 0;

  /* If the PC is in __mips16_ret_{d,s}f, this is a library helper function
     that we want to ignore.  */
  return (strcmp (name, "__mips16_ret_sf") == 0
	  || strcmp (name, "__mips16_ret_df") == 0);
}


void
_initialize_mips_tdep ()
{
  struct cmd_list_element *c;

  tm_print_insn = gdb_print_insn_mips;

  /* Let the user turn off floating point and set the fence post for
     heuristic_proc_start.  */

  c = add_set_cmd ("mipsfpu", class_support, var_string_noescape,
		   (char *) &mips_fpu_string,
		   "Set use of floating point coprocessor.\n\
Set to `none' to avoid using floating point instructions when calling\n\
functions or dealing with return values.  Set to `single' to use only\n\
single precision floating point as on the R4650.  Set to `double' for\n\
normal floating point support.",
		   &setlist);
  c->function.sfunc = mips_set_fpu_command;
  c = add_show_from_set (c, &showlist);
  c->function.sfunc = mips_show_fpu_command;

#ifndef MIPS_DEFAULT_FPU_TYPE
  mips_fpu = MIPS_FPU_DOUBLE;
  mips_fpu_string = strsave ("double");
#else
  mips_fpu = MIPS_DEFAULT_FPU_TYPE;
  switch (mips_fpu) 
  {
    case MIPS_FPU_DOUBLE:  mips_fpu_string = strsave ("double");  break;
    case MIPS_FPU_SINGLE:  mips_fpu_string = strsave ("single");  break;
    case MIPS_FPU_NONE:    mips_fpu_string = strsave ("none");    break;
  }    
#endif

  c = add_set_cmd ("processor", class_support, var_string_noescape,
		   (char *) &tmp_mips_processor_type,
		   "Set the type of MIPS processor in use.\n\
Set this to be able to access processor-type-specific registers.\n\
",
		   &setlist);
  c->function.cfunc = mips_set_processor_type_command;
  c = add_show_from_set (c, &showlist);
  c->function.cfunc = mips_show_processor_type_command;

  tmp_mips_processor_type = strsave (DEFAULT_MIPS_TYPE);
  mips_set_processor_type_command (strsave (DEFAULT_MIPS_TYPE), 0);

  /* We really would like to have both "0" and "unlimited" work, but
     command.c doesn't deal with that.  So make it a var_zinteger
     because the user can always use "999999" or some such for unlimited.  */
  c = add_set_cmd ("heuristic-fence-post", class_support, var_zinteger,
		   (char *) &heuristic_fence_post,
		   "\
Set the distance searched for the start of a function.\n\
If you are debugging a stripped executable, GDB needs to search through the\n\
program for the start of a function.  This command sets the distance of the\n\
search.  The only need to set it is when debugging a stripped executable.",
		   &setlist);
  /* We need to throw away the frame cache when we set this, since it
     might change our ability to get backtraces.  */
  c->function.sfunc = reinit_frame_cache_sfunc;
  add_show_from_set (c, &showlist);
}

#ifdef NO_SINGLE_STEP
/* Non-zero if we just simulated a single-step ptrace call.  This is
   needed because we cannot remove the breakpoints in the inferior
   process until after the `wait' in `wait_for_inferior'.  Used for
   4.4bsd for mips, where the kernel does not emulate single-step. */

int one_stepped;
CORE_ADDR target_addr;		/* Branch target offset, if we have a
				   breakpoint there... */
CORE_ADDR step_addr;		/* Offset of instruction after instruction
				   to be stepped, if we have a breakpoint
				   there. */
long step_cache [3];		/* Cache for instructions wiped out by
				   step breakpoint(s)... */

/* single_step() is called just before we want to resume the inferior,
   if we want to single-step it but there is no hardware or kernel single-step
   support (as on 4.4bsd for pmax).  We find all the possible targets of the
   coming instruction and breakpoint them.

   single_step is also called just after the inferior stops.  If we had
   set up a simulated single-step, we undo our damage. */

/* thoughts:

   For the current instruction, check to see if we're in a delay slot.
   If we are, the next instruction executed will either be the target of
   the branch or jump instruction preceding the current instruction, or
   it will be the instruction following the current instruction.   If
   we are not, then the next instruction executed will either be the
   instruction following the current instruction, or the instruction
   following that (if the current instruction is a branch likely instruction
   and the branch is not taken).

   So, if we are in a delay slot then we set a breakpoint for the target
   of the preceding instruction.   Unless the preceding instruction was
   a jump instruction (only jumps are unconditional), we also set a break-
   point at the instruction following the current one and the instruction
   following that.   Setting two breakpoints after the current instruction
   is cheaper and easier than figuring out whether the current instruction
   is a branch likely instruction. */

void
single_step (ignore)
     int ignore; /* pid, but we don't need it */
{
  CORE_ADDR pc;
  CORE_ADDR epc;
  CORE_ADDR next;
  unsigned long cause;
  unsigned long delay_instruction;

  if (!one_stepped)
    {
      epc = read_register (PC_REGNUM);
      cause = read_register (CAUSE_REGNUM);
      pc = epc;
#define CAUSE_BD	0x80000000UL
      if (cause & CAUSE_BD)
	pc += 4;
      next = pc + 4;
      target_addr = 0;
      step_addr = next;

      if (cause & CAUSE_BD)
	{
          delay_instruction =
	      read_memory_integer (epc, sizeof(delay_instruction));

/* Main instruction opcode mask - top six bits... */
#define MI_IMASK	0xfc000000UL
/* Opcode for ``special'' instructions.... */
#define MI_SPECIAL	0x0
/* Opcode for ``regimm'' instructions... */
#define MI_REGIMM	0x04000000
/* Mask for special instruction opcodes... */
#if 0
#  define MS_IMASK	0x37	/* XXX Mellon's original, broken? */
#else
#  define MS_IMASK	0x3f	/* XXX jrs attempt at fix */
#endif

/* JALR and JR are special instructions... */
#define MS_JALR		9
#define MS_JR		8
/* Target register for jump... */
#define MS_JR_REG(x)	(((x) & 0x03e00000) >> 21)
/* J and JAL are regular instructions... */
#define MI_J		0x08000000
#define	MI_JAL		0x0c000000
/* All the bits following the opcode are used in J and JAL instructions... */
#define MI_JMASK	(~MI_IMASK)
#define MI_JSIGN	0x20000000
/* Coprocessor instructions use the bottom two bits of the opcode as a
   coprocessor id, so we only test the top four bits to see if this is a
   COPz instruction... */
#define MC_MASK		0xf0000000UL
#define MC_COPz		0x40000000
/* The coprocessor subopcode is in the five bits following the regular
   instruction opcode... */
#define	MCB_MASK	0x03e00000
/* All coprocessor branches have the following subopcode... */
#define MCB_BC		0x01000000
/* Offsets for coprocessor branches are contained in the bottom 16 bis. */
#define	MCB_OFFSET	0xffff
/* The following are all the branch instructions with regular opcodes: */
#define MI_BEQ		0x10000000
#define MI_BEQL		0x50000000
#define MI_BGTZ		0x1c000000
#define MI_BGTZL	0x5c000000
#define MI_BLEZ		0x18000000
#define MI_BLEZL	0x58000000
#define	MI_BNE		0x14000000
#define MI_BNEL		0x54000000
/* They also use the lower sixteen bits for their target offset... */
#define	BRANCH_OFFSET	0xffff
/* REGIMM instructions have a second opcode in bits 20 through 16... */
#define REGIMM_MASK	0x001f0000
/* The following are all the ``regimm'' branch instructions: */
#define	MRI_BGEZ	0x00010000
#define	MRI_BGEZAL	0x00110000
#define	MRI_BGEZALL	0x00130000
#define	MRI_BGEZL	0x00030000
#define	MRI_BLTZ	0x00000000
#define	MRI_BLTZAL	0x00100000
#define	MRI_BLTZALL	0x00120000
#define	MRI_BLTZL	0x00020000

		/* Decode the instruction sufficiently to determine
		   the branch target... */
	  switch (delay_instruction & MI_IMASK)
	    {
	    case MI_J:
	    case MI_JAL:
	      target_addr = delay_instruction & MI_JMASK;
		/* Sign extend... */
	      if (target_addr & MI_JSIGN)
	        target_addr |= ~(CORE_ADDR)MI_JMASK;
	      target_addr += (pc >> 2) & ~MI_JMASK;
	      target_addr <<= 2;
	      step_addr = 0;
	      break;
	    case MI_SPECIAL:
	      switch (delay_instruction & MS_IMASK)
	        {
	        case MS_JALR:
	        case MS_JR:
	          target_addr +=
		    read_register (MS_JR_REG (delay_instruction));
		  step_addr = 0;
	          break;
		default:
		bad_delay:
		  error ("In delay slot of non-branch instruction: %x\n",
			 delay_instruction);
		  break;
	        }
	      break;
	    case MI_REGIMM:
	      switch (delay_instruction & REGIMM_MASK)
		{
		case MRI_BGEZ:
		case MRI_BGEZAL:
		case MRI_BGEZALL:
		case MRI_BGEZL:
		case MRI_BLTZ:
		case MRI_BLTZAL:
		case MRI_BLTZALL:
		case MRI_BLTZL:
			/* Compiler should sign extend this... */
		  target_addr = (short)delay_instruction;
		  target_addr <<= 2;
		  target_addr += pc;
		  break;
		default:
		  error ("In delay  slot, register-immediate insn: %x\n",
			 delay_instruction);
		  goto bad_delay;
		}
	      break;
	    case MI_BEQ:
	    case MI_BEQL:
	    case MI_BGTZ:
	    case MI_BGTZL:
	    case MI_BLEZ:
	    case MI_BLEZL:
	    case MI_BNE:
	    case MI_BNEL:
	      target_addr = (short)delay_instruction;
	      target_addr <<= 2;
	      target_addr += pc;
	      break;
	    default:
	      error ("In delay  slot, unrecognised instruction: %x\n",
			 delay_instruction);
	      goto bad_delay;
	    }
	}

	/* Don't try to put down two breakpoints in the same spot... */
      if (step_addr == target_addr)
	target_addr = 0;

      if (step_addr)
	{
	  target_insert_breakpoint (step_addr, (char *)&step_cache [0]);
	  if (step_addr + 4 != target_addr)
	    target_insert_breakpoint (step_addr + 4, (char *)&step_cache [1]);
        }
      if (target_addr)
        {
	  target_insert_breakpoint (target_addr, (char *)&step_cache [2]);
	}

	/* If the breakpoint occurred in a branch instruction,
	   re-run the branch (the breakpoint instruction should
	   be gone by now)... */
      if (epc != pc)
	{
	  write_register (PC_REGNUM, epc);
	}
      one_stepped = 1;
      return;
    }
  else
    {
      epc = read_register (PC_REGNUM);
      cause = read_register (CAUSE_REGNUM);
      pc = epc;
#define CAUSE_BD	0x80000000UL
      if (cause & CAUSE_BD)
	pc += 4;
      /* Remove step breakpoints */
      if (step_addr)
	{
	  target_remove_breakpoint (step_addr, (char *)&step_cache [0]);
	  if (step_addr + 4 != target_addr)
	    target_remove_breakpoint (step_addr + 4, (char *)&step_cache [1]);
	}

      if (target_addr)
	{
          target_remove_breakpoint (target_addr, (char *)&step_cache [2]);
	  target_addr = 0;
	}

      one_stepped = 0;
    }
}
#endif	/* NO_SINGLE_STEP */
