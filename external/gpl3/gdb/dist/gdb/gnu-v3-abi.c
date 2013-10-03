/* Abstraction of GNU v3 abi.
   Contributed by Jim Blandy <jimb@redhat.com>

   Copyright (C) 2001-2013 Free Software Foundation, Inc.

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
#include "value.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "demangle.h"
#include "objfiles.h"
#include "valprint.h"
#include "c-lang.h"
#include "exceptions.h"
#include "typeprint.h"

#include "gdb_assert.h"
#include "gdb_string.h"

static struct cp_abi_ops gnu_v3_abi_ops;

static int
gnuv3_is_vtable_name (const char *name)
{
  return strncmp (name, "_ZTV", 4) == 0;
}

static int
gnuv3_is_operator_name (const char *name)
{
  return strncmp (name, "operator", 8) == 0;
}


/* To help us find the components of a vtable, we build ourselves a
   GDB type object representing the vtable structure.  Following the
   V3 ABI, it goes something like this:

   struct gdb_gnu_v3_abi_vtable {

     / * An array of virtual call and virtual base offsets.  The real
         length of this array depends on the class hierarchy; we use
         negative subscripts to access the elements.  Yucky, but
         better than the alternatives.  * /
     ptrdiff_t vcall_and_vbase_offsets[0];

     / * The offset from a virtual pointer referring to this table
         to the top of the complete object.  * /
     ptrdiff_t offset_to_top;

     / * The type_info pointer for this class.  This is really a
         std::type_info *, but GDB doesn't really look at the
         type_info object itself, so we don't bother to get the type
         exactly right.  * /
     void *type_info;

     / * Virtual table pointers in objects point here.  * /

     / * Virtual function pointers.  Like the vcall/vbase array, the
         real length of this table depends on the class hierarchy.  * /
     void (*virtual_functions[0]) ();

   };

   The catch, of course, is that the exact layout of this table
   depends on the ABI --- word size, endianness, alignment, etc.  So
   the GDB type object is actually a per-architecture kind of thing.

   vtable_type_gdbarch_data is a gdbarch per-architecture data pointer
   which refers to the struct type * for this structure, laid out
   appropriately for the architecture.  */
static struct gdbarch_data *vtable_type_gdbarch_data;


/* Human-readable names for the numbers of the fields above.  */
enum {
  vtable_field_vcall_and_vbase_offsets,
  vtable_field_offset_to_top,
  vtable_field_type_info,
  vtable_field_virtual_functions
};


/* Return a GDB type representing `struct gdb_gnu_v3_abi_vtable',
   described above, laid out appropriately for ARCH.

   We use this function as the gdbarch per-architecture data
   initialization function.  */
static void *
build_gdb_vtable_type (struct gdbarch *arch)
{
  struct type *t;
  struct field *field_list, *field;
  int offset;

  struct type *void_ptr_type
    = builtin_type (arch)->builtin_data_ptr;
  struct type *ptr_to_void_fn_type
    = builtin_type (arch)->builtin_func_ptr;

  /* ARCH can't give us the true ptrdiff_t type, so we guess.  */
  struct type *ptrdiff_type
    = arch_integer_type (arch, gdbarch_ptr_bit (arch), 0, "ptrdiff_t");

  /* We assume no padding is necessary, since GDB doesn't know
     anything about alignment at the moment.  If this assumption bites
     us, we should add a gdbarch method which, given a type, returns
     the alignment that type requires, and then use that here.  */

  /* Build the field list.  */
  field_list = xmalloc (sizeof (struct field [4]));
  memset (field_list, 0, sizeof (struct field [4]));
  field = &field_list[0];
  offset = 0;

  /* ptrdiff_t vcall_and_vbase_offsets[0]; */
  FIELD_NAME (*field) = "vcall_and_vbase_offsets";
  FIELD_TYPE (*field) = lookup_array_range_type (ptrdiff_type, 0, -1);
  SET_FIELD_BITPOS (*field, offset * TARGET_CHAR_BIT);
  offset += TYPE_LENGTH (FIELD_TYPE (*field));
  field++;

  /* ptrdiff_t offset_to_top; */
  FIELD_NAME (*field) = "offset_to_top";
  FIELD_TYPE (*field) = ptrdiff_type;
  SET_FIELD_BITPOS (*field, offset * TARGET_CHAR_BIT);
  offset += TYPE_LENGTH (FIELD_TYPE (*field));
  field++;

  /* void *type_info; */
  FIELD_NAME (*field) = "type_info";
  FIELD_TYPE (*field) = void_ptr_type;
  SET_FIELD_BITPOS (*field, offset * TARGET_CHAR_BIT);
  offset += TYPE_LENGTH (FIELD_TYPE (*field));
  field++;

  /* void (*virtual_functions[0]) (); */
  FIELD_NAME (*field) = "virtual_functions";
  FIELD_TYPE (*field) = lookup_array_range_type (ptr_to_void_fn_type, 0, -1);
  SET_FIELD_BITPOS (*field, offset * TARGET_CHAR_BIT);
  offset += TYPE_LENGTH (FIELD_TYPE (*field));
  field++;

  /* We assumed in the allocation above that there were four fields.  */
  gdb_assert (field == (field_list + 4));

  t = arch_type (arch, TYPE_CODE_STRUCT, offset, NULL);
  TYPE_NFIELDS (t) = field - field_list;
  TYPE_FIELDS (t) = field_list;
  TYPE_TAG_NAME (t) = "gdb_gnu_v3_abi_vtable";
  INIT_CPLUS_SPECIFIC (t);

  return t;
}


