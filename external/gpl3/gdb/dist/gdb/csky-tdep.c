/* Target-dependent code for the CSKY architecture, for GDB.

   Copyright (C) 2010-2020 Free Software Foundation, Inc.

   Contributed by C-SKY Microsystems and Mentor Graphics.

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
#include "gdbsupport/gdb_assert.h"
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
#include "block.h"
#include "reggroups.h"
#include "elf/csky.h"
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
#include "dwarf2/frame.h"
#include "user-regs.h"
#include "valprint.h"
#include "csky-tdep.h"
#include "regset.h"
#include "opcode/csky.h"
#include <algorithm>
#include <vector>

/* Control debugging information emitted in this file.  */
static bool csky_debug = false;

static struct reggroup *cr_reggroup;
static struct reggroup *fr_reggroup;
static struct reggroup *vr_reggroup;
static struct reggroup *mmu_reggroup;
static struct reggroup *prof_reggroup;

/* Convenience function to print debug messages in prologue analysis.  */

static void
print_savedreg_msg (int regno, int offsets[], bool print_continuing)
{
  fprintf_unfiltered (gdb_stdlog, "csky: r%d saved at offset 0x%x\n",
		      regno, offsets[regno]);
  if (print_continuing)
    fprintf_unfiltered (gdb_stdlog, "csky: continuing\n");
}

/*  Check whether the instruction at ADDR is 16-bit or not.  */

static int
csky_pc_is_csky16 (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_byte target_mem[2];
  int status;
  unsigned int insn;
  int ret = 1;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  status = target_read_memory (addr, target_mem, 2);
  /* Assume a 16-bit instruction if we can't read memory.  */
  if (status)
    return 1;

  /* Get instruction from memory.  */
  insn = extract_unsigned_integer (target_mem, 2, byte_order);
  if ((insn & CSKY_32_INSN_MASK) == CSKY_32_INSN_MASK)
    ret = 0;
  else if (insn == CSKY_BKPT_INSN)
    {
      /* Check for 32-bit bkpt instruction which is all 0.  */
      status = target_read_memory (addr + 2, target_mem, 2);
      if (status)
	return 1;

      insn = extract_unsigned_integer (target_mem, 2, byte_order);
      if (insn == CSKY_BKPT_INSN)
	ret = 0;
    }
  return ret;
}

/* Get one instruction at ADDR and store it in INSN.  Return 2 for
   a 16-bit instruction or 4 for a 32-bit instruction.  */

static int
csky_get_insn (struct gdbarch *gdbarch, CORE_ADDR addr, unsigned int *insn)
{
  gdb_byte target_mem[2];
  unsigned int insn_type;
  int status;
  int insn_len = 2;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  status = target_read_memory (addr, target_mem, 2);
  if (status)
    memory_error (TARGET_XFER_E_IO, addr);

  insn_type = extract_unsigned_integer (target_mem, 2, byte_order);
  if (CSKY_32_INSN_MASK == (insn_type & CSKY_32_INSN_MASK))
    {
      status = target_read_memory (addr + 2, target_mem, 2);
      if (status)
	memory_error (TARGET_XFER_E_IO, addr);
      insn_type = ((insn_type << 16)
		   | extract_unsigned_integer (target_mem, 2, byte_order));
      insn_len = 4;
    }
  *insn = insn_type;
  return insn_len;
}

/* Implement the read_pc gdbarch method.  */

static CORE_ADDR
csky_read_pc (readable_regcache *regcache)
{
  ULONGEST pc;
  regcache->cooked_read (CSKY_PC_REGNUM, &pc);
  return pc;
}

/* Implement the write_pc gdbarch method.  */

static void
csky_write_pc (regcache *regcache, CORE_ADDR val)
{
  regcache_cooked_write_unsigned (regcache, CSKY_PC_REGNUM, val);
}

/* C-Sky ABI register names.  */

static const char *csky_register_names[] =
{
  /* General registers 0 - 31.  */
  "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
  "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",

  /* DSP hilo registers 36 and 37.  */
  "",      "",    "",     "",     "hi",    "lo",   "",    "",

  /* FPU/VPU general registers 40 - 71.  */
  "fr0", "fr1", "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",
  "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
  "vr0", "vr1", "vr2",  "vr3",  "vr4",  "vr5",  "vr6",  "vr7",
  "vr8", "vr9", "vr10", "vr11", "vr12", "vr13", "vr14", "vr15",

  /* Program counter 72.  */
  "pc",

  /* Optional registers (ar) 73 - 88.  */
  "ar0", "ar1", "ar2",  "ar3",  "ar4",  "ar5",  "ar6",  "ar7",
  "ar8", "ar9", "ar10", "ar11", "ar12", "ar13", "ar14", "ar15",

  /* Control registers (cr) 89 - 119.  */
  "psr",  "vbr",  "epsr", "fpsr", "epc",  "fpc",  "ss0",  "ss1",
  "ss2",  "ss3",  "ss4",  "gcr",  "gsr",  "cr13", "cr14", "cr15",
  "cr16", "cr17", "cr18", "cr19", "cr20", "cr21", "cr22", "cr23",
  "cr24", "cr25", "cr26", "cr27", "cr28", "cr29", "cr30", "cr31",

  /* FPU/VPU control registers 121 ~ 123.  */
  /* User sp 127.  */
  "fid", "fcr", "fesr", "", "", "", "usp",

  /* MMU control registers: 128 - 136.  */
  "mcr0", "mcr2", "mcr3", "mcr4", "mcr6", "mcr8", "mcr29", "mcr30",
  "mcr31", "", "", "",

  /* Profiling control registers 140 - 143.  */
  /* Profiling software general registers 144 - 157.  */
  "profcr0",  "profcr1",  "profcr2",  "profcr3",  "profsgr0",  "profsgr1",
  "profsgr2", "profsgr3", "profsgr4", "profsgr5", "profsgr6",  "profsgr7",
  "profsgr8", "profsgr9", "profsgr10","profsgr11","profsgr12", "profsgr13",
  "",	 "",

  /* Profiling architecture general registers 160 - 174.  */
  "profagr0", "profagr1", "profagr2", "profagr3", "profagr4", "profagr5",
  "profagr6", "profagr7", "profagr8", "profagr9", "profagr10","profagr11",
  "profagr12","profagr13","profagr14", "",

  /* Profiling extension general registers 176 - 188.  */
  "profxgr0", "profxgr1", "profxgr2", "profxgr3", "profxgr4", "profxgr5",
  "profxgr6", "profxgr7", "profxgr8", "profxgr9", "profxgr10","profxgr11",
  "profxgr12",

  /* Control registers in bank1.  */
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "cp1cr16", "cp1cr17", "cp1cr18", "cp1cr19", "cp1cr20", "", "", "",
  "", "", "", "", "", "", "", "",

  /* Control registers in bank3 (ICE).  */
  "sepsr", "sevbr", "seepsr", "", "seepc", "", "nsssp", "seusp",
  "sedcr", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", ""
};

/* Implement the register_name gdbarch method.  */

static const char *
csky_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_name (gdbarch, reg_nr);

  if (reg_nr < 0)
    return NULL;

  if (reg_nr >= gdbarch_num_regs (gdbarch))
    return NULL;

  return csky_register_names[reg_nr];
}

/* Construct vector type for vrx registers.  */

