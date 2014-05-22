/* Intel 386 target-dependent stuff.

   Copyright (C) 1988-2013 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "opcode/i386.h"
#include "arch-utils.h"
#include "command.h"
#include "dummy-frame.h"
#include "dwarf2-frame.h"
#include "doublest.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "inferior.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "objfiles.h"
#include "osabi.h"
#include "regcache.h"
#include "reggroups.h"
#include "regset.h"
#include "symfile.h"
#include "symtab.h"
#include "target.h"
#include "value.h"
#include "dis-asm.h"
#include "disasm.h"
#include "remote.h"
#include "exceptions.h"
#include "gdb_assert.h"
#include "gdb_string.h"

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "i386-xstate.h"

#include "record.h"
#include "record-full.h"
#include <stdint.h>

#include "features/i386/i386.c"
#include "features/i386/i386-avx.c"
#include "features/i386/i386-mmx.c"

#include "ax.h"
#include "ax-gdb.h"

#include "stap-probe.h"
#include "user-regs.h"
#include "cli/cli-utils.h"
#include "expression.h"
#include "parser-defs.h"
#include <ctype.h>

/* Register names.  */

static const char *i386_register_names[] =
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

static const char *i386_ymm_names[] =
{
  "ymm0",  "ymm1",   "ymm2",  "ymm3",
  "ymm4",  "ymm5",   "ymm6",  "ymm7",
};

static const char *i386_ymmh_names[] =
{
  "ymm0h",  "ymm1h",   "ymm2h",  "ymm3h",
  "ymm4h",  "ymm5h",   "ymm6h",  "ymm7h",
};

/* Register names for MMX pseudo-registers.  */

static const char *i386_mmx_names[] =
{
  "mm0", "mm1", "mm2", "mm3",
  "mm4", "mm5", "mm6", "mm7"
};

/* Register names for byte pseudo-registers.  */

static const char *i386_byte_names[] =
{
  "al", "cl", "dl", "bl", 
  "ah", "ch", "dh", "bh"
};

/* Register names for word pseudo-registers.  */

static const char *i386_word_names[] =
{
  "ax", "cx", "dx", "bx",
  "", "bp", "si", "di"
};

/* MMX register?  */

static int
i386_mmx_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int mm0_regnum = tdep->mm0_regnum;

  if (mm0_regnum < 0)
    return 0;

  regnum -= mm0_regnum;
  return regnum >= 0 && regnum < tdep->num_mmx_regs;
}

/* Byte register?  */

int
i386_byte_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  regnum -= tdep->al_regnum;
  return regnum >= 0 && regnum < tdep->num_byte_regs;
}

/* Word register?  */

int
i386_word_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  regnum -= tdep->ax_regnum;
  return regnum >= 0 && regnum < tdep->num_word_regs;
}

/* Dword register?  */

int
i386_dword_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int eax_regnum = tdep->eax_regnum;

  if (eax_regnum < 0)
    return 0;

  regnum -= eax_regnum;
  return regnum >= 0 && regnum < tdep->num_dword_regs;
}

static int
i386_ymmh_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int ymm0h_regnum = tdep->ymm0h_regnum;

  if (ymm0h_regnum < 0)
    return 0;

  regnum -= ymm0h_regnum;
  return regnum >= 0 && regnum < tdep->num_ymm_regs;
}

/* AVX register?  */

int
i386_ymm_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int ymm0_regnum = tdep->ymm0_regnum;

  if (ymm0_regnum < 0)
    return 0;

  regnum -= ymm0_regnum;
  return regnum >= 0 && regnum < tdep->num_ymm_regs;
}

/* SSE register?  */

int
i386_xmm_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int num_xmm_regs = I387_NUM_XMM_REGS (tdep);

  if (num_xmm_regs == 0)
    return 0;

  regnum -= I387_XMM0_REGNUM (tdep);
  return regnum >= 0 && regnum < num_xmm_regs;
}

static int
i386_mxcsr_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (I387_NUM_XMM_REGS (tdep) == 0)
    return 0;

  return (regnum == I387_MXCSR_REGNUM (tdep));
}

/* FP register?  */

int
i386_fp_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (I387_ST0_REGNUM (tdep) < 0)
    return 0;

  return (I387_ST0_REGNUM (tdep) <= regnum
	  && regnum < I387_FCTRL_REGNUM (tdep));
}

int
i386_fpc_regnum_p (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (I387_ST0_REGNUM (tdep) < 0)
    return 0;

  return (I387_FCTRL_REGNUM (tdep) <= regnum 
	  && regnum < I387_XMM0_REGNUM (tdep));
}

/* Return the name of register REGNUM, or the empty string if it is
   an anonymous register.  */

static const char *
i386_register_name (struct gdbarch *gdbarch, int regnum)
{
  /* Hide the upper YMM registers.  */
  if (i386_ymmh_regnum_p (gdbarch, regnum))
    return "";

  return tdesc_register_name (gdbarch, regnum);
}

/* Return the name of register REGNUM.  */

const char *
i386_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  if (i386_mmx_regnum_p (gdbarch, regnum))
    return i386_mmx_names[regnum - I387_MM0_REGNUM (tdep)];
  else if (i386_ymm_regnum_p (gdbarch, regnum))
    return i386_ymm_names[regnum - tdep->ymm0_regnum];
  else if (i386_byte_regnum_p (gdbarch, regnum))
    return i386_byte_names[regnum - tdep->al_regnum];
  else if (i386_word_regnum_p (gdbarch, regnum))
    return i386_word_names[regnum - tdep->ax_regnum];

  internal_error (__FILE__, __LINE__, _("invalid regnum"));
}

/* Convert a dbx register number REG to the appropriate register
   number used by GDB.  */

static int
i386_dbx_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* This implements what GCC calls the "default" register map
     (dbx_register_map[]).  */

  if (reg >= 0 && reg <= 7)
    {
      /* General-purpose registers.  The debug info calls %ebp
         register 4, and %esp register 5.  */
      if (reg == 4)
        return 5;
      else if (reg == 5)
        return 4;
      else return reg;
    }
  else if (reg >= 12 && reg <= 19)
    {
      /* Floating-point registers.  */
      return reg - 12 + I387_ST0_REGNUM (tdep);
    }
  else if (reg >= 21 && reg <= 28)
    {
      /* SSE registers.  */
      int ymm0_regnum = tdep->ymm0_regnum;

      if (ymm0_regnum >= 0
	  && i386_xmm_regnum_p (gdbarch, reg))
	return reg - 21 + ymm0_regnum;
      else
	return reg - 21 + I387_XMM0_REGNUM (tdep);
    }
  else if (reg >= 29 && reg <= 36)
    {
      /* MMX registers.  */
      return reg - 29 + I387_MM0_REGNUM (tdep);
    }

  /* This will hopefully provoke a warning.  */
  return gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);
}

/* Convert SVR4 register number REG to the appropriate register number
   used by GDB.  */

static int
i386_svr4_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* This implements the GCC register map that tries to be compatible
     with the SVR4 C compiler for DWARF (svr4_dbx_register_map[]).  */

  /* The SVR4 register numbering includes %eip and %eflags, and
     numbers the floating point registers differently.  */
  if (reg >= 0 && reg <= 9)
    {
      /* General-purpose registers.  */
      return reg;
    }
  else if (reg >= 11 && reg <= 18)
    {
      /* Floating-point registers.  */
      return reg - 11 + I387_ST0_REGNUM (tdep);
    }
  else if (reg >= 21 && reg <= 36)
    {
      /* The SSE and MMX registers have the same numbers as with dbx.  */
      return i386_dbx_reg_to_regnum (gdbarch, reg);
    }

  switch (reg)
    {
    case 37: return I387_FCTRL_REGNUM (tdep);
    case 38: return I387_FSTAT_REGNUM (tdep);
    case 39: return I387_MXCSR_REGNUM (tdep);
    case 40: return I386_ES_REGNUM;
    case 41: return I386_CS_REGNUM;
    case 42: return I386_SS_REGNUM;
    case 43: return I386_DS_REGNUM;
    case 44: return I386_FS_REGNUM;
    case 45: return I386_GS_REGNUM;
    }

  /* This will hopefully provoke a warning.  */
  return gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);
}



/* This is the variable that is set with "set disassembly-flavor", and
   its legitimate values.  */
static const char att_flavor[] = "att";
static const char intel_flavor[] = "intel";
static const char *const valid_flavors[] =
{
  att_flavor,
  intel_flavor,
  NULL
};
static const char *disassembly_flavor = att_flavor;


/* Use the program counter to determine the contents and size of a
   breakpoint instruction.  Return a pointer to a string of bytes that
   encode a breakpoint instruction, store the length of the string in
   *LEN and optionally adjust *PC to point to the correct memory
   location for inserting the breakpoint.

   On the i386 we have a single breakpoint that fits in a single byte
   and can be inserted anywhere.

   This function is 64-bit safe.  */

static const gdb_byte *
i386_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pc, int *len)
{
  static gdb_byte break_insn[] = { 0xcc }; /* int 3 */

  *len = sizeof (break_insn);
  return break_insn;
}

/* Displaced instruction handling.  */

/* Skip the legacy instruction prefixes in INSN.
   Not all prefixes are valid for any particular insn
   but we needn't care, the insn will fault if it's invalid.
   The result is a pointer to the first opcode byte,
   or NULL if we run off the end of the buffer.  */

static gdb_byte *
i386_skip_prefixes (gdb_byte *insn, size_t max_len)
{
  gdb_byte *end = insn + max_len;

  while (insn < end)
    {
      switch (*insn)
	{
	case DATA_PREFIX_OPCODE:
	case ADDR_PREFIX_OPCODE:
	case CS_PREFIX_OPCODE:
	case DS_PREFIX_OPCODE:
	case ES_PREFIX_OPCODE:
	case FS_PREFIX_OPCODE:
	case GS_PREFIX_OPCODE:
	case SS_PREFIX_OPCODE:
	case LOCK_PREFIX_OPCODE:
	case REPE_PREFIX_OPCODE:
	case REPNE_PREFIX_OPCODE:
	  ++insn;
	  continue;
	default:
	  return insn;
	}
    }

  return NULL;
}

static int
i386_absolute_jmp_p (const gdb_byte *insn)
{
  /* jmp far (absolute address in operand).  */
  if (insn[0] == 0xea)
    return 1;

  if (insn[0] == 0xff)
    {
      /* jump near, absolute indirect (/4).  */
      if ((insn[1] & 0x38) == 0x20)
        return 1;

      /* jump far, absolute indirect (/5).  */
      if ((insn[1] & 0x38) == 0x28)
        return 1;
    }

  return 0;
}

static int
i386_absolute_call_p (const gdb_byte *insn)
{
  /* call far, absolute.  */
  if (insn[0] == 0x9a)
    return 1;

  if (insn[0] == 0xff)
    {
      /* Call near, absolute indirect (/2).  */
      if ((insn[1] & 0x38) == 0x10)
        return 1;

      /* Call far, absolute indirect (/3).  */
      if ((insn[1] & 0x38) == 0x18)
        return 1;
    }

  return 0;
}

static int
i386_ret_p (const gdb_byte *insn)
{
  switch (insn[0])
    {
    case 0xc2: /* ret near, pop N bytes.  */
    case 0xc3: /* ret near */
    case 0xca: /* ret far, pop N bytes.  */
    case 0xcb: /* ret far */
    case 0xcf: /* iret */
      return 1;

    default:
      return 0;
    }
}

static int
i386_call_p (const gdb_byte *insn)
{
  if (i386_absolute_call_p (insn))
    return 1;

  /* call near, relative.  */
  if (insn[0] == 0xe8)
    return 1;

  return 0;
}

/* Return non-zero if INSN is a system call, and set *LENGTHP to its
   length in bytes.  Otherwise, return zero.  */

static int
i386_syscall_p (const gdb_byte *insn, int *lengthp)
{
  /* Is it 'int $0x80'?  */
  if ((insn[0] == 0xcd && insn[1] == 0x80)
      /* Or is it 'sysenter'?  */
      || (insn[0] == 0x0f && insn[1] == 0x34)
      /* Or is it 'syscall'?  */
      || (insn[0] == 0x0f && insn[1] == 0x05))
    {
      *lengthp = 2;
      return 1;
    }

  return 0;
}

/* Some kernels may run one past a syscall insn, so we have to cope.
   Otherwise this is just simple_displaced_step_copy_insn.  */

struct displaced_step_closure *
i386_displaced_step_copy_insn (struct gdbarch *gdbarch,
			       CORE_ADDR from, CORE_ADDR to,
			       struct regcache *regs)
{
  size_t len = gdbarch_max_insn_length (gdbarch);
  gdb_byte *buf = xmalloc (len);

  read_memory (from, buf, len);

  /* GDB may get control back after the insn after the syscall.
     Presumably this is a kernel bug.
     If this is a syscall, make sure there's a nop afterwards.  */
  {
    int syscall_length;
    gdb_byte *insn;

    insn = i386_skip_prefixes (buf, len);
    if (insn != NULL && i386_syscall_p (insn, &syscall_length))
      insn[syscall_length] = NOP_OPCODE;
  }

  write_memory (to, buf, len);

  if (debug_displaced)
    {
      fprintf_unfiltered (gdb_stdlog, "displaced: copy %s->%s: ",
                          paddress (gdbarch, from), paddress (gdbarch, to));
      displaced_step_dump_bytes (gdb_stdlog, buf, len);
    }

  return (struct displaced_step_closure *) buf;
}

/* Fix up the state of registers and memory after having single-stepped
   a displaced instruction.  */

void
i386_displaced_step_fixup (struct gdbarch *gdbarch,
                           struct displaced_step_closure *closure,
                           CORE_ADDR from, CORE_ADDR to,
                           struct regcache *regs)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* The offset we applied to the instruction's address.
     This could well be negative (when viewed as a signed 32-bit
     value), but ULONGEST won't reflect that, so take care when
     applying it.  */
  ULONGEST insn_offset = to - from;

  /* Since we use simple_displaced_step_copy_insn, our closure is a
     copy of the instruction.  */
  gdb_byte *insn = (gdb_byte *) closure;
  /* The start of the insn, needed in case we see some prefixes.  */
  gdb_byte *insn_start = insn;

  if (debug_displaced)
    fprintf_unfiltered (gdb_stdlog,
                        "displaced: fixup (%s, %s), "
                        "insn = 0x%02x 0x%02x ...\n",
                        paddress (gdbarch, from), paddress (gdbarch, to),
			insn[0], insn[1]);

  /* The list of issues to contend with here is taken from
     resume_execution in arch/i386/kernel/kprobes.c, Linux 2.6.20.
     Yay for Free Software!  */

  /* Relocate the %eip, if necessary.  */

  /* The instruction recognizers we use assume any leading prefixes
     have been skipped.  */
  {
    /* This is the size of the buffer in closure.  */
    size_t max_insn_len = gdbarch_max_insn_length (gdbarch);
    gdb_byte *opcode = i386_skip_prefixes (insn, max_insn_len);
    /* If there are too many prefixes, just ignore the insn.
       It will fault when run.  */
    if (opcode != NULL)
      insn = opcode;
  }

  /* Except in the case of absolute or indirect jump or call
     instructions, or a return instruction, the new eip is relative to
     the displaced instruction; make it relative.  Well, signal
     handler returns don't need relocation either, but we use the
     value of %eip to recognize those; see below.  */
  if (! i386_absolute_jmp_p (insn)
      && ! i386_absolute_call_p (insn)
      && ! i386_ret_p (insn))
    {
      ULONGEST orig_eip;
      int insn_len;

      regcache_cooked_read_unsigned (regs, I386_EIP_REGNUM, &orig_eip);

      /* A signal trampoline system call changes the %eip, resuming
         execution of the main program after the signal handler has
         returned.  That makes them like 'return' instructions; we
         shouldn't relocate %eip.

         But most system calls don't, and we do need to relocate %eip.

         Our heuristic for distinguishing these cases: if stepping
         over the system call instruction left control directly after
         the instruction, the we relocate --- control almost certainly
         doesn't belong in the displaced copy.  Otherwise, we assume
         the instruction has put control where it belongs, and leave
         it unrelocated.  Goodness help us if there are PC-relative
         system calls.  */
      if (i386_syscall_p (insn, &insn_len)
          && orig_eip != to + (insn - insn_start) + insn_len
	  /* GDB can get control back after the insn after the syscall.
	     Presumably this is a kernel bug.
	     i386_displaced_step_copy_insn ensures its a nop,
	     we add one to the length for it.  */
          && orig_eip != to + (insn - insn_start) + insn_len + 1)
        {
          if (debug_displaced)
            fprintf_unfiltered (gdb_stdlog,
                                "displaced: syscall changed %%eip; "
                                "not relocating\n");
        }
      else
        {
          ULONGEST eip = (orig_eip - insn_offset) & 0xffffffffUL;

	  /* If we just stepped over a breakpoint insn, we don't backup
	     the pc on purpose; this is to match behaviour without
	     stepping.  */

          regcache_cooked_write_unsigned (regs, I386_EIP_REGNUM, eip);

          if (debug_displaced)
            fprintf_unfiltered (gdb_stdlog,
                                "displaced: "
                                "relocated %%eip from %s to %s\n",
                                paddress (gdbarch, orig_eip),
				paddress (gdbarch, eip));
        }
    }

  /* If the instruction was PUSHFL, then the TF bit will be set in the
     pushed value, and should be cleared.  We'll leave this for later,
     since GDB already messes up the TF flag when stepping over a
     pushfl.  */

  /* If the instruction was a call, the return address now atop the
     stack is the address following the copied instruction.  We need
     to make it the address following the original instruction.  */
  if (i386_call_p (insn))
    {
      ULONGEST esp;
      ULONGEST retaddr;
      const ULONGEST retaddr_len = 4;

      regcache_cooked_read_unsigned (regs, I386_ESP_REGNUM, &esp);
      retaddr = read_memory_unsigned_integer (esp, retaddr_len, byte_order);
      retaddr = (retaddr - insn_offset) & 0xffffffffUL;
      write_memory_unsigned_integer (esp, retaddr_len, byte_order, retaddr);

      if (debug_displaced)
        fprintf_unfiltered (gdb_stdlog,
                            "displaced: relocated return addr at %s to %s\n",
                            paddress (gdbarch, esp),
                            paddress (gdbarch, retaddr));
    }
}

static void
append_insns (CORE_ADDR *to, ULONGEST len, const gdb_byte *buf)
{
  target_write_memory (*to, buf, len);
  *to += len;
}

static void
i386_relocate_instruction (struct gdbarch *gdbarch,
			   CORE_ADDR *to, CORE_ADDR oldloc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[I386_MAX_INSN_LEN];
  int offset = 0, rel32, newrel;
  int insn_length;
  gdb_byte *insn = buf;

  read_memory (oldloc, buf, I386_MAX_INSN_LEN);

  insn_length = gdb_buffered_insn_length (gdbarch, insn,
					  I386_MAX_INSN_LEN, oldloc);

  /* Get past the prefixes.  */
  insn = i386_skip_prefixes (insn, I386_MAX_INSN_LEN);

  /* Adjust calls with 32-bit relative addresses as push/jump, with
     the address pushed being the location where the original call in
     the user program would return to.  */
  if (insn[0] == 0xe8)
    {
      gdb_byte push_buf[16];
      unsigned int ret_addr;

      /* Where "ret" in the original code will return to.  */
      ret_addr = oldloc + insn_length;
      push_buf[0] = 0x68; /* pushq $...  */
      store_unsigned_integer (&push_buf[1], 4, byte_order, ret_addr);
      /* Push the push.  */
      append_insns (to, 5, push_buf);

      /* Convert the relative call to a relative jump.  */
      insn[0] = 0xe9;

      /* Adjust the destination offset.  */
      rel32 = extract_signed_integer (insn + 1, 4, byte_order);
      newrel = (oldloc - *to) + rel32;
      store_signed_integer (insn + 1, 4, byte_order, newrel);

      if (debug_displaced)
	fprintf_unfiltered (gdb_stdlog,
			    "Adjusted insn rel32=%s at %s to"
			    " rel32=%s at %s\n",
			    hex_string (rel32), paddress (gdbarch, oldloc),
			    hex_string (newrel), paddress (gdbarch, *to));

      /* Write the adjusted jump into its displaced location.  */
      append_insns (to, 5, insn);
      return;
    }

  /* Adjust jumps with 32-bit relative addresses.  Calls are already
     handled above.  */
  if (insn[0] == 0xe9)
    offset = 1;
  /* Adjust conditional jumps.  */
  else if (insn[0] == 0x0f && (insn[1] & 0xf0) == 0x80)
    offset = 2;

  if (offset)
    {
      rel32 = extract_signed_integer (insn + offset, 4, byte_order);
      newrel = (oldloc - *to) + rel32;
      store_signed_integer (insn + offset, 4, byte_order, newrel);
      if (debug_displaced)
	fprintf_unfiltered (gdb_stdlog,
			    "Adjusted insn rel32=%s at %s to"
			    " rel32=%s at %s\n",
			    hex_string (rel32), paddress (gdbarch, oldloc),
			    hex_string (newrel), paddress (gdbarch, *to));
    }

  /* Write the adjusted instructions into their displaced
     location.  */
  append_insns (to, insn_length, buf);
}


#ifdef I386_REGNO_TO_SYMMETRY
#error "The Sequent Symmetry is no longer supported."
#endif

/* According to the System V ABI, the registers %ebp, %ebx, %edi, %esi
   and %esp "belong" to the calling function.  Therefore these
   registers should be saved if they're going to be modified.  */

/* The maximum number of saved registers.  This should include all
   registers mentioned above, and %eip.  */
#define I386_NUM_SAVED_REGS	I386_NUM_GREGS

struct i386_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  int base_p;
  LONGEST sp_offset;
  CORE_ADDR pc;

  /* Saved registers.  */
  CORE_ADDR saved_regs[I386_NUM_SAVED_REGS];
  CORE_ADDR saved_sp;
  int saved_sp_reg;
  int pc_in_eax;

  /* Stack space reserved for local variables.  */
  long locals;
};

/* Allocate and initialize a frame cache.  */

static struct i386_frame_cache *
i386_alloc_frame_cache (void)
{
  struct i386_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct i386_frame_cache);

  /* Base address.  */
  cache->base_p = 0;
  cache->base = 0;
  cache->sp_offset = -4;
  cache->pc = 0;

  /* Saved registers.  We initialize these to -1 since zero is a valid
     offset (that's where %ebp is supposed to be stored).  */
  for (i = 0; i < I386_NUM_SAVED_REGS; i++)
    cache->saved_regs[i] = -1;
  cache->saved_sp = 0;
  cache->saved_sp_reg = -1;
  cache->pc_in_eax = 0;

  /* Frameless until proven otherwise.  */
  cache->locals = -1;

  return cache;
}

/* If the instruction at PC is a jump, return the address of its
   target.  Otherwise, return PC.  */

static CORE_ADDR
i386_follow_jump (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte op;
  long delta = 0;
  int data16 = 0;

  if (target_read_memory (pc, &op, 1))
    return pc;

  if (op == 0x66)
    {
      data16 = 1;
      op = read_memory_unsigned_integer (pc + 1, 1, byte_order);
    }

  switch (op)
    {
    case 0xe9:
      /* Relative jump: if data16 == 0, disp32, else disp16.  */
      if (data16)
	{
	  delta = read_memory_integer (pc + 2, 2, byte_order);

	  /* Include the size of the jmp instruction (including the
             0x66 prefix).  */
	  delta += 4;
	}
      else
	{
	  delta = read_memory_integer (pc + 1, 4, byte_order);

	  /* Include the size of the jmp instruction.  */
	  delta += 5;
	}
      break;
    case 0xeb:
      /* Relative jump, disp8 (ignore data16).  */
      delta = read_memory_integer (pc + data16 + 1, 1, byte_order);

      delta += data16 + 2;
      break;
    }

  return pc + delta;
}

/* Check whether PC points at a prologue for a function returning a
   structure or union.  If so, it updates CACHE and returns the
   address of the first instruction after the code sequence that
   removes the "hidden" argument from the stack or CURRENT_PC,
   whichever is smaller.  Otherwise, return PC.  */

static CORE_ADDR
i386_analyze_struct_return (CORE_ADDR pc, CORE_ADDR current_pc,
			    struct i386_frame_cache *cache)
{
  /* Functions that return a structure or union start with:

        popl %eax             0x58
        xchgl %eax, (%esp)    0x87 0x04 0x24
     or xchgl %eax, 0(%esp)   0x87 0x44 0x24 0x00

     (the System V compiler puts out the second `xchg' instruction,
     and the assembler doesn't try to optimize it, so the 'sib' form
     gets generated).  This sequence is used to get the address of the
     return buffer for a function that returns a structure.  */
  static gdb_byte proto1[3] = { 0x87, 0x04, 0x24 };
  static gdb_byte proto2[4] = { 0x87, 0x44, 0x24, 0x00 };
  gdb_byte buf[4];
  gdb_byte op;

  if (current_pc <= pc)
    return pc;

  if (target_read_memory (pc, &op, 1))
    return pc;

  if (op != 0x58)		/* popl %eax */
    return pc;

  if (target_read_memory (pc + 1, buf, 4))
    return pc;

  if (memcmp (buf, proto1, 3) != 0 && memcmp (buf, proto2, 4) != 0)
    return pc;

  if (current_pc == pc)
    {
      cache->sp_offset += 4;
      return current_pc;
    }

  if (current_pc == pc + 1)
    {
      cache->pc_in_eax = 1;
      return current_pc;
    }
  
  if (buf[1] == proto1[1])
    return pc + 4;
  else
    return pc + 5;
}

static CORE_ADDR
i386_skip_probe (CORE_ADDR pc)
{
  /* A function may start with

        pushl constant
        call _probe
	addl $4, %esp
	   
     followed by

        pushl %ebp

     etc.  */
  gdb_byte buf[8];
  gdb_byte op;

  if (target_read_memory (pc, &op, 1))
    return pc;

  if (op == 0x68 || op == 0x6a)
    {
      int delta;

      /* Skip past the `pushl' instruction; it has either a one-byte or a
	 four-byte operand, depending on the opcode.  */
      if (op == 0x68)
	delta = 5;
      else
	delta = 2;

      /* Read the following 8 bytes, which should be `call _probe' (6
	 bytes) followed by `addl $4,%esp' (2 bytes).  */
      read_memory (pc + delta, buf, sizeof (buf));
      if (buf[0] == 0xe8 && buf[6] == 0xc4 && buf[7] == 0x4)
	pc += delta + sizeof (buf);
    }

  return pc;
}

/* GCC 4.1 and later, can put code in the prologue to realign the
   stack pointer.  Check whether PC points to such code, and update
   CACHE accordingly.  Return the first instruction after the code
   sequence or CURRENT_PC, whichever is smaller.  If we don't
   recognize the code, return PC.  */

static CORE_ADDR
i386_analyze_stack_align (CORE_ADDR pc, CORE_ADDR current_pc,
			  struct i386_frame_cache *cache)
{
  /* There are 2 code sequences to re-align stack before the frame
     gets set up:

	1. Use a caller-saved saved register:

		leal  4(%esp), %reg
		andl  $-XXX, %esp
		pushl -4(%reg)

	2. Use a callee-saved saved register:

		pushl %reg
		leal  8(%esp), %reg
		andl  $-XXX, %esp
		pushl -4(%reg)

     "andl $-XXX, %esp" can be either 3 bytes or 6 bytes:
     
     	0x83 0xe4 0xf0			andl $-16, %esp
     	0x81 0xe4 0x00 0xff 0xff 0xff	andl $-256, %esp
   */

