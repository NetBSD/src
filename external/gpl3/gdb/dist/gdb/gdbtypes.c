/* Support routines for manipulating internal types for GDB.

   Copyright (C) 1992-2017 Free Software Foundation, Inc.

   Contributed by Cygnus Support, using pieces from other GDB modules.

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
#include "bfd.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "expression.h"
#include "language.h"
#include "target.h"
#include "value.h"
#include "demangle.h"
#include "complaints.h"
#include "gdbcmd.h"
#include "cp-abi.h"
#include "hashtab.h"
#include "cp-support.h"
#include "bcache.h"
#include "dwarf2loc.h"
#include "gdbcore.h"

/* Initialize BADNESS constants.  */

const struct rank LENGTH_MISMATCH_BADNESS = {100,0};

const struct rank TOO_FEW_PARAMS_BADNESS = {100,0};
const struct rank INCOMPATIBLE_TYPE_BADNESS = {100,0};

const struct rank EXACT_MATCH_BADNESS = {0,0};

const struct rank INTEGER_PROMOTION_BADNESS = {1,0};
const struct rank FLOAT_PROMOTION_BADNESS = {1,0};
const struct rank BASE_PTR_CONVERSION_BADNESS = {1,0};
const struct rank CV_CONVERSION_BADNESS = {1, 0};
const struct rank INTEGER_CONVERSION_BADNESS = {2,0};
const struct rank FLOAT_CONVERSION_BADNESS = {2,0};
const struct rank INT_FLOAT_CONVERSION_BADNESS = {2,0};
const struct rank VOID_PTR_CONVERSION_BADNESS = {2,0};
const struct rank BOOL_CONVERSION_BADNESS = {3,0};
const struct rank BASE_CONVERSION_BADNESS = {2,0};
const struct rank REFERENCE_CONVERSION_BADNESS = {2,0};
const struct rank NULL_POINTER_CONVERSION_BADNESS = {2,0};
const struct rank NS_POINTER_CONVERSION_BADNESS = {10,0};
const struct rank NS_INTEGER_POINTER_CONVERSION_BADNESS = {3,0};

/* Floatformat pairs.  */
const struct floatformat *floatformats_ieee_half[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_half_big,
  &floatformat_ieee_half_little
};
const struct floatformat *floatformats_ieee_single[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_single_big,
  &floatformat_ieee_single_little
};
const struct floatformat *floatformats_ieee_double[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_double_big,
  &floatformat_ieee_double_little
};
const struct floatformat *floatformats_ieee_double_littlebyte_bigword[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ieee_double_big,
  &floatformat_ieee_double_littlebyte_bigword
};
const struct floatformat *floatformats_i387_ext[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_i387_ext,
  &floatformat_i387_ext
};
const struct floatformat *floatformats_m68881_ext[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_m68881_ext,
  &floatformat_m68881_ext
};
const struct floatformat *floatformats_arm_ext[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_arm_ext_big,
  &floatformat_arm_ext_littlebyte_bigword
};
const struct floatformat *floatformats_ia64_spill[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ia64_spill_big,
  &floatformat_ia64_spill_little
};
const struct floatformat *floatformats_ia64_quad[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ia64_quad_big,
  &floatformat_ia64_quad_little
};
const struct floatformat *floatformats_vax_f[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_vax_f,
  &floatformat_vax_f
};
const struct floatformat *floatformats_vax_d[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_vax_d,
  &floatformat_vax_d
};
const struct floatformat *floatformats_ibm_long_double[BFD_ENDIAN_UNKNOWN] = {
  &floatformat_ibm_long_double_big,
  &floatformat_ibm_long_double_little
};

/* Should opaque types be resolved?  */

static int opaque_type_resolution = 1;

/* A flag to enable printing of debugging information of C++
   overloading.  */

unsigned int overload_debug = 0;

/* A flag to enable strict type checking.  */

static int strict_type_checking = 1;

/* A function to show whether opaque types are resolved.  */

static void
show_opaque_type_resolution (struct ui_file *file, int from_tty,
			     struct cmd_list_element *c, 
			     const char *value)
{
  fprintf_filtered (file, _("Resolution of opaque struct/class/union types "
			    "(if set before loading symbols) is %s.\n"),
		    value);
}

/* A function to show whether C++ overload debugging is enabled.  */

static void
show_overload_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of C++ overloading is %s.\n"), 
		    value);
}

/* A function to show the status of strict type checking.  */

static void
show_strict_type_checking (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Strict type checking is %s.\n"), value);
}


/* Allocate a new OBJFILE-associated type structure and fill it
   with some defaults.  Space for the type structure is allocated
   on the objfile's objfile_obstack.  */

struct type *
alloc_type (struct objfile *objfile)
{
  struct type *type;

  gdb_assert (objfile != NULL);

  /* Alloc the structure and start off with all fields zeroed.  */
  type = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct type);
  TYPE_MAIN_TYPE (type) = OBSTACK_ZALLOC (&objfile->objfile_obstack,
					  struct main_type);
  OBJSTAT (objfile, n_types++);

  TYPE_OBJFILE_OWNED (type) = 1;
  TYPE_OWNER (type).objfile = objfile;

  /* Initialize the fields that might not be zero.  */

  TYPE_CODE (type) = TYPE_CODE_UNDEF;
  TYPE_CHAIN (type) = type;	/* Chain back to itself.  */

  return type;
}

/* Allocate a new GDBARCH-associated type structure and fill it
   with some defaults.  Space for the type structure is allocated
   on the obstack associated with GDBARCH.  */

struct type *
alloc_type_arch (struct gdbarch *gdbarch)
{
  struct type *type;

  gdb_assert (gdbarch != NULL);

  /* Alloc the structure and start off with all fields zeroed.  */

  type = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct type);
  TYPE_MAIN_TYPE (type) = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct main_type);

  TYPE_OBJFILE_OWNED (type) = 0;
  TYPE_OWNER (type).gdbarch = gdbarch;

  /* Initialize the fields that might not be zero.  */

  TYPE_CODE (type) = TYPE_CODE_UNDEF;
  TYPE_CHAIN (type) = type;	/* Chain back to itself.  */

  return type;
}

/* If TYPE is objfile-associated, allocate a new type structure
   associated with the same objfile.  If TYPE is gdbarch-associated,
   allocate a new type structure associated with the same gdbarch.  */

struct type *
alloc_type_copy (const struct type *type)
{
  if (TYPE_OBJFILE_OWNED (type))
    return alloc_type (TYPE_OWNER (type).objfile);
  else
    return alloc_type_arch (TYPE_OWNER (type).gdbarch);
}

/* If TYPE is gdbarch-associated, return that architecture.
   If TYPE is objfile-associated, return that objfile's architecture.  */

struct gdbarch *
get_type_arch (const struct type *type)
{
  if (TYPE_OBJFILE_OWNED (type))
    return get_objfile_arch (TYPE_OWNER (type).objfile);
  else
    return TYPE_OWNER (type).gdbarch;
}

/* See gdbtypes.h.  */

struct type *
get_target_type (struct type *type)
{
  if (type != NULL)
    {
      type = TYPE_TARGET_TYPE (type);
      if (type != NULL)
	type = check_typedef (type);
    }

  return type;
}

/* See gdbtypes.h.  */

unsigned int
type_length_units (struct type *type)
{
  struct gdbarch *arch = get_type_arch (type);
  int unit_size = gdbarch_addressable_memory_unit_size (arch);

  return TYPE_LENGTH (type) / unit_size;
}

/* Alloc a new type instance structure, fill it with some defaults,
   and point it at OLDTYPE.  Allocate the new type instance from the
   same place as OLDTYPE.  */

static struct type *
alloc_type_instance (struct type *oldtype)
{
  struct type *type;

  /* Allocate the structure.  */

  if (! TYPE_OBJFILE_OWNED (oldtype))
    type = XCNEW (struct type);
  else
    type = OBSTACK_ZALLOC (&TYPE_OBJFILE (oldtype)->objfile_obstack,
			   struct type);

  TYPE_MAIN_TYPE (type) = TYPE_MAIN_TYPE (oldtype);

  TYPE_CHAIN (type) = type;	/* Chain back to itself for now.  */

  return type;
}

/* Clear all remnants of the previous type at TYPE, in preparation for
   replacing it with something else.  Preserve owner information.  */

static void
smash_type (struct type *type)
{
  int objfile_owned = TYPE_OBJFILE_OWNED (type);
  union type_owner owner = TYPE_OWNER (type);

  memset (TYPE_MAIN_TYPE (type), 0, sizeof (struct main_type));

  /* Restore owner information.  */
  TYPE_OBJFILE_OWNED (type) = objfile_owned;
  TYPE_OWNER (type) = owner;

  /* For now, delete the rings.  */
  TYPE_CHAIN (type) = type;

  /* For now, leave the pointer/reference types alone.  */
}

/* Lookup a pointer to a type TYPE.  TYPEPTR, if nonzero, points
   to a pointer to memory where the pointer type should be stored.
   If *TYPEPTR is zero, update it to point to the pointer type we return.
   We allocate new memory if needed.  */

struct type *
make_pointer_type (struct type *type, struct type **typeptr)
{
  struct type *ntype;	/* New type */
  struct type *chain;

  ntype = TYPE_POINTER_TYPE (type);

  if (ntype)
    {
      if (typeptr == 0)
	return ntype;		/* Don't care about alloc, 
				   and have new type.  */
      else if (*typeptr == 0)
	{
	  *typeptr = ntype;	/* Tracking alloc, and have new type.  */
	  return ntype;
	}
    }

  if (typeptr == 0 || *typeptr == 0)	/* We'll need to allocate one.  */
    {
      ntype = alloc_type_copy (type);
      if (typeptr)
	*typeptr = ntype;
    }
  else			/* We have storage, but need to reset it.  */
    {
      ntype = *typeptr;
      chain = TYPE_CHAIN (ntype);
      smash_type (ntype);
      TYPE_CHAIN (ntype) = chain;
    }

  TYPE_TARGET_TYPE (ntype) = type;
  TYPE_POINTER_TYPE (type) = ntype;

  /* FIXME!  Assumes the machine has only one representation for pointers!  */

  TYPE_LENGTH (ntype)
    = gdbarch_ptr_bit (get_type_arch (type)) / TARGET_CHAR_BIT;
  TYPE_CODE (ntype) = TYPE_CODE_PTR;

  /* Mark pointers as unsigned.  The target converts between pointers
     and addresses (CORE_ADDRs) using gdbarch_pointer_to_address and
     gdbarch_address_to_pointer.  */
  TYPE_UNSIGNED (ntype) = 1;

  /* Update the length of all the other variants of this type.  */
  chain = TYPE_CHAIN (ntype);
  while (chain != ntype)
    {
      TYPE_LENGTH (chain) = TYPE_LENGTH (ntype);
      chain = TYPE_CHAIN (chain);
    }

  return ntype;
}

/* Given a type TYPE, return a type of pointers to that type.
   May need to construct such a type if this is the first use.  */

struct type *
lookup_pointer_type (struct type *type)
{
  return make_pointer_type (type, (struct type **) 0);
}

/* Lookup a C++ `reference' to a type TYPE.  TYPEPTR, if nonzero,
   points to a pointer to memory where the reference type should be
   stored.  If *TYPEPTR is zero, update it to point to the reference
   type we return.  We allocate new memory if needed. REFCODE denotes
   the kind of reference type to lookup (lvalue or rvalue reference).  */

struct type *
make_reference_type (struct type *type, struct type **typeptr,
                      enum type_code refcode)
{
  struct type *ntype;	/* New type */
  struct type **reftype;
  struct type *chain;

  gdb_assert (refcode == TYPE_CODE_REF || refcode == TYPE_CODE_RVALUE_REF);

  ntype = (refcode == TYPE_CODE_REF ? TYPE_REFERENCE_TYPE (type)
           : TYPE_RVALUE_REFERENCE_TYPE (type));

  if (ntype)
    {
      if (typeptr == 0)
	return ntype;		/* Don't care about alloc, 
				   and have new type.  */
      else if (*typeptr == 0)
	{
	  *typeptr = ntype;	/* Tracking alloc, and have new type.  */
	  return ntype;
	}
    }

  if (typeptr == 0 || *typeptr == 0)	/* We'll need to allocate one.  */
    {
      ntype = alloc_type_copy (type);
      if (typeptr)
	*typeptr = ntype;
    }
  else			/* We have storage, but need to reset it.  */
    {
      ntype = *typeptr;
      chain = TYPE_CHAIN (ntype);
      smash_type (ntype);
      TYPE_CHAIN (ntype) = chain;
    }

  TYPE_TARGET_TYPE (ntype) = type;
  reftype = (refcode == TYPE_CODE_REF ? &TYPE_REFERENCE_TYPE (type)
             : &TYPE_RVALUE_REFERENCE_TYPE (type));

  *reftype = ntype;

  /* FIXME!  Assume the machine has only one representation for
     references, and that it matches the (only) representation for
     pointers!  */

  TYPE_LENGTH (ntype) =
    gdbarch_ptr_bit (get_type_arch (type)) / TARGET_CHAR_BIT;
  TYPE_CODE (ntype) = refcode;

  *reftype = ntype;

  /* Update the length of all the other variants of this type.  */
  chain = TYPE_CHAIN (ntype);
  while (chain != ntype)
    {
      TYPE_LENGTH (chain) = TYPE_LENGTH (ntype);
      chain = TYPE_CHAIN (chain);
    }

  return ntype;
}

/* Same as above, but caller doesn't care about memory allocation
   details.  */

struct type *
lookup_reference_type (struct type *type, enum type_code refcode)
{
  return make_reference_type (type, (struct type **) 0, refcode);
}

/* Lookup the lvalue reference type for the type TYPE.  */

struct type *
lookup_lvalue_reference_type (struct type *type)
{
  return lookup_reference_type (type, TYPE_CODE_REF);
}

/* Lookup the rvalue reference type for the type TYPE.  */

struct type *
lookup_rvalue_reference_type (struct type *type)
{
  return lookup_reference_type (type, TYPE_CODE_RVALUE_REF);
}

/* Lookup a function type that returns type TYPE.  TYPEPTR, if
   nonzero, points to a pointer to memory where the function type
   should be stored.  If *TYPEPTR is zero, update it to point to the
   function type we return.  We allocate new memory if needed.  */

struct type *
make_function_type (struct type *type, struct type **typeptr)
{
  struct type *ntype;	/* New type */

  if (typeptr == 0 || *typeptr == 0)	/* We'll need to allocate one.  */
    {
      ntype = alloc_type_copy (type);
      if (typeptr)
	*typeptr = ntype;
    }
  else			/* We have storage, but need to reset it.  */
    {
      ntype = *typeptr;
      smash_type (ntype);
    }

  TYPE_TARGET_TYPE (ntype) = type;

  TYPE_LENGTH (ntype) = 1;
  TYPE_CODE (ntype) = TYPE_CODE_FUNC;

  INIT_FUNC_SPECIFIC (ntype);

  return ntype;
}

/* Given a type TYPE, return a type of functions that return that type.
   May need to construct such a type if this is the first use.  */

struct type *
lookup_function_type (struct type *type)
{
  return make_function_type (type, (struct type **) 0);
}

/* Given a type TYPE and argument types, return the appropriate
   function type.  If the final type in PARAM_TYPES is NULL, make a
   varargs function.  */

struct type *
lookup_function_type_with_arguments (struct type *type,
				     int nparams,
				     struct type **param_types)
{
  struct type *fn = make_function_type (type, (struct type **) 0);
  int i;

  if (nparams > 0)
    {
      if (param_types[nparams - 1] == NULL)
	{
	  --nparams;
	  TYPE_VARARGS (fn) = 1;
	}
      else if (TYPE_CODE (check_typedef (param_types[nparams - 1]))
	       == TYPE_CODE_VOID)
	{
	  --nparams;
	  /* Caller should have ensured this.  */
	  gdb_assert (nparams == 0);
	  TYPE_PROTOTYPED (fn) = 1;
	}
    }

  TYPE_NFIELDS (fn) = nparams;
  TYPE_FIELDS (fn)
    = (struct field *) TYPE_ZALLOC (fn, nparams * sizeof (struct field));
  for (i = 0; i < nparams; ++i)
    TYPE_FIELD_TYPE (fn, i) = param_types[i];

  return fn;
}

/* Identify address space identifier by name --
   return the integer flag defined in gdbtypes.h.  */

int
address_space_name_to_int (struct gdbarch *gdbarch, char *space_identifier)
{
  int type_flags;

  /* Check for known address space delimiters.  */
  if (!strcmp (space_identifier, "code"))
    return TYPE_INSTANCE_FLAG_CODE_SPACE;
  else if (!strcmp (space_identifier, "data"))
    return TYPE_INSTANCE_FLAG_DATA_SPACE;
  else if (gdbarch_address_class_name_to_type_flags_p (gdbarch)
           && gdbarch_address_class_name_to_type_flags (gdbarch,
							space_identifier,
							&type_flags))
    return type_flags;
  else
    error (_("Unknown address space specifier: \"%s\""), space_identifier);
}

/* Identify address space identifier by integer flag as defined in 
   gdbtypes.h -- return the string version of the adress space name.  */

const char *
address_space_int_to_name (struct gdbarch *gdbarch, int space_flag)
{
  if (space_flag & TYPE_INSTANCE_FLAG_CODE_SPACE)
    return "code";
  else if (space_flag & TYPE_INSTANCE_FLAG_DATA_SPACE)
    return "data";
  else if ((space_flag & TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL)
           && gdbarch_address_class_type_flags_to_name_p (gdbarch))
    return gdbarch_address_class_type_flags_to_name (gdbarch, space_flag);
  else
    return NULL;
}

/* Create a new type with instance flags NEW_FLAGS, based on TYPE.

   If STORAGE is non-NULL, create the new type instance there.
   STORAGE must be in the same obstack as TYPE.  */

static struct type *
make_qualified_type (struct type *type, int new_flags,
		     struct type *storage)
{
  struct type *ntype;

  ntype = type;
  do
    {
      if (TYPE_INSTANCE_FLAGS (ntype) == new_flags)
	return ntype;
      ntype = TYPE_CHAIN (ntype);
    }
  while (ntype != type);

  /* Create a new type instance.  */
  if (storage == NULL)
    ntype = alloc_type_instance (type);
  else
    {
      /* If STORAGE was provided, it had better be in the same objfile
	 as TYPE.  Otherwise, we can't link it into TYPE's cv chain:
	 if one objfile is freed and the other kept, we'd have
	 dangling pointers.  */
      gdb_assert (TYPE_OBJFILE (type) == TYPE_OBJFILE (storage));

      ntype = storage;
      TYPE_MAIN_TYPE (ntype) = TYPE_MAIN_TYPE (type);
      TYPE_CHAIN (ntype) = ntype;
    }

  /* Pointers or references to the original type are not relevant to
     the new type.  */
  TYPE_POINTER_TYPE (ntype) = (struct type *) 0;
  TYPE_REFERENCE_TYPE (ntype) = (struct type *) 0;

  /* Chain the new qualified type to the old type.  */
  TYPE_CHAIN (ntype) = TYPE_CHAIN (type);
  TYPE_CHAIN (type) = ntype;

  /* Now set the instance flags and return the new type.  */
  TYPE_INSTANCE_FLAGS (ntype) = new_flags;

  /* Set length of new type to that of the original type.  */
  TYPE_LENGTH (ntype) = TYPE_LENGTH (type);

  return ntype;
}

/* Make an address-space-delimited variant of a type -- a type that
   is identical to the one supplied except that it has an address
   space attribute attached to it (such as "code" or "data").

   The space attributes "code" and "data" are for Harvard
   architectures.  The address space attributes are for architectures
   which have alternately sized pointers or pointers with alternate
   representations.  */

struct type *
make_type_with_address_space (struct type *type, int space_flag)
{
  int new_flags = ((TYPE_INSTANCE_FLAGS (type)
		    & ~(TYPE_INSTANCE_FLAG_CODE_SPACE
			| TYPE_INSTANCE_FLAG_DATA_SPACE
		        | TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL))
		   | space_flag);

  return make_qualified_type (type, new_flags, NULL);
}

/* Make a "c-v" variant of a type -- a type that is identical to the
   one supplied except that it may have const or volatile attributes
   CNST is a flag for setting the const attribute
   VOLTL is a flag for setting the volatile attribute
   TYPE is the base type whose variant we are creating.

   If TYPEPTR and *TYPEPTR are non-zero, then *TYPEPTR points to
   storage to hold the new qualified type; *TYPEPTR and TYPE must be
   in the same objfile.  Otherwise, allocate fresh memory for the new
   type whereever TYPE lives.  If TYPEPTR is non-zero, set it to the
   new type we construct.  */

struct type *
make_cv_type (int cnst, int voltl, 
	      struct type *type, 
	      struct type **typeptr)
{
  struct type *ntype;	/* New type */

  int new_flags = (TYPE_INSTANCE_FLAGS (type)
		   & ~(TYPE_INSTANCE_FLAG_CONST 
		       | TYPE_INSTANCE_FLAG_VOLATILE));

  if (cnst)
    new_flags |= TYPE_INSTANCE_FLAG_CONST;

  if (voltl)
    new_flags |= TYPE_INSTANCE_FLAG_VOLATILE;

  if (typeptr && *typeptr != NULL)
    {
      /* TYPE and *TYPEPTR must be in the same objfile.  We can't have
	 a C-V variant chain that threads across objfiles: if one
	 objfile gets freed, then the other has a broken C-V chain.

	 This code used to try to copy over the main type from TYPE to
	 *TYPEPTR if they were in different objfiles, but that's
	 wrong, too: TYPE may have a field list or member function
	 lists, which refer to types of their own, etc. etc.  The
	 whole shebang would need to be copied over recursively; you
	 can't have inter-objfile pointers.  The only thing to do is
	 to leave stub types as stub types, and look them up afresh by
	 name each time you encounter them.  */
      gdb_assert (TYPE_OBJFILE (*typeptr) == TYPE_OBJFILE (type));
    }
  
  ntype = make_qualified_type (type, new_flags, 
			       typeptr ? *typeptr : NULL);

  if (typeptr != NULL)
    *typeptr = ntype;

  return ntype;
}

/* Make a 'restrict'-qualified version of TYPE.  */

struct type *
make_restrict_type (struct type *type)
{
  return make_qualified_type (type,
			      (TYPE_INSTANCE_FLAGS (type)
			       | TYPE_INSTANCE_FLAG_RESTRICT),
			      NULL);
}

/* Make a type without const, volatile, or restrict.  */

struct type *
make_unqualified_type (struct type *type)
{
  return make_qualified_type (type,
			      (TYPE_INSTANCE_FLAGS (type)
			       & ~(TYPE_INSTANCE_FLAG_CONST
				   | TYPE_INSTANCE_FLAG_VOLATILE
				   | TYPE_INSTANCE_FLAG_RESTRICT)),
			      NULL);
}

