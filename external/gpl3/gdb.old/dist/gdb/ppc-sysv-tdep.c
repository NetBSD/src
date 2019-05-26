/* Target-dependent code for PowerPC systems using the SVR4 ABI
   for GDB, the GNU debugger.

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

#include "defs.h"
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "value.h"
#include "ppc-tdep.h"
#include "target.h"
#include "objfiles.h"
#include "infcall.h"
#include "dwarf2.h"
#include <algorithm>


/* Check whether FTPYE is a (pointer to) function type that should use
   the OpenCL vector ABI.  */

static int
ppc_sysv_use_opencl_abi (struct type *ftype)
{
  ftype = check_typedef (ftype);

  if (TYPE_CODE (ftype) == TYPE_CODE_PTR)
    ftype = check_typedef (TYPE_TARGET_TYPE (ftype));

  return (TYPE_CODE (ftype) == TYPE_CODE_FUNC
	  && TYPE_CALLING_CONVENTION (ftype) == DW_CC_GDB_IBM_OpenCL);
}

/* Pass the arguments in either registers, or in the stack.  Using the
   ppc sysv ABI, the first eight words of the argument list (that might
   be less than eight parameters if some parameters occupy more than one
   word) are passed in r3..r10 registers.  float and double parameters are
   passed in fpr's, in addition to that.  Rest of the parameters if any
   are passed in user stack.

   If the function is returning a structure, then the return address is passed
   in r3, then the first 7 words of the parametes can be passed in registers,
   starting from r4.  */