  gdb_byte buf[14];
  int reg;
  int offset, offset_and;
  static int regnums[8] = {
    I386_EAX_REGNUM,		/* %eax */
    I386_ECX_REGNUM,		/* %ecx */
    I386_EDX_REGNUM,		/* %edx */
    I386_EBX_REGNUM,		/* %ebx */
    I386_ESP_REGNUM,		/* %esp */
    I386_EBP_REGNUM,		/* %ebp */
    I386_ESI_REGNUM,		/* %esi */
    I386_EDI_REGNUM		/* %edi */
  };

  if (target_read_memory (pc, buf, sizeof buf))
    return pc;

  /* Check caller-saved saved register.  The first instruction has
     to be "leal 4(%esp), %reg".  */
  if (buf[0] == 0x8d && buf[2] == 0x24 && buf[3] == 0x4)
    {
      /* MOD must be binary 10 and R/M must be binary 100.  */
      if ((buf[1] & 0xc7) != 0x44)
	return pc;

      /* REG has register number.  */
      reg = (buf[1] >> 3) & 7;
      offset = 4;
    }
  else
    {
      /* Check callee-saved saved register.  The first instruction
	 has to be "pushl %reg".  */
      if ((buf[0] & 0xf8) != 0x50)
	return pc;

      /* Get register.  */
      reg = buf[0] & 0x7;

      /* The next instruction has to be "leal 8(%esp), %reg".  */
      if (buf[1] != 0x8d || buf[3] != 0x24 || buf[4] != 0x8)
	return pc;

      /* MOD must be binary 10 and R/M must be binary 100.  */
      if ((buf[2] & 0xc7) != 0x44)
	return pc;
      
      /* REG has register number.  Registers in pushl and leal have to
	 be the same.  */
      if (reg != ((buf[2] >> 3) & 7))
	return pc;

      offset = 5;
    }

  /* Rigister can't be %esp nor %ebp.  */
  if (reg == 4 || reg == 5)
    return pc;

  /* The next instruction has to be "andl $-XXX, %esp".  */
  if (buf[offset + 1] != 0xe4
      || (buf[offset] != 0x81 && buf[offset] != 0x83))
    return pc;

  offset_and = offset;
  offset += buf[offset] == 0x81 ? 6 : 3;

  /* The next instruction has to be "pushl -4(%reg)".  8bit -4 is
     0xfc.  REG must be binary 110 and MOD must be binary 01.  */
  if (buf[offset] != 0xff
      || buf[offset + 2] != 0xfc
      || (buf[offset + 1] & 0xf8) != 0x70)
    return pc;

  /* R/M has register.  Registers in leal and pushl have to be the
     same.  */
  if (reg != (buf[offset + 1] & 7))
    return pc;

  if (current_pc > pc + offset_and)
    cache->saved_sp_reg = regnums[reg];

  return min (pc + offset + 3, current_pc);
}

/* Maximum instruction length we need to handle.  */
#define I386_MAX_MATCHED_INSN_LEN	6

/* Instruction description.  */
struct i386_insn
{
  size_t len;
  gdb_byte insn[I386_MAX_MATCHED_INSN_LEN];
  gdb_byte mask[I386_MAX_MATCHED_INSN_LEN];
};

/* Return whether instruction at PC matches PATTERN.  */

static int
i386_match_pattern (CORE_ADDR pc, struct i386_insn pattern)
{
  gdb_byte op;

  if (target_read_memory (pc, &op, 1))
    return 0;

  if ((op & pattern.mask[0]) == pattern.insn[0])
    {
      gdb_byte buf[I386_MAX_MATCHED_INSN_LEN - 1];
      int insn_matched = 1;
      size_t i;

      gdb_assert (pattern.len > 1);
      gdb_assert (pattern.len <= I386_MAX_MATCHED_INSN_LEN);

      if (target_read_memory (pc + 1, buf, pattern.len - 1))
	return 0;

      for (i = 1; i < pattern.len; i++)
	{
	  if ((buf[i - 1] & pattern.mask[i]) != pattern.insn[i])
	    insn_matched = 0;
	}
      return insn_matched;
    }
  return 0;
}

/* Search for the instruction at PC in the list INSN_PATTERNS.  Return
   the first instruction description that matches.  Otherwise, return
   NULL.  */

static struct i386_insn *
i386_match_insn (CORE_ADDR pc, struct i386_insn *insn_patterns)
{
  struct i386_insn *pattern;

  for (pattern = insn_patterns; pattern->len > 0; pattern++)
    {
      if (i386_match_pattern (pc, *pattern))
	return pattern;
    }

  return NULL;
}

/* Return whether PC points inside a sequence of instructions that
   matches INSN_PATTERNS.  */

static int
i386_match_insn_block (CORE_ADDR pc, struct i386_insn *insn_patterns)
{
  CORE_ADDR current_pc;
  int ix, i;
  struct i386_insn *insn;

  insn = i386_match_insn (pc, insn_patterns);
  if (insn == NULL)
    return 0;

  current_pc = pc;
  ix = insn - insn_patterns;
  for (i = ix - 1; i >= 0; i--)
    {
      current_pc -= insn_patterns[i].len;

      if (!i386_match_pattern (current_pc, insn_patterns[i]))
	return 0;
    }

  current_pc = pc + insn->len;
  for (insn = insn_patterns + ix + 1; insn->len > 0; insn++)
    {
      if (!i386_match_pattern (current_pc, *insn))
	return 0;

      current_pc += insn->len;
    }

  return 1;
}

/* Some special instructions that might be migrated by GCC into the
   part of the prologue that sets up the new stack frame.  Because the
   stack frame hasn't been setup yet, no registers have been saved
   yet, and only the scratch registers %eax, %ecx and %edx can be
   touched.  */

struct i386_insn i386_frame_setup_skip_insns[] =
{
  /* Check for `movb imm8, r' and `movl imm32, r'.
    
     ??? Should we handle 16-bit operand-sizes here?  */

  /* `movb imm8, %al' and `movb imm8, %ah' */
  /* `movb imm8, %cl' and `movb imm8, %ch' */
  { 2, { 0xb0, 0x00 }, { 0xfa, 0x00 } },
  /* `movb imm8, %dl' and `movb imm8, %dh' */
  { 2, { 0xb2, 0x00 }, { 0xfb, 0x00 } },
  /* `movl imm32, %eax' and `movl imm32, %ecx' */
  { 5, { 0xb8 }, { 0xfe } },
  /* `movl imm32, %edx' */
  { 5, { 0xba }, { 0xff } },

  /* Check for `mov imm32, r32'.  Note that there is an alternative
     encoding for `mov m32, %eax'.

     ??? Should we handle SIB adressing here?
     ??? Should we handle 16-bit operand-sizes here?  */

  /* `movl m32, %eax' */
  { 5, { 0xa1 }, { 0xff } },
  /* `movl m32, %eax' and `mov; m32, %ecx' */
  { 6, { 0x89, 0x05 }, {0xff, 0xf7 } },
  /* `movl m32, %edx' */
  { 6, { 0x89, 0x15 }, {0xff, 0xff } },

  /* Check for `xorl r32, r32' and the equivalent `subl r32, r32'.
     Because of the symmetry, there are actually two ways to encode
     these instructions; opcode bytes 0x29 and 0x2b for `subl' and
     opcode bytes 0x31 and 0x33 for `xorl'.  */

  /* `subl %eax, %eax' */
  { 2, { 0x29, 0xc0 }, { 0xfd, 0xff } },
  /* `subl %ecx, %ecx' */
  { 2, { 0x29, 0xc9 }, { 0xfd, 0xff } },
  /* `subl %edx, %edx' */
  { 2, { 0x29, 0xd2 }, { 0xfd, 0xff } },
  /* `xorl %eax, %eax' */
  { 2, { 0x31, 0xc0 }, { 0xfd, 0xff } },
  /* `xorl %ecx, %ecx' */
  { 2, { 0x31, 0xc9 }, { 0xfd, 0xff } },
  /* `xorl %edx, %edx' */
  { 2, { 0x31, 0xd2 }, { 0xfd, 0xff } },
  { 0 }
};


/* Check whether PC points to a no-op instruction.  */
static CORE_ADDR
i386_skip_noop (CORE_ADDR pc)
{
  gdb_byte op;
  int check = 1;

  if (target_read_memory (pc, &op, 1))
    return pc;

  while (check) 
    {
      check = 0;
      /* Ignore `nop' instruction.  */
      if (op == 0x90) 
	{
	  pc += 1;
	  if (target_read_memory (pc, &op, 1))
	    return pc;
	  check = 1;
	}
      /* Ignore no-op instruction `mov %edi, %edi'.
	 Microsoft system dlls often start with
	 a `mov %edi,%edi' instruction.
	 The 5 bytes before the function start are
	 filled with `nop' instructions.
	 This pattern can be used for hot-patching:
	 The `mov %edi, %edi' instruction can be replaced by a
	 near jump to the location of the 5 `nop' instructions
	 which can be replaced by a 32-bit jump to anywhere
	 in the 32-bit address space.  */

      else if (op == 0x8b)
	{
	  if (target_read_memory (pc + 1, &op, 1))
	    return pc;

	  if (op == 0xff)
	    {
	      pc += 2;
	      if (target_read_memory (pc, &op, 1))
		return pc;

	      check = 1;
	    }
	}
    }
  return pc; 
}

/* Check whether PC points at a code that sets up a new stack frame.
   If so, it updates CACHE and returns the address of the first
   instruction after the sequence that sets up the frame or LIMIT,
   whichever is smaller.  If we don't recognize the code, return PC.  */

static CORE_ADDR
i386_analyze_frame_setup (struct gdbarch *gdbarch,
			  CORE_ADDR pc, CORE_ADDR limit,
			  struct i386_frame_cache *cache)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct i386_insn *insn;
  gdb_byte op;
  int skip = 0;

  if (limit <= pc)
    return limit;

  if (target_read_memory (pc, &op, 1))
    return pc;

  if (op == 0x55)		/* pushl %ebp */
    {
      /* Take into account that we've executed the `pushl %ebp' that
	 starts this instruction sequence.  */
      cache->saved_regs[I386_EBP_REGNUM] = 0;
      cache->sp_offset += 4;
      pc++;

      /* If that's all, return now.  */
      if (limit <= pc)
	return limit;

      /* Check for some special instructions that might be migrated by
	 GCC into the prologue and skip them.  At this point in the
	 prologue, code should only touch the scratch registers %eax,
	 %ecx and %edx, so while the number of posibilities is sheer,
	 it is limited.

	 Make sure we only skip these instructions if we later see the
	 `movl %esp, %ebp' that actually sets up the frame.  */
      while (pc + skip < limit)
	{
	  insn = i386_match_insn (pc + skip, i386_frame_setup_skip_insns);
	  if (insn == NULL)
	    break;

	  skip += insn->len;
	}

      /* If that's all, return now.  */
      if (limit <= pc + skip)
	return limit;

      if (target_read_memory (pc + skip, &op, 1))
	return pc + skip;

      /* The i386 prologue looks like

	 push   %ebp
	 mov    %esp,%ebp
	 sub    $0x10,%esp

	 and a different prologue can be generated for atom.

	 push   %ebp
	 lea    (%esp),%ebp
	 lea    -0x10(%esp),%esp

	 We handle both of them here.  */

      switch (op)
	{
	  /* Check for `movl %esp, %ebp' -- can be written in two ways.  */
	case 0x8b:
	  if (read_memory_unsigned_integer (pc + skip + 1, 1, byte_order)
	      != 0xec)
	    return pc;
	  pc += (skip + 2);
	  break;
	case 0x89:
	  if (read_memory_unsigned_integer (pc + skip + 1, 1, byte_order)
	      != 0xe5)
	    return pc;
	  pc += (skip + 2);
	  break;
	case 0x8d: /* Check for 'lea (%ebp), %ebp'.  */
	  if (read_memory_unsigned_integer (pc + skip + 1, 2, byte_order)
	      != 0x242c)
	    return pc;
	  pc += (skip + 3);
	  break;
	default:
	  return pc;
	}

      /* OK, we actually have a frame.  We just don't know how large
	 it is yet.  Set its size to zero.  We'll adjust it if
	 necessary.  We also now commit to skipping the special
	 instructions mentioned before.  */
      cache->locals = 0;

      /* If that's all, return now.  */
      if (limit <= pc)
	return limit;

      /* Check for stack adjustment 

	    subl $XXX, %esp
	 or
	    lea -XXX(%esp),%esp

	 NOTE: You can't subtract a 16-bit immediate from a 32-bit
	 reg, so we don't have to worry about a data16 prefix.  */
      if (target_read_memory (pc, &op, 1))
	return pc;
      if (op == 0x83)
	{
	  /* `subl' with 8-bit immediate.  */
	  if (read_memory_unsigned_integer (pc + 1, 1, byte_order) != 0xec)
	    /* Some instruction starting with 0x83 other than `subl'.  */
	    return pc;

	  /* `subl' with signed 8-bit immediate (though it wouldn't
	     make sense to be negative).  */
	  cache->locals = read_memory_integer (pc + 2, 1, byte_order);
	  return pc + 3;
	}
      else if (op == 0x81)
	{
	  /* Maybe it is `subl' with a 32-bit immediate.  */
	  if (read_memory_unsigned_integer (pc + 1, 1, byte_order) != 0xec)
	    /* Some instruction starting with 0x81 other than `subl'.  */
	    return pc;

	  /* It is `subl' with a 32-bit immediate.  */
	  cache->locals = read_memory_integer (pc + 2, 4, byte_order);
	  return pc + 6;
	}
      else if (op == 0x8d)
	{
	  /* The ModR/M byte is 0x64.  */
	  if (read_memory_unsigned_integer (pc + 1, 1, byte_order) != 0x64)
	    return pc;
	  /* 'lea' with 8-bit displacement.  */
	  cache->locals = -1 * read_memory_integer (pc + 3, 1, byte_order);
	  return pc + 4;
	}
      else
	{
	  /* Some instruction other than `subl' nor 'lea'.  */
	  return pc;
	}
    }
  else if (op == 0xc8)		/* enter */
    {
      cache->locals = read_memory_unsigned_integer (pc + 1, 2, byte_order);
      return pc + 4;
    }

  return pc;
}

/* Check whether PC points at code that saves registers on the stack.
   If so, it updates CACHE and returns the address of the first
   instruction after the register saves or CURRENT_PC, whichever is
   smaller.  Otherwise, return PC.  */

static CORE_ADDR
i386_analyze_register_saves (CORE_ADDR pc, CORE_ADDR current_pc,
			     struct i386_frame_cache *cache)
{
  CORE_ADDR offset = 0;
  gdb_byte op;
  int i;

  if (cache->locals > 0)
    offset -= cache->locals;
  for (i = 0; i < 8 && pc < current_pc; i++)
    {
      if (target_read_memory (pc, &op, 1))
	return pc;
      if (op < 0x50 || op > 0x57)
	break;

      offset -= 4;
      cache->saved_regs[op - 0x50] = offset;
      cache->sp_offset += 4;
      pc++;
    }

  return pc;
}

/* Do a full analysis of the prologue at PC and update CACHE
   accordingly.  Bail out early if CURRENT_PC is reached.  Return the
   address where the analysis stopped.

   We handle these cases:

   The startup sequence can be at the start of the function, or the
   function can start with a branch to startup code at the end.

   %ebp can be set up with either the 'enter' instruction, or "pushl
   %ebp, movl %esp, %ebp" (`enter' is too slow to be useful, but was
   once used in the System V compiler).

   Local space is allocated just below the saved %ebp by either the
   'enter' instruction, or by "subl $<size>, %esp".  'enter' has a
   16-bit unsigned argument for space to allocate, and the 'addl'
   instruction could have either a signed byte, or 32-bit immediate.

   Next, the registers used by this function are pushed.  With the
   System V compiler they will always be in the order: %edi, %esi,
   %ebx (and sometimes a harmless bug causes it to also save but not
   restore %eax); however, the code below is willing to see the pushes
   in any order, and will handle up to 8 of them.
 
   If the setup sequence is at the end of the function, then the next
   instruction will be a branch back to the start.  */

static CORE_ADDR
i386_analyze_prologue (struct gdbarch *gdbarch,
		       CORE_ADDR pc, CORE_ADDR current_pc,
		       struct i386_frame_cache *cache)
{
  pc = i386_skip_noop (pc);
  pc = i386_follow_jump (gdbarch, pc);
  pc = i386_analyze_struct_return (pc, current_pc, cache);
  pc = i386_skip_probe (pc);
  pc = i386_analyze_stack_align (pc, current_pc, cache);
  pc = i386_analyze_frame_setup (gdbarch, pc, current_pc, cache);
  return i386_analyze_register_saves (pc, current_pc, cache);
}

/* Return PC of first real instruction.  */

static CORE_ADDR
i386_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  static gdb_byte pic_pat[6] =
  {
    0xe8, 0, 0, 0, 0,		/* call 0x0 */
    0x5b,			/* popl %ebx */
  };
  struct i386_frame_cache cache;
  CORE_ADDR pc;
  gdb_byte op;
  int i;
  CORE_ADDR func_addr;

  if (find_pc_partial_function (start_pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      struct symtab *s = find_pc_symtab (func_addr);

      /* Clang always emits a line note before the prologue and another
	 one after.  We trust clang to emit usable line notes.  */
      if (post_prologue_pc
	  && (s != NULL
	      && s->producer != NULL
	      && strncmp (s->producer, "clang ", sizeof ("clang ") - 1) == 0))
        return max (start_pc, post_prologue_pc);
    }
 
  cache.locals = -1;
  pc = i386_analyze_prologue (gdbarch, start_pc, 0xffffffff, &cache);
  if (cache.locals < 0)
    return start_pc;

  /* Found valid frame setup.  */

  /* The native cc on SVR4 in -K PIC mode inserts the following code
     to get the address of the global offset table (GOT) into register
     %ebx:

        call	0x0
	popl    %ebx
        movl    %ebx,x(%ebp)    (optional)
        addl    y,%ebx

     This code is with the rest of the prologue (at the end of the
     function), so we have to skip it to get to the first real
     instruction at the start of the function.  */

  for (i = 0; i < 6; i++)
    {
      if (target_read_memory (pc + i, &op, 1))
	return pc;

      if (pic_pat[i] != op)
	break;
    }
  if (i == 6)
    {
      int delta = 6;

      if (target_read_memory (pc + delta, &op, 1))
	return pc;

      if (op == 0x89)		/* movl %ebx, x(%ebp) */
	{
	  op = read_memory_unsigned_integer (pc + delta + 1, 1, byte_order);

	  if (op == 0x5d)	/* One byte offset from %ebp.  */
	    delta += 3;
	  else if (op == 0x9d)	/* Four byte offset from %ebp.  */
	    delta += 6;
	  else			/* Unexpected instruction.  */
	    delta = 0;

          if (target_read_memory (pc + delta, &op, 1))
	    return pc;
	}

      /* addl y,%ebx */
      if (delta > 0 && op == 0x81
	  && read_memory_unsigned_integer (pc + delta + 1, 1, byte_order)
	     == 0xc3)
	{
	  pc += delta + 6;
	}
    }

  /* If the function starts with a branch (to startup code at the end)
     the last instruction should bring us back to the first
     instruction of the real code.  */
  if (i386_follow_jump (gdbarch, start_pc) != start_pc)
    pc = i386_follow_jump (gdbarch, pc);

  return pc;
}

/* Check that the code pointed to by PC corresponds to a call to
   __main, skip it if so.  Return PC otherwise.  */

CORE_ADDR
i386_skip_main_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte op;

  if (target_read_memory (pc, &op, 1))
    return pc;
  if (op == 0xe8)
    {
      gdb_byte buf[4];

      if (target_read_memory (pc + 1, buf, sizeof buf) == 0)
 	{
	  /* Make sure address is computed correctly as a 32bit
	     integer even if CORE_ADDR is 64 bit wide.  */
 	  struct minimal_symbol *s;
 	  CORE_ADDR call_dest;

	  call_dest = pc + 5 + extract_signed_integer (buf, 4, byte_order);
	  call_dest = call_dest & 0xffffffffU;
 	  s = lookup_minimal_symbol_by_pc (call_dest);
 	  if (s != NULL
 	      && SYMBOL_LINKAGE_NAME (s) != NULL
 	      && strcmp (SYMBOL_LINKAGE_NAME (s), "__main") == 0)
 	    pc += 5;
 	}
    }

  return pc;
}

/* This function is 64-bit safe.  */

static CORE_ADDR
i386_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  gdb_byte buf[8];

  frame_unwind_register (next_frame, gdbarch_pc_regnum (gdbarch), buf);
  return extract_typed_address (buf, builtin_type (gdbarch)->builtin_func_ptr);
}


/* Normal frames.  */

static void
i386_frame_cache_1 (struct frame_info *this_frame,
		    struct i386_frame_cache *cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  int i;

  cache->pc = get_frame_func (this_frame);

  /* In principle, for normal frames, %ebp holds the frame pointer,
     which holds the base address for the current stack frame.
     However, for functions that don't need it, the frame pointer is
     optional.  For these "frameless" functions the frame pointer is
     actually the frame pointer of the calling frame.  Signal
     trampolines are just a special case of a "frameless" function.
     They (usually) share their frame pointer with the frame that was
     in progress when the signal occurred.  */

  get_frame_register (this_frame, I386_EBP_REGNUM, buf);
  cache->base = extract_unsigned_integer (buf, 4, byte_order);
  if (cache->base == 0)
    {
      cache->base_p = 1;
      return;
    }

  /* For normal frames, %eip is stored at 4(%ebp).  */
  cache->saved_regs[I386_EIP_REGNUM] = 4;

  if (cache->pc != 0)
    i386_analyze_prologue (gdbarch, cache->pc, get_frame_pc (this_frame),
			   cache);

  if (cache->locals < 0)
    {
      /* We didn't find a valid frame, which means that CACHE->base
	 currently holds the frame pointer for our calling frame.  If
	 we're at the start of a function, or somewhere half-way its
	 prologue, the function's frame probably hasn't been fully
	 setup yet.  Try to reconstruct the base address for the stack
	 frame by looking at the stack pointer.  For truly "frameless"
	 functions this might work too.  */

      if (cache->saved_sp_reg != -1)
	{
	  /* Saved stack pointer has been saved.  */
	  get_frame_register (this_frame, cache->saved_sp_reg, buf);
	  cache->saved_sp = extract_unsigned_integer (buf, 4, byte_order);

	  /* We're halfway aligning the stack.  */
	  cache->base = ((cache->saved_sp - 4) & 0xfffffff0) - 4;
	  cache->saved_regs[I386_EIP_REGNUM] = cache->saved_sp - 4;

	  /* This will be added back below.  */
	  cache->saved_regs[I386_EIP_REGNUM] -= cache->base;
	}
      else if (cache->pc != 0
	       || target_read_memory (get_frame_pc (this_frame), buf, 1))
	{
	  /* We're in a known function, but did not find a frame
	     setup.  Assume that the function does not use %ebp.
	     Alternatively, we may have jumped to an invalid
	     address; in that case there is definitely no new
	     frame in %ebp.  */
	  get_frame_register (this_frame, I386_ESP_REGNUM, buf);
	  cache->base = extract_unsigned_integer (buf, 4, byte_order)
			+ cache->sp_offset;
	}
      else
	/* We're in an unknown function.  We could not find the start
	   of the function to analyze the prologue; our best option is
	   to assume a typical frame layout with the caller's %ebp
	   saved.  */
	cache->saved_regs[I386_EBP_REGNUM] = 0;
    }

  if (cache->saved_sp_reg != -1)
    {
      /* Saved stack pointer has been saved (but the SAVED_SP_REG
	 register may be unavailable).  */
      if (cache->saved_sp == 0
	  && deprecated_frame_register_read (this_frame,
					     cache->saved_sp_reg, buf))
	cache->saved_sp = extract_unsigned_integer (buf, 4, byte_order);
    }
  /* Now that we have the base address for the stack frame we can
     calculate the value of %esp in the calling frame.  */
  else if (cache->saved_sp == 0)
    cache->saved_sp = cache->base + 8;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < I386_NUM_SAVED_REGS; i++)
    if (cache->saved_regs[i] != -1)
      cache->saved_regs[i] += cache->base;

  cache->base_p = 1;
}

static struct i386_frame_cache *
i386_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  volatile struct gdb_exception ex;
  struct i386_frame_cache *cache;

  if (*this_cache)
    return *this_cache;

  cache = i386_alloc_frame_cache ();
  *this_cache = cache;

  TRY_CATCH (ex, RETURN_MASK_ERROR)
    {
      i386_frame_cache_1 (this_frame, cache);
    }
  if (ex.reason < 0 && ex.error != NOT_AVAILABLE_ERROR)
    throw_exception (ex);

  return cache;
}

static void
i386_frame_this_id (struct frame_info *this_frame, void **this_cache,
		    struct frame_id *this_id)
{
  struct i386_frame_cache *cache = i386_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  /* See the end of i386_push_dummy_call.  */
  (*this_id) = frame_id_build (cache->base + 8, cache->pc);
}

static enum unwind_stop_reason
i386_frame_unwind_stop_reason (struct frame_info *this_frame,
			       void **this_cache)
{
  struct i386_frame_cache *cache = i386_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    return UNWIND_UNAVAILABLE;

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return UNWIND_OUTERMOST;

  return UNWIND_NO_REASON;
}

static struct value *
i386_frame_prev_register (struct frame_info *this_frame, void **this_cache,
			  int regnum)
{
  struct i386_frame_cache *cache = i386_frame_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0);

  /* The System V ABI says that:

     "The flags register contains the system flags, such as the
     direction flag and the carry flag.  The direction flag must be
     set to the forward (that is, zero) direction before entry and
     upon exit from a function.  Other user flags have no specified
     role in the standard calling sequence and are not preserved."

     To guarantee the "upon exit" part of that statement we fake a
     saved flags register that has its direction flag cleared.

     Note that GCC doesn't seem to rely on the fact that the direction
     flag is cleared after a function return; it always explicitly
     clears the flag before operations where it matters.

     FIXME: kettenis/20030316: I'm not quite sure whether this is the
     right thing to do.  The way we fake the flags register here makes
     it impossible to change it.  */

  if (regnum == I386_EFLAGS_REGNUM)
    {
      ULONGEST val;

      val = get_frame_register_unsigned (this_frame, regnum);
      val &= ~(1 << 10);
      return frame_unwind_got_constant (this_frame, regnum, val);
    }

  if (regnum == I386_EIP_REGNUM && cache->pc_in_eax)
    return frame_unwind_got_register (this_frame, regnum, I386_EAX_REGNUM);

  if (regnum == I386_ESP_REGNUM
      && (cache->saved_sp != 0 || cache->saved_sp_reg != -1))
    {
      /* If the SP has been saved, but we don't know where, then this
	 means that SAVED_SP_REG register was found unavailable back
	 when we built the cache.  */
      if (cache->saved_sp == 0)
	return frame_unwind_got_register (this_frame, regnum,
					  cache->saved_sp_reg);
      else
	return frame_unwind_got_constant (this_frame, regnum,
					  cache->saved_sp);
    }

  if (regnum < I386_NUM_SAVED_REGS && cache->saved_regs[regnum] != -1)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind i386_frame_unwind =
{
  NORMAL_FRAME,
  i386_frame_unwind_stop_reason,
  i386_frame_this_id,
  i386_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Normal frames, but in a function epilogue.  */

/* The epilogue is defined here as the 'ret' instruction, which will
   follow any instruction such as 'leave' or 'pop %ebp' that destroys
   the function's stack frame.  */

static int
i386_in_function_epilogue_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  gdb_byte insn;
  struct symtab *symtab;

  symtab = find_pc_symtab (pc);
  if (symtab && symtab->epilogue_unwind_valid)
    return 0;

  if (target_read_memory (pc, &insn, 1))
    return 0;	/* Can't read memory at pc.  */

  if (insn != 0xc3)	/* 'ret' instruction.  */
    return 0;

  return 1;
}

