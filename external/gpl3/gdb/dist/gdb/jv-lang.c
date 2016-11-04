/* Java language support routines for GDB, the GNU debugger.

   Copyright (C) 1997-2016 Free Software Foundation, Inc.

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
#include "expression.h"
#include "parser-defs.h"
#include "language.h"
#include "symfile.h"
#include "objfiles.h"
#include "value.h"
#include "c-lang.h"
#include "jv-lang.h"
#include "varobj.h"
#include "gdbcore.h"
#include "block.h"
#include "demangle.h"
#include "dictionary.h"
#include <ctype.h>
#include "charset.h"
#include "valprint.h"
#include "cp-support.h"

/* Local functions */

extern void _initialize_java_language (void);

static int java_demangled_signature_length (const char *);
static void java_demangled_signature_copy (char *, const char *);

static struct compunit_symtab *get_java_class_symtab (struct gdbarch *gdbarch);
static char *get_java_utf8_name (struct obstack *obstack, struct value *name);
static int java_class_is_primitive (struct value *clas);
static struct value *java_value_string (char *ptr, int len);

static void java_emit_char (int c, struct type *type,
			    struct ui_file * stream, int quoter);

static char *java_class_name_from_physname (const char *physname);

static const struct objfile_data *jv_dynamics_objfile_data_key;

/* The dynamic objfile is kept per-program-space.  This key lets us
   associate the objfile with the program space.  */

static const struct program_space_data *jv_dynamics_progspace_key;

static struct type *java_link_class_type (struct gdbarch *,
					  struct type *, struct value *);

/* An instance of this structure is used to store some data that must
   be freed.  */

struct jv_per_objfile_data
{
  /* The expandable dictionary we use.  */
  struct dictionary *dict;
};

/* A function called when the dynamics_objfile is freed.  We use this
   to clean up some internal state.  */
static void
jv_per_objfile_free (struct objfile *objfile, void *data)
{
  struct jv_per_objfile_data *jv_data = (struct jv_per_objfile_data *) data;
  struct objfile *dynamics_objfile;

  dynamics_objfile
    = (struct objfile *) program_space_data (current_program_space,
					     jv_dynamics_progspace_key);
  gdb_assert (objfile == dynamics_objfile);

  if (jv_data->dict)
    dict_free (jv_data->dict);
  xfree (jv_data);

  set_program_space_data (current_program_space,
			  jv_dynamics_progspace_key,
			  NULL);
}

/* FIXME: carlton/2003-02-04: This is the main or only caller of
   allocate_objfile with first argument NULL; as a result, this code
   breaks every so often.  Somebody should write a test case that
   exercises GDB in various ways (e.g. something involving loading a
   dynamic library) after this code has been called.  */

static struct objfile *
get_dynamics_objfile (struct gdbarch *gdbarch)
{
  struct objfile *dynamics_objfile;

  dynamics_objfile
    = (struct objfile *) program_space_data (current_program_space,
					     jv_dynamics_progspace_key);

  if (dynamics_objfile == NULL)
    {
      struct jv_per_objfile_data *data;

      /* Mark it as shared so that it is cleared when the inferior is
	 re-run.  */
      dynamics_objfile = allocate_objfile (NULL, NULL,
					   OBJF_SHARED | OBJF_NOT_FILENAME);
      dynamics_objfile->per_bfd->gdbarch = gdbarch;

      data = XCNEW (struct jv_per_objfile_data);
      set_objfile_data (dynamics_objfile, jv_dynamics_objfile_data_key, data);

      set_program_space_data (current_program_space,
			      jv_dynamics_progspace_key,
			      dynamics_objfile);
    }
  return dynamics_objfile;
}

static struct compunit_symtab *
get_java_class_symtab (struct gdbarch *gdbarch)
{
  struct objfile *objfile = get_dynamics_objfile (gdbarch);
  struct compunit_symtab *class_symtab = objfile->compunit_symtabs;

  if (class_symtab == NULL)
    {
      struct blockvector *bv;
      struct block *bl;
      struct jv_per_objfile_data *jv_data;

      class_symtab = allocate_compunit_symtab (objfile, "<java-classes>");
      add_compunit_symtab_to_objfile (class_symtab);
      allocate_symtab (class_symtab, "<java-classes>");

      COMPUNIT_FILETABS (class_symtab)->language = language_java;
      bv = (struct blockvector *)
	obstack_alloc (&objfile->objfile_obstack,
		       sizeof (struct blockvector) + sizeof (struct block *));
      BLOCKVECTOR_NBLOCKS (bv) = 1;
      COMPUNIT_BLOCKVECTOR (class_symtab) = bv;

      /* Allocate dummy STATIC_BLOCK.  */
      bl = allocate_block (&objfile->objfile_obstack);
      BLOCK_DICT (bl) = dict_create_linear (&objfile->objfile_obstack,
					    NULL);
      BLOCKVECTOR_BLOCK (bv, STATIC_BLOCK) = bl;

      /* Allocate GLOBAL_BLOCK.  */
      bl = allocate_global_block (&objfile->objfile_obstack);
      BLOCK_DICT (bl) = dict_create_hashed_expandable ();
      set_block_compunit_symtab (bl, class_symtab);
      BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK) = bl;

      /* Arrange to free the dict.  */
      jv_data = ((struct jv_per_objfile_data *)
		 objfile_data (objfile, jv_dynamics_objfile_data_key));
      jv_data->dict = BLOCK_DICT (bl);
    }
  return class_symtab;
}