/* Return the ptrdiff_t type used in the vtable type.  */
static struct type *
vtable_ptrdiff_type (struct gdbarch *gdbarch)
{
  struct type *vtable_type = gdbarch_data (gdbarch, vtable_type_gdbarch_data);

  /* The "offset_to_top" field has the appropriate (ptrdiff_t) type.  */
  return TYPE_FIELD_TYPE (vtable_type, vtable_field_offset_to_top);
}

/* Return the offset from the start of the imaginary `struct
   gdb_gnu_v3_abi_vtable' object to the vtable's "address point"
   (i.e., where objects' virtual table pointers point).  */
static int
vtable_address_point_offset (struct gdbarch *gdbarch)
{
  struct type *vtable_type = gdbarch_data (gdbarch, vtable_type_gdbarch_data);

  return (TYPE_FIELD_BITPOS (vtable_type, vtable_field_virtual_functions)
          / TARGET_CHAR_BIT);
}


/* Determine whether structure TYPE is a dynamic class.  Cache the
   result.  */

static int
gnuv3_dynamic_class (struct type *type)
{
  int fieldnum, fieldelem;

  if (TYPE_CPLUS_DYNAMIC (type))
    return TYPE_CPLUS_DYNAMIC (type) == 1;

  ALLOCATE_CPLUS_STRUCT_TYPE (type);

  for (fieldnum = 0; fieldnum < TYPE_N_BASECLASSES (type); fieldnum++)
    if (BASETYPE_VIA_VIRTUAL (type, fieldnum)
	|| gnuv3_dynamic_class (TYPE_FIELD_TYPE (type, fieldnum)))
      {
	TYPE_CPLUS_DYNAMIC (type) = 1;
	return 1;
      }

  for (fieldnum = 0; fieldnum < TYPE_NFN_FIELDS (type); fieldnum++)
    for (fieldelem = 0; fieldelem < TYPE_FN_FIELDLIST_LENGTH (type, fieldnum);
	 fieldelem++)
      {
	struct fn_field *f = TYPE_FN_FIELDLIST1 (type, fieldnum);

	if (TYPE_FN_FIELD_VIRTUAL_P (f, fieldelem))
	  {
	    TYPE_CPLUS_DYNAMIC (type) = 1;
	    return 1;
	  }
      }

  TYPE_CPLUS_DYNAMIC (type) = -1;
  return 0;
}

/* Find the vtable for a value of CONTAINER_TYPE located at
   CONTAINER_ADDR.  Return a value of the correct vtable type for this
   architecture, or NULL if CONTAINER does not have a vtable.  */

static struct value *
gnuv3_get_vtable (struct gdbarch *gdbarch,
		  struct type *container_type, CORE_ADDR container_addr)
{
  struct type *vtable_type = gdbarch_data (gdbarch,
					   vtable_type_gdbarch_data);
  struct type *vtable_pointer_type;
  struct value *vtable_pointer;
  CORE_ADDR vtable_address;

  /* If this type does not have a virtual table, don't read the first
     field.  */
  if (!gnuv3_dynamic_class (check_typedef (container_type)))
    return NULL;

  /* We do not consult the debug information to find the virtual table.
     The ABI specifies that it is always at offset zero in any class,
     and debug information may not represent it.

     We avoid using value_contents on principle, because the object might
     be large.  */

  /* Find the type "pointer to virtual table".  */
  vtable_pointer_type = lookup_pointer_type (vtable_type);

  /* Load it from the start of the class.  */
  vtable_pointer = value_at (vtable_pointer_type, container_addr);
  vtable_address = value_as_address (vtable_pointer);

  /* Correct it to point at the start of the virtual table, rather
     than the address point.  */
  return value_at_lazy (vtable_type,
			vtable_address
			- vtable_address_point_offset (gdbarch));
}