static int
i386_epilogue_frame_sniffer (const struct frame_unwind *self,
			     struct frame_info *this_frame,
			     void **this_prologue_cache)
{
  if (frame_relative_level (this_frame) == 0)
    return i386_in_function_epilogue_p (get_frame_arch (this_frame),
					get_frame_pc (this_frame));
  else
    return 0;
}

static struct i386_frame_cache *
i386_epilogue_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  volatile struct gdb_exception ex;
  struct i386_frame_cache *cache;
  CORE_ADDR sp;

  if (*this_cache)
    return *this_cache;

  cache = i386_alloc_frame_cache ();
  *this_cache = cache;

  TRY_CATCH (ex, RETURN_MASK_ERROR)
    {
      cache->pc = get_frame_func (this_frame);

      /* At this point the stack looks as if we just entered the
	 function, with the return address at the top of the
	 stack.  */
      sp = get_frame_register_unsigned (this_frame, I386_ESP_REGNUM);
      cache->base = sp + cache->sp_offset;
      cache->saved_sp = cache->base + 8;
      cache->saved_regs[I386_EIP_REGNUM] = cache->base + 4;

      cache->base_p = 1;
    }
  if (ex.reason < 0 && ex.error != NOT_AVAILABLE_ERROR)
    throw_exception (ex);

  return cache;
}

static enum unwind_stop_reason
i386_epilogue_frame_unwind_stop_reason (struct frame_info *this_frame,
					void **this_cache)
{
  struct i386_frame_cache *cache =
    i386_epilogue_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    return UNWIND_UNAVAILABLE;

  return UNWIND_NO_REASON;
}

static void
i386_epilogue_frame_this_id (struct frame_info *this_frame,
			     void **this_cache,
			     struct frame_id *this_id)
{
  struct i386_frame_cache *cache =
    i386_epilogue_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    return;

  (*this_id) = frame_id_build (cache->base + 8, cache->pc);
}

static struct value *
i386_epilogue_frame_prev_register (struct frame_info *this_frame,
				   void **this_cache, int regnum)
{
  /* Make sure we've initialized the cache.  */
  i386_epilogue_frame_cache (this_frame, this_cache);

  return i386_frame_prev_register (this_frame, this_cache, regnum);
}

static const struct frame_unwind i386_epilogue_frame_unwind =
{
  NORMAL_FRAME,
  i386_epilogue_frame_unwind_stop_reason,
  i386_epilogue_frame_this_id,
  i386_epilogue_frame_prev_register,
  NULL, 
  i386_epilogue_frame_sniffer
};


/* Stack-based trampolines.  */

/* These trampolines are used on cross x86 targets, when taking the
   address of a nested function.  When executing these trampolines,
   no stack frame is set up, so we are in a similar situation as in
   epilogues and i386_epilogue_frame_this_id can be re-used.  */

/* Static chain passed in register.  */

struct i386_insn i386_tramp_chain_in_reg_insns[] =
{
  /* `movl imm32, %eax' and `movl imm32, %ecx' */
  { 5, { 0xb8 }, { 0xfe } },

  /* `jmp imm32' */
  { 5, { 0xe9 }, { 0xff } },

  {0}
};

/* Static chain passed on stack (when regparm=3).  */

struct i386_insn i386_tramp_chain_on_stack_insns[] =
{
  /* `push imm32' */
  { 5, { 0x68 }, { 0xff } },

  /* `jmp imm32' */
  { 5, { 0xe9 }, { 0xff } },

  {0}
};

/* Return whether PC points inside a stack trampoline.   */

static int
i386_in_stack_tramp_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  gdb_byte insn;
  const char *name;

  /* A stack trampoline is detected if no name is associated
    to the current pc and if it points inside a trampoline
    sequence.  */

  find_pc_partial_function (pc, &name, NULL, NULL);
  if (name)
    return 0;

  if (target_read_memory (pc, &insn, 1))
    return 0;

  if (!i386_match_insn_block (pc, i386_tramp_chain_in_reg_insns)
      && !i386_match_insn_block (pc, i386_tramp_chain_on_stack_insns))
    return 0;

  return 1;
}

static int
i386_stack_tramp_frame_sniffer (const struct frame_unwind *self,
				struct frame_info *this_frame,
				void **this_cache)
{
  if (frame_relative_level (this_frame) == 0)
    return i386_in_stack_tramp_p (get_frame_arch (this_frame),
				  get_frame_pc (this_frame));
  else
    return 0;
}

static const struct frame_unwind i386_stack_tramp_frame_unwind =
{
  NORMAL_FRAME,
  i386_epilogue_frame_unwind_stop_reason,
  i386_epilogue_frame_this_id,
  i386_epilogue_frame_prev_register,
  NULL, 
  i386_stack_tramp_frame_sniffer
};

/* Generate a bytecode expression to get the value of the saved PC.  */

static void
i386_gen_return_address (struct gdbarch *gdbarch,
			 struct agent_expr *ax, struct axs_value *value,
			 CORE_ADDR scope)
{
  /* The following sequence assumes the traditional use of the base
     register.  */
  ax_reg (ax, I386_EBP_REGNUM);
  ax_const_l (ax, 4);
  ax_simple (ax, aop_add);
  value->type = register_type (gdbarch, I386_EIP_REGNUM);
  value->kind = axs_lvalue_memory;
}


/* Signal trampolines.  */

static struct i386_frame_cache *
i386_sigtramp_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  volatile struct gdb_exception ex;
  struct i386_frame_cache *cache;
  CORE_ADDR addr;
  gdb_byte buf[4];

  if (*this_cache)
    return *this_cache;

  cache = i386_alloc_frame_cache ();

  TRY_CATCH (ex, RETURN_MASK_ERROR)
    {
      get_frame_register (this_frame, I386_ESP_REGNUM, buf);
      cache->base = extract_unsigned_integer (buf, 4, byte_order) - 4;

      addr = tdep->sigcontext_addr (this_frame);
      if (tdep->sc_reg_offset)
	{
	  int i;

	  gdb_assert (tdep->sc_num_regs <= I386_NUM_SAVED_REGS);

	  for (i = 0; i < tdep->sc_num_regs; i++)
	    if (tdep->sc_reg_offset[i] != -1)
	      cache->saved_regs[i] = addr + tdep->sc_reg_offset[i];
	}
      else
	{
	  cache->saved_regs[I386_EIP_REGNUM] = addr + tdep->sc_pc_offset;
	  cache->saved_regs[I386_ESP_REGNUM] = addr + tdep->sc_sp_offset;
	}

      cache->base_p = 1;
    }
  if (ex.reason < 0 && ex.error != NOT_AVAILABLE_ERROR)
    throw_exception (ex);

  *this_cache = cache;
  return cache;
}

static enum unwind_stop_reason
i386_sigtramp_frame_unwind_stop_reason (struct frame_info *this_frame,
					void **this_cache)
{
  struct i386_frame_cache *cache =
    i386_sigtramp_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    return UNWIND_UNAVAILABLE;

  return UNWIND_NO_REASON;
}

static void
i386_sigtramp_frame_this_id (struct frame_info *this_frame, void **this_cache,
			     struct frame_id *this_id)
{
  struct i386_frame_cache *cache =
    i386_sigtramp_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    return;

  /* See the end of i386_push_dummy_call.  */
  (*this_id) = frame_id_build (cache->base + 8, get_frame_pc (this_frame));
}

static struct value *
i386_sigtramp_frame_prev_register (struct frame_info *this_frame,
				   void **this_cache, int regnum)
{
  /* Make sure we've initialized the cache.  */
  i386_sigtramp_frame_cache (this_frame, this_cache);

  return i386_frame_prev_register (this_frame, this_cache, regnum);
}

static int
i386_sigtramp_frame_sniffer (const struct frame_unwind *self,
			     struct frame_info *this_frame,
			     void **this_prologue_cache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_frame_arch (this_frame));

  /* We shouldn't even bother if we don't have a sigcontext_addr
     handler.  */
  if (tdep->sigcontext_addr == NULL)
    return 0;

  if (tdep->sigtramp_p != NULL)
    {
      if (tdep->sigtramp_p (this_frame))
	return 1;
    }

  if (tdep->sigtramp_start != 0)
    {
      CORE_ADDR pc = get_frame_pc (this_frame);

      gdb_assert (tdep->sigtramp_end != 0);
      if (pc >= tdep->sigtramp_start && pc < tdep->sigtramp_end)
	return 1;
    }

  return 0;
}

static const struct frame_unwind i386_sigtramp_frame_unwind =
{
  SIGTRAMP_FRAME,
  i386_sigtramp_frame_unwind_stop_reason,
  i386_sigtramp_frame_this_id,
  i386_sigtramp_frame_prev_register,
  NULL,
  i386_sigtramp_frame_sniffer
};


static CORE_ADDR
i386_frame_base_address (struct frame_info *this_frame, void **this_cache)
{
  struct i386_frame_cache *cache = i386_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base i386_frame_base =
{
  &i386_frame_unwind,
  i386_frame_base_address,
  i386_frame_base_address,
  i386_frame_base_address
};

static struct frame_id
i386_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  CORE_ADDR fp;

  fp = get_frame_register_unsigned (this_frame, I386_EBP_REGNUM);

  /* See the end of i386_push_dummy_call.  */
  return frame_id_build (fp + 8, get_frame_pc (this_frame));
}

/* _Decimal128 function return values need 16-byte alignment on the
   stack.  */

static CORE_ADDR
i386_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  return sp & -(CORE_ADDR)16;
}


/* Figure out where the longjmp will land.  Slurp the args out of the
   stack.  We expect the first arg to be a pointer to the jmp_buf
   structure from which we extract the address that we will land at.
   This address is copied into PC.  This routine returns non-zero on
   success.  */

static int
i386_get_longjmp_target (struct frame_info *frame, CORE_ADDR *pc)
{
  gdb_byte buf[4];
  CORE_ADDR sp, jb_addr;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int jb_pc_offset = gdbarch_tdep (gdbarch)->jb_pc_offset;

  /* If JB_PC_OFFSET is -1, we have no way to find out where the
     longjmp will land.  */
  if (jb_pc_offset == -1)
    return 0;

  get_frame_register (frame, I386_ESP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 4, byte_order);
  if (target_read_memory (sp + 4, buf, 4))
    return 0;

  jb_addr = extract_unsigned_integer (buf, 4, byte_order);
  if (target_read_memory (jb_addr + jb_pc_offset, buf, 4))
    return 0;

  *pc = extract_unsigned_integer (buf, 4, byte_order);
  return 1;
}


/* Check whether TYPE must be 16-byte-aligned when passed as a
   function argument.  16-byte vectors, _Decimal128 and structures or
   unions containing such types must be 16-byte-aligned; other
   arguments are 4-byte-aligned.  */

static int
i386_16_byte_align_p (struct type *type)
{
  type = check_typedef (type);
  if ((TYPE_CODE (type) == TYPE_CODE_DECFLOAT
       || (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type)))
      && TYPE_LENGTH (type) == 16)
    return 1;
  if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
    return i386_16_byte_align_p (TYPE_TARGET_TYPE (type));
  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      || TYPE_CODE (type) == TYPE_CODE_UNION)
    {
      int i;
      for (i = 0; i < TYPE_NFIELDS (type); i++)
	{
	  if (i386_16_byte_align_p (TYPE_FIELD_TYPE (type, i)))
	    return 1;
	}
    }
  return 0;
}

/* Implementation for set_gdbarch_push_dummy_code.  */

static CORE_ADDR
i386_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp, CORE_ADDR funaddr,
		      struct value **args, int nargs, struct type *value_type,
		      CORE_ADDR *real_pc, CORE_ADDR *bp_addr,
		      struct regcache *regcache)
{
  /* Use 0xcc breakpoint - 1 byte.  */
  *bp_addr = sp - 1;
  *real_pc = funaddr;

  /* Keep the stack aligned.  */
  return sp - 16;
}

static CORE_ADDR
i386_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		      struct regcache *regcache, CORE_ADDR bp_addr, int nargs,
		      struct value **args, CORE_ADDR sp, int struct_return,
		      CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  int i;
  int write_pass;
  int args_space = 0;

  /* Determine the total space required for arguments and struct
     return address in a first pass (allowing for 16-byte-aligned
     arguments), then push arguments in a second pass.  */

  for (write_pass = 0; write_pass < 2; write_pass++)
    {
      int args_space_used = 0;

      if (struct_return)
	{
	  if (write_pass)
	    {
	      /* Push value address.  */
	      store_unsigned_integer (buf, 4, byte_order, struct_addr);
	      write_memory (sp, buf, 4);
	      args_space_used += 4;
	    }
	  else
	    args_space += 4;
	}

      for (i = 0; i < nargs; i++)
	{
	  int len = TYPE_LENGTH (value_enclosing_type (args[i]));

	  if (write_pass)
	    {
	      if (i386_16_byte_align_p (value_enclosing_type (args[i])))
		args_space_used = align_up (args_space_used, 16);

	      write_memory (sp + args_space_used,
			    value_contents_all (args[i]), len);
	      /* The System V ABI says that:

	      "An argument's size is increased, if necessary, to make it a
	      multiple of [32-bit] words.  This may require tail padding,
	      depending on the size of the argument."

	      This makes sure the stack stays word-aligned.  */
	      args_space_used += align_up (len, 4);
	    }
	  else
	    {
	      if (i386_16_byte_align_p (value_enclosing_type (args[i])))
		args_space = align_up (args_space, 16);
	      args_space += align_up (len, 4);
	    }
	}

      if (!write_pass)
	{
	  sp -= args_space;

	  /* The original System V ABI only requires word alignment,
	     but modern incarnations need 16-byte alignment in order
	     to support SSE.  Since wasting a few bytes here isn't
	     harmful we unconditionally enforce 16-byte alignment.  */
	  sp &= ~0xf;
	}
    }

  /* Store return address.  */
  sp -= 4;
  store_unsigned_integer (buf, 4, byte_order, bp_addr);
  write_memory (sp, buf, 4);

  /* Finally, update the stack pointer...  */
  store_unsigned_integer (buf, 4, byte_order, sp);
  regcache_cooked_write (regcache, I386_ESP_REGNUM, buf);

  /* ...and fake a frame pointer.  */
  regcache_cooked_write (regcache, I386_EBP_REGNUM, buf);

  /* MarkK wrote: This "+ 8" is all over the place:
     (i386_frame_this_id, i386_sigtramp_frame_this_id,
     i386_dummy_id).  It's there, since all frame unwinders for
     a given target have to agree (within a certain margin) on the
     definition of the stack address of a frame.  Otherwise frame id
     comparison might not work correctly.  Since DWARF2/GCC uses the
     stack address *before* the function call as a frame's CFA.  On
     the i386, when %ebp is used as a frame pointer, the offset
     between the contents %ebp and the CFA as defined by GCC.  */
  return sp + 8;
}

/* These registers are used for returning integers (and on some
   targets also for returning `struct' and `union' values when their
   size and alignment match an integer type).  */
#define LOW_RETURN_REGNUM	I386_EAX_REGNUM /* %eax */
#define HIGH_RETURN_REGNUM	I386_EDX_REGNUM /* %edx */

/* Read, for architecture GDBARCH, a function return value of TYPE
   from REGCACHE, and copy that into VALBUF.  */

static void
i386_extract_return_value (struct gdbarch *gdbarch, struct type *type,
			   struct regcache *regcache, gdb_byte *valbuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int len = TYPE_LENGTH (type);
  gdb_byte buf[I386_MAX_REGISTER_SIZE];

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      if (tdep->st0_regnum < 0)
	{
	  warning (_("Cannot find floating-point return value."));
	  memset (valbuf, 0, len);
	  return;
	}

      /* Floating-point return values can be found in %st(0).  Convert
	 its contents to the desired type.  This is probably not
	 exactly how it would happen on the target itself, but it is
	 the best we can do.  */
      regcache_raw_read (regcache, I386_ST0_REGNUM, buf);
      convert_typed_floating (buf, i387_ext_type (gdbarch), valbuf, type);
    }
  else
    {
      int low_size = register_size (gdbarch, LOW_RETURN_REGNUM);
      int high_size = register_size (gdbarch, HIGH_RETURN_REGNUM);

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
			_("Cannot extract return value of %d bytes long."),
			len);
    }
}

/* Write, for architecture GDBARCH, a function return value of TYPE
   from VALBUF into REGCACHE.  */

static void
i386_store_return_value (struct gdbarch *gdbarch, struct type *type,
			 struct regcache *regcache, const gdb_byte *valbuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int len = TYPE_LENGTH (type);

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      ULONGEST fstat;
      gdb_byte buf[I386_MAX_REGISTER_SIZE];

      if (tdep->st0_regnum < 0)
	{
	  warning (_("Cannot set floating-point return value."));
	  return;
	}

      /* Returning floating-point values is a bit tricky.  Apart from
         storing the return value in %st(0), we have to simulate the
         state of the FPU at function return point.  */

      /* Convert the value found in VALBUF to the extended
	 floating-point format used by the FPU.  This is probably
	 not exactly how it would happen on the target itself, but
	 it is the best we can do.  */
      convert_typed_floating (valbuf, type, buf, i387_ext_type (gdbarch));
      regcache_raw_write (regcache, I386_ST0_REGNUM, buf);

      /* Set the top of the floating-point register stack to 7.  The
         actual value doesn't really matter, but 7 is what a normal
         function return would end up with if the program started out
         with a freshly initialized FPU.  */
      regcache_raw_read_unsigned (regcache, I387_FSTAT_REGNUM (tdep), &fstat);
      fstat |= (7 << 11);
      regcache_raw_write_unsigned (regcache, I387_FSTAT_REGNUM (tdep), fstat);

      /* Mark %st(1) through %st(7) as empty.  Since we set the top of
         the floating-point register stack to 7, the appropriate value
         for the tag word is 0x3fff.  */
      regcache_raw_write_unsigned (regcache, I387_FTAG_REGNUM (tdep), 0x3fff);
    }
  else
    {
      int low_size = register_size (gdbarch, LOW_RETURN_REGNUM);
      int high_size = register_size (gdbarch, HIGH_RETURN_REGNUM);

      if (len <= low_size)
	regcache_raw_write_part (regcache, LOW_RETURN_REGNUM, 0, len, valbuf);
      else if (len <= (low_size + high_size))
	{
	  regcache_raw_write (regcache, LOW_RETURN_REGNUM, valbuf);
	  regcache_raw_write_part (regcache, HIGH_RETURN_REGNUM, 0,
				   len - low_size, valbuf + low_size);
	}
      else
	internal_error (__FILE__, __LINE__,
			_("Cannot store return value of %d bytes long."), len);
    }
}


/* This is the variable that is set with "set struct-convention", and
   its legitimate values.  */
static const char default_struct_convention[] = "default";
static const char pcc_struct_convention[] = "pcc";
static const char reg_struct_convention[] = "reg";
static const char *const valid_conventions[] =
{
  default_struct_convention,
  pcc_struct_convention,
  reg_struct_convention,
  NULL
};
static const char *struct_convention = default_struct_convention;

/* Return non-zero if TYPE, which is assumed to be a structure,
   a union type, or an array type, should be returned in registers
   for architecture GDBARCH.  */

static int
i386_reg_struct_return_p (struct gdbarch *gdbarch, struct type *type)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum type_code code = TYPE_CODE (type);
  int len = TYPE_LENGTH (type);

  gdb_assert (code == TYPE_CODE_STRUCT
              || code == TYPE_CODE_UNION
              || code == TYPE_CODE_ARRAY);

  if (struct_convention == pcc_struct_convention
      || (struct_convention == default_struct_convention
	  && tdep->struct_return == pcc_struct_return))
    return 0;

  /* Structures consisting of a single `float', `double' or 'long
     double' member are returned in %st(0).  */
  if (code == TYPE_CODE_STRUCT && TYPE_NFIELDS (type) == 1)
    {
      type = check_typedef (TYPE_FIELD_TYPE (type, 0));
      if (TYPE_CODE (type) == TYPE_CODE_FLT)
	return (len == 4 || len == 8 || len == 12);
    }

  return (len == 1 || len == 2 || len == 4 || len == 8);
}

/* Determine, for architecture GDBARCH, how a return value of TYPE
   should be returned.  If it is supposed to be returned in registers,
   and READBUF is non-zero, read the appropriate value from REGCACHE,
   and copy it into READBUF.  If WRITEBUF is non-zero, write the value
   from WRITEBUF into REGCACHE.  */

static enum return_value_convention
i386_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *type, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  enum type_code code = TYPE_CODE (type);

  if (((code == TYPE_CODE_STRUCT
	|| code == TYPE_CODE_UNION
	|| code == TYPE_CODE_ARRAY)
       && !i386_reg_struct_return_p (gdbarch, type))
      /* Complex double and long double uses the struct return covention.  */
      || (code == TYPE_CODE_COMPLEX && TYPE_LENGTH (type) == 16)
      || (code == TYPE_CODE_COMPLEX && TYPE_LENGTH (type) == 24)
      /* 128-bit decimal float uses the struct return convention.  */
      || (code == TYPE_CODE_DECFLOAT && TYPE_LENGTH (type) == 16))
    {
      /* The System V ABI says that:

	 "A function that returns a structure or union also sets %eax
	 to the value of the original address of the caller's area
	 before it returns.  Thus when the caller receives control
	 again, the address of the returned object resides in register
	 %eax and can be used to access the object."

	 So the ABI guarantees that we can always find the return
	 value just after the function has returned.  */

      /* Note that the ABI doesn't mention functions returning arrays,
         which is something possible in certain languages such as Ada.
         In this case, the value is returned as if it was wrapped in
         a record, so the convention applied to records also applies
         to arrays.  */

      if (readbuf)
	{
	  ULONGEST addr;

	  regcache_raw_read_unsigned (regcache, I386_EAX_REGNUM, &addr);
	  read_memory (addr, readbuf, TYPE_LENGTH (type));
	}

      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }

  /* This special case is for structures consisting of a single
     `float', `double' or 'long double' member.  These structures are
     returned in %st(0).  For these structures, we call ourselves
     recursively, changing TYPE into the type of the first member of
     the structure.  Since that should work for all structures that
     have only one member, we don't bother to check the member's type
     here.  */
  if (code == TYPE_CODE_STRUCT && TYPE_NFIELDS (type) == 1)
    {
      type = check_typedef (TYPE_FIELD_TYPE (type, 0));
      return i386_return_value (gdbarch, function, type, regcache,
				readbuf, writebuf);
    }

  if (readbuf)
    i386_extract_return_value (gdbarch, type, regcache, readbuf);
  if (writebuf)
    i386_store_return_value (gdbarch, type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}


struct type *
i387_ext_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (!tdep->i387_ext_type)
    {
      tdep->i387_ext_type = tdesc_find_type (gdbarch, "i387_ext");
      gdb_assert (tdep->i387_ext_type != NULL);
    }

  return tdep->i387_ext_type;
}

/* Construct vector type for pseudo YMM registers.  We can't use
   tdesc_find_type since YMM isn't described in target description.  */

static struct type *
i386_ymm_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (!tdep->i386_ymm_type)
    {
      const struct builtin_type *bt = builtin_type (gdbarch);

      /* The type we're building is this: */
#if 0
      union __gdb_builtin_type_vec256i
      {
        int128_t uint128[2];
        int64_t v2_int64[4];
        int32_t v4_int32[8];
        int16_t v8_int16[16];
        int8_t v16_int8[32];
        double v2_double[4];
        float v4_float[8];
      };
#endif

      struct type *t;

      t = arch_composite_type (gdbarch,
			       "__gdb_builtin_type_vec256i", TYPE_CODE_UNION);
      append_composite_type_field (t, "v8_float",
				   init_vector_type (bt->builtin_float, 8));
      append_composite_type_field (t, "v4_double",
				   init_vector_type (bt->builtin_double, 4));
      append_composite_type_field (t, "v32_int8",
				   init_vector_type (bt->builtin_int8, 32));
      append_composite_type_field (t, "v16_int16",
				   init_vector_type (bt->builtin_int16, 16));
      append_composite_type_field (t, "v8_int32",
				   init_vector_type (bt->builtin_int32, 8));
      append_composite_type_field (t, "v4_int64",
				   init_vector_type (bt->builtin_int64, 4));
      append_composite_type_field (t, "v2_int128",
				   init_vector_type (bt->builtin_int128, 2));

      TYPE_VECTOR (t) = 1;
      TYPE_NAME (t) = "builtin_type_vec256i";
      tdep->i386_ymm_type = t;
    }

  return tdep->i386_ymm_type;
}

/* Construct vector type for MMX registers.  */
static struct type *
i386_mmx_type (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (!tdep->i386_mmx_type)
    {
      const struct builtin_type *bt = builtin_type (gdbarch);

      /* The type we're building is this: */
#if 0
      union __gdb_builtin_type_vec64i
      {
        int64_t uint64;
        int32_t v2_int32[2];
        int16_t v4_int16[4];
        int8_t v8_int8[8];
      };
#endif

      struct type *t;

      t = arch_composite_type (gdbarch,
			       "__gdb_builtin_type_vec64i", TYPE_CODE_UNION);

      append_composite_type_field (t, "uint64", bt->builtin_int64);
      append_composite_type_field (t, "v2_int32",
				   init_vector_type (bt->builtin_int32, 2));
      append_composite_type_field (t, "v4_int16",
				   init_vector_type (bt->builtin_int16, 4));
      append_composite_type_field (t, "v8_int8",
				   init_vector_type (bt->builtin_int8, 8));

      TYPE_VECTOR (t) = 1;
      TYPE_NAME (t) = "builtin_type_vec64i";
      tdep->i386_mmx_type = t;
    }

  return tdep->i386_mmx_type;
}

/* Return the GDB type object for the "standard" data type of data in
   register REGNUM.  */

struct type *
i386_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  if (i386_mmx_regnum_p (gdbarch, regnum))
    return i386_mmx_type (gdbarch);
  else if (i386_ymm_regnum_p (gdbarch, regnum))
    return i386_ymm_type (gdbarch);
  else
    {
      const struct builtin_type *bt = builtin_type (gdbarch);
      if (i386_byte_regnum_p (gdbarch, regnum))
	return bt->builtin_int8;
      else if (i386_word_regnum_p (gdbarch, regnum))
	return bt->builtin_int16;
      else if (i386_dword_regnum_p (gdbarch, regnum))
	return bt->builtin_int32;
    }

  internal_error (__FILE__, __LINE__, _("invalid regnum"));
}

/* Map a cooked register onto a raw register or memory.  For the i386,
   the MMX registers need to be mapped onto floating point registers.  */

static int
i386_mmx_regnum_to_fp_regnum (struct regcache *regcache, int regnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_regcache_arch (regcache));
  int mmxreg, fpreg;
  ULONGEST fstat;
  int tos;

  mmxreg = regnum - tdep->mm0_regnum;
  regcache_raw_read_unsigned (regcache, I387_FSTAT_REGNUM (tdep), &fstat);
  tos = (fstat >> 11) & 0x7;
  fpreg = (mmxreg + tos) % 8;

  return (I387_ST0_REGNUM (tdep) + fpreg);
}

/* A helper function for us by i386_pseudo_register_read_value and
   amd64_pseudo_register_read_value.  It does all the work but reads
   the data into an already-allocated value.  */

void
i386_pseudo_register_read_into_value (struct gdbarch *gdbarch,
				      struct regcache *regcache,
				      int regnum,
				      struct value *result_value)
{
  gdb_byte raw_buf[MAX_REGISTER_SIZE];
  enum register_status status;
  gdb_byte *buf = value_contents_raw (result_value);

