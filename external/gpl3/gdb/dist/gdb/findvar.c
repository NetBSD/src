/* Find a variable's value in memory, for GDB, the GNU debugger.

   Copyright (C) 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995,
   1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2005, 2007, 2008, 2009,
   2010, 2011 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "frame.h"
#include "value.h"
#include "gdbcore.h"
#include "inferior.h"
#include "target.h"
#include "gdb_string.h"
#include "gdb_assert.h"
#include "floatformat.h"
#include "symfile.h"		/* for overlay functions */
#include "regcache.h"
#include "user-regs.h"
#include "block.h"
#include "objfiles.h"

/* Basic byte-swapping routines.  All 'extract' functions return a
   host-format integer from a target-format integer at ADDR which is
   LEN bytes long.  */

#if TARGET_CHAR_BIT != 8 || HOST_CHAR_BIT != 8
  /* 8 bit characters are a pretty safe assumption these days, so we
     assume it throughout all these swapping routines.  If we had to deal with
     9 bit characters, we would need to make len be in bits and would have
     to re-write these routines...  */
you lose
#endif

LONGEST
extract_signed_integer (const gdb_byte *addr, int len,
			enum bfd_endian byte_order)
{
  LONGEST retval;
  const unsigned char *p;
  const unsigned char *startaddr = addr;
  const unsigned char *endaddr = startaddr + len;

  if (len > (int) sizeof (LONGEST))
    error (_("\
That operation is not available on integers of more than %d bytes."),
	   (int) sizeof (LONGEST));

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  if (byte_order == BFD_ENDIAN_BIG)
    {
      p = startaddr;
      /* Do the sign extension once at the start.  */
      retval = ((LONGEST) * p ^ 0x80) - 0x80;
      for (++p; p < endaddr; ++p)
	retval = (retval << 8) | *p;
    }
  else
    {
      p = endaddr - 1;
      /* Do the sign extension once at the start.  */
      retval = ((LONGEST) * p ^ 0x80) - 0x80;
      for (--p; p >= startaddr; --p)
	retval = (retval << 8) | *p;
    }
  return retval;
}

ULONGEST
extract_unsigned_integer (const gdb_byte *addr, int len,
			  enum bfd_endian byte_order)
{
  ULONGEST retval;
  const unsigned char *p;
  const unsigned char *startaddr = addr;
  const unsigned char *endaddr = startaddr + len;

  if (len > (int) sizeof (ULONGEST))
    error (_("\
That operation is not available on integers of more than %d bytes."),
	   (int) sizeof (ULONGEST));

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  retval = 0;
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = startaddr; p < endaddr; ++p)
	retval = (retval << 8) | *p;
    }
  else
    {
      for (p = endaddr - 1; p >= startaddr; --p)
	retval = (retval << 8) | *p;
    }
  return retval;
}

/* Sometimes a long long unsigned integer can be extracted as a
   LONGEST value.  This is done so that we can print these values
   better.  If this integer can be converted to a LONGEST, this
   function returns 1 and sets *PVAL.  Otherwise it returns 0.  */

int
extract_long_unsigned_integer (const gdb_byte *addr, int orig_len,
			       enum bfd_endian byte_order, LONGEST *pval)
{
  const gdb_byte *p;
  const gdb_byte *first_addr;
  int len;

  len = orig_len;
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = addr;
	   len > (int) sizeof (LONGEST) && p < addr + orig_len;
	   p++)
	{
	  if (*p == 0)
	    len--;
	  else
	    break;
	}
      first_addr = p;
    }
  else
    {
      first_addr = addr;
      for (p = addr + orig_len - 1;
	   len > (int) sizeof (LONGEST) && p >= addr;
	   p--)
	{
	  if (*p == 0)
	    len--;
	  else
	    break;
	}
    }

  if (len <= (int) sizeof (LONGEST))
    {
      *pval = (LONGEST) extract_unsigned_integer (first_addr,
						  sizeof (LONGEST),
						  byte_order);
      return 1;
    }

  return 0;
}


