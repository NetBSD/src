/* Target-dependent code for Morpho ms1 processor, for GDB.

   Copyright 2005 Free Software Foundation, Inc.

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

/* Contributed by Michael Snyder, msnyder@redhat.com.  */

#include "defs.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "symtab.h"
#include "dis-asm.h"
#include "arch-utils.h"
#include "gdbtypes.h"
#include "gdb_string.h"
#include "regcache.h"
#include "reggroups.h"
#include "gdbcore.h"
#include "trad-frame.h"
#include "inferior.h"
#include "dwarf2-frame.h"
#include "infcall.h"
#include "gdb_assert.h"

enum ms1_arch_constants
{
  MS1_MAX_STRUCT_SIZE = 16
};

enum ms1_gdb_regnums
{
  MS1_R0_REGNUM,			/* 32 bit regs.  */
  MS1_R1_REGNUM,
  MS1_1ST_ARGREG = MS1_R1_REGNUM,
  MS1_R2_REGNUM,
  MS1_R3_REGNUM,
  MS1_R4_REGNUM,
  MS1_LAST_ARGREG = MS1_R4_REGNUM,
  MS1_R5_REGNUM,
  MS1_R6_REGNUM,
  MS1_R7_REGNUM,
  MS1_R8_REGNUM,
  MS1_R9_REGNUM,
  MS1_R10_REGNUM,
  MS1_R11_REGNUM,
  MS1_R12_REGNUM,
  MS1_FP_REGNUM = MS1_R12_REGNUM,
  MS1_R13_REGNUM,
  MS1_SP_REGNUM = MS1_R13_REGNUM,
  MS1_R14_REGNUM,
  MS1_RA_REGNUM = MS1_R14_REGNUM,
  MS1_R15_REGNUM,
  MS1_IRA_REGNUM = MS1_R15_REGNUM,
  MS1_PC_REGNUM,

  /* Interrupt Enable pseudo-register, exported by SID.  */
  MS1_INT_ENABLE_REGNUM,
  /* End of CPU regs.  */

  MS1_NUM_CPU_REGS,

  /* Co-processor registers.  */
  MS1_COPRO_REGNUM = MS1_NUM_CPU_REGS,	/* 16 bit regs.  */
  MS1_CPR0_REGNUM,
  MS1_CPR1_REGNUM,
  MS1_CPR2_REGNUM,
  MS1_CPR3_REGNUM,
  MS1_CPR4_REGNUM,
  MS1_CPR5_REGNUM,
  MS1_CPR6_REGNUM,
  MS1_CPR7_REGNUM,
  MS1_CPR8_REGNUM,
  MS1_CPR9_REGNUM,
  MS1_CPR10_REGNUM,
  MS1_CPR11_REGNUM,
  MS1_CPR12_REGNUM,
  MS1_CPR13_REGNUM,
  MS1_CPR14_REGNUM,
  MS1_CPR15_REGNUM,
  MS1_BYPA_REGNUM,		/* 32 bit regs.  */
  MS1_BYPB_REGNUM,
  MS1_BYPC_REGNUM,
  MS1_FLAG_REGNUM,
  MS1_CONTEXT_REGNUM,		/* 38 bits (treat as array of
				   six bytes).  */
  MS1_MAC_REGNUM,			/* 32 bits.  */
  MS1_Z1_REGNUM,			/* 16 bits.  */
  MS1_Z2_REGNUM,			/* 16 bits.  */
  MS1_ICHANNEL_REGNUM,		/* 32 bits.  */
  MS1_ISCRAMB_REGNUM,		/* 32 bits.  */
  MS1_QSCRAMB_REGNUM,		/* 32 bits.  */
  MS1_OUT_REGNUM,			/* 16 bits.  */
  MS1_EXMAC_REGNUM,		/* 32 bits (8 used).  */
  MS1_QCHANNEL_REGNUM,		/* 32 bits.  */

  /* Number of real registers.  */
  MS1_NUM_REGS,

  /* Pseudo-registers.  */
  MS1_COPRO_PSEUDOREG_REGNUM = MS1_NUM_REGS,
  MS1_MAC_PSEUDOREG_REGNUM,

  /* Two pseudo-regs ('coprocessor' and 'mac').  */
  MS1_NUM_PSEUDO_REGS = 2
};

/* Return name of register number specified by REGNUM.  */