CORE_ADDR
ppc_sysv_abi_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			      struct regcache *regcache, CORE_ADDR bp_addr,
			      int nargs, struct value **args, CORE_ADDR sp,
			      int struct_return, CORE_ADDR struct_addr)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int opencl_abi = ppc_sysv_use_opencl_abi (value_type (function));
  ULONGEST saved_sp;
  int argspace = 0;		/* 0 is an initial wrong guess.  */
  int write_pass;

  gdb_assert (tdep->wordsize == 4);

  regcache_cooked_read_unsigned (regcache, gdbarch_sp_regnum (gdbarch),
				 &saved_sp);

  /* Go through the argument list twice.

     Pass 1: Figure out how much new stack space is required for
     arguments and pushed values.  Unlike the PowerOpen ABI, the SysV
     ABI doesn't reserve any extra space for parameters which are put
     in registers, but does always push structures and then pass their
     address.

     Pass 2: Replay the same computation but this time also write the
     values out to the target.  */

  for (write_pass = 0; write_pass < 2; write_pass++)
    {
      int argno;
      /* Next available floating point register for float and double
         arguments.  */
      int freg = 1;
      /* Next available general register for non-float, non-vector
         arguments.  */
      int greg = 3;
      /* Next available vector register for vector arguments.  */
      int vreg = 2;
      /* Arguments start above the "LR save word" and "Back chain".  */
      int argoffset = 2 * tdep->wordsize;
      /* Structures start after the arguments.  */
      int structoffset = argoffset + argspace;

      /* If the function is returning a `struct', then the first word
         (which will be passed in r3) is used for struct return
         address.  In that case we should advance one word and start
         from r4 register to copy parameters.  */
      if (struct_return)
	{
	  if (write_pass)
	    regcache_cooked_write_signed (regcache,
					  tdep->ppc_gp0_regnum + greg,
					  struct_addr);
	  greg++;
	}

      for (argno = 0; argno < nargs; argno++)
	{
	  struct value *arg = args[argno];
	  struct type *type = check_typedef (value_type (arg));
	  int len = TYPE_LENGTH (type);
	  const bfd_byte *val = value_contents (arg);

	  if (TYPE_CODE (type) == TYPE_CODE_FLT && len <= 8
	      && !tdep->soft_float)
	    {
	      /* Floating point value converted to "double" then
	         passed in an FP register, when the registers run out,
	         8 byte aligned stack is used.  */
	      if (freg <= 8)
		{
		  if (write_pass)
		    {
		      /* Always store the floating point value using
		         the register's floating-point format.  */
		      gdb_byte regval[MAX_REGISTER_SIZE];
		      struct type *regtype
			= register_type (gdbarch, tdep->ppc_fp0_regnum + freg);
		      convert_typed_floating (val, type, regval, regtype);
		      regcache_cooked_write (regcache,
                                             tdep->ppc_fp0_regnum + freg,
					     regval);
		    }
		  freg++;
		}
	      else
		{
		  /* The SysV ABI tells us to convert floats to
		     doubles before writing them to an 8 byte aligned
		     stack location.  Unfortunately GCC does not do
		     that, and stores floats into 4 byte aligned
		     locations without converting them to doubles.
		     Since there is no know compiler that actually
		     follows the ABI here, we implement the GCC
		     convention.  */

		  /* Align to 4 bytes or 8 bytes depending on the type of
		     the argument (float or double).  */
		  argoffset = align_up (argoffset, len);
		  if (write_pass)
		      write_memory (sp + argoffset, val, len);
		  argoffset += len;
		}
	    }
	  else if (TYPE_CODE (type) == TYPE_CODE_FLT
		   && len == 16
		   && !tdep->soft_float
		   && (gdbarch_long_double_format (gdbarch)
		       == floatformats_ibm_long_double))
	    {
	      /* IBM long double passed in two FP registers if
		 available, otherwise 8-byte aligned stack.  */
	      if (freg <= 7)
		{
		  if (write_pass)
		    {
		      regcache_cooked_write (regcache,
					     tdep->ppc_fp0_regnum + freg,
					     val);
		      regcache_cooked_write (regcache,
					     tdep->ppc_fp0_regnum + freg + 1,
					     val + 8);
		    }
		  freg += 2;
		}
	      else
		{
		  argoffset = align_up (argoffset, 8);
		  if (write_pass)
		    write_memory (sp + argoffset, val, len);
		  argoffset += 16;
		}
	    }
	  else if (len == 8
		   && (TYPE_CODE (type) == TYPE_CODE_INT	/* long long */
		       || TYPE_CODE (type) == TYPE_CODE_FLT	/* double */
		       || (TYPE_CODE (type) == TYPE_CODE_DECFLOAT
			   && tdep->soft_float)))
	    {
	      /* "long long" or soft-float "double" or "_Decimal64"
	         passed in an odd/even register pair with the low
	         addressed word in the odd register and the high
	         addressed word in the even register, or when the
	         registers run out an 8 byte aligned stack
	         location.  */
	      if (greg > 9)
		{
		  /* Just in case GREG was 10.  */
		  greg = 11;
		  argoffset = align_up (argoffset, 8);
		  if (write_pass)
		    write_memory (sp + argoffset, val, len);
		  argoffset += 8;
		}
	      else
		{
		  /* Must start on an odd register - r3/r4 etc.  */
		  if ((greg & 1) == 0)
		    greg++;
		  if (write_pass)
		    {
		      regcache_cooked_write (regcache,
					     tdep->ppc_gp0_regnum + greg + 0,
					     val + 0);
		      regcache_cooked_write (regcache,
					     tdep->ppc_gp0_regnum + greg + 1,
					     val + 4);
		    }
		  greg += 2;
		}
	    }
	  else if (len == 16
		   && ((TYPE_CODE (type) == TYPE_CODE_FLT
			&& (gdbarch_long_double_format (gdbarch)
			    == floatformats_ibm_long_double))
		       || (TYPE_CODE (type) == TYPE_CODE_DECFLOAT
			   && tdep->soft_float)))
	    {
	      /* Soft-float IBM long double or _Decimal128 passed in
		 four consecutive registers, or on the stack.  The
		 registers are not necessarily odd/even pairs.  */
	      if (greg > 7)
		{
		  greg = 11;
		  argoffset = align_up (argoffset, 8);
		  if (write_pass)
		    write_memory (sp + argoffset, val, len);
		  argoffset += 16;
		}
	      else
		{
		  if (write_pass)
		    {
		      regcache_cooked_write (regcache,
					     tdep->ppc_gp0_regnum + greg + 0,
					     val + 0);
		      regcache_cooked_write (regcache,
					     tdep->ppc_gp0_regnum + greg + 1,
					     val + 4);
		      regcache_cooked_write (regcache,
					     tdep->ppc_gp0_regnum + greg + 2,
					     val + 8);
		      regcache_cooked_write (regcache,
					     tdep->ppc_gp0_regnum + greg + 3,
					     val + 12);
		    }
		  greg += 4;
		}
	    }
	  else if (TYPE_CODE (type) == TYPE_CODE_DECFLOAT && len <= 8
		   && !tdep->soft_float)
	    {
	      /* 32-bit and 64-bit decimal floats go in f1 .. f8.  They can
	         end up in memory.  */

	      if (freg <= 8)
		{
		  if (write_pass)
		    {
		      gdb_byte regval[MAX_REGISTER_SIZE];
		      const gdb_byte *p;

		      /* 32-bit decimal floats are right aligned in the
			 doubleword.  */
		      if (TYPE_LENGTH (type) == 4)
		      {
			memcpy (regval + 4, val, 4);
			p = regval;
		      }
		      else
			p = val;

		      regcache_cooked_write (regcache,
			  tdep->ppc_fp0_regnum + freg, p);
		    }

		  freg++;
		}
	      else
		{
		  argoffset = align_up (argoffset, len);

		  if (write_pass)
		    /* Write value in the stack's parameter save area.  */
		    write_memory (sp + argoffset, val, len);

		  argoffset += len;
		}
	    }
	  else if (TYPE_CODE (type) == TYPE_CODE_DECFLOAT && len == 16
		   && !tdep->soft_float)
	    {
	      /* 128-bit decimal floats go in f2 .. f7, always in even/odd
		 pairs.  They can end up in memory, using two doublewords.  */

	      if (freg <= 6)
		{
		  /* Make sure freg is even.  */
		  freg += freg & 1;

		  if (write_pass)
		    {
		      regcache_cooked_write (regcache,
					     tdep->ppc_fp0_regnum + freg, val);
		      regcache_cooked_write (regcache,
			  tdep->ppc_fp0_regnum + freg + 1, val + 8);
		    }
		}
	      else
		{
		  argoffset = align_up (argoffset, 8);

		  if (write_pass)
		    write_memory (sp + argoffset, val, 16);

		  argoffset += 16;
		}

	      /* If a 128-bit decimal float goes to the stack because only f7
	         and f8 are free (thus there's no even/odd register pair
		 available), these registers should be marked as occupied.
		 Hence we increase freg even when writing to memory.  */
	      freg += 2;
	    }
	  else if (len < 16
		   && TYPE_CODE (type) == TYPE_CODE_ARRAY
		   && TYPE_VECTOR (type)
		   && opencl_abi)
	    {
	      /* OpenCL vectors shorter than 16 bytes are passed as if
		 a series of independent scalars.  */
	      struct type *eltype = check_typedef (TYPE_TARGET_TYPE (type));
	      int i, nelt = TYPE_LENGTH (type) / TYPE_LENGTH (eltype);

	      for (i = 0; i < nelt; i++)
		{
		  const gdb_byte *elval = val + i * TYPE_LENGTH (eltype);

		  if (TYPE_CODE (eltype) == TYPE_CODE_FLT && !tdep->soft_float)
		    {
		      if (freg <= 8)
			{
			  if (write_pass)
			    {
			      int regnum = tdep->ppc_fp0_regnum + freg;
			      gdb_byte regval[MAX_REGISTER_SIZE];
			      struct type *regtype
				= register_type (gdbarch, regnum);
			      convert_typed_floating (elval, eltype,
						      regval, regtype);
			      regcache_cooked_write (regcache, regnum, regval);
			    }
			  freg++;
			}
		      else
			{
			  argoffset = align_up (argoffset, len);
			  if (write_pass)
			    write_memory (sp + argoffset, val, len);
			  argoffset += len;
			}
		    }
		  else if (TYPE_LENGTH (eltype) == 8)
		    {
		      if (greg > 9)
			{
			  /* Just in case GREG was 10.  */
			  greg = 11;
			  argoffset = align_up (argoffset, 8);
			  if (write_pass)
			    write_memory (sp + argoffset, elval,
					  TYPE_LENGTH (eltype));
			  argoffset += 8;
			}
		      else
			{
			  /* Must start on an odd register - r3/r4 etc.  */
			  if ((greg & 1) == 0)
			    greg++;
			  if (write_pass)
			    {
			      int regnum = tdep->ppc_gp0_regnum + greg;
			      regcache_cooked_write (regcache,
						     regnum + 0, elval + 0);
			      regcache_cooked_write (regcache,
						     regnum + 1, elval + 4);
			    }
			  greg += 2;
			}
		    }
		  else
		    {
		      gdb_byte word[MAX_REGISTER_SIZE];
		      store_unsigned_integer (word, tdep->wordsize, byte_order,
					      unpack_long (eltype, elval));

		      if (greg <= 10)
			{
			  if (write_pass)
			    regcache_cooked_write (regcache,
						   tdep->ppc_gp0_regnum + greg,
						   word);
			  greg++;
			}
		      else
			{
			  argoffset = align_up (argoffset, tdep->wordsize);
			  if (write_pass)
			    write_memory (sp + argoffset, word, tdep->wordsize);
			  argoffset += tdep->wordsize;
			}
		    }
		}
	    }
	  else if (len >= 16
		   && TYPE_CODE (type) == TYPE_CODE_ARRAY
		   && TYPE_VECTOR (type)
		   && opencl_abi)
	    {
	      /* OpenCL vectors 16 bytes or longer are passed as if
		 a series of AltiVec vectors.  */
	      int i;

	      for (i = 0; i < len / 16; i++)
		{
		  const gdb_byte *elval = val + i * 16;

		  if (vreg <= 13)
		    {
		      if (write_pass)
			regcache_cooked_write (regcache,
					       tdep->ppc_vr0_regnum + vreg,
					       elval);
		      vreg++;
		    }
		  else
		    {
		      argoffset = align_up (argoffset, 16);
		      if (write_pass)
			write_memory (sp + argoffset, elval, 16);
		      argoffset += 16;
		    }
		}
	    }
	  else if (len == 16
		   && TYPE_CODE (type) == TYPE_CODE_ARRAY
		   && TYPE_VECTOR (type)
		   && tdep->vector_abi == POWERPC_VEC_ALTIVEC)
	    {
	      /* Vector parameter passed in an Altivec register, or
	         when that runs out, 16 byte aligned stack location.  */
	      if (vreg <= 13)
		{
		  if (write_pass)
		    regcache_cooked_write (regcache,
					   tdep->ppc_vr0_regnum + vreg, val);
		  vreg++;
		}
	      else
		{
		  argoffset = align_up (argoffset, 16);
		  if (write_pass)
		    write_memory (sp + argoffset, val, 16);
		  argoffset += 16;
		}
	    }
	  else if (len == 8
		   && TYPE_CODE (type) == TYPE_CODE_ARRAY
		   && TYPE_VECTOR (type)
		   && tdep->vector_abi == POWERPC_VEC_SPE)
	    {
	      /* Vector parameter passed in an e500 register, or when
	         that runs out, 8 byte aligned stack location.  Note
	         that since e500 vector and general purpose registers
	         both map onto the same underlying register set, a
	         "greg" and not a "vreg" is consumed here.  A cooked
	         write stores the value in the correct locations
	         within the raw register cache.  */
	      if (greg <= 10)
		{
		  if (write_pass)
		    regcache_cooked_write (regcache,
					   tdep->ppc_ev0_regnum + greg, val);
		  greg++;
		}
	      else
		{
		  argoffset = align_up (argoffset, 8);
		  if (write_pass)
		    write_memory (sp + argoffset, val, 8);
		  argoffset += 8;
		}
	    }
	  else
	    {
	      /* Reduce the parameter down to something that fits in a
	         "word".  */
	      gdb_byte word[MAX_REGISTER_SIZE];
	      memset (word, 0, MAX_REGISTER_SIZE);
	      if (len > tdep->wordsize
		  || TYPE_CODE (type) == TYPE_CODE_STRUCT
		  || TYPE_CODE (type) == TYPE_CODE_UNION)
		{
		  /* Structs and large values are put in an
		     aligned stack slot ...  */
		  if (TYPE_CODE (type) == TYPE_CODE_ARRAY
		      && TYPE_VECTOR (type)
		      && len >= 16)
		    structoffset = align_up (structoffset, 16);
		  else
		    structoffset = align_up (structoffset, 8);

		  if (write_pass)
		    write_memory (sp + structoffset, val, len);
		  /* ... and then a "word" pointing to that address is
		     passed as the parameter.  */
		  store_unsigned_integer (word, tdep->wordsize, byte_order,
					  sp + structoffset);
		  structoffset += len;
		}
	      else if (TYPE_CODE (type) == TYPE_CODE_INT)
		/* Sign or zero extend the "int" into a "word".  */
		store_unsigned_integer (word, tdep->wordsize, byte_order,
					unpack_long (type, val));
	      else
		/* Always goes in the low address.  */
		memcpy (word, val, len);
	      /* Store that "word" in a register, or on the stack.
	         The words have "4" byte alignment.  */
	      if (greg <= 10)
		{
		  if (write_pass)
		    regcache_cooked_write (regcache,
					   tdep->ppc_gp0_regnum + greg, word);
		  greg++;
		}
	      else
		{
		  argoffset = align_up (argoffset, tdep->wordsize);
		  if (write_pass)
		    write_memory (sp + argoffset, word, tdep->wordsize);
		  argoffset += tdep->wordsize;
		}
	    }
	}

      /* Compute the actual stack space requirements.  */
      if (!write_pass)
	{
	  /* Remember the amount of space needed by the arguments.  */
	  argspace = argoffset;
	  /* Allocate space for both the arguments and the structures.  */
	  sp -= (argoffset + structoffset);
	  /* Ensure that the stack is still 16 byte aligned.  */
	  sp = align_down (sp, 16);
	}

      /* The psABI says that "A caller of a function that takes a
	 variable argument list shall set condition register bit 6 to
	 1 if it passes one or more arguments in the floating-point
	 registers.  It is strongly recommended that the caller set the
	 bit to 0 otherwise..."  Doing this for normal functions too
	 shouldn't hurt.  */
      if (write_pass)
	{
	  ULONGEST cr;

	  regcache_cooked_read_unsigned (regcache, tdep->ppc_cr_regnum, &cr);
	  if (freg > 1)
	    cr |= 0x02000000;
	  else
	    cr &= ~0x02000000;
	  regcache_cooked_write_unsigned (regcache, tdep->ppc_cr_regnum, cr);
	}
    }

  /* Update %sp.   */
  regcache_cooked_write_signed (regcache, gdbarch_sp_regnum (gdbarch), sp);

  /* Write the backchain (it occupies WORDSIZED bytes).  */
  write_memory_signed_integer (sp, tdep->wordsize, byte_order, saved_sp);

  /* Point the inferior function call's return address at the dummy's
     breakpoint.  */
  regcache_cooked_write_signed (regcache, tdep->ppc_lr_regnum, bp_addr);

  return sp;
}