static void
add_class_symtab_symbol (struct symbol *sym)
{
  struct compunit_symtab *cust = get_java_class_symtab (symbol_arch (sym));
  const struct blockvector *bv = COMPUNIT_BLOCKVECTOR (cust);

  dict_add_symbol (BLOCK_DICT (BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK)), sym);
}

static struct symbol *
add_class_symbol (struct type *type, CORE_ADDR addr)
{
  struct symbol *sym;
  struct objfile *objfile = get_dynamics_objfile (get_type_arch (type));

  sym = allocate_symbol (objfile);
  SYMBOL_SET_LANGUAGE (sym, language_java, &objfile->objfile_obstack);
  SYMBOL_SET_LINKAGE_NAME (sym, TYPE_TAG_NAME (type));
  SYMBOL_ACLASS_INDEX (sym) = LOC_TYPEDEF;
  /*  SYMBOL_VALUE (sym) = valu; */
  SYMBOL_TYPE (sym) = type;
  SYMBOL_DOMAIN (sym) = STRUCT_DOMAIN;
  SYMBOL_VALUE_ADDRESS (sym) = addr;
  return sym;
}

struct type *
java_lookup_class (char *name)
{
  struct symbol *sym;

  sym = lookup_symbol (name, expression_context_block, STRUCT_DOMAIN,
		       NULL).symbol;
  if (sym != NULL)
    return SYMBOL_TYPE (sym);
  /* FIXME - should search inferior's symbol table.  */
  return NULL;
}

/* Return a nul-terminated string (allocated on OBSTACK) for
   a name given by NAME (which has type Utf8Const*).  */

char *
get_java_utf8_name (struct obstack *obstack, struct value *name)
{
  char *chrs;
  struct value *temp = name;
  int name_length;
  CORE_ADDR data_addr;

  temp = value_struct_elt (&temp, NULL, "length", NULL, "structure");
  name_length = (int) value_as_long (temp);
  data_addr = value_address (temp) + TYPE_LENGTH (value_type (temp));
  chrs = (char *) obstack_alloc (obstack, name_length + 1);
  chrs[name_length] = '\0';
  read_memory (data_addr, (gdb_byte *) chrs, name_length);
  return chrs;
}

struct value *
java_class_from_object (struct value *obj_val)
{
  /* This is all rather inefficient, since the offsets of vtable and
     class are fixed.  FIXME */
  struct value *vtable_val;

  if (TYPE_CODE (value_type (obj_val)) == TYPE_CODE_PTR
      && TYPE_LENGTH (TYPE_TARGET_TYPE (value_type (obj_val))) == 0)
    obj_val = value_at (get_java_object_type (),
			value_as_address (obj_val));

  vtable_val = value_struct_elt (&obj_val, NULL, "vtable", NULL, "structure");
  return value_struct_elt (&vtable_val, NULL, "class", NULL, "structure");
}

/* Check if CLASS_IS_PRIMITIVE(value of clas): */
static int
java_class_is_primitive (struct value *clas)
{
  struct value *vtable = value_struct_elt (&clas, NULL, "vtable",
					   NULL, "struct");
  CORE_ADDR i = value_as_address (vtable);

  return (int) (i & 0x7fffffff) == (int) 0x7fffffff;
}

/* Read a GCJ Class object, and generated a gdb (TYPE_CODE_STRUCT) type.  */

struct type *
type_from_class (struct gdbarch *gdbarch, struct value *clas)
{
  struct type *type;
  char *name;
  struct value *temp;
  struct objfile *objfile;
  struct value *utf8_name;
  char *nptr;
  CORE_ADDR addr;

  type = check_typedef (value_type (clas));
  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    {
      if (value_logical_not (clas))
	return NULL;
      clas = value_ind (clas);
    }
  addr = value_address (clas);

  objfile = get_dynamics_objfile (gdbarch);
  if (java_class_is_primitive (clas))
    {
      struct value *sig;

      temp = clas;
      sig = value_struct_elt (&temp, NULL, "method_count", NULL, "structure");
      return java_primitive_type (gdbarch, value_as_long (sig));
    }

  /* Get Class name.  */
  /* If clasloader non-null, prepend loader address.  FIXME */
  temp = clas;
  utf8_name = value_struct_elt (&temp, NULL, "name", NULL, "structure");
  name = get_java_utf8_name (&objfile->objfile_obstack, utf8_name);
  for (nptr = name; *nptr != 0; nptr++)
    {
      if (*nptr == '/')
	*nptr = '.';
    }

  type = java_lookup_class (name);
  if (type != NULL)
    return type;

  type = alloc_type (objfile);
  TYPE_CODE (type) = TYPE_CODE_STRUCT;
  INIT_CPLUS_SPECIFIC (type);

  if (name[0] == '[')
    {
      char *signature = name;
      int namelen = java_demangled_signature_length (signature);

      if (namelen > strlen (name))
	name = (char *) obstack_alloc (&objfile->objfile_obstack, namelen + 1);
      java_demangled_signature_copy (name, signature);
      name[namelen] = '\0';
      temp = clas;
      /* Set array element type.  */
      temp = value_struct_elt (&temp, NULL, "methods", NULL, "structure");
      deprecated_set_value_type (temp,
				 lookup_pointer_type (value_type (clas)));
      TYPE_TARGET_TYPE (type) = type_from_class (gdbarch, temp);
    }

  ALLOCATE_CPLUS_STRUCT_TYPE (type);
  TYPE_TAG_NAME (type) = name;

  add_class_symtab_symbol (add_class_symbol (type, addr));
  return java_link_class_type (gdbarch, type, clas);
}

