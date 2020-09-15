/* Support for printing C++ values for GDB, the GNU debugger.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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
#include "gdb_obstack.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "command.h"
#include "gdbcmd.h"
#include "demangle.h"
#include "annotate.h"
#include "c-lang.h"
#include "target.h"
#include "cp-abi.h"
#include "valprint.h"
#include "cp-support.h"
#include "language.h"
#include "extension.h"
#include "typeprint.h"
#include "gdbsupport/byte-vector.h"
#include "gdbarch.h"
#include "cli/cli-style.h"

static struct obstack dont_print_vb_obstack;
static struct obstack dont_print_statmem_obstack;
static struct obstack dont_print_stat_array_obstack;

static void cp_print_static_field (struct type *, struct value *,
				   struct ui_file *, int,
				   const struct value_print_options *);

static void cp_print_value (struct value *, struct ui_file *,
			    int, const struct value_print_options *,
			    struct type **);


/* GCC versions after 2.4.5 use this.  */
const char vtbl_ptr_name[] = "__vtbl_ptr_type";

/* Return truth value for assertion that TYPE is of the type
   "pointer to virtual function".  */

int
cp_is_vtbl_ptr_type (struct type *type)
{
  const char *type_name = type->name ();

  return (type_name != NULL && !strcmp (type_name, vtbl_ptr_name));
}

/* Return truth value for the assertion that TYPE is of the type
   "pointer to virtual function table".  */

int
cp_is_vtbl_member (struct type *type)
{
  /* With older versions of g++, the vtbl field pointed to an array of
     structures.  Nowadays it points directly to the structure.  */
  if (type->code () == TYPE_CODE_PTR)
    {
      type = TYPE_TARGET_TYPE (type);
      if (type->code () == TYPE_CODE_ARRAY)
	{
	  type = TYPE_TARGET_TYPE (type);
	  if (type->code () == TYPE_CODE_STRUCT    /* if not using thunks */
	      || type->code () == TYPE_CODE_PTR)   /* if using thunks */
	    {
	      /* Virtual functions tables are full of pointers
	         to virtual functions.  */
	      return cp_is_vtbl_ptr_type (type);
	    }
	}
      else if (type->code () == TYPE_CODE_STRUCT)  /* if not using thunks */
	{
	  return cp_is_vtbl_ptr_type (type);
	}
      else if (type->code () == TYPE_CODE_PTR)     /* if using thunks */
	{
	  /* The type name of the thunk pointer is NULL when using
	     dwarf2.  We could test for a pointer to a function, but
	     there is no type info for the virtual table either, so it
	     wont help.  */
	  return cp_is_vtbl_ptr_type (type);
	}
    }
  return 0;
}

/* Mutually recursive subroutines of cp_print_value and c_val_print to
   print out a structure's fields: cp_print_value_fields and
   cp_print_value.

   TYPE, VALADDR, ADDRESS, STREAM, RECURSE, and OPTIONS have the same
   meanings as in cp_print_value and c_val_print.

   2nd argument REAL_TYPE is used to carry over the type of the
   derived class across the recursion to base classes.

   DONT_PRINT is an array of baseclass types that we should not print,
   or zero if called from top level.  */