  if (i386_mmx_regnum_p (gdbarch, regnum))
    {
      int fpnum = i386_mmx_regnum_to_fp_regnum (regcache, regnum);

      /* Extract (always little endian).  */
      status = regcache_raw_read (regcache, fpnum, raw_buf);
      if (status != REG_VALID)
	mark_value_bytes_unavailable (result_value, 0,
				      TYPE_LENGTH (value_type (result_value)));
      else
	memcpy (buf, raw_buf, register_size (gdbarch, regnum));
    }
  else
    {
      struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

      if (i386_ymm_regnum_p (gdbarch, regnum))
	{
	  regnum -= tdep->ymm0_regnum;

	  /* Extract (always little endian).  Read lower 128bits.  */
	  status = regcache_raw_read (regcache,
				      I387_XMM0_REGNUM (tdep) + regnum,
				      raw_buf);
	  if (status != REG_VALID)
	    mark_value_bytes_unavailable (result_value, 0, 16);
	  else
	    memcpy (buf, raw_buf, 16);
	  /* Read upper 128bits.  */
	  status = regcache_raw_read (regcache,
				      tdep->ymm0h_regnum + regnum,
				      raw_buf);
	  if (status != REG_VALID)
	    mark_value_bytes_unavailable (result_value, 16, 32);
	  else
	    memcpy (buf + 16, raw_buf, 16);
	}
      else if (i386_word_regnum_p (gdbarch, regnum))
	{
	  int gpnum = regnum - tdep->ax_regnum;

	  /* Extract (always little endian).  */
	  status = regcache_raw_read (regcache, gpnum, raw_buf);
	  if (status != REG_VALID)
	    mark_value_bytes_unavailable (result_value, 0,
					  TYPE_LENGTH (value_type (result_value)));
	  else
	    memcpy (buf, raw_buf, 2);
	}
      else if (i386_byte_regnum_p (gdbarch, regnum))
	{
	  /* Check byte pseudo registers last since this function will
	     be called from amd64_pseudo_register_read, which handles
	     byte pseudo registers differently.  */
	  int gpnum = regnum - tdep->al_regnum;

	  /* Extract (always little endian).  We read both lower and
	     upper registers.  */
	  status = regcache_raw_read (regcache, gpnum % 4, raw_buf);
	  if (status != REG_VALID)
	    mark_value_bytes_unavailable (result_value, 0,
					  TYPE_LENGTH (value_type (result_value)));
	  else if (gpnum >= 4)
	    memcpy (buf, raw_buf + 1, 1);
	  else
	    memcpy (buf, raw_buf, 1);
	}
      else
	internal_error (__FILE__, __LINE__, _("invalid regnum"));
    }
}

static struct value *
i386_pseudo_register_read_value (struct gdbarch *gdbarch,
				 struct regcache *regcache,
				 int regnum)
{
  struct value *result;

  result = allocate_value (register_type (gdbarch, regnum));
  VALUE_LVAL (result) = lval_register;
  VALUE_REGNUM (result) = regnum;

  i386_pseudo_register_read_into_value (gdbarch, regcache, regnum, result);

  return result;
}

void
i386_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int regnum, const gdb_byte *buf)
{
  gdb_byte raw_buf[MAX_REGISTER_SIZE];

  if (i386_mmx_regnum_p (gdbarch, regnum))
    {
      int fpnum = i386_mmx_regnum_to_fp_regnum (regcache, regnum);

      /* Read ...  */
      regcache_raw_read (regcache, fpnum, raw_buf);
      /* ... Modify ... (always little endian).  */
      memcpy (raw_buf, buf, register_size (gdbarch, regnum));
      /* ... Write.  */
      regcache_raw_write (regcache, fpnum, raw_buf);
    }
  else
    {
      struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

      if (i386_ymm_regnum_p (gdbarch, regnum))
	{
	  regnum -= tdep->ymm0_regnum;

	  /* ... Write lower 128bits.  */
	  regcache_raw_write (regcache,
			     I387_XMM0_REGNUM (tdep) + regnum,
			     buf);
	  /* ... Write upper 128bits.  */
	  regcache_raw_write (regcache,
			     tdep->ymm0h_regnum + regnum,
			     buf + 16);
	}
      else if (i386_word_regnum_p (gdbarch, regnum))
	{
	  int gpnum = regnum - tdep->ax_regnum;

	  /* Read ...  */
	  regcache_raw_read (regcache, gpnum, raw_buf);
	  /* ... Modify ... (always little endian).  */
	  memcpy (raw_buf, buf, 2);
	  /* ... Write.  */
	  regcache_raw_write (regcache, gpnum, raw_buf);
	}
      else if (i386_byte_regnum_p (gdbarch, regnum))
	{
	  /* Check byte pseudo registers last since this function will
	     be called from amd64_pseudo_register_read, which handles
	     byte pseudo registers differently.  */
	  int gpnum = regnum - tdep->al_regnum;

	  /* Read ...  We read both lower and upper registers.  */
	  regcache_raw_read (regcache, gpnum % 4, raw_buf);
	  /* ... Modify ... (always little endian).  */
	  if (gpnum >= 4)
	    memcpy (raw_buf + 1, buf, 1);
	  else
	    memcpy (raw_buf, buf, 1);
	  /* ... Write.  */
	  regcache_raw_write (regcache, gpnum % 4, raw_buf);
	}
      else
	internal_error (__FILE__, __LINE__, _("invalid regnum"));
    }
}


/* Return the register number of the register allocated by GCC after
   REGNUM, or -1 if there is no such register.  */

static int
i386_next_regnum (int regnum)
{
  /* GCC allocates the registers in the order:

     %eax, %edx, %ecx, %ebx, %esi, %edi, %ebp, %esp, ...

     Since storing a variable in %esp doesn't make any sense we return
     -1 for %ebp and for %esp itself.  */
  static int next_regnum[] =
  {
    I386_EDX_REGNUM,		/* Slot for %eax.  */
    I386_EBX_REGNUM,		/* Slot for %ecx.  */
    I386_ECX_REGNUM,		/* Slot for %edx.  */
    I386_ESI_REGNUM,		/* Slot for %ebx.  */
    -1, -1,			/* Slots for %esp and %ebp.  */
    I386_EDI_REGNUM,		/* Slot for %esi.  */
    I386_EBP_REGNUM		/* Slot for %edi.  */
  };

  if (regnum >= 0 && regnum < sizeof (next_regnum) / sizeof (next_regnum[0]))
    return next_regnum[regnum];

  return -1;
}

/* Return nonzero if a value of type TYPE stored in register REGNUM
   needs any special handling.  */

static int
i386_convert_register_p (struct gdbarch *gdbarch,
			 int regnum, struct type *type)
{
  int len = TYPE_LENGTH (type);

  /* Values may be spread across multiple registers.  Most debugging
     formats aren't expressive enough to specify the locations, so
     some heuristics is involved.  Right now we only handle types that
     have a length that is a multiple of the word size, since GCC
     doesn't seem to put any other types into registers.  */
  if (len > 4 && len % 4 == 0)
    {
      int last_regnum = regnum;

      while (len > 4)
	{
	  last_regnum = i386_next_regnum (last_regnum);
	  len -= 4;
	}

      if (last_regnum != -1)
	return 1;
    }

  return i387_convert_register_p (gdbarch, regnum, type);
}

/* Read a value of type TYPE from register REGNUM in frame FRAME, and
   return its contents in TO.  */

static int
i386_register_to_value (struct frame_info *frame, int regnum,
			struct type *type, gdb_byte *to,
			int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int len = TYPE_LENGTH (type);

  if (i386_fp_regnum_p (gdbarch, regnum))
    return i387_register_to_value (frame, regnum, type, to,
				   optimizedp, unavailablep);

  /* Read a value spread across multiple registers.  */

  gdb_assert (len > 4 && len % 4 == 0);

  while (len > 0)
    {
      gdb_assert (regnum != -1);
      gdb_assert (register_size (gdbarch, regnum) == 4);

      if (!get_frame_register_bytes (frame, regnum, 0,
				     register_size (gdbarch, regnum),
				     to, optimizedp, unavailablep))
	return 0;

      regnum = i386_next_regnum (regnum);
      len -= 4;
      to += 4;
    }

  *optimizedp = *unavailablep = 0;
  return 1;
}

/* Write the contents FROM of a value of type TYPE into register
   REGNUM in frame FRAME.  */

static void
i386_value_to_register (struct frame_info *frame, int regnum,
			struct type *type, const gdb_byte *from)
{
  int len = TYPE_LENGTH (type);

  if (i386_fp_regnum_p (get_frame_arch (frame), regnum))
    {
      i387_value_to_register (frame, regnum, type, from);
      return;
    }

  /* Write a value spread across multiple registers.  */

  gdb_assert (len > 4 && len % 4 == 0);

  while (len > 0)
    {
      gdb_assert (regnum != -1);
      gdb_assert (register_size (get_frame_arch (frame), regnum) == 4);

      put_frame_register (frame, regnum, from);
      regnum = i386_next_regnum (regnum);
      len -= 4;
      from += 4;
    }
}

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

void
i386_supply_gregset (const struct regset *regset, struct regcache *regcache,
		     int regnum, const void *gregs, size_t len)
{
  const struct gdbarch_tdep *tdep = gdbarch_tdep (regset->arch);
  const gdb_byte *regs = gregs;
  int i;

  gdb_assert (len == tdep->sizeof_gregset);

  for (i = 0; i < tdep->gregset_num_regs; i++)
    {
      if ((regnum == i || regnum == -1)
	  && tdep->gregset_reg_offset[i] != -1)
	regcache_raw_supply (regcache, i, regs + tdep->gregset_reg_offset[i]);
    }
}

/* Collect register REGNUM from the register cache REGCACHE and store
   it in the buffer specified by GREGS and LEN as described by the
   general-purpose register set REGSET.  If REGNUM is -1, do this for
   all registers in REGSET.  */

void
i386_collect_gregset (const struct regset *regset,
		      const struct regcache *regcache,
		      int regnum, void *gregs, size_t len)
{
  const struct gdbarch_tdep *tdep = gdbarch_tdep (regset->arch);
  gdb_byte *regs = gregs;
  int i;

  gdb_assert (len == tdep->sizeof_gregset);

  for (i = 0; i < tdep->gregset_num_regs; i++)
    {
      if ((regnum == i || regnum == -1)
	  && tdep->gregset_reg_offset[i] != -1)
	regcache_raw_collect (regcache, i, regs + tdep->gregset_reg_offset[i]);
    }
}

/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
i386_supply_fpregset (const struct regset *regset, struct regcache *regcache,
		      int regnum, const void *fpregs, size_t len)
{
  const struct gdbarch_tdep *tdep = gdbarch_tdep (regset->arch);

  if (len == I387_SIZEOF_FXSAVE)
    {
      i387_supply_fxsave (regcache, regnum, fpregs);
      return;
    }

  gdb_assert (len == tdep->sizeof_fpregset);
  i387_supply_fsave (regcache, regnum, fpregs);
}

/* Collect register REGNUM from the register cache REGCACHE and store
   it in the buffer specified by FPREGS and LEN as described by the
   floating-point register set REGSET.  If REGNUM is -1, do this for
   all registers in REGSET.  */

static void
i386_collect_fpregset (const struct regset *regset,
		       const struct regcache *regcache,
		       int regnum, void *fpregs, size_t len)
{
  const struct gdbarch_tdep *tdep = gdbarch_tdep (regset->arch);

  if (len == I387_SIZEOF_FXSAVE)
    {
      i387_collect_fxsave (regcache, regnum, fpregs);
      return;
    }

  gdb_assert (len == tdep->sizeof_fpregset);
  i387_collect_fsave (regcache, regnum, fpregs);
}

/* Similar to i386_supply_fpregset, but use XSAVE extended state.  */

static void
i386_supply_xstateregset (const struct regset *regset,
			  struct regcache *regcache, int regnum,
			  const void *xstateregs, size_t len)
{
  i387_supply_xsave (regcache, regnum, xstateregs);
}

/* Similar to i386_collect_fpregset , but use XSAVE extended state.  */

static void
i386_collect_xstateregset (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *xstateregs, size_t len)
{
  i387_collect_xsave (regcache, regnum, xstateregs, 1);
}

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */

const struct regset *
i386_regset_from_core_section (struct gdbarch *gdbarch,
			       const char *sect_name, size_t sect_size)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (strcmp (sect_name, ".reg") == 0 && sect_size == tdep->sizeof_gregset)
    {
      if (tdep->gregset == NULL)
	tdep->gregset = regset_alloc (gdbarch, i386_supply_gregset,
				      i386_collect_gregset);
      return tdep->gregset;
    }

  if ((strcmp (sect_name, ".reg2") == 0 && sect_size == tdep->sizeof_fpregset)
      || (strcmp (sect_name, ".reg-xfp") == 0
	  && sect_size == I387_SIZEOF_FXSAVE))
    {
      if (tdep->fpregset == NULL)
	tdep->fpregset = regset_alloc (gdbarch, i386_supply_fpregset,
				       i386_collect_fpregset);
      return tdep->fpregset;
    }

  if (strcmp (sect_name, ".reg-xstate") == 0)
    {
      if (tdep->xstateregset == NULL)
	tdep->xstateregset = regset_alloc (gdbarch,
					   i386_supply_xstateregset,
					   i386_collect_xstateregset);

      return tdep->xstateregset;
    }

  return NULL;
}


/* Stuff for WIN32 PE style DLL's but is pretty generic really.  */

CORE_ADDR
i386_pe_skip_trampoline_code (struct frame_info *frame,
			      CORE_ADDR pc, char *name)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* jmp *(dest) */
  if (pc && read_memory_unsigned_integer (pc, 2, byte_order) == 0x25ff)
    {
      unsigned long indirect =
	read_memory_unsigned_integer (pc + 2, 4, byte_order);
      struct minimal_symbol *indsym =
	indirect ? lookup_minimal_symbol_by_pc (indirect) : 0;
      const char *symname = indsym ? SYMBOL_LINKAGE_NAME (indsym) : 0;

      if (symname)
	{
	  if (strncmp (symname, "__imp_", 6) == 0
	      || strncmp (symname, "_imp_", 5) == 0)
	    return name ? 1 :
		   read_memory_unsigned_integer (indirect, 4, byte_order);
	}
    }
  return 0;			/* Not a trampoline.  */
}


/* Return whether the THIS_FRAME corresponds to a sigtramp
   routine.  */

int
i386_sigtramp_p (struct frame_info *this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  return (name && strcmp ("_sigtramp", name) == 0);
}


/* We have two flavours of disassembly.  The machinery on this page
   deals with switching between those.  */

static int
i386_print_insn (bfd_vma pc, struct disassemble_info *info)
{
  gdb_assert (disassembly_flavor == att_flavor
	      || disassembly_flavor == intel_flavor);

  /* FIXME: kettenis/20020915: Until disassembler_options is properly
     constified, cast to prevent a compiler warning.  */
  info->disassembler_options = (char *) disassembly_flavor;

  return print_insn_i386 (pc, info);
}


/* There are a few i386 architecture variants that differ only
   slightly from the generic i386 target.  For now, we don't give them
   their own source file, but include them here.  As a consequence,
   they'll always be included.  */

/* System V Release 4 (SVR4).  */

/* Return whether THIS_FRAME corresponds to a SVR4 sigtramp
   routine.  */

static int
i386_svr4_sigtramp_p (struct frame_info *this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  /* The origin of these symbols is currently unknown.  */
  find_pc_partial_function (pc, &name, NULL, NULL);
  return (name && (strcmp ("_sigreturn", name) == 0
		   || strcmp ("sigvechandler", name) == 0));
}

/* Assuming THIS_FRAME is for a SVR4 sigtramp routine, return the
   address of the associated sigcontext (ucontext) structure.  */

static CORE_ADDR
i386_svr4_sigcontext_addr (struct frame_info *this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  CORE_ADDR sp;

  get_frame_register (this_frame, I386_ESP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 4, byte_order);

  return read_memory_unsigned_integer (sp + 8, 4, byte_order);
}



/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

int
i386_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (*s == '$' /* Literal number.  */
	  || (isdigit (*s) && s[1] == '(' && s[2] == '%') /* Displacement.  */
	  || (*s == '(' && s[1] == '%') /* Register indirection.  */
	  || (*s == '%' && isalpha (s[1]))); /* Register access.  */
}

/* Implementation of `gdbarch_stap_parse_special_token', as defined in
   gdbarch.h.  */

int
i386_stap_parse_special_token (struct gdbarch *gdbarch,
			       struct stap_parse_info *p)
{
  /* In order to parse special tokens, we use a state-machine that go
     through every known token and try to get a match.  */
  enum
    {
      TRIPLET,
      THREE_ARG_DISPLACEMENT,
      DONE
    } current_state;

  current_state = TRIPLET;

  /* The special tokens to be parsed here are:

     - `register base + (register index * size) + offset', as represented
     in `(%rcx,%rax,8)', or `[OFFSET](BASE_REG,INDEX_REG[,SIZE])'.

     - Operands of the form `-8+3+1(%rbp)', which must be interpreted as
     `*(-8 + 3 - 1 + (void *) $eax)'.  */

  while (current_state != DONE)
    {
      const char *s = p->arg;

      switch (current_state)
	{
	case TRIPLET:
	    {
	      if (isdigit (*s) || *s == '-' || *s == '+')
		{
		  int got_minus[3];
		  int i;
		  long displacements[3];
		  const char *start;
		  char *regname;
		  int len;
		  struct stoken str;

		  got_minus[0] = 0;
		  if (*s == '+')
		    ++s;
		  else if (*s == '-')
		    {
		      ++s;
		      got_minus[0] = 1;
		    }

		  displacements[0] = strtol (s, (char **) &s, 10);

		  if (*s != '+' && *s != '-')
		    {
		      /* We are not dealing with a triplet.  */
		      break;
		    }

		  got_minus[1] = 0;
		  if (*s == '+')
		    ++s;
		  else
		    {
		      ++s;
		      got_minus[1] = 1;
		    }

		  displacements[1] = strtol (s, (char **) &s, 10);

		  if (*s != '+' && *s != '-')
		    {
		      /* We are not dealing with a triplet.  */
		      break;
		    }

		  got_minus[2] = 0;
		  if (*s == '+')
		    ++s;
		  else
		    {
		      ++s;
		      got_minus[2] = 1;
		    }

		  displacements[2] = strtol (s, (char **) &s, 10);

		  if (*s != '(' || s[1] != '%')
		    break;

		  s += 2;
		  start = s;

		  while (isalnum (*s))
		    ++s;

		  if (*s++ != ')')
		    break;

		  len = s - start;
		  regname = alloca (len + 1);

		  strncpy (regname, start, len);
		  regname[len] = '\0';

		  if (user_reg_map_name_to_regnum (gdbarch,
						   regname, len) == -1)
		    error (_("Invalid register name `%s' "
			     "on expression `%s'."),
			   regname, p->saved_arg);

		  for (i = 0; i < 3; i++)
		    {
		      write_exp_elt_opcode (OP_LONG);
		      write_exp_elt_type
			(builtin_type (gdbarch)->builtin_long);
		      write_exp_elt_longcst (displacements[i]);
		      write_exp_elt_opcode (OP_LONG);
		      if (got_minus[i])
			write_exp_elt_opcode (UNOP_NEG);
		    }

		  write_exp_elt_opcode (OP_REGISTER);
		  str.ptr = regname;
		  str.length = len;
		  write_exp_string (str);
		  write_exp_elt_opcode (OP_REGISTER);

		  write_exp_elt_opcode (UNOP_CAST);
		  write_exp_elt_type (builtin_type (gdbarch)->builtin_data_ptr);
		  write_exp_elt_opcode (UNOP_CAST);

		  write_exp_elt_opcode (BINOP_ADD);
		  write_exp_elt_opcode (BINOP_ADD);
		  write_exp_elt_opcode (BINOP_ADD);

		  write_exp_elt_opcode (UNOP_CAST);
		  write_exp_elt_type (lookup_pointer_type (p->arg_type));
		  write_exp_elt_opcode (UNOP_CAST);

		  write_exp_elt_opcode (UNOP_IND);

		  p->arg = s;

		  return 1;
		}
	      break;
	    }
	case THREE_ARG_DISPLACEMENT:
	    {
	      if (isdigit (*s) || *s == '(' || *s == '-' || *s == '+')
		{
		  int offset_minus = 0;
		  long offset = 0;
		  int size_minus = 0;
		  long size = 0;
		  const char *start;
		  char *base;
		  int len_base;
		  char *index;
		  int len_index;
		  struct stoken base_token, index_token;

		  if (*s == '+')
		    ++s;
		  else if (*s == '-')
		    {
		      ++s;
		      offset_minus = 1;
		    }

		  if (offset_minus && !isdigit (*s))
		    break;

		  if (isdigit (*s))
		    offset = strtol (s, (char **) &s, 10);

		  if (*s != '(' || s[1] != '%')
		    break;

		  s += 2;
		  start = s;

		  while (isalnum (*s))
		    ++s;

		  if (*s != ',' || s[1] != '%')
		    break;

		  len_base = s - start;
		  base = alloca (len_base + 1);
		  strncpy (base, start, len_base);
		  base[len_base] = '\0';

		  if (user_reg_map_name_to_regnum (gdbarch,
						   base, len_base) == -1)
		    error (_("Invalid register name `%s' "
			     "on expression `%s'."),
			   base, p->saved_arg);

		  s += 2;
		  start = s;

		  while (isalnum (*s))
		    ++s;

		  len_index = s - start;
		  index = alloca (len_index + 1);
		  strncpy (index, start, len_index);
		  index[len_index] = '\0';

		  if (user_reg_map_name_to_regnum (gdbarch,
						   index, len_index) == -1)
		    error (_("Invalid register name `%s' "
			     "on expression `%s'."),
			   index, p->saved_arg);

		  if (*s != ',' && *s != ')')
		    break;

		  if (*s == ',')
		    {
		      ++s;
		      if (*s == '+')
			++s;
		      else if (*s == '-')
			{
			  ++s;
			  size_minus = 1;
			}

		      size = strtol (s, (char **) &s, 10);

		      if (*s != ')')
			break;
		    }

		  ++s;

		  if (offset)
		    {
		      write_exp_elt_opcode (OP_LONG);
		      write_exp_elt_type
			(builtin_type (gdbarch)->builtin_long);
		      write_exp_elt_longcst (offset);
		      write_exp_elt_opcode (OP_LONG);
		      if (offset_minus)
			write_exp_elt_opcode (UNOP_NEG);
		    }

		  write_exp_elt_opcode (OP_REGISTER);
		  base_token.ptr = base;
		  base_token.length = len_base;
		  write_exp_string (base_token);
		  write_exp_elt_opcode (OP_REGISTER);

		  if (offset)
		    write_exp_elt_opcode (BINOP_ADD);

		  write_exp_elt_opcode (OP_REGISTER);
		  index_token.ptr = index;
		  index_token.length = len_index;
		  write_exp_string (index_token);
		  write_exp_elt_opcode (OP_REGISTER);

		  if (size)
		    {
		      write_exp_elt_opcode (OP_LONG);
		      write_exp_elt_type
			(builtin_type (gdbarch)->builtin_long);
		      write_exp_elt_longcst (size);
		      write_exp_elt_opcode (OP_LONG);
		      if (size_minus)
			write_exp_elt_opcode (UNOP_NEG);
		      write_exp_elt_opcode (BINOP_MUL);
		    }

		  write_exp_elt_opcode (BINOP_ADD);

		  write_exp_elt_opcode (UNOP_CAST);
		  write_exp_elt_type (lookup_pointer_type (p->arg_type));
		  write_exp_elt_opcode (UNOP_CAST);

		  write_exp_elt_opcode (UNOP_IND);

		  p->arg = s;

		  return 1;
		}
	      break;
	    }
	}

      /* Advancing to the next state.  */
      ++current_state;
    }

  return 0;
}



/* Generic ELF.  */

void
i386_elf_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* We typically use stabs-in-ELF with the SVR4 register numbering.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, i386_svr4_reg_to_regnum);

  /* Registering SystemTap handlers.  */
  set_gdbarch_stap_integer_prefix (gdbarch, "$");
  set_gdbarch_stap_register_prefix (gdbarch, "%");
  set_gdbarch_stap_register_indirection_prefix (gdbarch, "(");
  set_gdbarch_stap_register_indirection_suffix (gdbarch, ")");
  set_gdbarch_stap_is_single_operand (gdbarch,
				      i386_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					i386_stap_parse_special_token);
}

/* System V Release 4 (SVR4).  */

void
i386_svr4_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* System V Release 4 uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  /* System V Release 4 has shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  tdep->sigtramp_p = i386_svr4_sigtramp_p;
  tdep->sigcontext_addr = i386_svr4_sigcontext_addr;
  tdep->sc_pc_offset = 36 + 14 * 4;
  tdep->sc_sp_offset = 36 + 17 * 4;

  tdep->jb_pc_offset = 20;
}

/* DJGPP.  */

static void
i386_go32_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* DJGPP doesn't have any special frames for signal handlers.  */
  tdep->sigtramp_p = NULL;

  tdep->jb_pc_offset = 36;

  /* DJGPP does not support the SSE registers.  */
  if (! tdesc_has_registers (info.target_desc))
    tdep->tdesc = tdesc_i386_mmx;

  /* Native compiler is GCC, which uses the SVR4 register numbering
     even in COFF and STABS.  See the comment in i386_gdbarch_init,
     before the calls to set_gdbarch_stab_reg_to_regnum and
     set_gdbarch_sdb_reg_to_regnum.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, i386_svr4_reg_to_regnum);
  set_gdbarch_sdb_reg_to_regnum (gdbarch, i386_svr4_reg_to_regnum);

  set_gdbarch_has_dos_based_file_system (gdbarch, 1);
}


/* i386 register groups.  In addition to the normal groups, add "mmx"
   and "sse".  */

static struct reggroup *i386_sse_reggroup;
static struct reggroup *i386_mmx_reggroup;

static void
i386_init_reggroups (void)
{
  i386_sse_reggroup = reggroup_new ("sse", USER_REGGROUP);
  i386_mmx_reggroup = reggroup_new ("mmx", USER_REGGROUP);
}

static void
i386_add_reggroups (struct gdbarch *gdbarch)
{
  reggroup_add (gdbarch, i386_sse_reggroup);
  reggroup_add (gdbarch, i386_mmx_reggroup);
  reggroup_add (gdbarch, general_reggroup);
  reggroup_add (gdbarch, float_reggroup);
  reggroup_add (gdbarch, all_reggroup);
  reggroup_add (gdbarch, save_reggroup);
  reggroup_add (gdbarch, restore_reggroup);
  reggroup_add (gdbarch, vector_reggroup);
  reggroup_add (gdbarch, system_reggroup);
}