static struct type *
csky_vector_type (struct gdbarch *gdbarch)
{
  const struct builtin_type *bt = builtin_type (gdbarch);

  struct type *t;

  t = arch_composite_type (gdbarch, "__gdb_builtin_type_vec128i",
			   TYPE_CODE_UNION);

  append_composite_type_field (t, "u32",
			       init_vector_type (bt->builtin_int32, 4));
  append_composite_type_field (t, "u16",
			       init_vector_type (bt->builtin_int16, 8));
  append_composite_type_field (t, "u8",
			       init_vector_type (bt->builtin_int8, 16));

  TYPE_VECTOR (t) = 1;
  t->set_name ("builtin_type_vec128i");

  return t;
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

static struct type *
csky_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  /* PC, EPC, FPC is a text pointer.  */
  if ((reg_nr == CSKY_PC_REGNUM)  || (reg_nr == CSKY_EPC_REGNUM)
      || (reg_nr == CSKY_FPC_REGNUM))
    return builtin_type (gdbarch)->builtin_func_ptr;

  /* VBR is a data pointer.  */
  if (reg_nr == CSKY_VBR_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;

  /* Float register has 64 bits, and only in ck810.  */
  if ((reg_nr >=CSKY_FR0_REGNUM) && (reg_nr <= CSKY_FR0_REGNUM + 15))
      return arch_float_type (gdbarch, 64, "builtin_type_csky_ext",
			      floatformats_ieee_double);

  /* Vector register has 128 bits, and only in ck810.  */
  if ((reg_nr >= CSKY_VR0_REGNUM) && (reg_nr <= CSKY_VR0_REGNUM + 15))
    return csky_vector_type (gdbarch);

  /* Profiling general register has 48 bits, we use 64bit.  */
  if ((reg_nr >= CSKY_PROFGR_REGNUM) && (reg_nr <= CSKY_PROFGR_REGNUM + 44))
    return builtin_type (gdbarch)->builtin_uint64;

  if (reg_nr == CSKY_SP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;

  /* Others are 32 bits.  */
  return builtin_type (gdbarch)->builtin_int32;
}

/* Data structure to marshall items in a dummy stack frame when
   calling a function in the inferior.  */

struct stack_item
{
  stack_item (int len_, const gdb_byte *data_)
  : len (len_), data (data_)
  {}

  int len;
  const gdb_byte *data;
};

/* Implement the push_dummy_call gdbarch method.  */

static CORE_ADDR
csky_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		      struct regcache *regcache, CORE_ADDR bp_addr,
		      int nargs, struct value **args, CORE_ADDR sp,
		      function_call_return_method return_method,
		      CORE_ADDR struct_addr)
{
  int argnum;
  int argreg = CSKY_ABI_A0_REGNUM;
  int last_arg_regnum = CSKY_ABI_LAST_ARG_REGNUM;
  int need_dummy_stack = 0;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  std::vector<stack_item> stack_items;

  /* Set the return address.  For CSKY, the return breakpoint is
     always at BP_ADDR.  */
  regcache_cooked_write_unsigned (regcache, CSKY_LR_REGNUM, bp_addr);

  /* The struct_return pointer occupies the first parameter
     passing register.  */
  if (return_method == return_method_struct)
    {
      if (csky_debug)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "csky: struct return in %s = %s\n",
			      gdbarch_register_name (gdbarch, argreg),
			      paddress (gdbarch, struct_addr));
	}
      regcache_cooked_write_unsigned (regcache, argreg, struct_addr);
      argreg++;
    }

  /* Put parameters into argument registers in REGCACHE.
     In ABI argument registers are r0 through r3.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      int len;
      struct type *arg_type;
      const gdb_byte *val;

      arg_type = check_typedef (value_type (args[argnum]));
      len = TYPE_LENGTH (arg_type);
      val = value_contents (args[argnum]);

      /* Copy the argument to argument registers or the dummy stack.
	 Large arguments are split between registers and stack.

	 If len < 4, there is no need to worry about endianness since
	 the arguments will always be stored in the low address.  */
      if (len < 4)
	{
	  CORE_ADDR regval
	    = extract_unsigned_integer (val, len, byte_order);
	  regcache_cooked_write_unsigned (regcache, argreg, regval);
	  argreg++;
	}
      else
	{
	  while (len > 0)
	    {
	      int partial_len = len < 4 ? len : 4;
	      if (argreg <= last_arg_regnum)
		{
		  /* The argument is passed in an argument register.  */
		  CORE_ADDR regval
		    = extract_unsigned_integer (val, partial_len,
						byte_order);
		  if (byte_order == BFD_ENDIAN_BIG)
		    regval <<= (4 - partial_len) * 8;

		  /* Put regval into register in REGCACHE.  */
		  regcache_cooked_write_unsigned (regcache, argreg,
						  regval);
		  argreg++;
		}
	      else
		{
		  /* The argument should be pushed onto the dummy stack.  */
		  stack_items.emplace_back (4, val);
		  need_dummy_stack += 4;
		}
	      len -= partial_len;
	      val += partial_len;
	    }
	}
    }

  /* Transfer the dummy stack frame to the target.  */
  std::vector<stack_item>::reverse_iterator iter;
  for (iter = stack_items.rbegin (); iter != stack_items.rend (); ++iter)
    {
      sp -= iter->len;
      write_memory (sp, iter->data, iter->len);
    }

  /* Finally, update the SP register.  */
  regcache_cooked_write_unsigned (regcache, CSKY_SP_REGNUM, sp);
  return sp;
}

/* Implement the return_value gdbarch method.  */