static struct type *
gnuv3_rtti_type (struct value *value,
                 int *full_p, int *top_p, int *using_enc_p)
{
  struct gdbarch *gdbarch;
  struct type *values_type = check_typedef (value_type (value));
  struct value *vtable;
  struct minimal_symbol *vtable_symbol;
  const char *vtable_symbol_name;
  const char *class_name;
  struct type *run_time_type;
  LONGEST offset_to_top;

  /* We only have RTTI for class objects.  */
  if (TYPE_CODE (values_type) != TYPE_CODE_CLASS)
    return NULL;

  /* Java doesn't have RTTI following the C++ ABI.  */
  if (TYPE_CPLUS_REALLY_JAVA (values_type))
    return NULL;

  /* Determine architecture.  */
  gdbarch = get_type_arch (values_type);

  if (using_enc_p)
    *using_enc_p = 0;

  vtable = gnuv3_get_vtable (gdbarch, value_type (value),
			     value_as_address (value_addr (value)));
  if (vtable == NULL)
    return NULL;

  /* Find the linker symbol for this vtable.  */
  vtable_symbol
    = lookup_minimal_symbol_by_pc (value_address (vtable)
                                   + value_embedded_offset (vtable));
  if (! vtable_symbol)
    return NULL;
  
  /* The symbol's demangled name should be something like "vtable for
     CLASS", where CLASS is the name of the run-time type of VALUE.
     If we didn't like this approach, we could instead look in the
     type_info object itself to get the class name.  But this way
     should work just as well, and doesn't read target memory.  */
  vtable_symbol_name = SYMBOL_DEMANGLED_NAME (vtable_symbol);
  if (vtable_symbol_name == NULL
      || strncmp (vtable_symbol_name, "vtable for ", 11))
    {
      warning (_("can't find linker symbol for virtual table for `%s' value"),
	       TYPE_SAFE_NAME (values_type));
      if (vtable_symbol_name)
	warning (_("  found `%s' instead"), vtable_symbol_name);
      return NULL;
    }
  class_name = vtable_symbol_name + 11;

  /* Try to look up the class name as a type name.  */
  /* FIXME: chastain/2003-11-26: block=NULL is bogus.  See pr gdb/1465.  */
  run_time_type = cp_lookup_rtti_type (class_name, NULL);
  if (run_time_type == NULL)
    return NULL;

  /* Get the offset from VALUE to the top of the complete object.
     NOTE: this is the reverse of the meaning of *TOP_P.  */
  offset_to_top
    = value_as_long (value_field (vtable, vtable_field_offset_to_top));

  if (full_p)
    *full_p = (- offset_to_top == value_embedded_offset (value)
               && (TYPE_LENGTH (value_enclosing_type (value))
                   >= TYPE_LENGTH (run_time_type)));
  if (top_p)
    *top_p = - offset_to_top;
  return run_time_type;
}

/* Return a function pointer for CONTAINER's VTABLE_INDEX'th virtual
   function, of type FNTYPE.  */

static struct value *
gnuv3_get_virtual_fn (struct gdbarch *gdbarch, struct value *container,
		      struct type *fntype, int vtable_index)
{
  struct value *vtable, *vfn;

  /* Every class with virtual functions must have a vtable.  */
  vtable = gnuv3_get_vtable (gdbarch, value_type (container),
			     value_as_address (value_addr (container)));
  gdb_assert (vtable != NULL);

  /* Fetch the appropriate function pointer from the vtable.  */
  vfn = value_subscript (value_field (vtable, vtable_field_virtual_functions),
                         vtable_index);

  /* If this architecture uses function descriptors directly in the vtable,
     then the address of the vtable entry is actually a "function pointer"
     (i.e. points to the descriptor).  We don't need to scale the index
     by the size of a function descriptor; GCC does that before outputing
     debug information.  */
  if (gdbarch_vtable_function_descriptors (gdbarch))
    vfn = value_addr (vfn);

  /* Cast the function pointer to the appropriate type.  */
  vfn = value_cast (lookup_pointer_type (fntype), vfn);

  return vfn;
}

/* GNU v3 implementation of value_virtual_fn_field.  See cp-abi.h
   for a description of the arguments.  */

static struct value *
gnuv3_virtual_fn_field (struct value **value_p,
                        struct fn_field *f, int j,
			struct type *vfn_base, int offset)
{
  struct type *values_type = check_typedef (value_type (*value_p));
  struct gdbarch *gdbarch;

  /* Some simple sanity checks.  */
  if (TYPE_CODE (values_type) != TYPE_CODE_CLASS)
    error (_("Only classes can have virtual functions."));

  /* Determine architecture.  */
  gdbarch = get_type_arch (values_type);

  /* Cast our value to the base class which defines this virtual
     function.  This takes care of any necessary `this'
     adjustments.  */
  if (vfn_base != values_type)
    *value_p = value_cast (vfn_base, *value_p);

  return gnuv3_get_virtual_fn (gdbarch, *value_p, TYPE_FN_FIELD_TYPE (f, j),
			       TYPE_FN_FIELD_VOFFSET (f, j));
}

/* Compute the offset of the baseclass which is
   the INDEXth baseclass of class TYPE,
   for value at VALADDR (in host) at ADDRESS (in target).
   The result is the offset of the baseclass value relative
   to (the address of)(ARG) + OFFSET.

   -1 is returned on error.  */