static const char *
ms1_register_name (int regnum)
{
  static char *register_names[] = {
    /* CPU regs.  */
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "pc", "IE",
    /* Co-processor regs.  */
    "",				/* copro register.  */
    "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7",
    "cr8", "cr9", "cr10", "cr11", "cr12", "cr13", "cr14", "cr15",
    "bypa", "bypb", "bypc", "flag", "context", "" /* mac.  */ , "z1", "z2",
    "Ichannel", "Iscramb", "Qscramb", "out", "" /* ex-mac.  */ , "Qchannel",
    /* Pseudo-registers.  */
    "coprocessor", "MAC"
  };

  gdb_assert (regnum >= 0 && regnum < ARRAY_SIZE (register_names));
  return register_names[regnum];
}

/* Given ARCH and a register number specified by REGNUM, return the
   type of that register.  */

static struct type *
ms1_register_type (struct gdbarch *arch, int regnum)
{
  static struct type *void_func_ptr = NULL;
  static struct type *void_ptr = NULL;
  static struct type *copro_type;

  if (regnum >= 0 && regnum < MS1_NUM_REGS + MS1_NUM_PSEUDO_REGS)
    {
      if (void_func_ptr == NULL)
	{
	  struct type *temp;

	  void_ptr = lookup_pointer_type (builtin_type_void);
	  void_func_ptr =
	    lookup_pointer_type (lookup_function_type (builtin_type_void));
	  temp = create_range_type (NULL, builtin_type_unsigned_int, 0, 1);
	  copro_type = create_array_type (NULL, builtin_type_int16, temp);
	}
      switch (regnum)
	{
	case MS1_PC_REGNUM:
	case MS1_RA_REGNUM:
	case MS1_IRA_REGNUM:
	  return void_func_ptr;
	case MS1_SP_REGNUM:
	case MS1_FP_REGNUM:
	  return void_ptr;
	case MS1_INT_ENABLE_REGNUM:
	case MS1_ICHANNEL_REGNUM:
	case MS1_QCHANNEL_REGNUM:
	case MS1_ISCRAMB_REGNUM:
	case MS1_QSCRAMB_REGNUM:
	  return builtin_type_int32;
	case MS1_EXMAC_REGNUM:
	case MS1_MAC_REGNUM:
	  return builtin_type_uint32;
	case MS1_BYPA_REGNUM:
	case MS1_BYPB_REGNUM:
	case MS1_BYPC_REGNUM:
	case MS1_Z1_REGNUM:
	case MS1_Z2_REGNUM:
	case MS1_OUT_REGNUM:
	  return builtin_type_int16;
	case MS1_CONTEXT_REGNUM:
	  return builtin_type_long_long;
	case MS1_COPRO_REGNUM:
	case MS1_COPRO_PSEUDOREG_REGNUM:
	  return copro_type;
	case MS1_MAC_PSEUDOREG_REGNUM:
	  if (gdbarch_bfd_arch_info (arch)->mach == bfd_mach_mrisc2)
	    return builtin_type_uint64;
	  else
	    return builtin_type_uint32;
	case MS1_FLAG_REGNUM:
	  return builtin_type_unsigned_char;
	default:
	  if (regnum >= MS1_R0_REGNUM && regnum <= MS1_R15_REGNUM)
	    return builtin_type_int32;
	  else if (regnum >= MS1_CPR0_REGNUM && regnum <= MS1_CPR15_REGNUM)
	    return builtin_type_int16;
	}
    }
  internal_error (__FILE__, __LINE__,
		  _("ms1_register_type: illegal register number %d"), regnum);
}

/* Return true if register REGNUM is a member of the register group
   specified by GROUP.  */

static int
ms1_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			 struct reggroup *group)
{
  /* Groups of registers that can be displayed via "info reg".  */
  if (group == all_reggroup)
    return (regnum >= 0
	    && regnum < MS1_NUM_REGS + MS1_NUM_PSEUDO_REGS
	    && ms1_register_name (regnum)[0] != '\0');

  if (group == general_reggroup)
    return (regnum >= MS1_R0_REGNUM && regnum <= MS1_R15_REGNUM);

  if (group == float_reggroup)
    return 0;			/* No float regs.  */

  if (group == vector_reggroup)
    return 0;			/* No vector regs.  */

  /* For any that are not handled above.  */
  return default_register_reggroup_p (gdbarch, regnum, group);
}

/* Return the return value convention used for a given type TYPE.
   Optionally, fetch or set the return value via READBUF or
   WRITEBUF respectively using REGCACHE for the register
   values.  */

