/* Abstraction of various C++ ABI's we support, and the info we need
   to get from them.

   Contributed by Daniel Berlin <dberlin@redhat.com>

   Copyright (C) 2001-2023 Free Software Foundation, Inc.

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

#ifndef CP_ABI_H
#define CP_ABI_H

struct fn_field;
struct type;
struct value;
struct ui_file;
class frame_info_ptr;

/* The functions here that attempt to determine what sort of thing a
   mangled name refers to may well be revised in the future.  It would
   certainly be cleaner to carry this information explicitly in GDB's
   data structures than to derive it from the mangled name.  */


/* Kinds of constructors.  All these values are guaranteed to be
   non-zero.  */
enum ctor_kinds {

  /* Initialize a complete object, including virtual bases, using
     memory provided by caller.  */
  complete_object_ctor = 1,

  /* Initialize a base object of some larger object.  */
  base_object_ctor,

  /* An allocating complete-object constructor.  */
  complete_object_allocating_ctor
};

/* Return non-zero iff NAME is the mangled name of a constructor.
   Actually, return an `enum ctor_kind' value describing what *kind*
   of constructor it is.  */
extern enum ctor_kinds is_constructor_name (const char *name);


/* Kinds of destructors.  All these values are guaranteed to be
   non-zero.  */
enum dtor_kinds {

  /* A destructor which finalizes the entire object, and then calls
     `delete' on its storage.  */
  deleting_dtor = 1,

  /* A destructor which finalizes the entire object, but does not call
     `delete'.  */
  complete_object_dtor,

  /* A destructor which finalizes a subobject of some larger
     object.  */
  base_object_dtor
};
  
/* Return non-zero iff NAME is the mangled name of a destructor.
   Actually, return an `enum dtor_kind' value describing what *kind*
   of destructor it is.  */
extern enum dtor_kinds is_destructor_name (const char *name);


/* Return non-zero iff NAME is the mangled name of a vtable.  */
extern int is_vtable_name (const char *name);


/* Return non-zero iff NAME is the un-mangled name of an operator,
   perhaps scoped within some class.  */
extern int is_operator_name (const char *name);


/* Return an object's virtual function as a value.

   VALUEP is a pointer to a pointer to a value, holding the object
   whose virtual function we want to invoke.  If the ABI requires a
   virtual function's caller to adjust the `this' pointer by an amount
   retrieved from the vtable before invoking the function (i.e., we're
   not using "vtable thunks" to do the adjustment automatically), then
   this function may set *VALUEP to point to a new object with an
   appropriately tweaked address.

   The J'th element of the overload set F is the virtual function of
   *VALUEP we want to invoke.

   TYPE is the base type of *VALUEP whose method we're invoking ---
   this is the type containing F.  OFFSET is the offset of that base
   type within *VALUEP.  */
extern struct value *value_virtual_fn_field (struct value **valuep,
					     struct fn_field *f,
					     int j,
					     struct type *type,
					     int offset);


/* Try to find the run-time type of VALUE, using C++ run-time type
   information.  Return the run-time type, or zero if we can't figure
   it out.

   If we do find the run-time type:
   - Set *FULL to non-zero if VALUE already contains the complete
     run-time object, not just some embedded base class of the object.
   - Set *TOP and *USING_ENC to indicate where the enclosing object
     starts relative to VALUE:
     - If *USING_ENC is zero, then *TOP is the offset from the start
       of the complete object to the start of the embedded subobject
       VALUE represents.  In other words, the enclosing object starts
       at VALUE_ADDR (VALUE) + VALUE_OFFSET (VALUE) +
       value_embedded_offset (VALUE) + *TOP
     - If *USING_ENC is non-zero, then *TOP is the offset from the
       address of the complete object to the enclosing object stored
       in VALUE.  In other words, the enclosing object starts at
       VALUE_ADDR (VALUE) + VALUE_OFFSET (VALUE) + *TOP.
     If VALUE's type and enclosing type are the same, then these two
     cases are equivalent.

   FULL, TOP, and USING_ENC can each be zero, in which case we don't
   provide the corresponding piece of information.  */
extern struct type *value_rtti_type (struct value *value,
				     int *full, LONGEST *top,
				     int *using_enc);

/* Compute the offset of the baseclass which is the INDEXth baseclass
   of class TYPE, for value at VALADDR (in host) at ADDRESS (in
   target), offset by EMBEDDED_OFFSET.  VALADDR points to the raw
   contents of VAL.  The result is the offset of the baseclass value
   relative to (the address of)(ARG) + OFFSET.  */

