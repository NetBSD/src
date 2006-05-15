/* GNU/Linux on ARM target support.

   Copyright 1999, 2000, 2001, 2002, 2003, 2005 Free Software
   Foundation, Inc.

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
#include "target.h"
#include "value.h"
#include "gdbtypes.h"
#include "floatformat.h"
#include "gdbcore.h"
#include "frame.h"
#include "regcache.h"
#include "doublest.h"
#include "solib-svr4.h"
#include "osabi.h"

#include "arm-tdep.h"
#include "glibc-tdep.h"

/* Under ARM GNU/Linux the traditional way of performing a breakpoint
   is to execute a particular software interrupt, rather than use a
   particular undefined instruction to provoke a trap.  Upon exection
   of the software interrupt the kernel stops the inferior with a
   SIGTRAP, and wakes the debugger.  */

static const char arm_linux_arm_le_breakpoint[] = { 0x01, 0x00, 0x9f, 0xef };

static const char arm_linux_arm_be_breakpoint[] = { 0xef, 0x9f, 0x00, 0x01 };

/* However, the EABI syscall interface (new in Nov. 2005) does not look at
   the operand of the swi if old-ABI compatibility is disabled.  Therefore,
   use an undefined instruction instead.  This is supported as of kernel
   version 2.5.70 (May 2003), so should be a safe assumption for EABI
   binaries.  */

static const char eabi_linux_arm_le_breakpoint[] = { 0xf0, 0x01, 0xf0, 0xe7 };

static const char eabi_linux_arm_be_breakpoint[] = { 0xe7, 0xf0, 0x01, 0xf0 };

/* All the kernels which support Thumb support using a specific undefined
   instruction for the Thumb breakpoint.  */

static const char arm_linux_thumb_be_breakpoint[] = {0xde, 0x01};

static const char arm_linux_thumb_le_breakpoint[] = {0x01, 0xde};

/* Description of the longjmp buffer.  */
#define ARM_LINUX_JB_ELEMENT_SIZE	INT_REGISTER_SIZE
#define ARM_LINUX_JB_PC			21

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */
/* FIXME rearnsha/2002-02-23: This function shouldn't be necessary.
   The ARM generic one should be able to handle the model used by
   linux and the low-level formatting of the registers should be
   hidden behind the regcache abstraction.  */
static void
arm_linux_extract_return_value (struct type *type,
				char regbuf[],
				char *valbuf)
{
  /* ScottB: This needs to be looked at to handle the different
     floating point emulators on ARM GNU/Linux.  Right now the code
     assumes that fetch inferior registers does the right thing for
     GDB.  I suspect this won't handle NWFPE registers correctly, nor
     will the default ARM version (arm_extract_return_value()).  */

  int regnum = ((TYPE_CODE_FLT == TYPE_CODE (type))
		? ARM_F0_REGNUM : ARM_A1_REGNUM);
  memcpy (valbuf, &regbuf[DEPRECATED_REGISTER_BYTE (regnum)], TYPE_LENGTH (type));
}
   	  