static int
gnuv3_baseclass_offset (struct type *type, int index,
			const bfd_byte *valaddr, int embedded_offset,
			CORE_ADDR address, const struct value *val)
{
  struct gdbarch *gdbarch;
  struct type *ptr_type;
  struct value *vtable;
  struct value *vbase_array;
  long int cur_base_offset, base_offset;

  /* Determine architecture.  */
  gdbarch = get_type_arch (type);
  ptr_type = builtin_type (gdbarch)->builtin_data_ptr;

  /* If it isn't a virtual base, this is easy.  The offset is in the
     type definition.  Likewise for Java, which doesn't really have
     virtual inheritance in the C++ sense.  */
  if (!BASETYPE_VIA_VIRTUAL (type, index) || TYPE_CPLUS_REALLY_JAVA (type))
    return TYPE_BASECLASS_BITPOS (type, index) / 8;

  /* To access a virtual base, we need to use the vbase offset stored in
     our vtable.  Recent GCC versions provide this information.  If it isn't
     available, we could get what we needed from RTTI, or from drawing the
     complete inheritance graph based on the debug info.  Neither is
     worthwhile.  */
  cur_base_offset = TYPE_BASECLASS_BITPOS (type, index) / 8;
  if (cur_base_offset >= - vtable_address_point_offset (gdbarch))
    error (_("Expected a negative vbase offset (old compiler?)"));

  cur_base_offset = cur_base_offset + vtable_address_point_offset (gdbarch);
  if ((- cur_base_offset) % TYPE_LENGTH (ptr_type) != 0)
    error (_("Misaligned vbase offset."));
  cur_base_offset = cur_base_offset / ((int) TYPE_LENGTH (ptr_type));

  vtable = gnuv3_get_vtable (gdbarch, type, address + embedded_offset);
  gdb_assert (vtable != NULL);
  vbase_array = value_field (vtable, vtable_field_vcall_and_vbase_offsets);
  base_offset = value_as_long (value_subscript (vbase_array, cur_base_offset));
  return base_offset;
}

/* Locate a virtual method in DOMAIN or its non-virtual base classes
   which has virtual table index VOFFSET.  The method has an associated
   "this" adjustment of ADJUSTMENT bytes.  */

static const char *
gnuv3_find_method_in (struct type *domain, CORE_ADDR voffset,
		      LONGEST adjustment)
{
  int i;

  /* Search this class first.  */
  if (adjustment == 0)
    {
      int len;

      len = TYPE_NFN_FIELDS (domain);
      for (i = 0; i < len; i++)
	{
	  int len2, j;
	  struct fn_field *f;

	  f = TYPE_FN_FIELDLIST1 (domain, i);
	  len2 = TYPE_FN_FIELDLIST_LENGTH (domain, i);

	  check_stub_method_group (domain, i);
	  for (j = 0; j < len2; j++)
	    if (TYPE_FN_FIELD_VOFFSET (f, j) == voffset)
	      return TYPE_FN_FIELD_PHYSNAME (f, j);
	}
    }

  /* Next search non-virtual bases.  If it's in a virtual base,
     we're out of luck.  */
  for (i = 0; i < TYPE_N_BASECLASSES (domain); i++)
    {
      int pos;
      struct type *basetype;

      if (BASETYPE_VIA_VIRTUAL (domain, i))
	continue;

      pos = TYPE_BASECLASS_BITPOS (domain, i) / 8;
      basetype = TYPE_FIELD_TYPE (domain, i);
      /* Recurse with a modified adjustment.  We don't need to adjust
	 voffset.  */
      if (adjustment >= pos && adjustment < pos + TYPE_LENGTH (basetype))
	return gnuv3_find_method_in (basetype, voffset, adjustment - pos);
    }

  return NULL;
}

/* Decode GNU v3 method pointer.  */

