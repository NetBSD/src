/* Target-dependent code for the MIPS architecture, for GDB, the GNU Debugger.

   Copyright 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995, 1996,
   1997, 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

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
#include "arch-utils.h"
#include "regcache.h"
#include "osabi.h"

#include "opcode/mips.h"
#include "elf/mips.h"
#include "elf-bfd.h"
#include "symcat.h"

/* A useful bit in the CP0 status register (PS_REGNUM).  */
/* This bit is set if we are emulating 32-bit FPRs on a 64-bit chip.  */
#define ST0_FR (1 << 26)

/* The sizes of floating point registers.  */

enum
{
  MIPS_FPU_SINGLE_REGSIZE = 4,
  MIPS_FPU_DOUBLE_REGSIZE = 8
};

/* All the possible MIPS ABIs. */

enum mips_abi
  {
    MIPS_ABI_UNKNOWN = 0,
    MIPS_ABI_N32,
    MIPS_ABI_O32,
    MIPS_ABI_N64,
    MIPS_ABI_O64,
    MIPS_ABI_EABI32,
    MIPS_ABI_EABI64,
    MIPS_ABI_LAST
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

struct frame_extra_info
  {
    mips_extra_func_info_t proc_desc;
    int num_args;
  };

/* Various MIPS ISA options (related to stack analysis) can be
   overridden dynamically.  Establish an enum/array for managing
   them. */

static const char size_auto[] = "auto";
static const char size_32[] = "32";
static const char size_64[] = "64";

static const char *size_enums[] = {
  size_auto,
  size_32,
  size_64,
  0
};

/* Some MIPS boards don't support floating point while others only
   support single-precision floating-point operations.  See also
   FP_REGISTER_DOUBLE. */

enum mips_fpu_type
  {
    MIPS_FPU_DOUBLE,		/* Full double precision floating point.  */
    MIPS_FPU_SINGLE,		/* Single precision floating point (R4650).  */
    MIPS_FPU_NONE		/* No floating point.  */
  };

#ifndef MIPS_DEFAULT_FPU_TYPE
#define MIPS_DEFAULT_FPU_TYPE MIPS_FPU_DOUBLE
#endif
static int mips_fpu_type_auto = 1;
static enum mips_fpu_type mips_fpu_type = MIPS_DEFAULT_FPU_TYPE;

static int mips_debug = 0;

/* MIPS specific per-architecture information */
struct gdbarch_tdep
  {
    /* from the elf header */
    int elf_flags;

    /* mips options */
    enum mips_abi mips_abi;
    enum mips_abi found_abi;
    enum mips_fpu_type mips_fpu_type;
    int mips_last_arg_regnum;
    int mips_last_fp_arg_regnum;
    int mips_default_saved_regsize;
    int mips_fp_register_double;
    int mips_default_stack_argsize;
    int gdb_target_is_mips64;
    int default_mask_address_p;

    enum gdb_osabi osabi;
  };

#define MIPS_EABI (gdbarch_tdep (current_gdbarch)->mips_abi == MIPS_ABI_EABI32 \
		   || gdbarch_tdep (current_gdbarch)->mips_abi == MIPS_ABI_EABI64)

#define MIPS_LAST_FP_ARG_REGNUM (gdbarch_tdep (current_gdbarch)->mips_last_fp_arg_regnum)

#define MIPS_LAST_ARG_REGNUM (gdbarch_tdep (current_gdbarch)->mips_last_arg_regnum)

#define MIPS_FPU_TYPE (gdbarch_tdep (current_gdbarch)->mips_fpu_type)

/* Return the currently configured (or set) saved register size. */

#define MIPS_DEFAULT_SAVED_REGSIZE (gdbarch_tdep (current_gdbarch)->mips_default_saved_regsize)

static const char *mips_saved_regsize_string = size_auto;

#define MIPS_SAVED_REGSIZE (mips_saved_regsize())

static unsigned int
mips_saved_regsize (void)
{
  if (mips_saved_regsize_string == size_auto)
    return MIPS_DEFAULT_SAVED_REGSIZE;
  else if (mips_saved_regsize_string == size_64)
    return 8;
  else /* if (mips_saved_regsize_string == size_32) */
    return 4;
}

/* Functions for setting and testing a bit in a minimal symbol that
   marks it as 16-bit function.  The MSB of the minimal symbol's
   "info" field is used for this purpose. This field is already
   being used to store the symbol size, so the assumption is
   that the symbol size cannot exceed 2^31.

   ELF_MAKE_MSYMBOL_SPECIAL tests whether an ELF symbol is "special",
   i.e. refers to a 16-bit function, and sets a "special" bit in a
   minimal symbol to mark it as a 16-bit function

   MSYMBOL_IS_SPECIAL   tests the "special" bit in a minimal symbol
   MSYMBOL_SIZE         returns the size of the minimal symbol, i.e.
   the "info" field with the "special" bit masked out */

static void
mips_elf_make_msymbol_special (asymbol *sym, struct minimal_symbol *msym)
{
  if (((elf_symbol_type *)(sym))->internal_elf_sym.st_other == STO_MIPS16) 
    { 
      MSYMBOL_INFO (msym) = (char *) 
	(((long) MSYMBOL_INFO (msym)) | 0x80000000); 
      SYMBOL_VALUE_ADDRESS (msym) |= 1; 
    } 
}

static int
msymbol_is_special (struct minimal_symbol *msym)
{
  return (((long) MSYMBOL_INFO (msym) & 0x80000000) != 0);
}

static long
msymbol_size (struct minimal_symbol *msym)
{
  return ((long) MSYMBOL_INFO (msym) & 0x7fffffff);
}

/* XFER a value from the big/little/left end of the register.
   Depending on the size of the value it might occupy the entire
   register or just part of it.  Make an allowance for this, aligning
   things accordingly.  */

static void
mips_xfer_register (struct regcache *regcache, int reg_num, int length,
		    enum bfd_endian endian, bfd_byte *in, const bfd_byte *out,
		    int buf_offset)
{
  bfd_byte *reg = alloca (MAX_REGISTER_RAW_SIZE);
  int reg_offset = 0;
  /* Need to transfer the left or right part of the register, based on
     the targets byte order.  */
  switch (endian)
    {
    case BFD_ENDIAN_BIG:
      reg_offset = REGISTER_RAW_SIZE (reg_num) - length;
      break;
    case BFD_ENDIAN_LITTLE:
      reg_offset = 0;
      break;
    case BFD_ENDIAN_UNKNOWN: /* Indicates no alignment.  */
      reg_offset = 0;
      break;
    default:
      internal_error (__FILE__, __LINE__, "bad switch");
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
    regcache_raw_read_part (regcache, reg_num, reg_offset, length, in + buf_offset);
  if (out != NULL)
    regcache_raw_write_part (regcache, reg_num, reg_offset, length, out + buf_offset);
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
mips2_fp_compat (void)
{
  /* MIPS1 and MIPS2 have only 32 bit FPRs, and the FR bit is not
     meaningful.  */
  if (REGISTER_RAW_SIZE (FP0_REGNUM) == 4)
    return 0;

#if 0
  /* FIXME drow 2002-03-10: This is disabled until we can do it consistently,
     in all the places we deal with FP registers.  PR gdb/413.  */
  /* Otherwise check the FR bit in the status register - it controls
     the FP compatiblity mode.  If it is clear we are in compatibility
     mode.  */
  if ((read_register (PS_REGNUM) & ST0_FR) == 0)
    return 1;
#endif

  return 0;
}

/* Indicate that the ABI makes use of double-precision registers
   provided by the FPU (rather than combining pairs of registers to
   form double-precision values).  Do not use "TARGET_IS_MIPS64" to
   determine if the ABI is using double-precision registers.  See also
   MIPS_FPU_TYPE. */
#define FP_REGISTER_DOUBLE (gdbarch_tdep (current_gdbarch)->mips_fp_register_double)

/* The amount of space reserved on the stack for registers. This is
   different to MIPS_SAVED_REGSIZE as it determines the alignment of
   data allocated after the registers have run out. */

#define MIPS_DEFAULT_STACK_ARGSIZE (gdbarch_tdep (current_gdbarch)->mips_default_stack_argsize)

#define MIPS_STACK_ARGSIZE (mips_stack_argsize ())

static const char *mips_stack_argsize_string = size_auto;

static unsigned int
mips_stack_argsize (void)
{
  if (mips_stack_argsize_string == size_auto)
    return MIPS_DEFAULT_STACK_ARGSIZE;
  else if (mips_stack_argsize_string == size_64)
    return 8;
  else /* if (mips_stack_argsize_string == size_32) */
    return 4;
}

#define GDB_TARGET_IS_MIPS64 (gdbarch_tdep (current_gdbarch)->gdb_target_is_mips64 + 0)

#define MIPS_DEFAULT_MASK_ADDRESS_P (gdbarch_tdep (current_gdbarch)->default_mask_address_p)

#define VM_MIN_ADDRESS (CORE_ADDR)0x400000

int gdb_print_insn_mips (bfd_vma, disassemble_info *);

static void mips_print_register (int, int);

static mips_extra_func_info_t
heuristic_proc_desc (CORE_ADDR, CORE_ADDR, struct frame_info *, int);

static CORE_ADDR heuristic_proc_start (CORE_ADDR);

static CORE_ADDR read_next_frame_reg (struct frame_info *, int);

static int mips_set_processor_type (char *);

static void mips_show_processor_type_command (char *, int);

static void reinit_frame_cache_sfunc (char *, int, struct cmd_list_element *);

static mips_extra_func_info_t
find_proc_desc (CORE_ADDR pc, struct frame_info *next_frame, int cur_frame);

static CORE_ADDR after_prologue (CORE_ADDR pc,
				 mips_extra_func_info_t proc_desc);

static void mips_read_fp_register_single (int regno, char *rare_buffer);
static void mips_read_fp_register_double (int regno, char *rare_buffer);

static struct type *mips_float_register_type (void);
static struct type *mips_double_register_type (void);

/* This value is the model of MIPS in use.  It is derived from the value
   of the PrID register.  */

char *mips_processor_type;

char *tmp_mips_processor_type;

/* The list of available "set mips " and "show mips " commands */

static struct cmd_list_element *setmipscmdlist = NULL;
static struct cmd_list_element *showmipscmdlist = NULL;

/* A set of original names, to be used when restoring back to generic
   registers from a specific set.  */

char *mips_generic_reg_names[] = MIPS_REGISTER_NAMES;
char **mips_processor_reg_names = mips_generic_reg_names;

static const char *
mips_register_name (int i)
{
  return mips_processor_reg_names[i];
}
/* *INDENT-OFF* */
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
/* *INDENT-ON* */




/* Table to translate MIPS16 register field to actual register number.  */
static int mips16_to_32_reg[8] =
{16, 17, 2, 3, 4, 5, 6, 7};

/* Heuristic_proc_start may hunt through the text section for a long
   time across a 2400 baud serial line.  Allows the user to limit this
   search.  */

static unsigned int heuristic_fence_post = 0;

#define PROC_LOW_ADDR(proc) ((proc)->pdr.adr)	/* least address */
#define PROC_HIGH_ADDR(proc) ((proc)->high_addr)	/* upper address bound */
#define PROC_FRAME_OFFSET(proc) ((proc)->pdr.frameoffset)
#define PROC_FRAME_REG(proc) ((proc)->pdr.framereg)
#define PROC_FRAME_ADJUST(proc)  ((proc)->frame_adjust)
#define PROC_REG_MASK(proc) ((proc)->pdr.regmask)
#define PROC_FREG_MASK(proc) ((proc)->pdr.fregmask)
#define PROC_REG_OFFSET(proc) ((proc)->pdr.regoffset)
#define PROC_FREG_OFFSET(proc) ((proc)->pdr.fregoffset)
#define PROC_PC_REG(proc) ((proc)->pdr.pcreg)
/* FIXME drow/2002-06-10: If a pointer on the host is bigger than a long,
   this will corrupt pdr.iline.  Fortunately we don't use it.  */
#define PROC_SYMBOL(proc) (*(struct symbol**)&(proc)->pdr.isym)
#define _PROC_MAGIC_ 0x0F0F0F0F
#define PROC_DESC_IS_DUMMY(proc) ((proc)->pdr.isym == _PROC_MAGIC_)
#define SET_PROC_DESC_IS_DUMMY(proc) ((proc)->pdr.isym = _PROC_MAGIC_)

struct linked_proc_info
  {
    struct mips_extra_func_info info;
    struct linked_proc_info *next;
  }
 *linked_proc_desc_table = NULL;

void
mips_print_extra_frame_info (struct frame_info *fi)
{
  if (fi
      && fi->extra_info
      && fi->extra_info->proc_desc
      && fi->extra_info->proc_desc->pdr.framereg < NUM_REGS)
    printf_filtered (" frame pointer is at %s+%s\n",
		     REGISTER_NAME (fi->extra_info->proc_desc->pdr.framereg),
		     paddr_d (fi->extra_info->proc_desc->pdr.frameoffset));
}

/* Number of bytes of storage in the actual machine representation for
   register N.  NOTE: This indirectly defines the register size
   transfered by the GDB protocol. */

static int mips64_transfers_32bit_regs_p = 0;

static int
mips_register_raw_size (int reg_nr)
{
  if (mips64_transfers_32bit_regs_p)
    return REGISTER_VIRTUAL_SIZE (reg_nr);
  else if (reg_nr >= FP0_REGNUM && reg_nr < FP0_REGNUM + 32
	   && FP_REGISTER_DOUBLE)
    /* For MIPS_ABI_N32 (for example) we need 8 byte floating point
       registers.  */
    return 8;
  else
    return MIPS_REGSIZE;
}

/* Convert between RAW and VIRTUAL registers.  The RAW register size
   defines the remote-gdb packet. */

static int
mips_register_convertible (int reg_nr)
{
  if (mips64_transfers_32bit_regs_p)
    return 0;
  else
    return (REGISTER_RAW_SIZE (reg_nr) > REGISTER_VIRTUAL_SIZE (reg_nr));
}

static void
mips_register_convert_to_virtual (int n, struct type *virtual_type,
				  char *raw_buf, char *virt_buf)
{
  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
    memcpy (virt_buf,
	    raw_buf + (REGISTER_RAW_SIZE (n) - TYPE_LENGTH (virtual_type)),
	    TYPE_LENGTH (virtual_type));
  else
    memcpy (virt_buf,
	    raw_buf,
	    TYPE_LENGTH (virtual_type));
}

static void
mips_register_convert_to_raw (struct type *virtual_type, int n,
			      char *virt_buf, char *raw_buf)
{
  memset (raw_buf, 0, REGISTER_RAW_SIZE (n));
  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
    memcpy (raw_buf + (REGISTER_RAW_SIZE (n) - TYPE_LENGTH (virtual_type)),
	    virt_buf,
	    TYPE_LENGTH (virtual_type));
  else
    memcpy (raw_buf,
	    virt_buf,
	    TYPE_LENGTH (virtual_type));
}

void
mips_register_convert_to_type (int regnum, struct type *type, char *buffer)
{
  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
      && REGISTER_RAW_SIZE (regnum) == 4
      && (regnum) >= FP0_REGNUM && (regnum) < FP0_REGNUM + 32
      && TYPE_CODE(type) == TYPE_CODE_FLT
      && TYPE_LENGTH(type) == 8) 
    {
      char temp[4];
      memcpy (temp, ((char *)(buffer))+4, 4);
      memcpy (((char *)(buffer))+4, (buffer), 4);
      memcpy (((char *)(buffer)), temp, 4); 
    }
}

void
mips_register_convert_from_type (int regnum, struct type *type, char *buffer)
{
if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
    && REGISTER_RAW_SIZE (regnum) == 4
    && (regnum) >= FP0_REGNUM && (regnum) < FP0_REGNUM + 32
    && TYPE_CODE(type) == TYPE_CODE_FLT
    && TYPE_LENGTH(type) == 8) 
  {
    char temp[4];
    memcpy (temp, ((char *)(buffer))+4, 4);
    memcpy (((char *)(buffer))+4, (buffer), 4);
    memcpy (((char *)(buffer)), temp, 4);
  }
}

/* Return the GDB type object for the "standard" data type
   of data in register REG.  
   
   Note: kevinb/2002-08-01: The definition below should faithfully
   reproduce the behavior of each of the REGISTER_VIRTUAL_TYPE
   definitions found in config/mips/tm-*.h.  I'm concerned about
   the ``FCRCS_REGNUM <= reg && reg <= LAST_EMBED_REGNUM'' clause
   though.  In some cases FP_REGNUM is in this range, and I doubt
   that this code is correct for the 64-bit case.  */

static struct type *
mips_register_virtual_type (int reg)
{
  if (FP0_REGNUM <= reg && reg < FP0_REGNUM + 32)
    {
      /* Floating point registers...  */
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	return builtin_type_ieee_double_big;
      else
	return builtin_type_ieee_double_little;
    }
  else if (reg == PS_REGNUM /* CR */)
    return builtin_type_uint32;
  else if (FCRCS_REGNUM <= reg && reg <= LAST_EMBED_REGNUM)
    return builtin_type_uint32;
  else
    {
      /* Everything else...
         Return type appropriate for width of register.  */
      if (MIPS_REGSIZE == TYPE_LENGTH (builtin_type_uint64))
	return builtin_type_uint64;
      else
	return builtin_type_uint32;
    }
}

/* TARGET_READ_SP -- Remove useless bits from the stack pointer.  */

static CORE_ADDR
mips_read_sp (void)
{
  return ADDR_BITS_REMOVE (read_register (SP_REGNUM));
}

/* Should the upper word of 64-bit addresses be zeroed? */
enum auto_boolean mask_address_var = AUTO_BOOLEAN_AUTO;

static int
mips_mask_address_p (void)
{
  switch (mask_address_var)
    {
    case AUTO_BOOLEAN_TRUE:
      return 1;
    case AUTO_BOOLEAN_FALSE:
      return 0;
      break;
    case AUTO_BOOLEAN_AUTO:
      return MIPS_DEFAULT_MASK_ADDRESS_P;
    default:
      internal_error (__FILE__, __LINE__,
		      "mips_mask_address_p: bad switch");
      return -1;
    }
}

static void
show_mask_address (char *cmd, int from_tty, struct cmd_list_element *c)
{
  switch (mask_address_var)
    {
    case AUTO_BOOLEAN_TRUE:
      printf_filtered ("The 32 bit mips address mask is enabled\n");
      break;
    case AUTO_BOOLEAN_FALSE:
      printf_filtered ("The 32 bit mips address mask is disabled\n");
      break;
    case AUTO_BOOLEAN_AUTO:
      printf_filtered ("The 32 bit address mask is set automatically.  Currently %s\n",
		       mips_mask_address_p () ? "enabled" : "disabled");
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      "show_mask_address: bad switch");
      break;
    }
}

/* Should call_function allocate stack space for a struct return?  */

static int
mips_eabi_use_struct_convention (int gcc_p, struct type *type)
{
  return (TYPE_LENGTH (type) > 2 * MIPS_SAVED_REGSIZE);
}

static int
mips_n32n64_use_struct_convention (int gcc_p, struct type *type)
{
  return (TYPE_LENGTH (type) > 2 * MIPS_SAVED_REGSIZE);
}

static int
mips_o32_use_struct_convention (int gcc_p, struct type *type)
{
  return 1;	/* Structures are returned by ref in extra arg0.  */
}

/* Should call_function pass struct by reference? 
   For each architecture, structs are passed either by
   value or by reference, depending on their size.  */

static int
mips_eabi_reg_struct_has_addr (int gcc_p, struct type *type)
{
  enum type_code typecode = TYPE_CODE (check_typedef (type));
  int len = TYPE_LENGTH (check_typedef (type));

  if (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION)
    return (len > MIPS_SAVED_REGSIZE);

  return 0;
}

static int
mips_n32n64_reg_struct_has_addr (int gcc_p, struct type *type)
{
  return 0;	/* Assumption: N32/N64 never passes struct by ref.  */
}

static int
mips_o32_reg_struct_has_addr (int gcc_p, struct type *type)
{
  return 0;	/* Assumption: O32/O64 never passes struct by ref.  */
}

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
    return msymbol_is_special (sym);
  else
    return 0;
}

/* MIPS believes that the PC has a sign extended value.  Perhaphs the
   all registers should be sign extended for simplicity? */

static CORE_ADDR
mips_read_pc (ptid_t ptid)
{
  return read_signed_register_pid (PC_REGNUM, ptid);
}

/* This returns the PC of the first inst after the prologue.  If we can't
   find the prologue, then return 0.  */

static CORE_ADDR
after_prologue (CORE_ADDR pc,
		mips_extra_func_info_t proc_desc)
{
  struct symtab_and_line sal;
  CORE_ADDR func_addr, func_end;

  /* Pass cur_frame == 0 to find_proc_desc.  We should not attempt
     to read the stack pointer from the current machine state, because
     the current machine state has nothing to do with the information
     we need from the proc_desc; and the process may or may not exist
     right now.  */
  if (!proc_desc)
    proc_desc = find_proc_desc (pc, NULL, 0);

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
mips32_decode_reg_save (t_inst inst, unsigned long *gen_mask,
			unsigned long *float_mask)
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
	   || (inst & 0xffe00000) == 0xe7c00000		/* swc1 freg,n($r30) */
	   || (inst & 0xffe00000) == 0xf7a00000)	/* sdc1 freg,n($sp) */

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
mips16_decode_reg_save (t_inst inst, unsigned long *gen_mask)
{
  if ((inst & 0xf800) == 0xd000)	/* sw reg,n($sp) */
    {
      int reg = mips16_to_32_reg[(inst & 0x700) >> 8];
      *gen_mask |= (1 << reg);
    }
  else if ((inst & 0xff00) == 0xf900)	/* sd reg,n($sp) */
    {
      int reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
      *gen_mask |= (1 << reg);
    }
  else if ((inst & 0xff00) == 0x6200	/* sw $ra,n($sp) */
	   || (inst & 0xff00) == 0xfa00)	/* sd $ra,n($sp) */
    *gen_mask |= (1 << RA_REGNUM);
}


/* Fetch and return instruction from the specified location.  If the PC
   is odd, assume it's a MIPS16 instruction; otherwise MIPS32.  */