/* Treat the bytes at BUF as a pointer of type TYPE, and return the
   address it represents.  */
CORE_ADDR
extract_typed_address (const gdb_byte *buf, struct type *type)
{
  if (TYPE_CODE (type) != TYPE_CODE_PTR
      && TYPE_CODE (type) != TYPE_CODE_REF)
    internal_error (__FILE__, __LINE__,
		    _("extract_typed_address: "
		    "type is not a pointer or reference"));

  return gdbarch_pointer_to_address (get_type_arch (type), type, buf);
}

/* All 'store' functions accept a host-format integer and store a
   target-format integer at ADDR which is LEN bytes long.  */

void
store_signed_integer (gdb_byte *addr, int len,
		      enum bfd_endian byte_order, LONGEST val)
{
  gdb_byte *p;
  gdb_byte *startaddr = addr;
  gdb_byte *endaddr = startaddr + len;

  /* Start at the least significant end of the integer, and work towards
     the most significant.  */
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = endaddr - 1; p >= startaddr; --p)
	{
	  *p = val & 0xff;
	  val >>= 8;
	}
    }
  else
    {
      for (p = startaddr; p < endaddr; ++p)
	{
	  *p = val & 0xff;
	  val >>= 8;
	}
    }
}

void
store_unsigned_integer (gdb_byte *addr, int len,
			enum bfd_endian byte_order, ULONGEST val)
{
  unsigned char *p;
  unsigned char *startaddr = (unsigned char *) addr;
  unsigned char *endaddr = startaddr + len;

  /* Start at the least significant end of the integer, and work towards
     the most significant.  */
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = endaddr - 1; p >= startaddr; --p)
	{
	  *p = val & 0xff;
	  val >>= 8;
	}
    }
  else
    {
      for (p = startaddr; p < endaddr; ++p)
	{
	  *p = val & 0xff;
	  val >>= 8;
	}
    }
}

/* Store the address ADDR as a pointer of type TYPE at BUF, in target
   form.  */
void
store_typed_address (gdb_byte *buf, struct type *type, CORE_ADDR addr)
{
  if (TYPE_CODE (type) != TYPE_CODE_PTR
      && TYPE_CODE (type) != TYPE_CODE_REF)
    internal_error (__FILE__, __LINE__,
		    _("store_typed_address: "
		    "type is not a pointer or reference"));

  gdbarch_address_to_pointer (get_type_arch (type), type, buf, addr);
}



/* Return a `value' with the contents of (virtual or cooked) register
   REGNUM as found in the specified FRAME.  The register's type is
   determined by register_type().  */

struct value *
value_of_register (int regnum, struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  CORE_ADDR addr;
  int optim;
  int unavail;
  struct value *reg_val;
  int realnum;
  gdb_byte raw_buffer[MAX_REGISTER_SIZE];
  enum lval_type lval;

  /* User registers lie completely outside of the range of normal
     registers.  Catch them early so that the target never sees them.  */
  if (regnum >= gdbarch_num_regs (gdbarch)
		+ gdbarch_num_pseudo_regs (gdbarch))
    return value_of_user_reg (regnum, frame);

  frame_register (frame, regnum, &optim, &unavail,
		  &lval, &addr, &realnum, raw_buffer);

  reg_val = allocate_value (register_type (gdbarch, regnum));

  if (!optim && !unavail)
    memcpy (value_contents_raw (reg_val), raw_buffer,
	    register_size (gdbarch, regnum));
  else
    memset (value_contents_raw (reg_val), 0,
	    register_size (gdbarch, regnum));

  VALUE_LVAL (reg_val) = lval;
  set_value_address (reg_val, addr);
  VALUE_REGNUM (reg_val) = regnum;
  set_value_optimized_out (reg_val, optim);
  if (unavail)
    mark_value_bytes_unavailable (reg_val, 0, register_size (gdbarch, regnum));
  VALUE_FRAME_ID (reg_val) = get_frame_id (frame);
  return reg_val;
}

/* Return a `value' with the contents of (virtual or cooked) register
   REGNUM as found in the specified FRAME.  The register's type is
   determined by register_type().  The value is not fetched.  */