static enum return_value_convention
csky_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *valtype, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  CORE_ADDR regval;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = TYPE_LENGTH (valtype);
  unsigned int ret_regnum = CSKY_RET_REGNUM;

  /* Csky abi specifies that return values larger than 8 bytes
     are put on the stack.  */
  if (len > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    {
      if (readbuf != NULL)
	{
	  ULONGEST tmp;
	  /* By using store_unsigned_integer we avoid having to do
	     anything special for small big-endian values.  */
	  regcache->cooked_read (ret_regnum, &tmp);
	  store_unsigned_integer (readbuf, (len > 4 ? 4 : len),
				  byte_order, tmp);
	  if (len > 4)
	    {
	      regcache->cooked_read (ret_regnum + 1, &tmp);
	      store_unsigned_integer (readbuf + 4,  4, byte_order, tmp);
	    }
	}
      if (writebuf != NULL)
	{
	  regval = extract_unsigned_integer (writebuf, len > 4 ? 4 : len,
					     byte_order);
	  regcache_cooked_write_unsigned (regcache, ret_regnum, regval);
	  if (len > 4)
	    {
	      regval = extract_unsigned_integer ((gdb_byte *) writebuf + 4,
						 4, byte_order);
	      regcache_cooked_write_unsigned (regcache, ret_regnum + 1,
					      regval);
	    }

	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Implement the frame_align gdbarch method.

   Adjust the address downward (direction of stack growth) so that it
   is correctly aligned for a new stack frame.  */

static CORE_ADDR
csky_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 4);
}

/* Unwind cache used for gdbarch fallback unwinder.  */

struct csky_unwind_cache
{
  /* The stack pointer at the time this frame was created; i.e. the
     caller's stack pointer when this function was called.  It is used
     to identify this frame.  */
  CORE_ADDR prev_sp;

  /* The frame base for this frame is just prev_sp - frame size.
     FRAMESIZE is the distance from the frame pointer to the
     initial stack pointer.  */
  int framesize;

  /* The register used to hold the frame pointer for this frame.  */
  int framereg;

  /* Saved register offsets.  */
  struct trad_frame_saved_reg *saved_regs;
};

/* Do prologue analysis, returning the PC of the first instruction
   after the function prologue.  */

static CORE_ADDR
csky_analyze_prologue (struct gdbarch *gdbarch,
		       CORE_ADDR start_pc,
		       CORE_ADDR limit_pc,
		       CORE_ADDR end_pc,
		       struct frame_info *this_frame,
		       struct csky_unwind_cache *this_cache,
		       lr_type_t lr_type)
{
  CORE_ADDR addr;
  unsigned int insn, rn;
  int framesize = 0;
  int stacksize = 0;
  int register_offsets[CSKY_NUM_GREGS_SAVED_GREGS];
  int insn_len;
  /* For adjusting fp.  */
  int is_fp_saved = 0;
  int adjust_fp = 0;

  /* REGISTER_OFFSETS will contain offsets from the top of the frame
     (NOT the frame pointer) for the various saved registers, or -1
     if the register is not saved.  */
  for (rn = 0; rn < CSKY_NUM_GREGS_SAVED_GREGS; rn++)
    register_offsets[rn] = -1;

  /* Analyze the prologue.  Things we determine from analyzing the
     prologue include the size of the frame and which registers are
     saved (and where).  */
  if (csky_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "csky: Scanning prologue: start_pc = 0x%x,"
			  "limit_pc = 0x%x\n", (unsigned int) start_pc,
			  (unsigned int) limit_pc);
    }

  /* Default to 16 bit instruction.  */
  insn_len = 2;
  stacksize = 0;
  for (addr = start_pc; addr < limit_pc; addr += insn_len)
    {
      /* Get next insn.  */
      insn_len = csky_get_insn (gdbarch, addr, &insn);

      /* Check if 32 bit.  */
      if (insn_len == 4)
	{
	  /* subi32 sp,sp oimm12.  */
	  if (CSKY_32_IS_SUBI0 (insn))
	    {
	      /* Got oimm12.  */
	      int offset = CSKY_32_SUBI_IMM (insn);
	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: got subi sp,%d; continuing\n",
				      offset);
		}
	      stacksize += offset;
	      continue;
	    }
	  /* stm32 ry-rz,(sp).  */
	  else if (CSKY_32_IS_STMx0 (insn))
	    {
	      /* Spill register(s).  */
	      int start_register;
	      int reg_count;
	      int offset;

	      /* BIG WARNING! The CKCore ABI does not restrict functions
		 to taking only one stack allocation.  Therefore, when
		 we save a register, we record the offset of where it was
		 saved relative to the current stacksize.  This will
		 then give an offset from the SP upon entry to our
		 function.  Remember, stacksize is NOT constant until
		 we're done scanning the prologue.  */
	      start_register = CSKY_32_STM_VAL_REGNUM (insn);
	      reg_count = CSKY_32_STM_SIZE (insn);
	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: got stm r%d-r%d,(sp)\n",
				      start_register,
				      start_register + reg_count);
		}

	      for (rn = start_register, offset = 0;
		   rn <= start_register + reg_count;
		   rn++, offset += 4)
		{
		  register_offsets[rn] = stacksize - offset;
		  if (csky_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog,
					  "csky: r%d saved at 0x%x"
					  " (offset %d)\n",
					  rn, register_offsets[rn],
					  offset);
		    }
		}
	      if (csky_debug)
		fprintf_unfiltered (gdb_stdlog, "csky: continuing\n");
	      continue;
	    }
	  /* stw ry,(sp,disp).  */
	  else if (CSKY_32_IS_STWx0 (insn))
	    {
	      /* Spill register: see note for IS_STM above.  */
	      int disp;

	      rn = CSKY_32_ST_VAL_REGNUM (insn);
	      disp = CSKY_32_ST_OFFSET (insn);
	      register_offsets[rn] = stacksize - disp;
	      if (csky_debug)
		print_savedreg_msg (rn, register_offsets, true);
	      continue;
	    }
	  else if (CSKY_32_IS_MOV_FP_SP (insn))
	    {
	      /* SP is saved to FP reg, means code afer prologue may
		 modify SP.  */
	      is_fp_saved = 1;
	      adjust_fp = stacksize;
	      continue;
	    }
	  else if (CSKY_32_IS_MFCR_EPSR (insn))
	    {
	      unsigned int insn2;
	      addr += 4;
	      int mfcr_regnum = insn & 0x1f;
	      insn_len = csky_get_insn (gdbarch, addr, &insn2);
	      if (insn_len == 2)
		{
		  int stw_regnum = (insn2 >> 5) & 0x7;
		  if (CSKY_16_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_EPSR_REGNUM.  */
		      rn  = CSKY_NUM_GREGS;
		      offset = CSKY_16_STWx0_OFFSET (insn2);
		      register_offsets[rn] = stacksize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	      else
		{
		  /* INSN_LEN == 4.  */
		  int stw_regnum = (insn2 >> 21) & 0x1f;
		  if (CSKY_32_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_EPSR_REGNUM.  */
		      rn  = CSKY_NUM_GREGS;
		      offset = CSKY_32_ST_OFFSET (insn2);
		      register_offsets[rn] = framesize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	    }
	  else if (CSKY_32_IS_MFCR_FPSR (insn))
	    {
	      unsigned int insn2;
	      addr += 4;
	      int mfcr_regnum = insn & 0x1f;
	      insn_len = csky_get_insn (gdbarch, addr, &insn2);
	      if (insn_len == 2)
		{
		  int stw_regnum = (insn2 >> 5) & 0x7;
		  if (CSKY_16_IS_STWx0 (insn2) && (mfcr_regnum
						 == stw_regnum))
		    {
		      int offset;

		      /* CSKY_FPSR_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 1;
		      offset = CSKY_16_STWx0_OFFSET (insn2);
		      register_offsets[rn] = stacksize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	      else
		{
		  /* INSN_LEN == 4.  */
		  int stw_regnum = (insn2 >> 21) & 0x1f;
		  if (CSKY_32_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_FPSR_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 1;
		      offset = CSKY_32_ST_OFFSET (insn2);
		      register_offsets[rn] = framesize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	    }
	  else if (CSKY_32_IS_MFCR_EPC (insn))
	    {
	      unsigned int insn2;
	      addr += 4;
	      int mfcr_regnum = insn & 0x1f;
	      insn_len = csky_get_insn (gdbarch, addr, &insn2);
	      if (insn_len == 2)
		{
		  int stw_regnum = (insn2 >> 5) & 0x7;
		  if (CSKY_16_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_EPC_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 2;
		      offset = CSKY_16_STWx0_OFFSET (insn2);
		      register_offsets[rn] = stacksize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	      else
		{
		  /* INSN_LEN == 4.  */
		  int stw_regnum = (insn2 >> 21) & 0x1f;
		  if (CSKY_32_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_EPC_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 2;
		      offset = CSKY_32_ST_OFFSET (insn2);
		      register_offsets[rn] = framesize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	    }
	  else if (CSKY_32_IS_MFCR_FPC (insn))
	    {
	      unsigned int insn2;
	      addr += 4;
	      int mfcr_regnum = insn & 0x1f;
	      insn_len = csky_get_insn (gdbarch, addr, &insn2);
	      if (insn_len == 2)
		{
		  int stw_regnum = (insn2 >> 5) & 0x7;
		  if (CSKY_16_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_FPC_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 3;
		      offset = CSKY_16_STWx0_OFFSET (insn2);
		      register_offsets[rn] = stacksize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	      else
		{
		  /* INSN_LEN == 4.  */
		  int stw_regnum = (insn2 >> 21) & 0x1f;
		  if (CSKY_32_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_FPC_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 3;
		      offset = CSKY_32_ST_OFFSET (insn2);
		      register_offsets[rn] = framesize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	    }
	  else if (CSKY_32_IS_PUSH (insn))
	    {
	      /* Push for 32_bit.  */
	      int offset = 0;
	      if (CSKY_32_IS_PUSH_R29 (insn))
		{
		  stacksize += 4;
		  register_offsets[29] = stacksize;
		  if (csky_debug)
		    print_savedreg_msg (29, register_offsets, false);
		  offset += 4;
		}
	      if (CSKY_32_PUSH_LIST2 (insn))
		{
		  int num = CSKY_32_PUSH_LIST2 (insn);
		  int tmp = 0;
		  stacksize += num * 4;
		  offset += num * 4;
		  if (csky_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog,
					  "csky: push regs_array: r16-r%d\n",
					  16 + num - 1);
		    }
		  for (rn = 16; rn <= 16 + num - 1; rn++)
		    {
		       register_offsets[rn] = stacksize - tmp;
		       if (csky_debug)
			 {
			   fprintf_unfiltered (gdb_stdlog,
					       "csky: r%d saved at 0x%x"
					       " (offset %d)\n", rn,
					       register_offsets[rn], tmp);
			 }
		       tmp += 4;
		    }
		}
	      if (CSKY_32_IS_PUSH_R15 (insn))
		{
		  stacksize += 4;
		  register_offsets[15] = stacksize;
		  if (csky_debug)
		    print_savedreg_msg (15, register_offsets, false);
		  offset += 4;
		}
	      if (CSKY_32_PUSH_LIST1 (insn))
		{
		  int num = CSKY_32_PUSH_LIST1 (insn);
		  int tmp = 0;
		  stacksize += num * 4;
		  offset += num * 4;
		  if (csky_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog,
					  "csky: push regs_array: r4-r%d\n",
					  4 + num - 1);
		    }
		  for (rn = 4; rn <= 4 + num - 1; rn++)
		    {
		       register_offsets[rn] = stacksize - tmp;
		       if (csky_debug)
			 {
			   fprintf_unfiltered (gdb_stdlog,
					       "csky: r%d saved at 0x%x"
					       " (offset %d)\n", rn,
					       register_offsets[rn], tmp);
			 }
			tmp += 4;
		    }
		}

	      framesize = stacksize;
	      if (csky_debug)
		fprintf_unfiltered (gdb_stdlog, "csky: continuing\n");
	      continue;
	    }
	  else if (CSKY_32_IS_LRW4 (insn) || CSKY_32_IS_MOVI4 (insn)
		   || CSKY_32_IS_MOVIH4 (insn) || CSKY_32_IS_BMASKI4 (insn))
	    {
	      int adjust = 0;
	      int offset = 0;
	      unsigned int insn2;

	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: looking at large frame\n");
		}
	      if (CSKY_32_IS_LRW4 (insn))
		{
		  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
		  int literal_addr = (addr + ((insn & 0xffff) << 2))
				     & 0xfffffffc;
		  adjust = read_memory_unsigned_integer (literal_addr, 4,
							 byte_order);
		}
	      else if (CSKY_32_IS_MOVI4 (insn))
		adjust = (insn  & 0xffff);
	      else if (CSKY_32_IS_MOVIH4 (insn))
		adjust = (insn & 0xffff) << 16;
	      else
		{
		  /* CSKY_32_IS_BMASKI4 (insn).  */
		  adjust = (1 << (((insn & 0x3e00000) >> 21) + 1)) - 1;
		}

	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: base stacksize=0x%x\n", adjust);

		  /* May have zero or more insns which modify r4.  */
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: looking for r4 adjusters...\n");
		}

	      offset = 4;
	      insn_len = csky_get_insn (gdbarch, addr + offset, &insn2);
	      while (CSKY_IS_R4_ADJUSTER (insn2))
		{
		  if (CSKY_32_IS_ADDI4 (insn2))
		    {
		      int imm = (insn2 & 0xfff) + 1;
		      adjust += imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: addi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_SUBI4 (insn2))
		    {
		      int imm = (insn2 & 0xfff) + 1;
		      adjust -= imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: subi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_NOR4 (insn2))
		    {
		      adjust = ~adjust;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: nor r4,r4,r4\n");
			}
		    }
		  else if (CSKY_32_IS_ROTLI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      int temp = adjust >> (32 - imm);
		      adjust <<= imm;
		      adjust |= temp;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: rotli r4,r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_LISI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust <<= imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: lsli r4,r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_BSETI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust |= (1 << imm);
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: bseti r4,r4 %d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_BCLRI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust &= ~(1 << imm);
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: bclri r4,r4 %d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_IXH4 (insn2))
		    {
		      adjust *= 3;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: ixh r4,r4,r4\n");
			}
		    }
		  else if (CSKY_32_IS_IXW4 (insn2))
		    {
		      adjust *= 5;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: ixw r4,r4,r4\n");
			}
		    }
		  else if (CSKY_16_IS_ADDI4 (insn2))
		    {
		      int imm = (insn2 & 0xff) + 1;
		      adjust += imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: addi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_SUBI4 (insn2))
		    {
		      int imm = (insn2 & 0xff) + 1;
		      adjust -= imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: subi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_NOR4 (insn2))
		    {
		      adjust = ~adjust;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: nor r4,r4\n");
			}
		    }
		  else if (CSKY_16_IS_BSETI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust |= (1 << imm);
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: bseti r4, %d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_BCLRI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust &= ~(1 << imm);
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: bclri r4, %d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_LSLI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust <<= imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: lsli r4,r4, %d\n", imm);
			}
		    }

		  offset += insn_len;
		  insn_len =  csky_get_insn (gdbarch, addr + offset, &insn2);
		};

	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog, "csky: done looking for"
				      " r4 adjusters\n");
		}

	      /* If the next insn adjusts the stack pointer, we keep
		 everything; if not, we scrap it and we've found the
		 end of the prologue.  */
	      if (CSKY_IS_SUBU4 (insn2))
		{
		  addr += offset;
		  stacksize += adjust;
		  if (csky_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog,
					  "csky: found stack adjustment of"
					  " 0x%x bytes.\n", adjust);
		      fprintf_unfiltered (gdb_stdlog,
					  "csky: skipping to new address %s\n",
					  core_addr_to_string_nz (addr));
		      fprintf_unfiltered (gdb_stdlog,
					  "csky: continuing\n");
		    }
		  continue;
		}

	      /* None of these instructions are prologue, so don't touch
		 anything.  */
	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: no subu sp,sp,r4; NOT altering"
				      " stacksize.\n");
		}
	      break;
	    }
	}
      else
	{
	  /* insn_len != 4.  */

	  /* subi.sp sp,disp.  */
	  if (CSKY_16_IS_SUBI0 (insn))
	    {
	      int offset = CSKY_16_SUBI_IMM (insn);
	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: got subi r0,%d; continuing\n",
				      offset);
		}
	      stacksize += offset;
	      continue;
	    }
	  /* stw.16 rz,(sp,disp).  */
	  else if (CSKY_16_IS_STWx0 (insn))
	    {
	      /* Spill register: see note for IS_STM above.  */
	      int disp;

	      rn = CSKY_16_ST_VAL_REGNUM (insn);
	      disp = CSKY_16_ST_OFFSET (insn);
	      register_offsets[rn] = stacksize - disp;
	      if (csky_debug)
		print_savedreg_msg (rn, register_offsets, true);
	      continue;
	    }
	  else if (CSKY_16_IS_MOV_FP_SP (insn))
	    {
	      /* SP is saved to FP reg, means prologue may modify SP.  */
	      is_fp_saved = 1;
	      adjust_fp = stacksize;
	      continue;
	    }
	  else if (CSKY_16_IS_PUSH (insn))
	    {
	      /* Push for 16_bit.  */
	      int offset = 0;
	      if (CSKY_16_IS_PUSH_R15 (insn))
		{
		  stacksize += 4;
		  register_offsets[15] = stacksize;
		  if (csky_debug)
		    print_savedreg_msg (15, register_offsets, false);
		  offset += 4;
		 }
	      if (CSKY_16_PUSH_LIST1 (insn))
		{
		  int num = CSKY_16_PUSH_LIST1 (insn);
		  int tmp = 0;
		  stacksize += num * 4;
		  offset += num * 4;
		  if (csky_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog,
					  "csky: push regs_array: r4-r%d\n",
					  4 + num - 1);
		    }
		  for (rn = 4; rn <= 4 + num - 1; rn++)
		    {
		       register_offsets[rn] = stacksize - tmp;
		       if (csky_debug)
			 {
			   fprintf_unfiltered (gdb_stdlog,
					       "csky: r%d saved at 0x%x"
					       " (offset %d)\n", rn,
					       register_offsets[rn], offset);
			 }
		       tmp += 4;
		    }
		}

	      framesize = stacksize;
	      if (csky_debug)
		fprintf_unfiltered (gdb_stdlog, "csky: continuing\n");
	      continue;
	    }
	  else if (CSKY_16_IS_LRW4 (insn) || CSKY_16_IS_MOVI4 (insn))
	    {
	      int adjust = 0;
	      unsigned int insn2;

	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: looking at large frame\n");
		}
	      if (CSKY_16_IS_LRW4 (insn))
		{
		  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
		  int offset = ((insn & 0x300) >> 3) | (insn & 0x1f);
		  int literal_addr = (addr + ( offset << 2)) & 0xfffffffc;
		  adjust = read_memory_unsigned_integer (literal_addr, 4,
							 byte_order);
		}
	      else
		{
		  /* CSKY_16_IS_MOVI4 (insn).  */
		  adjust = (insn  & 0xff);
		}

	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: base stacksize=0x%x\n", adjust);
		}

	      /* May have zero or more instructions which modify r4.  */
	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "csky: looking for r4 adjusters...\n");
		}
	      int offset = 2;
	      insn_len = csky_get_insn (gdbarch, addr + offset, &insn2);
	      while (CSKY_IS_R4_ADJUSTER (insn2))
		{
		  if (CSKY_32_IS_ADDI4 (insn2))
		    {
		      int imm = (insn2 & 0xfff) + 1;
		      adjust += imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: addi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_SUBI4 (insn2))
		    {
		      int imm = (insn2 & 0xfff) + 1;
		      adjust -= imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: subi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_NOR4 (insn2))
		    {
		      adjust = ~adjust;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: nor r4,r4,r4\n");
			}
		    }
		  else if (CSKY_32_IS_ROTLI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      int temp = adjust >> (32 - imm);
		      adjust <<= imm;
		      adjust |= temp;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: rotli r4,r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_LISI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust <<= imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: lsli r4,r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_BSETI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust |= (1 << imm);
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: bseti r4,r4 %d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_BCLRI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust &= ~(1 << imm);
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: bclri r4,r4 %d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_IXH4 (insn2))
		    {
		      adjust *= 3;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: ixh r4,r4,r4\n");
			}
		    }
		  else if (CSKY_32_IS_IXW4 (insn2))
		    {
		      adjust *= 5;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: ixw r4,r4,r4\n");
			}
		    }
		  else if (CSKY_16_IS_ADDI4 (insn2))
		    {
		      int imm = (insn2 & 0xff) + 1;
		      adjust += imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: addi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_SUBI4 (insn2))
		    {
		      int imm = (insn2 & 0xff) + 1;
		      adjust -= imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: subi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_NOR4 (insn2))
		    {
		      adjust = ~adjust;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: nor r4,r4\n");
			}
		    }
		  else if (CSKY_16_IS_BSETI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust |= (1 << imm);
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: bseti r4, %d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_BCLRI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust &= ~(1 << imm);
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: bclri r4, %d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_LSLI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust <<= imm;
		      if (csky_debug)
			{
			  fprintf_unfiltered (gdb_stdlog,
					      "csky: lsli r4,r4, %d\n", imm);
			}
		    }

		  offset += insn_len;
		  insn_len = csky_get_insn (gdbarch, addr + offset, &insn2);
		};

	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog, "csky: "
				      "done looking for r4 adjusters\n");
		}

	      /* If the next instruction adjusts the stack pointer, we keep
		 everything; if not, we scrap it and we've found the end
		 of the prologue.  */
	      if (CSKY_IS_SUBU4 (insn2))
		{
		  addr += offset;
		  stacksize += adjust;
		  if (csky_debug)
		    {
		      fprintf_unfiltered (gdb_stdlog, "csky: "
					  "found stack adjustment of 0x%x"
					  " bytes.\n", adjust);
		      fprintf_unfiltered (gdb_stdlog, "csky: "
					  "skipping to new address %s\n",
					  core_addr_to_string_nz (addr));
		      fprintf_unfiltered (gdb_stdlog, "csky: continuing\n");
		    }
		  continue;
		}

	      /* None of these instructions are prologue, so don't touch
		 anything.  */
	      if (csky_debug)
		{
		  fprintf_unfiltered (gdb_stdlog, "csky: no subu sp,r4; "
				      "NOT altering stacksize.\n");
		}
	      break;
	    }
	}

      /* This is not a prologue instruction, so stop here.  */
      if (csky_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "csky: insn is not a prologue"
			      " insn -- ending scan\n");
	}
      break;
    }

  if (this_cache)
    {
      CORE_ADDR unwound_fp;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      this_cache->framesize = framesize;

      if (is_fp_saved)
	{
	  this_cache->framereg = CSKY_FP_REGNUM;
	  unwound_fp = get_frame_register_unsigned (this_frame,
						    this_cache->framereg);
	  this_cache->prev_sp = unwound_fp + adjust_fp;
	}
      else
	{
	  this_cache->framereg = CSKY_SP_REGNUM;
	  unwound_fp = get_frame_register_unsigned (this_frame,
						    this_cache->framereg);
	  this_cache->prev_sp = unwound_fp + stacksize;
	}

      /* Note where saved registers are stored.  The offsets in
	 REGISTER_OFFSETS are computed relative to the top of the frame.  */
      for (rn = 0; rn < CSKY_NUM_GREGS; rn++)
	{
	  if (register_offsets[rn] >= 0)
	    {
	      this_cache->saved_regs[rn].addr
		= this_cache->prev_sp - register_offsets[rn];
	      if (csky_debug)
		{
		  CORE_ADDR rn_value = read_memory_unsigned_integer (
		    this_cache->saved_regs[rn].addr, 4, byte_order);
		  fprintf_unfiltered (gdb_stdlog, "Saved register %s "
				      "stored at 0x%08lx, value=0x%08lx\n",
				      csky_register_names[rn],
				      (unsigned long)
					this_cache->saved_regs[rn].addr,
				      (unsigned long) rn_value);
		}
	    }
	}
      if (lr_type == LR_TYPE_EPC)
	{
	  /* rte || epc .  */
	  this_cache->saved_regs[CSKY_PC_REGNUM]
	    = this_cache->saved_regs[CSKY_EPC_REGNUM];
	}
      else if (lr_type == LR_TYPE_FPC)
	{
	  /* rfi || fpc .  */
	  this_cache->saved_regs[CSKY_PC_REGNUM]
	    = this_cache->saved_regs[CSKY_FPC_REGNUM];
	}
      else
	{
	  this_cache->saved_regs[CSKY_PC_REGNUM]
	    = this_cache->saved_regs[CSKY_LR_REGNUM];
	}
    }

  return addr;
}

