/* Self tests for disassembler for GDB, the GNU debugger.

   Copyright (C) 2017 Free Software Foundation, Inc.

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
#include "disasm.h"

#if GDB_SELF_TEST
#include "selftest.h"
#include "selftest-arch.h"

namespace selftests {

/* Test disassembly of one instruction.  */

static void
print_one_insn_test (struct gdbarch *gdbarch)
{
  size_t len = 0;
  const gdb_byte *insn = NULL;

  switch (gdbarch_bfd_arch_info (gdbarch)->arch)
    {
    case bfd_arch_bfin:
      /* M3.L = 0xe117 */
      static const gdb_byte bfin_insn[] = {0x17, 0xe1, 0xff, 0xff};

      insn = bfin_insn;
      len = sizeof (bfin_insn);
      break;
    case bfd_arch_arm:
      /* mov     r0, #0 */
      static const gdb_byte arm_insn[] = {0x0, 0x0, 0xa0, 0xe3};

      insn = arm_insn;
      len = sizeof (arm_insn);
      break;
    case bfd_arch_ia64:
    case bfd_arch_mep:
    case bfd_arch_mips:
    case bfd_arch_tic6x:
    case bfd_arch_xtensa:
      return;
    case bfd_arch_s390:
      /* nopr %r7 */
      static const gdb_byte s390_insn[] = {0x07, 0x07};

      insn = s390_insn;
      len = sizeof (s390_insn);
      break;
    case bfd_arch_xstormy16:
      /* nop */
      static const gdb_byte xstormy16_insn[] = {0x0, 0x0};

      insn = xstormy16_insn;
      len = sizeof (xstormy16_insn);
      break;
    case bfd_arch_arc:
      /* PR 21003 */
      if (gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_arc_arc601)
	return;
      /* fall through */
    case bfd_arch_nios2:
    case bfd_arch_score:
      /* nios2 and score need to know the current instruction to select
	 breakpoint instruction.  Give the breakpoint instruction kind
	 explicitly.  */
      int bplen;
      insn = gdbarch_sw_breakpoint_from_kind (gdbarch, 4, &bplen);
      len = bplen;
      break;
    default:
      {
	/* Test disassemble breakpoint instruction.  */
	CORE_ADDR pc = 0;
	int kind = gdbarch_breakpoint_kind_from_pc (gdbarch, &pc);
	int bplen;

	insn = gdbarch_sw_breakpoint_from_kind (gdbarch, kind, &bplen);
	len = bplen;

	break;
      }
    }
  SELF_CHECK (len > 0);

  /* Test gdb_disassembler for a given gdbarch by reading data from a
     pre-allocated buffer.  If you want to see the disassembled
     instruction printed to gdb_stdout, set verbose to true.  */
  static const bool verbose = false;

  class gdb_disassembler_test : public gdb_disassembler
  {
  public:

    explicit gdb_disassembler_test (struct gdbarch *gdbarch,
				    const gdb_byte *insn,
				    size_t len)
      : gdb_disassembler (gdbarch,
			  (verbose ? gdb_stdout : &null_stream),
			  gdb_disassembler_test::read_memory),
	m_insn (insn), m_len (len)
    {
    }

    int
    print_insn (CORE_ADDR memaddr)
    {
      if (verbose)
	{
	  fprintf_unfiltered (stream (), "%s ",
			      gdbarch_bfd_arch_info (arch ())->arch_name);
	}

      int len = gdb_disassembler::print_insn (memaddr);

      if (verbose)
	fprintf_unfiltered (stream (), "\n");

      return len;
    }

  private:
    /* A buffer contain one instruction.  */
    const gdb_byte *m_insn;

    /* Length of the buffer.  */
    size_t m_len;

    static int read_memory (bfd_vma memaddr, gdb_byte *myaddr,
			    unsigned int len, struct disassemble_info *info)
    {
      gdb_disassembler_test *self
	= static_cast<gdb_disassembler_test *>(info->application_data);

      /* The disassembler in opcodes may read more data than one
	 instruction.  Supply infinite consecutive copies
	 of the same instruction.  */
      for (size_t i = 0; i < len; i++)
	myaddr[i] = self->m_insn[(memaddr + i) % self->m_len];

      return 0;
    }
  };

  gdb_disassembler_test di (gdbarch, insn, len);

  SELF_CHECK (di.print_insn (0) == len);
}

/* Test disassembly on memory error.  */

static void
memory_error_test (struct gdbarch *gdbarch)
{
  class gdb_disassembler_test : public gdb_disassembler
  {
  public:
    gdb_disassembler_test (struct gdbarch *gdbarch)
      : gdb_disassembler (gdbarch, &null_stream,
			  gdb_disassembler_test::read_memory)
    {
    }

    static int read_memory (bfd_vma memaddr, gdb_byte *myaddr,
			    unsigned int len,
			    struct disassemble_info *info)
    {
      /* Always return an error.  */
      return -1;
    }
  };

  gdb_disassembler_test di (gdbarch);
  bool saw_memory_error = false;

  TRY
    {
      di.print_insn (0);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.error == MEMORY_ERROR)
	saw_memory_error = true;
    }
  END_CATCH

  /* Expect MEMORY_ERROR.  */
  SELF_CHECK (saw_memory_error);
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

/* Suppress warning from -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_disasm_selftests;

void
_initialize_disasm_selftests (void)
{
#if GDB_SELF_TEST
  register_self_test_foreach_arch (selftests::print_one_insn_test);
  register_self_test_foreach_arch (selftests::memory_error_test);
#endif
}