/* Make a '_Atomic'-qualified version of TYPE.  */

struct type *
make_atomic_type (struct type *type)
{
  return make_qualified_type (type,
			      (TYPE_INSTANCE_FLAGS (type)
			       | TYPE_INSTANCE_FLAG_ATOMIC),
			      NULL);
}

/* Replace the contents of ntype with the type *type.  This changes the
   contents, rather than the pointer for TYPE_MAIN_TYPE (ntype); thus
   the changes are propogated to all types in the TYPE_CHAIN.

   In order to build recursive types, it's inevitable that we'll need
   to update types in place --- but this sort of indiscriminate
   smashing is ugly, and needs to be replaced with something more
   controlled.  TYPE_MAIN_TYPE is a step in this direction; it's not
   clear if more steps are needed.  */

void
replace_type (struct type *ntype, struct type *type)
{
  struct type *chain;

  /* These two types had better be in the same objfile.  Otherwise,
     the assignment of one type's main type structure to the other
     will produce a type with references to objects (names; field
     lists; etc.) allocated on an objfile other than its own.  */
  gdb_assert (TYPE_OBJFILE (ntype) == TYPE_OBJFILE (type));

  *TYPE_MAIN_TYPE (ntype) = *TYPE_MAIN_TYPE (type);

  /* The type length is not a part of the main type.  Update it for
     each type on the variant chain.  */
  chain = ntype;
  do
    {
      /* Assert that this element of the chain has no address-class bits
	 set in its flags.  Such type variants might have type lengths
	 which are supposed to be different from the non-address-class
	 variants.  This assertion shouldn't ever be triggered because
	 symbol readers which do construct address-class variants don't
	 call replace_type().  */
      gdb_assert (TYPE_ADDRESS_CLASS_ALL (chain) == 0);

      TYPE_LENGTH (chain) = TYPE_LENGTH (type);
      chain = TYPE_CHAIN (chain);
    }
  while (ntype != chain);

  /* Assert that the two types have equivalent instance qualifiers.
     This should be true for at least all of our debug readers.  */
  gdb_assert (TYPE_INSTANCE_FLAGS (ntype) == TYPE_INSTANCE_FLAGS (type));
}

/* Implement direct support for MEMBER_TYPE in GNU C++.
   May need to construct such a type if this is the first use.
   The TYPE is the type of the member.  The DOMAIN is the type
   of the aggregate that the member belongs to.  */

struct type *
lookup_memberptr_type (struct type *type, struct type *domain)
{
  struct type *mtype;

  mtype = alloc_type_copy (type);
  smash_to_memberptr_type (mtype, domain, type);
  return mtype;
}

/* Return a pointer-to-method type, for a method of type TO_TYPE.  */

struct type *
lookup_methodptr_type (struct type *to_type)
{
  struct type *mtype;

  mtype = alloc_type_copy (to_type);
  smash_to_methodptr_type (mtype, to_type);
  return mtype;
}

/* Allocate a stub method whose return type is TYPE.  This apparently
   happens for speed of symbol reading, since parsing out the
   arguments to the method is cpu-intensive, the way we are doing it.
   So, we will fill in arguments later.  This always returns a fresh
   type.  */

struct type *
allocate_stub_method (struct type *type)
{
  struct type *mtype;

  mtype = alloc_type_copy (type);
  TYPE_CODE (mtype) = TYPE_CODE_METHOD;
  TYPE_LENGTH (mtype) = 1;
  TYPE_STUB (mtype) = 1;
  TYPE_TARGET_TYPE (mtype) = type;
  /* TYPE_SELF_TYPE (mtype) = unknown yet */
  return mtype;
}

/* Create a range type with a dynamic range from LOW_BOUND to
   HIGH_BOUND, inclusive.  See create_range_type for further details. */

struct type *
create_range_type (struct type *result_type, struct type *index_type,
		   const struct dynamic_prop *low_bound,
		   const struct dynamic_prop *high_bound)
{
  if (result_type == NULL)
    result_type = alloc_type_copy (index_type);
  TYPE_CODE (result_type) = TYPE_CODE_RANGE;
  TYPE_TARGET_TYPE (result_type) = index_type;
  if (TYPE_STUB (index_type))
    TYPE_TARGET_STUB (result_type) = 1;
  else
    TYPE_LENGTH (result_type) = TYPE_LENGTH (check_typedef (index_type));

  TYPE_RANGE_DATA (result_type) = (struct range_bounds *)
    TYPE_ZALLOC (result_type, sizeof (struct range_bounds));
  TYPE_RANGE_DATA (result_type)->low = *low_bound;
  TYPE_RANGE_DATA (result_type)->high = *high_bound;

  if (low_bound->kind == PROP_CONST && low_bound->data.const_val >= 0)
    TYPE_UNSIGNED (result_type) = 1;

  /* Ada allows the declaration of range types whose upper bound is
     less than the lower bound, so checking the lower bound is not
     enough.  Make sure we do not mark a range type whose upper bound
     is negative as unsigned.  */
  if (high_bound->kind == PROP_CONST && high_bound->data.const_val < 0)
    TYPE_UNSIGNED (result_type) = 0;

  return result_type;
}

/* Create a range type using either a blank type supplied in
   RESULT_TYPE, or creating a new type, inheriting the objfile from
   INDEX_TYPE.

   Indices will be of type INDEX_TYPE, and will range from LOW_BOUND
   to HIGH_BOUND, inclusive.

   FIXME: Maybe we should check the TYPE_CODE of RESULT_TYPE to make
   sure it is TYPE_CODE_UNDEF before we bash it into a range type?  */

struct type *
create_static_range_type (struct type *result_type, struct type *index_type,
			  LONGEST low_bound, LONGEST high_bound)
{
  struct dynamic_prop low, high;

  low.kind = PROP_CONST;
  low.data.const_val = low_bound;

  high.kind = PROP_CONST;
  high.data.const_val = high_bound;

  result_type = create_range_type (result_type, index_type, &low, &high);

  return result_type;
}

/* Predicate tests whether BOUNDS are static.  Returns 1 if all bounds values
   are static, otherwise returns 0.  */

static int
has_static_range (const struct range_bounds *bounds)
{
  return (bounds->low.kind == PROP_CONST
	  && bounds->high.kind == PROP_CONST);
}


/* Set *LOWP and *HIGHP to the lower and upper bounds of discrete type
   TYPE.  Return 1 if type is a range type, 0 if it is discrete (and
   bounds will fit in LONGEST), or -1 otherwise.  */

int
get_discrete_bounds (struct type *type, LONGEST *lowp, LONGEST *highp)
{
  type = check_typedef (type);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_RANGE:
      *lowp = TYPE_LOW_BOUND (type);
      *highp = TYPE_HIGH_BOUND (type);
      return 1;
    case TYPE_CODE_ENUM:
      if (TYPE_NFIELDS (type) > 0)
	{
	  /* The enums may not be sorted by value, so search all
	     entries.  */
	  int i;

	  *lowp = *highp = TYPE_FIELD_ENUMVAL (type, 0);
	  for (i = 0; i < TYPE_NFIELDS (type); i++)
	    {
	      if (TYPE_FIELD_ENUMVAL (type, i) < *lowp)
		*lowp = TYPE_FIELD_ENUMVAL (type, i);
	      if (TYPE_FIELD_ENUMVAL (type, i) > *highp)
		*highp = TYPE_FIELD_ENUMVAL (type, i);
	    }

	  /* Set unsigned indicator if warranted.  */
	  if (*lowp >= 0)
	    {
	      TYPE_UNSIGNED (type) = 1;
	    }
	}
      else
	{
	  *lowp = 0;
	  *highp = -1;
	}
      return 0;
    case TYPE_CODE_BOOL:
      *lowp = 0;
      *highp = 1;
      return 0;
    case TYPE_CODE_INT:
      if (TYPE_LENGTH (type) > sizeof (LONGEST))	/* Too big */
	return -1;
      if (!TYPE_UNSIGNED (type))
	{
	  *lowp = -(1 << (TYPE_LENGTH (type) * TARGET_CHAR_BIT - 1));
	  *highp = -*lowp - 1;
	  return 0;
	}
      /* ... fall through for unsigned ints ...  */
    case TYPE_CODE_CHAR:
      *lowp = 0;
      /* This round-about calculation is to avoid shifting by
         TYPE_LENGTH (type) * TARGET_CHAR_BIT, which will not work
         if TYPE_LENGTH (type) == sizeof (LONGEST).  */
      *highp = 1 << (TYPE_LENGTH (type) * TARGET_CHAR_BIT - 1);
      *highp = (*highp - 1) | *highp;
      return 0;
    default:
      return -1;
    }
}

/* Assuming TYPE is a simple, non-empty array type, compute its upper
   and lower bound.  Save the low bound into LOW_BOUND if not NULL.
   Save the high bound into HIGH_BOUND if not NULL.

   Return 1 if the operation was successful.  Return zero otherwise,
   in which case the values of LOW_BOUND and HIGH_BOUNDS are unmodified.

   We now simply use get_discrete_bounds call to get the values
   of the low and high bounds.
   get_discrete_bounds can return three values:
   1, meaning that index is a range,
   0, meaning that index is a discrete type,
   or -1 for failure.  */

int
get_array_bounds (struct type *type, LONGEST *low_bound, LONGEST *high_bound)
{
  struct type *index = TYPE_INDEX_TYPE (type);
  LONGEST low = 0;
  LONGEST high = 0;
  int res;

  if (index == NULL)
    return 0;

  res = get_discrete_bounds (index, &low, &high);
  if (res == -1)
    return 0;

  /* Check if the array bounds are undefined.  */
  if (res == 1
      && ((low_bound && TYPE_ARRAY_LOWER_BOUND_IS_UNDEFINED (type))
	  || (high_bound && TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (type))))
    return 0;

  if (low_bound)
    *low_bound = low;

  if (high_bound)
    *high_bound = high;

  return 1;
}

/* Assuming that TYPE is a discrete type and VAL is a valid integer
   representation of a value of this type, save the corresponding
   position number in POS.

   Its differs from VAL only in the case of enumeration types.  In
   this case, the position number of the value of the first listed
   enumeration literal is zero; the position number of the value of
   each subsequent enumeration literal is one more than that of its
   predecessor in the list.

   Return 1 if the operation was successful.  Return zero otherwise,
   in which case the value of POS is unmodified.
*/

int
discrete_position (struct type *type, LONGEST val, LONGEST *pos)
{
  if (TYPE_CODE (type) == TYPE_CODE_ENUM)
    {
      int i;

      for (i = 0; i < TYPE_NFIELDS (type); i += 1)
        {
          if (val == TYPE_FIELD_ENUMVAL (type, i))
	    {
	      *pos = i;
	      return 1;
	    }
        }
      /* Invalid enumeration value.  */
      return 0;
    }
  else
    {
      *pos = val;
      return 1;
    }
}

/* Create an array type using either a blank type supplied in
   RESULT_TYPE, or creating a new type, inheriting the objfile from
   RANGE_TYPE.

   Elements will be of type ELEMENT_TYPE, the indices will be of type
   RANGE_TYPE.

   If BIT_STRIDE is not zero, build a packed array type whose element
   size is BIT_STRIDE.  Otherwise, ignore this parameter.

   FIXME: Maybe we should check the TYPE_CODE of RESULT_TYPE to make
   sure it is TYPE_CODE_UNDEF before we bash it into an array
   type?  */

struct type *
create_array_type_with_stride (struct type *result_type,
			       struct type *element_type,
			       struct type *range_type,
			       unsigned int bit_stride)
{
  if (result_type == NULL)
    result_type = alloc_type_copy (range_type);

  TYPE_CODE (result_type) = TYPE_CODE_ARRAY;
  TYPE_TARGET_TYPE (result_type) = element_type;
  if (has_static_range (TYPE_RANGE_DATA (range_type))
      && (!type_not_associated (result_type)
	  && !type_not_allocated (result_type)))
    {
      LONGEST low_bound, high_bound;

      if (get_discrete_bounds (range_type, &low_bound, &high_bound) < 0)
	low_bound = high_bound = 0;
      element_type = check_typedef (element_type);
      /* Be careful when setting the array length.  Ada arrays can be
	 empty arrays with the high_bound being smaller than the low_bound.
	 In such cases, the array length should be zero.  */
      if (high_bound < low_bound)
	TYPE_LENGTH (result_type) = 0;
      else if (bit_stride > 0)
	TYPE_LENGTH (result_type) =
	  (bit_stride * (high_bound - low_bound + 1) + 7) / 8;
      else
	TYPE_LENGTH (result_type) =
	  TYPE_LENGTH (element_type) * (high_bound - low_bound + 1);
    }
  else
    {
      /* This type is dynamic and its length needs to be computed
         on demand.  In the meantime, avoid leaving the TYPE_LENGTH
         undefined by setting it to zero.  Although we are not expected
         to trust TYPE_LENGTH in this case, setting the size to zero
         allows us to avoid allocating objects of random sizes in case
         we accidently do.  */
      TYPE_LENGTH (result_type) = 0;
    }

  TYPE_NFIELDS (result_type) = 1;
  TYPE_FIELDS (result_type) =
    (struct field *) TYPE_ZALLOC (result_type, sizeof (struct field));
  TYPE_INDEX_TYPE (result_type) = range_type;
  if (bit_stride > 0)
    TYPE_FIELD_BITSIZE (result_type, 0) = bit_stride;

  /* TYPE_TARGET_STUB will take care of zero length arrays.  */
  if (TYPE_LENGTH (result_type) == 0)
    TYPE_TARGET_STUB (result_type) = 1;

  return result_type;
}

/* Same as create_array_type_with_stride but with no bit_stride
   (BIT_STRIDE = 0), thus building an unpacked array.  */

struct type *
create_array_type (struct type *result_type,
		   struct type *element_type,
		   struct type *range_type)
{
  return create_array_type_with_stride (result_type, element_type,
					range_type, 0);
}

struct type *
lookup_array_range_type (struct type *element_type,
			 LONGEST low_bound, LONGEST high_bound)
{
  struct gdbarch *gdbarch = get_type_arch (element_type);
  struct type *index_type = builtin_type (gdbarch)->builtin_int;
  struct type *range_type
    = create_static_range_type (NULL, index_type, low_bound, high_bound);

  return create_array_type (NULL, element_type, range_type);
}

/* Create a string type using either a blank type supplied in
   RESULT_TYPE, or creating a new type.  String types are similar
   enough to array of char types that we can use create_array_type to
   build the basic type and then bash it into a string type.

   For fixed length strings, the range type contains 0 as the lower
   bound and the length of the string minus one as the upper bound.

   FIXME: Maybe we should check the TYPE_CODE of RESULT_TYPE to make
   sure it is TYPE_CODE_UNDEF before we bash it into a string
   type?  */

struct type *
create_string_type (struct type *result_type,
		    struct type *string_char_type,
		    struct type *range_type)
{
  result_type = create_array_type (result_type,
				   string_char_type,
				   range_type);
  TYPE_CODE (result_type) = TYPE_CODE_STRING;
  return result_type;
}

struct type *
lookup_string_range_type (struct type *string_char_type,
			  LONGEST low_bound, LONGEST high_bound)
{
  struct type *result_type;

  result_type = lookup_array_range_type (string_char_type,
					 low_bound, high_bound);
  TYPE_CODE (result_type) = TYPE_CODE_STRING;
  return result_type;
}

struct type *
create_set_type (struct type *result_type, struct type *domain_type)
{
  if (result_type == NULL)
    result_type = alloc_type_copy (domain_type);

  TYPE_CODE (result_type) = TYPE_CODE_SET;
  TYPE_NFIELDS (result_type) = 1;
  TYPE_FIELDS (result_type)
    = (struct field *) TYPE_ZALLOC (result_type, sizeof (struct field));

  if (!TYPE_STUB (domain_type))
    {
      LONGEST low_bound, high_bound, bit_length;

      if (get_discrete_bounds (domain_type, &low_bound, &high_bound) < 0)
	low_bound = high_bound = 0;
      bit_length = high_bound - low_bound + 1;
      TYPE_LENGTH (result_type)
	= (bit_length + TARGET_CHAR_BIT - 1) / TARGET_CHAR_BIT;
      if (low_bound >= 0)
	TYPE_UNSIGNED (result_type) = 1;
    }
  TYPE_FIELD_TYPE (result_type, 0) = domain_type;

  return result_type;
}

/* Convert ARRAY_TYPE to a vector type.  This may modify ARRAY_TYPE
   and any array types nested inside it.  */

void
make_vector_type (struct type *array_type)
{
  struct type *inner_array, *elt_type;
  int flags;

  /* Find the innermost array type, in case the array is
     multi-dimensional.  */
  inner_array = array_type;
  while (TYPE_CODE (TYPE_TARGET_TYPE (inner_array)) == TYPE_CODE_ARRAY)
    inner_array = TYPE_TARGET_TYPE (inner_array);

  elt_type = TYPE_TARGET_TYPE (inner_array);
  if (TYPE_CODE (elt_type) == TYPE_CODE_INT)
    {
      flags = TYPE_INSTANCE_FLAGS (elt_type) | TYPE_INSTANCE_FLAG_NOTTEXT;
      elt_type = make_qualified_type (elt_type, flags, NULL);
      TYPE_TARGET_TYPE (inner_array) = elt_type;
    }

  TYPE_VECTOR (array_type) = 1;
}

struct type *
init_vector_type (struct type *elt_type, int n)
{
  struct type *array_type;

  array_type = lookup_array_range_type (elt_type, 0, n - 1);
  make_vector_type (array_type);
  return array_type;
}

/* Internal routine called by TYPE_SELF_TYPE to return the type that TYPE
   belongs to.  In c++ this is the class of "this", but TYPE_THIS_TYPE is too
   confusing.  "self" is a common enough replacement for "this".
   TYPE must be one of TYPE_CODE_METHODPTR, TYPE_CODE_MEMBERPTR, or
   TYPE_CODE_METHOD.  */

struct type *
internal_type_self_type (struct type *type)
{
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE)
	return NULL;
      gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_SELF_TYPE);
      return TYPE_MAIN_TYPE (type)->type_specific.self_type;
    case TYPE_CODE_METHOD:
      if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE)
	return NULL;
      gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_FUNC);
      return TYPE_MAIN_TYPE (type)->type_specific.func_stuff->self_type;
    default:
      gdb_assert_not_reached ("bad type");
    }
}

/* Set the type of the class that TYPE belongs to.
   In c++ this is the class of "this".
   TYPE must be one of TYPE_CODE_METHODPTR, TYPE_CODE_MEMBERPTR, or
   TYPE_CODE_METHOD.  */

void
set_type_self_type (struct type *type, struct type *self_type)
{
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE)
	TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_SELF_TYPE;
      gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_SELF_TYPE);
      TYPE_MAIN_TYPE (type)->type_specific.self_type = self_type;
      break;
    case TYPE_CODE_METHOD:
      if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_NONE)
	INIT_FUNC_SPECIFIC (type);
      gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_FUNC);
      TYPE_MAIN_TYPE (type)->type_specific.func_stuff->self_type = self_type;
      break;
    default:
      gdb_assert_not_reached ("bad type");
    }
}

/* Smash TYPE to be a type of pointers to members of SELF_TYPE with type
   TO_TYPE.  A member pointer is a wierd thing -- it amounts to a
   typed offset into a struct, e.g. "an int at offset 8".  A MEMBER
   TYPE doesn't include the offset (that's the value of the MEMBER
   itself), but does include the structure type into which it points
   (for some reason).

   When "smashing" the type, we preserve the objfile that the old type
   pointed to, since we aren't changing where the type is actually
   allocated.  */

void
smash_to_memberptr_type (struct type *type, struct type *self_type,
			 struct type *to_type)
{
  smash_type (type);
  TYPE_CODE (type) = TYPE_CODE_MEMBERPTR;
  TYPE_TARGET_TYPE (type) = to_type;
  set_type_self_type (type, self_type);
  /* Assume that a data member pointer is the same size as a normal
     pointer.  */
  TYPE_LENGTH (type)
    = gdbarch_ptr_bit (get_type_arch (to_type)) / TARGET_CHAR_BIT;
}

/* Smash TYPE to be a type of pointer to methods type TO_TYPE.

   When "smashing" the type, we preserve the objfile that the old type
   pointed to, since we aren't changing where the type is actually
   allocated.  */

void
smash_to_methodptr_type (struct type *type, struct type *to_type)
{
  smash_type (type);
  TYPE_CODE (type) = TYPE_CODE_METHODPTR;
  TYPE_TARGET_TYPE (type) = to_type;
  set_type_self_type (type, TYPE_SELF_TYPE (to_type));
  TYPE_LENGTH (type) = cplus_method_ptr_size (to_type);
}

/* Smash TYPE to be a type of method of SELF_TYPE with type TO_TYPE.
   METHOD just means `function that gets an extra "this" argument'.

   When "smashing" the type, we preserve the objfile that the old type
   pointed to, since we aren't changing where the type is actually
   allocated.  */

void
smash_to_method_type (struct type *type, struct type *self_type,
		      struct type *to_type, struct field *args,
		      int nargs, int varargs)
{
  smash_type (type);
  TYPE_CODE (type) = TYPE_CODE_METHOD;
  TYPE_TARGET_TYPE (type) = to_type;
  set_type_self_type (type, self_type);
  TYPE_FIELDS (type) = args;
  TYPE_NFIELDS (type) = nargs;
  if (varargs)
    TYPE_VARARGS (type) = 1;
  TYPE_LENGTH (type) = 1;	/* In practice, this is never needed.  */
}

/* Return a typename for a struct/union/enum type without "struct ",
   "union ", or "enum ".  If the type has a NULL name, return NULL.  */

const char *
type_name_no_tag (const struct type *type)
{
  if (TYPE_TAG_NAME (type) != NULL)
    return TYPE_TAG_NAME (type);

  /* Is there code which expects this to return the name if there is
     no tag name?  My guess is that this is mainly used for C++ in
     cases where the two will always be the same.  */
  return TYPE_NAME (type);
}

/* A wrapper of type_name_no_tag which calls error if the type is anonymous.
   Since GCC PR debug/47510 DWARF provides associated information to detect the
   anonymous class linkage name from its typedef.

   Parameter TYPE should not yet have CHECK_TYPEDEF applied, this function will
   apply it itself.  */

const char *
type_name_no_tag_or_error (struct type *type)
{
  struct type *saved_type = type;
  const char *name;
  struct objfile *objfile;

  type = check_typedef (type);

  name = type_name_no_tag (type);
  if (name != NULL)
    return name;

  name = type_name_no_tag (saved_type);
  objfile = TYPE_OBJFILE (saved_type);
  error (_("Invalid anonymous type %s [in module %s], GCC PR debug/47510 bug?"),
	 name ? name : "<anonymous>",
	 objfile ? objfile_name (objfile) : "<arch>");
}