struct value *
value_of_register_lazy (struct frame_info *frame, int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct value *reg_val;

  gdb_assert (regnum < (gdbarch_num_regs (gdbarch)
			+ gdbarch_num_pseudo_regs (gdbarch)));

  /* We should have a valid (i.e. non-sentinel) frame.  */
  gdb_assert (frame_id_p (get_frame_id (frame)));

  reg_val = allocate_value_lazy (register_type (gdbarch, regnum));
  VALUE_LVAL (reg_val) = lval_register;
  VALUE_REGNUM (reg_val) = regnum;
  VALUE_FRAME_ID (reg_val) = get_frame_id (frame);
  return reg_val;
}

/* Given a pointer of type TYPE in target form in BUF, return the
   address it represents.  */
CORE_ADDR
unsigned_pointer_to_address (struct gdbarch *gdbarch,
			     struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  return extract_unsigned_integer (buf, TYPE_LENGTH (type), byte_order);
}

CORE_ADDR
signed_pointer_to_address (struct gdbarch *gdbarch,
			   struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  return extract_signed_integer (buf, TYPE_LENGTH (type), byte_order);
}

/* Given an address, store it as a pointer of type TYPE in target
   format in BUF.  */
void
unsigned_address_to_pointer (struct gdbarch *gdbarch, struct type *type,
			     gdb_byte *buf, CORE_ADDR addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  store_unsigned_integer (buf, TYPE_LENGTH (type), byte_order, addr);
}

void
address_to_signed_pointer (struct gdbarch *gdbarch, struct type *type,
			   gdb_byte *buf, CORE_ADDR addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  store_signed_integer (buf, TYPE_LENGTH (type), byte_order, addr);
}

/* Will calling read_var_value or locate_var_value on SYM end
   up caring what frame it is being evaluated relative to?  SYM must
   be non-NULL.  */
int
symbol_read_needs_frame (struct symbol *sym)
{
  switch (SYMBOL_CLASS (sym))
    {
      /* All cases listed explicitly so that gcc -Wall will detect it if
         we failed to consider one.  */
    case LOC_COMPUTED:
      /* FIXME: cagney/2004-01-26: It should be possible to
	 unconditionally call the SYMBOL_COMPUTED_OPS method when available.
	 Unfortunately DWARF 2 stores the frame-base (instead of the
	 function) location in a function's symbol.  Oops!  For the
	 moment enable this when/where applicable.  */
      return SYMBOL_COMPUTED_OPS (sym)->read_needs_frame (sym);

    case LOC_REGISTER:
    case LOC_ARG:
    case LOC_REF_ARG:
    case LOC_REGPARM_ADDR:
    case LOC_LOCAL:
      return 1;

    case LOC_UNDEF:
    case LOC_CONST:
    case LOC_STATIC:
    case LOC_TYPEDEF:

    case LOC_LABEL:
      /* Getting the address of a label can be done independently of the block,
         even if some *uses* of that address wouldn't work so well without
         the right frame.  */

    case LOC_BLOCK:
    case LOC_CONST_BYTES:
    case LOC_UNRESOLVED:
    case LOC_OPTIMIZED_OUT:
      return 0;
    }
  return 1;
}

/* Given a struct symbol for a variable,
   and a stack frame id, read the value of the variable
   and return a (pointer to a) struct value containing the value.
   If the variable cannot be found, return a zero pointer.  */