static t_inst
mips_fetch_instruction (CORE_ADDR addr)
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


/* These the fields of 32 bit mips instructions */
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

static CORE_ADDR
mips32_relative_offset (unsigned long inst)
{
  long x;
  x = itype_immediate (inst);
  if (x & 0x8000)		/* sign bit set */
    {
      x |= 0xffff0000;		/* sign extension */
    }
  x = x << 2;
  return x;
}

/* Determine whate to set a single step breakpoint while considering
   branch prediction */
static CORE_ADDR
mips32_next_pc (CORE_ADDR pc)
{
  unsigned long inst;
  int op;
  inst = mips_fetch_instruction (pc);
  if ((inst & 0xe0000000) != 0)	/* Not a special, jump or branch instruction */
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
	    case 3:		/* BGTZ */
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
	  int fcrcs = read_signed_register (FCRCS_REGNUM);
	  int cond = ((fcrcs >> 24) & 0x0e) | ((fcrcs >> 23) & 0x01);

	  if (((cond >> cnum) & 0x01) == tf)
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	}
      else
	pc += 4;		/* Not a branch, next instruction is easy */
    }
  else
    {				/* This gets way messy */

      /* Further subdivide into SPECIAL, REGIMM and other */
      switch (op = itype_op (inst) & 0x07)	/* extract bits 28,27,26 */
	{
	case 0:		/* SPECIAL */
	  op = rtype_funct (inst);
	  switch (op)
	    {
	    case 8:		/* JR */
	    case 9:		/* JALR */
	      /* Set PC to that address */
	      pc = read_signed_register (rtype_rs (inst));
	      break;
	    default:
	      pc += 4;
	    }

	  break;	/* end SPECIAL */
	case 1:		/* REGIMM */
	  {
	    op = itype_rt (inst);	/* branch condition */
	    switch (op)
	      {
	      case 0:		/* BLTZ */
	      case 2:		/* BLTZL */
	      case 16:		/* BLTZAL */
	      case 18:		/* BLTZALL */
	      less_branch:
		if (read_signed_register (itype_rs (inst)) < 0)
		  pc += mips32_relative_offset (inst) + 4;
		else
		  pc += 8;	/* after the delay slot */
		break;
	      case 1:		/* BGEZ */
	      case 3:		/* BGEZL */
	      case 17:		/* BGEZAL */
	      case 19:		/* BGEZALL */
	      greater_equal_branch:
		if (read_signed_register (itype_rs (inst)) >= 0)
		  pc += mips32_relative_offset (inst) + 4;
		else
		  pc += 8;	/* after the delay slot */
		break;
		/* All of the other instructions in the REGIMM category */
	      default:
		pc += 4;
	      }
	  }
	  break;	/* end REGIMM */
	case 2:		/* J */
	case 3:		/* JAL */
	  {
	    unsigned long reg;
	    reg = jtype_target (inst) << 2;
	    /* Upper four bits get never changed... */
	    pc = reg + ((pc + 4) & 0xf0000000);
	  }
	  break;
	  /* FIXME case JALX : */
	  {
	    unsigned long reg;
	    reg = jtype_target (inst) << 2;
	    pc = reg + ((pc + 4) & 0xf0000000) + 1;	/* yes, +1 */
	    /* Add 1 to indicate 16 bit mode - Invert ISA mode */
	  }
	  break;		/* The new PC will be alternate mode */
	case 4:		/* BEQ, BEQL */
	equal_branch:
	  if (read_signed_register (itype_rs (inst)) ==
	      read_signed_register (itype_rt (inst)))
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 5:		/* BNE, BNEL */
	neq_branch:
	  if (read_signed_register (itype_rs (inst)) !=
	      read_signed_register (itype_rt (inst)))
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 6:		/* BLEZ, BLEZL */
	less_zero_branch:
	  if (read_signed_register (itype_rs (inst) <= 0))
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	case 7:
	default:
	greater_branch:	/* BGTZ, BGTZL */
	  if (read_signed_register (itype_rs (inst) > 0))
	    pc += mips32_relative_offset (inst) + 4;
	  else
	    pc += 8;
	  break;
	}			/* switch */
    }				/* else */
  return pc;
}				/* mips32_next_pc */

/* Decoding the next place to set a breakpoint is irregular for the
   mips 16 variant, but fortunately, there fewer instructions. We have to cope
   ith extensions for 16 bit instructions and a pair of actual 32 bit instructions.
   We dont want to set a single step instruction on the extend instruction
   either.
 */

/* Lots of mips16 instruction formats */
/* Predicting jumps requires itype,ritype,i8type
   and their extensions      extItype,extritype,extI8type
 */
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
   then, only the fields which are involved in instruction extension */
struct upk_mips16
  {
    CORE_ADDR offset;
    unsigned int regx;		/* Function in i8 type */
    unsigned int regy;
  };


/* The EXT-I, EXT-ri nad EXT-I8 instructions all have the same format
   for the bits which make up the immediatate extension.  */

static CORE_ADDR
extended_offset (unsigned int extension)
{
  CORE_ADDR value;
  value = (extension >> 21) & 0x3f;	/* * extract 15:11 */
  value = value << 6;
  value |= (extension >> 16) & 0x1f;	/* extrace 10:5 */
  value = value << 5;
  value |= extension & 0x01f;	/* extract 4:0 */
  return value;
}

/* Only call this function if you know that this is an extendable
   instruction, It wont malfunction, but why make excess remote memory references?
   If the immediate operands get sign extended or somthing, do it after
   the extension is performed.
 */
/* FIXME: Every one of these cases needs to worry about sign extension
   when the offset is to be used in relative addressing */


static unsigned int
fetch_mips_16 (CORE_ADDR pc)
{
  char buf[8];
  pc &= 0xfffffffe;		/* clear the low order bit */
  target_read_memory (pc, buf, 2);
  return extract_unsigned_integer (buf, 2);
}

static void
unpack_mips16 (CORE_ADDR pc,
	       unsigned int extension,
	       unsigned int inst,
	       enum mips16_inst_fmts insn_format,
	       struct upk_mips16 *upk)
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
	    value |= inst & 0x7ff;		/* eleven bits from instruction */
	  }
	else
	  {
	    value = inst & 0x7ff;
	    /* FIXME : Consider sign extension */
	  }
	offset = value;
	regx = -1;
	regy = -1;
      }
      break;
    case ritype:
    case i8type:
      {				/* A register identifier and an offset */
	/* Most of the fields are the same as I type but the
	   immediate value is of a different length */
	CORE_ADDR value;
	if (extension)
	  {
	    value = extended_offset (extension);
	    value = value << 8;	/* from the original instruction */
	    value |= inst & 0xff;	/* eleven bits from instruction */
	    regx = (extension >> 8) & 0x07;	/* or i8 funct */
	    if (value & 0x4000)	/* test the sign bit , bit 26 */
	      {
		value &= ~0x3fff;	/* remove the sign bit */
		value = -value;
	      }
	  }
	else
	  {
	    value = inst & 0xff;	/* 8 bits */
	    regx = (inst >> 8) & 0x07;	/* or i8 funct */
	    /* FIXME: Do sign extension , this format needs it */
	    if (value & 0x80)	/* THIS CONFUSES ME */
	      {
		value &= 0xef;	/* remove the sign bit */
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
	nexthalf = mips_fetch_instruction (pc + 2);	/* low bit still set */
	value |= nexthalf;
	offset = value;
	regx = -1;
	regy = -1;
	break;
      }
    default:
      internal_error (__FILE__, __LINE__,
		      "bad switch");
    }
  upk->offset = offset;
  upk->regx = regx;
  upk->regy = regy;
}


static CORE_ADDR
add_offset_16 (CORE_ADDR pc, int offset)
{
  return ((offset << 2) | ((pc + 2) & (0xf0000000)));
}

static CORE_ADDR
extended_mips16_next_pc (CORE_ADDR pc,
			 unsigned int extension,
			 unsigned int insn)
{
  int op = (insn >> 11);
  switch (op)
    {
    case 2:		/* Branch */
      {
	CORE_ADDR offset;
	struct upk_mips16 upk;
	unpack_mips16 (pc, extension, insn, itype, &upk);
	offset = upk.offset;
	if (offset & 0x800)
	  {
	    offset &= 0xeff;
	    offset = -offset;
	  }
	pc += (offset << 1) + 2;
	break;
      }
    case 3:		/* JAL , JALX - Watch out, these are 32 bit instruction */
      {
	struct upk_mips16 upk;
	unpack_mips16 (pc, extension, insn, jalxtype, &upk);
	pc = add_offset_16 (pc, upk.offset);
	if ((insn >> 10) & 0x01)	/* Exchange mode */
	  pc = pc & ~0x01;	/* Clear low bit, indicate 32 bit mode */
	else
	  pc |= 0x01;
	break;
      }
    case 4:		/* beqz */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (pc, extension, insn, ritype, &upk);
	reg = read_signed_register (upk.regx);
	if (reg == 0)
	  pc += (upk.offset << 1) + 2;
	else
	  pc += 2;
	break;
      }
    case 5:		/* bnez */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (pc, extension, insn, ritype, &upk);
	reg = read_signed_register (upk.regx);
	if (reg != 0)
	  pc += (upk.offset << 1) + 2;
	else
	  pc += 2;
	break;
      }
    case 12:		/* I8 Formats btez btnez */
      {
	struct upk_mips16 upk;
	int reg;
	unpack_mips16 (pc, extension, insn, i8type, &upk);
	/* upk.regx contains the opcode */
	reg = read_signed_register (24);	/* Test register is 24 */
	if (((upk.regx == 0) && (reg == 0))	/* BTEZ */
	    || ((upk.regx == 1) && (reg != 0)))	/* BTNEZ */
	  /* pc = add_offset_16(pc,upk.offset) ; */
	  pc += (upk.offset << 1) + 2;
	else
	  pc += 2;
	break;
      }
    case 29:		/* RR Formats JR, JALR, JALR-RA */
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
		break;	/* Function return instruction */
	      case 2:
		reg = upk.regx;
		break;
	      default:
		reg = 31;
		break;	/* BOGUS Guess */
	      }
	    pc = read_signed_register (reg);
	  }
	else
	  pc += 2;
	break;
      }
    case 30:
      /* This is an instruction extension.  Fetch the real instruction
         (which follows the extension) and decode things based on
         that. */
      {
	pc += 2;
	pc = extended_mips16_next_pc (pc, insn, fetch_mips_16 (pc));
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
mips16_next_pc (CORE_ADDR pc)
{
  unsigned int insn = fetch_mips_16 (pc);
  return extended_mips16_next_pc (pc, 0, insn);
}

/* The mips_next_pc function supports single_step when the remote
   target monitor or stub is not developed enough to do a single_step.
   It works by decoding the current instruction and predicting where a
   branch will go. This isnt hard because all the data is available.
   The MIPS32 and MIPS16 variants are quite different */
CORE_ADDR
mips_next_pc (CORE_ADDR pc)
{
  if (pc & 0x01)
    return mips16_next_pc (pc);
  else
    return mips32_next_pc (pc);
}

/* Guaranteed to set fci->saved_regs to some values (it never leaves it
   NULL).

   Note: kevinb/2002-08-09: The only caller of this function is (and
   should remain) mips_frame_init_saved_regs().  In fact,
   aside from calling mips_find_saved_regs(), mips_frame_init_saved_regs()
   does nothing more than set frame->saved_regs[SP_REGNUM].  These two
   functions should really be combined and now that there is only one
   caller, it should be straightforward.  (Watch out for multiple returns
   though.)  */

static void
mips_find_saved_regs (struct frame_info *fci)
{
  int ireg;
  CORE_ADDR reg_position;
  /* r0 bit means kernel trap */
  int kernel_trap;
  /* What registers have been saved?  Bitmasks.  */
  unsigned long gen_mask, float_mask;
  mips_extra_func_info_t proc_desc;
  t_inst inst;

  frame_saved_regs_zalloc (fci);

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
	  fci->saved_regs[ireg] = reg_position;
	}
      for (ireg = 0; ireg < MIPS_NUMREGS; ireg++)
	{
	  reg_position = fci->frame + SIGFRAME_FPREGSAVE_OFF
	    + ireg * SIGFRAME_REG_SIZE;
	  fci->saved_regs[FP0_REGNUM + ireg] = reg_position;
	}
      fci->saved_regs[PC_REGNUM] = fci->frame + SIGFRAME_PC_OFF;
      return;
    }

  proc_desc = fci->extra_info->proc_desc;
  if (proc_desc == NULL)
    /* I'm not sure how/whether this can happen.  Normally when we can't
       find a proc_desc, we "synthesize" one using heuristic_proc_desc
       and set the saved_regs right away.  */
    return;

  kernel_trap = PROC_REG_MASK (proc_desc) & 1;
  gen_mask = kernel_trap ? 0xFFFFFFFF : PROC_REG_MASK (proc_desc);
  float_mask = kernel_trap ? 0xFFFFFFFF : PROC_FREG_MASK (proc_desc);

  if (				/* In any frame other than the innermost or a frame interrupted by
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
  for (ireg = MIPS_NUMREGS - 1; gen_mask; --ireg, gen_mask <<= 1)
    if (gen_mask & 0x80000000)
      {
	fci->saved_regs[ireg] = reg_position;
	reg_position -= MIPS_SAVED_REGSIZE;
      }

  /* The MIPS16 entry instruction saves $s0 and $s1 in the reverse order
     of that normally used by gcc.  Therefore, we have to fetch the first
     instruction of the function, and if it's an entry instruction that
     saves $s0 or $s1, correct their saved addresses.  */
  if (pc_is_mips16 (PROC_LOW_ADDR (proc_desc)))
    {
      inst = mips_fetch_instruction (PROC_LOW_ADDR (proc_desc));
      if ((inst & 0xf81f) == 0xe809 && (inst & 0x700) != 0x700)		/* entry */
	{
	  int reg;
	  int sreg_count = (inst >> 6) & 3;

	  /* Check if the ra register was pushed on the stack.  */
	  reg_position = fci->frame + PROC_REG_OFFSET (proc_desc);
	  if (inst & 0x20)
	    reg_position -= MIPS_SAVED_REGSIZE;

	  /* Check if the s0 and s1 registers were pushed on the stack.  */
	  for (reg = 16; reg < sreg_count + 16; reg++)
	    {
	      fci->saved_regs[reg] = reg_position;
	      reg_position -= MIPS_SAVED_REGSIZE;
	    }
	}
    }

  /* Fill in the offsets for the registers which float_mask says
     were saved.  */
  reg_position = fci->frame + PROC_FREG_OFFSET (proc_desc);

  /* Apparently, the freg_offset gives the offset to the first 64 bit
     saved.

     When the ABI specifies 64 bit saved registers, the FREG_OFFSET
     designates the first saved 64 bit register.

     When the ABI specifies 32 bit saved registers, the ``64 bit saved
     DOUBLE'' consists of two adjacent 32 bit registers, Hence
     FREG_OFFSET, designates the address of the lower register of the
     register pair.  Adjust the offset so that it designates the upper
     register of the pair -- i.e., the address of the first saved 32
     bit register.  */

  if (MIPS_SAVED_REGSIZE == 4)
    reg_position += MIPS_SAVED_REGSIZE;

  /* Fill in the offsets for the float registers which float_mask says
     were saved.  */
  for (ireg = MIPS_NUMREGS - 1; float_mask; --ireg, float_mask <<= 1)
    if (float_mask & 0x80000000)
      {
	fci->saved_regs[FP0_REGNUM + ireg] = reg_position;
	reg_position -= MIPS_SAVED_REGSIZE;
      }

  fci->saved_regs[PC_REGNUM] = fci->saved_regs[RA_REGNUM];
}

/* Set up the 'saved_regs' array.  This is a data structure containing
   the addresses on the stack where each register has been saved, for
   each stack frame.  Registers that have not been saved will have
   zero here.  The stack pointer register is special:  rather than the
   address where the stack register has been saved, saved_regs[SP_REGNUM]
   will have the actual value of the previous frame's stack register.  */

static void
mips_frame_init_saved_regs (struct frame_info *frame)
{
  if (frame->saved_regs == NULL)
    {
      mips_find_saved_regs (frame);
    }
  frame->saved_regs[SP_REGNUM] = frame->frame;
}

static CORE_ADDR
read_next_frame_reg (struct frame_info *fi, int regno)
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
	    FRAME_INIT_SAVED_REGS (fi);
	  if (fi->saved_regs[regno])
	    return read_memory_integer (ADDR_BITS_REMOVE (fi->saved_regs[regno]), MIPS_SAVED_REGSIZE);
	}
    }
  return read_signed_register (regno);
}

/* mips_addr_bits_remove - remove useless address bits  */

static CORE_ADDR
mips_addr_bits_remove (CORE_ADDR addr)
{
  if (GDB_TARGET_IS_MIPS64)
    {
      if (mips_mask_address_p () && (addr >> 32 == (CORE_ADDR) 0xffffffff))
	{
	  /* This hack is a work-around for existing boards using
	     PMON, the simulator, and any other 64-bit targets that
	     doesn't have true 64-bit addressing.  On these targets,
	     the upper 32 bits of addresses are ignored by the
	     hardware.  Thus, the PC or SP are likely to have been
	     sign extended to all 1s by instruction sequences that
	     load 32-bit addresses.  For example, a typical piece of
	     code that loads an address is this:
	         lui $r2, <upper 16 bits>
	         ori $r2, <lower 16 bits>
	     But the lui sign-extends the value such that the upper 32
	     bits may be all 1s.  The workaround is simply to mask off
	     these bits.  In the future, gcc may be changed to support
	     true 64-bit addressing, and this masking will have to be
	     disabled.  */
	  addr &= (CORE_ADDR) 0xffffffff;
	}
    }
  else if (mips_mask_address_p ())
    {
      /* FIXME: This is wrong!  mips_addr_bits_remove() shouldn't be
         masking off bits, instead, the actual target should be asking
         for the address to be converted to a valid pointer. */
      /* Even when GDB is configured for some 32-bit targets
	 (e.g. mips-elf), BFD is configured to handle 64-bit targets,
	 so CORE_ADDR is 64 bits.  So we still have to mask off
	 useless bits from addresses.  */
      addr &= (CORE_ADDR) 0xffffffff;
    }
  return addr;
}

/* mips_software_single_step() is called just before we want to resume
   the inferior, if we want to single-step it but there is no hardware
   or kernel single-step support (MIPS on GNU/Linux for example).  We find
   the target of the coming instruction and breakpoint it.

   single_step is also called just after the inferior stops.  If we had
   set up a simulated single-step, we undo our damage.  */

void
mips_software_single_step (enum target_signal sig, int insert_breakpoints_p)
{
  static CORE_ADDR next_pc;
  typedef char binsn_quantum[BREAKPOINT_MAX];
  static binsn_quantum break_mem;
  CORE_ADDR pc;

  if (insert_breakpoints_p)
    {
      pc = read_register (PC_REGNUM);
      next_pc = mips_next_pc (pc);

      target_insert_breakpoint (next_pc, break_mem);
    }
  else
    target_remove_breakpoint (next_pc, break_mem);
}

static void
mips_init_frame_pc_first (int fromleaf, struct frame_info *prev)
{
  CORE_ADDR pc, tmp;

  pc = ((fromleaf) ? SAVED_PC_AFTER_CALL (prev->next) :
	prev->next ? FRAME_SAVED_PC (prev->next) : read_pc ());
  tmp = SKIP_TRAMPOLINE_CODE (pc);
  prev->pc = tmp ? tmp : pc;
}


static CORE_ADDR
mips_frame_saved_pc (struct frame_info *frame)
{
  CORE_ADDR saved_pc;
  mips_extra_func_info_t proc_desc = frame->extra_info->proc_desc;
  /* We have to get the saved pc from the sigcontext
     if it is a signal handler frame.  */
  int pcreg = frame->signal_handler_caller ? PC_REGNUM
  : (proc_desc ? PROC_PC_REG (proc_desc) : RA_REGNUM);

  if (proc_desc && PROC_DESC_IS_DUMMY (proc_desc))
    saved_pc = read_memory_integer (frame->frame - MIPS_SAVED_REGSIZE, MIPS_SAVED_REGSIZE);
  else
    saved_pc = read_next_frame_reg (frame, pcreg);

  return ADDR_BITS_REMOVE (saved_pc);
}

static struct mips_extra_func_info temp_proc_desc;
static CORE_ADDR temp_saved_regs[NUM_REGS];

/* Set a register's saved stack address in temp_saved_regs.  If an address
   has already been set for this register, do nothing; this way we will
   only recognize the first save of a given register in a function prologue.
   This is a helper function for mips{16,32}_heuristic_proc_desc.  */

static void
set_reg_offset (int regno, CORE_ADDR offset)
{
  if (temp_saved_regs[regno] == 0)
    temp_saved_regs[regno] = offset;
}


/* Test whether the PC points to the return instruction at the
   end of a function. */

static int
mips_about_to_return (CORE_ADDR pc)
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


/* This fencepost looks highly suspicious to me.  Removing it also
   seems suspicious as it could affect remote debugging across serial
   lines.  */

