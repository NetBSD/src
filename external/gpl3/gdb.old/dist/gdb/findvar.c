/* Find a variable's value in memory, for GDB, the GNU debugger.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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
#include "floatformat.h"
#include "symfile.h"		/* for overlay functions */
#include "regcache.h"
#include "user-regs.h"
#include "block.h"
#include "objfiles.h"
#include "language.h"
#include "dwarf2loc.h"

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
  struct value *reg_val;

  /* User registers lie completely outside of the range of normal
     registers.  Catch them early so that the target never sees them.  */
  if (regnum >= gdbarch_num_regs (gdbarch)
		+ gdbarch_num_pseudo_regs (gdbarch))
    return value_of_user_reg (regnum, frame);

  reg_val = value_of_register_lazy (frame, regnum);
  value_fetch_lazy (reg_val);
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

/* See value.h.  */

enum symbol_needs_kind
symbol_read_needs (struct symbol *sym)
{
  if (SYMBOL_COMPUTED_OPS (sym) != NULL)
    return SYMBOL_COMPUTED_OPS (sym)->get_symbol_read_needs (sym);

  switch (SYMBOL_CLASS (sym))
    {
      /* All cases listed explicitly so that gcc -Wall will detect it if
         we failed to consider one.  */
    case LOC_COMPUTED:
      gdb_assert_not_reached (_("LOC_COMPUTED variable missing a method"));

    case LOC_REGISTER:
    case LOC_ARG:
    case LOC_REF_ARG:
    case LOC_REGPARM_ADDR:
    case LOC_LOCAL:
      return SYMBOL_NEEDS_FRAME;

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
      return SYMBOL_NEEDS_NONE;
    }
  return SYMBOL_NEEDS_FRAME;
}

/* See value.h.  */

int
symbol_read_needs_frame (struct symbol *sym)
{
  return symbol_read_needs (sym) == SYMBOL_NEEDS_FRAME;
}

/* Private data to be used with minsym_lookup_iterator_cb.  */

struct minsym_lookup_data
{
  /* The name of the minimal symbol we are searching for.  */
  const char *name;

  /* The field where the callback should store the minimal symbol
     if found.  It should be initialized to NULL before the search
     is started.  */
  struct bound_minimal_symbol result;
};

/* A callback function for gdbarch_iterate_over_objfiles_in_search_order.
   It searches by name for a minimal symbol within the given OBJFILE.
   The arguments are passed via CB_DATA, which in reality is a pointer
   to struct minsym_lookup_data.  */

static int
minsym_lookup_iterator_cb (struct objfile *objfile, void *cb_data)
{
  struct minsym_lookup_data *data = (struct minsym_lookup_data *) cb_data;

  gdb_assert (data->result.minsym == NULL);

  data->result = lookup_minimal_symbol (data->name, NULL, objfile);

  /* The iterator should stop iff a match was found.  */
  return (data->result.minsym != NULL);
}

/* Given static link expression and the frame it lives in, look for the frame
   the static links points to and return it.  Return NULL if we could not find
   such a frame.   */

static struct frame_info *
follow_static_link (struct frame_info *frame,
		    const struct dynamic_prop *static_link)
{
  CORE_ADDR upper_frame_base;

  if (!dwarf2_evaluate_property (static_link, frame, NULL, &upper_frame_base))
    return NULL;

  /* Now climb up the stack frame until we reach the frame we are interested
     in.  */
  for (; frame != NULL; frame = get_prev_frame (frame))
    {
      struct symbol *framefunc = get_frame_function (frame);

      /* Stacks can be quite deep: give the user a chance to stop this.  */
      QUIT;

      /* If we don't know how to compute FRAME's base address, don't give up:
	 maybe the frame we are looking for is upper in the stace frame.  */
      if (framefunc != NULL
	  && SYMBOL_BLOCK_OPS (framefunc) != NULL
	  && SYMBOL_BLOCK_OPS (framefunc)->get_frame_base != NULL
	  && (SYMBOL_BLOCK_OPS (framefunc)->get_frame_base (framefunc, frame)
	      == upper_frame_base))
	break;
    }

  return frame;
}