/* Handle the return-value conventions for Decimal Floating Point values.  */
static enum return_value_convention
get_decimal_float_return_value (struct gdbarch *gdbarch, struct type *valtype,
				struct regcache *regcache, gdb_byte *readbuf,
				const gdb_byte *writebuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  gdb_assert (TYPE_CODE (valtype) == TYPE_CODE_DECFLOAT);

  /* 32-bit and 64-bit decimal floats in f1.  */
  if (TYPE_LENGTH (valtype) <= 8)
    {
      if (writebuf != NULL)
	{
	  gdb_byte regval[MAX_REGISTER_SIZE];
	  const gdb_byte *p;

	  /* 32-bit decimal float is right aligned in the doubleword.  */
	  if (TYPE_LENGTH (valtype) == 4)
	    {
	      memcpy (regval + 4, writebuf, 4);
	      p = regval;
	    }
	  else
	    p = writebuf;

	  regcache_cooked_write (regcache, tdep->ppc_fp0_regnum + 1, p);
	}
      if (readbuf != NULL)
	{
	  regcache_cooked_read (regcache, tdep->ppc_fp0_regnum + 1, readbuf);

	  /* Left align 32-bit decimal float.  */
	  if (TYPE_LENGTH (valtype) == 4)
	    memcpy (readbuf, readbuf + 4, 4);
	}
    }
  /* 128-bit decimal floats in f2,f3.  */
  else if (TYPE_LENGTH (valtype) == 16)
    {
      if (writebuf != NULL || readbuf != NULL)
	{
	  int i;

	  for (i = 0; i < 2; i++)
	    {
	      if (writebuf != NULL)
		regcache_cooked_write (regcache, tdep->ppc_fp0_regnum + 2 + i,
				       writebuf + i * 8);
	      if (readbuf != NULL)
		regcache_cooked_read (regcache, tdep->ppc_fp0_regnum + 2 + i,
				      readbuf + i * 8);
	    }
	}
    }
  else
    /* Can't happen.  */
    internal_error (__FILE__, __LINE__, _("Unknown decimal float size."));

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Handle the return-value conventions specified by the SysV 32-bit
   PowerPC ABI (including all the supplements):

   no floating-point: floating-point values returned using 32-bit
   general-purpose registers.

   Altivec: 128-bit vectors returned using vector registers.

   e500: 64-bit vectors returned using the full full 64 bit EV
   register, floating-point values returned using 32-bit
   general-purpose registers.

   GCC (broken): Small struct values right (instead of left) aligned
   when returned in general-purpose registers.  */

static enum return_value_convention
do_ppc_sysv_return_value (struct gdbarch *gdbarch, struct type *func_type,
			  struct type *type, struct regcache *regcache,
			  gdb_byte *readbuf, const gdb_byte *writebuf,
			  int broken_gcc)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int opencl_abi = func_type? ppc_sysv_use_opencl_abi (func_type) : 0;

  gdb_assert (tdep->wordsize == 4);

  if (TYPE_CODE (type) == TYPE_CODE_FLT
      && TYPE_LENGTH (type) <= 8
      && !tdep->soft_float)
    {
      if (readbuf)
	{
	  /* Floats and doubles stored in "f1".  Convert the value to
	     the required type.  */
	  gdb_byte regval[MAX_REGISTER_SIZE];
	  struct type *regtype = register_type (gdbarch,
                                                tdep->ppc_fp0_regnum + 1);
	  regcache_cooked_read (regcache, tdep->ppc_fp0_regnum + 1, regval);
	  convert_typed_floating (regval, regtype, readbuf, type);
	}
      if (writebuf)
	{
	  /* Floats and doubles stored in "f1".  Convert the value to
	     the register's "double" type.  */
	  gdb_byte regval[MAX_REGISTER_SIZE];
	  struct type *regtype = register_type (gdbarch, tdep->ppc_fp0_regnum);
	  convert_typed_floating (writebuf, type, regval, regtype);
	  regcache_cooked_write (regcache, tdep->ppc_fp0_regnum + 1, regval);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (TYPE_CODE (type) == TYPE_CODE_FLT
      && TYPE_LENGTH (type) == 16
      && !tdep->soft_float
      && (gdbarch_long_double_format (gdbarch)
	  == floatformats_ibm_long_double))
    {
      /* IBM long double stored in f1 and f2.  */
      if (readbuf)
	{
	  regcache_cooked_read (regcache, tdep->ppc_fp0_regnum + 1, readbuf);
	  regcache_cooked_read (regcache, tdep->ppc_fp0_regnum + 2,
				readbuf + 8);
	}
      if (writebuf)
	{
	  regcache_cooked_write (regcache, tdep->ppc_fp0_regnum + 1, writebuf);
	  regcache_cooked_write (regcache, tdep->ppc_fp0_regnum + 2,
				 writebuf + 8);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (TYPE_LENGTH (type) == 16
      && ((TYPE_CODE (type) == TYPE_CODE_FLT
	   && (gdbarch_long_double_format (gdbarch)
	       == floatformats_ibm_long_double))
	  || (TYPE_CODE (type) == TYPE_CODE_DECFLOAT && tdep->soft_float)))
    {
      /* Soft-float IBM long double or _Decimal128 stored in r3, r4,
	 r5, r6.  */
      if (readbuf)
	{
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 3, readbuf);
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 4,
				readbuf + 4);
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 5,
				readbuf + 8);
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 6,
				readbuf + 12);
	}
      if (writebuf)
	{
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 3, writebuf);
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 4,
				 writebuf + 4);
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 5,
				 writebuf + 8);
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 6,
				 writebuf + 12);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if ((TYPE_CODE (type) == TYPE_CODE_INT && TYPE_LENGTH (type) == 8)
      || (TYPE_CODE (type) == TYPE_CODE_FLT && TYPE_LENGTH (type) == 8)
      || (TYPE_CODE (type) == TYPE_CODE_DECFLOAT && TYPE_LENGTH (type) == 8
	  && tdep->soft_float))
    {
      if (readbuf)
	{
	  /* A long long, double or _Decimal64 stored in the 32 bit
	     r3/r4.  */
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 3,
				readbuf + 0);
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 4,
				readbuf + 4);
	}
      if (writebuf)
	{
	  /* A long long, double or _Decimal64 stored in the 32 bit
	     r3/r4.  */
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 3,
				 writebuf + 0);
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 4,
				 writebuf + 4);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (TYPE_CODE (type) == TYPE_CODE_DECFLOAT && !tdep->soft_float)
    return get_decimal_float_return_value (gdbarch, type, regcache, readbuf,
					   writebuf);
  else if ((TYPE_CODE (type) == TYPE_CODE_INT
	    || TYPE_CODE (type) == TYPE_CODE_CHAR
	    || TYPE_CODE (type) == TYPE_CODE_BOOL
	    || TYPE_CODE (type) == TYPE_CODE_PTR
	    || TYPE_IS_REFERENCE (type)
	    || TYPE_CODE (type) == TYPE_CODE_ENUM)
	   && TYPE_LENGTH (type) <= tdep->wordsize)
    {
      if (readbuf)
	{
	  /* Some sort of integer stored in r3.  Since TYPE isn't
	     bigger than the register, sign extension isn't a problem
	     - just do everything unsigned.  */
	  ULONGEST regval;
	  regcache_cooked_read_unsigned (regcache, tdep->ppc_gp0_regnum + 3,
					 &regval);
	  store_unsigned_integer (readbuf, TYPE_LENGTH (type), byte_order,
				  regval);
	}
      if (writebuf)
	{
	  /* Some sort of integer stored in r3.  Use unpack_long since
	     that should handle any required sign extension.  */
	  regcache_cooked_write_unsigned (regcache, tdep->ppc_gp0_regnum + 3,
					  unpack_long (type, writebuf));
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  /* OpenCL vectors < 16 bytes are returned as distinct
     scalars in f1..f2 or r3..r10.  */
  if (TYPE_CODE (type) == TYPE_CODE_ARRAY
      && TYPE_VECTOR (type)
      && TYPE_LENGTH (type) < 16
      && opencl_abi)
    {
      struct type *eltype = check_typedef (TYPE_TARGET_TYPE (type));
      int i, nelt = TYPE_LENGTH (type) / TYPE_LENGTH (eltype);

      for (i = 0; i < nelt; i++)
	{
	  int offset = i * TYPE_LENGTH (eltype);

	  if (TYPE_CODE (eltype) == TYPE_CODE_FLT)
	    {
	      int regnum = tdep->ppc_fp0_regnum + 1 + i;
	      gdb_byte regval[MAX_REGISTER_SIZE];
	      struct type *regtype = register_type (gdbarch, regnum);

	      if (writebuf != NULL)
		{
		  convert_typed_floating (writebuf + offset, eltype,
					  regval, regtype);
		  regcache_cooked_write (regcache, regnum, regval);
		}
	      if (readbuf != NULL)
		{
		  regcache_cooked_read (regcache, regnum, regval);
		  convert_typed_floating (regval, regtype,
					  readbuf + offset, eltype);
		}
	    }
	  else
	    {
	      int regnum = tdep->ppc_gp0_regnum + 3 + i;
	      ULONGEST regval;

	      if (writebuf != NULL)
		{
		  regval = unpack_long (eltype, writebuf + offset);
		  regcache_cooked_write_unsigned (regcache, regnum, regval);
		}
	      if (readbuf != NULL)
		{
		  regcache_cooked_read_unsigned (regcache, regnum, &regval);
		  store_unsigned_integer (readbuf + offset,
					  TYPE_LENGTH (eltype), byte_order,
					  regval);
		}
	    }
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  /* OpenCL vectors >= 16 bytes are returned in v2..v9.  */
  if (TYPE_CODE (type) == TYPE_CODE_ARRAY
      && TYPE_VECTOR (type)
      && TYPE_LENGTH (type) >= 16
      && opencl_abi)
    {
      int n_regs = TYPE_LENGTH (type) / 16;
      int i;

      for (i = 0; i < n_regs; i++)
	{
	  int offset = i * 16;
	  int regnum = tdep->ppc_vr0_regnum + 2 + i;

	  if (writebuf != NULL)
	    regcache_cooked_write (regcache, regnum, writebuf + offset);
	  if (readbuf != NULL)
	    regcache_cooked_read (regcache, regnum, readbuf + offset);
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (TYPE_LENGTH (type) == 16
      && TYPE_CODE (type) == TYPE_CODE_ARRAY
      && TYPE_VECTOR (type)
      && tdep->vector_abi == POWERPC_VEC_ALTIVEC)
    {
      if (readbuf)
	{
	  /* Altivec places the return value in "v2".  */
	  regcache_cooked_read (regcache, tdep->ppc_vr0_regnum + 2, readbuf);
	}
      if (writebuf)
	{
	  /* Altivec places the return value in "v2".  */
	  regcache_cooked_write (regcache, tdep->ppc_vr0_regnum + 2, writebuf);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (TYPE_LENGTH (type) == 16
      && TYPE_CODE (type) == TYPE_CODE_ARRAY
      && TYPE_VECTOR (type)
      && tdep->vector_abi == POWERPC_VEC_GENERIC)
    {
      /* GCC -maltivec -mabi=no-altivec returns vectors in r3/r4/r5/r6.
	 GCC without AltiVec returns them in memory, but it warns about
	 ABI risks in that case; we don't try to support it.  */
      if (readbuf)
	{
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 3,
				readbuf + 0);
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 4,
				readbuf + 4);
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 5,
				readbuf + 8);
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 6,
				readbuf + 12);
	}
      if (writebuf)
	{
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 3,
				 writebuf + 0);
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 4,
				 writebuf + 4);
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 5,
				 writebuf + 8);
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 6,
				 writebuf + 12);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (TYPE_LENGTH (type) == 8
      && TYPE_CODE (type) == TYPE_CODE_ARRAY
      && TYPE_VECTOR (type)
      && tdep->vector_abi == POWERPC_VEC_SPE)
    {
      /* The e500 ABI places return values for the 64-bit DSP types
	 (__ev64_opaque__) in r3.  However, in GDB-speak, ev3
	 corresponds to the entire r3 value for e500, whereas GDB's r3
	 only corresponds to the least significant 32-bits.  So place
	 the 64-bit DSP type's value in ev3.  */
      if (readbuf)
	regcache_cooked_read (regcache, tdep->ppc_ev0_regnum + 3, readbuf);
      if (writebuf)
	regcache_cooked_write (regcache, tdep->ppc_ev0_regnum + 3, writebuf);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (broken_gcc && TYPE_LENGTH (type) <= 8)
    {
      /* GCC screwed up for structures or unions whose size is less
	 than or equal to 8 bytes..  Instead of left-aligning, it
	 right-aligns the data into the buffer formed by r3, r4.  */
      gdb_byte regvals[MAX_REGISTER_SIZE * 2];
      int len = TYPE_LENGTH (type);
      int offset = (2 * tdep->wordsize - len) % tdep->wordsize;

      if (readbuf)
	{
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 3,
				regvals + 0 * tdep->wordsize);
	  if (len > tdep->wordsize)
	    regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 4,
				  regvals + 1 * tdep->wordsize);
	  memcpy (readbuf, regvals + offset, len);
	}
      if (writebuf)
	{
	  memset (regvals, 0, sizeof regvals);
	  memcpy (regvals + offset, writebuf, len);
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 3,
				 regvals + 0 * tdep->wordsize);
	  if (len > tdep->wordsize)
	    regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 4,
				   regvals + 1 * tdep->wordsize);
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (TYPE_LENGTH (type) <= 8)
    {
      if (readbuf)
	{
	  /* This matches SVr4 PPC, it does not match GCC.  */
	  /* The value is right-padded to 8 bytes and then loaded, as
	     two "words", into r3/r4.  */
	  gdb_byte regvals[MAX_REGISTER_SIZE * 2];
	  regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 3,
				regvals + 0 * tdep->wordsize);
	  if (TYPE_LENGTH (type) > tdep->wordsize)
	    regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 4,
				  regvals + 1 * tdep->wordsize);
	  memcpy (readbuf, regvals, TYPE_LENGTH (type));
	}
      if (writebuf)
	{
	  /* This matches SVr4 PPC, it does not match GCC.  */
	  /* The value is padded out to 8 bytes and then loaded, as
	     two "words" into r3/r4.  */
	  gdb_byte regvals[MAX_REGISTER_SIZE * 2];
	  memset (regvals, 0, sizeof regvals);
	  memcpy (regvals, writebuf, TYPE_LENGTH (type));
	  regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 3,
				 regvals + 0 * tdep->wordsize);
	  if (TYPE_LENGTH (type) > tdep->wordsize)
	    regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 4,
				   regvals + 1 * tdep->wordsize);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  return RETURN_VALUE_STRUCT_CONVENTION;
}

