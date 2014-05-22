/* Target-dependent code for PowerPC systems using the SVR4 ABI
   for GDB, the GNU debugger.

   Copyright (C) 2000-2013 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "gdb_assert.h"
#include "ppc-tdep.h"
#include "target.h"
#include "objfiles.h"
#include "infcall.h"
#include "dwarf2.h"


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

/* Handle the return-value conventions for Decimal Floating Point values
   in both ppc32 and ppc64, which are the same.  */
static int
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
	    || TYPE_CODE (type) == TYPE_CODE_REF
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
  struct minimal_symbol *dot_fn;
  struct minimal_symbol *fn;
  /* Find the minimal symbol that corresponds to CODE_ADDR (should
     have a name of the form ".FN").  */
  dot_fn = lookup_minimal_symbol_by_pc (code_addr);
  if (dot_fn == NULL || SYMBOL_LINKAGE_NAME (dot_fn)[0] != '.')
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
  fn = lookup_minimal_symbol (SYMBOL_LINKAGE_NAME (dot_fn) + 1, NULL,
			      dot_fn_section->objfile);
  if (fn == NULL)
    return 0;
  /* Found a descriptor.  */
  (*desc_addr) = SYMBOL_VALUE_ADDRESS (fn);
  return 1;
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
      /* Next available floating point register for float and double
         arguments.  */
      int freg = 1;
      /* Next available general register for non-vector (but possibly
         float) arguments.  */
      int greg = 3;
      /* Next available vector register for vector arguments.  */
      int vreg = 2;
      /* The address, at which the next general purpose parameter
         (integer, struct, float, vector, ...) should be saved.  */
      CORE_ADDR gparam;
      /* The address, at which the next by-reference parameter
	 (non-Altivec vector, variably-sized type) should be saved.  */
      CORE_ADDR refparam;

      if (!write_pass)
	{
	  /* During the first pass, GPARAM and REFPARAM are more like
	     offsets (start address zero) than addresses.  That way
	     they accumulate the total stack space each region
	     requires.  */
	  gparam = 0;
	  refparam = 0;
	}
      else
	{
	  /* Decrement the stack pointer making space for the Altivec
	     and general on-stack parameters.  Set refparam and gparam
	     to their corresponding regions.  */
	  refparam = align_down (sp - refparam_size, 16);
	  gparam = align_down (refparam - gparam_size, 16);
	  /* Add in space for the TOC, link editor double word,
	     compiler double word, LR save area, CR save area.  */
	  sp = align_down (gparam - 48, 16);
	}

      /* If the function is returning a `struct', then there is an
         extra hidden parameter (which will be passed in r3)
         containing the address of that struct..  In that case we
         should advance one word and start from r4 register to copy
         parameters.  This also consumes one on-stack parameter slot.  */
      if (struct_return)
	{
	  if (write_pass)
	    regcache_cooked_write_signed (regcache,
					  tdep->ppc_gp0_regnum + greg,
					  struct_addr);
	  greg++;
	  gparam = align_up (gparam + tdep->wordsize, tdep->wordsize);
	}

      for (argno = 0; argno < nargs; argno++)
	{
	  struct value *arg = args[argno];
	  struct type *type = check_typedef (value_type (arg));
	  const bfd_byte *val = value_contents (arg);

	  if (TYPE_CODE (type) == TYPE_CODE_FLT && TYPE_LENGTH (type) <= 8)
	    {
	      /* Floats and Doubles go in f1 .. f13.  They also
	         consume a left aligned GREG,, and can end up in
	         memory.  */
	      if (write_pass)
		{
		  gdb_byte regval[MAX_REGISTER_SIZE];
		  const gdb_byte *p;

		  /* Version 1.7 of the 64-bit PowerPC ELF ABI says:

		     "Single precision floating point values are mapped to
		     the first word in a single doubleword."

		     And version 1.9 says:

		     "Single precision floating point values are mapped to
		     the second word in a single doubleword."

		     GDB then writes single precision floating point values
		     at both words in a doubleword, to support both ABIs.  */
		  if (TYPE_LENGTH (type) == 4)
		    {
		      memcpy (regval, val, 4);
		      memcpy (regval + 4, val, 4);
		      p = regval;
		    }
		  else
		    p = val;

		  /* Write value in the stack's parameter save area.  */
		  write_memory (gparam, p, 8);

		  if (freg <= 13)
		    {
		      struct type *regtype
                        = register_type (gdbarch, tdep->ppc_fp0_regnum);

		      convert_typed_floating (val, type, regval, regtype);
		      regcache_cooked_write (regcache,
                                             tdep->ppc_fp0_regnum + freg,
					     regval);
		    }
		  if (greg <= 10)
		    regcache_cooked_write (regcache,
					   tdep->ppc_gp0_regnum + greg,
					   regval);
		}

	      freg++;
	      greg++;
	      /* Always consume parameter stack space.  */
	      gparam = align_up (gparam + 8, tdep->wordsize);
	    }
	  else if (TYPE_CODE (type) == TYPE_CODE_FLT
		   && TYPE_LENGTH (type) == 16
		   && (gdbarch_long_double_format (gdbarch)
		       == floatformats_ibm_long_double))
	    {
	      /* IBM long double stored in two doublewords of the
		 parameter save area and corresponding registers.  */
	      if (write_pass)
		{
		  if (!tdep->soft_float && freg <= 13)
		    {
		      regcache_cooked_write (regcache,
                                             tdep->ppc_fp0_regnum + freg,
					     val);
		      if (freg <= 12)
			regcache_cooked_write (regcache,
					       tdep->ppc_fp0_regnum + freg + 1,
					       val + 8);
		    }
		  if (greg <= 10)
		    {
		      regcache_cooked_write (regcache,
					     tdep->ppc_gp0_regnum + greg,
					     val);
		      if (greg <= 9)
			regcache_cooked_write (regcache,
					       tdep->ppc_gp0_regnum + greg + 1,
					       val + 8);
		    }
		  write_memory (gparam, val, TYPE_LENGTH (type));
		}
	      freg += 2;
	      greg += 2;
	      gparam = align_up (gparam + TYPE_LENGTH (type), tdep->wordsize);
	    }
	  else if (TYPE_CODE (type) == TYPE_CODE_DECFLOAT
		   && TYPE_LENGTH (type) <= 8)
	    {
	      /* 32-bit and 64-bit decimal floats go in f1 .. f13.  They can
	         end up in memory.  */
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

		  /* Write value in the stack's parameter save area.  */
		  write_memory (gparam, p, 8);

		  if (freg <= 13)
		    regcache_cooked_write (regcache,
					   tdep->ppc_fp0_regnum + freg, p);
		}

	      freg++;
	      greg++;
	      /* Always consume parameter stack space.  */
	      gparam = align_up (gparam + 8, tdep->wordsize);
	    }
	  else if (TYPE_CODE (type) == TYPE_CODE_DECFLOAT &&
		   TYPE_LENGTH (type) == 16)
	    {
	      /* 128-bit decimal floats go in f2 .. f12, always in even/odd
	         pairs.  They can end up in memory, using two doublewords.  */
	      if (write_pass)
		{
		  if (freg <= 12)
		    {
		      /* Make sure freg is even.  */
		      freg += freg & 1;
		      regcache_cooked_write (regcache,
                                             tdep->ppc_fp0_regnum + freg, val);
		      regcache_cooked_write (regcache,
			  tdep->ppc_fp0_regnum + freg + 1, val + 8);
		    }

		  write_memory (gparam, val, TYPE_LENGTH (type));
		}

	      freg += 2;
	      greg += 2;
	      gparam = align_up (gparam + TYPE_LENGTH (type), tdep->wordsize);
	    }
	  else if (TYPE_LENGTH (type) < 16
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

		  if (TYPE_CODE (eltype) == TYPE_CODE_FLT)
		    {
		      if (write_pass)
			{
			  gdb_byte regval[MAX_REGISTER_SIZE];
			  const gdb_byte *p;

			  if (TYPE_LENGTH (eltype) == 4)
			    {
			      memcpy (regval, elval, 4);
			      memcpy (regval + 4, elval, 4);
			      p = regval;
			    }
			  else
			    p = elval;

			  write_memory (gparam, p, 8);

			  if (freg <= 13)
			    {
			      int regnum = tdep->ppc_fp0_regnum + freg;
			      struct type *regtype
				= register_type (gdbarch, regnum);

			      convert_typed_floating (elval, eltype,
						      regval, regtype);
			      regcache_cooked_write (regcache, regnum, regval);
			    }

			  if (greg <= 10)
			    regcache_cooked_write (regcache,
						   tdep->ppc_gp0_regnum + greg,
						   regval);
			}

		      freg++;
		      greg++;
		      gparam = align_up (gparam + 8, tdep->wordsize);
		    }
		  else
		    {
		      if (write_pass)
			{
			  ULONGEST word = unpack_long (eltype, elval);
			  if (greg <= 10)
			    regcache_cooked_write_unsigned
			      (regcache, tdep->ppc_gp0_regnum + greg, word);

			  write_memory_unsigned_integer
			    (gparam, tdep->wordsize, byte_order, word);
			}

		      greg++;
		      gparam = align_up (gparam + TYPE_LENGTH (eltype),
					 tdep->wordsize);
		    }
		}
	    }
	  else if (TYPE_LENGTH (type) >= 16
		   && TYPE_CODE (type) == TYPE_CODE_ARRAY
		   && TYPE_VECTOR (type)
		   && opencl_abi)
	    {
	      /* OpenCL vectors 16 bytes or longer are passed as if
		 a series of AltiVec vectors.  */
	      int i;

	      for (i = 0; i < TYPE_LENGTH (type) / 16; i++)
		{
		  const gdb_byte *elval = val + i * 16;

		  gparam = align_up (gparam, 16);
		  greg += greg & 1;

		  if (write_pass)
		    {
		      if (vreg <= 13)
			regcache_cooked_write (regcache,
					       tdep->ppc_vr0_regnum + vreg,
					       elval);

		      write_memory (gparam, elval, 16);
		    }

		  greg += 2;
		  vreg++;
		  gparam += 16;
		}
	    }
	  else if (TYPE_LENGTH (type) == 16 && TYPE_VECTOR (type)
		   && TYPE_CODE (type) == TYPE_CODE_ARRAY
		   && tdep->vector_abi == POWERPC_VEC_ALTIVEC)
	    {
	      /* In the Altivec ABI, vectors go in the vector registers
		 v2 .. v13, as well as the parameter area -- always at
		 16-byte aligned addresses.  */

	      gparam = align_up (gparam, 16);
	      greg += greg & 1;

	      if (write_pass)
		{
		  if (vreg <= 13)
		    regcache_cooked_write (regcache,
					   tdep->ppc_vr0_regnum + vreg, val);

		  write_memory (gparam, val, TYPE_LENGTH (type));
		}

	      greg += 2;
	      vreg++;
	      gparam += 16;
	    }
	  else if (TYPE_LENGTH (type) >= 16 && TYPE_VECTOR (type)
		   && TYPE_CODE (type) == TYPE_CODE_ARRAY)
	    {
	      /* Non-Altivec vectors are passed by reference.  */

	      /* Copy value onto the stack ...  */
	      refparam = align_up (refparam, 16);
	      if (write_pass)
		write_memory (refparam, val, TYPE_LENGTH (type));

	      /* ... and pass a pointer to the copy as parameter.  */
	      if (write_pass)
		{
		  if (greg <= 10)
		    regcache_cooked_write_unsigned (regcache,
						    tdep->ppc_gp0_regnum +
						    greg, refparam);
		  write_memory_unsigned_integer (gparam, tdep->wordsize,
						 byte_order, refparam);
		}
	      greg++;
	      gparam = align_up (gparam + tdep->wordsize, tdep->wordsize);
	      refparam = align_up (refparam + TYPE_LENGTH (type), tdep->wordsize);
	    }
	  else if ((TYPE_CODE (type) == TYPE_CODE_INT
		    || TYPE_CODE (type) == TYPE_CODE_ENUM
		    || TYPE_CODE (type) == TYPE_CODE_BOOL
		    || TYPE_CODE (type) == TYPE_CODE_CHAR
		    || TYPE_CODE (type) == TYPE_CODE_PTR
		    || TYPE_CODE (type) == TYPE_CODE_REF)
		   && TYPE_LENGTH (type) <= 8)
	    {
	      /* Scalars and Pointers get sign[un]extended and go in
	         gpr3 .. gpr10.  They can also end up in memory.  */
	      if (write_pass)
		{
		  /* Sign extend the value, then store it unsigned.  */
		  ULONGEST word = unpack_long (type, val);
		  /* Convert any function code addresses into
		     descriptors.  */
		  if (TYPE_CODE (type) == TYPE_CODE_PTR
		      || TYPE_CODE (type) == TYPE_CODE_REF)
		    {
		      struct type *target_type;
		      target_type = check_typedef (TYPE_TARGET_TYPE (type));

		      if (TYPE_CODE (target_type) == TYPE_CODE_FUNC
			  || TYPE_CODE (target_type) == TYPE_CODE_METHOD)
			{
			  CORE_ADDR desc = word;
			  convert_code_addr_to_desc_addr (word, &desc);
			  word = desc;
			}
		    }
		  if (greg <= 10)
		    regcache_cooked_write_unsigned (regcache,
						    tdep->ppc_gp0_regnum +
						    greg, word);
		  write_memory_unsigned_integer (gparam, tdep->wordsize,
						 byte_order, word);
		}
	      greg++;
	      gparam = align_up (gparam + TYPE_LENGTH (type), tdep->wordsize);
	    }
	  else
	    {
	      int byte;
	      for (byte = 0; byte < TYPE_LENGTH (type);
		   byte += tdep->wordsize)
		{
		  if (write_pass && greg <= 10)
		    {
		      gdb_byte regval[MAX_REGISTER_SIZE];
		      int len = TYPE_LENGTH (type) - byte;
		      if (len > tdep->wordsize)
			len = tdep->wordsize;
		      memset (regval, 0, sizeof regval);
		      /* The ABI (version 1.9) specifies that values
			 smaller than one doubleword are right-aligned
			 and those larger are left-aligned.  GCC
			 versions before 3.4 implemented this
			 incorrectly; see
			 <http://gcc.gnu.org/gcc-3.4/powerpc-abi.html>.  */
		      if (byte == 0)
			memcpy (regval + tdep->wordsize - len,
				val + byte, len);
		      else
			memcpy (regval, val + byte, len);
		      regcache_cooked_write (regcache, greg, regval);
		    }
		  greg++;
		}
	      if (write_pass)
		{
		  /* WARNING: cagney/2003-09-21: Strictly speaking, this
		     isn't necessary, unfortunately, GCC appears to get
		     "struct convention" parameter passing wrong putting
		     odd sized structures in memory instead of in a
		     register.  Work around this by always writing the
		     value to memory.  Fortunately, doing this
		     simplifies the code.  */
		  int len = TYPE_LENGTH (type);
		  if (len < tdep->wordsize)
		    write_memory (gparam + tdep->wordsize - len, val, len);
		  else
		    write_memory (gparam, val, len);
		}
	      if (freg <= 13
		  && TYPE_CODE (type) == TYPE_CODE_STRUCT
		  && TYPE_NFIELDS (type) == 1
		  && TYPE_LENGTH (type) <= 16)
		{
		  /* The ABI (version 1.9) specifies that structs
		     containing a single floating-point value, at any
		     level of nesting of single-member structs, are
		     passed in floating-point registers.  */
		  while (TYPE_CODE (type) == TYPE_CODE_STRUCT
			 && TYPE_NFIELDS (type) == 1)
		    type = check_typedef (TYPE_FIELD_TYPE (type, 0));
		  if (TYPE_CODE (type) == TYPE_CODE_FLT)
		    {
		      if (TYPE_LENGTH (type) <= 8)
			{
			  if (write_pass)
			    {
			      gdb_byte regval[MAX_REGISTER_SIZE];
			      struct type *regtype
				= register_type (gdbarch,
						 tdep->ppc_fp0_regnum);
			      convert_typed_floating (val, type, regval,
						      regtype);
			      regcache_cooked_write (regcache,
						     (tdep->ppc_fp0_regnum
						      + freg),
						     regval);
			    }
			  freg++;
			}
		      else if (TYPE_LENGTH (type) == 16
			       && (gdbarch_long_double_format (gdbarch)
				   == floatformats_ibm_long_double))
			{
			  if (write_pass)
			    {
			      regcache_cooked_write (regcache,
						     (tdep->ppc_fp0_regnum
						      + freg),
						     val);
			      if (freg <= 12)
				regcache_cooked_write (regcache,
						       (tdep->ppc_fp0_regnum
							+ freg + 1),
						       val + 8);
			    }
			  freg += 2;
			}
		    }
		}
	      /* Always consume parameter stack space.  */
	      gparam = align_up (gparam + TYPE_LENGTH (type), tdep->wordsize);
	    }
	}

      if (!write_pass)
	{
	  /* Save the true region sizes ready for the second pass.  */
	  refparam_size = refparam;
	  /* Make certain that the general parameter save area is at
	     least the minimum 8 registers (or doublewords) in size.  */
	  if (greg < 8)
	    gparam_size = 8 * tdep->wordsize;
	  else
	    gparam_size = gparam;
	}
    }

  /* Update %sp.   */
  regcache_cooked_write_signed (regcache, gdbarch_sp_regnum (gdbarch), sp);

  /* Write the backchain (it occupies WORDSIZED bytes).  */
  write_memory_signed_integer (sp, tdep->wordsize, byte_order, back_chain);

  /* Point the inferior function call's return address at the dummy's
     breakpoint.  */
  regcache_cooked_write_signed (regcache, tdep->ppc_lr_regnum, bp_addr);

  /* Use the func_addr to find the descriptor, and use that to find
     the TOC.  If we're calling via a function pointer, the pointer
     itself identifies the descriptor.  */
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

  return sp;
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
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct type *func_type = function ? value_type (function) : NULL;
  int opencl_abi = func_type? ppc_sysv_use_opencl_abi (func_type) : 0;

  /* This function exists to support a calling convention that
     requires floating-point registers.  It shouldn't be used on
     processors that lack them.  */
  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  /* Floats and doubles in F1.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_FLT && TYPE_LENGTH (valtype) <= 8)
    {
      gdb_byte regval[MAX_REGISTER_SIZE];
      struct type *regtype = register_type (gdbarch, tdep->ppc_fp0_regnum);
      if (writebuf != NULL)
	{
	  convert_typed_floating (writebuf, valtype, regval, regtype);
	  regcache_cooked_write (regcache, tdep->ppc_fp0_regnum + 1, regval);
	}
      if (readbuf != NULL)
	{
	  regcache_cooked_read (regcache, tdep->ppc_fp0_regnum + 1, regval);
	  convert_typed_floating (regval, regtype, readbuf, valtype);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  if (TYPE_CODE (valtype) == TYPE_CODE_DECFLOAT)
    return get_decimal_float_return_value (gdbarch, valtype, regcache, readbuf,
					   writebuf);
  /* Integers in r3.  */
  if ((TYPE_CODE (valtype) == TYPE_CODE_INT
       || TYPE_CODE (valtype) == TYPE_CODE_ENUM
       || TYPE_CODE (valtype) == TYPE_CODE_CHAR
       || TYPE_CODE (valtype) == TYPE_CODE_BOOL)
      && TYPE_LENGTH (valtype) <= 8)
    {
      if (writebuf != NULL)
	{
	  /* Be careful to sign extend the value.  */
	  regcache_cooked_write_unsigned (regcache, tdep->ppc_gp0_regnum + 3,
					  unpack_long (valtype, writebuf));
	}
      if (readbuf != NULL)
	{
	  /* Extract the integer from r3.  Since this is truncating the
	     value, there isn't a sign extension problem.  */
	  ULONGEST regval;
	  regcache_cooked_read_unsigned (regcache, tdep->ppc_gp0_regnum + 3,
					 &regval);
	  store_unsigned_integer (readbuf, TYPE_LENGTH (valtype), byte_order,
				  regval);
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  /* All pointers live in r3.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_PTR
      || TYPE_CODE (valtype) == TYPE_CODE_REF)
    {
      /* All pointers live in r3.  */
      if (writebuf != NULL)
	regcache_cooked_write (regcache, tdep->ppc_gp0_regnum + 3, writebuf);
      if (readbuf != NULL)
	regcache_cooked_read (regcache, tdep->ppc_gp0_regnum + 3, readbuf);
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  /* OpenCL vectors < 16 bytes are returned as distinct
     scalars in f1..f2 or r3..r10.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_ARRAY
      && TYPE_VECTOR (valtype)
      && TYPE_LENGTH (valtype) < 16
      && opencl_abi)
    {
      struct type *eltype = check_typedef (TYPE_TARGET_TYPE (valtype));
      int i, nelt = TYPE_LENGTH (valtype) / TYPE_LENGTH (eltype);

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
  if (TYPE_CODE (valtype) == TYPE_CODE_ARRAY
      && TYPE_VECTOR (valtype)
      && TYPE_LENGTH (valtype) >= 16
      && opencl_abi)
    {
      int n_regs = TYPE_LENGTH (valtype) / 16;
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
  /* Array type has more than one use.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_ARRAY)
    {
      /* Small character arrays are returned, right justified, in r3.  */
      if (TYPE_LENGTH (valtype) <= 8
        && TYPE_CODE (TYPE_TARGET_TYPE (valtype)) == TYPE_CODE_INT
        && TYPE_LENGTH (TYPE_TARGET_TYPE (valtype)) == 1)
        {
          int offset = (register_size (gdbarch, tdep->ppc_gp0_regnum + 3)
                       - TYPE_LENGTH (valtype));
          if (writebuf != NULL)
           regcache_cooked_write_part (regcache, tdep->ppc_gp0_regnum + 3,
                                      offset, TYPE_LENGTH (valtype), writebuf);
          if (readbuf != NULL)
           regcache_cooked_read_part (regcache, tdep->ppc_gp0_regnum + 3,
                                      offset, TYPE_LENGTH (valtype), readbuf);
          return RETURN_VALUE_REGISTER_CONVENTION;
	}
      /* A VMX vector is returned in v2.  */
      if (TYPE_CODE (valtype) == TYPE_CODE_ARRAY
	  && TYPE_VECTOR (valtype)
	  && tdep->vector_abi == POWERPC_VEC_ALTIVEC)
        {
          if (readbuf)
            regcache_cooked_read (regcache, tdep->ppc_vr0_regnum + 2, readbuf);
          if (writebuf)
            regcache_cooked_write (regcache, tdep->ppc_vr0_regnum + 2,
				   writebuf);
          return RETURN_VALUE_REGISTER_CONVENTION;
        }
    }
  /* Big floating point values get stored in adjacent floating
     point registers, starting with F1.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_FLT
      && (TYPE_LENGTH (valtype) == 16 || TYPE_LENGTH (valtype) == 32))
    {
      if (writebuf || readbuf != NULL)
	{
	  int i;
	  for (i = 0; i < TYPE_LENGTH (valtype) / 8; i++)
	    {
	      if (writebuf != NULL)
		regcache_cooked_write (regcache, tdep->ppc_fp0_regnum + 1 + i,
				       (const bfd_byte *) writebuf + i * 8);
	      if (readbuf != NULL)
		regcache_cooked_read (regcache, tdep->ppc_fp0_regnum + 1 + i,
				      (bfd_byte *) readbuf + i * 8);
	    }
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  /* Complex values get returned in f1:f2, need to convert.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_COMPLEX
      && (TYPE_LENGTH (valtype) == 8 || TYPE_LENGTH (valtype) == 16))
    {
      if (regcache != NULL)
	{
	  int i;
	  for (i = 0; i < 2; i++)
	    {
	      gdb_byte regval[MAX_REGISTER_SIZE];
	      struct type *regtype =
		register_type (gdbarch, tdep->ppc_fp0_regnum);
	      struct type *target_type;
	      target_type = check_typedef (TYPE_TARGET_TYPE (valtype));
	      if (writebuf != NULL)
		{
		  convert_typed_floating ((const bfd_byte *) writebuf +
					  i * TYPE_LENGTH (target_type), 
					  target_type, regval, regtype);
		  regcache_cooked_write (regcache,
                                         tdep->ppc_fp0_regnum + 1 + i,
					 regval);
		}
	      if (readbuf != NULL)
		{
		  regcache_cooked_read (regcache,
                                        tdep->ppc_fp0_regnum + 1 + i,
                                        regval);
		  convert_typed_floating (regval, regtype,
					  (bfd_byte *) readbuf +
					  i * TYPE_LENGTH (target_type),
					  target_type);
		}
	    }
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  /* Big complex values get stored in f1:f4.  */
  if (TYPE_CODE (valtype) == TYPE_CODE_COMPLEX && TYPE_LENGTH (valtype) == 32)
    {
      if (regcache != NULL)
	{
	  int i;
	  for (i = 0; i < 4; i++)
	    {
	      if (writebuf != NULL)
		regcache_cooked_write (regcache, tdep->ppc_fp0_regnum + 1 + i,
				       (const bfd_byte *) writebuf + i * 8);
	      if (readbuf != NULL)
		regcache_cooked_read (regcache, tdep->ppc_fp0_regnum + 1 + i,
				      (bfd_byte *) readbuf + i * 8);
	    }
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
  return RETURN_VALUE_STRUCT_CONVENTION;
}

