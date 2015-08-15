/* GNU/Linux on ARM target support.

   Copyright (C) 1999-2014 Free Software Foundation, Inc.

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
#include "regset.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "breakpoint.h"
#include "auxv.h"
#include "xml-syscall.h"

#include "arm-tdep.h"
#include "arm-linux-tdep.h"
#include "linux-tdep.h"
#include "glibc-tdep.h"
#include "arch-utils.h"
#include "inferior.h"
#include "gdbthread.h"
#include "symfile.h"

#include "cli/cli-utils.h"
#include "stap-probe.h"
#include "parser-defs.h"
#include "user-regs.h"
#include <ctype.h>
#include "elf/common.h"
#include <string.h>

extern int arm_apcs_32;

/* Under ARM GNU/Linux the traditional way of performing a breakpoint
   is to execute a particular software interrupt, rather than use a
   particular undefined instruction to provoke a trap.  Upon exection
   of the software interrupt the kernel stops the inferior with a
   SIGTRAP, and wakes the debugger.  */

static const gdb_byte arm_linux_arm_le_breakpoint[] = { 0x01, 0x00, 0x9f, 0xef };

static const gdb_byte arm_linux_arm_be_breakpoint[] = { 0xef, 0x9f, 0x00, 0x01 };

/* However, the EABI syscall interface (new in Nov. 2005) does not look at
   the operand of the swi if old-ABI compatibility is disabled.  Therefore,
   use an undefined instruction instead.  This is supported as of kernel
   version 2.5.70 (May 2003), so should be a safe assumption for EABI
   binaries.  */

static const gdb_byte eabi_linux_arm_le_breakpoint[] = { 0xf0, 0x01, 0xf0, 0xe7 };

static const gdb_byte eabi_linux_arm_be_breakpoint[] = { 0xe7, 0xf0, 0x01, 0xf0 };

/* All the kernels which support Thumb support using a specific undefined
   instruction for the Thumb breakpoint.  */

static const gdb_byte arm_linux_thumb_be_breakpoint[] = {0xde, 0x01};

static const gdb_byte arm_linux_thumb_le_breakpoint[] = {0x01, 0xde};

/* Because the 16-bit Thumb breakpoint is affected by Thumb-2 IT blocks,
   we must use a length-appropriate breakpoint for 32-bit Thumb
   instructions.  See also thumb_get_next_pc.  */

static const gdb_byte arm_linux_thumb2_be_breakpoint[] = { 0xf7, 0xf0, 0xa0, 0x00 };

static const gdb_byte arm_linux_thumb2_le_breakpoint[] = { 0xf0, 0xf7, 0x00, 0xa0 };

/* Description of the longjmp buffer.  The buffer is treated as an array of 
   elements of size ARM_LINUX_JB_ELEMENT_SIZE.

   The location of saved registers in this buffer (in particular the PC
   to use after longjmp is called) varies depending on the ABI (in 
   particular the FP model) and also (possibly) the C Library.

   For glibc, eglibc, and uclibc the following holds:  If the FP model is 
   SoftVFP or VFP (which implies EABI) then the PC is at offset 9 in the 
   buffer.  This is also true for the SoftFPA model.  However, for the FPA 
   model the PC is at offset 21 in the buffer.  */
#define ARM_LINUX_JB_ELEMENT_SIZE	INT_REGISTER_SIZE
#define ARM_LINUX_JB_PC_FPA		21
#define ARM_LINUX_JB_PC_EABI		9

/*
   Dynamic Linking on ARM GNU/Linux
   --------------------------------

   Note: PLT = procedure linkage table
   GOT = global offset table

   As much as possible, ELF dynamic linking defers the resolution of
   jump/call addresses until the last minute.  The technique used is
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

   The PLT is a synthetic area, created by the linker.  It exists in
   both executables and libraries.  It is an array of stubs, one per
   imported function call.  It looks like this:

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

   NOTE: PLT[0] borrows an offset .word from PLT[1].  This is a little
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
   data fixups.  The first 3 entries of the GOT are special.  The next
   M entries (where M is the number of entries in the PLT) belong to
   the PLT fixups.  The next D (all remaining) entries belong to
   various data fixups.  The actual size of the GOT is 3 + M + D.

   The GOT is also a synthetic area, created by the linker.  It exists
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

/* The constants below were determined by examining the following files
   in the linux kernel sources:

      arch/arm/kernel/signal.c
	  - see SWI_SYS_SIGRETURN and SWI_SYS_RT_SIGRETURN
      include/asm-arm/unistd.h
	  - see __NR_sigreturn, __NR_rt_sigreturn, and __NR_SYSCALL_BASE */

#define ARM_LINUX_SIGRETURN_INSTR	0xef900077
#define ARM_LINUX_RT_SIGRETURN_INSTR	0xef9000ad

/* For ARM EABI, the syscall number is not in the SWI instruction
   (instead it is loaded into r7).  We recognize the pattern that
   glibc uses...  alternatively, we could arrange to do this by
   function name, but they are not always exported.  */
#define ARM_SET_R7_SIGRETURN		0xe3a07077
#define ARM_SET_R7_RT_SIGRETURN		0xe3a070ad
#define ARM_EABI_SYSCALL		0xef000000

/* OABI syscall restart trampoline, used for EABI executables too
   whenever OABI support has been enabled in the kernel.  */