/* Fill in class TYPE with data from the CLAS value.  */

static struct type *
java_link_class_type (struct gdbarch *gdbarch,
		      struct type *type, struct value *clas)
{
  struct value *temp;
  const char *unqualified_name;
  const char *name = TYPE_TAG_NAME (type);
  int ninterfaces, nfields, nmethods;
  int type_is_object = 0;
  struct fn_field *fn_fields;
  struct fn_fieldlist *fn_fieldlists;
  struct value *fields;
  struct value *methods;
  struct value *method = NULL;
  struct value *field = NULL;
  int i, j;
  struct objfile *objfile = get_dynamics_objfile (gdbarch);
  struct type *tsuper;

  gdb_assert (name != NULL);
  unqualified_name = strrchr (name, '.');
  if (unqualified_name == NULL)
    unqualified_name = name;

  temp = clas;
  temp = value_struct_elt (&temp, NULL, "superclass", NULL, "structure");
  if (strcmp (name, "java.lang.Object") == 0)
    {
      tsuper = get_java_object_type ();
      if (tsuper && TYPE_CODE (tsuper) == TYPE_CODE_PTR)
	tsuper = TYPE_TARGET_TYPE (tsuper);
      type_is_object = 1;
    }
  else
    tsuper = type_from_class (gdbarch, temp);

#if 1
  ninterfaces = 0;
#else
  temp = clas;
  ninterfaces = value_as_long (value_struct_elt (&temp, NULL, "interface_len",
						 NULL, "structure"));
#endif
  TYPE_N_BASECLASSES (type) = (tsuper == NULL ? 0 : 1) + ninterfaces;
  temp = clas;
  nfields = value_as_long (value_struct_elt (&temp, NULL, "field_count",
					     NULL, "structure"));
  nfields += TYPE_N_BASECLASSES (type);
  nfields++;			/* Add one for dummy "class" field.  */
  TYPE_NFIELDS (type) = nfields;
  TYPE_FIELDS (type) = (struct field *)
    TYPE_ALLOC (type, sizeof (struct field) * nfields);

  memset (TYPE_FIELDS (type), 0, sizeof (struct field) * nfields);

  TYPE_FIELD_PRIVATE_BITS (type) =
    (B_TYPE *) TYPE_ALLOC (type, B_BYTES (nfields));
  B_CLRALL (TYPE_FIELD_PRIVATE_BITS (type), nfields);

  TYPE_FIELD_PROTECTED_BITS (type) =
    (B_TYPE *) TYPE_ALLOC (type, B_BYTES (nfields));
  B_CLRALL (TYPE_FIELD_PROTECTED_BITS (type), nfields);

  TYPE_FIELD_IGNORE_BITS (type) =
    (B_TYPE *) TYPE_ALLOC (type, B_BYTES (nfields));
  B_CLRALL (TYPE_FIELD_IGNORE_BITS (type), nfields);

  TYPE_FIELD_VIRTUAL_BITS (type) = (B_TYPE *)
    TYPE_ALLOC (type, B_BYTES (TYPE_N_BASECLASSES (type)));
  B_CLRALL (TYPE_FIELD_VIRTUAL_BITS (type), TYPE_N_BASECLASSES (type));

  if (tsuper != NULL)
    {
      TYPE_BASECLASS (type, 0) = tsuper;
      if (type_is_object)
	SET_TYPE_FIELD_PRIVATE (type, 0);
    }

  i = strlen (name);
  if (i > 2 && name[i - 1] == ']' && tsuper != NULL)
    {
      /* FIXME */
      TYPE_LENGTH (type) = TYPE_LENGTH (tsuper) + 4;   /* size with "length" */
    }
  else
    {
      temp = clas;
      temp = value_struct_elt (&temp, NULL, "size_in_bytes",
			       NULL, "structure");
      TYPE_LENGTH (type) = value_as_long (temp);
    }

  fields = NULL;
  nfields--;			/* First set up dummy "class" field.  */
  SET_FIELD_PHYSADDR (TYPE_FIELD (type, nfields), value_address (clas));
  TYPE_FIELD_NAME (type, nfields) = "class";
  TYPE_FIELD_TYPE (type, nfields) = value_type (clas);
  SET_TYPE_FIELD_PRIVATE (type, nfields);