/* Assuming VAR is a symbol that can be reached from FRAME thanks to lexical
   rules, look for the frame that is actually hosting VAR and return it.  If,
   for some reason, we found no such frame, return NULL.

   This kind of computation is necessary to correctly handle lexically nested
   functions.

   Note that in some cases, we know what scope VAR comes from but we cannot
   reach the specific frame that hosts the instance of VAR we are looking for.
   For backward compatibility purposes (with old compilers), we then look for
   the first frame that can host it.  */

static struct frame_info *
get_hosting_frame (struct symbol *var, const struct block *var_block,
		   struct frame_info *frame)
{
  const struct block *frame_block = NULL;

  if (!symbol_read_needs_frame (var))
    return NULL;

  /* Some symbols for local variables have no block: this happens when they are
     not produced by a debug information reader, for instance when GDB creates
     synthetic symbols.  Without block information, we must assume they are
     local to FRAME. In this case, there is nothing to do.  */
  else if (var_block == NULL)
    return frame;

  /* We currently assume that all symbols with a location list need a frame.
     This is true in practice because selecting the location description
     requires to compute the CFA, hence requires a frame.  However we have
     tests that embed global/static symbols with null location lists.
     We want to get <optimized out> instead of <frame required> when evaluating
     them so return a frame instead of raising an error.  */
  else if (var_block == block_global_block (var_block)
	   || var_block == block_static_block (var_block))
    return frame;

  /* We have to handle the "my_func::my_local_var" notation.  This requires us
     to look for upper frames when we find no block for the current frame: here
     and below, handle when frame_block == NULL.  */
  if (frame != NULL)
    frame_block = get_frame_block (frame, NULL);

  /* Climb up the call stack until reaching the frame we are looking for.  */
  while (frame != NULL && frame_block != var_block)
    {
      /* Stacks can be quite deep: give the user a chance to stop this.  */
      QUIT;

      if (frame_block == NULL)
	{
	  frame = get_prev_frame (frame);
	  if (frame == NULL)
	    break;
	  frame_block = get_frame_block (frame, NULL);
	}

      /* If we failed to find the proper frame, fallback to the heuristic
	 method below.  */
      else if (frame_block == block_global_block (frame_block))
	{
	  frame = NULL;
	  break;
	}

      /* Assuming we have a block for this frame: if we are at the function
	 level, the immediate upper lexical block is in an outer function:
	 follow the static link.  */
      else if (BLOCK_FUNCTION (frame_block))
	{
	  const struct dynamic_prop *static_link
	    = block_static_link (frame_block);
	  int could_climb_up = 0;

	  if (static_link != NULL)
	    {
	      frame = follow_static_link (frame, static_link);
	      if (frame != NULL)
		{
		  frame_block = get_frame_block (frame, NULL);
		  could_climb_up = frame_block != NULL;
		}
	    }
	  if (!could_climb_up)
	    {
	      frame = NULL;
	      break;
	    }
	}

      else
	/* We must be in some function nested lexical block.  Just get the
	   outer block: both must share the same frame.  */
	frame_block = BLOCK_SUPERBLOCK (frame_block);
    }

  /* Old compilers may not provide a static link, or they may provide an
     invalid one.  For such cases, fallback on the old way to evaluate
     non-local references: just climb up the call stack and pick the first
     frame that contains the variable we are looking for.  */
  if (frame == NULL)
    {
      frame = block_innermost_frame (var_block);
      if (frame == NULL)
	{
	  if (BLOCK_FUNCTION (var_block)
	      && !block_inlined_p (var_block)
	      && SYMBOL_PRINT_NAME (BLOCK_FUNCTION (var_block)))
	    error (_("No frame is currently executing in block %s."),
		   SYMBOL_PRINT_NAME (BLOCK_FUNCTION (var_block)));
	  else
	    error (_("No frame is currently executing in specified"
		     " block"));
	}
    }

  return frame;
}