void
cp_print_value_fields (struct value *val, struct ui_file *stream,
		       int recurse, const struct value_print_options *options,
		       struct type **dont_print_vb,
		       int dont_print_statmem)
{
  int i, len, n_baseclasses;
  int fields_seen = 0;
  static int last_set_recurse = -1;

  struct type *type = check_typedef (value_type (val));

  if (recurse == 0)
    {
      /* Any object can be left on obstacks only during an unexpected
	 error.  */

      if (obstack_object_size (&dont_print_statmem_obstack) > 0)
	{
	  obstack_free (&dont_print_statmem_obstack, NULL);
	  obstack_begin (&dont_print_statmem_obstack,
			 32 * sizeof (CORE_ADDR));
	}
      if (obstack_object_size (&dont_print_stat_array_obstack) > 0)
	{
	  obstack_free (&dont_print_stat_array_obstack, NULL);
	  obstack_begin (&dont_print_stat_array_obstack,
			 32 * sizeof (struct type *));
	}
    }

  fprintf_filtered (stream, "{");
  len = type->num_fields ();
  n_baseclasses = TYPE_N_BASECLASSES (type);

  /* First, print out baseclasses such that we don't print
     duplicates of virtual baseclasses.  */

  if (n_baseclasses > 0)
    cp_print_value (val, stream, recurse + 1, options, dont_print_vb);

  /* Second, print out data fields */

  /* If there are no data fields, skip this part */
  if (len == n_baseclasses || !len)
    fprintf_styled (stream, metadata_style.style (), "<No data fields>");
  else
    {
      size_t statmem_obstack_initial_size = 0;
      size_t stat_array_obstack_initial_size = 0;
      struct type *vptr_basetype = NULL;
      int vptr_fieldno;

      if (dont_print_statmem == 0)
	{
	  statmem_obstack_initial_size =
	    obstack_object_size (&dont_print_statmem_obstack);

	  if (last_set_recurse != recurse)
	    {
	      stat_array_obstack_initial_size =
		obstack_object_size (&dont_print_stat_array_obstack);

	      last_set_recurse = recurse;
	    }
	}

      vptr_fieldno = get_vptr_fieldno (type, &vptr_basetype);
      for (i = n_baseclasses; i < len; i++)
	{
	  const gdb_byte *valaddr = value_contents_for_printing (val);

	  /* If requested, skip printing of static fields.  */
	  if (!options->static_field_print
	      && field_is_static (&type->field (i)))
	    continue;

	  if (fields_seen)
	    {
	      fputs_filtered (",", stream);
	      if (!options->prettyformat)
		fputs_filtered (" ", stream);
	    }
	  else if (n_baseclasses > 0)
	    {
	      if (options->prettyformat)
		{
		  fprintf_filtered (stream, "\n");
		  print_spaces_filtered (2 + 2 * recurse, stream);
		  fputs_filtered ("members of ", stream);
		  fputs_filtered (type->name (), stream);
		  fputs_filtered (":", stream);
		}
	    }
	  fields_seen = 1;

	  if (options->prettyformat)
	    {
	      fprintf_filtered (stream, "\n");
	      print_spaces_filtered (2 + 2 * recurse, stream);
	    }
	  else
	    {
	      wrap_here (n_spaces (2 + 2 * recurse));
	    }

	  annotate_field_begin (type->field (i).type ());

	  if (field_is_static (&type->field (i)))
	    {
	      fputs_filtered ("static ", stream);
	      fprintf_symbol_filtered (stream,
				       TYPE_FIELD_NAME (type, i),
				       current_language->la_language,
				       DMGL_PARAMS | DMGL_ANSI);
	    }
	  else
	    fputs_styled (TYPE_FIELD_NAME (type, i),
			  variable_name_style.style (), stream);
	  annotate_field_name_end ();

	  /* We tweak various options in a few cases below.  */
	  value_print_options options_copy = *options;
	  value_print_options *opts = &options_copy;

	  /* Do not print leading '=' in case of anonymous
	     unions.  */
	  if (strcmp (TYPE_FIELD_NAME (type, i), ""))
	    fputs_filtered (" = ", stream);
	  else
	    {
	      /* If this is an anonymous field then we want to consider it
		 as though it is at its parent's depth when it comes to the
		 max print depth.  */
	      if (opts->max_depth != -1 && opts->max_depth < INT_MAX)
		++opts->max_depth;
	    }
	  annotate_field_value ();

	  if (!field_is_static (&type->field (i))
	      && TYPE_FIELD_PACKED (type, i))
	    {
	      struct value *v;

	      /* Bitfields require special handling, especially due to
	         byte order problems.  */
	      if (TYPE_FIELD_IGNORE (type, i))
		{
		  fputs_styled ("<optimized out or zero length>",
				metadata_style.style (), stream);
		}
	      else if (value_bits_synthetic_pointer (val,
						     TYPE_FIELD_BITPOS (type,
									i),
						     TYPE_FIELD_BITSIZE (type,
									 i)))
		{
		  fputs_styled (_("<synthetic pointer>"),
				metadata_style.style (), stream);
		}
	      else
		{
		  opts->deref_ref = 0;

		  v = value_field_bitfield (type, i, valaddr,
					    value_embedded_offset (val), val);

		  common_val_print (v, stream, recurse + 1,
				    opts, current_language);
		}
	    }
	  else
	    {
	      if (TYPE_FIELD_IGNORE (type, i))
		{
		  fputs_styled ("<optimized out or zero length>",
				metadata_style.style (), stream);
		}
	      else if (field_is_static (&type->field (i)))
		{
		  try
		    {
		      struct value *v = value_static_field (type, i);

		      cp_print_static_field (type->field (i).type (),
					     v, stream, recurse + 1,
					     opts);
		    }
		  catch (const gdb_exception_error &ex)
		    {
		      fprintf_styled (stream, metadata_style.style (),
				      _("<error reading variable: %s>"),
				      ex.what ());
		    }
		}
	      else if (i == vptr_fieldno && type == vptr_basetype)
		{
		  int i_offset = TYPE_FIELD_BITPOS (type, i) / 8;
		  struct type *i_type = type->field (i).type ();

		  if (valprint_check_validity (stream, i_type, i_offset, val))
		    {
		      CORE_ADDR addr;

		      i_offset += value_embedded_offset (val);
		      addr = extract_typed_address (valaddr + i_offset, i_type);
		      print_function_pointer_address (opts,
						      get_type_arch (type),
						      addr, stream);
		    }
		}
	      else
		{
		  struct value *v = value_primitive_field (val, 0, i, type);
		  opts->deref_ref = 0;
		  common_val_print (v, stream, recurse + 1, opts,
				    current_language);
		}
	    }
	  annotate_field_end ();
	}

      if (dont_print_statmem == 0)
	{
	  size_t obstack_final_size =
           obstack_object_size (&dont_print_statmem_obstack);

	  if (obstack_final_size > statmem_obstack_initial_size)
	    {
	      /* In effect, a pop of the printed-statics stack.  */
	      size_t shrink_bytes
		= statmem_obstack_initial_size - obstack_final_size;
	      obstack_blank_fast (&dont_print_statmem_obstack, shrink_bytes);
	    }

	  if (last_set_recurse != recurse)
	    {
	      obstack_final_size =
		obstack_object_size (&dont_print_stat_array_obstack);

	      if (obstack_final_size > stat_array_obstack_initial_size)
		{
		  void *free_to_ptr =
		    (char *) obstack_next_free (&dont_print_stat_array_obstack)
		    - (obstack_final_size
		       - stat_array_obstack_initial_size);

		  obstack_free (&dont_print_stat_array_obstack,
				free_to_ptr);
		}
	      last_set_recurse = -1;
	    }
	}

      if (options->prettyformat)
	{
	  fprintf_filtered (stream, "\n");
	  print_spaces_filtered (2 * recurse, stream);
	}
    }				/* if there are data fields */

  fprintf_filtered (stream, "}");
}