  for (i = TYPE_N_BASECLASSES (type); i < nfields; i++)
    {
      int accflags;
      int boffset;

      if (fields == NULL)
	{
	  temp = clas;
	  fields = value_struct_elt (&temp, NULL, "fields", NULL, "structure");
	  field = value_ind (fields);
	}
      else
	{			/* Re-use field value for next field.  */
	  CORE_ADDR addr
	    = value_address (field) + TYPE_LENGTH (value_type (field));

	  set_value_address (field, addr);
	  set_value_lazy (field, 1);
	}
      temp = field;
      temp = value_struct_elt (&temp, NULL, "name", NULL, "structure");
      TYPE_FIELD_NAME (type, i) =
	get_java_utf8_name (&objfile->objfile_obstack, temp);
      temp = field;
      accflags = value_as_long (value_struct_elt (&temp, NULL, "accflags",
						  NULL, "structure"));
      temp = field;
      temp = value_struct_elt (&temp, NULL, "info", NULL, "structure");
      boffset = value_as_long (value_struct_elt (&temp, NULL, "boffset",
						 NULL, "structure"));
      if (accflags & 0x0001)	/* public access */
	{
	  /* ??? */
	}
      if (accflags & 0x0002)	/* private access */
	{
	  SET_TYPE_FIELD_PRIVATE (type, i);
	}
      if (accflags & 0x0004)	/* protected access */
	{
	  SET_TYPE_FIELD_PROTECTED (type, i);
	}
      if (accflags & 0x0008)	/* ACC_STATIC */
	SET_FIELD_PHYSADDR (TYPE_FIELD (type, i), boffset);
      else
	SET_FIELD_BITPOS (TYPE_FIELD (type, i), 8 * boffset);
      if (accflags & 0x8000)	/* FIELD_UNRESOLVED_FLAG */
	{
	  TYPE_FIELD_TYPE (type, i) = get_java_object_type ();	/* FIXME */
	}
      else
	{
	  struct type *ftype;

	  temp = field;
	  temp = value_struct_elt (&temp, NULL, "type", NULL, "structure");
	  ftype = type_from_class (gdbarch, temp);
	  if (TYPE_CODE (ftype) == TYPE_CODE_STRUCT)
	    ftype = lookup_pointer_type (ftype);
	  TYPE_FIELD_TYPE (type, i) = ftype;
	}
    }

  temp = clas;
  nmethods = value_as_long (value_struct_elt (&temp, NULL, "method_count",
					      NULL, "structure"));
  j = nmethods * sizeof (struct fn_field);
  fn_fields = (struct fn_field *)
    obstack_alloc (&objfile->objfile_obstack, j);
  memset (fn_fields, 0, j);
  fn_fieldlists = (struct fn_fieldlist *)
    alloca (nmethods * sizeof (struct fn_fieldlist));

  methods = NULL;
  for (i = 0; i < nmethods; i++)
    {
      const char *mname;
      int k;

      if (methods == NULL)
	{
	  temp = clas;
	  methods = value_struct_elt (&temp, NULL, "methods",
				      NULL, "structure");
	  method = value_ind (methods);
	}
      else
	{			/* Re-use method value for next method.  */
	  CORE_ADDR addr
	    = value_address (method) + TYPE_LENGTH (value_type (method));

	  set_value_address (method, addr);
	  set_value_lazy (method, 1);
	}

      /* Get method name.  */
      temp = method;
      temp = value_struct_elt (&temp, NULL, "name", NULL, "structure");
      mname = get_java_utf8_name (&objfile->objfile_obstack, temp);
      if (strcmp (mname, "<init>") == 0)
	mname = unqualified_name;

      /* Check for an existing method with the same name.
       * This makes building the fn_fieldslists an O(nmethods**2)
       * operation.  That could be using hashing, but I doubt it
       * is worth it.  Note that we do maintain the order of methods
       * in the inferior's Method table (as long as that is grouped
       * by method name), which I think is desirable.  --PB */
      for (k = 0, j = TYPE_NFN_FIELDS (type);;)
	{
	  if (--j < 0)
	    {			/* No match - new method name.  */
	      j = TYPE_NFN_FIELDS (type)++;
	      fn_fieldlists[j].name = mname;
	      fn_fieldlists[j].length = 1;
	      fn_fieldlists[j].fn_fields = &fn_fields[i];
	      k = i;
	      break;
	    }
	  if (strcmp (mname, fn_fieldlists[j].name) == 0)
	    {		/* Found an existing method with the same name.  */
	      int l;

	      if (mname != unqualified_name)
		obstack_free (&objfile->objfile_obstack, mname);
	      mname = fn_fieldlists[j].name;
	      fn_fieldlists[j].length++;
	      k = i - k;	/* Index of new slot.  */
	      /* Shift intervening fn_fields (between k and i) down.  */
	      for (l = i; l > k; l--)
		fn_fields[l] = fn_fields[l - 1];
	      for (l = TYPE_NFN_FIELDS (type); --l > j;)
		fn_fieldlists[l].fn_fields++;
	      break;
	    }
	  k += fn_fieldlists[j].length;
	}
      fn_fields[k].physname = "";
      fn_fields[k].is_stub = 1;
      /* FIXME */
      fn_fields[k].type = lookup_function_type
			   (builtin_java_type (gdbarch)->builtin_void);
      TYPE_CODE (fn_fields[k].type) = TYPE_CODE_METHOD;
    }

  j = TYPE_NFN_FIELDS (type) * sizeof (struct fn_fieldlist);
  TYPE_FN_FIELDLISTS (type) = (struct fn_fieldlist *)
    obstack_alloc (&objfile->objfile_obstack, j);
  memcpy (TYPE_FN_FIELDLISTS (type), fn_fieldlists, j);

  return type;
}