/* A default implementation for the "la_read_var_value" hook in
   the language vector which should work in most situations.  */

struct value *
default_read_var_value (struct symbol *var, const struct block *var_block,
			struct frame_info *frame)
{
  struct value *v;
  struct type *type = SYMBOL_TYPE (var);
  CORE_ADDR addr;
  enum symbol_needs_kind sym_need;

  /* Call check_typedef on our type to make sure that, if TYPE is
     a TYPE_CODE_TYPEDEF, its length is set to the length of the target type
     instead of zero.  However, we do not replace the typedef type by the
     target type, because we want to keep the typedef in order to be able to
     set the returned value type description correctly.  */
  check_typedef (type);

  sym_need = symbol_read_needs (var);
  if (sym_need == SYMBOL_NEEDS_FRAME)
    gdb_assert (frame != NULL);
  else if (sym_need == SYMBOL_NEEDS_REGISTERS && !target_has_registers)
    error (_("Cannot read `%s' without registers"), SYMBOL_PRINT_NAME (var));

  if (frame != NULL)
    frame = get_hosting_frame (var, var_block, frame);

  if (SYMBOL_COMPUTED_OPS (var) != NULL)
    return SYMBOL_COMPUTED_OPS (var)->read_variable (var, frame);

  switch (SYMBOL_CLASS (var))
    {
    case LOC_CONST:
      if (is_dynamic_type (type))
	{
	  /* Value is a constant byte-sequence and needs no memory access.  */
	  type = resolve_dynamic_type (type, NULL, /* Unused address.  */ 0);
	}
      /* Put the constant back in target format. */
      v = allocate_value (type);
      store_signed_integer (value_contents_raw (v), TYPE_LENGTH (type),
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
					SYMBOL_OBJ_SECTION (symbol_objfile (var),
							    var));

	  store_typed_address (value_contents_raw (v), type, addr);
	}
      else
	store_typed_address (value_contents_raw (v), type,
			      SYMBOL_VALUE_ADDRESS (var));
      VALUE_LVAL (v) = not_lval;
      return v;

    case LOC_CONST_BYTES:
      if (is_dynamic_type (type))
	{
	  /* Value is a constant byte-sequence and needs no memory access.  */
	  type = resolve_dynamic_type (type, NULL, /* Unused address.  */ 0);
	}
      v = allocate_value (type);
      memcpy (value_contents_raw (v), SYMBOL_VALUE_BYTES (var),
	      TYPE_LENGTH (type));
      VALUE_LVAL (v) = not_lval;
      return v;

    case LOC_STATIC:
      if (overlay_debugging)
	addr = symbol_overlayed_address (SYMBOL_VALUE_ADDRESS (var),
					 SYMBOL_OBJ_SECTION (symbol_objfile (var),
							     var));
      else
	addr = SYMBOL_VALUE_ADDRESS (var);
      break;

    case LOC_ARG:
      addr = get_frame_args_address (frame);
      if (!addr)
	error (_("Unknown argument list address for `%s'."),
	       SYMBOL_PRINT_NAME (var));
      addr += SYMBOL_VALUE (var);
      break;

    case LOC_REF_ARG:
      {
	struct value *ref;
	CORE_ADDR argref;

	argref = get_frame_args_address (frame);
	if (!argref)
	  error (_("Unknown argument list address for `%s'."),
		 SYMBOL_PRINT_NAME (var));
	argref += SYMBOL_VALUE (var);
	ref = value_at (lookup_pointer_type (type), argref);
	addr = value_as_address (ref);
	break;
      }

    case LOC_LOCAL:
      addr = get_frame_locals_address (frame);
      addr += SYMBOL_VALUE (var);
      break;

    case LOC_TYPEDEF:
      error (_("Cannot look up value of a typedef `%s'."),
	     SYMBOL_PRINT_NAME (var));
      break;

