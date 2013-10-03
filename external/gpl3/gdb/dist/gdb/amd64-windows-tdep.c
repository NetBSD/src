/* Copyright (C) 2009-2013 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "amd64-tdep.h"
#include "solib.h"
#include "solib-target.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "regcache.h"
#include "windows-tdep.h"
#include "frame.h"

/* The registers used to pass integer arguments during a function call.  */
static int amd64_windows_dummy_call_integer_regs[] =
{
  AMD64_RCX_REGNUM,          /* %rcx */
  AMD64_RDX_REGNUM,          /* %rdx */
  8,                         /* %r8 */
  9                          /* %r9 */
};

/* Implement the "classify" method in the gdbarch_tdep structure
   for amd64-windows.  */

static void
amd64_windows_classify (struct type *type, enum amd64_reg_class class[2])
{
  switch (TYPE_CODE (type))
    {
      case TYPE_CODE_ARRAY:
	/* Arrays are always passed by memory.	*/
	class[0] = class[1] = AMD64_MEMORY;
	break;

      case TYPE_CODE_STRUCT:
      case TYPE_CODE_UNION:
        /* Struct/Union types whose size is 1, 2, 4, or 8 bytes
	   are passed as if they were integers of the same size.
	   Types of different sizes are passed by memory.  */
	if (TYPE_LENGTH (type) == 1
	    || TYPE_LENGTH (type) == 2
	    || TYPE_LENGTH (type) == 4
	    || TYPE_LENGTH (type) == 8)
	  {
	    class[0] = AMD64_INTEGER;
	    class[1] = AMD64_NO_CLASS;
	  }
	else
	  class[0] = class[1] = AMD64_MEMORY;
	break;

      default:
	/* For all the other types, the conventions are the same as
	   with the System V ABI.  */
	amd64_classify (type, class);
    }
}

/* Implement the "return_value" gdbarch method for amd64-windows.  */

static enum return_value_convention
amd64_windows_return_value (struct gdbarch *gdbarch, struct value *function,
			    struct type *type, struct regcache *regcache,
			    gdb_byte *readbuf, const gdb_byte *writebuf)
{
  int len = TYPE_LENGTH (type);
  int regnum = -1;

  /* See if our value is returned through a register.  If it is, then
     store the associated register number in REGNUM.  */
  switch (TYPE_CODE (type))
    {
      case TYPE_CODE_FLT:
      case TYPE_CODE_DECFLOAT:
        /* __m128, __m128i, __m128d, floats, and doubles are returned
           via XMM0.  */
        if (len == 4 || len == 8 || len == 16)
          regnum = AMD64_XMM0_REGNUM;
        break;
      default:
        /* All other values that are 1, 2, 4 or 8 bytes long are returned
           via RAX.  */
        if (len == 1 || len == 2 || len == 4 || len == 8)
          regnum = AMD64_RAX_REGNUM;
        break;
    }

  if (regnum < 0)
    {
      /* RAX contains the address where the return value has been stored.  */
      if (readbuf)
        {
	  ULONGEST addr;

	  regcache_raw_read_unsigned (regcache, AMD64_RAX_REGNUM, &addr);
	  read_memory (addr, readbuf, TYPE_LENGTH (type));
	}
      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }
  else
    {
      /* Extract the return value from the register where it was stored.  */
      if (readbuf)
	regcache_raw_read_part (regcache, regnum, 0, len, readbuf);
      if (writebuf)
	regcache_raw_write_part (regcache, regnum, 0, len, writebuf);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Check that the code pointed to by PC corresponds to a call to
   __main, skip it if so.  Return PC otherwise.  */

static CORE_ADDR
amd64_skip_main_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte op;

  target_read_memory (pc, &op, 1);
  if (op == 0xe8)
    {
      gdb_byte buf[4];

      if (target_read_memory (pc + 1, buf, sizeof buf) == 0)
 	{
 	  struct minimal_symbol *s;
 	  CORE_ADDR call_dest;

	  call_dest = pc + 5 + extract_signed_integer (buf, 4, byte_order);
 	  s = lookup_minimal_symbol_by_pc (call_dest);
 	  if (s != NULL
 	      && SYMBOL_LINKAGE_NAME (s) != NULL
 	      && strcmp (SYMBOL_LINKAGE_NAME (s), "__main") == 0)
 	    pc += 5;
 	}
    }

  return pc;
}

/* Check Win64 DLL jmp trampolines and find jump destination.  */

static CORE_ADDR
amd64_windows_skip_trampoline_code (struct frame_info *frame, CORE_ADDR pc)
{
  CORE_ADDR destination = 0;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Check for jmp *<offset>(%rip) (jump near, absolute indirect (/4)).  */
  if (pc && read_memory_unsigned_integer (pc, 2, byte_order) == 0x25ff)
    {
      /* Get opcode offset and see if we can find a reference in our data.  */
      ULONGEST offset
	= read_memory_unsigned_integer (pc + 2, 4, byte_order);

      /* Get address of function pointer at end of pc.  */
      CORE_ADDR indirect_addr = pc + offset + 6;

      struct minimal_symbol *indsym
	= indirect_addr ? lookup_minimal_symbol_by_pc (indirect_addr) : NULL;
      const char *symname = indsym ? SYMBOL_LINKAGE_NAME (indsym) : NULL;

      if (symname)
	{
	  if (strncmp (symname, "__imp_", 6) == 0
	      || strncmp (symname, "_imp_", 5) == 0)
	    destination
	      = read_memory_unsigned_integer (indirect_addr, 8, byte_order);
	}
    }

  return destination;
}

/* Implement the "auto_wide_charset" gdbarch method.  */

static const char *
amd64_windows_auto_wide_charset (void)
{
  return "UTF-16";
}

static void
amd64_windows_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  amd64_init_abi (info, gdbarch);

  /* On Windows, "long"s are only 32bit.  */
  set_gdbarch_long_bit (gdbarch, 32);

  /* Function calls.  */
  tdep->call_dummy_num_integer_regs =
    ARRAY_SIZE (amd64_windows_dummy_call_integer_regs);
  tdep->call_dummy_integer_regs = amd64_windows_dummy_call_integer_regs;
  tdep->classify = amd64_windows_classify;
  tdep->memory_args_by_pointer = 1;
  tdep->integer_param_regs_saved_in_caller_frame = 1;
  set_gdbarch_return_value (gdbarch, amd64_windows_return_value);
  set_gdbarch_skip_main_prologue (gdbarch, amd64_skip_main_prologue);
  set_gdbarch_skip_trampoline_code (gdbarch,
				    amd64_windows_skip_trampoline_code);

  set_gdbarch_iterate_over_objfiles_in_search_order
    (gdbarch, windows_iterate_over_objfiles_in_search_order);

  set_gdbarch_auto_wide_charset (gdbarch, amd64_windows_auto_wide_charset);

  set_solib_ops (gdbarch, &solib_target_so_ops);
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_amd64_windows_tdep;

void
_initialize_amd64_windows_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64, GDB_OSABI_CYGWIN,
                          amd64_windows_init_abi);
}