struct type *
get_java_object_type (void)
{
  struct symbol *sym;

  sym = lookup_symbol ("java.lang.Object", NULL, STRUCT_DOMAIN, NULL).symbol;
  if (sym == NULL)
    error (_("cannot find java.lang.Object"));
  return SYMBOL_TYPE (sym);
}

int
get_java_object_header_size (struct gdbarch *gdbarch)
{
  struct type *objtype = get_java_object_type ();

  if (objtype == NULL)
    return (2 * gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT);
  else
    return TYPE_LENGTH (objtype);
}

int
is_object_type (struct type *type)
{
  type = check_typedef (type);
  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    {
      struct type *ttype = check_typedef (TYPE_TARGET_TYPE (type));
      const char *name;
      if (TYPE_CODE (ttype) != TYPE_CODE_STRUCT)
	return 0;
      while (TYPE_N_BASECLASSES (ttype) > 0)
	ttype = TYPE_BASECLASS (ttype, 0);
      name = TYPE_TAG_NAME (ttype);
      if (name != NULL && strcmp (name, "java.lang.Object") == 0)
	return 1;
      name
	= TYPE_NFIELDS (ttype) > 0 ? TYPE_FIELD_NAME (ttype, 0) : (char *) 0;
      if (name != NULL && strcmp (name, "vtable") == 0)
	return 1;
    }
  return 0;
}

struct type *
java_primitive_type (struct gdbarch *gdbarch, int signature)
{
  const struct builtin_java_type *builtin = builtin_java_type (gdbarch);

  switch (signature)
    {
    case 'B':
      return builtin->builtin_byte;
    case 'S':
      return builtin->builtin_short;
    case 'I':
      return builtin->builtin_int;
    case 'J':
      return builtin->builtin_long;
    case 'Z':
      return builtin->builtin_boolean;
    case 'C':
      return builtin->builtin_char;
    case 'F':
      return builtin->builtin_float;
    case 'D':
      return builtin->builtin_double;
    case 'V':
      return builtin->builtin_void;
    }
  error (_("unknown signature '%c' for primitive type"), (char) signature);
}

/* If name[0 .. namelen-1] is the name of a primitive Java type,
   return that type.  Otherwise, return NULL.  */

struct type *
java_primitive_type_from_name (struct gdbarch *gdbarch,
			       const char *name, int namelen)
{
  const struct builtin_java_type *builtin = builtin_java_type (gdbarch);

  switch (name[0])
    {
    case 'b':
      if (namelen == 4 && memcmp (name, "byte", 4) == 0)
	return builtin->builtin_byte;
      if (namelen == 7 && memcmp (name, "boolean", 7) == 0)
	return builtin->builtin_boolean;
      break;
    case 'c':
      if (namelen == 4 && memcmp (name, "char", 4) == 0)
	return builtin->builtin_char;
      break;
    case 'd':
      if (namelen == 6 && memcmp (name, "double", 6) == 0)
	return builtin->builtin_double;
      break;
    case 'f':
      if (namelen == 5 && memcmp (name, "float", 5) == 0)
	return builtin->builtin_float;
      break;
    case 'i':
      if (namelen == 3 && memcmp (name, "int", 3) == 0)
	return builtin->builtin_int;
      break;
    case 'l':
      if (namelen == 4 && memcmp (name, "long", 4) == 0)
	return builtin->builtin_long;
      break;
    case 's':
      if (namelen == 5 && memcmp (name, "short", 5) == 0)
	return builtin->builtin_short;
      break;
    case 'v':
      if (namelen == 4 && memcmp (name, "void", 4) == 0)
	return builtin->builtin_void;
      break;
    }
  return NULL;
}

static char *
java_primitive_type_name (int signature)
{
  switch (signature)
    {
    case 'B':
      return "byte";
    case 'S':
      return "short";
    case 'I':
      return "int";
    case 'J':
      return "long";
    case 'Z':
      return "boolean";
    case 'C':
      return "char";
    case 'F':
      return "float";
    case 'D':
      return "double";
    case 'V':
      return "void";
    }
  error (_("unknown signature '%c' for primitive type"), (char) signature);
}

/* Return the length (in bytes) of demangled name of the Java type
   signature string SIGNATURE.  */

static int
java_demangled_signature_length (const char *signature)
{
  int array = 0;

  for (; *signature == '['; signature++)
    array += 2;			/* Two chars for "[]".  */
  switch (signature[0])
    {
    case 'L':
      /* Subtract 2 for 'L' and ';'.  */
      return strlen (signature) - 2 + array;
    default:
      return strlen (java_primitive_type_name (signature[0])) + array;
    }
}

/* Demangle the Java type signature SIGNATURE, leaving the result in
   RESULT.  */

static void
java_demangled_signature_copy (char *result, const char *signature)
{
  int array = 0;
  char *ptr;
  int i;

  while (*signature == '[')
    {
      array++;
      signature++;
    }
  switch (signature[0])
    {
    case 'L':
      /* Subtract 2 for 'L' and ';', but add 1 for final nul.  */
      signature++;
      ptr = result;
      for (; *signature != ';' && *signature != '\0'; signature++)
	{
	  if (*signature == '/')
	    *ptr++ = '.';
	  else
	    *ptr++ = *signature;
	}
      break;
    default:
      ptr = java_primitive_type_name (signature[0]);
      i = strlen (ptr);
      strcpy (result, ptr);
      ptr = result + i;
      break;
    }
  while (--array >= 0)
    {
      *ptr++ = '[';
      *ptr++ = ']';
    }
}