/*
   Dynamic Linking on ARM GNU/Linux
   --------------------------------

   Note: PLT = procedure linkage table
   GOT = global offset table

   As much as possible, ELF dynamic linking defers the resolution of
   jump/call addresses until the last minute. The technique used is
   inspired by the i386 ELF design, and is based on the following
   constraints.

   1) The calling technique should not force a change in the assembly
   code produced for apps; it MAY cause changes in the way assembly
   code is produced for position independent code (i.e. shared
   libraries).

   2) The technique must be such that all executable areas must not be
   modified; and any modified areas must not be executed.

   To do this, there are three steps involved in a typical jump:

   1) in the code
   2) through the PLT
   3) using a pointer from the GOT

   When the executable or library is first loaded, each GOT entry is
   initialized to point to the code which implements dynamic name
   resolution and code finding.  This is normally a function in the
   program interpreter (on ARM GNU/Linux this is usually
   ld-linux.so.2, but it does not have to be).  On the first
   invocation, the function is located and the GOT entry is replaced
   with the real function address.  Subsequent calls go through steps
   1, 2 and 3 and end up calling the real code.

   1) In the code: 

   b    function_call
   bl   function_call

   This is typical ARM code using the 26 bit relative branch or branch
   and link instructions.  The target of the instruction
   (function_call is usually the address of the function to be called.
   In position independent code, the target of the instruction is
   actually an entry in the PLT when calling functions in a shared
   library.  Note that this call is identical to a normal function
   call, only the target differs.

   2) In the PLT:

   The PLT is a synthetic area, created by the linker. It exists in
   both executables and libraries. It is an array of stubs, one per
   imported function call. It looks like this:

   PLT[0]:
   str     lr, [sp, #-4]!       @push the return address (lr)
   ldr     lr, [pc, #16]   @load from 6 words ahead
   add     lr, pc, lr      @form an address for GOT[0]
   ldr     pc, [lr, #8]!   @jump to the contents of that addr

   The return address (lr) is pushed on the stack and used for
   calculations.  The load on the second line loads the lr with
   &GOT[3] - . - 20.  The addition on the third leaves:

   lr = (&GOT[3] - . - 20) + (. + 8)
   lr = (&GOT[3] - 12)
   lr = &GOT[0]

   On the fourth line, the pc and lr are both updated, so that:

   pc = GOT[2]
   lr = &GOT[0] + 8
   = &GOT[2]

   NOTE: PLT[0] borrows an offset .word from PLT[1]. This is a little
   "tight", but allows us to keep all the PLT entries the same size.

   PLT[n+1]:
   ldr     ip, [pc, #4]    @load offset from gotoff
   add     ip, pc, ip      @add the offset to the pc
   ldr     pc, [ip]        @jump to that address
   gotoff: .word   GOT[n+3] - .

   The load on the first line, gets an offset from the fourth word of
   the PLT entry.  The add on the second line makes ip = &GOT[n+3],
   which contains either a pointer to PLT[0] (the fixup trampoline) or
   a pointer to the actual code.

   3) In the GOT:

   The GOT contains helper pointers for both code (PLT) fixups and
   data fixups.  The first 3 entries of the GOT are special. The next
   M entries (where M is the number of entries in the PLT) belong to
   the PLT fixups. The next D (all remaining) entries belong to
   various data fixups. The actual size of the GOT is 3 + M + D.

   The GOT is also a synthetic area, created by the linker. It exists
   in both executables and libraries.  When the GOT is first
   initialized , all the GOT entries relating to PLT fixups are
   pointing to code back at PLT[0].

   The special entries in the GOT are:

   GOT[0] = linked list pointer used by the dynamic loader
   GOT[1] = pointer to the reloc table for this module
   GOT[2] = pointer to the fixup/resolver code

   The first invocation of function call comes through and uses the
   fixup/resolver code.  On the entry to the fixup/resolver code:

   ip = &GOT[n+3]
   lr = &GOT[2]
   stack[0] = return address (lr) of the function call
   [r0, r1, r2, r3] are still the arguments to the function call

   This is enough information for the fixup/resolver code to work
   with.  Before the fixup/resolver code returns, it actually calls
   the requested function and repairs &GOT[n+3].  */

/* Fetch, and possibly build, an appropriate link_map_offsets structure
   for ARM linux targets using the struct offsets defined in <link.h>.
   Note, however, that link.h is not actually referred to in this file.
   Instead, the relevant structs offsets were obtained from examining
   link.h.  (We can't refer to link.h from this file because the host
   system won't necessarily have it, or if it does, the structs which
   it defines will refer to the host system, not the target).  */

static struct link_map_offsets *
arm_linux_svr4_fetch_link_map_offsets (void)
{
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = 0;

  if (lmp == 0)
    {
      lmp = &lmo;

      lmo.r_debug_size = 8;	/* Actual size is 20, but this is all we
                                   need.  */

      lmo.r_map_offset = 4;
      lmo.r_map_size   = 4;

      lmo.link_map_size = 20;	/* Actual size is 552, but this is all we
                                   need.  */

      lmo.l_addr_offset = 0;
      lmo.l_addr_size   = 4;

      lmo.l_name_offset = 4;
      lmo.l_name_size   = 4;

      lmo.l_next_offset = 12;
      lmo.l_next_size   = 4;

      lmo.l_prev_offset = 16;
      lmo.l_prev_size   = 4;
    }

    return lmp;
}

/* The constants below were determined by examining the following files
   in the linux kernel sources:

      arch/arm/kernel/signal.c
	  - see SWI_SYS_SIGRETURN and SWI_SYS_RT_SIGRETURN
      include/asm-arm/unistd.h
	  - see __NR_sigreturn, __NR_rt_sigreturn, and __NR_SYSCALL_BASE */

#define ARM_LINUX_SIGRETURN_INSTR	0xef900077
#define ARM_LINUX_RT_SIGRETURN_INSTR	0xef9000ad