enum return_value_convention
ppc_sysv_abi_return_value (struct gdbarch *gdbarch, struct value *function,
			   struct type *valtype, struct regcache *regcache,
			   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  return do_ppc_sysv_return_value (gdbarch,
				   function ? value_type (function) : NULL,
				   valtype, regcache, readbuf, writebuf, 0);
}

enum return_value_convention
ppc_sysv_abi_broken_return_value (struct gdbarch *gdbarch,
				  struct value *function,
				  struct type *valtype,
				  struct regcache *regcache,
				  gdb_byte *readbuf, const gdb_byte *writebuf)
{
  return do_ppc_sysv_return_value (gdbarch,
				   function ? value_type (function) : NULL,
				   valtype, regcache, readbuf, writebuf, 1);
}

/* The helper function for 64-bit SYSV push_dummy_call.  Converts the
   function's code address back into the function's descriptor
   address.

   Find a value for the TOC register.  Every symbol should have both
   ".FN" and "FN" in the minimal symbol table.  "FN" points at the
   FN's descriptor, while ".FN" points at the entry point (which
   matches FUNC_ADDR).  Need to reverse from FUNC_ADDR back to the
   FN's descriptor address (while at the same time being careful to
   find "FN" in the same object file as ".FN").  */

static int
convert_code_addr_to_desc_addr (CORE_ADDR code_addr, CORE_ADDR *desc_addr)
{
  struct obj_section *dot_fn_section;
  struct bound_minimal_symbol dot_fn;
  struct bound_minimal_symbol fn;

  /* Find the minimal symbol that corresponds to CODE_ADDR (should
     have a name of the form ".FN").  */
  dot_fn = lookup_minimal_symbol_by_pc (code_addr);
  if (dot_fn.minsym == NULL || MSYMBOL_LINKAGE_NAME (dot_fn.minsym)[0] != '.')
    return 0;
  /* Get the section that contains CODE_ADDR.  Need this for the
     "objfile" that it contains.  */
  dot_fn_section = find_pc_section (code_addr);
  if (dot_fn_section == NULL || dot_fn_section->objfile == NULL)
    return 0;
  /* Now find the corresponding "FN" (dropping ".") minimal symbol's
     address.  Only look for the minimal symbol in ".FN"'s object file
     - avoids problems when two object files (i.e., shared libraries)
     contain a minimal symbol with the same name.  */
  fn = lookup_minimal_symbol (MSYMBOL_LINKAGE_NAME (dot_fn.minsym) + 1, NULL,
			      dot_fn_section->objfile);
  if (fn.minsym == NULL)
    return 0;
  /* Found a descriptor.  */
  (*desc_addr) = BMSYMBOL_VALUE_ADDRESS (fn);
  return 1;
}

