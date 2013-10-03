/* Target-dependent code for the IA-64 for GDB, the GNU debugger.

   Copyright (C) 2010-2013 Free Software Foundation, Inc.

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
#include "ia64-tdep.h"
#include "ia64-hpux-tdep.h"
#include "osabi.h"
#include "gdbtypes.h"
#include "solib.h"
#include "target.h"
#include "frame.h"
#include "regcache.h"
#include "gdbcore.h"
#include "inferior.h"

/* A sequence of instructions pushed on the stack when we want to perform
   an inferior function call.  The main purpose of this code is to save
   the output region of the register frame belonging to the function
   from which we are making the call.  Normally, all registers are saved
   prior to the call, but this does not include stacked registers because
   they are seen by GDB as pseudo registers.

   With Linux kernels, these stacked registers can be saved by simply
   creating a new register frame, or in other words by moving the BSP.
   But the HP/UX kernel does not allow this.  So we rely on this code
   instead, that makes functions calls whose only purpose is to create
   new register frames.

   The array below is the result obtained after assembling the code
   shown below. It's an array of bytes in order to make it independent
   of the host endianess, in case it ends up being used on more than
   one target.

   start:
        // Save b0 before using it (into preserved reg: r4).
        mov r4 = b0
        ;;

        br.call.dptk.few b0 = stub#
        ;;

        // Add a nop bundle where we can insert our dummy breakpoint.
        nop.m 0
        nop.i 0
        nop.i 0
        ;;

   stub:
        // Alloc a new register stack frame.  Here, we set the size
        // of all regions to zero.  Eventually, GDB will manually
        // change the instruction to set the size of the local region
        // to match size of the output region of the function from
        // which we are making the function call.  This is to protect
        // the value of the output registers of the function from
        // which we are making the call.
        alloc r6 = ar.pfs, 0, 0, 0, 0

        // Save b0 before using it again (into preserved reg: r5).
        mov r5 = b0
        ;;

        //  Now that we have protected the entire output region of the
        //  register stack frame, we can call our function that will
        //  setup the arguments, and call our target function.
        br.call.dptk.few b0 = call_dummy#
        ;;

        //  Restore b0, ar.pfs, and return
        mov b0 = r5
        mov.i ar.pfs = r6
        ;;
        br.ret.dptk.few b0
        ;;

   call_dummy:
        //  Alloc a new frame, with 2 local registers, and 8 output registers
        //  (8 output registers for the maximum of 8 slots passed by register).
        alloc r32 = ar.pfs, 2, 0, 8, 0

        //  Save b0 before using it to call our target function.
        mov r33 = b0

        // Load the argument values placed by GDB inside r14-r21 in their
        // proper registers.
        or r34 = r14, r0
        or r35 = r15, r0
        or r36 = r16, r0
        or r37 = r17, r0
        or r38 = r18, r0
        or r39 = r19, r0
        or r40 = r20, r0
        or r41 = r21, r0
        ;;

        // actual call
        br.call.dptk.few b0 = b1
        ;;

        mov.i ar.pfs=r32
        mov b0=r33
        ;;

        br.ret.dptk.few b0
        ;;

*/

static const gdb_byte ia64_hpux_dummy_code[] =
{
  0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x40, 0x00,
  0x00, 0x62, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x20, 0x00, 0x00, 0x52,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
  0x02, 0x30, 0x00, 0x00, 0x80, 0x05, 0x50, 0x00,
  0x00, 0x62, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x30, 0x00, 0x00, 0x52,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x28,
  0x04, 0x80, 0x03, 0x00, 0x60, 0x00, 0xaa, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x80, 0x00, 0x00, 0x84, 0x02,
  0x00, 0x00, 0x29, 0x04, 0x80, 0x05, 0x10, 0x02,
  0x00, 0x62, 0x00, 0x40, 0xe4, 0x00, 0x38, 0x80,
  0x00, 0x18, 0x3d, 0x00, 0x0e, 0x20, 0x40, 0x82,
  0x00, 0x1c, 0x40, 0xa0, 0x14, 0x01, 0x38, 0x80,
  0x00, 0x30, 0x49, 0x00, 0x0e, 0x20, 0x70, 0x9a,
  0x00, 0x1c, 0x40, 0x00, 0x45, 0x01, 0x38, 0x80,
  0x0a, 0x48, 0x55, 0x00, 0x0e, 0x20, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
  0x1d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x10, 0x00, 0x80, 0x12,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x01, 0x55, 0x00, 0x00, 0x10, 0x0a, 0x00, 0x07,
  0x1d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x80, 0x00, 0x00, 0x84, 0x02
};