static CORE_ADDR
heuristic_proc_start (CORE_ADDR pc)
{
  CORE_ADDR start_pc;
  CORE_ADDR fence;
  int instlen;
  int seen_adjsp = 0;

  pc = ADDR_BITS_REMOVE (pc);
  start_pc = pc;
  fence = start_pc - heuristic_fence_post;
  if (start_pc == 0)
    return 0;

  if (heuristic_fence_post == UINT_MAX
      || fence < VM_MIN_ADDRESS)
    fence = VM_MIN_ADDRESS;

  instlen = pc_is_mips16 (pc) ? MIPS16_INSTLEN : MIPS_INSTLEN;

  /* search back for previous return */
  for (start_pc -= instlen;; start_pc -= instlen)
    if (start_pc < fence)
      {
	/* It's not clear to me why we reach this point when
	   stop_soon_quietly, but with this test, at least we
	   don't print out warnings for every child forked (eg, on
	   decstation).  22apr93 rich@cygnus.com.  */
	if (!stop_soon_quietly)
	  {
	    static int blurb_printed = 0;

	    warning ("Warning: GDB can't find the start of the function at 0x%s.",
		     paddr_nz (pc));

	    if (!blurb_printed)
	      {
		/* This actually happens frequently in embedded
		   development, when you first connect to a board
		   and your stack pointer and pc are nowhere in
		   particular.  This message needs to give people
		   in that situation enough information to
		   determine that it's no big deal.  */
		printf_filtered ("\n\
    GDB is unable to find the start of the function at 0x%s\n\
and thus can't determine the size of that function's stack frame.\n\
This means that GDB may be unable to access that stack frame, or\n\
the frames below it.\n\
    This problem is most likely caused by an invalid program counter or\n\
stack pointer.\n\
    However, if you think GDB should simply search farther back\n\
from 0x%s for code which looks like the beginning of a\n\
function, you can increase the range of the search using the `set\n\
heuristic-fence-post' command.\n",
				 paddr_nz (pc), paddr_nz (pc));
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
	if (((inst & 0xf81f) == 0xe809 && (inst & 0x700) != 0x700)	/* entry */
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
    else if (mips_about_to_return (start_pc))
      {
	start_pc += 2 * MIPS_INSTLEN;	/* skip return, and its delay slot */
	break;
      }

  return start_pc;
}

/* Fetch the immediate value from a MIPS16 instruction.
   If the previous instruction was an EXTEND, use it to extend
   the upper bits of the immediate value.  This is a helper function
   for mips16_heuristic_proc_desc.  */

static int
mips16_get_imm (unsigned short prev_inst,	/* previous instruction */
		unsigned short inst,	/* current instruction */
		int nbits,		/* number of bits in imm field */
		int scale,		/* scale factor to be applied to imm */
		int is_signed)		/* is the imm field signed? */
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


/* Fill in values in temp_proc_desc based on the MIPS16 instruction
   stream from start_pc to limit_pc.  */

static void
mips16_heuristic_proc_desc (CORE_ADDR start_pc, CORE_ADDR limit_pc,
			    struct frame_info *next_frame, CORE_ADDR sp)
{
  CORE_ADDR cur_pc;
  CORE_ADDR frame_addr = 0;	/* Value of $r17, used as frame pointer */
  unsigned short prev_inst = 0;	/* saved copy of previous instruction */
  unsigned inst = 0;		/* current instruction */
  unsigned entry_inst = 0;	/* the entry instruction */
  int reg, offset;

  PROC_FRAME_OFFSET (&temp_proc_desc) = 0;	/* size of stack frame */
  PROC_FRAME_ADJUST (&temp_proc_desc) = 0;	/* offset of FP from SP */

  for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += MIPS16_INSTLEN)
    {
      /* Save the previous instruction.  If it's an EXTEND, we'll extract
         the immediate offset extension from it in mips16_get_imm.  */
      prev_inst = inst;

      /* Fetch and decode the instruction.   */
      inst = (unsigned short) mips_fetch_instruction (cur_pc);
      if ((inst & 0xff00) == 0x6300	/* addiu sp */
	  || (inst & 0xff00) == 0xfb00)		/* daddiu sp */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 8, 1);
	  if (offset < 0)	/* negative stack adjustment? */
	    PROC_FRAME_OFFSET (&temp_proc_desc) -= offset;
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
	  PROC_REG_MASK (&temp_proc_desc) |= (1 << reg);
	  set_reg_offset (reg, sp + offset);
	}
      else if ((inst & 0xff00) == 0xf900)	/* sd reg,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 8, 0);
	  reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
	  PROC_REG_MASK (&temp_proc_desc) |= (1 << reg);
	  set_reg_offset (reg, sp + offset);
	}
      else if ((inst & 0xff00) == 0x6200)	/* sw $ra,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 4, 0);
	  PROC_REG_MASK (&temp_proc_desc) |= (1 << RA_REGNUM);
	  set_reg_offset (RA_REGNUM, sp + offset);
	}
      else if ((inst & 0xff00) == 0xfa00)	/* sd $ra,n($sp) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 8, 8, 0);
	  PROC_REG_MASK (&temp_proc_desc) |= (1 << RA_REGNUM);
	  set_reg_offset (RA_REGNUM, sp + offset);
	}
      else if (inst == 0x673d)	/* move $s1, $sp */
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
	  PROC_REG_MASK (&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, frame_addr + offset);
	}
      else if ((inst & 0xFF00) == 0x7900)	/* sd reg,offset($s1) */
	{
	  offset = mips16_get_imm (prev_inst, inst, 5, 8, 0);
	  reg = mips16_to_32_reg[(inst & 0xe0) >> 5];
	  PROC_REG_MASK (&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, frame_addr + offset);
	}
      else if ((inst & 0xf81f) == 0xe809 && (inst & 0x700) != 0x700)	/* entry */
	entry_inst = inst;	/* save for later processing */
      else if ((inst & 0xf800) == 0x1800)	/* jal(x) */
	cur_pc += MIPS16_INSTLEN;	/* 32-bit instruction */
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
      PROC_FRAME_OFFSET (&temp_proc_desc) += 32;

      /* Now we can calculate what the SP must have been at the
         start of the function prologue.  */
      sp += PROC_FRAME_OFFSET (&temp_proc_desc);

      /* Check if a0-a3 were saved in the caller's argument save area.  */
      for (reg = 4, offset = 0; reg < areg_count + 4; reg++)
	{
	  PROC_REG_MASK (&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, sp + offset);
	  offset += MIPS_SAVED_REGSIZE;
	}

      /* Check if the ra register was pushed on the stack.  */
      offset = -4;
      if (entry_inst & 0x20)
	{
	  PROC_REG_MASK (&temp_proc_desc) |= 1 << RA_REGNUM;
	  set_reg_offset (RA_REGNUM, sp + offset);
	  offset -= MIPS_SAVED_REGSIZE;
	}

      /* Check if the s0 and s1 registers were pushed on the stack.  */
      for (reg = 16; reg < sreg_count + 16; reg++)
	{
	  PROC_REG_MASK (&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, sp + offset);
	  offset -= MIPS_SAVED_REGSIZE;
	}
    }
}

static void
mips32_heuristic_proc_desc (CORE_ADDR start_pc, CORE_ADDR limit_pc,
			    struct frame_info *next_frame, CORE_ADDR sp)
{
  CORE_ADDR cur_pc;
  CORE_ADDR frame_addr = 0;	/* Value of $r30. Used by gcc for frame-pointer */
restart:
  memset (temp_saved_regs, '\0', SIZEOF_FRAME_SAVED_REGS);
  PROC_FRAME_OFFSET (&temp_proc_desc) = 0;
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

      if (high_word == 0x27bd	/* addiu $sp,$sp,-i */
	  || high_word == 0x23bd	/* addi $sp,$sp,-i */
	  || high_word == 0x67bd)	/* daddiu $sp,$sp,-i */
	{
	  if (low_word & 0x8000)	/* negative stack adjustment? */
	    PROC_FRAME_OFFSET (&temp_proc_desc) += 0x10000 - low_word;
	  else
	    /* Exit loop if a positive stack adjustment is found, which
	       usually means that the stack cleanup code in the function
	       epilogue is reached.  */
	    break;
	}
      else if ((high_word & 0xFFE0) == 0xafa0)	/* sw reg,offset($sp) */
	{
	  PROC_REG_MASK (&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, sp + low_word);
	}
      else if ((high_word & 0xFFE0) == 0xffa0)	/* sd reg,offset($sp) */
	{
	  /* Irix 6.2 N32 ABI uses sd instructions for saving $gp and $ra,
	     but the register size used is only 32 bits. Make the address
	     for the saved register point to the lower 32 bits.  */
	  PROC_REG_MASK (&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, sp + low_word + 8 - MIPS_REGSIZE);
	}
      else if (high_word == 0x27be)	/* addiu $30,$sp,size */
	{
	  /* Old gcc frame, r30 is virtual frame pointer.  */
	  if ((long) low_word != PROC_FRAME_OFFSET (&temp_proc_desc))
	    frame_addr = sp + low_word;
	  else if (PROC_FRAME_REG (&temp_proc_desc) == SP_REGNUM)
	    {
	      unsigned alloca_adjust;
	      PROC_FRAME_REG (&temp_proc_desc) = 30;
	      frame_addr = read_next_frame_reg (next_frame, 30);
	      alloca_adjust = (unsigned) (frame_addr - (sp + low_word));
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
	      frame_addr = read_next_frame_reg (next_frame, 30);
	      alloca_adjust = (unsigned) (frame_addr - sp);
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
      else if ((high_word & 0xFFE0) == 0xafc0)	/* sw reg,offset($30) */
	{
	  PROC_REG_MASK (&temp_proc_desc) |= 1 << reg;
	  set_reg_offset (reg, frame_addr + low_word);
	}
    }
}

static mips_extra_func_info_t
heuristic_proc_desc (CORE_ADDR start_pc, CORE_ADDR limit_pc,
		     struct frame_info *next_frame, int cur_frame)
{
  CORE_ADDR sp;

  if (cur_frame)
    sp = read_next_frame_reg (next_frame, SP_REGNUM);
  else
    sp = 0;