int
i386_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			  struct reggroup *group)
{
  const struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int fp_regnum_p, mmx_regnum_p, xmm_regnum_p, mxcsr_regnum_p,
      ymm_regnum_p, ymmh_regnum_p;

  /* Don't include pseudo registers, except for MMX, in any register
     groups.  */
  if (i386_byte_regnum_p (gdbarch, regnum))
    return 0;

  if (i386_word_regnum_p (gdbarch, regnum))
    return 0;

  if (i386_dword_regnum_p (gdbarch, regnum))
    return 0;

  mmx_regnum_p = i386_mmx_regnum_p (gdbarch, regnum);
  if (group == i386_mmx_reggroup)
    return mmx_regnum_p;

  xmm_regnum_p = i386_xmm_regnum_p (gdbarch, regnum);
  mxcsr_regnum_p = i386_mxcsr_regnum_p (gdbarch, regnum);
  if (group == i386_sse_reggroup)
    return xmm_regnum_p || mxcsr_regnum_p;

  ymm_regnum_p = i386_ymm_regnum_p (gdbarch, regnum);
  if (group == vector_reggroup)
    return (mmx_regnum_p
	    || ymm_regnum_p
	    || mxcsr_regnum_p
	    || (xmm_regnum_p
		&& ((tdep->xcr0 & I386_XSTATE_AVX_MASK)
		    == I386_XSTATE_SSE_MASK)));

  fp_regnum_p = (i386_fp_regnum_p (gdbarch, regnum)
		 || i386_fpc_regnum_p (gdbarch, regnum));
  if (group == float_reggroup)
    return fp_regnum_p;

  /* For "info reg all", don't include upper YMM registers nor XMM
     registers when AVX is supported.  */
  ymmh_regnum_p = i386_ymmh_regnum_p (gdbarch, regnum);
  if (group == all_reggroup
      && ((xmm_regnum_p
	   && (tdep->xcr0 & I386_XSTATE_AVX))
	  || ymmh_regnum_p))
    return 0;

  if (group == general_reggroup)
    return (!fp_regnum_p
	    && !mmx_regnum_p
	    && !mxcsr_regnum_p
	    && !xmm_regnum_p
	    && !ymm_regnum_p
	    && !ymmh_regnum_p);

  return default_register_reggroup_p (gdbarch, regnum, group);
}


/* Get the ARGIth function argument for the current function.  */

static CORE_ADDR
i386_fetch_pointer_argument (struct frame_info *frame, int argi, 
			     struct type *type)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (frame, I386_ESP_REGNUM);
  return read_memory_unsigned_integer (sp + (4 * (argi + 1)), 4, byte_order);
}

static void
i386_skip_permanent_breakpoint (struct regcache *regcache)
{
  CORE_ADDR current_pc = regcache_read_pc (regcache);

 /* On i386, breakpoint is exactly 1 byte long, so we just
    adjust the PC in the regcache.  */
  current_pc += 1;
  regcache_write_pc (regcache, current_pc);
}


#define PREFIX_REPZ	0x01
#define PREFIX_REPNZ	0x02
#define PREFIX_LOCK	0x04
#define PREFIX_DATA	0x08
#define PREFIX_ADDR	0x10

/* operand size */
enum
{
  OT_BYTE = 0,
  OT_WORD,
  OT_LONG,
  OT_QUAD,
  OT_DQUAD,
};

/* i386 arith/logic operations */
enum
{
  OP_ADDL,
  OP_ORL,
  OP_ADCL,
  OP_SBBL,
  OP_ANDL,
  OP_SUBL,
  OP_XORL,
  OP_CMPL,
};

struct i386_record_s
{
  struct gdbarch *gdbarch;
  struct regcache *regcache;
  CORE_ADDR orig_addr;
  CORE_ADDR addr;
  int aflag;
  int dflag;
  int override;
  uint8_t modrm;
  uint8_t mod, reg, rm;
  int ot;
  uint8_t rex_x;
  uint8_t rex_b;
  int rip_offset;
  int popl_esp_hack;
  const int *regmap;
};

/* Parse the "modrm" part of the memory address irp->addr points at.
   Returns -1 if something goes wrong, 0 otherwise.  */

static int
i386_record_modrm (struct i386_record_s *irp)
{
  struct gdbarch *gdbarch = irp->gdbarch;

  if (record_read_memory (gdbarch, irp->addr, &irp->modrm, 1))
    return -1;

  irp->addr++;
  irp->mod = (irp->modrm >> 6) & 3;
  irp->reg = (irp->modrm >> 3) & 7;
  irp->rm = irp->modrm & 7;

  return 0;
}

/* Extract the memory address that the current instruction writes to,
   and return it in *ADDR.  Return -1 if something goes wrong.  */

static int
i386_record_lea_modrm_addr (struct i386_record_s *irp, uint64_t *addr)
{
  struct gdbarch *gdbarch = irp->gdbarch;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[4];
  ULONGEST offset64;

  *addr = 0;
  if (irp->aflag)
    {
      /* 32 bits */
      int havesib = 0;
      uint8_t scale = 0;
      uint8_t byte;
      uint8_t index = 0;
      uint8_t base = irp->rm;

      if (base == 4)
	{
	  havesib = 1;
	  if (record_read_memory (gdbarch, irp->addr, &byte, 1))
	    return -1;
	  irp->addr++;
	  scale = (byte >> 6) & 3;
	  index = ((byte >> 3) & 7) | irp->rex_x;
	  base = (byte & 7);
	}
      base |= irp->rex_b;

      switch (irp->mod)
	{
	case 0:
	  if ((base & 7) == 5)
	    {
	      base = 0xff;
	      if (record_read_memory (gdbarch, irp->addr, buf, 4))
		return -1;
	      irp->addr += 4;
	      *addr = extract_signed_integer (buf, 4, byte_order);
	      if (irp->regmap[X86_RECORD_R8_REGNUM] && !havesib)
		*addr += irp->addr + irp->rip_offset;
	    }
	  break;
	case 1:
	  if (record_read_memory (gdbarch, irp->addr, buf, 1))
	    return -1;
	  irp->addr++;
	  *addr = (int8_t) buf[0];
	  break;
	case 2:
	  if (record_read_memory (gdbarch, irp->addr, buf, 4))
	    return -1;
	  *addr = extract_signed_integer (buf, 4, byte_order);
	  irp->addr += 4;
	  break;
	}

      offset64 = 0;
      if (base != 0xff)
        {
	  if (base == 4 && irp->popl_esp_hack)
	    *addr += irp->popl_esp_hack;
	  regcache_raw_read_unsigned (irp->regcache, irp->regmap[base],
                                      &offset64);
	}
      if (irp->aflag == 2)
        {
	  *addr += offset64;
        }
      else
        *addr = (uint32_t) (offset64 + *addr);

      if (havesib && (index != 4 || scale != 0))
	{
	  regcache_raw_read_unsigned (irp->regcache, irp->regmap[index],
                                      &offset64);
	  if (irp->aflag == 2)
	    *addr += offset64 << scale;
	  else
	    *addr = (uint32_t) (*addr + (offset64 << scale));
	}
    }
  else
    {
      /* 16 bits */
      switch (irp->mod)
	{
	case 0:
	  if (irp->rm == 6)
	    {
	      if (record_read_memory (gdbarch, irp->addr, buf, 2))
		return -1;
	      irp->addr += 2;
	      *addr = extract_signed_integer (buf, 2, byte_order);
	      irp->rm = 0;
	      goto no_rm;
	    }
	  break;
	case 1:
	  if (record_read_memory (gdbarch, irp->addr, buf, 1))
	    return -1;
	  irp->addr++;
	  *addr = (int8_t) buf[0];
	  break;
	case 2:
	  if (record_read_memory (gdbarch, irp->addr, buf, 2))
	    return -1;
	  irp->addr += 2;
	  *addr = extract_signed_integer (buf, 2, byte_order);
	  break;
	}

      switch (irp->rm)
	{
	case 0:
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REBX_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_RESI_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  break;
	case 1:
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REBX_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REDI_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  break;
	case 2:
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REBP_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_RESI_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  break;
	case 3:
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REBP_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REDI_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  break;
	case 4:
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_RESI_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  break;
	case 5:
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REDI_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  break;
	case 6:
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REBP_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  break;
	case 7:
	  regcache_raw_read_unsigned (irp->regcache,
				      irp->regmap[X86_RECORD_REBX_REGNUM],
                                      &offset64);
	  *addr = (uint32_t) (*addr + offset64);
	  break;
	}
      *addr &= 0xffff;
    }

 no_rm:
  return 0;
}

/* Record the address and contents of the memory that will be changed
   by the current instruction.  Return -1 if something goes wrong, 0
   otherwise.  */

static int
i386_record_lea_modrm (struct i386_record_s *irp)
{
  struct gdbarch *gdbarch = irp->gdbarch;
  uint64_t addr;

  if (irp->override >= 0)
    {
      if (record_full_memory_query)
        {
	  int q;

          target_terminal_ours ();
          q = yquery (_("\
Process record ignores the memory change of instruction at address %s\n\
because it can't get the value of the segment register.\n\
Do you want to stop the program?"),
                      paddress (gdbarch, irp->orig_addr));
            target_terminal_inferior ();
            if (q)
              return -1;
        }

      return 0;
    }

  if (i386_record_lea_modrm_addr (irp, &addr))
    return -1;

  if (record_full_arch_list_add_mem (addr, 1 << irp->ot))
    return -1;

  return 0;
}

/* Record the effects of a push operation.  Return -1 if something
   goes wrong, 0 otherwise.  */

static int
i386_record_push (struct i386_record_s *irp, int size)
{
  ULONGEST addr;

  if (record_full_arch_list_add_reg (irp->regcache,
				     irp->regmap[X86_RECORD_RESP_REGNUM]))
    return -1;
  regcache_raw_read_unsigned (irp->regcache,
			      irp->regmap[X86_RECORD_RESP_REGNUM],
			      &addr);
  if (record_full_arch_list_add_mem ((CORE_ADDR) addr - size, size))
    return -1;

  return 0;
}


/* Defines contents to record.  */
#define I386_SAVE_FPU_REGS              0xfffd
#define I386_SAVE_FPU_ENV               0xfffe
#define I386_SAVE_FPU_ENV_REG_STACK     0xffff

/* Record the values of the floating point registers which will be
   changed by the current instruction.  Returns -1 if something is
   wrong, 0 otherwise.  */

static int i386_record_floats (struct gdbarch *gdbarch,
                               struct i386_record_s *ir,
                               uint32_t iregnum)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int i;

  /* Oza: Because of floating point insn push/pop of fpu stack is going to
     happen.  Currently we store st0-st7 registers, but we need not store all
     registers all the time, in future we use ftag register and record only
     those who are not marked as an empty.  */

  if (I386_SAVE_FPU_REGS == iregnum)
    {
      for (i = I387_ST0_REGNUM (tdep); i <= I387_ST0_REGNUM (tdep) + 7; i++)
        {
          if (record_full_arch_list_add_reg (ir->regcache, i))
            return -1;
        }
    }
  else if (I386_SAVE_FPU_ENV == iregnum)
    {
      for (i = I387_FCTRL_REGNUM (tdep); i <= I387_FOP_REGNUM (tdep); i++)
	      {
	      if (record_full_arch_list_add_reg (ir->regcache, i))
	        return -1;
	      }
    }
  else if (I386_SAVE_FPU_ENV_REG_STACK == iregnum)
    {
      for (i = I387_ST0_REGNUM (tdep); i <= I387_FOP_REGNUM (tdep); i++)
      {
        if (record_full_arch_list_add_reg (ir->regcache, i))
          return -1;
      }
    }
  else if ((iregnum >= I387_ST0_REGNUM (tdep)) &&
           (iregnum <= I387_FOP_REGNUM (tdep)))
    {
      if (record_full_arch_list_add_reg (ir->regcache,iregnum))
        return -1;
    }
  else
    {
      /* Parameter error.  */
      return -1;
    }
  if(I386_SAVE_FPU_ENV != iregnum)
    {
    for (i = I387_FCTRL_REGNUM (tdep); i <= I387_FOP_REGNUM (tdep); i++)
      {
      if (record_full_arch_list_add_reg (ir->regcache, i))
        return -1;
      }
    }
  return 0;
}

/* Parse the current instruction, and record the values of the
   registers and memory that will be changed by the current
   instruction.  Returns -1 if something goes wrong, 0 otherwise.  */

#define I386_RECORD_FULL_ARCH_LIST_ADD_REG(regnum) \
    record_full_arch_list_add_reg (ir.regcache, ir.regmap[(regnum)])

