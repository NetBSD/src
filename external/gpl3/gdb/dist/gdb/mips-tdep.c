/* Target-dependent code for the MIPS architecture, for GDB, the GNU Debugger.

   Copyright (C) 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
   2010, 2011 Free Software Foundation, Inc.

   Contributed by Alessandro Forin(af@cs.cmu.edu) at CMU
   and by Per Bothner(bothner@cs.wisc.edu) at U.Wisconsin.

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
#include "gdb_string.h"
#include "gdb_assert.h"
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
#include "arch-utils.h"
#include "regcache.h"
#include "osabi.h"
#include "mips-tdep.h"
#include "block.h"
#include "reggroups.h"
#include "opcode/mips.h"
#include "elf/mips.h"
#include "elf-bfd.h"
#include "symcat.h"
#include "sim-regno.h"
#include "dis-asm.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "infcall.h"
#include "floatformat.h"
#include "remote.h"
#include "target-descriptions.h"
#include "dwarf2-frame.h"
#include "user-regs.h"
#include "valprint.h"
#include "ax.h"

static const struct objfile_data *mips_pdr_data;

static struct type *mips_register_type (struct gdbarch *gdbarch, int regnum);

/* A useful bit in the CP0 status register (MIPS_PS_REGNUM).  */
/* This bit is set if we are emulating 32-bit FPRs on a 64-bit chip.  */
#define ST0_FR (1 << 26)

/* The sizes of floating point registers.  */

enum
{
  MIPS_FPU_SINGLE_REGSIZE = 4,
  MIPS_FPU_DOUBLE_REGSIZE = 8
};

enum
{
  MIPS32_REGSIZE = 4,
  MIPS64_REGSIZE = 8
};

static const char *mips_abi_string;

static const char *mips_abi_strings[] = {
  "auto",
  "n32",
  "o32",
  "n64",
  "o64",
  "eabi32",
  "eabi64",
  NULL
};

/* The standard register names, and all the valid aliases for them.  */
struct register_alias
{
  const char *name;
  int regnum;
};

/* Aliases for o32 and most other ABIs.  */
const struct register_alias mips_o32_aliases[] = {
  { "ta0", 12 },
  { "ta1", 13 },
  { "ta2", 14 },
  { "ta3", 15 }
};

/* Aliases for n32 and n64.  */
const struct register_alias mips_n32_n64_aliases[] = {
  { "ta0", 8 },
  { "ta1", 9 },
  { "ta2", 10 },
  { "ta3", 11 }
};

/* Aliases for ABI-independent registers.  */
const struct register_alias mips_register_aliases[] = {
  /* The architecture manuals specify these ABI-independent names for
     the GPRs.  */
#define R(n) { "r" #n, n }
  R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7),
  R(8), R(9), R(10), R(11), R(12), R(13), R(14), R(15),
  R(16), R(17), R(18), R(19), R(20), R(21), R(22), R(23),
  R(24), R(25), R(26), R(27), R(28), R(29), R(30), R(31),
#undef R

  /* k0 and k1 are sometimes called these instead (for "kernel
     temp").  */
  { "kt0", 26 },
  { "kt1", 27 },

  /* This is the traditional GDB name for the CP0 status register.  */
  { "sr", MIPS_PS_REGNUM },

  /* This is the traditional GDB name for the CP0 BadVAddr register.  */
  { "bad", MIPS_EMBED_BADVADDR_REGNUM },

  /* This is the traditional GDB name for the FCSR.  */
  { "fsr", MIPS_EMBED_FP0_REGNUM + 32 }
};

const struct register_alias mips_numeric_register_aliases[] = {
#define R(n) { #n, n }
  R(0), R(1), R(2), R(3), R(4), R(5), R(6), R(7),
  R(8), R(9), R(10), R(11), R(12), R(13), R(14), R(15),
  R(16), R(17), R(18), R(19), R(20), R(21), R(22), R(23),
  R(24), R(25), R(26), R(27), R(28), R(29), R(30), R(31),
#undef R
};

#ifndef MIPS_DEFAULT_FPU_TYPE
#define MIPS_DEFAULT_FPU_TYPE MIPS_FPU_DOUBLE
#endif
static int mips_fpu_type_auto = 1;
static enum mips_fpu_type mips_fpu_type = MIPS_DEFAULT_FPU_TYPE;

static int mips_debug = 0;

/* Properties (for struct target_desc) describing the g/G packet
   layout.  */
#define PROPERTY_GP32 "internal: transfers-32bit-registers"
#define PROPERTY_GP64 "internal: transfers-64bit-registers"

struct target_desc *mips_tdesc_gp32;
struct target_desc *mips_tdesc_gp64;

const struct mips_regnum *
mips_regnum (struct gdbarch *gdbarch)
{
  return gdbarch_tdep (gdbarch)->regnum;
}

static int
mips_fpa0_regnum (struct gdbarch *gdbarch)
{
  return mips_regnum (gdbarch)->fp0 + 12;
}

#define MIPS_EABI(gdbarch) (gdbarch_tdep (gdbarch)->mips_abi \
		     == MIPS_ABI_EABI32 \
		   || gdbarch_tdep (gdbarch)->mips_abi == MIPS_ABI_EABI64)

#define MIPS_LAST_FP_ARG_REGNUM(gdbarch) \
  (gdbarch_tdep (gdbarch)->mips_last_fp_arg_regnum)

#define MIPS_LAST_ARG_REGNUM(gdbarch) \
  (gdbarch_tdep (gdbarch)->mips_last_arg_regnum)

#define MIPS_FPU_TYPE(gdbarch) (gdbarch_tdep (gdbarch)->mips_fpu_type)

/* MIPS16 function addresses are odd (bit 0 is set).  Here are some
   functions to test, set, or clear bit 0 of addresses.  */

static CORE_ADDR
is_mips16_addr (CORE_ADDR addr)
{
  return ((addr) & 1);
}

static CORE_ADDR
unmake_mips16_addr (CORE_ADDR addr)
{
  return ((addr) & ~(CORE_ADDR) 1);
}

static CORE_ADDR
make_mips16_addr (CORE_ADDR addr)
{
  return ((addr) | (CORE_ADDR) 1);
}

/* Return the MIPS ABI associated with GDBARCH.  */
enum mips_abi
mips_abi (struct gdbarch *gdbarch)
{
  return gdbarch_tdep (gdbarch)->mips_abi;
}

int
mips_isa_regsize (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* If we know how big the registers are, use that size.  */
  if (tdep->register_size_valid_p)
    return tdep->register_size;

  /* Fall back to the previous behavior.  */
  return (gdbarch_bfd_arch_info (gdbarch)->bits_per_word
	  / gdbarch_bfd_arch_info (gdbarch)->bits_per_byte);
}

/* Return the currently configured (or set) saved register size.  */

unsigned int
mips_abi_regsize (struct gdbarch *gdbarch)
{
  switch (mips_abi (gdbarch))
    {
    case MIPS_ABI_EABI32:
    case MIPS_ABI_O32:
      return 4;
    case MIPS_ABI_N32:
    case MIPS_ABI_N64:
    case MIPS_ABI_O64:
    case MIPS_ABI_EABI64:
      return 8;
    case MIPS_ABI_UNKNOWN:
    case MIPS_ABI_LAST:
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }
}

/* Functions for setting and testing a bit in a minimal symbol that
   marks it as 16-bit function.  The MSB of the minimal symbol's
   "info" field is used for this purpose.

   gdbarch_elf_make_msymbol_special tests whether an ELF symbol is "special",
   i.e. refers to a 16-bit function, and sets a "special" bit in a
   minimal symbol to mark it as a 16-bit function

   MSYMBOL_IS_SPECIAL   tests the "special" bit in a minimal symbol  */

static void
mips_elf_make_msymbol_special (asymbol * sym, struct minimal_symbol *msym)
{
  if (((elf_symbol_type *) (sym))->internal_elf_sym.st_other == STO_MIPS16)
    {
      MSYMBOL_TARGET_FLAG_1 (msym) = 1;
    }
}

static int
msymbol_is_special (struct minimal_symbol *msym)
{
  return MSYMBOL_TARGET_FLAG_1 (msym);
}

/* XFER a value from the big/little/left end of the register.
   Depending on the size of the value it might occupy the entire
   register or just part of it.  Make an allowance for this, aligning
   things accordingly.  */

static void
mips_xfer_register (struct gdbarch *gdbarch, struct regcache *regcache,
		    int reg_num, int length,
		    enum bfd_endian endian, gdb_byte *in,
		    const gdb_byte *out, int buf_offset)
{
  int reg_offset = 0;

  gdb_assert (reg_num >= gdbarch_num_regs (gdbarch));
  /* Need to transfer the left or right part of the register, based on
     the targets byte order.  */
  switch (endian)
    {
    case BFD_ENDIAN_BIG:
      reg_offset = register_size (gdbarch, reg_num) - length;
      break;
    case BFD_ENDIAN_LITTLE:
      reg_offset = 0;
      break;
    case BFD_ENDIAN_UNKNOWN:	/* Indicates no alignment.  */
      reg_offset = 0;
      break;
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }
  if (mips_debug)
    fprintf_unfiltered (gdb_stderr,
			"xfer $%d, reg offset %d, buf offset %d, length %d, ",
			reg_num, reg_offset, buf_offset, length);
  if (mips_debug && out != NULL)
    {
      int i;
      fprintf_unfiltered (gdb_stdlog, "out ");
      for (i = 0; i < length; i++)
	fprintf_unfiltered (gdb_stdlog, "%02x", out[buf_offset + i]);
    }
  if (in != NULL)
    regcache_cooked_read_part (regcache, reg_num, reg_offset, length,
			       in + buf_offset);
  if (out != NULL)
    regcache_cooked_write_part (regcache, reg_num, reg_offset, length,
				out + buf_offset);
  if (mips_debug && in != NULL)
    {
      int i;
      fprintf_unfiltered (gdb_stdlog, "in ");
      for (i = 0; i < length; i++)
	fprintf_unfiltered (gdb_stdlog, "%02x", in[buf_offset + i]);
    }
  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog, "\n");
}

/* Determine if a MIPS3 or later cpu is operating in MIPS{1,2} FPU
   compatiblity mode.  A return value of 1 means that we have
   physical 64-bit registers, but should treat them as 32-bit registers.  */

static int
mips2_fp_compat (struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  /* MIPS1 and MIPS2 have only 32 bit FPRs, and the FR bit is not
     meaningful.  */
  if (register_size (gdbarch, mips_regnum (gdbarch)->fp0) == 4)
    return 0;

#if 0
  /* FIXME drow 2002-03-10: This is disabled until we can do it consistently,
     in all the places we deal with FP registers.  PR gdb/413.  */
  /* Otherwise check the FR bit in the status register - it controls
     the FP compatiblity mode.  If it is clear we are in compatibility
     mode.  */
  if ((get_frame_register_unsigned (frame, MIPS_PS_REGNUM) & ST0_FR) == 0)
    return 1;
#endif

  return 0;
}

#define VM_MIN_ADDRESS (CORE_ADDR)0x400000

static CORE_ADDR heuristic_proc_start (struct gdbarch *, CORE_ADDR);

static void reinit_frame_cache_sfunc (char *, int, struct cmd_list_element *);

/* The list of available "set mips " and "show mips " commands.  */

static struct cmd_list_element *setmipscmdlist = NULL;
static struct cmd_list_element *showmipscmdlist = NULL;

/* Integer registers 0 thru 31 are handled explicitly by
   mips_register_name().  Processor specific registers 32 and above
   are listed in the following tables.  */

enum
{ NUM_MIPS_PROCESSOR_REGS = (90 - 32) };

/* Generic MIPS.  */

static const char *mips_generic_reg_names[NUM_MIPS_PROCESSOR_REGS] = {
  "sr", "lo", "hi", "bad", "cause", "pc",
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
  "fsr", "fir", "" /*"fp" */ , "",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
};

/* Names of IDT R3041 registers.  */

static const char *mips_r3041_reg_names[] = {
  "sr", "lo", "hi", "bad", "cause", "pc",
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
  "fsr", "fir", "", /*"fp" */ "",
  "", "", "bus", "ccfg", "", "", "", "",
  "", "", "port", "cmp", "", "", "epc", "prid",
};

/* Names of tx39 registers.  */

static const char *mips_tx39_reg_names[NUM_MIPS_PROCESSOR_REGS] = {
  "sr", "lo", "hi", "bad", "cause", "pc",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "config", "cache", "debug", "depc", "epc", ""
};

/* Names of IRIX registers.  */
static const char *mips_irix_reg_names[NUM_MIPS_PROCESSOR_REGS] = {
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
  "pc", "cause", "bad", "hi", "lo", "fsr", "fir"
};


/* Return the name of the register corresponding to REGNO.  */
static const char *
mips_register_name (struct gdbarch *gdbarch, int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  /* GPR names for all ABIs other than n32/n64.  */
  static char *mips_gpr_names[] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
  };

  /* GPR names for n32 and n64 ABIs.  */
  static char *mips_n32_n64_gpr_names[] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "a4", "a5", "a6", "a7", "t0", "t1", "t2", "t3",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
  };

  enum mips_abi abi = mips_abi (gdbarch);

  /* Map [gdbarch_num_regs .. 2*gdbarch_num_regs) onto the raw registers, 
     but then don't make the raw register names visible.  This (upper)
     range of user visible register numbers are the pseudo-registers.

     This approach was adopted accommodate the following scenario:
     It is possible to debug a 64-bit device using a 32-bit
     programming model.  In such instances, the raw registers are
     configured to be 64-bits wide, while the pseudo registers are
     configured to be 32-bits wide.  The registers that the user
     sees - the pseudo registers - match the users expectations
     given the programming model being used.  */
  int rawnum = regno % gdbarch_num_regs (gdbarch);
  if (regno < gdbarch_num_regs (gdbarch))
    return "";

  /* The MIPS integer registers are always mapped from 0 to 31.  The
     names of the registers (which reflects the conventions regarding
     register use) vary depending on the ABI.  */
  if (0 <= rawnum && rawnum < 32)
    {
      if (abi == MIPS_ABI_N32 || abi == MIPS_ABI_N64)
	return mips_n32_n64_gpr_names[rawnum];
      else
	return mips_gpr_names[rawnum];
    }
  else if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_name (gdbarch, rawnum);
  else if (32 <= rawnum && rawnum < gdbarch_num_regs (gdbarch))
    {
      gdb_assert (rawnum - 32 < NUM_MIPS_PROCESSOR_REGS);
      return tdep->mips_processor_reg_names[rawnum - 32];
    }
  else
    internal_error (__FILE__, __LINE__,
		    _("mips_register_name: bad register number %d"), rawnum);
}

/* Return the groups that a MIPS register can be categorised into.  */

static int
mips_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			  struct reggroup *reggroup)
{
  int vector_p;
  int float_p;
  int raw_p;
  int rawnum = regnum % gdbarch_num_regs (gdbarch);
  int pseudo = regnum / gdbarch_num_regs (gdbarch);
  if (reggroup == all_reggroup)
    return pseudo;
  vector_p = TYPE_VECTOR (register_type (gdbarch, regnum));
  float_p = TYPE_CODE (register_type (gdbarch, regnum)) == TYPE_CODE_FLT;
  /* FIXME: cagney/2003-04-13: Can't yet use gdbarch_num_regs
     (gdbarch), as not all architectures are multi-arch.  */
  raw_p = rawnum < gdbarch_num_regs (gdbarch);
  if (gdbarch_register_name (gdbarch, regnum) == NULL
      || gdbarch_register_name (gdbarch, regnum)[0] == '\0')
    return 0;
  if (reggroup == float_reggroup)
    return float_p && pseudo;
  if (reggroup == vector_reggroup)
    return vector_p && pseudo;
  if (reggroup == general_reggroup)
    return (!vector_p && !float_p) && pseudo;
  /* Save the pseudo registers.  Need to make certain that any code
     extracting register values from a saved register cache also uses
     pseudo registers.  */
  if (reggroup == save_reggroup)
    return raw_p && pseudo;
  /* Restore the same pseudo register.  */
  if (reggroup == restore_reggroup)
    return raw_p && pseudo;
  return 0;
}

/* Return the groups that a MIPS register can be categorised into.
   This version is only used if we have a target description which
   describes real registers (and their groups).  */

static int
mips_tdesc_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				struct reggroup *reggroup)
{
  int rawnum = regnum % gdbarch_num_regs (gdbarch);
  int pseudo = regnum / gdbarch_num_regs (gdbarch);
  int ret;

  /* Only save, restore, and display the pseudo registers.  Need to
     make certain that any code extracting register values from a
     saved register cache also uses pseudo registers.

     Note: saving and restoring the pseudo registers is slightly
     strange; if we have 64 bits, we should save and restore all
     64 bits.  But this is hard and has little benefit.  */
  if (!pseudo)
    return 0;

  ret = tdesc_register_in_reggroup_p (gdbarch, rawnum, reggroup);
  if (ret != -1)
    return ret;

  return mips_register_reggroup_p (gdbarch, regnum, reggroup);
}

/* Map the symbol table registers which live in the range [1 *
   gdbarch_num_regs .. 2 * gdbarch_num_regs) back onto the corresponding raw
   registers.  Take care of alignment and size problems.  */

static enum register_status
mips_pseudo_register_read (struct gdbarch *gdbarch, struct regcache *regcache,
			   int cookednum, gdb_byte *buf)
{
  int rawnum = cookednum % gdbarch_num_regs (gdbarch);
  gdb_assert (cookednum >= gdbarch_num_regs (gdbarch)
	      && cookednum < 2 * gdbarch_num_regs (gdbarch));
  if (register_size (gdbarch, rawnum) == register_size (gdbarch, cookednum))
    return regcache_raw_read (regcache, rawnum, buf);
  else if (register_size (gdbarch, rawnum) >
	   register_size (gdbarch, cookednum))
    {
      if (gdbarch_tdep (gdbarch)->mips64_transfers_32bit_regs_p)
	return regcache_raw_read_part (regcache, rawnum, 0, 4, buf);
      else
	{
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	  LONGEST regval;
	  enum register_status status;

	  status = regcache_raw_read_signed (regcache, rawnum, &regval);
	  if (status == REG_VALID)
	    store_signed_integer (buf, 4, byte_order, regval);
	  return status;
	}
    }
  else
    internal_error (__FILE__, __LINE__, _("bad register size"));
}

static void
mips_pseudo_register_write (struct gdbarch *gdbarch,
			    struct regcache *regcache, int cookednum,
			    const gdb_byte *buf)
{
  int rawnum = cookednum % gdbarch_num_regs (gdbarch);
  gdb_assert (cookednum >= gdbarch_num_regs (gdbarch)
	      && cookednum < 2 * gdbarch_num_regs (gdbarch));
  if (register_size (gdbarch, rawnum) == register_size (gdbarch, cookednum))
    regcache_raw_write (regcache, rawnum, buf);
  else if (register_size (gdbarch, rawnum) >
	   register_size (gdbarch, cookednum))
    {
      if (gdbarch_tdep (gdbarch)->mips64_transfers_32bit_regs_p)
	regcache_raw_write_part (regcache, rawnum, 0, 4, buf);
      else
	{
	  /* Sign extend the shortened version of the register prior
	     to placing it in the raw register.  This is required for
	     some mips64 parts in order to avoid unpredictable behavior.  */
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	  LONGEST regval = extract_signed_integer (buf, 4, byte_order);
	  regcache_raw_write_signed (regcache, rawnum, regval);
	}
    }
  else
    internal_error (__FILE__, __LINE__, _("bad register size"));
}

static int
mips_ax_pseudo_register_collect (struct gdbarch *gdbarch,
				 struct agent_expr *ax, int reg)
{
  int rawnum = reg % gdbarch_num_regs (gdbarch);
  gdb_assert (reg >= gdbarch_num_regs (gdbarch)
	      && reg < 2 * gdbarch_num_regs (gdbarch));

  ax_reg_mask (ax, rawnum);

  return 0;
}

static int
mips_ax_pseudo_register_push_stack (struct gdbarch *gdbarch,
				    struct agent_expr *ax, int reg)
{
  int rawnum = reg % gdbarch_num_regs (gdbarch);
  gdb_assert (reg >= gdbarch_num_regs (gdbarch)
	      && reg < 2 * gdbarch_num_regs (gdbarch));
  if (register_size (gdbarch, rawnum) >= register_size (gdbarch, reg))
    {
      ax_reg (ax, rawnum);

      if (register_size (gdbarch, rawnum) > register_size (gdbarch, reg))
        {
	  if (!gdbarch_tdep (gdbarch)->mips64_transfers_32bit_regs_p
	      || gdbarch_byte_order (gdbarch) != BFD_ENDIAN_BIG)
	    {
	      ax_const_l (ax, 32);
	      ax_simple (ax, aop_lsh);
	    }
	  ax_const_l (ax, 32);
	  ax_simple (ax, aop_rsh_signed);
	}
    }
  else
    internal_error (__FILE__, __LINE__, _("bad register size"));

  return 0;
}

/* Table to translate MIPS16 register field to actual register number.  */
static int mips16_to_32_reg[8] = { 16, 17, 2, 3, 4, 5, 6, 7 };

/* Heuristic_proc_start may hunt through the text section for a long
   time across a 2400 baud serial line.  Allows the user to limit this
   search.  */

static unsigned int heuristic_fence_post = 0;

/* Number of bytes of storage in the actual machine representation for
   register N.  NOTE: This defines the pseudo register type so need to
   rebuild the architecture vector.  */

static int mips64_transfers_32bit_regs_p = 0;

static void
set_mips64_transfers_32bit_regs (char *args, int from_tty,
				 struct cmd_list_element *c)
{
  struct gdbarch_info info;
  gdbarch_info_init (&info);
  /* FIXME: cagney/2003-11-15: Should be setting a field in "info"
     instead of relying on globals.  Doing that would let generic code
     handle the search for this specific architecture.  */
  if (!gdbarch_update_p (info))
    {
      mips64_transfers_32bit_regs_p = 0;
      error (_("32-bit compatibility mode not supported"));
    }
}

/* Convert to/from a register and the corresponding memory value.  */

/* This predicate tests for the case of an 8 byte floating point
   value that is being transferred to or from a pair of floating point
   registers each of which are (or are considered to be) only 4 bytes
   wide.  */
static int
mips_convert_register_float_case_p (struct gdbarch *gdbarch, int regnum,
				    struct type *type)
{
  return (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
	  && register_size (gdbarch, regnum) == 4
	  && (regnum % gdbarch_num_regs (gdbarch))
		>= mips_regnum (gdbarch)->fp0
	  && (regnum % gdbarch_num_regs (gdbarch))
		< mips_regnum (gdbarch)->fp0 + 32
	  && TYPE_CODE (type) == TYPE_CODE_FLT && TYPE_LENGTH (type) == 8);
}

/* This predicate tests for the case of a value of less than 8
   bytes in width that is being transfered to or from an 8 byte
   general purpose register.  */
static int
mips_convert_register_gpreg_case_p (struct gdbarch *gdbarch, int regnum,
				    struct type *type)
{
  int num_regs = gdbarch_num_regs (gdbarch);

  return (register_size (gdbarch, regnum) == 8
          && regnum % num_regs > 0 && regnum % num_regs < 32
          && TYPE_LENGTH (type) < 8);
}

static int
mips_convert_register_p (struct gdbarch *gdbarch,
			 int regnum, struct type *type)
{
  return mips_convert_register_float_case_p (gdbarch, regnum, type)
      || mips_convert_register_gpreg_case_p (gdbarch, regnum, type);
}

static int
mips_register_to_value (struct frame_info *frame, int regnum,
			struct type *type, gdb_byte *to,
			int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  if (mips_convert_register_float_case_p (gdbarch, regnum, type))
    {
      get_frame_register (frame, regnum + 0, to + 4);
      get_frame_register (frame, regnum + 1, to + 0);

      if (!get_frame_register_bytes (frame, regnum + 0, 0, 4, to + 4,
				     optimizedp, unavailablep))
	return 0;

      if (!get_frame_register_bytes (frame, regnum + 1, 0, 4, to + 0,
				     optimizedp, unavailablep))
	return 0;
      *optimizedp = *unavailablep = 0;
      return 1;
    }
  else if (mips_convert_register_gpreg_case_p (gdbarch, regnum, type))
    {
      int len = TYPE_LENGTH (type);
      CORE_ADDR offset;

      offset = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG ? 8 - len : 0;
      if (!get_frame_register_bytes (frame, regnum, offset, len, to,
				     optimizedp, unavailablep))
	return 0;

      *optimizedp = *unavailablep = 0;
      return 1;
    }
  else
    {
      internal_error (__FILE__, __LINE__,
                      _("mips_register_to_value: unrecognized case"));
    }
}