  if (start_pc == 0)
    return NULL;
  memset (&temp_proc_desc, '\0', sizeof (temp_proc_desc));
  memset (&temp_saved_regs, '\0', SIZEOF_FRAME_SAVED_REGS);
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

struct mips_objfile_private
{
  bfd_size_type size;
  char *contents;
};

/* Global used to communicate between non_heuristic_proc_desc and
   compare_pdr_entries within qsort ().  */
static bfd *the_bfd;

static int
compare_pdr_entries (const void *a, const void *b)
{
  CORE_ADDR lhs = bfd_get_32 (the_bfd, (bfd_byte *) a);
  CORE_ADDR rhs = bfd_get_32 (the_bfd, (bfd_byte *) b);

  if (lhs < rhs)
    return -1;
  else if (lhs == rhs)
    return 0;
  else
    return 1;
}

static mips_extra_func_info_t
non_heuristic_proc_desc (CORE_ADDR pc, CORE_ADDR *addrptr)
{
  CORE_ADDR startaddr;
  mips_extra_func_info_t proc_desc;
  struct block *b = block_for_pc (pc);
  struct symbol *sym;
  struct obj_section *sec;
  struct mips_objfile_private *priv;

  if (PC_IN_CALL_DUMMY (pc, 0, 0))
    return NULL;

  find_pc_partial_function (pc, NULL, &startaddr, NULL);
  if (addrptr)
    *addrptr = startaddr;

  priv = NULL;

  sec = find_pc_section (pc);
  if (sec != NULL)
    {
      priv = (struct mips_objfile_private *) sec->objfile->obj_private;

      /* Search the ".pdr" section generated by GAS.  This includes most of
	 the information normally found in ECOFF PDRs.  */

      the_bfd = sec->objfile->obfd;
      if (priv == NULL
	  && (the_bfd->format == bfd_object
	      && bfd_get_flavour (the_bfd) == bfd_target_elf_flavour
	      && elf_elfheader (the_bfd)->e_ident[EI_CLASS] == ELFCLASS64))
	{
	  /* Right now GAS only outputs the address as a four-byte sequence.
	     This means that we should not bother with this method on 64-bit
	     targets (until that is fixed).  */

	  priv = obstack_alloc (& sec->objfile->psymbol_obstack,
				sizeof (struct mips_objfile_private));
	  priv->size = 0;
	  sec->objfile->obj_private = priv;
	}
      else if (priv == NULL)
	{
	  asection *bfdsec;

	  priv = obstack_alloc (& sec->objfile->psymbol_obstack,
				sizeof (struct mips_objfile_private));

	  bfdsec = bfd_get_section_by_name (sec->objfile->obfd, ".pdr");
	  if (bfdsec != NULL)
	    {
	      priv->size = bfd_section_size (sec->objfile->obfd, bfdsec);
	      priv->contents = obstack_alloc (& sec->objfile->psymbol_obstack,
					      priv->size);
	      bfd_get_section_contents (sec->objfile->obfd, bfdsec,
					priv->contents, 0, priv->size);

	      /* In general, the .pdr section is sorted.  However, in the
		 presence of multiple code sections (and other corner cases)
		 it can become unsorted.  Sort it so that we can use a faster
		 binary search.  */
	      qsort (priv->contents, priv->size / 32, 32, compare_pdr_entries);
	    }
	  else
	    priv->size = 0;

	  sec->objfile->obj_private = priv;
	}
      the_bfd = NULL;

      if (priv->size != 0)
	{
	  int low, mid, high;
	  char *ptr;

	  low = 0;
	  high = priv->size / 32;

	  do
	    {
	      CORE_ADDR pdr_pc;

	      mid = (low + high) / 2;

	      ptr = priv->contents + mid * 32;
	      pdr_pc = bfd_get_signed_32 (sec->objfile->obfd, ptr);
	      pdr_pc += ANOFFSET (sec->objfile->section_offsets,
				  SECT_OFF_TEXT (sec->objfile));
	      if (pdr_pc == startaddr)
		break;
	      if (pdr_pc > startaddr)
		high = mid;
	      else
		low = mid + 1;
	    }
	  while (low != high);

	  if (low != high)
	    {
	      struct symbol *sym = find_pc_function (pc);

	      /* Fill in what we need of the proc_desc.  */
	      proc_desc = (mips_extra_func_info_t)
		obstack_alloc (&sec->objfile->psymbol_obstack,
			       sizeof (struct mips_extra_func_info));
	      PROC_LOW_ADDR (proc_desc) = startaddr;

	      /* Only used for dummy frames.  */
	      PROC_HIGH_ADDR (proc_desc) = 0;

	      PROC_FRAME_OFFSET (proc_desc)
		= bfd_get_32 (sec->objfile->obfd, ptr + 20);
	      PROC_FRAME_REG (proc_desc) = bfd_get_32 (sec->objfile->obfd,
						       ptr + 24);
	      PROC_FRAME_ADJUST (proc_desc) = 0;
	      PROC_REG_MASK (proc_desc) = bfd_get_32 (sec->objfile->obfd,
						      ptr + 4);
	      PROC_FREG_MASK (proc_desc) = bfd_get_32 (sec->objfile->obfd,
						       ptr + 12);
	      PROC_REG_OFFSET (proc_desc) = bfd_get_32 (sec->objfile->obfd,
							ptr + 8);
	      PROC_FREG_OFFSET (proc_desc)
		= bfd_get_32 (sec->objfile->obfd, ptr + 16);
	      PROC_PC_REG (proc_desc) = bfd_get_32 (sec->objfile->obfd,
						    ptr + 28);
	      proc_desc->pdr.isym = (long) sym;

	      return proc_desc;
	    }
	}
    }

  if (b == NULL)
    return NULL;

  if (startaddr > BLOCK_START (b))
    {
      /* This is the "pathological" case referred to in a comment in
	 print_frame_info.  It might be better to move this check into
	 symbol reading.  */
      return NULL;
    }

  sym = lookup_symbol (MIPS_EFI_SYMBOL_NAME, b, LABEL_NAMESPACE, 0, NULL);

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
find_proc_desc (CORE_ADDR pc, struct frame_info *next_frame, int cur_frame)
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
	    PROC_DESC_IS_DUMMY (proc_desc) ? 0 : PROC_SYMBOL (proc_desc);

	  if (proc_symbol)
	    {
	      val = find_pc_line (BLOCK_START
				  (SYMBOL_BLOCK_VALUE (proc_symbol)),
				  0);
	      val.pc = val.end ? val.end : pc;
	    }
	  if (!proc_symbol || pc < val.pc)
	    {
	      mips_extra_func_info_t found_heuristic =
		heuristic_proc_desc (PROC_LOW_ADDR (proc_desc),
				     pc, next_frame, cur_frame);
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
	if (PROC_LOW_ADDR (&link->info) <= pc
	    && PROC_HIGH_ADDR (&link->info) > pc)
	  return &link->info;

      if (startaddr == 0)
	startaddr = heuristic_proc_start (pc);

      proc_desc =
	heuristic_proc_desc (startaddr, pc, next_frame, cur_frame);
    }
  return proc_desc;
}

static CORE_ADDR
get_frame_pointer (struct frame_info *frame,
		   mips_extra_func_info_t proc_desc)
{
  return ADDR_BITS_REMOVE (read_next_frame_reg (frame, 
						PROC_FRAME_REG (proc_desc)) +
			   PROC_FRAME_OFFSET (proc_desc) - 
			   PROC_FRAME_ADJUST (proc_desc));
}

static mips_extra_func_info_t cached_proc_desc;

static CORE_ADDR
mips_frame_chain (struct frame_info *frame)
{
  mips_extra_func_info_t proc_desc;
  CORE_ADDR tmp;
  CORE_ADDR saved_pc = FRAME_SAVED_PC (frame);

  if (saved_pc == 0 || inside_entry_file (saved_pc))
    return 0;

  /* Check if the PC is inside a call stub.  If it is, fetch the
     PC of the caller of that stub.  */
  if ((tmp = SKIP_TRAMPOLINE_CODE (saved_pc)) != 0)
    saved_pc = tmp;

  /* Look up the procedure descriptor for this PC.  */
  proc_desc = find_proc_desc (saved_pc, frame, 1);
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
      && !frame->signal_handler_caller
      /* Check if this is a call dummy frame.  */
      && frame->pc != CALL_DUMMY_ADDRESS ())
    return 0;
  else
    return get_frame_pointer (frame, proc_desc);
}

static void
mips_init_extra_frame_info (int fromleaf, struct frame_info *fci)
{
  int regnum;

  /* Use proc_desc calculated in frame_chain */
  mips_extra_func_info_t proc_desc =
    fci->next ? cached_proc_desc : find_proc_desc (fci->pc, fci->next, 1);

  fci->extra_info = (struct frame_extra_info *)
    frame_obstack_alloc (sizeof (struct frame_extra_info));

  fci->saved_regs = NULL;
  fci->extra_info->proc_desc =
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
				    (CORE_ADDR *) NULL, (CORE_ADDR *) NULL);
	  if (!PC_IN_SIGTRAMP (fci->pc, name))
	    {
	      frame_saved_regs_zalloc (fci);
	      memcpy (fci->saved_regs, temp_saved_regs, SIZEOF_FRAME_SAVED_REGS);
	      fci->saved_regs[PC_REGNUM]
		= fci->saved_regs[RA_REGNUM];
	      /* Set value of previous frame's stack pointer.  Remember that
	         saved_regs[SP_REGNUM] is special in that it contains the
		 value of the stack pointer register.  The other saved_regs
		 values are addresses (in the inferior) at which a given
		 register's value may be found.  */
	      fci->saved_regs[SP_REGNUM] = fci->frame;
	    }
	}

      /* hack: if argument regs are saved, guess these contain args */
      /* assume we can't tell how many args for now */
      fci->extra_info->num_args = -1;
      for (regnum = MIPS_LAST_ARG_REGNUM; regnum >= A0_REGNUM; regnum--)
	{
	  if (PROC_REG_MASK (proc_desc) & (1 << regnum))
	    {
	      fci->extra_info->num_args = regnum - A0_REGNUM + 1;
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
setup_arbitrary_frame (int argc, CORE_ADDR *argv)
{
  if (argc != 2)
    error ("MIPS frame specifications require two arguments: sp and pc");

  return create_new_frame (argv[0], argv[1]);
}

/* According to the current ABI, should the type be passed in a
   floating-point register (assuming that there is space)?  When there
   is no FPU, FP are not even considered as possibile candidates for
   FP registers and, consequently this returns false - forces FP
   arguments into integer registers. */

static int
fp_register_arg_p (enum type_code typecode, struct type *arg_type)
{
  return ((typecode == TYPE_CODE_FLT
	   || (MIPS_EABI
	       && (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION)
	       && TYPE_NFIELDS (arg_type) == 1
	       && TYPE_CODE (TYPE_FIELD_TYPE (arg_type, 0)) == TYPE_CODE_FLT))
	  && MIPS_FPU_TYPE != MIPS_FPU_NONE);
}

/* On o32, argument passing in GPRs depends on the alignment of the type being
   passed.  Return 1 if this type must be aligned to a doubleword boundary. */

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

/* Macros to round N up or down to the next A boundary; 
   A must be a power of two.  */

#define ROUND_DOWN(n,a) ((n) & ~((a)-1))
#define ROUND_UP(n,a) (((n)+(a)-1) & ~((a)-1))

static CORE_ADDR
mips_eabi_push_arguments (int nargs,
			  struct value **args,
			  CORE_ADDR sp,
			  int struct_return,
			  CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = ROUND_DOWN (sp, 16);
  struct_addr = ROUND_DOWN (struct_addr, 16);

  /* Now make space on the stack for the args.  We allocate more
     than necessary for EABI, because the first few arguments are
     passed in registers, but that's OK.  */
  for (argnum = 0; argnum < nargs; argnum++)
    len += ROUND_UP (TYPE_LENGTH (VALUE_TYPE (args[argnum])), 
		     MIPS_STACK_ARGSIZE);
  sp -= ROUND_UP (len, 16);

  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog, 
			"mips_eabi_push_arguments: sp=0x%s allocated %d\n",
			paddr_nz (sp), ROUND_UP (len, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = A0_REGNUM;
  float_argreg = FPA0_REGNUM;

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (struct_return)
    {
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_eabi_push_arguments: struct_return reg=%d 0x%s\n",
			    argreg, paddr_nz (struct_addr));
      write_register (argreg++, struct_addr);
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      char *val;
      char *valbuf = alloca (MAX_REGISTER_RAW_SIZE);
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (VALUE_TYPE (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_eabi_push_arguments: %d len=%d type=%d",
			    argnum + 1, len, (int) typecode);

      /* The EABI passes structures that do not fit in a register by
         reference.  */
      if (len > MIPS_SAVED_REGSIZE
	  && (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION))
	{
	  store_address (valbuf, MIPS_SAVED_REGSIZE, VALUE_ADDRESS (arg));
	  typecode = TYPE_CODE_PTR;
	  len = MIPS_SAVED_REGSIZE;
	  val = valbuf;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stdlog, " push");
	}
      else
	val = (char *) VALUE_CONTENTS (arg);

      /* 32-bit ABIs always start floating point arguments in an
         even-numbered floating point register.  Round the FP register
         up before the check to see if there are any FP registers
         left.  Non MIPS_EABI targets also pass the FP in the integer
         registers so also round up normal registers.  */
      if (!FP_REGISTER_DOUBLE
	  && fp_register_arg_p (typecode, arg_type))
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
      if (fp_register_arg_p (typecode, arg_type)
	  && float_argreg <= MIPS_LAST_FP_ARG_REGNUM)
	{
	  if (!FP_REGISTER_DOUBLE && len == 8)
	    {
	      int low_offset = TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? 4 : 0;
	      unsigned long regval;

	      /* Write the low word of the double to the even register(s).  */
	      regval = extract_unsigned_integer (val + low_offset, 4);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      write_register (float_argreg++, regval);

	      /* Write the high word of the double to the odd register(s).  */
	      regval = extract_unsigned_integer (val + 4 - low_offset, 4);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      write_register (float_argreg++, regval);
	    }
	  else
	    {
	      /* This is a floating point value that fits entirely
	         in a single register.  */
	      /* On 32 bit ABI's the float_argreg is further adjusted
                 above to ensure that it is even register aligned.  */
	      LONGEST regval = extract_unsigned_integer (val, len);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, len));
	      write_register (float_argreg++, regval);
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
	  int odd_sized_struct = ((len > MIPS_SAVED_REGSIZE) &&
				  (len % MIPS_SAVED_REGSIZE != 0));

	  /* Note: Floating-point values that didn't fit into an FP
             register are only written to memory.  */
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = 
		len < MIPS_SAVED_REGSIZE ? len : MIPS_SAVED_REGSIZE;

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " -- partial=%d",
				    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > MIPS_LAST_ARG_REGNUM
		  || odd_sized_struct
		  || fp_register_arg_p (typecode, arg_type))
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored? */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
		    {
		      if (MIPS_STACK_ARGSIZE == 8 &&
			  (typecode == TYPE_CODE_INT ||
			   typecode == TYPE_CODE_PTR ||
			   typecode == TYPE_CODE_FLT) && len <= 4)
			longword_offset = MIPS_STACK_ARGSIZE - len;
		      else if ((typecode == TYPE_CODE_STRUCT ||
				typecode == TYPE_CODE_UNION) &&
			       TYPE_LENGTH (arg_type) < MIPS_STACK_ARGSIZE)
			longword_offset = MIPS_STACK_ARGSIZE - len;
		    }

		  if (mips_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, " - stack_offset=0x%s",
					  paddr_nz (stack_offset));
		      fprintf_unfiltered (gdb_stdlog, " longword_offset=0x%s",
					  paddr_nz (longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      fprintf_unfiltered (gdb_stdlog, " @0x%s ", 
					  paddr_nz (addr));
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
	      if (argreg <= MIPS_LAST_ARG_REGNUM
		  && !fp_register_arg_p (typecode, arg_type))
		{
		  LONGEST regval = extract_unsigned_integer (val, partial_len);

		  if (mips_debug)
		    fprintf_filtered (gdb_stdlog, " - reg=%d val=%s",
				      argreg,
				      phex (regval, MIPS_SAVED_REGSIZE));
		  write_register (argreg, regval);
		  argreg++;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the the offset into the stack at which we
		 will copy the next parameter.

	         In the new EABI (and the NABI32), the stack_offset
	         only needs to be adjusted when it has been used.  */

	      if (stack_used_p)
		stack_offset += ROUND_UP (partial_len, MIPS_STACK_ARGSIZE);
	    }
	}
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog, "\n");
    }

  /* Return adjusted stack pointer.  */
  return sp;
}

/* N32/N64 version of push_arguments.  */

static CORE_ADDR
mips_n32n64_push_arguments (int nargs,
			    struct value **args,
			    CORE_ADDR sp,
			    int struct_return,
			    CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = ROUND_DOWN (sp, 16);
  struct_addr = ROUND_DOWN (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    len += ROUND_UP (TYPE_LENGTH (VALUE_TYPE (args[argnum])), 
		     MIPS_STACK_ARGSIZE);
  sp -= ROUND_UP (len, 16);

  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog, 
			"mips_n32n64_push_arguments: sp=0x%s allocated %d\n",
			paddr_nz (sp), ROUND_UP (len, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = A0_REGNUM;
  float_argreg = FPA0_REGNUM;

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (struct_return)
    {
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_n32n64_push_arguments: struct_return reg=%d 0x%s\n",
			    argreg, paddr_nz (struct_addr));
      write_register (argreg++, struct_addr);
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      char *val;
      char *valbuf = alloca (MAX_REGISTER_RAW_SIZE);
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (VALUE_TYPE (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_n32n64_push_arguments: %d len=%d type=%d",
			    argnum + 1, len, (int) typecode);

      val = (char *) VALUE_CONTENTS (arg);

      if (fp_register_arg_p (typecode, arg_type)
	  && float_argreg <= MIPS_LAST_FP_ARG_REGNUM)
	{
	  /* This is a floating point value that fits entirely
	     in a single register.  */
	  /* On 32 bit ABI's the float_argreg is further adjusted
	     above to ensure that it is even register aligned.  */
	  LONGEST regval = extract_unsigned_integer (val, len);
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				float_argreg, phex (regval, len));
	  write_register (float_argreg++, regval);

	  if (mips_debug)
	    fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				argreg, phex (regval, len));
	  write_register (argreg, regval);
	  argreg += 1;
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
	  int odd_sized_struct = ((len > MIPS_SAVED_REGSIZE) &&
				  (len % MIPS_SAVED_REGSIZE != 0));
	  /* Note: Floating-point values that didn't fit into an FP
             register are only written to memory.  */
	  while (len > 0)
	    {
	      /* Rememer if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = len < MIPS_SAVED_REGSIZE ? 
		len : MIPS_SAVED_REGSIZE;

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " -- partial=%d",
				    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > MIPS_LAST_ARG_REGNUM
		  || odd_sized_struct
		  || fp_register_arg_p (typecode, arg_type))
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored? */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
		    {
		      if (MIPS_STACK_ARGSIZE == 8 &&
			  (typecode == TYPE_CODE_INT ||
			   typecode == TYPE_CODE_PTR ||
			   typecode == TYPE_CODE_FLT) && len <= 4)
			longword_offset = MIPS_STACK_ARGSIZE - len;
		      else if ((typecode == TYPE_CODE_STRUCT ||
				typecode == TYPE_CODE_UNION) &&
			       TYPE_LENGTH (arg_type) < MIPS_STACK_ARGSIZE)
			longword_offset = MIPS_STACK_ARGSIZE - len;
		    }

		  if (mips_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, " - stack_offset=0x%s",
					  paddr_nz (stack_offset));
		      fprintf_unfiltered (gdb_stdlog, " longword_offset=0x%s",
					  paddr_nz (longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      fprintf_unfiltered (gdb_stdlog, " @0x%s ", 
					  paddr_nz (addr));
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
	      if (argreg <= MIPS_LAST_ARG_REGNUM
		  && !fp_register_arg_p (typecode, arg_type))
		{
		  LONGEST regval = extract_unsigned_integer (val, partial_len);

		  /* A non-floating-point argument being passed in a
		     general register.  If a struct or union, and if
		     the remaining length is smaller than the register
		     size, we have to adjust the register value on
		     big endian targets.

		     It does not seem to be necessary to do the
		     same for integral types.

		     cagney/2001-07-23: gdb/179: Also, GCC, when
		     outputting LE O32 with sizeof (struct) <
		     MIPS_SAVED_REGSIZE, generates a left shift as
		     part of storing the argument in a register a
		     register (the left shift isn't generated when
		     sizeof (struct) >= MIPS_SAVED_REGSIZE).  Since it
		     is quite possible that this is GCC contradicting
		     the LE/O32 ABI, GDB has not been adjusted to
		     accommodate this.  Either someone needs to
		     demonstrate that the LE/O32 ABI specifies such a
		     left shift OR this new ABI gets identified as
		     such and GDB gets tweaked accordingly.  */

		  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
		      && partial_len < MIPS_SAVED_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT ||
			  typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS_SAVED_REGSIZE - partial_len) *
				TARGET_CHAR_BIT);

		  if (mips_debug)
		    fprintf_filtered (gdb_stdlog, " - reg=%d val=%s",
				      argreg,
				      phex (regval, MIPS_SAVED_REGSIZE));
		  write_register (argreg, regval);
		  argreg++;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the the offset into the stack at which we
		 will copy the next parameter.

	         In N32 (N64?), the stack_offset only needs to be
	         adjusted when it has been used.  */

	      if (stack_used_p)
		stack_offset += ROUND_UP (partial_len, MIPS_STACK_ARGSIZE);
	    }
	}
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog, "\n");
    }

  /* Return adjusted stack pointer.  */
  return sp;
}

/* O32 version of push_arguments.  */

static CORE_ADDR
mips_o32_push_arguments (int nargs,
			 struct value **args,
			 CORE_ADDR sp,
			 int struct_return,
			 CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = ROUND_DOWN (sp, 16);
  struct_addr = ROUND_DOWN (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    len += ROUND_UP (TYPE_LENGTH (VALUE_TYPE (args[argnum])), 
		     MIPS_STACK_ARGSIZE);
  sp -= ROUND_UP (len, 16);

  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog, 
			"mips_o32_push_arguments: sp=0x%s allocated %d\n",
			paddr_nz (sp), ROUND_UP (len, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = A0_REGNUM;
  float_argreg = FPA0_REGNUM;

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (struct_return)
    {
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_o32_push_arguments: struct_return reg=%d 0x%s\n",
			    argreg, paddr_nz (struct_addr));
      write_register (argreg++, struct_addr);
      stack_offset += MIPS_STACK_ARGSIZE;
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      char *val;
      char *valbuf = alloca (MAX_REGISTER_RAW_SIZE);
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (VALUE_TYPE (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_o32_push_arguments: %d len=%d type=%d",
			    argnum + 1, len, (int) typecode);

      val = (char *) VALUE_CONTENTS (arg);

      /* 32-bit ABIs always start floating point arguments in an
         even-numbered floating point register.  Round the FP register
         up before the check to see if there are any FP registers
         left.  O32/O64 targets also pass the FP in the integer
         registers so also round up normal registers.  */
      if (!FP_REGISTER_DOUBLE
	  && fp_register_arg_p (typecode, arg_type))
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

      if (fp_register_arg_p (typecode, arg_type)
	  && float_argreg <= MIPS_LAST_FP_ARG_REGNUM)
	{
	  if (!FP_REGISTER_DOUBLE && len == 8)
	    {
	      int low_offset = TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? 4 : 0;
	      unsigned long regval;

	      /* Write the low word of the double to the even register(s).  */
	      regval = extract_unsigned_integer (val + low_offset, 4);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      write_register (float_argreg++, regval);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, 4));
	      write_register (argreg++, regval);

	      /* Write the high word of the double to the odd register(s).  */
	      regval = extract_unsigned_integer (val + 4 - low_offset, 4);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      write_register (float_argreg++, regval);

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, 4));
	      write_register (argreg++, regval);
	    }
	  else
	    {
	      /* This is a floating point value that fits entirely
	         in a single register.  */
	      /* On 32 bit ABI's the float_argreg is further adjusted
                 above to ensure that it is even register aligned.  */
	      LONGEST regval = extract_unsigned_integer (val, len);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, len));
	      write_register (float_argreg++, regval);
	      /* CAGNEY: 32 bit MIPS ABI's always reserve two FP
		 registers for each argument.  The below is (my
		 guess) to ensure that the corresponding integer
		 register has reserved the same space.  */
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, len));
	      write_register (argreg, regval);
	      argreg += FP_REGISTER_DOUBLE ? 1 : 2;
	    }
	  /* Reserve space for the FP register.  */
	  stack_offset += ROUND_UP (len, MIPS_STACK_ARGSIZE);
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
	  int odd_sized_struct = ((len > MIPS_SAVED_REGSIZE) &&
				  (len % MIPS_SAVED_REGSIZE != 0));
	  /* Structures should be aligned to eight bytes (even arg registers)
	     on MIPS_ABI_O32, if their first member has double precision.  */
	  if (MIPS_SAVED_REGSIZE < 8
	      && mips_type_needs_double_align (arg_type))
	    {
	      if ((argreg & 1))
	        argreg++;
	    }
	  /* Note: Floating-point values that didn't fit into an FP
             register are only written to memory.  */
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = 
		len < MIPS_SAVED_REGSIZE ? len : MIPS_SAVED_REGSIZE;

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " -- partial=%d",
				    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > MIPS_LAST_ARG_REGNUM
		  || odd_sized_struct
		  || fp_register_arg_p (typecode, arg_type))
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored? */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
		    {
		      if (MIPS_STACK_ARGSIZE == 8 &&
			  (typecode == TYPE_CODE_INT ||
			   typecode == TYPE_CODE_PTR ||
			   typecode == TYPE_CODE_FLT) && len <= 4)
			longword_offset = MIPS_STACK_ARGSIZE - len;
		    }

		  if (mips_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, " - stack_offset=0x%s",
					  paddr_nz (stack_offset));
		      fprintf_unfiltered (gdb_stdlog, " longword_offset=0x%s",
					  paddr_nz (longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      fprintf_unfiltered (gdb_stdlog, " @0x%s ", 
					  paddr_nz (addr));
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
	      if (argreg <= MIPS_LAST_ARG_REGNUM
		  && !fp_register_arg_p (typecode, arg_type))
		{
		  LONGEST regval = extract_signed_integer (val, partial_len);
		  /* Value may need to be sign extended, because 
		     MIPS_REGSIZE != MIPS_SAVED_REGSIZE.  */

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
		     MIPS_SAVED_REGSIZE, generates a left shift as
		     part of storing the argument in a register a
		     register (the left shift isn't generated when
		     sizeof (struct) >= MIPS_SAVED_REGSIZE).  Since it
		     is quite possible that this is GCC contradicting
		     the LE/O32 ABI, GDB has not been adjusted to
		     accommodate this.  Either someone needs to
		     demonstrate that the LE/O32 ABI specifies such a
		     left shift OR this new ABI gets identified as
		     such and GDB gets tweaked accordingly.  */

		  if (MIPS_SAVED_REGSIZE < 8
		      && TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
		      && partial_len < MIPS_SAVED_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT ||
			  typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS_SAVED_REGSIZE - partial_len) *
				TARGET_CHAR_BIT);

		  if (mips_debug)
		    fprintf_filtered (gdb_stdlog, " - reg=%d val=%s",
				      argreg,
				      phex (regval, MIPS_SAVED_REGSIZE));
		  write_register (argreg, regval);
		  argreg++;

		  /* Prevent subsequent floating point arguments from
		     being passed in floating point registers.  */
		  float_argreg = MIPS_LAST_FP_ARG_REGNUM + 1;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the the offset into the stack at which we
		 will copy the next parameter.

		 In older ABIs, the caller reserved space for
		 registers that contained arguments.  This was loosely
		 refered to as their "home".  Consequently, space is
		 always allocated.  */

	      stack_offset += ROUND_UP (partial_len, MIPS_STACK_ARGSIZE);
	    }
	}
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog, "\n");
    }

  /* Return adjusted stack pointer.  */
  return sp;
}

/* O64 version of push_arguments.  */

static CORE_ADDR
mips_o64_push_arguments (int nargs,
			 struct value **args,
			 CORE_ADDR sp,
			 int struct_return,
			 CORE_ADDR struct_addr)
{
  int argreg;
  int float_argreg;
  int argnum;
  int len = 0;
  int stack_offset = 0;

  /* First ensure that the stack and structure return address (if any)
     are properly aligned.  The stack has to be at least 64-bit
     aligned even on 32-bit machines, because doubles must be 64-bit
     aligned.  For n32 and n64, stack frames need to be 128-bit
     aligned, so we round to this widest known alignment.  */

  sp = ROUND_DOWN (sp, 16);
  struct_addr = ROUND_DOWN (struct_addr, 16);

  /* Now make space on the stack for the args.  */
  for (argnum = 0; argnum < nargs; argnum++)
    len += ROUND_UP (TYPE_LENGTH (VALUE_TYPE (args[argnum])), 
		     MIPS_STACK_ARGSIZE);
  sp -= ROUND_UP (len, 16);

  if (mips_debug)
    fprintf_unfiltered (gdb_stdlog, 
			"mips_o64_push_arguments: sp=0x%s allocated %d\n",
			paddr_nz (sp), ROUND_UP (len, 16));

  /* Initialize the integer and float register pointers.  */
  argreg = A0_REGNUM;
  float_argreg = FPA0_REGNUM;

  /* The struct_return pointer occupies the first parameter-passing reg.  */
  if (struct_return)
    {
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_o64_push_arguments: struct_return reg=%d 0x%s\n",
			    argreg, paddr_nz (struct_addr));
      write_register (argreg++, struct_addr);
      stack_offset += MIPS_STACK_ARGSIZE;
    }

  /* Now load as many as possible of the first arguments into
     registers, and push the rest onto the stack.  Loop thru args
     from first to last.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      char *val;
      char *valbuf = alloca (MAX_REGISTER_RAW_SIZE);
      struct value *arg = args[argnum];
      struct type *arg_type = check_typedef (VALUE_TYPE (arg));
      int len = TYPE_LENGTH (arg_type);
      enum type_code typecode = TYPE_CODE (arg_type);

      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "mips_o64_push_arguments: %d len=%d type=%d",
			    argnum + 1, len, (int) typecode);

      val = (char *) VALUE_CONTENTS (arg);

      /* 32-bit ABIs always start floating point arguments in an
         even-numbered floating point register.  Round the FP register
         up before the check to see if there are any FP registers
         left.  O32/O64 targets also pass the FP in the integer
         registers so also round up normal registers.  */
      if (!FP_REGISTER_DOUBLE
	  && fp_register_arg_p (typecode, arg_type))
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

      if (fp_register_arg_p (typecode, arg_type)
	  && float_argreg <= MIPS_LAST_FP_ARG_REGNUM)
	{
	  if (!FP_REGISTER_DOUBLE && len == 8)
	    {
	      int low_offset = TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? 4 : 0;
	      unsigned long regval;

	      /* Write the low word of the double to the even register(s).  */
	      regval = extract_unsigned_integer (val + low_offset, 4);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      write_register (float_argreg++, regval);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, 4));
	      write_register (argreg++, regval);

	      /* Write the high word of the double to the odd register(s).  */
	      regval = extract_unsigned_integer (val + 4 - low_offset, 4);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, 4));
	      write_register (float_argreg++, regval);

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, 4));
	      write_register (argreg++, regval);
	    }
	  else
	    {
	      /* This is a floating point value that fits entirely
	         in a single register.  */
	      /* On 32 bit ABI's the float_argreg is further adjusted
                 above to ensure that it is even register aligned.  */
	      LONGEST regval = extract_unsigned_integer (val, len);
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - fpreg=%d val=%s",
				    float_argreg, phex (regval, len));
	      write_register (float_argreg++, regval);
	      /* CAGNEY: 32 bit MIPS ABI's always reserve two FP
		 registers for each argument.  The below is (my
		 guess) to ensure that the corresponding integer
		 register has reserved the same space.  */
	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " - reg=%d val=%s",
				    argreg, phex (regval, len));
	      write_register (argreg, regval);
	      argreg += FP_REGISTER_DOUBLE ? 1 : 2;
	    }
	  /* Reserve space for the FP register.  */
	  stack_offset += ROUND_UP (len, MIPS_STACK_ARGSIZE);
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
	  int odd_sized_struct = ((len > MIPS_SAVED_REGSIZE) &&
				  (len % MIPS_SAVED_REGSIZE != 0));
	  /* Structures should be aligned to eight bytes (even arg registers)
	     on MIPS_ABI_O32, if their first member has double precision.  */
	  if (MIPS_SAVED_REGSIZE < 8
	      && mips_type_needs_double_align (arg_type))
	    {
	      if ((argreg & 1))
	        argreg++;
	    }
	  /* Note: Floating-point values that didn't fit into an FP
             register are only written to memory.  */
	  while (len > 0)
	    {
	      /* Remember if the argument was written to the stack.  */
	      int stack_used_p = 0;
	      int partial_len = 
		len < MIPS_SAVED_REGSIZE ? len : MIPS_SAVED_REGSIZE;

	      if (mips_debug)
		fprintf_unfiltered (gdb_stdlog, " -- partial=%d",
				    partial_len);

	      /* Write this portion of the argument to the stack.  */
	      if (argreg > MIPS_LAST_ARG_REGNUM
		  || odd_sized_struct
		  || fp_register_arg_p (typecode, arg_type))
		{
		  /* Should shorter than int integer values be
		     promoted to int before being stored? */
		  int longword_offset = 0;
		  CORE_ADDR addr;
		  stack_used_p = 1;
		  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
		    {
		      if (MIPS_STACK_ARGSIZE == 8 &&
			  (typecode == TYPE_CODE_INT ||
			   typecode == TYPE_CODE_PTR ||
			   typecode == TYPE_CODE_FLT) && len <= 4)
			longword_offset = MIPS_STACK_ARGSIZE - len;
		    }

		  if (mips_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, " - stack_offset=0x%s",
					  paddr_nz (stack_offset));
		      fprintf_unfiltered (gdb_stdlog, " longword_offset=0x%s",
					  paddr_nz (longword_offset));
		    }

		  addr = sp + stack_offset + longword_offset;

		  if (mips_debug)
		    {
		      int i;
		      fprintf_unfiltered (gdb_stdlog, " @0x%s ", 
					  paddr_nz (addr));
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
	      if (argreg <= MIPS_LAST_ARG_REGNUM
		  && !fp_register_arg_p (typecode, arg_type))
		{
		  LONGEST regval = extract_signed_integer (val, partial_len);
		  /* Value may need to be sign extended, because 
		     MIPS_REGSIZE != MIPS_SAVED_REGSIZE.  */

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
		     MIPS_SAVED_REGSIZE, generates a left shift as
		     part of storing the argument in a register a
		     register (the left shift isn't generated when
		     sizeof (struct) >= MIPS_SAVED_REGSIZE).  Since it
		     is quite possible that this is GCC contradicting
		     the LE/O32 ABI, GDB has not been adjusted to
		     accommodate this.  Either someone needs to
		     demonstrate that the LE/O32 ABI specifies such a
		     left shift OR this new ABI gets identified as
		     such and GDB gets tweaked accordingly.  */

		  if (MIPS_SAVED_REGSIZE < 8
		      && TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
		      && partial_len < MIPS_SAVED_REGSIZE
		      && (typecode == TYPE_CODE_STRUCT ||
			  typecode == TYPE_CODE_UNION))
		    regval <<= ((MIPS_SAVED_REGSIZE - partial_len) *
				TARGET_CHAR_BIT);

		  if (mips_debug)
		    fprintf_filtered (gdb_stdlog, " - reg=%d val=%s",
				      argreg,
				      phex (regval, MIPS_SAVED_REGSIZE));
		  write_register (argreg, regval);
		  argreg++;

		  /* Prevent subsequent floating point arguments from
		     being passed in floating point registers.  */
		  float_argreg = MIPS_LAST_FP_ARG_REGNUM + 1;
		}

	      len -= partial_len;
	      val += partial_len;

	      /* Compute the the offset into the stack at which we
		 will copy the next parameter.

		 In older ABIs, the caller reserved space for
		 registers that contained arguments.  This was loosely
		 refered to as their "home".  Consequently, space is
		 always allocated.  */

	      stack_offset += ROUND_UP (partial_len, MIPS_STACK_ARGSIZE);
	    }
	}
      if (mips_debug)
	fprintf_unfiltered (gdb_stdlog, "\n");
    }

  /* Return adjusted stack pointer.  */
  return sp;
}