int
i386_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		     CORE_ADDR input_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int prefixes = 0;
  int regnum = 0;
  uint32_t opcode;
  uint8_t opcode8;
  ULONGEST addr;
  gdb_byte buf[MAX_REGISTER_SIZE];
  struct i386_record_s ir;
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  uint8_t rex_w = -1;
  uint8_t rex_r = 0;

  memset (&ir, 0, sizeof (struct i386_record_s));
  ir.regcache = regcache;
  ir.addr = input_addr;
  ir.orig_addr = input_addr;
  ir.aflag = 1;
  ir.dflag = 1;
  ir.override = -1;
  ir.popl_esp_hack = 0;
  ir.regmap = tdep->record_regmap;
  ir.gdbarch = gdbarch;

  if (record_debug > 1)
    fprintf_unfiltered (gdb_stdlog, "Process record: i386_process_record "
			            "addr = %s\n",
			paddress (gdbarch, ir.addr));

  /* prefixes */
  while (1)
    {
      if (record_read_memory (gdbarch, ir.addr, &opcode8, 1))
	return -1;
      ir.addr++;
      switch (opcode8)	/* Instruction prefixes */
	{
	case REPE_PREFIX_OPCODE:
	  prefixes |= PREFIX_REPZ;
	  break;
	case REPNE_PREFIX_OPCODE:
	  prefixes |= PREFIX_REPNZ;
	  break;
	case LOCK_PREFIX_OPCODE:
	  prefixes |= PREFIX_LOCK;
	  break;
	case CS_PREFIX_OPCODE:
	  ir.override = X86_RECORD_CS_REGNUM;
	  break;
	case SS_PREFIX_OPCODE:
	  ir.override = X86_RECORD_SS_REGNUM;
	  break;
	case DS_PREFIX_OPCODE:
	  ir.override = X86_RECORD_DS_REGNUM;
	  break;
	case ES_PREFIX_OPCODE:
	  ir.override = X86_RECORD_ES_REGNUM;
	  break;
	case FS_PREFIX_OPCODE:
	  ir.override = X86_RECORD_FS_REGNUM;
	  break;
	case GS_PREFIX_OPCODE:
	  ir.override = X86_RECORD_GS_REGNUM;
	  break;
	case DATA_PREFIX_OPCODE:
	  prefixes |= PREFIX_DATA;
	  break;
	case ADDR_PREFIX_OPCODE:
	  prefixes |= PREFIX_ADDR;
	  break;
        case 0x40:	/* i386 inc %eax */
        case 0x41:	/* i386 inc %ecx */
        case 0x42:	/* i386 inc %edx */
        case 0x43:	/* i386 inc %ebx */
        case 0x44:	/* i386 inc %esp */
        case 0x45:	/* i386 inc %ebp */
        case 0x46:	/* i386 inc %esi */
        case 0x47:	/* i386 inc %edi */
        case 0x48:	/* i386 dec %eax */
        case 0x49:	/* i386 dec %ecx */
        case 0x4a:	/* i386 dec %edx */
        case 0x4b:	/* i386 dec %ebx */
        case 0x4c:	/* i386 dec %esp */
        case 0x4d:	/* i386 dec %ebp */
        case 0x4e:	/* i386 dec %esi */
        case 0x4f:	/* i386 dec %edi */
          if (ir.regmap[X86_RECORD_R8_REGNUM])	/* 64 bit target */
            {
               /* REX */
               rex_w = (opcode8 >> 3) & 1;
               rex_r = (opcode8 & 0x4) << 1;
               ir.rex_x = (opcode8 & 0x2) << 2;
               ir.rex_b = (opcode8 & 0x1) << 3;
            }
	  else					/* 32 bit target */
	    goto out_prefixes;
          break;
	default:
	  goto out_prefixes;
	  break;
	}
    }
 out_prefixes:
  if (ir.regmap[X86_RECORD_R8_REGNUM] && rex_w == 1)
    {
      ir.dflag = 2;
    }
  else
    {
      if (prefixes & PREFIX_DATA)
        ir.dflag ^= 1;
    }
  if (prefixes & PREFIX_ADDR)
    ir.aflag ^= 1;
  else if (ir.regmap[X86_RECORD_R8_REGNUM])
    ir.aflag = 2;

  /* Now check op code.  */
  opcode = (uint32_t) opcode8;
 reswitch:
  switch (opcode)
    {
    case 0x0f:
      if (record_read_memory (gdbarch, ir.addr, &opcode8, 1))
	return -1;
      ir.addr++;
      opcode = (uint32_t) opcode8 | 0x0f00;
      goto reswitch;
      break;

    case 0x00:    /* arith & logic */
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x0c:
    case 0x0d:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
    case 0x1d:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x38:
    case 0x39:
    case 0x3a:
    case 0x3b:
    case 0x3c:
    case 0x3d:
      if (((opcode >> 3) & 7) != OP_CMPL)
	{
	  if ((opcode & 1) == 0)
	    ir.ot = OT_BYTE;
	  else
	    ir.ot = ir.dflag + OT_WORD;

	  switch ((opcode >> 1) & 3)
	    {
	    case 0:    /* OP Ev, Gv */
	      if (i386_record_modrm (&ir))
		return -1;
	      if (ir.mod != 3)
		{
		  if (i386_record_lea_modrm (&ir))
		    return -1;
		}
	      else
		{
                  ir.rm |= ir.rex_b;
		  if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
		    ir.rm &= 0x3;
		  I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm);
		}
	      break;
	    case 1:    /* OP Gv, Ev */
	      if (i386_record_modrm (&ir))
		return -1;
              ir.reg |= rex_r;
	      if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
		ir.reg &= 0x3;
	      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
	      break;
	    case 2:    /* OP A, Iv */
	      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
	      break;
	    }
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x80:    /* GRP1 */
    case 0x81:
    case 0x82:
    case 0x83:
      if (i386_record_modrm (&ir))
	return -1;

      if (ir.reg != OP_CMPL)
	{
	  if ((opcode & 1) == 0)
	    ir.ot = OT_BYTE;
	  else
	    ir.ot = ir.dflag + OT_WORD;

	  if (ir.mod != 3)
	    {
              if (opcode == 0x83)
                ir.rip_offset = 1;
              else
                ir.rip_offset = (ir.ot > OT_LONG) ? 4 : (1 << ir.ot);
	      if (i386_record_lea_modrm (&ir))
		return -1;
	    }
	  else
	    I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm | ir.rex_b);
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x40:      /* inc */
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:

    case 0x48:      /* dec */
    case 0x49:
    case 0x4a:
    case 0x4b:
    case 0x4c:
    case 0x4d:
    case 0x4e:
    case 0x4f:

      I386_RECORD_FULL_ARCH_LIST_ADD_REG (opcode & 7);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0xf6:    /* GRP3 */
    case 0xf7:
      if ((opcode & 1) == 0)
	ir.ot = OT_BYTE;
      else
	ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;

      if (ir.mod != 3 && ir.reg == 0)
        ir.rip_offset = (ir.ot > OT_LONG) ? 4 : (1 << ir.ot);

      switch (ir.reg)
	{
	case 0:    /* test */
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	case 2:    /* not */
	case 3:    /* neg */
	  if (ir.mod != 3)
	    {
	      if (i386_record_lea_modrm (&ir))
		return -1;
	    }
	  else
	    {
              ir.rm |= ir.rex_b;
	      if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
		ir.rm &= 0x3;
	      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm);
	    }
	  if (ir.reg == 3)  /* neg */
	    I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	case 4:    /* mul  */
	case 5:    /* imul */
	case 6:    /* div  */
	case 7:    /* idiv */
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
	  if (ir.ot != OT_BYTE)
	    I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDX_REGNUM);
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	default:
	  ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	  break;
	}
      break;

    case 0xfe:    /* GRP4 */
    case 0xff:    /* GRP5 */
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.reg >= 2 && opcode == 0xfe)
	{
	  ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}
      switch (ir.reg)
	{
	case 0:    /* inc */
	case 1:    /* dec */
          if ((opcode & 1) == 0)
	    ir.ot = OT_BYTE;
          else
	    ir.ot = ir.dflag + OT_WORD;
	  if (ir.mod != 3)
	    {
	      if (i386_record_lea_modrm (&ir))
		return -1;
	    }
	  else
	    {
	      ir.rm |= ir.rex_b;
	      if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
		ir.rm &= 0x3;
	      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm);
	    }
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	case 2:    /* call */
          if (ir.regmap[X86_RECORD_R8_REGNUM] && ir.dflag)
            ir.dflag = 2;
	  if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
	    return -1;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	case 3:    /* lcall */
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_CS_REGNUM);
	  if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
	    return -1;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	case 4:    /* jmp  */
	case 5:    /* ljmp */
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	case 6:    /* push */
          if (ir.regmap[X86_RECORD_R8_REGNUM] && ir.dflag)
            ir.dflag = 2;
	  if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
	    return -1;
	  break;
	default:
	  ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	  break;
	}
      break;

    case 0x84:    /* test */
    case 0x85:
    case 0xa8:
    case 0xa9:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x98:    /* CWDE/CBW */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      break;

    case 0x99:    /* CDQ/CWD */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDX_REGNUM);
      break;

    case 0x0faf:  /* imul */
    case 0x69:
    case 0x6b:
      ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;
      if (opcode == 0x69)
        ir.rip_offset = (ir.ot > OT_LONG) ? 4 : (1 << ir.ot);
      else if (opcode == 0x6b)
        ir.rip_offset = 1;
      ir.reg |= rex_r;
      if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	ir.reg &= 0x3;
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fc0:  /* xadd */
    case 0x0fc1:
      if ((opcode & 1) == 0)
	ir.ot = OT_BYTE;
      else
	ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;
      ir.reg |= rex_r;
      if (ir.mod == 3)
	{
	  if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	    ir.reg &= 0x3;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
	  if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	    ir.rm &= 0x3;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm);
	}
      else
	{
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	  if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	    ir.reg &= 0x3;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fb0:  /* cmpxchg */
    case 0x0fb1:
      if ((opcode & 1) == 0)
	ir.ot = OT_BYTE;
      else
	ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
	{
          ir.reg |= rex_r;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
	  if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	    ir.reg &= 0x3;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
	}
      else
	{
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fc7:    /* cmpxchg8b */
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
	{
	  ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDX_REGNUM);
      if (i386_record_lea_modrm (&ir))
	return -1;
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x50:    /* push */
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x68:
    case 0x6a:
      if (ir.regmap[X86_RECORD_R8_REGNUM] && ir.dflag)
        ir.dflag = 2;
      if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
	return -1;
      break;

    case 0x06:    /* push es */
    case 0x0e:    /* push cs */
    case 0x16:    /* push ss */
    case 0x1e:    /* push ds */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
	  ir.addr -= 1;
	  goto no_support;
	}
      if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
	return -1;
      break;

    case 0x0fa0:    /* push fs */
    case 0x0fa8:    /* push gs */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
	  ir.addr -= 2;
	  goto no_support;
	}
      if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
	return -1;
      break;

    case 0x60:    /* pusha */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
	  ir.addr -= 1;
	  goto no_support;
	}
      if (i386_record_push (&ir, 1 << (ir.dflag + 4)))
	return -1;
      break;

    case 0x58:    /* pop */
    case 0x59:
    case 0x5a:
    case 0x5b:
    case 0x5c:
    case 0x5d:
    case 0x5e:
    case 0x5f:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG ((opcode & 0x7) | ir.rex_b);
      break;

    case 0x61:    /* popa */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
	  ir.addr -= 1;
	  goto no_support;
	}
      for (regnum = X86_RECORD_REAX_REGNUM; 
	   regnum <= X86_RECORD_REDI_REGNUM;
	   regnum++)
	I386_RECORD_FULL_ARCH_LIST_ADD_REG (regnum);
      break;

    case 0x8f:    /* pop */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
	ir.ot = ir.dflag ? OT_QUAD : OT_WORD;
      else
        ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
	I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm | ir.rex_b);
      else
	{
          ir.popl_esp_hack = 1 << ir.ot;
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      break;

    case 0xc8:    /* enter */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REBP_REGNUM);
      if (ir.regmap[X86_RECORD_R8_REGNUM] && ir.dflag)
        ir.dflag = 2;
      if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
	return -1;
      break;

    case 0xc9:    /* leave */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REBP_REGNUM);
      break;

    case 0x07:    /* pop es */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
	  ir.addr -= 1;
	  goto no_support;
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_ES_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x17:    /* pop ss */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
	  ir.addr -= 1;
	  goto no_support;
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_SS_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x1f:    /* pop ds */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
	  ir.addr -= 1;
	  goto no_support;
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_DS_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fa1:    /* pop fs */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_FS_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fa9:    /* pop gs */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_GS_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x88:    /* mov */
    case 0x89:
    case 0xc6:
    case 0xc7:
      if ((opcode & 1) == 0)
	ir.ot = OT_BYTE;
      else
	ir.ot = ir.dflag + OT_WORD;

      if (i386_record_modrm (&ir))
	return -1;

      if (ir.mod != 3)
	{
          if (opcode == 0xc6 || opcode == 0xc7)
	    ir.rip_offset = (ir.ot > OT_LONG) ? 4 : (1 << ir.ot);
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	}
      else
	{
          if (opcode == 0xc6 || opcode == 0xc7)
	    ir.rm |= ir.rex_b;
	  if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	    ir.rm &= 0x3;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm);
	}
      break;

    case 0x8a:    /* mov */
    case 0x8b:
      if ((opcode & 1) == 0)
	ir.ot = OT_BYTE;
      else
	ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;
      ir.reg |= rex_r;
      if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	ir.reg &= 0x3;
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
      break;

    case 0x8c:    /* mov seg */
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.reg > 5)
	{
	  ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}

      if (ir.mod == 3)
	I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm);
      else
	{
	  ir.ot = OT_WORD;
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	}
      break;

    case 0x8e:    /* mov seg */
      if (i386_record_modrm (&ir))
	return -1;
      switch (ir.reg)
	{
	case 0:
	  regnum = X86_RECORD_ES_REGNUM;
	  break;
	case 2:
	  regnum = X86_RECORD_SS_REGNUM;
	  break;
	case 3:
	  regnum = X86_RECORD_DS_REGNUM;
	  break;
	case 4:
	  regnum = X86_RECORD_FS_REGNUM;
	  break;
	case 5:
	  regnum = X86_RECORD_GS_REGNUM;
	  break;
	default:
	  ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	  break;
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (regnum);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fb6:    /* movzbS */
    case 0x0fb7:    /* movzwS */
    case 0x0fbe:    /* movsbS */
    case 0x0fbf:    /* movswS */
      if (i386_record_modrm (&ir))
	return -1;
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg | rex_r);
      break;

    case 0x8d:      /* lea */
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
	{
	  ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}
      ir.ot = ir.dflag;
      ir.reg |= rex_r;
      if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	ir.reg &= 0x3;
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
      break;

    case 0xa0:    /* mov EAX */
    case 0xa1:

    case 0xd7:    /* xlat */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      break;

    case 0xa2:    /* mov EAX */
    case 0xa3:
      if (ir.override >= 0)
        {
          if (record_full_memory_query)
            {
	      int q;

              target_terminal_ours ();
              q = yquery (_("\
Process record ignores the memory change of instruction at address %s\n\
because it can't get the value of the segment register.\n\
Do you want to stop the program?"),
                          paddress (gdbarch, ir.orig_addr));
              target_terminal_inferior ();
              if (q)
                return -1;
            }
	}
      else
	{
          if ((opcode & 1) == 0)
	    ir.ot = OT_BYTE;
	  else
	    ir.ot = ir.dflag + OT_WORD;
	  if (ir.aflag == 2)
	    {
              if (record_read_memory (gdbarch, ir.addr, buf, 8))
		return -1;
	      ir.addr += 8;
	      addr = extract_unsigned_integer (buf, 8, byte_order);
	    }
          else if (ir.aflag)
	    {
              if (record_read_memory (gdbarch, ir.addr, buf, 4))
		return -1;
	      ir.addr += 4;
              addr = extract_unsigned_integer (buf, 4, byte_order);
	    }
          else
	    {
              if (record_read_memory (gdbarch, ir.addr, buf, 2))
		return -1;
	      ir.addr += 2;
              addr = extract_unsigned_integer (buf, 2, byte_order);
	    }
	  if (record_full_arch_list_add_mem (addr, 1 << ir.ot))
	    return -1;
        }
      break;

    case 0xb0:    /* mov R, Ib */
    case 0xb1:
    case 0xb2:
    case 0xb3:
    case 0xb4:
    case 0xb5:
    case 0xb6:
    case 0xb7:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG ((ir.regmap[X86_RECORD_R8_REGNUM])
					  ? ((opcode & 0x7) | ir.rex_b)
					  : ((opcode & 0x7) & 0x3));
      break;

    case 0xb8:    /* mov R, Iv */
    case 0xb9:
    case 0xba:
    case 0xbb:
    case 0xbc:
    case 0xbd:
    case 0xbe:
    case 0xbf:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG ((opcode & 0x7) | ir.rex_b);
      break;

    case 0x91:    /* xchg R, EAX */
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (opcode & 0x7);
      break;

    case 0x86:    /* xchg Ev, Gv */
    case 0x87:
      if ((opcode & 1) == 0)
	ir.ot = OT_BYTE;
      else
	ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
	{
	  ir.rm |= ir.rex_b;
	  if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	    ir.rm &= 0x3;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm);
	}
      else
	{
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	}
      ir.reg |= rex_r;
      if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	ir.reg &= 0x3;
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
      break;

    case 0xc4:    /* les Gv */
    case 0xc5:    /* lds Gv */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
	  ir.addr -= 1;
	  goto no_support;
	}
      /* FALLTHROUGH */
    case 0x0fb2:    /* lss Gv */
    case 0x0fb4:    /* lfs Gv */
    case 0x0fb5:    /* lgs Gv */
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
	{
	  if (opcode > 0xff)
	    ir.addr -= 3;
	  else
	    ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}
      switch (opcode)
	{
	case 0xc4:    /* les Gv */
	  regnum = X86_RECORD_ES_REGNUM;
	  break;
	case 0xc5:    /* lds Gv */
	  regnum = X86_RECORD_DS_REGNUM;
	  break;
	case 0x0fb2:  /* lss Gv */
	  regnum = X86_RECORD_SS_REGNUM;
	  break;
	case 0x0fb4:  /* lfs Gv */
	  regnum = X86_RECORD_FS_REGNUM;
	  break;
	case 0x0fb5:  /* lgs Gv */
	  regnum = X86_RECORD_GS_REGNUM;
	  break;
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (regnum);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg | rex_r);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0xc0:    /* shifts */
    case 0xc1:
    case 0xd0:
    case 0xd1:
    case 0xd2:
    case 0xd3:
      if ((opcode & 1) == 0)
	ir.ot = OT_BYTE;
      else
	ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod != 3 && (opcode == 0xd2 || opcode == 0xd3))
	{
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	}
      else
	{
	  ir.rm |= ir.rex_b;
	  if (ir.ot == OT_BYTE && !ir.regmap[X86_RECORD_R8_REGNUM])
	    ir.rm &= 0x3;
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm);
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fa4:
    case 0x0fa5:
    case 0x0fac:
    case 0x0fad:
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
	{
	  if (record_full_arch_list_add_reg (ir.regcache, ir.rm))
	    return -1;
	}
      else
	{
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	}
      break;

    case 0xd8:    /* Floats.  */
    case 0xd9:
    case 0xda:
    case 0xdb:
    case 0xdc:
    case 0xdd:
    case 0xde:
    case 0xdf:
      if (i386_record_modrm (&ir))
	return -1;
      ir.reg |= ((opcode & 7) << 3);
      if (ir.mod != 3)
	{
	  /* Memory.  */
	  uint64_t addr64;

	  if (i386_record_lea_modrm_addr (&ir, &addr64))
	    return -1;
	  switch (ir.reg)
	    {
	    case 0x02:
            case 0x12:
            case 0x22:
            case 0x32:
	      /* For fcom, ficom nothing to do.  */
              break;
	    case 0x03:
            case 0x13:
            case 0x23:
            case 0x33:
	      /* For fcomp, ficomp pop FPU stack, store all.  */
              if (i386_record_floats (gdbarch, &ir, I386_SAVE_FPU_REGS))
                return -1;
              break;
            case 0x00:
            case 0x01:
	    case 0x04:
	    case 0x05:
	    case 0x06:
	    case 0x07:
	    case 0x10:
	    case 0x11:
	    case 0x14:
	    case 0x15:
	    case 0x16:
	    case 0x17:
	    case 0x20:
	    case 0x21:
	    case 0x24:
	    case 0x25:
	    case 0x26:
	    case 0x27:
	    case 0x30:
	    case 0x31:
	    case 0x34:
	    case 0x35:
	    case 0x36:
	    case 0x37:
              /* For fadd, fmul, fsub, fsubr, fdiv, fdivr, fiadd, fimul,
                 fisub, fisubr, fidiv, fidivr, modR/M.reg is an extension
                 of code,  always affects st(0) register.  */
              if (i386_record_floats (gdbarch, &ir, I387_ST0_REGNUM (tdep)))
                return -1;
	      break;
	    case 0x08:
	    case 0x0a:
	    case 0x0b:
	    case 0x18:
	    case 0x19:
	    case 0x1a:
	    case 0x1b:
            case 0x1d:
	    case 0x28:
	    case 0x29:
	    case 0x2a:
	    case 0x2b:
	    case 0x38:
	    case 0x39:
	    case 0x3a:
	    case 0x3b:
            case 0x3c:
            case 0x3d:
	      switch (ir.reg & 7)
		{
		case 0:
		  /* Handling fld, fild.  */
		  if (i386_record_floats (gdbarch, &ir, I386_SAVE_FPU_REGS))
		    return -1;
		  break;
		case 1:
		  switch (ir.reg >> 4)
		    {
		    case 0:
		      if (record_full_arch_list_add_mem (addr64, 4))
			return -1;
		      break;
		    case 2:
		      if (record_full_arch_list_add_mem (addr64, 8))
			return -1;
		      break;
		    case 3:
		      break;
		    default:
		      if (record_full_arch_list_add_mem (addr64, 2))
			return -1;
		      break;
		    }
		  break;
		default:
		  switch (ir.reg >> 4)
		    {
		    case 0:
		      if (record_full_arch_list_add_mem (addr64, 4))
			return -1;
		      if (3 == (ir.reg & 7))
			{
			  /* For fstp m32fp.  */
			  if (i386_record_floats (gdbarch, &ir,
						  I386_SAVE_FPU_REGS))
			    return -1;
			}
		      break;
		    case 1:
		      if (record_full_arch_list_add_mem (addr64, 4))
			return -1;
		      if ((3 == (ir.reg & 7))
			  || (5 == (ir.reg & 7))
			  || (7 == (ir.reg & 7)))
			{
			  /* For fstp insn.  */
			  if (i386_record_floats (gdbarch, &ir,
						  I386_SAVE_FPU_REGS))
			    return -1;
			}
		      break;
		    case 2:
		      if (record_full_arch_list_add_mem (addr64, 8))
			return -1;
		      if (3 == (ir.reg & 7))
			{
			  /* For fstp m64fp.  */
			  if (i386_record_floats (gdbarch, &ir,
						  I386_SAVE_FPU_REGS))
			    return -1;
			}
		      break;
		    case 3:
		      if ((3 <= (ir.reg & 7)) && (6 <= (ir.reg & 7)))
			{
			  /* For fistp, fbld, fild, fbstp.  */
			  if (i386_record_floats (gdbarch, &ir,
						  I386_SAVE_FPU_REGS))
			    return -1;
			}
		      /* Fall through */
		    default:
		      if (record_full_arch_list_add_mem (addr64, 2))
			return -1;
		      break;
		    }
		  break;
		}
	      break;
	    case 0x0c:
              /* Insn fldenv.  */
              if (i386_record_floats (gdbarch, &ir,
                                      I386_SAVE_FPU_ENV_REG_STACK))
                return -1;
              break;
	    case 0x0d:
              /* Insn fldcw.  */
              if (i386_record_floats (gdbarch, &ir, I387_FCTRL_REGNUM (tdep)))
                return -1;
              break;
	    case 0x2c:
              /* Insn frstor.  */
              if (i386_record_floats (gdbarch, &ir,
                                      I386_SAVE_FPU_ENV_REG_STACK))
                return -1;
	      break;
	    case 0x0e:
	      if (ir.dflag)
		{
		  if (record_full_arch_list_add_mem (addr64, 28))
		    return -1;
		}
	      else
		{
		  if (record_full_arch_list_add_mem (addr64, 14))
		    return -1;
		}
	      break;
	    case 0x0f:
	    case 0x2f:
	      if (record_full_arch_list_add_mem (addr64, 2))
		return -1;
              /* Insn fstp, fbstp.  */
              if (i386_record_floats (gdbarch, &ir, I386_SAVE_FPU_REGS))
                return -1;
	      break;
	    case 0x1f:
	    case 0x3e:
	      if (record_full_arch_list_add_mem (addr64, 10))
		return -1;
	      break;
	    case 0x2e:
	      if (ir.dflag)
		{
		  if (record_full_arch_list_add_mem (addr64, 28))
		    return -1;
		  addr64 += 28;
		}
	      else
		{
		  if (record_full_arch_list_add_mem (addr64, 14))
		    return -1;
		  addr64 += 14;
		}
	      if (record_full_arch_list_add_mem (addr64, 80))
		return -1;
	      /* Insn fsave.  */
	      if (i386_record_floats (gdbarch, &ir,
				      I386_SAVE_FPU_ENV_REG_STACK))
		return -1;
	      break;
	    case 0x3f:
	      if (record_full_arch_list_add_mem (addr64, 8))
		return -1;
	      /* Insn fistp.  */
	      if (i386_record_floats (gdbarch, &ir, I386_SAVE_FPU_REGS))
		return -1;
	      break;
	    default:
	      ir.addr -= 2;
	      opcode = opcode << 8 | ir.modrm;
	      goto no_support;
	      break;
	    }
	}
      /* Opcode is an extension of modR/M byte.  */
      else
        {
	  switch (opcode)
	    {
	    case 0xd8:
	      if (i386_record_floats (gdbarch, &ir, I387_ST0_REGNUM (tdep)))
		return -1;
	      break;
	    case 0xd9:
	      if (0x0c == (ir.modrm >> 4))
		{
		  if ((ir.modrm & 0x0f) <= 7)
		    {
		      if (i386_record_floats (gdbarch, &ir,
					      I386_SAVE_FPU_REGS))
			return -1;
		    }
                  else
		    {
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep)))
			return -1;
		      /* If only st(0) is changing, then we have already
			 recorded.  */
		      if ((ir.modrm & 0x0f) - 0x08)
			{
			  if (i386_record_floats (gdbarch, &ir,
						  I387_ST0_REGNUM (tdep) +
						  ((ir.modrm & 0x0f) - 0x08)))
			    return -1;
			}
		    }
		}
              else
                {
		  switch (ir.modrm)
		    {
		    case 0xe0:
		    case 0xe1:
		    case 0xf0:
		    case 0xf5:
		    case 0xf8:
		    case 0xfa:
		    case 0xfc:
		    case 0xfe:
		    case 0xff:
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep)))
			return -1;
		      break;
		    case 0xf1:
		    case 0xf2:
		    case 0xf3:
		    case 0xf4:
		    case 0xf6:
		    case 0xf7:
		    case 0xe8:
		    case 0xe9:
		    case 0xea:
		    case 0xeb:
		    case 0xec:
		    case 0xed:
		    case 0xee:
		    case 0xf9:
		    case 0xfb:
		      if (i386_record_floats (gdbarch, &ir,
					      I386_SAVE_FPU_REGS))
			return -1;
		      break;
		    case 0xfd:
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep)))
			return -1;
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep) + 1))
			return -1;
		      break;
		    }
		}
              break;
            case 0xda:
              if (0xe9 == ir.modrm)
                {
		  if (i386_record_floats (gdbarch, &ir, I386_SAVE_FPU_REGS))
		    return -1;
                }
              else if ((0x0c == ir.modrm >> 4) || (0x0d == ir.modrm >> 4))
                {
		  if (i386_record_floats (gdbarch, &ir,
					  I387_ST0_REGNUM (tdep)))
		    return -1;
		  if (((ir.modrm & 0x0f) > 0) && ((ir.modrm & 0x0f) <= 7))
		    {
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep) +
					      (ir.modrm & 0x0f)))
			return -1;
		    }
		  else if ((ir.modrm & 0x0f) - 0x08)
		    {
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep) +
					      ((ir.modrm & 0x0f) - 0x08)))
			return -1;
		    }
                }
              break;
            case 0xdb:
              if (0xe3 == ir.modrm)
                {
		  if (i386_record_floats (gdbarch, &ir, I386_SAVE_FPU_ENV))
		    return -1;
                }
              else if ((0x0c == ir.modrm >> 4) || (0x0d == ir.modrm >> 4))
                {
		  if (i386_record_floats (gdbarch, &ir,
					  I387_ST0_REGNUM (tdep)))
		    return -1;
		  if (((ir.modrm & 0x0f) > 0) && ((ir.modrm & 0x0f) <= 7))
		    {
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep) +
					      (ir.modrm & 0x0f)))
			return -1;
		    }
		  else if ((ir.modrm & 0x0f) - 0x08)
		    {
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep) +
					      ((ir.modrm & 0x0f) - 0x08)))
			return -1;
		    }
                }
              break;
            case 0xdc:
              if ((0x0c == ir.modrm >> 4)
		  || (0x0d == ir.modrm >> 4)
		  || (0x0f == ir.modrm >> 4))
                {
		  if ((ir.modrm & 0x0f) <= 7)
		    {
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep) +
					      (ir.modrm & 0x0f)))
			return -1;
		    }
		  else
		    {
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep) +
					      ((ir.modrm & 0x0f) - 0x08)))
			return -1;
		    }
                }
	      break;
            case 0xdd:
              if (0x0c == ir.modrm >> 4)
                {
                  if (i386_record_floats (gdbarch, &ir,
                                          I387_FTAG_REGNUM (tdep)))
                    return -1;
                }
              else if ((0x0d == ir.modrm >> 4) || (0x0e == ir.modrm >> 4))
                {
                  if ((ir.modrm & 0x0f) <= 7)
                    {
		      if (i386_record_floats (gdbarch, &ir,
					      I387_ST0_REGNUM (tdep) +
					      (ir.modrm & 0x0f)))
			return -1;
                    }
                  else
                    {
                      if (i386_record_floats (gdbarch, &ir,
					      I386_SAVE_FPU_REGS))
                        return -1;
                    }
                }
              break;
            case 0xde:
              if ((0x0c == ir.modrm >> 4)
		  || (0x0e == ir.modrm >> 4)
		  || (0x0f == ir.modrm >> 4)
		  || (0xd9 == ir.modrm))
                {
		  if (i386_record_floats (gdbarch, &ir, I386_SAVE_FPU_REGS))
		    return -1;
                }
              break;
            case 0xdf:
              if (0xe0 == ir.modrm)
                {
		  if (record_full_arch_list_add_reg (ir.regcache,
						     I386_EAX_REGNUM))
		    return -1;
                }
              else if ((0x0f == ir.modrm >> 4) || (0x0e == ir.modrm >> 4))
                {
		  if (i386_record_floats (gdbarch, &ir, I386_SAVE_FPU_REGS))
		    return -1;
                }
              break;
	    }
	}
      break;
      /* string ops */
    case 0xa4:    /* movsS */
    case 0xa5:
    case 0xaa:    /* stosS */
    case 0xab:
    case 0x6c:    /* insS */
    case 0x6d:
      regcache_raw_read_unsigned (ir.regcache,
                                  ir.regmap[X86_RECORD_RECX_REGNUM],
                                  &addr);
      if (addr)
        {
          ULONGEST es, ds;

          if ((opcode & 1) == 0)
	    ir.ot = OT_BYTE;
          else
	    ir.ot = ir.dflag + OT_WORD;
          regcache_raw_read_unsigned (ir.regcache,
                                      ir.regmap[X86_RECORD_REDI_REGNUM],
                                      &addr);

          regcache_raw_read_unsigned (ir.regcache,
                                      ir.regmap[X86_RECORD_ES_REGNUM],
                                      &es);
          regcache_raw_read_unsigned (ir.regcache,
                                      ir.regmap[X86_RECORD_DS_REGNUM],
                                      &ds);
          if (ir.aflag && (es != ds))
            {
              /* addr += ((uint32_t) read_register (I386_ES_REGNUM)) << 4; */
              if (record_full_memory_query)
                {
	          int q;

                  target_terminal_ours ();
                  q = yquery (_("\
Process record ignores the memory change of instruction at address %s\n\
because it can't get the value of the segment register.\n\
Do you want to stop the program?"),
                              paddress (gdbarch, ir.orig_addr));
                  target_terminal_inferior ();
                  if (q)
                    return -1;
                }
            }
          else
            {
              if (record_full_arch_list_add_mem (addr, 1 << ir.ot))
                return -1;
            }

          if (prefixes & (PREFIX_REPZ | PREFIX_REPNZ))
            I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RECX_REGNUM);
          if (opcode == 0xa4 || opcode == 0xa5)
            I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESI_REGNUM);
          I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDI_REGNUM);
          I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	}
      break;

    case 0xa6:    /* cmpsS */
    case 0xa7:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDI_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESI_REGNUM);
      if (prefixes & (PREFIX_REPZ | PREFIX_REPNZ))
        I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RECX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0xac:    /* lodsS */
    case 0xad:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESI_REGNUM);
      if (prefixes & (PREFIX_REPZ | PREFIX_REPNZ))
        I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RECX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0xae:    /* scasS */
    case 0xaf:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDI_REGNUM);
      if (prefixes & (PREFIX_REPZ | PREFIX_REPNZ))
        I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RECX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x6e:    /* outsS */
    case 0x6f:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESI_REGNUM);
      if (prefixes & (PREFIX_REPZ | PREFIX_REPNZ))
        I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RECX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0xe4:    /* port I/O */
    case 0xe5:
    case 0xec:
    case 0xed:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      break;

    case 0xe6:
    case 0xe7:
    case 0xee:
    case 0xef:
      break;

      /* control */
    case 0xc2:    /* ret im */
    case 0xc3:    /* ret */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0xca:    /* lret im */
    case 0xcb:    /* lret */
    case 0xcf:    /* iret */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_CS_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0xe8:    /* call im */
      if (ir.regmap[X86_RECORD_R8_REGNUM] && ir.dflag)
        ir.dflag = 2;
      if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
        return -1;
      break;

    case 0x9a:    /* lcall im */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
          ir.addr -= 1;
          goto no_support;
        }
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_CS_REGNUM);
      if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
        return -1;
      break;

    case 0xe9:    /* jmp im */
    case 0xea:    /* ljmp im */
    case 0xeb:    /* jmp Jb */
    case 0x70:    /* jcc Jb */
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7a:
    case 0x7b:
    case 0x7c:
    case 0x7d:
    case 0x7e:
    case 0x7f:
    case 0x0f80:  /* jcc Jv */
    case 0x0f81:
    case 0x0f82:
    case 0x0f83:
    case 0x0f84:
    case 0x0f85:
    case 0x0f86:
    case 0x0f87:
    case 0x0f88:
    case 0x0f89:
    case 0x0f8a:
    case 0x0f8b:
    case 0x0f8c:
    case 0x0f8d:
    case 0x0f8e:
    case 0x0f8f:
      break;

    case 0x0f90:  /* setcc Gv */
    case 0x0f91:
    case 0x0f92:
    case 0x0f93:
    case 0x0f94:
    case 0x0f95:
    case 0x0f96:
    case 0x0f97:
    case 0x0f98:
    case 0x0f99:
    case 0x0f9a:
    case 0x0f9b:
    case 0x0f9c:
    case 0x0f9d:
    case 0x0f9e:
    case 0x0f9f:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      ir.ot = OT_BYTE;
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
        I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rex_b ? (ir.rm | ir.rex_b)
					    : (ir.rm & 0x3));
      else
	{
	  if (i386_record_lea_modrm (&ir))
	    return -1;
	}
      break;

    case 0x0f40:    /* cmov Gv, Ev */
    case 0x0f41:
    case 0x0f42:
    case 0x0f43:
    case 0x0f44:
    case 0x0f45:
    case 0x0f46:
    case 0x0f47:
    case 0x0f48:
    case 0x0f49:
    case 0x0f4a:
    case 0x0f4b:
    case 0x0f4c:
    case 0x0f4d:
    case 0x0f4e:
    case 0x0f4f:
      if (i386_record_modrm (&ir))
	return -1;
      ir.reg |= rex_r;
      if (ir.dflag == OT_BYTE)
	ir.reg &= 0x3;
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
      break;

      /* flags */
    case 0x9c:    /* pushf */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      if (ir.regmap[X86_RECORD_R8_REGNUM] && ir.dflag)
        ir.dflag = 2;
      if (i386_record_push (&ir, 1 << (ir.dflag + 1)))
        return -1;
      break;

    case 0x9d:    /* popf */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x9e:    /* sahf */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
          ir.addr -= 1;
          goto no_support;
        }
      /* FALLTHROUGH */
    case 0xf5:    /* cmc */
    case 0xf8:    /* clc */
    case 0xf9:    /* stc */
    case 0xfc:    /* cld */
    case 0xfd:    /* std */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x9f:    /* lahf */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
          ir.addr -= 1;
          goto no_support;
        }
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      break;

      /* bit operations */
    case 0x0fba:    /* bt/bts/btr/btc Gv, im */
      ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.reg < 4)
	{
	  ir.addr -= 2;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}
      if (ir.reg != 4)
	{
          if (ir.mod == 3)
            I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm | ir.rex_b);
	  else
	    {
	      if (i386_record_lea_modrm (&ir))
		return -1;
	    }
	}
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fa3:    /* bt Gv, Ev */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fab:    /* bts */
    case 0x0fb3:    /* btr */
    case 0x0fbb:    /* btc */
      ir.ot = ir.dflag + OT_WORD;
      if (i386_record_modrm (&ir))
        return -1;
      if (ir.mod == 3)
        I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm | ir.rex_b);
      else
        {
          uint64_t addr64;
          if (i386_record_lea_modrm_addr (&ir, &addr64))
            return -1;
          regcache_raw_read_unsigned (ir.regcache,
                                      ir.regmap[ir.reg | rex_r],
                                      &addr);
          switch (ir.dflag)
            {
            case 0:
              addr64 += ((int16_t) addr >> 4) << 4;
              break;
            case 1:
              addr64 += ((int32_t) addr >> 5) << 5;
              break;
            case 2:
              addr64 += ((int64_t) addr >> 6) << 6;
              break;
            }
          if (record_full_arch_list_add_mem (addr64, 1 << ir.ot))
            return -1;
          if (i386_record_lea_modrm (&ir))
            return -1;
        }
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0fbc:    /* bsf */
    case 0x0fbd:    /* bsr */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg | rex_r);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

      /* bcd */
    case 0x27:    /* daa */
    case 0x2f:    /* das */
    case 0x37:    /* aaa */
    case 0x3f:    /* aas */
    case 0xd4:    /* aam */
    case 0xd5:    /* aad */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
          ir.addr -= 1;
          goto no_support;
        }
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

      /* misc */
    case 0x90:    /* nop */
      if (prefixes & PREFIX_LOCK)
	{
	  ir.addr -= 1;
	  goto no_support;
	}
      break;

    case 0x9b:    /* fwait */
      if (record_read_memory (gdbarch, ir.addr, &opcode8, 1))
	return -1;
      opcode = (uint32_t) opcode8;
      ir.addr++;
      goto reswitch;
      break;

      /* XXX */
    case 0xcc:    /* int3 */
      printf_unfiltered (_("Process record does not support instruction "
			   "int3.\n"));
      ir.addr -= 1;
      goto no_support;
      break;

      /* XXX */
    case 0xcd:    /* int */
      {
	int ret;
	uint8_t interrupt;
	if (record_read_memory (gdbarch, ir.addr, &interrupt, 1))
	  return -1;
	ir.addr++;
	if (interrupt != 0x80
	    || tdep->i386_intx80_record == NULL)
	  {
	    printf_unfiltered (_("Process record does not support "
				 "instruction int 0x%02x.\n"),
			       interrupt);
	    ir.addr -= 2;
	    goto no_support;
	  }
	ret = tdep->i386_intx80_record (ir.regcache);
	if (ret)
	  return ret;
      }
      break;

      /* XXX */
    case 0xce:    /* into */
      printf_unfiltered (_("Process record does not support "
			   "instruction into.\n"));
      ir.addr -= 1;
      goto no_support;
      break;

    case 0xfa:    /* cli */
    case 0xfb:    /* sti */
      break;

    case 0x62:    /* bound */
      printf_unfiltered (_("Process record does not support "
			   "instruction bound.\n"));
      ir.addr -= 1;
      goto no_support;
      break;

    case 0x0fc8:    /* bswap reg */
    case 0x0fc9:
    case 0x0fca:
    case 0x0fcb:
    case 0x0fcc:
    case 0x0fcd:
    case 0x0fce:
    case 0x0fcf:
      I386_RECORD_FULL_ARCH_LIST_ADD_REG ((opcode & 7) | ir.rex_b);
      break;

    case 0xd6:    /* salc */
      if (ir.regmap[X86_RECORD_R8_REGNUM])
        {
          ir.addr -= 1;
          goto no_support;
        }
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0xe0:    /* loopnz */
    case 0xe1:    /* loopz */
    case 0xe2:    /* loop */
    case 0xe3:    /* jecxz */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RECX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0f30:    /* wrmsr */
      printf_unfiltered (_("Process record does not support "
			   "instruction wrmsr.\n"));
      ir.addr -= 2;
      goto no_support;
      break;

    case 0x0f32:    /* rdmsr */
      printf_unfiltered (_("Process record does not support "
			   "instruction rdmsr.\n"));
      ir.addr -= 2;
      goto no_support;
      break;

    case 0x0f31:    /* rdtsc */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDX_REGNUM);
      break;

    case 0x0f34:    /* sysenter */
      {
	int ret;
        if (ir.regmap[X86_RECORD_R8_REGNUM])
          {
            ir.addr -= 2;
            goto no_support;
          }
	if (tdep->i386_sysenter_record == NULL)
	  {
	    printf_unfiltered (_("Process record does not support "
				 "instruction sysenter.\n"));
	    ir.addr -= 2;
	    goto no_support;
	  }
	ret = tdep->i386_sysenter_record (ir.regcache);
	if (ret)
	  return ret;
      }
      break;

    case 0x0f35:    /* sysexit */
      printf_unfiltered (_("Process record does not support "
			   "instruction sysexit.\n"));
      ir.addr -= 2;
      goto no_support;
      break;

    case 0x0f05:    /* syscall */
      {
	int ret;
	if (tdep->i386_syscall_record == NULL)
	  {
	    printf_unfiltered (_("Process record does not support "
				 "instruction syscall.\n"));
	    ir.addr -= 2;
	    goto no_support;
	  }
	ret = tdep->i386_syscall_record (ir.regcache);
	if (ret)
	  return ret;
      }
      break;

    case 0x0f07:    /* sysret */
      printf_unfiltered (_("Process record does not support "
                           "instruction sysret.\n"));
      ir.addr -= 2;
      goto no_support;
      break;

    case 0x0fa2:    /* cpuid */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RECX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REBX_REGNUM);
      break;

    case 0xf4:    /* hlt */
      printf_unfiltered (_("Process record does not support "
			   "instruction hlt.\n"));
      ir.addr -= 1;
      goto no_support;
      break;

    case 0x0f00:
      if (i386_record_modrm (&ir))
	return -1;
      switch (ir.reg)
	{
	case 0:  /* sldt */
	case 1:  /* str  */
	  if (ir.mod == 3)
            I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm | ir.rex_b);
	  else
	    {
	      ir.ot = OT_WORD;
	      if (i386_record_lea_modrm (&ir))
		return -1;
	    }
	  break;
	case 2:  /* lldt */
	case 3:  /* ltr */
	  break;
	case 4:  /* verr */
	case 5:  /* verw */
          I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	default:
	  ir.addr -= 3;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	  break;
	}
      break;

    case 0x0f01:
      if (i386_record_modrm (&ir))
	return -1;
      switch (ir.reg)
	{
	case 0:  /* sgdt */
	  {
	    uint64_t addr64;

	    if (ir.mod == 3)
	      {
		ir.addr -= 3;
		opcode = opcode << 8 | ir.modrm;
		goto no_support;
	      }
	    if (ir.override >= 0)
	      {
                if (record_full_memory_query)
                  {
	            int q;

                    target_terminal_ours ();
                    q = yquery (_("\
Process record ignores the memory change of instruction at address %s\n\
because it can't get the value of the segment register.\n\
Do you want to stop the program?"),
                                paddress (gdbarch, ir.orig_addr));
                    target_terminal_inferior ();
                    if (q)
                      return -1;
                  }
	      }
	    else
	      {
		if (i386_record_lea_modrm_addr (&ir, &addr64))
		  return -1;
		if (record_full_arch_list_add_mem (addr64, 2))
		  return -1;
		addr64 += 2;
                if (ir.regmap[X86_RECORD_R8_REGNUM])
                  {
                    if (record_full_arch_list_add_mem (addr64, 8))
		      return -1;
                  }
                else
                  {
                    if (record_full_arch_list_add_mem (addr64, 4))
		      return -1;
                  }
	      }
	  }
	  break;
	case 1:
	  if (ir.mod == 3)
	    {
	      switch (ir.rm)
		{
		case 0:  /* monitor */
		  break;
		case 1:  /* mwait */
		  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
		  break;
		default:
		  ir.addr -= 3;
		  opcode = opcode << 8 | ir.modrm;
		  goto no_support;
		  break;
		}
	    }
	  else
	    {
	      /* sidt */
	      if (ir.override >= 0)
		{
                  if (record_full_memory_query)
                    {
	              int q;

                      target_terminal_ours ();
                      q = yquery (_("\
Process record ignores the memory change of instruction at address %s\n\
because it can't get the value of the segment register.\n\
Do you want to stop the program?"),
                                  paddress (gdbarch, ir.orig_addr));
                      target_terminal_inferior ();
                      if (q)
                        return -1;
                    }
		}
	      else
		{
		  uint64_t addr64;

		  if (i386_record_lea_modrm_addr (&ir, &addr64))
		    return -1;
		  if (record_full_arch_list_add_mem (addr64, 2))
		    return -1;
		  addr64 += 2;
                  if (ir.regmap[X86_RECORD_R8_REGNUM])
                    {
                      if (record_full_arch_list_add_mem (addr64, 8))
		        return -1;
                    }
                  else
                    {
                      if (record_full_arch_list_add_mem (addr64, 4))
		        return -1;
                    }
		}
	    }
	  break;
	case 2:  /* lgdt */
	  if (ir.mod == 3)
	    {
	      /* xgetbv */
	      if (ir.rm == 0)
		{
		  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
		  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDX_REGNUM);
		  break;
		}
	      /* xsetbv */
	      else if (ir.rm == 1)
		break;
	    }
	case 3:  /* lidt */
	  if (ir.mod == 3)
	    {
	      ir.addr -= 3;
	      opcode = opcode << 8 | ir.modrm;
	      goto no_support;
	    }
	  break;
	case 4:  /* smsw */
	  if (ir.mod == 3)
	    {
	      if (record_full_arch_list_add_reg (ir.regcache, ir.rm | ir.rex_b))
		return -1;
	    }
	  else
	    {
	      ir.ot = OT_WORD;
	      if (i386_record_lea_modrm (&ir))
		return -1;
	    }
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	case 6:  /* lmsw */
	  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	case 7:  /* invlpg */
	  if (ir.mod == 3)
	    {
	      if (ir.rm == 0 && ir.regmap[X86_RECORD_R8_REGNUM])
	        I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_GS_REGNUM);
	      else
	        {
	          ir.addr -= 3;
	          opcode = opcode << 8 | ir.modrm;
	          goto no_support;
	        }
	    }
	  else
	    I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  break;
	default:
	  ir.addr -= 3;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	  break;
	}
      break;

    case 0x0f08:    /* invd */
    case 0x0f09:    /* wbinvd */
      break;

    case 0x63:    /* arpl */
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3 || ir.regmap[X86_RECORD_R8_REGNUM])
        {
          I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.regmap[X86_RECORD_R8_REGNUM]
					      ? (ir.reg | rex_r) : ir.rm);
        }
      else
        {
          ir.ot = ir.dflag ? OT_LONG : OT_WORD;
          if (i386_record_lea_modrm (&ir))
            return -1;
        }
      if (!ir.regmap[X86_RECORD_R8_REGNUM])
        I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0f02:    /* lar */
    case 0x0f03:    /* lsl */
      if (i386_record_modrm (&ir))
	return -1;
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg | rex_r);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    case 0x0f18:
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3 && ir.reg == 3)
        {
	  ir.addr -= 3;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}
      break;

    case 0x0f19:
    case 0x0f1a:
    case 0x0f1b:
    case 0x0f1c:
    case 0x0f1d:
    case 0x0f1e:
    case 0x0f1f:
      /* nop (multi byte) */
      break;

    case 0x0f20:    /* mov reg, crN */
    case 0x0f22:    /* mov crN, reg */
      if (i386_record_modrm (&ir))
	return -1;
      if ((ir.modrm & 0xc0) != 0xc0)
	{
	  ir.addr -= 3;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}
      switch (ir.reg)
	{
	case 0:
	case 2:
	case 3:
	case 4:
	case 8:
	  if (opcode & 2)
	    I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	  else
            I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm | ir.rex_b);
	  break;
	default:
	  ir.addr -= 3;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	  break;
	}
      break;

    case 0x0f21:    /* mov reg, drN */
    case 0x0f23:    /* mov drN, reg */
      if (i386_record_modrm (&ir))
	return -1;
      if ((ir.modrm & 0xc0) != 0xc0 || ir.reg == 4
	  || ir.reg == 5 || ir.reg >= 8)
	{
	  ir.addr -= 3;
	  opcode = opcode << 8 | ir.modrm;
	  goto no_support;
	}
      if (opcode & 2)
        I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      else
	I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm | ir.rex_b);
      break;

    case 0x0f06:    /* clts */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      break;

    /* MMX 3DNow! SSE SSE2 SSE3 SSSE3 SSE4 */

    case 0x0f0d:    /* 3DNow! prefetch */
      break;

    case 0x0f0e:    /* 3DNow! femms */
    case 0x0f77:    /* emms */
      if (i386_fpc_regnum_p (gdbarch, I387_FTAG_REGNUM(tdep)))
        goto no_support;
      record_full_arch_list_add_reg (ir.regcache, I387_FTAG_REGNUM(tdep));
      break;

    case 0x0f0f:    /* 3DNow! data */
      if (i386_record_modrm (&ir))
	return -1;
      if (record_read_memory (gdbarch, ir.addr, &opcode8, 1))
	return -1;
      ir.addr++;
      switch (opcode8)
        {
        case 0x0c:    /* 3DNow! pi2fw */
        case 0x0d:    /* 3DNow! pi2fd */
        case 0x1c:    /* 3DNow! pf2iw */
        case 0x1d:    /* 3DNow! pf2id */
        case 0x8a:    /* 3DNow! pfnacc */
        case 0x8e:    /* 3DNow! pfpnacc */
        case 0x90:    /* 3DNow! pfcmpge */
        case 0x94:    /* 3DNow! pfmin */
        case 0x96:    /* 3DNow! pfrcp */
        case 0x97:    /* 3DNow! pfrsqrt */
        case 0x9a:    /* 3DNow! pfsub */
        case 0x9e:    /* 3DNow! pfadd */
        case 0xa0:    /* 3DNow! pfcmpgt */
        case 0xa4:    /* 3DNow! pfmax */
        case 0xa6:    /* 3DNow! pfrcpit1 */
        case 0xa7:    /* 3DNow! pfrsqit1 */
        case 0xaa:    /* 3DNow! pfsubr */
        case 0xae:    /* 3DNow! pfacc */
        case 0xb0:    /* 3DNow! pfcmpeq */
        case 0xb4:    /* 3DNow! pfmul */
        case 0xb6:    /* 3DNow! pfrcpit2 */
        case 0xb7:    /* 3DNow! pmulhrw */
        case 0xbb:    /* 3DNow! pswapd */
        case 0xbf:    /* 3DNow! pavgusb */
          if (!i386_mmx_regnum_p (gdbarch, I387_MM0_REGNUM (tdep) + ir.reg))
            goto no_support_3dnow_data;
          record_full_arch_list_add_reg (ir.regcache, ir.reg);
          break;

        default:
no_support_3dnow_data:
          opcode = (opcode << 8) | opcode8;
          goto no_support;
          break;
        }
      break;

    case 0x0faa:    /* rsm */
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REAX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RECX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REBX_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REBP_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_RESI_REGNUM);
      I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REDI_REGNUM);
      break;

    case 0x0fae:
      if (i386_record_modrm (&ir))
	return -1;
      switch(ir.reg)
        {
        case 0:    /* fxsave */
          {
            uint64_t tmpu64;

            I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
	    if (i386_record_lea_modrm_addr (&ir, &tmpu64))
	      return -1;
            if (record_full_arch_list_add_mem (tmpu64, 512))
              return -1;
          }
          break;

        case 1:    /* fxrstor */
          {
            int i;

            I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);

            for (i = I387_MM0_REGNUM (tdep);
                 i386_mmx_regnum_p (gdbarch, i); i++)
              record_full_arch_list_add_reg (ir.regcache, i);

            for (i = I387_XMM0_REGNUM (tdep);
                 i386_xmm_regnum_p (gdbarch, i); i++)
              record_full_arch_list_add_reg (ir.regcache, i);

            if (i386_mxcsr_regnum_p (gdbarch, I387_MXCSR_REGNUM(tdep)))
              record_full_arch_list_add_reg (ir.regcache,
					     I387_MXCSR_REGNUM(tdep));

            for (i = I387_ST0_REGNUM (tdep);
                 i386_fp_regnum_p (gdbarch, i); i++)
              record_full_arch_list_add_reg (ir.regcache, i);

            for (i = I387_FCTRL_REGNUM (tdep);
                 i386_fpc_regnum_p (gdbarch, i); i++)
              record_full_arch_list_add_reg (ir.regcache, i);
          }
          break;

        case 2:    /* ldmxcsr */
          if (!i386_mxcsr_regnum_p (gdbarch, I387_MXCSR_REGNUM(tdep)))
            goto no_support;
          record_full_arch_list_add_reg (ir.regcache, I387_MXCSR_REGNUM(tdep));
          break;

        case 3:    /* stmxcsr */
          ir.ot = OT_LONG;
          if (i386_record_lea_modrm (&ir))
            return -1;
          break;

        case 5:    /* lfence */
        case 6:    /* mfence */
        case 7:    /* sfence clflush */
          break;

        default:
          opcode = (opcode << 8) | ir.modrm;
          goto no_support;
          break;
        }
      break;

    case 0x0fc3:    /* movnti */
      ir.ot = (ir.dflag == 2) ? OT_QUAD : OT_LONG;
      if (i386_record_modrm (&ir))
	return -1;
      if (ir.mod == 3)
        goto no_support;
      ir.reg |= rex_r;
      if (i386_record_lea_modrm (&ir))
        return -1;
      break;

    /* Add prefix to opcode.  */
    case 0x0f10:
    case 0x0f11:
    case 0x0f12:
    case 0x0f13:
    case 0x0f14:
    case 0x0f15:
    case 0x0f16:
    case 0x0f17:
    case 0x0f28:
    case 0x0f29:
    case 0x0f2a:
    case 0x0f2b:
    case 0x0f2c:
    case 0x0f2d:
    case 0x0f2e:
    case 0x0f2f:
    case 0x0f38:
    case 0x0f39:
    case 0x0f3a:
    case 0x0f50:
    case 0x0f51:
    case 0x0f52:
    case 0x0f53:
    case 0x0f54:
    case 0x0f55:
    case 0x0f56:
    case 0x0f57:
    case 0x0f58:
    case 0x0f59:
    case 0x0f5a:
    case 0x0f5b:
    case 0x0f5c:
    case 0x0f5d:
    case 0x0f5e:
    case 0x0f5f:
    case 0x0f60:
    case 0x0f61:
    case 0x0f62:
    case 0x0f63:
    case 0x0f64:
    case 0x0f65:
    case 0x0f66:
    case 0x0f67:
    case 0x0f68:
    case 0x0f69:
    case 0x0f6a:
    case 0x0f6b:
    case 0x0f6c:
    case 0x0f6d:
    case 0x0f6e:
    case 0x0f6f:
    case 0x0f70:
    case 0x0f71:
    case 0x0f72:
    case 0x0f73:
    case 0x0f74:
    case 0x0f75:
    case 0x0f76:
    case 0x0f7c:
    case 0x0f7d:
    case 0x0f7e:
    case 0x0f7f:
    case 0x0fb8:
    case 0x0fc2:
    case 0x0fc4:
    case 0x0fc5:
    case 0x0fc6:
    case 0x0fd0:
    case 0x0fd1:
    case 0x0fd2:
    case 0x0fd3:
    case 0x0fd4:
    case 0x0fd5:
    case 0x0fd6:
    case 0x0fd7:
    case 0x0fd8:
    case 0x0fd9:
    case 0x0fda:
    case 0x0fdb:
    case 0x0fdc:
    case 0x0fdd:
    case 0x0fde:
    case 0x0fdf:
    case 0x0fe0:
    case 0x0fe1:
    case 0x0fe2:
    case 0x0fe3:
    case 0x0fe4:
    case 0x0fe5:
    case 0x0fe6:
    case 0x0fe7:
    case 0x0fe8:
    case 0x0fe9:
    case 0x0fea:
    case 0x0feb:
    case 0x0fec:
    case 0x0fed:
    case 0x0fee:
    case 0x0fef:
    case 0x0ff0:
    case 0x0ff1:
    case 0x0ff2:
    case 0x0ff3:
    case 0x0ff4:
    case 0x0ff5:
    case 0x0ff6:
    case 0x0ff7:
    case 0x0ff8:
    case 0x0ff9:
    case 0x0ffa:
    case 0x0ffb:
    case 0x0ffc:
    case 0x0ffd:
    case 0x0ffe:
      switch (prefixes)
        {
        case PREFIX_REPNZ:
          opcode |= 0xf20000;
          break;
        case PREFIX_DATA:
          opcode |= 0x660000;
          break;
        case PREFIX_REPZ:
          opcode |= 0xf30000;
          break;
        }