static enum return_value_convention
ms1_return_value (struct gdbarch *gdbarch, struct type *type,
		  struct regcache *regcache, gdb_byte *readbuf,
		  const gdb_byte *writebuf)
{
  if (TYPE_LENGTH (type) > 4)
    {
      /* Return values > 4 bytes are returned in memory, 
         pointed to by R11.  */
      if (readbuf)
	{
	  ULONGEST addr;

	  regcache_cooked_read_unsigned (regcache, MS1_R11_REGNUM, &addr);
	  read_memory (addr, readbuf, TYPE_LENGTH (type));
	}

      if (writebuf)
	{
	  ULONGEST addr;

	  regcache_cooked_read_unsigned (regcache, MS1_R11_REGNUM, &addr);
	  write_memory (addr, writebuf, TYPE_LENGTH (type));
	}

      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }
  else
    {
      if (readbuf)
	{
	  ULONGEST temp;

	  /* Return values of <= 4 bytes are returned in R11.  */
	  regcache_cooked_read_unsigned (regcache, MS1_R11_REGNUM, &temp);
	  store_unsigned_integer (readbuf, TYPE_LENGTH (type), temp);
	}

      if (writebuf)
	{
	  if (TYPE_LENGTH (type) < 4)
	    {
	      gdb_byte buf[4];
	      /* Add leading zeros to the value.  */
	      memset (buf, 0, sizeof (buf));
	      memcpy (buf + sizeof (buf) - TYPE_LENGTH (type),
		      writebuf, TYPE_LENGTH (type));
	      regcache_cooked_write (regcache, MS1_R11_REGNUM, buf);
	    }
	  else			/* (TYPE_LENGTH (type) == 4 */
	    regcache_cooked_write (regcache, MS1_R11_REGNUM, writebuf);
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* If the input address, PC, is in a function prologue, return the
   address of the end of the prologue, otherwise return the input
   address.

   Note:  PC is likely to be the function start, since this function
   is mainly used for advancing a breakpoint to the first line, or
   stepping to the first line when we have stepped into a function
   call.  */

static CORE_ADDR
ms1_skip_prologue (CORE_ADDR pc)
{
  CORE_ADDR func_addr = 0, func_end = 0;
  char *func_name;
  unsigned long instr;

  if (find_pc_partial_function (pc, &func_name, &func_addr, &func_end))
    {
      struct symtab_and_line sal;
      struct symbol *sym;

      /* Found a function.  */
      sym = lookup_symbol (func_name, NULL, VAR_DOMAIN, NULL, NULL);
      if (sym && SYMBOL_LANGUAGE (sym) != language_asm)
	{
	  /* Don't use this trick for assembly source files.  */
	  sal = find_pc_line (func_addr, 0);

	  if (sal.end && sal.end < func_end)
	    {
	      /* Found a line number, use it as end of prologue.  */
	      return sal.end;
	    }
	}
    }

  /* No function symbol, or no line symbol.  Use prologue scanning method.  */
  for (;; pc += 4)
    {
      instr = read_memory_unsigned_integer (pc, 4);
      if (instr == 0x12000000)	/* nop */
	continue;
      if (instr == 0x12ddc000)	/* copy sp into fp */
	continue;
      instr >>= 16;
      if (instr == 0x05dd)	/* subi sp, sp, imm */
	continue;
      if (instr >= 0x43c0 && instr <= 0x43df)	/* push */
	continue;
      /* Not an obvious prologue instruction.  */
      break;
    }

  return pc;
}

/* The breakpoint instruction must be the same size as the smallest
   instruction in the instruction set.

   The BP for ms1 is defined as 0x68000000.  */

static const gdb_byte *
ms1_breakpoint_from_pc (CORE_ADDR *bp_addr, int *bp_size)
{
  static gdb_byte breakpoint[] = { 0x68, 0, 0, 0 };

  *bp_size = 4;
  return breakpoint;
}

/* Fetch the pseudo registers:

   There are two pseudo-registers:
   1) The 'coprocessor' pseudo-register (which mirrors the 
   "real" coprocessor register sent by the target), and
   2) The 'MAC' pseudo-register (which represents the union
   of the original 32 bit target MAC register and the new
   8-bit extended-MAC register).  */

static void
ms1_pseudo_register_read (struct gdbarch *gdbarch,
			  struct regcache *regcache, int regno, gdb_byte *buf)
{
  switch (regno)
    {
    case MS1_COPRO_REGNUM:
    case MS1_COPRO_PSEUDOREG_REGNUM:
      regcache_raw_read (regcache, MS1_COPRO_REGNUM, buf);
      break;
    case MS1_MAC_REGNUM:
    case MS1_MAC_PSEUDOREG_REGNUM:
      if (gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_mrisc2)
	{
	  ULONGEST oldmac = 0, ext_mac = 0;
	  ULONGEST newmac;

	  regcache_cooked_read_unsigned (regcache, MS1_MAC_REGNUM, &oldmac);
	  regcache_cooked_read_unsigned (regcache, MS1_EXMAC_REGNUM, &ext_mac);
	  newmac =
	    (oldmac & 0xffffffff) | ((long long) (ext_mac & 0xff) << 32);
	  store_signed_integer (buf, 8, newmac);
	}
      else
	regcache_raw_read (regcache, MS1_MAC_REGNUM, buf);
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("ms1_pseudo_register_read: bad reg # (%d)"), regno);
      break;
    }
}

/* Write the pseudo registers:

   Ms1 pseudo-registers are stored directly to the target.  The
   'coprocessor' register is special, because when it is modified, all
   the other coprocessor regs must be flushed from the reg cache.  */

static void
ms1_pseudo_register_write (struct gdbarch *gdbarch,
			   struct regcache *regcache,
			   int regno, const gdb_byte *buf)
{
  int i;

  switch (regno)
    {
    case MS1_COPRO_REGNUM:
    case MS1_COPRO_PSEUDOREG_REGNUM:
      regcache_raw_write (regcache, MS1_COPRO_REGNUM, buf);
      for (i = MS1_NUM_CPU_REGS; i < MS1_NUM_REGS; i++)
	set_register_cached (i, 0);
      break;
    case MS1_MAC_REGNUM:
    case MS1_MAC_PSEUDOREG_REGNUM:
      if (gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_mrisc2)
	{
	  /* The 8-byte MAC pseudo-register must be broken down into two
	     32-byte registers.  */
	  unsigned int oldmac, ext_mac;
	  ULONGEST newmac;

	  newmac = extract_unsigned_integer (buf, 8);
	  oldmac = newmac & 0xffffffff;
	  ext_mac = (newmac >> 32) & 0xff;
	  regcache_cooked_write_unsigned (regcache, MS1_MAC_REGNUM, oldmac);
	  regcache_cooked_write_unsigned (regcache, MS1_EXMAC_REGNUM, ext_mac);
	}
      else
	regcache_raw_write (regcache, MS1_MAC_REGNUM, buf);
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("ms1_pseudo_register_write: bad reg # (%d)"), regno);
      break;
    }
}

static CORE_ADDR
ms1_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Register size is 4 bytes.  */
  return align_down (sp, 4);
}