static int
gnuv3_decode_method_ptr (struct gdbarch *gdbarch,
			 const gdb_byte *contents,
			 CORE_ADDR *value_p,
			 LONGEST *adjustment_p)
{
  struct type *funcptr_type = builtin_type (gdbarch)->builtin_func_ptr;
  struct type *offset_type = vtable_ptrdiff_type (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR ptr_value;
  LONGEST voffset, adjustment;
  int vbit;

  /* Extract the pointer to member.  The first element is either a pointer
     or a vtable offset.  For pointers, we need to use extract_typed_address
     to allow the back-end to convert the pointer to a GDB address -- but
     vtable offsets we must handle as integers.  At this point, we do not
     yet know which case we have, so we extract the value under both
     interpretations and choose the right one later on.  */
  ptr_value = extract_typed_address (contents, funcptr_type);
  voffset = extract_signed_integer (contents,
				    TYPE_LENGTH (funcptr_type), byte_order);
  contents += TYPE_LENGTH (funcptr_type);
  adjustment = extract_signed_integer (contents,
				       TYPE_LENGTH (offset_type), byte_order);

  if (!gdbarch_vbit_in_delta (gdbarch))
    {
      vbit = voffset & 1;
      voffset = voffset ^ vbit;
    }
  else
    {
      vbit = adjustment & 1;
      adjustment = adjustment >> 1;
    }

  *value_p = vbit? voffset : ptr_value;
  *adjustment_p = adjustment;
  return vbit;
}

/* GNU v3 implementation of cplus_print_method_ptr.  */

static void
gnuv3_print_method_ptr (const gdb_byte *contents,
			struct type *type,
			struct ui_file *stream)
{
  struct type *domain = TYPE_DOMAIN_TYPE (type);
  struct gdbarch *gdbarch = get_type_arch (domain);
  CORE_ADDR ptr_value;
  LONGEST adjustment;
  int vbit;

  /* Extract the pointer to member.  */
  vbit = gnuv3_decode_method_ptr (gdbarch, contents, &ptr_value, &adjustment);

  /* Check for NULL.  */
  if (ptr_value == 0 && vbit == 0)
    {
      fprintf_filtered (stream, "NULL");
      return;
    }

  /* Search for a virtual method.  */
  if (vbit)
    {
      CORE_ADDR voffset;
      const char *physname;

      /* It's a virtual table offset, maybe in this class.  Search
	 for a field with the correct vtable offset.  First convert it
	 to an index, as used in TYPE_FN_FIELD_VOFFSET.  */
      voffset = ptr_value / TYPE_LENGTH (vtable_ptrdiff_type (gdbarch));

      physname = gnuv3_find_method_in (domain, voffset, adjustment);

      /* If we found a method, print that.  We don't bother to disambiguate
	 possible paths to the method based on the adjustment.  */
      if (physname)
	{
	  char *demangled_name = cplus_demangle (physname,
						 DMGL_ANSI | DMGL_PARAMS);

	  fprintf_filtered (stream, "&virtual ");
	  if (demangled_name == NULL)
	    fputs_filtered (physname, stream);
	  else
	    {
	      fputs_filtered (demangled_name, stream);
	      xfree (demangled_name);
	    }
	  return;
	}
    }
  else if (ptr_value != 0)
    {
      /* Found a non-virtual function: print out the type.  */
      fputs_filtered ("(", stream);
      c_print_type (type, "", stream, -1, 0, &type_print_raw_options);
      fputs_filtered (") ", stream);
    }

  /* We didn't find it; print the raw data.  */
  if (vbit)
    {
      fprintf_filtered (stream, "&virtual table offset ");
      print_longest (stream, 'd', 1, ptr_value);
    }
  else
    {
      struct value_print_options opts;

      get_user_print_options (&opts);
      print_address_demangle (&opts, gdbarch, ptr_value, stream, demangle);
    }

  if (adjustment)
    {
      fprintf_filtered (stream, ", this adjustment ");
      print_longest (stream, 'd', 1, adjustment);
    }
}

/* GNU v3 implementation of cplus_method_ptr_size.  */

static int
gnuv3_method_ptr_size (struct type *type)
{
  struct gdbarch *gdbarch = get_type_arch (type);

  return 2 * TYPE_LENGTH (builtin_type (gdbarch)->builtin_data_ptr);
}

/* GNU v3 implementation of cplus_make_method_ptr.  */

static void
gnuv3_make_method_ptr (struct type *type, gdb_byte *contents,
		       CORE_ADDR value, int is_virtual)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  int size = TYPE_LENGTH (builtin_type (gdbarch)->builtin_data_ptr);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* FIXME drow/2006-12-24: The adjustment of "this" is currently
     always zero, since the method pointer is of the correct type.
     But if the method pointer came from a base class, this is
     incorrect - it should be the offset to the base.  The best
     fix might be to create the pointer to member pointing at the
     base class and cast it to the derived class, but that requires
     support for adjusting pointers to members when casting them -
     not currently supported by GDB.  */

  if (!gdbarch_vbit_in_delta (gdbarch))
    {
      store_unsigned_integer (contents, size, byte_order, value | is_virtual);
      store_unsigned_integer (contents + size, size, byte_order, 0);
    }
  else
    {
      store_unsigned_integer (contents, size, byte_order, value);
      store_unsigned_integer (contents + size, size, byte_order, is_virtual);
    }
}

/* GNU v3 implementation of cplus_method_ptr_to_value.  */

static struct value *
gnuv3_method_ptr_to_value (struct value **this_p, struct value *method_ptr)
{
  struct gdbarch *gdbarch;
  const gdb_byte *contents = value_contents (method_ptr);
  CORE_ADDR ptr_value;
  struct type *domain_type, *final_type, *method_type;
  LONGEST adjustment;
  int vbit;

  domain_type = TYPE_DOMAIN_TYPE (check_typedef (value_type (method_ptr)));
  final_type = lookup_pointer_type (domain_type);

  method_type = TYPE_TARGET_TYPE (check_typedef (value_type (method_ptr)));

  /* Extract the pointer to member.  */
  gdbarch = get_type_arch (domain_type);
  vbit = gnuv3_decode_method_ptr (gdbarch, contents, &ptr_value, &adjustment);

  /* First convert THIS to match the containing type of the pointer to
     member.  This cast may adjust the value of THIS.  */
  *this_p = value_cast (final_type, *this_p);

  /* Then apply whatever adjustment is necessary.  This creates a somewhat
     strange pointer: it claims to have type FINAL_TYPE, but in fact it
     might not be a valid FINAL_TYPE.  For instance, it might be a
     base class of FINAL_TYPE.  And if it's not the primary base class,
     then printing it out as a FINAL_TYPE object would produce some pretty
     garbage.

     But we don't really know the type of the first argument in
     METHOD_TYPE either, which is why this happens.  We can't
     dereference this later as a FINAL_TYPE, but once we arrive in the
     called method we'll have debugging information for the type of
     "this" - and that'll match the value we produce here.

     You can provoke this case by casting a Base::* to a Derived::*, for
     instance.  */
  *this_p = value_cast (builtin_type (gdbarch)->builtin_data_ptr, *this_p);
  *this_p = value_ptradd (*this_p, adjustment);
  *this_p = value_cast (final_type, *this_p);

  if (vbit)
    {
      LONGEST voffset;

      voffset = ptr_value / TYPE_LENGTH (vtable_ptrdiff_type (gdbarch));
      return gnuv3_get_virtual_fn (gdbarch, value_ind (*this_p),
				   method_type, voffset);
    }
  else
    return value_from_pointer (lookup_pointer_type (method_type), ptr_value);
}