/* Walk down the type tree of TYPE counting consecutive base elements.
   If *FIELD_TYPE is NULL, then set it to the first valid floating point
   or vector type.  If a non-floating point or vector type is found, or
   if a floating point or vector type that doesn't match a non-NULL
   *FIELD_TYPE is found, then return -1, otherwise return the count in the
   sub-tree.  */

static LONGEST
ppc64_aggregate_candidate (struct type *type,
			   struct type **field_type)
{
  type = check_typedef (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      if (!*field_type)
	*field_type = type;
      if (TYPE_CODE (*field_type) == TYPE_CODE (type)
	  && TYPE_LENGTH (*field_type) == TYPE_LENGTH (type))
	return 1;
      break;

    case TYPE_CODE_COMPLEX:
      type = TYPE_TARGET_TYPE (type);
      if (TYPE_CODE (type) == TYPE_CODE_FLT
	  || TYPE_CODE (type) == TYPE_CODE_DECFLOAT)
	{
	  if (!*field_type)
	    *field_type = type;
	  if (TYPE_CODE (*field_type) == TYPE_CODE (type)
	      && TYPE_LENGTH (*field_type) == TYPE_LENGTH (type))
	    return 2;
	}
      break;

    case TYPE_CODE_ARRAY:
      if (TYPE_VECTOR (type))
	{
	  if (!*field_type)
	    *field_type = type;
	  if (TYPE_CODE (*field_type) == TYPE_CODE (type)
	      && TYPE_LENGTH (*field_type) == TYPE_LENGTH (type))
	    return 1;
	}
      else
	{
	  LONGEST count, low_bound, high_bound;

	  count = ppc64_aggregate_candidate
		   (TYPE_TARGET_TYPE (type), field_type);
	  if (count == -1)
	    return -1;

	  if (!get_array_bounds (type, &low_bound, &high_bound))
	    return -1;
	  count *= high_bound - low_bound;

	  /* There must be no padding.  */
	  if (count == 0)
	    return TYPE_LENGTH (type) == 0 ? 0 : -1;
	  else if (TYPE_LENGTH (type) != count * TYPE_LENGTH (*field_type))
	    return -1;

	  return count;
	}
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
	{
	  LONGEST count = 0;
	  int i;

	  for (i = 0; i < TYPE_NFIELDS (type); i++)
	    {
	      LONGEST sub_count;

	      if (field_is_static (&TYPE_FIELD (type, i)))
		continue;

	      sub_count = ppc64_aggregate_candidate
			   (TYPE_FIELD_TYPE (type, i), field_type);
	      if (sub_count == -1)
		return -1;

	      if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
		count += sub_count;
	      else
		count = std::max (count, sub_count);
	    }

	  /* There must be no padding.  */
	  if (count == 0)
	    return TYPE_LENGTH (type) == 0 ? 0 : -1;
	  else if (TYPE_LENGTH (type) != count * TYPE_LENGTH (*field_type))
	    return -1;

	  return count;
	}
      break;

    default:
      break;
    }

  return -1;
}

/* If an argument of type TYPE is a homogeneous float or vector aggregate
   that shall be passed in FP/vector registers according to the ELFv2 ABI,
   return the homogeneous element type in *ELT_TYPE and the number of
   elements in *N_ELTS, and return non-zero.  Otherwise, return zero.  */

static int
ppc64_elfv2_abi_homogeneous_aggregate (struct type *type,
				       struct type **elt_type, int *n_elts)
{
  /* Complex types at the top level are treated separately.  However,
     complex types can be elements of homogeneous aggregates.  */
  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      || TYPE_CODE (type) == TYPE_CODE_UNION
      || (TYPE_CODE (type) == TYPE_CODE_ARRAY && !TYPE_VECTOR (type)))
    {
      struct type *field_type = NULL;
      LONGEST field_count = ppc64_aggregate_candidate (type, &field_type);

      if (field_count > 0)
	{
	  int n_regs = ((TYPE_CODE (field_type) == TYPE_CODE_FLT
			 || TYPE_CODE (field_type) == TYPE_CODE_DECFLOAT)?
			(TYPE_LENGTH (field_type) + 7) >> 3 : 1);

	  /* The ELFv2 ABI allows homogeneous aggregates to occupy
	     up to 8 registers.  */
	  if (field_count * n_regs <= 8)
	    {
	      if (elt_type)
		*elt_type = field_type;
	      if (n_elts)
		*n_elts = (int) field_count;
	      /* Note that field_count is LONGEST since it may hold the size
		 of an array, while *n_elts is int since its value is bounded
		 by the number of registers used for argument passing.  The
		 cast cannot overflow due to the bounds checking above.  */
	      return 1;
	    }
	}
    }

  return 0;
}

/* Structure holding the next argument position.  */
struct ppc64_sysv_argpos
  {
    /* Register cache holding argument registers.  If this is NULL,
       we only simulate argument processing without actually updating
       any registers or memory.  */
    struct regcache *regcache;
    /* Next available general-purpose argument register.  */
    int greg;
    /* Next available floating-point argument register.  */
    int freg;
    /* Next available vector argument register.  */
    int vreg;
    /* The address, at which the next general purpose parameter
       (integer, struct, float, vector, ...) should be saved.  */
    CORE_ADDR gparam;
    /* The address, at which the next by-reference parameter
       (non-Altivec vector, variably-sized type) should be saved.  */
    CORE_ADDR refparam;
  };

/* VAL is a value of length LEN.  Store it into the argument area on the
   stack and load it into the corresponding general-purpose registers
   required by the ABI, and update ARGPOS.

   If ALIGN is nonzero, it specifies the minimum alignment required
   for the on-stack copy of the argument.  */

static void
ppc64_sysv_abi_push_val (struct gdbarch *gdbarch,
			 const bfd_byte *val, int len, int align,
			 struct ppc64_sysv_argpos *argpos)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int offset = 0;

  /* Enforce alignment of stack location, if requested.  */
  if (align > tdep->wordsize)
    {
      CORE_ADDR aligned_gparam = align_up (argpos->gparam, align);

      argpos->greg += (aligned_gparam - argpos->gparam) / tdep->wordsize;
      argpos->gparam = aligned_gparam;
    }

  /* The ABI (version 1.9) specifies that values smaller than one
     doubleword are right-aligned and those larger are left-aligned.
     GCC versions before 3.4 implemented this incorrectly; see
     <http://gcc.gnu.org/gcc-3.4/powerpc-abi.html>.  */
  if (len < tdep->wordsize
      && gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    offset = tdep->wordsize - len;

  if (argpos->regcache)
    write_memory (argpos->gparam + offset, val, len);
  argpos->gparam = align_up (argpos->gparam + len, tdep->wordsize);

  while (len >= tdep->wordsize)
    {
      if (argpos->regcache && argpos->greg <= 10)
	regcache_cooked_write (argpos->regcache,
			       tdep->ppc_gp0_regnum + argpos->greg, val);
      argpos->greg++;
      len -= tdep->wordsize;
      val += tdep->wordsize;
    }

  if (len > 0)
    {
      if (argpos->regcache && argpos->greg <= 10)
	regcache_cooked_write_part (argpos->regcache,
				    tdep->ppc_gp0_regnum + argpos->greg,
				    offset, len, val);
      argpos->greg++;
    }
}

/* The same as ppc64_sysv_abi_push_val, but using a single-word integer
   value VAL as argument.  */

static void
ppc64_sysv_abi_push_integer (struct gdbarch *gdbarch, ULONGEST val,
			     struct ppc64_sysv_argpos *argpos)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[MAX_REGISTER_SIZE];

  if (argpos->regcache)
    store_unsigned_integer (buf, tdep->wordsize, byte_order, val);
  ppc64_sysv_abi_push_val (gdbarch, buf, tdep->wordsize, 0, argpos);
}

/* VAL is a value of TYPE, a (binary or decimal) floating-point type.
   Load it into a floating-point register if required by the ABI,
   and update ARGPOS.  */