static void
mips_value_to_register (struct frame_info *frame, int regnum,
			struct type *type, const gdb_byte *from)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  if (mips_convert_register_float_case_p (gdbarch, regnum, type))
    {
      put_frame_register (frame, regnum + 0, from + 4);
      put_frame_register (frame, regnum + 1, from + 0);
    }
  else if (mips_convert_register_gpreg_case_p (gdbarch, regnum, type))
    {
      gdb_byte fill[8];
      int len = TYPE_LENGTH (type);
      
      /* Sign extend values, irrespective of type, that are stored to 
         a 64-bit general purpose register.  (32-bit unsigned values
	 are stored as signed quantities within a 64-bit register.
	 When performing an operation, in compiled code, that combines
	 a 32-bit unsigned value with a signed 64-bit value, a type
	 conversion is first performed that zeroes out the high 32 bits.)  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	{
	  if (from[0] & 0x80)
	    store_signed_integer (fill, 8, BFD_ENDIAN_BIG, -1);
	  else
	    store_signed_integer (fill, 8, BFD_ENDIAN_BIG, 0);
	  put_frame_register_bytes (frame, regnum, 0, 8 - len, fill);
	  put_frame_register_bytes (frame, regnum, 8 - len, len, from);
	}
      else
	{
	  if (from[len-1] & 0x80)
	    store_signed_integer (fill, 8, BFD_ENDIAN_LITTLE, -1);
	  else
	    store_signed_integer (fill, 8, BFD_ENDIAN_LITTLE, 0);
	  put_frame_register_bytes (frame, regnum, 0, len, from);
	  put_frame_register_bytes (frame, regnum, len, 8 - len, fill);
	}
    }
  else
    {
      internal_error (__FILE__, __LINE__,
                      _("mips_value_to_register: unrecognized case"));
    }
}

/* Return the GDB type object for the "standard" data type of data in
   register REG.  */

static struct type *
mips_register_type (struct gdbarch *gdbarch, int regnum)
{
  gdb_assert (regnum >= 0 && regnum < 2 * gdbarch_num_regs (gdbarch));
  if ((regnum % gdbarch_num_regs (gdbarch)) >= mips_regnum (gdbarch)->fp0
      && (regnum % gdbarch_num_regs (gdbarch))
	 < mips_regnum (gdbarch)->fp0 + 32)
    {
      /* The floating-point registers raw, or cooked, always match
         mips_isa_regsize(), and also map 1:1, byte for byte.  */
      if (mips_isa_regsize (gdbarch) == 4)
	return builtin_type (gdbarch)->builtin_float;
      else
	return builtin_type (gdbarch)->builtin_double;
    }
  else if (regnum < gdbarch_num_regs (gdbarch))
    {
      /* The raw or ISA registers.  These are all sized according to
	 the ISA regsize.  */
      if (mips_isa_regsize (gdbarch) == 4)
	return builtin_type (gdbarch)->builtin_int32;
      else
	return builtin_type (gdbarch)->builtin_int64;
    }
  else
    {
      /* The cooked or ABI registers.  These are sized according to
	 the ABI (with a few complications).  */
      if (regnum >= (gdbarch_num_regs (gdbarch)
		     + mips_regnum (gdbarch)->fp_control_status)
	  && regnum <= gdbarch_num_regs (gdbarch) + MIPS_LAST_EMBED_REGNUM)
	/* The pseudo/cooked view of the embedded registers is always
	   32-bit.  The raw view is handled below.  */
	return builtin_type (gdbarch)->builtin_int32;
      else if (gdbarch_tdep (gdbarch)->mips64_transfers_32bit_regs_p)
	/* The target, while possibly using a 64-bit register buffer,
	   is only transfering 32-bits of each integer register.
	   Reflect this in the cooked/pseudo (ABI) register value.  */
	return builtin_type (gdbarch)->builtin_int32;
      else if (mips_abi_regsize (gdbarch) == 4)
	/* The ABI is restricted to 32-bit registers (the ISA could be
	   32- or 64-bit).  */
	return builtin_type (gdbarch)->builtin_int32;
      else
	/* 64-bit ABI.  */
	return builtin_type (gdbarch)->builtin_int64;
    }
}

/* Return the GDB type for the pseudo register REGNUM, which is the
   ABI-level view.  This function is only called if there is a target
   description which includes registers, so we know precisely the
   types of hardware registers.  */

static struct type *
mips_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  const int num_regs = gdbarch_num_regs (gdbarch);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int rawnum = regnum % num_regs;
  struct type *rawtype;

  gdb_assert (regnum >= num_regs && regnum < 2 * num_regs);

  /* Absent registers are still absent.  */
  rawtype = gdbarch_register_type (gdbarch, rawnum);
  if (TYPE_LENGTH (rawtype) == 0)
    return rawtype;

  if (rawnum >= MIPS_EMBED_FP0_REGNUM && rawnum < MIPS_EMBED_FP0_REGNUM + 32)
    /* Present the floating point registers however the hardware did;
       do not try to convert between FPU layouts.  */
    return rawtype;

  if (rawnum >= MIPS_EMBED_FP0_REGNUM + 32 && rawnum <= MIPS_LAST_EMBED_REGNUM)
    {
      /* The pseudo/cooked view of embedded registers is always
	 32-bit, even if the target transfers 64-bit values for them.
	 New targets relying on XML descriptions should only transfer
	 the necessary 32 bits, but older versions of GDB expected 64,
	 so allow the target to provide 64 bits without interfering
	 with the displayed type.  */
      return builtin_type (gdbarch)->builtin_int32;
    }

  /* Use pointer types for registers if we can.  For n32 we can not,
     since we do not have a 64-bit pointer type.  */
  if (mips_abi_regsize (gdbarch)
      == TYPE_LENGTH (builtin_type (gdbarch)->builtin_data_ptr))
    {
      if (rawnum == MIPS_SP_REGNUM || rawnum == MIPS_EMBED_BADVADDR_REGNUM)
	return builtin_type (gdbarch)->builtin_data_ptr;
      else if (rawnum == MIPS_EMBED_PC_REGNUM)
	return builtin_type (gdbarch)->builtin_func_ptr;
    }

  if (mips_abi_regsize (gdbarch) == 4 && TYPE_LENGTH (rawtype) == 8
      && rawnum >= MIPS_ZERO_REGNUM && rawnum <= MIPS_EMBED_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_int32;

  /* For all other registers, pass through the hardware type.  */
  return rawtype;
}

/* Should the upper word of 64-bit addresses be zeroed?  */
enum auto_boolean mask_address_var = AUTO_BOOLEAN_AUTO;

static int
mips_mask_address_p (struct gdbarch_tdep *tdep)
{
  switch (mask_address_var)
    {
    case AUTO_BOOLEAN_TRUE:
      return 1;
    case AUTO_BOOLEAN_FALSE:
      return 0;
      break;
    case AUTO_BOOLEAN_AUTO:
      return tdep->default_mask_address_p;
    default:
      internal_error (__FILE__, __LINE__,
		      _("mips_mask_address_p: bad switch"));
      return -1;
    }
}

static void
show_mask_address (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (target_gdbarch);

  deprecated_show_value_hack (file, from_tty, c, value);
  switch (mask_address_var)
    {
    case AUTO_BOOLEAN_TRUE:
      printf_filtered ("The 32 bit mips address mask is enabled\n");
      break;
    case AUTO_BOOLEAN_FALSE:
      printf_filtered ("The 32 bit mips address mask is disabled\n");
      break;
    case AUTO_BOOLEAN_AUTO:
      printf_filtered
	("The 32 bit address mask is set automatically.  Currently %s\n",
	 mips_mask_address_p (tdep) ? "enabled" : "disabled");
      break;
    default:
      internal_error (__FILE__, __LINE__, _("show_mask_address: bad switch"));
      break;
    }
}

/* Tell if the program counter value in MEMADDR is in a MIPS16 function.  */

int
mips_pc_is_mips16 (CORE_ADDR memaddr)
{
  struct minimal_symbol *sym;

  /* If bit 0 of the address is set, assume this is a MIPS16 address.  */
  if (is_mips16_addr (memaddr))
    return 1;

  /* A flag indicating that this is a MIPS16 function is stored by elfread.c in
     the high bit of the info field.  Use this to decide if the function is
     MIPS16 or normal MIPS.  */
  sym = lookup_minimal_symbol_by_pc (memaddr);
  if (sym)
    return msymbol_is_special (sym);
  else
    return 0;
}

/* MIPS believes that the PC has a sign extended value.  Perhaps the
   all registers should be sign extended for simplicity?  */

static CORE_ADDR
mips_read_pc (struct regcache *regcache)
{
  ULONGEST pc;
  int regnum = mips_regnum (get_regcache_arch (regcache))->pc;
  regcache_cooked_read_signed (regcache, regnum, &pc);
  if (is_mips16_addr (pc))
    pc = unmake_mips16_addr (pc);
  return pc;
}

static CORE_ADDR
mips_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  ULONGEST pc;

  pc = frame_unwind_register_signed
	 (next_frame, gdbarch_num_regs (gdbarch) + mips_regnum (gdbarch)->pc);
  if (is_mips16_addr (pc))
    pc = unmake_mips16_addr (pc);
  return pc;
}

static CORE_ADDR
mips_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  return frame_unwind_register_signed
	   (next_frame, gdbarch_num_regs (gdbarch) + MIPS_SP_REGNUM);
}

/* Assuming THIS_FRAME is a dummy, return the frame ID of that
   dummy frame.  The frame ID's base needs to match the TOS value
   saved by save_dummy_frame_tos(), and the PC match the dummy frame's
   breakpoint.  */

static struct frame_id
mips_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  return frame_id_build
	   (get_frame_register_signed (this_frame,
				       gdbarch_num_regs (gdbarch)
				       + MIPS_SP_REGNUM),
	    get_frame_pc (this_frame));
}

static void
mips_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  int regnum = mips_regnum (get_regcache_arch (regcache))->pc;
  if (mips_pc_is_mips16 (pc))
    regcache_cooked_write_unsigned (regcache, regnum, make_mips16_addr (pc));
  else
    regcache_cooked_write_unsigned (regcache, regnum, pc);
}

/* Fetch and return instruction from the specified location.  If the PC
   is odd, assume it's a MIPS16 instruction; otherwise MIPS32.  */

static ULONGEST
mips_fetch_instruction (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[MIPS_INSN32_SIZE];
  int instlen;
  int status;

  if (mips_pc_is_mips16 (addr))
    {
      instlen = MIPS_INSN16_SIZE;
      addr = unmake_mips16_addr (addr);
    }
  else
    instlen = MIPS_INSN32_SIZE;
  status = target_read_memory (addr, buf, instlen);
  if (status)
    memory_error (status, addr);
  return extract_unsigned_integer (buf, instlen, byte_order);
}

/* These are the fields of 32 bit mips instructions.  */
#define mips32_op(x) (x >> 26)
#define itype_op(x) (x >> 26)
#define itype_rs(x) ((x >> 21) & 0x1f)
#define itype_rt(x) ((x >> 16) & 0x1f)
#define itype_immediate(x) (x & 0xffff)

#define jtype_op(x) (x >> 26)
#define jtype_target(x) (x & 0x03ffffff)

#define rtype_op(x) (x >> 26)
#define rtype_rs(x) ((x >> 21) & 0x1f)
#define rtype_rt(x) ((x >> 16) & 0x1f)
#define rtype_rd(x) ((x >> 11) & 0x1f)
#define rtype_shamt(x) ((x >> 6) & 0x1f)
#define rtype_funct(x) (x & 0x3f)

static LONGEST
mips32_relative_offset (ULONGEST inst)
{
  return ((itype_immediate (inst) ^ 0x8000) - 0x8000) << 2;
}

/* Determine where to set a single step breakpoint while considering
   branch prediction.  */
static CORE_ADDR
mips32_next_pc (struct frame_info *frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  unsigned long inst;
  int op;
  inst = mips_fetch_instruction (gdbarch, pc);
  if ((inst & 0xe0000000) != 0)		/* Not a special, jump or branch
					   instruction.  */
    {
      if (itype_op (inst) >> 2 == 5)
	/* BEQL, BNEL, BLEZL, BGTZL: bits 0101xx */
	{
	  op = (itype_op (inst) & 0x03);
	  switch (op)
	    {
	    case 0:		/* BEQL */
	      goto equal_branch;
	    case 1:		/* BNEL */
	      goto neq_branch;
	    case 2:		/* BLEZL */
	      goto less_branch;
	    case 3:		/* BGTZL */
	      goto greater_branch;
	    default:
	      pc += 4;
	    }
	}
      else if (itype_op (inst) == 17 && itype_rs (inst) == 8)
	/* BC1F, BC1FL, BC1T, BC1TL: 010001 01000 */
	{
	  int tf = itype_rt (inst) & 0x01;
	  int cnum = itype_rt (inst) >> 2;
	  int fcrcs =
	    get_frame_register_signed (frame,
				       mips_regnum (get_frame_arch (frame))->
						fp_control_status);
	  int cond = ((fcrcs >> 24) & 0x0e) | ((fcrcs >> 23) & 0x01);

	  if (((cond >> cnum) & 0x01) == tf)
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	}
      else
	pc += 4;		/* Not a branch, next instruction is easy.  */
    }
  else
    {				/* This gets way messy.  */

      /* Further subdivide into SPECIAL, REGIMM and other.  */
      switch (op = itype_op (inst) & 0x07)	/* Extract bits 28,27,26.  */
	{
	case 0:		/* SPECIAL */
	  op = rtype_funct (inst);
	  switch (op)
	    {
	    case 8:		/* JR */
	    case 9:		/* JALR */
	      /* Set PC to that address.  */
	      pc = get_frame_register_signed (frame, rtype_rs (inst));
	      break;
	    case 12:            /* SYSCALL */
	      {
		struct gdbarch_tdep *tdep;

		tdep = gdbarch_tdep (get_frame_arch (frame));
		if (tdep->syscall_next_pc != NULL)
		  pc = tdep->syscall_next_pc (frame);
		else
		  pc += 4;
	      }
	      break;
	    default:
	      pc += 4;
	    }

	  break;		/* end SPECIAL */
	case 1:			/* REGIMM */
	  {
	    op = itype_rt (inst);	/* branch condition */
	    switch (op)
	      {
	      case 0:		/* BLTZ */
	      case 2:		/* BLTZL */
	      case 16:		/* BLTZAL */
	      case 18:		/* BLTZALL */
	      less_branch:
		if (get_frame_register_signed (frame, itype_rs (inst)) < 0)
		  pc += mips32_relative_offset (inst) + 4;
		else
		  pc += 8;	/* after the delay slot */
		break;
	      case 1:		/* BGEZ */
	      case 3:		/* BGEZL */
	      case 17:		/* BGEZAL */
	      case 19:		/* BGEZALL */
		if (get_frame_register_signed (frame, itype_rs (inst)) >= 0)
		  pc += mips32_relative_offset (inst) + 4;
		else
		  pc += 8;	/* after the delay slot */
		break;
		/* All of the other instructions in the REGIMM category */
	      default:
		pc += 4;
	      }
	  }
	  break;		/* end REGIMM */
	case 2:		/* J */
	case 3:		/* JAL */
	  {
	    unsigned long reg;
	    reg = jtype_target (inst) << 2;
	    /* Upper four bits get never changed...  */
	    pc = reg + ((pc + 4) & ~(CORE_ADDR) 0x0fffffff);
	  }
	  break;
	  /* FIXME case JALX : */
	  {
	    unsigned long reg;
	    reg = jtype_target (inst) << 2;
	    pc = reg + ((pc + 4) & ~(CORE_ADDR) 0x0fffffff) + 1;  /* yes, +1 */
	    /* Add 1 to indicate 16 bit mode - Invert ISA mode */
	  }
	  break;		/* The new PC will be alternate mode */
	case 4:		/* BEQ, BEQL */
	equal_branch:
	  if (get_frame_register_signed (frame, itype_rs (inst)) ==
	      get_frame_register_signed (frame, itype_rt (inst)))
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 5:		/* BNE, BNEL */
	neq_branch:
	  if (get_frame_register_signed (frame, itype_rs (inst)) !=
	      get_frame_register_signed (frame, itype_rt (inst)))
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 6:		/* BLEZ, BLEZL */
	  if (get_frame_register_signed (frame, itype_rs (inst)) <= 0)
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 7:
	default:
	greater_branch:	/* BGTZ, BGTZL */
	  if (get_frame_register_signed (frame, itype_rs (inst)) > 0)
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	}			/* switch */
    }				/* else */
  return pc;
}				/* mips32_next_pc */

/* Decoding the next place to set a breakpoint is irregular for the
   mips 16 variant, but fortunately, there fewer instructions.  We have
   to cope ith extensions for 16 bit instructions and a pair of actual
   32 bit instructions.  We dont want to set a single step instruction
   on the extend instruction either.  */

/* Lots of mips16 instruction formats */
/* Predicting jumps requires itype,ritype,i8type
   and their extensions      extItype,extritype,extI8type.  */
enum mips16_inst_fmts
{
  itype,			/* 0  immediate 5,10 */
  ritype,			/* 1   5,3,8 */
  rrtype,			/* 2   5,3,3,5 */
  rritype,			/* 3   5,3,3,5 */
  rrrtype,			/* 4   5,3,3,3,2 */
  rriatype,			/* 5   5,3,3,1,4 */
  shifttype,			/* 6   5,3,3,3,2 */
  i8type,			/* 7   5,3,8 */
  i8movtype,			/* 8   5,3,3,5 */
  i8mov32rtype,			/* 9   5,3,5,3 */
  i64type,			/* 10  5,3,8 */
  ri64type,			/* 11  5,3,3,5 */
  jalxtype,			/* 12  5,1,5,5,16 - a 32 bit instruction */
  exiItype,			/* 13  5,6,5,5,1,1,1,1,1,1,5 */
  extRitype,			/* 14  5,6,5,5,3,1,1,1,5 */
  extRRItype,			/* 15  5,5,5,5,3,3,5 */
  extRRIAtype,			/* 16  5,7,4,5,3,3,1,4 */
  EXTshifttype,			/* 17  5,5,1,1,1,1,1,1,5,3,3,1,1,1,2 */
  extI8type,			/* 18  5,6,5,5,3,1,1,1,5 */
  extI64type,			/* 19  5,6,5,5,3,1,1,1,5 */
  extRi64type,			/* 20  5,6,5,5,3,3,5 */
  extshift64type		/* 21  5,5,1,1,1,1,1,1,5,1,1,1,3,5 */
};
/* I am heaping all the fields of the formats into one structure and
   then, only the fields which are involved in instruction extension.  */
struct upk_mips16
{
  CORE_ADDR offset;
  unsigned int regx;		/* Function in i8 type.  */
  unsigned int regy;
};


/* The EXT-I, EXT-ri nad EXT-I8 instructions all have the same format
   for the bits which make up the immediate extension.  */

static CORE_ADDR
extended_offset (unsigned int extension)
{
  CORE_ADDR value;
  value = (extension >> 21) & 0x3f;	/* * extract 15:11 */
  value = value << 6;
  value |= (extension >> 16) & 0x1f;	/* extract 10:5 */
  value = value << 5;
  value |= extension & 0x01f;	/* extract 4:0 */
  return value;
}

/* Only call this function if you know that this is an extendable
   instruction.  It won't malfunction, but why make excess remote memory
   references?  If the immediate operands get sign extended or something,
   do it after the extension is performed.  */
/* FIXME: Every one of these cases needs to worry about sign extension
   when the offset is to be used in relative addressing.  */

static unsigned int
fetch_mips_16 (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];
  pc &= 0xfffffffe;		/* Clear the low order bit.  */
  target_read_memory (pc, buf, 2);
  return extract_unsigned_integer (buf, 2, byte_order);
}

static void
unpack_mips16 (struct gdbarch *gdbarch, CORE_ADDR pc,
	       unsigned int extension,
	       unsigned int inst,
	       enum mips16_inst_fmts insn_format, struct upk_mips16 *upk)
{
  CORE_ADDR offset;
  int regx;
  int regy;
  switch (insn_format)
    {
    case itype:
      {
	CORE_ADDR value;
	if (extension)
	  {
	    value = extended_offset (extension);
	    value = value << 11;	/* rom for the original value */
	    value |= inst & 0x7ff;	/* eleven bits from instruction */
	  }
	else
	  {
	    value = inst & 0x7ff;
	    /* FIXME : Consider sign extension.  */
	  }
	offset = value;
	regx = -1;
	regy = -1;
      }
      break;
    case ritype:
    case i8type:
      {				/* A register identifier and an offset.  */
	/* Most of the fields are the same as I type but the
	   immediate value is of a different length.  */
	CORE_ADDR value;
	if (extension)
	  {
	    value = extended_offset (extension);
	    value = value << 8;		/* from the original instruction */
	    value |= inst & 0xff;	/* eleven bits from instruction */
	    regx = (extension >> 8) & 0x07;	/* or i8 funct */
	    if (value & 0x4000)		/* Test the sign bit, bit 26.  */
	      {
		value &= ~0x3fff;	/* Remove the sign bit.  */
		value = -value;
	      }
	  }
	else
	  {
	    value = inst & 0xff;	/* 8 bits */
	    regx = (inst >> 8) & 0x07;	/* or i8 funct */
	    /* FIXME: Do sign extension, this format needs it.  */
	    if (value & 0x80)	/* THIS CONFUSES ME.  */
	      {
		value &= 0xef;	/* Remove the sign bit.  */
		value = -value;
	      }
	  }
	offset = value;
	regy = -1;
	break;
      }
    case jalxtype:
      {
	unsigned long value;
	unsigned int nexthalf;
	value = ((inst & 0x1f) << 5) | ((inst >> 5) & 0x1f);
	value = value << 16;
	nexthalf = mips_fetch_instruction (gdbarch, pc + 2);  /* low bit
								 still set.  */
	value |= nexthalf;
	offset = value;
	regx = -1;
	regy = -1;
	break;
      }
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }
  upk->offset = offset;
  upk->regx = regx;
  upk->regy = regy;
}


static CORE_ADDR
add_offset_16 (CORE_ADDR pc, int offset)
{
  return ((offset << 2) | ((pc + 2) & (~(CORE_ADDR) 0x0fffffff)));
}

static CORE_ADDR
extended_mips16_next_pc (struct frame_info *frame, CORE_ADDR pc,
			 unsigned int extension, unsigned int insn)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int op = (insn >> 11);
  switch (op)
    {
    case 2:			/* Branch */
      {
	CORE_ADDR offset;
	struct upk_mips16 upk;
	unpack_mips16 (gdbarch, pc, extension, insn, itype, &upk);
	offset = upk.offset;
	if (offset & 0x800)
	  {
	    offset &= 0xeff;
	    offset = -offset;
	  }
	pc += (offset << 1) + 2;
	break;
      }
    case 3:			/* JAL , JALX - Watch out, these are 32 bit
				   instructions.  */
      {
	struct upk_mips16 upk;
	unpack_mips16 (gdbarch, pc, extension, insn, jalxtype, &upk);
	pc = add_offset_16 (pc, upk.offset);
	if ((insn >> 10) & 0x01)	/* Exchange mode */
	  pc = pc & ~0x01;	/* Clear low bit, indicate 32 bit mode.  */
	else
	  pc |= 0x01;
	break;
      }
    case 4:			/* beqz */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (gdbarch, pc, extension, insn, ritype, &upk);
	reg = get_frame_register_signed (frame, upk.regx);
	if (reg == 0)
	  pc += (upk.offset << 1) + 2;
	else
	  pc += 2;
	break;
      }
    case 5:			/* bnez */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (gdbarch, pc, extension, insn, ritype, &upk);
	reg = get_frame_register_signed (frame, upk.regx);
	if (reg != 0)
	  pc += (upk.offset << 1) + 2;
	else
	  pc += 2;
	break;
      }
    case 12:			/* I8 Formats btez btnez */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (gdbarch, pc, extension, insn, i8type, &upk);
	/* upk.regx contains the opcode */
	reg = get_frame_register_signed (frame, 24);  /* Test register is 24 */
	if (((upk.regx == 0) && (reg == 0))	/* BTEZ */
	    || ((upk.regx == 1) && (reg != 0)))	/* BTNEZ */
	  /* pc = add_offset_16(pc,upk.offset) ; */
	  pc += (upk.offset << 1) + 2;
	else
	  pc += 2;
	break;
      }
    case 29:			/* RR Formats JR, JALR, JALR-RA */
      {
	struct upk_mips16 upk;
	/* upk.fmt = rrtype; */
	op = insn & 0x1f;
	if (op == 0)
	  {
	    int reg;
	    upk.regx = (insn >> 8) & 0x07;
	    upk.regy = (insn >> 5) & 0x07;
	    switch (upk.regy)
	      {
	      case 0:
		reg = upk.regx;
		break;
	      case 1:
		reg = 31;
		break;		/* Function return instruction.  */
	      case 2:
		reg = upk.regx;
		break;
	      default:
		reg = 31;
		break;		/* BOGUS Guess */
	      }
	    pc = get_frame_register_signed (frame, reg);
	  }
	else
	  pc += 2;
	break;
      }
    case 30:
      /* This is an instruction extension.  Fetch the real instruction
         (which follows the extension) and decode things based on
         that.  */
      {
	pc += 2;
	pc = extended_mips16_next_pc (frame, pc, insn,
				      fetch_mips_16 (gdbarch, pc));
	break;
      }
    default:
      {
	pc += 2;
	break;
      }
    }
  return pc;
}

static CORE_ADDR
mips16_next_pc (struct frame_info *frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  unsigned int insn = fetch_mips_16 (gdbarch, pc);
  return extended_mips16_next_pc (frame, pc, 0, insn);
}

/* The mips_next_pc function supports single_step when the remote
   target monitor or stub is not developed enough to do a single_step.
   It works by decoding the current instruction and predicting where a
   branch will go.  This isnt hard because all the data is available.
   The MIPS32 and MIPS16 variants are quite different.  */
static CORE_ADDR
mips_next_pc (struct frame_info *frame, CORE_ADDR pc)
{
  if (is_mips16_addr (pc))
    return mips16_next_pc (frame, pc);
  else
    return mips32_next_pc (frame, pc);
}

struct mips_frame_cache
{
  CORE_ADDR base;
  struct trad_frame_saved_reg *saved_regs;
};

/* Set a register's saved stack address in temp_saved_regs.  If an
   address has already been set for this register, do nothing; this
   way we will only recognize the first save of a given register in a
   function prologue.

   For simplicity, save the address in both [0 .. gdbarch_num_regs) and
   [gdbarch_num_regs .. 2*gdbarch_num_regs).
   Strictly speaking, only the second range is used as it is only second
   range (the ABI instead of ISA registers) that comes into play when finding
   saved registers in a frame.  */

static void
set_reg_offset (struct gdbarch *gdbarch, struct mips_frame_cache *this_cache,
		int regnum, CORE_ADDR offset)
{
  if (this_cache != NULL
      && this_cache->saved_regs[regnum].addr == -1)
    {
      this_cache->saved_regs[regnum + 0 * gdbarch_num_regs (gdbarch)].addr
        = offset;
      this_cache->saved_regs[regnum + 1 * gdbarch_num_regs (gdbarch)].addr
        = offset;
    }
}