/* Lookup a typedef or primitive type named NAME, visible in lexical
   block BLOCK.  If NOERR is nonzero, return zero if NAME is not
   suitably defined.  */

struct type *
lookup_typename (const struct language_defn *language,
		 struct gdbarch *gdbarch, const char *name,
		 const struct block *block, int noerr)
{
  struct symbol *sym;

  sym = lookup_symbol_in_language (name, block, VAR_DOMAIN,
				   language->la_language, NULL).symbol;
  if (sym != NULL && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
    return SYMBOL_TYPE (sym);

  if (noerr)
    return NULL;
  error (_("No type named %s."), name);
}

struct type *
lookup_unsigned_typename (const struct language_defn *language,
			  struct gdbarch *gdbarch, const char *name)
{
  char *uns = (char *) alloca (strlen (name) + 10);

  strcpy (uns, "unsigned ");
  strcpy (uns + 9, name);
  return lookup_typename (language, gdbarch, uns, (struct block *) NULL, 0);
}

struct type *
lookup_signed_typename (const struct language_defn *language,
			struct gdbarch *gdbarch, const char *name)
{
  struct type *t;
  char *uns = (char *) alloca (strlen (name) + 8);

  strcpy (uns, "signed ");
  strcpy (uns + 7, name);
  t = lookup_typename (language, gdbarch, uns, (struct block *) NULL, 1);
  /* If we don't find "signed FOO" just try again with plain "FOO".  */
  if (t != NULL)
    return t;
  return lookup_typename (language, gdbarch, name, (struct block *) NULL, 0);
}

/* Lookup a structure type named "struct NAME",
   visible in lexical block BLOCK.  */

struct type *
lookup_struct (const char *name, const struct block *block)
{
  struct symbol *sym;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, 0).symbol;

  if (sym == NULL)
    {
      error (_("No struct type named %s."), name);
    }
  if (TYPE_CODE (SYMBOL_TYPE (sym)) != TYPE_CODE_STRUCT)
    {
      error (_("This context has class, union or enum %s, not a struct."),
	     name);
    }
  return (SYMBOL_TYPE (sym));
}

/* Lookup a union type named "union NAME",
   visible in lexical block BLOCK.  */

struct type *
lookup_union (const char *name, const struct block *block)
{
  struct symbol *sym;
  struct type *t;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, 0).symbol;

  if (sym == NULL)
    error (_("No union type named %s."), name);

  t = SYMBOL_TYPE (sym);

  if (TYPE_CODE (t) == TYPE_CODE_UNION)
    return t;

  /* If we get here, it's not a union.  */
  error (_("This context has class, struct or enum %s, not a union."), 
	 name);
}

/* Lookup an enum type named "enum NAME",
   visible in lexical block BLOCK.  */

struct type *
lookup_enum (const char *name, const struct block *block)
{
  struct symbol *sym;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, 0).symbol;
  if (sym == NULL)
    {
      error (_("No enum type named %s."), name);
    }
  if (TYPE_CODE (SYMBOL_TYPE (sym)) != TYPE_CODE_ENUM)
    {
      error (_("This context has class, struct or union %s, not an enum."), 
	     name);
    }
  return (SYMBOL_TYPE (sym));
}

/* Lookup a template type named "template NAME<TYPE>",
   visible in lexical block BLOCK.  */

struct type *
lookup_template_type (char *name, struct type *type, 
		      const struct block *block)
{
  struct symbol *sym;
  char *nam = (char *) 
    alloca (strlen (name) + strlen (TYPE_NAME (type)) + 4);

  strcpy (nam, name);
  strcat (nam, "<");
  strcat (nam, TYPE_NAME (type));
  strcat (nam, " >");	/* FIXME, extra space still introduced in gcc?  */

  sym = lookup_symbol (nam, block, VAR_DOMAIN, 0).symbol;

  if (sym == NULL)
    {
      error (_("No template type named %s."), name);
    }
  if (TYPE_CODE (SYMBOL_TYPE (sym)) != TYPE_CODE_STRUCT)
    {
      error (_("This context has class, union or enum %s, not a struct."),
	     name);
    }
  return (SYMBOL_TYPE (sym));
}

/* Given a type TYPE, lookup the type of the component of type named
   NAME.

   TYPE can be either a struct or union, or a pointer or reference to
   a struct or union.  If it is a pointer or reference, its target
   type is automatically used.  Thus '.' and '->' are interchangable,
   as specified for the definitions of the expression element types
   STRUCTOP_STRUCT and STRUCTOP_PTR.

   If NOERR is nonzero, return zero if NAME is not suitably defined.
   If NAME is the name of a baseclass type, return that type.  */

struct type *
lookup_struct_elt_type (struct type *type, const char *name, int noerr)
{
  int i;

  for (;;)
    {
      type = check_typedef (type);
      if (TYPE_CODE (type) != TYPE_CODE_PTR
	  && TYPE_CODE (type) != TYPE_CODE_REF)
	break;
      type = TYPE_TARGET_TYPE (type);
    }

  if (TYPE_CODE (type) != TYPE_CODE_STRUCT 
      && TYPE_CODE (type) != TYPE_CODE_UNION)
    {
      std::string type_name = type_to_string (type);
      error (_("Type %s is not a structure or union type."),
	     type_name.c_str ());
    }

#if 0
  /* FIXME: This change put in by Michael seems incorrect for the case
     where the structure tag name is the same as the member name.
     I.e. when doing "ptype bell->bar" for "struct foo { int bar; int
     foo; } bell;" Disabled by fnf.  */
  {
    char *type_name;

    type_name = type_name_no_tag (type);
    if (type_name != NULL && strcmp (type_name, name) == 0)
      return type;
  }
#endif

  for (i = TYPE_NFIELDS (type) - 1; i >= TYPE_N_BASECLASSES (type); i--)
    {
      const char *t_field_name = TYPE_FIELD_NAME (type, i);

      if (t_field_name && (strcmp_iw (t_field_name, name) == 0))
	{
	  return TYPE_FIELD_TYPE (type, i);
	}
     else if (!t_field_name || *t_field_name == '\0')
	{
	  struct type *subtype 
	    = lookup_struct_elt_type (TYPE_FIELD_TYPE (type, i), name, 1);

	  if (subtype != NULL)
	    return subtype;
	}
    }

  /* OK, it's not in this class.  Recursively check the baseclasses.  */
  for (i = TYPE_N_BASECLASSES (type) - 1; i >= 0; i--)
    {
      struct type *t;

      t = lookup_struct_elt_type (TYPE_BASECLASS (type, i), name, 1);
      if (t != NULL)
	{
	  return t;
	}
    }

  if (noerr)
    {
      return NULL;
    }

  std::string type_name = type_to_string (type);
  error (_("Type %s has no component named %s."), type_name.c_str (), name);
}

/* Store in *MAX the largest number representable by unsigned integer type
   TYPE.  */

void
get_unsigned_type_max (struct type *type, ULONGEST *max)
{
  unsigned int n;

  type = check_typedef (type);
  gdb_assert (TYPE_CODE (type) == TYPE_CODE_INT && TYPE_UNSIGNED (type));
  gdb_assert (TYPE_LENGTH (type) <= sizeof (ULONGEST));

  /* Written this way to avoid overflow.  */
  n = TYPE_LENGTH (type) * TARGET_CHAR_BIT;
  *max = ((((ULONGEST) 1 << (n - 1)) - 1) << 1) | 1;
}

/* Store in *MIN, *MAX the smallest and largest numbers representable by
   signed integer type TYPE.  */

void
get_signed_type_minmax (struct type *type, LONGEST *min, LONGEST *max)
{
  unsigned int n;

  type = check_typedef (type);
  gdb_assert (TYPE_CODE (type) == TYPE_CODE_INT && !TYPE_UNSIGNED (type));
  gdb_assert (TYPE_LENGTH (type) <= sizeof (LONGEST));

  n = TYPE_LENGTH (type) * TARGET_CHAR_BIT;
  *min = -((ULONGEST) 1 << (n - 1));
  *max = ((ULONGEST) 1 << (n - 1)) - 1;
}

/* Internal routine called by TYPE_VPTR_FIELDNO to return the value of
   cplus_stuff.vptr_fieldno.

   cplus_stuff is initialized to cplus_struct_default which does not
   set vptr_fieldno to -1 for portability reasons (IWBN to use C99
   designated initializers).  We cope with that here.  */

int
internal_type_vptr_fieldno (struct type *type)
{
  type = check_typedef (type);
  gdb_assert (TYPE_CODE (type) == TYPE_CODE_STRUCT
	      || TYPE_CODE (type) == TYPE_CODE_UNION);
  if (!HAVE_CPLUS_STRUCT (type))
    return -1;
  return TYPE_RAW_CPLUS_SPECIFIC (type)->vptr_fieldno;
}

/* Set the value of cplus_stuff.vptr_fieldno.  */

void
set_type_vptr_fieldno (struct type *type, int fieldno)
{
  type = check_typedef (type);
  gdb_assert (TYPE_CODE (type) == TYPE_CODE_STRUCT
	      || TYPE_CODE (type) == TYPE_CODE_UNION);
  if (!HAVE_CPLUS_STRUCT (type))
    ALLOCATE_CPLUS_STRUCT_TYPE (type);
  TYPE_RAW_CPLUS_SPECIFIC (type)->vptr_fieldno = fieldno;
}

/* Internal routine called by TYPE_VPTR_BASETYPE to return the value of
   cplus_stuff.vptr_basetype.  */

struct type *
internal_type_vptr_basetype (struct type *type)
{
  type = check_typedef (type);
  gdb_assert (TYPE_CODE (type) == TYPE_CODE_STRUCT
	      || TYPE_CODE (type) == TYPE_CODE_UNION);
  gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_CPLUS_STUFF);
  return TYPE_RAW_CPLUS_SPECIFIC (type)->vptr_basetype;
}

/* Set the value of cplus_stuff.vptr_basetype.  */

void
set_type_vptr_basetype (struct type *type, struct type *basetype)
{
  type = check_typedef (type);
  gdb_assert (TYPE_CODE (type) == TYPE_CODE_STRUCT
	      || TYPE_CODE (type) == TYPE_CODE_UNION);
  if (!HAVE_CPLUS_STRUCT (type))
    ALLOCATE_CPLUS_STRUCT_TYPE (type);
  TYPE_RAW_CPLUS_SPECIFIC (type)->vptr_basetype = basetype;
}

/* Lookup the vptr basetype/fieldno values for TYPE.
   If found store vptr_basetype in *BASETYPEP if non-NULL, and return
   vptr_fieldno.  Also, if found and basetype is from the same objfile,
   cache the results.
   If not found, return -1 and ignore BASETYPEP.
   Callers should be aware that in some cases (for example,
   the type or one of its baseclasses is a stub type and we are
   debugging a .o file, or the compiler uses DWARF-2 and is not GCC),
   this function will not be able to find the
   virtual function table pointer, and vptr_fieldno will remain -1 and
   vptr_basetype will remain NULL or incomplete.  */

int
get_vptr_fieldno (struct type *type, struct type **basetypep)
{
  type = check_typedef (type);

  if (TYPE_VPTR_FIELDNO (type) < 0)
    {
      int i;

      /* We must start at zero in case the first (and only) baseclass
         is virtual (and hence we cannot share the table pointer).  */
      for (i = 0; i < TYPE_N_BASECLASSES (type); i++)
	{
	  struct type *baseclass = check_typedef (TYPE_BASECLASS (type, i));
	  int fieldno;
	  struct type *basetype;

	  fieldno = get_vptr_fieldno (baseclass, &basetype);
	  if (fieldno >= 0)
	    {
	      /* If the type comes from a different objfile we can't cache
		 it, it may have a different lifetime.  PR 2384 */
	      if (TYPE_OBJFILE (type) == TYPE_OBJFILE (basetype))
		{
		  set_type_vptr_fieldno (type, fieldno);
		  set_type_vptr_basetype (type, basetype);
		}
	      if (basetypep)
		*basetypep = basetype;
	      return fieldno;
	    }
	}

      /* Not found.  */
      return -1;
    }
  else
    {
      if (basetypep)
	*basetypep = TYPE_VPTR_BASETYPE (type);
      return TYPE_VPTR_FIELDNO (type);
    }
}

static void
stub_noname_complaint (void)
{
  complaint (&symfile_complaints, _("stub type has NULL name"));
}

/* Worker for is_dynamic_type.  */

static int
is_dynamic_type_internal (struct type *type, int top_level)
{
  type = check_typedef (type);

  /* We only want to recognize references at the outermost level.  */
  if (top_level && TYPE_CODE (type) == TYPE_CODE_REF)
    type = check_typedef (TYPE_TARGET_TYPE (type));

  /* Types that have a dynamic TYPE_DATA_LOCATION are considered
     dynamic, even if the type itself is statically defined.
     From a user's point of view, this may appear counter-intuitive;
     but it makes sense in this context, because the point is to determine
     whether any part of the type needs to be resolved before it can
     be exploited.  */
  if (TYPE_DATA_LOCATION (type) != NULL
      && (TYPE_DATA_LOCATION_KIND (type) == PROP_LOCEXPR
	  || TYPE_DATA_LOCATION_KIND (type) == PROP_LOCLIST))
    return 1;

  if (TYPE_ASSOCIATED_PROP (type))
    return 1;

  if (TYPE_ALLOCATED_PROP (type))
    return 1;

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_RANGE:
      {
	/* A range type is obviously dynamic if it has at least one
	   dynamic bound.  But also consider the range type to be
	   dynamic when its subtype is dynamic, even if the bounds
	   of the range type are static.  It allows us to assume that
	   the subtype of a static range type is also static.  */
	return (!has_static_range (TYPE_RANGE_DATA (type))
		|| is_dynamic_type_internal (TYPE_TARGET_TYPE (type), 0));
      }

    case TYPE_CODE_ARRAY:
      {
	gdb_assert (TYPE_NFIELDS (type) == 1);

	/* The array is dynamic if either the bounds are dynamic,
	   or the elements it contains have a dynamic contents.  */
	if (is_dynamic_type_internal (TYPE_INDEX_TYPE (type), 0))
	  return 1;
	return is_dynamic_type_internal (TYPE_TARGET_TYPE (type), 0);
      }

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      {
	int i;

	for (i = 0; i < TYPE_NFIELDS (type); ++i)
	  if (!field_is_static (&TYPE_FIELD (type, i))
	      && is_dynamic_type_internal (TYPE_FIELD_TYPE (type, i), 0))
	    return 1;
      }
      break;
    }

  return 0;
}

/* See gdbtypes.h.  */

int
is_dynamic_type (struct type *type)
{
  return is_dynamic_type_internal (type, 1);
}

static struct type *resolve_dynamic_type_internal
  (struct type *type, struct property_addr_info *addr_stack, int top_level);

/* Given a dynamic range type (dyn_range_type) and a stack of
   struct property_addr_info elements, return a static version
   of that type.  */

static struct type *
resolve_dynamic_range (struct type *dyn_range_type,
		       struct property_addr_info *addr_stack)
{
  CORE_ADDR value;
  struct type *static_range_type, *static_target_type;
  const struct dynamic_prop *prop;
  struct dynamic_prop low_bound, high_bound;

  gdb_assert (TYPE_CODE (dyn_range_type) == TYPE_CODE_RANGE);

  prop = &TYPE_RANGE_DATA (dyn_range_type)->low;
  if (dwarf2_evaluate_property (prop, NULL, addr_stack, &value))
    {
      low_bound.kind = PROP_CONST;
      low_bound.data.const_val = value;
    }
  else
    {
      low_bound.kind = PROP_UNDEFINED;
      low_bound.data.const_val = 0;
    }

  prop = &TYPE_RANGE_DATA (dyn_range_type)->high;
  if (dwarf2_evaluate_property (prop, NULL, addr_stack, &value))
    {
      high_bound.kind = PROP_CONST;
      high_bound.data.const_val = value;

      if (TYPE_RANGE_DATA (dyn_range_type)->flag_upper_bound_is_count)
	high_bound.data.const_val
	  = low_bound.data.const_val + high_bound.data.const_val - 1;
    }
  else
    {
      high_bound.kind = PROP_UNDEFINED;
      high_bound.data.const_val = 0;
    }

  static_target_type
    = resolve_dynamic_type_internal (TYPE_TARGET_TYPE (dyn_range_type),
				     addr_stack, 0);
  static_range_type = create_range_type (copy_type (dyn_range_type),
					 static_target_type,
					 &low_bound, &high_bound);
  TYPE_RANGE_DATA (static_range_type)->flag_bound_evaluated = 1;
  return static_range_type;
}

/* Resolves dynamic bound values of an array type TYPE to static ones.
   ADDR_STACK is a stack of struct property_addr_info to be used
   if needed during the dynamic resolution.  */

static struct type *
resolve_dynamic_array (struct type *type,
		       struct property_addr_info *addr_stack)
{
  CORE_ADDR value;
  struct type *elt_type;
  struct type *range_type;
  struct type *ary_dim;
  struct dynamic_prop *prop;

  gdb_assert (TYPE_CODE (type) == TYPE_CODE_ARRAY);

  type = copy_type (type);

  elt_type = type;
  range_type = check_typedef (TYPE_INDEX_TYPE (elt_type));
  range_type = resolve_dynamic_range (range_type, addr_stack);

  /* Resolve allocated/associated here before creating a new array type, which
     will update the length of the array accordingly.  */
  prop = TYPE_ALLOCATED_PROP (type);
  if (prop != NULL && dwarf2_evaluate_property (prop, NULL, addr_stack, &value))
    {
      TYPE_DYN_PROP_ADDR (prop) = value;
      TYPE_DYN_PROP_KIND (prop) = PROP_CONST;
    }
  prop = TYPE_ASSOCIATED_PROP (type);
  if (prop != NULL && dwarf2_evaluate_property (prop, NULL, addr_stack, &value))
    {
      TYPE_DYN_PROP_ADDR (prop) = value;
      TYPE_DYN_PROP_KIND (prop) = PROP_CONST;
    }

  ary_dim = check_typedef (TYPE_TARGET_TYPE (elt_type));

  if (ary_dim != NULL && TYPE_CODE (ary_dim) == TYPE_CODE_ARRAY)
    elt_type = resolve_dynamic_array (ary_dim, addr_stack);
  else
    elt_type = TYPE_TARGET_TYPE (type);

  return create_array_type_with_stride (type, elt_type, range_type,
                                        TYPE_FIELD_BITSIZE (type, 0));
}

/* Resolve dynamic bounds of members of the union TYPE to static
   bounds.  ADDR_STACK is a stack of struct property_addr_info
   to be used if needed during the dynamic resolution.  */

static struct type *
resolve_dynamic_union (struct type *type,
		       struct property_addr_info *addr_stack)
{
  struct type *resolved_type;
  int i;
  unsigned int max_len = 0;

  gdb_assert (TYPE_CODE (type) == TYPE_CODE_UNION);

  resolved_type = copy_type (type);
  TYPE_FIELDS (resolved_type)
    = (struct field *) TYPE_ALLOC (resolved_type,
				   TYPE_NFIELDS (resolved_type)
				   * sizeof (struct field));
  memcpy (TYPE_FIELDS (resolved_type),
	  TYPE_FIELDS (type),
	  TYPE_NFIELDS (resolved_type) * sizeof (struct field));
  for (i = 0; i < TYPE_NFIELDS (resolved_type); ++i)
    {
      struct type *t;

      if (field_is_static (&TYPE_FIELD (type, i)))
	continue;

      t = resolve_dynamic_type_internal (TYPE_FIELD_TYPE (resolved_type, i),
					 addr_stack, 0);
      TYPE_FIELD_TYPE (resolved_type, i) = t;
      if (TYPE_LENGTH (t) > max_len)
	max_len = TYPE_LENGTH (t);
    }

  TYPE_LENGTH (resolved_type) = max_len;
  return resolved_type;
}

/* Resolve dynamic bounds of members of the struct TYPE to static
   bounds.  ADDR_STACK is a stack of struct property_addr_info to
   be used if needed during the dynamic resolution.  */

static struct type *
resolve_dynamic_struct (struct type *type,
			struct property_addr_info *addr_stack)
{
  struct type *resolved_type;
  int i;
  unsigned resolved_type_bit_length = 0;

  gdb_assert (TYPE_CODE (type) == TYPE_CODE_STRUCT);
  gdb_assert (TYPE_NFIELDS (type) > 0);

  resolved_type = copy_type (type);
  TYPE_FIELDS (resolved_type)
    = (struct field *) TYPE_ALLOC (resolved_type,
				   TYPE_NFIELDS (resolved_type)
				   * sizeof (struct field));
  memcpy (TYPE_FIELDS (resolved_type),
	  TYPE_FIELDS (type),
	  TYPE_NFIELDS (resolved_type) * sizeof (struct field));
  for (i = 0; i < TYPE_NFIELDS (resolved_type); ++i)
    {
      unsigned new_bit_length;
      struct property_addr_info pinfo;

      if (field_is_static (&TYPE_FIELD (type, i)))
	continue;

      /* As we know this field is not a static field, the field's
	 field_loc_kind should be FIELD_LOC_KIND_BITPOS.  Verify
	 this is the case, but only trigger a simple error rather
	 than an internal error if that fails.  While failing
	 that verification indicates a bug in our code, the error
	 is not severe enough to suggest to the user he stops
	 his debugging session because of it.  */
      if (TYPE_FIELD_LOC_KIND (type, i) != FIELD_LOC_KIND_BITPOS)
	error (_("Cannot determine struct field location"
		 " (invalid location kind)"));

      pinfo.type = check_typedef (TYPE_FIELD_TYPE (type, i));
      pinfo.valaddr = addr_stack->valaddr;
      pinfo.addr
	= (addr_stack->addr
	   + (TYPE_FIELD_BITPOS (resolved_type, i) / TARGET_CHAR_BIT));
      pinfo.next = addr_stack;

      TYPE_FIELD_TYPE (resolved_type, i)
	= resolve_dynamic_type_internal (TYPE_FIELD_TYPE (resolved_type, i),
					 &pinfo, 0);
      gdb_assert (TYPE_FIELD_LOC_KIND (resolved_type, i)
		  == FIELD_LOC_KIND_BITPOS);

      new_bit_length = TYPE_FIELD_BITPOS (resolved_type, i);
      if (TYPE_FIELD_BITSIZE (resolved_type, i) != 0)
	new_bit_length += TYPE_FIELD_BITSIZE (resolved_type, i);
      else
	new_bit_length += (TYPE_LENGTH (TYPE_FIELD_TYPE (resolved_type, i))
			   * TARGET_CHAR_BIT);

      /* Normally, we would use the position and size of the last field
	 to determine the size of the enclosing structure.  But GCC seems
	 to be encoding the position of some fields incorrectly when
	 the struct contains a dynamic field that is not placed last.
	 So we compute the struct size based on the field that has
	 the highest position + size - probably the best we can do.  */
      if (new_bit_length > resolved_type_bit_length)
	resolved_type_bit_length = new_bit_length;
    }