static CORE_ADDR
mips_push_return_address (CORE_ADDR pc, CORE_ADDR sp)
{
  /* Set the return address register to point to the entry
     point of the program, where a breakpoint lies in wait.  */
  write_register (RA_REGNUM, CALL_DUMMY_ADDRESS ());
  return sp;
}

static void
mips_push_register (CORE_ADDR * sp, int regno)
{
  char *buffer = alloca (MAX_REGISTER_RAW_SIZE);
  int regsize;
  int offset;
  if (MIPS_SAVED_REGSIZE < REGISTER_RAW_SIZE (regno))
    {
      regsize = MIPS_SAVED_REGSIZE;
      offset = (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
		? REGISTER_RAW_SIZE (regno) - MIPS_SAVED_REGSIZE
		: 0);
    }
  else
    {
      regsize = REGISTER_RAW_SIZE (regno);
      offset = 0;
    }
  *sp -= regsize;
  read_register_gen (regno, buffer);
  write_memory (*sp, buffer + offset, regsize);
}

/* MASK(i,j) == (1<<i) + (1<<(i+1)) + ... + (1<<j)). Assume i<=j<(MIPS_NUMREGS-1). */
#define MASK(i,j) (((1 << ((j)+1))-1) ^ ((1 << (i))-1))

static void
mips_push_dummy_frame (void)
{
  int ireg;
  struct linked_proc_info *link = (struct linked_proc_info *)
  xmalloc (sizeof (struct linked_proc_info));
  mips_extra_func_info_t proc_desc = &link->info;
  CORE_ADDR sp = ADDR_BITS_REMOVE (read_signed_register (SP_REGNUM));
  CORE_ADDR old_sp = sp;
  link->next = linked_proc_desc_table;
  linked_proc_desc_table = link;

/* FIXME!   are these correct ? */
#define PUSH_FP_REGNUM 16	/* must be a register preserved across calls */
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
   *    Saved PC
   *    Saved MMHI, MMLO, FPC_CSR
   *    Saved R31
   *    Saved R28
   *    ...
   *    Saved R1
   *    Saved D18 (i.e. F19, F18)
   *    ...
   *    Saved D0 (i.e. F1, F0)
   *    Argument build area and stack arguments written via mips_push_arguments
   *  (low memory)
   */

  /* Save special registers (PC, MMHI, MMLO, FPC_CSR) */
  PROC_FRAME_REG (proc_desc) = PUSH_FP_REGNUM;
  PROC_FRAME_OFFSET (proc_desc) = 0;
  PROC_FRAME_ADJUST (proc_desc) = 0;
  mips_push_register (&sp, PC_REGNUM);
  mips_push_register (&sp, HI_REGNUM);
  mips_push_register (&sp, LO_REGNUM);
  mips_push_register (&sp, MIPS_FPU_TYPE == MIPS_FPU_NONE ? 0 : FCRCS_REGNUM);

  /* Save general CPU registers */
  PROC_REG_MASK (proc_desc) = GEN_REG_SAVE_MASK;
  /* PROC_REG_OFFSET is the offset of the first saved register from FP.  */
  PROC_REG_OFFSET (proc_desc) = sp - old_sp - MIPS_SAVED_REGSIZE;
  for (ireg = 32; --ireg >= 0;)
    if (PROC_REG_MASK (proc_desc) & (1 << ireg))
      mips_push_register (&sp, ireg);

  /* Save floating point registers starting with high order word */
  PROC_FREG_MASK (proc_desc) =
    MIPS_FPU_TYPE == MIPS_FPU_DOUBLE ? FLOAT_REG_SAVE_MASK
    : MIPS_FPU_TYPE == MIPS_FPU_SINGLE ? FLOAT_SINGLE_REG_SAVE_MASK : 0;
  /* PROC_FREG_OFFSET is the offset of the first saved *double* register
     from FP.  */
  PROC_FREG_OFFSET (proc_desc) = sp - old_sp - 8;
  for (ireg = 32; --ireg >= 0;)
    if (PROC_FREG_MASK (proc_desc) & (1 << ireg))
      mips_push_register (&sp, ireg + FP0_REGNUM);

  /* Update the frame pointer for the call dummy and the stack pointer.
     Set the procedure's starting and ending addresses to point to the
     call dummy address at the entry point.  */
  write_register (PUSH_FP_REGNUM, old_sp);
  write_register (SP_REGNUM, sp);
  PROC_LOW_ADDR (proc_desc) = CALL_DUMMY_ADDRESS ();
  PROC_HIGH_ADDR (proc_desc) = CALL_DUMMY_ADDRESS () + 4;
  SET_PROC_DESC_IS_DUMMY (proc_desc);
  PROC_PC_REG (proc_desc) = RA_REGNUM;
}

static void
mips_pop_frame (void)
{
  register int regnum;
  struct frame_info *frame = get_current_frame ();
  CORE_ADDR new_sp = FRAME_FP (frame);

  mips_extra_func_info_t proc_desc = frame->extra_info->proc_desc;

  write_register (PC_REGNUM, FRAME_SAVED_PC (frame));
  if (frame->saved_regs == NULL)
    FRAME_INIT_SAVED_REGS (frame);
  for (regnum = 0; regnum < NUM_REGS; regnum++)
    {
      if (regnum != SP_REGNUM && regnum != PC_REGNUM
	  && frame->saved_regs[regnum])
	write_register (regnum,
			read_memory_integer (frame->saved_regs[regnum],
					     MIPS_SAVED_REGSIZE));
    }

  write_register (SP_REGNUM, new_sp);
  flush_cached_frames ();

  if (proc_desc && PROC_DESC_IS_DUMMY (proc_desc))
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

      xfree (pi_ptr);

      write_register (HI_REGNUM,
		      read_memory_integer (new_sp - 2 * MIPS_SAVED_REGSIZE,
					   MIPS_SAVED_REGSIZE));
      write_register (LO_REGNUM,
		      read_memory_integer (new_sp - 3 * MIPS_SAVED_REGSIZE,
					   MIPS_SAVED_REGSIZE));
      if (MIPS_FPU_TYPE != MIPS_FPU_NONE)
	write_register (FCRCS_REGNUM,
			read_memory_integer (new_sp - 4 * MIPS_SAVED_REGSIZE,
					     MIPS_SAVED_REGSIZE));
    }
}

static void
mips_fix_call_dummy (char *dummy, CORE_ADDR pc, CORE_ADDR fun, int nargs, 
		     struct value **args, struct type *type, int gcc_p)
{
  write_register(T9_REGNUM, fun);
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
   the raw/cooked register interface is in place. (We need a general
   interface that can deal with dynamic saved register sizes -- fp
   regs could be 32 bits wide in one frame and 64 on the frame above
   and below).  */

static struct type *
mips_float_register_type (void)
{
  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
    return builtin_type_ieee_single_big;
  else
    return builtin_type_ieee_single_little;
}

static struct type *
mips_double_register_type (void)
{
  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
    return builtin_type_ieee_double_big;
  else
    return builtin_type_ieee_double_little;
}

/* Copy a 32-bit single-precision value from the current frame
   into rare_buffer.  */

static void
mips_read_fp_register_single (int regno, char *rare_buffer)
{
  int raw_size = REGISTER_RAW_SIZE (regno);
  char *raw_buffer = alloca (raw_size);

  if (!frame_register_read (selected_frame, regno, raw_buffer))
    error ("can't read register %d (%s)", regno, REGISTER_NAME (regno));
  if (raw_size == 8)
    {
      /* We have a 64-bit value for this register.  Find the low-order
	 32 bits.  */
      int offset;

      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
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
mips_read_fp_register_double (int regno, char *rare_buffer)
{
  int raw_size = REGISTER_RAW_SIZE (regno);

  if (raw_size == 8 && !mips2_fp_compat ())
    {
      /* We have a 64-bit value for this register, and we should use
	 all 64 bits.  */
      if (!frame_register_read (selected_frame, regno, rare_buffer))
	error ("can't read register %d (%s)", regno, REGISTER_NAME (regno));
    }
  else
    {
      if ((regno - FP0_REGNUM) & 1)
	internal_error (__FILE__, __LINE__,
			"mips_read_fp_register_double: bad access to "
			"odd-numbered FP register");

      /* mips_read_fp_register_single will find the correct 32 bits from
	 each register.  */
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	{
	  mips_read_fp_register_single (regno, rare_buffer + 4);
	  mips_read_fp_register_single (regno + 1, rare_buffer);
	}
      else
	{
	  mips_read_fp_register_single (regno, rare_buffer);
	  mips_read_fp_register_single (regno + 1, rare_buffer + 4);
	}
    }
}

static void
mips_print_register (int regnum, int all)
{
  char *raw_buffer = alloca (MAX_REGISTER_RAW_SIZE);

  /* Get the data in raw format.  */
  if (!frame_register_read (selected_frame, regnum, raw_buffer))
    {
      printf_filtered ("%s: [Invalid]", REGISTER_NAME (regnum));
      return;
    }

  /* If we have a actual 32-bit floating point register (or we are in
     32-bit compatibility mode), and the register is even-numbered,
     also print it as a double (spanning two registers).  */
  if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT
      && (REGISTER_RAW_SIZE (regnum) == 4
	  || mips2_fp_compat ())
      && !((regnum - FP0_REGNUM) & 1))
    {
      char *dbuffer = alloca (2 * MAX_REGISTER_RAW_SIZE);

      mips_read_fp_register_double (regnum, dbuffer);

      printf_filtered ("(d%d: ", regnum - FP0_REGNUM);
      val_print (mips_double_register_type (), dbuffer, 0, 0,
		 gdb_stdout, 0, 1, 0, Val_pretty_default);
      printf_filtered ("); ");
    }
  fputs_filtered (REGISTER_NAME (regnum), gdb_stdout);

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
    if (REGISTER_RAW_SIZE (regnum) == 8 && !mips2_fp_compat ())
      {
	/* We have a meaningful 64-bit value in this register.  Show
	   it as a 32-bit float and a 64-bit double.  */
	int offset = 4 * (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG);

	printf_filtered (" (float) ");
	val_print (mips_float_register_type (), raw_buffer + offset, 0, 0,
		   gdb_stdout, 0, 1, 0, Val_pretty_default);
	printf_filtered (", (double) ");
	val_print (mips_double_register_type (), raw_buffer, 0, 0,
		   gdb_stdout, 0, 1, 0, Val_pretty_default);
      }
    else
      val_print (REGISTER_VIRTUAL_TYPE (regnum), raw_buffer, 0, 0,
		 gdb_stdout, 0, 1, 0, Val_pretty_default);
  /* Else print as integer in hex.  */
  else
    {
      int offset;

      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
        offset = REGISTER_RAW_SIZE (regnum) - REGISTER_VIRTUAL_SIZE (regnum);
      else
	offset = 0;

      print_scalar_formatted (raw_buffer + offset,
			      REGISTER_VIRTUAL_TYPE (regnum),
			      'x', 0, gdb_stdout);
    }
}

/* Replacement for generic do_registers_info.
   Print regs in pretty columns.  */

static int
do_fp_register_row (int regnum)
{				/* do values for FP (float) regs */
  char *raw_buffer;
  double doub, flt1, flt2;	/* doubles extracted from raw hex data */
  int inv1, inv2, inv3;

  raw_buffer = (char *) alloca (2 * REGISTER_RAW_SIZE (FP0_REGNUM));

  if (REGISTER_RAW_SIZE (regnum) == 4 || mips2_fp_compat ())
    {
      /* 4-byte registers: we can fit two registers per row.  */
      /* Also print every pair of 4-byte regs as an 8-byte double.  */
      mips_read_fp_register_single (regnum, raw_buffer);
      flt1 = unpack_double (mips_float_register_type (), raw_buffer, &inv1);

      mips_read_fp_register_single (regnum + 1, raw_buffer);
      flt2 = unpack_double (mips_float_register_type (), raw_buffer, &inv2);

      mips_read_fp_register_double (regnum, raw_buffer);
      doub = unpack_double (mips_double_register_type (), raw_buffer, &inv3);

      printf_filtered (" %-5s", REGISTER_NAME (regnum));
      if (inv1)
	printf_filtered (": <invalid float>");
      else
	printf_filtered ("%-17.9g", flt1);

      printf_filtered (" %-5s", REGISTER_NAME (regnum + 1));
      if (inv2)
	printf_filtered (": <invalid float>");
      else
	printf_filtered ("%-17.9g", flt2);

      printf_filtered (" dbl: ");
      if (inv3)
	printf_filtered ("<invalid double>");
      else
	printf_filtered ("%-24.17g", doub);
      printf_filtered ("\n");

      /* may want to do hex display here (future enhancement) */
      regnum += 2;
    }
  else
    {
      /* Eight byte registers: print each one as float AND as double.  */
      mips_read_fp_register_single (regnum, raw_buffer);
      flt1 = unpack_double (mips_double_register_type (), raw_buffer, &inv1);

      mips_read_fp_register_double (regnum, raw_buffer);
      doub = unpack_double (mips_double_register_type (), raw_buffer, &inv3);

      printf_filtered (" %-5s: ", REGISTER_NAME (regnum));
      if (inv1)
	printf_filtered ("<invalid float>");
      else
	printf_filtered ("flt: %-17.9g", flt1);

      printf_filtered (" dbl: ");
      if (inv3)
	printf_filtered ("<invalid double>");
      else
	printf_filtered ("%-24.17g", doub);

      printf_filtered ("\n");
      /* may want to do hex display here (future enhancement) */
      regnum++;
    }
  return regnum;
}

/* Print a row's worth of GP (int) registers, with name labels above */

static int
do_gp_register_row (int regnum)
{
  /* do values for GP (int) regs */
  char *raw_buffer = alloca (MAX_REGISTER_RAW_SIZE);
  int ncols = (MIPS_REGSIZE == 8 ? 4 : 8);	/* display cols per row */
  int col, byte;
  int start_regnum = regnum;
  int numregs = NUM_REGS;


  /* For GP registers, we print a separate row of names above the vals */
  printf_filtered ("     ");
  for (col = 0; col < ncols && regnum < numregs; regnum++)
    {
      if (*REGISTER_NAME (regnum) == '\0')
	continue;		/* unused register */
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
	break;			/* end the row: reached FP register */
      printf_filtered (MIPS_REGSIZE == 8 ? "%17s" : "%9s",
		       REGISTER_NAME (regnum));
      col++;
    }
  printf_filtered (start_regnum < MIPS_NUMREGS ? "\n R%-4d" : "\n      ",
		   start_regnum);	/* print the R0 to R31 names */

  regnum = start_regnum;	/* go back to start of row */
  /* now print the values in hex, 4 or 8 to the row */
  for (col = 0; col < ncols && regnum < numregs; regnum++)
    {
      if (*REGISTER_NAME (regnum) == '\0')
	continue;		/* unused register */
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
	break;			/* end row: reached FP register */
      /* OK: get the data in raw format.  */
      if (!frame_register_read (selected_frame, regnum, raw_buffer))
	error ("can't read register %d (%s)", regnum, REGISTER_NAME (regnum));
      /* pad small registers */
      for (byte = 0; byte < (MIPS_REGSIZE - REGISTER_VIRTUAL_SIZE (regnum)); byte++)
	printf_filtered ("  ");
      /* Now print the register value in hex, endian order. */
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
	for (byte = REGISTER_RAW_SIZE (regnum) - REGISTER_VIRTUAL_SIZE (regnum);
	     byte < REGISTER_RAW_SIZE (regnum);
	     byte++)
	  printf_filtered ("%02x", (unsigned char) raw_buffer[byte]);
      else
	for (byte = REGISTER_VIRTUAL_SIZE (regnum) - 1;
	     byte >= 0;
	     byte--)
	  printf_filtered ("%02x", (unsigned char) raw_buffer[byte]);
      printf_filtered (" ");
      col++;
    }
  if (col > 0)			/* ie. if we actually printed anything... */
    printf_filtered ("\n");

  return regnum;
}

/* MIPS_DO_REGISTERS_INFO(): called by "info register" command */

static void
mips_do_registers_info (int regnum, int fpregs)
{
  if (regnum != -1)		/* do one specified register */
    {
      if (*(REGISTER_NAME (regnum)) == '\0')
	error ("Not a valid register for the current processor type");

      mips_print_register (regnum, 0);
      printf_filtered ("\n");
    }
  else
    /* do all (or most) registers */
    {
      regnum = 0;
      while (regnum < NUM_REGS)
	{
	  if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT)
	    if (fpregs)		/* true for "INFO ALL-REGISTERS" command */
	      regnum = do_fp_register_row (regnum);	/* FP regs */
	    else
	      regnum += MIPS_NUMREGS;	/* skip floating point regs */
	  else
	    regnum = do_gp_register_row (regnum);	/* GP (int) regs */
	}
    }
}

/* Is this a branch with a delay slot?  */

static int is_delayed (unsigned long);

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

int
mips_step_skips_delay (CORE_ADDR pc)
{
  char buf[MIPS_INSTLEN];

  /* There is no branch delay slot on MIPS16.  */
  if (pc_is_mips16 (pc))
    return 0;

  if (target_read_memory (pc, buf, MIPS_INSTLEN) != 0)
    /* If error reading memory, guess that it is not a delayed branch.  */
    return 0;
  return is_delayed ((unsigned long) extract_unsigned_integer (buf, MIPS_INSTLEN));
}


/* Skip the PC past function prologue instructions (32-bit version).
   This is a helper function for mips_skip_prologue.  */

static CORE_ADDR
mips32_skip_prologue (CORE_ADDR pc)
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

      if (high_word == 0x27bd	/* addiu $sp,$sp,offset */
	  || high_word == 0x67bd)	/* daddiu $sp,$sp,offset */
	seen_sp_adjust = 1;
      else if (inst == 0x03a1e823 ||	/* subu $sp,$sp,$at */
	       inst == 0x03a8e823)	/* subu $sp,$sp,$t0 */
	seen_sp_adjust = 1;
      else if (((inst & 0xFFE00000) == 0xAFA00000	/* sw reg,n($sp) */
		|| (inst & 0xFFE00000) == 0xFFA00000)	/* sd reg,n($sp) */
	       && (inst & 0x001F0000))	/* reg != $zero */
	continue;

      else if ((inst & 0xFFE00000) == 0xE7A00000)	/* swc1 freg,n($sp) */
	continue;
      else if ((inst & 0xF3E00000) == 0xA3C00000 && (inst & 0x001F0000))
	/* sx reg,n($s8) */
	continue;		/* reg != $zero */

      /* move $s8,$sp.  With different versions of gas this will be either
         `addu $s8,$sp,$zero' or `or $s8,$sp,$zero' or `daddu s8,sp,$0'.
         Accept any one of these.  */
      else if (inst == 0x03A0F021 || inst == 0x03a0f025 || inst == 0x03a0f02d)
	continue;