/* Detect whether PC is at a point where the stack frame has been
   destroyed.  */

static int
csky_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  unsigned int insn;
  CORE_ADDR addr;
  CORE_ADDR func_start, func_end;

  if (!find_pc_partial_function (pc, NULL, &func_start, &func_end))
    return 0;

  bool fp_saved = false;
  int insn_len;
  for (addr = func_start; addr < func_end; addr += insn_len)
    {
      /* Get next insn.  */
      insn_len = csky_get_insn (gdbarch, addr, &insn);

      if (insn_len == 2)
	{
	  /* Is sp is saved to fp.  */
	  if (CSKY_16_IS_MOV_FP_SP (insn))
	    fp_saved = true;
	  /* If sp was saved to fp and now being restored from
	     fp then it indicates the start of epilog.  */
	  else if (fp_saved && CSKY_16_IS_MOV_SP_FP (insn))
	    return pc >= addr;
	}
    }
  return 0;
}

/* Implement the skip_prologue gdbarch hook.  */

static CORE_ADDR
csky_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end;
  struct symtab_and_line sal;
  const int default_search_limit = 128;

  /* See if we can find the end of the prologue using the symbol table.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }
  else
    func_end = pc + default_search_limit;

  /* Find the end of prologue.  Default lr_type.  */
  return csky_analyze_prologue (gdbarch, pc, func_end, func_end,
				NULL, NULL, LR_TYPE_R15);
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
csky_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  if (csky_pc_is_csky16 (gdbarch, *pcptr))
    return CSKY_INSN_SIZE16;
  else
    return CSKY_INSN_SIZE32;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