/* Special val_print routine to avoid printing multiple copies of
   virtual baseclasses.  */

static void
cp_print_value (struct value *val, struct ui_file *stream,
		int recurse, const struct value_print_options *options,
		struct type **dont_print_vb)
{
  struct type *type = check_typedef (value_type (val));
  CORE_ADDR address = value_address (val);
  struct type **last_dont_print
    = (struct type **) obstack_next_free (&dont_print_vb_obstack);
  struct obstack tmp_obstack = dont_print_vb_obstack;
  int i, n_baseclasses = TYPE_N_BASECLASSES (type);
  const gdb_byte *valaddr = value_contents_for_printing (val);

  if (dont_print_vb == 0)
    {
      /* If we're at top level, carve out a completely fresh chunk of
         the obstack and use that until this particular invocation
         returns.  */
      /* Bump up the high-water mark.  Now alpha is omega.  */
      obstack_finish (&dont_print_vb_obstack);
    }

  for (i = 0; i < n_baseclasses; i++)
    {
      LONGEST boffset = 0;
      int skip = 0;
      struct type *baseclass = check_typedef (TYPE_BASECLASS (type, i));
      const char *basename = baseclass->name ();
      struct value *base_val = NULL;

      if (BASETYPE_VIA_VIRTUAL (type, i))
	{
	  struct type **first_dont_print
	    = (struct type **) obstack_base (&dont_print_vb_obstack);

	  int j = (struct type **)
	    obstack_next_free (&dont_print_vb_obstack) - first_dont_print;

	  while (--j >= 0)
	    if (baseclass == first_dont_print[j])
	      goto flush_it;

	  obstack_ptr_grow (&dont_print_vb_obstack, baseclass);
	}

      try
	{
	  boffset = baseclass_offset (type, i, valaddr,
				      value_embedded_offset (val),
				      address, val);
	}
      catch (const gdb_exception_error &ex)
	{
	  if (ex.error == NOT_AVAILABLE_ERROR)
	    skip = -1;
	  else
	    skip = 1;
	}

      if (skip == 0)
	{
	  if (BASETYPE_VIA_VIRTUAL (type, i))
	    {
	      /* The virtual base class pointer might have been
		 clobbered by the user program. Make sure that it
		 still points to a valid memory location.  */

	      if (boffset < 0 || boffset >= TYPE_LENGTH (type))
		{
		  gdb::byte_vector buf (TYPE_LENGTH (baseclass));

		  if (target_read_memory (address + boffset, buf.data (),
					  TYPE_LENGTH (baseclass)) != 0)
		    skip = 1;
		  base_val = value_from_contents_and_address (baseclass,
							      buf.data (),
							      address + boffset);
		  baseclass = value_type (base_val);
		  boffset = 0;
		}
	      else
		{
		  base_val = val;
		}
	    }
	  else
	    {
	      base_val = val;
	    }
	}

      /* Now do the printing.  */
      if (options->prettyformat)
	{
	  fprintf_filtered (stream, "\n");
	  print_spaces_filtered (2 * recurse, stream);
	}
      fputs_filtered ("<", stream);
      /* Not sure what the best notation is in the case where there is
         no baseclass name.  */
      fputs_filtered (basename ? basename : "", stream);
      fputs_filtered ("> = ", stream);

      if (skip < 0)
	val_print_unavailable (stream);
      else if (skip > 0)
	val_print_invalid_address (stream);
      else
	{
	  int result = 0;

	  if (options->max_depth > -1
	      && recurse >= options->max_depth)
	    {
	      const struct language_defn *language = current_language;
	      gdb_assert (language->la_struct_too_deep_ellipsis != NULL);
	      fputs_filtered (language->la_struct_too_deep_ellipsis, stream);
	    }
	  else
	    {
	      struct value *baseclass_val = value_primitive_field (val, 0,
								   i, type);

	      /* Attempt to run an extension language pretty-printer on the
		 baseclass if possible.  */
	      if (!options->raw)
		result
		  = apply_ext_lang_val_pretty_printer (baseclass_val, stream,
						       recurse, options,
						       current_language);

	      if (!result)
		cp_print_value_fields (baseclass_val, stream, recurse, options,
				       ((struct type **)
					obstack_base (&dont_print_vb_obstack)),
				       0);
	    }
	}
      fputs_filtered (", ", stream);

    flush_it:
      ;
    }