static void
ppc64_sysv_abi_push_freg (struct gdbarch *gdbarch,
			  struct type *type, const bfd_byte *val,
			  struct ppc64_sysv_argpos *argpos)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  if (tdep->soft_float)
    return;

  if (TYPE_LENGTH (type) <= 8
      && TYPE_CODE (type) == TYPE_CODE_FLT)
    {
      /* Floats and doubles go in f1 .. f13.  32-bit floats are converted
 	 to double first.  */
      if (argpos->regcache && argpos->freg <= 13)
	{
	  int regnum = tdep->ppc_fp0_regnum + argpos->freg;
	  struct type *regtype = register_type (gdbarch, regnum);
	  gdb_byte regval[MAX_REGISTER_SIZE];

	  convert_typed_floating (val, type, regval, regtype);
	  regcache_cooked_write (argpos->regcache, regnum, regval);
	}

      argpos->freg++;
    }
  else if (TYPE_LENGTH (type) <= 8
	   && TYPE_CODE (type) == TYPE_CODE_DECFLOAT)
    {
      /* Floats and doubles go in f1 .. f13.  32-bit decimal floats are
	 placed in the least significant word.  */
      if (argpos->regcache && argpos->freg <= 13)
	{
	  int regnum = tdep->ppc_fp0_regnum + argpos->freg;
	  int offset = 0;

	  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	    offset = 8 - TYPE_LENGTH (type);

	  regcache_cooked_write_part (argpos->regcache, regnum,
				      offset, TYPE_LENGTH (type), val);
	}

      argpos->freg++;
    }
  else if (TYPE_LENGTH (type) == 16
	   && TYPE_CODE (type) == TYPE_CODE_FLT
	   && (gdbarch_long_double_format (gdbarch)
	       == floatformats_ibm_long_double))
    {
      /* IBM long double stored in two consecutive FPRs.  */
      if (argpos->regcache && argpos->freg <= 13)
	{
	  int regnum = tdep->ppc_fp0_regnum + argpos->freg;

	  regcache_cooked_write (argpos->regcache, regnum, val);
	  if (argpos->freg <= 12)
	    regcache_cooked_write (argpos->regcache, regnum + 1, val + 8);
	}

      argpos->freg += 2;
    }
  else if (TYPE_LENGTH (type) == 16
	   && TYPE_CODE (type) == TYPE_CODE_DECFLOAT)
    {
      /* 128-bit decimal floating-point values are stored in and even/odd
	 pair of FPRs, with the even FPR holding the most significant half.  */
      argpos->freg += argpos->freg & 1;

      if (argpos->regcache && argpos->freg <= 12)
	{
	  int regnum = tdep->ppc_fp0_regnum + argpos->freg;
	  int lopart = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG ? 8 : 0;
	  int hipart = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG ? 0 : 8;

	  regcache_cooked_write (argpos->regcache, regnum, val + hipart);
	  regcache_cooked_write (argpos->regcache, regnum + 1, val + lopart);
	}

      argpos->freg += 2;
    }
}

/* VAL is a value of AltiVec vector type.  Load it into a vector register
   if required by the ABI, and update ARGPOS.  */

static void
ppc64_sysv_abi_push_vreg (struct gdbarch *gdbarch, const bfd_byte *val,
			  struct ppc64_sysv_argpos *argpos)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (argpos->regcache && argpos->vreg <= 13)
    regcache_cooked_write (argpos->regcache,
			   tdep->ppc_vr0_regnum + argpos->vreg, val);

  argpos->vreg++;
}

/* VAL is a value of TYPE.  Load it into memory and/or registers
   as required by the ABI, and update ARGPOS.  */

static void
ppc64_sysv_abi_push_param (struct gdbarch *gdbarch,
			   struct type *type, const bfd_byte *val,
			   struct ppc64_sysv_argpos *argpos)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (TYPE_CODE (type) == TYPE_CODE_FLT
      || TYPE_CODE (type) == TYPE_CODE_DECFLOAT)
    {
      /* Floating-point scalars are passed in floating-point registers.  */
      ppc64_sysv_abi_push_val (gdbarch, val, TYPE_LENGTH (type), 0, argpos);
      ppc64_sysv_abi_push_freg (gdbarch, type, val, argpos);
    }
  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type)
	   && tdep->vector_abi == POWERPC_VEC_ALTIVEC
	   && TYPE_LENGTH (type) == 16)
    {
      /* AltiVec vectors are passed aligned, and in vector registers.  */
      ppc64_sysv_abi_push_val (gdbarch, val, TYPE_LENGTH (type), 16, argpos);
      ppc64_sysv_abi_push_vreg (gdbarch, val, argpos);
    }
  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type)
	   && TYPE_LENGTH (type) >= 16)
    {
      /* Non-Altivec vectors are passed by reference.  */

      /* Copy value onto the stack ...  */
      CORE_ADDR addr = align_up (argpos->refparam, 16);
      if (argpos->regcache)
	write_memory (addr, val, TYPE_LENGTH (type));
      argpos->refparam = align_up (addr + TYPE_LENGTH (type), tdep->wordsize);

      /* ... and pass a pointer to the copy as parameter.  */
      ppc64_sysv_abi_push_integer (gdbarch, addr, argpos);
    }
  else if ((TYPE_CODE (type) == TYPE_CODE_INT
	    || TYPE_CODE (type) == TYPE_CODE_ENUM
	    || TYPE_CODE (type) == TYPE_CODE_BOOL
	    || TYPE_CODE (type) == TYPE_CODE_CHAR
	    || TYPE_CODE (type) == TYPE_CODE_PTR
	    || TYPE_IS_REFERENCE (type))
	   && TYPE_LENGTH (type) <= tdep->wordsize)
    {
      ULONGEST word = 0;

      if (argpos->regcache)
	{
	  /* Sign extend the value, then store it unsigned.  */
	  word = unpack_long (type, val);

	  /* Convert any function code addresses into descriptors.  */
	  if (tdep->elf_abi == POWERPC_ELF_V1
	      && (TYPE_CODE (type) == TYPE_CODE_PTR
		  || TYPE_CODE (type) == TYPE_CODE_REF))
	    {
	      struct type *target_type
		= check_typedef (TYPE_TARGET_TYPE (type));

	      if (TYPE_CODE (target_type) == TYPE_CODE_FUNC
		  || TYPE_CODE (target_type) == TYPE_CODE_METHOD)
		{
		  CORE_ADDR desc = word;

		  convert_code_addr_to_desc_addr (word, &desc);
		  word = desc;
		}
	    }
	}

      ppc64_sysv_abi_push_integer (gdbarch, word, argpos);
    }
  else
    {
      ppc64_sysv_abi_push_val (gdbarch, val, TYPE_LENGTH (type), 0, argpos);

      /* The ABI (version 1.9) specifies that structs containing a
	 single floating-point value, at any level of nesting of
	 single-member structs, are passed in floating-point registers.  */
      if (TYPE_CODE (type) == TYPE_CODE_STRUCT
	  && TYPE_NFIELDS (type) == 1)
	{
	  while (TYPE_CODE (type) == TYPE_CODE_STRUCT
		 && TYPE_NFIELDS (type) == 1)
	    type = check_typedef (TYPE_FIELD_TYPE (type, 0));

	  if (TYPE_CODE (type) == TYPE_CODE_FLT)
	    ppc64_sysv_abi_push_freg (gdbarch, type, val, argpos);
	}

      /* In the ELFv2 ABI, homogeneous floating-point or vector
	 aggregates are passed in a series of registers.  */
      if (tdep->elf_abi == POWERPC_ELF_V2)
	{
	  struct type *eltype;
	  int i, nelt;

	  if (ppc64_elfv2_abi_homogeneous_aggregate (type, &eltype, &nelt))
	    for (i = 0; i < nelt; i++)
	      {
		const gdb_byte *elval = val + i * TYPE_LENGTH (eltype);

		if (TYPE_CODE (eltype) == TYPE_CODE_FLT
		    || TYPE_CODE (eltype) == TYPE_CODE_DECFLOAT)
		  ppc64_sysv_abi_push_freg (gdbarch, eltype, elval, argpos);
		else if (TYPE_CODE (eltype) == TYPE_CODE_ARRAY
			 && TYPE_VECTOR (eltype)
			 && tdep->vector_abi == POWERPC_VEC_ALTIVEC
			 && TYPE_LENGTH (eltype) == 16)
		  ppc64_sysv_abi_push_vreg (gdbarch, elval, argpos);
	      }
	}
    }
}

/* Pass the arguments in either registers, or in the stack.  Using the
   ppc 64 bit SysV ABI.

   This implements a dumbed down version of the ABI.  It always writes
   values to memory, GPR and FPR, even when not necessary.  Doing this
   greatly simplifies the logic.  */