/* Fetch the immediate value from a MIPS16 instruction.
   If the previous instruction was an EXTEND, use it to extend
   the upper bits of the immediate value.  This is a helper function
   for mips16_scan_prologue.  */

static int
mips16_get_imm (unsigned short prev_inst,	/* previous instruction */
		unsigned short inst,	/* current instruction */
		int nbits,	/* number of bits in imm field */
		int scale,	/* scale factor to be applied to imm */
		int is_signed)	/* is the imm field signed?  */
{
  int offset;

  if ((prev_inst & 0xf800) == 0xf000)	/* prev instruction was EXTEND? */
    {
      offset = ((prev_inst & 0x1f) << 11) | (prev_inst & 0x7e0);
      if (offset & 0x8000)	/* check for negative extend */
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


/* Analyze the function prologue from START_PC to LIMIT_PC. Builds
   the associated FRAME_CACHE if not null.
   Return the address of the first instruction past the prologue.  */

static CORE_ADDR
mips16_scan_prologue (struct gdbarch *gdbarch,
		      CORE_ADDR start_pc, CORE_ADDR limit_pc,
                      struct frame_info *this_frame,
                      struct mips_frame_cache *this_cache)
{
  CORE_ADDR cur_pc;
  CORE_ADDR frame_addr = 0;	/* Value of $r17, used as frame pointer.  */
  CORE_ADDR sp;
  long frame_offset = 0;        /* Size of stack frame.  */
  long frame_adjust = 0;        /* Offset of FP from SP.  */
  int frame_reg = MIPS_SP_REGNUM;
  unsigned short prev_inst = 0;	/* saved copy of previous instruction.  */
  unsigned inst = 0;		/* current instruction */
  unsigned entry_inst = 0;	/* the entry instruction */
  unsigned save_inst = 0;	/* the save instruction */
  int reg, offset;

  int extend_bytes = 0;
  int prev_extend_bytes;
  CORE_ADDR end_prologue_addr = 0;

  /* Can be called when there's no process, and hence when there's no
     THIS_FRAME.  */
  if (this_frame != NULL)
    sp = get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch)
				    + MIPS_SP_REGNUM);
  else
    sp = 0;

  if (limit_pc > start_pc + 200)
    limit_pc = start_pc + 200;

  for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += MIPS_INSN16_SIZE)
    {
      /* Save the previous instruction.  If it's an EXTEND, we'll extract
         the immediate offset extension from it in mips16_get_imm.  */
      prev_inst = inst;

      /* Fetch and decode the instruction.  */
      inst = (unsigned short) mips_fetch_instruction (gdbarch, cur_pc);

      /* Normally we ignore extend instructions.  However, if it is
         not followed by a valid prologue instruction, then this
         instruction is not part of the prologue either.  We must
         remember in this case to adjust the end_prologue_addr back
         over the extend.  */
      if ((inst & 0xf800) == 0xf000)    /* extend */
        {
          extend_bytes = MIPS_INSN16_SIZE;
          continue;
        }

      prev_extend_bytes = extend_bytes;
      extend_bytes = 0;

      if ((inst & 0xff00) == 0x6300	/* addiu sp */
	  || (inst & 0xff00) == 0xfb00)	/* daddiu sp */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 8, 1);
	  if (offset < 0)	/* Negative stack adjustment?  */
	    frame_offset -= offset;
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
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	}
      else if ((inst & 0xff00) == 0xf900)	/* sd reg,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 8, 0);
	  reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	}
      else if ((inst & 0xff00) == 0x6200)	/* sw $ra,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  set_reg_offset (gdbarch, this_cache, MIPS_RA_REGNUM, sp + offset);
	}
      else if ((inst & 0xff00) == 0xfa00)	/* sd $ra,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 8, 0);
	  set_reg_offset (gdbarch, this_cache, MIPS_RA_REGNUM, sp + offset);
	}
      else if (inst == 0x673d)	/* move $s1, $sp */
	{
	  frame_addr = sp;
	  frame_reg = 17;
	}
      else if ((inst & 0xff00) == 0x0100)	/* addiu $s1,sp,n */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  frame_addr = sp + offset;
	  frame_reg = 17;
	  frame_adjust = offset;
	}
      else if ((inst & 0xFF00) == 0xd900)	/* sw reg,offset($s1) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 4, 0);
	  reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
	  set_reg_offset (gdbarch, this_cache, reg, frame_addr + offset);
	}
      else if ((inst & 0xFF00) == 0x7900)	/* sd reg,offset($s1) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 8, 0);
	  reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
	  set_reg_offset (gdbarch, this_cache, reg, frame_addr + offset);
	}
      else if ((inst & 0xf81f) == 0xe809
               && (inst & 0x700) != 0x700)	/* entry */
	entry_inst = inst;	/* Save for later processing.  */
      else if ((inst & 0xff80) == 0x6480)	/* save */
	{
	  save_inst = inst;	/* Save for later processing.  */
	  if (prev_extend_bytes)		/* extend */
	    save_inst |= prev_inst << 16;
	}
      else if ((inst & 0xf800) == 0x1800)	/* jal(x) */
	cur_pc += MIPS_INSN16_SIZE;	/* 32-bit instruction */
      else if ((inst & 0xff1c) == 0x6704)	/* move reg,$a0-$a3 */
        {
          /* This instruction is part of the prologue, but we don't
             need to do anything special to handle it.  */
        }
      else
        {
          /* This instruction is not an instruction typically found
             in a prologue, so we must have reached the end of the
             prologue.  */
          if (end_prologue_addr == 0)
            end_prologue_addr = cur_pc - prev_extend_bytes;
        }
    }

  /* The entry instruction is typically the first instruction in a function,
     and it stores registers at offsets relative to the value of the old SP
     (before the prologue).  But the value of the sp parameter to this
     function is the new SP (after the prologue has been executed).  So we
     can't calculate those offsets until we've seen the entire prologue,
     and can calculate what the old SP must have been.  */
  if (entry_inst != 0)
    {
      int areg_count = (entry_inst >> 8) & 7;
      int sreg_count = (entry_inst >> 6) & 3;

      /* The entry instruction always subtracts 32 from the SP.  */
      frame_offset += 32;

      /* Now we can calculate what the SP must have been at the
         start of the function prologue.  */
      sp += frame_offset;

      /* Check if a0-a3 were saved in the caller's argument save area.  */
      for (reg = 4, offset = 0; reg < areg_count + 4; reg++)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset += mips_abi_regsize (gdbarch);
	}

      /* Check if the ra register was pushed on the stack.  */
      offset = -4;
      if (entry_inst & 0x20)
	{
	  set_reg_offset (gdbarch, this_cache, MIPS_RA_REGNUM, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}

      /* Check if the s0 and s1 registers were pushed on the stack.  */
      for (reg = 16; reg < sreg_count + 16; reg++)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}
    }

  /* The SAVE instruction is similar to ENTRY, except that defined by the
     MIPS16e ASE of the MIPS Architecture.  Unlike with ENTRY though, the
     size of the frame is specified as an immediate field of instruction
     and an extended variation exists which lets additional registers and
     frame space to be specified.  The instruction always treats registers
     as 32-bit so its usefulness for 64-bit ABIs is questionable.  */
  if (save_inst != 0 && mips_abi_regsize (gdbarch) == 4)
    {
      static int args_table[16] = {
	0, 0, 0, 0, 1, 1, 1, 1,
	2, 2, 2, 0, 3, 3, 4, -1,
      };
      static int astatic_table[16] = {
	0, 1, 2, 3, 0, 1, 2, 3,
	0, 1, 2, 4, 0, 1, 0, -1,
      };
      int aregs = (save_inst >> 16) & 0xf;
      int xsregs = (save_inst >> 24) & 0x7;
      int args = args_table[aregs];
      int astatic = astatic_table[aregs];
      long frame_size;

      if (args < 0)
	{
	  warning (_("Invalid number of argument registers encoded in SAVE."));
	  args = 0;
	}
      if (astatic < 0)
	{
	  warning (_("Invalid number of static registers encoded in SAVE."));
	  astatic = 0;
	}

      /* For standard SAVE the frame size of 0 means 128.  */
      frame_size = ((save_inst >> 16) & 0xf0) | (save_inst & 0xf);
      if (frame_size == 0 && (save_inst >> 16) == 0)
	frame_size = 16;
      frame_size *= 8;
      frame_offset += frame_size;

      /* Now we can calculate what the SP must have been at the
         start of the function prologue.  */
      sp += frame_offset;

      /* Check if A0-A3 were saved in the caller's argument save area.  */
      for (reg = MIPS_A0_REGNUM, offset = 0; reg < args + 4; reg++)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset += mips_abi_regsize (gdbarch);
	}

      offset = -4;

      /* Check if the RA register was pushed on the stack.  */
      if (save_inst & 0x40)
	{
	  set_reg_offset (gdbarch, this_cache, MIPS_RA_REGNUM, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}

      /* Check if the S8 register was pushed on the stack.  */
      if (xsregs > 6)
	{
	  set_reg_offset (gdbarch, this_cache, 30, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	  xsregs--;
	}
      /* Check if S2-S7 were pushed on the stack.  */
      for (reg = 18 + xsregs - 1; reg > 18 - 1; reg--)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}

      /* Check if the S1 register was pushed on the stack.  */
      if (save_inst & 0x10)
	{
	  set_reg_offset (gdbarch, this_cache, 17, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}
      /* Check if the S0 register was pushed on the stack.  */
      if (save_inst & 0x20)
	{
	  set_reg_offset (gdbarch, this_cache, 16, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}

      /* Check if A0-A3 were pushed on the stack.  */
      for (reg = MIPS_A0_REGNUM + 3; reg > MIPS_A0_REGNUM + 3 - astatic; reg--)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + offset);
	  offset -= mips_abi_regsize (gdbarch);
	}
    }

  if (this_cache != NULL)
    {
      this_cache->base =
        (get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch) + frame_reg)
         + frame_offset - frame_adjust);
      /* FIXME: brobecker/2004-10-10: Just as in the mips32 case, we should
         be able to get rid of the assignment below, evetually.  But it's
         still needed for now.  */
      this_cache->saved_regs[gdbarch_num_regs (gdbarch)
			     + mips_regnum (gdbarch)->pc]
        = this_cache->saved_regs[gdbarch_num_regs (gdbarch) + MIPS_RA_REGNUM];
    }

  /* If we didn't reach the end of the prologue when scanning the function
     instructions, then set end_prologue_addr to the address of the
     instruction immediately after the last one we scanned.  */
  if (end_prologue_addr == 0)
    end_prologue_addr = cur_pc;

  return end_prologue_addr;
}

/* Heuristic unwinder for 16-bit MIPS instruction set (aka MIPS16).
   Procedures that use the 32-bit instruction set are handled by the
   mips_insn32 unwinder.  */

static struct mips_frame_cache *
mips_insn16_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct mips_frame_cache *cache;

  if ((*this_cache) != NULL)
    return (*this_cache);
  cache = FRAME_OBSTACK_ZALLOC (struct mips_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Analyze the function prologue.  */
  {
    const CORE_ADDR pc = get_frame_address_in_block (this_frame);
    CORE_ADDR start_addr;

    find_pc_partial_function (pc, NULL, &start_addr, NULL);
    if (start_addr == 0)
      start_addr = heuristic_proc_start (gdbarch, pc);
    /* We can't analyze the prologue if we couldn't find the begining
       of the function.  */
    if (start_addr == 0)
      return cache;

    mips16_scan_prologue (gdbarch, start_addr, pc, this_frame, *this_cache);
  }
  
  /* gdbarch_sp_regnum contains the value and not the address.  */
  trad_frame_set_value (cache->saved_regs,
			gdbarch_num_regs (gdbarch) + MIPS_SP_REGNUM,
			cache->base);

  return (*this_cache);
}

static void
mips_insn16_frame_this_id (struct frame_info *this_frame, void **this_cache,
			   struct frame_id *this_id)
{
  struct mips_frame_cache *info = mips_insn16_frame_cache (this_frame,
							   this_cache);
  /* This marks the outermost frame.  */
  if (info->base == 0)
    return;
  (*this_id) = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
mips_insn16_frame_prev_register (struct frame_info *this_frame,
				 void **this_cache, int regnum)
{
  struct mips_frame_cache *info = mips_insn16_frame_cache (this_frame,
							   this_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static int
mips_insn16_frame_sniffer (const struct frame_unwind *self,
			   struct frame_info *this_frame, void **this_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (mips_pc_is_mips16 (pc))
    return 1;
  return 0;
}

static const struct frame_unwind mips_insn16_frame_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  mips_insn16_frame_this_id,
  mips_insn16_frame_prev_register,
  NULL,
  mips_insn16_frame_sniffer
};

static CORE_ADDR
mips_insn16_frame_base_address (struct frame_info *this_frame,
				void **this_cache)
{
  struct mips_frame_cache *info = mips_insn16_frame_cache (this_frame,
							   this_cache);
  return info->base;
}

static const struct frame_base mips_insn16_frame_base =
{
  &mips_insn16_frame_unwind,
  mips_insn16_frame_base_address,
  mips_insn16_frame_base_address,
  mips_insn16_frame_base_address
};

static const struct frame_base *
mips_insn16_frame_base_sniffer (struct frame_info *this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (mips_pc_is_mips16 (pc))
    return &mips_insn16_frame_base;
  else
    return NULL;
}

/* Mark all the registers as unset in the saved_regs array
   of THIS_CACHE.  Do nothing if THIS_CACHE is null.  */

static void
reset_saved_regs (struct gdbarch *gdbarch, struct mips_frame_cache *this_cache)
{
  if (this_cache == NULL || this_cache->saved_regs == NULL)
    return;

  {
    const int num_regs = gdbarch_num_regs (gdbarch);
    int i;

    for (i = 0; i < num_regs; i++)
      {
        this_cache->saved_regs[i].addr = -1;
      }
  }
}

/* Analyze the function prologue from START_PC to LIMIT_PC.  Builds
   the associated FRAME_CACHE if not null.  
   Return the address of the first instruction past the prologue.  */

static CORE_ADDR
mips32_scan_prologue (struct gdbarch *gdbarch,
		      CORE_ADDR start_pc, CORE_ADDR limit_pc,
                      struct frame_info *this_frame,
                      struct mips_frame_cache *this_cache)
{
  CORE_ADDR cur_pc;
  CORE_ADDR frame_addr = 0; /* Value of $r30. Used by gcc for
			       frame-pointer.  */
  CORE_ADDR sp;
  long frame_offset;
  int  frame_reg = MIPS_SP_REGNUM;

  CORE_ADDR end_prologue_addr = 0;
  int seen_sp_adjust = 0;
  int load_immediate_bytes = 0;
  int in_delay_slot = 0;
  int regsize_is_64_bits = (mips_abi_regsize (gdbarch) == 8);

  /* Can be called when there's no process, and hence when there's no
     THIS_FRAME.  */
  if (this_frame != NULL)
    sp = get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch)
				    + MIPS_SP_REGNUM);
  else
    sp = 0;

  if (limit_pc > start_pc + 200)
    limit_pc = start_pc + 200;

restart:

  frame_offset = 0;
  for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += MIPS_INSN32_SIZE)
    {
      unsigned long inst, high_word, low_word;
      int reg;

      /* Fetch the instruction.  */
      inst = (unsigned long) mips_fetch_instruction (gdbarch, cur_pc);

      /* Save some code by pre-extracting some useful fields.  */
      high_word = (inst >> 16) & 0xffff;
      low_word = inst & 0xffff;
      reg = high_word & 0x1f;

      if (high_word == 0x27bd		/* addiu $sp,$sp,-i */
	  || high_word == 0x23bd	/* addi $sp,$sp,-i */
	  || high_word == 0x67bd)	/* daddiu $sp,$sp,-i */
	{
	  if (low_word & 0x8000)	/* Negative stack adjustment?  */
            frame_offset += 0x10000 - low_word;
	  else
	    /* Exit loop if a positive stack adjustment is found, which
	       usually means that the stack cleanup code in the function
	       epilogue is reached.  */
	    break;
          seen_sp_adjust = 1;
	}
      else if (((high_word & 0xFFE0) == 0xafa0) /* sw reg,offset($sp) */
               && !regsize_is_64_bits)
	{
	  set_reg_offset (gdbarch, this_cache, reg, sp + low_word);
	}
      else if (((high_word & 0xFFE0) == 0xffa0)	/* sd reg,offset($sp) */
               && regsize_is_64_bits)
	{
	  /* Irix 6.2 N32 ABI uses sd instructions for saving $gp and $ra.  */
	  set_reg_offset (gdbarch, this_cache, reg, sp + low_word);
	}
      else if (high_word == 0x27be)	/* addiu $30,$sp,size */
	{
	  /* Old gcc frame, r30 is virtual frame pointer.  */
	  if ((long) low_word != frame_offset)
	    frame_addr = sp + low_word;
	  else if (this_frame && frame_reg == MIPS_SP_REGNUM)
	    {
	      unsigned alloca_adjust;

	      frame_reg = 30;
	      frame_addr = get_frame_register_signed
		(this_frame, gdbarch_num_regs (gdbarch) + 30);

	      alloca_adjust = (unsigned) (frame_addr - (sp + low_word));
	      if (alloca_adjust > 0)
		{
                  /* FP > SP + frame_size.  This may be because of
                     an alloca or somethings similar.  Fix sp to
                     "pre-alloca" value, and try again.  */
		  sp += alloca_adjust;
                  /* Need to reset the status of all registers.  Otherwise,
                     we will hit a guard that prevents the new address
                     for each register to be recomputed during the second
                     pass.  */
                  reset_saved_regs (gdbarch, this_cache);
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
	  if (this_frame && frame_reg == MIPS_SP_REGNUM)
	    {
	      unsigned alloca_adjust;

	      frame_reg = 30;
	      frame_addr = get_frame_register_signed
		(this_frame, gdbarch_num_regs (gdbarch) + 30);

	      alloca_adjust = (unsigned) (frame_addr - sp);
	      if (alloca_adjust > 0)
	        {
                  /* FP > SP + frame_size.  This may be because of
                     an alloca or somethings similar.  Fix sp to
                     "pre-alloca" value, and try again.  */
	          sp = frame_addr;
                  /* Need to reset the status of all registers.  Otherwise,
                     we will hit a guard that prevents the new address
                     for each register to be recomputed during the second
                     pass.  */
                  reset_saved_regs (gdbarch, this_cache);
	          goto restart;
	        }
	    }
	}
      else if ((high_word & 0xFFE0) == 0xafc0 	/* sw reg,offset($30) */
               && !regsize_is_64_bits)
	{
	  set_reg_offset (gdbarch, this_cache, reg, frame_addr + low_word);
	}
      else if ((high_word & 0xFFE0) == 0xE7A0 /* swc1 freg,n($sp) */
               || (high_word & 0xF3E0) == 0xA3C0 /* sx reg,n($s8) */
               || (inst & 0xFF9F07FF) == 0x00800021 /* move reg,$a0-$a3 */
               || high_word == 0x3c1c /* lui $gp,n */
               || high_word == 0x279c /* addiu $gp,$gp,n */
               || inst == 0x0399e021 /* addu $gp,$gp,$t9 */
               || inst == 0x033ce021 /* addu $gp,$t9,$gp */
              )
       {
         /* These instructions are part of the prologue, but we don't
            need to do anything special to handle them.  */
       }
      /* The instructions below load $at or $t0 with an immediate
         value in preparation for a stack adjustment via
         subu $sp,$sp,[$at,$t0].  These instructions could also
         initialize a local variable, so we accept them only before
         a stack adjustment instruction was seen.  */
      else if (!seen_sp_adjust
               && (high_word == 0x3c01 /* lui $at,n */
                   || high_word == 0x3c08 /* lui $t0,n */
                   || high_word == 0x3421 /* ori $at,$at,n */
                   || high_word == 0x3508 /* ori $t0,$t0,n */
                   || high_word == 0x3401 /* ori $at,$zero,n */
                   || high_word == 0x3408 /* ori $t0,$zero,n */
                  ))
       {
          load_immediate_bytes += MIPS_INSN32_SIZE;     	/* FIXME!  */
       }
      else
       {
         /* This instruction is not an instruction typically found
            in a prologue, so we must have reached the end of the
            prologue.  */
         /* FIXME: brobecker/2004-10-10: Can't we just break out of this
            loop now?  Why would we need to continue scanning the function
            instructions?  */
         if (end_prologue_addr == 0)
           end_prologue_addr = cur_pc;

	 /* Check for branches and jumps.  For now, only jump to
	    register are caught (i.e. returns).  */
	 if ((itype_op (inst) & 0x07) == 0 && rtype_funct (inst) == 8)
	   in_delay_slot = 1;
       }

      /* If the previous instruction was a jump, we must have reached
	 the end of the prologue by now.  Stop scanning so that we do
	 not go past the function return.  */
      if (in_delay_slot)
	break;
    }

  if (this_cache != NULL)
    {
      this_cache->base = 
        (get_frame_register_signed (this_frame,
				    gdbarch_num_regs (gdbarch) + frame_reg)
         + frame_offset);
      /* FIXME: brobecker/2004-09-15: We should be able to get rid of
         this assignment below, eventually.  But it's still needed
         for now.  */
      this_cache->saved_regs[gdbarch_num_regs (gdbarch)
			     + mips_regnum (gdbarch)->pc]
        = this_cache->saved_regs[gdbarch_num_regs (gdbarch)
				 + MIPS_RA_REGNUM];
    }

  /* If we didn't reach the end of the prologue when scanning the function
     instructions, then set end_prologue_addr to the address of the
     instruction immediately after the last one we scanned.  */
  /* brobecker/2004-10-10: I don't think this would ever happen, but
     we may as well be careful and do our best if we have a null
     end_prologue_addr.  */
  if (end_prologue_addr == 0)
    end_prologue_addr = cur_pc;
     
  /* In a frameless function, we might have incorrectly
     skipped some load immediate instructions.  Undo the skipping
     if the load immediate was not followed by a stack adjustment.  */
  if (load_immediate_bytes && !seen_sp_adjust)
    end_prologue_addr -= load_immediate_bytes;

  return end_prologue_addr;
}

/* Heuristic unwinder for procedures using 32-bit instructions (covers
   both 32-bit and 64-bit MIPS ISAs).  Procedures using 16-bit
   instructions (a.k.a. MIPS16) are handled by the mips_insn16
   unwinder.  */

static struct mips_frame_cache *
mips_insn32_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct mips_frame_cache *cache;

  if ((*this_cache) != NULL)
    return (*this_cache);

  cache = FRAME_OBSTACK_ZALLOC (struct mips_frame_cache);
  (*this_cache) = cache;
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Analyze the function prologue.  */
  {
    const CORE_ADDR pc = get_frame_address_in_block (this_frame);
    CORE_ADDR start_addr;

    find_pc_partial_function (pc, NULL, &start_addr, NULL);
    if (start_addr == 0)
      start_addr = heuristic_proc_start (gdbarch, pc);
    /* We can't analyze the prologue if we couldn't find the begining
       of the function.  */
    if (start_addr == 0)
      return cache;

    mips32_scan_prologue (gdbarch, start_addr, pc, this_frame, *this_cache);
  }
  
  /* gdbarch_sp_regnum contains the value and not the address.  */
  trad_frame_set_value (cache->saved_regs,
			gdbarch_num_regs (gdbarch) + MIPS_SP_REGNUM,
			cache->base);

  return (*this_cache);
}

static void
mips_insn32_frame_this_id (struct frame_info *this_frame, void **this_cache,
			   struct frame_id *this_id)
{
  struct mips_frame_cache *info = mips_insn32_frame_cache (this_frame,
							   this_cache);
  /* This marks the outermost frame.  */
  if (info->base == 0)
    return;
  (*this_id) = frame_id_build (info->base, get_frame_func (this_frame));
}

static struct value *
mips_insn32_frame_prev_register (struct frame_info *this_frame,
				 void **this_cache, int regnum)
{
  struct mips_frame_cache *info = mips_insn32_frame_cache (this_frame,
							   this_cache);
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static int
mips_insn32_frame_sniffer (const struct frame_unwind *self,
			   struct frame_info *this_frame, void **this_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (! mips_pc_is_mips16 (pc))
    return 1;
  return 0;
}

static const struct frame_unwind mips_insn32_frame_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  mips_insn32_frame_this_id,
  mips_insn32_frame_prev_register,
  NULL,
  mips_insn32_frame_sniffer
};

static CORE_ADDR
mips_insn32_frame_base_address (struct frame_info *this_frame,
				void **this_cache)
{
  struct mips_frame_cache *info = mips_insn32_frame_cache (this_frame,
							   this_cache);
  return info->base;
}

static const struct frame_base mips_insn32_frame_base =
{
  &mips_insn32_frame_unwind,
  mips_insn32_frame_base_address,
  mips_insn32_frame_base_address,
  mips_insn32_frame_base_address
};

static const struct frame_base *
mips_insn32_frame_base_sniffer (struct frame_info *this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (! mips_pc_is_mips16 (pc))
    return &mips_insn32_frame_base;
  else
    return NULL;
}