  /* The length of a type won't change for fortran, but it does for C and Ada.
     For fortran the size of dynamic fields might change over time but not the
     type length of the structure.  If we adapt it, we run into problems
     when calculating the element offset for arrays of structs.  */
  if (current_language->la_language != language_fortran)
    TYPE_LENGTH (resolved_type)
      = (resolved_type_bit_length + TARGET_CHAR_BIT - 1) / TARGET_CHAR_BIT;

  /* The Ada language uses this field as a cache for static fixed types: reset
     it as RESOLVED_TYPE must have its own static fixed type.  */
  TYPE_TARGET_TYPE (resolved_type) = NULL;

  return resolved_type;
}

/* Worker for resolved_dynamic_type.  */

static struct type *
resolve_dynamic_type_internal (struct type *type,
			       struct property_addr_info *addr_stack,
			       int top_level)
{
  struct type *real_type = check_typedef (type);
  struct type *resolved_type = type;
  struct dynamic_prop *prop;
  CORE_ADDR value;

  if (!is_dynamic_type_internal (real_type, top_level))
    return type;

  if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
    {
      resolved_type = copy_type (type);
      TYPE_TARGET_TYPE (resolved_type)
	= resolve_dynamic_type_internal (TYPE_TARGET_TYPE (type), addr_stack,
					 top_level);
    }
  else 
    {
      /* Before trying to resolve TYPE, make sure it is not a stub.  */
      type = real_type;

      switch (TYPE_CODE (type))
	{
	case TYPE_CODE_REF:
	  {
	    struct property_addr_info pinfo;

	    pinfo.type = check_typedef (TYPE_TARGET_TYPE (type));
	    pinfo.valaddr = NULL;
	    if (addr_stack->valaddr != NULL)
	      pinfo.addr = extract_typed_address (addr_stack->valaddr, type);
	    else
	      pinfo.addr = read_memory_typed_address (addr_stack->addr, type);
	    pinfo.next = addr_stack;

	    resolved_type = copy_type (type);
	    TYPE_TARGET_TYPE (resolved_type)
	      = resolve_dynamic_type_internal (TYPE_TARGET_TYPE (type),
					       &pinfo, top_level);
	    break;
	  }

	case TYPE_CODE_ARRAY:
	  resolved_type = resolve_dynamic_array (type, addr_stack);
	  break;

	case TYPE_CODE_RANGE:
	  resolved_type = resolve_dynamic_range (type, addr_stack);
	  break;

	case TYPE_CODE_UNION:
	  resolved_type = resolve_dynamic_union (type, addr_stack);
	  break;

	case TYPE_CODE_STRUCT:
	  resolved_type = resolve_dynamic_struct (type, addr_stack);
	  break;
	}
    }

  /* Resolve data_location attribute.  */
  prop = TYPE_DATA_LOCATION (resolved_type);
  if (prop != NULL
      && dwarf2_evaluate_property (prop, NULL, addr_stack, &value))
    {
      TYPE_DYN_PROP_ADDR (prop) = value;
      TYPE_DYN_PROP_KIND (prop) = PROP_CONST;
    }

  return resolved_type;
}

/* See gdbtypes.h  */

struct type *
resolve_dynamic_type (struct type *type, const gdb_byte *valaddr,
		      CORE_ADDR addr)
{
  struct property_addr_info pinfo
    = {check_typedef (type), valaddr, addr, NULL};

  return resolve_dynamic_type_internal (type, &pinfo, 1);
}

/* See gdbtypes.h  */

struct dynamic_prop *
get_dyn_prop (enum dynamic_prop_node_kind prop_kind, const struct type *type)
{
  struct dynamic_prop_list *node = TYPE_DYN_PROP_LIST (type);

  while (node != NULL)
    {
      if (node->prop_kind == prop_kind)
        return &node->prop;
      node = node->next;
    }
  return NULL;
}

/* See gdbtypes.h  */

void
add_dyn_prop (enum dynamic_prop_node_kind prop_kind, struct dynamic_prop prop,
              struct type *type, struct objfile *objfile)
{
  struct dynamic_prop_list *temp;

  gdb_assert (TYPE_OBJFILE_OWNED (type));

  temp = XOBNEW (&objfile->objfile_obstack, struct dynamic_prop_list);
  temp->prop_kind = prop_kind;
  temp->prop = prop;
  temp->next = TYPE_DYN_PROP_LIST (type);

  TYPE_DYN_PROP_LIST (type) = temp;
}

/* Remove dynamic property from TYPE in case it exists.  */

void
remove_dyn_prop (enum dynamic_prop_node_kind prop_kind,
                 struct type *type)
{
  struct dynamic_prop_list *prev_node, *curr_node;

  curr_node = TYPE_DYN_PROP_LIST (type);
  prev_node = NULL;

  while (NULL != curr_node)
    {
      if (curr_node->prop_kind == prop_kind)
	{
	  /* Update the linked list but don't free anything.
	     The property was allocated on objstack and it is not known
	     if we are on top of it.  Nevertheless, everything is released
	     when the complete objstack is freed.  */
	  if (NULL == prev_node)
	    TYPE_DYN_PROP_LIST (type) = curr_node->next;
	  else
	    prev_node->next = curr_node->next;

	  return;
	}

      prev_node = curr_node;
      curr_node = curr_node->next;
    }
}

/* Find the real type of TYPE.  This function returns the real type,
   after removing all layers of typedefs, and completing opaque or stub
   types.  Completion changes the TYPE argument, but stripping of
   typedefs does not.

   Instance flags (e.g. const/volatile) are preserved as typedefs are
   stripped.  If necessary a new qualified form of the underlying type
   is created.

   NOTE: This will return a typedef if TYPE_TARGET_TYPE for the typedef has
   not been computed and we're either in the middle of reading symbols, or
   there was no name for the typedef in the debug info.

   NOTE: Lookup of opaque types can throw errors for invalid symbol files.
   QUITs in the symbol reading code can also throw.
   Thus this function can throw an exception.

   If TYPE is a TYPE_CODE_TYPEDEF, its length is updated to the length of
   the target type.

   If this is a stubbed struct (i.e. declared as struct foo *), see if
   we can find a full definition in some other file.  If so, copy this
   definition, so we can use it in future.  There used to be a comment
   (but not any code) that if we don't find a full definition, we'd
   set a flag so we don't spend time in the future checking the same
   type.  That would be a mistake, though--we might load in more
   symbols which contain a full definition for the type.  */

struct type *
check_typedef (struct type *type)
{
  struct type *orig_type = type;
  /* While we're removing typedefs, we don't want to lose qualifiers.
     E.g., const/volatile.  */
  int instance_flags = TYPE_INSTANCE_FLAGS (type);

  gdb_assert (type);

  while (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
    {
      if (!TYPE_TARGET_TYPE (type))
	{
	  const char *name;
	  struct symbol *sym;

	  /* It is dangerous to call lookup_symbol if we are currently
	     reading a symtab.  Infinite recursion is one danger.  */
	  if (currently_reading_symtab)
	    return make_qualified_type (type, instance_flags, NULL);

	  name = type_name_no_tag (type);
	  /* FIXME: shouldn't we separately check the TYPE_NAME and
	     the TYPE_TAG_NAME, and look in STRUCT_DOMAIN and/or
	     VAR_DOMAIN as appropriate?  (this code was written before
	     TYPE_NAME and TYPE_TAG_NAME were separate).  */
	  if (name == NULL)
	    {
	      stub_noname_complaint ();
	      return make_qualified_type (type, instance_flags, NULL);
	    }
	  sym = lookup_symbol (name, 0, STRUCT_DOMAIN, 0).symbol;
	  if (sym)
	    TYPE_TARGET_TYPE (type) = SYMBOL_TYPE (sym);
	  else					/* TYPE_CODE_UNDEF */
	    TYPE_TARGET_TYPE (type) = alloc_type_arch (get_type_arch (type));
	}
      type = TYPE_TARGET_TYPE (type);

      /* Preserve the instance flags as we traverse down the typedef chain.

	 Handling address spaces/classes is nasty, what do we do if there's a
	 conflict?
	 E.g., what if an outer typedef marks the type as class_1 and an inner
	 typedef marks the type as class_2?
	 This is the wrong place to do such error checking.  We leave it to
	 the code that created the typedef in the first place to flag the
	 error.  We just pick the outer address space (akin to letting the
	 outer cast in a chain of casting win), instead of assuming
	 "it can't happen".  */
      {
	const int ALL_SPACES = (TYPE_INSTANCE_FLAG_CODE_SPACE
				| TYPE_INSTANCE_FLAG_DATA_SPACE);
	const int ALL_CLASSES = TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL;
	int new_instance_flags = TYPE_INSTANCE_FLAGS (type);

	/* Treat code vs data spaces and address classes separately.  */
	if ((instance_flags & ALL_SPACES) != 0)
	  new_instance_flags &= ~ALL_SPACES;
	if ((instance_flags & ALL_CLASSES) != 0)
	  new_instance_flags &= ~ALL_CLASSES;

	instance_flags |= new_instance_flags;
      }
    }

  /* If this is a struct/class/union with no fields, then check
     whether a full definition exists somewhere else.  This is for
     systems where a type definition with no fields is issued for such
     types, instead of identifying them as stub types in the first
     place.  */

  if (TYPE_IS_OPAQUE (type) 
      && opaque_type_resolution 
      && !currently_reading_symtab)
    {
      const char *name = type_name_no_tag (type);
      struct type *newtype;

      if (name == NULL)
	{
	  stub_noname_complaint ();
	  return make_qualified_type (type, instance_flags, NULL);
	}
      newtype = lookup_transparent_type (name);

      if (newtype)
	{
	  /* If the resolved type and the stub are in the same
	     objfile, then replace the stub type with the real deal.
	     But if they're in separate objfiles, leave the stub
	     alone; we'll just look up the transparent type every time
	     we call check_typedef.  We can't create pointers between
	     types allocated to different objfiles, since they may
	     have different lifetimes.  Trying to copy NEWTYPE over to
	     TYPE's objfile is pointless, too, since you'll have to
	     move over any other types NEWTYPE refers to, which could
	     be an unbounded amount of stuff.  */
	  if (TYPE_OBJFILE (newtype) == TYPE_OBJFILE (type))
	    type = make_qualified_type (newtype,
					TYPE_INSTANCE_FLAGS (type),
					type);
	  else
	    type = newtype;
	}
    }
  /* Otherwise, rely on the stub flag being set for opaque/stubbed
     types.  */
  else if (TYPE_STUB (type) && !currently_reading_symtab)
    {
      const char *name = type_name_no_tag (type);
      /* FIXME: shouldn't we separately check the TYPE_NAME and the
         TYPE_TAG_NAME, and look in STRUCT_DOMAIN and/or VAR_DOMAIN
         as appropriate?  (this code was written before TYPE_NAME and
         TYPE_TAG_NAME were separate).  */
      struct symbol *sym;

      if (name == NULL)
	{
	  stub_noname_complaint ();
	  return make_qualified_type (type, instance_flags, NULL);
	}
      sym = lookup_symbol (name, 0, STRUCT_DOMAIN, 0).symbol;
      if (sym)
        {
          /* Same as above for opaque types, we can replace the stub
             with the complete type only if they are in the same
             objfile.  */
	  if (TYPE_OBJFILE (SYMBOL_TYPE(sym)) == TYPE_OBJFILE (type))
            type = make_qualified_type (SYMBOL_TYPE (sym),
					TYPE_INSTANCE_FLAGS (type),
					type);
	  else
	    type = SYMBOL_TYPE (sym);
        }
    }

  if (TYPE_TARGET_STUB (type))
    {
      struct type *target_type = check_typedef (TYPE_TARGET_TYPE (type));

      if (TYPE_STUB (target_type) || TYPE_TARGET_STUB (target_type))
	{
	  /* Nothing we can do.  */
	}
      else if (TYPE_CODE (type) == TYPE_CODE_RANGE)
	{
	  TYPE_LENGTH (type) = TYPE_LENGTH (target_type);
	  TYPE_TARGET_STUB (type) = 0;
	}
    }

  type = make_qualified_type (type, instance_flags, NULL);

  /* Cache TYPE_LENGTH for future use.  */
  TYPE_LENGTH (orig_type) = TYPE_LENGTH (type);

  return type;
}

/* Parse a type expression in the string [P..P+LENGTH).  If an error
   occurs, silently return a void type.  */

static struct type *
safe_parse_type (struct gdbarch *gdbarch, char *p, int length)
{
  struct ui_file *saved_gdb_stderr;
  struct type *type = NULL; /* Initialize to keep gcc happy.  */

  /* Suppress error messages.  */
  saved_gdb_stderr = gdb_stderr;
  gdb_stderr = &null_stream;

  /* Call parse_and_eval_type() without fear of longjmp()s.  */
  TRY
    {
      type = parse_and_eval_type (p, length);
    }
  CATCH (except, RETURN_MASK_ERROR)
    {
      type = builtin_type (gdbarch)->builtin_void;
    }
  END_CATCH

  /* Stop suppressing error messages.  */
  gdb_stderr = saved_gdb_stderr;

  return type;
}

/* Ugly hack to convert method stubs into method types.

   He ain't kiddin'.  This demangles the name of the method into a
   string including argument types, parses out each argument type,
   generates a string casting a zero to that type, evaluates the
   string, and stuffs the resulting type into an argtype vector!!!
   Then it knows the type of the whole function (including argument
   types for overloading), which info used to be in the stab's but was
   removed to hack back the space required for them.  */

static void
check_stub_method (struct type *type, int method_id, int signature_id)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  struct fn_field *f;
  char *mangled_name = gdb_mangle_name (type, method_id, signature_id);
  char *demangled_name = gdb_demangle (mangled_name,
				       DMGL_PARAMS | DMGL_ANSI);
  char *argtypetext, *p;
  int depth = 0, argcount = 1;
  struct field *argtypes;
  struct type *mtype;

  /* Make sure we got back a function string that we can use.  */
  if (demangled_name)
    p = strchr (demangled_name, '(');
  else
    p = NULL;

  if (demangled_name == NULL || p == NULL)
    error (_("Internal: Cannot demangle mangled name `%s'."), 
	   mangled_name);

  /* Now, read in the parameters that define this type.  */
  p += 1;
  argtypetext = p;
  while (*p)
    {
      if (*p == '(' || *p == '<')
	{
	  depth += 1;
	}
      else if (*p == ')' || *p == '>')
	{
	  depth -= 1;
	}
      else if (*p == ',' && depth == 0)
	{
	  argcount += 1;
	}

      p += 1;
    }

  /* If we read one argument and it was ``void'', don't count it.  */
  if (startswith (argtypetext, "(void)"))
    argcount -= 1;

  /* We need one extra slot, for the THIS pointer.  */

  argtypes = (struct field *)
    TYPE_ALLOC (type, (argcount + 1) * sizeof (struct field));
  p = argtypetext;

  /* Add THIS pointer for non-static methods.  */
  f = TYPE_FN_FIELDLIST1 (type, method_id);
  if (TYPE_FN_FIELD_STATIC_P (f, signature_id))
    argcount = 0;
  else
    {
      argtypes[0].type = lookup_pointer_type (type);
      argcount = 1;
    }

  if (*p != ')')		/* () means no args, skip while.  */
    {
      depth = 0;
      while (*p)
	{
	  if (depth <= 0 && (*p == ',' || *p == ')'))
	    {
	      /* Avoid parsing of ellipsis, they will be handled below.
	         Also avoid ``void'' as above.  */
	      if (strncmp (argtypetext, "...", p - argtypetext) != 0
		  && strncmp (argtypetext, "void", p - argtypetext) != 0)
		{
		  argtypes[argcount].type =
		    safe_parse_type (gdbarch, argtypetext, p - argtypetext);
		  argcount += 1;
		}
	      argtypetext = p + 1;
	    }

	  if (*p == '(' || *p == '<')
	    {
	      depth += 1;
	    }
	  else if (*p == ')' || *p == '>')
	    {
	      depth -= 1;
	    }

	  p += 1;
	}
    }

  TYPE_FN_FIELD_PHYSNAME (f, signature_id) = mangled_name;

  /* Now update the old "stub" type into a real type.  */
  mtype = TYPE_FN_FIELD_TYPE (f, signature_id);
  /* MTYPE may currently be a function (TYPE_CODE_FUNC).
     We want a method (TYPE_CODE_METHOD).  */
  smash_to_method_type (mtype, type, TYPE_TARGET_TYPE (mtype),
			argtypes, argcount, p[-2] == '.');
  TYPE_STUB (mtype) = 0;
  TYPE_FN_FIELD_STUB (f, signature_id) = 0;

  xfree (demangled_name);
}

/* This is the external interface to check_stub_method, above.  This
   function unstubs all of the signatures for TYPE's METHOD_ID method
   name.  After calling this function TYPE_FN_FIELD_STUB will be
   cleared for each signature and TYPE_FN_FIELDLIST_NAME will be
   correct.

   This function unfortunately can not die until stabs do.  */

void
check_stub_method_group (struct type *type, int method_id)
{
  int len = TYPE_FN_FIELDLIST_LENGTH (type, method_id);
  struct fn_field *f = TYPE_FN_FIELDLIST1 (type, method_id);
  int j, found_stub = 0;

  for (j = 0; j < len; j++)
    if (TYPE_FN_FIELD_STUB (f, j))
      {
	found_stub = 1;
	check_stub_method (type, method_id, j);
      }

  /* GNU v3 methods with incorrect names were corrected when we read
     in type information, because it was cheaper to do it then.  The
     only GNU v2 methods with incorrect method names are operators and
     destructors; destructors were also corrected when we read in type
     information.

     Therefore the only thing we need to handle here are v2 operator
     names.  */
  if (found_stub && !startswith (TYPE_FN_FIELD_PHYSNAME (f, 0), "_Z"))
    {
      int ret;
      char dem_opname[256];

      ret = cplus_demangle_opname (TYPE_FN_FIELDLIST_NAME (type, 
							   method_id),
				   dem_opname, DMGL_ANSI);
      if (!ret)
	ret = cplus_demangle_opname (TYPE_FN_FIELDLIST_NAME (type, 
							     method_id),
				     dem_opname, 0);
      if (ret)
	TYPE_FN_FIELDLIST_NAME (type, method_id) = xstrdup (dem_opname);
    }
}

/* Ensure it is in .rodata (if available) by workarounding GCC PR 44690.  */
const struct cplus_struct_type cplus_struct_default = { };

void
allocate_cplus_struct_type (struct type *type)
{
  if (HAVE_CPLUS_STRUCT (type))
    /* Structure was already allocated.  Nothing more to do.  */
    return;

  TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_CPLUS_STUFF;
  TYPE_RAW_CPLUS_SPECIFIC (type) = (struct cplus_struct_type *)
    TYPE_ALLOC (type, sizeof (struct cplus_struct_type));
  *(TYPE_RAW_CPLUS_SPECIFIC (type)) = cplus_struct_default;
  set_type_vptr_fieldno (type, -1);
}

const struct gnat_aux_type gnat_aux_default =
  { NULL };

/* Set the TYPE's type-specific kind to TYPE_SPECIFIC_GNAT_STUFF,
   and allocate the associated gnat-specific data.  The gnat-specific
   data is also initialized to gnat_aux_default.  */

void
allocate_gnat_aux_type (struct type *type)
{
  TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_GNAT_STUFF;
  TYPE_GNAT_SPECIFIC (type) = (struct gnat_aux_type *)
    TYPE_ALLOC (type, sizeof (struct gnat_aux_type));
  *(TYPE_GNAT_SPECIFIC (type)) = gnat_aux_default;
}

/* Helper function to initialize a newly allocated type.  Set type code
   to CODE and initialize the type-specific fields accordingly.  */

static void
set_type_code (struct type *type, enum type_code code)
{
  TYPE_CODE (type) = code;

  switch (code)
    {
      case TYPE_CODE_STRUCT:
      case TYPE_CODE_UNION:
      case TYPE_CODE_NAMESPACE:
        INIT_CPLUS_SPECIFIC (type);
        break;
      case TYPE_CODE_FLT:
        TYPE_SPECIFIC_FIELD (type) = TYPE_SPECIFIC_FLOATFORMAT;
        break;
      case TYPE_CODE_FUNC:
	INIT_FUNC_SPECIFIC (type);
        break;
    }
}

/* Helper function to verify floating-point format and size.
   BIT is the type size in bits; if BIT equals -1, the size is
   determined by the floatformat.  Returns size to be used.  */

static int
verify_floatformat (int bit, const struct floatformat **floatformats)
{
  gdb_assert (floatformats != NULL);
  gdb_assert (floatformats[0] != NULL && floatformats[1] != NULL);

  if (bit == -1)
    bit = floatformats[0]->totalsize;
  gdb_assert (bit >= 0);

  size_t len = bit / TARGET_CHAR_BIT;
  gdb_assert (len >= floatformat_totalsize_bytes (floatformats[0]));
  gdb_assert (len >= floatformat_totalsize_bytes (floatformats[1]));

  return bit;
}

/* Helper function to initialize the standard scalar types.

   If NAME is non-NULL, then it is used to initialize the type name.
   Note that NAME is not copied; it is required to have a lifetime at
   least as long as OBJFILE.  */

struct type *
init_type (struct objfile *objfile, enum type_code code, int length,
	   const char *name)
{
  struct type *type;

  type = alloc_type (objfile);
  set_type_code (type, code);
  TYPE_LENGTH (type) = length;
  TYPE_NAME (type) = name;

  return type;
}

/* Allocate a TYPE_CODE_INT type structure associated with OBJFILE.
   BIT is the type size in bits.  If UNSIGNED_P is non-zero, set
   the type's TYPE_UNSIGNED flag.  NAME is the type name.  */

struct type *
init_integer_type (struct objfile *objfile,
		   int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = init_type (objfile, TYPE_CODE_INT, bit / TARGET_CHAR_BIT, name);
  if (unsigned_p)
    TYPE_UNSIGNED (t) = 1;

  return t;
}

/* Allocate a TYPE_CODE_CHAR type structure associated with OBJFILE.
   BIT is the type size in bits.  If UNSIGNED_P is non-zero, set
   the type's TYPE_UNSIGNED flag.  NAME is the type name.  */

struct type *
init_character_type (struct objfile *objfile,
		     int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = init_type (objfile, TYPE_CODE_CHAR, bit / TARGET_CHAR_BIT, name);
  if (unsigned_p)
    TYPE_UNSIGNED (t) = 1;

  return t;
}