extern int baseclass_offset (struct type *type,
			     int index, const gdb_byte *valaddr,
			     LONGEST embedded_offset,
			     CORE_ADDR address,
			     const struct value *val);

/* Describe the target of a pointer to method.  CONTENTS is the byte
   pattern representing the pointer to method.  TYPE is the pointer to
   method type.  STREAM is the stream to print it to.  */
void cplus_print_method_ptr (const gdb_byte *contents,
			     struct type *type,
			     struct ui_file *stream);

/* Return the size of a pointer to member function of type
   TO_TYPE.  */
int cplus_method_ptr_size (struct type *to_type);

/* Return the method which should be called by applying METHOD_PTR to
   *THIS_P, and adjust *THIS_P if necessary.  */
struct value *cplus_method_ptr_to_value (struct value **this_p,
					 struct value *method_ptr);

/* Create the byte pattern in CONTENTS representing a pointer of type
   TYPE to member function at ADDRESS (if IS_VIRTUAL is 0) or with
   virtual table offset ADDRESS (if IS_VIRTUAL is 1).  This is the
   opposite of cplus_method_ptr_to_value.  */
void cplus_make_method_ptr (struct type *type, gdb_byte *CONTENTS,
			    CORE_ADDR address, int is_virtual);

/* Print the vtable for VALUE, if there is one.  If there is no
   vtable, print a message, but do not throw.  */

void cplus_print_vtable (struct value *value);

/* Implement 'typeid': find the type info for VALUE, if possible.  If
   the type info cannot be found, throw an exception.  */

extern struct value *cplus_typeid (struct value *value);

/* Return the type of 'typeid' for the current C++ ABI on the given
   architecture.  */

extern struct type *cplus_typeid_type (struct gdbarch *gdbarch);

/* Given a value which holds a pointer to a std::type_info, return the
   type which that type_info represents.  Throw an exception if the
   type cannot be found.  */

extern struct type *cplus_type_from_type_info (struct value *value);

/* Given a value which holds a pointer to a std::type_info, return the
   name of the type which that type_info represents.  Throw an
   exception if the type name cannot be found.  */

extern std::string cplus_typename_from_type_info (struct value *value);

/* Determine if we are currently in a C++ thunk.  If so, get the
   address of the routine we are thunking to and continue to there
   instead.  */

CORE_ADDR cplus_skip_trampoline (frame_info_ptr frame,
				 CORE_ADDR stop_pc);

/* Return a struct that provides pass-by-reference information
   about the given TYPE.  */

extern struct language_pass_by_ref_info cp_pass_by_reference
  (struct type *type);

struct cp_abi_ops
{
  const char *shortname;
  const char *longname;
  const char *doc;

  /* ABI-specific implementations for the functions declared
     above.  */
  enum ctor_kinds (*is_constructor_name) (const char *name);
  enum dtor_kinds (*is_destructor_name) (const char *name);
  int (*is_vtable_name) (const char *name);
  int (*is_operator_name) (const char *name);
  struct value *(*virtual_fn_field) (struct value **arg1p,
				     struct fn_field * f,
				     int j, struct type * type,
				     int offset);
  struct type *(*rtti_type) (struct value *v, int *full,
			     LONGEST *top, int *using_enc);
  int (*baseclass_offset) (struct type *type, int index,
			   const bfd_byte *valaddr, LONGEST embedded_offset,
			   CORE_ADDR address, const struct value *val);
  void (*print_method_ptr) (const gdb_byte *contents,
			    struct type *type,
			    struct ui_file *stream);
  int (*method_ptr_size) (struct type *);
  void (*make_method_ptr) (struct type *, gdb_byte *,
			   CORE_ADDR, int);
  struct value * (*method_ptr_to_value) (struct value **,
					 struct value *);
  void (*print_vtable) (struct value *);
  struct value *(*get_typeid) (struct value *value);
  struct type *(*get_typeid_type) (struct gdbarch *gdbarch);
  struct type *(*get_type_from_type_info) (struct value *value);
  std::string (*get_typename_from_type_info) (struct value *value);
  CORE_ADDR (*skip_trampoline) (frame_info_ptr, CORE_ADDR);
  struct language_pass_by_ref_info (*pass_by_reference) (struct type *type);
};


extern int register_cp_abi (struct cp_abi_ops *abi);
extern void set_cp_abi_as_auto_default (const char *short_name);

#endif /* CP_ABI_H */