  if (dont_print_vb == 0)
    {
      /* Free the space used to deal with the printing
         of this type from top level.  */
      obstack_free (&dont_print_vb_obstack, last_dont_print);
      /* Reset watermark so that we can continue protecting
         ourselves from whatever we were protecting ourselves.  */
      dont_print_vb_obstack = tmp_obstack;
    }
}

/* Print value of a static member.  To avoid infinite recursion when
   printing a class that contains a static instance of the class, we
   keep the addresses of all printed static member classes in an
   obstack and refuse to print them more than once.

   VAL contains the value to print, TYPE, STREAM, RECURSE, and OPTIONS
   have the same meanings as in c_val_print.  */

static void
cp_print_static_field (struct type *type,
		       struct value *val,
		       struct ui_file *stream,
		       int recurse,
		       const struct value_print_options *options)
{
  struct value_print_options opts;

  if (value_entirely_optimized_out (val))
    {
      val_print_optimized_out (val, stream);
      return;
    }

  struct type *real_type = check_typedef (type);
  if (real_type->code () == TYPE_CODE_STRUCT)
    {
      CORE_ADDR *first_dont_print;
      CORE_ADDR addr = value_address (val);
      int i;

      first_dont_print
	= (CORE_ADDR *) obstack_base (&dont_print_statmem_obstack);
      i = obstack_object_size (&dont_print_statmem_obstack)
	/ sizeof (CORE_ADDR);

      while (--i >= 0)
	{
	  if (addr == first_dont_print[i])
	    {
	      fputs_styled (_("<same as static member of an already"
			      " seen type>"),
			    metadata_style.style (), stream);
	      return;
	    }
	}

      obstack_grow (&dont_print_statmem_obstack, (char *) &addr,
		    sizeof (CORE_ADDR));
      cp_print_value_fields (val, stream, recurse, options, NULL, 1);
      return;
    }

  if (real_type->code () == TYPE_CODE_ARRAY)
    {
      struct type **first_dont_print;
      int i;
      struct type *target_type = TYPE_TARGET_TYPE (type);

      first_dont_print
	= (struct type **) obstack_base (&dont_print_stat_array_obstack);
      i = obstack_object_size (&dont_print_stat_array_obstack)
	/ sizeof (struct type *);

      while (--i >= 0)
	{
	  if (target_type == first_dont_print[i])
	    {
	      fputs_styled (_("<same as static member of an already"
			      " seen type>"),
			    metadata_style.style (), stream);
	      return;
	    }
	}

      obstack_grow (&dont_print_stat_array_obstack,
		    (char *) &target_type,
		    sizeof (struct type *));
    }

  opts = *options;
  opts.deref_ref = 0;
  common_val_print (val, stream, recurse, &opts, current_language);
}