static struct trad_frame_cache *
mips_stub_frame_cache (struct frame_info *this_frame, void **this_cache)
{
  CORE_ADDR pc;
  CORE_ADDR start_addr;
  CORE_ADDR stack_addr;
  struct trad_frame_cache *this_trad_cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int num_regs = gdbarch_num_regs (gdbarch);

  if ((*this_cache) != NULL)
    return (*this_cache);
  this_trad_cache = trad_frame_cache_zalloc (this_frame);
  (*this_cache) = this_trad_cache;

  /* The return address is in the link register.  */
  trad_frame_set_reg_realreg (this_trad_cache,
			      gdbarch_pc_regnum (gdbarch),
			      num_regs + MIPS_RA_REGNUM);

  /* Frame ID, since it's a frameless / stackless function, no stack
     space is allocated and SP on entry is the current SP.  */
  pc = get_frame_pc (this_frame);
  find_pc_partial_function (pc, NULL, &start_addr, NULL);
  stack_addr = get_frame_register_signed (this_frame,
					  num_regs + MIPS_SP_REGNUM);
  trad_frame_set_id (this_trad_cache, frame_id_build (stack_addr, start_addr));

  /* Assume that the frame's base is the same as the
     stack-pointer.  */
  trad_frame_set_this_base (this_trad_cache, stack_addr);

  return this_trad_cache;
}

static void
mips_stub_frame_this_id (struct frame_info *this_frame, void **this_cache,
			 struct frame_id *this_id)
{
  struct trad_frame_cache *this_trad_cache
    = mips_stub_frame_cache (this_frame, this_cache);
  trad_frame_get_id (this_trad_cache, this_id);
}

static struct value *
mips_stub_frame_prev_register (struct frame_info *this_frame,
			       void **this_cache, int regnum)
{
  struct trad_frame_cache *this_trad_cache
    = mips_stub_frame_cache (this_frame, this_cache);
  return trad_frame_get_register (this_trad_cache, this_frame, regnum);
}

static int
mips_stub_frame_sniffer (const struct frame_unwind *self,
			 struct frame_info *this_frame, void **this_cache)
{
  gdb_byte dummy[4];
  struct obj_section *s;
  CORE_ADDR pc = get_frame_address_in_block (this_frame);
  struct minimal_symbol *msym;

  /* Use the stub unwinder for unreadable code.  */
  if (target_read_memory (get_frame_pc (this_frame), dummy, 4) != 0)
    return 1;

  if (in_plt_section (pc, NULL))
    return 1;

  /* Binutils for MIPS puts lazy resolution stubs into .MIPS.stubs.  */
  s = find_pc_section (pc);

  if (s != NULL
      && strcmp (bfd_get_section_name (s->objfile->obfd, s->the_bfd_section),
		 ".MIPS.stubs") == 0)
    return 1;

  /* Calling a PIC function from a non-PIC function passes through a
     stub.  The stub for foo is named ".pic.foo".  */
  msym = lookup_minimal_symbol_by_pc (pc);
  if (msym != NULL
      && SYMBOL_LINKAGE_NAME (msym) != NULL
      && strncmp (SYMBOL_LINKAGE_NAME (msym), ".pic.", 5) == 0)
    return 1;

  return 0;
}

static const struct frame_unwind mips_stub_frame_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  mips_stub_frame_this_id,
  mips_stub_frame_prev_register,
  NULL,
  mips_stub_frame_sniffer
};

static CORE_ADDR
mips_stub_frame_base_address (struct frame_info *this_frame,
			      void **this_cache)
{
  struct trad_frame_cache *this_trad_cache
    = mips_stub_frame_cache (this_frame, this_cache);
  return trad_frame_get_this_base (this_trad_cache);
}

static const struct frame_base mips_stub_frame_base =
{
  &mips_stub_frame_unwind,
  mips_stub_frame_base_address,
  mips_stub_frame_base_address,
  mips_stub_frame_base_address
};

static const struct frame_base *
mips_stub_frame_base_sniffer (struct frame_info *this_frame)
{
  if (mips_stub_frame_sniffer (&mips_stub_frame_unwind, this_frame, NULL))
    return &mips_stub_frame_base;
  else
    return NULL;
}

/* mips_addr_bits_remove - remove useless address bits  */

static CORE_ADDR
mips_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (is_mips16_addr (addr))
    addr = unmake_mips16_addr (addr);

  if (mips_mask_address_p (tdep) && (((ULONGEST) addr) >> 32 == 0xffffffffUL))
    /* This hack is a work-around for existing boards using PMON, the
       simulator, and any other 64-bit targets that doesn't have true
       64-bit addressing.  On these targets, the upper 32 bits of
       addresses are ignored by the hardware.  Thus, the PC or SP are
       likely to have been sign extended to all 1s by instruction
       sequences that load 32-bit addresses.  For example, a typical
       piece of code that loads an address is this:

       lui $r2, <upper 16 bits>
       ori $r2, <lower 16 bits>

       But the lui sign-extends the value such that the upper 32 bits
       may be all 1s.  The workaround is simply to mask off these
       bits.  In the future, gcc may be changed to support true 64-bit
       addressing, and this masking will have to be disabled.  */
    return addr &= 0xffffffffUL;
  else
    return addr;
}

/* Instructions used during single-stepping of atomic sequences.  */
#define LL_OPCODE 0x30
#define LLD_OPCODE 0x34
#define SC_OPCODE 0x38
#define SCD_OPCODE 0x3c

/* Checks for an atomic sequence of instructions beginning with a LL/LLD
   instruction and ending with a SC/SCD instruction.  If such a sequence
   is found, attempt to step through it.  A breakpoint is placed at the end of 
   the sequence.  */

static int
deal_with_atomic_sequence (struct gdbarch *gdbarch,
			   struct address_space *aspace, CORE_ADDR pc)
{
  CORE_ADDR breaks[2] = {-1, -1};
  CORE_ADDR loc = pc;
  CORE_ADDR branch_bp; /* Breakpoint at branch instruction's destination.  */
  unsigned long insn;
  int insn_count;
  int index;
  int last_breakpoint = 0; /* Defaults to 0 (no breakpoints placed).  */  
  const int atomic_sequence_length = 16; /* Instruction sequence length.  */

  if (pc & 0x01)
    return 0;

  insn = mips_fetch_instruction (gdbarch, loc);
  /* Assume all atomic sequences start with a ll/lld instruction.  */
  if (itype_op (insn) != LL_OPCODE && itype_op (insn) != LLD_OPCODE)
    return 0;

  /* Assume that no atomic sequence is longer than "atomic_sequence_length" 
     instructions.  */
  for (insn_count = 0; insn_count < atomic_sequence_length; ++insn_count)
    {
      int is_branch = 0;
      loc += MIPS_INSN32_SIZE;
      insn = mips_fetch_instruction (gdbarch, loc);

      /* Assume that there is at most one branch in the atomic
	 sequence.  If a branch is found, put a breakpoint in its
	 destination address.  */
      switch (itype_op (insn))
	{
	case 0: /* SPECIAL */
	  if (rtype_funct (insn) >> 1 == 4) /* JR, JALR */
	    return 0; /* fallback to the standard single-step code.  */
	  break;
	case 1: /* REGIMM */
	  is_branch = ((itype_rt (insn) & 0xc0) == 0); /* B{LT,GE}Z* */
	  break;
	case 2: /* J */
	case 3: /* JAL */
	  return 0; /* fallback to the standard single-step code.  */
	case 4: /* BEQ */
	case 5: /* BNE */
	case 6: /* BLEZ */
	case 7: /* BGTZ */
	case 20: /* BEQL */
	case 21: /* BNEL */
	case 22: /* BLEZL */
	case 23: /* BGTTL */
	  is_branch = 1;
	  break;
	case 17: /* COP1 */
	case 18: /* COP2 */
	case 19: /* COP3 */
	  is_branch = (itype_rs (insn) == 8); /* BCzF, BCzFL, BCzT, BCzTL */
	  break;
	}
      if (is_branch)
	{
	  branch_bp = loc + mips32_relative_offset (insn) + 4;
	  if (last_breakpoint >= 1)
	    return 0; /* More than one branch found, fallback to the
			 standard single-step code.  */
	  breaks[1] = branch_bp;
	  last_breakpoint++;
	}

      if (itype_op (insn) == SC_OPCODE || itype_op (insn) == SCD_OPCODE)
	break;
    }

  /* Assume that the atomic sequence ends with a sc/scd instruction.  */
  if (itype_op (insn) != SC_OPCODE && itype_op (insn) != SCD_OPCODE)
    return 0;

  loc += MIPS_INSN32_SIZE;

  /* Insert a breakpoint right after the end of the atomic sequence.  */
  breaks[0] = loc;

  /* Check for duplicated breakpoints.  Check also for a breakpoint
     placed (branch instruction's destination) in the atomic sequence.  */
  if (last_breakpoint && pc <= breaks[1] && breaks[1] <= breaks[0])
    last_breakpoint = 0;

  /* Effectively inserts the breakpoints.  */
  for (index = 0; index <= last_breakpoint; index++)
    insert_single_step_breakpoint (gdbarch, aspace, breaks[index]);

  return 1;
}

/* mips_software_single_step() is called just before we want to resume
   the inferior, if we want to single-step it but there is no hardware
   or kernel single-step support (MIPS on GNU/Linux for example).  We find
   the target of the coming instruction and breakpoint it.  */

int
mips_software_single_step (struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct address_space *aspace = get_frame_address_space (frame);
  CORE_ADDR pc, next_pc;

  pc = get_frame_pc (frame);
  if (deal_with_atomic_sequence (gdbarch, aspace, pc))
    return 1;

  next_pc = mips_next_pc (frame, pc);

  insert_single_step_breakpoint (gdbarch, aspace, next_pc);
  return 1;
}

/* Test whether the PC points to the return instruction at the
   end of a function.  */

static int
mips_about_to_return (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  if (mips_pc_is_mips16 (pc))
    /* This mips16 case isn't necessarily reliable.  Sometimes the compiler
       generates a "jr $ra"; other times it generates code to load
       the return address from the stack to an accessible register (such
       as $a3), then a "jr" using that register.  This second case
       is almost impossible to distinguish from an indirect jump
       used for switch statements, so we don't even try.  */
    return mips_fetch_instruction (gdbarch, pc) == 0xe820;	/* jr $ra */
  else
    return mips_fetch_instruction (gdbarch, pc) == 0x3e00008;	/* jr $ra */
}


/* This fencepost looks highly suspicious to me.  Removing it also
   seems suspicious as it could affect remote debugging across serial
   lines.  */

static CORE_ADDR
heuristic_proc_start (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR start_pc;
  CORE_ADDR fence;
  int instlen;
  int seen_adjsp = 0;
  struct inferior *inf;

  pc = gdbarch_addr_bits_remove (gdbarch, pc);
  start_pc = pc;
  fence = start_pc - heuristic_fence_post;
  if (start_pc == 0)
    return 0;

  if (heuristic_fence_post == UINT_MAX || fence < VM_MIN_ADDRESS)
    fence = VM_MIN_ADDRESS;

  instlen = mips_pc_is_mips16 (pc) ? MIPS_INSN16_SIZE : MIPS_INSN32_SIZE;

  inf = current_inferior ();

  /* Search back for previous return.  */
  for (start_pc -= instlen;; start_pc -= instlen)
    if (start_pc < fence)
      {
	/* It's not clear to me why we reach this point when
	   stop_soon, but with this test, at least we
	   don't print out warnings for every child forked (eg, on
	   decstation).  22apr93 rich@cygnus.com.  */
	if (inf->control.stop_soon == NO_STOP_QUIETLY)
	  {
	    static int blurb_printed = 0;

	    warning (_("GDB can't find the start of the function at %s."),
		     paddress (gdbarch, pc));

	    if (!blurb_printed)
	      {
		/* This actually happens frequently in embedded
		   development, when you first connect to a board
		   and your stack pointer and pc are nowhere in
		   particular.  This message needs to give people
		   in that situation enough information to
		   determine that it's no big deal.  */
		printf_filtered ("\n\
    GDB is unable to find the start of the function at %s\n\
and thus can't determine the size of that function's stack frame.\n\
This means that GDB may be unable to access that stack frame, or\n\
the frames below it.\n\
    This problem is most likely caused by an invalid program counter or\n\
stack pointer.\n\
    However, if you think GDB should simply search farther back\n\
from %s for code which looks like the beginning of a\n\
function, you can increase the range of the search using the `set\n\
heuristic-fence-post' command.\n",
			paddress (gdbarch, pc), paddress (gdbarch, pc));
		blurb_printed = 1;
	      }
	  }

	return 0;
      }
    else if (mips_pc_is_mips16 (start_pc))
      {
	unsigned short inst;

	/* On MIPS16, any one of the following is likely to be the
	   start of a function:
  	   extend save
	   save
	   entry
	   addiu sp,-n
	   daddiu sp,-n
	   extend -n followed by 'addiu sp,+n' or 'daddiu sp,+n'.  */
	inst = mips_fetch_instruction (gdbarch, start_pc);
	if ((inst & 0xff80) == 0x6480)		/* save */
	  {
	    if (start_pc - instlen >= fence)
	      {
		inst = mips_fetch_instruction (gdbarch, start_pc - instlen);
		if ((inst & 0xf800) == 0xf000)	/* extend */
		  start_pc -= instlen;
	      }
	    break;
	  }
	else if (((inst & 0xf81f) == 0xe809
		  && (inst & 0x700) != 0x700)	/* entry */
		 || (inst & 0xff80) == 0x6380	/* addiu sp,-n */
		 || (inst & 0xff80) == 0xfb80	/* daddiu sp,-n */
		 || ((inst & 0xf810) == 0xf010 && seen_adjsp))	/* extend -n */
	  break;
	else if ((inst & 0xff00) == 0x6300	/* addiu sp */
		 || (inst & 0xff00) == 0xfb00)	/* daddiu sp */
	  seen_adjsp = 1;
	else
	  seen_adjsp = 0;
      }
    else if (mips_about_to_return (gdbarch, start_pc))
      {
	/* Skip return and its delay slot.  */
	start_pc += 2 * MIPS_INSN32_SIZE;
	break;
      }

  return start_pc;
}

struct mips_objfile_private
{
  bfd_size_type size;
  char *contents;
};

/* According to the current ABI, should the type be passed in a
   floating-point register (assuming that there is space)?  When there
   is no FPU, FP are not even considered as possible candidates for
   FP registers and, consequently this returns false - forces FP
   arguments into integer registers.  */

static int
fp_register_arg_p (struct gdbarch *gdbarch, enum type_code typecode,
		   struct type *arg_type)
{
  return ((typecode == TYPE_CODE_FLT
	   || (MIPS_EABI (gdbarch)
	       && (typecode == TYPE_CODE_STRUCT
		   || typecode == TYPE_CODE_UNION)
	       && TYPE_NFIELDS (arg_type) == 1
	       && TYPE_CODE (check_typedef (TYPE_FIELD_TYPE (arg_type, 0))) 
	       == TYPE_CODE_FLT))
	  && MIPS_FPU_TYPE(gdbarch) != MIPS_FPU_NONE);
}

/* On o32, argument passing in GPRs depends on the alignment of the type being
   passed.  Return 1 if this type must be aligned to a doubleword boundary.  */

static int
mips_type_needs_double_align (struct type *type)
{
  enum type_code typecode = TYPE_CODE (type);

  if (typecode == TYPE_CODE_FLT && TYPE_LENGTH (type) == 8)
    return 1;
  else if (typecode == TYPE_CODE_STRUCT)
    {
      if (TYPE_NFIELDS (type) < 1)
	return 0;
      return mips_type_needs_double_align (TYPE_FIELD_TYPE (type, 0));
    }
  else if (typecode == TYPE_CODE_UNION)
    {
      int i, n;

      n = TYPE_NFIELDS (type);
      for (i = 0; i < n; i++)
	if (mips_type_needs_double_align (TYPE_FIELD_TYPE (type, i)))
	  return 1;
      return 0;
    }
  return 0;
}

/* Adjust the address downward (direction of stack growth) so that it
   is correctly aligned for a new stack frame.  */
static CORE_ADDR
mips_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 16);
}