/* Objects of this type are stored in a hash table and a vector when
   printing the vtables for a class.  */

struct value_and_voffset
{
  /* The value representing the object.  */
  struct value *value;

  /* The maximum vtable offset we've found for any object at this
     offset in the outermost object.  */
  int max_voffset;
};

typedef struct value_and_voffset *value_and_voffset_p;
DEF_VEC_P (value_and_voffset_p);

/* Hash function for value_and_voffset.  */

static hashval_t
hash_value_and_voffset (const void *p)
{
  const struct value_and_voffset *o = p;

  return value_address (o->value) + value_embedded_offset (o->value);
}

/* Equality function for value_and_voffset.  */

static int
eq_value_and_voffset (const void *a, const void *b)
{
  const struct value_and_voffset *ova = a;
  const struct value_and_voffset *ovb = b;

  return (value_address (ova->value) + value_embedded_offset (ova->value)
	  == value_address (ovb->value) + value_embedded_offset (ovb->value));
}

/* qsort comparison function for value_and_voffset.  */

static int
compare_value_and_voffset (const void *a, const void *b)
{
  const struct value_and_voffset * const *ova = a;
  CORE_ADDR addra = (value_address ((*ova)->value)
		     + value_embedded_offset ((*ova)->value));
  const struct value_and_voffset * const *ovb = b;
  CORE_ADDR addrb = (value_address ((*ovb)->value)
		     + value_embedded_offset ((*ovb)->value));

  if (addra < addrb)
    return -1;
  if (addra > addrb)
    return 1;
  return 0;
}

/* A helper function used when printing vtables.  This determines the
   key (most derived) sub-object at each address and also computes the
   maximum vtable offset seen for the corresponding vtable.  Updates
   OFFSET_HASH and OFFSET_VEC with a new value_and_voffset object, if
   needed.  VALUE is the object to examine.  */

static void
compute_vtable_size (htab_t offset_hash,
		     VEC (value_and_voffset_p) **offset_vec,
		     struct value *value)
{
  int i;
  struct type *type = check_typedef (value_type (value));
  void **slot;
  struct value_and_voffset search_vo, *current_vo;

  /* If the object is not dynamic, then we are done; as it cannot have
     dynamic base types either.  */
  if (!gnuv3_dynamic_class (type))
    return;

  /* Update the hash and the vec, if needed.  */
  search_vo.value = value;
  slot = htab_find_slot (offset_hash, &search_vo, INSERT);
  if (*slot)
    current_vo = *slot;
  else
    {
      current_vo = XNEW (struct value_and_voffset);
      current_vo->value = value;
      current_vo->max_voffset = -1;
      *slot = current_vo;
      VEC_safe_push (value_and_voffset_p, *offset_vec, current_vo);
    }

  /* Update the value_and_voffset object with the highest vtable
     offset from this class.  */
  for (i = 0; i < TYPE_NFN_FIELDS (type); ++i)
    {
      int j;
      struct fn_field *fn = TYPE_FN_FIELDLIST1 (type, i);

      for (j = 0; j < TYPE_FN_FIELDLIST_LENGTH (type, i); ++j)
	{
	  if (TYPE_FN_FIELD_VIRTUAL_P (fn, j))
	    {
	      int voffset = TYPE_FN_FIELD_VOFFSET (fn, j);

	      if (voffset > current_vo->max_voffset)
		current_vo->max_voffset = voffset;
	    }
	}
    }

  /* Recurse into base classes.  */
  for (i = 0; i < TYPE_N_BASECLASSES (type); ++i)
    compute_vtable_size (offset_hash, offset_vec, value_field (value, i));
}

/* Helper for gnuv3_print_vtable that prints a single vtable.  */

static void
print_one_vtable (struct gdbarch *gdbarch, struct value *value,
		  int max_voffset,
		  struct value_print_options *opts)
{
  int i;
  struct type *type = check_typedef (value_type (value));
  struct value *vtable;
  CORE_ADDR vt_addr;

  vtable = gnuv3_get_vtable (gdbarch, type,
			     value_address (value)
			     + value_embedded_offset (value));
  vt_addr = value_address (value_field (vtable,
					vtable_field_virtual_functions));

  printf_filtered (_("vtable for '%s' @ %s (subobject @ %s):\n"),
		   TYPE_SAFE_NAME (type),
		   paddress (gdbarch, vt_addr),
		   paddress (gdbarch, (value_address (value)
				       + value_embedded_offset (value))));