/* Return the demangled name of the Java type signature string SIGNATURE,
   as a freshly allocated copy.  */

char *
java_demangle_type_signature (const char *signature)
{
  int length = java_demangled_signature_length (signature);
  char *result = (char *) xmalloc (length + 1);

  java_demangled_signature_copy (result, signature);
  result[length] = '\0';
  return result;
}

/* Return the type of TYPE followed by DIMS pairs of [ ].
   If DIMS == 0, TYPE is returned.  */

struct type *
java_array_type (struct type *type, int dims)
{
  while (dims-- > 0)
    {
      /* FIXME  This is bogus!  Java arrays are not gdb arrays!  */
      type = lookup_array_range_type (type, 0, 0);
    }

  return type;
}

/* Create a Java string in the inferior from a (Utf8) literal.  */

static struct value *
java_value_string (char *ptr, int len)
{
  error (_("not implemented - java_value_string"));	/* FIXME */
}

/* Return the encoding that should be used for the character type
   TYPE.  */

static const char *
java_get_encoding (struct type *type)
{
  struct gdbarch *arch = get_type_arch (type);
  const char *encoding;

  if (type == builtin_java_type (arch)->builtin_char)
    {
      if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG)
	encoding = "UTF-16BE";
      else
	encoding = "UTF-16LE";
    }
  else
    encoding = target_charset (arch);

  return encoding;
}

/* Print the character C on STREAM as part of the contents of a literal
   string whose delimiter is QUOTER.  Note that that format for printing
   characters and strings is language specific.  */

static void
java_emit_char (int c, struct type *type, struct ui_file *stream, int quoter)
{
  const char *encoding = java_get_encoding (type);

  generic_emit_char (c, type, stream, quoter, encoding);
}

/* Implementation of la_printchar method.  */

static void
java_printchar (int c, struct type *type, struct ui_file *stream)
{
  fputs_filtered ("'", stream);
  LA_EMIT_CHAR (c, type, stream, '\'');
  fputs_filtered ("'", stream);
}

/* Implementation of la_printstr method.  */

static void
java_printstr (struct ui_file *stream, struct type *type,
	       const gdb_byte *string,
	       unsigned int length, const char *encoding, int force_ellipses,
	       const struct value_print_options *options)
{
  const char *type_encoding = java_get_encoding (type);

  if (!encoding || !*encoding)
    encoding = type_encoding;

  generic_printstr (stream, type, string, length, encoding,
		    force_ellipses, '"', 0, options);
}

static struct value *
evaluate_subexp_java (struct type *expect_type, struct expression *exp,
		      int *pos, enum noside noside)
{
  int pc = *pos;
  int i;
  const char *name;
  enum exp_opcode op = exp->elts[*pos].opcode;
  struct value *arg1;
  struct value *arg2;
  struct type *type;

  switch (op)
    {
    case UNOP_IND:
      if (noside == EVAL_SKIP)
	goto standard;
      (*pos)++;
      arg1 = evaluate_subexp_java (NULL_TYPE, exp, pos, EVAL_NORMAL);
      if (is_object_type (value_type (arg1)))
	{
	  struct type *type;

	  type = type_from_class (exp->gdbarch, java_class_from_object (arg1));
	  arg1 = value_cast (lookup_pointer_type (type), arg1);
	}
      return value_ind (arg1);

    case BINOP_SUBSCRIPT:
      (*pos)++;
      arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
      arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
      if (noside == EVAL_SKIP)
	goto nosideret;
      /* If the user attempts to subscript something that is not an
         array or pointer type (like a plain int variable for example),
         then report this as an error.  */

      arg1 = coerce_ref (arg1);
      type = check_typedef (value_type (arg1));
      if (TYPE_CODE (type) == TYPE_CODE_PTR)
	type = check_typedef (TYPE_TARGET_TYPE (type));
      name = TYPE_NAME (type);
      if (name == NULL)
	name = TYPE_TAG_NAME (type);
      i = name == NULL ? 0 : strlen (name);
      if (TYPE_CODE (type) == TYPE_CODE_STRUCT
	  && i > 2 && name[i - 1] == ']')
	{
	  enum bfd_endian byte_order = gdbarch_byte_order (exp->gdbarch);
	  CORE_ADDR address;
	  long length, index;
	  struct type *el_type;
	  gdb_byte buf4[4];

	  struct value *clas = java_class_from_object (arg1);
	  struct value *temp = clas;
	  /* Get CLASS_ELEMENT_TYPE of the array type.  */
	  temp = value_struct_elt (&temp, NULL, "methods",
				   NULL, "structure");
	  deprecated_set_value_type (temp, value_type (clas));
	  el_type = type_from_class (exp->gdbarch, temp);
	  if (TYPE_CODE (el_type) == TYPE_CODE_STRUCT)
	    el_type = lookup_pointer_type (el_type);

	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    return value_zero (el_type, VALUE_LVAL (arg1));
	  address = value_as_address (arg1);
	  address += get_java_object_header_size (exp->gdbarch);
	  read_memory (address, buf4, 4);
	  length = (long) extract_signed_integer (buf4, 4, byte_order);
	  index = (long) value_as_long (arg2);
	  if (index >= length || index < 0)
	    error (_("array index (%ld) out of bounds (length: %ld)"),
		   index, length);
	  address = (address + 4) + index * TYPE_LENGTH (el_type);
	  return value_at (el_type, address);
	}
      else if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
	{
	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    return value_zero (TYPE_TARGET_TYPE (type), VALUE_LVAL (arg1));
	  else
	    return value_subscript (arg1, value_as_long (arg2));
	}
      if (name)
	error (_("cannot subscript something of type `%s'"), name);
      else
	error (_("cannot subscript requested type"));

    case OP_STRING:
      (*pos)++;
      i = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (i + 1);
      if (noside == EVAL_SKIP)
	goto nosideret;
      return java_value_string (&exp->elts[pc + 2].string, i);

    case STRUCTOP_PTR:
      arg1 = evaluate_subexp_standard (expect_type, exp, pos, noside);
      /* Convert object field (such as TYPE.class) to reference.  */
      if (TYPE_CODE (value_type (arg1)) == TYPE_CODE_STRUCT)
	arg1 = value_addr (arg1);
      return arg1;
    default:
      break;
    }
standard:
  return evaluate_subexp_standard (expect_type, exp, pos, noside);
nosideret:
  return value_from_longest (builtin_type (exp->gdbarch)->builtin_int, 1);
}