static CORE_ADDR
mips_eabi_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			   struct regcache *regcache, CORE_ADDR bp_addr,
			   int nargs, struct value **args, CORE_ADDR sp,
			   int struct_return, CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);
  int regsize = mips_abi_regsize (gdbarch);

  /* For shared libraries, "t9" needs to point at the function
     address.  */
  regcache_cooked_write_signed (regcache, MIPS_T9_REGNUM, func_addr);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, MIPS_RA_REGNUM, bp_addr);

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = align_down (sp, 16);
  struct_addr = align_down (struct_addr, 16);

  /* Now make space on the stack for the args.  We allocate more
     than necessary for EABI, because the first few arguments are
     passed in registers, but that's OK.  */
  for (argnum = 0; argnum < nargs; argnum++)
    len += align_up (TYPE_LENGTH (value_type (args[argnum])), regsize);
  sp -= align_up (len, 16);

  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog,
			"mips_eabi_push_dummy_call: sp=%s allocated %ld\n",
			paddress (gdbarch, sp), (long) align_up (len, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = MIPS_A0_REGNUM;
  float_argreg = mips_fpa0_regnum (gdbarch);

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (struct_return)
    {
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_eabi_push_dummy_call: "
			    "struct_return reg=%d %s\n",
			    argreg, paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      gdb_byte valbuf[MAX_REGISTER_SIZE];
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (value_type (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_eabi_push_dummy_call: %d len=%d type=%d",
			    argnum + 1, len, (int) typecode);

      /* Function pointer arguments to mips16 code need to be made into
         mips16 pointers.  */
      if (typecode == TYPE_CODE_PTR
          && TYPE_CODE (TYPE_TARGET_TYPE (arg_type)) == TYPE_CODE_FUNC)
	{
	  CORE_ADDR addr = extract_signed_integer (value_contents (arg),
						   len, byte_order);
	  if (mips_pc_is_mips16 (addr))
	    {
	      store_signed_integer (valbuf, len, byte_order, 
				    make_mips16_addr (addr));
	      val = valbuf;
	    }
	  else
	    val = value_contents (arg);
	}
      /* The EABI passes structures that do not fit in a register by
         reference.  */
      else if (len > regsize
	  && (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION))
	{
	  store_unsigned_integer (valbuf, regsize, byte_order,
				  value_address (arg));
	  typecode = TYPE_CODE_PTR;
	  len = regsize;
	  val = valbuf;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stdlog, " push");
	}
      else
	val = value_contents (arg);

      /* 32-bit ABIs always start floating point arguments in an
         even-numbered floating point register.  Round the FP register
         up before the check to see if there are any FP registers
         left.  Non MIPS_EABI targets also pass the FP in the integer
         registers so also round up normal registers.  */
      if (regsize < 8 && fp_register_arg_p (gdbarch, typecode, arg_type))
	{
	  if ((float_argreg & 1))
	    float_argreg++;
	}

      /* Floating point arguments passed in registers have to be
         treated specially.  On 32-bit architectures, doubles
         are passed in register pairs; the even register gets
         the low word, and the odd register gets the high word.
         On non-EABI processors, the first two floating point arguments are
         also copied to general registers, because MIPS16 functions
         don't use float registers for arguments.  This duplication of
         arguments in general registers can't hurt non-MIPS16 functions
         because those registers are normally skipped.  */
      /* MIPS_EABI squeezes a struct that contains a single floating
         point value into an FP register instead of pushing it onto the
         stack.  */
      if (fp_register_arg_p (gdbarch, typecode, arg_type)
	  && float_argreg <= MIPS_LAST_FP_ARG_REGNUM (gdbarch))
	{
	  /* EABI32 will pass doubles in consecutive registers, even on
	     64-bit cores.  At one time, we used to check the size of
	     `float_argreg' to determine whether or not to pass doubles
	     in consecutive registers, but this is not sufficient for
	     making the ABI determination.  */
	  if (len == 8 && mips_abi (gdbarch) == MIPS_ABI_EABI32)
	    {
	      int low_offset = gdbarch_byte_order (gdbarch)
			       == BFD_ENDIAN_BIG ? 4 : 0;
	      long regval;

	      /* Write the low word of the double to the even register(s).  */
	      regval = extract_signed_integer (val + low_offset,
					       4, byte_order);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      regcache_cooked_write_signed (regcache, float_argreg++, regval);

	      /* Write the high word of the double to the odd register(s).  */
	      regval = extract_signed_integer (val + 4 - low_offset,
					       4, byte_order);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      regcache_cooked_write_signed (regcache, float_argreg++, regval);
	    }
	  else
	    {
	      /* This is a floating point value that fits entirely
	         in a single register.  */
	      /* On 32 bit ABI's the float_argreg is further adjusted
	         above to ensure that it is even register aligned.  */
	      LONGEST regval = extract_signed_integer (val, len, byte_order);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, len));
	      regcache_cooked_write_signed (regcache, float_argreg++, regval);
	    }
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* Note: structs whose size is not a multiple of regsize
	     are treated specially: Irix cc passes
	     them in registers where gcc sometimes puts them on the
	     stack.  For maximum compatibility, we will put them in
	     both places.  */
	  int odd_sized_struct = (len > regsize && len % regsize != 0);

	  /* Note: Floating-point values that didn't fit into an FP
	     register are only written to memory.  */
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = (len < regsize ? len : regsize);

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " -- partial=%d",
				    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > MIPS_LAST_ARG_REGNUM (gdbarch)
		  || odd_sized_struct
		  || fp_register_arg_p (gdbarch, typecode, arg_type))
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored?  */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
		    {
		      if (regsize == 8
			  && (typecode == TYPE_CODE_INT
			      || typecode == TYPE_CODE_PTR
			      || typecode == TYPE_CODE_FLT) && len <= 4)
			longword_offset = regsize - len;
		      else if ((typecode == TYPE_CODE_STRUCT
				|| typecode == TYPE_CODE_UNION)
			       && TYPE_LENGTH (arg_type) < regsize)
			longword_offset = regsize - len;
		    }

		  if (mips_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, " - stack_offset=%s",
					  paddress (gdbarch, stack_offset));
		      fprintf_unfiltered (gdb_stdlog, " longword_offset=%s",
					  paddress (gdbarch, longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      fprintf_unfiltered (gdb_stdlog, " @%s ",
					  paddress (gdbarch, addr));
		      for (i = 0; i < partial_len; i++)
			{
			  fprintf_unfiltered (gdb_stdlog, "%02x",
					      val[i] & 0xff);
			}
		    }
		  write_memory (addr, val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.  Odd sized
	         structs may go thru BOTH paths.  Floating point
	         arguments will not.  */
	      /* Write this portion of the argument to a general
	         purpose register.  */
	      if (argreg <= MIPS_LAST_ARG_REGNUM (gdbarch)
		  && !fp_register_arg_p (gdbarch, typecode, arg_type))
		{
		  LONGEST regval =
		    extract_signed_integer (val, partial_len, byte_order);

		  if (mips_debug)
		    fprintf_filtered (gdb_stdlog, " - reg=%d val=%s",
				      argreg,
				      phex (regval, regsize));
		  regcache_cooked_write_signed (regcache, argreg, regval);
		  argreg++;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the offset into the stack at which we will
	         copy the next parameter.

	         In the new EABI (and the NABI32), the stack_offset
	         only needs to be adjusted when it has been used.  */

	      if (stack_used_p)
		stack_offset += align_up (partial_len, regsize);
	    }
	}
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog, "\n");
    }

  regcache_cooked_write_signed (regcache, MIPS_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

/* Determine the return value convention being used.  */

static enum return_value_convention
mips_eabi_return_value (struct gdbarch *gdbarch, struct type *func_type,
			struct type *type, struct regcache *regcache,
			gdb_byte *readbuf, const gdb_byte *writebuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int fp_return_type = 0;
  int offset, regnum, xfer;

  if (TYPE_LENGTH (type) > 2 * mips_abi_regsize (gdbarch))
    return RETURN_VALUE_STRUCT_CONVENTION;

  /* Floating point type?  */
  if (tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      if (TYPE_CODE (type) == TYPE_CODE_FLT)
	fp_return_type = 1;
      /* Structs with a single field of float type 
	 are returned in a floating point register.  */
      if ((TYPE_CODE (type) == TYPE_CODE_STRUCT
	   || TYPE_CODE (type) == TYPE_CODE_UNION)
	  && TYPE_NFIELDS (type) == 1)
	{
	  struct type *fieldtype = TYPE_FIELD_TYPE (type, 0);

	  if (TYPE_CODE (check_typedef (fieldtype)) == TYPE_CODE_FLT)
	    fp_return_type = 1;
	}
    }

  if (fp_return_type)      
    {
      /* A floating-point value belongs in the least significant part
	 of FP0/FP1.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $fp0\n");
      regnum = mips_regnum (gdbarch)->fp0;
    }
  else 
    {
      /* An integer value goes in V0/V1.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return scalar in $v0\n");
      regnum = MIPS_V0_REGNUM;
    }
  for (offset = 0;
       offset < TYPE_LENGTH (type);
       offset += mips_abi_regsize (gdbarch), regnum++)
    {
      xfer = mips_abi_regsize (gdbarch);
      if (offset + xfer > TYPE_LENGTH (type))
	xfer = TYPE_LENGTH (type) - offset;
      mips_xfer_register (gdbarch, regcache,
			  gdbarch_num_regs (gdbarch) + regnum, xfer,
			  gdbarch_byte_order (gdbarch), readbuf, writebuf,
			  offset);
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* N32/N64 ABI stuff.  */

/* Search for a naturally aligned double at OFFSET inside a struct
   ARG_TYPE.  The N32 / N64 ABIs pass these in floating point
   registers.  */

static int
mips_n32n64_fp_arg_chunk_p (struct gdbarch *gdbarch, struct type *arg_type,
			    int offset)
{
  int i;

  if (TYPE_CODE (arg_type) != TYPE_CODE_STRUCT)
    return 0;

  if (MIPS_FPU_TYPE (gdbarch) != MIPS_FPU_DOUBLE)
    return 0;

  if (TYPE_LENGTH (arg_type) < offset + MIPS64_REGSIZE)
    return 0;

  for (i = 0; i < TYPE_NFIELDS (arg_type); i++)
    {
      int pos;
      struct type *field_type;

      /* We're only looking at normal fields.  */
      if (field_is_static (&TYPE_FIELD (arg_type, i))
	  || (TYPE_FIELD_BITPOS (arg_type, i) % 8) != 0)
	continue;

      /* If we have gone past the offset, there is no double to pass.  */
      pos = TYPE_FIELD_BITPOS (arg_type, i) / 8;
      if (pos > offset)
	return 0;

      field_type = check_typedef (TYPE_FIELD_TYPE (arg_type, i));

      /* If this field is entirely before the requested offset, go
	 on to the next one.  */
      if (pos + TYPE_LENGTH (field_type) <= offset)
	continue;

      /* If this is our special aligned double, we can stop.  */
      if (TYPE_CODE (field_type) == TYPE_CODE_FLT
	  && TYPE_LENGTH (field_type) == MIPS64_REGSIZE)
	return 1;

      /* This field starts at or before the requested offset, and
	 overlaps it.  If it is a structure, recurse inwards.  */
      return mips_n32n64_fp_arg_chunk_p (gdbarch, field_type, offset - pos);
    }

  return 0;
}

static CORE_ADDR
mips_n32n64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			     struct regcache *regcache, CORE_ADDR bp_addr,
			     int nargs, struct value **args, CORE_ADDR sp,
			     int struct_return, CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);

  /* For shared libraries, "t9" needs to point at the function
     address.  */
  regcache_cooked_write_signed (regcache, MIPS_T9_REGNUM, func_addr);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, MIPS_RA_REGNUM, bp_addr);

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = align_down (sp, 16);
  struct_addr = align_down (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    len += align_up (TYPE_LENGTH (value_type (args[argnum])), MIPS64_REGSIZE);
  sp -= align_up (len, 16);

  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog,
			"mips_n32n64_push_dummy_call: sp=%s allocated %ld\n",
			paddress (gdbarch, sp), (long) align_up (len, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = MIPS_A0_REGNUM;
  float_argreg = mips_fpa0_regnum (gdbarch);

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (struct_return)
    {
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_n32n64_push_dummy_call: "
			    "struct_return reg=%d %s\n",
			    argreg, paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (value_type (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_n32n64_push_dummy_call: %d len=%d type=%d",
			    argnum + 1, len, (int) typecode);

      val = value_contents (arg);

      /* A 128-bit long double value requires an even-odd pair of
	 floating-point registers.  */
      if (len == 16
	  && fp_register_arg_p (gdbarch, typecode, arg_type)
	  && (float_argreg & 1))
	{
	  float_argreg++;
	  argreg++;
	}

      if (fp_register_arg_p (gdbarch, typecode, arg_type)
	  && argreg <= MIPS_LAST_ARG_REGNUM (gdbarch))
	{
	  /* This is a floating point value that fits entirely
	     in a single register or a pair of registers.  */
	  int reglen = (len <= MIPS64_REGSIZE ? len : MIPS64_REGSIZE);
	  LONGEST regval = extract_unsigned_integer (val, reglen, byte_order);
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				float_argreg, phex (regval, reglen));
	  regcache_cooked_write_unsigned (regcache, float_argreg, regval);

	  if (mips_debug)
	    fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				argreg, phex (regval, reglen));
	  regcache_cooked_write_unsigned (regcache, argreg, regval);
	  float_argreg++;
	  argreg++;
	  if (len == 16)
	    {
	      regval = extract_unsigned_integer (val + reglen,
						 reglen, byte_order);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, reglen));
	      regcache_cooked_write_unsigned (regcache, float_argreg, regval);

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, reglen));
	      regcache_cooked_write_unsigned (regcache, argreg, regval);
	      float_argreg++;
	      argreg++;
	    }
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* For N32/N64, structs, unions, or other composite types are
	     treated as a sequence of doublewords, and are passed in integer
	     or floating point registers as though they were simple scalar
	     parameters to the extent that they fit, with any excess on the
	     stack packed according to the normal memory layout of the
	     object.
	     The caller does not reserve space for the register arguments;
	     the callee is responsible for reserving it if required.  */
	  /* Note: Floating-point values that didn't fit into an FP
	     register are only written to memory.  */
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = (len < MIPS64_REGSIZE ? len : MIPS64_REGSIZE);

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " -- partial=%d",
				    partial_len);

	      if (fp_register_arg_p (gdbarch, typecode, arg_type))
		gdb_assert (argreg > MIPS_LAST_ARG_REGNUM (gdbarch));

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > MIPS_LAST_ARG_REGNUM (gdbarch))
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored?  */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
		    {
		      if ((typecode == TYPE_CODE_INT
			   || typecode == TYPE_CODE_PTR)
			  && len <= 4)
			longword_offset = MIPS64_REGSIZE - len;
		    }

		  if (mips_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, " - stack_offset=%s",
					  paddress (gdbarch, stack_offset));
		      fprintf_unfiltered (gdb_stdlog, " longword_offset=%s",
					  paddress (gdbarch, longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      fprintf_unfiltered (gdb_stdlog, " @%s ",
					  paddress (gdbarch, addr));
		      for (i = 0; i < partial_len; i++)
			{
			  fprintf_unfiltered (gdb_stdlog, "%02x",
					      val[i] & 0xff);
			}
		    }
		  write_memory (addr, val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.  Odd sized
	         structs may go thru BOTH paths.  */
	      /* Write this portion of the argument to a general
	         purpose register.  */
	      if (argreg <= MIPS_LAST_ARG_REGNUM (gdbarch))
		{
		  LONGEST regval;

		  /* Sign extend pointers, 32-bit integers and signed
		     16-bit and 8-bit integers; everything else is taken
		     as is.  */

		  if ((partial_len == 4
		       && (typecode == TYPE_CODE_PTR
			   || typecode == TYPE_CODE_INT))
		      || (partial_len < 4
			  && typecode == TYPE_CODE_INT
			  && !TYPE_UNSIGNED (arg_type)))
		    regval = extract_signed_integer (val, partial_len,
						     byte_order);
		  else
		    regval = extract_unsigned_integer (val, partial_len,
						       byte_order);

		  /* A non-floating-point argument being passed in a
		     general register.  If a struct or union, and if
		     the remaining length is smaller than the register
		     size, we have to adjust the register value on
		     big endian targets.

		     It does not seem to be necessary to do the
		     same for integral types.  */

		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
		      && partial_len < MIPS64_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT
			  || typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS64_REGSIZE - partial_len)
				* TARGET_CHAR_BIT);

		  if (mips_debug)
		    fprintf_filtered (gdb_stdlog, " - reg=%d val=%s",
				      argreg,
				      phex (regval, MIPS64_REGSIZE));
		  regcache_cooked_write_unsigned (regcache, argreg, regval);

		  if (mips_n32n64_fp_arg_chunk_p (gdbarch, arg_type,
						  TYPE_LENGTH (arg_type) - len))
		    {
		      if (mips_debug)
			fprintf_filtered (gdb_stdlog, " - fpreg=%d val=%s",
					  float_argreg,
					  phex (regval, MIPS64_REGSIZE));
		      regcache_cooked_write_unsigned (regcache, float_argreg,
						      regval);
		    }

		  float_argreg++;
		  argreg++;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the offset into the stack at which we will
	         copy the next parameter.

	         In N32 (N64?), the stack_offset only needs to be
	         adjusted when it has been used.  */

	      if (stack_used_p)
		stack_offset += align_up (partial_len, MIPS64_REGSIZE);
	    }
	}
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog, "\n");
    }

  regcache_cooked_write_signed (regcache, MIPS_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

static enum return_value_convention
mips_n32n64_return_value (struct gdbarch *gdbarch, struct type *func_type,
			  struct type *type, struct regcache *regcache,
			  gdb_byte *readbuf, const gdb_byte *writebuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* From MIPSpro N32 ABI Handbook, Document Number: 007-2816-004

     Function results are returned in $2 (and $3 if needed), or $f0 (and $f2
     if needed), as appropriate for the type.  Composite results (struct,
     union, or array) are returned in $2/$f0 and $3/$f2 according to the
     following rules:

     * A struct with only one or two floating point fields is returned in $f0
     (and $f2 if necessary).  This is a generalization of the Fortran COMPLEX
     case.

     * Any other composite results of at most 128 bits are returned in
     $2 (first 64 bits) and $3 (remainder, if necessary).

     * Larger composite results are handled by converting the function to a
     procedure with an implicit first parameter, which is a pointer to an area
     reserved by the caller to receive the result.  [The o32-bit ABI requires
     that all composite results be handled by conversion to implicit first
     parameters.  The MIPS/SGI Fortran implementation has always made a
     specific exception to return COMPLEX results in the floating point
     registers.]  */

  if (TYPE_LENGTH (type) > 2 * MIPS64_REGSIZE)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else if (TYPE_CODE (type) == TYPE_CODE_FLT
	   && TYPE_LENGTH (type) == 16
	   && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A 128-bit floating-point value fills both $f0 and $f2.  The
	 two registers are used in the same as memory order, so the
	 eight bytes with the lower memory address are in $f0.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $f0 and $f2\n");
      mips_xfer_register (gdbarch, regcache,
			  gdbarch_num_regs (gdbarch)
			  + mips_regnum (gdbarch)->fp0,
			  8, gdbarch_byte_order (gdbarch),
			  readbuf, writebuf, 0);
      mips_xfer_register (gdbarch, regcache,
			  gdbarch_num_regs (gdbarch)
			  + mips_regnum (gdbarch)->fp0 + 2,
			  8, gdbarch_byte_order (gdbarch),
			  readbuf ? readbuf + 8 : readbuf,
			  writebuf ? writebuf + 8 : writebuf, 0);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else if (TYPE_CODE (type) == TYPE_CODE_FLT
	   && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A single or double floating-point value that fits in FP0.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $fp0\n");
      mips_xfer_register (gdbarch, regcache,
			  gdbarch_num_regs (gdbarch)
			  + mips_regnum (gdbarch)->fp0,
			  TYPE_LENGTH (type),
			  gdbarch_byte_order (gdbarch),
			  readbuf, writebuf, 0);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else if (TYPE_CODE (type) == TYPE_CODE_STRUCT
	   && TYPE_NFIELDS (type) <= 2
	   && TYPE_NFIELDS (type) >= 1
	   && ((TYPE_NFIELDS (type) == 1
		&& (TYPE_CODE (check_typedef (TYPE_FIELD_TYPE (type, 0)))
		    == TYPE_CODE_FLT))
	       || (TYPE_NFIELDS (type) == 2
		   && (TYPE_CODE (check_typedef (TYPE_FIELD_TYPE (type, 0)))
		       == TYPE_CODE_FLT)
		   && (TYPE_CODE (check_typedef (TYPE_FIELD_TYPE (type, 1)))
		       == TYPE_CODE_FLT))))
    {
      /* A struct that contains one or two floats.  Each value is part
         in the least significant part of their floating point
         register (or GPR, for soft float).  */
      int regnum;
      int field;
      for (field = 0, regnum = (tdep->mips_fpu_type != MIPS_FPU_NONE
				? mips_regnum (gdbarch)->fp0
				: MIPS_V0_REGNUM);
	   field < TYPE_NFIELDS (type); field++, regnum += 2)
	{
	  int offset = (FIELD_BITPOS (TYPE_FIELDS (type)[field])
			/ TARGET_CHAR_BIT);
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return float struct+%d\n",
				offset);
	  if (TYPE_LENGTH (TYPE_FIELD_TYPE (type, field)) == 16)
	    {
	      /* A 16-byte long double field goes in two consecutive
		 registers.  */
	      mips_xfer_register (gdbarch, regcache,
				  gdbarch_num_regs (gdbarch) + regnum,
				  8,
				  gdbarch_byte_order (gdbarch),
				  readbuf, writebuf, offset);
	      mips_xfer_register (gdbarch, regcache,
				  gdbarch_num_regs (gdbarch) + regnum + 1,
				  8,
				  gdbarch_byte_order (gdbarch),
				  readbuf, writebuf, offset + 8);
	    }
	  else
	    mips_xfer_register (gdbarch, regcache,
				gdbarch_num_regs (gdbarch) + regnum,
				TYPE_LENGTH (TYPE_FIELD_TYPE (type, field)),
				gdbarch_byte_order (gdbarch),
				readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else if (TYPE_CODE (type) == TYPE_CODE_STRUCT
	   || TYPE_CODE (type) == TYPE_CODE_UNION
	   || TYPE_CODE (type) == TYPE_CODE_ARRAY)
    {
      /* A composite type.  Extract the left justified value,
         regardless of the byte order.  I.e. DO NOT USE
         mips_xfer_lower.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += register_size (gdbarch, regnum), regnum++)
	{
	  int xfer = register_size (gdbarch, regnum);
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return struct+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum,
			      xfer, BFD_ENDIAN_UNKNOWN, readbuf, writebuf,
			      offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else
    {
      /* A scalar extract each part but least-significant-byte
         justified.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += register_size (gdbarch, regnum), regnum++)
	{
	  int xfer = register_size (gdbarch, regnum);
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return scalar+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum,
			      xfer, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* O32 ABI stuff.  */

static CORE_ADDR
mips_o32_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			  struct regcache *regcache, CORE_ADDR bp_addr,
			  int nargs, struct value **args, CORE_ADDR sp,
			  int struct_return, CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);

  /* For shared libraries, "t9" needs to point at the function
     address.  */
  regcache_cooked_write_signed (regcache, MIPS_T9_REGNUM, func_addr);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, MIPS_RA_REGNUM, bp_addr);

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = align_down (sp, 16);
  struct_addr = align_down (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      struct type *arg_type = check_typedef (value_type (args[argnum]));
      int arglen = TYPE_LENGTH (arg_type);

      /* Align to double-word if necessary.  */
      if (mips_type_needs_double_align (arg_type))
	len = align_up (len, MIPS32_REGSIZE * 2);
      /* Allocate space on the stack.  */
      len += align_up (arglen, MIPS32_REGSIZE);
    }
  sp -= align_up (len, 16);

  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog,
			"mips_o32_push_dummy_call: sp=%s allocated %ld\n",
			paddress (gdbarch, sp), (long) align_up (len, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = MIPS_A0_REGNUM;
  float_argreg = mips_fpa0_regnum (gdbarch);

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (struct_return)
    {
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_o32_push_dummy_call: "
			    "struct_return reg=%d %s\n",
			    argreg, paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
      stack_offset += MIPS32_REGSIZE;
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (value_type (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_o32_push_dummy_call: %d len=%d type=%d",
			    argnum + 1, len, (int) typecode);

      val = value_contents (arg);

      /* 32-bit ABIs always start floating point arguments in an
         even-numbered floating point register.  Round the FP register
         up before the check to see if there are any FP registers
         left.  O32/O64 targets also pass the FP in the integer
         registers so also round up normal registers.  */
      if (fp_register_arg_p (gdbarch, typecode, arg_type))
	{
	  if ((float_argreg & 1))
	    float_argreg++;
	}

      /* Floating point arguments passed in registers have to be
         treated specially.  On 32-bit architectures, doubles
         are passed in register pairs; the even register gets
         the low word, and the odd register gets the high word.
         On O32/O64, the first two floating point arguments are
         also copied to general registers, because MIPS16 functions
         don't use float registers for arguments.  This duplication of
         arguments in general registers can't hurt non-MIPS16 functions
         because those registers are normally skipped.  */

      if (fp_register_arg_p (gdbarch, typecode, arg_type)
	  && float_argreg <= MIPS_LAST_FP_ARG_REGNUM (gdbarch))
	{
	  if (register_size (gdbarch, float_argreg) < 8 && len == 8)
	    {
	      int low_offset = gdbarch_byte_order (gdbarch)
			       == BFD_ENDIAN_BIG ? 4 : 0;
	      unsigned long regval;

	      /* Write the low word of the double to the even register(s).  */
	      regval = extract_unsigned_integer (val + low_offset,
						 4, byte_order);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      regcache_cooked_write_unsigned (regcache,
					      float_argreg++, regval);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, 4));
	      regcache_cooked_write_unsigned (regcache, argreg++, regval);

	      /* Write the high word of the double to the odd register(s).  */
	      regval = extract_unsigned_integer (val + 4 - low_offset,
						 4, byte_order);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      regcache_cooked_write_unsigned (regcache,
					      float_argreg++, regval);

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, 4));
	      regcache_cooked_write_unsigned (regcache, argreg++, regval);
	    }
	  else
	    {
	      /* This is a floating point value that fits entirely
	         in a single register.  */
	      /* On 32 bit ABI's the float_argreg is further adjusted
	         above to ensure that it is even register aligned.  */
	      LONGEST regval = extract_unsigned_integer (val, len, byte_order);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, len));
	      regcache_cooked_write_unsigned (regcache,
					      float_argreg++, regval);
	      /* Although two FP registers are reserved for each
		 argument, only one corresponding integer register is
		 reserved.  */
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, len));
	      regcache_cooked_write_unsigned (regcache, argreg++, regval);
	    }
	  /* Reserve space for the FP register.  */
	  stack_offset += align_up (len, MIPS32_REGSIZE);
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* Note: structs whose size is not a multiple of MIPS32_REGSIZE
	     are treated specially: Irix cc passes
	     them in registers where gcc sometimes puts them on the
	     stack.  For maximum compatibility, we will put them in
	     both places.  */
	  int odd_sized_struct = (len > MIPS32_REGSIZE
				  && len % MIPS32_REGSIZE != 0);
	  /* Structures should be aligned to eight bytes (even arg registers)
	     on MIPS_ABI_O32, if their first member has double precision.  */
	  if (mips_type_needs_double_align (arg_type))
	    {
	      if ((argreg & 1))
		{
		  argreg++;
		  stack_offset += MIPS32_REGSIZE;
		}
	    }
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = (len < MIPS32_REGSIZE ? len : MIPS32_REGSIZE);

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " -- partial=%d",
				    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > MIPS_LAST_ARG_REGNUM (gdbarch)
		  || odd_sized_struct)
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored?  */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;

		  if (mips_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, " - stack_offset=%s",
					  paddress (gdbarch, stack_offset));
		      fprintf_unfiltered (gdb_stdlog, " longword_offset=%s",
					  paddress (gdbarch, longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      fprintf_unfiltered (gdb_stdlog, " @%s ",
					  paddress (gdbarch, addr));
		      for (i = 0; i < partial_len; i++)
			{
			  fprintf_unfiltered (gdb_stdlog, "%02x",
					      val[i] & 0xff);
			}
		    }
		  write_memory (addr, val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.  Odd sized
	         structs may go thru BOTH paths.  */
	      /* Write this portion of the argument to a general
	         purpose register.  */
	      if (argreg <= MIPS_LAST_ARG_REGNUM (gdbarch))
		{
		  LONGEST regval = extract_signed_integer (val, partial_len,
							   byte_order);
		  /* Value may need to be sign extended, because
		     mips_isa_regsize() != mips_abi_regsize().  */

		  /* A non-floating-point argument being passed in a
		     general register.  If a struct or union, and if
		     the remaining length is smaller than the register
		     size, we have to adjust the register value on
		     big endian targets.

		     It does not seem to be necessary to do the
		     same for integral types.

		     Also don't do this adjustment on O64 binaries.

		     cagney/2001-07-23: gdb/179: Also, GCC, when
		     outputting LE O32 with sizeof (struct) <
		     mips_abi_regsize(), generates a left shift
		     as part of storing the argument in a register
		     (the left shift isn't generated when
		     sizeof (struct) >= mips_abi_regsize()).  Since
		     it is quite possible that this is GCC
		     contradicting the LE/O32 ABI, GDB has not been
		     adjusted to accommodate this.  Either someone
		     needs to demonstrate that the LE/O32 ABI
		     specifies such a left shift OR this new ABI gets
		     identified as such and GDB gets tweaked
		     accordingly.  */

		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
		      && partial_len < MIPS32_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT
			  || typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS32_REGSIZE - partial_len)
				* TARGET_CHAR_BIT);

		  if (mips_debug)
		    fprintf_filtered (gdb_stdlog, " - reg=%d val=%s",
				      argreg,
				      phex (regval, MIPS32_REGSIZE));
		  regcache_cooked_write_unsigned (regcache, argreg, regval);
		  argreg++;

		  /* Prevent subsequent floating point arguments from
		     being passed in floating point registers.  */
		  float_argreg = MIPS_LAST_FP_ARG_REGNUM (gdbarch) + 1;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the offset into the stack at which we will
	         copy the next parameter.

	         In older ABIs, the caller reserved space for
	         registers that contained arguments.  This was loosely
	         refered to as their "home".  Consequently, space is
	         always allocated.  */

	      stack_offset += align_up (partial_len, MIPS32_REGSIZE);
	    }
	}
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog, "\n");
    }

  regcache_cooked_write_signed (regcache, MIPS_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

static enum return_value_convention
mips_o32_return_value (struct gdbarch *gdbarch, struct type *func_type,
		       struct type *type, struct regcache *regcache,
		       gdb_byte *readbuf, const gdb_byte *writebuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      || TYPE_CODE (type) == TYPE_CODE_UNION
      || TYPE_CODE (type) == TYPE_CODE_ARRAY)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else if (TYPE_CODE (type) == TYPE_CODE_FLT
	   && TYPE_LENGTH (type) == 4 && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A single-precision floating-point value.  It fits in the
         least significant part of FP0.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $fp0\n");
      mips_xfer_register (gdbarch, regcache,
			  gdbarch_num_regs (gdbarch)
			    + mips_regnum (gdbarch)->fp0,
			  TYPE_LENGTH (type),
			  gdbarch_byte_order (gdbarch),
			  readbuf, writebuf, 0);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else if (TYPE_CODE (type) == TYPE_CODE_FLT
	   && TYPE_LENGTH (type) == 8 && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A double-precision floating-point value.  The most
         significant part goes in FP1, and the least significant in
         FP0.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $fp1/$fp0\n");
      switch (gdbarch_byte_order (gdbarch))
	{
	case BFD_ENDIAN_LITTLE:
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch)
				+ mips_regnum (gdbarch)->fp0 +
			      0, 4, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, 0);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch)
				+ mips_regnum (gdbarch)->fp0 + 1,
			      4, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, 4);
	  break;
	case BFD_ENDIAN_BIG:
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch)
				+ mips_regnum (gdbarch)->fp0 + 1,
			      4, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, 0);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch)
				+ mips_regnum (gdbarch)->fp0 + 0,
			      4, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, 4);
	  break;
	default:
	  internal_error (__FILE__, __LINE__, _("bad switch"));
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
#if 0
  else if (TYPE_CODE (type) == TYPE_CODE_STRUCT
	   && TYPE_NFIELDS (type) <= 2
	   && TYPE_NFIELDS (type) >= 1
	   && ((TYPE_NFIELDS (type) == 1
		&& (TYPE_CODE (TYPE_FIELD_TYPE (type, 0))
		    == TYPE_CODE_FLT))
	       || (TYPE_NFIELDS (type) == 2
		   && (TYPE_CODE (TYPE_FIELD_TYPE (type, 0))
		       == TYPE_CODE_FLT)
		   && (TYPE_CODE (TYPE_FIELD_TYPE (type, 1))
		       == TYPE_CODE_FLT)))
	   && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A struct that contains one or two floats.  Each value is part
         in the least significant part of their floating point
         register..  */
      gdb_byte reg[MAX_REGISTER_SIZE];
      int regnum;
      int field;
      for (field = 0, regnum = mips_regnum (gdbarch)->fp0;
	   field < TYPE_NFIELDS (type); field++, regnum += 2)
	{
	  int offset = (FIELD_BITPOS (TYPE_FIELDS (type)[field])
			/ TARGET_CHAR_BIT);
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return float struct+%d\n",
				offset);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum,
			      TYPE_LENGTH (TYPE_FIELD_TYPE (type, field)),
			      gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
#endif
#if 0
  else if (TYPE_CODE (type) == TYPE_CODE_STRUCT
	   || TYPE_CODE (type) == TYPE_CODE_UNION)
    {
      /* A structure or union.  Extract the left justified value,
         regardless of the byte order.  I.e. DO NOT USE
         mips_xfer_lower.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += register_size (gdbarch, regnum), regnum++)
	{
	  int xfer = register_size (gdbarch, regnum);
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return struct+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum, xfer,
			      BFD_ENDIAN_UNKNOWN, readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
#endif
  else
    {
      /* A scalar extract each part but least-significant-byte
         justified.  o32 thinks registers are 4 byte, regardless of
         the ISA.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += MIPS32_REGSIZE, regnum++)
	{
	  int xfer = MIPS32_REGSIZE;
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return scalar+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum, xfer,
			      gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* O64 ABI.  This is a hacked up kind of 64-bit version of the o32
   ABI.  */

static CORE_ADDR
mips_o64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			  struct regcache *regcache, CORE_ADDR bp_addr,
			  int nargs,
			  struct value **args, CORE_ADDR sp,
			  int struct_return, CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR func_addr = find_function_addr (function, NULL);

  /* For shared libraries, "t9" needs to point at the function
     address.  */
  regcache_cooked_write_signed (regcache, MIPS_T9_REGNUM, func_addr);

  /* Set the return address register to point to the entry point of
     the program, where a breakpoint lies in wait.  */
  regcache_cooked_write_signed (regcache, MIPS_RA_REGNUM, bp_addr);

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = align_down (sp, 16);
  struct_addr = align_down (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      struct type *arg_type = check_typedef (value_type (args[argnum]));
      int arglen = TYPE_LENGTH (arg_type);

      /* Allocate space on the stack.  */
      len += align_up (arglen, MIPS64_REGSIZE);
    }
  sp -= align_up (len, 16);

  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog,
			"mips_o64_push_dummy_call: sp=%s allocated %ld\n",
			paddress (gdbarch, sp), (long) align_up (len, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = MIPS_A0_REGNUM;
  float_argreg = mips_fpa0_regnum (gdbarch);

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (struct_return)
    {
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_o64_push_dummy_call: "
			    "struct_return reg=%d %s\n",
			    argreg, paddress (gdbarch, struct_addr));
      regcache_cooked_write_unsigned (regcache, argreg++, struct_addr);
      stack_offset += MIPS64_REGSIZE;
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      const gdb_byte *val;
      gdb_byte valbuf[MAX_REGISTER_SIZE];
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (value_type (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_o64_push_dummy_call: %d len=%d type=%d",
			    argnum + 1, len, (int) typecode);

      val = value_contents (arg);

      /* Function pointer arguments to mips16 code need to be made into
         mips16 pointers.  */
      if (typecode == TYPE_CODE_PTR
          && TYPE_CODE (TYPE_TARGET_TYPE (arg_type)) == TYPE_CODE_FUNC)
	{
	  CORE_ADDR addr = extract_signed_integer (value_contents (arg),
						   len, byte_order);
	  if (mips_pc_is_mips16 (addr))
	    {
	      store_signed_integer (valbuf, len, byte_order, 
				    make_mips16_addr (addr));
	      val = valbuf;
	    }
	}

      /* Floating point arguments passed in registers have to be
         treated specially.  On 32-bit architectures, doubles
         are passed in register pairs; the even register gets
         the low word, and the odd register gets the high word.
         On O32/O64, the first two floating point arguments are
         also copied to general registers, because MIPS16 functions
         don't use float registers for arguments.  This duplication of
         arguments in general registers can't hurt non-MIPS16 functions
         because those registers are normally skipped.  */

      if (fp_register_arg_p (gdbarch, typecode, arg_type)
	  && float_argreg <= MIPS_LAST_FP_ARG_REGNUM (gdbarch))
	{
	  LONGEST regval = extract_unsigned_integer (val, len, byte_order);
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				float_argreg, phex (regval, len));
	  regcache_cooked_write_unsigned (regcache, float_argreg++, regval);
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				argreg, phex (regval, len));
	  regcache_cooked_write_unsigned (regcache, argreg, regval);
	  argreg++;
	  /* Reserve space for the FP register.  */
	  stack_offset += align_up (len, MIPS64_REGSIZE);
	}
      else
	{
	  /* Copy the argument to general registers or the stack in
	     register-sized pieces.  Large arguments are split between
	     registers and stack.  */
	  /* Note: structs whose size is not a multiple of MIPS64_REGSIZE
	     are treated specially: Irix cc passes them in registers
	     where gcc sometimes puts them on the stack.  For maximum
	     compatibility, we will put them in both places.  */
	  int odd_sized_struct = (len > MIPS64_REGSIZE
				  && len % MIPS64_REGSIZE != 0);
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = (len < MIPS64_REGSIZE ? len : MIPS64_REGSIZE);

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " -- partial=%d",
				    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > MIPS_LAST_ARG_REGNUM (gdbarch)
		  || odd_sized_struct)
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored?  */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
		    {
		      if ((typecode == TYPE_CODE_INT
			   || typecode == TYPE_CODE_PTR
			   || typecode == TYPE_CODE_FLT)
			  && len <= 4)
			longword_offset = MIPS64_REGSIZE - len;
		    }

		  if (mips_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, " - stack_offset=%s",
					  paddress (gdbarch, stack_offset));
		      fprintf_unfiltered (gdb_stdlog, " longword_offset=%s",
					  paddress (gdbarch, longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      fprintf_unfiltered (gdb_stdlog, " @%s ",
					  paddress (gdbarch, addr));
		      for (i = 0; i < partial_len; i++)
			{
			  fprintf_unfiltered (gdb_stdlog, "%02x",
					      val[i] & 0xff);
			}
		    }
		  write_memory (addr, val, partial_len);
		}

	      /* Note!!! This is NOT an else clause.  Odd sized
	         structs may go thru BOTH paths.  */
	      /* Write this portion of the argument to a general
	         purpose register.  */
	      if (argreg <= MIPS_LAST_ARG_REGNUM (gdbarch))
		{
		  LONGEST regval = extract_signed_integer (val, partial_len,
							   byte_order);
		  /* Value may need to be sign extended, because
		     mips_isa_regsize() != mips_abi_regsize().  */

		  /* A non-floating-point argument being passed in a
		     general register.  If a struct or union, and if
		     the remaining length is smaller than the register
		     size, we have to adjust the register value on
		     big endian targets.

		     It does not seem to be necessary to do the
		     same for integral types.  */

		  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
		      && partial_len < MIPS64_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT
			  || typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS64_REGSIZE - partial_len)
				* TARGET_CHAR_BIT);

		  if (mips_debug)
		    fprintf_filtered (gdb_stdlog, " - reg=%d val=%s",
				      argreg,
				      phex (regval, MIPS64_REGSIZE));
		  regcache_cooked_write_unsigned (regcache, argreg, regval);
		  argreg++;

		  /* Prevent subsequent floating point arguments from
		     being passed in floating point registers.  */
		  float_argreg = MIPS_LAST_FP_ARG_REGNUM (gdbarch) + 1;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the offset into the stack at which we will
	         copy the next parameter.

	         In older ABIs, the caller reserved space for
	         registers that contained arguments.  This was loosely
	         refered to as their "home".  Consequently, space is
	         always allocated.  */

	      stack_offset += align_up (partial_len, MIPS64_REGSIZE);
	    }
	}
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog, "\n");
    }

  regcache_cooked_write_signed (regcache, MIPS_SP_REGNUM, sp);

  /* Return adjusted stack pointer.  */
  return sp;
}