csky_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  *size = kind;
  if (kind == CSKY_INSN_SIZE16)
    {
      static gdb_byte csky_16_breakpoint[] = { 0, 0 };
      return csky_16_breakpoint;
    }
  else
    {
      static gdb_byte csky_32_breakpoint[] = { 0, 0, 0, 0 };
      return csky_32_breakpoint;
    }
}

/* Implement the memory_insert_breakpoint gdbarch method.  */

static int
csky_memory_insert_breakpoint (struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  int val;
  const unsigned char *bp;
  gdb_byte bp_write_record1[] = { 0, 0, 0, 0 };
  gdb_byte bp_write_record2[] = { 0, 0, 0, 0 };
  gdb_byte bp_record[] = { 0, 0, 0, 0 };

  /* Sanity-check bp_address.  */
  if (bp_tgt->reqstd_address % 2)
    warning (_("Invalid breakpoint address 0x%x is an odd number."),
	     (unsigned int) bp_tgt->reqstd_address);
  scoped_restore restore_memory
    = make_scoped_restore_show_memory_breakpoints (1);

  /* Determine appropriate breakpoint_kind for this address.  */
  bp_tgt->kind = csky_breakpoint_kind_from_pc (gdbarch,
					       &bp_tgt->reqstd_address);

  /* Save the memory contents.  */
  bp_tgt->shadow_len = bp_tgt->kind;

  /* Fill bp_tgt->placed_address.  */
  bp_tgt->placed_address = bp_tgt->reqstd_address;

  if (bp_tgt->kind == CSKY_INSN_SIZE16)
    {
      if ((bp_tgt->reqstd_address % 4) == 0)
	{
	  /* Read two bytes.  */
	  val = target_read_memory (bp_tgt->reqstd_address,
				    bp_tgt->shadow_contents, 2);
	  if (val)
	    return val;

	  /* Read two bytes.  */
	  val = target_read_memory (bp_tgt->reqstd_address + 2,
				    bp_record, 2);
	  if (val)
	    return val;

	  /* Write the breakpoint.  */
	  bp_write_record1[2] = bp_record[0];
	  bp_write_record1[3] = bp_record[1];
	  bp = bp_write_record1;
	  val = target_write_raw_memory (bp_tgt->reqstd_address, bp,
					 CSKY_WR_BKPT_MODE);
	}
      else
	{
	  val = target_read_memory (bp_tgt->reqstd_address,
				    bp_tgt->shadow_contents, 2);
	  if (val)
	    return val;

	  val = target_read_memory (bp_tgt->reqstd_address - 2,
				    bp_record, 2);
	  if (val)
	    return val;

	  /* Write the breakpoint.  */
	  bp_write_record1[0] = bp_record[0];
	  bp_write_record1[1] = bp_record[1];
	  bp = bp_write_record1;
	  val = target_write_raw_memory (bp_tgt->reqstd_address - 2,
					 bp, CSKY_WR_BKPT_MODE);
	}
    }
  else
    {
      if (bp_tgt->placed_address % 4 == 0)
	{
	  val = target_read_memory (bp_tgt->reqstd_address,
				    bp_tgt->shadow_contents,
				    CSKY_WR_BKPT_MODE);
	  if (val)
	    return val;

	  /* Write the breakpoint.  */
	  bp = bp_write_record1;
	  val = target_write_raw_memory (bp_tgt->reqstd_address,
					 bp, CSKY_WR_BKPT_MODE);
	}
      else
	{
	  val = target_read_memory (bp_tgt->reqstd_address,
				    bp_tgt->shadow_contents,
				    CSKY_WR_BKPT_MODE);
	  if (val)
	    return val;

	  val = target_read_memory (bp_tgt->reqstd_address - 2,
				    bp_record, 2);
	  if (val)
	    return val;

	  val = target_read_memory (bp_tgt->reqstd_address + 4,
				    bp_record + 2, 2);
	  if (val)
	    return val;

	  bp_write_record1[0] = bp_record[0];
	  bp_write_record1[1] = bp_record[1];
	  bp_write_record2[2] = bp_record[2];
	  bp_write_record2[3] = bp_record[3];

	  /* Write the breakpoint.  */
	  bp = bp_write_record1;
	  val = target_write_raw_memory (bp_tgt->reqstd_address - 2, bp,
					 CSKY_WR_BKPT_MODE);
	  if (val)
	    return val;

	  /* Write the breakpoint.  */
	  bp = bp_write_record2;
	  val = target_write_raw_memory (bp_tgt->reqstd_address + 2, bp,
					 CSKY_WR_BKPT_MODE);
	}
    }
  return val;
}

