/* Support for printing C++ values for GDB, the GNU debugger.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
#include <string.h>
#include "c-lang.h"
#include "target.h"
#include "cp-abi.h"
#include "valprint.h"
#include "cp-support.h"
#include "language.h"
#include "python/python.h"
#include "exceptions.h"
#include "typeprint.h"

/* Controls printing of vtbl's.  */
static void
show_vtblprint (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("\
Printing of C++ virtual function tables is %s.\n"),
		    value);
}

/* Controls looking up an object's derived type using what we find in
   its vtables.  */
static void
show_objectprint (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c,
		  const char *value)
{
  fprintf_filtered (file, _("\
Printing of object's derived type based on vtable info is %s.\n"),
		    value);
}

static void
show_static_field_print (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c,
			 const char *value)
{
  fprintf_filtered (file,
		    _("Printing of C++ static members is %s.\n"),
		    value);
}


static struct obstack dont_print_vb_obstack;
static struct obstack dont_print_statmem_obstack;
static struct obstack dont_print_stat_array_obstack;

extern void _initialize_cp_valprint (void);

static void cp_print_static_field (struct type *, struct value *,
				   struct ui_file *, int,
				   const struct value_print_options *);

static void cp_print_value (struct type *, struct type *,
			    const gdb_byte *, int,
			    CORE_ADDR, struct ui_file *,
			    int, const struct value *,
			    const struct value_print_options *,
			    struct type **);


/* GCC versions after 2.4.5 use this.  */
const char vtbl_ptr_name[] = "__vtbl_ptr_type";

/* Return truth value for assertion that TYPE is of the type
   "pointer to virtual function".  */

int
cp_is_vtbl_ptr_type (struct type *type)
{
  const char *typename = type_name_no_tag (type);

  return (typename != NULL && !strcmp (typename, vtbl_ptr_name));
}

/* Return truth value for the assertion that TYPE is of the type
   "pointer to virtual function table".  */