/* Implements the "info registers" command.   When ``all'' is non-zero,
   the coprocessor registers will be printed in addition to the rest
   of the registers.  */

static void
ms1_registers_info (struct gdbarch *gdbarch,
		    struct ui_file *file,
		    struct frame_info *frame, int regnum, int all)
{
  if (regnum == -1)
    {
      int lim;

      lim = all ? MS1_NUM_REGS : MS1_NUM_CPU_REGS;

      for (regnum = 0; regnum < lim; regnum++)
	{
	  /* Don't display the Qchannel register since it will be displayed
	     along with Ichannel.  (See below.)  */
	  if (regnum == MS1_QCHANNEL_REGNUM)
	    continue;

	  ms1_registers_info (gdbarch, file, frame, regnum, all);

	  /* Display the Qchannel register immediately after Ichannel.  */
	  if (regnum == MS1_ICHANNEL_REGNUM)
	    ms1_registers_info (gdbarch, file, frame, MS1_QCHANNEL_REGNUM, all);
	}
    }
  else
    {
      if (regnum == MS1_EXMAC_REGNUM)
	return;
      else if (regnum == MS1_CONTEXT_REGNUM)
	{
	  /* Special output handling for 38-bit context register.  */
	  unsigned char *buff;
	  unsigned int *bytes, i, regsize;

	  regsize = register_size (gdbarch, regnum);

	  buff = alloca (regsize);
	  bytes = alloca (regsize * sizeof (*bytes));

	  frame_register_read (frame, regnum, buff);

	  fputs_filtered (REGISTER_NAME (regnum), file);
	  print_spaces_filtered (15 - strlen (REGISTER_NAME (regnum)), file);
	  fputs_filtered ("0x", file);

	  for (i = 0; i < regsize; i++)
	    fprintf_filtered (file, "%02x", (unsigned int)
			      extract_unsigned_integer (buff + i, 1));
	  fputs_filtered ("\t", file);
	  print_longest (file, 'd', 0,
			 extract_unsigned_integer (buff, regsize));
	  fputs_filtered ("\n", file);
	}
      else if (regnum == MS1_COPRO_REGNUM
               || regnum == MS1_COPRO_PSEUDOREG_REGNUM)
	{
	  /* Special output handling for the 'coprocessor' register.  */
	  char *buf;

	  buf = alloca (register_size (gdbarch, MS1_COPRO_REGNUM));
	  frame_register_read (frame, MS1_COPRO_REGNUM, buf);
	  /* And print.  */
	  regnum = MS1_COPRO_PSEUDOREG_REGNUM;
	  fputs_filtered (REGISTER_NAME (regnum), file);
	  print_spaces_filtered (15 - strlen (REGISTER_NAME (regnum)), file);
	  val_print (register_type (gdbarch, regnum), buf,
		     0, 0, file, 0, 1, 0, Val_no_prettyprint);
	  fputs_filtered ("\n", file);
	}
      else if (regnum == MS1_MAC_REGNUM || regnum == MS1_MAC_PSEUDOREG_REGNUM)
	{
	  ULONGEST oldmac, ext_mac, newmac;
	  char buf[3 * sizeof (LONGEST)];

	  /* Get the two "real" mac registers.  */
	  frame_register_read (frame, MS1_MAC_REGNUM, buf);
	  oldmac = extract_unsigned_integer (buf,
					     register_size (gdbarch,
							    MS1_MAC_REGNUM));
	  if (gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_mrisc2)
	    {
	      frame_register_read (frame, MS1_EXMAC_REGNUM, buf);
	      ext_mac = extract_unsigned_integer (buf,
						  register_size (gdbarch,
								 MS1_EXMAC_REGNUM));
	    }
	  else
	    ext_mac = 0;

	  /* Add them together.  */
	  newmac = (oldmac & 0xffffffff) + ((ext_mac & 0xff) << 32);

	  /* And print.  */
	  regnum = MS1_MAC_PSEUDOREG_REGNUM;
	  fputs_filtered (REGISTER_NAME (regnum), file);
	  print_spaces_filtered (15 - strlen (REGISTER_NAME (regnum)), file);
	  fputs_filtered ("0x", file);
	  print_longest (file, 'x', 0, newmac);
	  fputs_filtered ("\t", file);
	  print_longest (file, 'u', 0, newmac);
	  fputs_filtered ("\n", file);
	}
      else
	default_print_registers_info (gdbarch, file, frame, regnum, all);
    }
}

