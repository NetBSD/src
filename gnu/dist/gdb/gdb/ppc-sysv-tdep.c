/* Target-dependent code for PowerPC systems using the SVR4 ABI
   for GDB, the GNU debugger.

   Copyright 2000, 2001, 2002 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "value.h"
#include "gdb_string.h"

#include "ppc-tdep.h"

/* round2 rounds x up to the nearest multiple of s assuming that s is a
   power of 2 */

#undef round2
#define round2(x,s) ((((long) (x) - 1) & ~(long)((s)-1)) + (s))

/* Pass the arguments in either registers, or in the stack. Using the
   ppc sysv ABI, the first eight words of the argument list (that might
   be less than eight parameters if some parameters occupy more than one
   word) are passed in r3..r10 registers.  float and double parameters are
   passed in fpr's, in addition to that. Rest of the parameters if any
   are passed in user stack. 

   If the function is returning a structure, then the return address is passed
   in r3, then the first 7 words of the parametes can be passed in registers,
   starting from r4. */

CORE_ADDR
ppc_sysv_abi_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
			     int struct_return, CORE_ADDR struct_addr)
{
  int argno;
  /* Next available general register for non-float, non-vector arguments. */
  int greg;
  /* Next available floating point register for float arguments. */
  int freg;
  /* Next available vector register for vector arguments. */
  int vreg;
  int argstkspace;
  int structstkspace;
  int argoffset;
  int structoffset;
  struct value *arg;
  struct type *type;
  int len;
  char old_sp_buf[4];
  CORE_ADDR saved_sp;

  greg = struct_return ? 4 : 3;
  freg = 1;
  vreg = 2;
  argstkspace = 0;
  structstkspace = 0;

  /* Figure out how much new stack space is required for arguments
     which don't fit in registers.  Unlike the PowerOpen ABI, the
     SysV ABI doesn't reserve any extra space for parameters which
     are put in registers. */
  for (argno = 0; argno < nargs; argno++)
    {
      arg = args[argno];
      type = check_typedef (VALUE_TYPE (arg));
      len = TYPE_LENGTH (type);

      if (TYPE_CODE (type) == TYPE_CODE_FLT)
	{
	  if (freg <= 8)
	    freg++;
	  else
	    {
	      /* SysV ABI converts floats to doubles when placed in
	         memory and requires 8 byte alignment */
	      if (argstkspace & 0x4)
		argstkspace += 4;
	      argstkspace += 8;
	    }
	}
      else if (TYPE_CODE (type) == TYPE_CODE_INT && len == 8)	/* long long */
	{
	  if (greg > 9)
	    {
	      greg = 11;
	      if (argstkspace & 0x4)
		argstkspace += 4;
	      argstkspace += 8;
	    }
	  else
	    {
	      if ((greg & 1) == 0)
		greg++;
	      greg += 2;
	    }
	}
      else if (!TYPE_VECTOR (type))
        {
	  if (len > 4
	      || TYPE_CODE (type) == TYPE_CODE_STRUCT
	      || TYPE_CODE (type) == TYPE_CODE_UNION)
	    {
	      /* Rounding to the nearest multiple of 8 may not be necessary,
		 but it is safe.  Particularly since we don't know the
		 field types of the structure */
	      structstkspace += round2 (len, 8);
	    }
	  if (greg <= 10)
	    greg++;
	  else
	    argstkspace += 4;
    	}
      else
        {
          if (len == 16
	      && TYPE_CODE (type) == TYPE_CODE_ARRAY
	      && TYPE_VECTOR (type))
	    {
	      if (vreg <= 13)
		vreg++;
	      else
		{
		  /* Vector arguments must be aligned to 16 bytes on
                     the stack. */
		  argstkspace += round2 (argstkspace, 16);
		  argstkspace += 16;
		}
	    }
	}
    }

  /* Get current SP location */
  saved_sp = read_sp ();

  sp -= argstkspace + structstkspace;

  /* Allocate space for backchain and callee's saved lr */
  sp -= 8;

  /* Make sure that we maintain 16 byte alignment */
  sp &= ~0x0f;

  /* Update %sp before proceeding any further */
  write_register (SP_REGNUM, sp);

  /* write the backchain */
  store_address (old_sp_buf, 4, saved_sp);
  write_memory (sp, old_sp_buf, 4);

  argoffset = 8;
  structoffset = argoffset + argstkspace;
  freg = 1;
  greg = 3;
  vreg = 2;
  /* Fill in r3 with the return structure, if any */
  if (struct_return)
    {
      char val_buf[4];
      store_address (val_buf, 4, struct_addr);
      memcpy (&registers[REGISTER_BYTE (greg)], val_buf, 4);
      greg++;
    }
  /* Now fill in the registers and stack... */
  for (argno = 0; argno < nargs; argno++)
    {
      arg = args[argno];
      type = check_typedef (VALUE_TYPE (arg));
      len = TYPE_LENGTH (type);

      if (TYPE_CODE (type) == TYPE_CODE_FLT)
	{
	  if (freg <= 8)
	    {
	      if (len > 8)
		printf_unfiltered (
				   "Fatal Error: a floating point parameter #%d with a size > 8 is found!\n", argno);
	      memcpy (&registers[REGISTER_BYTE (FP0_REGNUM + freg)],
		      VALUE_CONTENTS (arg), len);
	      freg++;
	    }
	  else
	    {
	      /* SysV ABI converts floats to doubles when placed in
	         memory and requires 8 byte alignment */
	      /* FIXME: Convert floats to doubles */
	      if (argoffset & 0x4)
		argoffset += 4;
	      write_memory (sp + argoffset, (char *) VALUE_CONTENTS (arg), len);
	      argoffset += 8;
	    }
	}
      else if (TYPE_CODE (type) == TYPE_CODE_INT && len == 8)	/* long long */
	{
	  if (greg > 9)
	    {
	      greg = 11;
	      if (argoffset & 0x4)
		argoffset += 4;
	      write_memory (sp + argoffset, (char *) VALUE_CONTENTS (arg), len);
	      argoffset += 8;
	    }
	  else
	    {
	      if ((greg & 1) == 0)
		greg++;

	      memcpy (&registers[REGISTER_BYTE (greg)],
		      VALUE_CONTENTS (arg), 4);
	      memcpy (&registers[REGISTER_BYTE (greg + 1)],
		      VALUE_CONTENTS (arg) + 4, 4);
	      greg += 2;
	    }
	}
      else if (!TYPE_VECTOR (type))
	{
	  char val_buf[4];
	  if (len > 4
	      || TYPE_CODE (type) == TYPE_CODE_STRUCT
	      || TYPE_CODE (type) == TYPE_CODE_UNION)
	    {
	      write_memory (sp + structoffset, VALUE_CONTENTS (arg), len);
	      store_address (val_buf, 4, sp + structoffset);
	      structoffset += round2 (len, 8);
	    }
	  else
	    {
	      memset (val_buf, 0, 4);
	      memcpy (val_buf, VALUE_CONTENTS (arg), len);
	    }
	  if (greg <= 10)
	    {
	      memcpy (&registers[REGISTER_BYTE (greg)], val_buf, 4);
	      greg++;
	    }
	  else
	    {
	      write_memory (sp + argoffset, val_buf, 4);
	      argoffset += 4;
	    }
	}
      else
	{
	  if (len == 16
	      && TYPE_CODE (type) == TYPE_CODE_ARRAY
	      && TYPE_VECTOR (type))
	    {
	      struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch);
	      char *v_val_buf = alloca (16);
	      memset (v_val_buf, 0, 16);
	      memcpy (v_val_buf, VALUE_CONTENTS (arg), len);
	      if (vreg <= 13)
		{
		  memcpy (&registers[REGISTER_BYTE (tdep->ppc_vr0_regnum
						    + vreg)],
			  v_val_buf, 16);
		  vreg++;
		}
	      else
		{
		  write_memory (sp + argoffset, v_val_buf, 16);
		  argoffset += 16;
		}
	    }
        }
    }

  target_store_registers (-1);
  return sp;
}

/* Until November 2001, gcc was not complying to the SYSV ABI for 
   returning structures less than or equal to 8 bytes in size.  It was
   returning everything in memory.  When this was corrected, it wasn't
   fixed for native platforms.  */
int     
ppc_sysv_abi_broken_use_struct_convention (int gcc_p, struct type *value_type)
{  
  if (TYPE_LENGTH (value_type) == 16 
      && TYPE_VECTOR (value_type))
    return 0;                            

  return generic_use_struct_convention (gcc_p, value_type);
}

/* Structures 8 bytes or less long are returned in the r3 & r4
   registers, according to the SYSV ABI. */
int
ppc_sysv_abi_use_struct_convention (int gcc_p, struct type *value_type)
{
  if (TYPE_LENGTH (value_type) == 16
      && TYPE_VECTOR (value_type))
    return 0;

  return (TYPE_LENGTH (value_type) > 8);
}   