struct value *
read_var_value (struct symbol *var, struct frame_info *frame)
{
  struct value *v;
  struct type *type = SYMBOL_TYPE (var);
  CORE_ADDR addr;
  int len;

  /* Call check_typedef on our type to make sure that, if TYPE is
     a TYPE_CODE_TYPEDEF, its length is set to the length of the target type
     instead of zero.  However, we do not replace the typedef type by the
     target type, because we want to keep the typedef in order to be able to
     set the returned value type description correctly.  */
  check_typedef (type);

  len = TYPE_LENGTH (type);

  if (symbol_read_needs_frame (var))
    gdb_assert (frame);

  switch (SYMBOL_CLASS (var))
    {
    case LOC_CONST:
      /* Put the constant back in target format.  */
      v = allocate_value (type);
      store_signed_integer (value_contents_raw (v), len,
			    gdbarch_byte_order (get_type_arch (type)),
			    (LONGEST) SYMBOL_VALUE (var));
      VALUE_LVAL (v) = not_lval;
      return v;

    case LOC_LABEL:
      /* Put the constant back in target format.  */
      v = allocate_value (type);
      if (overlay_debugging)
	{
	  CORE_ADDR addr
	    = symbol_overlayed_address (SYMBOL_VALUE_ADDRESS (var),
					SYMBOL_OBJ_SECTION (var));

	  store_typed_address (value_contents_raw (v), type, addr);
	}
      else
	store_typed_address (value_contents_raw (v), type,
			      SYMBOL_VALUE_ADDRESS (var));
      VALUE_LVAL (v) = not_lval;
      return v;

    case LOC_CONST_BYTES:
      v = allocate_value (type);
      memcpy (value_contents_raw (v), SYMBOL_VALUE_BYTES (var), len);
      VALUE_LVAL (v) = not_lval;
      return v;

    case LOC_STATIC:
      v = allocate_value_lazy (type);
      if (overlay_debugging)
	addr = symbol_overlayed_address (SYMBOL_VALUE_ADDRESS (var),
					 SYMBOL_OBJ_SECTION (var));
      else
	addr = SYMBOL_VALUE_ADDRESS (var);
      break;

    case LOC_ARG:
      addr = get_frame_args_address (frame);
      if (!addr)
	return 0;
      addr += SYMBOL_VALUE (var);
      v = allocate_value_lazy (type);
      break;

    case LOC_REF_ARG:
      {
	struct value *ref;
	CORE_ADDR argref;

	argref = get_frame_args_address (frame);
	if (!argref)
	  return 0;
	argref += SYMBOL_VALUE (var);
	ref = value_at (lookup_pointer_type (type), argref);
	addr = value_as_address (ref);
	v = allocate_value_lazy (type);
	break;
      }

    case LOC_LOCAL:
      addr = get_frame_locals_address (frame);
      addr += SYMBOL_VALUE (var);
      v = allocate_value_lazy (type);
      break;

    case LOC_TYPEDEF:
      error (_("Cannot look up value of a typedef"));
      break;

    case LOC_BLOCK:
      v = allocate_value_lazy (type);
      if (overlay_debugging)
	addr = symbol_overlayed_address
	  (BLOCK_START (SYMBOL_BLOCK_VALUE (var)), SYMBOL_OBJ_SECTION (var));
      else
	addr = BLOCK_START (SYMBOL_BLOCK_VALUE (var));
      break;

    case LOC_REGISTER:
    case LOC_REGPARM_ADDR:
      {
	int regno = SYMBOL_REGISTER_OPS (var)
		      ->register_number (var, get_frame_arch (frame));
	struct value *regval;

	if (SYMBOL_CLASS (var) == LOC_REGPARM_ADDR)
	  {
	    regval = value_from_register (lookup_pointer_type (type),
					  regno,
					  frame);

	    if (regval == NULL)
	      error (_("Value of register variable not available."));

	    addr = value_as_address (regval);
	    v = allocate_value_lazy (type);
	  }
	else
	  {
	    regval = value_from_register (type, regno, frame);

	    if (regval == NULL)
	      error (_("Value of register variable not available."));
	    return regval;
	  }
      }
      break;

    case LOC_COMPUTED:
      /* FIXME: cagney/2004-01-26: It should be possible to
	 unconditionally call the SYMBOL_COMPUTED_OPS method when available.
	 Unfortunately DWARF 2 stores the frame-base (instead of the
	 function) location in a function's symbol.  Oops!  For the
	 moment enable this when/where applicable.  */
      return SYMBOL_COMPUTED_OPS (var)->read_variable (var, frame);

    case LOC_UNRESOLVED:
      {
	struct minimal_symbol *msym;
	struct obj_section *obj_section;

	msym = lookup_minimal_symbol (SYMBOL_LINKAGE_NAME (var), NULL, NULL);
	if (msym == NULL)
	  return 0;
	if (overlay_debugging)
	  addr = symbol_overlayed_address (SYMBOL_VALUE_ADDRESS (msym),
					   SYMBOL_OBJ_SECTION (msym));
	else
	  addr = SYMBOL_VALUE_ADDRESS (msym);

	obj_section = SYMBOL_OBJ_SECTION (msym);
	if (obj_section
	    && (obj_section->the_bfd_section->flags & SEC_THREAD_LOCAL) != 0)
	  addr = target_translate_tls_address (obj_section->objfile, addr);
	v = allocate_value_lazy (type);
      }
      break;

    case LOC_OPTIMIZED_OUT:
      v = allocate_value_lazy (type);
      VALUE_LVAL (v) = not_lval;
      set_value_optimized_out (v, 1);
      return v;

    default:
      error (_("Cannot look up value of a botched symbol."));
      break;
    }

  VALUE_LVAL (v) = lval_memory;
  set_value_address (v, addr);
  return v;
}