      else if ((inst & 0xFF9F07FF) == 0x00800021)	/* move reg,$a0-$a3 */
	continue;
      else if (high_word == 0x3c1c)	/* lui $gp,n */
	continue;
      else if (high_word == 0x279c)	/* addiu $gp,$gp,n */
	continue;
      else if (inst == 0x0399e021	/* addu $gp,$gp,$t9 */
	       || inst == 0x033ce021)	/* addu $gp,$t9,$gp */
	continue;
      /* The following instructions load $at or $t0 with an immediate
         value in preparation for a stack adjustment via
         subu $sp,$sp,[$at,$t0]. These instructions could also initialize
         a local variable, so we accept them only before a stack adjustment
         instruction was seen.  */
      else if (!seen_sp_adjust)
	{
	  if (high_word == 0x3c01 ||	/* lui $at,n */
	      high_word == 0x3c08)	/* lui $t0,n */
	    {
	      load_immediate_bytes += MIPS_INSTLEN;	/* FIXME!! */
	      continue;
	    }
	  else if (high_word == 0x3421 ||	/* ori $at,$at,n */
		   high_word == 0x3508 ||	/* ori $t0,$t0,n */
		   high_word == 0x3401 ||	/* ori $at,$zero,n */
		   high_word == 0x3408)		/* ori $t0,$zero,n */
	    {
	      load_immediate_bytes += MIPS_INSTLEN;	/* FIXME!! */
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
mips16_skip_prologue (CORE_ADDR pc)
{
  CORE_ADDR end_pc;
  int extend_bytes = 0;
  int prev_extend_bytes;

  /* Table of instructions likely to be found in a function prologue.  */
  static struct
    {
      unsigned short inst;
      unsigned short mask;
    }
  table[] =
  {
    {
      0x6300, 0xff00
    }
    ,				/* addiu $sp,offset */
    {
      0xfb00, 0xff00
    }
    ,				/* daddiu $sp,offset */
    {
      0xd000, 0xf800
    }
    ,				/* sw reg,n($sp) */
    {
      0xf900, 0xff00
    }
    ,				/* sd reg,n($sp) */
    {
      0x6200, 0xff00
    }
    ,				/* sw $ra,n($sp) */
    {
      0xfa00, 0xff00
    }
    ,				/* sd $ra,n($sp) */
    {
      0x673d, 0xffff
    }
    ,				/* move $s1,sp */
    {
      0xd980, 0xff80
    }
    ,				/* sw $a0-$a3,n($s1) */
    {
      0x6704, 0xff1c
    }
    ,				/* move reg,$a0-$a3 */
    {
      0xe809, 0xf81f
    }
    ,				/* entry pseudo-op */
    {
      0x0100, 0xff00
    }
    ,				/* addiu $s1,$sp,n */
    {
      0, 0
    }				/* end of table marker */
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
      if ((inst & 0xf800) == 0xf000)	/* extend */
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
      if (table[i].mask != 0)	/* it was in table? */
	continue;		/* ignore it */
      else
	/* non-prologue */
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

static CORE_ADDR
mips_skip_prologue (CORE_ADDR pc)
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
    return mips16_skip_prologue (pc);
  else
    return mips32_skip_prologue (pc);
}

/* Determine how a return value is stored within the MIPS register
   file, given the return type `valtype'. */

struct return_value_word
{
  int len;
  int reg;
  int reg_offset;
  int buf_offset;
};

static void
return_value_location (struct type *valtype,
		       struct return_value_word *hi,
		       struct return_value_word *lo)
{
  int len = TYPE_LENGTH (valtype);

  if (TYPE_CODE (valtype) == TYPE_CODE_FLT
      && ((MIPS_FPU_TYPE == MIPS_FPU_DOUBLE && (len == 4 || len == 8))
	  || (MIPS_FPU_TYPE == MIPS_FPU_SINGLE && len == 4)))
    {
      if (!FP_REGISTER_DOUBLE && len == 8)
	{
	  /* We need to break a 64bit float in two 32 bit halves and
	     spread them across a floating-point register pair. */
	  lo->buf_offset = TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? 4 : 0;
	  hi->buf_offset = TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? 0 : 4;
	  lo->reg_offset = ((TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
			     && REGISTER_RAW_SIZE (FP0_REGNUM) == 8)
			    ? 4 : 0);
	  hi->reg_offset = lo->reg_offset;
	  lo->reg = FP0_REGNUM + 0;
	  hi->reg = FP0_REGNUM + 1;
	  lo->len = 4;
	  hi->len = 4;
	}
      else
	{
	  /* The floating point value fits in a single floating-point
	     register. */
	  lo->reg_offset = ((TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
			     && REGISTER_RAW_SIZE (FP0_REGNUM) == 8
			     && len == 4)
			    ? 4 : 0);
	  lo->reg = FP0_REGNUM;
	  lo->len = len;
	  lo->buf_offset = 0;
	  hi->len = 0;
	  hi->reg_offset = 0;
	  hi->buf_offset = 0;
	  hi->reg = 0;
	}
    }
  else
    {
      /* Locate a result possibly spread across two registers. */
      int regnum = 2;
      lo->reg = regnum + 0;
      hi->reg = regnum + 1;
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
	  && len < MIPS_SAVED_REGSIZE)
	{
	  /* "un-left-justify" the value in the low register */
	  lo->reg_offset = MIPS_SAVED_REGSIZE - len;
	  lo->len = len;
	  hi->reg_offset = 0;
	  hi->len = 0;
	}
      else if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
	       && len > MIPS_SAVED_REGSIZE	/* odd-size structs */
	       && len < MIPS_SAVED_REGSIZE * 2
	       && (TYPE_CODE (valtype) == TYPE_CODE_STRUCT ||
		   TYPE_CODE (valtype) == TYPE_CODE_UNION))
	{
	  /* "un-left-justify" the value spread across two registers. */
	  lo->reg_offset = 2 * MIPS_SAVED_REGSIZE - len;
	  lo->len = MIPS_SAVED_REGSIZE - lo->reg_offset;
	  hi->reg_offset = 0;
	  hi->len = len - lo->len;
	}
      else
	{
	  /* Only perform a partial copy of the second register. */
	  lo->reg_offset = 0;
	  hi->reg_offset = 0;
	  if (len > MIPS_SAVED_REGSIZE)
	    {
	      lo->len = MIPS_SAVED_REGSIZE;
	      hi->len = len - MIPS_SAVED_REGSIZE;
	    }
	  else
	    {
	      lo->len = len;
	      hi->len = 0;
	    }
	}
      if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG
	  && REGISTER_RAW_SIZE (regnum) == 8
	  && MIPS_SAVED_REGSIZE == 4)
	{
	  /* Account for the fact that only the least-signficant part
	     of the register is being used */
	  lo->reg_offset += 4;
	  hi->reg_offset += 4;
	}
      lo->buf_offset = 0;
      hi->buf_offset = lo->len;
    }
}

/* Given a return value in `regbuf' with a type `valtype', extract and
   copy its value into `valbuf'. */

static void
mips_eabi_extract_return_value (struct type *valtype,
				char regbuf[REGISTER_BYTES],
				char *valbuf)
{
  struct return_value_word lo;
  struct return_value_word hi;
  return_value_location (valtype, &hi, &lo);

  memcpy (valbuf + lo.buf_offset,
	  regbuf + REGISTER_BYTE (lo.reg) + lo.reg_offset,
	  lo.len);

  if (hi.len > 0)
    memcpy (valbuf + hi.buf_offset,
	    regbuf + REGISTER_BYTE (hi.reg) + hi.reg_offset,
	    hi.len);
}

static void
mips_o64_extract_return_value (struct type *valtype,
			       char regbuf[REGISTER_BYTES],
			       char *valbuf)
{
  struct return_value_word lo;
  struct return_value_word hi;
  return_value_location (valtype, &hi, &lo);

  memcpy (valbuf + lo.buf_offset,
	  regbuf + REGISTER_BYTE (lo.reg) + lo.reg_offset,
	  lo.len);

  if (hi.len > 0)
    memcpy (valbuf + hi.buf_offset,
	    regbuf + REGISTER_BYTE (hi.reg) + hi.reg_offset,
	    hi.len);
}

/* Given a return value in `valbuf' with a type `valtype', write it's
   value into the appropriate register. */

static void
mips_eabi_store_return_value (struct type *valtype, char *valbuf)
{
  char *raw_buffer = alloca (MAX_REGISTER_RAW_SIZE);
  struct return_value_word lo;
  struct return_value_word hi;
  return_value_location (valtype, &hi, &lo);

  memset (raw_buffer, 0, sizeof (raw_buffer));
  memcpy (raw_buffer + lo.reg_offset, valbuf + lo.buf_offset, lo.len);
  write_register_bytes (REGISTER_BYTE (lo.reg),
			raw_buffer,
			REGISTER_RAW_SIZE (lo.reg));

  if (hi.len > 0)
    {
      memset (raw_buffer, 0, sizeof (raw_buffer));
      memcpy (raw_buffer + hi.reg_offset, valbuf + hi.buf_offset, hi.len);
      write_register_bytes (REGISTER_BYTE (hi.reg),
			    raw_buffer,
			    REGISTER_RAW_SIZE (hi.reg));
    }
}

static void
mips_o64_store_return_value (struct type *valtype, char *valbuf)
{
  char *raw_buffer = alloca (MAX_REGISTER_RAW_SIZE);
  struct return_value_word lo;
  struct return_value_word hi;
  return_value_location (valtype, &hi, &lo);

  memset (raw_buffer, 0, sizeof (raw_buffer));
  memcpy (raw_buffer + lo.reg_offset, valbuf + lo.buf_offset, lo.len);
  write_register_bytes (REGISTER_BYTE (lo.reg),
			raw_buffer,
			REGISTER_RAW_SIZE (lo.reg));

  if (hi.len > 0)
    {
      memset (raw_buffer, 0, sizeof (raw_buffer));
      memcpy (raw_buffer + hi.reg_offset, valbuf + hi.buf_offset, hi.len);
      write_register_bytes (REGISTER_BYTE (hi.reg),
			    raw_buffer,
			    REGISTER_RAW_SIZE (hi.reg));
    }
}

/* O32 ABI stuff.  */

static void
mips_o32_xfer_return_value (struct type *type,
			    struct regcache *regcache,
			    bfd_byte *in, const bfd_byte *out)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  if (TYPE_CODE (type) == TYPE_CODE_FLT
      && TYPE_LENGTH (type) == 4
      && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A single-precision floating-point value.  It fits in the
         least significant part of FP0.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $fp0\n");
      mips_xfer_register (regcache, FP0_REGNUM, TYPE_LENGTH (type),
			  TARGET_BYTE_ORDER, in, out, 0);
    }
  else if (TYPE_CODE (type) == TYPE_CODE_FLT
	   && TYPE_LENGTH (type) == 8
	   && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A double-precision floating-point value.  It fits in the
         least significant part of FP0/FP1 but with byte ordering
         based on the target (???).  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $fp0/$fp1\n");
      switch (TARGET_BYTE_ORDER)
	{
	case BFD_ENDIAN_LITTLE:
	  mips_xfer_register (regcache, FP0_REGNUM + 0, 4,
			      TARGET_BYTE_ORDER, in, out, 0);
	  mips_xfer_register (regcache, FP0_REGNUM + 1, 4,
			      TARGET_BYTE_ORDER, in, out, 4);
	  break;
	case BFD_ENDIAN_BIG:
	  mips_xfer_register (regcache, FP0_REGNUM + 1, 4,
			      TARGET_BYTE_ORDER, in, out, 0);
	  mips_xfer_register (regcache, FP0_REGNUM + 0, 4,
			      TARGET_BYTE_ORDER, in, out, 4);
	  break;
	default:
	  internal_error (__FILE__, __LINE__, "bad switch");
	}
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
      bfd_byte *reg = alloca (MAX_REGISTER_RAW_SIZE);
      int regnum;
      int field;
      for (field = 0, regnum = FP0_REGNUM;
	   field < TYPE_NFIELDS (type);
	   field++, regnum += 2)
	{
	  int offset = (FIELD_BITPOS (TYPE_FIELDS (type)[field])
			/ TARGET_CHAR_BIT);
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return float struct+%d\n", offset);
	  mips_xfer_register (regcache, regnum, TYPE_LENGTH (TYPE_FIELD_TYPE (type, field)),
			      TARGET_BYTE_ORDER, in, out, offset);
	}
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
      for (offset = 0, regnum = V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += REGISTER_RAW_SIZE (regnum), regnum++)
	{
	  int xfer = REGISTER_RAW_SIZE (regnum);
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return struct+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (regcache, regnum, xfer, BFD_ENDIAN_UNKNOWN,
			      in, out, offset);
	}
    }
#endif
  else
    {
      /* A scalar extract each part but least-significant-byte
         justified.  o32 thinks registers are 4 byte, regardless of
         the ISA.  mips_stack_argsize controls this.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += mips_stack_argsize (), regnum++)
	{
	  int xfer = mips_stack_argsize ();
	  int pos = 0;
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return scalar+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (regcache, regnum, xfer, TARGET_BYTE_ORDER,
			      in, out, offset);
	}
    }
}

static void
mips_o32_extract_return_value (struct type *type,
			       struct regcache *regcache,
			       void *valbuf)
{
  mips_o32_xfer_return_value (type, regcache, valbuf, NULL); 
}

static void
mips_o32_store_return_value (struct type *type, char *valbuf)
{
  mips_o32_xfer_return_value (type, current_regcache, NULL, valbuf); 
}

/* N32/N44 ABI stuff.  */

static void
mips_n32n64_xfer_return_value (struct type *type,
			       struct regcache *regcache,
			       bfd_byte *in, const bfd_byte *out)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  if (TYPE_CODE (type) == TYPE_CODE_FLT
      && tdep->mips_fpu_type != MIPS_FPU_NONE)
    {
      /* A floating-point value belongs in the least significant part
         of FP0.  */
      if (mips_debug)
	fprintf_unfiltered (gdb_stderr, "Return float in $fp0\n");
      mips_xfer_register (regcache, FP0_REGNUM, TYPE_LENGTH (type),
			  TARGET_BYTE_ORDER, in, out, 0);
    }
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
      bfd_byte *reg = alloca (MAX_REGISTER_RAW_SIZE);
      int regnum;
      int field;
      for (field = 0, regnum = FP0_REGNUM;
	   field < TYPE_NFIELDS (type);
	   field++, regnum += 2)
	{
	  int offset = (FIELD_BITPOS (TYPE_FIELDS (type)[field])
			/ TARGET_CHAR_BIT);
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return float struct+%d\n", offset);
	  mips_xfer_register (regcache, regnum, TYPE_LENGTH (TYPE_FIELD_TYPE (type, field)),
			      TARGET_BYTE_ORDER, in, out, offset);
	}
    }
  else if (TYPE_CODE (type) == TYPE_CODE_STRUCT
	   || TYPE_CODE (type) == TYPE_CODE_UNION)
    {
      /* A structure or union.  Extract the left justified value,
         regardless of the byte order.  I.e. DO NOT USE
         mips_xfer_lower.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += REGISTER_RAW_SIZE (regnum), regnum++)
	{
	  int xfer = REGISTER_RAW_SIZE (regnum);
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return struct+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (regcache, regnum, xfer, BFD_ENDIAN_UNKNOWN,
			      in, out, offset);
	}
    }
  else
    {
      /* A scalar extract each part but least-significant-byte
         justified.  */
      int offset;
      int regnum;
      for (offset = 0, regnum = V0_REGNUM;
	   offset < TYPE_LENGTH (type);
	   offset += REGISTER_RAW_SIZE (regnum), regnum++)
	{
	  int xfer = REGISTER_RAW_SIZE (regnum);
	  int pos = 0;
	  if (offset + xfer > TYPE_LENGTH (type))
	    xfer = TYPE_LENGTH (type) - offset;
	  if (mips_debug)
	    fprintf_unfiltered (gdb_stderr, "Return scalar+%d:%d in $%d\n",
				offset, xfer, regnum);
	  mips_xfer_register (regcache, regnum, xfer, TARGET_BYTE_ORDER,
			      in, out, offset);
	}
    }
}

static void
mips_n32n64_extract_return_value (struct type *type,
				  struct regcache *regcache,
				  void *valbuf)
{
  mips_n32n64_xfer_return_value (type, regcache, valbuf, NULL);
}

static void
mips_n32n64_store_return_value (struct type *type, char *valbuf)
{
  mips_n32n64_xfer_return_value (type, current_regcache, NULL, valbuf);
}

static void
mips_store_struct_return (CORE_ADDR addr, CORE_ADDR sp)
{
  /* Nothing to do -- push_arguments does all the work.  */
}

static CORE_ADDR
mips_extract_struct_value_address (struct regcache *ignore)
{
  /* FIXME: This will only work at random.  The caller passes the
     struct_return address in V0, but it is not preserved.  It may
     still be there, or this may be a random value.  */
  return read_register (V0_REGNUM);
}

/* Exported procedure: Is PC in the signal trampoline code */

static int
mips_pc_in_sigtramp (CORE_ADDR pc, char *ignore)
{
  if (sigtramp_address == 0)
    fixup_sigtramp ();
  return (pc >= sigtramp_address && pc < sigtramp_end);
}

/* Root of all "set mips "/"show mips " commands. This will eventually be
   used for all MIPS-specific commands.  */

static void
show_mips_command (char *args, int from_tty)
{
  help_list (showmipscmdlist, "show mips ", all_commands, gdb_stdout);
}

static void
set_mips_command (char *args, int from_tty)
{
  printf_unfiltered ("\"set mips\" must be followed by an appropriate subcommand.\n");
  help_list (setmipscmdlist, "set mips ", all_commands, gdb_stdout);
}

/* Commands to show/set the MIPS FPU type.  */

static void
show_mipsfpu_command (char *args, int from_tty)
{
  char *fpu;
  switch (MIPS_FPU_TYPE)
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
      internal_error (__FILE__, __LINE__, "bad switch");
    }
  if (mips_fpu_type_auto)
    printf_unfiltered ("The MIPS floating-point coprocessor is set automatically (currently %s)\n",
		       fpu);
  else
    printf_unfiltered ("The MIPS floating-point coprocessor is assumed to be %s\n",
		       fpu);
}


static void
set_mipsfpu_command (char *args, int from_tty)
{
  printf_unfiltered ("\"set mipsfpu\" must be followed by \"double\", \"single\",\"none\" or \"auto\".\n");
  show_mipsfpu_command (args, from_tty);
}

static void
set_mipsfpu_single_command (char *args, int from_tty)
{
  mips_fpu_type = MIPS_FPU_SINGLE;
  mips_fpu_type_auto = 0;
  gdbarch_tdep (current_gdbarch)->mips_fpu_type = MIPS_FPU_SINGLE;
}

static void
set_mipsfpu_double_command (char *args, int from_tty)
{
  mips_fpu_type = MIPS_FPU_DOUBLE;
  mips_fpu_type_auto = 0;
  gdbarch_tdep (current_gdbarch)->mips_fpu_type = MIPS_FPU_DOUBLE;
}

static void
set_mipsfpu_none_command (char *args, int from_tty)
{
  mips_fpu_type = MIPS_FPU_NONE;
  mips_fpu_type_auto = 0;
  gdbarch_tdep (current_gdbarch)->mips_fpu_type = MIPS_FPU_NONE;
}

static void
set_mipsfpu_auto_command (char *args, int from_tty)
{
  mips_fpu_type_auto = 1;
}

/* Command to set the processor type.  */

void
mips_set_processor_type_command (char *args, int from_tty)
{
  int i;

  if (tmp_mips_processor_type == NULL || *tmp_mips_processor_type == '\0')
    {
      printf_unfiltered ("The known MIPS processor types are as follows:\n\n");
      for (i = 0; mips_processor_type_table[i].name != NULL; ++i)
	printf_unfiltered ("%s\n", mips_processor_type_table[i].name);

      /* Restore the value.  */
      tmp_mips_processor_type = xstrdup (mips_processor_type);

      return;
    }

  if (!mips_set_processor_type (tmp_mips_processor_type))
    {
      error ("Unknown processor type `%s'.", tmp_mips_processor_type);
      /* Restore its value.  */
      tmp_mips_processor_type = xstrdup (mips_processor_type);
    }
}

static void
mips_show_processor_type_command (char *args, int from_tty)
{
}

/* Modify the actual processor type. */

static int
mips_set_processor_type (char *str)
{
  int i;

  if (str == NULL)
    return 0;

  for (i = 0; mips_processor_type_table[i].name != NULL; ++i)
    {
      if (strcasecmp (str, mips_processor_type_table[i].name) == 0)
	{
	  mips_processor_type = str;
	  mips_processor_reg_names = mips_processor_type_table[i].regnames;
	  return 1;
	  /* FIXME tweak fpu flag too */
	}
    }

  return 0;
}

/* Attempt to identify the particular processor model by reading the
   processor id.  */

char *
mips_read_processor_type (void)
{
  CORE_ADDR prid;

  prid = read_register (PRID_REGNUM);

  if ((prid & ~0xf) == 0x700)
    return savestring ("r3041", strlen ("r3041"));

  return NULL;
}

/* Just like reinit_frame_cache, but with the right arguments to be
   callable as an sfunc.  */

static void
reinit_frame_cache_sfunc (char *args, int from_tty,
			  struct cmd_list_element *c)
{
  reinit_frame_cache ();
}

int
gdb_print_insn_mips (bfd_vma memaddr, disassemble_info *info)
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
    info->mach = pc_is_mips16 (PROC_LOW_ADDR (proc_desc)) ?
      bfd_mach_mips16 : TM_PRINT_INSN_MACH;
  else
    info->mach = pc_is_mips16 (memaddr) ?
      bfd_mach_mips16 : TM_PRINT_INSN_MACH;

  /* Round down the instruction address to the appropriate boundary.  */
  memaddr &= (info->mach == bfd_mach_mips16 ? ~1 : ~3);

  /* Call the appropriate disassembler based on the target endian-ness.  */
  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
    return print_insn_big_mips (memaddr, info);
  else
    return print_insn_little_mips (memaddr, info);
}

/* Old-style breakpoint macros.
   The IDT board uses an unusual breakpoint value, and sometimes gets
   confused when it sees the usual MIPS breakpoint instruction.  */

#define BIG_BREAKPOINT {0, 0x5, 0, 0xd}
#define LITTLE_BREAKPOINT {0xd, 0, 0x5, 0}
#define PMON_BIG_BREAKPOINT {0, 0, 0, 0xd}
#define PMON_LITTLE_BREAKPOINT {0xd, 0, 0, 0}
#define IDT_BIG_BREAKPOINT {0, 0, 0x0a, 0xd}
#define IDT_LITTLE_BREAKPOINT {0xd, 0x0a, 0, 0}
#define MIPS16_BIG_BREAKPOINT {0xe8, 0xa5}
#define MIPS16_LITTLE_BREAKPOINT {0xa5, 0xe8}

/* This function implements the BREAKPOINT_FROM_PC macro.  It uses the program
   counter value to determine whether a 16- or 32-bit breakpoint should be
   used.  It returns a pointer to a string of bytes that encode a breakpoint
   instruction, stores the length of the string to *lenptr, and adjusts pc
   (if necessary) to point to the actual memory location where the
   breakpoint should be inserted.  */