    case LOC_BLOCK:
      if (overlay_debugging)
	addr = symbol_overlayed_address
	  (BLOCK_START (SYMBOL_BLOCK_VALUE (var)),
	   SYMBOL_OBJ_SECTION (symbol_objfile (var), var));
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
	      error (_("Value of register variable not available for `%s'."),
	             SYMBOL_PRINT_NAME (var));

	    addr = value_as_address (regval);
	  }
	else
	  {
	    regval = value_from_register (type, regno, frame);

	    if (regval == NULL)
	      error (_("Value of register variable not available for `%s'."),
	             SYMBOL_PRINT_NAME (var));
	    return regval;
	  }
      }
      break;

    case LOC_COMPUTED:
      gdb_assert_not_reached (_("LOC_COMPUTED variable missing a method"));

    case LOC_UNRESOLVED:
      {
	struct minsym_lookup_data lookup_data;
	struct minimal_symbol *msym;
	struct obj_section *obj_section;

	memset (&lookup_data, 0, sizeof (lookup_data));
	lookup_data.name = SYMBOL_LINKAGE_NAME (var);

	gdbarch_iterate_over_objfiles_in_search_order
	  (symbol_arch (var),
	   minsym_lookup_iterator_cb, &lookup_data,
	   symbol_objfile (var));
	msym = lookup_data.result.minsym;

	/* If we can't find the minsym there's a problem in the symbol info.
	   The symbol exists in the debug info, but it's missing in the minsym
	   table.  */
	if (msym == NULL)
	  {
	    const char *flavour_name
	      = objfile_flavour_name (symbol_objfile (var));

	    /* We can't get here unless we've opened the file, so flavour_name
	       can't be NULL.  */
	    gdb_assert (flavour_name != NULL);
	    error (_("Missing %s symbol \"%s\"."),
		   flavour_name, SYMBOL_LINKAGE_NAME (var));
	  }
	obj_section = MSYMBOL_OBJ_SECTION (lookup_data.result.objfile, msym);
	/* Relocate address, unless there is no section or the variable is
	   a TLS variable. */
	if (obj_section == NULL
	    || (obj_section->the_bfd_section->flags & SEC_THREAD_LOCAL) != 0)
	   addr = MSYMBOL_VALUE_RAW_ADDRESS (msym);
	else
	   addr = BMSYMBOL_VALUE_ADDRESS (lookup_data.result);
	if (overlay_debugging)
	  addr = symbol_overlayed_address (addr, obj_section);
	/* Determine address of TLS variable. */
	if (obj_section
	    && (obj_section->the_bfd_section->flags & SEC_THREAD_LOCAL) != 0)
	  addr = target_translate_tls_address (obj_section->objfile, addr);
      }
      break;

    case LOC_OPTIMIZED_OUT:
      return allocate_optimized_out_value (type);

    default:
      error (_("Cannot look up value of a botched symbol `%s'."),
	     SYMBOL_PRINT_NAME (var));
      break;
    }

  v = value_at_lazy (type, addr);
  return v;
}

/* Calls VAR's language la_read_var_value hook with the given arguments.  */

struct value *
read_var_value (struct symbol *var, const struct block *var_block,
		struct frame_info *frame)
{
  const struct language_defn *lang = language_def (SYMBOL_LANGUAGE (var));

  gdb_assert (lang != NULL);
  gdb_assert (lang->la_read_var_value != NULL);

  return lang->la_read_var_value (var, var_block, frame);
}

/* Install default attributes for register values.  */