static enum return_value_convention
mips_o64_return_value (struct gdbarch *gdbarch, struct type *func_type,
		       struct type *type, struct regcache *regcache,
		       gdb_byte *readbuf, const gdb_byte *writebuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      || TYPE_CODE (type) == TYPE_CODE_UNION
      || TYPE_CODE (type) == TYPE_CODE_ARRAY)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else if (fp_register_arg_p (gdbarch, TYPE_CODE (type), type))
    {
      /* A floating-point value.  It fits in the least significant
         part of FP0.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $fp0\n");
      mips_xfer_register (gdbarch, regcache,
			  gdbarch_num_regs (gdbarch)
			    + mips_regnum (gdbarch)->fp0,
			  TYPE_LENGTH (type),
			  gdbarch_byte_order (gdbarch),
			  readbuf, writebuf, 0);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  else
    {
      /* A scalar extract each part but least-significant-byte
         justified.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = MIPS_V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += MIPS64_REGSIZE, regnum++)
	{
	  int xfer = MIPS64_REGSIZE;
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return scalar+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (gdbarch, regcache,
			      gdbarch_num_regs (gdbarch) + regnum,
			      xfer, gdbarch_byte_order (gdbarch),
			      readbuf, writebuf, offset);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Floating point register management.

   Background: MIPS1 & 2 fp registers are 32 bits wide.  To support
   64bit operations, these early MIPS cpus treat fp register pairs
   (f0,f1) as a single register (d0).  Later MIPS cpu's have 64 bit fp
   registers and offer a compatibility mode that emulates the MIPS2 fp
   model.  When operating in MIPS2 fp compat mode, later cpu's split
   double precision floats into two 32-bit chunks and store them in
   consecutive fp regs.  To display 64-bit floats stored in this
   fashion, we have to combine 32 bits from f0 and 32 bits from f1.
   Throw in user-configurable endianness and you have a real mess.

   The way this works is:
     - If we are in 32-bit mode or on a 32-bit processor, then a 64-bit
       double-precision value will be split across two logical registers.
       The lower-numbered logical register will hold the low-order bits,
       regardless of the processor's endianness.
     - If we are on a 64-bit processor, and we are looking for a
       single-precision value, it will be in the low ordered bits
       of a 64-bit GPR (after mfc1, for example) or a 64-bit register
       save slot in memory.
     - If we are in 64-bit mode, everything is straightforward.

   Note that this code only deals with "live" registers at the top of the
   stack.  We will attempt to deal with saved registers later, when
   the raw/cooked register interface is in place.  (We need a general
   interface that can deal with dynamic saved register sizes -- fp
   regs could be 32 bits wide in one frame and 64 on the frame above
   and below).  */

/* Copy a 32-bit single-precision value from the current frame
   into rare_buffer.  */

static void
mips_read_fp_register_single (struct frame_info *frame, int regno,
			      gdb_byte *rare_buffer)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int raw_size = register_size (gdbarch, regno);
  gdb_byte *raw_buffer = alloca (raw_size);

  if (!frame_register_read (frame, regno, raw_buffer))
    error (_("can't read register %d (%s)"),
	   regno, gdbarch_register_name (gdbarch, regno));
  if (raw_size == 8)
    {
      /* We have a 64-bit value for this register.  Find the low-order
         32 bits.  */
      int offset;

      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	offset = 4;
      else
	offset = 0;

      memcpy (rare_buffer, raw_buffer + offset, 4);
    }
  else
    {
      memcpy (rare_buffer, raw_buffer, 4);
    }
}

/* Copy a 64-bit double-precision value from the current frame into
   rare_buffer.  This may include getting half of it from the next
   register.  */

static void
mips_read_fp_register_double (struct frame_info *frame, int regno,
			      gdb_byte *rare_buffer)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int raw_size = register_size (gdbarch, regno);

  if (raw_size == 8 && !mips2_fp_compat (frame))
    {
      /* We have a 64-bit value for this register, and we should use
         all 64 bits.  */
      if (!frame_register_read (frame, regno, rare_buffer))
	error (_("can't read register %d (%s)"),
	       regno, gdbarch_register_name (gdbarch, regno));
    }
  else
    {
      int rawnum = regno % gdbarch_num_regs (gdbarch);

      if ((rawnum - mips_regnum (gdbarch)->fp0) & 1)
	internal_error (__FILE__, __LINE__,
			_("mips_read_fp_register_double: bad access to "
			"odd-numbered FP register"));

      /* mips_read_fp_register_single will find the correct 32 bits from
         each register.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	{
	  mips_read_fp_register_single (frame, regno, rare_buffer + 4);
	  mips_read_fp_register_single (frame, regno + 1, rare_buffer);
	}
      else
	{
	  mips_read_fp_register_single (frame, regno, rare_buffer);
	  mips_read_fp_register_single (frame, regno + 1, rare_buffer + 4);
	}
    }
}

static void
mips_print_fp_register (struct ui_file *file, struct frame_info *frame,
			int regnum)
{				/* Do values for FP (float) regs.  */
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte *raw_buffer;
  double doub, flt1;	/* Doubles extracted from raw hex data.  */
  int inv1, inv2;

  raw_buffer = alloca (2 * register_size (gdbarch,
					  mips_regnum (gdbarch)->fp0));

  fprintf_filtered (file, "%s:", gdbarch_register_name (gdbarch, regnum));
  fprintf_filtered (file, "%*s",
		    4 - (int) strlen (gdbarch_register_name (gdbarch, regnum)),
		    "");

  if (register_size (gdbarch, regnum) == 4 || mips2_fp_compat (frame))
    {
      struct value_print_options opts;

      /* 4-byte registers: Print hex and floating.  Also print even
         numbered registers as doubles.  */
      mips_read_fp_register_single (frame, regnum, raw_buffer);
      flt1 = unpack_double (builtin_type (gdbarch)->builtin_float,
			    raw_buffer, &inv1);

      get_formatted_print_options (&opts, 'x');
      print_scalar_formatted (raw_buffer,
			      builtin_type (gdbarch)->builtin_uint32,
			      &opts, 'w', file);

      fprintf_filtered (file, " flt: ");
      if (inv1)
	fprintf_filtered (file, " <invalid float> ");
      else
	fprintf_filtered (file, "%-17.9g", flt1);

      if ((regnum - gdbarch_num_regs (gdbarch)) % 2 == 0)
	{
	  mips_read_fp_register_double (frame, regnum, raw_buffer);
	  doub = unpack_double (builtin_type (gdbarch)->builtin_double,
				raw_buffer, &inv2);

	  fprintf_filtered (file, " dbl: ");
	  if (inv2)
	    fprintf_filtered (file, "<invalid double>");
	  else
	    fprintf_filtered (file, "%-24.17g", doub);
	}
    }
  else
    {
      struct value_print_options opts;

      /* Eight byte registers: print each one as hex, float and double.  */
      mips_read_fp_register_single (frame, regnum, raw_buffer);
      flt1 = unpack_double (builtin_type (gdbarch)->builtin_float,
			    raw_buffer, &inv1);

      mips_read_fp_register_double (frame, regnum, raw_buffer);
      doub = unpack_double (builtin_type (gdbarch)->builtin_double,
			    raw_buffer, &inv2);

      get_formatted_print_options (&opts, 'x');
      print_scalar_formatted (raw_buffer,
			      builtin_type (gdbarch)->builtin_uint64,
			      &opts, 'g', file);

      fprintf_filtered (file, " flt: ");
      if (inv1)
	fprintf_filtered (file, "<invalid float>");
      else
	fprintf_filtered (file, "%-17.9g", flt1);

      fprintf_filtered (file, " dbl: ");
      if (inv2)
	fprintf_filtered (file, "<invalid double>");
      else
	fprintf_filtered (file, "%-24.17g", doub);
    }
}

static void
mips_print_register (struct ui_file *file, struct frame_info *frame,
		     int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int offset;
  struct value_print_options opts;
  struct value *val;

  if (TYPE_CODE (register_type (gdbarch, regnum)) == TYPE_CODE_FLT)
    {
      mips_print_fp_register (file, frame, regnum);
      return;
    }

  val = get_frame_register_value (frame, regnum);
  if (value_optimized_out (val))
    {
      fprintf_filtered (file, "%s: [Invalid]",
			gdbarch_register_name (gdbarch, regnum));
      return;
    }

  fputs_filtered (gdbarch_register_name (gdbarch, regnum), file);

  /* The problem with printing numeric register names (r26, etc.) is that
     the user can't use them on input.  Probably the best solution is to
     fix it so that either the numeric or the funky (a2, etc.) names
     are accepted on input.  */
  if (regnum < MIPS_NUMREGS)
    fprintf_filtered (file, "(r%d): ", regnum);
  else
    fprintf_filtered (file, ": ");

  get_formatted_print_options (&opts, 'x');
  val_print_scalar_formatted (value_type (val),
			      value_contents_for_printing (val),
			      value_embedded_offset (val),
			      val,
			      &opts, 0, file);
}

/* Replacement for generic do_registers_info.
   Print regs in pretty columns.  */

static int
print_fp_register_row (struct ui_file *file, struct frame_info *frame,
		       int regnum)
{
  fprintf_filtered (file, " ");
  mips_print_fp_register (file, frame, regnum);
  fprintf_filtered (file, "\n");
  return regnum + 1;
}


/* Print a row's worth of GP (int) registers, with name labels above.  */

static int
print_gp_register_row (struct ui_file *file, struct frame_info *frame,
		       int start_regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  /* Do values for GP (int) regs.  */
  gdb_byte raw_buffer[MAX_REGISTER_SIZE];
  int ncols = (mips_abi_regsize (gdbarch) == 8 ? 4 : 8);    /* display cols
							       per row.  */
  int col, byte;
  int regnum;

  /* For GP registers, we print a separate row of names above the vals.  */
  for (col = 0, regnum = start_regnum;
       col < ncols && regnum < gdbarch_num_regs (gdbarch)
			       + gdbarch_num_pseudo_regs (gdbarch);
       regnum++)
    {
      if (*gdbarch_register_name (gdbarch, regnum) == '\0')
	continue;		/* unused register */
      if (TYPE_CODE (register_type (gdbarch, regnum)) ==
	  TYPE_CODE_FLT)
	break;			/* End the row: reached FP register.  */
      /* Large registers are handled separately.  */
      if (register_size (gdbarch, regnum) > mips_abi_regsize (gdbarch))
	{
	  if (col > 0)
	    break;		/* End the row before this register.  */

	  /* Print this register on a row by itself.  */
	  mips_print_register (file, frame, regnum);
	  fprintf_filtered (file, "\n");
	  return regnum + 1;
	}
      if (col == 0)
	fprintf_filtered (file, "     ");
      fprintf_filtered (file,
			mips_abi_regsize (gdbarch) == 8 ? "%17s" : "%9s",
			gdbarch_register_name (gdbarch, regnum));
      col++;
    }

  if (col == 0)
    return regnum;

  /* Print the R0 to R31 names.  */
  if ((start_regnum % gdbarch_num_regs (gdbarch)) < MIPS_NUMREGS)
    fprintf_filtered (file, "\n R%-4d",
		      start_regnum % gdbarch_num_regs (gdbarch));
  else
    fprintf_filtered (file, "\n      ");

  /* Now print the values in hex, 4 or 8 to the row.  */
  for (col = 0, regnum = start_regnum;
       col < ncols && regnum < gdbarch_num_regs (gdbarch)
			       + gdbarch_num_pseudo_regs (gdbarch);
       regnum++)
    {
      if (*gdbarch_register_name (gdbarch, regnum) == '\0')
	continue;		/* unused register */
      if (TYPE_CODE (register_type (gdbarch, regnum)) ==
	  TYPE_CODE_FLT)
	break;			/* End row: reached FP register.  */
      if (register_size (gdbarch, regnum) > mips_abi_regsize (gdbarch))
	break;			/* End row: large register.  */

      /* OK: get the data in raw format.  */
      if (!frame_register_read (frame, regnum, raw_buffer))
	error (_("can't read register %d (%s)"),
	       regnum, gdbarch_register_name (gdbarch, regnum));
      /* pad small registers */
      for (byte = 0;
	   byte < (mips_abi_regsize (gdbarch)
		   - register_size (gdbarch, regnum)); byte++)
	printf_filtered ("  ");
      /* Now print the register value in hex, endian order.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	for (byte =
	     register_size (gdbarch, regnum) - register_size (gdbarch, regnum);
	     byte < register_size (gdbarch, regnum); byte++)
	  fprintf_filtered (file, "%02x", raw_buffer[byte]);
      else
	for (byte = register_size (gdbarch, regnum) - 1;
	     byte >= 0; byte--)
	  fprintf_filtered (file, "%02x", raw_buffer[byte]);
      fprintf_filtered (file, " ");
      col++;
    }
  if (col > 0)			/* ie. if we actually printed anything...  */
    fprintf_filtered (file, "\n");

  return regnum;
}

/* MIPS_DO_REGISTERS_INFO(): called by "info register" command.  */

static void
mips_print_registers_info (struct gdbarch *gdbarch, struct ui_file *file,
			   struct frame_info *frame, int regnum, int all)
{
  if (regnum != -1)		/* Do one specified register.  */
    {
      gdb_assert (regnum >= gdbarch_num_regs (gdbarch));
      if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	error (_("Not a valid register for the current processor type"));

      mips_print_register (file, frame, regnum);
      fprintf_filtered (file, "\n");
    }
  else
    /* Do all (or most) registers.  */
    {
      regnum = gdbarch_num_regs (gdbarch);
      while (regnum < gdbarch_num_regs (gdbarch)
		      + gdbarch_num_pseudo_regs (gdbarch))
	{
	  if (TYPE_CODE (register_type (gdbarch, regnum)) ==
	      TYPE_CODE_FLT)
	    {
	      if (all)		/* True for "INFO ALL-REGISTERS" command.  */
		regnum = print_fp_register_row (file, frame, regnum);
	      else
		regnum += MIPS_NUMREGS;	/* Skip floating point regs.  */
	    }
	  else
	    regnum = print_gp_register_row (file, frame, regnum);
	}
    }
}

/* Is this a branch with a delay slot?  */

static int
is_delayed (unsigned long insn)
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

static int
mips_single_step_through_delay (struct gdbarch *gdbarch,
				struct frame_info *frame)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc = get_frame_pc (frame);
  gdb_byte buf[MIPS_INSN32_SIZE];

  /* There is no branch delay slot on MIPS16.  */
  if (mips_pc_is_mips16 (pc))
    return 0;

  if (!breakpoint_here_p (get_frame_address_space (frame), pc + 4))
    return 0;

  if (!safe_frame_unwind_memory (frame, pc, buf, sizeof buf))
    /* If error reading memory, guess that it is not a delayed
       branch.  */
    return 0;
  return is_delayed (extract_unsigned_integer (buf, sizeof buf, byte_order));
}

/* To skip prologues, I use this predicate.  Returns either PC itself
   if the code at PC does not look like a function prologue; otherwise
   returns an address that (if we're lucky) follows the prologue.  If
   LENIENT, then we must skip everything which is involved in setting
   up the frame (it's OK to skip more, just so long as we don't skip
   anything which might clobber the registers which are being saved.
   We must skip more in the case where part of the prologue is in the
   delay slot of a non-prologue instruction).  */

static CORE_ADDR
mips_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR limit_pc;
  CORE_ADDR func_addr;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 100;          /* Magic.  */

  if (mips_pc_is_mips16 (pc))
    return mips16_scan_prologue (gdbarch, pc, limit_pc, NULL, NULL);
  else
    return mips32_scan_prologue (gdbarch, pc, limit_pc, NULL, NULL);
}

/* Check whether the PC is in a function epilogue (32-bit version).
   This is a helper function for mips_in_function_epilogue_p.  */
static int
mips32_in_function_epilogue_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0, func_end = 0;

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      /* The MIPS epilogue is max. 12 bytes long.  */
      CORE_ADDR addr = func_end - 12;

      if (addr < func_addr + 4)
        addr = func_addr + 4;
      if (pc < addr)
        return 0;

      for (; pc < func_end; pc += MIPS_INSN32_SIZE)
	{
	  unsigned long high_word;
	  unsigned long inst;

	  inst = mips_fetch_instruction (gdbarch, pc);
	  high_word = (inst >> 16) & 0xffff;

	  if (high_word != 0x27bd	/* addiu $sp,$sp,offset */
	      && high_word != 0x67bd	/* daddiu $sp,$sp,offset */
	      && inst != 0x03e00008	/* jr $ra */
	      && inst != 0x00000000)	/* nop */
	    return 0;
	}

      return 1;
    }

  return 0;
}

/* Check whether the PC is in a function epilogue (16-bit version).
   This is a helper function for mips_in_function_epilogue_p.  */
static int
mips16_in_function_epilogue_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0, func_end = 0;

  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      /* The MIPS epilogue is max. 12 bytes long.  */
      CORE_ADDR addr = func_end - 12;

      if (addr < func_addr + 4)
        addr = func_addr + 4;
      if (pc < addr)
        return 0;

      for (; pc < func_end; pc += MIPS_INSN16_SIZE)
	{
	  unsigned short inst;

	  inst = mips_fetch_instruction (gdbarch, pc);

	  if ((inst & 0xf800) == 0xf000)	/* extend */
	    continue;

	  if (inst != 0x6300		/* addiu $sp,offset */
	      && inst != 0xfb00		/* daddiu $sp,$sp,offset */
	      && inst != 0xe820		/* jr $ra */
	      && inst != 0xe8a0		/* jrc $ra */
	      && inst != 0x6500)	/* nop */
	    return 0;
	}

      return 1;
    }

  return 0;
}

/* The epilogue is defined here as the area at the end of a function,
   after an instruction which destroys the function's stack frame.  */
static int
mips_in_function_epilogue_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  if (mips_pc_is_mips16 (pc))
    return mips16_in_function_epilogue_p (gdbarch, pc);
  else
    return mips32_in_function_epilogue_p (gdbarch, pc);
}

/* Root of all "set mips "/"show mips " commands.  This will eventually be
   used for all MIPS-specific commands.  */

static void
show_mips_command (char *args, int from_tty)
{
  help_list (showmipscmdlist, "show mips ", all_commands, gdb_stdout);
}

static void
set_mips_command (char *args, int from_tty)
{
  printf_unfiltered
    ("\"set mips\" must be followed by an appropriate subcommand.\n");
  help_list (setmipscmdlist, "set mips ", all_commands, gdb_stdout);
}

/* Commands to show/set the MIPS FPU type.  */

static void
show_mipsfpu_command (char *args, int from_tty)
{
  char *fpu;

  if (gdbarch_bfd_arch_info (target_gdbarch)->arch != bfd_arch_mips)
    {
      printf_unfiltered
	("The MIPS floating-point coprocessor is unknown "
	 "because the current architecture is not MIPS.\n");
      return;
    }

  switch (MIPS_FPU_TYPE (target_gdbarch))
    {
    case MIPS_FPU_SINGLE:
      fpu = "single-precision";
      break;
    case MIPS_FPU_DOUBLE:
      fpu = "double-precision";
      break;
    case MIPS_FPU_NONE:
      fpu = "absent (none)";
      break;
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }
  if (mips_fpu_type_auto)
    printf_unfiltered ("The MIPS floating-point coprocessor "
		       "is set automatically (currently %s)\n",
		       fpu);
  else
    printf_unfiltered
      ("The MIPS floating-point coprocessor is assumed to be %s\n", fpu);
}


static void
set_mipsfpu_command (char *args, int from_tty)
{
  printf_unfiltered ("\"set mipsfpu\" must be followed by \"double\", "
		     "\"single\",\"none\" or \"auto\".\n");
  show_mipsfpu_command (args, from_tty);
}

static void
set_mipsfpu_single_command (char *args, int from_tty)
{
  struct gdbarch_info info;
  gdbarch_info_init (&info);
  mips_fpu_type = MIPS_FPU_SINGLE;
  mips_fpu_type_auto = 0;
  /* FIXME: cagney/2003-11-15: Should be setting a field in "info"
     instead of relying on globals.  Doing that would let generic code
     handle the search for this specific architecture.  */
  if (!gdbarch_update_p (info))
    internal_error (__FILE__, __LINE__, _("set mipsfpu failed"));
}

static void
set_mipsfpu_double_command (char *args, int from_tty)
{
  struct gdbarch_info info;
  gdbarch_info_init (&info);
  mips_fpu_type = MIPS_FPU_DOUBLE;
  mips_fpu_type_auto = 0;
  /* FIXME: cagney/2003-11-15: Should be setting a field in "info"
     instead of relying on globals.  Doing that would let generic code
     handle the search for this specific architecture.  */
  if (!gdbarch_update_p (info))
    internal_error (__FILE__, __LINE__, _("set mipsfpu failed"));
}