static char *java_demangle (const char *mangled, int options)
{
  return gdb_demangle (mangled, options | DMGL_JAVA);
}

/* la_sniff_from_mangled_name for Java.  */

static int
java_sniff_from_mangled_name (const char *mangled, char **demangled)
{
  *demangled = java_demangle (mangled, DMGL_PARAMS | DMGL_ANSI);
  return *demangled != NULL;
}

/* Find the member function name of the demangled name NAME.  NAME
   must be a method name including arguments, in order to correctly
   locate the last component.

   This function return a pointer to the first dot before the
   member function name, or NULL if the name was not of the
   expected form.  */

static const char *
java_find_last_component (const char *name)
{
  const char *p;

  /* Find argument list.  */
  p = strchr (name, '(');

  if (p == NULL)
    return NULL;

  /* Back up and find first dot prior to argument list.  */
  while (p > name && *p != '.')
    p--;

  if (p == name)
    return NULL;

  return p;
}

/* Return the name of the class containing method PHYSNAME.  */

static char *
java_class_name_from_physname (const char *physname) 
{
  char *ret = NULL;
  const char *end;
  char *demangled_name = java_demangle (physname, DMGL_PARAMS | DMGL_ANSI);

  if (demangled_name == NULL)
    return NULL;

  end = java_find_last_component (demangled_name);
  if (end != NULL)
    {
      ret = (char *) xmalloc (end - demangled_name + 1);
      memcpy (ret, demangled_name, end - demangled_name);
      ret[end - demangled_name] = '\0';
    }

  xfree (demangled_name);
  return ret;
}

/* Table mapping opcodes into strings for printing operators
   and precedences of the operators.  */

const struct op_print java_op_print_tab[] =
{
  {",", BINOP_COMMA, PREC_COMMA, 0},
  {"=", BINOP_ASSIGN, PREC_ASSIGN, 1},
  {"||", BINOP_LOGICAL_OR, PREC_LOGICAL_OR, 0},
  {"&&", BINOP_LOGICAL_AND, PREC_LOGICAL_AND, 0},
  {"|", BINOP_BITWISE_IOR, PREC_BITWISE_IOR, 0},
  {"^", BINOP_BITWISE_XOR, PREC_BITWISE_XOR, 0},
  {"&", BINOP_BITWISE_AND, PREC_BITWISE_AND, 0},
  {"==", BINOP_EQUAL, PREC_EQUAL, 0},
  {"!=", BINOP_NOTEQUAL, PREC_EQUAL, 0},
  {"<=", BINOP_LEQ, PREC_ORDER, 0},
  {">=", BINOP_GEQ, PREC_ORDER, 0},
  {">", BINOP_GTR, PREC_ORDER, 0},
  {"<", BINOP_LESS, PREC_ORDER, 0},
  {">>", BINOP_RSH, PREC_SHIFT, 0},
  {"<<", BINOP_LSH, PREC_SHIFT, 0},
  {"+", BINOP_ADD, PREC_ADD, 0},
  {"-", BINOP_SUB, PREC_ADD, 0},
  {"*", BINOP_MUL, PREC_MUL, 0},
  {"/", BINOP_DIV, PREC_MUL, 0},
  {"%", BINOP_REM, PREC_MUL, 0},
  {"-", UNOP_NEG, PREC_PREFIX, 0},
  {"!", UNOP_LOGICAL_NOT, PREC_PREFIX, 0},
  {"~", UNOP_COMPLEMENT, PREC_PREFIX, 0},
  {"*", UNOP_IND, PREC_PREFIX, 0},
  {"++", UNOP_PREINCREMENT, PREC_PREFIX, 0},
  {"--", UNOP_PREDECREMENT, PREC_PREFIX, 0},
  {NULL, OP_NULL, PREC_PREFIX, 0}
};