static const unsigned char *
mips_breakpoint_from_pc (CORE_ADDR * pcptr, int *lenptr)
{
  if (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
    {
      if (pc_is_mips16 (*pcptr))
	{
	  static unsigned char mips16_big_breakpoint[] =
	    MIPS16_BIG_BREAKPOINT;
	  *pcptr = UNMAKE_MIPS16_ADDR (*pcptr);
	  *lenptr = sizeof (mips16_big_breakpoint);
	  return mips16_big_breakpoint;
	}
      else
	{
	  static unsigned char big_breakpoint[] = BIG_BREAKPOINT;
	  static unsigned char pmon_big_breakpoint[] = PMON_BIG_BREAKPOINT;
	  static unsigned char idt_big_breakpoint[] = IDT_BIG_BREAKPOINT;

	  *lenptr = sizeof (big_breakpoint);

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
	  static unsigned char mips16_little_breakpoint[] =
	    MIPS16_LITTLE_BREAKPOINT;
	  *pcptr = UNMAKE_MIPS16_ADDR (*pcptr);
	  *lenptr = sizeof (mips16_little_breakpoint);
	  return mips16_little_breakpoint;
	}
      else
	{
	  static unsigned char little_breakpoint[] = LITTLE_BREAKPOINT;
	  static unsigned char pmon_little_breakpoint[] =
	    PMON_LITTLE_BREAKPOINT;
	  static unsigned char idt_little_breakpoint[] =
	    IDT_LITTLE_BREAKPOINT;

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
   and the the target PC is in $2.  Otherwise this is effectively
   a return stub and the target PC is in $18.

   See the source code for the stubs in gcc/config/mips/mips16.S for
   gory details.

   This function implements the SKIP_TRAMPOLINE_CODE macro.
 */

static CORE_ADDR
mips_skip_stub (CORE_ADDR pc)
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
    return read_signed_register (RA_REGNUM);

  if (strncmp (name, "__mips16_call_stub_", 19) == 0)
    {
      /* If the PC is in __mips16_call_stub_{1..10}, this is a call stub
         and the target PC is in $2.  */
      if (name[19] >= '0' && name[19] <= '9')
	return read_signed_register (2);

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

	      CORE_ADDR target_pc = read_signed_register (2);
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
		    pc = (inst << 16) & 0xffff0000;	/* high word */
		  else if ((inst & 0xffff0000) == 0x24210000)	/* addiu $at */
		    return pc | (inst & 0xffff);	/* low word */
		}

	      /* Couldn't find the lui/addui pair, so return stub address.  */
	      return target_pc;
	    }
	  else
	    /* This is the 'return' part of a call stub.  The return
	       address is in $r18.  */
	    return read_signed_register (18);
	}
    }
  return 0;			/* not a stub */
}


/* Return non-zero if the PC is inside a call thunk (aka stub or trampoline).
   This implements the IN_SOLIB_CALL_TRAMPOLINE macro.  */

static int
mips_in_call_stub (CORE_ADDR pc, char *name)
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

  return 0;			/* not a stub */
}


/* Return non-zero if the PC is inside a return thunk (aka stub or trampoline).
   This implements the IN_SOLIB_RETURN_TRAMPOLINE macro.  */

static int
mips_in_return_stub (CORE_ADDR pc, char *name)
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

  return 0;			/* not a stub */
}


/* Return non-zero if the PC is in a library helper function that should
   be ignored.  This implements the IGNORE_HELPER_CALL macro.  */

int
mips_ignore_helper (CORE_ADDR pc)
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


/* Return a location where we can set a breakpoint that will be hit
   when an inferior function call returns.  This is normally the
   program's entry point.  Executables that don't have an entry
   point (e.g. programs in ROM) should define a symbol __CALL_DUMMY_ADDRESS
   whose address is the location where the breakpoint should be placed.  */

static CORE_ADDR
mips_call_dummy_address (void)
{
  struct minimal_symbol *sym;

  sym = lookup_minimal_symbol ("__CALL_DUMMY_ADDRESS", NULL, NULL);
  if (sym)
    return SYMBOL_VALUE_ADDRESS (sym);
  else
    return entry_point_address ();
}


/* If the current gcc for this target does not produce correct debugging
   information for float parameters, both prototyped and unprototyped, then
   define this macro.  This forces gdb to  always assume that floats are
   passed as doubles and then converted in the callee.

   For the mips chip, it appears that the debug info marks the parameters as
   floats regardless of whether the function is prototyped, but the actual
   values are passed as doubles for the non-prototyped case and floats for
   the prototyped case.  Thus we choose to make the non-prototyped case work
   for C and break the prototyped case, since the non-prototyped case is
   probably much more common.  (FIXME). */

static int
mips_coerce_float_to_double (struct type *formal, struct type *actual)
{
  return current_language->la_language == language_c;
}

/* When debugging a 64 MIPS target running a 32 bit ABI, the size of
   the register stored on the stack (32) is different to its real raw
   size (64).  The below ensures that registers are fetched from the
   stack using their ABI size and then stored into the RAW_BUFFER
   using their raw size.

   The alternative to adding this function would be to add an ABI
   macro - REGISTER_STACK_SIZE(). */

static void
mips_get_saved_register (char *raw_buffer,
			 int *optimized,
			 CORE_ADDR *addrp,
			 struct frame_info *frame,
			 int regnum,
			 enum lval_type *lval)
{
  CORE_ADDR addr;

  if (!target_has_registers)
    error ("No registers.");

  /* Normal systems don't optimize out things with register numbers.  */
  if (optimized != NULL)
    *optimized = 0;
  addr = find_saved_register (frame, regnum);
  if (addr != 0)
    {
      if (lval != NULL)
	*lval = lval_memory;
      if (regnum == SP_REGNUM)
	{
	  if (raw_buffer != NULL)
	    {
	      /* Put it back in target format.  */
	      store_address (raw_buffer, REGISTER_RAW_SIZE (regnum),
			     (LONGEST) addr);
	    }
	  if (addrp != NULL)
	    *addrp = 0;
	  return;
	}
      if (raw_buffer != NULL)
	{
	  LONGEST val;
	  if (regnum < 32)
	    /* Only MIPS_SAVED_REGSIZE bytes of GP registers are
               saved. */
	    val = read_memory_integer (addr, MIPS_SAVED_REGSIZE);
	  else
	    val = read_memory_integer (addr, REGISTER_RAW_SIZE (regnum));
	  store_address (raw_buffer, REGISTER_RAW_SIZE (regnum), val);
	}
    }
  else
    {
      if (lval != NULL)
	*lval = lval_register;
      addr = REGISTER_BYTE (regnum);
      if (raw_buffer != NULL)
	read_register_gen (regnum, raw_buffer);
    }
  if (addrp != NULL)
    *addrp = addr;
}

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

static CORE_ADDR
mips_saved_pc_after_call (struct frame_info *frame)
{
  return read_signed_register (RA_REGNUM);
}


/* Convert a dbx stab register number (from `r' declaration) to a gdb
   REGNUM */

static int
mips_stab_reg_to_regnum (int num)
{
  if (num < 32)
    return num;
  else
    return num + FP0_REGNUM - 38;
}

/* Convert a ecoff register number to a gdb REGNUM */

static int
mips_ecoff_reg_to_regnum (int num)
{
  if (num < 32)
    return num;
  else
    return num + FP0_REGNUM - 32;
}

/* Convert an integer into an address.  By first converting the value
   into a pointer and then extracting it signed, the address is
   guarenteed to be correctly sign extended.  */

static CORE_ADDR
mips_integer_to_address (struct type *type, void *buf)
{
  char *tmp = alloca (TYPE_LENGTH (builtin_type_void_data_ptr));
  LONGEST val = unpack_long (type, buf);
  store_signed_integer (tmp, TYPE_LENGTH (builtin_type_void_data_ptr), val);
  return extract_signed_integer (tmp,
				 TYPE_LENGTH (builtin_type_void_data_ptr));
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
    warning ("unsupported ABI %s.", name + 8);
}

static enum mips_abi
global_mips_abi (void)
{
  int i;

  for (i = 0; mips_abi_strings[i] != NULL; i++)
    if (mips_abi_strings[i] == mips_abi_string)
      return (enum mips_abi) i;

  internal_error (__FILE__, __LINE__,
		  "unknown ABI string");
}

static struct gdbarch *
mips_gdbarch_init (struct gdbarch_info info,
		   struct gdbarch_list *arches)
{
  static LONGEST mips_call_dummy_words[] =
  {0};
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  int elf_flags;
  enum mips_abi mips_abi, found_abi, wanted_abi;
  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;

  /* Reset the disassembly info, in case it was set to something
     non-default.  */
  tm_print_insn_info.flavour = bfd_target_unknown_flavour;
  tm_print_insn_info.arch = bfd_arch_unknown;
  tm_print_insn_info.mach = 0;

  elf_flags = 0;

  if (info.abfd)
    {
      /* First of all, extract the elf_flags, if available.  */
      if (bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
	elf_flags = elf_elfheader (info.abfd)->e_flags;

      /* Try to determine the OS ABI of the object we are loading.  If
	 we end up with `unknown', just leave it that way.  */
      osabi = gdbarch_lookup_osabi (info.abfd);
    }

  /* Check ELF_FLAGS to see if it specifies the ABI being used.  */
  switch ((elf_flags & EF_MIPS_ABI))
    {
    case E_MIPS_ABI_O32:
      mips_abi = MIPS_ABI_O32;
      break;
    case E_MIPS_ABI_O64:
      mips_abi = MIPS_ABI_O64;
      break;
    case E_MIPS_ABI_EABI32:
      mips_abi = MIPS_ABI_EABI32;
      break;
    case E_MIPS_ABI_EABI64:
      mips_abi = MIPS_ABI_EABI64;
      break;
    default:
      if ((elf_flags & EF_MIPS_ABI2))
	mips_abi = MIPS_ABI_N32;
      else
	mips_abi = MIPS_ABI_UNKNOWN;
      break;
    }

  /* GCC creates a pseudo-section whose name describes the ABI.  */
  if (mips_abi == MIPS_ABI_UNKNOWN && info.abfd != NULL)
    bfd_map_over_sections (info.abfd, mips_find_abi_section, &mips_abi);

  /* If we have no bfd, then mips_abi will still be MIPS_ABI_UNKNOWN.
     Use the ABI from the last architecture if there is one.  */
  if (info.abfd == NULL && arches != NULL)
    mips_abi = gdbarch_tdep (arches->gdbarch)->found_abi;

  /* Try the architecture for any hint of the correct ABI.  */
  if (mips_abi == MIPS_ABI_UNKNOWN
      && info.bfd_arch_info != NULL
      && info.bfd_arch_info->arch == bfd_arch_mips)
    {
      switch (info.bfd_arch_info->mach)
	{
	case bfd_mach_mips3900:
	  mips_abi = MIPS_ABI_EABI32;
	  break;
	case bfd_mach_mips4100:
	case bfd_mach_mips5000:
	  mips_abi = MIPS_ABI_EABI64;
	  break;
	case bfd_mach_mips8000:
	case bfd_mach_mips10000:
	  /* On Irix, ELF64 executables use the N64 ABI.  The
	     pseudo-sections which describe the ABI aren't present
	     on IRIX.  (Even for executables created by gcc.)  */
	  if (bfd_get_flavour (info.abfd) == bfd_target_elf_flavour
	      && elf_elfheader (info.abfd)->e_ident[EI_CLASS] == ELFCLASS64)
	    mips_abi = MIPS_ABI_N64;
	  else
	    mips_abi = MIPS_ABI_N32;
	  break;
	}
    }

  if (mips_abi == MIPS_ABI_UNKNOWN)
    mips_abi = MIPS_ABI_O32;

  /* Now that we have found what the ABI for this binary would be,
     check whether the user is overriding it.  */
  found_abi = mips_abi;
  wanted_abi = global_mips_abi ();
  if (wanted_abi != MIPS_ABI_UNKNOWN)
    mips_abi = wanted_abi;

  if (gdbarch_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "mips_gdbarch_init: elf_flags = 0x%08x\n",
			  elf_flags);
      fprintf_unfiltered (gdb_stdlog,
			  "mips_gdbarch_init: mips_abi = %d\n",
			  mips_abi);
      fprintf_unfiltered (gdb_stdlog,
			  "mips_gdbarch_init: found_mips_abi = %d\n",
			  found_abi);
    }

  /* try to find a pre-existing architecture */
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
      if (gdbarch_tdep (arches->gdbarch)->osabi == osabi)
        return arches->gdbarch;
    }

  /* Need a new architecture.  Fill in a target specific vector.  */
  tdep = (struct gdbarch_tdep *) xmalloc (sizeof (struct gdbarch_tdep));
  gdbarch = gdbarch_alloc (&info, tdep);
  tdep->elf_flags = elf_flags;
  tdep->osabi = osabi;

  /* Initially set everything according to the default ABI/ISA.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_register_raw_size (gdbarch, mips_register_raw_size);
  set_gdbarch_max_register_raw_size (gdbarch, 8);
  set_gdbarch_max_register_virtual_size (gdbarch, 8);
  tdep->found_abi = found_abi;
  tdep->mips_abi = mips_abi;

  set_gdbarch_elf_make_msymbol_special (gdbarch, 
					mips_elf_make_msymbol_special);

  switch (mips_abi)
    {
    case MIPS_ABI_O32:
      set_gdbarch_push_arguments (gdbarch, mips_o32_push_arguments);
      set_gdbarch_deprecated_store_return_value (gdbarch, mips_o32_store_return_value);
      set_gdbarch_extract_return_value (gdbarch, mips_o32_extract_return_value);
      tdep->mips_default_saved_regsize = 4;
      tdep->mips_default_stack_argsize = 4;
      tdep->mips_fp_register_double = 0;
      tdep->mips_last_arg_regnum = A0_REGNUM + 4 - 1;
      tdep->mips_last_fp_arg_regnum = FPA0_REGNUM + 4 - 1;
      tdep->gdb_target_is_mips64 = 0;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_reg_struct_has_addr (gdbarch, 
				       mips_o32_reg_struct_has_addr);
      set_gdbarch_use_struct_convention (gdbarch, 
					 mips_o32_use_struct_convention);
      break;
    case MIPS_ABI_O64:
      set_gdbarch_push_arguments (gdbarch, mips_o64_push_arguments);
      set_gdbarch_deprecated_store_return_value (gdbarch, mips_o64_store_return_value);
      set_gdbarch_deprecated_extract_return_value (gdbarch, mips_o64_extract_return_value);
      tdep->mips_default_saved_regsize = 8;
      tdep->mips_default_stack_argsize = 8;
      tdep->mips_fp_register_double = 1;
      tdep->mips_last_arg_regnum = A0_REGNUM + 4 - 1;
      tdep->mips_last_fp_arg_regnum = FPA0_REGNUM + 4 - 1;
      tdep->gdb_target_is_mips64 = 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_reg_struct_has_addr (gdbarch, 
				       mips_o32_reg_struct_has_addr);
      set_gdbarch_use_struct_convention (gdbarch, 
					 mips_o32_use_struct_convention);
      break;
    case MIPS_ABI_EABI32:
      set_gdbarch_push_arguments (gdbarch, mips_eabi_push_arguments);
      set_gdbarch_deprecated_store_return_value (gdbarch, mips_eabi_store_return_value);
      set_gdbarch_deprecated_extract_return_value (gdbarch, mips_eabi_extract_return_value);
      tdep->mips_default_saved_regsize = 4;
      tdep->mips_default_stack_argsize = 4;
      tdep->mips_fp_register_double = 0;
      tdep->mips_last_arg_regnum = A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = FPA0_REGNUM + 8 - 1;
      tdep->gdb_target_is_mips64 = 0;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_reg_struct_has_addr (gdbarch, 
				       mips_eabi_reg_struct_has_addr);
      set_gdbarch_use_struct_convention (gdbarch, 
					 mips_eabi_use_struct_convention);
      break;
    case MIPS_ABI_EABI64:
      set_gdbarch_push_arguments (gdbarch, mips_eabi_push_arguments);
      set_gdbarch_deprecated_store_return_value (gdbarch, mips_eabi_store_return_value);
      set_gdbarch_deprecated_extract_return_value (gdbarch, mips_eabi_extract_return_value);
      tdep->mips_default_saved_regsize = 8;
      tdep->mips_default_stack_argsize = 8;
      tdep->mips_fp_register_double = 1;
      tdep->mips_last_arg_regnum = A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = FPA0_REGNUM + 8 - 1;
      tdep->gdb_target_is_mips64 = 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 64);
      set_gdbarch_ptr_bit (gdbarch, 64);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_reg_struct_has_addr (gdbarch, 
				       mips_eabi_reg_struct_has_addr);
      set_gdbarch_use_struct_convention (gdbarch, 
					 mips_eabi_use_struct_convention);
      break;
    case MIPS_ABI_N32:
      set_gdbarch_push_arguments (gdbarch, mips_n32n64_push_arguments);
      set_gdbarch_deprecated_store_return_value (gdbarch, mips_n32n64_store_return_value);
      set_gdbarch_extract_return_value (gdbarch, mips_n32n64_extract_return_value);
      tdep->mips_default_saved_regsize = 8;
      tdep->mips_default_stack_argsize = 8;
      tdep->mips_fp_register_double = 1;
      tdep->mips_last_arg_regnum = A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = FPA0_REGNUM + 8 - 1;
      tdep->gdb_target_is_mips64 = 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 32);
      set_gdbarch_ptr_bit (gdbarch, 32);
      set_gdbarch_long_long_bit (gdbarch, 64);

      /* Set up the disassembler info, so that we get the right
	 register names from libopcodes.  */
      tm_print_insn_info.flavour = bfd_target_elf_flavour;
      tm_print_insn_info.arch = bfd_arch_mips;
      if (info.bfd_arch_info != NULL
	  && info.bfd_arch_info->arch == bfd_arch_mips
	  && info.bfd_arch_info->mach)
	tm_print_insn_info.mach = info.bfd_arch_info->mach;
      else
	tm_print_insn_info.mach = bfd_mach_mips8000;

      set_gdbarch_use_struct_convention (gdbarch, 
					 mips_n32n64_use_struct_convention);
      set_gdbarch_reg_struct_has_addr (gdbarch, 
				       mips_n32n64_reg_struct_has_addr);
      break;
    case MIPS_ABI_N64:
      set_gdbarch_push_arguments (gdbarch, mips_n32n64_push_arguments);
      set_gdbarch_deprecated_store_return_value (gdbarch, mips_n32n64_store_return_value);
      set_gdbarch_extract_return_value (gdbarch, mips_n32n64_extract_return_value);
      tdep->mips_default_saved_regsize = 8;
      tdep->mips_default_stack_argsize = 8;
      tdep->mips_fp_register_double = 1;
      tdep->mips_last_arg_regnum = A0_REGNUM + 8 - 1;
      tdep->mips_last_fp_arg_regnum = FPA0_REGNUM + 8 - 1;
      tdep->gdb_target_is_mips64 = 1;
      tdep->default_mask_address_p = 0;
      set_gdbarch_long_bit (gdbarch, 64);
      set_gdbarch_ptr_bit (gdbarch, 64);
      set_gdbarch_long_long_bit (gdbarch, 64);

      /* Set up the disassembler info, so that we get the right
	 register names from libopcodes.  */
      tm_print_insn_info.flavour = bfd_target_elf_flavour;
      tm_print_insn_info.arch = bfd_arch_mips;
      if (info.bfd_arch_info != NULL
	  && info.bfd_arch_info->arch == bfd_arch_mips
	  && info.bfd_arch_info->mach)
	tm_print_insn_info.mach = info.bfd_arch_info->mach;
      else
	tm_print_insn_info.mach = bfd_mach_mips8000;

      set_gdbarch_use_struct_convention (gdbarch, 
					 mips_n32n64_use_struct_convention);
      set_gdbarch_reg_struct_has_addr (gdbarch, 
				       mips_n32n64_reg_struct_has_addr);
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      "unknown ABI in switch");
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

  /* enable/disable the MIPS FPU */
  if (!mips_fpu_type_auto)
    tdep->mips_fpu_type = mips_fpu_type;
  else if (info.bfd_arch_info != NULL
	   && info.bfd_arch_info->arch == bfd_arch_mips)
    switch (info.bfd_arch_info->mach)
      {
      case bfd_mach_mips3900:
      case bfd_mach_mips4100:
      case bfd_mach_mips4111:
	tdep->mips_fpu_type = MIPS_FPU_NONE;
	break;
      case bfd_mach_mips4650:
	tdep->mips_fpu_type = MIPS_FPU_SINGLE;
	break;
      default:
	tdep->mips_fpu_type = MIPS_FPU_DOUBLE;
	break;
      }
  else
    tdep->mips_fpu_type = MIPS_FPU_DOUBLE;

  /* MIPS version of register names.  NOTE: At present the MIPS
     register name management is part way between the old -
     #undef/#define REGISTER_NAMES and the new REGISTER_NAME(nr).
     Further work on it is required.  */
  /* NOTE: many targets (esp. embedded) do not go thru the
     gdbarch_register_name vector at all, instead bypassing it
     by defining REGISTER_NAMES.  */
  set_gdbarch_register_name (gdbarch, mips_register_name);
  set_gdbarch_read_pc (gdbarch, mips_read_pc);
  set_gdbarch_write_pc (gdbarch, generic_target_write_pc);
  set_gdbarch_read_fp (gdbarch, generic_target_read_fp);
  set_gdbarch_read_sp (gdbarch, mips_read_sp);
  set_gdbarch_write_sp (gdbarch, generic_target_write_sp);

  /* Add/remove bits from an address.  The MIPS needs be careful to
     ensure that all 32 bit addresses are sign extended to 64 bits.  */
  set_gdbarch_addr_bits_remove (gdbarch, mips_addr_bits_remove);

  /* There's a mess in stack frame creation.  See comments in
     blockframe.c near reference to INIT_FRAME_PC_FIRST.  */
  set_gdbarch_init_frame_pc_first (gdbarch, mips_init_frame_pc_first);
  set_gdbarch_init_frame_pc (gdbarch, init_frame_pc_noop);

  /* Map debug register numbers onto internal register numbers.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, mips_stab_reg_to_regnum);
  set_gdbarch_ecoff_reg_to_regnum (gdbarch, mips_ecoff_reg_to_regnum);

  /* Initialize a frame */
  set_gdbarch_init_extra_frame_info (gdbarch, mips_init_extra_frame_info);
  set_gdbarch_frame_init_saved_regs (gdbarch, mips_frame_init_saved_regs);

  /* MIPS version of CALL_DUMMY */

  set_gdbarch_call_dummy_p (gdbarch, 1);
  set_gdbarch_call_dummy_stack_adjust_p (gdbarch, 0);
  set_gdbarch_use_generic_dummy_frames (gdbarch, 0);
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_call_dummy_address (gdbarch, mips_call_dummy_address);
  set_gdbarch_push_return_address (gdbarch, mips_push_return_address);
  set_gdbarch_push_dummy_frame (gdbarch, mips_push_dummy_frame);
  set_gdbarch_pop_frame (gdbarch, mips_pop_frame);
  set_gdbarch_call_dummy_start_offset (gdbarch, 0);
  set_gdbarch_call_dummy_breakpoint_offset_p (gdbarch, 1);
  set_gdbarch_call_dummy_breakpoint_offset (gdbarch, 0);
  set_gdbarch_call_dummy_length (gdbarch, 0);
  set_gdbarch_fix_call_dummy (gdbarch, mips_fix_call_dummy);
  set_gdbarch_pc_in_call_dummy (gdbarch, pc_in_call_dummy_at_entry_point);
  set_gdbarch_call_dummy_words (gdbarch, mips_call_dummy_words);
  set_gdbarch_sizeof_call_dummy_words (gdbarch, sizeof (mips_call_dummy_words));
  set_gdbarch_push_return_address (gdbarch, mips_push_return_address);
  set_gdbarch_register_convertible (gdbarch, mips_register_convertible);
  set_gdbarch_register_convert_to_virtual (gdbarch, 
					   mips_register_convert_to_virtual);
  set_gdbarch_register_convert_to_raw (gdbarch, 
				       mips_register_convert_to_raw);

  set_gdbarch_coerce_float_to_double (gdbarch, mips_coerce_float_to_double);

  set_gdbarch_frame_chain (gdbarch, mips_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, func_frame_chain_valid);
  set_gdbarch_frameless_function_invocation (gdbarch, 
					     generic_frameless_function_invocation_not);
  set_gdbarch_frame_saved_pc (gdbarch, mips_frame_saved_pc);
  set_gdbarch_frame_args_address (gdbarch, default_frame_address);
  set_gdbarch_frame_locals_address (gdbarch, default_frame_address);
  set_gdbarch_frame_num_args (gdbarch, frame_num_args_unknown);
  set_gdbarch_frame_args_skip (gdbarch, 0);

  set_gdbarch_get_saved_register (gdbarch, mips_get_saved_register);

  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_from_pc (gdbarch, mips_breakpoint_from_pc);
  set_gdbarch_decr_pc_after_break (gdbarch, 0);

  set_gdbarch_skip_prologue (gdbarch, mips_skip_prologue);
  set_gdbarch_saved_pc_after_call (gdbarch, mips_saved_pc_after_call);

  set_gdbarch_pointer_to_address (gdbarch, signed_pointer_to_address);
  set_gdbarch_address_to_pointer (gdbarch, address_to_signed_pointer);
  set_gdbarch_integer_to_address (gdbarch, mips_integer_to_address);

  set_gdbarch_function_start_offset (gdbarch, 0);

  /* There are MIPS targets which do not yet use this since they still
     define REGISTER_VIRTUAL_TYPE.  */
  set_gdbarch_register_virtual_type (gdbarch, mips_register_virtual_type);
  set_gdbarch_register_virtual_size (gdbarch, generic_register_size);

  set_gdbarch_do_registers_info (gdbarch, mips_do_registers_info);
  set_gdbarch_pc_in_sigtramp (gdbarch, mips_pc_in_sigtramp);

  /* Hook in OS ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch, osabi);

  set_gdbarch_store_struct_return (gdbarch, mips_store_struct_return);
  set_gdbarch_extract_struct_value_address (gdbarch, 
					    mips_extract_struct_value_address);
  
  set_gdbarch_skip_trampoline_code (gdbarch, mips_skip_stub);

  set_gdbarch_in_solib_call_trampoline (gdbarch, mips_in_call_stub);
  set_gdbarch_in_solib_return_trampoline (gdbarch, mips_in_return_stub);

  return gdbarch;
}

static void
mips_abi_update (char *ignore_args, int from_tty, 
		 struct cmd_list_element *c)
{
  struct gdbarch_info info;

  /* Force the architecture to update, and (if it's a MIPS architecture)
     mips_gdbarch_init will take care of the rest.  */
  gdbarch_info_init (&info);
  gdbarch_update_p (info);
}