/* Find the field in *SELF, or its non-virtual base classes, with
   bit offset OFFSET.  Set *SELF to the containing type and *FIELDNO
   to the containing field number.  If OFFSET is not exactly at the
   start of some field, set *SELF to NULL.  */

static void
cp_find_class_member (struct type **self_p, int *fieldno,
		      LONGEST offset)
{
  struct type *self;
  unsigned int i;
  unsigned len;

  *self_p = check_typedef (*self_p);
  self = *self_p;
  len = self->num_fields ();

  for (i = TYPE_N_BASECLASSES (self); i < len; i++)
    {
      LONGEST bitpos = TYPE_FIELD_BITPOS (self, i);

      QUIT;
      if (offset == bitpos)
	{
	  *fieldno = i;
	  return;
	}
    }

  for (i = 0; i < TYPE_N_BASECLASSES (self); i++)
    {
      LONGEST bitpos = TYPE_FIELD_BITPOS (self, i);
      LONGEST bitsize = 8 * TYPE_LENGTH (self->field (i).type ());

      if (offset >= bitpos && offset < bitpos + bitsize)
	{
	  *self_p = self->field (i).type ();
	  cp_find_class_member (self_p, fieldno, offset - bitpos);
	  return;
	}
    }

  *self_p = NULL;
}

void
cp_print_class_member (const gdb_byte *valaddr, struct type *type,
		       struct ui_file *stream, const char *prefix)
{
  enum bfd_endian byte_order = type_byte_order (type);

  /* VAL is a byte offset into the structure type SELF_TYPE.
     Find the name of the field for that offset and
     print it.  */
  struct type *self_type = TYPE_SELF_TYPE (type);
  LONGEST val;
  int fieldno;

  val = extract_signed_integer (valaddr,
				TYPE_LENGTH (type),
				byte_order);

  /* Pointers to data members are usually byte offsets into an object.
     Because a data member can have offset zero, and a NULL pointer to
     member must be distinct from any valid non-NULL pointer to
     member, either the value is biased or the NULL value has a
     special representation; both are permitted by ISO C++.  HP aCC
     used a bias of 0x20000000; HP cfront used a bias of 1; g++ 3.x
     and other compilers which use the Itanium ABI use -1 as the NULL
     value.  GDB only supports that last form; to add support for
     another form, make this into a cp-abi hook.  */

  if (val == -1)
    {
      fprintf_filtered (stream, "NULL");
      return;
    }

  cp_find_class_member (&self_type, &fieldno, val << 3);

  if (self_type != NULL)
    {
      const char *name;

      fputs_filtered (prefix, stream);
      name = self_type->name ();
      if (name)
	fputs_filtered (name, stream);
      else
	c_type_print_base (self_type, stream, 0, 0, &type_print_raw_options);
      fprintf_filtered (stream, "::");
      fputs_styled (TYPE_FIELD_NAME (self_type, fieldno),
		    variable_name_style.style (), stream);
    }
  else
    fprintf_filtered (stream, "%ld", (long) val);
}


void _initialize_cp_valprint ();
void
_initialize_cp_valprint ()
{
  obstack_begin (&dont_print_stat_array_obstack,
		 32 * sizeof (struct type *));
  obstack_begin (&dont_print_statmem_obstack,
		 32 * sizeof (CORE_ADDR));
  obstack_begin (&dont_print_vb_obstack,
		 32 * sizeof (struct type *));
}