/* Allocate a TYPE_CODE_BOOL type structure associated with OBJFILE.
   BIT is the type size in bits.  If UNSIGNED_P is non-zero, set
   the type's TYPE_UNSIGNED flag.  NAME is the type name.  */

struct type *
init_boolean_type (struct objfile *objfile,
		   int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = init_type (objfile, TYPE_CODE_BOOL, bit / TARGET_CHAR_BIT, name);
  if (unsigned_p)
    TYPE_UNSIGNED (t) = 1;

  return t;
}

/* Allocate a TYPE_CODE_FLT type structure associated with OBJFILE.
   BIT is the type size in bits; if BIT equals -1, the size is
   determined by the floatformat.  NAME is the type name.  Set the
   TYPE_FLOATFORMAT from FLOATFORMATS.  */

struct type *
init_float_type (struct objfile *objfile,
		 int bit, const char *name,
		 const struct floatformat **floatformats)
{
  struct type *t;

  bit = verify_floatformat (bit, floatformats);
  t = init_type (objfile, TYPE_CODE_FLT, bit / TARGET_CHAR_BIT, name);
  TYPE_FLOATFORMAT (t) = floatformats;

  return t;
}

/* Allocate a TYPE_CODE_DECFLOAT type structure associated with OBJFILE.
   BIT is the type size in bits.  NAME is the type name.  */

struct type *
init_decfloat_type (struct objfile *objfile, int bit, const char *name)
{
  struct type *t;

  t = init_type (objfile, TYPE_CODE_DECFLOAT, bit / TARGET_CHAR_BIT, name);
  return t;
}

/* Allocate a TYPE_CODE_COMPLEX type structure associated with OBJFILE.
   NAME is the type name.  TARGET_TYPE is the component float type.  */

struct type *
init_complex_type (struct objfile *objfile,
		   const char *name, struct type *target_type)
{
  struct type *t;

  t = init_type (objfile, TYPE_CODE_COMPLEX,
		 2 * TYPE_LENGTH (target_type), name);
  TYPE_TARGET_TYPE (t) = target_type;
  return t;
}

/* Allocate a TYPE_CODE_PTR type structure associated with OBJFILE.
   BIT is the pointer type size in bits.  NAME is the type name.
   TARGET_TYPE is the pointer target type.  Always sets the pointer type's
   TYPE_UNSIGNED flag.  */

struct type *
init_pointer_type (struct objfile *objfile,
		   int bit, const char *name, struct type *target_type)
{
  struct type *t;

  t = init_type (objfile, TYPE_CODE_PTR, bit / TARGET_CHAR_BIT, name);
  TYPE_TARGET_TYPE (t) = target_type;
  TYPE_UNSIGNED (t) = 1;
  return t;
}


/* Queries on types.  */

int
can_dereference (struct type *t)
{
  /* FIXME: Should we return true for references as well as
     pointers?  */
  t = check_typedef (t);
  return
    (t != NULL
     && TYPE_CODE (t) == TYPE_CODE_PTR
     && TYPE_CODE (TYPE_TARGET_TYPE (t)) != TYPE_CODE_VOID);
}

int
is_integral_type (struct type *t)
{
  t = check_typedef (t);
  return
    ((t != NULL)
     && ((TYPE_CODE (t) == TYPE_CODE_INT)
	 || (TYPE_CODE (t) == TYPE_CODE_ENUM)
	 || (TYPE_CODE (t) == TYPE_CODE_FLAGS)
	 || (TYPE_CODE (t) == TYPE_CODE_CHAR)
	 || (TYPE_CODE (t) == TYPE_CODE_RANGE)
	 || (TYPE_CODE (t) == TYPE_CODE_BOOL)));
}

/* Return true if TYPE is scalar.  */

int
is_scalar_type (struct type *type)
{
  type = check_typedef (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_SET:
    case TYPE_CODE_STRING:
      return 0;
    default:
      return 1;
    }
}

/* Return true if T is scalar, or a composite type which in practice has
   the memory layout of a scalar type.  E.g., an array or struct with only
   one scalar element inside it, or a union with only scalar elements.  */

int
is_scalar_type_recursive (struct type *t)
{
  t = check_typedef (t);

  if (is_scalar_type (t))
    return 1;
  /* Are we dealing with an array or string of known dimensions?  */
  else if ((TYPE_CODE (t) == TYPE_CODE_ARRAY
	    || TYPE_CODE (t) == TYPE_CODE_STRING) && TYPE_NFIELDS (t) == 1
	   && TYPE_CODE (TYPE_INDEX_TYPE (t)) == TYPE_CODE_RANGE)
    {
      LONGEST low_bound, high_bound;
      struct type *elt_type = check_typedef (TYPE_TARGET_TYPE (t));

      get_discrete_bounds (TYPE_INDEX_TYPE (t), &low_bound, &high_bound);

      return high_bound == low_bound && is_scalar_type_recursive (elt_type);
    }
  /* Are we dealing with a struct with one element?  */
  else if (TYPE_CODE (t) == TYPE_CODE_STRUCT && TYPE_NFIELDS (t) == 1)
    return is_scalar_type_recursive (TYPE_FIELD_TYPE (t, 0));
  else if (TYPE_CODE (t) == TYPE_CODE_UNION)
    {
      int i, n = TYPE_NFIELDS (t);

      /* If all elements of the union are scalar, then the union is scalar.  */
      for (i = 0; i < n; i++)
	if (!is_scalar_type_recursive (TYPE_FIELD_TYPE (t, i)))
	  return 0;

      return 1;
    }

  return 0;
}

/* Return true is T is a class or a union.  False otherwise.  */

int
class_or_union_p (const struct type *t)
{
  return (TYPE_CODE (t) == TYPE_CODE_STRUCT
          || TYPE_CODE (t) == TYPE_CODE_UNION);
}

/* A helper function which returns true if types A and B represent the
   "same" class type.  This is true if the types have the same main
   type, or the same name.  */

int
class_types_same_p (const struct type *a, const struct type *b)
{
  return (TYPE_MAIN_TYPE (a) == TYPE_MAIN_TYPE (b)
	  || (TYPE_NAME (a) && TYPE_NAME (b)
	      && !strcmp (TYPE_NAME (a), TYPE_NAME (b))));
}

/* If BASE is an ancestor of DCLASS return the distance between them.
   otherwise return -1;
   eg:

   class A {};
   class B: public A {};
   class C: public B {};
   class D: C {};

   distance_to_ancestor (A, A, 0) = 0
   distance_to_ancestor (A, B, 0) = 1
   distance_to_ancestor (A, C, 0) = 2
   distance_to_ancestor (A, D, 0) = 3

   If PUBLIC is 1 then only public ancestors are considered,
   and the function returns the distance only if BASE is a public ancestor
   of DCLASS.
   Eg:

   distance_to_ancestor (A, D, 1) = -1.  */

static int
distance_to_ancestor (struct type *base, struct type *dclass, int is_public)
{
  int i;
  int d;

  base = check_typedef (base);
  dclass = check_typedef (dclass);

  if (class_types_same_p (base, dclass))
    return 0;

  for (i = 0; i < TYPE_N_BASECLASSES (dclass); i++)
    {
      if (is_public && ! BASETYPE_VIA_PUBLIC (dclass, i))
	continue;

      d = distance_to_ancestor (base, TYPE_BASECLASS (dclass, i), is_public);
      if (d >= 0)
	return 1 + d;
    }

  return -1;
}

/* Check whether BASE is an ancestor or base class or DCLASS
   Return 1 if so, and 0 if not.
   Note: If BASE and DCLASS are of the same type, this function
   will return 1. So for some class A, is_ancestor (A, A) will
   return 1.  */

int
is_ancestor (struct type *base, struct type *dclass)
{
  return distance_to_ancestor (base, dclass, 0) >= 0;
}

/* Like is_ancestor, but only returns true when BASE is a public
   ancestor of DCLASS.  */

int
is_public_ancestor (struct type *base, struct type *dclass)
{
  return distance_to_ancestor (base, dclass, 1) >= 0;
}

/* A helper function for is_unique_ancestor.  */

static int
is_unique_ancestor_worker (struct type *base, struct type *dclass,
			   int *offset,
			   const gdb_byte *valaddr, int embedded_offset,
			   CORE_ADDR address, struct value *val)
{
  int i, count = 0;

  base = check_typedef (base);
  dclass = check_typedef (dclass);

  for (i = 0; i < TYPE_N_BASECLASSES (dclass) && count < 2; ++i)
    {
      struct type *iter;
      int this_offset;

      iter = check_typedef (TYPE_BASECLASS (dclass, i));

      this_offset = baseclass_offset (dclass, i, valaddr, embedded_offset,
				      address, val);

      if (class_types_same_p (base, iter))
	{
	  /* If this is the first subclass, set *OFFSET and set count
	     to 1.  Otherwise, if this is at the same offset as
	     previous instances, do nothing.  Otherwise, increment
	     count.  */
	  if (*offset == -1)
	    {
	      *offset = this_offset;
	      count = 1;
	    }
	  else if (this_offset == *offset)
	    {
	      /* Nothing.  */
	    }
	  else
	    ++count;
	}
      else
	count += is_unique_ancestor_worker (base, iter, offset,
					    valaddr,
					    embedded_offset + this_offset,
					    address, val);
    }

  return count;
}

/* Like is_ancestor, but only returns true if BASE is a unique base
   class of the type of VAL.  */

int
is_unique_ancestor (struct type *base, struct value *val)
{
  int offset = -1;

  return is_unique_ancestor_worker (base, value_type (val), &offset,
				    value_contents_for_printing (val),
				    value_embedded_offset (val),
				    value_address (val), val) == 1;
}


/* Overload resolution.  */

/* Return the sum of the rank of A with the rank of B.  */

struct rank
sum_ranks (struct rank a, struct rank b)
{
  struct rank c;
  c.rank = a.rank + b.rank;
  c.subrank = a.subrank + b.subrank;
  return c;
}

/* Compare rank A and B and return:
   0 if a = b
   1 if a is better than b
  -1 if b is better than a.  */

int
compare_ranks (struct rank a, struct rank b)
{
  if (a.rank == b.rank)
    {
      if (a.subrank == b.subrank)
	return 0;
      if (a.subrank < b.subrank)
	return 1;
      if (a.subrank > b.subrank)
	return -1;
    }

  if (a.rank < b.rank)
    return 1;

  /* a.rank > b.rank */
  return -1;
}

/* Functions for overload resolution begin here.  */

/* Compare two badness vectors A and B and return the result.
   0 => A and B are identical
   1 => A and B are incomparable
   2 => A is better than B
   3 => A is worse than B  */

int
compare_badness (struct badness_vector *a, struct badness_vector *b)
{
  int i;
  int tmp;
  short found_pos = 0;		/* any positives in c? */
  short found_neg = 0;		/* any negatives in c? */

  /* differing lengths => incomparable */
  if (a->length != b->length)
    return 1;

  /* Subtract b from a */
  for (i = 0; i < a->length; i++)
    {
      tmp = compare_ranks (b->rank[i], a->rank[i]);
      if (tmp > 0)
	found_pos = 1;
      else if (tmp < 0)
	found_neg = 1;
    }

  if (found_pos)
    {
      if (found_neg)
	return 1;		/* incomparable */
      else
	return 3;		/* A > B */
    }
  else
    /* no positives */
    {
      if (found_neg)
	return 2;		/* A < B */
      else
	return 0;		/* A == B */
    }
}

/* Rank a function by comparing its parameter types (PARMS, length
   NPARMS), to the types of an argument list (ARGS, length NARGS).
   Return a pointer to a badness vector.  This has NARGS + 1
   entries.  */

struct badness_vector *
rank_function (struct type **parms, int nparms, 
	       struct value **args, int nargs)
{
  int i;
  struct badness_vector *bv = XNEW (struct badness_vector);
  int min_len = nparms < nargs ? nparms : nargs;

  bv->length = nargs + 1;	/* add 1 for the length-match rank.  */
  bv->rank = XNEWVEC (struct rank, nargs + 1);

  /* First compare the lengths of the supplied lists.
     If there is a mismatch, set it to a high value.  */

  /* pai/1997-06-03 FIXME: when we have debug info about default
     arguments and ellipsis parameter lists, we should consider those
     and rank the length-match more finely.  */

  LENGTH_MATCH (bv) = (nargs != nparms)
		      ? LENGTH_MISMATCH_BADNESS
		      : EXACT_MATCH_BADNESS;

  /* Now rank all the parameters of the candidate function.  */
  for (i = 1; i <= min_len; i++)
    bv->rank[i] = rank_one_type (parms[i - 1], value_type (args[i - 1]),
				 args[i - 1]);

  /* If more arguments than parameters, add dummy entries.  */
  for (i = min_len + 1; i <= nargs; i++)
    bv->rank[i] = TOO_FEW_PARAMS_BADNESS;

  return bv;
}

/* Compare the names of two integer types, assuming that any sign
   qualifiers have been checked already.  We do it this way because
   there may be an "int" in the name of one of the types.  */

static int
integer_types_same_name_p (const char *first, const char *second)
{
  int first_p, second_p;

  /* If both are shorts, return 1; if neither is a short, keep
     checking.  */
  first_p = (strstr (first, "short") != NULL);
  second_p = (strstr (second, "short") != NULL);
  if (first_p && second_p)
    return 1;
  if (first_p || second_p)
    return 0;

  /* Likewise for long.  */
  first_p = (strstr (first, "long") != NULL);
  second_p = (strstr (second, "long") != NULL);
  if (first_p && second_p)
    return 1;
  if (first_p || second_p)
    return 0;

  /* Likewise for char.  */
  first_p = (strstr (first, "char") != NULL);
  second_p = (strstr (second, "char") != NULL);
  if (first_p && second_p)
    return 1;
  if (first_p || second_p)
    return 0;

  /* They must both be ints.  */
  return 1;
}

/* Compares type A to type B returns 1 if the represent the same type
   0 otherwise.  */

int
types_equal (struct type *a, struct type *b)
{
  /* Identical type pointers.  */
  /* However, this still doesn't catch all cases of same type for b
     and a.  The reason is that builtin types are different from
     the same ones constructed from the object.  */
  if (a == b)
    return 1;

  /* Resolve typedefs */
  if (TYPE_CODE (a) == TYPE_CODE_TYPEDEF)
    a = check_typedef (a);
  if (TYPE_CODE (b) == TYPE_CODE_TYPEDEF)
    b = check_typedef (b);

  /* If after resolving typedefs a and b are not of the same type
     code then they are not equal.  */
  if (TYPE_CODE (a) != TYPE_CODE (b))
    return 0;

  /* If a and b are both pointers types or both reference types then
     they are equal of the same type iff the objects they refer to are
     of the same type.  */
  if (TYPE_CODE (a) == TYPE_CODE_PTR
      || TYPE_CODE (a) == TYPE_CODE_REF)
    return types_equal (TYPE_TARGET_TYPE (a),
                        TYPE_TARGET_TYPE (b));

  /* Well, damnit, if the names are exactly the same, I'll say they
     are exactly the same.  This happens when we generate method
     stubs.  The types won't point to the same address, but they
     really are the same.  */

  if (TYPE_NAME (a) && TYPE_NAME (b)
      && strcmp (TYPE_NAME (a), TYPE_NAME (b)) == 0)
    return 1;

  /* Check if identical after resolving typedefs.  */
  if (a == b)
    return 1;

  /* Two function types are equal if their argument and return types
     are equal.  */
  if (TYPE_CODE (a) == TYPE_CODE_FUNC)
    {
      int i;

      if (TYPE_NFIELDS (a) != TYPE_NFIELDS (b))
	return 0;
      
      if (!types_equal (TYPE_TARGET_TYPE (a), TYPE_TARGET_TYPE (b)))
	return 0;

      for (i = 0; i < TYPE_NFIELDS (a); ++i)
	if (!types_equal (TYPE_FIELD_TYPE (a, i), TYPE_FIELD_TYPE (b, i)))
	  return 0;

      return 1;
    }

  return 0;
}

/* Deep comparison of types.  */

/* An entry in the type-equality bcache.  */

typedef struct type_equality_entry
{
  struct type *type1, *type2;
} type_equality_entry_d;

DEF_VEC_O (type_equality_entry_d);

/* A helper function to compare two strings.  Returns 1 if they are
   the same, 0 otherwise.  Handles NULLs properly.  */

static int
compare_maybe_null_strings (const char *s, const char *t)
{
  if (s == NULL && t != NULL)
    return 0;
  else if (s != NULL && t == NULL)
    return 0;
  else if (s == NULL && t== NULL)
    return 1;
  return strcmp (s, t) == 0;
}

/* A helper function for check_types_worklist that checks two types for
   "deep" equality.  Returns non-zero if the types are considered the
   same, zero otherwise.  */

static int
check_types_equal (struct type *type1, struct type *type2,
		   VEC (type_equality_entry_d) **worklist)
{
  type1 = check_typedef (type1);
  type2 = check_typedef (type2);

  if (type1 == type2)
    return 1;

  if (TYPE_CODE (type1) != TYPE_CODE (type2)
      || TYPE_LENGTH (type1) != TYPE_LENGTH (type2)
      || TYPE_UNSIGNED (type1) != TYPE_UNSIGNED (type2)
      || TYPE_NOSIGN (type1) != TYPE_NOSIGN (type2)
      || TYPE_VARARGS (type1) != TYPE_VARARGS (type2)
      || TYPE_VECTOR (type1) != TYPE_VECTOR (type2)
      || TYPE_NOTTEXT (type1) != TYPE_NOTTEXT (type2)
      || TYPE_INSTANCE_FLAGS (type1) != TYPE_INSTANCE_FLAGS (type2)
      || TYPE_NFIELDS (type1) != TYPE_NFIELDS (type2))
    return 0;

  if (!compare_maybe_null_strings (TYPE_TAG_NAME (type1),
				   TYPE_TAG_NAME (type2)))
    return 0;
  if (!compare_maybe_null_strings (TYPE_NAME (type1), TYPE_NAME (type2)))
    return 0;

  if (TYPE_CODE (type1) == TYPE_CODE_RANGE)
    {
      if (memcmp (TYPE_RANGE_DATA (type1), TYPE_RANGE_DATA (type2),
		  sizeof (*TYPE_RANGE_DATA (type1))) != 0)
	return 0;
    }
  else
    {
      int i;

      for (i = 0; i < TYPE_NFIELDS (type1); ++i)
	{
	  const struct field *field1 = &TYPE_FIELD (type1, i);
	  const struct field *field2 = &TYPE_FIELD (type2, i);
	  struct type_equality_entry entry;

	  if (FIELD_ARTIFICIAL (*field1) != FIELD_ARTIFICIAL (*field2)
	      || FIELD_BITSIZE (*field1) != FIELD_BITSIZE (*field2)
	      || FIELD_LOC_KIND (*field1) != FIELD_LOC_KIND (*field2))
	    return 0;
	  if (!compare_maybe_null_strings (FIELD_NAME (*field1),
					   FIELD_NAME (*field2)))
	    return 0;
	  switch (FIELD_LOC_KIND (*field1))
	    {
	    case FIELD_LOC_KIND_BITPOS:
	      if (FIELD_BITPOS (*field1) != FIELD_BITPOS (*field2))
		return 0;
	      break;
	    case FIELD_LOC_KIND_ENUMVAL:
	      if (FIELD_ENUMVAL (*field1) != FIELD_ENUMVAL (*field2))
		return 0;
	      break;
	    case FIELD_LOC_KIND_PHYSADDR:
	      if (FIELD_STATIC_PHYSADDR (*field1)
		  != FIELD_STATIC_PHYSADDR (*field2))
		return 0;
	      break;
	    case FIELD_LOC_KIND_PHYSNAME:
	      if (!compare_maybe_null_strings (FIELD_STATIC_PHYSNAME (*field1),
					       FIELD_STATIC_PHYSNAME (*field2)))
		return 0;
	      break;
	    case FIELD_LOC_KIND_DWARF_BLOCK:
	      {
		struct dwarf2_locexpr_baton *block1, *block2;

		block1 = FIELD_DWARF_BLOCK (*field1);
		block2 = FIELD_DWARF_BLOCK (*field2);
		if (block1->per_cu != block2->per_cu
		    || block1->size != block2->size
		    || memcmp (block1->data, block2->data, block1->size) != 0)
		  return 0;
	      }
	      break;
	    default:
	      internal_error (__FILE__, __LINE__, _("Unsupported field kind "
						    "%d by check_types_equal"),
			      FIELD_LOC_KIND (*field1));
	    }

	  entry.type1 = FIELD_TYPE (*field1);
	  entry.type2 = FIELD_TYPE (*field2);
	  VEC_safe_push (type_equality_entry_d, *worklist, &entry);
	}
    }

  if (TYPE_TARGET_TYPE (type1) != NULL)
    {
      struct type_equality_entry entry;

      if (TYPE_TARGET_TYPE (type2) == NULL)
	return 0;

      entry.type1 = TYPE_TARGET_TYPE (type1);
      entry.type2 = TYPE_TARGET_TYPE (type2);
      VEC_safe_push (type_equality_entry_d, *worklist, &entry);
    }
  else if (TYPE_TARGET_TYPE (type2) != NULL)
    return 0;

  return 1;
}

/* Check types on a worklist for equality.  Returns zero if any pair
   is not equal, non-zero if they are all considered equal.  */

static int
check_types_worklist (VEC (type_equality_entry_d) **worklist,
		      struct bcache *cache)
{
  while (!VEC_empty (type_equality_entry_d, *worklist))
    {
      struct type_equality_entry entry;
      int added;

      entry = *VEC_last (type_equality_entry_d, *worklist);
      VEC_pop (type_equality_entry_d, *worklist);

      /* If the type pair has already been visited, we know it is
	 ok.  */
      bcache_full (&entry, sizeof (entry), cache, &added);
      if (!added)
	continue;

      if (check_types_equal (entry.type1, entry.type2, worklist) == 0)
	return 0;
    }

  return 1;
}

/* Return non-zero if types TYPE1 and TYPE2 are equal, as determined by a
   "deep comparison".  Otherwise return zero.  */

int
types_deeply_equal (struct type *type1, struct type *type2)
{
  struct gdb_exception except = exception_none;
  int result = 0;
  struct bcache *cache;
  VEC (type_equality_entry_d) *worklist = NULL;
  struct type_equality_entry entry;

  gdb_assert (type1 != NULL && type2 != NULL);

  /* Early exit for the simple case.  */
  if (type1 == type2)
    return 1;

  cache = bcache_xmalloc (NULL, NULL);

  entry.type1 = type1;
  entry.type2 = type2;
  VEC_safe_push (type_equality_entry_d, worklist, &entry);

  /* check_types_worklist calls several nested helper functions, some
     of which can raise a GDB exception, so we just check and rethrow
     here.  If there is a GDB exception, a comparison is not capable
     (or trusted), so exit.  */
  TRY
    {
      result = check_types_worklist (&worklist, cache);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      except = ex;
    }
  END_CATCH

  bcache_xfree (cache);
  VEC_free (type_equality_entry_d, worklist);

  /* Rethrow if there was a problem.  */
  if (except.reason < 0)
    throw_exception (except);

  return result;
}