  for (i = 0; i <= max_voffset; ++i)
    {
      /* Initialize it just to avoid a GCC false warning.  */
      CORE_ADDR addr = 0;
      struct value *vfn;
      volatile struct gdb_exception ex;

      printf_filtered ("[%d]: ", i);

      vfn = value_subscript (value_field (vtable,
					  vtable_field_virtual_functions),
			     i);

      if (gdbarch_vtable_function_descriptors (gdbarch))
	vfn = value_addr (vfn);

      TRY_CATCH (ex, RETURN_MASK_ERROR)
	{
	  addr = value_as_address (vfn);
	}
      if (ex.reason < 0)
	printf_filtered (_("<error: %s>"), ex.message);
      else
	print_function_pointer_address (opts, gdbarch, addr, gdb_stdout);
      printf_filtered ("\n");
    }
}

/* Implementation of the print_vtable method.  */

static void
gnuv3_print_vtable (struct value *value)
{
  struct gdbarch *gdbarch;
  struct type *type;
  struct value *vtable;
  struct value_print_options opts;
  htab_t offset_hash;
  struct cleanup *cleanup;
  VEC (value_and_voffset_p) *result_vec = NULL;
  struct value_and_voffset *iter;
  int i, count;

  value = coerce_ref (value);
  type = check_typedef (value_type (value));
  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    {
      value = value_ind (value);
      type = check_typedef (value_type (value));
    }

  get_user_print_options (&opts);

  /* Respect 'set print object'.  */
  if (opts.objectprint)
    {
      value = value_full_object (value, NULL, 0, 0, 0);
      type = check_typedef (value_type (value));
    }

  gdbarch = get_type_arch (type);
  vtable = gnuv3_get_vtable (gdbarch, type,
			     value_as_address (value_addr (value)));

  if (!vtable)
    {
      printf_filtered (_("This object does not have a virtual function table\n"));
      return;
    }

  offset_hash = htab_create_alloc (1, hash_value_and_voffset,
				   eq_value_and_voffset,
				   xfree, xcalloc, xfree);
  cleanup = make_cleanup_htab_delete (offset_hash);
  make_cleanup (VEC_cleanup (value_and_voffset_p), &result_vec);

  compute_vtable_size (offset_hash, &result_vec, value);

  qsort (VEC_address (value_and_voffset_p, result_vec),
	 VEC_length (value_and_voffset_p, result_vec),
	 sizeof (value_and_voffset_p),
	 compare_value_and_voffset);

  count = 0;
  for (i = 0; VEC_iterate (value_and_voffset_p, result_vec, i, iter); ++i)
    {
      if (iter->max_voffset >= 0)
	{
	  if (count > 0)
	    printf_filtered ("\n");
	  print_one_vtable (gdbarch, iter->value, iter->max_voffset, &opts);
	  ++count;
	}
    }

  do_cleanups (cleanup);
}

/* Determine if we are currently in a C++ thunk.  If so, get the address
   of the routine we are thunking to and continue to there instead.  */

static CORE_ADDR 
gnuv3_skip_trampoline (struct frame_info *frame, CORE_ADDR stop_pc)
{
  CORE_ADDR real_stop_pc, method_stop_pc;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct minimal_symbol *thunk_sym, *fn_sym;
  struct obj_section *section;
  const char *thunk_name, *fn_name;
  
  real_stop_pc = gdbarch_skip_trampoline_code (gdbarch, frame, stop_pc);
  if (real_stop_pc == 0)
    real_stop_pc = stop_pc;

  /* Find the linker symbol for this potential thunk.  */
  thunk_sym = lookup_minimal_symbol_by_pc (real_stop_pc);
  section = find_pc_section (real_stop_pc);
  if (thunk_sym == NULL || section == NULL)
    return 0;

  /* The symbol's demangled name should be something like "virtual
     thunk to FUNCTION", where FUNCTION is the name of the function
     being thunked to.  */
  thunk_name = SYMBOL_DEMANGLED_NAME (thunk_sym);
  if (thunk_name == NULL || strstr (thunk_name, " thunk to ") == NULL)
    return 0;

  fn_name = strstr (thunk_name, " thunk to ") + strlen (" thunk to ");
  fn_sym = lookup_minimal_symbol (fn_name, NULL, section->objfile);
  if (fn_sym == NULL)
    return 0;

  method_stop_pc = SYMBOL_VALUE_ADDRESS (fn_sym);
  real_stop_pc = gdbarch_skip_trampoline_code
		   (gdbarch, frame, method_stop_pc);
  if (real_stop_pc == 0)
    real_stop_pc = method_stop_pc;

  return real_stop_pc;
}

/* Return nonzero if a type should be passed by reference.

   The rule in the v3 ABI document comes from section 3.1.1.  If the
   type has a non-trivial copy constructor or destructor, then the
   caller must make a copy (by calling the copy constructor if there
   is one or perform the copy itself otherwise), pass the address of
   the copy, and then destroy the temporary (if necessary).

   For return values with non-trivial copy constructors or
   destructors, space will be allocated in the caller, and a pointer
   will be passed as the first argument (preceding "this").

   We don't have a bulletproof mechanism for determining whether a
   constructor or destructor is trivial.  For GCC and DWARF2 debug
   information, we can check the artificial flag.

   We don't do anything with the constructors or destructors,
   but we have to get the argument passing right anyway.  */