struct value *
default_value_from_register (struct gdbarch *gdbarch, struct type *type,
                             int regnum, struct frame_id frame_id)
{
  int len = TYPE_LENGTH (type);
  struct value *value = allocate_value (type);

  VALUE_LVAL (value) = lval_register;
  VALUE_FRAME_ID (value) = frame_id;
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

/* VALUE must be an lval_register value.  If regnum is the value's
   associated register number, and len the length of the values type,
   read one or more registers in FRAME, starting with register REGNUM,
   until we've read LEN bytes.

   If any of the registers we try to read are optimized out, then mark the
   complete resulting value as optimized out.  */

void
read_frame_register_value (struct value *value, struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  LONGEST offset = 0;
  LONGEST reg_offset = value_offset (value);
  int regnum = VALUE_REGNUM (value);
  int len = type_length_units (check_typedef (value_type (value)));

  gdb_assert (VALUE_LVAL (value) == lval_register);

  /* Skip registers wholly inside of REG_OFFSET.  */
  while (reg_offset >= register_size (gdbarch, regnum))
    {
      reg_offset -= register_size (gdbarch, regnum);
      regnum++;
    }

  /* Copy the data.  */
  while (len > 0)
    {
      struct value *regval = get_frame_register_value (frame, regnum);
      int reg_len = type_length_units (value_type (regval)) - reg_offset;

      /* If the register length is larger than the number of bytes
         remaining to copy, then only copy the appropriate bytes.  */
      if (reg_len > len)
	reg_len = len;

      value_contents_copy (value, offset, regval, reg_offset, reg_len);

      offset += reg_len;
      len -= reg_len;
      reg_offset = 0;
      regnum++;
    }
}

/* Return a value of type TYPE, stored in register REGNUM, in frame FRAME.  */

struct value *
value_from_register (struct type *type, int regnum, struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct type *type1 = check_typedef (type);
  struct value *v;

  if (gdbarch_convert_register_p (gdbarch, regnum, type1))
    {
      int optim, unavail, ok;

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

      if (!ok)
	{
	  if (optim)
	    mark_value_bytes_optimized_out (v, 0, TYPE_LENGTH (type));
	  if (unavail)
	    mark_value_bytes_unavailable (v, 0, TYPE_LENGTH (type));
	}
    }
  else
    {
      /* Construct the value.  */
      v = gdbarch_value_from_register (gdbarch, type,
				       regnum, get_frame_id (frame));

      /* Get the data.  */
      read_frame_register_value (v, frame);
    }

  return v;
}

/* Return contents of register REGNUM in frame FRAME as address.
   Will abort if register value is not available.  */

CORE_ADDR
address_from_register (int regnum, struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct type *type = builtin_type (gdbarch)->builtin_data_ptr;
  struct value *value;
  CORE_ADDR result;
  int regnum_max_excl = (gdbarch_num_regs (gdbarch)
			 + gdbarch_num_pseudo_regs (gdbarch));

  if (regnum < 0 || regnum >= regnum_max_excl)
    error (_("Invalid register #%d, expecting 0 <= # < %d"), regnum,
	   regnum_max_excl);

  /* This routine may be called during early unwinding, at a time
     where the ID of FRAME is not yet known.  Calling value_from_register
     would therefore abort in get_frame_id.  However, since we only need
     a temporary value that is never used as lvalue, we actually do not
     really need to set its VALUE_FRAME_ID.  Therefore, we re-implement
     the core of value_from_register, but use the null_frame_id.  */

  /* Some targets require a special conversion routine even for plain
     pointer types.  Avoid constructing a value object in those cases.  */
  if (gdbarch_convert_register_p (gdbarch, regnum, type))
    {
      gdb_byte *buf = (gdb_byte *) alloca (TYPE_LENGTH (type));
      int optim, unavail, ok;

      ok = gdbarch_register_to_value (gdbarch, frame, regnum, type,
				      buf, &optim, &unavail);
      if (!ok)
	{
	  /* This function is used while computing a location expression.
	     Complain about the value being optimized out, rather than
	     letting value_as_address complain about some random register
	     the expression depends on not being saved.  */
	  error_value_optimized_out ();
	}

      return unpack_long (type, buf);
    }

  value = gdbarch_value_from_register (gdbarch, type, regnum, null_frame_id);
  read_frame_register_value (value, frame);

  if (value_optimized_out (value))
    {
      /* This function is used while computing a location expression.
	 Complain about the value being optimized out, rather than
	 letting value_as_address complain about some random register
	 the expression depends on not being saved.  */
      error_value_optimized_out ();
    }

  result = value_as_address (value);
  release_value (value);
  value_free (value);

  return result;
}