/* Set up the callee's arguments for an inferior function call.  The
   arguments are pushed on the stack or are placed in registers as
   appropriate.  It also sets up the return address (which points to
   the call dummy breakpoint).

   Returns the updated (and aligned) stack pointer.  */

static CORE_ADDR
ms1_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		     struct regcache *regcache, CORE_ADDR bp_addr,
		     int nargs, struct value **args, CORE_ADDR sp,
		     int struct_return, CORE_ADDR struct_addr)
{
#define wordsize 4
  char buf[MS1_MAX_STRUCT_SIZE];
  int argreg = MS1_1ST_ARGREG;
  int split_param_len = 0;
  int stack_dest = sp;
  int slacklen;
  int typelen;
  int i, j;

  /* First handle however many args we can fit into MS1_1ST_ARGREG thru
     MS1_LAST_ARGREG.  */
  for (i = 0; i < nargs && argreg <= MS1_LAST_ARGREG; i++)
    {
      const char *val;
      typelen = TYPE_LENGTH (value_type (args[i]));
      switch (typelen)
	{
	case 1:
	case 2:
	case 3:
	case 4:
	  regcache_cooked_write_unsigned (regcache, argreg++,
					  extract_unsigned_integer
					  (value_contents (args[i]),
					   wordsize));
	  break;
	case 8:
	case 12:
	case 16:
	  val = value_contents (args[i]);
	  while (typelen > 0)
	    {
	      if (argreg <= MS1_LAST_ARGREG)
		{
		  /* This word of the argument is passed in a register.  */
		  regcache_cooked_write_unsigned (regcache, argreg++,
						  extract_unsigned_integer
						  (val, wordsize));
		  typelen -= wordsize;
		  val += wordsize;
		}
	      else
		{
		  /* Remainder of this arg must be passed on the stack
		     (deferred to do later).  */
		  split_param_len = typelen;
		  memcpy (buf, val, typelen);
		  break;	/* No more args can be handled in regs.  */
		}
	    }
	  break;
	default:
	  /* By reverse engineering of gcc output, args bigger than
	     16 bytes go on the stack, and their address is passed
	     in the argreg.  */
	  stack_dest -= typelen;
	  write_memory (stack_dest, value_contents (args[i]), typelen);
	  regcache_cooked_write_unsigned (regcache, argreg++, stack_dest);
	  break;
	}
    }

  /* Next, the rest of the arguments go onto the stack, in reverse order.  */
  for (j = nargs - 1; j >= i; j--)
    {
      char *val;
      /* Right-justify the value in an aligned-length buffer.  */
      typelen = TYPE_LENGTH (value_type (args[j]));
      slacklen = (wordsize - (typelen % wordsize)) % wordsize;
      val = alloca (typelen + slacklen);
      memcpy (val, value_contents (args[j]), typelen);
      memset (val + typelen, 0, slacklen);
      /* Now write this data to the stack.  */
      stack_dest -= typelen + slacklen;
      write_memory (stack_dest, val, typelen + slacklen);
    }

  /* Finally, if a param needs to be split between registers and stack, 
     write the second half to the stack now.  */
  if (split_param_len != 0)
    {
      stack_dest -= split_param_len;
      write_memory (stack_dest, buf, split_param_len);
    }

  /* Set up return address (provided to us as bp_addr).  */
  regcache_cooked_write_unsigned (regcache, MS1_RA_REGNUM, bp_addr);

  /* Store struct return address, if given.  */
  if (struct_return && struct_addr != 0)
    regcache_cooked_write_unsigned (regcache, MS1_R11_REGNUM, struct_addr);

  /* Set aside 16 bytes for the callee to save regs 1-4.  */
  stack_dest -= 16;

  /* Update the stack pointer.  */
  regcache_cooked_write_unsigned (regcache, MS1_SP_REGNUM, stack_dest);

  /* And that should do it.  Return the new stack pointer.  */
  return stack_dest;
}


