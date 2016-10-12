/* Target-dependent code for GNU/Linux x86.

   Copyright (C) 2002-2016 Free Software Foundation, Inc.

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

#ifndef I386_LINUX_TDEP_H
#define I386_LINUX_TDEP_H

/* The Linux kernel pretends there is an additional "orig_eax"
   register.  Since GDB needs access to that register to be able to
   properly restart system calls when necessary (see
   i386-linux-tdep.c) we need our own versions of a number of
   functions that deal with GDB's register cache.  */

/* Register number for the "orig_eax" pseudo-register.  If this
   pseudo-register contains a value >= 0 it is interpreted as the
   system call number that the kernel is supposed to restart.  */
#define I386_LINUX_ORIG_EAX_REGNUM (I386_ZMM7H_REGNUM + 1)

/* Total number of registers for GNU/Linux.  */
#define I386_LINUX_NUM_REGS (I386_LINUX_ORIG_EAX_REGNUM + 1)

/* Get XSAVE extended state xcr0 from core dump.  */
extern uint64_t i386_linux_core_read_xcr0 (bfd *abfd);

/* Handle and display information related to the MPX bound violation
   to the user.  */
extern void i386_linux_handle_segmentation_fault (struct gdbarch *gdbarch,
						  struct ui_out *uiout);

/* Linux target description.  */
extern struct target_desc *tdesc_i386_linux;
extern struct target_desc *tdesc_i386_mmx_linux;
extern struct target_desc *tdesc_i386_avx_linux;
extern struct target_desc *tdesc_i386_mpx_linux;
extern struct target_desc *tdesc_i386_avx_mpx_linux;
extern struct target_desc *tdesc_i386_avx512_linux;

/* Format of XSAVE extended state is:
 	struct
	{
	  fxsave_bytes[0..463]
	  sw_usable_bytes[464..511]
	  xstate_hdr_bytes[512..575]
	  avx_bytes[576..831]
	  mpx_bytes [960..1032]
	  avx512_k_regs[1088..1152]
	  avx512_zmmh_regs0-7[1153..1407]
	  avx512_zmmh_regs8-15[1408..1663]
	  avx512_zmm_regs16-31[1664..2687]
	  future_state etc
	};

  Same memory layout will be used for the coredump NT_X86_XSTATE
  representing the XSAVE extended state registers.

  The first 8 bytes of the sw_usable_bytes[464..467] is the OS enabled
  extended state mask, which is the same as the extended control register
  0 (the XFEATURE_ENABLED_MASK register), XCR0.  We can use this mask
  together with the mask saved in the xstate_hdr_bytes to determine what
  states the processor/OS supports and what state, used or initialized,
  the process/thread is in.  */ 
#define I386_LINUX_XSAVE_XCR0_OFFSET 464

extern int i386_linux_gregset_reg_offset[];

/* Return x86 siginfo type.  */
extern struct type *x86_linux_get_siginfo_type (struct gdbarch *gdbarch);

#endif /* i386-linux-tdep.h */