/* Restore the breakpoint shadow_contents to the target.  */

static int
csky_memory_remove_breakpoint (struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  int val;
  gdb_byte bp_record[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  /* Different for shadow_len 2 or 4.  */
  if (bp_tgt->shadow_len == 2)
    {
      /* Do word-sized writes on word-aligned boundaries and read
	 padding bytes as necessary.  */
      if (bp_tgt->reqstd_address % 4 == 0)
	{
	  val = target_read_memory (bp_tgt->reqstd_address + 2,
				    bp_record + 2, 2);
	  if (val)
	    return val;
	  bp_record[0] = bp_tgt->shadow_contents[0];
	  bp_record[1] = bp_tgt->shadow_contents[1];
	  return target_write_raw_memory (bp_tgt->reqstd_address,
					  bp_record, CSKY_WR_BKPT_MODE);
	}
      else
	{
	  val = target_read_memory (bp_tgt->reqstd_address - 2,
				    bp_record, 2);
	  if (val)
	    return val;
	  bp_record[2] = bp_tgt->shadow_contents[0];
	  bp_record[3] = bp_tgt->shadow_contents[1];
	  return target_write_raw_memory (bp_tgt->reqstd_address - 2,
					  bp_record, CSKY_WR_BKPT_MODE);
	}
    }
  else
    {
      /* Do word-sized writes on word-aligned boundaries and read
	 padding bytes as necessary.  */
      if (bp_tgt->placed_address % 4 == 0)
	{
	  return target_write_raw_memory (bp_tgt->reqstd_address,
					  bp_tgt->shadow_contents,
					  CSKY_WR_BKPT_MODE);
	}
      else
	{
	  val = target_read_memory (bp_tgt->reqstd_address - 2,
				    bp_record, 2);
	  if (val)
	    return val;
	  val = target_read_memory (bp_tgt->reqstd_address + 4,
				    bp_record+6, 2);
	  if (val)
	    return val;

	  bp_record[2] = bp_tgt->shadow_contents[0];
	  bp_record[3] = bp_tgt->shadow_contents[1];
	  bp_record[4] = bp_tgt->shadow_contents[2];
	  bp_record[5] = bp_tgt->shadow_contents[3];

	  return target_write_raw_memory (bp_tgt->reqstd_address - 2,
					  bp_record,
					  CSKY_WR_BKPT_MODE * 2);
	}
    }
}

/* Determine link register type.  */

static lr_type_t
csky_analyze_lr_type (struct gdbarch *gdbarch,
		      CORE_ADDR start_pc, CORE_ADDR end_pc)
{
  CORE_ADDR addr;
  unsigned int insn, insn_len;
  insn_len = 2;

  for (addr = start_pc; addr < end_pc; addr += insn_len)
    {
      insn_len = csky_get_insn (gdbarch, addr, &insn);
      if (insn_len == 4)
	{
	  if (CSKY_32_IS_MFCR_EPSR (insn) || CSKY_32_IS_MFCR_EPC (insn)
	      || CSKY_32_IS_RTE (insn))
	    return LR_TYPE_EPC;
	}
      else if (CSKY_32_IS_MFCR_FPSR (insn) || CSKY_32_IS_MFCR_FPC (insn)
	       || CSKY_32_IS_RFI (insn))
	return LR_TYPE_FPC;
      else if (CSKY_32_IS_JMP (insn) || CSKY_32_IS_BR (insn)
	       || CSKY_32_IS_JMPIX (insn) || CSKY_32_IS_JMPI (insn))
	return LR_TYPE_R15;
      else
	{
	  /* 16 bit instruction.  */
	  if (CSKY_16_IS_JMP (insn) || CSKY_16_IS_BR (insn)
	      || CSKY_16_IS_JMPIX (insn))
	    return LR_TYPE_R15;
	}
    }
    return LR_TYPE_R15;
}

/* Heuristic unwinder.  */

static struct csky_unwind_cache *
csky_frame_unwind_cache (struct frame_info *this_frame)
{
  CORE_ADDR prologue_start, prologue_end, func_end, prev_pc, block_addr;
  struct csky_unwind_cache *cache;
  const struct block *bl;
  unsigned long func_size = 0;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  unsigned int sp_regnum = CSKY_SP_REGNUM;

  /* Default lr type is r15.  */
  lr_type_t lr_type = LR_TYPE_R15;

  cache = FRAME_OBSTACK_ZALLOC (struct csky_unwind_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Assume there is no frame until proven otherwise.  */
  cache->framereg = sp_regnum;

  cache->framesize = 0;

  prev_pc = get_frame_pc (this_frame);
  block_addr = get_frame_address_in_block (this_frame);
  if (find_pc_partial_function (block_addr, NULL, &prologue_start,
				&func_end) == 0)
    /* We couldn't find a function containing block_addr, so bail out
       and hope for the best.  */
    return cache;

  /* Get the (function) symbol matching prologue_start.  */
  bl = block_for_pc (prologue_start);
  if (bl != NULL)
    func_size = bl->endaddr - bl->startaddr;
  else
    {
      struct bound_minimal_symbol msymbol
	= lookup_minimal_symbol_by_pc (prologue_start);
      if (msymbol.minsym != NULL)
	func_size = MSYMBOL_SIZE (msymbol.minsym);
    }

  /* If FUNC_SIZE is 0 we may have a special-case use of lr
     e.g. exception or interrupt.  */
  if (func_size == 0)
    lr_type = csky_analyze_lr_type (gdbarch, prologue_start, func_end);

  prologue_end = std::min (func_end, prev_pc);

  /* Analyze the function prologue.  */
  csky_analyze_prologue (gdbarch, prologue_start, prologue_end,
			    func_end, this_frame, cache, lr_type);

  /* gdbarch_sp_regnum contains the value and not the address.  */
  trad_frame_set_value (cache->saved_regs, sp_regnum, cache->prev_sp);
  return cache;
}

/* Implement the this_id function for the normal unwinder.  */

static void
csky_frame_this_id (struct frame_info *this_frame,
		    void **this_prologue_cache, struct frame_id *this_id)
{
  struct csky_unwind_cache *cache;
  struct frame_id id;

  if (*this_prologue_cache == NULL)
    *this_prologue_cache = csky_frame_unwind_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_prologue_cache;

  /* This marks the outermost frame.  */
  if (cache->prev_sp == 0)
    return;

  id = frame_id_build (cache->prev_sp, get_frame_func (this_frame));
  *this_id = id;
}

/* Implement the prev_register function for the normal unwinder.  */

static struct value *
csky_frame_prev_register (struct frame_info *this_frame,
			  void **this_prologue_cache, int regnum)
{
  struct csky_unwind_cache *cache;

  if (*this_prologue_cache == NULL)
    *this_prologue_cache = csky_frame_unwind_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_prologue_cache;

  return trad_frame_get_prev_register (this_frame, cache->saved_regs,
				       regnum);
}

/* Data structures for the normal prologue-analysis-based
   unwinder.  */

static const struct frame_unwind csky_unwind_cache = {
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  csky_frame_this_id,
  csky_frame_prev_register,
  NULL,
  default_frame_sniffer,
  NULL,
  NULL
};



static int
csky_stub_unwind_sniffer (const struct frame_unwind *self,
			 struct frame_info *this_frame,
			 void **this_prologue_cache)
{
  CORE_ADDR addr_in_block;

  addr_in_block = get_frame_address_in_block (this_frame);

  if (find_pc_partial_function (addr_in_block, NULL, NULL, NULL) == 0
      || in_plt_section (addr_in_block))
    return 1;

  return 0;
}

static struct csky_unwind_cache *
csky_make_stub_cache (struct frame_info *this_frame)
{
  struct csky_unwind_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct csky_unwind_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);
  cache->prev_sp = get_frame_register_unsigned (this_frame, CSKY_SP_REGNUM);

  return cache;
}

static void
csky_stub_this_id (struct frame_info *this_frame,
		  void **this_cache,
		  struct frame_id *this_id)
{
  struct csky_unwind_cache *cache;

  if (*this_cache == NULL)
    *this_cache = csky_make_stub_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_cache;

  /* Our frame ID for a stub frame is the current SP and LR.  */
  *this_id = frame_id_build (cache->prev_sp, get_frame_pc (this_frame));
}

static struct value *
csky_stub_prev_register (struct frame_info *this_frame,
			    void **this_cache,
			    int prev_regnum)
{
  struct csky_unwind_cache *cache;

  if (*this_cache == NULL)
    *this_cache = csky_make_stub_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_cache;

  /* If we are asked to unwind the PC, then return the LR.  */
  if (prev_regnum == CSKY_PC_REGNUM)
    {
      CORE_ADDR lr;

      lr = frame_unwind_register_unsigned (this_frame, CSKY_LR_REGNUM);
      return frame_unwind_got_constant (this_frame, prev_regnum, lr);
    }

  if (prev_regnum == CSKY_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, prev_regnum, cache->prev_sp);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs,
				       prev_regnum);
}