/* The 'unwind_cache' data structure.  */

struct ms1_unwind_cache
{
  /* The previous frame's inner most stack address.  
     Used as this frame ID's stack_addr.  */
  CORE_ADDR prev_sp;
  CORE_ADDR frame_base;
  int framesize;
  int frameless_p;

  /* Table indicating the location of each and every register.  */
  struct trad_frame_saved_reg *saved_regs;
};

/* Initialize an unwind_cache.  Build up the saved_regs table etc. for
   the frame.  */

static struct ms1_unwind_cache *
ms1_frame_unwind_cache (struct frame_info *next_frame,
			void **this_prologue_cache)
{
  struct gdbarch *gdbarch;
  struct ms1_unwind_cache *info;
  CORE_ADDR next_addr, start_addr, end_addr, prologue_end_addr;
  unsigned long instr, upper_half, delayed_store = 0;
  int regnum, offset;
  ULONGEST sp, fp;

  if ((*this_prologue_cache))
    return (*this_prologue_cache);

  gdbarch = get_frame_arch (next_frame);
  info = FRAME_OBSTACK_ZALLOC (struct ms1_unwind_cache);
  (*this_prologue_cache) = info;

  info->prev_sp = 0;
  info->framesize = 0;
  info->frame_base = 0;
  info->frameless_p = 1;
  info->saved_regs = trad_frame_alloc_saved_regs (next_frame);

  /* Grab the frame-relative values of SP and FP, needed below. 
     The frame_saved_register function will find them on the
     stack or in the registers as appropriate.  */
  frame_unwind_unsigned_register (next_frame, MS1_SP_REGNUM, &sp);
  frame_unwind_unsigned_register (next_frame, MS1_FP_REGNUM, &fp);

  start_addr = frame_func_unwind (next_frame);

  /* Return early if GDB couldn't find the function.  */
  if (start_addr == 0)
    return info;

  end_addr = frame_pc_unwind (next_frame);
  prologue_end_addr = skip_prologue_using_sal (start_addr);
  if (end_addr == 0)
  for (next_addr = start_addr; next_addr < end_addr; next_addr += 4)
    {
      instr = get_frame_memory_unsigned (next_frame, next_addr, 4);
      if (delayed_store)	/* previous instr was a push */
	{
	  upper_half = delayed_store >> 16;
	  regnum = upper_half & 0xf;
	  offset = delayed_store & 0xffff;
	  switch (upper_half & 0xfff0)
	    {
	    case 0x43c0:	/* push using frame pointer */
	      info->saved_regs[regnum].addr = offset;
	      break;
	    case 0x43d0:	/* push using stack pointer */
	      info->saved_regs[regnum].addr = offset;
	      break;
	    default:		/* lint */
	      break;
	    }
	  delayed_store = 0;
	}

      switch (instr)
	{
	case 0x12000000:	/* NO-OP */
	  continue;
	case 0x12ddc000:	/* copy sp into fp */
	  info->frameless_p = 0;	/* Record that the frame pointer is in use.  */
	  continue;
	default:
	  upper_half = instr >> 16;
	  if (upper_half == 0x05dd ||	/* subi  sp, sp, imm */
	      upper_half == 0x07dd)	/* subui sp, sp, imm */
	    {
	      /* Record the frame size.  */
	      info->framesize = instr & 0xffff;
	      continue;
	    }
	  if ((upper_half & 0xfff0) == 0x43c0 ||	/* frame push */
	      (upper_half & 0xfff0) == 0x43d0)	/* stack push */
	    {
	      /* Save this instruction, but don't record the 
	         pushed register as 'saved' until we see the
	         next instruction.  That's because of deferred stores
	         on this target -- GDB won't be able to read the register
	         from the stack until one instruction later.  */
	      delayed_store = instr;
	      continue;
	    }
	  /* Not a prologue instruction.  Is this the end of the prologue?
	     This is the most difficult decision; when to stop scanning. 

	     If we have no line symbol, then the best thing we can do
	     is to stop scanning when we encounter an instruction that
	     is not likely to be a part of the prologue. 

	     But if we do have a line symbol, then we should 
	     keep scanning until we reach it (or we reach end_addr).  */

	  if (prologue_end_addr && (prologue_end_addr > (next_addr + 4)))
	    continue;		/* Keep scanning, recording saved_regs etc.  */
	  else
	    break;		/* Quit scanning: breakpoint can be set here.  */
	}
    }

  /* Special handling for the "saved" address of the SP:
     The SP is of course never saved on the stack at all, so
     by convention what we put here is simply the previous 
     _value_ of the SP (as opposed to an address where the
     previous value would have been pushed).  This will also
     give us the frame base address.  */

  if (info->frameless_p)
    {
      info->frame_base = sp + info->framesize;
      info->prev_sp = sp + info->framesize;
    }
  else
    {
      info->frame_base = fp + info->framesize;
      info->prev_sp = fp + info->framesize;
    }
  /* Save prev_sp in saved_regs as a value, not as an address.  */
  trad_frame_set_value (info->saved_regs, MS1_SP_REGNUM, info->prev_sp);

  /* Now convert frame offsets to actual addresses (not offsets).  */
  for (regnum = 0; regnum < MS1_NUM_REGS; regnum++)
    if (trad_frame_addr_p (info->saved_regs, regnum))
      info->saved_regs[regnum].addr += info->frame_base - info->framesize;

  /* The call instruction moves the caller's PC in the callee's RA reg.
     Since this is an unwind, do the reverse.  Copy the location of RA
     into PC (the address / regnum) so that a request for PC will be
     converted into a request for the RA.  */
  info->saved_regs[MS1_PC_REGNUM] = info->saved_regs[MS1_RA_REGNUM];

  return info;
}