#define ARM_OABI_SYSCALL_RESTART_SYSCALL 0xef900000
#define ARM_LDR_PC_SP_12		0xe49df00c
#define ARM_LDR_PC_SP_4			0xe49df004

static void
arm_linux_sigtramp_cache (struct frame_info *this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func, int regs_offset)
{
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  CORE_ADDR base = sp + regs_offset;
  int i;

  for (i = 0; i < 16; i++)
    trad_frame_set_reg_addr (this_cache, i, base + i * 4);

  trad_frame_set_reg_addr (this_cache, ARM_PS_REGNUM, base + 16 * 4);

  /* The VFP or iWMMXt registers may be saved on the stack, but there's
     no reliable way to restore them (yet).  */

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

/* There are a couple of different possible stack layouts that
   we need to support.

   Before version 2.6.18, the kernel used completely independent
   layouts for non-RT and RT signals.  For non-RT signals the stack
   began directly with a struct sigcontext.  For RT signals the stack
   began with two redundant pointers (to the siginfo and ucontext),
   and then the siginfo and ucontext.

   As of version 2.6.18, the non-RT signal frame layout starts with
   a ucontext and the RT signal frame starts with a siginfo and then
   a ucontext.  Also, the ucontext now has a designated save area
   for coprocessor registers.

   For RT signals, it's easy to tell the difference: we look for
   pinfo, the pointer to the siginfo.  If it has the expected
   value, we have an old layout.  If it doesn't, we have the new
   layout.

   For non-RT signals, it's a bit harder.  We need something in one
   layout or the other with a recognizable offset and value.  We can't
   use the return trampoline, because ARM usually uses SA_RESTORER,
   in which case the stack return trampoline is not filled in.
   We can't use the saved stack pointer, because sigaltstack might
   be in use.  So for now we guess the new layout...  */

/* There are three words (trap_no, error_code, oldmask) in
   struct sigcontext before r0.  */
#define ARM_SIGCONTEXT_R0 0xc

/* There are five words (uc_flags, uc_link, and three for uc_stack)
   in the ucontext_t before the sigcontext.  */
#define ARM_UCONTEXT_SIGCONTEXT 0x14

/* There are three elements in an rt_sigframe before the ucontext:
   pinfo, puc, and info.  The first two are pointers and the third
   is a struct siginfo, with size 128 bytes.  We could follow puc
   to the ucontext, but it's simpler to skip the whole thing.  */
#define ARM_OLD_RT_SIGFRAME_SIGINFO 0x8
#define ARM_OLD_RT_SIGFRAME_UCONTEXT 0x88

#define ARM_NEW_RT_SIGFRAME_UCONTEXT 0x80

#define ARM_NEW_SIGFRAME_MAGIC 0x5ac3c35a

static void
arm_linux_sigreturn_init (const struct tramp_frame *self,
			  struct frame_info *this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  ULONGEST uc_flags = read_memory_unsigned_integer (sp, 4, byte_order);

  if (uc_flags == ARM_NEW_SIGFRAME_MAGIC)
    arm_linux_sigtramp_cache (this_frame, this_cache, func,
			      ARM_UCONTEXT_SIGCONTEXT
			      + ARM_SIGCONTEXT_R0);
  else
    arm_linux_sigtramp_cache (this_frame, this_cache, func,
			      ARM_SIGCONTEXT_R0);
}

static void
arm_linux_rt_sigreturn_init (const struct tramp_frame *self,
			  struct frame_info *this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  ULONGEST pinfo = read_memory_unsigned_integer (sp, 4, byte_order);

  if (pinfo == sp + ARM_OLD_RT_SIGFRAME_SIGINFO)
    arm_linux_sigtramp_cache (this_frame, this_cache, func,
			      ARM_OLD_RT_SIGFRAME_UCONTEXT
			      + ARM_UCONTEXT_SIGCONTEXT
			      + ARM_SIGCONTEXT_R0);
  else
    arm_linux_sigtramp_cache (this_frame, this_cache, func,
			      ARM_NEW_RT_SIGFRAME_UCONTEXT
			      + ARM_UCONTEXT_SIGCONTEXT
			      + ARM_SIGCONTEXT_R0);
}

static void
arm_linux_restart_syscall_init (const struct tramp_frame *self,
				struct frame_info *this_frame,
				struct trad_frame_cache *this_cache,
				CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  CORE_ADDR pc = get_frame_memory_unsigned (this_frame, sp, 4);
  CORE_ADDR cpsr = get_frame_register_unsigned (this_frame, ARM_PS_REGNUM);
  ULONGEST t_bit = arm_psr_thumb_bit (gdbarch);
  int sp_offset;

  /* There are two variants of this trampoline; with older kernels, the
     stub is placed on the stack, while newer kernels use the stub from
     the vector page.  They are identical except that the older version
     increments SP by 12 (to skip stored PC and the stub itself), while
     the newer version increments SP only by 4 (just the stored PC).  */
  if (self->insn[1].bytes == ARM_LDR_PC_SP_4)
    sp_offset = 4;
  else
    sp_offset = 12;

  /* Update Thumb bit in CPSR.  */
  if (pc & 1)
    cpsr |= t_bit;
  else
    cpsr &= ~t_bit;

  /* Remove Thumb bit from PC.  */
  pc = gdbarch_addr_bits_remove (gdbarch, pc);

  /* Save previous register values.  */
  trad_frame_set_reg_value (this_cache, ARM_SP_REGNUM, sp + sp_offset);
  trad_frame_set_reg_value (this_cache, ARM_PC_REGNUM, pc);
  trad_frame_set_reg_value (this_cache, ARM_PS_REGNUM, cpsr);

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static struct tramp_frame arm_linux_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { ARM_LINUX_SIGRETURN_INSTR, -1 },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_sigreturn_init
};

static struct tramp_frame arm_linux_rt_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { ARM_LINUX_RT_SIGRETURN_INSTR, -1 },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_rt_sigreturn_init
};