/* Allocated status of type TYPE.  Return zero if type TYPE is allocated.
   Otherwise return one.  */

int
type_not_allocated (const struct type *type)
{
  struct dynamic_prop *prop = TYPE_ALLOCATED_PROP (type);

  return (prop && TYPE_DYN_PROP_KIND (prop) == PROP_CONST
         && !TYPE_DYN_PROP_ADDR (prop));
}

/* Associated status of type TYPE.  Return zero if type TYPE is associated.
   Otherwise return one.  */

int
type_not_associated (const struct type *type)
{
  struct dynamic_prop *prop = TYPE_ASSOCIATED_PROP (type);

  return (prop && TYPE_DYN_PROP_KIND (prop) == PROP_CONST
         && !TYPE_DYN_PROP_ADDR (prop));
}

/* Compare one type (PARM) for compatibility with another (ARG).
 * PARM is intended to be the parameter type of a function; and
 * ARG is the supplied argument's type.  This function tests if
 * the latter can be converted to the former.
 * VALUE is the argument's value or NULL if none (or called recursively)
 *
 * Return 0 if they are identical types;
 * Otherwise, return an integer which corresponds to how compatible
 * PARM is to ARG.  The higher the return value, the worse the match.
 * Generally the "bad" conversions are all uniformly assigned a 100.  */