/* Install default attributes for register values.  */

struct value *
default_value_from_register (struct type *type, int regnum,
			     struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int len = TYPE_LENGTH (type);
  struct value *value = allocate_value (type);

  VALUE_LVAL (value) = lval_register;
  VALUE_FRAME_ID (value) = get_frame_id (frame);
  VALUE_REGNUM (value) = regnum;

  /* Any structure stored in more than one register will always be
     an integral number of registers.  Otherwise, you need to do
     some fiddling with the last register copied here for little
     endian machines.  */
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG
      && len < register_size (gdbarch, regnum))
    /* Big-endian, and we want less than full size.  */
    set_value_offset (value, register_size (gdbarch, regnum) - len);
  else
    set_value_offset (value, 0);

  return value;
}

/* Return a value of type TYPE, stored in register REGNUM, in frame FRAME.  */

struct value *
value_from_register (struct type *type, int regnum, struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct type *type1 = check_typedef (type);
  struct value *v;
  int optim, unavail, ok;

  if (gdbarch_convert_register_p (gdbarch, regnum, type1))
    {
      /* The ISA/ABI need to something weird when obtaining the
         specified value from this register.  It might need to
         re-order non-adjacent, starting with REGNUM (see MIPS and
         i386).  It might need to convert the [float] register into
         the corresponding [integer] type (see Alpha).  The assumption
         is that gdbarch_register_to_value populates the entire value
         including the location.  */
      v = allocate_value (type);
      VALUE_LVAL (v) = lval_register;
      VALUE_FRAME_ID (v) = get_frame_id (frame);
      VALUE_REGNUM (v) = regnum;
      ok = gdbarch_register_to_value (gdbarch, frame, regnum, type1,
				      value_contents_raw (v), &optim,
				      &unavail);
    }
  else
    {
      int len = TYPE_LENGTH (type);

      /* Construct the value.  */
      v = gdbarch_value_from_register (gdbarch, type, regnum, frame);

      /* Get the data.  */
      ok = get_frame_register_bytes (frame, regnum, value_offset (v), len,
				     value_contents_raw (v),
				     &optim, &unavail);
    }

  if (!ok)
    {
      if (optim)
	set_value_optimized_out (v, 1);
      if (unavail)
	mark_value_bytes_unavailable (v, 0, TYPE_LENGTH (type));
    }

  return v;
}

/* Return contents of register REGNUM in frame FRAME as address,
   interpreted as value of type TYPE.   Will abort if register
   value is not available.  */

CORE_ADDR
address_from_register (struct type *type, int regnum, struct frame_info *frame)
{
  struct value *value;
  CORE_ADDR result;

  value = value_from_register (type, regnum, frame);
  gdb_assert (value);

  result = value_as_address (value);
  release_value (value);
  value_free (value);

  return result;
}