static void
mips_dump_tdep (struct gdbarch *current_gdbarch, struct ui_file *file)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
  if (tdep != NULL)
    {
      int ef_mips_arch;
      int ef_mips_32bitmode;
      /* determine the ISA */
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
      /* determine the size of a pointer */
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
			  tdep->mips_abi,
			  mips_abi_strings[tdep->mips_abi]);
      fprintf_unfiltered (file,
			  "mips_dump_tdep: mips_mask_address_p() %d (default %d)\n",
			  mips_mask_address_p (),
			  tdep->default_mask_address_p);
    }
  fprintf_unfiltered (file,
		      "mips_dump_tdep: FP_REGISTER_DOUBLE = %d\n",
		      FP_REGISTER_DOUBLE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_DEFAULT_FPU_TYPE = %d (%s)\n",
		      MIPS_DEFAULT_FPU_TYPE,
		      (MIPS_DEFAULT_FPU_TYPE == MIPS_FPU_NONE ? "none"
		       : MIPS_DEFAULT_FPU_TYPE == MIPS_FPU_SINGLE ? "single"
		       : MIPS_DEFAULT_FPU_TYPE == MIPS_FPU_DOUBLE ? "double"
		       : "???"));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_EABI = %d\n",
		      MIPS_EABI);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_LAST_FP_ARG_REGNUM = %d (%d regs)\n",
		      MIPS_LAST_FP_ARG_REGNUM,
		      MIPS_LAST_FP_ARG_REGNUM - FPA0_REGNUM + 1);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_FPU_TYPE = %d (%s)\n",
		      MIPS_FPU_TYPE,
		      (MIPS_FPU_TYPE == MIPS_FPU_NONE ? "none"
		       : MIPS_FPU_TYPE == MIPS_FPU_SINGLE ? "single"
		       : MIPS_FPU_TYPE == MIPS_FPU_DOUBLE ? "double"
		       : "???"));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_DEFAULT_SAVED_REGSIZE = %d\n",
		      MIPS_DEFAULT_SAVED_REGSIZE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: FP_REGISTER_DOUBLE = %d\n",
		      FP_REGISTER_DOUBLE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_DEFAULT_STACK_ARGSIZE = %d\n",
		      MIPS_DEFAULT_STACK_ARGSIZE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_STACK_ARGSIZE = %d\n",
		      MIPS_STACK_ARGSIZE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_REGSIZE = %d\n",
		      MIPS_REGSIZE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: A0_REGNUM = %d\n",
		      A0_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: ADDR_BITS_REMOVE # %s\n",
		      XSTRING (ADDR_BITS_REMOVE(ADDR)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: ATTACH_DETACH # %s\n",
		      XSTRING (ATTACH_DETACH));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: BADVADDR_REGNUM = %d\n",
		      BADVADDR_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: BIG_BREAKPOINT = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: CAUSE_REGNUM = %d\n",
		      CAUSE_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: CPLUS_MARKER = %c\n",
		      CPLUS_MARKER);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: DO_REGISTERS_INFO # %s\n",
		      XSTRING (DO_REGISTERS_INFO));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: DWARF_REG_TO_REGNUM # %s\n",
		      XSTRING (DWARF_REG_TO_REGNUM (REGNUM)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: ECOFF_REG_TO_REGNUM # %s\n",
		      XSTRING (ECOFF_REG_TO_REGNUM (REGNUM)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: FCRCS_REGNUM = %d\n",
		      FCRCS_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: FCRIR_REGNUM = %d\n",
		      FCRIR_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: FIRST_EMBED_REGNUM = %d\n",
		      FIRST_EMBED_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: FPA0_REGNUM = %d\n",
		      FPA0_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: GDB_TARGET_IS_MIPS64 = %d\n",
		      GDB_TARGET_IS_MIPS64);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: GDB_TARGET_MASK_DISAS_PC # %s\n",
		      XSTRING (GDB_TARGET_MASK_DISAS_PC (PC)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: GDB_TARGET_UNMASK_DISAS_PC # %s\n",
		      XSTRING (GDB_TARGET_UNMASK_DISAS_PC (PC)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: GEN_REG_SAVE_MASK = %d\n",
		      GEN_REG_SAVE_MASK);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: HAVE_NONSTEPPABLE_WATCHPOINT # %s\n",
		      XSTRING (HAVE_NONSTEPPABLE_WATCHPOINT));
  fprintf_unfiltered (file,
		      "mips_dump_tdep:  HI_REGNUM = %d\n",
		      HI_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: IDT_BIG_BREAKPOINT = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: IDT_LITTLE_BREAKPOINT = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: IGNORE_HELPER_CALL # %s\n",
		      XSTRING (IGNORE_HELPER_CALL (PC)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: IN_SOLIB_CALL_TRAMPOLINE # %s\n",
		      XSTRING (IN_SOLIB_CALL_TRAMPOLINE (PC, NAME)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: IN_SOLIB_RETURN_TRAMPOLINE # %s\n",
		      XSTRING (IN_SOLIB_RETURN_TRAMPOLINE (PC, NAME)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: IS_MIPS16_ADDR = FIXME!\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: LAST_EMBED_REGNUM = %d\n",
		      LAST_EMBED_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: LITTLE_BREAKPOINT = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: LO_REGNUM = %d\n",
		      LO_REGNUM);
#ifdef MACHINE_CPROC_FP_OFFSET
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MACHINE_CPROC_FP_OFFSET = %d\n",
		      MACHINE_CPROC_FP_OFFSET);
#endif
#ifdef MACHINE_CPROC_PC_OFFSET
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MACHINE_CPROC_PC_OFFSET = %d\n",
		      MACHINE_CPROC_PC_OFFSET);
#endif
#ifdef MACHINE_CPROC_SP_OFFSET
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MACHINE_CPROC_SP_OFFSET = %d\n",
		      MACHINE_CPROC_SP_OFFSET);
#endif
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MAKE_MIPS16_ADDR = FIXME!\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS16_BIG_BREAKPOINT = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS16_INSTLEN = %d\n",
		      MIPS16_INSTLEN);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS16_LITTLE_BREAKPOINT = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_DEFAULT_ABI = FIXME!\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_EFI_SYMBOL_NAME = multi-arch!!\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_INSTLEN = %d\n",
		      MIPS_INSTLEN);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_LAST_ARG_REGNUM = %d (%d regs)\n",
		      MIPS_LAST_ARG_REGNUM,
		      MIPS_LAST_ARG_REGNUM - A0_REGNUM + 1);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_NUMREGS = %d\n",
		      MIPS_NUMREGS);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_REGISTER_NAMES = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: MIPS_SAVED_REGSIZE = %d\n",
		      MIPS_SAVED_REGSIZE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: OP_LDFPR = used?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: OP_LDGPR = used?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PMON_BIG_BREAKPOINT = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PMON_LITTLE_BREAKPOINT = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PRID_REGNUM = %d\n",
		      PRID_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PRINT_EXTRA_FRAME_INFO # %s\n",
		      XSTRING (PRINT_EXTRA_FRAME_INFO (FRAME)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_DESC_IS_DUMMY = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_FRAME_ADJUST = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_FRAME_OFFSET = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_FRAME_REG = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_FREG_MASK = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_FREG_OFFSET = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_HIGH_ADDR = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_LOW_ADDR = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_PC_REG = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_REG_MASK = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_REG_OFFSET = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PROC_SYMBOL = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PS_REGNUM = %d\n",
		      PS_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: PUSH_FP_REGNUM = %d\n",
		      PUSH_FP_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: RA_REGNUM = %d\n",
		      RA_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: REGISTER_CONVERT_FROM_TYPE # %s\n",
		      XSTRING (REGISTER_CONVERT_FROM_TYPE (REGNUM, VALTYPE, RAW_BUFFER)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: REGISTER_CONVERT_TO_TYPE # %s\n",
		      XSTRING (REGISTER_CONVERT_TO_TYPE (REGNUM, VALTYPE, RAW_BUFFER)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: REGISTER_NAMES = delete?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: ROUND_DOWN = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: ROUND_UP = function?\n");
#ifdef SAVED_BYTES
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SAVED_BYTES = %d\n",
		      SAVED_BYTES);
#endif
#ifdef SAVED_FP
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SAVED_FP = %d\n",
		      SAVED_FP);
#endif
#ifdef SAVED_PC
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SAVED_PC = %d\n",
		      SAVED_PC);
#endif
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SETUP_ARBITRARY_FRAME # %s\n",
		      XSTRING (SETUP_ARBITRARY_FRAME (NUMARGS, ARGS)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SET_PROC_DESC_IS_DUMMY = function?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SIGFRAME_BASE = %d\n",
		      SIGFRAME_BASE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SIGFRAME_FPREGSAVE_OFF = %d\n",
		      SIGFRAME_FPREGSAVE_OFF);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SIGFRAME_PC_OFF = %d\n",
		      SIGFRAME_PC_OFF);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SIGFRAME_REGSAVE_OFF = %d\n",
		      SIGFRAME_REGSAVE_OFF);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SIGFRAME_REG_SIZE = %d\n",
		      SIGFRAME_REG_SIZE);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SKIP_TRAMPOLINE_CODE # %s\n",
		      XSTRING (SKIP_TRAMPOLINE_CODE (PC)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SOFTWARE_SINGLE_STEP # %s\n",
		      XSTRING (SOFTWARE_SINGLE_STEP (SIG, BP_P)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: SOFTWARE_SINGLE_STEP_P () = %d\n",
		      SOFTWARE_SINGLE_STEP_P ());
  fprintf_unfiltered (file,
		      "mips_dump_tdep: STAB_REG_TO_REGNUM # %s\n",
		      XSTRING (STAB_REG_TO_REGNUM (REGNUM)));
#ifdef STACK_END_ADDR
  fprintf_unfiltered (file,
		      "mips_dump_tdep: STACK_END_ADDR = %d\n",
		      STACK_END_ADDR);
#endif
  fprintf_unfiltered (file,
		      "mips_dump_tdep: STEP_SKIPS_DELAY # %s\n",
		      XSTRING (STEP_SKIPS_DELAY (PC)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: STEP_SKIPS_DELAY_P = %d\n",
		      STEP_SKIPS_DELAY_P);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: STOPPED_BY_WATCHPOINT # %s\n",
		      XSTRING (STOPPED_BY_WATCHPOINT (WS)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: T9_REGNUM = %d\n",
		      T9_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TABULAR_REGISTER_OUTPUT = used?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TARGET_CAN_USE_HARDWARE_WATCHPOINT # %s\n",
		      XSTRING (TARGET_CAN_USE_HARDWARE_WATCHPOINT (TYPE,CNT,OTHERTYPE)));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TARGET_HAS_HARDWARE_WATCHPOINTS # %s\n",
		      XSTRING (TARGET_HAS_HARDWARE_WATCHPOINTS));
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TARGET_MIPS = used?\n");
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TM_PRINT_INSN_MACH # %s\n",
		      XSTRING (TM_PRINT_INSN_MACH));
#ifdef TRACE_CLEAR
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TRACE_CLEAR # %s\n",
		      XSTRING (TRACE_CLEAR (THREAD, STATE)));
#endif
#ifdef TRACE_FLAVOR
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TRACE_FLAVOR = %d\n",
		      TRACE_FLAVOR);
#endif
#ifdef TRACE_FLAVOR_SIZE
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TRACE_FLAVOR_SIZE = %d\n",
		      TRACE_FLAVOR_SIZE);
#endif
#ifdef TRACE_SET
  fprintf_unfiltered (file,
		      "mips_dump_tdep: TRACE_SET # %s\n",
		      XSTRING (TRACE_SET (X,STATE)));
#endif
  fprintf_unfiltered (file,
		      "mips_dump_tdep: UNMAKE_MIPS16_ADDR = function?\n");
#ifdef UNUSED_REGNUM
  fprintf_unfiltered (file,
		      "mips_dump_tdep: UNUSED_REGNUM = %d\n",
		      UNUSED_REGNUM);
#endif
  fprintf_unfiltered (file,
		      "mips_dump_tdep: V0_REGNUM = %d\n",
		      V0_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: VM_MIN_ADDRESS = %ld\n",
		      (long) VM_MIN_ADDRESS);
#ifdef VX_NUM_REGS
  fprintf_unfiltered (file,
		      "mips_dump_tdep: VX_NUM_REGS = %d (used?)\n",
		      VX_NUM_REGS);
#endif
  fprintf_unfiltered (file,
		      "mips_dump_tdep: ZERO_REGNUM = %d\n",
		      ZERO_REGNUM);
  fprintf_unfiltered (file,
		      "mips_dump_tdep: _PROC_MAGIC_ = %d\n",
		      _PROC_MAGIC_);

  fprintf_unfiltered (file,
		      "mips_dump_tdep: OS ABI = %s\n",
		      gdbarch_osabi_name (tdep->osabi));
}

void
_initialize_mips_tdep (void)
{
  static struct cmd_list_element *mipsfpulist = NULL;
  struct cmd_list_element *c;

  mips_abi_string = mips_abi_strings [MIPS_ABI_UNKNOWN];
  if (MIPS_ABI_LAST + 1
      != sizeof (mips_abi_strings) / sizeof (mips_abi_strings[0]))
    internal_error (__FILE__, __LINE__, "mips_abi_strings out of sync");

  gdbarch_register (bfd_arch_mips, mips_gdbarch_init, mips_dump_tdep);
  if (!tm_print_insn)		/* Someone may have already set it */
    tm_print_insn = gdb_print_insn_mips;

  /* Add root prefix command for all "set mips"/"show mips" commands */
  add_prefix_cmd ("mips", no_class, set_mips_command,
		  "Various MIPS specific commands.",
		  &setmipscmdlist, "set mips ", 0, &setlist);

  add_prefix_cmd ("mips", no_class, show_mips_command,
		  "Various MIPS specific commands.",
		  &showmipscmdlist, "show mips ", 0, &showlist);

  /* Allow the user to override the saved register size. */
  add_show_from_set (add_set_enum_cmd ("saved-gpreg-size",
				       class_obscure,
				       size_enums,
				       &mips_saved_regsize_string, "\
Set size of general purpose registers saved on the stack.\n\
This option can be set to one of:\n\
  32    - Force GDB to treat saved GP registers as 32-bit\n\
  64    - Force GDB to treat saved GP registers as 64-bit\n\
  auto  - Allow GDB to use the target's default setting or autodetect the\n\
          saved GP register size from information contained in the executable.\n\
          (default: auto)",
				       &setmipscmdlist),
		     &showmipscmdlist);

  /* Allow the user to override the argument stack size. */
  add_show_from_set (add_set_enum_cmd ("stack-arg-size",
				       class_obscure,
				       size_enums,
				       &mips_stack_argsize_string, "\
Set the amount of stack space reserved for each argument.\n\
This option can be set to one of:\n\
  32    - Force GDB to allocate 32-bit chunks per argument\n\
  64    - Force GDB to allocate 64-bit chunks per argument\n\
  auto  - Allow GDB to determine the correct setting from the current\n\
          target and executable (default)",
				       &setmipscmdlist),
		     &showmipscmdlist);

  /* Allow the user to override the ABI. */
  c = add_set_enum_cmd
    ("abi", class_obscure, mips_abi_strings, &mips_abi_string,
     "Set the ABI used by this program.\n"
     "This option can be set to one of:\n"
     "  auto  - the default ABI associated with the current binary\n"
     "  o32\n"
     "  o64\n"
     "  n32\n"
     "  n64\n"
     "  eabi32\n"
     "  eabi64",
     &setmipscmdlist);
  add_show_from_set (c, &showmipscmdlist);
  set_cmd_sfunc (c, mips_abi_update);

  /* Let the user turn off floating point and set the fence post for
     heuristic_proc_start.  */

  add_prefix_cmd ("mipsfpu", class_support, set_mipsfpu_command,
		  "Set use of MIPS floating-point coprocessor.",
		  &mipsfpulist, "set mipsfpu ", 0, &setlist);
  add_cmd ("single", class_support, set_mipsfpu_single_command,
	   "Select single-precision MIPS floating-point coprocessor.",
	   &mipsfpulist);
  add_cmd ("double", class_support, set_mipsfpu_double_command,
	   "Select double-precision MIPS floating-point coprocessor.",
	   &mipsfpulist);
  add_alias_cmd ("on", "double", class_support, 1, &mipsfpulist);
  add_alias_cmd ("yes", "double", class_support, 1, &mipsfpulist);
  add_alias_cmd ("1", "double", class_support, 1, &mipsfpulist);
  add_cmd ("none", class_support, set_mipsfpu_none_command,
	   "Select no MIPS floating-point coprocessor.",
	   &mipsfpulist);
  add_alias_cmd ("off", "none", class_support, 1, &mipsfpulist);
  add_alias_cmd ("no", "none", class_support, 1, &mipsfpulist);
  add_alias_cmd ("0", "none", class_support, 1, &mipsfpulist);
  add_cmd ("auto", class_support, set_mipsfpu_auto_command,
	   "Select MIPS floating-point coprocessor automatically.",
	   &mipsfpulist);
  add_cmd ("mipsfpu", class_support, show_mipsfpu_command,
	   "Show current use of MIPS floating-point coprocessor target.",
	   &showlist);

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
  set_cmd_sfunc (c, reinit_frame_cache_sfunc);
  add_show_from_set (c, &showlist);

  /* Allow the user to control whether the upper bits of 64-bit
     addresses should be zeroed.  */
  add_setshow_auto_boolean_cmd ("mask-address", no_class, &mask_address_var, "\
Set zeroing of upper 32 bits of 64-bit addresses.\n\
Use \"on\" to enable the masking, \"off\" to disable it and \"auto\" to \n\
allow GDB to determine the correct value.\n", "\
Show zeroing of upper 32 bits of 64-bit addresses.",
				NULL, show_mask_address,
				&setmipscmdlist, &showmipscmdlist);

  /* Allow the user to control the size of 32 bit registers within the
     raw remote packet.  */
  add_show_from_set (add_set_cmd ("remote-mips64-transfers-32bit-regs",
				  class_obscure,
				  var_boolean,
				  (char *)&mips64_transfers_32bit_regs_p, "\
Set compatibility with MIPS targets that transfers 32 and 64 bit quantities.\n\
Use \"on\" to enable backward compatibility with older MIPS 64 GDB+target\n\
that would transfer 32 bits for some registers (e.g. SR, FSR) and\n\
64 bits for others.  Use \"off\" to disable compatibility mode",
				  &setlist),
		     &showlist);

  /* Debug this files internals. */
  add_show_from_set (add_set_cmd ("mips", class_maintenance, var_zinteger,
				  &mips_debug, "Set mips debugging.\n\
When non-zero, mips specific debugging is enabled.", &setdebuglist),
		     &showdebuglist);
}