struct rank
rank_one_type (struct type *parm, struct type *arg, struct value *value)
{
  struct rank rank = {0,0};

  /* Resolve typedefs */
  if (TYPE_CODE (parm) == TYPE_CODE_TYPEDEF)
    parm = check_typedef (parm);
  if (TYPE_CODE (arg) == TYPE_CODE_TYPEDEF)
    arg = check_typedef (arg);

  if (TYPE_IS_REFERENCE (parm) && value != NULL)
    {
      if (VALUE_LVAL (value) == not_lval)
	{
	  /* Rvalues should preferably bind to rvalue references or const
	     lvalue references.  */
	  if (TYPE_CODE (parm) == TYPE_CODE_RVALUE_REF)
	    rank.subrank = REFERENCE_CONVERSION_RVALUE;
	  else if (TYPE_CONST (TYPE_TARGET_TYPE (parm)))
	    rank.subrank = REFERENCE_CONVERSION_CONST_LVALUE;
	  else
	    return INCOMPATIBLE_TYPE_BADNESS;
	  return sum_ranks (rank, REFERENCE_CONVERSION_BADNESS);
	}
      else
	{
	  /* Lvalues should prefer lvalue overloads.  */
	  if (TYPE_CODE (parm) == TYPE_CODE_RVALUE_REF)
	    {
	      rank.subrank = REFERENCE_CONVERSION_RVALUE;
	      return sum_ranks (rank, REFERENCE_CONVERSION_BADNESS);
	    }
	}
    }

  if (types_equal (parm, arg))
    {
      struct type *t1 = parm;
      struct type *t2 = arg;

      /* For pointers and references, compare target type.  */
      if (TYPE_CODE (parm) == TYPE_CODE_PTR || TYPE_IS_REFERENCE (parm))
	{
	  t1 = TYPE_TARGET_TYPE (parm);
	  t2 = TYPE_TARGET_TYPE (arg);
	}

      /* Make sure they are CV equal, too.  */
      if (TYPE_CONST (t1) != TYPE_CONST (t2))
	rank.subrank |= CV_CONVERSION_CONST;
      if (TYPE_VOLATILE (t1) != TYPE_VOLATILE (t2))
	rank.subrank |= CV_CONVERSION_VOLATILE;
      if (rank.subrank != 0)
	return sum_ranks (CV_CONVERSION_BADNESS, rank);
      return EXACT_MATCH_BADNESS;
    }

  /* See through references, since we can almost make non-references
     references.  */

  if (TYPE_IS_REFERENCE (arg))
    return (sum_ranks (rank_one_type (parm, TYPE_TARGET_TYPE (arg), NULL),
                       REFERENCE_CONVERSION_BADNESS));
  if (TYPE_IS_REFERENCE (parm))
    return (sum_ranks (rank_one_type (TYPE_TARGET_TYPE (parm), arg, NULL),
                       REFERENCE_CONVERSION_BADNESS));
  if (overload_debug)
  /* Debugging only.  */
    fprintf_filtered (gdb_stderr, 
		      "------ Arg is %s [%d], parm is %s [%d]\n",
		      TYPE_NAME (arg), TYPE_CODE (arg), 
		      TYPE_NAME (parm), TYPE_CODE (parm));

  /* x -> y means arg of type x being supplied for parameter of type y.  */

  switch (TYPE_CODE (parm))
    {
    case TYPE_CODE_PTR:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_PTR:

	  /* Allowed pointer conversions are:
	     (a) pointer to void-pointer conversion.  */
	  if (TYPE_CODE (TYPE_TARGET_TYPE (parm)) == TYPE_CODE_VOID)
	    return VOID_PTR_CONVERSION_BADNESS;

	  /* (b) pointer to ancestor-pointer conversion.  */
	  rank.subrank = distance_to_ancestor (TYPE_TARGET_TYPE (parm),
	                                       TYPE_TARGET_TYPE (arg),
	                                       0);
	  if (rank.subrank >= 0)
	    return sum_ranks (BASE_PTR_CONVERSION_BADNESS, rank);

	  return INCOMPATIBLE_TYPE_BADNESS;
	case TYPE_CODE_ARRAY:
	  {
	    struct type *t1 = TYPE_TARGET_TYPE (parm);
	    struct type *t2 = TYPE_TARGET_TYPE (arg);

	    if (types_equal (t1, t2))
	      {
		/* Make sure they are CV equal.  */
		if (TYPE_CONST (t1) != TYPE_CONST (t2))
		  rank.subrank |= CV_CONVERSION_CONST;
		if (TYPE_VOLATILE (t1) != TYPE_VOLATILE (t2))
		  rank.subrank |= CV_CONVERSION_VOLATILE;
		if (rank.subrank != 0)
		  return sum_ranks (CV_CONVERSION_BADNESS, rank);
		return EXACT_MATCH_BADNESS;
	      }
	    return INCOMPATIBLE_TYPE_BADNESS;
	  }
	case TYPE_CODE_FUNC:
	  return rank_one_type (TYPE_TARGET_TYPE (parm), arg, NULL);
	case TYPE_CODE_INT:
	  if (value != NULL && TYPE_CODE (value_type (value)) == TYPE_CODE_INT)
	    {
	      if (value_as_long (value) == 0)
		{
		  /* Null pointer conversion: allow it to be cast to a pointer.
		     [4.10.1 of C++ standard draft n3290]  */
		  return NULL_POINTER_CONVERSION_BADNESS;
		}
	      else
		{
		  /* If type checking is disabled, allow the conversion.  */
		  if (!strict_type_checking)
		    return NS_INTEGER_POINTER_CONVERSION_BADNESS;
		}
	    }
	  /* fall through  */
	case TYPE_CODE_ENUM:
	case TYPE_CODE_FLAGS:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_BOOL:
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
    case TYPE_CODE_ARRAY:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_PTR:
	case TYPE_CODE_ARRAY:
	  return rank_one_type (TYPE_TARGET_TYPE (parm), 
				TYPE_TARGET_TYPE (arg), NULL);
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
    case TYPE_CODE_FUNC:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_PTR:	/* funcptr -> func */
	  return rank_one_type (parm, TYPE_TARGET_TYPE (arg), NULL);
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
    case TYPE_CODE_INT:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_INT:
	  if (TYPE_LENGTH (arg) == TYPE_LENGTH (parm))
	    {
	      /* Deal with signed, unsigned, and plain chars and
	         signed and unsigned ints.  */
	      if (TYPE_NOSIGN (parm))
		{
		  /* This case only for character types.  */
		  if (TYPE_NOSIGN (arg))
		    return EXACT_MATCH_BADNESS;	/* plain char -> plain char */
		  else		/* signed/unsigned char -> plain char */
		    return INTEGER_CONVERSION_BADNESS;
		}
	      else if (TYPE_UNSIGNED (parm))
		{
		  if (TYPE_UNSIGNED (arg))
		    {
		      /* unsigned int -> unsigned int, or 
			 unsigned long -> unsigned long */
		      if (integer_types_same_name_p (TYPE_NAME (parm), 
						     TYPE_NAME (arg)))
			return EXACT_MATCH_BADNESS;
		      else if (integer_types_same_name_p (TYPE_NAME (arg), 
							  "int")
			       && integer_types_same_name_p (TYPE_NAME (parm),
							     "long"))
			/* unsigned int -> unsigned long */
			return INTEGER_PROMOTION_BADNESS;
		      else
			/* unsigned long -> unsigned int */
			return INTEGER_CONVERSION_BADNESS;
		    }
		  else
		    {
		      if (integer_types_same_name_p (TYPE_NAME (arg), 
						     "long")
			  && integer_types_same_name_p (TYPE_NAME (parm), 
							"int"))
			/* signed long -> unsigned int */
			return INTEGER_CONVERSION_BADNESS;
		      else
			/* signed int/long -> unsigned int/long */
			return INTEGER_CONVERSION_BADNESS;
		    }
		}
	      else if (!TYPE_NOSIGN (arg) && !TYPE_UNSIGNED (arg))
		{
		  if (integer_types_same_name_p (TYPE_NAME (parm), 
						 TYPE_NAME (arg)))
		    return EXACT_MATCH_BADNESS;
		  else if (integer_types_same_name_p (TYPE_NAME (arg), 
						      "int")
			   && integer_types_same_name_p (TYPE_NAME (parm), 
							 "long"))
		    return INTEGER_PROMOTION_BADNESS;
		  else
		    return INTEGER_CONVERSION_BADNESS;
		}
	      else
		return INTEGER_CONVERSION_BADNESS;
	    }
	  else if (TYPE_LENGTH (arg) < TYPE_LENGTH (parm))
	    return INTEGER_PROMOTION_BADNESS;
	  else
	    return INTEGER_CONVERSION_BADNESS;
	case TYPE_CODE_ENUM:
	case TYPE_CODE_FLAGS:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_BOOL:
	  if (TYPE_DECLARED_CLASS (arg))
	    return INCOMPATIBLE_TYPE_BADNESS;
	  return INTEGER_PROMOTION_BADNESS;
	case TYPE_CODE_FLT:
	  return INT_FLOAT_CONVERSION_BADNESS;
	case TYPE_CODE_PTR:
	  return NS_POINTER_CONVERSION_BADNESS;
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_ENUM:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_ENUM:
	  if (TYPE_DECLARED_CLASS (parm) || TYPE_DECLARED_CLASS (arg))
	    return INCOMPATIBLE_TYPE_BADNESS;
	  return INTEGER_CONVERSION_BADNESS;
	case TYPE_CODE_FLT:
	  return INT_FLOAT_CONVERSION_BADNESS;
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_CHAR:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_RANGE:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_ENUM:
	  if (TYPE_DECLARED_CLASS (arg))
	    return INCOMPATIBLE_TYPE_BADNESS;
	  return INTEGER_CONVERSION_BADNESS;
	case TYPE_CODE_FLT:
	  return INT_FLOAT_CONVERSION_BADNESS;
	case TYPE_CODE_INT:
	  if (TYPE_LENGTH (arg) > TYPE_LENGTH (parm))
	    return INTEGER_CONVERSION_BADNESS;
	  else if (TYPE_LENGTH (arg) < TYPE_LENGTH (parm))
	    return INTEGER_PROMOTION_BADNESS;
	  /* >>> !! else fall through !! <<< */
	case TYPE_CODE_CHAR:
	  /* Deal with signed, unsigned, and plain chars for C++ and
	     with int cases falling through from previous case.  */
	  if (TYPE_NOSIGN (parm))
	    {
	      if (TYPE_NOSIGN (arg))
		return EXACT_MATCH_BADNESS;
	      else
		return INTEGER_CONVERSION_BADNESS;
	    }
	  else if (TYPE_UNSIGNED (parm))
	    {
	      if (TYPE_UNSIGNED (arg))
		return EXACT_MATCH_BADNESS;
	      else
		return INTEGER_PROMOTION_BADNESS;
	    }
	  else if (!TYPE_NOSIGN (arg) && !TYPE_UNSIGNED (arg))
	    return EXACT_MATCH_BADNESS;
	  else
	    return INTEGER_CONVERSION_BADNESS;
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_RANGE:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_ENUM:
	  return INTEGER_CONVERSION_BADNESS;
	case TYPE_CODE_FLT:
	  return INT_FLOAT_CONVERSION_BADNESS;
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_BOOL:
      switch (TYPE_CODE (arg))
	{
	  /* n3290 draft, section 4.12.1 (conv.bool):

	     "A prvalue of arithmetic, unscoped enumeration, pointer, or
	     pointer to member type can be converted to a prvalue of type
	     bool.  A zero value, null pointer value, or null member pointer
	     value is converted to false; any other value is converted to
	     true.  A prvalue of type std::nullptr_t can be converted to a
	     prvalue of type bool; the resulting value is false."  */
	case TYPE_CODE_INT:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_ENUM:
	case TYPE_CODE_FLT:
	case TYPE_CODE_MEMBERPTR:
	case TYPE_CODE_PTR:
	  return BOOL_CONVERSION_BADNESS;
	case TYPE_CODE_RANGE:
	  return INCOMPATIBLE_TYPE_BADNESS;
	case TYPE_CODE_BOOL:
	  return EXACT_MATCH_BADNESS;
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_FLT:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_FLT:
	  if (TYPE_LENGTH (arg) < TYPE_LENGTH (parm))
	    return FLOAT_PROMOTION_BADNESS;
	  else if (TYPE_LENGTH (arg) == TYPE_LENGTH (parm))
	    return EXACT_MATCH_BADNESS;
	  else
	    return FLOAT_CONVERSION_BADNESS;
	case TYPE_CODE_INT:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_ENUM:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_CHAR:
	  return INT_FLOAT_CONVERSION_BADNESS;
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_COMPLEX:
      switch (TYPE_CODE (arg))
	{		/* Strictly not needed for C++, but...  */
	case TYPE_CODE_FLT:
	  return FLOAT_PROMOTION_BADNESS;
	case TYPE_CODE_COMPLEX:
	  return EXACT_MATCH_BADNESS;
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_STRUCT:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_STRUCT:
	  /* Check for derivation */
	  rank.subrank = distance_to_ancestor (parm, arg, 0);
	  if (rank.subrank >= 0)
	    return sum_ranks (BASE_CONVERSION_BADNESS, rank);
	  /* else fall through */
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_UNION:
      switch (TYPE_CODE (arg))
	{
	case TYPE_CODE_UNION:
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_MEMBERPTR:
      switch (TYPE_CODE (arg))
	{
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_METHOD:
      switch (TYPE_CODE (arg))
	{

	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_REF:
      switch (TYPE_CODE (arg))
	{

	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}

      break;
    case TYPE_CODE_SET:
      switch (TYPE_CODE (arg))
	{
	  /* Not in C++ */
	case TYPE_CODE_SET:
	  return rank_one_type (TYPE_FIELD_TYPE (parm, 0), 
				TYPE_FIELD_TYPE (arg, 0), NULL);
	default:
	  return INCOMPATIBLE_TYPE_BADNESS;
	}
      break;
    case TYPE_CODE_VOID:
    default:
      return INCOMPATIBLE_TYPE_BADNESS;
    }				/* switch (TYPE_CODE (arg)) */
}

/* End of functions for overload resolution.  */

/* Routines to pretty-print types.  */

static void
print_bit_vector (B_TYPE *bits, int nbits)
{
  int bitno;

  for (bitno = 0; bitno < nbits; bitno++)
    {
      if ((bitno % 8) == 0)
	{
	  puts_filtered (" ");
	}
      if (B_TST (bits, bitno))
	printf_filtered (("1"));
      else
	printf_filtered (("0"));
    }
}

/* Note the first arg should be the "this" pointer, we may not want to
   include it since we may get into a infinitely recursive
   situation.  */

static void
print_args (struct field *args, int nargs, int spaces)
{
  if (args != NULL)
    {
      int i;

      for (i = 0; i < nargs; i++)
	{
	  printfi_filtered (spaces, "[%d] name '%s'\n", i,
			    args[i].name != NULL ? args[i].name : "<NULL>");
	  recursive_dump_type (args[i].type, spaces + 2);
	}
    }
}

int
field_is_static (struct field *f)
{
  /* "static" fields are the fields whose location is not relative
     to the address of the enclosing struct.  It would be nice to
     have a dedicated flag that would be set for static fields when
     the type is being created.  But in practice, checking the field
     loc_kind should give us an accurate answer.  */
  return (FIELD_LOC_KIND (*f) == FIELD_LOC_KIND_PHYSNAME
	  || FIELD_LOC_KIND (*f) == FIELD_LOC_KIND_PHYSADDR);
}

static void
dump_fn_fieldlists (struct type *type, int spaces)
{
  int method_idx;
  int overload_idx;
  struct fn_field *f;

  printfi_filtered (spaces, "fn_fieldlists ");
  gdb_print_host_address (TYPE_FN_FIELDLISTS (type), gdb_stdout);
  printf_filtered ("\n");
  for (method_idx = 0; method_idx < TYPE_NFN_FIELDS (type); method_idx++)
    {
      f = TYPE_FN_FIELDLIST1 (type, method_idx);
      printfi_filtered (spaces + 2, "[%d] name '%s' (",
			method_idx,
			TYPE_FN_FIELDLIST_NAME (type, method_idx));
      gdb_print_host_address (TYPE_FN_FIELDLIST_NAME (type, method_idx),
			      gdb_stdout);
      printf_filtered (_(") length %d\n"),
		       TYPE_FN_FIELDLIST_LENGTH (type, method_idx));
      for (overload_idx = 0;
	   overload_idx < TYPE_FN_FIELDLIST_LENGTH (type, method_idx);
	   overload_idx++)
	{
	  printfi_filtered (spaces + 4, "[%d] physname '%s' (",
			    overload_idx,
			    TYPE_FN_FIELD_PHYSNAME (f, overload_idx));
	  gdb_print_host_address (TYPE_FN_FIELD_PHYSNAME (f, overload_idx),
				  gdb_stdout);
	  printf_filtered (")\n");
	  printfi_filtered (spaces + 8, "type ");
	  gdb_print_host_address (TYPE_FN_FIELD_TYPE (f, overload_idx), 
				  gdb_stdout);
	  printf_filtered ("\n");

	  recursive_dump_type (TYPE_FN_FIELD_TYPE (f, overload_idx),
			       spaces + 8 + 2);

	  printfi_filtered (spaces + 8, "args ");
	  gdb_print_host_address (TYPE_FN_FIELD_ARGS (f, overload_idx), 
				  gdb_stdout);
	  printf_filtered ("\n");
	  print_args (TYPE_FN_FIELD_ARGS (f, overload_idx),
		      TYPE_NFIELDS (TYPE_FN_FIELD_TYPE (f, overload_idx)),
		      spaces + 8 + 2);
	  printfi_filtered (spaces + 8, "fcontext ");
	  gdb_print_host_address (TYPE_FN_FIELD_FCONTEXT (f, overload_idx),
				  gdb_stdout);
	  printf_filtered ("\n");

	  printfi_filtered (spaces + 8, "is_const %d\n",
			    TYPE_FN_FIELD_CONST (f, overload_idx));
	  printfi_filtered (spaces + 8, "is_volatile %d\n",
			    TYPE_FN_FIELD_VOLATILE (f, overload_idx));
	  printfi_filtered (spaces + 8, "is_private %d\n",
			    TYPE_FN_FIELD_PRIVATE (f, overload_idx));
	  printfi_filtered (spaces + 8, "is_protected %d\n",
			    TYPE_FN_FIELD_PROTECTED (f, overload_idx));
	  printfi_filtered (spaces + 8, "is_stub %d\n",
			    TYPE_FN_FIELD_STUB (f, overload_idx));
	  printfi_filtered (spaces + 8, "voffset %u\n",
			    TYPE_FN_FIELD_VOFFSET (f, overload_idx));
	}
    }
}

static void
print_cplus_stuff (struct type *type, int spaces)
{
  printfi_filtered (spaces, "vptr_fieldno %d\n", TYPE_VPTR_FIELDNO (type));
  printfi_filtered (spaces, "vptr_basetype ");
  gdb_print_host_address (TYPE_VPTR_BASETYPE (type), gdb_stdout);
  puts_filtered ("\n");
  if (TYPE_VPTR_BASETYPE (type) != NULL)
    recursive_dump_type (TYPE_VPTR_BASETYPE (type), spaces + 2);

  printfi_filtered (spaces, "n_baseclasses %d\n",
		    TYPE_N_BASECLASSES (type));
  printfi_filtered (spaces, "nfn_fields %d\n",
		    TYPE_NFN_FIELDS (type));
  if (TYPE_N_BASECLASSES (type) > 0)
    {
      printfi_filtered (spaces, "virtual_field_bits (%d bits at *",
			TYPE_N_BASECLASSES (type));
      gdb_print_host_address (TYPE_FIELD_VIRTUAL_BITS (type), 
			      gdb_stdout);
      printf_filtered (")");

      print_bit_vector (TYPE_FIELD_VIRTUAL_BITS (type),
			TYPE_N_BASECLASSES (type));
      puts_filtered ("\n");
    }
  if (TYPE_NFIELDS (type) > 0)
    {
      if (TYPE_FIELD_PRIVATE_BITS (type) != NULL)
	{
	  printfi_filtered (spaces, 
			    "private_field_bits (%d bits at *",
			    TYPE_NFIELDS (type));
	  gdb_print_host_address (TYPE_FIELD_PRIVATE_BITS (type), 
				  gdb_stdout);
	  printf_filtered (")");
	  print_bit_vector (TYPE_FIELD_PRIVATE_BITS (type),
			    TYPE_NFIELDS (type));
	  puts_filtered ("\n");
	}
      if (TYPE_FIELD_PROTECTED_BITS (type) != NULL)
	{
	  printfi_filtered (spaces, 
			    "protected_field_bits (%d bits at *",
			    TYPE_NFIELDS (type));
	  gdb_print_host_address (TYPE_FIELD_PROTECTED_BITS (type), 
				  gdb_stdout);
	  printf_filtered (")");
	  print_bit_vector (TYPE_FIELD_PROTECTED_BITS (type),
			    TYPE_NFIELDS (type));
	  puts_filtered ("\n");
	}
    }
  if (TYPE_NFN_FIELDS (type) > 0)
    {
      dump_fn_fieldlists (type, spaces);
    }
}

/* Print the contents of the TYPE's type_specific union, assuming that
   its type-specific kind is TYPE_SPECIFIC_GNAT_STUFF.  */

static void
print_gnat_stuff (struct type *type, int spaces)
{
  struct type *descriptive_type = TYPE_DESCRIPTIVE_TYPE (type);

  if (descriptive_type == NULL)
    printfi_filtered (spaces + 2, "no descriptive type\n");
  else
    {
      printfi_filtered (spaces + 2, "descriptive type\n");
      recursive_dump_type (descriptive_type, spaces + 4);
    }
}

static struct obstack dont_print_type_obstack;

void
recursive_dump_type (struct type *type, int spaces)
{
  int idx;

  if (spaces == 0)
    obstack_begin (&dont_print_type_obstack, 0);

  if (TYPE_NFIELDS (type) > 0
      || (HAVE_CPLUS_STRUCT (type) && TYPE_NFN_FIELDS (type) > 0))
    {
      struct type **first_dont_print
	= (struct type **) obstack_base (&dont_print_type_obstack);

      int i = (struct type **) 
	obstack_next_free (&dont_print_type_obstack) - first_dont_print;

      while (--i >= 0)
	{
	  if (type == first_dont_print[i])
	    {
	      printfi_filtered (spaces, "type node ");
	      gdb_print_host_address (type, gdb_stdout);
	      printf_filtered (_(" <same as already seen type>\n"));
	      return;
	    }
	}

      obstack_ptr_grow (&dont_print_type_obstack, type);
    }

  printfi_filtered (spaces, "type node ");
  gdb_print_host_address (type, gdb_stdout);
  printf_filtered ("\n");
  printfi_filtered (spaces, "name '%s' (",
		    TYPE_NAME (type) ? TYPE_NAME (type) : "<NULL>");
  gdb_print_host_address (TYPE_NAME (type), gdb_stdout);
  printf_filtered (")\n");
  printfi_filtered (spaces, "tagname '%s' (",
		    TYPE_TAG_NAME (type) ? TYPE_TAG_NAME (type) : "<NULL>");
  gdb_print_host_address (TYPE_TAG_NAME (type), gdb_stdout);
  printf_filtered (")\n");
  printfi_filtered (spaces, "code 0x%x ", TYPE_CODE (type));
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_UNDEF:
      printf_filtered ("(TYPE_CODE_UNDEF)");
      break;
    case TYPE_CODE_PTR:
      printf_filtered ("(TYPE_CODE_PTR)");
      break;
    case TYPE_CODE_ARRAY:
      printf_filtered ("(TYPE_CODE_ARRAY)");
      break;
    case TYPE_CODE_STRUCT:
      printf_filtered ("(TYPE_CODE_STRUCT)");
      break;
    case TYPE_CODE_UNION:
      printf_filtered ("(TYPE_CODE_UNION)");
      break;
    case TYPE_CODE_ENUM:
      printf_filtered ("(TYPE_CODE_ENUM)");
      break;
    case TYPE_CODE_FLAGS:
      printf_filtered ("(TYPE_CODE_FLAGS)");
      break;
    case TYPE_CODE_FUNC:
      printf_filtered ("(TYPE_CODE_FUNC)");
      break;
    case TYPE_CODE_INT:
      printf_filtered ("(TYPE_CODE_INT)");
      break;
    case TYPE_CODE_FLT:
      printf_filtered ("(TYPE_CODE_FLT)");
      break;
    case TYPE_CODE_VOID:
      printf_filtered ("(TYPE_CODE_VOID)");
      break;
    case TYPE_CODE_SET:
      printf_filtered ("(TYPE_CODE_SET)");
      break;
    case TYPE_CODE_RANGE:
      printf_filtered ("(TYPE_CODE_RANGE)");
      break;
    case TYPE_CODE_STRING:
      printf_filtered ("(TYPE_CODE_STRING)");
      break;
    case TYPE_CODE_ERROR:
      printf_filtered ("(TYPE_CODE_ERROR)");
      break;
    case TYPE_CODE_MEMBERPTR:
      printf_filtered ("(TYPE_CODE_MEMBERPTR)");
      break;
    case TYPE_CODE_METHODPTR:
      printf_filtered ("(TYPE_CODE_METHODPTR)");
      break;
    case TYPE_CODE_METHOD:
      printf_filtered ("(TYPE_CODE_METHOD)");
      break;
    case TYPE_CODE_REF:
      printf_filtered ("(TYPE_CODE_REF)");
      break;
    case TYPE_CODE_CHAR:
      printf_filtered ("(TYPE_CODE_CHAR)");
      break;
    case TYPE_CODE_BOOL:
      printf_filtered ("(TYPE_CODE_BOOL)");
      break;
    case TYPE_CODE_COMPLEX:
      printf_filtered ("(TYPE_CODE_COMPLEX)");
      break;
    case TYPE_CODE_TYPEDEF:
      printf_filtered ("(TYPE_CODE_TYPEDEF)");
      break;
    case TYPE_CODE_NAMESPACE:
      printf_filtered ("(TYPE_CODE_NAMESPACE)");
      break;
    default:
      printf_filtered ("(UNKNOWN TYPE CODE)");
      break;
    }
  puts_filtered ("\n");
  printfi_filtered (spaces, "length %d\n", TYPE_LENGTH (type));
  if (TYPE_OBJFILE_OWNED (type))
    {
      printfi_filtered (spaces, "objfile ");
      gdb_print_host_address (TYPE_OWNER (type).objfile, gdb_stdout);
    }
  else
    {
      printfi_filtered (spaces, "gdbarch ");
      gdb_print_host_address (TYPE_OWNER (type).gdbarch, gdb_stdout);
    }
  printf_filtered ("\n");
  printfi_filtered (spaces, "target_type ");
  gdb_print_host_address (TYPE_TARGET_TYPE (type), gdb_stdout);
  printf_filtered ("\n");
  if (TYPE_TARGET_TYPE (type) != NULL)
    {
      recursive_dump_type (TYPE_TARGET_TYPE (type), spaces + 2);
    }
  printfi_filtered (spaces, "pointer_type ");
  gdb_print_host_address (TYPE_POINTER_TYPE (type), gdb_stdout);
  printf_filtered ("\n");
  printfi_filtered (spaces, "reference_type ");
  gdb_print_host_address (TYPE_REFERENCE_TYPE (type), gdb_stdout);
  printf_filtered ("\n");
  printfi_filtered (spaces, "type_chain ");
  gdb_print_host_address (TYPE_CHAIN (type), gdb_stdout);
  printf_filtered ("\n");
  printfi_filtered (spaces, "instance_flags 0x%x", 
		    TYPE_INSTANCE_FLAGS (type));
  if (TYPE_CONST (type))
    {
      puts_filtered (" TYPE_CONST");
    }
  if (TYPE_VOLATILE (type))
    {
      puts_filtered (" TYPE_VOLATILE");
    }
  if (TYPE_CODE_SPACE (type))
    {
      puts_filtered (" TYPE_CODE_SPACE");
    }
  if (TYPE_DATA_SPACE (type))
    {
      puts_filtered (" TYPE_DATA_SPACE");
    }
  if (TYPE_ADDRESS_CLASS_1 (type))
    {
      puts_filtered (" TYPE_ADDRESS_CLASS_1");
    }
  if (TYPE_ADDRESS_CLASS_2 (type))
    {
      puts_filtered (" TYPE_ADDRESS_CLASS_2");
    }
  if (TYPE_RESTRICT (type))
    {
      puts_filtered (" TYPE_RESTRICT");
    }
  if (TYPE_ATOMIC (type))
    {
      puts_filtered (" TYPE_ATOMIC");
    }
  puts_filtered ("\n");

  printfi_filtered (spaces, "flags");
  if (TYPE_UNSIGNED (type))
    {
      puts_filtered (" TYPE_UNSIGNED");
    }
  if (TYPE_NOSIGN (type))
    {
      puts_filtered (" TYPE_NOSIGN");
    }
  if (TYPE_STUB (type))
    {
      puts_filtered (" TYPE_STUB");
    }
  if (TYPE_TARGET_STUB (type))
    {
      puts_filtered (" TYPE_TARGET_STUB");
    }
  if (TYPE_STATIC (type))
    {
      puts_filtered (" TYPE_STATIC");
    }
  if (TYPE_PROTOTYPED (type))
    {
      puts_filtered (" TYPE_PROTOTYPED");
    }
  if (TYPE_INCOMPLETE (type))
    {
      puts_filtered (" TYPE_INCOMPLETE");
    }
  if (TYPE_VARARGS (type))
    {
      puts_filtered (" TYPE_VARARGS");
    }
  /* This is used for things like AltiVec registers on ppc.  Gcc emits
     an attribute for the array type, which tells whether or not we
     have a vector, instead of a regular array.  */
  if (TYPE_VECTOR (type))
    {
      puts_filtered (" TYPE_VECTOR");
    }
  if (TYPE_FIXED_INSTANCE (type))
    {
      puts_filtered (" TYPE_FIXED_INSTANCE");
    }
  if (TYPE_STUB_SUPPORTED (type))
    {
      puts_filtered (" TYPE_STUB_SUPPORTED");
    }
  if (TYPE_NOTTEXT (type))
    {
      puts_filtered (" TYPE_NOTTEXT");
    }
  puts_filtered ("\n");
  printfi_filtered (spaces, "nfields %d ", TYPE_NFIELDS (type));
  gdb_print_host_address (TYPE_FIELDS (type), gdb_stdout);
  puts_filtered ("\n");
  for (idx = 0; idx < TYPE_NFIELDS (type); idx++)
    {
      if (TYPE_CODE (type) == TYPE_CODE_ENUM)
	printfi_filtered (spaces + 2,
			  "[%d] enumval %s type ",
			  idx, plongest (TYPE_FIELD_ENUMVAL (type, idx)));
      else
	printfi_filtered (spaces + 2,
			  "[%d] bitpos %s bitsize %d type ",
			  idx, plongest (TYPE_FIELD_BITPOS (type, idx)),
			  TYPE_FIELD_BITSIZE (type, idx));
      gdb_print_host_address (TYPE_FIELD_TYPE (type, idx), gdb_stdout);
      printf_filtered (" name '%s' (",
		       TYPE_FIELD_NAME (type, idx) != NULL
		       ? TYPE_FIELD_NAME (type, idx)
		       : "<NULL>");
      gdb_print_host_address (TYPE_FIELD_NAME (type, idx), gdb_stdout);
      printf_filtered (")\n");
      if (TYPE_FIELD_TYPE (type, idx) != NULL)
	{
	  recursive_dump_type (TYPE_FIELD_TYPE (type, idx), spaces + 4);
	}
    }
  if (TYPE_CODE (type) == TYPE_CODE_RANGE)
    {
      printfi_filtered (spaces, "low %s%s  high %s%s\n",
			plongest (TYPE_LOW_BOUND (type)), 
			TYPE_LOW_BOUND_UNDEFINED (type) ? " (undefined)" : "",
			plongest (TYPE_HIGH_BOUND (type)),
			TYPE_HIGH_BOUND_UNDEFINED (type) 
			? " (undefined)" : "");
    }

  switch (TYPE_SPECIFIC_FIELD (type))
    {
      case TYPE_SPECIFIC_CPLUS_STUFF:
	printfi_filtered (spaces, "cplus_stuff ");
	gdb_print_host_address (TYPE_CPLUS_SPECIFIC (type), 
				gdb_stdout);
	puts_filtered ("\n");
	print_cplus_stuff (type, spaces);
	break;

      case TYPE_SPECIFIC_GNAT_STUFF:
	printfi_filtered (spaces, "gnat_stuff ");
	gdb_print_host_address (TYPE_GNAT_SPECIFIC (type), gdb_stdout);
	puts_filtered ("\n");
	print_gnat_stuff (type, spaces);
	break;

      case TYPE_SPECIFIC_FLOATFORMAT:
	printfi_filtered (spaces, "floatformat ");
	if (TYPE_FLOATFORMAT (type) == NULL)
	  puts_filtered ("(null)");
	else
	  {
	    puts_filtered ("{ ");
	    if (TYPE_FLOATFORMAT (type)[0] == NULL
		|| TYPE_FLOATFORMAT (type)[0]->name == NULL)
	      puts_filtered ("(null)");
	    else
	      puts_filtered (TYPE_FLOATFORMAT (type)[0]->name);

	    puts_filtered (", ");
	    if (TYPE_FLOATFORMAT (type)[1] == NULL
		|| TYPE_FLOATFORMAT (type)[1]->name == NULL)
	      puts_filtered ("(null)");
	    else
	      puts_filtered (TYPE_FLOATFORMAT (type)[1]->name);

	    puts_filtered (" }");
	  }
	puts_filtered ("\n");
	break;

      case TYPE_SPECIFIC_FUNC:
	printfi_filtered (spaces, "calling_convention %d\n",
                          TYPE_CALLING_CONVENTION (type));
	/* tail_call_list is not printed.  */
	break;

      case TYPE_SPECIFIC_SELF_TYPE:
	printfi_filtered (spaces, "self_type ");
	gdb_print_host_address (TYPE_SELF_TYPE (type), gdb_stdout);
	puts_filtered ("\n");
	break;
    }

  if (spaces == 0)
    obstack_free (&dont_print_type_obstack, NULL);
}

/* Trivial helpers for the libiberty hash table, for mapping one
   type to another.  */

struct type_pair
{
  struct type *old, *newobj;
};

static hashval_t
type_pair_hash (const void *item)
{
  const struct type_pair *pair = (const struct type_pair *) item;

  return htab_hash_pointer (pair->old);
}

static int
type_pair_eq (const void *item_lhs, const void *item_rhs)
{
  const struct type_pair *lhs = (const struct type_pair *) item_lhs;
  const struct type_pair *rhs = (const struct type_pair *) item_rhs;

  return lhs->old == rhs->old;
}

/* Allocate the hash table used by copy_type_recursive to walk
   types without duplicates.  We use OBJFILE's obstack, because
   OBJFILE is about to be deleted.  */

htab_t
create_copied_types_hash (struct objfile *objfile)
{
  return htab_create_alloc_ex (1, type_pair_hash, type_pair_eq,
			       NULL, &objfile->objfile_obstack,
			       hashtab_obstack_allocate,
			       dummy_obstack_deallocate);
}

/* Recursively copy (deep copy) a dynamic attribute list of a type.  */

static struct dynamic_prop_list *
copy_dynamic_prop_list (struct obstack *objfile_obstack,
			struct dynamic_prop_list *list)
{
  struct dynamic_prop_list *copy = list;
  struct dynamic_prop_list **node_ptr = &copy;

  while (*node_ptr != NULL)
    {
      struct dynamic_prop_list *node_copy;

      node_copy = ((struct dynamic_prop_list *)
		   obstack_copy (objfile_obstack, *node_ptr,
				 sizeof (struct dynamic_prop_list)));
      node_copy->prop = (*node_ptr)->prop;
      *node_ptr = node_copy;

      node_ptr = &node_copy->next;
    }

  return copy;
}

/* Recursively copy (deep copy) TYPE, if it is associated with
   OBJFILE.  Return a new type owned by the gdbarch associated with the type, a
   saved type if we have already visited TYPE (using COPIED_TYPES), or TYPE if
   it is not associated with OBJFILE.  */

struct type *
copy_type_recursive (struct objfile *objfile, 
		     struct type *type,
		     htab_t copied_types)
{
  struct type_pair *stored, pair;
  void **slot;
  struct type *new_type;

  if (! TYPE_OBJFILE_OWNED (type))
    return type;

  /* This type shouldn't be pointing to any types in other objfiles;
     if it did, the type might disappear unexpectedly.  */
  gdb_assert (TYPE_OBJFILE (type) == objfile);

  pair.old = type;
  slot = htab_find_slot (copied_types, &pair, INSERT);
  if (*slot != NULL)
    return ((struct type_pair *) *slot)->newobj;

  new_type = alloc_type_arch (get_type_arch (type));

  /* We must add the new type to the hash table immediately, in case
     we encounter this type again during a recursive call below.  */
  stored = XOBNEW (&objfile->objfile_obstack, struct type_pair);
  stored->old = type;
  stored->newobj = new_type;
  *slot = stored;

  /* Copy the common fields of types.  For the main type, we simply
     copy the entire thing and then update specific fields as needed.  */
  *TYPE_MAIN_TYPE (new_type) = *TYPE_MAIN_TYPE (type);
  TYPE_OBJFILE_OWNED (new_type) = 0;
  TYPE_OWNER (new_type).gdbarch = get_type_arch (type);

  if (TYPE_NAME (type))
    TYPE_NAME (new_type) = xstrdup (TYPE_NAME (type));
  if (TYPE_TAG_NAME (type))
    TYPE_TAG_NAME (new_type) = xstrdup (TYPE_TAG_NAME (type));

  TYPE_INSTANCE_FLAGS (new_type) = TYPE_INSTANCE_FLAGS (type);
  TYPE_LENGTH (new_type) = TYPE_LENGTH (type);

  /* Copy the fields.  */
  if (TYPE_NFIELDS (type))
    {
      int i, nfields;

      nfields = TYPE_NFIELDS (type);
      TYPE_FIELDS (new_type) = XCNEWVEC (struct field, nfields);
      for (i = 0; i < nfields; i++)
	{
	  TYPE_FIELD_ARTIFICIAL (new_type, i) = 
	    TYPE_FIELD_ARTIFICIAL (type, i);
	  TYPE_FIELD_BITSIZE (new_type, i) = TYPE_FIELD_BITSIZE (type, i);
	  if (TYPE_FIELD_TYPE (type, i))
	    TYPE_FIELD_TYPE (new_type, i)
	      = copy_type_recursive (objfile, TYPE_FIELD_TYPE (type, i),
				     copied_types);
	  if (TYPE_FIELD_NAME (type, i))
	    TYPE_FIELD_NAME (new_type, i) = 
	      xstrdup (TYPE_FIELD_NAME (type, i));
	  switch (TYPE_FIELD_LOC_KIND (type, i))
	    {
	    case FIELD_LOC_KIND_BITPOS:
	      SET_FIELD_BITPOS (TYPE_FIELD (new_type, i),
				TYPE_FIELD_BITPOS (type, i));
	      break;
	    case FIELD_LOC_KIND_ENUMVAL:
	      SET_FIELD_ENUMVAL (TYPE_FIELD (new_type, i),
				 TYPE_FIELD_ENUMVAL (type, i));
	      break;
	    case FIELD_LOC_KIND_PHYSADDR:
	      SET_FIELD_PHYSADDR (TYPE_FIELD (new_type, i),
				  TYPE_FIELD_STATIC_PHYSADDR (type, i));
	      break;
	    case FIELD_LOC_KIND_PHYSNAME:
	      SET_FIELD_PHYSNAME (TYPE_FIELD (new_type, i),
				  xstrdup (TYPE_FIELD_STATIC_PHYSNAME (type,
								       i)));
	      break;
	    default:
	      internal_error (__FILE__, __LINE__,
			      _("Unexpected type field location kind: %d"),
			      TYPE_FIELD_LOC_KIND (type, i));
	    }
	}
    }

  /* For range types, copy the bounds information.  */
  if (TYPE_CODE (type) == TYPE_CODE_RANGE)
    {
      TYPE_RANGE_DATA (new_type) = XNEW (struct range_bounds);
      *TYPE_RANGE_DATA (new_type) = *TYPE_RANGE_DATA (type);
    }

  if (TYPE_DYN_PROP_LIST (type) != NULL)
    TYPE_DYN_PROP_LIST (new_type)
      = copy_dynamic_prop_list (&objfile->objfile_obstack,
				TYPE_DYN_PROP_LIST (type));


  /* Copy pointers to other types.  */
  if (TYPE_TARGET_TYPE (type))
    TYPE_TARGET_TYPE (new_type) = 
      copy_type_recursive (objfile, 
			   TYPE_TARGET_TYPE (type),
			   copied_types);

  /* Maybe copy the type_specific bits.

     NOTE drow/2005-12-09: We do not copy the C++-specific bits like
     base classes and methods.  There's no fundamental reason why we
     can't, but at the moment it is not needed.  */

  switch (TYPE_SPECIFIC_FIELD (type))
    {
    case TYPE_SPECIFIC_NONE:
      break;
    case TYPE_SPECIFIC_FUNC:
      INIT_FUNC_SPECIFIC (new_type);
      TYPE_CALLING_CONVENTION (new_type) = TYPE_CALLING_CONVENTION (type);
      TYPE_NO_RETURN (new_type) = TYPE_NO_RETURN (type);
      TYPE_TAIL_CALL_LIST (new_type) = NULL;
      break;
    case TYPE_SPECIFIC_FLOATFORMAT:
      TYPE_FLOATFORMAT (new_type) = TYPE_FLOATFORMAT (type);
      break;
    case TYPE_SPECIFIC_CPLUS_STUFF:
      INIT_CPLUS_SPECIFIC (new_type);
      break;
    case TYPE_SPECIFIC_GNAT_STUFF:
      INIT_GNAT_SPECIFIC (new_type);
      break;
    case TYPE_SPECIFIC_SELF_TYPE:
      set_type_self_type (new_type,
			  copy_type_recursive (objfile, TYPE_SELF_TYPE (type),
					       copied_types));
      break;
    default:
      gdb_assert_not_reached ("bad type_specific_kind");
    }

  return new_type;
}

/* Make a copy of the given TYPE, except that the pointer & reference
   types are not preserved.
   
   This function assumes that the given type has an associated objfile.
   This objfile is used to allocate the new type.  */

struct type *
copy_type (const struct type *type)
{
  struct type *new_type;

  gdb_assert (TYPE_OBJFILE_OWNED (type));

  new_type = alloc_type_copy (type);
  TYPE_INSTANCE_FLAGS (new_type) = TYPE_INSTANCE_FLAGS (type);
  TYPE_LENGTH (new_type) = TYPE_LENGTH (type);
  memcpy (TYPE_MAIN_TYPE (new_type), TYPE_MAIN_TYPE (type),
	  sizeof (struct main_type));
  if (TYPE_DYN_PROP_LIST (type) != NULL)
    TYPE_DYN_PROP_LIST (new_type)
      = copy_dynamic_prop_list (&TYPE_OBJFILE (type) -> objfile_obstack,
				TYPE_DYN_PROP_LIST (type));

  return new_type;
}

/* Helper functions to initialize architecture-specific types.  */

/* Allocate a type structure associated with GDBARCH and set its
   CODE, LENGTH, and NAME fields.  */

struct type *
arch_type (struct gdbarch *gdbarch,
	   enum type_code code, int length, const char *name)
{
  struct type *type;

  type = alloc_type_arch (gdbarch);
  set_type_code (type, code);
  TYPE_LENGTH (type) = length;

  if (name)
    TYPE_NAME (type) = gdbarch_obstack_strdup (gdbarch, name);

  return type;
}

/* Allocate a TYPE_CODE_INT type structure associated with GDBARCH.
   BIT is the type size in bits.  If UNSIGNED_P is non-zero, set
   the type's TYPE_UNSIGNED flag.  NAME is the type name.  */

struct type *
arch_integer_type (struct gdbarch *gdbarch,
		   int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = arch_type (gdbarch, TYPE_CODE_INT, bit / TARGET_CHAR_BIT, name);
  if (unsigned_p)
    TYPE_UNSIGNED (t) = 1;

  return t;
}

/* Allocate a TYPE_CODE_CHAR type structure associated with GDBARCH.
   BIT is the type size in bits.  If UNSIGNED_P is non-zero, set
   the type's TYPE_UNSIGNED flag.  NAME is the type name.  */

struct type *
arch_character_type (struct gdbarch *gdbarch,
		     int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = arch_type (gdbarch, TYPE_CODE_CHAR, bit / TARGET_CHAR_BIT, name);
  if (unsigned_p)
    TYPE_UNSIGNED (t) = 1;

  return t;
}

/* Allocate a TYPE_CODE_BOOL type structure associated with GDBARCH.
   BIT is the type size in bits.  If UNSIGNED_P is non-zero, set
   the type's TYPE_UNSIGNED flag.  NAME is the type name.  */

struct type *
arch_boolean_type (struct gdbarch *gdbarch,
		   int bit, int unsigned_p, const char *name)
{
  struct type *t;

  t = arch_type (gdbarch, TYPE_CODE_BOOL, bit / TARGET_CHAR_BIT, name);
  if (unsigned_p)
    TYPE_UNSIGNED (t) = 1;

  return t;
}

/* Allocate a TYPE_CODE_FLT type structure associated with GDBARCH.
   BIT is the type size in bits; if BIT equals -1, the size is
   determined by the floatformat.  NAME is the type name.  Set the
   TYPE_FLOATFORMAT from FLOATFORMATS.  */

struct type *
arch_float_type (struct gdbarch *gdbarch,
		 int bit, const char *name,
		 const struct floatformat **floatformats)
{
  struct type *t;

  bit = verify_floatformat (bit, floatformats);
  t = arch_type (gdbarch, TYPE_CODE_FLT, bit / TARGET_CHAR_BIT, name);
  TYPE_FLOATFORMAT (t) = floatformats;

  return t;
}

/* Allocate a TYPE_CODE_DECFLOAT type structure associated with GDBARCH.
   BIT is the type size in bits.  NAME is the type name.  */

struct type *
arch_decfloat_type (struct gdbarch *gdbarch, int bit, const char *name)
{
  struct type *t;

  t = arch_type (gdbarch, TYPE_CODE_DECFLOAT, bit / TARGET_CHAR_BIT, name);
  return t;
}

/* Allocate a TYPE_CODE_COMPLEX type structure associated with GDBARCH.
   NAME is the type name.  TARGET_TYPE is the component float type.  */

struct type *
arch_complex_type (struct gdbarch *gdbarch,
		   const char *name, struct type *target_type)
{
  struct type *t;

  t = arch_type (gdbarch, TYPE_CODE_COMPLEX,
		 2 * TYPE_LENGTH (target_type), name);
  TYPE_TARGET_TYPE (t) = target_type;
  return t;
}

/* Allocate a TYPE_CODE_PTR type structure associated with GDBARCH.
   BIT is the pointer type size in bits.  NAME is the type name.
   TARGET_TYPE is the pointer target type.  Always sets the pointer type's
   TYPE_UNSIGNED flag.  */

struct type *
arch_pointer_type (struct gdbarch *gdbarch,
		   int bit, const char *name, struct type *target_type)
{
  struct type *t;

  t = arch_type (gdbarch, TYPE_CODE_PTR, bit / TARGET_CHAR_BIT, name);
  TYPE_TARGET_TYPE (t) = target_type;
  TYPE_UNSIGNED (t) = 1;
  return t;
}

/* Allocate a TYPE_CODE_FLAGS type structure associated with GDBARCH.
   NAME is the type name.  LENGTH is the size of the flag word in bytes.  */

struct type *
arch_flags_type (struct gdbarch *gdbarch, const char *name, int length)
{
  int max_nfields = length * TARGET_CHAR_BIT;
  struct type *type;

  type = arch_type (gdbarch, TYPE_CODE_FLAGS, length, name);
  TYPE_UNSIGNED (type) = 1;
  TYPE_NFIELDS (type) = 0;
  /* Pre-allocate enough space assuming every field is one bit.  */
  TYPE_FIELDS (type)
    = (struct field *) TYPE_ZALLOC (type, max_nfields * sizeof (struct field));

  return type;
}

/* Add field to TYPE_CODE_FLAGS type TYPE to indicate the bit at
   position BITPOS is called NAME.  Pass NAME as "" for fields that
   should not be printed.  */

void
append_flags_type_field (struct type *type, int start_bitpos, int nr_bits,
			 struct type *field_type, const char *name)
{
  int type_bitsize = TYPE_LENGTH (type) * TARGET_CHAR_BIT;
  int field_nr = TYPE_NFIELDS (type);

  gdb_assert (TYPE_CODE (type) == TYPE_CODE_FLAGS);
  gdb_assert (TYPE_NFIELDS (type) + 1 <= type_bitsize);
  gdb_assert (start_bitpos >= 0 && start_bitpos < type_bitsize);
  gdb_assert (nr_bits >= 1 && nr_bits <= type_bitsize);
  gdb_assert (name != NULL);

  TYPE_FIELD_NAME (type, field_nr) = xstrdup (name);
  TYPE_FIELD_TYPE (type, field_nr) = field_type;
  SET_FIELD_BITPOS (TYPE_FIELD (type, field_nr), start_bitpos);
  TYPE_FIELD_BITSIZE (type, field_nr) = nr_bits;
  ++TYPE_NFIELDS (type);
}

/* Special version of append_flags_type_field to add a flag field.
   Add field to TYPE_CODE_FLAGS type TYPE to indicate the bit at
   position BITPOS is called NAME.  */

void
append_flags_type_flag (struct type *type, int bitpos, const char *name)
{
  struct gdbarch *gdbarch = get_type_arch (type);

  append_flags_type_field (type, bitpos, 1,
			   builtin_type (gdbarch)->builtin_bool,
			   name);
}

/* Allocate a TYPE_CODE_STRUCT or TYPE_CODE_UNION type structure (as
   specified by CODE) associated with GDBARCH.  NAME is the type name.  */

struct type *
arch_composite_type (struct gdbarch *gdbarch, const char *name,
		     enum type_code code)
{
  struct type *t;

  gdb_assert (code == TYPE_CODE_STRUCT || code == TYPE_CODE_UNION);
  t = arch_type (gdbarch, code, 0, NULL);
  TYPE_TAG_NAME (t) = name;
  INIT_CPLUS_SPECIFIC (t);
  return t;
}

/* Add new field with name NAME and type FIELD to composite type T.
   Do not set the field's position or adjust the type's length;
   the caller should do so.  Return the new field.  */

struct field *
append_composite_type_field_raw (struct type *t, const char *name,
				 struct type *field)
{
  struct field *f;

  TYPE_NFIELDS (t) = TYPE_NFIELDS (t) + 1;
  TYPE_FIELDS (t) = XRESIZEVEC (struct field, TYPE_FIELDS (t),
				TYPE_NFIELDS (t));
  f = &(TYPE_FIELDS (t)[TYPE_NFIELDS (t) - 1]);
  memset (f, 0, sizeof f[0]);
  FIELD_TYPE (f[0]) = field;
  FIELD_NAME (f[0]) = name;
  return f;
}

/* Add new field with name NAME and type FIELD to composite type T.
   ALIGNMENT (if non-zero) specifies the minimum field alignment.  */

void
append_composite_type_field_aligned (struct type *t, const char *name,
				     struct type *field, int alignment)
{
  struct field *f = append_composite_type_field_raw (t, name, field);

  if (TYPE_CODE (t) == TYPE_CODE_UNION)
    {
      if (TYPE_LENGTH (t) < TYPE_LENGTH (field))
	TYPE_LENGTH (t) = TYPE_LENGTH (field);
    }
  else if (TYPE_CODE (t) == TYPE_CODE_STRUCT)
    {
      TYPE_LENGTH (t) = TYPE_LENGTH (t) + TYPE_LENGTH (field);
      if (TYPE_NFIELDS (t) > 1)
	{
	  SET_FIELD_BITPOS (f[0],
			    (FIELD_BITPOS (f[-1])
			     + (TYPE_LENGTH (FIELD_TYPE (f[-1]))
				* TARGET_CHAR_BIT)));

	  if (alignment)
	    {
	      int left;

	      alignment *= TARGET_CHAR_BIT;
	      left = FIELD_BITPOS (f[0]) % alignment;

	      if (left)
		{
		  SET_FIELD_BITPOS (f[0], FIELD_BITPOS (f[0]) + (alignment - left));
		  TYPE_LENGTH (t) += (alignment - left) / TARGET_CHAR_BIT;
		}
	    }
	}
    }
}

/* Add new field with name NAME and type FIELD to composite type T.  */

void
append_composite_type_field (struct type *t, const char *name,
			     struct type *field)
{
  append_composite_type_field_aligned (t, name, field, 0);
}

static struct gdbarch_data *gdbtypes_data;

const struct builtin_type *
builtin_type (struct gdbarch *gdbarch)
{
  return (const struct builtin_type *) gdbarch_data (gdbarch, gdbtypes_data);
}

static void *
gdbtypes_post_init (struct gdbarch *gdbarch)
{
  struct builtin_type *builtin_type
    = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct builtin_type);

  /* Basic types.  */
  builtin_type->builtin_void
    = arch_type (gdbarch, TYPE_CODE_VOID, 1, "void");
  builtin_type->builtin_char
    = arch_integer_type (gdbarch, TARGET_CHAR_BIT,
			 !gdbarch_char_signed (gdbarch), "char");
  TYPE_NOSIGN (builtin_type->builtin_char) = 1;
  builtin_type->builtin_signed_char
    = arch_integer_type (gdbarch, TARGET_CHAR_BIT,
			 0, "signed char");
  builtin_type->builtin_unsigned_char
    = arch_integer_type (gdbarch, TARGET_CHAR_BIT,
			 1, "unsigned char");
  builtin_type->builtin_short
    = arch_integer_type (gdbarch, gdbarch_short_bit (gdbarch),
			 0, "short");
  builtin_type->builtin_unsigned_short
    = arch_integer_type (gdbarch, gdbarch_short_bit (gdbarch),
			 1, "unsigned short");
  builtin_type->builtin_int
    = arch_integer_type (gdbarch, gdbarch_int_bit (gdbarch),
			 0, "int");
  builtin_type->builtin_unsigned_int
    = arch_integer_type (gdbarch, gdbarch_int_bit (gdbarch),
			 1, "unsigned int");
  builtin_type->builtin_long
    = arch_integer_type (gdbarch, gdbarch_long_bit (gdbarch),
			 0, "long");
  builtin_type->builtin_unsigned_long
    = arch_integer_type (gdbarch, gdbarch_long_bit (gdbarch),
			 1, "unsigned long");
  builtin_type->builtin_long_long
    = arch_integer_type (gdbarch, gdbarch_long_long_bit (gdbarch),
			 0, "long long");
  builtin_type->builtin_unsigned_long_long
    = arch_integer_type (gdbarch, gdbarch_long_long_bit (gdbarch),
			 1, "unsigned long long");
  builtin_type->builtin_float
    = arch_float_type (gdbarch, gdbarch_float_bit (gdbarch),
		       "float", gdbarch_float_format (gdbarch));
  builtin_type->builtin_double
    = arch_float_type (gdbarch, gdbarch_double_bit (gdbarch),
		       "double", gdbarch_double_format (gdbarch));
  builtin_type->builtin_long_double
    = arch_float_type (gdbarch, gdbarch_long_double_bit (gdbarch),
		       "long double", gdbarch_long_double_format (gdbarch));
  builtin_type->builtin_complex
    = arch_complex_type (gdbarch, "complex",
			 builtin_type->builtin_float);
  builtin_type->builtin_double_complex
    = arch_complex_type (gdbarch, "double complex",
			 builtin_type->builtin_double);
  builtin_type->builtin_string
    = arch_type (gdbarch, TYPE_CODE_STRING, 1, "string");
  builtin_type->builtin_bool
    = arch_type (gdbarch, TYPE_CODE_BOOL, 1, "bool");

  /* The following three are about decimal floating point types, which
     are 32-bits, 64-bits and 128-bits respectively.  */
  builtin_type->builtin_decfloat
    = arch_decfloat_type (gdbarch, 32, "_Decimal32");
  builtin_type->builtin_decdouble
    = arch_decfloat_type (gdbarch, 64, "_Decimal64");
  builtin_type->builtin_declong
    = arch_decfloat_type (gdbarch, 128, "_Decimal128");

  /* "True" character types.  */
  builtin_type->builtin_true_char
    = arch_character_type (gdbarch, TARGET_CHAR_BIT, 0, "true character");
  builtin_type->builtin_true_unsigned_char
    = arch_character_type (gdbarch, TARGET_CHAR_BIT, 1, "true character");

  /* Fixed-size integer types.  */
  builtin_type->builtin_int0
    = arch_integer_type (gdbarch, 0, 0, "int0_t");
  builtin_type->builtin_int8
    = arch_integer_type (gdbarch, 8, 0, "int8_t");
  builtin_type->builtin_uint8
    = arch_integer_type (gdbarch, 8, 1, "uint8_t");
  builtin_type->builtin_int16
    = arch_integer_type (gdbarch, 16, 0, "int16_t");
  builtin_type->builtin_uint16
    = arch_integer_type (gdbarch, 16, 1, "uint16_t");
  builtin_type->builtin_int32
    = arch_integer_type (gdbarch, 32, 0, "int32_t");
  builtin_type->builtin_uint32
    = arch_integer_type (gdbarch, 32, 1, "uint32_t");
  builtin_type->builtin_int64
    = arch_integer_type (gdbarch, 64, 0, "int64_t");
  builtin_type->builtin_uint64
    = arch_integer_type (gdbarch, 64, 1, "uint64_t");
  builtin_type->builtin_int128
    = arch_integer_type (gdbarch, 128, 0, "int128_t");
  builtin_type->builtin_uint128
    = arch_integer_type (gdbarch, 128, 1, "uint128_t");
  TYPE_INSTANCE_FLAGS (builtin_type->builtin_int8) |=
    TYPE_INSTANCE_FLAG_NOTTEXT;
  TYPE_INSTANCE_FLAGS (builtin_type->builtin_uint8) |=
    TYPE_INSTANCE_FLAG_NOTTEXT;

  /* Wide character types.  */
  builtin_type->builtin_char16
    = arch_integer_type (gdbarch, 16, 1, "char16_t");
  builtin_type->builtin_char32
    = arch_integer_type (gdbarch, 32, 1, "char32_t");
  builtin_type->builtin_wchar
    = arch_integer_type (gdbarch, gdbarch_wchar_bit (gdbarch),
			 !gdbarch_wchar_signed (gdbarch), "wchar_t");

  /* Default data/code pointer types.  */
  builtin_type->builtin_data_ptr
    = lookup_pointer_type (builtin_type->builtin_void);
  builtin_type->builtin_func_ptr
    = lookup_pointer_type (lookup_function_type (builtin_type->builtin_void));
  builtin_type->builtin_func_func
    = lookup_function_type (builtin_type->builtin_func_ptr);

  /* This type represents a GDB internal function.  */
  builtin_type->internal_fn
    = arch_type (gdbarch, TYPE_CODE_INTERNAL_FUNCTION, 0,
		 "<internal function>");

  /* This type represents an xmethod.  */
  builtin_type->xmethod
    = arch_type (gdbarch, TYPE_CODE_XMETHOD, 0, "<xmethod>");

  return builtin_type;
}

/* This set of objfile-based types is intended to be used by symbol
   readers as basic types.  */

static const struct objfile_data *objfile_type_data;

const struct objfile_type *
objfile_type (struct objfile *objfile)
{
  struct gdbarch *gdbarch;
  struct objfile_type *objfile_type
    = (struct objfile_type *) objfile_data (objfile, objfile_type_data);

  if (objfile_type)
    return objfile_type;

  objfile_type = OBSTACK_CALLOC (&objfile->objfile_obstack,
				 1, struct objfile_type);

  /* Use the objfile architecture to determine basic type properties.  */
  gdbarch = get_objfile_arch (objfile);

  /* Basic types.  */
  objfile_type->builtin_void
    = init_type (objfile, TYPE_CODE_VOID, 1, "void");
  objfile_type->builtin_char
    = init_integer_type (objfile, TARGET_CHAR_BIT,
			 !gdbarch_char_signed (gdbarch), "char");
  TYPE_NOSIGN (objfile_type->builtin_char) = 1;
  objfile_type->builtin_signed_char
    = init_integer_type (objfile, TARGET_CHAR_BIT,
			 0, "signed char");
  objfile_type->builtin_unsigned_char
    = init_integer_type (objfile, TARGET_CHAR_BIT,
			 1, "unsigned char");
  objfile_type->builtin_short
    = init_integer_type (objfile, gdbarch_short_bit (gdbarch),
			 0, "short");
  objfile_type->builtin_unsigned_short
    = init_integer_type (objfile, gdbarch_short_bit (gdbarch),
			 1, "unsigned short");
  objfile_type->builtin_int
    = init_integer_type (objfile, gdbarch_int_bit (gdbarch),
			 0, "int");
  objfile_type->builtin_unsigned_int
    = init_integer_type (objfile, gdbarch_int_bit (gdbarch),
			 1, "unsigned int");
  objfile_type->builtin_long
    = init_integer_type (objfile, gdbarch_long_bit (gdbarch),
			 0, "long");
  objfile_type->builtin_unsigned_long
    = init_integer_type (objfile, gdbarch_long_bit (gdbarch),
			 1, "unsigned long");
  objfile_type->builtin_long_long
    = init_integer_type (objfile, gdbarch_long_long_bit (gdbarch),
			 0, "long long");
  objfile_type->builtin_unsigned_long_long
    = init_integer_type (objfile, gdbarch_long_long_bit (gdbarch),
			 1, "unsigned long long");
  objfile_type->builtin_float
    = init_float_type (objfile, gdbarch_float_bit (gdbarch),
		       "float", gdbarch_float_format (gdbarch));
  objfile_type->builtin_double
    = init_float_type (objfile, gdbarch_double_bit (gdbarch),
		       "double", gdbarch_double_format (gdbarch));
  objfile_type->builtin_long_double
    = init_float_type (objfile, gdbarch_long_double_bit (gdbarch),
		       "long double", gdbarch_long_double_format (gdbarch));

  /* This type represents a type that was unrecognized in symbol read-in.  */
  objfile_type->builtin_error
    = init_type (objfile, TYPE_CODE_ERROR, 0, "<unknown type>");

  /* The following set of types is used for symbols with no
     debug information.  */
  objfile_type->nodebug_text_symbol
    = init_type (objfile, TYPE_CODE_FUNC, 1,
		 "<text variable, no debug info>");
  TYPE_TARGET_TYPE (objfile_type->nodebug_text_symbol)
    = objfile_type->builtin_int;
  objfile_type->nodebug_text_gnu_ifunc_symbol
    = init_type (objfile, TYPE_CODE_FUNC, 1,
		 "<text gnu-indirect-function variable, no debug info>");
  TYPE_TARGET_TYPE (objfile_type->nodebug_text_gnu_ifunc_symbol)
    = objfile_type->nodebug_text_symbol;
  TYPE_GNU_IFUNC (objfile_type->nodebug_text_gnu_ifunc_symbol) = 1;
  objfile_type->nodebug_got_plt_symbol
    = init_pointer_type (objfile, gdbarch_addr_bit (gdbarch),
			 "<text from jump slot in .got.plt, no debug info>",
			 objfile_type->nodebug_text_symbol);
  objfile_type->nodebug_data_symbol
    = init_integer_type (objfile, gdbarch_int_bit (gdbarch), 0,
			 "<data variable, no debug info>");
  objfile_type->nodebug_unknown_symbol
    = init_integer_type (objfile, TARGET_CHAR_BIT, 0,
			 "<variable (not text or data), no debug info>");
  objfile_type->nodebug_tls_symbol
    = init_integer_type (objfile, gdbarch_int_bit (gdbarch), 0,
			 "<thread local variable, no debug info>");

  /* NOTE: on some targets, addresses and pointers are not necessarily
     the same.

     The upshot is:
     - gdb's `struct type' always describes the target's
       representation.
     - gdb's `struct value' objects should always hold values in
       target form.
     - gdb's CORE_ADDR values are addresses in the unified virtual
       address space that the assembler and linker work with.  Thus,
       since target_read_memory takes a CORE_ADDR as an argument, it
       can access any memory on the target, even if the processor has
       separate code and data address spaces.

     In this context, objfile_type->builtin_core_addr is a bit odd:
     it's a target type for a value the target will never see.  It's
     only used to hold the values of (typeless) linker symbols, which
     are indeed in the unified virtual address space.  */

  objfile_type->builtin_core_addr
    = init_integer_type (objfile, gdbarch_addr_bit (gdbarch), 1,
			 "__CORE_ADDR");

  set_objfile_data (objfile, objfile_type_data, objfile_type);
  return objfile_type;
}

extern initialize_file_ftype _initialize_gdbtypes;

void
_initialize_gdbtypes (void)
{
  gdbtypes_data = gdbarch_data_register_post_init (gdbtypes_post_init);
  objfile_type_data = register_objfile_data ();

  add_setshow_zuinteger_cmd ("overload", no_class, &overload_debug,
			     _("Set debugging of C++ overloading."),
			     _("Show debugging of C++ overloading."),
			     _("When enabled, ranking of the "
			       "functions is displayed."),
			     NULL,
			     show_overload_debug,
			     &setdebuglist, &showdebuglist);

  /* Add user knob for controlling resolution of opaque types.  */
  add_setshow_boolean_cmd ("opaque-type-resolution", class_support,
			   &opaque_type_resolution,
			   _("Set resolution of opaque struct/class/union"
			     " types (if set before loading symbols)."),
			   _("Show resolution of opaque struct/class/union"
			     " types (if set before loading symbols)."),
			   NULL, NULL,
			   show_opaque_type_resolution,
			   &setlist, &showlist);

  /* Add an option to permit non-strict type checking.  */
  add_setshow_boolean_cmd ("type", class_support,
			   &strict_type_checking,
			   _("Set strict type checking."),
			   _("Show strict type checking."),
			   NULL, NULL,
			   show_strict_type_checking,
			   &setchecklist, &showchecklist);
}