static void
set_mipsfpu_none_command (char *args, int from_tty)
{
  struct gdbarch_info info;
  gdbarch_info_init (&info);
  mips_fpu_type = MIPS_FPU_NONE;
  mips_fpu_type_auto = 0;
  /* FIXME: cagney/2003-11-15: Should be setting a field in "info"
     instead of relying on globals.  Doing that would let generic code
     handle the search for this specific architecture.  */
  if (!gdbarch_update_p (info))
    internal_error (__FILE__, __LINE__, _("set mipsfpu failed"));
}

static void
set_mipsfpu_auto_command (char *args, int from_tty)
{
  mips_fpu_type_auto = 1;
}

/* Attempt to identify the particular processor model by reading the
   processor id.  NOTE: cagney/2003-11-15: Firstly it isn't clear that
   the relevant processor still exists (it dates back to '94) and
   secondly this is not the way to do this.  The processor type should
   be set by forcing an architecture change.  */

void
deprecated_mips_set_processor_regs_hack (void)
{
  struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  ULONGEST prid;

  regcache_cooked_read_unsigned (regcache, MIPS_PRID_REGNUM, &prid);
  if ((prid & ~0xf) == 0x700)
    tdep->mips_processor_reg_names = mips_r3041_reg_names;
}

/* Just like reinit_frame_cache, but with the right arguments to be
   callable as an sfunc.  */

static void
reinit_frame_cache_sfunc (char *args, int from_tty,
			  struct cmd_list_element *c)
{
  reinit_frame_cache ();
}

static int
gdb_print_insn_mips (bfd_vma memaddr, struct disassemble_info *info)
{
  /* FIXME: cagney/2003-06-26: Is this even necessary?  The
     disassembler needs to be able to locally determine the ISA, and
     not rely on GDB.  Otherwize the stand-alone 'objdump -d' will not
     work.  */
  if (mips_pc_is_mips16 (memaddr))
    info->mach = bfd_mach_mips16;

  /* Round down the instruction address to the appropriate boundary.  */
  memaddr &= (info->mach == bfd_mach_mips16 ? ~1 : ~3);

  /* Set the disassembler options.  */
  if (!info->disassembler_options)
    /* This string is not recognized explicitly by the disassembler,
       but it tells the disassembler to not try to guess the ABI from
       the bfd elf headers, such that, if the user overrides the ABI
       of a program linked as NewABI, the disassembly will follow the
       register naming conventions specified by the user.  */
    info->disassembler_options = "gpr-names=32";

  /* Call the appropriate disassembler based on the target endian-ness.  */
  if (info->endian == BFD_ENDIAN_BIG)
    return print_insn_big_mips (memaddr, info);
  else
    return print_insn_little_mips (memaddr, info);
}

static int
gdb_print_insn_mips_n32 (bfd_vma memaddr, struct disassemble_info *info)
{
  /* Set up the disassembler info, so that we get the right
     register names from libopcodes.  */
  info->disassembler_options = "gpr-names=n32";
  info->flavour = bfd_target_elf_flavour;

  return gdb_print_insn_mips (memaddr, info);
}

static int
gdb_print_insn_mips_n64 (bfd_vma memaddr, struct disassemble_info *info)
{
  /* Set up the disassembler info, so that we get the right
     register names from libopcodes.  */
  info->disassembler_options = "gpr-names=64";
  info->flavour = bfd_target_elf_flavour;

  return gdb_print_insn_mips (memaddr, info);
}

/* This function implements gdbarch_breakpoint_from_pc.  It uses the
   program counter value to determine whether a 16- or 32-bit breakpoint
   should be used.  It returns a pointer to a string of bytes that encode a
   breakpoint instruction, stores the length of the string to *lenptr, and
   adjusts pc (if necessary) to point to the actual memory location where
   the breakpoint should be inserted.  */

static const gdb_byte *
mips_breakpoint_from_pc (struct gdbarch *gdbarch,
			 CORE_ADDR *pcptr, int *lenptr)
{
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    {
      if (mips_pc_is_mips16 (*pcptr))
	{
	  static gdb_byte mips16_big_breakpoint[] = { 0xe8, 0xa5 };
	  *pcptr = unmake_mips16_addr (*pcptr);
	  *lenptr = sizeof (mips16_big_breakpoint);
	  return mips16_big_breakpoint;
	}
      else
	{
	  /* The IDT board uses an unusual breakpoint value, and
	     sometimes gets confused when it sees the usual MIPS
	     breakpoint instruction.  */
	  static gdb_byte big_breakpoint[] = { 0, 0x5, 0, 0xd };
	  static gdb_byte pmon_big_breakpoint[] = { 0, 0, 0, 0xd };
	  static gdb_byte idt_big_breakpoint[] = { 0, 0, 0x0a, 0xd };
	  /* Likewise, IRIX appears to expect a different breakpoint,
	     although this is not apparent until you try to use pthreads.  */
	  static gdb_byte irix_big_breakpoint[] = { 0, 0, 0, 0xd };

	  *lenptr = sizeof (big_breakpoint);

	  if (strcmp (target_shortname, "mips") == 0)
	    return idt_big_breakpoint;
	  else if (strcmp (target_shortname, "ddb") == 0
		   || strcmp (target_shortname, "pmon") == 0
		   || strcmp (target_shortname, "lsi") == 0)
	    return pmon_big_breakpoint;
	  else if (gdbarch_osabi (gdbarch) == GDB_OSABI_IRIX)
	    return irix_big_breakpoint;
	  else
	    return big_breakpoint;
	}
    }
  else
    {
      if (mips_pc_is_mips16 (*pcptr))
	{
	  static gdb_byte mips16_little_breakpoint[] = { 0xa5, 0xe8 };
	  *pcptr = unmake_mips16_addr (*pcptr);
	  *lenptr = sizeof (mips16_little_breakpoint);
	  return mips16_little_breakpoint;
	}
      else
	{
	  static gdb_byte little_breakpoint[] = { 0xd, 0, 0x5, 0 };
	  static gdb_byte pmon_little_breakpoint[] = { 0xd, 0, 0, 0 };
	  static gdb_byte idt_little_breakpoint[] = { 0xd, 0x0a, 0, 0 };

	  *lenptr = sizeof (little_breakpoint);

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

/* If PC is in a mips16 call or return stub, return the address of the target
   PC, which is either the callee or the caller.  There are several
   cases which must be handled:

   * If the PC is in __mips16_ret_{d,s}f, this is a return stub and the
   target PC is in $31 ($ra).
   * If the PC is in __mips16_call_stub_{1..10}, this is a call stub
   and the target PC is in $2.
   * If the PC at the start of __mips16_call_stub_{s,d}f_{0..10}, i.e.
   before the jal instruction, this is effectively a call stub
   and the target PC is in $2.  Otherwise this is effectively
   a return stub and the target PC is in $18.

   See the source code for the stubs in gcc/config/mips/mips16.S for
   gory details.  */

static CORE_ADDR
mips_skip_mips16_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  char *name;
  CORE_ADDR start_addr;

  /* Find the starting address and name of the function containing the PC.  */
  if (find_pc_partial_function (pc, &name, &start_addr, NULL) == 0)
    return 0;

  /* If the PC is in __mips16_ret_{d,s}f, this is a return stub and the
     target PC is in $31 ($ra).  */
  if (strcmp (name, "__mips16_ret_sf") == 0
      || strcmp (name, "__mips16_ret_df") == 0)
    return get_frame_register_signed (frame, MIPS_RA_REGNUM);

  if (strncmp (name, "__mips16_call_stub_", 19) == 0)
    {
      /* If the PC is in __mips16_call_stub_{1..10}, this is a call stub
         and the target PC is in $2.  */
      if (name[19] >= '0' && name[19] <= '9')
	return get_frame_register_signed (frame, 2);

      /* If the PC at the start of __mips16_call_stub_{s,d}f_{0..10}, i.e.
         before the jal instruction, this is effectively a call stub
         and the target PC is in $2.  Otherwise this is effectively
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

	      CORE_ADDR target_pc = get_frame_register_signed (frame, 2);
	      ULONGEST inst;
	      int i;

	      /* See if the name of the target function is  __fn_stub_*.  */
	      if (find_pc_partial_function (target_pc, &name, NULL, NULL) ==
		  0)
		return target_pc;
	      if (strncmp (name, "__fn_stub_", 10) != 0
		  && strcmp (name, "etext") != 0
		  && strcmp (name, "_etext") != 0)
		return target_pc;

	      /* Scan through this _fn_stub_ code for the lui/addiu pair.
	         The limit on the search is arbitrarily set to 20
	         instructions.  FIXME.  */
	      for (i = 0, pc = 0; i < 20; i++, target_pc += MIPS_INSN32_SIZE)
		{
		  inst = mips_fetch_instruction (gdbarch, target_pc);
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
	    return get_frame_register_signed (frame, 18);
	}
    }
  return 0;			/* not a stub */
}

/* If the current PC is the start of a non-PIC-to-PIC stub, return the
   PC of the stub target.  The stub just loads $t9 and jumps to it,
   so that $t9 has the correct value at function entry.  */

static CORE_ADDR
mips_skip_pic_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct minimal_symbol *msym;
  int i;
  gdb_byte stub_code[16];
  int32_t stub_words[4];

  /* The stub for foo is named ".pic.foo", and is either two
     instructions inserted before foo or a three instruction sequence
     which jumps to foo.  */
  msym = lookup_minimal_symbol_by_pc (pc);
  if (msym == NULL
      || SYMBOL_VALUE_ADDRESS (msym) != pc
      || SYMBOL_LINKAGE_NAME (msym) == NULL
      || strncmp (SYMBOL_LINKAGE_NAME (msym), ".pic.", 5) != 0)
    return 0;

  /* A two-instruction header.  */
  if (MSYMBOL_SIZE (msym) == 8)
    return pc + 8;

  /* A three-instruction (plus delay slot) trampoline.  */
  if (MSYMBOL_SIZE (msym) == 16)
    {
      if (target_read_memory (pc, stub_code, 16) != 0)
	return 0;
      for (i = 0; i < 4; i++)
	stub_words[i] = extract_unsigned_integer (stub_code + i * 4,
						  4, byte_order);

      /* A stub contains these instructions:
	 lui	t9, %hi(target)
	 j	target
	  addiu	t9, t9, %lo(target)
	 nop

	 This works even for N64, since stubs are only generated with
	 -msym32.  */
      if ((stub_words[0] & 0xffff0000U) == 0x3c190000
	  && (stub_words[1] & 0xfc000000U) == 0x08000000
	  && (stub_words[2] & 0xffff0000U) == 0x27390000
	  && stub_words[3] == 0x00000000)
	return (((stub_words[0] & 0x0000ffff) << 16)
		+ (stub_words[2] & 0x0000ffff));
    }

  /* Not a recognized stub.  */
  return 0;
}

static CORE_ADDR
mips_skip_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  CORE_ADDR target_pc;

  target_pc = mips_skip_mips16_trampoline_code (frame, pc);
  if (target_pc)
    return target_pc;

  target_pc = find_solib_trampoline_target (frame, pc);
  if (target_pc)
    return target_pc;

  target_pc = mips_skip_pic_trampoline_code (frame, pc);
  if (target_pc)
    return target_pc;

  return 0;
}

/* Convert a dbx stab register number (from `r' declaration) to a GDB
   [1 * gdbarch_num_regs .. 2 * gdbarch_num_regs) REGNUM.  */

static int
mips_stab_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  int regnum;
  if (num >= 0 && num < 32)
    regnum = num;
  else if (num >= 38 && num < 70)
    regnum = num + mips_regnum (gdbarch)->fp0 - 38;
  else if (num == 70)
    regnum = mips_regnum (gdbarch)->hi;
  else if (num == 71)
    regnum = mips_regnum (gdbarch)->lo;
  else
    /* This will hopefully (eventually) provoke a warning.  Should
       we be calling complaint() here?  */
    return gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);
  return gdbarch_num_regs (gdbarch) + regnum;
}


/* Convert a dwarf, dwarf2, or ecoff register number to a GDB [1 *
   gdbarch_num_regs .. 2 * gdbarch_num_regs) REGNUM.  */

static int
mips_dwarf_dwarf2_ecoff_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  int regnum;
  if (num >= 0 && num < 32)
    regnum = num;
  else if (num >= 32 && num < 64)
    regnum = num + mips_regnum (gdbarch)->fp0 - 32;
  else if (num == 64)
    regnum = mips_regnum (gdbarch)->hi;
  else if (num == 65)
    regnum = mips_regnum (gdbarch)->lo;
  else
    /* This will hopefully (eventually) provoke a warning.  Should we
       be calling complaint() here?  */
    return gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);
  return gdbarch_num_regs (gdbarch) + regnum;
}

static int
mips_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  /* Only makes sense to supply raw registers.  */
  gdb_assert (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch));
  /* FIXME: cagney/2002-05-13: Need to look at the pseudo register to
     decide if it is valid.  Should instead define a standard sim/gdb
     register numbering scheme.  */
  if (gdbarch_register_name (gdbarch,
			     gdbarch_num_regs (gdbarch) + regnum) != NULL
      && gdbarch_register_name (gdbarch,
			        gdbarch_num_regs (gdbarch)
				+ regnum)[0] != '\0')
    return regnum;
  else
    return LEGACY_SIM_REGNO_IGNORE;
}


/* Convert an integer into an address.  Extracting the value signed
   guarantees a correctly sign extended address.  */

static CORE_ADDR
mips_integer_to_address (struct gdbarch *gdbarch,
			 struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  return extract_signed_integer (buf, TYPE_LENGTH (type), byte_order);
}

/* Dummy virtual frame pointer method.  This is no more or less accurate
   than most other architectures; we just need to be explicit about it,
   because the pseudo-register gdbarch_sp_regnum will otherwise lead to
   an assertion failure.  */

static void
mips_virtual_frame_pointer (struct gdbarch *gdbarch, 
			    CORE_ADDR pc, int *reg, LONGEST *offset)
{
  *reg = MIPS_SP_REGNUM;
  *offset = 0;
}

static void
mips_find_abi_section (bfd *abfd, asection *sect, void *obj)
{
  enum mips_abi *abip = (enum mips_abi *) obj;
  const char *name = bfd_get_section_name (abfd, sect);

  if (*abip != MIPS_ABI_UNKNOWN)
    return;

  if (strncmp (name, ".mdebug.", 8) != 0)
    return;

  if (strcmp (name, ".mdebug.abi32") == 0)
    *abip = MIPS_ABI_O32;
  else if (strcmp (name, ".mdebug.abiN32") == 0)
    *abip = MIPS_ABI_N32;
  else if (strcmp (name, ".mdebug.abi64") == 0)
    *abip = MIPS_ABI_N64;
  else if (strcmp (name, ".mdebug.abiO64") == 0)
    *abip = MIPS_ABI_O64;
  else if (strcmp (name, ".mdebug.eabi32") == 0)
    *abip = MIPS_ABI_EABI32;
  else if (strcmp (name, ".mdebug.eabi64") == 0)
    *abip = MIPS_ABI_EABI64;
  else
    warning (_("unsupported ABI %s."), name + 8);
}

static void
mips_find_long_section (bfd *abfd, asection *sect, void *obj)
{
  int *lbp = (int *) obj;
  const char *name = bfd_get_section_name (abfd, sect);

  if (strncmp (name, ".gcc_compiled_long32", 20) == 0)
    *lbp = 32;
  else if (strncmp (name, ".gcc_compiled_long64", 20) == 0)
    *lbp = 64;
  else if (strncmp (name, ".gcc_compiled_long", 18) == 0)
    warning (_("unrecognized .gcc_compiled_longXX"));
}

static enum mips_abi
global_mips_abi (void)
{
  int i;

  for (i = 0; mips_abi_strings[i] != NULL; i++)
    if (mips_abi_strings[i] == mips_abi_string)
      return (enum mips_abi) i;

  internal_error (__FILE__, __LINE__, _("unknown ABI string"));
}

static void
mips_register_g_packet_guesses (struct gdbarch *gdbarch)
{
  /* If the size matches the set of 32-bit or 64-bit integer registers,
     assume that's what we've got.  */
  register_remote_g_packet_guess (gdbarch, 38 * 4, mips_tdesc_gp32);
  register_remote_g_packet_guess (gdbarch, 38 * 8, mips_tdesc_gp64);

  /* If the size matches the full set of registers GDB traditionally
     knows about, including floating point, for either 32-bit or
     64-bit, assume that's what we've got.  */
  register_remote_g_packet_guess (gdbarch, 90 * 4, mips_tdesc_gp32);
  register_remote_g_packet_guess (gdbarch, 90 * 8, mips_tdesc_gp64);

  /* Otherwise we don't have a useful guess.  */
}

static struct value *
value_of_mips_user_reg (struct frame_info *frame, const void *baton)
{
  const int *reg_p = baton;
  return value_of_register (*reg_p, frame);
}

static struct gdbarch *
mips_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  int elf_flags;
  enum mips_abi mips_abi, found_abi, wanted_abi;
  int i, num_regs;
  enum mips_fpu_type fpu_type;
  struct tdesc_arch_data *tdesc_data = NULL;
  int elf_fpu_type = 0;

  /* Check any target description for validity.  */
  if (tdesc_has_registers (info.target_desc))
    {
      static const char *const mips_gprs[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
      };
      static const char *const mips_fprs[] = {
	"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
	"f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
	"f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
	"f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
      };

      const struct tdesc_feature *feature;
      int valid_p;

      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.mips.cpu");
      if (feature == NULL)
	return NULL;

      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      for (i = MIPS_ZERO_REGNUM; i <= MIPS_RA_REGNUM; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data, i,
					    mips_gprs[i]);


      valid_p &= tdesc_numbered_register (feature, tdesc_data,
					  MIPS_EMBED_LO_REGNUM, "lo");
      valid_p &= tdesc_numbered_register (feature, tdesc_data,
					  MIPS_EMBED_HI_REGNUM, "hi");
      valid_p &= tdesc_numbered_register (feature, tdesc_data,
					  MIPS_EMBED_PC_REGNUM, "pc");

      if (!valid_p)
	{
	  tdesc_data_cleanup (tdesc_data);
	  return NULL;
	}

      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.mips.cp0");
      if (feature == NULL)
	{
	  tdesc_data_cleanup (tdesc_data);
	  return NULL;
	}

      valid_p = 1;
      valid_p &= tdesc_numbered_register (feature, tdesc_data,
					  MIPS_EMBED_BADVADDR_REGNUM,
					  "badvaddr");
      valid_p &= tdesc_numbered_register (feature, tdesc_data,
					  MIPS_PS_REGNUM, "status");
      valid_p &= tdesc_numbered_register (feature, tdesc_data,
					  MIPS_EMBED_CAUSE_REGNUM, "cause");

      if (!valid_p)
	{
	  tdesc_data_cleanup (tdesc_data);
	  return NULL;
	}

      /* FIXME drow/2007-05-17: The FPU should be optional.  The MIPS
	 backend is not prepared for that, though.  */
      feature = tdesc_find_feature (info.target_desc,
				    "org.gnu.gdb.mips.fpu");
      if (feature == NULL)
	{
	  tdesc_data_cleanup (tdesc_data);
	  return NULL;
	}

      valid_p = 1;
      for (i = 0; i < 32; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data,
					    i + MIPS_EMBED_FP0_REGNUM,
					    mips_fprs[i]);

      valid_p &= tdesc_numbered_register (feature, tdesc_data,
					  MIPS_EMBED_FP0_REGNUM + 32, "fcsr");
      valid_p &= tdesc_numbered_register (feature, tdesc_data,
					  MIPS_EMBED_FP0_REGNUM + 33, "fir");

      if (!valid_p)
	{
	  tdesc_data_cleanup (tdesc_data);
	  return NULL;
	}

      /* It would be nice to detect an attempt to use a 64-bit ABI
	 when only 32-bit registers are provided.  */
    }

  /* First of all, extract the elf_flags, if available.  */
  if (info.abfd && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_flags = elf_elfheader (info.abfd)->e_flags;
  else if (arches != NULL)
    elf_flags = gdbarch_tdep (arches->gdbarch)->elf_flags;
  else
    elf_flags = 0;
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog,
			"mips_gdbarch_init: elf_flags = 0x%08x\n", elf_flags);

  /* Check ELF_FLAGS to see if it specifies the ABI being used.  */
  switch ((elf_flags & EF_MIPS_ABI))
    {
    case E_MIPS_ABI_O32:
      found_abi = MIPS_ABI_O32;
      break;
    case E_MIPS_ABI_O64:
      found_abi = MIPS_ABI_O64;
      break;
    case E_MIPS_ABI_EABI32:
      found_abi = MIPS_ABI_EABI32;
      break;
    case E_MIPS_ABI_EABI64:
      found_abi = MIPS_ABI_EABI64;
      break;
    default:
      if ((elf_flags & EF_MIPS_ABI2))
	found_abi = MIPS_ABI_N32;
      else
	found_abi = MIPS_ABI_UNKNOWN;
      break;
    }

  /* GCC creates a pseudo-section whose name describes the ABI.  */
  if (found_abi == MIPS_ABI_UNKNOWN && info.abfd != NULL)
    bfd_map_over_sections (info.abfd, mips_find_abi_section, &found_abi);

  /* If we have no useful BFD information, use the ABI from the last
     MIPS architecture (if there is one).  */
  if (found_abi == MIPS_ABI_UNKNOWN && info.abfd == NULL && arches != NULL)
    found_abi = gdbarch_tdep (arches->gdbarch)->found_abi;

  /* Try the architecture for any hint of the correct ABI.  */
  if (found_abi == MIPS_ABI_UNKNOWN
      && info.bfd_arch_info != NULL
      && info.bfd_arch_info->arch == bfd_arch_mips)
    {
      switch (info.bfd_arch_info->mach)
	{
	case bfd_mach_mips3900:
	  found_abi = MIPS_ABI_EABI32;
	  break;
	case bfd_mach_mips4100:
	case bfd_mach_mips5000:
	  found_abi = MIPS_ABI_EABI64;
	  break;
	case bfd_mach_mips8000:
	case bfd_mach_mips10000:
	  /* On Irix, ELF64 executables use the N64 ABI.  The
	     pseudo-sections which describe the ABI aren't present
	     on IRIX.  (Even for executables created by gcc.)  */
	  if (bfd_get_flavour (info.abfd) == bfd_target_elf_flavour
	      && elf_elfheader (info.abfd)->e_ident[EI_CLASS] == ELFCLASS64)
	    found_abi = MIPS_ABI_N64;
	  else
	    found_abi = MIPS_ABI_N32;
	  break;
	}
    }

  /* Default 64-bit objects to N64 instead of O32.  */
  if (found_abi == MIPS_ABI_UNKNOWN
      && info.abfd != NULL
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour
      && elf_elfheader (info.abfd)->e_ident[EI_CLASS] == ELFCLASS64)
    found_abi = MIPS_ABI_N64;

  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "mips_gdbarch_init: found_abi = %d\n",
			found_abi);

  /* What has the user specified from the command line?  */
  wanted_abi = global_mips_abi ();
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "mips_gdbarch_init: wanted_abi = %d\n",
			wanted_abi);

  /* Now that we have found what the ABI for this binary would be,
     check whether the user is overriding it.  */
  if (wanted_abi != MIPS_ABI_UNKNOWN)
    mips_abi = wanted_abi;
  else if (found_abi != MIPS_ABI_UNKNOWN)
    mips_abi = found_abi;
  else
    mips_abi = MIPS_ABI_O32;
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog, "mips_gdbarch_init: mips_abi = %d\n",
			mips_abi);

  /* Also used when doing an architecture lookup.  */
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog,
			"mips_gdbarch_init: "
			"mips64_transfers_32bit_regs_p = %d\n",
			mips64_transfers_32bit_regs_p);

  /* Determine the MIPS FPU type.  */
#ifdef HAVE_ELF
  if (info.abfd
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_fpu_type = bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_GNU,
					     Tag_GNU_MIPS_ABI_FP);