reswitch_prefix_add:
      switch (opcode)
        {
        case 0x0f38:
        case 0x660f38:
        case 0xf20f38:
        case 0x0f3a:
        case 0x660f3a:
          if (record_read_memory (gdbarch, ir.addr, &opcode8, 1))
	    return -1;
          ir.addr++;
          opcode = (uint32_t) opcode8 | opcode << 8;
          goto reswitch_prefix_add;
          break;

        case 0x0f10:        /* movups */
        case 0x660f10:      /* movupd */
        case 0xf30f10:      /* movss */
        case 0xf20f10:      /* movsd */
        case 0x0f12:        /* movlps */
        case 0x660f12:      /* movlpd */
        case 0xf30f12:      /* movsldup */
        case 0xf20f12:      /* movddup */
        case 0x0f14:        /* unpcklps */
        case 0x660f14:      /* unpcklpd */
        case 0x0f15:        /* unpckhps */
        case 0x660f15:      /* unpckhpd */
        case 0x0f16:        /* movhps */
        case 0x660f16:      /* movhpd */
        case 0xf30f16:      /* movshdup */
        case 0x0f28:        /* movaps */
        case 0x660f28:      /* movapd */
        case 0x0f2a:        /* cvtpi2ps */
        case 0x660f2a:      /* cvtpi2pd */
        case 0xf30f2a:      /* cvtsi2ss */
        case 0xf20f2a:      /* cvtsi2sd */
        case 0x0f2c:        /* cvttps2pi */
        case 0x660f2c:      /* cvttpd2pi */
        case 0x0f2d:        /* cvtps2pi */
        case 0x660f2d:      /* cvtpd2pi */
        case 0x660f3800:    /* pshufb */
        case 0x660f3801:    /* phaddw */
        case 0x660f3802:    /* phaddd */
        case 0x660f3803:    /* phaddsw */
        case 0x660f3804:    /* pmaddubsw */
        case 0x660f3805:    /* phsubw */
        case 0x660f3806:    /* phsubd */
        case 0x660f3807:    /* phsubsw */
        case 0x660f3808:    /* psignb */
        case 0x660f3809:    /* psignw */
        case 0x660f380a:    /* psignd */
        case 0x660f380b:    /* pmulhrsw */
        case 0x660f3810:    /* pblendvb */
        case 0x660f3814:    /* blendvps */
        case 0x660f3815:    /* blendvpd */
        case 0x660f381c:    /* pabsb */
        case 0x660f381d:    /* pabsw */
        case 0x660f381e:    /* pabsd */
        case 0x660f3820:    /* pmovsxbw */
        case 0x660f3821:    /* pmovsxbd */
        case 0x660f3822:    /* pmovsxbq */
        case 0x660f3823:    /* pmovsxwd */
        case 0x660f3824:    /* pmovsxwq */
        case 0x660f3825:    /* pmovsxdq */
        case 0x660f3828:    /* pmuldq */
        case 0x660f3829:    /* pcmpeqq */
        case 0x660f382a:    /* movntdqa */
        case 0x660f3a08:    /* roundps */
        case 0x660f3a09:    /* roundpd */
        case 0x660f3a0a:    /* roundss */
        case 0x660f3a0b:    /* roundsd */
        case 0x660f3a0c:    /* blendps */
        case 0x660f3a0d:    /* blendpd */
        case 0x660f3a0e:    /* pblendw */
        case 0x660f3a0f:    /* palignr */
        case 0x660f3a20:    /* pinsrb */
        case 0x660f3a21:    /* insertps */
        case 0x660f3a22:    /* pinsrd pinsrq */
        case 0x660f3a40:    /* dpps */
        case 0x660f3a41:    /* dppd */
        case 0x660f3a42:    /* mpsadbw */
        case 0x660f3a60:    /* pcmpestrm */
        case 0x660f3a61:    /* pcmpestri */
        case 0x660f3a62:    /* pcmpistrm */
        case 0x660f3a63:    /* pcmpistri */
        case 0x0f51:        /* sqrtps */
        case 0x660f51:      /* sqrtpd */
        case 0xf20f51:      /* sqrtsd */
        case 0xf30f51:      /* sqrtss */
        case 0x0f52:        /* rsqrtps */
        case 0xf30f52:      /* rsqrtss */
        case 0x0f53:        /* rcpps */
        case 0xf30f53:      /* rcpss */
        case 0x0f54:        /* andps */
        case 0x660f54:      /* andpd */
        case 0x0f55:        /* andnps */
        case 0x660f55:      /* andnpd */
        case 0x0f56:        /* orps */
        case 0x660f56:      /* orpd */
        case 0x0f57:        /* xorps */
        case 0x660f57:      /* xorpd */
        case 0x0f58:        /* addps */
        case 0x660f58:      /* addpd */
        case 0xf20f58:      /* addsd */
        case 0xf30f58:      /* addss */
        case 0x0f59:        /* mulps */
        case 0x660f59:      /* mulpd */
        case 0xf20f59:      /* mulsd */
        case 0xf30f59:      /* mulss */
        case 0x0f5a:        /* cvtps2pd */
        case 0x660f5a:      /* cvtpd2ps */
        case 0xf20f5a:      /* cvtsd2ss */
        case 0xf30f5a:      /* cvtss2sd */
        case 0x0f5b:        /* cvtdq2ps */
        case 0x660f5b:      /* cvtps2dq */
        case 0xf30f5b:      /* cvttps2dq */
        case 0x0f5c:        /* subps */
        case 0x660f5c:      /* subpd */
        case 0xf20f5c:      /* subsd */
        case 0xf30f5c:      /* subss */
        case 0x0f5d:        /* minps */
        case 0x660f5d:      /* minpd */
        case 0xf20f5d:      /* minsd */
        case 0xf30f5d:      /* minss */
        case 0x0f5e:        /* divps */
        case 0x660f5e:      /* divpd */
        case 0xf20f5e:      /* divsd */
        case 0xf30f5e:      /* divss */
        case 0x0f5f:        /* maxps */
        case 0x660f5f:      /* maxpd */
        case 0xf20f5f:      /* maxsd */
        case 0xf30f5f:      /* maxss */
        case 0x660f60:      /* punpcklbw */
        case 0x660f61:      /* punpcklwd */
        case 0x660f62:      /* punpckldq */
        case 0x660f63:      /* packsswb */
        case 0x660f64:      /* pcmpgtb */
        case 0x660f65:      /* pcmpgtw */
        case 0x660f66:      /* pcmpgtd */
        case 0x660f67:      /* packuswb */
        case 0x660f68:      /* punpckhbw */
        case 0x660f69:      /* punpckhwd */
        case 0x660f6a:      /* punpckhdq */
        case 0x660f6b:      /* packssdw */
        case 0x660f6c:      /* punpcklqdq */
        case 0x660f6d:      /* punpckhqdq */
        case 0x660f6e:      /* movd */
        case 0x660f6f:      /* movdqa */
        case 0xf30f6f:      /* movdqu */
        case 0x660f70:      /* pshufd */
        case 0xf20f70:      /* pshuflw */
        case 0xf30f70:      /* pshufhw */
        case 0x660f74:      /* pcmpeqb */
        case 0x660f75:      /* pcmpeqw */
        case 0x660f76:      /* pcmpeqd */
        case 0x660f7c:      /* haddpd */
        case 0xf20f7c:      /* haddps */
        case 0x660f7d:      /* hsubpd */
        case 0xf20f7d:      /* hsubps */
        case 0xf30f7e:      /* movq */
        case 0x0fc2:        /* cmpps */
        case 0x660fc2:      /* cmppd */
        case 0xf20fc2:      /* cmpsd */
        case 0xf30fc2:      /* cmpss */
        case 0x660fc4:      /* pinsrw */
        case 0x0fc6:        /* shufps */
        case 0x660fc6:      /* shufpd */
        case 0x660fd0:      /* addsubpd */
        case 0xf20fd0:      /* addsubps */
        case 0x660fd1:      /* psrlw */
        case 0x660fd2:      /* psrld */
        case 0x660fd3:      /* psrlq */
        case 0x660fd4:      /* paddq */
        case 0x660fd5:      /* pmullw */
        case 0xf30fd6:      /* movq2dq */
        case 0x660fd8:      /* psubusb */
        case 0x660fd9:      /* psubusw */
        case 0x660fda:      /* pminub */
        case 0x660fdb:      /* pand */
        case 0x660fdc:      /* paddusb */
        case 0x660fdd:      /* paddusw */
        case 0x660fde:      /* pmaxub */
        case 0x660fdf:      /* pandn */
        case 0x660fe0:      /* pavgb */
        case 0x660fe1:      /* psraw */
        case 0x660fe2:      /* psrad */
        case 0x660fe3:      /* pavgw */
        case 0x660fe4:      /* pmulhuw */
        case 0x660fe5:      /* pmulhw */
        case 0x660fe6:      /* cvttpd2dq */
        case 0xf20fe6:      /* cvtpd2dq */
        case 0xf30fe6:      /* cvtdq2pd */
        case 0x660fe8:      /* psubsb */
        case 0x660fe9:      /* psubsw */
        case 0x660fea:      /* pminsw */
        case 0x660feb:      /* por */
        case 0x660fec:      /* paddsb */
        case 0x660fed:      /* paddsw */
        case 0x660fee:      /* pmaxsw */
        case 0x660fef:      /* pxor */
        case 0xf20ff0:      /* lddqu */
        case 0x660ff1:      /* psllw */
        case 0x660ff2:      /* pslld */
        case 0x660ff3:      /* psllq */
        case 0x660ff4:      /* pmuludq */
        case 0x660ff5:      /* pmaddwd */
        case 0x660ff6:      /* psadbw */
        case 0x660ff8:      /* psubb */
        case 0x660ff9:      /* psubw */
        case 0x660ffa:      /* psubd */
        case 0x660ffb:      /* psubq */
        case 0x660ffc:      /* paddb */
        case 0x660ffd:      /* paddw */
        case 0x660ffe:      /* paddd */
          if (i386_record_modrm (&ir))
	    return -1;
          ir.reg |= rex_r;
          if (!i386_xmm_regnum_p (gdbarch, I387_XMM0_REGNUM (tdep) + ir.reg))
            goto no_support;
          record_full_arch_list_add_reg (ir.regcache,
					 I387_XMM0_REGNUM (tdep) + ir.reg);
          if ((opcode & 0xfffffffc) == 0x660f3a60)
            I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
          break;

        case 0x0f11:        /* movups */
        case 0x660f11:      /* movupd */
        case 0xf30f11:      /* movss */
        case 0xf20f11:      /* movsd */
        case 0x0f13:        /* movlps */
        case 0x660f13:      /* movlpd */
        case 0x0f17:        /* movhps */
        case 0x660f17:      /* movhpd */
        case 0x0f29:        /* movaps */
        case 0x660f29:      /* movapd */
        case 0x660f3a14:    /* pextrb */
        case 0x660f3a15:    /* pextrw */
        case 0x660f3a16:    /* pextrd pextrq */
        case 0x660f3a17:    /* extractps */
        case 0x660f7f:      /* movdqa */
        case 0xf30f7f:      /* movdqu */
          if (i386_record_modrm (&ir))
	    return -1;
          if (ir.mod == 3)
            {
              if (opcode == 0x0f13 || opcode == 0x660f13
                  || opcode == 0x0f17 || opcode == 0x660f17)
                goto no_support;
              ir.rm |= ir.rex_b;
              if (!i386_xmm_regnum_p (gdbarch,
				      I387_XMM0_REGNUM (tdep) + ir.rm))
                goto no_support;
              record_full_arch_list_add_reg (ir.regcache,
					     I387_XMM0_REGNUM (tdep) + ir.rm);
            }
          else
            {
              switch (opcode)
                {
                  case 0x660f3a14:
                    ir.ot = OT_BYTE;
                    break;
                  case 0x660f3a15:
                    ir.ot = OT_WORD;
                    break;
                  case 0x660f3a16:
                    ir.ot = OT_LONG;
                    break;
                  case 0x660f3a17:
                    ir.ot = OT_QUAD;
                    break;
                  default:
                    ir.ot = OT_DQUAD;
                    break;
                }
              if (i386_record_lea_modrm (&ir))
                return -1;
            }
          break;

        case 0x0f2b:      /* movntps */
        case 0x660f2b:    /* movntpd */
        case 0x0fe7:      /* movntq */
        case 0x660fe7:    /* movntdq */
          if (ir.mod == 3)
            goto no_support;
          if (opcode == 0x0fe7)
            ir.ot = OT_QUAD;
          else
            ir.ot = OT_DQUAD;
          if (i386_record_lea_modrm (&ir))
            return -1;
          break;

        case 0xf30f2c:      /* cvttss2si */
        case 0xf20f2c:      /* cvttsd2si */
        case 0xf30f2d:      /* cvtss2si */
        case 0xf20f2d:      /* cvtsd2si */
        case 0xf20f38f0:    /* crc32 */
        case 0xf20f38f1:    /* crc32 */
        case 0x0f50:        /* movmskps */
        case 0x660f50:      /* movmskpd */
        case 0x0fc5:        /* pextrw */
        case 0x660fc5:      /* pextrw */
        case 0x0fd7:        /* pmovmskb */
        case 0x660fd7:      /* pmovmskb */
          I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg | rex_r);
          break;

        case 0x0f3800:    /* pshufb */
        case 0x0f3801:    /* phaddw */
        case 0x0f3802:    /* phaddd */
        case 0x0f3803:    /* phaddsw */
        case 0x0f3804:    /* pmaddubsw */
        case 0x0f3805:    /* phsubw */
        case 0x0f3806:    /* phsubd */
        case 0x0f3807:    /* phsubsw */
        case 0x0f3808:    /* psignb */
        case 0x0f3809:    /* psignw */
        case 0x0f380a:    /* psignd */
        case 0x0f380b:    /* pmulhrsw */
        case 0x0f381c:    /* pabsb */
        case 0x0f381d:    /* pabsw */
        case 0x0f381e:    /* pabsd */
        case 0x0f382b:    /* packusdw */
        case 0x0f3830:    /* pmovzxbw */
        case 0x0f3831:    /* pmovzxbd */
        case 0x0f3832:    /* pmovzxbq */
        case 0x0f3833:    /* pmovzxwd */
        case 0x0f3834:    /* pmovzxwq */
        case 0x0f3835:    /* pmovzxdq */
        case 0x0f3837:    /* pcmpgtq */
        case 0x0f3838:    /* pminsb */
        case 0x0f3839:    /* pminsd */
        case 0x0f383a:    /* pminuw */
        case 0x0f383b:    /* pminud */
        case 0x0f383c:    /* pmaxsb */
        case 0x0f383d:    /* pmaxsd */
        case 0x0f383e:    /* pmaxuw */
        case 0x0f383f:    /* pmaxud */
        case 0x0f3840:    /* pmulld */
        case 0x0f3841:    /* phminposuw */
        case 0x0f3a0f:    /* palignr */
        case 0x0f60:      /* punpcklbw */
        case 0x0f61:      /* punpcklwd */
        case 0x0f62:      /* punpckldq */
        case 0x0f63:      /* packsswb */
        case 0x0f64:      /* pcmpgtb */
        case 0x0f65:      /* pcmpgtw */
        case 0x0f66:      /* pcmpgtd */
        case 0x0f67:      /* packuswb */
        case 0x0f68:      /* punpckhbw */
        case 0x0f69:      /* punpckhwd */
        case 0x0f6a:      /* punpckhdq */
        case 0x0f6b:      /* packssdw */
        case 0x0f6e:      /* movd */
        case 0x0f6f:      /* movq */
        case 0x0f70:      /* pshufw */
        case 0x0f74:      /* pcmpeqb */
        case 0x0f75:      /* pcmpeqw */
        case 0x0f76:      /* pcmpeqd */
        case 0x0fc4:      /* pinsrw */
        case 0x0fd1:      /* psrlw */
        case 0x0fd2:      /* psrld */
        case 0x0fd3:      /* psrlq */
        case 0x0fd4:      /* paddq */
        case 0x0fd5:      /* pmullw */
        case 0xf20fd6:    /* movdq2q */
        case 0x0fd8:      /* psubusb */
        case 0x0fd9:      /* psubusw */
        case 0x0fda:      /* pminub */
        case 0x0fdb:      /* pand */
        case 0x0fdc:      /* paddusb */
        case 0x0fdd:      /* paddusw */
        case 0x0fde:      /* pmaxub */
        case 0x0fdf:      /* pandn */
        case 0x0fe0:      /* pavgb */
        case 0x0fe1:      /* psraw */
        case 0x0fe2:      /* psrad */
        case 0x0fe3:      /* pavgw */
        case 0x0fe4:      /* pmulhuw */
        case 0x0fe5:      /* pmulhw */
        case 0x0fe8:      /* psubsb */
        case 0x0fe9:      /* psubsw */
        case 0x0fea:      /* pminsw */
        case 0x0feb:      /* por */
        case 0x0fec:      /* paddsb */
        case 0x0fed:      /* paddsw */
        case 0x0fee:      /* pmaxsw */
        case 0x0fef:      /* pxor */
        case 0x0ff1:      /* psllw */
        case 0x0ff2:      /* pslld */
        case 0x0ff3:      /* psllq */
        case 0x0ff4:      /* pmuludq */
        case 0x0ff5:      /* pmaddwd */
        case 0x0ff6:      /* psadbw */
        case 0x0ff8:      /* psubb */
        case 0x0ff9:      /* psubw */
        case 0x0ffa:      /* psubd */
        case 0x0ffb:      /* psubq */
        case 0x0ffc:      /* paddb */
        case 0x0ffd:      /* paddw */
        case 0x0ffe:      /* paddd */
          if (i386_record_modrm (&ir))
	    return -1;
          if (!i386_mmx_regnum_p (gdbarch, I387_MM0_REGNUM (tdep) + ir.reg))
            goto no_support;
          record_full_arch_list_add_reg (ir.regcache,
					 I387_MM0_REGNUM (tdep) + ir.reg);
          break;

        case 0x0f71:    /* psllw */
        case 0x0f72:    /* pslld */
        case 0x0f73:    /* psllq */
          if (i386_record_modrm (&ir))
	    return -1;
          if (!i386_mmx_regnum_p (gdbarch, I387_MM0_REGNUM (tdep) + ir.rm))
            goto no_support;
          record_full_arch_list_add_reg (ir.regcache,
					 I387_MM0_REGNUM (tdep) + ir.rm);
          break;

        case 0x660f71:    /* psllw */
        case 0x660f72:    /* pslld */
        case 0x660f73:    /* psllq */
          if (i386_record_modrm (&ir))
	    return -1;
          ir.rm |= ir.rex_b;
          if (!i386_xmm_regnum_p (gdbarch, I387_XMM0_REGNUM (tdep) + ir.rm))
            goto no_support;
          record_full_arch_list_add_reg (ir.regcache,
					 I387_XMM0_REGNUM (tdep) + ir.rm);
          break;

        case 0x0f7e:      /* movd */
        case 0x660f7e:    /* movd */
          if (i386_record_modrm (&ir))
	    return -1;
          if (ir.mod == 3)
            I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.rm | ir.rex_b);
          else
            {
              if (ir.dflag == 2)
                ir.ot = OT_QUAD;
              else
                ir.ot = OT_LONG;
              if (i386_record_lea_modrm (&ir))
                return -1;
            }
          break;

        case 0x0f7f:    /* movq */
          if (i386_record_modrm (&ir))
	    return -1;
          if (ir.mod == 3)
            {
              if (!i386_mmx_regnum_p (gdbarch, I387_MM0_REGNUM (tdep) + ir.rm))
                goto no_support;
              record_full_arch_list_add_reg (ir.regcache,
					     I387_MM0_REGNUM (tdep) + ir.rm);
            }
          else
            {
              ir.ot = OT_QUAD;
              if (i386_record_lea_modrm (&ir))
                return -1;
            }
          break;

        case 0xf30fb8:    /* popcnt */
          if (i386_record_modrm (&ir))
	    return -1;
          I386_RECORD_FULL_ARCH_LIST_ADD_REG (ir.reg);
          I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
          break;

        case 0x660fd6:    /* movq */
          if (i386_record_modrm (&ir))
	    return -1;
          if (ir.mod == 3)
            {
              ir.rm |= ir.rex_b;
              if (!i386_xmm_regnum_p (gdbarch,
				      I387_XMM0_REGNUM (tdep) + ir.rm))
                goto no_support;
              record_full_arch_list_add_reg (ir.regcache,
					     I387_XMM0_REGNUM (tdep) + ir.rm);
            }
          else
            {
              ir.ot = OT_QUAD;
              if (i386_record_lea_modrm (&ir))
                return -1;
            }
          break;

        case 0x660f3817:    /* ptest */
        case 0x0f2e:        /* ucomiss */
        case 0x660f2e:      /* ucomisd */
        case 0x0f2f:        /* comiss */
        case 0x660f2f:      /* comisd */
          I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_EFLAGS_REGNUM);
          break;

        case 0x0ff7:    /* maskmovq */
          regcache_raw_read_unsigned (ir.regcache,
                                      ir.regmap[X86_RECORD_REDI_REGNUM],
                                      &addr);
          if (record_full_arch_list_add_mem (addr, 64))
            return -1;
          break;

        case 0x660ff7:    /* maskmovdqu */
          regcache_raw_read_unsigned (ir.regcache,
                                      ir.regmap[X86_RECORD_REDI_REGNUM],
                                      &addr);
          if (record_full_arch_list_add_mem (addr, 128))
            return -1;
          break;

        default:
          goto no_support;
          break;
        }
      break;

    default:
      goto no_support;
      break;
    }

  /* In the future, maybe still need to deal with need_dasm.  */
  I386_RECORD_FULL_ARCH_LIST_ADD_REG (X86_RECORD_REIP_REGNUM);
  if (record_full_arch_list_add_end ())
    return -1;

  return 0;

 no_support:
  printf_unfiltered (_("Process record does not support instruction 0x%02x "
                       "at address %s.\n"),
                     (unsigned int) (opcode),
                     paddress (gdbarch, ir.orig_addr));
  return -1;
}