struct frame_unwind csky_stub_unwind = {
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  csky_stub_this_id,
  csky_stub_prev_register,
  NULL,
  csky_stub_unwind_sniffer
};

/* Implement the this_base, this_locals, and this_args hooks
   for the normal unwinder.  */

static CORE_ADDR
csky_frame_base_address (struct frame_info *this_frame, void **this_cache)
{
  struct csky_unwind_cache *cache;

  if (*this_cache == NULL)
    *this_cache = csky_frame_unwind_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_cache;

  return cache->prev_sp - cache->framesize;
}

static const struct frame_base csky_frame_base = {
  &csky_unwind_cache,
  csky_frame_base_address,
  csky_frame_base_address,
  csky_frame_base_address
};

/* Initialize register access method.  */

static void
csky_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			    struct dwarf2_frame_state_reg *reg,
			    struct frame_info *this_frame)
{
  if (regnum == gdbarch_pc_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_RA;
  else if (regnum == gdbarch_sp_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_CFA;
}

/* Create csky register groups.  */

static void
csky_init_reggroup ()
{
  cr_reggroup = reggroup_new ("cr", USER_REGGROUP);
  fr_reggroup = reggroup_new ("fr", USER_REGGROUP);
  vr_reggroup = reggroup_new ("vr", USER_REGGROUP);
  mmu_reggroup = reggroup_new ("mmu", USER_REGGROUP);
  prof_reggroup = reggroup_new ("profiling", USER_REGGROUP);
}

/* Add register groups into reggroup list.  */

static void
csky_add_reggroups (struct gdbarch *gdbarch)
{
  reggroup_add (gdbarch, all_reggroup);
  reggroup_add (gdbarch, general_reggroup);
  reggroup_add (gdbarch, cr_reggroup);
  reggroup_add (gdbarch, fr_reggroup);
  reggroup_add (gdbarch, vr_reggroup);
  reggroup_add (gdbarch, mmu_reggroup);
  reggroup_add (gdbarch, prof_reggroup);
}

/* Return the groups that a CSKY register can be categorised into.  */

static int
csky_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			  struct reggroup *reggroup)
{
  int raw_p;

  if (gdbarch_register_name (gdbarch, regnum) == NULL
      || gdbarch_register_name (gdbarch, regnum)[0] == '\0')
    return 0;

  if (reggroup == all_reggroup)
    return 1;

  raw_p = regnum < gdbarch_num_regs (gdbarch);
  if (reggroup == save_reggroup || reggroup == restore_reggroup)
    return raw_p;

  if (((regnum >= CSKY_R0_REGNUM) && (regnum <= CSKY_R0_REGNUM + 31))
      && (reggroup == general_reggroup))
    return 1;

  if (((regnum == CSKY_PC_REGNUM)
       || ((regnum >= CSKY_CR0_REGNUM)
	   && (regnum <= CSKY_CR0_REGNUM + 30)))
      && (reggroup == cr_reggroup))
    return 2;

  if ((((regnum >= CSKY_VR0_REGNUM) && (regnum <= CSKY_VR0_REGNUM + 15))
       || ((regnum >= CSKY_VCR0_REGNUM)
	   && (regnum <= CSKY_VCR0_REGNUM + 2)))
      && (reggroup == vr_reggroup))
    return 3;

  if (((regnum >= CSKY_MMU_REGNUM) && (regnum <= CSKY_MMU_REGNUM + 8))
      && (reggroup == mmu_reggroup))
    return 4;

  if (((regnum >= CSKY_PROFCR_REGNUM)
       && (regnum <= CSKY_PROFCR_REGNUM + 48))
      && (reggroup == prof_reggroup))
    return 5;

  if ((((regnum >= CSKY_FR0_REGNUM) && (regnum <= CSKY_FR0_REGNUM + 15))
       || ((regnum >= CSKY_VCR0_REGNUM) && (regnum <= CSKY_VCR0_REGNUM + 2)))
      && (reggroup == fr_reggroup))
    return 6;

  return 0;
}

/* Implement the dwarf2_reg_to_regnum gdbarch method.  */

static int
csky_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int dw_reg)
{
  if (dw_reg < 0 || dw_reg >= CSKY_NUM_REGS)
    return -1;
  return dw_reg;
}