CORE_ADDR
ppc64_sysv_abi_push_dummy_call (struct gdbarch *gdbarch,
				struct value *function,
				struct regcache *regcache, CORE_ADDR bp_addr,
				int nargs, struct value **args, CORE_ADDR sp,
				int struct_return, CORE_ADDR struct_addr)
{
  CORE_ADDR func_addr = find_function_addr (function, NULL);
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int opencl_abi = ppc_sysv_use_opencl_abi (value_type (function));
  ULONGEST back_chain;
  /* See for-loop comment below.  */
  int write_pass;
  /* Size of the by-reference parameter copy region, the final value is
     computed in the for-loop below.  */
  LONGEST refparam_size = 0;
  /* Size of the general parameter region, the final value is computed
     in the for-loop below.  */
  LONGEST gparam_size = 0;
  /* Kevin writes ... I don't mind seeing tdep->wordsize used in the
     calls to align_up(), align_down(), etc. because this makes it
     easier to reuse this code (in a copy/paste sense) in the future,
     but it is a 64-bit ABI and asserting that the wordsize is 8 bytes
     at some point makes it easier to verify that this function is
     correct without having to do a non-local analysis to figure out
     the possible values of tdep->wordsize.  */
  gdb_assert (tdep->wordsize == 8);

  /* This function exists to support a calling convention that
     requires floating-point registers.  It shouldn't be used on
     processors that lack them.  */
  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  /* By this stage in the proceedings, SP has been decremented by "red
     zone size" + "struct return size".  Fetch the stack-pointer from
     before this and use that as the BACK_CHAIN.  */
  regcache_cooked_read_unsigned (regcache, gdbarch_sp_regnum (gdbarch),
				 &back_chain);

  /* Go through the argument list twice.

     Pass 1: Compute the function call's stack space and register
     requirements.

     Pass 2: Replay the same computation but this time also write the
     values out to the target.  */

  for (write_pass = 0; write_pass < 2; write_pass++)
    {
      int argno;

      struct ppc64_sysv_argpos argpos;
      argpos.greg = 3;
      argpos.freg = 1;
      argpos.vreg = 2;

      if (!write_pass)
	{
	  /* During the first pass, GPARAM and REFPARAM are more like
	     offsets (start address zero) than addresses.  That way
	     they accumulate the total stack space each region
	     requires.  */
	  argpos.regcache = NULL;
	  argpos.gparam = 0;
	  argpos.refparam = 0;
	}
      else
	{
	  /* Decrement the stack pointer making space for the Altivec
	     and general on-stack parameters.  Set refparam and gparam
	     to their corresponding regions.  */
	  argpos.regcache = regcache;
	  argpos.refparam = align_down (sp - refparam_size, 16);
	  argpos.gparam = align_down (argpos.refparam - gparam_size, 16);
	  /* Add in space for the TOC, link editor double word (v1 only),
	     compiler double word (v1 only), LR save area, CR save area,
	     and backchain.  */
	  if (tdep->elf_abi == POWERPC_ELF_V1)
	    sp = align_down (argpos.gparam - 48, 16);
	  else
	    sp = align_down (argpos.gparam - 32, 16);
	}

      /* If the function is returning a `struct', then there is an
         extra hidden parameter (which will be passed in r3)
         containing the address of that struct..  In that case we
         should advance one word and start from r4 register to copy
         parameters.  This also consumes one on-stack parameter slot.  */
      if (struct_return)
	ppc64_sysv_abi_push_integer (gdbarch, struct_addr, &argpos);

      for (argno = 0; argno < nargs; argno++)
	{
	  struct value *arg = args[argno];
	  struct type *type = check_typedef (value_type (arg));
	  const bfd_byte *val = value_contents (arg);

	  if (TYPE_CODE (type) == TYPE_CODE_COMPLEX)
	    {
	      /* Complex types are passed as if two independent scalars.  */
	      struct type *eltype = check_typedef (TYPE_TARGET_TYPE (type));

	      ppc64_sysv_abi_push_param (gdbarch, eltype, val, &argpos);
	      ppc64_sysv_abi_push_param (gdbarch, eltype,
				 	 val + TYPE_LENGTH (eltype), &argpos);
	    }
	  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY && TYPE_VECTOR (type)
		   && opencl_abi)
	    {
	      /* OpenCL vectors shorter than 16 bytes are passed as if
		 a series of independent scalars; OpenCL vectors 16 bytes
		 or longer are passed as if a series of AltiVec vectors.  */
	      struct type *eltype;
	      int i, nelt;

	      if (TYPE_LENGTH (type) < 16)
		eltype = check_typedef (TYPE_TARGET_TYPE (type));
	      else
		eltype = register_type (gdbarch, tdep->ppc_vr0_regnum);

	      nelt = TYPE_LENGTH (type) / TYPE_LENGTH (eltype);
	      for (i = 0; i < nelt; i++)
		{
		  const gdb_byte *elval = val + i * TYPE_LENGTH (eltype);

		  ppc64_sysv_abi_push_param (gdbarch, eltype, elval, &argpos);
		}
	    }
	  else
	    {
	      /* All other types are passed as single arguments.  */
	      ppc64_sysv_abi_push_param (gdbarch, type, val, &argpos);
	    }
	}

      if (!write_pass)
	{
	  /* Save the true region sizes ready for the second pass.  */
	  refparam_size = argpos.refparam;
	  /* Make certain that the general parameter save area is at
	     least the minimum 8 registers (or doublewords) in size.  */
	  if (argpos.greg < 8)
	    gparam_size = 8 * tdep->wordsize;
	  else
	    gparam_size = argpos.gparam;
	}
    }

  /* Update %sp.   */
  regcache_cooked_write_signed (regcache, gdbarch_sp_regnum (gdbarch), sp);

  /* Write the backchain (it occupies WORDSIZED bytes).  */
  write_memory_signed_integer (sp, tdep->wordsize, byte_order, back_chain);

  /* Point the inferior function call's return address at the dummy's
     breakpoint.  */
  regcache_cooked_write_signed (regcache, tdep->ppc_lr_regnum, bp_addr);

  /* In the ELFv1 ABI, use the func_addr to find the descriptor, and use
     that to find the TOC.  If we're calling via a function pointer,
     the pointer itself identifies the descriptor.  */
  if (tdep->elf_abi == POWERPC_ELF_V1)
    {
      struct type *ftype = check_typedef (value_type (function));
      CORE_ADDR desc_addr = value_as_address (function);

      if (TYPE_CODE (ftype) == TYPE_CODE_PTR
	  || convert_code_addr_to_desc_addr (func_addr, &desc_addr))
	{
	  /* The TOC is the second double word in the descriptor.  */
	  CORE_ADDR toc =
	    read_memory_unsigned_integer (desc_addr + tdep->wordsize,
					  tdep->wordsize, byte_order);

	  regcache_cooked_write_unsigned (regcache,
					  tdep->ppc_gp0_regnum + 2, toc);
	}
    }

  /* In the ELFv2 ABI, we need to pass the target address in r12 since
     we may be calling a global entry point.  */
  if (tdep->elf_abi == POWERPC_ELF_V2)
    regcache_cooked_write_unsigned (regcache,
				    tdep->ppc_gp0_regnum + 12, func_addr);

  return sp;
}

/* Subroutine of ppc64_sysv_abi_return_value that handles "base" types:
   integer, floating-point, and AltiVec vector types.

   This routine also handles components of aggregate return types;
   INDEX describes which part of the aggregate is to be handled.

   Returns true if VALTYPE is some such base type that could be handled,
   false otherwise.  */
static int
ppc64_sysv_abi_return_value_base (struct gdbarch *gdbarch, struct type *valtype,
				  struct regcache *regcache, gdb_byte *readbuf,
				  const gdb_byte *writebuf, int index)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Integers live in GPRs starting at r3.  */
  if ((TYPE_CODE (valtype) == TYPE_CODE_INT
       || TYPE_CODE (valtype) == TYPE_CODE_ENUM
       || TYPE_CODE (valtype) == TYPE_CODE_CHAR
       || TYPE_CODE (valtype) == TYPE_CODE_BOOL)
      && TYPE_LENGTH (valtype) <= 8)
    {
      int regnum = tdep->ppc_gp0_regnum + 3 + index;

      if (writebuf != NULL)
	{
	  /* Be careful to sign extend the value.  */
	  regcache_cooked_write_unsigned (regcache, regnum,
					  unpack_long (valtype, writebuf));
	}
      if (readbuf != NULL)
	{
	  /* Extract the integer from GPR.  Since this is truncating the
	     value, there isn't a sign extension problem.  */
	  ULONGEST regval;

	  regcache_cooked_read_unsigned (regcache, regnum, &regval);
	  store_unsigned_integer (readbuf, TYPE_LENGTH (valtype),
				  gdbarch_byte_order (gdbarch), regval);
	}
      return 1;
    }

  /* Floats and doubles go in f1 .. f13.  32-bit floats are converted
     to double first.  */
  if (TYPE_LENGTH (valtype) <= 8
      && TYPE_CODE (valtype) == TYPE_CODE_FLT)
    {
      int regnum = tdep->ppc_fp0_regnum + 1 + index;
      struct type *regtype = register_type (gdbarch, regnum);
      gdb_byte regval[MAX_REGISTER_SIZE];

      if (writebuf != NULL)
	{
	  convert_typed_floating (writebuf, valtype, regval, regtype);
	  regcache_cooked_write (regcache, regnum, regval);
	}
      if (readbuf != NULL)
	{
	  regcache_cooked_read (regcache, regnum, regval);
	  convert_typed_floating (regval, regtype, readbuf, valtype);
	}
      return 1;
    }

  /* Floats and doubles go in f1 .. f13.  32-bit decimal floats are
     placed in the least significant word.  */
  if (TYPE_LENGTH (valtype) <= 8
      && TYPE_CODE (valtype) == TYPE_CODE_DECFLOAT)
    {
      int regnum = tdep->ppc_fp0_regnum + 1 + index;
      int offset = 0;

      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	offset = 8 - TYPE_LENGTH (valtype);

      if (writebuf != NULL)
	regcache_cooked_write_part (regcache, regnum,
				    offset, TYPE_LENGTH (valtype), writebuf);
      if (readbuf != NULL)
	regcache_cooked_read_part (regcache, regnum,
				   offset, TYPE_LENGTH (valtype), readbuf);
      return 1;
    }

  /* IBM long double stored in two consecutive FPRs.  */
  if (TYPE_LENGTH (valtype) == 16
      && TYPE_CODE (valtype) == TYPE_CODE_FLT
      && (gdbarch_long_double_format (gdbarch)
	  == floatformats_ibm_long_double))
    {
      int regnum = tdep->ppc_fp0_regnum + 1 + 2 * index;

      if (writebuf != NULL)
	{
	  regcache_cooked_write (regcache, regnum, writebuf);
	  regcache_cooked_write (regcache, regnum + 1, writebuf + 8);
	}
      if (readbuf != NULL)
	{
	  regcache_cooked_read (regcache, regnum, readbuf);
	  regcache_cooked_read (regcache, regnum + 1, readbuf + 8);
	}
      return 1;
    }

  /* 128-bit decimal floating-point values are stored in an even/odd
     pair of FPRs, with the even FPR holding the most significant half.  */
  if (TYPE_LENGTH (valtype) == 16
      && TYPE_CODE (valtype) == TYPE_CODE_DECFLOAT)
    {
      int regnum = tdep->ppc_fp0_regnum + 2 + 2 * index;
      int lopart = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG ? 8 : 0;
      int hipart = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG ? 0 : 8;

      if (writebuf != NULL)
	{
	  regcache_cooked_write (regcache, regnum, writebuf + hipart);
	  regcache_cooked_write (regcache, regnum + 1, writebuf + lopart);
	}
      if (readbuf != NULL)
	{
	  regcache_cooked_read (regcache, regnum, readbuf + hipart);
	  regcache_cooked_read (regcache, regnum + 1, readbuf + lopart);
	}
      return 1;
    }

  /* AltiVec vectors are returned in VRs starting at v2.  */
  if (TYPE_LENGTH (valtype) == 16
      && TYPE_CODE (valtype) == TYPE_CODE_ARRAY && TYPE_VECTOR (valtype)
      && tdep->vector_abi == POWERPC_VEC_ALTIVEC)
    {
      int regnum = tdep->ppc_vr0_regnum + 2 + index;

      if (writebuf != NULL)
	regcache_cooked_write (regcache, regnum, writebuf);
      if (readbuf != NULL)
	regcache_cooked_read (regcache, regnum, readbuf);
      return 1;
    }

  /* Short vectors are returned in GPRs starting at r3.  */
  if (TYPE_LENGTH (valtype) <= 8
      && TYPE_CODE (valtype) == TYPE_CODE_ARRAY && TYPE_VECTOR (valtype))
    {
      int regnum = tdep->ppc_gp0_regnum + 3 + index;
      int offset = 0;

      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	offset = 8 - TYPE_LENGTH (valtype);

      if (writebuf != NULL)
	regcache_cooked_write_part (regcache, regnum,
				    offset, TYPE_LENGTH (valtype), writebuf);
      if (readbuf != NULL)
	regcache_cooked_read_part (regcache, regnum,
				   offset, TYPE_LENGTH (valtype), readbuf);
      return 1;
    }

  return 0;
}

