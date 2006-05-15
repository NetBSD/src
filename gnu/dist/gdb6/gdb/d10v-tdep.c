// OBSOLETE /* Target-dependent code for Renesas D10V, for GDB.
// OBSOLETE 
// OBSOLETE    Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003 Free Software
// OBSOLETE    Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE /*  Contributed by Martin Hunt, hunt@cygnus.com */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "frame.h"
// OBSOLETE #include "frame-unwind.h"
// OBSOLETE #include "frame-base.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "gdbtypes.h"
// OBSOLETE #include "gdbcmd.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "gdb_string.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "dis-asm.h"
// OBSOLETE #include "symfile.h"
// OBSOLETE #include "objfiles.h"
// OBSOLETE #include "language.h"
// OBSOLETE #include "arch-utils.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE #include "remote.h"
// OBSOLETE #include "floatformat.h"
// OBSOLETE #include "gdb/sim-d10v.h"
// OBSOLETE #include "sim-regno.h"
// OBSOLETE #include "disasm.h"
// OBSOLETE #include "trad-frame.h"
// OBSOLETE 
// OBSOLETE #include "gdb_assert.h"
// OBSOLETE 
// OBSOLETE struct gdbarch_tdep
// OBSOLETE   {
// OBSOLETE     int a0_regnum;
// OBSOLETE     int nr_dmap_regs;
// OBSOLETE     unsigned long (*dmap_register) (void *regcache, int nr);
// OBSOLETE     unsigned long (*imap_register) (void *regcache, int nr);
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE /* These are the addresses the D10V-EVA board maps data and
// OBSOLETE    instruction memory to.  */
// OBSOLETE 
// OBSOLETE enum memspace {
// OBSOLETE   DMEM_START  = 0x2000000,
// OBSOLETE   IMEM_START  = 0x1000000,
// OBSOLETE   STACK_START = 0x200bffe
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* d10v register names.  */
// OBSOLETE 
// OBSOLETE enum
// OBSOLETE   {
// OBSOLETE     R0_REGNUM = 0,
// OBSOLETE     R3_REGNUM = 3,
// OBSOLETE     D10V_FP_REGNUM = 11,
// OBSOLETE     LR_REGNUM = 13,
// OBSOLETE     D10V_SP_REGNUM = 15,
// OBSOLETE     PSW_REGNUM = 16,
// OBSOLETE     D10V_PC_REGNUM = 18,
// OBSOLETE     NR_IMAP_REGS = 2,
// OBSOLETE     NR_A_REGS = 2,
// OBSOLETE     TS2_NUM_REGS = 37,
// OBSOLETE     TS3_NUM_REGS = 42,
// OBSOLETE     /* d10v calling convention.  */
// OBSOLETE     ARG1_REGNUM = R0_REGNUM,
// OBSOLETE     ARGN_REGNUM = R3_REGNUM
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE nr_dmap_regs (struct gdbarch *gdbarch)
// OBSOLETE {
// OBSOLETE   return gdbarch_tdep (gdbarch)->nr_dmap_regs;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE a0_regnum (struct gdbarch *gdbarch)
// OBSOLETE {
// OBSOLETE   return gdbarch_tdep (gdbarch)->a0_regnum;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Local functions */
// OBSOLETE 
// OBSOLETE extern void _initialize_d10v_tdep (void);
// OBSOLETE 
// OBSOLETE static void d10v_eva_prepare_to_trace (void);
// OBSOLETE 
// OBSOLETE static void d10v_eva_get_trace_data (void);
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
// OBSOLETE {
// OBSOLETE   /* Align to the size of an instruction (so that they can safely be
// OBSOLETE      pushed onto the stack.  */
// OBSOLETE   return sp & ~3;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static const unsigned char *
// OBSOLETE d10v_breakpoint_from_pc (CORE_ADDR *pcptr, int *lenptr)
// OBSOLETE {
// OBSOLETE   static unsigned char breakpoint[] =
// OBSOLETE   {0x2f, 0x90, 0x5e, 0x00};
// OBSOLETE   *lenptr = sizeof (breakpoint);
// OBSOLETE   return breakpoint;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Map the REG_NR onto an ascii name.  Return NULL or an empty string
// OBSOLETE    when the reg_nr isn't valid.  */
// OBSOLETE 
// OBSOLETE enum ts2_regnums
// OBSOLETE   {
// OBSOLETE     TS2_IMAP0_REGNUM = 32,
// OBSOLETE     TS2_DMAP_REGNUM = 34,
// OBSOLETE     TS2_NR_DMAP_REGS = 1,
// OBSOLETE     TS2_A0_REGNUM = 35
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE static const char *
// OBSOLETE d10v_ts2_register_name (int reg_nr)
// OBSOLETE {
// OBSOLETE   static char *register_names[] =
// OBSOLETE   {
// OBSOLETE     "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
// OBSOLETE     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
// OBSOLETE     "psw", "bpsw", "pc", "bpc", "cr4", "cr5", "cr6", "rpt_c",
// OBSOLETE     "rpt_s", "rpt_e", "mod_s", "mod_e", "cr12", "cr13", "iba", "cr15",
// OBSOLETE     "imap0", "imap1", "dmap", "a0", "a1"
// OBSOLETE   };
// OBSOLETE   if (reg_nr < 0)
// OBSOLETE     return NULL;
// OBSOLETE   if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
// OBSOLETE     return NULL;
// OBSOLETE   return register_names[reg_nr];
// OBSOLETE }
// OBSOLETE 
// OBSOLETE enum ts3_regnums
// OBSOLETE   {
// OBSOLETE     TS3_IMAP0_REGNUM = 36,
// OBSOLETE     TS3_DMAP0_REGNUM = 38,
// OBSOLETE     TS3_NR_DMAP_REGS = 4,
// OBSOLETE     TS3_A0_REGNUM = 32
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE static const char *
// OBSOLETE d10v_ts3_register_name (int reg_nr)
// OBSOLETE {
// OBSOLETE   static char *register_names[] =
// OBSOLETE   {
// OBSOLETE     "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
// OBSOLETE     "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
// OBSOLETE     "psw", "bpsw", "pc", "bpc", "cr4", "cr5", "cr6", "rpt_c",
// OBSOLETE     "rpt_s", "rpt_e", "mod_s", "mod_e", "cr12", "cr13", "iba", "cr15",
// OBSOLETE     "a0", "a1",
// OBSOLETE     "spi", "spu",
// OBSOLETE     "imap0", "imap1",
// OBSOLETE     "dmap0", "dmap1", "dmap2", "dmap3"
// OBSOLETE   };
// OBSOLETE   if (reg_nr < 0)
// OBSOLETE     return NULL;
// OBSOLETE   if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
// OBSOLETE     return NULL;
// OBSOLETE   return register_names[reg_nr];
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Access the DMAP/IMAP registers in a target independent way.
// OBSOLETE 
// OBSOLETE    Divide the D10V's 64k data space into four 16k segments:
// OBSOLETE    0x0000 -- 0x3fff, 0x4000 -- 0x7fff, 0x8000 -- 0xbfff, and 
// OBSOLETE    0xc000 -- 0xffff.
// OBSOLETE 
// OBSOLETE    On the TS2, the first two segments (0x0000 -- 0x3fff, 0x4000 --
// OBSOLETE    0x7fff) always map to the on-chip data RAM, and the fourth always
// OBSOLETE    maps to I/O space.  The third (0x8000 - 0xbfff) can be mapped into
// OBSOLETE    unified memory or instruction memory, under the control of the
// OBSOLETE    single DMAP register.
// OBSOLETE 
// OBSOLETE    On the TS3, there are four DMAP registers, each of which controls
// OBSOLETE    one of the segments.  */
// OBSOLETE 
// OBSOLETE static unsigned long
// OBSOLETE d10v_ts2_dmap_register (void *regcache, int reg_nr)
// OBSOLETE {
// OBSOLETE   switch (reg_nr)
// OBSOLETE     {
// OBSOLETE     case 0:
// OBSOLETE     case 1:
// OBSOLETE       return 0x2000;
// OBSOLETE     case 2:
// OBSOLETE       {
// OBSOLETE 	ULONGEST reg;
// OBSOLETE 	regcache_cooked_read_unsigned (regcache, TS2_DMAP_REGNUM, &reg);
// OBSOLETE 	return reg;
// OBSOLETE       }
// OBSOLETE     default:
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static unsigned long
// OBSOLETE d10v_ts3_dmap_register (void *regcache, int reg_nr)
// OBSOLETE {
// OBSOLETE   ULONGEST reg;
// OBSOLETE   regcache_cooked_read_unsigned (regcache, TS3_DMAP0_REGNUM + reg_nr, &reg);
// OBSOLETE   return reg;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static unsigned long
// OBSOLETE d10v_ts2_imap_register (void *regcache, int reg_nr)
// OBSOLETE {
// OBSOLETE   ULONGEST reg;
// OBSOLETE   regcache_cooked_read_unsigned (regcache, TS2_IMAP0_REGNUM + reg_nr, &reg);
// OBSOLETE   return reg;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static unsigned long
// OBSOLETE d10v_ts3_imap_register (void *regcache, int reg_nr)
// OBSOLETE {
// OBSOLETE   ULONGEST reg;
// OBSOLETE   regcache_cooked_read_unsigned (regcache, TS3_IMAP0_REGNUM + reg_nr, &reg);
// OBSOLETE   return reg;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* MAP GDB's internal register numbering (determined by the layout
// OBSOLETE    from the DEPRECATED_REGISTER_BYTE array) onto the simulator's
// OBSOLETE    register numbering.  */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE d10v_ts2_register_sim_regno (int nr)
// OBSOLETE {
// OBSOLETE   /* Only makes sense to supply raw registers.  */
// OBSOLETE   gdb_assert (nr >= 0 && nr < NUM_REGS);
// OBSOLETE   if (nr >= TS2_IMAP0_REGNUM
// OBSOLETE       && nr < TS2_IMAP0_REGNUM + NR_IMAP_REGS)
// OBSOLETE     return nr - TS2_IMAP0_REGNUM + SIM_D10V_IMAP0_REGNUM;
// OBSOLETE   if (nr == TS2_DMAP_REGNUM)
// OBSOLETE     return nr - TS2_DMAP_REGNUM + SIM_D10V_TS2_DMAP_REGNUM;
// OBSOLETE   if (nr >= TS2_A0_REGNUM
// OBSOLETE       && nr < TS2_A0_REGNUM + NR_A_REGS)
// OBSOLETE     return nr - TS2_A0_REGNUM + SIM_D10V_A0_REGNUM;
// OBSOLETE   return nr;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE d10v_ts3_register_sim_regno (int nr)
// OBSOLETE {
// OBSOLETE   /* Only makes sense to supply raw registers.  */
// OBSOLETE   gdb_assert (nr >= 0 && nr < NUM_REGS);
// OBSOLETE   if (nr >= TS3_IMAP0_REGNUM
// OBSOLETE       && nr < TS3_IMAP0_REGNUM + NR_IMAP_REGS)
// OBSOLETE     return nr - TS3_IMAP0_REGNUM + SIM_D10V_IMAP0_REGNUM;
// OBSOLETE   if (nr >= TS3_DMAP0_REGNUM
// OBSOLETE       && nr < TS3_DMAP0_REGNUM + TS3_NR_DMAP_REGS)
// OBSOLETE     return nr - TS3_DMAP0_REGNUM + SIM_D10V_DMAP0_REGNUM;
// OBSOLETE   if (nr >= TS3_A0_REGNUM
// OBSOLETE       && nr < TS3_A0_REGNUM + NR_A_REGS)
// OBSOLETE     return nr - TS3_A0_REGNUM + SIM_D10V_A0_REGNUM;
// OBSOLETE   return nr;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Return the GDB type object for the "standard" data type
// OBSOLETE    of data in register N.  */
// OBSOLETE 
// OBSOLETE static struct type *
// OBSOLETE d10v_register_type (struct gdbarch *gdbarch, int reg_nr)
// OBSOLETE {
// OBSOLETE   if (reg_nr == D10V_PC_REGNUM)
// OBSOLETE     return builtin_type (gdbarch)->builtin_func_ptr;
// OBSOLETE   if (reg_nr == D10V_SP_REGNUM || reg_nr == D10V_FP_REGNUM)
// OBSOLETE     return builtin_type (gdbarch)->builtin_data_ptr;
// OBSOLETE   else if (reg_nr >= a0_regnum (gdbarch)
// OBSOLETE 	   && reg_nr < (a0_regnum (gdbarch) + NR_A_REGS))
// OBSOLETE     return builtin_type_int64;
// OBSOLETE   else
// OBSOLETE     return builtin_type_int16;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE d10v_iaddr_p (CORE_ADDR x)
// OBSOLETE {
// OBSOLETE   return (((x) & 0x3000000) == IMEM_START);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_make_daddr (CORE_ADDR x)
// OBSOLETE {
// OBSOLETE   return ((x) | DMEM_START);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_make_iaddr (CORE_ADDR x)
// OBSOLETE {
// OBSOLETE   if (d10v_iaddr_p (x))
// OBSOLETE     return x;	/* Idempotency -- x is already in the IMEM space.  */
// OBSOLETE   else
// OBSOLETE     return (((x) << 2) | IMEM_START);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_convert_iaddr_to_raw (CORE_ADDR x)
// OBSOLETE {
// OBSOLETE   return (((x) >> 2) & 0xffff);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_convert_daddr_to_raw (CORE_ADDR x)
// OBSOLETE {
// OBSOLETE   return ((x) & 0xffff);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE d10v_address_to_pointer (struct type *type, void *buf, CORE_ADDR addr)
// OBSOLETE {
// OBSOLETE   /* Is it a code address?  */
// OBSOLETE   if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_FUNC
// OBSOLETE       || TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_METHOD)
// OBSOLETE     {
// OBSOLETE       store_unsigned_integer (buf, TYPE_LENGTH (type), 
// OBSOLETE                               d10v_convert_iaddr_to_raw (addr));
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* Strip off any upper segment bits.  */
// OBSOLETE       store_unsigned_integer (buf, TYPE_LENGTH (type), 
// OBSOLETE                               d10v_convert_daddr_to_raw (addr));
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_pointer_to_address (struct type *type, const void *buf)
// OBSOLETE {
// OBSOLETE   CORE_ADDR addr = extract_unsigned_integer (buf, TYPE_LENGTH (type));
// OBSOLETE   /* Is it a code address?  */
// OBSOLETE   if (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_FUNC
// OBSOLETE       || TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_METHOD
// OBSOLETE       || TYPE_CODE_SPACE (TYPE_TARGET_TYPE (type)))
// OBSOLETE     return d10v_make_iaddr (addr);
// OBSOLETE   else
// OBSOLETE     return d10v_make_daddr (addr);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Don't do anything if we have an integer, this way users can type 'x
// OBSOLETE    <addr>' w/o having gdb outsmart them.  The internal gdb conversions
// OBSOLETE    to the correct space are taken care of in the pointer_to_address
// OBSOLETE    function.  If we don't do this, 'x $fp' wouldn't work.  */
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_integer_to_address (struct type *type, void *buf)
// OBSOLETE {
// OBSOLETE   LONGEST val;
// OBSOLETE   val = unpack_long (type, buf);
// OBSOLETE   return val;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Handle the d10v's return_value convention.  */
// OBSOLETE 
// OBSOLETE static enum return_value_convention
// OBSOLETE d10v_return_value (struct gdbarch *gdbarch, struct type *valtype,
// OBSOLETE 		   struct regcache *regcache, void *readbuf,
// OBSOLETE 		   const void *writebuf)
// OBSOLETE {
// OBSOLETE   if (TYPE_LENGTH (valtype) > 8)
// OBSOLETE     /* Anything larger than 8 bytes (4 registers) goes on the stack.  */
// OBSOLETE     return RETURN_VALUE_STRUCT_CONVENTION;
// OBSOLETE   if (TYPE_LENGTH (valtype) == 5
// OBSOLETE       || TYPE_LENGTH (valtype) == 6)
// OBSOLETE     /* Anything 5 or 6 bytes in size goes in memory.  Contents don't
// OBSOLETE        appear to matter.  Note that 7 and 8 byte objects do end up in
// OBSOLETE        registers!  */
// OBSOLETE     return RETURN_VALUE_STRUCT_CONVENTION;
// OBSOLETE   if (TYPE_LENGTH (valtype) == 1)
// OBSOLETE     {
// OBSOLETE       /* All single byte values go in a register stored right-aligned.
// OBSOLETE          Note: 2 byte integer values are handled further down.  */
// OBSOLETE       if (readbuf)
// OBSOLETE 	{
// OBSOLETE 	  /* Since TYPE is smaller than the register, there isn't a
// OBSOLETE              sign extension problem.  Let the extraction truncate the
// OBSOLETE              register value.  */
// OBSOLETE 	  ULONGEST regval;
// OBSOLETE 	  regcache_cooked_read_unsigned (regcache, R0_REGNUM,
// OBSOLETE 					 &regval);
// OBSOLETE 	  store_unsigned_integer (readbuf, TYPE_LENGTH (valtype), regval);
// OBSOLETE 
// OBSOLETE 	}
// OBSOLETE       if (writebuf)
// OBSOLETE 	{
// OBSOLETE 	  ULONGEST regval;
// OBSOLETE 	  if (TYPE_CODE (valtype) == TYPE_CODE_INT)
// OBSOLETE 	    /* Some sort of integer value stored in R0.  Use
// OBSOLETE 	       unpack_long since that should handle any required sign
// OBSOLETE 	       extension.  */
// OBSOLETE 	    regval = unpack_long (valtype, writebuf);
// OBSOLETE 	  else
// OBSOLETE 	    /* Some other type.  Don't sign-extend the value when
// OBSOLETE                storing it in the register.  */
// OBSOLETE 	    regval = extract_unsigned_integer (writebuf, 1);
// OBSOLETE 	  regcache_cooked_write_unsigned (regcache, R0_REGNUM, regval);
// OBSOLETE 	}
// OBSOLETE       return RETURN_VALUE_REGISTER_CONVENTION;
// OBSOLETE     }
// OBSOLETE   if ((TYPE_CODE (valtype) == TYPE_CODE_STRUCT
// OBSOLETE        || TYPE_CODE (valtype) == TYPE_CODE_UNION)
// OBSOLETE       && TYPE_NFIELDS (valtype) > 1
// OBSOLETE       && TYPE_FIELD_BITPOS (valtype, 1) == 8)
// OBSOLETE     /* If a composite is 8 bit aligned (determined by looking at the
// OBSOLETE        start address of the second field), put it in memory.  */
// OBSOLETE     return RETURN_VALUE_STRUCT_CONVENTION;
// OBSOLETE   /* Assume it is in registers.  */
// OBSOLETE   if (writebuf || readbuf)
// OBSOLETE     {
// OBSOLETE       int reg;
// OBSOLETE       /* Per above, the value is never more than 8 bytes long.  */
// OBSOLETE       gdb_assert (TYPE_LENGTH (valtype) <= 8);
// OBSOLETE       /* Xfer 2 bytes at a time.  */
// OBSOLETE       for (reg = 0; (reg * 2) + 1 < TYPE_LENGTH (valtype); reg++)
// OBSOLETE 	{
// OBSOLETE 	  if (readbuf)
// OBSOLETE 	    regcache_cooked_read (regcache, R0_REGNUM + reg,
// OBSOLETE 				  (bfd_byte *) readbuf + reg * 2);
// OBSOLETE 	  if (writebuf)
// OBSOLETE 	    regcache_cooked_write (regcache, R0_REGNUM + reg,
// OBSOLETE 				   (bfd_byte *) writebuf + reg * 2);
// OBSOLETE 	}
// OBSOLETE       /* Any trailing byte ends up _left_ aligned.  */
// OBSOLETE       if ((reg * 2) < TYPE_LENGTH (valtype))
// OBSOLETE 	{
// OBSOLETE 	  if (readbuf)
// OBSOLETE 	    regcache_cooked_read_part (regcache, R0_REGNUM + reg,
// OBSOLETE 				       0, 1, (bfd_byte *) readbuf + reg * 2);
// OBSOLETE 	  if (writebuf)
// OBSOLETE 	    regcache_cooked_write_part (regcache, R0_REGNUM + reg,
// OBSOLETE 					0, 1, (bfd_byte *) writebuf + reg * 2);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   return RETURN_VALUE_REGISTER_CONVENTION;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE check_prologue (unsigned short op)
// OBSOLETE {
// OBSOLETE   /* st  rn, @-sp */
// OBSOLETE   if ((op & 0x7E1F) == 0x6C1F)
// OBSOLETE     return 1;
// OBSOLETE 
// OBSOLETE   /* st2w  rn, @-sp */
// OBSOLETE   if ((op & 0x7E3F) == 0x6E1F)
// OBSOLETE     return 1;
// OBSOLETE 
// OBSOLETE   /* subi  sp, n */
// OBSOLETE   if ((op & 0x7FE1) == 0x01E1)
// OBSOLETE     return 1;
// OBSOLETE 
// OBSOLETE   /* mv  r11, sp */
// OBSOLETE   if (op == 0x417E)
// OBSOLETE     return 1;
// OBSOLETE 
// OBSOLETE   /* nop */
// OBSOLETE   if (op == 0x5E00)
// OBSOLETE     return 1;
// OBSOLETE 
// OBSOLETE   /* st  rn, @sp */
// OBSOLETE   if ((op & 0x7E1F) == 0x681E)
// OBSOLETE     return 1;
// OBSOLETE 
// OBSOLETE   /* st2w  rn, @sp */
// OBSOLETE   if ((op & 0x7E3F) == 0x3A1E)
// OBSOLETE     return 1;
// OBSOLETE 
// OBSOLETE   return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_skip_prologue (CORE_ADDR pc)
// OBSOLETE {
// OBSOLETE   unsigned long op;
// OBSOLETE   unsigned short op1, op2;
// OBSOLETE   CORE_ADDR func_addr, func_end;
// OBSOLETE   struct symtab_and_line sal;
// OBSOLETE 
// OBSOLETE   /* If we have line debugging information, then the end of the prologue 
// OBSOLETE      should be the first assembly instruction of the first source line.  */
// OBSOLETE   if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
// OBSOLETE     {
// OBSOLETE       sal = find_pc_line (func_addr, 0);
// OBSOLETE       if (sal.end && sal.end < func_end)
// OBSOLETE 	return sal.end;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (target_read_memory (pc, (char *) &op, 4))
// OBSOLETE     return pc;			/* Can't access it -- assume no prologue.  */
// OBSOLETE 
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       op = (unsigned long) read_memory_integer (pc, 4);
// OBSOLETE       if ((op & 0xC0000000) == 0xC0000000)
// OBSOLETE 	{
// OBSOLETE 	  /* long instruction */
// OBSOLETE 	  if (((op & 0x3FFF0000) != 0x01FF0000) &&	/* add3 sp,sp,n */
// OBSOLETE 	      ((op & 0x3F0F0000) != 0x340F0000) &&	/* st  rn, @(offset,sp) */
// OBSOLETE 	      ((op & 0x3F1F0000) != 0x350F0000))	/* st2w  rn, @(offset,sp) */
// OBSOLETE 	    break;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* short instructions */
// OBSOLETE 	  if ((op & 0xC0000000) == 0x80000000)
// OBSOLETE 	    {
// OBSOLETE 	      op2 = (op & 0x3FFF8000) >> 15;
// OBSOLETE 	      op1 = op & 0x7FFF;
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      op1 = (op & 0x3FFF8000) >> 15;
// OBSOLETE 	      op2 = op & 0x7FFF;
// OBSOLETE 	    }
// OBSOLETE 	  if (check_prologue (op1))
// OBSOLETE 	    {
// OBSOLETE 	      if (!check_prologue (op2))
// OBSOLETE 		{
// OBSOLETE 		  /* If the previous opcode was really part of the
// OBSOLETE 		     prologue and not just a NOP, then we want to
// OBSOLETE 		     break after both instructions.  */
// OBSOLETE 		  if (op1 != 0x5E00)
// OBSOLETE 		    pc += 4;
// OBSOLETE 		  break;
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    break;
// OBSOLETE 	}
// OBSOLETE       pc += 4;
// OBSOLETE     }
// OBSOLETE   return pc;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE struct d10v_unwind_cache
// OBSOLETE {
// OBSOLETE   /* The previous frame's inner most stack address.  Used as this
// OBSOLETE      frame ID's stack_addr.  */
// OBSOLETE   CORE_ADDR prev_sp;
// OBSOLETE   /* The frame's base, optionally used by the high-level debug info.  */
// OBSOLETE   CORE_ADDR base;
// OBSOLETE   int size;
// OBSOLETE   /* How far the SP and r11 (FP) have been offset from the start of
// OBSOLETE      the stack frame (as defined by the previous frame's stack
// OBSOLETE      pointer).  */
// OBSOLETE   LONGEST sp_offset;
// OBSOLETE   LONGEST r11_offset;
// OBSOLETE   int uses_frame;
// OBSOLETE   /* Table indicating the location of each and every register.  */
// OBSOLETE   struct trad_frame_saved_reg *saved_regs;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE prologue_find_regs (struct d10v_unwind_cache *info, unsigned short op,
// OBSOLETE 		    CORE_ADDR addr)
// OBSOLETE {
// OBSOLETE   int n;
// OBSOLETE 
// OBSOLETE   /* st  rn, @-sp */
// OBSOLETE   if ((op & 0x7E1F) == 0x6C1F)
// OBSOLETE     {
// OBSOLETE       n = (op & 0x1E0) >> 5;
// OBSOLETE       info->sp_offset -= 2;
// OBSOLETE       info->saved_regs[n].addr = info->sp_offset;
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* st2w  rn, @-sp */
// OBSOLETE   else if ((op & 0x7E3F) == 0x6E1F)
// OBSOLETE     {
// OBSOLETE       n = (op & 0x1E0) >> 5;
// OBSOLETE       info->sp_offset -= 4;
// OBSOLETE       info->saved_regs[n + 0].addr = info->sp_offset + 0;
// OBSOLETE       info->saved_regs[n + 1].addr = info->sp_offset + 2;
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* subi  sp, n */
// OBSOLETE   if ((op & 0x7FE1) == 0x01E1)
// OBSOLETE     {
// OBSOLETE       n = (op & 0x1E) >> 1;
// OBSOLETE       if (n == 0)
// OBSOLETE 	n = 16;
// OBSOLETE       info->sp_offset -= n;
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* mv  r11, sp */
// OBSOLETE   if (op == 0x417E)
// OBSOLETE     {
// OBSOLETE       info->uses_frame = 1;
// OBSOLETE       info->r11_offset = info->sp_offset;
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* st  rn, @r11 */
// OBSOLETE   if ((op & 0x7E1F) == 0x6816)
// OBSOLETE     {
// OBSOLETE       n = (op & 0x1E0) >> 5;
// OBSOLETE       info->saved_regs[n].addr = info->r11_offset;
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* nop */
// OBSOLETE   if (op == 0x5E00)
// OBSOLETE     return 1;
// OBSOLETE 
// OBSOLETE   /* st  rn, @sp */
// OBSOLETE   if ((op & 0x7E1F) == 0x681E)
// OBSOLETE     {
// OBSOLETE       n = (op & 0x1E0) >> 5;
// OBSOLETE       info->saved_regs[n].addr = info->sp_offset;
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* st2w  rn, @sp */
// OBSOLETE   if ((op & 0x7E3F) == 0x3A1E)
// OBSOLETE     {
// OBSOLETE       n = (op & 0x1E0) >> 5;
// OBSOLETE       info->saved_regs[n + 0].addr = info->sp_offset + 0;
// OBSOLETE       info->saved_regs[n + 1].addr = info->sp_offset + 2;
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Put here the code to store, into fi->saved_regs, the addresses of
// OBSOLETE    the saved registers of frame described by FRAME_INFO.  This
// OBSOLETE    includes special registers such as pc and fp saved in special ways
// OBSOLETE    in the stack frame.  sp is even more special: the address we return
// OBSOLETE    for it IS the sp for the next frame.  */
// OBSOLETE 
// OBSOLETE static struct d10v_unwind_cache *
// OBSOLETE d10v_frame_unwind_cache (struct frame_info *next_frame,
// OBSOLETE 			 void **this_prologue_cache)
// OBSOLETE {
// OBSOLETE   struct gdbarch *gdbarch = get_frame_arch (next_frame);
// OBSOLETE   CORE_ADDR pc;
// OBSOLETE   ULONGEST prev_sp;
// OBSOLETE   ULONGEST this_base;
// OBSOLETE   unsigned long op;
// OBSOLETE   unsigned short op1, op2;
// OBSOLETE   int i;
// OBSOLETE   struct d10v_unwind_cache *info;
// OBSOLETE 
// OBSOLETE   if ((*this_prologue_cache))
// OBSOLETE     return (*this_prologue_cache);
// OBSOLETE 
// OBSOLETE   info = FRAME_OBSTACK_ZALLOC (struct d10v_unwind_cache);
// OBSOLETE   (*this_prologue_cache) = info;
// OBSOLETE   info->saved_regs = trad_frame_alloc_saved_regs (next_frame);
// OBSOLETE 
// OBSOLETE   info->size = 0;
// OBSOLETE   info->sp_offset = 0;
// OBSOLETE 
// OBSOLETE   info->uses_frame = 0;
// OBSOLETE   for (pc = frame_func_unwind (next_frame);
// OBSOLETE        pc > 0 && pc < frame_pc_unwind (next_frame);
// OBSOLETE        pc += 4)
// OBSOLETE     {
// OBSOLETE       op = get_frame_memory_unsigned (next_frame, pc, 4);
// OBSOLETE       if ((op & 0xC0000000) == 0xC0000000)
// OBSOLETE 	{
// OBSOLETE 	  /* long instruction */
// OBSOLETE 	  if ((op & 0x3FFF0000) == 0x01FF0000)
// OBSOLETE 	    {
// OBSOLETE 	      /* add3 sp,sp,n */
// OBSOLETE 	      short n = op & 0xFFFF;
// OBSOLETE 	      info->sp_offset += n;
// OBSOLETE 	    }
// OBSOLETE 	  else if ((op & 0x3F0F0000) == 0x340F0000)
// OBSOLETE 	    {
// OBSOLETE 	      /* st  rn, @(offset,sp) */
// OBSOLETE 	      short offset = op & 0xFFFF;
// OBSOLETE 	      short n = (op >> 20) & 0xF;
// OBSOLETE 	      info->saved_regs[n].addr = info->sp_offset + offset;
// OBSOLETE 	    }
// OBSOLETE 	  else if ((op & 0x3F1F0000) == 0x350F0000)
// OBSOLETE 	    {
// OBSOLETE 	      /* st2w  rn, @(offset,sp) */
// OBSOLETE 	      short offset = op & 0xFFFF;
// OBSOLETE 	      short n = (op >> 20) & 0xF;
// OBSOLETE 	      info->saved_regs[n + 0].addr = info->sp_offset + offset + 0;
// OBSOLETE 	      info->saved_regs[n + 1].addr = info->sp_offset + offset + 2;
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    break;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* short instructions */
// OBSOLETE 	  if ((op & 0xC0000000) == 0x80000000)
// OBSOLETE 	    {
// OBSOLETE 	      op2 = (op & 0x3FFF8000) >> 15;
// OBSOLETE 	      op1 = op & 0x7FFF;
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      op1 = (op & 0x3FFF8000) >> 15;
// OBSOLETE 	      op2 = op & 0x7FFF;
// OBSOLETE 	    }
// OBSOLETE 	  if (!prologue_find_regs (info, op1, pc) 
// OBSOLETE 	      || !prologue_find_regs (info, op2, pc))
// OBSOLETE 	    break;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   info->size = -info->sp_offset;
// OBSOLETE 
// OBSOLETE   /* Compute the previous frame's stack pointer (which is also the
// OBSOLETE      frame's ID's stack address), and this frame's base pointer.  */
// OBSOLETE   if (info->uses_frame)
// OBSOLETE     {
// OBSOLETE       /* The SP was moved to the FP.  This indicates that a new frame
// OBSOLETE          was created.  Get THIS frame's FP value by unwinding it from
// OBSOLETE          the next frame.  */
// OBSOLETE       frame_unwind_unsigned_register (next_frame, D10V_FP_REGNUM, &this_base);
// OBSOLETE       /* The FP points at the last saved register.  Adjust the FP back
// OBSOLETE          to before the first saved register giving the SP.  */
// OBSOLETE       prev_sp = this_base + info->size;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* Assume that the FP is this frame's SP but with that pushed
// OBSOLETE          stack space added back.  */
// OBSOLETE       frame_unwind_unsigned_register (next_frame, D10V_SP_REGNUM, &this_base);
// OBSOLETE       prev_sp = this_base + info->size;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Convert that SP/BASE into real addresses.  */
// OBSOLETE   info->prev_sp =  d10v_make_daddr (prev_sp);
// OBSOLETE   info->base = d10v_make_daddr (this_base);
// OBSOLETE 
// OBSOLETE   /* Adjust all the saved registers so that they contain addresses and
// OBSOLETE      not offsets.  */
// OBSOLETE   for (i = 0; i < NUM_REGS - 1; i++)
// OBSOLETE     if (trad_frame_addr_p (info->saved_regs, i))
// OBSOLETE       {
// OBSOLETE 	info->saved_regs[i].addr = (info->prev_sp + info->saved_regs[i].addr);
// OBSOLETE       }
// OBSOLETE 
// OBSOLETE   /* The call instruction moves the caller's PC in the callee's LR.
// OBSOLETE      Since this is an unwind, do the reverse.  Copy the location of LR
// OBSOLETE      into PC (the address / regnum) so that a request for PC will be
// OBSOLETE      converted into a request for the LR.  */
// OBSOLETE   info->saved_regs[D10V_PC_REGNUM] = info->saved_regs[LR_REGNUM];
// OBSOLETE 
// OBSOLETE   /* The previous frame's SP needed to be computed.  Save the computed
// OBSOLETE      value.  */
// OBSOLETE   trad_frame_set_value (info->saved_regs, D10V_SP_REGNUM,
// OBSOLETE 			d10v_make_daddr (prev_sp));
// OBSOLETE 
// OBSOLETE   return info;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE d10v_print_registers_info (struct gdbarch *gdbarch, struct ui_file *file,
// OBSOLETE 			   struct frame_info *frame, int regnum, int all)
// OBSOLETE {
// OBSOLETE   struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
// OBSOLETE   if (regnum >= 0)
// OBSOLETE     {
// OBSOLETE       default_print_registers_info (gdbarch, file, frame, regnum, all);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     ULONGEST pc, psw, rpt_s, rpt_e, rpt_c;
// OBSOLETE     pc = get_frame_register_unsigned (frame, D10V_PC_REGNUM);
// OBSOLETE     psw = get_frame_register_unsigned (frame, PSW_REGNUM);
// OBSOLETE     rpt_s = get_frame_register_unsigned (frame, frame_map_name_to_regnum (frame, "rpt_s", -1));
// OBSOLETE     rpt_e = get_frame_register_unsigned (frame, frame_map_name_to_regnum (frame, "rpt_e", -1));
// OBSOLETE     rpt_c = get_frame_register_unsigned (frame, frame_map_name_to_regnum (frame, "rpt_c", -1));
// OBSOLETE     fprintf_filtered (file, "PC=%04lx (0x%lx) PSW=%04lx RPT_S=%04lx RPT_E=%04lx RPT_C=%04lx\n",
// OBSOLETE 		     (long) pc, (long) d10v_make_iaddr (pc), (long) psw,
// OBSOLETE 		     (long) rpt_s, (long) rpt_e, (long) rpt_c);
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     int group;
// OBSOLETE     for (group = 0; group < 16; group += 8)
// OBSOLETE       {
// OBSOLETE 	int r;
// OBSOLETE 	fprintf_filtered (file, "R%d-R%-2d", group, group + 7);
// OBSOLETE 	for (r = group; r < group + 8; r++)
// OBSOLETE 	  {
// OBSOLETE 	    ULONGEST tmp;
// OBSOLETE 	    tmp = get_frame_register_unsigned (frame, r);
// OBSOLETE 	    fprintf_filtered (file, " %04lx", (long) tmp);
// OBSOLETE 	  }
// OBSOLETE 	fprintf_filtered (file, "\n");
// OBSOLETE       }
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   /* Note: The IMAP/DMAP registers don't participate in function
// OBSOLETE      calls.  Don't bother trying to unwind them.  */
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     int a;
// OBSOLETE     for (a = 0; a < NR_IMAP_REGS; a++)
// OBSOLETE       {
// OBSOLETE 	if (a > 0)
// OBSOLETE 	  fprintf_filtered (file, "    ");
// OBSOLETE 	fprintf_filtered (file, "IMAP%d %04lx", a,
// OBSOLETE 			  tdep->imap_register (current_regcache, a));
// OBSOLETE       }
// OBSOLETE     if (nr_dmap_regs (gdbarch) == 1)
// OBSOLETE       /* Registers DMAP0 and DMAP1 are constant.  Just return dmap2.  */
// OBSOLETE       fprintf_filtered (file, "    DMAP %04lx\n",
// OBSOLETE 			tdep->dmap_register (current_regcache, 2));
// OBSOLETE     else
// OBSOLETE       {
// OBSOLETE 	for (a = 0; a < nr_dmap_regs (gdbarch); a++)
// OBSOLETE 	  {
// OBSOLETE 	    fprintf_filtered (file, "    DMAP%d %04lx", a,
// OBSOLETE 			      tdep->dmap_register (current_regcache, a));
// OBSOLETE 	  }
// OBSOLETE 	fprintf_filtered (file, "\n");
// OBSOLETE       }
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     char num[MAX_REGISTER_SIZE];
// OBSOLETE     int a;
// OBSOLETE     fprintf_filtered (file, "A0-A%d", NR_A_REGS - 1);
// OBSOLETE     for (a = a0_regnum (gdbarch); a < a0_regnum (gdbarch) + NR_A_REGS; a++)
// OBSOLETE       {
// OBSOLETE 	int i;
// OBSOLETE 	fprintf_filtered (file, "  ");
// OBSOLETE 	get_frame_register (frame, a, num);
// OBSOLETE 	for (i = 0; i < register_size (gdbarch, a); i++)
// OBSOLETE 	  {
// OBSOLETE 	    fprintf_filtered (file, "%02x", (num[i] & 0xff));
// OBSOLETE 	  }
// OBSOLETE       }
// OBSOLETE   }
// OBSOLETE   fprintf_filtered (file, "\n");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE show_regs (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   d10v_print_registers_info (current_gdbarch, gdb_stdout,
// OBSOLETE 			     get_current_frame (), -1, 1);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_read_pc (ptid_t ptid)
// OBSOLETE {
// OBSOLETE   ptid_t save_ptid;
// OBSOLETE   CORE_ADDR pc;
// OBSOLETE   CORE_ADDR retval;
// OBSOLETE 
// OBSOLETE   save_ptid = inferior_ptid;
// OBSOLETE   inferior_ptid = ptid;
// OBSOLETE   pc = (int) read_register (D10V_PC_REGNUM);
// OBSOLETE   inferior_ptid = save_ptid;
// OBSOLETE   retval = d10v_make_iaddr (pc);
// OBSOLETE   return retval;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE d10v_write_pc (CORE_ADDR val, ptid_t ptid)
// OBSOLETE {
// OBSOLETE   ptid_t save_ptid;
// OBSOLETE 
// OBSOLETE   save_ptid = inferior_ptid;
// OBSOLETE   inferior_ptid = ptid;
// OBSOLETE   write_register (D10V_PC_REGNUM, d10v_convert_iaddr_to_raw (val));
// OBSOLETE   inferior_ptid = save_ptid;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame)
// OBSOLETE {
// OBSOLETE   ULONGEST sp;
// OBSOLETE   frame_unwind_unsigned_register (next_frame, D10V_SP_REGNUM, &sp);
// OBSOLETE   return d10v_make_daddr (sp);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* When arguments must be pushed onto the stack, they go on in reverse
// OBSOLETE    order.  The below implements a FILO (stack) to do this.  */
// OBSOLETE 
// OBSOLETE struct stack_item
// OBSOLETE {
// OBSOLETE   int len;
// OBSOLETE   struct stack_item *prev;
// OBSOLETE   void *data;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static struct stack_item *push_stack_item (struct stack_item *prev,
// OBSOLETE 					   void *contents, int len);
// OBSOLETE static struct stack_item *
// OBSOLETE push_stack_item (struct stack_item *prev, void *contents, int len)
// OBSOLETE {
// OBSOLETE   struct stack_item *si;
// OBSOLETE   si = xmalloc (sizeof (struct stack_item));
// OBSOLETE   si->data = xmalloc (len);
// OBSOLETE   si->len = len;
// OBSOLETE   si->prev = prev;
// OBSOLETE   memcpy (si->data, contents, len);
// OBSOLETE   return si;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct stack_item *pop_stack_item (struct stack_item *si);
// OBSOLETE static struct stack_item *
// OBSOLETE pop_stack_item (struct stack_item *si)
// OBSOLETE {
// OBSOLETE   struct stack_item *dead = si;
// OBSOLETE   si = si->prev;
// OBSOLETE   xfree (dead->data);
// OBSOLETE   xfree (dead);
// OBSOLETE   return si;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_push_dummy_code (struct gdbarch *gdbarch,
// OBSOLETE 		      CORE_ADDR sp, CORE_ADDR funaddr, int using_gcc,
// OBSOLETE 		      struct value **args, int nargs,
// OBSOLETE 		      struct type *value_type,
// OBSOLETE 		      CORE_ADDR *real_pc, CORE_ADDR *bp_addr)
// OBSOLETE {
// OBSOLETE   /* Allocate space sufficient for a breakpoint.  */
// OBSOLETE   sp = (sp - 4) & ~3;
// OBSOLETE   /* Store the address of that breakpoint taking care to first convert
// OBSOLETE      it into a code (IADDR) address from a stack (DADDR) address.
// OBSOLETE      This of course assumes that the two virtual addresses map onto
// OBSOLETE      the same real address.  */
// OBSOLETE   (*bp_addr) = d10v_make_iaddr (d10v_convert_iaddr_to_raw (sp));
// OBSOLETE   /* d10v always starts the call at the callee's entry point.  */
// OBSOLETE   (*real_pc) = funaddr;
// OBSOLETE   return sp;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
// OBSOLETE 		      struct regcache *regcache, CORE_ADDR bp_addr,
// OBSOLETE 		      int nargs, struct value **args, CORE_ADDR sp, 
// OBSOLETE 		      int struct_return, CORE_ADDR struct_addr)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE   int regnum = ARG1_REGNUM;
// OBSOLETE   struct stack_item *si = NULL;
// OBSOLETE   long val;
// OBSOLETE 
// OBSOLETE   /* Set the return address.  For the d10v, the return breakpoint is
// OBSOLETE      always at BP_ADDR.  */
// OBSOLETE   regcache_cooked_write_unsigned (regcache, LR_REGNUM,
// OBSOLETE 				  d10v_convert_iaddr_to_raw (bp_addr));
// OBSOLETE 
// OBSOLETE   /* If STRUCT_RETURN is true, then the struct return address (in
// OBSOLETE      STRUCT_ADDR) will consume the first argument-passing register.
// OBSOLETE      Both adjust the register count and store that value.  */
// OBSOLETE   if (struct_return)
// OBSOLETE     {
// OBSOLETE       regcache_cooked_write_unsigned (regcache, regnum, struct_addr);
// OBSOLETE       regnum++;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Fill in registers and arg lists */
// OBSOLETE   for (i = 0; i < nargs; i++)
// OBSOLETE     {
// OBSOLETE       struct value *arg = args[i];
// OBSOLETE       struct type *type = check_typedef (VALUE_TYPE (arg));
// OBSOLETE       char *contents = VALUE_CONTENTS (arg);
// OBSOLETE       int len = TYPE_LENGTH (type);
// OBSOLETE       int aligned_regnum = (regnum + 1) & ~1;
// OBSOLETE 
// OBSOLETE       /* printf ("push: type=%d len=%d\n", TYPE_CODE (type), len); */
// OBSOLETE       if (len <= 2 && regnum <= ARGN_REGNUM)
// OBSOLETE 	/* fits in a single register, do not align */
// OBSOLETE 	{
// OBSOLETE 	  val = extract_unsigned_integer (contents, len);
// OBSOLETE 	  regcache_cooked_write_unsigned (regcache, regnum++, val);
// OBSOLETE 	}
// OBSOLETE       else if (len <= (ARGN_REGNUM - aligned_regnum + 1) * 2)
// OBSOLETE 	/* value fits in remaining registers, store keeping left
// OBSOLETE 	   aligned */
// OBSOLETE 	{
// OBSOLETE 	  int b;
// OBSOLETE 	  regnum = aligned_regnum;
// OBSOLETE 	  for (b = 0; b < (len & ~1); b += 2)
// OBSOLETE 	    {
// OBSOLETE 	      val = extract_unsigned_integer (&contents[b], 2);
// OBSOLETE 	      regcache_cooked_write_unsigned (regcache, regnum++, val);
// OBSOLETE 	    }
// OBSOLETE 	  if (b < len)
// OBSOLETE 	    {
// OBSOLETE 	      val = extract_unsigned_integer (&contents[b], 1);
// OBSOLETE 	      regcache_cooked_write_unsigned (regcache, regnum++, (val << 8));
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* arg will go onto stack */
// OBSOLETE 	  regnum = ARGN_REGNUM + 1;
// OBSOLETE 	  si = push_stack_item (si, contents, len);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   while (si)
// OBSOLETE     {
// OBSOLETE       sp = (sp - si->len) & ~1;
// OBSOLETE       write_memory (sp, si->data, si->len);
// OBSOLETE       si = pop_stack_item (si);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Finally, update the SP register.  */
// OBSOLETE   regcache_cooked_write_unsigned (regcache, D10V_SP_REGNUM,
// OBSOLETE 				  d10v_convert_daddr_to_raw (sp));
// OBSOLETE 
// OBSOLETE   return sp;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Translate a GDB virtual ADDR/LEN into a format the remote target
// OBSOLETE    understands.  Returns number of bytes that can be transfered
// OBSOLETE    starting at TARG_ADDR.  Return ZERO if no bytes can be transfered
// OBSOLETE    (segmentation fault).  Since the simulator knows all about how the
// OBSOLETE    VM system works, we just call that to do the translation.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE remote_d10v_translate_xfer_address (struct gdbarch *gdbarch,
// OBSOLETE 				    struct regcache *regcache,
// OBSOLETE 				    CORE_ADDR memaddr, int nr_bytes,
// OBSOLETE 				    CORE_ADDR *targ_addr, int *targ_len)
// OBSOLETE {
// OBSOLETE   struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
// OBSOLETE   long out_addr;
// OBSOLETE   long out_len;
// OBSOLETE   out_len = sim_d10v_translate_addr (memaddr, nr_bytes, &out_addr, regcache,
// OBSOLETE 				     tdep->dmap_register, tdep->imap_register);
// OBSOLETE   *targ_addr = out_addr;
// OBSOLETE   *targ_len = out_len;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* The following code implements access to, and display of, the D10V's
// OBSOLETE    instruction trace buffer.  The buffer consists of 64K or more
// OBSOLETE    4-byte words of data, of which each words includes an 8-bit count,
// OBSOLETE    an 8-bit segment number, and a 16-bit instruction address.
// OBSOLETE 
// OBSOLETE    In theory, the trace buffer is continuously capturing instruction
// OBSOLETE    data that the CPU presents on its "debug bus", but in practice, the
// OBSOLETE    ROMified GDB stub only enables tracing when it continues or steps
// OBSOLETE    the program, and stops tracing when the program stops; so it
// OBSOLETE    actually works for GDB to read the buffer counter out of memory and
// OBSOLETE    then read each trace word.  The counter records where the tracing
// OBSOLETE    stops, but there is no record of where it started, so we remember
// OBSOLETE    the PC when we resumed and then search backwards in the trace
// OBSOLETE    buffer for a word that includes that address.  This is not perfect,
// OBSOLETE    because you will miss trace data if the resumption PC is the target
// OBSOLETE    of a branch.  (The value of the buffer counter is semi-random, any
// OBSOLETE    trace data from a previous program stop is gone.)  */
// OBSOLETE 
// OBSOLETE /* The address of the last word recorded in the trace buffer.  */
// OBSOLETE 
// OBSOLETE #define DBBC_ADDR (0xd80000)
// OBSOLETE 
// OBSOLETE /* The base of the trace buffer, at least for the "Board_0".  */
// OBSOLETE 
// OBSOLETE #define TRACE_BUFFER_BASE (0xf40000)
// OBSOLETE 
// OBSOLETE static void trace_command (char *, int);
// OBSOLETE 
// OBSOLETE static void untrace_command (char *, int);
// OBSOLETE 
// OBSOLETE static void trace_info (char *, int);
// OBSOLETE 
// OBSOLETE static void tdisassemble_command (char *, int);
// OBSOLETE 
// OBSOLETE static void display_trace (int, int);
// OBSOLETE 
// OBSOLETE /* True when instruction traces are being collected.  */
// OBSOLETE 
// OBSOLETE static int tracing;
// OBSOLETE 
// OBSOLETE /* Remembered PC.  */
// OBSOLETE 
// OBSOLETE static CORE_ADDR last_pc;
// OBSOLETE 
// OBSOLETE /* True when trace output should be displayed whenever program stops.  */
// OBSOLETE 
// OBSOLETE static int trace_display;
// OBSOLETE 
// OBSOLETE /* True when trace listing should include source lines.  */
// OBSOLETE 
// OBSOLETE static int default_trace_show_source = 1;
// OBSOLETE 
// OBSOLETE struct trace_buffer
// OBSOLETE   {
// OBSOLETE     int size;
// OBSOLETE     short *counts;
// OBSOLETE     CORE_ADDR *addrs;
// OBSOLETE   }
// OBSOLETE trace_data;
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE trace_command (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   /* Clear the host-side trace buffer, allocating space if needed.  */
// OBSOLETE   trace_data.size = 0;
// OBSOLETE   if (trace_data.counts == NULL)
// OBSOLETE     trace_data.counts = XCALLOC (65536, short);
// OBSOLETE   if (trace_data.addrs == NULL)
// OBSOLETE     trace_data.addrs = XCALLOC (65536, CORE_ADDR);
// OBSOLETE 
// OBSOLETE   tracing = 1;
// OBSOLETE 
// OBSOLETE   printf_filtered ("Tracing is now on.\n");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE untrace_command (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   tracing = 0;
// OBSOLETE 
// OBSOLETE   printf_filtered ("Tracing is now off.\n");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE trace_info (char *args, int from_tty)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   if (trace_data.size)
// OBSOLETE     {
// OBSOLETE       printf_filtered ("%d entries in trace buffer:\n", trace_data.size);
// OBSOLETE 
// OBSOLETE       for (i = 0; i < trace_data.size; ++i)
// OBSOLETE 	{
// OBSOLETE 	  printf_filtered ("%d: %d instruction%s at 0x%s\n",
// OBSOLETE 			   i,
// OBSOLETE 			   trace_data.counts[i],
// OBSOLETE 			   (trace_data.counts[i] == 1 ? "" : "s"),
// OBSOLETE 			   paddr_nz (trace_data.addrs[i]));
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     printf_filtered ("No entries in trace buffer.\n");
// OBSOLETE 
// OBSOLETE   printf_filtered ("Tracing is currently %s.\n", (tracing ? "on" : "off"));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE d10v_eva_prepare_to_trace (void)
// OBSOLETE {
// OBSOLETE   if (!tracing)
// OBSOLETE     return;
// OBSOLETE 
// OBSOLETE   last_pc = read_register (D10V_PC_REGNUM);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Collect trace data from the target board and format it into a form
// OBSOLETE    more useful for display.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE d10v_eva_get_trace_data (void)
// OBSOLETE {
// OBSOLETE   int count, i, j, oldsize;
// OBSOLETE   int trace_addr, trace_seg, trace_cnt, next_cnt;
// OBSOLETE   unsigned int last_trace, trace_word, next_word;
// OBSOLETE   unsigned int *tmpspace;
// OBSOLETE 
// OBSOLETE   if (!tracing)
// OBSOLETE     return;
// OBSOLETE 
// OBSOLETE   tmpspace = xmalloc (65536 * sizeof (unsigned int));
// OBSOLETE 
// OBSOLETE   last_trace = read_memory_unsigned_integer (DBBC_ADDR, 2) << 2;
// OBSOLETE 
// OBSOLETE   /* Collect buffer contents from the target, stopping when we reach
// OBSOLETE      the word recorded when execution resumed.  */
// OBSOLETE 
// OBSOLETE   count = 0;
// OBSOLETE   while (last_trace > 0)
// OBSOLETE     {
// OBSOLETE       QUIT;
// OBSOLETE       trace_word =
// OBSOLETE 	read_memory_unsigned_integer (TRACE_BUFFER_BASE + last_trace, 4);
// OBSOLETE       trace_addr = trace_word & 0xffff;
// OBSOLETE       last_trace -= 4;
// OBSOLETE       /* Ignore an apparently nonsensical entry.  */
// OBSOLETE       if (trace_addr == 0xffd5)
// OBSOLETE 	continue;
// OBSOLETE       tmpspace[count++] = trace_word;
// OBSOLETE       if (trace_addr == last_pc)
// OBSOLETE 	break;
// OBSOLETE       if (count > 65535)
// OBSOLETE 	break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Move the data to the host-side trace buffer, adjusting counts to
// OBSOLETE      include the last instruction executed and transforming the address
// OBSOLETE      into something that GDB likes.  */
// OBSOLETE 
// OBSOLETE   for (i = 0; i < count; ++i)
// OBSOLETE     {
// OBSOLETE       trace_word = tmpspace[i];
// OBSOLETE       next_word = ((i == 0) ? 0 : tmpspace[i - 1]);
// OBSOLETE       trace_addr = trace_word & 0xffff;
// OBSOLETE       next_cnt = (next_word >> 24) & 0xff;
// OBSOLETE       j = trace_data.size + count - i - 1;
// OBSOLETE       trace_data.addrs[j] = (trace_addr << 2) + 0x1000000;
// OBSOLETE       trace_data.counts[j] = next_cnt + 1;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   oldsize = trace_data.size;
// OBSOLETE   trace_data.size += count;
// OBSOLETE 
// OBSOLETE   xfree (tmpspace);
// OBSOLETE 
// OBSOLETE   if (trace_display)
// OBSOLETE     display_trace (oldsize, trace_data.size);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE tdisassemble_command (char *arg, int from_tty)
// OBSOLETE {
// OBSOLETE   int i, count;
// OBSOLETE   CORE_ADDR low, high;
// OBSOLETE 
// OBSOLETE   if (!arg)
// OBSOLETE     {
// OBSOLETE       low = 0;
// OBSOLETE       high = trace_data.size;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     { 
// OBSOLETE       char *space_index = strchr (arg, ' ');
// OBSOLETE       if (space_index == NULL)
// OBSOLETE 	{
// OBSOLETE 	  low = parse_and_eval_address (arg);
// OBSOLETE 	  high = low + 5;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* Two arguments.  */
// OBSOLETE 	  *space_index = '\0';
// OBSOLETE 	  low = parse_and_eval_address (arg);
// OBSOLETE 	  high = parse_and_eval_address (space_index + 1);
// OBSOLETE 	  if (high < low)
// OBSOLETE 	    high = low;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   printf_filtered ("Dump of trace from %s to %s:\n", 
// OBSOLETE 		   paddr_u (low), paddr_u (high));
// OBSOLETE 
// OBSOLETE   display_trace (low, high);
// OBSOLETE 
// OBSOLETE   printf_filtered ("End of trace dump.\n");
// OBSOLETE   gdb_flush (gdb_stdout);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE display_trace (int low, int high)
// OBSOLETE {
// OBSOLETE   int i, count, trace_show_source, first, suppress;
// OBSOLETE   CORE_ADDR next_address;
// OBSOLETE 
// OBSOLETE   trace_show_source = default_trace_show_source;
// OBSOLETE   if (!have_full_symbols () && !have_partial_symbols ())
// OBSOLETE     {
// OBSOLETE       trace_show_source = 0;
// OBSOLETE       printf_filtered ("No symbol table is loaded.  Use the \"file\" command.\n");
// OBSOLETE       printf_filtered ("Trace will not display any source.\n");
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   first = 1;
// OBSOLETE   suppress = 0;
// OBSOLETE   for (i = low; i < high; ++i)
// OBSOLETE     {
// OBSOLETE       next_address = trace_data.addrs[i];
// OBSOLETE       count = trace_data.counts[i];
// OBSOLETE       while (count-- > 0)
// OBSOLETE 	{
// OBSOLETE 	  QUIT;
// OBSOLETE 	  if (trace_show_source)
// OBSOLETE 	    {
// OBSOLETE 	      struct symtab_and_line sal, sal_prev;
// OBSOLETE 
// OBSOLETE 	      sal_prev = find_pc_line (next_address - 4, 0);
// OBSOLETE 	      sal = find_pc_line (next_address, 0);
// OBSOLETE 
// OBSOLETE 	      if (sal.symtab)
// OBSOLETE 		{
// OBSOLETE 		  if (first || sal.line != sal_prev.line)
// OBSOLETE 		    print_source_lines (sal.symtab, sal.line, sal.line + 1, 0);
// OBSOLETE 		  suppress = 0;
// OBSOLETE 		}
// OBSOLETE 	      else
// OBSOLETE 		{
// OBSOLETE 		  if (!suppress)
// OBSOLETE 		    /* FIXME-32x64--assumes sal.pc fits in long.  */
// OBSOLETE 		    printf_filtered ("No source file for address %s.\n",
// OBSOLETE 				     hex_string ((unsigned long) sal.pc));
// OBSOLETE 		  suppress = 1;
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	  first = 0;
// OBSOLETE 	  print_address (next_address, gdb_stdout);
// OBSOLETE 	  printf_filtered (":");
// OBSOLETE 	  printf_filtered ("\t");
// OBSOLETE 	  wrap_here ("    ");
// OBSOLETE 	  next_address += gdb_print_insn (next_address, gdb_stdout);
// OBSOLETE 	  printf_filtered ("\n");
// OBSOLETE 	  gdb_flush (gdb_stdout);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
// OBSOLETE {
// OBSOLETE   ULONGEST pc;
// OBSOLETE   frame_unwind_unsigned_register (next_frame, D10V_PC_REGNUM, &pc);
// OBSOLETE   return d10v_make_iaddr (pc);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Given a GDB frame, determine the address of the calling function's
// OBSOLETE    frame.  This will be used to create a new GDB frame struct.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE d10v_frame_this_id (struct frame_info *next_frame,
// OBSOLETE 		    void **this_prologue_cache,
// OBSOLETE 		    struct frame_id *this_id)
// OBSOLETE {
// OBSOLETE   struct d10v_unwind_cache *info
// OBSOLETE     = d10v_frame_unwind_cache (next_frame, this_prologue_cache);
// OBSOLETE   CORE_ADDR base;
// OBSOLETE   CORE_ADDR func;
// OBSOLETE   struct frame_id id;
// OBSOLETE 
// OBSOLETE   /* The FUNC is easy.  */
// OBSOLETE   func = frame_func_unwind (next_frame);
// OBSOLETE 
// OBSOLETE   /* Hopefully the prologue analysis either correctly determined the
// OBSOLETE      frame's base (which is the SP from the previous frame), or set
// OBSOLETE      that base to "NULL".  */
// OBSOLETE   base = info->prev_sp;
// OBSOLETE   if (base == STACK_START || base == 0)
// OBSOLETE     return;
// OBSOLETE 
// OBSOLETE   id = frame_id_build (base, func);
// OBSOLETE 
// OBSOLETE   (*this_id) = id;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE d10v_frame_prev_register (struct frame_info *next_frame,
// OBSOLETE 			  void **this_prologue_cache,
// OBSOLETE 			  int regnum, int *optimizedp,
// OBSOLETE 			  enum lval_type *lvalp, CORE_ADDR *addrp,
// OBSOLETE 			  int *realnump, void *bufferp)
// OBSOLETE {
// OBSOLETE   struct d10v_unwind_cache *info
// OBSOLETE     = d10v_frame_unwind_cache (next_frame, this_prologue_cache);
// OBSOLETE   trad_frame_get_prev_register (next_frame, info->saved_regs, regnum,
// OBSOLETE 				optimizedp, lvalp, addrp, realnump, bufferp);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static const struct frame_unwind d10v_frame_unwind = {
// OBSOLETE   NORMAL_FRAME,
// OBSOLETE   d10v_frame_this_id,
// OBSOLETE   d10v_frame_prev_register
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static const struct frame_unwind *
// OBSOLETE d10v_frame_sniffer (struct frame_info *next_frame)
// OBSOLETE {
// OBSOLETE   return &d10v_frame_unwind;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static CORE_ADDR
// OBSOLETE d10v_frame_base_address (struct frame_info *next_frame, void **this_cache)
// OBSOLETE {
// OBSOLETE   struct d10v_unwind_cache *info
// OBSOLETE     = d10v_frame_unwind_cache (next_frame, this_cache);
// OBSOLETE   return info->base;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static const struct frame_base d10v_frame_base = {
// OBSOLETE   &d10v_frame_unwind,
// OBSOLETE   d10v_frame_base_address,
// OBSOLETE   d10v_frame_base_address,
// OBSOLETE   d10v_frame_base_address
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Assuming NEXT_FRAME->prev is a dummy, return the frame ID of that
// OBSOLETE    dummy frame.  The frame ID's base needs to match the TOS value
// OBSOLETE    saved by save_dummy_frame_tos(), and the PC match the dummy frame's
// OBSOLETE    breakpoint.  */
// OBSOLETE 
// OBSOLETE static struct frame_id
// OBSOLETE d10v_unwind_dummy_id (struct gdbarch *gdbarch, struct frame_info *next_frame)
// OBSOLETE {
// OBSOLETE   return frame_id_build (d10v_unwind_sp (gdbarch, next_frame),
// OBSOLETE 			 frame_pc_unwind (next_frame));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static gdbarch_init_ftype d10v_gdbarch_init;
// OBSOLETE 
// OBSOLETE static struct gdbarch *
// OBSOLETE d10v_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
// OBSOLETE {
// OBSOLETE   struct gdbarch *gdbarch;
// OBSOLETE   int d10v_num_regs;
// OBSOLETE   struct gdbarch_tdep *tdep;
// OBSOLETE   gdbarch_register_name_ftype *d10v_register_name;
// OBSOLETE   gdbarch_register_sim_regno_ftype *d10v_register_sim_regno;
// OBSOLETE 
// OBSOLETE   /* Find a candidate among the list of pre-declared architectures.  */
// OBSOLETE   arches = gdbarch_list_lookup_by_info (arches, &info);
// OBSOLETE   if (arches != NULL)
// OBSOLETE     return arches->gdbarch;
// OBSOLETE 
// OBSOLETE   /* None found, create a new architecture from the information
// OBSOLETE      provided.  */
// OBSOLETE   tdep = XMALLOC (struct gdbarch_tdep);
// OBSOLETE   gdbarch = gdbarch_alloc (&info, tdep);
// OBSOLETE 
// OBSOLETE   switch (info.bfd_arch_info->mach)
// OBSOLETE     {
// OBSOLETE     case bfd_mach_d10v_ts2:
// OBSOLETE       d10v_num_regs = 37;
// OBSOLETE       d10v_register_name = d10v_ts2_register_name;
// OBSOLETE       d10v_register_sim_regno = d10v_ts2_register_sim_regno;
// OBSOLETE       tdep->a0_regnum = TS2_A0_REGNUM;
// OBSOLETE       tdep->nr_dmap_regs = TS2_NR_DMAP_REGS;
// OBSOLETE       tdep->dmap_register = d10v_ts2_dmap_register;
// OBSOLETE       tdep->imap_register = d10v_ts2_imap_register;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE     case bfd_mach_d10v_ts3:
// OBSOLETE       d10v_num_regs = 42;
// OBSOLETE       d10v_register_name = d10v_ts3_register_name;
// OBSOLETE       d10v_register_sim_regno = d10v_ts3_register_sim_regno;
// OBSOLETE       tdep->a0_regnum = TS3_A0_REGNUM;
// OBSOLETE       tdep->nr_dmap_regs = TS3_NR_DMAP_REGS;
// OBSOLETE       tdep->dmap_register = d10v_ts3_dmap_register;
// OBSOLETE       tdep->imap_register = d10v_ts3_imap_register;
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   set_gdbarch_read_pc (gdbarch, d10v_read_pc);
// OBSOLETE   set_gdbarch_write_pc (gdbarch, d10v_write_pc);
// OBSOLETE   set_gdbarch_unwind_sp (gdbarch, d10v_unwind_sp);
// OBSOLETE 
// OBSOLETE   set_gdbarch_num_regs (gdbarch, d10v_num_regs);
// OBSOLETE   set_gdbarch_sp_regnum (gdbarch, D10V_SP_REGNUM);
// OBSOLETE   set_gdbarch_register_name (gdbarch, d10v_register_name);
// OBSOLETE   set_gdbarch_register_type (gdbarch, d10v_register_type);
// OBSOLETE 
// OBSOLETE   set_gdbarch_ptr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
// OBSOLETE   set_gdbarch_addr_bit (gdbarch, 32);
// OBSOLETE   set_gdbarch_address_to_pointer (gdbarch, d10v_address_to_pointer);
// OBSOLETE   set_gdbarch_pointer_to_address (gdbarch, d10v_pointer_to_address);
// OBSOLETE   set_gdbarch_integer_to_address (gdbarch, d10v_integer_to_address);
// OBSOLETE   set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
// OBSOLETE   set_gdbarch_int_bit (gdbarch, 2 * TARGET_CHAR_BIT);
// OBSOLETE   set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
// OBSOLETE   set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);
// OBSOLETE   /* NOTE: The d10v as a 32 bit ``float'' and ``double''. ``long
// OBSOLETE      double'' is 64 bits.  */
// OBSOLETE   set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
// OBSOLETE   set_gdbarch_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);
// OBSOLETE   set_gdbarch_long_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);
// OBSOLETE   switch (info.byte_order)
// OBSOLETE     {
// OBSOLETE     case BFD_ENDIAN_BIG:
// OBSOLETE       set_gdbarch_float_format (gdbarch, &floatformat_ieee_single_big);
// OBSOLETE       set_gdbarch_double_format (gdbarch, &floatformat_ieee_single_big);
// OBSOLETE       set_gdbarch_long_double_format (gdbarch, &floatformat_ieee_double_big);
// OBSOLETE       break;
// OBSOLETE     case BFD_ENDIAN_LITTLE:
// OBSOLETE       set_gdbarch_float_format (gdbarch, &floatformat_ieee_single_little);
// OBSOLETE       set_gdbarch_double_format (gdbarch, &floatformat_ieee_single_little);
// OBSOLETE       set_gdbarch_long_double_format (gdbarch, 
// OBSOLETE 				      &floatformat_ieee_double_little);
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       internal_error (__FILE__, __LINE__,
// OBSOLETE 		      "d10v_gdbarch_init: bad byte order for float format");
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   set_gdbarch_return_value (gdbarch, d10v_return_value);
// OBSOLETE   set_gdbarch_push_dummy_code (gdbarch, d10v_push_dummy_code);
// OBSOLETE   set_gdbarch_push_dummy_call (gdbarch, d10v_push_dummy_call);
// OBSOLETE 
// OBSOLETE   set_gdbarch_skip_prologue (gdbarch, d10v_skip_prologue);
// OBSOLETE   set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
// OBSOLETE   set_gdbarch_decr_pc_after_break (gdbarch, 4);
// OBSOLETE   set_gdbarch_breakpoint_from_pc (gdbarch, d10v_breakpoint_from_pc);
// OBSOLETE 
// OBSOLETE   set_gdbarch_remote_translate_xfer_address (gdbarch, 
// OBSOLETE 					     remote_d10v_translate_xfer_address);
// OBSOLETE 
// OBSOLETE   set_gdbarch_frame_align (gdbarch, d10v_frame_align);
// OBSOLETE 
// OBSOLETE   set_gdbarch_register_sim_regno (gdbarch, d10v_register_sim_regno);
// OBSOLETE 
// OBSOLETE   set_gdbarch_print_registers_info (gdbarch, d10v_print_registers_info);
// OBSOLETE 
// OBSOLETE   frame_unwind_append_sniffer (gdbarch, d10v_frame_sniffer);
// OBSOLETE   frame_base_set_default (gdbarch, &d10v_frame_base);
// OBSOLETE 
// OBSOLETE   /* Methods for saving / extracting a dummy frame's ID.  The ID's
// OBSOLETE      stack address must match the SP value returned by
// OBSOLETE      PUSH_DUMMY_CALL, and saved by generic_save_dummy_frame_tos.  */
// OBSOLETE   set_gdbarch_unwind_dummy_id (gdbarch, d10v_unwind_dummy_id);
// OBSOLETE 
// OBSOLETE   /* Return the unwound PC value.  */
// OBSOLETE   set_gdbarch_unwind_pc (gdbarch, d10v_unwind_pc);
// OBSOLETE 
// OBSOLETE   set_gdbarch_print_insn (gdbarch, print_insn_d10v);
// OBSOLETE 
// OBSOLETE   return gdbarch;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_d10v_tdep (void)
// OBSOLETE {
// OBSOLETE   register_gdbarch_init (bfd_arch_d10v, d10v_gdbarch_init);
// OBSOLETE 
// OBSOLETE   deprecated_target_resume_hook = d10v_eva_prepare_to_trace;
// OBSOLETE   deprecated_target_wait_loop_hook = d10v_eva_get_trace_data;
// OBSOLETE 
// OBSOLETE   deprecate_cmd (add_com ("regs", class_vars, show_regs, 
// OBSOLETE 			  "Print all registers"),
// OBSOLETE 		 "info registers");
// OBSOLETE 
// OBSOLETE   add_com ("itrace", class_support, trace_command,
// OBSOLETE 	   "Enable tracing of instruction execution.");
// OBSOLETE 
// OBSOLETE   add_com ("iuntrace", class_support, untrace_command,
// OBSOLETE 	   "Disable tracing of instruction execution.");
// OBSOLETE 
// OBSOLETE   add_com ("itdisassemble", class_vars, tdisassemble_command,
// OBSOLETE 	   "Disassemble the trace buffer.\n\
// OBSOLETE Two optional arguments specify a range of trace buffer entries\n\
// OBSOLETE as reported by info trace (NOT addresses!).");
// OBSOLETE 
// OBSOLETE   add_info ("itrace", trace_info,
// OBSOLETE 	    "Display info about the trace data buffer.");
// OBSOLETE 
// OBSOLETE   add_setshow_boolean_cmd ("itracedisplay", no_class, &trace_display, "\
// OBSOLETE Set automatic display of trace.", "\
// OBSOLETE Show automatic display of trace.", "\
// OBSOLETE Controls the display of d10v specific instruction trace information.", "\
// OBSOLETE Automatic display of trace is %s.",
// OBSOLETE 			   NULL, NULL, &setlist, &showlist);
// OBSOLETE   add_setshow_boolean_cmd ("itracesource", no_class,
// OBSOLETE 			   &default_trace_show_source, "\
// OBSOLETE Set display of source code with trace.", "\
// OBSOLETE Show display of source code with trace.", "\
// OBSOLETE When on source code is included in the d10v instruction trace display.", "\
// OBSOLETE Display of source code with trace is %s.",
// OBSOLETE 			   NULL, NULL, &setlist, &showlist);
// OBSOLETE }