/* Override interface for command: info register.  */

static void
csky_print_registers_info (struct gdbarch *gdbarch, struct ui_file *file,
			   struct frame_info *frame, int regnum, int all)
{
  /* Call default print_registers_info function.  */
  default_print_registers_info (gdbarch, file, frame, regnum, all);

  /* For command: info register.  */
  if (regnum == -1 && all == 0)
    {
      default_print_registers_info (gdbarch, file, frame,
				    CSKY_PC_REGNUM, 0);
      default_print_registers_info (gdbarch, file, frame,
				    CSKY_EPC_REGNUM, 0);
      default_print_registers_info (gdbarch, file, frame,
				    CSKY_CR0_REGNUM, 0);
      default_print_registers_info (gdbarch, file, frame,
				    CSKY_EPSR_REGNUM, 0);
    }
  return;
}

/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
csky_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;

  /* Find a candidate among the list of pre-declared architectures.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found, create a new architecture from the information
     provided.  */
  tdep = XCNEW (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  /* Target data types.  */
  set_gdbarch_ptr_bit (gdbarch, 32);
  set_gdbarch_addr_bit (gdbarch, 32);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);

  /* Information about the target architecture.  */
  set_gdbarch_return_value (gdbarch, csky_return_value);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, csky_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, csky_sw_breakpoint_from_kind);

  /* Register architecture.  */
  set_gdbarch_num_regs (gdbarch, CSKY_NUM_REGS);
  set_gdbarch_pc_regnum (gdbarch, CSKY_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, CSKY_SP_REGNUM);
  set_gdbarch_register_name (gdbarch, csky_register_name);
  set_gdbarch_register_type (gdbarch, csky_register_type);
  set_gdbarch_read_pc (gdbarch, csky_read_pc);
  set_gdbarch_write_pc (gdbarch, csky_write_pc);
  set_gdbarch_print_registers_info (gdbarch, csky_print_registers_info);
  csky_add_reggroups (gdbarch);
  set_gdbarch_register_reggroup_p (gdbarch, csky_register_reggroup_p);
  set_gdbarch_stab_reg_to_regnum (gdbarch, csky_dwarf_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, csky_dwarf_reg_to_regnum);
  dwarf2_frame_set_init_reg (gdbarch, csky_dwarf2_frame_init_reg);

  /* Functions to analyze frames.  */
  frame_base_set_default (gdbarch, &csky_frame_base);
  set_gdbarch_skip_prologue (gdbarch, csky_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_frame_align (gdbarch, csky_frame_align);
  set_gdbarch_stack_frame_destroyed_p (gdbarch, csky_stack_frame_destroyed_p);

  /* Functions handling dummy frames.  */
  set_gdbarch_push_dummy_call (gdbarch, csky_push_dummy_call);

  /* Frame unwinders.  Use DWARF debug info if available,
     otherwise use our own unwinder.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &csky_stub_unwind);
  frame_unwind_append_unwinder (gdbarch, &csky_unwind_cache);

  /* Breakpoints.  */
  set_gdbarch_memory_insert_breakpoint (gdbarch,
					csky_memory_insert_breakpoint);
  set_gdbarch_memory_remove_breakpoint (gdbarch,
					csky_memory_remove_breakpoint);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Support simple overlay manager.  */
  set_gdbarch_overlay_update (gdbarch, simple_overlay_update);
  set_gdbarch_char_signed (gdbarch, 0);
  return gdbarch;
}

void _initialize_csky_tdep ();
void
_initialize_csky_tdep ()
{

  register_gdbarch_init (bfd_arch_csky, csky_gdbarch_init);

  csky_init_reggroup ();

  /* Allow debugging this file's internals.  */
  add_setshow_boolean_cmd ("csky", class_maintenance, &csky_debug,
			   _("Set C-Sky debugging."),
			   _("Show C-Sky debugging."),
			   _("When on, C-Sky specific debugging is enabled."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);
}