/* arm_linux_in_sigtramp determines if PC points at one of the
   instructions which cause control to return to the Linux kernel upon
   return from a signal handler.  FUNC_NAME is unused.  */

int
arm_linux_in_sigtramp (CORE_ADDR pc, char *func_name)
{
  unsigned long inst;

  inst = read_memory_integer (pc, 4);

  return (inst == ARM_LINUX_SIGRETURN_INSTR
	  || inst == ARM_LINUX_RT_SIGRETURN_INSTR);

}

/* arm_linux_sigcontext_register_address returns the address in the
   sigcontext of register REGNO given a stack pointer value SP and
   program counter value PC.  The value 0 is returned if PC is not
   pointing at one of the signal return instructions or if REGNO is
   not saved in the sigcontext struct.  */

CORE_ADDR
arm_linux_sigcontext_register_address (CORE_ADDR sp, CORE_ADDR pc, int regno)
{
  unsigned long inst;
  CORE_ADDR reg_addr = 0;

  inst = read_memory_integer (pc, 4);

  if (inst == ARM_LINUX_SIGRETURN_INSTR
      || inst == ARM_LINUX_RT_SIGRETURN_INSTR)
    {
      CORE_ADDR sigcontext_addr;

      /* The sigcontext structure is at different places for the two
         signal return instructions.  For ARM_LINUX_SIGRETURN_INSTR,
	 it starts at the SP value.  For ARM_LINUX_RT_SIGRETURN_INSTR,
	 it is at SP+8.  For the latter instruction, it may also be
	 the case that the address of this structure may be determined
	 by reading the 4 bytes at SP, but I'm not convinced this is
	 reliable.

	 In any event, these magic constants (0 and 8) may be
	 determined by examining struct sigframe and struct
	 rt_sigframe in arch/arm/kernel/signal.c in the Linux kernel
	 sources.  */

      if (inst == ARM_LINUX_RT_SIGRETURN_INSTR)
	sigcontext_addr = sp + 8;
      else /* inst == ARM_LINUX_SIGRETURN_INSTR */
        sigcontext_addr = sp + 0;

      /* The layout of the sigcontext structure for ARM GNU/Linux is
         in include/asm-arm/sigcontext.h in the Linux kernel sources.

	 There are three 4-byte fields which precede the saved r0
	 field.  (This accounts for the 12 in the code below.)  The
	 sixteen registers (4 bytes per field) follow in order.  The
	 PSR value follows the sixteen registers which accounts for
	 the constant 19 below. */

      if (0 <= regno && regno <= ARM_PC_REGNUM)
	reg_addr = sigcontext_addr + 12 + (4 * regno);
      else if (regno == ARM_PS_REGNUM)
	reg_addr = sigcontext_addr + 19 * 4;
    }

  return reg_addr;
}

static void
arm_linux_init_abi (struct gdbarch_info info,
		    struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  tdep->lowest_pc = 0x8000;
  if (info.byte_order == BFD_ENDIAN_BIG)
    {
      if (tdep->arm_abi == ARM_ABI_AAPCS)
	tdep->arm_breakpoint = eabi_linux_arm_be_breakpoint;
      else
	tdep->arm_breakpoint = arm_linux_arm_be_breakpoint;
      tdep->thumb_breakpoint = arm_linux_thumb_be_breakpoint;
    }
  else
    {
      if (tdep->arm_abi == ARM_ABI_AAPCS)
	tdep->arm_breakpoint = eabi_linux_arm_le_breakpoint;
      else
	tdep->arm_breakpoint = arm_linux_arm_le_breakpoint;
      tdep->thumb_breakpoint = arm_linux_thumb_le_breakpoint;
    }
  tdep->arm_breakpoint_size = sizeof (arm_linux_arm_le_breakpoint);
  tdep->thumb_breakpoint_size = sizeof (arm_linux_thumb_le_breakpoint);

  if (tdep->fp_model == ARM_FLOAT_AUTO)
    tdep->fp_model = ARM_FLOAT_FPA;

  tdep->jb_pc = ARM_LINUX_JB_PC;
  tdep->jb_elt_size = ARM_LINUX_JB_ELEMENT_SIZE;

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, arm_linux_svr4_fetch_link_map_offsets);

  /* The following override shouldn't be needed.  */
  set_gdbarch_deprecated_extract_return_value (gdbarch, arm_linux_extract_return_value);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);
}

void
_initialize_arm_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_LINUX,
			  arm_linux_init_abi);
}
