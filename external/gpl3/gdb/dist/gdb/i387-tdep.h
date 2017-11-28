/* Target-dependent code for the i387.

   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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

#ifndef I387_TDEP_H
#define I387_TDEP_H

struct gdbarch;
struct frame_info;
struct regcache;
struct type;
struct ui_file;

/* Number of i387 floating point registers.  */
#define I387_NUM_REGS	16

#define I387_ST0_REGNUM(tdep) ((tdep)->st0_regnum)
#define I387_NUM_XMM_REGS(tdep) ((tdep)->num_xmm_regs)
#define I387_NUM_XMM_AVX512_REGS(tdep) ((tdep)->num_xmm_avx512_regs)
#define I387_MM0_REGNUM(tdep) ((tdep)->mm0_regnum)
#define I387_NUM_YMM_REGS(tdep) ((tdep)->num_ymm_regs)
#define I387_YMM0H_REGNUM(tdep) ((tdep)->ymm0h_regnum)

#define I387_BND0R_REGNUM(tdep) ((tdep)->bnd0r_regnum)
#define I387_BNDCFGU_REGNUM(tdep) ((tdep)->bndcfgu_regnum)

/* Set of constants used for 32 and 64-bit.  */
#define I387_NUM_MPX_REGS 6
#define I387_NUM_BND_REGS 4
#define I387_NUM_MPX_CTRL_REGS 2
#define I387_NUM_K_REGS 8
#define I387_NUM_PKEYS_REGS 1

#define I387_PKRU_REGNUM(tdep) ((tdep)->pkru_regnum)
#define I387_K0_REGNUM(tdep) ((tdep)->k0_regnum)
#define I387_NUM_ZMMH_REGS(tdep) ((tdep)->num_zmm_regs)
#define I387_ZMM0H_REGNUM(tdep) ((tdep)->zmm0h_regnum)
#define I387_NUM_YMM_AVX512_REGS(tdep) ((tdep)->num_ymm_avx512_regs)
#define I387_YMM16H_REGNUM(tdep) ((tdep)->ymm16h_regnum)

#define I387_FCTRL_REGNUM(tdep) (I387_ST0_REGNUM (tdep) + 8)
#define I387_FSTAT_REGNUM(tdep) (I387_FCTRL_REGNUM (tdep) + 1)
#define I387_FTAG_REGNUM(tdep) (I387_FCTRL_REGNUM (tdep) + 2)
#define I387_FISEG_REGNUM(tdep) (I387_FCTRL_REGNUM (tdep) + 3)
#define I387_FIOFF_REGNUM(tdep) (I387_FCTRL_REGNUM (tdep) + 4)
#define I387_FOSEG_REGNUM(tdep) (I387_FCTRL_REGNUM (tdep) + 5)
#define I387_FOOFF_REGNUM(tdep) (I387_FCTRL_REGNUM (tdep) + 6)
#define I387_FOP_REGNUM(tdep) (I387_FCTRL_REGNUM (tdep) + 7)
#define I387_XMM0_REGNUM(tdep) (I387_ST0_REGNUM (tdep) + 16)
#define I387_XMM16_REGNUM(tdep) ((tdep)->xmm16_regnum)
#define I387_MXCSR_REGNUM(tdep) \
  (I387_XMM0_REGNUM (tdep) + I387_NUM_XMM_REGS (tdep))
#define I387_YMM0_REGNUM(tdep) (I387_MXCSR_REGNUM(tdep) + 1)
#define I387_YMMENDH_REGNUM(tdep) \
  (I387_YMM0H_REGNUM (tdep) + I387_NUM_YMM_REGS (tdep))

#define I387_MPXEND_REGNUM(tdep) \
  (I387_BND0R_REGNUM (tdep) + I387_NUM_MPX_REGS)

#define I387_KEND_REGNUM(tdep) \
  (I387_K0_REGNUM (tdep) + I387_NUM_K_REGS)
#define I387_ZMMENDH_REGNUM(tdep) \
  (I387_ZMM0H_REGNUM (tdep) + I387_NUM_ZMMH_REGS (tdep))
#define I387_YMMH_AVX512_END_REGNUM(tdep) \
  (I387_YMM16H_REGNUM (tdep) + I387_NUM_YMM_AVX512_REGS (tdep))
#define I387_XMM_AVX512_END_REGNUM(tdep) \
  (I387_XMM16_REGNUM (tdep) + I387_NUM_XMM_AVX512_REGS (tdep))

#define I387_PKEYSEND_REGNUM(tdep) \
  (I387_PKRU_REGNUM (tdep) + I387_NUM_PKEYS_REGS)

/* Print out the i387 floating point state.  */

extern void i387_print_float_info (struct gdbarch *gdbarch,
				   struct ui_file *file,
				   struct frame_info *frame,
				   const char *args);

/* Return nonzero if a value of type TYPE stored in register REGNUM
   needs any special handling.  */

extern int i387_convert_register_p (struct gdbarch *gdbarch, int regnum,
				    struct type *type);

/* Read a value of type TYPE from register REGNUM in frame FRAME, and
   return its contents in TO.  */

extern int i387_register_to_value (struct frame_info *frame, int regnum,
				   struct type *type, gdb_byte *to,
				   int *optimizedp, int *unavailablep);

/* Write the contents FROM of a value of type TYPE into register
   REGNUM in frame FRAME.  */

extern void i387_value_to_register (struct frame_info *frame, int regnum,
				    struct type *type, const gdb_byte *from);


/* Size of the memory area use by the 'fsave' and 'fxsave'
   instructions.  */
#define I387_SIZEOF_FSAVE	108
#define I387_SIZEOF_FXSAVE	512

/* Fill register REGNUM in REGCACHE with the appropriate value from
   *FSAVE.  This function masks off any of the reserved bits in
   *FSAVE.  */

extern void i387_supply_fsave (struct regcache *regcache, int regnum,
			       const void *fsave);

/* Fill register REGNUM (if it is a floating-point register) in *FSAVE
   with the value from REGCACHE.  If REGNUM is -1, do this for all
   registers.  This function doesn't touch any of the reserved bits in
   *FSAVE.  */

extern void i387_collect_fsave (const struct regcache *regcache, int regnum,
				void *fsave);

/* Fill register REGNUM in REGCACHE with the appropriate
   floating-point or SSE register value from *FXSAVE.  This function
   masks off any of the reserved bits in *FXSAVE.  */

extern void i387_supply_fxsave (struct regcache *regcache, int regnum,
				const void *fxsave);

/* Similar to i387_supply_fxsave, but use XSAVE extended state.  */

extern void i387_supply_xsave (struct regcache *regcache, int regnum,
			       const void *xsave);

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FXSAVE with the value from REGCACHE.  If REGNUM is -1, do this for
   all registers.  This function doesn't touch any of the reserved
   bits in *FXSAVE.  */

extern void i387_collect_fxsave (const struct regcache *regcache, int regnum,
				 void *fxsave);

/* Similar to i387_collect_fxsave, but use XSAVE extended state.  */

extern void i387_collect_xsave (const struct regcache *regcache,
				int regnum, void *xsave, int gcore);

/* Prepare the FPU stack in REGCACHE for a function return.  */

extern void i387_return_value (struct gdbarch *gdbarch,
			       struct regcache *regcache);

/* Set all bnd registers to the INIT state.  INIT state means
   all memory range can be accessed.  */
extern void i387_reset_bnd_regs (struct gdbarch *gdbarch,
			         struct regcache *regcache);
#endif /* i387-tdep.h */