enum java_primitive_types
{
  java_primitive_type_int,
  java_primitive_type_short,
  java_primitive_type_long,
  java_primitive_type_byte,
  java_primitive_type_boolean,
  java_primitive_type_char,
  java_primitive_type_float,
  java_primitive_type_double,
  java_primitive_type_void,
  nr_java_primitive_types
};

static void
java_language_arch_info (struct gdbarch *gdbarch,
			 struct language_arch_info *lai)
{
  const struct builtin_java_type *builtin = builtin_java_type (gdbarch);

  lai->string_char_type = builtin->builtin_char;
  lai->primitive_type_vector
    = GDBARCH_OBSTACK_CALLOC (gdbarch, nr_java_primitive_types + 1,
                              struct type *);
  lai->primitive_type_vector [java_primitive_type_int]
    = builtin->builtin_int;
  lai->primitive_type_vector [java_primitive_type_short]
    = builtin->builtin_short;
  lai->primitive_type_vector [java_primitive_type_long]
    = builtin->builtin_long;
  lai->primitive_type_vector [java_primitive_type_byte]
    = builtin->builtin_byte;
  lai->primitive_type_vector [java_primitive_type_boolean]
    = builtin->builtin_boolean;
  lai->primitive_type_vector [java_primitive_type_char]
    = builtin->builtin_char;
  lai->primitive_type_vector [java_primitive_type_float]
    = builtin->builtin_float;
  lai->primitive_type_vector [java_primitive_type_double]
    = builtin->builtin_double;
  lai->primitive_type_vector [java_primitive_type_void]
    = builtin->builtin_void;

  lai->bool_type_symbol = "boolean";
  lai->bool_type_default = builtin->builtin_boolean;
}

const struct exp_descriptor exp_descriptor_java = 
{
  print_subexp_standard,
  operator_length_standard,
  operator_check_standard,
  op_name_standard,
  dump_subexp_body_standard,
  evaluate_subexp_java
};

static const char *java_extensions[] =
{
  ".java", ".class", NULL
};

const struct language_defn java_language_defn =
{
  "java",			/* Language name */
  "Java",
  language_java,
  range_check_off,
  case_sensitive_on,
  array_row_major,
  macro_expansion_no,
  java_extensions,
  &exp_descriptor_java,
  java_parse,
  java_yyerror,
  null_post_parser,
  java_printchar,		/* Print a character constant */
  java_printstr,		/* Function to print string constant */
  java_emit_char,		/* Function to print a single character */
  java_print_type,		/* Print a type using appropriate syntax */
  default_print_typedef,	/* Print a typedef using appropriate syntax */
  java_val_print,		/* Print a value using appropriate syntax */
  java_value_print,		/* Print a top-level value */
  default_read_var_value,	/* la_read_var_value */
  NULL,				/* Language specific skip_trampoline */
  "this",	                /* name_of_this */
  basic_lookup_symbol_nonlocal,	/* lookup_symbol_nonlocal */
  basic_lookup_transparent_type,/* lookup_transparent_type */
  java_demangle,		/* Language specific symbol demangler */
  java_sniff_from_mangled_name,
  java_class_name_from_physname,/* Language specific class name */
  java_op_print_tab,		/* expression operators for printing */
  0,				/* not c-style arrays */
  0,				/* String lower bound */
  default_word_break_characters,
  default_make_symbol_completion_list,
  java_language_arch_info,
  default_print_array_index,
  default_pass_by_reference,
  default_get_string,
  NULL,				/* la_get_symbol_name_cmp */
  iterate_over_symbols,
  &java_varobj_ops,
  NULL,
  NULL,
  LANG_MAGIC
};

static void *
build_java_types (struct gdbarch *gdbarch)
{
  struct builtin_java_type *builtin_java_type
    = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct builtin_java_type);

  builtin_java_type->builtin_int
    = arch_integer_type (gdbarch, 32, 0, "int");
  builtin_java_type->builtin_short
    = arch_integer_type (gdbarch, 16, 0, "short");
  builtin_java_type->builtin_long
    = arch_integer_type (gdbarch, 64, 0, "long");
  builtin_java_type->builtin_byte
    = arch_integer_type (gdbarch, 8, 0, "byte");
  builtin_java_type->builtin_boolean
    = arch_boolean_type (gdbarch, 8, 0, "boolean");
  builtin_java_type->builtin_char
    = arch_character_type (gdbarch, 16, 1, "char");
  builtin_java_type->builtin_float
    = arch_float_type (gdbarch, 32, "float", NULL);
  builtin_java_type->builtin_double
    = arch_float_type (gdbarch, 64, "double", NULL);
  builtin_java_type->builtin_void
    = arch_type (gdbarch, TYPE_CODE_VOID, 1, "void");

  return builtin_java_type;
}

static struct gdbarch_data *java_type_data;

const struct builtin_java_type *
builtin_java_type (struct gdbarch *gdbarch)
{
  return ((const struct builtin_java_type *)
	  gdbarch_data (gdbarch, java_type_data));
}

void
_initialize_java_language (void)
{
  jv_dynamics_objfile_data_key
    = register_objfile_data_with_cleanup (NULL, jv_per_objfile_free);
  jv_dynamics_progspace_key = register_program_space_data ();

  java_type_data = gdbarch_data_register_post_init (build_java_types);

  add_language (&java_language_defn);
}