static int
gnuv3_pass_by_reference (struct type *type)
{
  int fieldnum, fieldelem;

  CHECK_TYPEDEF (type);

  /* We're only interested in things that can have methods.  */
  if (TYPE_CODE (type) != TYPE_CODE_STRUCT
      && TYPE_CODE (type) != TYPE_CODE_CLASS
      && TYPE_CODE (type) != TYPE_CODE_UNION)
    return 0;

  for (fieldnum = 0; fieldnum < TYPE_NFN_FIELDS (type); fieldnum++)
    for (fieldelem = 0; fieldelem < TYPE_FN_FIELDLIST_LENGTH (type, fieldnum);
	 fieldelem++)
      {
	struct fn_field *fn = TYPE_FN_FIELDLIST1 (type, fieldnum);
	const char *name = TYPE_FN_FIELDLIST_NAME (type, fieldnum);
	struct type *fieldtype = TYPE_FN_FIELD_TYPE (fn, fieldelem);

	/* If this function is marked as artificial, it is compiler-generated,
	   and we assume it is trivial.  */
	if (TYPE_FN_FIELD_ARTIFICIAL (fn, fieldelem))
	  continue;

	/* If we've found a destructor, we must pass this by reference.  */
	if (name[0] == '~')
	  return 1;

	/* If the mangled name of this method doesn't indicate that it
	   is a constructor, we're not interested.

	   FIXME drow/2007-09-23: We could do this using the name of
	   the method and the name of the class instead of dealing
	   with the mangled name.  We don't have a convenient function
	   to strip off both leading scope qualifiers and trailing
	   template arguments yet.  */
	if (!is_constructor_name (TYPE_FN_FIELD_PHYSNAME (fn, fieldelem))
	    && !TYPE_FN_FIELD_CONSTRUCTOR (fn, fieldelem))
	  continue;

	/* If this method takes two arguments, and the second argument is
	   a reference to this class, then it is a copy constructor.  */
	if (TYPE_NFIELDS (fieldtype) == 2
	    && TYPE_CODE (TYPE_FIELD_TYPE (fieldtype, 1)) == TYPE_CODE_REF
	    && check_typedef (TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (fieldtype,
								 1))) == type)
	  return 1;
      }

  /* Even if all the constructors and destructors were artificial, one
     of them may have invoked a non-artificial constructor or
     destructor in a base class.  If any base class needs to be passed
     by reference, so does this class.  Similarly for members, which
     are constructed whenever this class is.  We do not need to worry
     about recursive loops here, since we are only looking at members
     of complete class type.  Also ignore any static members.  */
  for (fieldnum = 0; fieldnum < TYPE_NFIELDS (type); fieldnum++)
    if (! field_is_static (&TYPE_FIELD (type, fieldnum))
        && gnuv3_pass_by_reference (TYPE_FIELD_TYPE (type, fieldnum)))
      return 1;

  return 0;
}

static void
init_gnuv3_ops (void)
{
  vtable_type_gdbarch_data
    = gdbarch_data_register_post_init (build_gdb_vtable_type);

  gnu_v3_abi_ops.shortname = "gnu-v3";
  gnu_v3_abi_ops.longname = "GNU G++ Version 3 ABI";
  gnu_v3_abi_ops.doc = "G++ Version 3 ABI";
  gnu_v3_abi_ops.is_destructor_name =
    (enum dtor_kinds (*) (const char *))is_gnu_v3_mangled_dtor;
  gnu_v3_abi_ops.is_constructor_name =
    (enum ctor_kinds (*) (const char *))is_gnu_v3_mangled_ctor;
  gnu_v3_abi_ops.is_vtable_name = gnuv3_is_vtable_name;
  gnu_v3_abi_ops.is_operator_name = gnuv3_is_operator_name;
  gnu_v3_abi_ops.rtti_type = gnuv3_rtti_type;
  gnu_v3_abi_ops.virtual_fn_field = gnuv3_virtual_fn_field;
  gnu_v3_abi_ops.baseclass_offset = gnuv3_baseclass_offset;
  gnu_v3_abi_ops.print_method_ptr = gnuv3_print_method_ptr;
  gnu_v3_abi_ops.method_ptr_size = gnuv3_method_ptr_size;
  gnu_v3_abi_ops.make_method_ptr = gnuv3_make_method_ptr;
  gnu_v3_abi_ops.method_ptr_to_value = gnuv3_method_ptr_to_value;
  gnu_v3_abi_ops.print_vtable = gnuv3_print_vtable;
  gnu_v3_abi_ops.skip_trampoline = gnuv3_skip_trampoline;
  gnu_v3_abi_ops.pass_by_reference = gnuv3_pass_by_reference;
}

extern initialize_file_ftype _initialize_gnu_v3_abi; /* -Wmissing-prototypes */

void
_initialize_gnu_v3_abi (void)
{
  init_gnuv3_ops ();

  register_cp_abi (&gnu_v3_abi_ops);
  set_cp_abi_as_auto_default (gnu_v3_abi_ops.shortname);
}