/* The offset to be used in order to get the __reason pseudo-register
   when using one of the *UREGS ttrace requests (see system header file
   /usr/include/ia64/sys/uregs.h for more details).

   The documentation for this pseudo-register says that a nonzero value
   indicates that the thread stopped due to a fault, trap, or interrupt.
   A null value indicates a stop inside a syscall.  */
#define IA64_HPUX_UREG_REASON 0x00070000

/* Return nonzero if the value of the register identified by REGNUM
   can be modified.  */

static int
ia64_hpux_can_store_ar_register (int regnum)
{
  switch (regnum)
    {
      case IA64_RSC_REGNUM:
      case IA64_RNAT_REGNUM:
      case IA64_CSD_REGNUM:
      case IA64_SSD_REGNUM:
      case IA64_CCV_REGNUM:
      case IA64_UNAT_REGNUM:
      case IA64_FPSR_REGNUM:
      case IA64_PFS_REGNUM:
      case IA64_LC_REGNUM:
      case IA64_EC_REGNUM:
         return 1;
         break;

      default:
         return 0;
         break;
    }
}

/* The "cannot_store_register" target_ops method.  */

static int
ia64_hpux_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  /* General registers.  */

  if (regnum == IA64_GR0_REGNUM)
    return 1;

  /* FP register.  */

  if (regnum == IA64_FR0_REGNUM || regnum == IA64_FR1_REGNUM)
    return 1;

  /* Application registers.  */
  if (regnum >= IA64_AR0_REGNUM && regnum <= IA64_AR0_REGNUM + 127)
    return (!ia64_hpux_can_store_ar_register (regnum));

  /* We can store all other registers.  */
  return 0;
}

/* Return nonzero if the inferior is stopped inside a system call.  */

static int
ia64_hpux_stopped_in_syscall (struct gdbarch *gdbarch)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct target_ops *ops = &current_target;
  gdb_byte buf[8];
  int len;

  len = target_read (ops, TARGET_OBJECT_HPUX_UREGS, NULL,
		     buf, IA64_HPUX_UREG_REASON, sizeof (buf));
  if (len == -1)
    /* The target wasn't able to tell us.  Assume we are not stopped
       in a system call, which is the normal situation.  */
    return 0;
  gdb_assert (len == 8);

  return (extract_unsigned_integer (buf, len, byte_order) == 0);
}

/* The "size_of_register_frame" gdbarch_tdep routine for ia64-hpux.  */

static int
ia64_hpux_size_of_register_frame (struct frame_info *this_frame,
				  ULONGEST cfm)
{
  int sof;

  if (frame_relative_level (this_frame) == 0
      && ia64_hpux_stopped_in_syscall (get_frame_arch (this_frame)))
    /* If the inferior stopped in a system call, the base address
       of the register frame is at BSP - SOL instead of BSP - SOF.
       This is an HP-UX exception.  */
    sof = (cfm & 0x3f80) >> 7;
  else
    sof = (cfm & 0x7f);

  return sof;
}

/* Implement the push_dummy_code gdbarch method.

   This function assumes that the SP is already 16-byte-aligned.  */

static CORE_ADDR
ia64_hpux_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp,
			   CORE_ADDR funaddr, struct value **args, int nargs,
			   struct type *value_type, CORE_ADDR *real_pc,
			   CORE_ADDR *bp_addr, struct regcache *regcache)
{
  ULONGEST cfm;
  int sof, sol, sor, soo;
  gdb_byte buf[16];

  regcache_cooked_read_unsigned (regcache, IA64_CFM_REGNUM, &cfm);
  sof = cfm & 0x7f;
  sol = (cfm >> 7) & 0x7f;
  sor = (cfm >> 14) & 0xf;
  soo = sof - sol - sor;

  /* Reserve some space on the stack to hold the dummy code.  */
  sp = sp - sizeof (ia64_hpux_dummy_code);

  /* Set the breakpoint address at the first instruction of the bundle
     in the dummy code that has only nops.  This is where the dummy code
     expects us to break.  */
  *bp_addr = sp + 0x20;

  /* Start the inferior function call from the dummy code.  The dummy
     code will then call our function.  */
  *real_pc = sp;

  /* Transfer the dummy code to the inferior.  */
  write_memory (sp, ia64_hpux_dummy_code, sizeof (ia64_hpux_dummy_code));

  /* Update the size of the local portion of the register frame allocated
     by ``stub'' to match the size of the output region of the current
     register frame.  This allows us to save the stacked registers.

     The "alloc" instruction is located at slot 0 of the bundle at +0x30.
     Update the "sof" and "sol" portion of that instruction which are
     respectively at bits 18-24 and 25-31 of the bundle.  */
  memcpy (buf, ia64_hpux_dummy_code + 0x30, sizeof (buf));

  buf[2] |= ((soo & 0x3f) << 2);
  buf[3] |= (soo << 1);
  if (soo > 63)
    buf[3] |= 1;

  write_memory (sp + 0x30, buf, sizeof (buf));

  /* Return the new (already properly aligned) SP.  */
  return sp;
}