static const int i386_record_regmap[] =
{
  I386_EAX_REGNUM, I386_ECX_REGNUM, I386_EDX_REGNUM, I386_EBX_REGNUM,
  I386_ESP_REGNUM, I386_EBP_REGNUM, I386_ESI_REGNUM, I386_EDI_REGNUM,
  0, 0, 0, 0, 0, 0, 0, 0,
  I386_EIP_REGNUM, I386_EFLAGS_REGNUM, I386_CS_REGNUM, I386_SS_REGNUM,
  I386_DS_REGNUM, I386_ES_REGNUM, I386_FS_REGNUM, I386_GS_REGNUM
};

/* Check that the given address appears suitable for a fast
   tracepoint, which on x86-64 means that we need an instruction of at
   least 5 bytes, so that we can overwrite it with a 4-byte-offset
   jump and not have to worry about program jumps to an address in the
   middle of the tracepoint jump.  On x86, it may be possible to use
   4-byte jumps with a 2-byte offset to a trampoline located in the
   bottom 64 KiB of memory.  Returns 1 if OK, and writes a size
   of instruction to replace, and 0 if not, plus an explanatory
   string.  */

static int
i386_fast_tracepoint_valid_at (struct gdbarch *gdbarch,
			       CORE_ADDR addr, int *isize, char **msg)
{
  int len, jumplen;
  static struct ui_file *gdb_null = NULL;

  /*  Ask the target for the minimum instruction length supported.  */
  jumplen = target_get_min_fast_tracepoint_insn_len ();

  if (jumplen < 0)
    {
      /* If the target does not support the get_min_fast_tracepoint_insn_len
	 operation, assume that fast tracepoints will always be implemented
	 using 4-byte relative jumps on both x86 and x86-64.  */
      jumplen = 5;
    }
  else if (jumplen == 0)
    {
      /* If the target does support get_min_fast_tracepoint_insn_len but
	 returns zero, then the IPA has not loaded yet.  In this case,
	 we optimistically assume that truncated 2-byte relative jumps
	 will be available on x86, and compensate later if this assumption
	 turns out to be incorrect.  On x86-64 architectures, 4-byte relative
	 jumps will always be used.  */
      jumplen = (register_size (gdbarch, 0) == 8) ? 5 : 4;
    }

  /* Dummy file descriptor for the disassembler.  */
  if (!gdb_null)
    gdb_null = ui_file_new ();

  /* Check for fit.  */
  len = gdb_print_insn (gdbarch, addr, gdb_null, NULL);
  if (isize)
    *isize = len;

  if (len < jumplen)
    {
      /* Return a bit of target-specific detail to add to the caller's
	 generic failure message.  */
      if (msg)
	*msg = xstrprintf (_("; instruction is only %d bytes long, "
			     "need at least %d bytes for the jump"),
			   len, jumplen);
      return 0;
    }
  else
    {
      if (msg)
	*msg = NULL;
      return 1;
    }
}

static int
i386_validate_tdesc_p (struct gdbarch_tdep *tdep,
		       struct tdesc_arch_data *tdesc_data)
{
  const struct target_desc *tdesc = tdep->tdesc;
  const struct tdesc_feature *feature_core;
  const struct tdesc_feature *feature_sse, *feature_avx;
  int i, num_regs, valid_p;

  if (! tdesc_has_registers (tdesc))
    return 0;

  /* Get core registers.  */
  feature_core = tdesc_find_feature (tdesc, "org.gnu.gdb.i386.core");
  if (feature_core == NULL)
    return 0;

  /* Get SSE registers.  */
  feature_sse = tdesc_find_feature (tdesc, "org.gnu.gdb.i386.sse");

  /* Try AVX registers.  */
  feature_avx = tdesc_find_feature (tdesc, "org.gnu.gdb.i386.avx");

  valid_p = 1;

  /* The XCR0 bits.  */
  if (feature_avx)
    {
      /* AVX register description requires SSE register description.  */
      if (!feature_sse)
	return 0;

      tdep->xcr0 = I386_XSTATE_AVX_MASK;

      /* It may have been set by OSABI initialization function.  */
      if (tdep->num_ymm_regs == 0)
	{
	  tdep->ymmh_register_names = i386_ymmh_names;
	  tdep->num_ymm_regs = 8;
	  tdep->ymm0h_regnum = I386_YMM0H_REGNUM;
	}

      for (i = 0; i < tdep->num_ymm_regs; i++)
	valid_p &= tdesc_numbered_register (feature_avx, tdesc_data,
					    tdep->ymm0h_regnum + i,
					    tdep->ymmh_register_names[i]);
    }
  else if (feature_sse)
    tdep->xcr0 = I386_XSTATE_SSE_MASK;
  else
    {
      tdep->xcr0 = I386_XSTATE_X87_MASK;
      tdep->num_xmm_regs = 0;
    }

  num_regs = tdep->num_core_regs;
  for (i = 0; i < num_regs; i++)
    valid_p &= tdesc_numbered_register (feature_core, tdesc_data, i,
					tdep->register_names[i]);

  if (feature_sse)
    {
      /* Need to include %mxcsr, so add one.  */
      num_regs += tdep->num_xmm_regs + 1;
      for (; i < num_regs; i++)
	valid_p &= tdesc_numbered_register (feature_sse, tdesc_data, i,
					    tdep->register_names[i]);
    }

  return valid_p;
}


static struct gdbarch *
i386_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch_tdep *tdep;
  struct gdbarch *gdbarch;
  struct tdesc_arch_data *tdesc_data;
  const struct target_desc *tdesc;
  int mm0_regnum;
  int ymm0_regnum;

  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* Allocate space for the new architecture.  */
  tdep = XCALLOC (1, struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  /* General-purpose registers.  */
  tdep->gregset = NULL;
  tdep->gregset_reg_offset = NULL;
  tdep->gregset_num_regs = I386_NUM_GREGS;
  tdep->sizeof_gregset = 0;

  /* Floating-point registers.  */
  tdep->fpregset = NULL;
  tdep->sizeof_fpregset = I387_SIZEOF_FSAVE;

  tdep->xstateregset = NULL;

  /* The default settings include the FPU registers, the MMX registers
     and the SSE registers.  This can be overridden for a specific ABI
     by adjusting the members `st0_regnum', `mm0_regnum' and
     `num_xmm_regs' of `struct gdbarch_tdep', otherwise the registers
     will show up in the output of "info all-registers".  */

  tdep->st0_regnum = I386_ST0_REGNUM;

  /* I386_NUM_XREGS includes %mxcsr, so substract one.  */
  tdep->num_xmm_regs = I386_NUM_XREGS - 1;

  tdep->jb_pc_offset = -1;
  tdep->struct_return = pcc_struct_return;
  tdep->sigtramp_start = 0;
  tdep->sigtramp_end = 0;
  tdep->sigtramp_p = i386_sigtramp_p;
  tdep->sigcontext_addr = NULL;
  tdep->sc_reg_offset = NULL;
  tdep->sc_pc_offset = -1;
  tdep->sc_sp_offset = -1;

  tdep->xsave_xcr0_offset = -1;

  tdep->record_regmap = i386_record_regmap;

  set_gdbarch_long_long_align_bit (gdbarch, 32);

  /* The format used for `long double' on almost all i386 targets is
     the i387 extended floating-point format.  In fact, of all targets
     in the GCC 2.95 tree, only OSF/1 does it different, and insists
     on having a `long double' that's not `long' at all.  */
  set_gdbarch_long_double_format (gdbarch, floatformats_i387_ext);

  /* Although the i387 extended floating-point has only 80 significant
     bits, a `long double' actually takes up 96, probably to enforce
     alignment.  */
  set_gdbarch_long_double_bit (gdbarch, 96);

  /* Register numbers of various important registers.  */
  set_gdbarch_sp_regnum (gdbarch, I386_ESP_REGNUM); /* %esp */
  set_gdbarch_pc_regnum (gdbarch, I386_EIP_REGNUM); /* %eip */
  set_gdbarch_ps_regnum (gdbarch, I386_EFLAGS_REGNUM); /* %eflags */
  set_gdbarch_fp0_regnum (gdbarch, I386_ST0_REGNUM); /* %st(0) */

  /* NOTE: kettenis/20040418: GCC does have two possible register
     numbering schemes on the i386: dbx and SVR4.  These schemes
     differ in how they number %ebp, %esp, %eflags, and the
     floating-point registers, and are implemented by the arrays
     dbx_register_map[] and svr4_dbx_register_map in
     gcc/config/i386.c.  GCC also defines a third numbering scheme in
     gcc/config/i386.c, which it designates as the "default" register
     map used in 64bit mode.  This last register numbering scheme is
     implemented in dbx64_register_map, and is used for AMD64; see
     amd64-tdep.c.

     Currently, each GCC i386 target always uses the same register
     numbering scheme across all its supported debugging formats
     i.e. SDB (COFF), stabs and DWARF 2.  This is because
     gcc/sdbout.c, gcc/dbxout.c and gcc/dwarf2out.c all use the
     DBX_REGISTER_NUMBER macro which is defined by each target's
     respective config header in a manner independent of the requested
     output debugging format.

     This does not match the arrangement below, which presumes that
     the SDB and stabs numbering schemes differ from the DWARF and
     DWARF 2 ones.  The reason for this arrangement is that it is
     likely to get the numbering scheme for the target's
     default/native debug format right.  For targets where GCC is the
     native compiler (FreeBSD, NetBSD, OpenBSD, GNU/Linux) or for
     targets where the native toolchain uses a different numbering
     scheme for a particular debug format (stabs-in-ELF on Solaris)
     the defaults below will have to be overridden, like
     i386_elf_init_abi() does.  */

  /* Use the dbx register numbering scheme for stabs and COFF.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, i386_dbx_reg_to_regnum);
  set_gdbarch_sdb_reg_to_regnum (gdbarch, i386_dbx_reg_to_regnum);

  /* Use the SVR4 register numbering scheme for DWARF 2.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, i386_svr4_reg_to_regnum);

  /* We don't set gdbarch_stab_reg_to_regnum, since ECOFF doesn't seem to
     be in use on any of the supported i386 targets.  */

  set_gdbarch_print_float_info (gdbarch, i387_print_float_info);

  set_gdbarch_get_longjmp_target (gdbarch, i386_get_longjmp_target);

  /* Call dummy code.  */
  set_gdbarch_call_dummy_location (gdbarch, ON_STACK);
  set_gdbarch_push_dummy_code (gdbarch, i386_push_dummy_code);
  set_gdbarch_push_dummy_call (gdbarch, i386_push_dummy_call);
  set_gdbarch_frame_align (gdbarch, i386_frame_align);

  set_gdbarch_convert_register_p (gdbarch, i386_convert_register_p);
  set_gdbarch_register_to_value (gdbarch,  i386_register_to_value);
  set_gdbarch_value_to_register (gdbarch, i386_value_to_register);

  set_gdbarch_return_value (gdbarch, i386_return_value);

  set_gdbarch_skip_prologue (gdbarch, i386_skip_prologue);

  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_from_pc (gdbarch, i386_breakpoint_from_pc);
  set_gdbarch_decr_pc_after_break (gdbarch, 1);
  set_gdbarch_max_insn_length (gdbarch, I386_MAX_INSN_LEN);

  set_gdbarch_frame_args_skip (gdbarch, 8);

  set_gdbarch_print_insn (gdbarch, i386_print_insn);

  set_gdbarch_dummy_id (gdbarch, i386_dummy_id);

  set_gdbarch_unwind_pc (gdbarch, i386_unwind_pc);

  /* Add the i386 register groups.  */
  i386_add_reggroups (gdbarch);
  tdep->register_reggroup_p = i386_register_reggroup_p;

  /* Helper for function argument information.  */
  set_gdbarch_fetch_pointer_argument (gdbarch, i386_fetch_pointer_argument);

  /* Hook the function epilogue frame unwinder.  This unwinder is
     appended to the list first, so that it supercedes the DWARF
     unwinder in function epilogues (where the DWARF unwinder
     currently fails).  */
  frame_unwind_append_unwinder (gdbarch, &i386_epilogue_frame_unwind);

  /* Hook in the DWARF CFI frame unwinder.  This unwinder is appended
     to the list before the prologue-based unwinders, so that DWARF
     CFI info will be used if it is available.  */
  dwarf2_append_unwinders (gdbarch);

  frame_base_set_default (gdbarch, &i386_frame_base);

  /* Pseudo registers may be changed by amd64_init_abi.  */
  set_gdbarch_pseudo_register_read_value (gdbarch,
					  i386_pseudo_register_read_value);
  set_gdbarch_pseudo_register_write (gdbarch, i386_pseudo_register_write);

  set_tdesc_pseudo_register_type (gdbarch, i386_pseudo_register_type);
  set_tdesc_pseudo_register_name (gdbarch, i386_pseudo_register_name);

  /* Override the normal target description method to make the AVX
     upper halves anonymous.  */
  set_gdbarch_register_name (gdbarch, i386_register_name);

  /* Even though the default ABI only includes general-purpose registers,
     floating-point registers and the SSE registers, we have to leave a
     gap for the upper AVX registers.  */
  set_gdbarch_num_regs (gdbarch, I386_AVX_NUM_REGS);

  /* Get the x86 target description from INFO.  */
  tdesc = info.target_desc;
  if (! tdesc_has_registers (tdesc))
    tdesc = tdesc_i386;
  tdep->tdesc = tdesc;

  tdep->num_core_regs = I386_NUM_GREGS + I387_NUM_REGS;
  tdep->register_names = i386_register_names;

  /* No upper YMM registers.  */
  tdep->ymmh_register_names = NULL;
  tdep->ymm0h_regnum = -1;

  tdep->num_byte_regs = 8;
  tdep->num_word_regs = 8;
  tdep->num_dword_regs = 0;
  tdep->num_mmx_regs = 8;
  tdep->num_ymm_regs = 0;

  tdesc_data = tdesc_data_alloc ();

  set_gdbarch_relocate_instruction (gdbarch, i386_relocate_instruction);

  set_gdbarch_gen_return_address (gdbarch, i386_gen_return_address);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  info.tdep_info = (void *) tdesc_data;
  gdbarch_init_osabi (info, gdbarch);

  if (!i386_validate_tdesc_p (tdep, tdesc_data))
    {
      tdesc_data_cleanup (tdesc_data);
      xfree (tdep);
      gdbarch_free (gdbarch);
      return NULL;
    }

  /* Wire in pseudo registers.  Number of pseudo registers may be
     changed.  */
  set_gdbarch_num_pseudo_regs (gdbarch, (tdep->num_byte_regs
					 + tdep->num_word_regs
					 + tdep->num_dword_regs
					 + tdep->num_mmx_regs
					 + tdep->num_ymm_regs));

  /* Target description may be changed.  */
  tdesc = tdep->tdesc;

  tdesc_use_registers (gdbarch, tdesc, tdesc_data);

  /* Override gdbarch_register_reggroup_p set in tdesc_use_registers.  */
  set_gdbarch_register_reggroup_p (gdbarch, tdep->register_reggroup_p);

  /* Make %al the first pseudo-register.  */
  tdep->al_regnum = gdbarch_num_regs (gdbarch);
  tdep->ax_regnum = tdep->al_regnum + tdep->num_byte_regs;

  ymm0_regnum = tdep->ax_regnum + tdep->num_word_regs;
  if (tdep->num_dword_regs)
    {
      /* Support dword pseudo-register if it hasn't been disabled.  */
      tdep->eax_regnum = ymm0_regnum;
      ymm0_regnum += tdep->num_dword_regs;
    }
  else
    tdep->eax_regnum = -1;

  mm0_regnum = ymm0_regnum;
  if (tdep->num_ymm_regs)
    {
      /* Support YMM pseudo-register if it is available.  */
      tdep->ymm0_regnum = ymm0_regnum;
      mm0_regnum += tdep->num_ymm_regs;
    }
  else
    tdep->ymm0_regnum = -1;

  if (tdep->num_mmx_regs != 0)
    {
      /* Support MMX pseudo-register if MMX hasn't been disabled.  */
      tdep->mm0_regnum = mm0_regnum;
    }
  else
    tdep->mm0_regnum = -1;

  /* Hook in the legacy prologue-based unwinders last (fallback).  */
  frame_unwind_append_unwinder (gdbarch, &i386_stack_tramp_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &i386_sigtramp_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &i386_frame_unwind);

  /* If we have a register mapping, enable the generic core file
     support, unless it has already been enabled.  */
  if (tdep->gregset_reg_offset
      && !gdbarch_regset_from_core_section_p (gdbarch))
    set_gdbarch_regset_from_core_section (gdbarch,
					  i386_regset_from_core_section);

  set_gdbarch_skip_permanent_breakpoint (gdbarch,
					 i386_skip_permanent_breakpoint);

  set_gdbarch_fast_tracepoint_valid_at (gdbarch,
					i386_fast_tracepoint_valid_at);

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


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386_tdep (void);

void
_initialize_i386_tdep (void)
{
  register_gdbarch_init (bfd_arch_i386, i386_gdbarch_init);

  /* Add the variable that controls the disassembly flavor.  */
  add_setshow_enum_cmd ("disassembly-flavor", no_class, valid_flavors,
			&disassembly_flavor, _("\
Set the disassembly flavor."), _("\
Show the disassembly flavor."), _("\
The valid values are \"att\" and \"intel\", and the default value is \"att\"."),
			NULL,
			NULL, /* FIXME: i18n: */
			&setlist, &showlist);

  /* Add the variable that controls the convention for returning
     structs.  */
  add_setshow_enum_cmd ("struct-convention", no_class, valid_conventions,
			&struct_convention, _("\
Set the convention for returning small structs."), _("\
Show the convention for returning small structs."), _("\
Valid values are \"default\", \"pcc\" and \"reg\", and the default value\n\
is \"default\"."),
			NULL,
			NULL, /* FIXME: i18n: */
			&setlist, &showlist);

  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_coff_flavour,
				  i386_coff_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_SVR4,
			  i386_svr4_init_abi);
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_GO32,
			  i386_go32_init_abi);

  /* Initialize the i386-specific register groups.  */
  i386_init_reggroups ();

  /* Initialize the standard target descriptions.  */
  initialize_tdesc_i386 ();
  initialize_tdesc_i386_mmx ();
  initialize_tdesc_i386_avx ();

  /* Tell remote stub that we support XML target description.  */
  register_remote_support_xml ("i386");
}