static CORE_ADDR
ms1_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  ULONGEST pc;

  frame_unwind_unsigned_register (next_frame, MS1_PC_REGNUM, &pc);
  return pc;
}

static CORE_ADDR
ms1_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  ULONGEST sp;

  frame_unwind_unsigned_register (next_frame, MS1_SP_REGNUM, &sp);
  return sp;
}

/* Assuming NEXT_FRAME->prev is a dummy, return the frame ID of that
   dummy frame.  The frame ID's base needs to match the TOS value
   saved by save_dummy_frame_tos(), and the PC match the dummy frame's
   breakpoint.  */

static struct frame_id
ms1_unwind_dummy_id (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  return frame_id_build (ms1_unwind_sp (gdbarch, next_frame),
			 frame_pc_unwind (next_frame));
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
ms1_frame_this_id (struct frame_info *next_frame,
		   void **this_prologue_cache, struct frame_id *this_id)
{
  struct ms1_unwind_cache *info =
    ms1_frame_unwind_cache (next_frame, this_prologue_cache);

  if (!(info == NULL || info->prev_sp == 0))
    {
      (*this_id) = frame_id_build (info->prev_sp,
				   frame_func_unwind (next_frame));
    }
  return;
}

static void
ms1_frame_prev_register (struct frame_info *next_frame,
			 void **this_prologue_cache,
			 int regnum, int *optimizedp,
			 enum lval_type *lvalp, CORE_ADDR *addrp,
			 int *realnump, gdb_byte *bufferp)
{
  struct ms1_unwind_cache *info =
    ms1_frame_unwind_cache (next_frame, this_prologue_cache);

  trad_frame_get_prev_register (next_frame, info->saved_regs, regnum,
				optimizedp, lvalp, addrp, realnump, bufferp);
}

static CORE_ADDR
ms1_frame_base_address (struct frame_info *next_frame,
			void **this_prologue_cache)
{
  struct ms1_unwind_cache *info =
    ms1_frame_unwind_cache (next_frame, this_prologue_cache);

  return info->frame_base;
}

/* This is a shared interface:  the 'frame_unwind' object is what's
   returned by the 'sniffer' function, and in turn specifies how to
   get a frame's ID and prev_regs.

   This exports the 'prev_register' and 'this_id' methods.  */

static const struct frame_unwind ms1_frame_unwind = {
  NORMAL_FRAME,
  ms1_frame_this_id,
  ms1_frame_prev_register
};

/* The sniffer is a registered function that identifies our family of
   frame unwind functions (this_id and prev_register).  */

static const struct frame_unwind *
ms1_frame_sniffer (struct frame_info *next_frame)
{
  return &ms1_frame_unwind;
}

/* Another shared interface:  the 'frame_base' object specifies how to
   unwind a frame and secure the base addresses for frame objects
   (locals, args).  */

static struct frame_base ms1_frame_base = {
  &ms1_frame_unwind,
  ms1_frame_base_address,
  ms1_frame_base_address,
  ms1_frame_base_address
};

static struct gdbarch *
ms1_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  static void ms1_frame_unwind_init (struct gdbarch *);

  /* Find a candidate among the list of pre-declared architectures.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found, create a new architecture from the information
     provided.  */
  gdbarch = gdbarch_alloc (&info, NULL);

  switch (info.byte_order)
    {
    case BFD_ENDIAN_BIG:
      set_gdbarch_float_format (gdbarch, &floatformat_ieee_single_big);
      set_gdbarch_double_format (gdbarch, &floatformat_ieee_double_big);
      set_gdbarch_long_double_format (gdbarch, &floatformat_ieee_double_big);
      break;
    case BFD_ENDIAN_LITTLE:
      set_gdbarch_float_format (gdbarch, &floatformat_ieee_single_little);
      set_gdbarch_double_format (gdbarch, &floatformat_ieee_double_little);
      set_gdbarch_long_double_format (gdbarch,
				      &floatformat_ieee_double_little);
      break;
    default:
      internal_error (__FILE__, __LINE__,
		      _("ms1_gdbarch_init: bad byte order for float format"));
    }

  set_gdbarch_register_name (gdbarch, ms1_register_name);
  set_gdbarch_num_regs (gdbarch, MS1_NUM_REGS);
  set_gdbarch_num_pseudo_regs (gdbarch, MS1_NUM_PSEUDO_REGS);
  set_gdbarch_pc_regnum (gdbarch, MS1_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, MS1_SP_REGNUM);
  set_gdbarch_pseudo_register_read (gdbarch, ms1_pseudo_register_read);
  set_gdbarch_pseudo_register_write (gdbarch, ms1_pseudo_register_write);
  set_gdbarch_skip_prologue (gdbarch, ms1_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_from_pc (gdbarch, ms1_breakpoint_from_pc);
  set_gdbarch_decr_pc_after_break (gdbarch, 0);
  set_gdbarch_frame_args_skip (gdbarch, 0);
  set_gdbarch_print_insn (gdbarch, print_insn_ms1);
  set_gdbarch_register_type (gdbarch, ms1_register_type);
  set_gdbarch_register_reggroup_p (gdbarch, ms1_register_reggroup_p);

  set_gdbarch_return_value (gdbarch, ms1_return_value);
  set_gdbarch_sp_regnum (gdbarch, MS1_SP_REGNUM);

  set_gdbarch_frame_align (gdbarch, ms1_frame_align);

  set_gdbarch_print_registers_info (gdbarch, ms1_registers_info);

  set_gdbarch_push_dummy_call (gdbarch, ms1_push_dummy_call);

  /* Target builtin data types.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 32);

  /* Register the DWARF 2 sniffer first, and then the traditional prologue
     based sniffer.  */
  frame_unwind_append_sniffer (gdbarch, dwarf2_frame_sniffer);
  frame_unwind_append_sniffer (gdbarch, ms1_frame_sniffer);
  frame_base_set_default (gdbarch, &ms1_frame_base);

  /* Register the 'unwind_pc' method.  */
  set_gdbarch_unwind_pc (gdbarch, ms1_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, ms1_unwind_sp);

  /* Methods for saving / extracting a dummy frame's ID.  
     The ID's stack address must match the SP value returned by
     PUSH_DUMMY_CALL, and saved by generic_save_dummy_frame_tos.  */
  set_gdbarch_unwind_dummy_id (gdbarch, ms1_unwind_dummy_id);

  return gdbarch;
}

void
_initialize_ms1_tdep (void)
{
  register_gdbarch_init (bfd_arch_ms1, ms1_gdbarch_init);
}