static struct tramp_frame arm_eabi_linux_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { ARM_SET_R7_SIGRETURN, -1 },
    { ARM_EABI_SYSCALL, -1 },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_sigreturn_init
};

static struct tramp_frame arm_eabi_linux_rt_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { ARM_SET_R7_RT_SIGRETURN, -1 },
    { ARM_EABI_SYSCALL, -1 },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_rt_sigreturn_init
};

static struct tramp_frame arm_linux_restart_syscall_tramp_frame = {
  NORMAL_FRAME,
  4,
  {
    { ARM_OABI_SYSCALL_RESTART_SYSCALL, -1 },
    { ARM_LDR_PC_SP_12, -1 },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_restart_syscall_init
};

static struct tramp_frame arm_kernel_linux_restart_syscall_tramp_frame = {
  NORMAL_FRAME,
  4,
  {
    { ARM_OABI_SYSCALL_RESTART_SYSCALL, -1 },
    { ARM_LDR_PC_SP_4, -1 },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_restart_syscall_init
};

/* Core file and register set support.  */

#define ARM_LINUX_SIZEOF_GREGSET (18 * INT_REGISTER_SIZE)

void
arm_linux_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs_buf, size_t len)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const gdb_byte *gregs = gregs_buf;
  int regno;
  CORE_ADDR reg_pc;
  gdb_byte pc_buf[INT_REGISTER_SIZE];