/* The 64 bit ABI return value convention.

   Return non-zero if the return-value is stored in a register, return
   0 if the return-value is instead stored on the stack (a.k.a.,
   struct return convention).

   For a return-value stored in a register: when WRITEBUF is non-NULL,
   copy the buffer to the corresponding register return-value location
   location; when READBUF is non-NULL, fill the buffer from the
   corresponding register return-value location.  */
enum return_value_convention
ppc64_sysv_abi_return_value (struct gdbarch *gdbarch, struct value *function,
			     struct type *valtype, struct regcache *regcache,
			     gdb_byte *readbuf, const gdb_byte *writebuf)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  struct type *func_type = function ? value_type (function) : NULL;
  int opencl_abi = func_type? ppc_sysv_use_opencl_abi (func_type) : 0;
  struct type *eltype;
  int nelt, i, ok;

  /* This function exists to support a calling convention that
     requires floating-point registers.  It shouldn't be used on
     processors that lack them.  */
  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  /* Complex types are returned as if two independent scalars.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_COMPLEX)
    {
      eltype = check_typedef (TYPE_TARGET_TYPE (valtype));

      for (i = 0; i < 2; i++)
	{
	  ok = ppc64_sysv_abi_return_value_base (gdbarch, eltype, regcache,
						 readbuf, writebuf, i);
	  gdb_assert (ok);

	  if (readbuf)
	    readbuf += TYPE_LENGTH (eltype);
	  if (writebuf)
	    writebuf += TYPE_LENGTH (eltype);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  /* OpenCL vectors shorter than 16 bytes are returned as if
     a series of independent scalars; OpenCL vectors 16 bytes
     or longer are returned as if a series of AltiVec vectors.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_ARRAY && TYPE_VECTOR (valtype)
      && opencl_abi)
    {
      if (TYPE_LENGTH (valtype) < 16)
	eltype = check_typedef (TYPE_TARGET_TYPE (valtype));
      else
	eltype = register_type (gdbarch, tdep->ppc_vr0_regnum);

      nelt = TYPE_LENGTH (valtype) / TYPE_LENGTH (eltype);
      for (i = 0; i < nelt; i++)
	{
	  ok = ppc64_sysv_abi_return_value_base (gdbarch, eltype, regcache,
						 readbuf, writebuf, i);
	  gdb_assert (ok);

	  if (readbuf)
	    readbuf += TYPE_LENGTH (eltype);
	  if (writebuf)
	    writebuf += TYPE_LENGTH (eltype);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  /* All pointers live in r3.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_PTR || TYPE_IS_REFERENCE (valtype))
    {
      int regnum = tdep->ppc_gp0_regnum + 3;

      if (writebuf != NULL)
	regcache_cooked_write (regcache, regnum, writebuf);
      if (readbuf != NULL)
	regcache_cooked_read (regcache, regnum, readbuf);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  /* Small character arrays are returned, right justified, in r3.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_ARRAY
      && !TYPE_VECTOR (valtype)
      && TYPE_LENGTH (valtype) <= 8
      && TYPE_CODE (TYPE_TARGET_TYPE (valtype)) == TYPE_CODE_INT
      && TYPE_LENGTH (TYPE_TARGET_TYPE (valtype)) == 1)
    {
      int regnum = tdep->ppc_gp0_regnum + 3;
      int offset = (register_size (gdbarch, regnum) - TYPE_LENGTH (valtype));

      if (writebuf != NULL)
	regcache_cooked_write_part (regcache, regnum,
				    offset, TYPE_LENGTH (valtype), writebuf);
      if (readbuf != NULL)
	regcache_cooked_read_part (regcache, regnum,
				   offset, TYPE_LENGTH (valtype), readbuf);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  /* In the ELFv2 ABI, homogeneous floating-point or vector
     aggregates are returned in registers.  */
  if (tdep->elf_abi == POWERPC_ELF_V2
      && ppc64_elfv2_abi_homogeneous_aggregate (valtype, &eltype, &nelt)
      && (TYPE_CODE (eltype) == TYPE_CODE_FLT
	  || TYPE_CODE (eltype) == TYPE_CODE_DECFLOAT
	  || (TYPE_CODE (eltype) == TYPE_CODE_ARRAY
	      && TYPE_VECTOR (eltype)
	      && tdep->vector_abi == POWERPC_VEC_ALTIVEC
	      && TYPE_LENGTH (eltype) == 16)))
    {
      for (i = 0; i < nelt; i++)
	{
	  ok = ppc64_sysv_abi_return_value_base (gdbarch, eltype, regcache,
						 readbuf, writebuf, i);
	  gdb_assert (ok);

	  if (readbuf)
	    readbuf += TYPE_LENGTH (eltype);
	  if (writebuf)
	    writebuf += TYPE_LENGTH (eltype);
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  /* In the ELFv2 ABI, aggregate types of up to 16 bytes are
     returned in registers r3:r4.  */
  if (tdep->elf_abi == POWERPC_ELF_V2
      && TYPE_LENGTH (valtype) <= 16
      && (TYPE_CODE (valtype) == TYPE_CODE_STRUCT
	  || TYPE_CODE (valtype) == TYPE_CODE_UNION
	  || (TYPE_CODE (valtype) == TYPE_CODE_ARRAY
	      && !TYPE_VECTOR (valtype))))
    {
      int n_regs = ((TYPE_LENGTH (valtype) + tdep->wordsize - 1)
		    / tdep->wordsize);
      int i;

      for (i = 0; i < n_regs; i++)
	{
	  gdb_byte regval[MAX_REGISTER_SIZE];
	  int regnum = tdep->ppc_gp0_regnum + 3 + i;
	  int offset = i * tdep->wordsize;
	  int len = TYPE_LENGTH (valtype) - offset;

	  if (len > tdep->wordsize)
	    len = tdep->wordsize;

	  if (writebuf != NULL)
	    {
	      memset (regval, 0, sizeof regval);
	      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
		  && offset == 0)
		memcpy (regval + tdep->wordsize - len, writebuf, len);
	      else
		memcpy (regval, writebuf + offset, len);
	      regcache_cooked_write (regcache, regnum, regval);
	    }
	  if (readbuf != NULL)
	    {
	      regcache_cooked_read (regcache, regnum, regval);
	      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
		  && offset == 0)
		memcpy (readbuf, regval + tdep->wordsize - len, len);
	      else
		memcpy (readbuf + offset, regval, len);
	    }
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  /* Handle plain base types.  */
  if (ppc64_sysv_abi_return_value_base (gdbarch, valtype, regcache,
					readbuf, writebuf, 0))
    return RETURN_VALUE_REGISTER_CONVENTION;

  return RETURN_VALUE_STRUCT_CONVENTION;
}