int
cp_is_vtbl_member (struct type *type)
{
  /* With older versions of g++, the vtbl field pointed to an array of
     structures.  Nowadays it points directly to the structure.  */
  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    {
      type = TYPE_TARGET_TYPE (type);
      if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
	{
	  type = TYPE_TARGET_TYPE (type);
	  if (TYPE_CODE (type) == TYPE_CODE_STRUCT    /* if not using thunks */
	      || TYPE_CODE (type) == TYPE_CODE_PTR)   /* if using thunks */
	    {
	      /* Virtual functions tables are full of pointers
	         to virtual functions.  */
	      return cp_is_vtbl_ptr_type (type);
	    }
	}
      else if (TYPE_CODE (type) == TYPE_CODE_STRUCT)  /* if not using thunks */
	{
	  return cp_is_vtbl_ptr_type (type);
	}
      else if (TYPE_CODE (type) == TYPE_CODE_PTR)     /* if using thunks */
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
cp_print_value_fields (struct type *type, struct type *real_type,
		       const gdb_byte *valaddr, int offset,
		       CORE_ADDR address, struct ui_file *stream,
		       int recurse, const struct value *val,
		       const struct value_print_options *options,
		       struct type **dont_print_vb,
		       int dont_print_statmem)
{
  int i, len, n_baseclasses;
  int fields_seen = 0;
  static int last_set_recurse = -1;

  CHECK_TYPEDEF (type);
  
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
  len = TYPE_NFIELDS (type);
  n_baseclasses = TYPE_N_BASECLASSES (type);

  /* First, print out baseclasses such that we don't print
     duplicates of virtual baseclasses.  */

  if (n_baseclasses > 0)
    cp_print_value (type, real_type, valaddr, 
		    offset, address, stream,
		    recurse + 1, val, options,
		    dont_print_vb);

  /* Second, print out data fields */

  /* If there are no data fields, skip this part */
  if (len == n_baseclasses || !len)
    fprintf_filtered (stream, "<No data fields>");
  else
    {
      int statmem_obstack_initial_size = 0;
      int stat_array_obstack_initial_size = 0;
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
	  /* If requested, skip printing of static fields.  */
	  if (!options->static_field_print
	      && field_is_static (&TYPE_FIELD (type, i)))
	    continue;

	  if (fields_seen)
	    fprintf_filtered (stream, ", ");
	  else if (n_baseclasses > 0)
	    {
	      if (options->prettyformat)
		{
		  fprintf_filtered (stream, "\n");
		  print_spaces_filtered (2 + 2 * recurse, stream);
		  fputs_filtered ("members of ", stream);
		  fputs_filtered (type_name_no_tag (type), stream);
		  fputs_filtered (": ", stream);
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

	  annotate_field_begin (TYPE_FIELD_TYPE (type, i));

	  if (field_is_static (&TYPE_FIELD (type, i)))
	    fputs_filtered ("static ", stream);
	  fprintf_symbol_filtered (stream,
				   TYPE_FIELD_NAME (type, i),
				   current_language->la_language,
				   DMGL_PARAMS | DMGL_ANSI);
	  annotate_field_name_end ();
	  /* Do not print leading '=' in case of anonymous
	     unions.  */
	  if (strcmp (TYPE_FIELD_NAME (type, i), ""))
	    fputs_filtered (" = ", stream);
	  annotate_field_value ();

	  if (!field_is_static (&TYPE_FIELD (type, i))
	      && TYPE_FIELD_PACKED (type, i))
	    {
	      struct value *v;

	      /* Bitfields require special handling, especially due to
	         byte order problems.  */
	      if (TYPE_FIELD_IGNORE (type, i))
		{
		  fputs_filtered ("<optimized out or zero length>", stream);
		}
	      else if (value_bits_synthetic_pointer (val,
						     TYPE_FIELD_BITPOS (type,
									i),
						     TYPE_FIELD_BITSIZE (type,
									 i)))
		{
		  fputs_filtered (_("<synthetic pointer>"), stream);
		}
	      else if (!value_bits_valid (val,
					  TYPE_FIELD_BITPOS (type, i),
					  TYPE_FIELD_BITSIZE (type, i)))
		{
		  val_print_optimized_out (val, stream);
		}
	      else
		{
		  struct value_print_options opts = *options;

		  opts.deref_ref = 0;

		  v = value_field_bitfield (type, i, valaddr, offset, val);

		  common_val_print (v, stream, recurse + 1, &opts,
				    current_language);
		}
	    }
	  else
	    {
	      if (TYPE_FIELD_IGNORE (type, i))
		{
		  fputs_filtered ("<optimized out or zero length>",
				  stream);
		}
	      else if (field_is_static (&TYPE_FIELD (type, i)))
		{
		  volatile struct gdb_exception ex;
		  struct value *v = NULL;

		  TRY_CATCH (ex, RETURN_MASK_ERROR)
		    {
		      v = value_static_field (type, i);
		    }

		  if (ex.reason < 0)
		    fprintf_filtered (stream,
				      _("<error reading variable: %s>"),
				      ex.message);
		  cp_print_static_field (TYPE_FIELD_TYPE (type, i),
					 v, stream, recurse + 1,
					 options);
		}
	      else if (i == vptr_fieldno && type == vptr_basetype)
		{
		  int i_offset = offset + TYPE_FIELD_BITPOS (type, i) / 8;
		  struct type *i_type = TYPE_FIELD_TYPE (type, i);

		  if (valprint_check_validity (stream, i_type, i_offset, val))
		    {
		      CORE_ADDR addr;
		      
		      addr = extract_typed_address (valaddr + i_offset, i_type);
		      print_function_pointer_address (options,
						      get_type_arch (type),
						      addr, stream);
		    }
		}
	      else
		{
		  struct value_print_options opts = *options;

		  opts.deref_ref = 0;
		  val_print (TYPE_FIELD_TYPE (type, i),
			     valaddr, 
			     offset + TYPE_FIELD_BITPOS (type, i) / 8,
			     address,
			     stream, recurse + 1, val, &opts,
			     current_language);
		}
	    }
	  annotate_field_end ();
	}

      if (dont_print_statmem == 0)
	{
	  int obstack_final_size =
           obstack_object_size (&dont_print_statmem_obstack);

	  if (obstack_final_size > statmem_obstack_initial_size)
	    {
	      /* In effect, a pop of the printed-statics stack.  */

	      void *free_to_ptr =
		obstack_next_free (&dont_print_statmem_obstack) -
		(obstack_final_size - statmem_obstack_initial_size);

	      obstack_free (&dont_print_statmem_obstack,
			    free_to_ptr);
	    }

	  if (last_set_recurse != recurse)
	    {
	      int obstack_final_size =
		obstack_object_size (&dont_print_stat_array_obstack);
	      
	      if (obstack_final_size > stat_array_obstack_initial_size)
		{
		  void *free_to_ptr =
		    obstack_next_free (&dont_print_stat_array_obstack)
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

/* Like cp_print_value_fields, but find the runtime type of the object
   and pass it as the `real_type' argument to cp_print_value_fields.
   This function is a hack to work around the fact that
   common_val_print passes the embedded offset to val_print, but not
   the enclosing type.  */

void
cp_print_value_fields_rtti (struct type *type,
			    const gdb_byte *valaddr, int offset,
			    CORE_ADDR address,
			    struct ui_file *stream, int recurse,
			    const struct value *val,
			    const struct value_print_options *options,
			    struct type **dont_print_vb, 
			    int dont_print_statmem)
{
  struct type *real_type = NULL;

  /* We require all bits to be valid in order to attempt a
     conversion.  */
  if (value_bits_valid (val, TARGET_CHAR_BIT * offset,
			TARGET_CHAR_BIT * TYPE_LENGTH (type)))
    {
      struct value *value;
      int full, top, using_enc;

      /* Ugh, we have to convert back to a value here.  */
      value = value_from_contents_and_address (type, valaddr + offset,
					       address + offset);
      /* We don't actually care about most of the result here -- just
	 the type.  We already have the correct offset, due to how
	 val_print was initially called.  */
      real_type = value_rtti_type (value, &full, &top, &using_enc);
    }

  if (!real_type)
    real_type = type;

  cp_print_value_fields (type, real_type, valaddr, offset,
			 address, stream, recurse, val, options,
			 dont_print_vb, dont_print_statmem);
}

/* Special val_print routine to avoid printing multiple copies of
   virtual baseclasses.  */

static void
cp_print_value (struct type *type, struct type *real_type,
		const gdb_byte *valaddr, int offset,
		CORE_ADDR address, struct ui_file *stream,
		int recurse, const struct value *val,
		const struct value_print_options *options,
		struct type **dont_print_vb)
{
  struct type **last_dont_print
    = (struct type **) obstack_next_free (&dont_print_vb_obstack);
  struct obstack tmp_obstack = dont_print_vb_obstack;
  int i, n_baseclasses = TYPE_N_BASECLASSES (type);
  int thisoffset;
  struct type *thistype;

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
      int boffset = 0;
      int skip;
      struct type *baseclass = check_typedef (TYPE_BASECLASS (type, i));
      const char *basename = TYPE_NAME (baseclass);
      const gdb_byte *base_valaddr = NULL;
      const struct value *base_val = NULL;
      volatile struct gdb_exception ex;

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

      thisoffset = offset;
      thistype = real_type;

      TRY_CATCH (ex, RETURN_MASK_ERROR)
	{
	  boffset = baseclass_offset (type, i, valaddr, offset, address, val);
	}
      if (ex.reason < 0 && ex.error == NOT_AVAILABLE_ERROR)
	skip = -1;
      else if (ex.reason < 0)
	skip = 1;
      else
 	{
	  skip = 0;

	  if (BASETYPE_VIA_VIRTUAL (type, i))
	    {
	      /* The virtual base class pointer might have been
		 clobbered by the user program. Make sure that it
		 still points to a valid memory location.  */

	      if ((boffset + offset) < 0
		  || (boffset + offset) >= TYPE_LENGTH (real_type))
		{
		  gdb_byte *buf;
		  struct cleanup *back_to;

		  buf = xmalloc (TYPE_LENGTH (baseclass));
		  back_to = make_cleanup (xfree, buf);

		  if (target_read_memory (address + boffset, buf,
					  TYPE_LENGTH (baseclass)) != 0)
		    skip = 1;
		  base_val = value_from_contents_and_address (baseclass,
							      buf,
							      address + boffset);
		  thisoffset = 0;
		  boffset = 0;
		  thistype = baseclass;
		  base_valaddr = value_contents_for_printing_const (base_val);
		  do_cleanups (back_to);
		}
	      else
		{
		  base_valaddr = valaddr;
		  base_val = val;
		}
	    }
	  else
	    {
	      base_valaddr = valaddr;
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

	  /* Attempt to run the Python pretty-printers on the
	     baseclass if possible.  */
	  if (!options->raw)
	    result = apply_val_pretty_printer (baseclass, base_valaddr,
					       thisoffset + boffset,
					       value_address (base_val),
					       stream, recurse, base_val,
					       options, current_language);


	  	  
	  if (!result)
	    cp_print_value_fields (baseclass, thistype, base_valaddr,
				   thisoffset + boffset,
				   value_address (base_val),
				   stream, recurse, base_val, options,
				   ((struct type **)
				    obstack_base (&dont_print_vb_obstack)),
				   0);
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

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
    {
      CORE_ADDR *first_dont_print;
      CORE_ADDR addr;
      int i;

      first_dont_print
	= (CORE_ADDR *) obstack_base (&dont_print_statmem_obstack);
      i = obstack_object_size (&dont_print_statmem_obstack)
	/ sizeof (CORE_ADDR);

      while (--i >= 0)
	{
	  if (value_address (val) == first_dont_print[i])
	    {
	      fputs_filtered ("<same as static member of an already"
			      " seen type>",
			      stream);
	      return;
	    }
	}

      addr = value_address (val);
      obstack_grow (&dont_print_statmem_obstack, (char *) &addr,
		    sizeof (CORE_ADDR));
      CHECK_TYPEDEF (type);
      cp_print_value_fields (type, value_enclosing_type (val),
			     value_contents_for_printing (val),
			     value_embedded_offset (val), addr,
			     stream, recurse, val,
			     options, NULL, 1);
      return;
    }

  if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
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
	      fputs_filtered ("<same as static member of an already"
			      " seen type>",
			      stream);
	      return;
	    }
	}

      obstack_grow (&dont_print_stat_array_obstack,
		    (char *) &target_type,
		    sizeof (struct type *));
    }

  opts = *options;
  opts.deref_ref = 0;
  val_print (type, value_contents_for_printing (val), 
	     value_embedded_offset (val),
	     value_address (val),
	     stream, recurse, val,
	     &opts, current_language);
}


/* Find the field in *DOMAIN, or its non-virtual base classes, with
   bit offset OFFSET.  Set *DOMAIN to the containing type and *FIELDNO
   to the containing field number.  If OFFSET is not exactly at the
   start of some field, set *DOMAIN to NULL.  */

static void
cp_find_class_member (struct type **domain_p, int *fieldno,
		      LONGEST offset)
{
  struct type *domain;
  unsigned int i;
  unsigned len;

  *domain_p = check_typedef (*domain_p);
  domain = *domain_p;
  len = TYPE_NFIELDS (domain);

  for (i = TYPE_N_BASECLASSES (domain); i < len; i++)
    {
      LONGEST bitpos = TYPE_FIELD_BITPOS (domain, i);

      QUIT;
      if (offset == bitpos)
	{
	  *fieldno = i;
	  return;
	}
    }

  for (i = 0; i < TYPE_N_BASECLASSES (domain); i++)
    {
      LONGEST bitpos = TYPE_FIELD_BITPOS (domain, i);
      LONGEST bitsize = 8 * TYPE_LENGTH (TYPE_FIELD_TYPE (domain, i));

      if (offset >= bitpos && offset < bitpos + bitsize)
	{
	  *domain_p = TYPE_FIELD_TYPE (domain, i);
	  cp_find_class_member (domain_p, fieldno, offset - bitpos);
	  return;
	}
    }

  *domain_p = NULL;
}

void
cp_print_class_member (const gdb_byte *valaddr, struct type *type,
		       struct ui_file *stream, char *prefix)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));

  /* VAL is a byte offset into the structure type DOMAIN.
     Find the name of the field for that offset and
     print it.  */
  struct type *domain = TYPE_DOMAIN_TYPE (type);
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

  cp_find_class_member (&domain, &fieldno, val << 3);

  if (domain != NULL)
    {
      const char *name;

      fputs_filtered (prefix, stream);
      name = type_name_no_tag (domain);
      if (name)
	fputs_filtered (name, stream);
      else
	c_type_print_base (domain, stream, 0, 0, &type_print_raw_options);
      fprintf_filtered (stream, "::");
      fputs_filtered (TYPE_FIELD_NAME (domain, fieldno), stream);
    }
  else
    fprintf_filtered (stream, "%ld", (long) val);
}


void
_initialize_cp_valprint (void)
{
  add_setshow_boolean_cmd ("static-members", class_support,
			   &user_print_options.static_field_print, _("\
Set printing of C++ static members."), _("\
Show printing of C++ static members."), NULL,
			   NULL,
			   show_static_field_print,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("vtbl", class_support,
			   &user_print_options.vtblprint, _("\
Set printing of C++ virtual function tables."), _("\
Show printing of C++ virtual function tables."), NULL,
			   NULL,
			   show_vtblprint,
			   &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("object", class_support,
			   &user_print_options.objectprint, _("\
Set printing of object's derived type based on vtable info."), _("\
Show printing of object's derived type based on vtable info."), NULL,
			   NULL,
			   show_objectprint,
			   &setprintlist, &showprintlist);

  obstack_begin (&dont_print_stat_array_obstack,
		 32 * sizeof (struct type *));
  obstack_begin (&dont_print_statmem_obstack,
		 32 * sizeof (CORE_ADDR));
  obstack_begin (&dont_print_vb_obstack,
		 32 * sizeof (struct type *));
}