  for (regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache_raw_supply (regcache, regno,
			   gregs + INT_REGISTER_SIZE * regno);

  if (regnum == ARM_PS_REGNUM || regnum == -1)
    {
      if (arm_apcs_32)
	regcache_raw_supply (regcache, ARM_PS_REGNUM,
			     gregs + INT_REGISTER_SIZE * ARM_CPSR_GREGNUM);
      else
	regcache_raw_supply (regcache, ARM_PS_REGNUM,
			     gregs + INT_REGISTER_SIZE * ARM_PC_REGNUM);
    }

  if (regnum == ARM_PC_REGNUM || regnum == -1)
    {
      reg_pc = extract_unsigned_integer (gregs
					 + INT_REGISTER_SIZE * ARM_PC_REGNUM,
					 INT_REGISTER_SIZE, byte_order);
      reg_pc = gdbarch_addr_bits_remove (gdbarch, reg_pc);
      store_unsigned_integer (pc_buf, INT_REGISTER_SIZE, byte_order, reg_pc);
      regcache_raw_supply (regcache, ARM_PC_REGNUM, pc_buf);
    }
}

void
arm_linux_collect_gregset (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *gregs_buf, size_t len)
{
  gdb_byte *gregs = gregs_buf;
  int regno;

  for (regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache_raw_collect (regcache, regno,
			    gregs + INT_REGISTER_SIZE * regno);

  if (regnum == ARM_PS_REGNUM || regnum == -1)
    {
      if (arm_apcs_32)
	regcache_raw_collect (regcache, ARM_PS_REGNUM,
			      gregs + INT_REGISTER_SIZE * ARM_CPSR_GREGNUM);
      else
	regcache_raw_collect (regcache, ARM_PS_REGNUM,
			      gregs + INT_REGISTER_SIZE * ARM_PC_REGNUM);
    }

  if (regnum == ARM_PC_REGNUM || regnum == -1)
    regcache_raw_collect (regcache, ARM_PC_REGNUM,
			  gregs + INT_REGISTER_SIZE * ARM_PC_REGNUM);
}

/* Support for register format used by the NWFPE FPA emulator.  */

#define typeNone		0x00
#define typeSingle		0x01
#define typeDouble		0x02
#define typeExtended		0x03

void
supply_nwfpe_register (struct regcache *regcache, int regno,
		       const gdb_byte *regs)
{
  const gdb_byte *reg_data;
  gdb_byte reg_tag;
  gdb_byte buf[FP_REGISTER_SIZE];

  reg_data = regs + (regno - ARM_F0_REGNUM) * FP_REGISTER_SIZE;
  reg_tag = regs[(regno - ARM_F0_REGNUM) + NWFPE_TAGS_OFFSET];
  memset (buf, 0, FP_REGISTER_SIZE);

  switch (reg_tag)
    {
    case typeSingle:
      memcpy (buf, reg_data, 4);
      break;
    case typeDouble:
      memcpy (buf, reg_data + 4, 4);
      memcpy (buf + 4, reg_data, 4);
      break;
    case typeExtended:
      /* We want sign and exponent, then least significant bits,
	 then most significant.  NWFPE does sign, most, least.  */
      memcpy (buf, reg_data, 4);
      memcpy (buf + 4, reg_data + 8, 4);
      memcpy (buf + 8, reg_data + 4, 4);
      break;
    default:
      break;
    }

  regcache_raw_supply (regcache, regno, buf);
}

void
collect_nwfpe_register (const struct regcache *regcache, int regno,
			gdb_byte *regs)
{
  gdb_byte *reg_data;
  gdb_byte reg_tag;
  gdb_byte buf[FP_REGISTER_SIZE];

  regcache_raw_collect (regcache, regno, buf);

  /* NOTE drow/2006-06-07: This code uses the tag already in the
     register buffer.  I've preserved that when moving the code
     from the native file to the target file.  But this doesn't
     always make sense.  */

  reg_data = regs + (regno - ARM_F0_REGNUM) * FP_REGISTER_SIZE;
  reg_tag = regs[(regno - ARM_F0_REGNUM) + NWFPE_TAGS_OFFSET];

  switch (reg_tag)
    {
    case typeSingle:
      memcpy (reg_data, buf, 4);
      break;
    case typeDouble:
      memcpy (reg_data, buf + 4, 4);
      memcpy (reg_data + 4, buf, 4);
      break;
    case typeExtended:
      memcpy (reg_data, buf, 4);
      memcpy (reg_data + 4, buf + 8, 4);
      memcpy (reg_data + 8, buf + 4, 4);
      break;
    default:
      break;
    }
}

void
arm_linux_supply_nwfpe (const struct regset *regset,
			struct regcache *regcache,
			int regnum, const void *regs_buf, size_t len)
{
  const gdb_byte *regs = regs_buf;
  int regno;

  if (regnum == ARM_FPS_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, ARM_FPS_REGNUM,
			 regs + NWFPE_FPSR_OFFSET);

  for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      supply_nwfpe_register (regcache, regno, regs);
}

void
arm_linux_collect_nwfpe (const struct regset *regset,
			 const struct regcache *regcache,
			 int regnum, void *regs_buf, size_t len)
{
  gdb_byte *regs = regs_buf;
  int regno;

  for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      collect_nwfpe_register (regcache, regno, regs);

  if (regnum == ARM_FPS_REGNUM || regnum == -1)
    regcache_raw_collect (regcache, ARM_FPS_REGNUM,
			  regs + INT_REGISTER_SIZE * ARM_FPS_REGNUM);
}

/* Support VFP register format.  */

#define ARM_LINUX_SIZEOF_VFP (32 * 8 + 4)

static void
arm_linux_supply_vfp (const struct regset *regset,
		      struct regcache *regcache,
		      int regnum, const void *regs_buf, size_t len)
{
  const gdb_byte *regs = regs_buf;
  int regno;

  if (regnum == ARM_FPSCR_REGNUM || regnum == -1)
    regcache_raw_supply (regcache, ARM_FPSCR_REGNUM, regs + 32 * 8);

  for (regno = ARM_D0_REGNUM; regno <= ARM_D31_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache_raw_supply (regcache, regno,
			   regs + (regno - ARM_D0_REGNUM) * 8);
}

static void
arm_linux_collect_vfp (const struct regset *regset,
			 const struct regcache *regcache,
			 int regnum, void *regs_buf, size_t len)
{
  gdb_byte *regs = regs_buf;
  int regno;

  if (regnum == ARM_FPSCR_REGNUM || regnum == -1)
    regcache_raw_collect (regcache, ARM_FPSCR_REGNUM, regs + 32 * 8);

  for (regno = ARM_D0_REGNUM; regno <= ARM_D31_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache_raw_collect (regcache, regno,
			    regs + (regno - ARM_D0_REGNUM) * 8);
}

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */

static const struct regset *
arm_linux_regset_from_core_section (struct gdbarch *gdbarch,
				    const char *sect_name, size_t sect_size)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (strcmp (sect_name, ".reg") == 0
      && sect_size == ARM_LINUX_SIZEOF_GREGSET)
    {
      if (tdep->gregset == NULL)
        tdep->gregset = regset_alloc (gdbarch, arm_linux_supply_gregset,
                                      arm_linux_collect_gregset);
      return tdep->gregset;
    }

  if (strcmp (sect_name, ".reg2") == 0
      && sect_size == ARM_LINUX_SIZEOF_NWFPE)
    {
      if (tdep->fpregset == NULL)
        tdep->fpregset = regset_alloc (gdbarch, arm_linux_supply_nwfpe,
                                       arm_linux_collect_nwfpe);
      return tdep->fpregset;
    }

  if (strcmp (sect_name, ".reg-arm-vfp") == 0
      && sect_size == ARM_LINUX_SIZEOF_VFP)
    {
      if (tdep->vfpregset == NULL)
        tdep->vfpregset = regset_alloc (gdbarch, arm_linux_supply_vfp,
					arm_linux_collect_vfp);
      return tdep->vfpregset;
    }

  return NULL;
}

/* Core file register set sections.  */

static struct core_regset_section arm_linux_fpa_regset_sections[] =
{
  { ".reg", ARM_LINUX_SIZEOF_GREGSET, "general-purpose" },
  { ".reg2", ARM_LINUX_SIZEOF_NWFPE, "FPA floating-point" },
  { NULL, 0}
};

static struct core_regset_section arm_linux_vfp_regset_sections[] =
{
  { ".reg", ARM_LINUX_SIZEOF_GREGSET, "general-purpose" },
  { ".reg-arm-vfp", ARM_LINUX_SIZEOF_VFP, "VFP floating-point" },
  { NULL, 0}
};

/* Determine target description from core file.  */

static const struct target_desc *
arm_linux_core_read_description (struct gdbarch *gdbarch,
                                 struct target_ops *target,
                                 bfd *abfd)
{
  CORE_ADDR arm_hwcap = 0;

  if (target_auxv_search (target, AT_HWCAP, &arm_hwcap) != 1)
    return NULL;

  if (arm_hwcap & HWCAP_VFP)
    {
      /* NEON implies VFPv3-D32 or no-VFP unit.  Say that we only support
         Neon with VFPv3-D32.  */
      if (arm_hwcap & HWCAP_NEON)
	return tdesc_arm_with_neon;
      else if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
	return tdesc_arm_with_vfpv3;
      else
	return tdesc_arm_with_vfpv2;
    }

  return NULL;
}


/* Copy the value of next pc of sigreturn and rt_sigrturn into PC,
   return 1.  In addition, set IS_THUMB depending on whether we
   will return to ARM or Thumb code.  Return 0 if it is not a
   rt_sigreturn/sigreturn syscall.  */
static int
arm_linux_sigreturn_return_addr (struct frame_info *frame,
				 unsigned long svc_number,
				 CORE_ADDR *pc, int *is_thumb)
{
  /* Is this a sigreturn or rt_sigreturn syscall?  */
  if (svc_number == 119 || svc_number == 173)
    {
      if (get_frame_type (frame) == SIGTRAMP_FRAME)
	{
	  ULONGEST t_bit = arm_psr_thumb_bit (frame_unwind_arch (frame));
	  CORE_ADDR cpsr
	    = frame_unwind_register_unsigned (frame, ARM_PS_REGNUM);

	  *is_thumb = (cpsr & t_bit) != 0;
	  *pc = frame_unwind_caller_pc (frame);
	  return 1;
	}
    }
  return 0;
}

/* At a ptrace syscall-stop, return the syscall number.  This either
   comes from the SWI instruction (OABI) or from r7 (EABI).

   When the function fails, it should return -1.  */

static LONGEST
arm_linux_get_syscall_number (struct gdbarch *gdbarch,
			      ptid_t ptid)
{
  struct regcache *regs = get_thread_regcache (ptid);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  ULONGEST pc;
  ULONGEST cpsr;
  ULONGEST t_bit = arm_psr_thumb_bit (gdbarch);
  int is_thumb;
  ULONGEST svc_number = -1;

  regcache_cooked_read_unsigned (regs, ARM_PC_REGNUM, &pc);
  regcache_cooked_read_unsigned (regs, ARM_PS_REGNUM, &cpsr);
  is_thumb = (cpsr & t_bit) != 0;

  if (is_thumb)
    {
      regcache_cooked_read_unsigned (regs, 7, &svc_number);
    }
  else
    {
      enum bfd_endian byte_order_for_code = 
	gdbarch_byte_order_for_code (gdbarch);

      /* PC gets incremented before the syscall-stop, so read the
	 previous instruction.  */
      unsigned long this_instr = 
	read_memory_unsigned_integer (pc - 4, 4, byte_order_for_code);

      unsigned long svc_operand = (0x00ffffff & this_instr);

      if (svc_operand)
	{
          /* OABI */
	  svc_number = svc_operand - 0x900000;
	}
      else
	{
          /* EABI */
	  regcache_cooked_read_unsigned (regs, 7, &svc_number);
	}
    }

  return svc_number;
}

/* When FRAME is at a syscall instruction, return the PC of the next
   instruction to be executed.  */

static CORE_ADDR
arm_linux_syscall_next_pc (struct frame_info *frame)
{
  CORE_ADDR pc = get_frame_pc (frame);
  CORE_ADDR return_addr = 0;
  int is_thumb = arm_frame_is_thumb (frame);
  ULONGEST svc_number = 0;

  if (is_thumb)
    {
      svc_number = get_frame_register_unsigned (frame, 7);
      return_addr = pc + 2;
    }
  else
    {
      struct gdbarch *gdbarch = get_frame_arch (frame);
      enum bfd_endian byte_order_for_code = 
	gdbarch_byte_order_for_code (gdbarch);
      unsigned long this_instr = 
	read_memory_unsigned_integer (pc, 4, byte_order_for_code);

      unsigned long svc_operand = (0x00ffffff & this_instr);
      if (svc_operand)  /* OABI.  */
	{
	  svc_number = svc_operand - 0x900000;
	}
      else /* EABI.  */
	{
	  svc_number = get_frame_register_unsigned (frame, 7);
	}

      return_addr = pc + 4;
    }

  arm_linux_sigreturn_return_addr (frame, svc_number, &return_addr, &is_thumb);

  /* Addresses for calling Thumb functions have the bit 0 set.  */
  if (is_thumb)
    return_addr |= 1;

  return return_addr;
}


/* Insert a single step breakpoint at the next executed instruction.  */

static int
arm_linux_software_single_step (struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct address_space *aspace = get_frame_address_space (frame);
  CORE_ADDR next_pc;

  if (arm_deal_with_atomic_sequence (frame))
    return 1;

  next_pc = arm_get_next_pc (frame, get_frame_pc (frame));

  /* The Linux kernel offers some user-mode helpers in a high page.  We can
     not read this page (as of 2.6.23), and even if we could then we couldn't
     set breakpoints in it, and even if we could then the atomic operations
     would fail when interrupted.  They are all called as functions and return
     to the address in LR, so step to there instead.  */
  if (next_pc > 0xffff0000)
    next_pc = get_frame_register_unsigned (frame, ARM_LR_REGNUM);

  arm_insert_single_step_breakpoint (gdbarch, aspace, next_pc);

  return 1;
}

/* Support for displaced stepping of Linux SVC instructions.  */

static void
arm_linux_cleanup_svc (struct gdbarch *gdbarch,
		       struct regcache *regs,
		       struct displaced_step_closure *dsc)
{
  CORE_ADDR from = dsc->insn_addr;
  ULONGEST apparent_pc;
  int within_scratch;

  regcache_cooked_read_unsigned (regs, ARM_PC_REGNUM, &apparent_pc);

  within_scratch = (apparent_pc >= dsc->scratch_base
		    && apparent_pc < (dsc->scratch_base
				      + DISPLACED_MODIFIED_INSNS * 4 + 4));

  if (debug_displaced)
    {
      fprintf_unfiltered (gdb_stdlog, "displaced: PC is apparently %.8lx after "
			  "SVC step ", (unsigned long) apparent_pc);
      if (within_scratch)
        fprintf_unfiltered (gdb_stdlog, "(within scratch space)\n");
      else
        fprintf_unfiltered (gdb_stdlog, "(outside scratch space)\n");
    }

  if (within_scratch)
    displaced_write_reg (regs, dsc, ARM_PC_REGNUM, from + 4, BRANCH_WRITE_PC);
}

static int
arm_linux_copy_svc (struct gdbarch *gdbarch, struct regcache *regs,
		    struct displaced_step_closure *dsc)
{
  CORE_ADDR return_to = 0;

  struct frame_info *frame;
  unsigned int svc_number = displaced_read_reg (regs, dsc, 7);
  int is_sigreturn = 0;
  int is_thumb;

  frame = get_current_frame ();

  is_sigreturn = arm_linux_sigreturn_return_addr(frame, svc_number,
						 &return_to, &is_thumb);
  if (is_sigreturn)
    {
	  struct symtab_and_line sal;

	  if (debug_displaced)
	    fprintf_unfiltered (gdb_stdlog, "displaced: found "
	      "sigreturn/rt_sigreturn SVC call.  PC in frame = %lx\n",
	      (unsigned long) get_frame_pc (frame));

	  if (debug_displaced)
	    fprintf_unfiltered (gdb_stdlog, "displaced: unwind pc = %lx.  "
	      "Setting momentary breakpoint.\n", (unsigned long) return_to);

	  gdb_assert (inferior_thread ()->control.step_resume_breakpoint
		      == NULL);

	  sal = find_pc_line (return_to, 0);
	  sal.pc = return_to;
	  sal.section = find_pc_overlay (return_to);
	  sal.explicit_pc = 1;

	  frame = get_prev_frame (frame);

	  if (frame)
	    {
	      inferior_thread ()->control.step_resume_breakpoint
        	= set_momentary_breakpoint (gdbarch, sal, get_frame_id (frame),
					    bp_step_resume);

	      /* set_momentary_breakpoint invalidates FRAME.  */
	      frame = NULL;

	      /* We need to make sure we actually insert the momentary
	         breakpoint set above.  */
	      insert_breakpoints ();
	    }
	  else if (debug_displaced)
	    fprintf_unfiltered (gdb_stderr, "displaced: couldn't find previous "
				"frame to set momentary breakpoint for "
				"sigreturn/rt_sigreturn\n");
	}
      else if (debug_displaced)
	fprintf_unfiltered (gdb_stdlog, "displaced: sigreturn/rt_sigreturn "
			    "SVC call not in signal trampoline frame\n");
    

  /* Preparation: If we detect sigreturn, set momentary breakpoint at resume
		  location, else nothing.
     Insn: unmodified svc.
     Cleanup: if pc lands in scratch space, pc <- insn_addr + 4
              else leave pc alone.  */


  dsc->cleanup = &arm_linux_cleanup_svc;
  /* Pretend we wrote to the PC, so cleanup doesn't set PC to the next
     instruction.  */
  dsc->wrote_to_pc = 1;

  return 0;
}


/* The following two functions implement single-stepping over calls to Linux
   kernel helper routines, which perform e.g. atomic operations on architecture
   variants which don't support them natively.

   When this function is called, the PC will be pointing at the kernel helper
   (at an address inaccessible to GDB), and r14 will point to the return
   address.  Displaced stepping always executes code in the copy area:
   so, make the copy-area instruction branch back to the kernel helper (the
   "from" address), and make r14 point to the breakpoint in the copy area.  In
   that way, we regain control once the kernel helper returns, and can clean
   up appropriately (as if we had just returned from the kernel helper as it
   would have been called from the non-displaced location).  */

static void
cleanup_kernel_helper_return (struct gdbarch *gdbarch,
			      struct regcache *regs,
			      struct displaced_step_closure *dsc)
{
  displaced_write_reg (regs, dsc, ARM_LR_REGNUM, dsc->tmp[0], CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, ARM_PC_REGNUM, dsc->tmp[0], BRANCH_WRITE_PC);
}

static void
arm_catch_kernel_helper_return (struct gdbarch *gdbarch, CORE_ADDR from,
				CORE_ADDR to, struct regcache *regs,
				struct displaced_step_closure *dsc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  dsc->numinsns = 1;
  dsc->insn_addr = from;
  dsc->cleanup = &cleanup_kernel_helper_return;
  /* Say we wrote to the PC, else cleanup will set PC to the next
     instruction in the helper, which isn't helpful.  */
  dsc->wrote_to_pc = 1;

  /* Preparation: tmp[0] <- r14
                  r14 <- <scratch space>+4
		  *(<scratch space>+8) <- from
     Insn: ldr pc, [r14, #4]
     Cleanup: r14 <- tmp[0], pc <- tmp[0].  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, ARM_LR_REGNUM);
  displaced_write_reg (regs, dsc, ARM_LR_REGNUM, (ULONGEST) to + 4,
		       CANNOT_WRITE_PC);
  write_memory_unsigned_integer (to + 8, 4, byte_order, from);

  dsc->modinsn[0] = 0xe59ef004;  /* ldr pc, [lr, #4].  */
}

/* Linux-specific displaced step instruction copying function.  Detects when
   the program has stepped into a Linux kernel helper routine (which must be
   handled as a special case), falling back to arm_displaced_step_copy_insn()
   if it hasn't.  */

static struct displaced_step_closure *
arm_linux_displaced_step_copy_insn (struct gdbarch *gdbarch,
				    CORE_ADDR from, CORE_ADDR to,
				    struct regcache *regs)
{
  struct displaced_step_closure *dsc
    = xmalloc (sizeof (struct displaced_step_closure));

  /* Detect when we enter an (inaccessible by GDB) Linux kernel helper, and
     stop at the return location.  */
  if (from > 0xffff0000)
    {
      if (debug_displaced)
        fprintf_unfiltered (gdb_stdlog, "displaced: detected kernel helper "
			    "at %.8lx\n", (unsigned long) from);

      arm_catch_kernel_helper_return (gdbarch, from, to, regs, dsc);
    }
  else
    {
      /* Override the default handling of SVC instructions.  */
      dsc->u.svc.copy_svc_os = arm_linux_copy_svc;

      arm_process_displaced_insn (gdbarch, from, to, regs, dsc);
    }

  arm_displaced_init_closure (gdbarch, from, to, dsc);

  return dsc;
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
arm_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (*s == '#' || *s == '$' || isdigit (*s) /* Literal number.  */
	  || *s == '[' /* Register indirection or
			  displacement.  */
	  || isalpha (*s)); /* Register value.  */
}

/* This routine is used to parse a special token in ARM's assembly.

   The special tokens parsed by it are:

      - Register displacement (e.g, [fp, #-8])

   It returns one if the special token has been parsed successfully,
   or zero if the current token is not considered special.  */

static int
arm_stap_parse_special_token (struct gdbarch *gdbarch,
			      struct stap_parse_info *p)
{
  if (*p->arg == '[')
    {
      /* Temporary holder for lookahead.  */
      const char *tmp = p->arg;
      char *endp;
      /* Used to save the register name.  */
      const char *start;
      char *regname;
      int len, offset;
      int got_minus = 0;
      long displacement;
      struct stoken str;

      ++tmp;
      start = tmp;

      /* Register name.  */
      while (isalnum (*tmp))
	++tmp;

      if (*tmp != ',')
	return 0;

      len = tmp - start;
      regname = alloca (len + 2);

      offset = 0;
      if (isdigit (*start))
	{
	  /* If we are dealing with a register whose name begins with a
	     digit, it means we should prefix the name with the letter
	     `r', because GDB expects this name pattern.  Otherwise (e.g.,
	     we are dealing with the register `fp'), we don't need to
	     add such a prefix.  */
	  regname[0] = 'r';
	  offset = 1;
	}

      strncpy (regname + offset, start, len);
      len += offset;
      regname[len] = '\0';

      if (user_reg_map_name_to_regnum (gdbarch, regname, len) == -1)
	error (_("Invalid register name `%s' on expression `%s'."),
	       regname, p->saved_arg);

      ++tmp;
      tmp = skip_spaces_const (tmp);
      if (*tmp == '#' || *tmp == '$')
	++tmp;

      if (*tmp == '-')
	{
	  ++tmp;
	  got_minus = 1;
	}

      displacement = strtol (tmp, &endp, 10);
      tmp = endp;

      /* Skipping last `]'.  */
      if (*tmp++ != ']')
	return 0;

      /* The displacement.  */
      write_exp_elt_opcode (OP_LONG);
      write_exp_elt_type (builtin_type (gdbarch)->builtin_long);
      write_exp_elt_longcst (displacement);
      write_exp_elt_opcode (OP_LONG);
      if (got_minus)
	write_exp_elt_opcode (UNOP_NEG);

      /* The register name.  */
      write_exp_elt_opcode (OP_REGISTER);
      str.ptr = regname;
      str.length = len;
      write_exp_string (str);
      write_exp_elt_opcode (OP_REGISTER);

      write_exp_elt_opcode (BINOP_ADD);

      /* Casting to the expected type.  */
      write_exp_elt_opcode (UNOP_CAST);
      write_exp_elt_type (lookup_pointer_type (p->arg_type));
      write_exp_elt_opcode (UNOP_CAST);

      write_exp_elt_opcode (UNOP_IND);

      p->arg = tmp;
    }
  else
    return 0;

  return 1;
}

static void
arm_linux_init_abi (struct gdbarch_info info,
		    struct gdbarch *gdbarch)
{
  static const char *const stap_integer_prefixes[] = { "#", "$", "", NULL };
  static const char *const stap_register_prefixes[] = { "r", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "[",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { "]",
								    NULL };
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  linux_init_abi (info, gdbarch);

  tdep->lowest_pc = 0x8000;
  if (info.byte_order == BFD_ENDIAN_BIG)
    {
      if (tdep->arm_abi == ARM_ABI_AAPCS)
	tdep->arm_breakpoint = eabi_linux_arm_be_breakpoint;
      else
	tdep->arm_breakpoint = arm_linux_arm_be_breakpoint;
      tdep->thumb_breakpoint = arm_linux_thumb_be_breakpoint;
      tdep->thumb2_breakpoint = arm_linux_thumb2_be_breakpoint;
    }
  else
    {
      if (tdep->arm_abi == ARM_ABI_AAPCS)
	tdep->arm_breakpoint = eabi_linux_arm_le_breakpoint;
      else
	tdep->arm_breakpoint = arm_linux_arm_le_breakpoint;
      tdep->thumb_breakpoint = arm_linux_thumb_le_breakpoint;
      tdep->thumb2_breakpoint = arm_linux_thumb2_le_breakpoint;
    }
  tdep->arm_breakpoint_size = sizeof (arm_linux_arm_le_breakpoint);
  tdep->thumb_breakpoint_size = sizeof (arm_linux_thumb_le_breakpoint);
  tdep->thumb2_breakpoint_size = sizeof (arm_linux_thumb2_le_breakpoint);

  if (tdep->fp_model == ARM_FLOAT_AUTO)
    tdep->fp_model = ARM_FLOAT_FPA;

  switch (tdep->fp_model)
    {
    case ARM_FLOAT_FPA:
      tdep->jb_pc = ARM_LINUX_JB_PC_FPA;
      break;
    case ARM_FLOAT_SOFT_FPA:
    case ARM_FLOAT_SOFT_VFP:
    case ARM_FLOAT_VFP:
      tdep->jb_pc = ARM_LINUX_JB_PC_EABI;
      break;
    default:
      internal_error
	(__FILE__, __LINE__,
         _("arm_linux_init_abi: Floating point model not supported"));
      break;
    }
  tdep->jb_elt_size = ARM_LINUX_JB_ELEMENT_SIZE;

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, arm_linux_software_single_step);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
                                             svr4_fetch_objfile_link_map);

  tramp_frame_prepend_unwinder (gdbarch,
				&arm_linux_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_linux_rt_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_eabi_linux_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_eabi_linux_rt_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_linux_restart_syscall_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_kernel_linux_restart_syscall_tramp_frame);

  /* Core file support.  */
  set_gdbarch_regset_from_core_section (gdbarch,
					arm_linux_regset_from_core_section);
  set_gdbarch_core_read_description (gdbarch, arm_linux_core_read_description);

  if (tdep->have_vfp_registers)
    set_gdbarch_core_regset_sections (gdbarch, arm_linux_vfp_regset_sections);
  else if (tdep->have_fpa_registers)
    set_gdbarch_core_regset_sections (gdbarch, arm_linux_fpa_regset_sections);

  set_gdbarch_get_siginfo_type (gdbarch, linux_get_siginfo_type);

  /* Displaced stepping.  */
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					arm_linux_displaced_step_copy_insn);
  set_gdbarch_displaced_step_fixup (gdbarch, arm_displaced_step_fixup);
  set_gdbarch_displaced_step_free_closure (gdbarch,
					   simple_displaced_step_free_closure);
  set_gdbarch_displaced_step_location (gdbarch, displaced_step_at_entry_point);

  /* Reversible debugging, process record.  */
  set_gdbarch_process_record (gdbarch, arm_process_record);

  /* SystemTap functions.  */
  set_gdbarch_stap_integer_prefixes (gdbarch, stap_integer_prefixes);
  set_gdbarch_stap_register_prefixes (gdbarch, stap_register_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					  stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					  stap_register_indirection_suffixes);
  set_gdbarch_stap_gdb_register_prefix (gdbarch, "r");
  set_gdbarch_stap_is_single_operand (gdbarch, arm_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					arm_stap_parse_special_token);

  tdep->syscall_next_pc = arm_linux_syscall_next_pc;

  /* `catch syscall' */
  set_xml_syscall_file_name ("syscalls/arm-linux.xml");
  set_gdbarch_get_syscall_number (gdbarch, arm_linux_get_syscall_number);

  /* Syscall record.  */
  tdep->arm_swi_record = NULL;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_arm_linux_tdep;

void
_initialize_arm_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_LINUX,
			  arm_linux_init_abi);
}