/* The "allocate_new_rse_frame" ia64_infcall_ops routine for ia64-hpux.  */

static void
ia64_hpux_allocate_new_rse_frame (struct regcache *regcache, ULONGEST bsp,
				  int sof)
{
  /* We cannot change the value of the BSP register on HP-UX,
     so we can't allocate a new RSE frame.  */
}

/* The "store_argument_in_slot" ia64_infcall_ops routine for ia64-hpux.  */

static void
ia64_hpux_store_argument_in_slot (struct regcache *regcache, CORE_ADDR bsp,
                                  int slotnum, gdb_byte *buf)
{
  /* The call sequence on this target expects us to place the arguments
     inside r14 - r21.  */
  regcache_cooked_write (regcache, IA64_GR0_REGNUM + 14 + slotnum, buf);
}

/* The "set_function_addr" ia64_infcall_ops routine for ia64-hpux.  */

static void
ia64_hpux_set_function_addr (struct regcache *regcache, CORE_ADDR func_addr)
{
  /* The calling sequence calls the function whose address is placed
     in register b1.  */
  regcache_cooked_write_unsigned (regcache, IA64_BR1_REGNUM, func_addr);
}

/* The ia64_infcall_ops structure for ia64-hpux.  */

static const struct ia64_infcall_ops ia64_hpux_infcall_ops =
{
  ia64_hpux_allocate_new_rse_frame,
  ia64_hpux_store_argument_in_slot,
  ia64_hpux_set_function_addr
};

/* The "dummy_id" gdbarch routine for ia64-hpux.  */

static struct frame_id
ia64_hpux_dummy_id (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  CORE_ADDR sp, pc, bp_addr, bsp;

  sp = get_frame_register_unsigned (this_frame, IA64_GR12_REGNUM);

  /* Just double-check that the frame PC is within a certain region
     of the stack that would be plausible for our dummy code (the dummy
     code was pushed at SP + 16).  If not, then return a null frame ID.
     This is necessary in our case, because it is possible to produce
     the same frame ID for a normal frame, if that frame corresponds
     to the function called by our dummy code, and the function has not
     modified the registers that we use to build the dummy frame ID.  */
  pc = get_frame_pc (this_frame);
  if (pc < sp + 16 || pc >= sp + 16 + sizeof (ia64_hpux_dummy_code))
    return null_frame_id;

  /* The call sequence is such that the address of the dummy breakpoint
     we inserted is stored in r5.  */
  bp_addr = get_frame_register_unsigned (this_frame, IA64_GR5_REGNUM);

  bsp = get_frame_register_unsigned (this_frame, IA64_BSP_REGNUM);

  return frame_id_build_special (sp, bp_addr, bsp);
}

/* Should be set to non-NULL if the ia64-hpux solib module is linked in.
   This may not be the case because the shared library support code can
   only be compiled on ia64-hpux.  */

struct target_so_ops *ia64_hpux_so_ops = NULL;

/* The "find_global_pointer_from_solib" gdbarch_tdep routine for
   ia64-hpux.  */

static CORE_ADDR
ia64_hpux_find_global_pointer_from_solib (struct gdbarch *gdbarch,
					  CORE_ADDR faddr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct target_ops *ops = &current_target;
  gdb_byte buf[8];
  LONGEST len;

  len = target_read (ops, TARGET_OBJECT_HPUX_SOLIB_GOT,
		     paddress (gdbarch, faddr), buf, 0, sizeof (buf));

  return extract_unsigned_integer (buf, len, byte_order);
}

static void
ia64_hpux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  tdep->size_of_register_frame = ia64_hpux_size_of_register_frame;

  set_gdbarch_long_double_format (gdbarch, floatformats_ia64_quad);
  set_gdbarch_cannot_store_register (gdbarch, ia64_hpux_cannot_store_register);

  /* Inferior functions must be called from stack. */
  set_gdbarch_call_dummy_location (gdbarch, ON_STACK);
  set_gdbarch_push_dummy_code (gdbarch, ia64_hpux_push_dummy_code);
  tdep->infcall_ops = ia64_hpux_infcall_ops;
  tdep->find_global_pointer_from_solib
      = ia64_hpux_find_global_pointer_from_solib;
  set_gdbarch_dummy_id (gdbarch, ia64_hpux_dummy_id);

  if (ia64_hpux_so_ops)
    set_solib_ops (gdbarch, ia64_hpux_so_ops);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_ia64_hpux_tdep;

void
_initialize_ia64_hpux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_ia64, 0, GDB_OSABI_HPUX_ELF,
			  ia64_hpux_init_abi);
}