#endif /* HAVE_ELF */

  if (!mips_fpu_type_auto)
    fpu_type = mips_fpu_type;
  else if (elf_fpu_type != 0)
    {
      switch (elf_fpu_type)
	{
	case 1:
	  fpu_type = MIPS_FPU_DOUBLE;
	  break;
	case 2:
	  fpu_type = MIPS_FPU_SINGLE;
	  break;
	case 3:
	default:
	  /* Soft float or unknown.  */
	  fpu_type = MIPS_FPU_NONE;
	  break;
	}
    }
  else if (info.bfd_arch_info != NULL
	   && info.bfd_arch_info->arch == bfd_arch_mips)
    switch (info.bfd_arch_info->mach)
      {
      case bfd_mach_mips3900:
      case bfd_mach_mips4100:
      case bfd_mach_mips4111:
      case bfd_mach_mips4120:
	fpu_type = MIPS_FPU_NONE;
	break;
      case bfd_mach_mips4650:
	fpu_type = MIPS_FPU_SINGLE;
	break;
      default:
	fpu_type = MIPS_FPU_DOUBLE;
	break;
      }
  else if (arches != NULL)
    fpu_type = gdbarch_tdep (arches->gdbarch)->mips_fpu_type;
  else
    fpu_type = MIPS_FPU_DOUBLE;
  if (gdbarch_debug)
    fprintf_unfiltered (gdb_stdlog,
			"mips_gdbarch_init: fpu_type = %d\n", fpu_type);

  /* Check for blatant incompatibilities.  */

  /* If we have only 32-bit registers, then we can't debug a 64-bit
     ABI.  */
  if (info.target_desc
      && tdesc_property (info.target_desc, PROPERTY_GP32) != NULL
      && mips_abi != MIPS_ABI_EABI32
      && mips_abi != MIPS_ABI_O32)
    {
      if (tdesc_data != NULL)
	tdesc_data_cleanup (tdesc_data);
      return NULL;
    }

  /* Try to find a pre-existing architecture.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      /* MIPS needs to be pedantic about which ABI the object is
         using.  */
      if (gdbarch_tdep (arches->gdbarch)->elf_flags != elf_flags)
	continue;
      if (gdbarch_tdep (arches->gdbarch)->mips_abi != mips_abi)
	continue;
      /* Need to be pedantic about which register virtual size is
         used.  */
      if (gdbarch_tdep (arches->gdbarch)->mips64_transfers_32bit_regs_p
	  != mips64_transfers_32bit_regs_p)
	continue;
      /* Be pedantic about which FPU is selected.  */
      if (gdbarch_tdep (arches->gdbarch)->mips_fpu_type != fpu_type)
	continue;

      if (tdesc_data != NULL)
	tdesc_data_cleanup (tdesc_data);
      return arches->gdbarch;
    }

  /* Need a new architecture.  Fill in a target specific vector.  */
  tdep = (struct gdbarch_tdep *) xmalloc (sizeof (struct gdbarch_tdep));
  gdbarch = gdbarch_alloc (&info, tdep);
  tdep->elf_flags = elf_flags;
  tdep->mips64_transfers_32bit_regs_p = mips64_transfers_32bit_regs_p;
  tdep->found_abi = found_abi;
  tdep->mips_abi = mips_abi;
  tdep->mips_fpu_type = fpu_type;
  tdep->register_size_valid_p = 0;
  tdep->register_size = 0;
  tdep->gregset = NULL;
  tdep->gregset64 = NULL;
  tdep->fpregset = NULL;
  tdep->fpregset64 = NULL;

  if (info.target_desc)
    {
      /* Some useful properties can be inferred from the target.  */
      if (tdesc_property (info.target_desc, PROPERTY_GP32) != NULL)
	{
	  tdep->register_size_valid_p = 1;
	  tdep->register_size = 4;
	}
      else if (tdesc_property (info.target_desc, PROPERTY_GP64) != NULL)
	{
	  tdep->register_size_valid_p = 1;
	  tdep->register_size = 8;
	}
    }

  /* Initially set everything according to the default ABI/ISA.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_register_reggroup_p (gdbarch, mips_register_reggroup_p);
  set_gdbarch_pseudo_register_read (gdbarch, mips_pseudo_register_read);
  set_gdbarch_pseudo_register_write (gdbarch, mips_pseudo_register_write);

  set_gdbarch_ax_pseudo_register_collect (gdbarch,
					  mips_ax_pseudo_register_collect);
  set_gdbarch_ax_pseudo_register_push_stack
      (gdbarch, mips_ax_pseudo_register_push_stack);

  set_gdbarch_elf_make_msymbol_special (gdbarch,
					mips_elf_make_msymbol_special);

  /* Fill in the OS dependant register numbers and names.  */
  {
    const char **reg_names;
    struct mips_regnum *regnum = GDBARCH_OBSTACK_ZALLOC (gdbarch,
							 struct mips_regnum);
    if (tdesc_has_registers (info.target_desc))
      {
	regnum->lo = MIPS_EMBED_LO_REGNUM;
	regnum->hi = MIPS_EMBED_HI_REGNUM;
	regnum->badvaddr = MIPS_EMBED_BADVADDR_REGNUM;
	regnum->cause = MIPS_EMBED_CAUSE_REGNUM;
	regnum->pc = MIPS_EMBED_PC_REGNUM;
	regnum->fp0 = MIPS_EMBED_FP0_REGNUM;
	regnum->fp_control_status = 70;
	regnum->fp_implementation_revision = 71;
	num_regs = MIPS_LAST_EMBED_REGNUM + 1;
	reg_names = NULL;
      }
    else if (info.osabi == GDB_OSABI_IRIX)
      {
	regnum->fp0 = 32;
	regnum->pc = 64;
	regnum->cause = 65;
	regnum->badvaddr = 66;
	regnum->hi = 67;
	regnum->lo = 68;
	regnum->fp_control_status = 69;
	regnum->fp_implementation_revision = 70;
	num_regs = 71;
	reg_names = mips_irix_reg_names;
      }
    else
      {
	regnum->lo = MIPS_EMBED_LO_REGNUM;
	regnum->hi = MIPS_EMBED_HI_REGNUM;
	regnum->badvaddr = MIPS_EMBED_BADVADDR_REGNUM;
	regnum->cause = MIPS_EMBED_CAUSE_REGNUM;
	regnum->pc = MIPS_EMBED_PC_REGNUM;
	regnum->fp0 = MIPS_EMBED_FP0_REGNUM;
	regnum->fp_control_status = 70;
	regnum->fp_implementation_revision = 71;
	num_regs = 90;
	if (info.bfd_arch_info != NULL
	    && info.bfd_arch_info->mach == bfd_mach_mips3900)
	  reg_names = mips_tx39_reg_names;
	else
	  reg_names = mips_generic_reg_names;
      }
    /* FIXME: cagney/2003-11-15: For MIPS, hasn't gdbarch_pc_regnum been
       replaced by gdbarch_read_pc?  */
    set_gdbarch_pc_regnum (gdbarch, regnum->pc + num_regs);
    set_gdbarch_sp_regnum (gdbarch, MIPS_SP_REGNUM + num_regs);
    set_gdbarch_fp0_regnum (gdbarch, regnum->fp0);
    set_gdbarch_num_regs (gdbarch, num_regs);
    set_gdbarch_num_pseudo_regs (gdbarch, num_regs);
    set_gdbarch_register_name (gdbarch, mips_register_name);
    set_gdbarch_virtual_frame_pointer (gdbarch, mips_virtual_frame_pointer);
    tdep->mips_processor_reg_names = reg_names;
    tdep->regnum = regnum;
  }

  switch (mips_abi)
    {
    case MIPS_ABI_O32:
      set_gdbarch_push_dummy_call (gdbarch, mips_o32_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_o32_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 4 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 4 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      break;
    case MIPS_ABI_O64:
      set_gdbarch_push_dummy_call (gdbarch, mips_o64_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_o64_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 4 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 4 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      break;
    case MIPS_ABI_EABI32:
      set_gdbarch_push_dummy_call (gdbarch, mips_eabi_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_eabi_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 8 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      break;
    case MIPS_ABI_EABI64:
      set_gdbarch_push_dummy_call (gdbarch, mips_eabi_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_eabi_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 8 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 64);
      set_gdbarch_ptr_bit (gdbarch, 64);
      set_gdbarch_long_long_bit (gdbarch, 64);
      break;
    case MIPS_ABI_N32:
      set_gdbarch_push_dummy_call (gdbarch, mips_n32n64_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_n32n64_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 8 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_long_double_bit (gdbarch, 128);
      set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);
      break;
    case MIPS_ABI_N64:
      set_gdbarch_push_dummy_call (gdbarch, mips_n32n64_push_dummy_call);
      set_gdbarch_return_value (gdbarch, mips_n32n64_return_value);
      tdep->mips_last_arg_regnum = MIPS_A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = tdep->regnum->fp0 + 12 + 8 - 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 64);
      set_gdbarch_ptr_bit (gdbarch, 64);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_long_double_bit (gdbarch, 128);
      set_gdbarch_long_double_format (gdbarch, floatformats_ibm_long_double);
      break;
    default:
      internal_error (__FILE__, __LINE__, _("unknown ABI in switch"));
    }

  /* GCC creates a pseudo-section whose name specifies the size of
     longs, since -mlong32 or -mlong64 may be used independent of
     other options.  How those options affect pointer sizes is ABI and
     architecture dependent, so use them to override the default sizes
     set by the ABI.  This table shows the relationship between ABI,
     -mlongXX, and size of pointers:

     ABI		-mlongXX	ptr bits
     ---		--------	--------
     o32		32		32
     o32		64		32
     n32		32		32
     n32		64		64
     o64		32		32
     o64		64		64
     n64		32		32
     n64		64		64
     eabi32		32		32
     eabi32		64		32
     eabi64		32		32
     eabi64		64		64

    Note that for o32 and eabi32, pointers are always 32 bits
    regardless of any -mlongXX option.  For all others, pointers and
    longs are the same, as set by -mlongXX or set by defaults.  */

  if (info.abfd != NULL)
    {
      int long_bit = 0;

      bfd_map_over_sections (info.abfd, mips_find_long_section, &long_bit);
      if (long_bit)
	{
	  set_gdbarch_long_bit (gdbarch, long_bit);
	  switch (mips_abi)
	    {
	    case MIPS_ABI_O32:
	    case MIPS_ABI_EABI32:
	      break;
	    case MIPS_ABI_N32:
	    case MIPS_ABI_O64:
	    case MIPS_ABI_N64:
	    case MIPS_ABI_EABI64:
	      set_gdbarch_ptr_bit (gdbarch, long_bit);
	      break;
	    default:
	      internal_error (__FILE__, __LINE__, _("unknown ABI in switch"));
	    }
	}
    }

  /* FIXME: jlarmour/2000-04-07: There *is* a flag EF_MIPS_32BIT_MODE
     that could indicate -gp32 BUT gas/config/tc-mips.c contains the
     comment:

     ``We deliberately don't allow "-gp32" to set the MIPS_32BITMODE
     flag in object files because to do so would make it impossible to
     link with libraries compiled without "-gp32".  This is
     unnecessarily restrictive.

     We could solve this problem by adding "-gp32" multilibs to gcc,
     but to set this flag before gcc is built with such multilibs will
     break too many systems.''

     But even more unhelpfully, the default linker output target for
     mips64-elf is elf32-bigmips, and has EF_MIPS_32BIT_MODE set, even
     for 64-bit programs - you need to change the ABI to change this,
     and not all gcc targets support that currently.  Therefore using
     this flag to detect 32-bit mode would do the wrong thing given
     the current gcc - it would make GDB treat these 64-bit programs
     as 32-bit programs by default.  */

  set_gdbarch_read_pc (gdbarch, mips_read_pc);
  set_gdbarch_write_pc (gdbarch, mips_write_pc);

  /* Add/remove bits from an address.  The MIPS needs be careful to
     ensure that all 32 bit addresses are sign extended to 64 bits.  */
  set_gdbarch_addr_bits_remove (gdbarch, mips_addr_bits_remove);

  /* Unwind the frame.  */
  set_gdbarch_unwind_pc (gdbarch, mips_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, mips_unwind_sp);
  set_gdbarch_dummy_id (gdbarch, mips_dummy_id);

  /* Map debug register numbers onto internal register numbers.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, mips_stab_reg_to_regnum);
  set_gdbarch_ecoff_reg_to_regnum (gdbarch,
				   mips_dwarf_dwarf2_ecoff_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch,
				    mips_dwarf_dwarf2_ecoff_reg_to_regnum);
  set_gdbarch_register_sim_regno (gdbarch, mips_register_sim_regno);

  /* MIPS version of CALL_DUMMY.  */

  /* NOTE: cagney/2003-08-05: Eventually call dummy location will be
     replaced by a command, and all targets will default to on stack
     (regardless of the stack's execute status).  */
  set_gdbarch_call_dummy_location (gdbarch, AT_SYMBOL);
  set_gdbarch_frame_align (gdbarch, mips_frame_align);

  set_gdbarch_convert_register_p (gdbarch, mips_convert_register_p);
  set_gdbarch_register_to_value (gdbarch, mips_register_to_value);
  set_gdbarch_value_to_register (gdbarch, mips_value_to_register);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_from_pc (gdbarch, mips_breakpoint_from_pc);

  set_gdbarch_skip_prologue (gdbarch, mips_skip_prologue);

  set_gdbarch_in_function_epilogue_p (gdbarch, mips_in_function_epilogue_p);

  set_gdbarch_pointer_to_address (gdbarch, signed_pointer_to_address);
  set_gdbarch_address_to_pointer (gdbarch, address_to_signed_pointer);
  set_gdbarch_integer_to_address (gdbarch, mips_integer_to_address);

  set_gdbarch_register_type (gdbarch, mips_register_type);

  set_gdbarch_print_registers_info (gdbarch, mips_print_registers_info);

  if (mips_abi == MIPS_ABI_N32)
    set_gdbarch_print_insn (gdbarch, gdb_print_insn_mips_n32);
  else if (mips_abi == MIPS_ABI_N64)
    set_gdbarch_print_insn (gdbarch, gdb_print_insn_mips_n64);
  else
    set_gdbarch_print_insn (gdbarch, gdb_print_insn_mips);

  /* FIXME: cagney/2003-08-29: The macros target_have_steppable_watchpoint,
     HAVE_NONSTEPPABLE_WATCHPOINT, and target_have_continuable_watchpoint
     need to all be folded into the target vector.  Since they are
     being used as guards for target_stopped_by_watchpoint, why not have
     target_stopped_by_watchpoint return the type of watchpoint that the code
     is sitting on?  */
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  set_gdbarch_skip_trampoline_code (gdbarch, mips_skip_trampoline_code);

  set_gdbarch_single_step_through_delay (gdbarch,
					 mips_single_step_through_delay);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 1);

  mips_register_g_packet_guesses (gdbarch);

  /* Hook in OS ABI-specific overrides, if they have been registered.  */
  info.tdep_info = (void *) tdesc_data;
  gdbarch_init_osabi (info, gdbarch);

  /* Unwind the frame.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &mips_stub_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &mips_insn16_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &mips_insn32_frame_unwind);
  frame_base_append_sniffer (gdbarch, dwarf2_frame_base_sniffer);
  frame_base_append_sniffer (gdbarch, mips_stub_frame_base_sniffer);
  frame_base_append_sniffer (gdbarch, mips_insn16_frame_base_sniffer);
  frame_base_append_sniffer (gdbarch, mips_insn32_frame_base_sniffer);

  if (tdesc_data)
    {
      set_tdesc_pseudo_register_type (gdbarch, mips_pseudo_register_type);
      tdesc_use_registers (gdbarch, info.target_desc, tdesc_data);

      /* Override the normal target description methods to handle our
	 dual real and pseudo registers.  */
      set_gdbarch_register_name (gdbarch, mips_register_name);
      set_gdbarch_register_reggroup_p (gdbarch,
				       mips_tdesc_register_reggroup_p);

      num_regs = gdbarch_num_regs (gdbarch);
      set_gdbarch_num_pseudo_regs (gdbarch, num_regs);
      set_gdbarch_pc_regnum (gdbarch, tdep->regnum->pc + num_regs);
      set_gdbarch_sp_regnum (gdbarch, MIPS_SP_REGNUM + num_regs);
    }

  /* Add ABI-specific aliases for the registers.  */
  if (mips_abi == MIPS_ABI_N32 || mips_abi == MIPS_ABI_N64)
    for (i = 0; i < ARRAY_SIZE (mips_n32_n64_aliases); i++)
      user_reg_add (gdbarch, mips_n32_n64_aliases[i].name,
		    value_of_mips_user_reg, &mips_n32_n64_aliases[i].regnum);
  else
    for (i = 0; i < ARRAY_SIZE (mips_o32_aliases); i++)
      user_reg_add (gdbarch, mips_o32_aliases[i].name,
		    value_of_mips_user_reg, &mips_o32_aliases[i].regnum);

  /* Add some other standard aliases.  */
  for (i = 0; i < ARRAY_SIZE (mips_register_aliases); i++)
    user_reg_add (gdbarch, mips_register_aliases[i].name,
		  value_of_mips_user_reg, &mips_register_aliases[i].regnum);

  for (i = 0; i < ARRAY_SIZE (mips_numeric_register_aliases); i++)
    user_reg_add (gdbarch, mips_numeric_register_aliases[i].name,
		  value_of_mips_user_reg, 
		  &mips_numeric_register_aliases[i].regnum);

  return gdbarch;
}

static void
mips_abi_update (char *ignore_args, int from_tty, struct cmd_list_element *c)
{
  struct gdbarch_info info;

  /* Force the architecture to update, and (if it's a MIPS architecture)
     mips_gdbarch_init will take care of the rest.  */
  gdbarch_info_init (&info);
  gdbarch_update_p (info);
}

/* Print out which MIPS ABI is in use.  */

static void
show_mips_abi (struct ui_file *file,
	       int from_tty,
	       struct cmd_list_element *ignored_cmd,
	       const char *ignored_value)
{
  if (gdbarch_bfd_arch_info (target_gdbarch)->arch != bfd_arch_mips)
    fprintf_filtered
      (file, 
       "The MIPS ABI is unknown because the current architecture "
       "is not MIPS.\n");
  else
    {
      enum mips_abi global_abi = global_mips_abi ();
      enum mips_abi actual_abi = mips_abi (target_gdbarch);
      const char *actual_abi_str = mips_abi_strings[actual_abi];

      if (global_abi == MIPS_ABI_UNKNOWN)
	fprintf_filtered
	  (file, 
	   "The MIPS ABI is set automatically (currently \"%s\").\n",
	   actual_abi_str);
      else if (global_abi == actual_abi)
	fprintf_filtered
	  (file,
	   "The MIPS ABI is assumed to be \"%s\" (due to user setting).\n",
	   actual_abi_str);
      else
	{
	  /* Probably shouldn't happen...  */
	  fprintf_filtered (file,
			    "The (auto detected) MIPS ABI \"%s\" is in use "
			    "even though the user setting was \"%s\".\n",
	     actual_abi_str, mips_abi_strings[global_abi]);
	}
    }
}

static void
mips_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  if (tdep != NULL)
    {
      int ef_mips_arch;
      int ef_mips_32bitmode;
      /* Determine the ISA.  */
      switch (tdep->elf_flags & EF_MIPS_ARCH)
	{
	case E_MIPS_ARCH_1:
	  ef_mips_arch = 1;
	  break;
	case E_MIPS_ARCH_2:
	  ef_mips_arch = 2;
	  break;
	case E_MIPS_ARCH_3:
	  ef_mips_arch = 3;
	  break;
	case E_MIPS_ARCH_4:
	  ef_mips_arch = 4;
	  break;
	default:
	  ef_mips_arch = 0;
	  break;
	}
      /* Determine the size of a pointer.  */
      ef_mips_32bitmode = (tdep->elf_flags & EF_MIPS_32BITMODE);
      fprintf_unfiltered (file,
			  "mips_dump_tdep: tdep->elf_flags = 0x%x\n",
			  tdep->elf_flags);
      fprintf_unfiltered (file,
			  "mips_dump_tdep: ef_mips_32bitmode = %d\n",
			  ef_mips_32bitmode);
      fprintf_unfiltered (file,
			  "mips_dump_tdep: ef_mips_arch = %d\n",
			  ef_mips_arch);
      fprintf_unfiltered (file,
			  "mips_dump_tdep: tdep->mips_abi = %d (%s)\n",
			  tdep->mips_abi, mips_abi_strings[tdep->mips_abi]);
      fprintf_unfiltered (file,
			  "mips_dump_tdep: "
			  "mips_mask_address_p() %d (default %d)\n",
			  mips_mask_address_p (tdep),
			  tdep->default_mask_address_p);
    }
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_DEFAULT_FPU_TYPE = %d (%s)\n",
		      MIPS_DEFAULT_FPU_TYPE,
		      (MIPS_DEFAULT_FPU_TYPE == MIPS_FPU_NONE ? "none"
		       : MIPS_DEFAULT_FPU_TYPE == MIPS_FPU_SINGLE ? "single"
		       : MIPS_DEFAULT_FPU_TYPE == MIPS_FPU_DOUBLE ? "double"
		       : "???"));
  fprintf_unfiltered (file, "mips_dump_tdep: MIPS_EABI = %d\n",
		      MIPS_EABI (gdbarch));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_FPU_TYPE = %d (%s)\n",
		      MIPS_FPU_TYPE (gdbarch),
		      (MIPS_FPU_TYPE (gdbarch) == MIPS_FPU_NONE ? "none"
		       : MIPS_FPU_TYPE (gdbarch) == MIPS_FPU_SINGLE ? "single"
		       : MIPS_FPU_TYPE (gdbarch) == MIPS_FPU_DOUBLE ? "double"
		       : "???"));
}

extern initialize_file_ftype _initialize_mips_tdep; /* -Wmissing-prototypes */

void
_initialize_mips_tdep (void)
{
  static struct cmd_list_element *mipsfpulist = NULL;
  struct cmd_list_element *c;

  mips_abi_string = mips_abi_strings[MIPS_ABI_UNKNOWN];
  if (MIPS_ABI_LAST + 1
      != sizeof (mips_abi_strings) / sizeof (mips_abi_strings[0]))
    internal_error (__FILE__, __LINE__, _("mips_abi_strings out of sync"));

  gdbarch_register (bfd_arch_mips, mips_gdbarch_init, mips_dump_tdep);

  mips_pdr_data = register_objfile_data ();

  /* Create feature sets with the appropriate properties.  The values
     are not important.  */
  mips_tdesc_gp32 = allocate_target_description ();
  set_tdesc_property (mips_tdesc_gp32, PROPERTY_GP32, "");

  mips_tdesc_gp64 = allocate_target_description ();
  set_tdesc_property (mips_tdesc_gp64, PROPERTY_GP64, "");

  /* Add root prefix command for all "set mips"/"show mips" commands.  */
  add_prefix_cmd ("mips", no_class, set_mips_command,
		  _("Various MIPS specific commands."),
		  &setmipscmdlist, "set mips ", 0, &setlist);

  add_prefix_cmd ("mips", no_class, show_mips_command,
		  _("Various MIPS specific commands."),
		  &showmipscmdlist, "show mips ", 0, &showlist);

  /* Allow the user to override the ABI.  */
  add_setshow_enum_cmd ("abi", class_obscure, mips_abi_strings,
			&mips_abi_string, _("\
Set the MIPS ABI used by this program."), _("\
Show the MIPS ABI used by this program."), _("\
This option can be set to one of:\n\
  auto  - the default ABI associated with the current binary\n\
  o32\n\
  o64\n\
  n32\n\
  n64\n\
  eabi32\n\
  eabi64"),
			mips_abi_update,
			show_mips_abi,
			&setmipscmdlist, &showmipscmdlist);

  /* Let the user turn off floating point and set the fence post for
     heuristic_proc_start.  */

  add_prefix_cmd ("mipsfpu", class_support, set_mipsfpu_command,
		  _("Set use of MIPS floating-point coprocessor."),
		  &mipsfpulist, "set mipsfpu ", 0, &setlist);
  add_cmd ("single", class_support, set_mipsfpu_single_command,
	   _("Select single-precision MIPS floating-point coprocessor."),
	   &mipsfpulist);
  add_cmd ("double", class_support, set_mipsfpu_double_command,
	   _("Select double-precision MIPS floating-point coprocessor."),
	   &mipsfpulist);
  add_alias_cmd ("on", "double", class_support, 1, &mipsfpulist);
  add_alias_cmd ("yes", "double", class_support, 1, &mipsfpulist);
  add_alias_cmd ("1", "double", class_support, 1, &mipsfpulist);
  add_cmd ("none", class_support, set_mipsfpu_none_command,
	   _("Select no MIPS floating-point coprocessor."), &mipsfpulist);
  add_alias_cmd ("off", "none", class_support, 1, &mipsfpulist);
  add_alias_cmd ("no", "none", class_support, 1, &mipsfpulist);
  add_alias_cmd ("0", "none", class_support, 1, &mipsfpulist);
  add_cmd ("auto", class_support, set_mipsfpu_auto_command,
	   _("Select MIPS floating-point coprocessor automatically."),
	   &mipsfpulist);
  add_cmd ("mipsfpu", class_support, show_mipsfpu_command,
	   _("Show current use of MIPS floating-point coprocessor target."),
	   &showlist);

  /* We really would like to have both "0" and "unlimited" work, but
     command.c doesn't deal with that.  So make it a var_zinteger
     because the user can always use "999999" or some such for unlimited.  */
  add_setshow_zinteger_cmd ("heuristic-fence-post", class_support,
			    &heuristic_fence_post, _("\
Set the distance searched for the start of a function."), _("\
Show the distance searched for the start of a function."), _("\
If you are debugging a stripped executable, GDB needs to search through the\n\
program for the start of a function.  This command sets the distance of the\n\
search.  The only need to set it is when debugging a stripped executable."),
			    reinit_frame_cache_sfunc,
			    NULL, /* FIXME: i18n: The distance searched for
				     the start of a function is %s.  */
			    &setlist, &showlist);

  /* Allow the user to control whether the upper bits of 64-bit
     addresses should be zeroed.  */
  add_setshow_auto_boolean_cmd ("mask-address", no_class,
				&mask_address_var, _("\
Set zeroing of upper 32 bits of 64-bit addresses."), _("\
Show zeroing of upper 32 bits of 64-bit addresses."), _("\
Use \"on\" to enable the masking, \"off\" to disable it and \"auto\" to\n\
allow GDB to determine the correct value."),
				NULL, show_mask_address,
				&setmipscmdlist, &showmipscmdlist);

  /* Allow the user to control the size of 32 bit registers within the
     raw remote packet.  */
  add_setshow_boolean_cmd ("remote-mips64-transfers-32bit-regs", class_obscure,
			   &mips64_transfers_32bit_regs_p, _("\
Set compatibility with 64-bit MIPS target that transfers 32-bit quantities."),
			   _("\
Show compatibility with 64-bit MIPS target that transfers 32-bit quantities."),
			   _("\
Use \"on\" to enable backward compatibility with older MIPS 64 GDB+target\n\
that would transfer 32 bits for some registers (e.g. SR, FSR) and\n\
64 bits for others.  Use \"off\" to disable compatibility mode"),
			   set_mips64_transfers_32bit_regs,
			   NULL, /* FIXME: i18n: Compatibility with 64-bit
				    MIPS target that transfers 32-bit
				    quantities is %s.  */
			   &setlist, &showlist);

  /* Debug this files internals.  */
  add_setshow_zinteger_cmd ("mips", class_maintenance,
			    &mips_debug, _("\
Set mips debugging."), _("\
Show mips debugging."), _("\
When non-zero, mips specific debugging is enabled."),
			    NULL,
			    NULL, /* FIXME: i18n: Mips debugging is
				     currently %s.  */
			    &setdebuglist, &showdebuglist);
}
