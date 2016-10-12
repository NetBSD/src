/* Ada language support routines for GDB, the GNU debugger.

   Copyright (C) 1992-2016 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "demangle.h"
#include "gdb_regex.h"
#include "frame.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "expression.h"
#include "parser-defs.h"
#include "language.h"
#include "varobj.h"
#include "c-lang.h"
#include "inferior.h"
#include "symfile.h"
#include "objfiles.h"
#include "breakpoint.h"
#include "gdbcore.h"
#include "hashtab.h"
#include "gdb_obstack.h"
#include "ada-lang.h"
#include "completer.h"
#include <sys/stat.h>
#include "ui-out.h"
#include "block.h"
#include "infcall.h"
#include "dictionary.h"
#include "annotate.h"
#include "valprint.h"
#include "source.h"
#include "observer.h"
#include "vec.h"
#include "stack.h"
#include "gdb_vecs.h"
#include "typeprint.h"
#include "namespace.h"

#include "psymtab.h"
#include "value.h"
#include "mi/mi-common.h"
#include "arch-utils.h"
#include "cli/cli-utils.h"

/* Define whether or not the C operator '/' truncates towards zero for
   differently signed operands (truncation direction is undefined in C).
   Copied from valarith.c.  */

#ifndef TRUNCATION_TOWARDS_ZERO
#define TRUNCATION_TOWARDS_ZERO ((-5 / 2) == -2)
#endif

static struct type *desc_base_type (struct type *);

static struct type *desc_bounds_type (struct type *);

static struct value *desc_bounds (struct value *);

static int fat_pntr_bounds_bitpos (struct type *);

static int fat_pntr_bounds_bitsize (struct type *);

static struct type *desc_data_target_type (struct type *);

static struct value *desc_data (struct value *);

static int fat_pntr_data_bitpos (struct type *);

static int fat_pntr_data_bitsize (struct type *);

static struct value *desc_one_bound (struct value *, int, int);

static int desc_bound_bitpos (struct type *, int, int);

static int desc_bound_bitsize (struct type *, int, int);

static struct type *desc_index_type (struct type *, int);

static int desc_arity (struct type *);

static int ada_type_match (struct type *, struct type *, int);

static int ada_args_match (struct symbol *, struct value **, int);

static int full_match (const char *, const char *);

static struct value *make_array_descriptor (struct type *, struct value *);

static void ada_add_block_symbols (struct obstack *,
                                   const struct block *, const char *,
                                   domain_enum, struct objfile *, int);

static void ada_add_all_symbols (struct obstack *, const struct block *,
				 const char *, domain_enum, int, int *);

static int is_nonfunction (struct block_symbol *, int);

static void add_defn_to_vec (struct obstack *, struct symbol *,
                             const struct block *);

static int num_defns_collected (struct obstack *);

static struct block_symbol *defns_collected (struct obstack *, int);

static struct value *resolve_subexp (struct expression **, int *, int,
                                     struct type *);

static void replace_operator_with_call (struct expression **, int, int, int,
                                        struct symbol *, const struct block *);

static int possible_user_operator_p (enum exp_opcode, struct value **);

static char *ada_op_name (enum exp_opcode);

static const char *ada_decoded_op_name (enum exp_opcode);

static int numeric_type_p (struct type *);

static int integer_type_p (struct type *);

static int scalar_type_p (struct type *);

static int discrete_type_p (struct type *);

static enum ada_renaming_category parse_old_style_renaming (struct type *,
							    const char **,
							    int *,
							    const char **);

static struct symbol *find_old_style_renaming_symbol (const char *,
						      const struct block *);

static struct type *ada_lookup_struct_elt_type (struct type *, char *,
                                                int, int, int *);

static struct value *evaluate_subexp_type (struct expression *, int *);

static struct type *ada_find_parallel_type_with_name (struct type *,
                                                      const char *);

static int is_dynamic_field (struct type *, int);

static struct type *to_fixed_variant_branch_type (struct type *,
						  const gdb_byte *,
                                                  CORE_ADDR, struct value *);

static struct type *to_fixed_array_type (struct type *, struct value *, int);

static struct type *to_fixed_range_type (struct type *, struct value *);

static struct type *to_static_fixed_type (struct type *);
static struct type *static_unwrap_type (struct type *type);

static struct value *unwrap_value (struct value *);

static struct type *constrained_packed_array_type (struct type *, long *);

static struct type *decode_constrained_packed_array_type (struct type *);

static long decode_packed_array_bitsize (struct type *);

static struct value *decode_constrained_packed_array (struct value *);

static int ada_is_packed_array_type  (struct type *);

static int ada_is_unconstrained_packed_array_type (struct type *);

static struct value *value_subscript_packed (struct value *, int,
                                             struct value **);

static void move_bits (gdb_byte *, int, const gdb_byte *, int, int, int);

static struct value *coerce_unspec_val_to_type (struct value *,
                                                struct type *);

static struct value *get_var_value (char *, char *);

static int lesseq_defined_than (struct symbol *, struct symbol *);

static int equiv_types (struct type *, struct type *);

static int is_name_suffix (const char *);

static int advance_wild_match (const char **, const char *, int);

static int wild_match (const char *, const char *);

static struct value *ada_coerce_ref (struct value *);

static LONGEST pos_atr (struct value *);

static struct value *value_pos_atr (struct type *, struct value *);

static struct value *value_val_atr (struct type *, struct value *);

static struct symbol *standard_lookup (const char *, const struct block *,
                                       domain_enum);

static struct value *ada_search_struct_field (const char *, struct value *, int,
                                              struct type *);

static struct value *ada_value_primitive_field (struct value *, int, int,
                                                struct type *);

static int find_struct_field (const char *, struct type *, int,
                              struct type **, int *, int *, int *, int *);

static struct value *ada_to_fixed_value_create (struct type *, CORE_ADDR,
                                                struct value *);

static int ada_resolve_function (struct block_symbol *, int,
                                 struct value **, int, const char *,
                                 struct type *);

static int ada_is_direct_array_type (struct type *);

static void ada_language_arch_info (struct gdbarch *,
				    struct language_arch_info *);

static struct value *ada_index_struct_field (int, struct value *, int,
					     struct type *);

static struct value *assign_aggregate (struct value *, struct value *, 
				       struct expression *,
				       int *, enum noside);

static void aggregate_assign_from_choices (struct value *, struct value *, 
					   struct expression *,
					   int *, LONGEST *, int *,
					   int, LONGEST, LONGEST);

static void aggregate_assign_positional (struct value *, struct value *,
					 struct expression *,
					 int *, LONGEST *, int *, int,
					 LONGEST, LONGEST);


static void aggregate_assign_others (struct value *, struct value *,
				     struct expression *,
				     int *, LONGEST *, int, LONGEST, LONGEST);


static void add_component_interval (LONGEST, LONGEST, LONGEST *, int *, int);


static struct value *ada_evaluate_subexp (struct type *, struct expression *,
					  int *, enum noside);

static void ada_forward_operator_length (struct expression *, int, int *,
					 int *);

static struct type *ada_find_any_type (const char *name);


/* The result of a symbol lookup to be stored in our symbol cache.  */

struct cache_entry
{
  /* The name used to perform the lookup.  */
  const char *name;
  /* The namespace used during the lookup.  */
  domain_enum domain;
  /* The symbol returned by the lookup, or NULL if no matching symbol
     was found.  */
  struct symbol *sym;
  /* The block where the symbol was found, or NULL if no matching
     symbol was found.  */
  const struct block *block;
  /* A pointer to the next entry with the same hash.  */
  struct cache_entry *next;
};

/* The Ada symbol cache, used to store the result of Ada-mode symbol
   lookups in the course of executing the user's commands.

   The cache is implemented using a simple, fixed-sized hash.
   The size is fixed on the grounds that there are not likely to be
   all that many symbols looked up during any given session, regardless
   of the size of the symbol table.  If we decide to go to a resizable
   table, let's just use the stuff from libiberty instead.  */

#define HASH_SIZE 1009

struct ada_symbol_cache
{
  /* An obstack used to store the entries in our cache.  */
  struct obstack cache_space;

  /* The root of the hash table used to implement our symbol cache.  */
  struct cache_entry *root[HASH_SIZE];
};

static void ada_free_symbol_cache (struct ada_symbol_cache *sym_cache);

/* Maximum-sized dynamic type.  */
static unsigned int varsize_limit;

/* FIXME: brobecker/2003-09-17: No longer a const because it is
   returned by a function that does not return a const char *.  */
static char *ada_completer_word_break_characters =
#ifdef VMS
  " \t\n!@#%^&*()+=|~`}{[]\";:?/,-";
#else
  " \t\n!@#$%^&*()+=|~`}{[]\";:?/,-";
#endif

/* The name of the symbol to use to get the name of the main subprogram.  */
static const char ADA_MAIN_PROGRAM_SYMBOL_NAME[]
  = "__gnat_ada_main_program_name";

/* Limit on the number of warnings to raise per expression evaluation.  */
static int warning_limit = 2;

/* Number of warning messages issued; reset to 0 by cleanups after
   expression evaluation.  */
static int warnings_issued = 0;

static const char *known_runtime_file_name_patterns[] = {
  ADA_KNOWN_RUNTIME_FILE_NAME_PATTERNS NULL
};

static const char *known_auxiliary_function_name_patterns[] = {
  ADA_KNOWN_AUXILIARY_FUNCTION_NAME_PATTERNS NULL
};

/* Space for allocating results of ada_lookup_symbol_list.  */
static struct obstack symbol_list_obstack;

/* Maintenance-related settings for this module.  */

static struct cmd_list_element *maint_set_ada_cmdlist;
static struct cmd_list_element *maint_show_ada_cmdlist;

/* Implement the "maintenance set ada" (prefix) command.  */

static void
maint_set_ada_cmd (char *args, int from_tty)
{
  help_list (maint_set_ada_cmdlist, "maintenance set ada ", all_commands,
	     gdb_stdout);
}

/* Implement the "maintenance show ada" (prefix) command.  */

static void
maint_show_ada_cmd (char *args, int from_tty)
{
  cmd_show_list (maint_show_ada_cmdlist, from_tty, "");
}

/* The "maintenance ada set/show ignore-descriptive-type" value.  */

static int ada_ignore_descriptive_types_p = 0;

			/* Inferior-specific data.  */

/* Per-inferior data for this module.  */

struct ada_inferior_data
{
  /* The ada__tags__type_specific_data type, which is used when decoding
     tagged types.  With older versions of GNAT, this type was directly
     accessible through a component ("tsd") in the object tag.  But this
     is no longer the case, so we cache it for each inferior.  */
  struct type *tsd_type;

  /* The exception_support_info data.  This data is used to determine
     how to implement support for Ada exception catchpoints in a given
     inferior.  */
  const struct exception_support_info *exception_info;
};

/* Our key to this module's inferior data.  */
static const struct inferior_data *ada_inferior_data;

/* A cleanup routine for our inferior data.  */
static void
ada_inferior_data_cleanup (struct inferior *inf, void *arg)
{
  struct ada_inferior_data *data;

  data = (struct ada_inferior_data *) inferior_data (inf, ada_inferior_data);
  if (data != NULL)
    xfree (data);
}

/* Return our inferior data for the given inferior (INF).

   This function always returns a valid pointer to an allocated
   ada_inferior_data structure.  If INF's inferior data has not
   been previously set, this functions creates a new one with all
   fields set to zero, sets INF's inferior to it, and then returns
   a pointer to that newly allocated ada_inferior_data.  */

static struct ada_inferior_data *
get_ada_inferior_data (struct inferior *inf)
{
  struct ada_inferior_data *data;

  data = (struct ada_inferior_data *) inferior_data (inf, ada_inferior_data);
  if (data == NULL)
    {
      data = XCNEW (struct ada_inferior_data);
      set_inferior_data (inf, ada_inferior_data, data);
    }

  return data;
}

/* Perform all necessary cleanups regarding our module's inferior data
   that is required after the inferior INF just exited.  */

static void
ada_inferior_exit (struct inferior *inf)
{
  ada_inferior_data_cleanup (inf, NULL);
  set_inferior_data (inf, ada_inferior_data, NULL);
}


			/* program-space-specific data.  */

/* This module's per-program-space data.  */
struct ada_pspace_data
{
  /* The Ada symbol cache.  */
  struct ada_symbol_cache *sym_cache;
};

/* Key to our per-program-space data.  */
static const struct program_space_data *ada_pspace_data_handle;

/* Return this module's data for the given program space (PSPACE).
   If not is found, add a zero'ed one now.

   This function always returns a valid object.  */

static struct ada_pspace_data *
get_ada_pspace_data (struct program_space *pspace)
{
  struct ada_pspace_data *data;

  data = ((struct ada_pspace_data *)
	  program_space_data (pspace, ada_pspace_data_handle));
  if (data == NULL)
    {
      data = XCNEW (struct ada_pspace_data);
      set_program_space_data (pspace, ada_pspace_data_handle, data);
    }

  return data;
}

/* The cleanup callback for this module's per-program-space data.  */

static void
ada_pspace_data_cleanup (struct program_space *pspace, void *data)
{
  struct ada_pspace_data *pspace_data = (struct ada_pspace_data *) data;

  if (pspace_data->sym_cache != NULL)
    ada_free_symbol_cache (pspace_data->sym_cache);
  xfree (pspace_data);
}

                        /* Utilities */

/* If TYPE is a TYPE_CODE_TYPEDEF type, return the target type after
   all typedef layers have been peeled.  Otherwise, return TYPE.

   Normally, we really expect a typedef type to only have 1 typedef layer.
   In other words, we really expect the target type of a typedef type to be
   a non-typedef type.  This is particularly true for Ada units, because
   the language does not have a typedef vs not-typedef distinction.
   In that respect, the Ada compiler has been trying to eliminate as many
   typedef definitions in the debugging information, since they generally
   do not bring any extra information (we still use typedef under certain
   circumstances related mostly to the GNAT encoding).

   Unfortunately, we have seen situations where the debugging information
   generated by the compiler leads to such multiple typedef layers.  For
   instance, consider the following example with stabs:

     .stabs  "pck__float_array___XUP:Tt(0,46)=s16P_ARRAY:(0,47)=[...]"[...]
     .stabs  "pck__float_array___XUP:t(0,36)=(0,46)",128,0,6,0

   This is an error in the debugging information which causes type
   pck__float_array___XUP to be defined twice, and the second time,
   it is defined as a typedef of a typedef.

   This is on the fringe of legality as far as debugging information is
   concerned, and certainly unexpected.  But it is easy to handle these
   situations correctly, so we can afford to be lenient in this case.  */

static struct type *
ada_typedef_target_type (struct type *type)
{
  while (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
    type = TYPE_TARGET_TYPE (type);
  return type;
}

/* Given DECODED_NAME a string holding a symbol name in its
   decoded form (ie using the Ada dotted notation), returns
   its unqualified name.  */

static const char *
ada_unqualified_name (const char *decoded_name)
{
  const char *result;
  
  /* If the decoded name starts with '<', it means that the encoded
     name does not follow standard naming conventions, and thus that
     it is not your typical Ada symbol name.  Trying to unqualify it
     is therefore pointless and possibly erroneous.  */
  if (decoded_name[0] == '<')
    return decoded_name;

  result = strrchr (decoded_name, '.');
  if (result != NULL)
    result++;                   /* Skip the dot...  */
  else
    result = decoded_name;

  return result;
}

/* Return a string starting with '<', followed by STR, and '>'.
   The result is good until the next call.  */

static char *
add_angle_brackets (const char *str)
{
  static char *result = NULL;

  xfree (result);
  result = xstrprintf ("<%s>", str);
  return result;
}

static char *
ada_get_gdb_completer_word_break_characters (void)
{
  return ada_completer_word_break_characters;
}

/* Print an array element index using the Ada syntax.  */

static void
ada_print_array_index (struct value *index_value, struct ui_file *stream,
                       const struct value_print_options *options)
{
  LA_VALUE_PRINT (index_value, stream, options);
  fprintf_filtered (stream, " => ");
}

/* Assuming VECT points to an array of *SIZE objects of size
   ELEMENT_SIZE, grow it to contain at least MIN_SIZE objects,
   updating *SIZE as necessary and returning the (new) array.  */

void *
grow_vect (void *vect, size_t *size, size_t min_size, int element_size)
{
  if (*size < min_size)
    {
      *size *= 2;
      if (*size < min_size)
        *size = min_size;
      vect = xrealloc (vect, *size * element_size);
    }
  return vect;
}

/* True (non-zero) iff TARGET matches FIELD_NAME up to any trailing
   suffix of FIELD_NAME beginning "___".  */

static int
field_name_match (const char *field_name, const char *target)
{
  int len = strlen (target);

  return
    (strncmp (field_name, target, len) == 0
     && (field_name[len] == '\0'
         || (startswith (field_name + len, "___")
             && strcmp (field_name + strlen (field_name) - 6,
                        "___XVN") != 0)));
}


/* Assuming TYPE is a TYPE_CODE_STRUCT or a TYPE_CODE_TYPDEF to
   a TYPE_CODE_STRUCT, find the field whose name matches FIELD_NAME,
   and return its index.  This function also handles fields whose name
   have ___ suffixes because the compiler sometimes alters their name
   by adding such a suffix to represent fields with certain constraints.
   If the field could not be found, return a negative number if
   MAYBE_MISSING is set.  Otherwise raise an error.  */

int
ada_get_field_index (const struct type *type, const char *field_name,
                     int maybe_missing)
{
  int fieldno;
  struct type *struct_type = check_typedef ((struct type *) type);

  for (fieldno = 0; fieldno < TYPE_NFIELDS (struct_type); fieldno++)
    if (field_name_match (TYPE_FIELD_NAME (struct_type, fieldno), field_name))
      return fieldno;

  if (!maybe_missing)
    error (_("Unable to find field %s in struct %s.  Aborting"),
           field_name, TYPE_NAME (struct_type));

  return -1;
}

/* The length of the prefix of NAME prior to any "___" suffix.  */

int
ada_name_prefix_len (const char *name)
{
  if (name == NULL)
    return 0;
  else
    {
      const char *p = strstr (name, "___");

      if (p == NULL)
        return strlen (name);
      else
        return p - name;
    }
}

/* Return non-zero if SUFFIX is a suffix of STR.
   Return zero if STR is null.  */

static int
is_suffix (const char *str, const char *suffix)
{
  int len1, len2;

  if (str == NULL)
    return 0;
  len1 = strlen (str);
  len2 = strlen (suffix);
  return (len1 >= len2 && strcmp (str + len1 - len2, suffix) == 0);
}

/* The contents of value VAL, treated as a value of type TYPE.  The
   result is an lval in memory if VAL is.  */

static struct value *
coerce_unspec_val_to_type (struct value *val, struct type *type)
{
  type = ada_check_typedef (type);
  if (value_type (val) == type)
    return val;
  else
    {
      struct value *result;

      /* Make sure that the object size is not unreasonable before
         trying to allocate some memory for it.  */
      ada_ensure_varsize_limit (type);

      if (value_lazy (val)
          || TYPE_LENGTH (type) > TYPE_LENGTH (value_type (val)))
	result = allocate_value_lazy (type);
      else
	{
	  result = allocate_value (type);
	  value_contents_copy_raw (result, 0, val, 0, TYPE_LENGTH (type));
	}
      set_value_component_location (result, val);
      set_value_bitsize (result, value_bitsize (val));
      set_value_bitpos (result, value_bitpos (val));
      set_value_address (result, value_address (val));
      return result;
    }
}

static const gdb_byte *
cond_offset_host (const gdb_byte *valaddr, long offset)
{
  if (valaddr == NULL)
    return NULL;
  else
    return valaddr + offset;
}

static CORE_ADDR
cond_offset_target (CORE_ADDR address, long offset)
{
  if (address == 0)
    return 0;
  else
    return address + offset;
}

/* Issue a warning (as for the definition of warning in utils.c, but
   with exactly one argument rather than ...), unless the limit on the
   number of warnings has passed during the evaluation of the current
   expression.  */

/* FIXME: cagney/2004-10-10: This function is mimicking the behavior
   provided by "complaint".  */
static void lim_warning (const char *format, ...) ATTRIBUTE_PRINTF (1, 2);

static void
lim_warning (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  warnings_issued += 1;
  if (warnings_issued <= warning_limit)
    vwarning (format, args);

  va_end (args);
}

/* Issue an error if the size of an object of type T is unreasonable,
   i.e. if it would be a bad idea to allocate a value of this type in
   GDB.  */

void
ada_ensure_varsize_limit (const struct type *type)
{
  if (TYPE_LENGTH (type) > varsize_limit)
    error (_("object size is larger than varsize-limit"));
}

/* Maximum value of a SIZE-byte signed integer type.  */
static LONGEST
max_of_size (int size)
{
  LONGEST top_bit = (LONGEST) 1 << (size * 8 - 2);

  return top_bit | (top_bit - 1);
}

/* Minimum value of a SIZE-byte signed integer type.  */
static LONGEST
min_of_size (int size)
{
  return -max_of_size (size) - 1;
}

/* Maximum value of a SIZE-byte unsigned integer type.  */
static ULONGEST
umax_of_size (int size)
{
  ULONGEST top_bit = (ULONGEST) 1 << (size * 8 - 1);

  return top_bit | (top_bit - 1);
}

/* Maximum value of integral type T, as a signed quantity.  */
static LONGEST
max_of_type (struct type *t)
{
  if (TYPE_UNSIGNED (t))
    return (LONGEST) umax_of_size (TYPE_LENGTH (t));
  else
    return max_of_size (TYPE_LENGTH (t));
}

/* Minimum value of integral type T, as a signed quantity.  */
static LONGEST
min_of_type (struct type *t)
{
  if (TYPE_UNSIGNED (t)) 
    return 0;
  else
    return min_of_size (TYPE_LENGTH (t));
}

/* The largest value in the domain of TYPE, a discrete type, as an integer.  */
LONGEST
ada_discrete_type_high_bound (struct type *type)
{
  type = resolve_dynamic_type (type, NULL, 0);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_RANGE:
      return TYPE_HIGH_BOUND (type);
    case TYPE_CODE_ENUM:
      return TYPE_FIELD_ENUMVAL (type, TYPE_NFIELDS (type) - 1);
    case TYPE_CODE_BOOL:
      return 1;
    case TYPE_CODE_CHAR:
    case TYPE_CODE_INT:
      return max_of_type (type);
    default:
      error (_("Unexpected type in ada_discrete_type_high_bound."));
    }
}

/* The smallest value in the domain of TYPE, a discrete type, as an integer.  */
LONGEST
ada_discrete_type_low_bound (struct type *type)
{
  type = resolve_dynamic_type (type, NULL, 0);
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_RANGE:
      return TYPE_LOW_BOUND (type);
    case TYPE_CODE_ENUM:
      return TYPE_FIELD_ENUMVAL (type, 0);
    case TYPE_CODE_BOOL:
      return 0;
    case TYPE_CODE_CHAR:
    case TYPE_CODE_INT:
      return min_of_type (type);
    default:
      error (_("Unexpected type in ada_discrete_type_low_bound."));
    }
}

/* The identity on non-range types.  For range types, the underlying
   non-range scalar type.  */

static struct type *
get_base_type (struct type *type)
{
  while (type != NULL && TYPE_CODE (type) == TYPE_CODE_RANGE)
    {
      if (type == TYPE_TARGET_TYPE (type) || TYPE_TARGET_TYPE (type) == NULL)
        return type;
      type = TYPE_TARGET_TYPE (type);
    }
  return type;
}

/* Return a decoded version of the given VALUE.  This means returning
   a value whose type is obtained by applying all the GNAT-specific
   encondings, making the resulting type a static but standard description
   of the initial type.  */

struct value *
ada_get_decoded_value (struct value *value)
{
  struct type *type = ada_check_typedef (value_type (value));

  if (ada_is_array_descriptor_type (type)
      || (ada_is_constrained_packed_array_type (type)
          && TYPE_CODE (type) != TYPE_CODE_PTR))
    {
      if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)  /* array access type.  */
        value = ada_coerce_to_simple_array_ptr (value);
      else
        value = ada_coerce_to_simple_array (value);
    }
  else
    value = ada_to_fixed_value (value);

  return value;
}

/* Same as ada_get_decoded_value, but with the given TYPE.
   Because there is no associated actual value for this type,
   the resulting type might be a best-effort approximation in
   the case of dynamic types.  */

struct type *
ada_get_decoded_type (struct type *type)
{
  type = to_static_fixed_type (type);
  if (ada_is_constrained_packed_array_type (type))
    type = ada_coerce_to_simple_array_type (type);
  return type;
}



                                /* Language Selection */

/* If the main program is in Ada, return language_ada, otherwise return LANG
   (the main program is in Ada iif the adainit symbol is found).  */

enum language
ada_update_initial_language (enum language lang)
{
  if (lookup_minimal_symbol ("adainit", (const char *) NULL,
                             (struct objfile *) NULL).minsym != NULL)
    return language_ada;

  return lang;
}

/* If the main procedure is written in Ada, then return its name.
   The result is good until the next call.  Return NULL if the main
   procedure doesn't appear to be in Ada.  */

char *
ada_main_name (void)
{
  struct bound_minimal_symbol msym;
  static char *main_program_name = NULL;

  /* For Ada, the name of the main procedure is stored in a specific
     string constant, generated by the binder.  Look for that symbol,
     extract its address, and then read that string.  If we didn't find
     that string, then most probably the main procedure is not written
     in Ada.  */
  msym = lookup_minimal_symbol (ADA_MAIN_PROGRAM_SYMBOL_NAME, NULL, NULL);

  if (msym.minsym != NULL)
    {
      CORE_ADDR main_program_name_addr;
      int err_code;

      main_program_name_addr = BMSYMBOL_VALUE_ADDRESS (msym);
      if (main_program_name_addr == 0)
        error (_("Invalid address for Ada main program name."));

      xfree (main_program_name);
      target_read_string (main_program_name_addr, &main_program_name,
                          1024, &err_code);

      if (err_code != 0)
        return NULL;
      return main_program_name;
    }

  /* The main procedure doesn't seem to be in Ada.  */
  return NULL;
}

                                /* Symbols */

/* Table of Ada operators and their GNAT-encoded names.  Last entry is pair
   of NULLs.  */

const struct ada_opname_map ada_opname_table[] = {
  {"Oadd", "\"+\"", BINOP_ADD},
  {"Osubtract", "\"-\"", BINOP_SUB},
  {"Omultiply", "\"*\"", BINOP_MUL},
  {"Odivide", "\"/\"", BINOP_DIV},
  {"Omod", "\"mod\"", BINOP_MOD},
  {"Orem", "\"rem\"", BINOP_REM},
  {"Oexpon", "\"**\"", BINOP_EXP},
  {"Olt", "\"<\"", BINOP_LESS},
  {"Ole", "\"<=\"", BINOP_LEQ},
  {"Ogt", "\">\"", BINOP_GTR},
  {"Oge", "\">=\"", BINOP_GEQ},
  {"Oeq", "\"=\"", BINOP_EQUAL},
  {"One", "\"/=\"", BINOP_NOTEQUAL},
  {"Oand", "\"and\"", BINOP_BITWISE_AND},
  {"Oor", "\"or\"", BINOP_BITWISE_IOR},
  {"Oxor", "\"xor\"", BINOP_BITWISE_XOR},
  {"Oconcat", "\"&\"", BINOP_CONCAT},
  {"Oabs", "\"abs\"", UNOP_ABS},
  {"Onot", "\"not\"", UNOP_LOGICAL_NOT},
  {"Oadd", "\"+\"", UNOP_PLUS},
  {"Osubtract", "\"-\"", UNOP_NEG},
  {NULL, NULL}
};

/* The "encoded" form of DECODED, according to GNAT conventions.
   The result is valid until the next call to ada_encode.  */

char *
ada_encode (const char *decoded)
{
  static char *encoding_buffer = NULL;
  static size_t encoding_buffer_size = 0;
  const char *p;
  int k;

  if (decoded == NULL)
    return NULL;

  GROW_VECT (encoding_buffer, encoding_buffer_size,
             2 * strlen (decoded) + 10);

  k = 0;
  for (p = decoded; *p != '\0'; p += 1)
    {
      if (*p == '.')
        {
          encoding_buffer[k] = encoding_buffer[k + 1] = '_';
          k += 2;
        }
      else if (*p == '"')
        {
          const struct ada_opname_map *mapping;

          for (mapping = ada_opname_table;
               mapping->encoded != NULL
               && !startswith (p, mapping->decoded); mapping += 1)
            ;
          if (mapping->encoded == NULL)
            error (_("invalid Ada operator name: %s"), p);
          strcpy (encoding_buffer + k, mapping->encoded);
          k += strlen (mapping->encoded);
          break;
        }
      else
        {
          encoding_buffer[k] = *p;
          k += 1;
        }
    }

  encoding_buffer[k] = '\0';
  return encoding_buffer;
}

/* Return NAME folded to lower case, or, if surrounded by single
   quotes, unfolded, but with the quotes stripped away.  Result good
   to next call.  */

char *
ada_fold_name (const char *name)
{
  static char *fold_buffer = NULL;
  static size_t fold_buffer_size = 0;

  int len = strlen (name);
  GROW_VECT (fold_buffer, fold_buffer_size, len + 1);

  if (name[0] == '\'')
    {
      strncpy (fold_buffer, name + 1, len - 2);
      fold_buffer[len - 2] = '\000';
    }
  else
    {
      int i;

      for (i = 0; i <= len; i += 1)
        fold_buffer[i] = tolower (name[i]);
    }

  return fold_buffer;
}

/* Return nonzero if C is either a digit or a lowercase alphabet character.  */

static int
is_lower_alphanum (const char c)
{
  return (isdigit (c) || (isalpha (c) && islower (c)));
}

/* ENCODED is the linkage name of a symbol and LEN contains its length.
   This function saves in LEN the length of that same symbol name but
   without either of these suffixes:
     . .{DIGIT}+
     . ${DIGIT}+
     . ___{DIGIT}+
     . __{DIGIT}+.

   These are suffixes introduced by the compiler for entities such as
   nested subprogram for instance, in order to avoid name clashes.
   They do not serve any purpose for the debugger.  */

static void
ada_remove_trailing_digits (const char *encoded, int *len)
{
  if (*len > 1 && isdigit (encoded[*len - 1]))
    {
      int i = *len - 2;

      while (i > 0 && isdigit (encoded[i]))
        i--;
      if (i >= 0 && encoded[i] == '.')
        *len = i;
      else if (i >= 0 && encoded[i] == '$')
        *len = i;
      else if (i >= 2 && startswith (encoded + i - 2, "___"))
        *len = i - 2;
      else if (i >= 1 && startswith (encoded + i - 1, "__"))
        *len = i - 1;
    }
}

/* Remove the suffix introduced by the compiler for protected object
   subprograms.  */

static void
ada_remove_po_subprogram_suffix (const char *encoded, int *len)
{
  /* Remove trailing N.  */

  /* Protected entry subprograms are broken into two
     separate subprograms: The first one is unprotected, and has
     a 'N' suffix; the second is the protected version, and has
     the 'P' suffix.  The second calls the first one after handling
     the protection.  Since the P subprograms are internally generated,
     we leave these names undecoded, giving the user a clue that this
     entity is internal.  */

  if (*len > 1
      && encoded[*len - 1] == 'N'
      && (isdigit (encoded[*len - 2]) || islower (encoded[*len - 2])))
    *len = *len - 1;
}

/* Remove trailing X[bn]* suffixes (indicating names in package bodies).  */

static void
ada_remove_Xbn_suffix (const char *encoded, int *len)
{
  int i = *len - 1;

  while (i > 0 && (encoded[i] == 'b' || encoded[i] == 'n'))
    i--;

  if (encoded[i] != 'X')
    return;

  if (i == 0)
    return;

  if (isalnum (encoded[i-1]))
    *len = i;
}

/* If ENCODED follows the GNAT entity encoding conventions, then return
   the decoded form of ENCODED.  Otherwise, return "<%s>" where "%s" is
   replaced by ENCODED.

   The resulting string is valid until the next call of ada_decode.
   If the string is unchanged by decoding, the original string pointer
   is returned.  */

const char *
ada_decode (const char *encoded)
{
  int i, j;
  int len0;
  const char *p;
  char *decoded;
  int at_start_name;
  static char *decoding_buffer = NULL;
  static size_t decoding_buffer_size = 0;

  /* The name of the Ada main procedure starts with "_ada_".
     This prefix is not part of the decoded name, so skip this part
     if we see this prefix.  */
  if (startswith (encoded, "_ada_"))
    encoded += 5;

  /* If the name starts with '_', then it is not a properly encoded
     name, so do not attempt to decode it.  Similarly, if the name
     starts with '<', the name should not be decoded.  */
  if (encoded[0] == '_' || encoded[0] == '<')
    goto Suppress;

  len0 = strlen (encoded);

  ada_remove_trailing_digits (encoded, &len0);
  ada_remove_po_subprogram_suffix (encoded, &len0);

  /* Remove the ___X.* suffix if present.  Do not forget to verify that
     the suffix is located before the current "end" of ENCODED.  We want
     to avoid re-matching parts of ENCODED that have previously been
     marked as discarded (by decrementing LEN0).  */
  p = strstr (encoded, "___");
  if (p != NULL && p - encoded < len0 - 3)
    {
      if (p[3] == 'X')
        len0 = p - encoded;
      else
        goto Suppress;
    }

  /* Remove any trailing TKB suffix.  It tells us that this symbol
     is for the body of a task, but that information does not actually
     appear in the decoded name.  */

  if (len0 > 3 && startswith (encoded + len0 - 3, "TKB"))
    len0 -= 3;

  /* Remove any trailing TB suffix.  The TB suffix is slightly different
     from the TKB suffix because it is used for non-anonymous task
     bodies.  */

  if (len0 > 2 && startswith (encoded + len0 - 2, "TB"))
    len0 -= 2;

  /* Remove trailing "B" suffixes.  */
  /* FIXME: brobecker/2006-04-19: Not sure what this are used for...  */

  if (len0 > 1 && startswith (encoded + len0 - 1, "B"))
    len0 -= 1;

  /* Make decoded big enough for possible expansion by operator name.  */

  GROW_VECT (decoding_buffer, decoding_buffer_size, 2 * len0 + 1);
  decoded = decoding_buffer;

  /* Remove trailing __{digit}+ or trailing ${digit}+.  */

  if (len0 > 1 && isdigit (encoded[len0 - 1]))
    {
      i = len0 - 2;
      while ((i >= 0 && isdigit (encoded[i]))
             || (i >= 1 && encoded[i] == '_' && isdigit (encoded[i - 1])))
        i -= 1;
      if (i > 1 && encoded[i] == '_' && encoded[i - 1] == '_')
        len0 = i - 1;
      else if (encoded[i] == '$')
        len0 = i;
    }

  /* The first few characters that are not alphabetic are not part
     of any encoding we use, so we can copy them over verbatim.  */

  for (i = 0, j = 0; i < len0 && !isalpha (encoded[i]); i += 1, j += 1)
    decoded[j] = encoded[i];

  at_start_name = 1;
  while (i < len0)
    {
      /* Is this a symbol function?  */
      if (at_start_name && encoded[i] == 'O')
        {
          int k;

          for (k = 0; ada_opname_table[k].encoded != NULL; k += 1)
            {
              int op_len = strlen (ada_opname_table[k].encoded);
              if ((strncmp (ada_opname_table[k].encoded + 1, encoded + i + 1,
                            op_len - 1) == 0)
                  && !isalnum (encoded[i + op_len]))
                {
                  strcpy (decoded + j, ada_opname_table[k].decoded);
                  at_start_name = 0;
                  i += op_len;
                  j += strlen (ada_opname_table[k].decoded);
                  break;
                }
            }
          if (ada_opname_table[k].encoded != NULL)
            continue;
        }
      at_start_name = 0;

      /* Replace "TK__" with "__", which will eventually be translated
         into "." (just below).  */

      if (i < len0 - 4 && startswith (encoded + i, "TK__"))
        i += 2;

      /* Replace "__B_{DIGITS}+__" sequences by "__", which will eventually
         be translated into "." (just below).  These are internal names
         generated for anonymous blocks inside which our symbol is nested.  */

      if (len0 - i > 5 && encoded [i] == '_' && encoded [i+1] == '_'
          && encoded [i+2] == 'B' && encoded [i+3] == '_'
          && isdigit (encoded [i+4]))
        {
          int k = i + 5;
          
          while (k < len0 && isdigit (encoded[k]))
            k++;  /* Skip any extra digit.  */

          /* Double-check that the "__B_{DIGITS}+" sequence we found
             is indeed followed by "__".  */
          if (len0 - k > 2 && encoded [k] == '_' && encoded [k+1] == '_')
            i = k;
        }

      /* Remove _E{DIGITS}+[sb] */

      /* Just as for protected object subprograms, there are 2 categories
         of subprograms created by the compiler for each entry.  The first
         one implements the actual entry code, and has a suffix following
         the convention above; the second one implements the barrier and
         uses the same convention as above, except that the 'E' is replaced
         by a 'B'.

         Just as above, we do not decode the name of barrier functions
         to give the user a clue that the code he is debugging has been
         internally generated.  */

      if (len0 - i > 3 && encoded [i] == '_' && encoded[i+1] == 'E'
          && isdigit (encoded[i+2]))
        {
          int k = i + 3;

          while (k < len0 && isdigit (encoded[k]))
            k++;

          if (k < len0
              && (encoded[k] == 'b' || encoded[k] == 's'))
            {
              k++;
              /* Just as an extra precaution, make sure that if this
                 suffix is followed by anything else, it is a '_'.
                 Otherwise, we matched this sequence by accident.  */
              if (k == len0
                  || (k < len0 && encoded[k] == '_'))
                i = k;
            }
        }

      /* Remove trailing "N" in [a-z0-9]+N__.  The N is added by
         the GNAT front-end in protected object subprograms.  */

      if (i < len0 + 3
          && encoded[i] == 'N' && encoded[i+1] == '_' && encoded[i+2] == '_')
        {
          /* Backtrack a bit up until we reach either the begining of
             the encoded name, or "__".  Make sure that we only find
             digits or lowercase characters.  */
          const char *ptr = encoded + i - 1;

          while (ptr >= encoded && is_lower_alphanum (ptr[0]))
            ptr--;
          if (ptr < encoded
              || (ptr > encoded && ptr[0] == '_' && ptr[-1] == '_'))
            i++;
        }

      if (encoded[i] == 'X' && i != 0 && isalnum (encoded[i - 1]))
        {
          /* This is a X[bn]* sequence not separated from the previous
             part of the name with a non-alpha-numeric character (in other
             words, immediately following an alpha-numeric character), then
             verify that it is placed at the end of the encoded name.  If
             not, then the encoding is not valid and we should abort the
             decoding.  Otherwise, just skip it, it is used in body-nested
             package names.  */
          do
            i += 1;
          while (i < len0 && (encoded[i] == 'b' || encoded[i] == 'n'));
          if (i < len0)
            goto Suppress;
        }
      else if (i < len0 - 2 && encoded[i] == '_' && encoded[i + 1] == '_')
        {
         /* Replace '__' by '.'.  */
          decoded[j] = '.';
          at_start_name = 1;
          i += 2;
          j += 1;
        }
      else
        {
          /* It's a character part of the decoded name, so just copy it
             over.  */
          decoded[j] = encoded[i];
          i += 1;
          j += 1;
        }
    }
  decoded[j] = '\000';

  /* Decoded names should never contain any uppercase character.
     Double-check this, and abort the decoding if we find one.  */

  for (i = 0; decoded[i] != '\0'; i += 1)
    if (isupper (decoded[i]) || decoded[i] == ' ')
      goto Suppress;

  if (strcmp (decoded, encoded) == 0)
    return encoded;
  else
    return decoded;

Suppress:
  GROW_VECT (decoding_buffer, decoding_buffer_size, strlen (encoded) + 3);
  decoded = decoding_buffer;
  if (encoded[0] == '<')
    strcpy (decoded, encoded);
  else
    xsnprintf (decoded, decoding_buffer_size, "<%s>", encoded);
  return decoded;

}

/* Table for keeping permanent unique copies of decoded names.  Once
   allocated, names in this table are never released.  While this is a
   storage leak, it should not be significant unless there are massive
   changes in the set of decoded names in successive versions of a 
   symbol table loaded during a single session.  */
static struct htab *decoded_names_store;

/* Returns the decoded name of GSYMBOL, as for ada_decode, caching it
   in the language-specific part of GSYMBOL, if it has not been
   previously computed.  Tries to save the decoded name in the same
   obstack as GSYMBOL, if possible, and otherwise on the heap (so that,
   in any case, the decoded symbol has a lifetime at least that of
   GSYMBOL).
   The GSYMBOL parameter is "mutable" in the C++ sense: logically
   const, but nevertheless modified to a semantically equivalent form
   when a decoded name is cached in it.  */

const char *
ada_decode_symbol (const struct general_symbol_info *arg)
{
  struct general_symbol_info *gsymbol = (struct general_symbol_info *) arg;
  const char **resultp =
    &gsymbol->language_specific.demangled_name;

  if (!gsymbol->ada_mangled)
    {
      const char *decoded = ada_decode (gsymbol->name);
      struct obstack *obstack = gsymbol->language_specific.obstack;

      gsymbol->ada_mangled = 1;

      if (obstack != NULL)
	*resultp
	  = (const char *) obstack_copy0 (obstack, decoded, strlen (decoded));
      else
        {
	  /* Sometimes, we can't find a corresponding objfile, in
	     which case, we put the result on the heap.  Since we only
	     decode when needed, we hope this usually does not cause a
	     significant memory leak (FIXME).  */

          char **slot = (char **) htab_find_slot (decoded_names_store,
                                                  decoded, INSERT);

          if (*slot == NULL)
            *slot = xstrdup (decoded);
          *resultp = *slot;
        }
    }

  return *resultp;
}

static char *
ada_la_decode (const char *encoded, int options)
{
  return xstrdup (ada_decode (encoded));
}

/* Implement la_sniff_from_mangled_name for Ada.  */

static int
ada_sniff_from_mangled_name (const char *mangled, char **out)
{
  const char *demangled = ada_decode (mangled);

  *out = NULL;

  if (demangled != mangled && demangled != NULL && demangled[0] != '<')
    {
      /* Set the gsymbol language to Ada, but still return 0.
	 Two reasons for that:

	 1. For Ada, we prefer computing the symbol's decoded name
	 on the fly rather than pre-compute it, in order to save
	 memory (Ada projects are typically very large).

	 2. There are some areas in the definition of the GNAT
	 encoding where, with a bit of bad luck, we might be able
	 to decode a non-Ada symbol, generating an incorrect
	 demangled name (Eg: names ending with "TB" for instance
	 are identified as task bodies and so stripped from
	 the decoded name returned).

	 Returning 1, here, but not setting *DEMANGLED, helps us get a
	 little bit of the best of both worlds.  Because we're last,
	 we should not affect any of the other languages that were
	 able to demangle the symbol before us; we get to correctly
	 tag Ada symbols as such; and even if we incorrectly tagged a
	 non-Ada symbol, which should be rare, any routing through the
	 Ada language should be transparent (Ada tries to behave much
	 like C/C++ with non-Ada symbols).  */
      return 1;
    }

  return 0;
}

/* Returns non-zero iff SYM_NAME matches NAME, ignoring any trailing
   suffixes that encode debugging information or leading _ada_ on
   SYM_NAME (see is_name_suffix commentary for the debugging
   information that is ignored).  If WILD, then NAME need only match a
   suffix of SYM_NAME minus the same suffixes.  Also returns 0 if
   either argument is NULL.  */

static int
match_name (const char *sym_name, const char *name, int wild)
{
  if (sym_name == NULL || name == NULL)
    return 0;
  else if (wild)
    return wild_match (sym_name, name) == 0;
  else
    {
      int len_name = strlen (name);

      return (strncmp (sym_name, name, len_name) == 0
              && is_name_suffix (sym_name + len_name))
        || (startswith (sym_name, "_ada_")
            && strncmp (sym_name + 5, name, len_name) == 0
            && is_name_suffix (sym_name + len_name + 5));
    }
}


                                /* Arrays */

/* Assuming that INDEX_DESC_TYPE is an ___XA structure, a structure
   generated by the GNAT compiler to describe the index type used
   for each dimension of an array, check whether it follows the latest
   known encoding.  If not, fix it up to conform to the latest encoding.
   Otherwise, do nothing.  This function also does nothing if
   INDEX_DESC_TYPE is NULL.

   The GNAT encoding used to describle the array index type evolved a bit.
   Initially, the information would be provided through the name of each
   field of the structure type only, while the type of these fields was
   described as unspecified and irrelevant.  The debugger was then expected
   to perform a global type lookup using the name of that field in order
   to get access to the full index type description.  Because these global
   lookups can be very expensive, the encoding was later enhanced to make
   the global lookup unnecessary by defining the field type as being
   the full index type description.

   The purpose of this routine is to allow us to support older versions
   of the compiler by detecting the use of the older encoding, and by
   fixing up the INDEX_DESC_TYPE to follow the new one (at this point,
   we essentially replace each field's meaningless type by the associated
   index subtype).  */

void
ada_fixup_array_indexes_type (struct type *index_desc_type)
{
  int i;

  if (index_desc_type == NULL)
    return;
  gdb_assert (TYPE_NFIELDS (index_desc_type) > 0);

  /* Check if INDEX_DESC_TYPE follows the older encoding (it is sufficient
     to check one field only, no need to check them all).  If not, return
     now.

     If our INDEX_DESC_TYPE was generated using the older encoding,
     the field type should be a meaningless integer type whose name
     is not equal to the field name.  */
  if (TYPE_NAME (TYPE_FIELD_TYPE (index_desc_type, 0)) != NULL
      && strcmp (TYPE_NAME (TYPE_FIELD_TYPE (index_desc_type, 0)),
                 TYPE_FIELD_NAME (index_desc_type, 0)) == 0)
    return;

  /* Fixup each field of INDEX_DESC_TYPE.  */
  for (i = 0; i < TYPE_NFIELDS (index_desc_type); i++)
   {
     const char *name = TYPE_FIELD_NAME (index_desc_type, i);
     struct type *raw_type = ada_check_typedef (ada_find_any_type (name));

     if (raw_type)
       TYPE_FIELD_TYPE (index_desc_type, i) = raw_type;
   }
}

/* Names of MAX_ADA_DIMENS bounds in P_BOUNDS fields of array descriptors.  */

static char *bound_name[] = {
  "LB0", "UB0", "LB1", "UB1", "LB2", "UB2", "LB3", "UB3",
  "LB4", "UB4", "LB5", "UB5", "LB6", "UB6", "LB7", "UB7"
};

/* Maximum number of array dimensions we are prepared to handle.  */

#define MAX_ADA_DIMENS (sizeof(bound_name) / (2*sizeof(char *)))


/* The desc_* routines return primitive portions of array descriptors
   (fat pointers).  */

/* The descriptor or array type, if any, indicated by TYPE; removes
   level of indirection, if needed.  */

static struct type *
desc_base_type (struct type *type)
{
  if (type == NULL)
    return NULL;
  type = ada_check_typedef (type);
  if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
    type = ada_typedef_target_type (type);

  if (type != NULL
      && (TYPE_CODE (type) == TYPE_CODE_PTR
          || TYPE_CODE (type) == TYPE_CODE_REF))
    return ada_check_typedef (TYPE_TARGET_TYPE (type));
  else
    return type;
}

/* True iff TYPE indicates a "thin" array pointer type.  */

static int
is_thin_pntr (struct type *type)
{
  return
    is_suffix (ada_type_name (desc_base_type (type)), "___XUT")
    || is_suffix (ada_type_name (desc_base_type (type)), "___XUT___XVE");
}

/* The descriptor type for thin pointer type TYPE.  */

static struct type *
thin_descriptor_type (struct type *type)
{
  struct type *base_type = desc_base_type (type);

  if (base_type == NULL)
    return NULL;
  if (is_suffix (ada_type_name (base_type), "___XVE"))
    return base_type;
  else
    {
      struct type *alt_type = ada_find_parallel_type (base_type, "___XVE");

      if (alt_type == NULL)
        return base_type;
      else
        return alt_type;
    }
}

/* A pointer to the array data for thin-pointer value VAL.  */

static struct value *
thin_data_pntr (struct value *val)
{
  struct type *type = ada_check_typedef (value_type (val));
  struct type *data_type = desc_data_target_type (thin_descriptor_type (type));

  data_type = lookup_pointer_type (data_type);

  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    return value_cast (data_type, value_copy (val));
  else
    return value_from_longest (data_type, value_address (val));
}

/* True iff TYPE indicates a "thick" array pointer type.  */

static int
is_thick_pntr (struct type *type)
{
  type = desc_base_type (type);
  return (type != NULL && TYPE_CODE (type) == TYPE_CODE_STRUCT
          && lookup_struct_elt_type (type, "P_BOUNDS", 1) != NULL);
}

/* If TYPE is the type of an array descriptor (fat or thin pointer) or a
   pointer to one, the type of its bounds data; otherwise, NULL.  */

static struct type *
desc_bounds_type (struct type *type)
{
  struct type *r;

  type = desc_base_type (type);

  if (type == NULL)
    return NULL;
  else if (is_thin_pntr (type))
    {
      type = thin_descriptor_type (type);
      if (type == NULL)
        return NULL;
      r = lookup_struct_elt_type (type, "BOUNDS", 1);
      if (r != NULL)
        return ada_check_typedef (r);
    }
  else if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
    {
      r = lookup_struct_elt_type (type, "P_BOUNDS", 1);
      if (r != NULL)
        return ada_check_typedef (TYPE_TARGET_TYPE (ada_check_typedef (r)));
    }
  return NULL;
}

/* If ARR is an array descriptor (fat or thin pointer), or pointer to
   one, a pointer to its bounds data.   Otherwise NULL.  */

static struct value *
desc_bounds (struct value *arr)
{
  struct type *type = ada_check_typedef (value_type (arr));

  if (is_thin_pntr (type))
    {
      struct type *bounds_type =
        desc_bounds_type (thin_descriptor_type (type));
      LONGEST addr;

      if (bounds_type == NULL)
        error (_("Bad GNAT array descriptor"));

      /* NOTE: The following calculation is not really kosher, but
         since desc_type is an XVE-encoded type (and shouldn't be),
         the correct calculation is a real pain.  FIXME (and fix GCC).  */
      if (TYPE_CODE (type) == TYPE_CODE_PTR)
        addr = value_as_long (arr);
      else
        addr = value_address (arr);

      return
        value_from_longest (lookup_pointer_type (bounds_type),
                            addr - TYPE_LENGTH (bounds_type));
    }

  else if (is_thick_pntr (type))
    {
      struct value *p_bounds = value_struct_elt (&arr, NULL, "P_BOUNDS", NULL,
					       _("Bad GNAT array descriptor"));
      struct type *p_bounds_type = value_type (p_bounds);

      if (p_bounds_type
	  && TYPE_CODE (p_bounds_type) == TYPE_CODE_PTR)
	{
	  struct type *target_type = TYPE_TARGET_TYPE (p_bounds_type);

	  if (TYPE_STUB (target_type))
	    p_bounds = value_cast (lookup_pointer_type
				   (ada_check_typedef (target_type)),
				   p_bounds);
	}
      else
	error (_("Bad GNAT array descriptor"));

      return p_bounds;
    }
  else
    return NULL;
}

/* If TYPE is the type of an array-descriptor (fat pointer),  the bit
   position of the field containing the address of the bounds data.  */

static int
fat_pntr_bounds_bitpos (struct type *type)
{
  return TYPE_FIELD_BITPOS (desc_base_type (type), 1);
}

/* If TYPE is the type of an array-descriptor (fat pointer), the bit
   size of the field containing the address of the bounds data.  */

static int
fat_pntr_bounds_bitsize (struct type *type)
{
  type = desc_base_type (type);

  if (TYPE_FIELD_BITSIZE (type, 1) > 0)
    return TYPE_FIELD_BITSIZE (type, 1);
  else
    return 8 * TYPE_LENGTH (ada_check_typedef (TYPE_FIELD_TYPE (type, 1)));
}

/* If TYPE is the type of an array descriptor (fat or thin pointer) or a
   pointer to one, the type of its array data (a array-with-no-bounds type);
   otherwise, NULL.  Use ada_type_of_array to get an array type with bounds
   data.  */

static struct type *
desc_data_target_type (struct type *type)
{
  type = desc_base_type (type);

  /* NOTE: The following is bogus; see comment in desc_bounds.  */
  if (is_thin_pntr (type))
    return desc_base_type (TYPE_FIELD_TYPE (thin_descriptor_type (type), 1));
  else if (is_thick_pntr (type))
    {
      struct type *data_type = lookup_struct_elt_type (type, "P_ARRAY", 1);

      if (data_type
	  && TYPE_CODE (ada_check_typedef (data_type)) == TYPE_CODE_PTR)
	return ada_check_typedef (TYPE_TARGET_TYPE (data_type));
    }

  return NULL;
}

/* If ARR is an array descriptor (fat or thin pointer), a pointer to
   its array data.  */

static struct value *
desc_data (struct value *arr)
{
  struct type *type = value_type (arr);

  if (is_thin_pntr (type))
    return thin_data_pntr (arr);
  else if (is_thick_pntr (type))
    return value_struct_elt (&arr, NULL, "P_ARRAY", NULL,
                             _("Bad GNAT array descriptor"));
  else
    return NULL;
}


/* If TYPE is the type of an array-descriptor (fat pointer), the bit
   position of the field containing the address of the data.  */

static int
fat_pntr_data_bitpos (struct type *type)
{
  return TYPE_FIELD_BITPOS (desc_base_type (type), 0);
}

/* If TYPE is the type of an array-descriptor (fat pointer), the bit
   size of the field containing the address of the data.  */

static int
fat_pntr_data_bitsize (struct type *type)
{
  type = desc_base_type (type);

  if (TYPE_FIELD_BITSIZE (type, 0) > 0)
    return TYPE_FIELD_BITSIZE (type, 0);
  else
    return TARGET_CHAR_BIT * TYPE_LENGTH (TYPE_FIELD_TYPE (type, 0));
}

/* If BOUNDS is an array-bounds structure (or pointer to one), return
   the Ith lower bound stored in it, if WHICH is 0, and the Ith upper
   bound, if WHICH is 1.  The first bound is I=1.  */

static struct value *
desc_one_bound (struct value *bounds, int i, int which)
{
  return value_struct_elt (&bounds, NULL, bound_name[2 * i + which - 2], NULL,
                           _("Bad GNAT array descriptor bounds"));
}

/* If BOUNDS is an array-bounds structure type, return the bit position
   of the Ith lower bound stored in it, if WHICH is 0, and the Ith upper
   bound, if WHICH is 1.  The first bound is I=1.  */

static int
desc_bound_bitpos (struct type *type, int i, int which)
{
  return TYPE_FIELD_BITPOS (desc_base_type (type), 2 * i + which - 2);
}

/* If BOUNDS is an array-bounds structure type, return the bit field size
   of the Ith lower bound stored in it, if WHICH is 0, and the Ith upper
   bound, if WHICH is 1.  The first bound is I=1.  */

static int
desc_bound_bitsize (struct type *type, int i, int which)
{
  type = desc_base_type (type);

  if (TYPE_FIELD_BITSIZE (type, 2 * i + which - 2) > 0)
    return TYPE_FIELD_BITSIZE (type, 2 * i + which - 2);
  else
    return 8 * TYPE_LENGTH (TYPE_FIELD_TYPE (type, 2 * i + which - 2));
}

/* If TYPE is the type of an array-bounds structure, the type of its
   Ith bound (numbering from 1).  Otherwise, NULL.  */

static struct type *
desc_index_type (struct type *type, int i)
{
  type = desc_base_type (type);

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
    return lookup_struct_elt_type (type, bound_name[2 * i - 2], 1);
  else
    return NULL;
}

/* The number of index positions in the array-bounds type TYPE.
   Return 0 if TYPE is NULL.  */

static int
desc_arity (struct type *type)
{
  type = desc_base_type (type);

  if (type != NULL)
    return TYPE_NFIELDS (type) / 2;
  return 0;
}

/* Non-zero iff TYPE is a simple array type (not a pointer to one) or 
   an array descriptor type (representing an unconstrained array
   type).  */

static int
ada_is_direct_array_type (struct type *type)
{
  if (type == NULL)
    return 0;
  type = ada_check_typedef (type);
  return (TYPE_CODE (type) == TYPE_CODE_ARRAY
          || ada_is_array_descriptor_type (type));
}

/* Non-zero iff TYPE represents any kind of array in Ada, or a pointer
 * to one.  */

static int
ada_is_array_type (struct type *type)
{
  while (type != NULL 
	 && (TYPE_CODE (type) == TYPE_CODE_PTR 
	     || TYPE_CODE (type) == TYPE_CODE_REF))
    type = TYPE_TARGET_TYPE (type);
  return ada_is_direct_array_type (type);
}

/* Non-zero iff TYPE is a simple array type or pointer to one.  */

int
ada_is_simple_array_type (struct type *type)
{
  if (type == NULL)
    return 0;
  type = ada_check_typedef (type);
  return (TYPE_CODE (type) == TYPE_CODE_ARRAY
          || (TYPE_CODE (type) == TYPE_CODE_PTR
              && TYPE_CODE (ada_check_typedef (TYPE_TARGET_TYPE (type)))
                 == TYPE_CODE_ARRAY));
}

/* Non-zero iff TYPE belongs to a GNAT array descriptor.  */

int
ada_is_array_descriptor_type (struct type *type)
{
  struct type *data_type = desc_data_target_type (type);

  if (type == NULL)
    return 0;
  type = ada_check_typedef (type);
  return (data_type != NULL
	  && TYPE_CODE (data_type) == TYPE_CODE_ARRAY
	  && desc_arity (desc_bounds_type (type)) > 0);
}

/* Non-zero iff type is a partially mal-formed GNAT array
   descriptor.  FIXME: This is to compensate for some problems with
   debugging output from GNAT.  Re-examine periodically to see if it
   is still needed.  */

int
ada_is_bogus_array_descriptor (struct type *type)
{
  return
    type != NULL
    && TYPE_CODE (type) == TYPE_CODE_STRUCT
    && (lookup_struct_elt_type (type, "P_BOUNDS", 1) != NULL
        || lookup_struct_elt_type (type, "P_ARRAY", 1) != NULL)
    && !ada_is_array_descriptor_type (type);
}


/* If ARR has a record type in the form of a standard GNAT array descriptor,
   (fat pointer) returns the type of the array data described---specifically,
   a pointer-to-array type.  If BOUNDS is non-zero, the bounds data are filled
   in from the descriptor; otherwise, they are left unspecified.  If
   the ARR denotes a null array descriptor and BOUNDS is non-zero,
   returns NULL.  The result is simply the type of ARR if ARR is not
   a descriptor.  */
struct type *
ada_type_of_array (struct value *arr, int bounds)
{
  if (ada_is_constrained_packed_array_type (value_type (arr)))
    return decode_constrained_packed_array_type (value_type (arr));

  if (!ada_is_array_descriptor_type (value_type (arr)))
    return value_type (arr);

  if (!bounds)
    {
      struct type *array_type =
	ada_check_typedef (desc_data_target_type (value_type (arr)));

      if (ada_is_unconstrained_packed_array_type (value_type (arr)))
	TYPE_FIELD_BITSIZE (array_type, 0) =
	  decode_packed_array_bitsize (value_type (arr));
      
      return array_type;
    }
  else
    {
      struct type *elt_type;
      int arity;
      struct value *descriptor;

      elt_type = ada_array_element_type (value_type (arr), -1);
      arity = ada_array_arity (value_type (arr));

      if (elt_type == NULL || arity == 0)
        return ada_check_typedef (value_type (arr));

      descriptor = desc_bounds (arr);
      if (value_as_long (descriptor) == 0)
        return NULL;
      while (arity > 0)
        {
          struct type *range_type = alloc_type_copy (value_type (arr));
          struct type *array_type = alloc_type_copy (value_type (arr));
          struct value *low = desc_one_bound (descriptor, arity, 0);
          struct value *high = desc_one_bound (descriptor, arity, 1);

          arity -= 1;
          create_static_range_type (range_type, value_type (low),
				    longest_to_int (value_as_long (low)),
				    longest_to_int (value_as_long (high)));
          elt_type = create_array_type (array_type, elt_type, range_type);

	  if (ada_is_unconstrained_packed_array_type (value_type (arr)))
	    {
	      /* We need to store the element packed bitsize, as well as
	         recompute the array size, because it was previously
		 computed based on the unpacked element size.  */
	      LONGEST lo = value_as_long (low);
	      LONGEST hi = value_as_long (high);

	      TYPE_FIELD_BITSIZE (elt_type, 0) =
		decode_packed_array_bitsize (value_type (arr));
	      /* If the array has no element, then the size is already
	         zero, and does not need to be recomputed.  */
	      if (lo < hi)
		{
		  int array_bitsize =
		        (hi - lo + 1) * TYPE_FIELD_BITSIZE (elt_type, 0);

		  TYPE_LENGTH (array_type) = (array_bitsize + 7) / 8;
		}
	    }
        }

      return lookup_pointer_type (elt_type);
    }
}

/* If ARR does not represent an array, returns ARR unchanged.
   Otherwise, returns either a standard GDB array with bounds set
   appropriately or, if ARR is a non-null fat pointer, a pointer to a standard
   GDB array.  Returns NULL if ARR is a null fat pointer.  */

struct value *
ada_coerce_to_simple_array_ptr (struct value *arr)
{
  if (ada_is_array_descriptor_type (value_type (arr)))
    {
      struct type *arrType = ada_type_of_array (arr, 1);

      if (arrType == NULL)
        return NULL;
      return value_cast (arrType, value_copy (desc_data (arr)));
    }
  else if (ada_is_constrained_packed_array_type (value_type (arr)))
    return decode_constrained_packed_array (arr);
  else
    return arr;
}

/* If ARR does not represent an array, returns ARR unchanged.
   Otherwise, returns a standard GDB array describing ARR (which may
   be ARR itself if it already is in the proper form).  */

struct value *
ada_coerce_to_simple_array (struct value *arr)
{
  if (ada_is_array_descriptor_type (value_type (arr)))
    {
      struct value *arrVal = ada_coerce_to_simple_array_ptr (arr);

      if (arrVal == NULL)
        error (_("Bounds unavailable for null array pointer."));
      ada_ensure_varsize_limit (TYPE_TARGET_TYPE (value_type (arrVal)));
      return value_ind (arrVal);
    }
  else if (ada_is_constrained_packed_array_type (value_type (arr)))
    return decode_constrained_packed_array (arr);
  else
    return arr;
}

/* If TYPE represents a GNAT array type, return it translated to an
   ordinary GDB array type (possibly with BITSIZE fields indicating
   packing).  For other types, is the identity.  */

struct type *
ada_coerce_to_simple_array_type (struct type *type)
{
  if (ada_is_constrained_packed_array_type (type))
    return decode_constrained_packed_array_type (type);

  if (ada_is_array_descriptor_type (type))
    return ada_check_typedef (desc_data_target_type (type));

  return type;
}

/* Non-zero iff TYPE represents a standard GNAT packed-array type.  */

static int
ada_is_packed_array_type  (struct type *type)
{
  if (type == NULL)
    return 0;
  type = desc_base_type (type);
  type = ada_check_typedef (type);
  return
    ada_type_name (type) != NULL
    && strstr (ada_type_name (type), "___XP") != NULL;
}

/* Non-zero iff TYPE represents a standard GNAT constrained
   packed-array type.  */

int
ada_is_constrained_packed_array_type (struct type *type)
{
  return ada_is_packed_array_type (type)
    && !ada_is_array_descriptor_type (type);
}

/* Non-zero iff TYPE represents an array descriptor for a
   unconstrained packed-array type.  */

static int
ada_is_unconstrained_packed_array_type (struct type *type)
{
  return ada_is_packed_array_type (type)
    && ada_is_array_descriptor_type (type);
}

/* Given that TYPE encodes a packed array type (constrained or unconstrained),
   return the size of its elements in bits.  */

static long
decode_packed_array_bitsize (struct type *type)
{
  const char *raw_name;
  const char *tail;
  long bits;

  /* Access to arrays implemented as fat pointers are encoded as a typedef
     of the fat pointer type.  We need the name of the fat pointer type
     to do the decoding, so strip the typedef layer.  */
  if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
    type = ada_typedef_target_type (type);

  raw_name = ada_type_name (ada_check_typedef (type));
  if (!raw_name)
    raw_name = ada_type_name (desc_base_type (type));

  if (!raw_name)
    return 0;

  tail = strstr (raw_name, "___XP");
  gdb_assert (tail != NULL);

  if (sscanf (tail + sizeof ("___XP") - 1, "%ld", &bits) != 1)
    {
      lim_warning
	(_("could not understand bit size information on packed array"));
      return 0;
    }

  return bits;
}

/* Given that TYPE is a standard GDB array type with all bounds filled
   in, and that the element size of its ultimate scalar constituents
   (that is, either its elements, or, if it is an array of arrays, its
   elements' elements, etc.) is *ELT_BITS, return an identical type,
   but with the bit sizes of its elements (and those of any
   constituent arrays) recorded in the BITSIZE components of its
   TYPE_FIELD_BITSIZE values, and with *ELT_BITS set to its total size
   in bits.

   Note that, for arrays whose index type has an XA encoding where
   a bound references a record discriminant, getting that discriminant,
   and therefore the actual value of that bound, is not possible
   because none of the given parameters gives us access to the record.
   This function assumes that it is OK in the context where it is being
   used to return an array whose bounds are still dynamic and where
   the length is arbitrary.  */

static struct type *
constrained_packed_array_type (struct type *type, long *elt_bits)
{
  struct type *new_elt_type;
  struct type *new_type;
  struct type *index_type_desc;
  struct type *index_type;
  LONGEST low_bound, high_bound;

  type = ada_check_typedef (type);
  if (TYPE_CODE (type) != TYPE_CODE_ARRAY)
    return type;

  index_type_desc = ada_find_parallel_type (type, "___XA");
  if (index_type_desc)
    index_type = to_fixed_range_type (TYPE_FIELD_TYPE (index_type_desc, 0),
				      NULL);
  else
    index_type = TYPE_INDEX_TYPE (type);

  new_type = alloc_type_copy (type);
  new_elt_type =
    constrained_packed_array_type (ada_check_typedef (TYPE_TARGET_TYPE (type)),
				   elt_bits);
  create_array_type (new_type, new_elt_type, index_type);
  TYPE_FIELD_BITSIZE (new_type, 0) = *elt_bits;
  TYPE_NAME (new_type) = ada_type_name (type);

  if ((TYPE_CODE (check_typedef (index_type)) == TYPE_CODE_RANGE
       && is_dynamic_type (check_typedef (index_type)))
      || get_discrete_bounds (index_type, &low_bound, &high_bound) < 0)
    low_bound = high_bound = 0;
  if (high_bound < low_bound)
    *elt_bits = TYPE_LENGTH (new_type) = 0;
  else
    {
      *elt_bits *= (high_bound - low_bound + 1);
      TYPE_LENGTH (new_type) =
        (*elt_bits + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT;
    }

  TYPE_FIXED_INSTANCE (new_type) = 1;
  return new_type;
}

/* The array type encoded by TYPE, where
   ada_is_constrained_packed_array_type (TYPE).  */

static struct type *
decode_constrained_packed_array_type (struct type *type)
{
  const char *raw_name = ada_type_name (ada_check_typedef (type));
  char *name;
  const char *tail;
  struct type *shadow_type;
  long bits;

  if (!raw_name)
    raw_name = ada_type_name (desc_base_type (type));

  if (!raw_name)
    return NULL;

  name = (char *) alloca (strlen (raw_name) + 1);
  tail = strstr (raw_name, "___XP");
  type = desc_base_type (type);

  memcpy (name, raw_name, tail - raw_name);
  name[tail - raw_name] = '\000';

  shadow_type = ada_find_parallel_type_with_name (type, name);

  if (shadow_type == NULL)
    {
      lim_warning (_("could not find bounds information on packed array"));
      return NULL;
    }
  shadow_type = check_typedef (shadow_type);

  if (TYPE_CODE (shadow_type) != TYPE_CODE_ARRAY)
    {
      lim_warning (_("could not understand bounds "
		     "information on packed array"));
      return NULL;
    }

  bits = decode_packed_array_bitsize (type);
  return constrained_packed_array_type (shadow_type, &bits);
}

/* Given that ARR is a struct value *indicating a GNAT constrained packed
   array, returns a simple array that denotes that array.  Its type is a
   standard GDB array type except that the BITSIZEs of the array
   target types are set to the number of bits in each element, and the
   type length is set appropriately.  */

static struct value *
decode_constrained_packed_array (struct value *arr)
{
  struct type *type;

  /* If our value is a pointer, then dereference it. Likewise if
     the value is a reference.  Make sure that this operation does not
     cause the target type to be fixed, as this would indirectly cause
     this array to be decoded.  The rest of the routine assumes that
     the array hasn't been decoded yet, so we use the basic "coerce_ref"
     and "value_ind" routines to perform the dereferencing, as opposed
     to using "ada_coerce_ref" or "ada_value_ind".  */
  arr = coerce_ref (arr);
  if (TYPE_CODE (ada_check_typedef (value_type (arr))) == TYPE_CODE_PTR)
    arr = value_ind (arr);

  type = decode_constrained_packed_array_type (value_type (arr));
  if (type == NULL)
    {
      error (_("can't unpack array"));
      return NULL;
    }

  if (gdbarch_bits_big_endian (get_type_arch (value_type (arr)))
      && ada_is_modular_type (value_type (arr)))
    {
       /* This is a (right-justified) modular type representing a packed
 	 array with no wrapper.  In order to interpret the value through
 	 the (left-justified) packed array type we just built, we must
 	 first left-justify it.  */
      int bit_size, bit_pos;
      ULONGEST mod;

      mod = ada_modulus (value_type (arr)) - 1;
      bit_size = 0;
      while (mod > 0)
	{
	  bit_size += 1;
	  mod >>= 1;
	}
      bit_pos = HOST_CHAR_BIT * TYPE_LENGTH (value_type (arr)) - bit_size;
      arr = ada_value_primitive_packed_val (arr, NULL,
					    bit_pos / HOST_CHAR_BIT,
					    bit_pos % HOST_CHAR_BIT,
					    bit_size,
					    type);
    }

  return coerce_unspec_val_to_type (arr, type);
}


/* The value of the element of packed array ARR at the ARITY indices
   given in IND.   ARR must be a simple array.  */

static struct value *
value_subscript_packed (struct value *arr, int arity, struct value **ind)
{
  int i;
  int bits, elt_off, bit_off;
  long elt_total_bit_offset;
  struct type *elt_type;
  struct value *v;

  bits = 0;
  elt_total_bit_offset = 0;
  elt_type = ada_check_typedef (value_type (arr));
  for (i = 0; i < arity; i += 1)
    {
      if (TYPE_CODE (elt_type) != TYPE_CODE_ARRAY
          || TYPE_FIELD_BITSIZE (elt_type, 0) == 0)
        error
          (_("attempt to do packed indexing of "
	     "something other than a packed array"));
      else
        {
          struct type *range_type = TYPE_INDEX_TYPE (elt_type);
          LONGEST lowerbound, upperbound;
          LONGEST idx;

          if (get_discrete_bounds (range_type, &lowerbound, &upperbound) < 0)
            {
              lim_warning (_("don't know bounds of array"));
              lowerbound = upperbound = 0;
            }

          idx = pos_atr (ind[i]);
          if (idx < lowerbound || idx > upperbound)
            lim_warning (_("packed array index %ld out of bounds"),
			 (long) idx);
          bits = TYPE_FIELD_BITSIZE (elt_type, 0);
          elt_total_bit_offset += (idx - lowerbound) * bits;
          elt_type = ada_check_typedef (TYPE_TARGET_TYPE (elt_type));
        }
    }
  elt_off = elt_total_bit_offset / HOST_CHAR_BIT;
  bit_off = elt_total_bit_offset % HOST_CHAR_BIT;

  v = ada_value_primitive_packed_val (arr, NULL, elt_off, bit_off,
                                      bits, elt_type);
  return v;
}

/* Non-zero iff TYPE includes negative integer values.  */

static int
has_negatives (struct type *type)
{
  switch (TYPE_CODE (type))
    {
    default:
      return 0;
    case TYPE_CODE_INT:
      return !TYPE_UNSIGNED (type);
    case TYPE_CODE_RANGE:
      return TYPE_LOW_BOUND (type) < 0;
    }
}

/* With SRC being a buffer containing BIT_SIZE bits of data at BIT_OFFSET,
   unpack that data into UNPACKED.  UNPACKED_LEN is the size in bytes of
   the unpacked buffer.

   The size of the unpacked buffer (UNPACKED_LEN) is expected to be large
   enough to contain at least BIT_OFFSET bits.  If not, an error is raised.

   IS_BIG_ENDIAN is nonzero if the data is stored in big endian mode,
   zero otherwise.

   IS_SIGNED_TYPE is nonzero if the data corresponds to a signed type.

   IS_SCALAR is nonzero if the data corresponds to a signed type.  */

static void
ada_unpack_from_contents (const gdb_byte *src, int bit_offset, int bit_size,
			  gdb_byte *unpacked, int unpacked_len,
			  int is_big_endian, int is_signed_type,
			  int is_scalar)
{
  int src_len = (bit_size + bit_offset + HOST_CHAR_BIT - 1) / 8;
  int src_idx;                  /* Index into the source area */
  int src_bytes_left;           /* Number of source bytes left to process.  */
  int srcBitsLeft;              /* Number of source bits left to move */
  int unusedLS;                 /* Number of bits in next significant
                                   byte of source that are unused */

  int unpacked_idx;             /* Index into the unpacked buffer */
  int unpacked_bytes_left;      /* Number of bytes left to set in unpacked.  */

  unsigned long accum;          /* Staging area for bits being transferred */
  int accumSize;                /* Number of meaningful bits in accum */
  unsigned char sign;

  /* Transmit bytes from least to most significant; delta is the direction
     the indices move.  */
  int delta = is_big_endian ? -1 : 1;

  /* Make sure that unpacked is large enough to receive the BIT_SIZE
     bits from SRC.  .*/
  if ((bit_size + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT > unpacked_len)
    error (_("Cannot unpack %d bits into buffer of %d bytes"),
	   bit_size, unpacked_len);

  srcBitsLeft = bit_size;
  src_bytes_left = src_len;
  unpacked_bytes_left = unpacked_len;
  sign = 0;

  if (is_big_endian)
    {
      src_idx = src_len - 1;
      if (is_signed_type
	  && ((src[0] << bit_offset) & (1 << (HOST_CHAR_BIT - 1))))
        sign = ~0;

      unusedLS =
        (HOST_CHAR_BIT - (bit_size + bit_offset) % HOST_CHAR_BIT)
        % HOST_CHAR_BIT;

      if (is_scalar)
	{
          accumSize = 0;
          unpacked_idx = unpacked_len - 1;
	}
      else
	{
          /* Non-scalar values must be aligned at a byte boundary...  */
          accumSize =
            (HOST_CHAR_BIT - bit_size % HOST_CHAR_BIT) % HOST_CHAR_BIT;
          /* ... And are placed at the beginning (most-significant) bytes
             of the target.  */
          unpacked_idx = (bit_size + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT - 1;
          unpacked_bytes_left = unpacked_idx + 1;
	}
    }
  else
    {
      int sign_bit_offset = (bit_size + bit_offset - 1) % 8;

      src_idx = unpacked_idx = 0;
      unusedLS = bit_offset;
      accumSize = 0;

      if (is_signed_type && (src[src_len - 1] & (1 << sign_bit_offset)))
        sign = ~0;
    }

  accum = 0;
  while (src_bytes_left > 0)
    {
      /* Mask for removing bits of the next source byte that are not
         part of the value.  */
      unsigned int unusedMSMask =
        (1 << (srcBitsLeft >= HOST_CHAR_BIT ? HOST_CHAR_BIT : srcBitsLeft)) -
        1;
      /* Sign-extend bits for this byte.  */
      unsigned int signMask = sign & ~unusedMSMask;

      accum |=
        (((src[src_idx] >> unusedLS) & unusedMSMask) | signMask) << accumSize;
      accumSize += HOST_CHAR_BIT - unusedLS;
      if (accumSize >= HOST_CHAR_BIT)
        {
          unpacked[unpacked_idx] = accum & ~(~0UL << HOST_CHAR_BIT);
          accumSize -= HOST_CHAR_BIT;
          accum >>= HOST_CHAR_BIT;
          unpacked_bytes_left -= 1;
          unpacked_idx += delta;
        }
      srcBitsLeft -= HOST_CHAR_BIT - unusedLS;
      unusedLS = 0;
      src_bytes_left -= 1;
      src_idx += delta;
    }
  while (unpacked_bytes_left > 0)
    {
      accum |= sign << accumSize;
      unpacked[unpacked_idx] = accum & ~(~0UL << HOST_CHAR_BIT);
      accumSize -= HOST_CHAR_BIT;
      if (accumSize < 0)
	accumSize = 0;
      accum >>= HOST_CHAR_BIT;
      unpacked_bytes_left -= 1;
      unpacked_idx += delta;
    }
}

/* Create a new value of type TYPE from the contents of OBJ starting
   at byte OFFSET, and bit offset BIT_OFFSET within that byte,
   proceeding for BIT_SIZE bits.  If OBJ is an lval in memory, then
   assigning through the result will set the field fetched from.
   VALADDR is ignored unless OBJ is NULL, in which case,
   VALADDR+OFFSET must address the start of storage containing the 
   packed value.  The value returned  in this case is never an lval.
   Assumes 0 <= BIT_OFFSET < HOST_CHAR_BIT.  */

struct value *
ada_value_primitive_packed_val (struct value *obj, const gdb_byte *valaddr,
				long offset, int bit_offset, int bit_size,
                                struct type *type)
{
  struct value *v;
  const gdb_byte *src;                /* First byte containing data to unpack */
  gdb_byte *unpacked;
  const int is_scalar = is_scalar_type (type);
  const int is_big_endian = gdbarch_bits_big_endian (get_type_arch (type));
  gdb_byte *staging = NULL;
  int staging_len = 0;
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);

  type = ada_check_typedef (type);

  if (obj == NULL)
    src = valaddr + offset;
  else
    src = value_contents (obj) + offset;

  if (is_dynamic_type (type))
    {
      /* The length of TYPE might by dynamic, so we need to resolve
	 TYPE in order to know its actual size, which we then use
	 to create the contents buffer of the value we return.
	 The difficulty is that the data containing our object is
	 packed, and therefore maybe not at a byte boundary.  So, what
	 we do, is unpack the data into a byte-aligned buffer, and then
	 use that buffer as our object's value for resolving the type.  */
      staging_len = (bit_size + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT;
      staging = (gdb_byte *) malloc (staging_len);
      make_cleanup (xfree, staging);

      ada_unpack_from_contents (src, bit_offset, bit_size,
			        staging, staging_len,
				is_big_endian, has_negatives (type),
				is_scalar);
      type = resolve_dynamic_type (type, staging, 0);
      if (TYPE_LENGTH (type) < (bit_size + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT)
	{
	  /* This happens when the length of the object is dynamic,
	     and is actually smaller than the space reserved for it.
	     For instance, in an array of variant records, the bit_size
	     we're given is the array stride, which is constant and
	     normally equal to the maximum size of its element.
	     But, in reality, each element only actually spans a portion
	     of that stride.  */
	  bit_size = TYPE_LENGTH (type) * HOST_CHAR_BIT;
	}
    }

  if (obj == NULL)
    {
      v = allocate_value (type);
      src = valaddr + offset;
    }
  else if (VALUE_LVAL (obj) == lval_memory && value_lazy (obj))
    {
      int src_len = (bit_size + bit_offset + HOST_CHAR_BIT - 1) / 8;
      gdb_byte *buf;

      v = value_at (type, value_address (obj) + offset);
      buf = (gdb_byte *) alloca (src_len);
      read_memory (value_address (v), buf, src_len);
      src = buf;
    }
  else
    {
      v = allocate_value (type);
      src = value_contents (obj) + offset;
    }

  if (obj != NULL)
    {
      long new_offset = offset;

      set_value_component_location (v, obj);
      set_value_bitpos (v, bit_offset + value_bitpos (obj));
      set_value_bitsize (v, bit_size);
      if (value_bitpos (v) >= HOST_CHAR_BIT)
        {
	  ++new_offset;
          set_value_bitpos (v, value_bitpos (v) - HOST_CHAR_BIT);
        }
      set_value_offset (v, new_offset);

      /* Also set the parent value.  This is needed when trying to
	 assign a new value (in inferior memory).  */
      set_value_parent (v, obj);
    }
  else
    set_value_bitsize (v, bit_size);
  unpacked = value_contents_writeable (v);

  if (bit_size == 0)
    {
      memset (unpacked, 0, TYPE_LENGTH (type));
      do_cleanups (old_chain);
      return v;
    }

  if (staging != NULL && staging_len == TYPE_LENGTH (type))
    {
      /* Small short-cut: If we've unpacked the data into a buffer
	 of the same size as TYPE's length, then we can reuse that,
	 instead of doing the unpacking again.  */
      memcpy (unpacked, staging, staging_len);
    }
  else
    ada_unpack_from_contents (src, bit_offset, bit_size,
			      unpacked, TYPE_LENGTH (type),
			      is_big_endian, has_negatives (type), is_scalar);

  do_cleanups (old_chain);
  return v;
}

/* Move N bits from SOURCE, starting at bit offset SRC_OFFSET to
   TARGET, starting at bit offset TARG_OFFSET.  SOURCE and TARGET must
   not overlap.  */
static void
move_bits (gdb_byte *target, int targ_offset, const gdb_byte *source,
	   int src_offset, int n, int bits_big_endian_p)
{
  unsigned int accum, mask;
  int accum_bits, chunk_size;

  target += targ_offset / HOST_CHAR_BIT;
  targ_offset %= HOST_CHAR_BIT;
  source += src_offset / HOST_CHAR_BIT;
  src_offset %= HOST_CHAR_BIT;
  if (bits_big_endian_p)
    {
      accum = (unsigned char) *source;
      source += 1;
      accum_bits = HOST_CHAR_BIT - src_offset;

      while (n > 0)
        {
          int unused_right;

          accum = (accum << HOST_CHAR_BIT) + (unsigned char) *source;
          accum_bits += HOST_CHAR_BIT;
          source += 1;
          chunk_size = HOST_CHAR_BIT - targ_offset;
          if (chunk_size > n)
            chunk_size = n;
          unused_right = HOST_CHAR_BIT - (chunk_size + targ_offset);
          mask = ((1 << chunk_size) - 1) << unused_right;
          *target =
            (*target & ~mask)
            | ((accum >> (accum_bits - chunk_size - unused_right)) & mask);
          n -= chunk_size;
          accum_bits -= chunk_size;
          target += 1;
          targ_offset = 0;
        }
    }
  else
    {
      accum = (unsigned char) *source >> src_offset;
      source += 1;
      accum_bits = HOST_CHAR_BIT - src_offset;

      while (n > 0)
        {
          accum = accum + ((unsigned char) *source << accum_bits);
          accum_bits += HOST_CHAR_BIT;
          source += 1;
          chunk_size = HOST_CHAR_BIT - targ_offset;
          if (chunk_size > n)
            chunk_size = n;
          mask = ((1 << chunk_size) - 1) << targ_offset;
          *target = (*target & ~mask) | ((accum << targ_offset) & mask);
          n -= chunk_size;
          accum_bits -= chunk_size;
          accum >>= chunk_size;
          target += 1;
          targ_offset = 0;
        }
    }
}

/* Store the contents of FROMVAL into the location of TOVAL.
   Return a new value with the location of TOVAL and contents of
   FROMVAL.   Handles assignment into packed fields that have
   floating-point or non-scalar types.  */

static struct value *
ada_value_assign (struct value *toval, struct value *fromval)
{
  struct type *type = value_type (toval);
  int bits = value_bitsize (toval);

  toval = ada_coerce_ref (toval);
  fromval = ada_coerce_ref (fromval);

  if (ada_is_direct_array_type (value_type (toval)))
    toval = ada_coerce_to_simple_array (toval);
  if (ada_is_direct_array_type (value_type (fromval)))
    fromval = ada_coerce_to_simple_array (fromval);

  if (!deprecated_value_modifiable (toval))
    error (_("Left operand of assignment is not a modifiable lvalue."));

  if (VALUE_LVAL (toval) == lval_memory
      && bits > 0
      && (TYPE_CODE (type) == TYPE_CODE_FLT
          || TYPE_CODE (type) == TYPE_CODE_STRUCT))
    {
      int len = (value_bitpos (toval)
		 + bits + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT;
      int from_size;
      gdb_byte *buffer = (gdb_byte *) alloca (len);
      struct value *val;
      CORE_ADDR to_addr = value_address (toval);

      if (TYPE_CODE (type) == TYPE_CODE_FLT)
        fromval = value_cast (type, fromval);

      read_memory (to_addr, buffer, len);
      from_size = value_bitsize (fromval);
      if (from_size == 0)
	from_size = TYPE_LENGTH (value_type (fromval)) * TARGET_CHAR_BIT;
      if (gdbarch_bits_big_endian (get_type_arch (type)))
        move_bits (buffer, value_bitpos (toval),
		   value_contents (fromval), from_size - bits, bits, 1);
      else
        move_bits (buffer, value_bitpos (toval),
		   value_contents (fromval), 0, bits, 0);
      write_memory_with_notification (to_addr, buffer, len);

      val = value_copy (toval);
      memcpy (value_contents_raw (val), value_contents (fromval),
              TYPE_LENGTH (type));
      deprecated_set_value_type (val, type);

      return val;
    }

  return value_assign (toval, fromval);
}


/* Given that COMPONENT is a memory lvalue that is part of the lvalue
   CONTAINER, assign the contents of VAL to COMPONENTS's place in
   CONTAINER.  Modifies the VALUE_CONTENTS of CONTAINER only, not
   COMPONENT, and not the inferior's memory.  The current contents
   of COMPONENT are ignored.

   Although not part of the initial design, this function also works
   when CONTAINER and COMPONENT are not_lval's: it works as if CONTAINER
   had a null address, and COMPONENT had an address which is equal to
   its offset inside CONTAINER.  */

static void
value_assign_to_component (struct value *container, struct value *component,
			   struct value *val)
{
  LONGEST offset_in_container =
    (LONGEST)  (value_address (component) - value_address (container));
  int bit_offset_in_container =
    value_bitpos (component) - value_bitpos (container);
  int bits;

  val = value_cast (value_type (component), val);

  if (value_bitsize (component) == 0)
    bits = TARGET_CHAR_BIT * TYPE_LENGTH (value_type (component));
  else
    bits = value_bitsize (component);

  if (gdbarch_bits_big_endian (get_type_arch (value_type (container))))
    move_bits (value_contents_writeable (container) + offset_in_container,
	       value_bitpos (container) + bit_offset_in_container,
	       value_contents (val),
	       TYPE_LENGTH (value_type (component)) * TARGET_CHAR_BIT - bits,
	       bits, 1);
  else
    move_bits (value_contents_writeable (container) + offset_in_container,
	       value_bitpos (container) + bit_offset_in_container,
	       value_contents (val), 0, bits, 0);
}

/* The value of the element of array ARR at the ARITY indices given in IND.
   ARR may be either a simple array, GNAT array descriptor, or pointer
   thereto.  */

struct value *
ada_value_subscript (struct value *arr, int arity, struct value **ind)
{
  int k;
  struct value *elt;
  struct type *elt_type;

  elt = ada_coerce_to_simple_array (arr);

  elt_type = ada_check_typedef (value_type (elt));
  if (TYPE_CODE (elt_type) == TYPE_CODE_ARRAY
      && TYPE_FIELD_BITSIZE (elt_type, 0) > 0)
    return value_subscript_packed (elt, arity, ind);

  for (k = 0; k < arity; k += 1)
    {
      if (TYPE_CODE (elt_type) != TYPE_CODE_ARRAY)
        error (_("too many subscripts (%d expected)"), k);
      elt = value_subscript (elt, pos_atr (ind[k]));
    }
  return elt;
}

/* Assuming ARR is a pointer to a GDB array, the value of the element
   of *ARR at the ARITY indices given in IND.
   Does not read the entire array into memory.

   Note: Unlike what one would expect, this function is used instead of
   ada_value_subscript for basically all non-packed array types.  The reason
   for this is that a side effect of doing our own pointer arithmetics instead
   of relying on value_subscript is that there is no implicit typedef peeling.
   This is important for arrays of array accesses, where it allows us to
   preserve the fact that the array's element is an array access, where the
   access part os encoded in a typedef layer.  */

static struct value *
ada_value_ptr_subscript (struct value *arr, int arity, struct value **ind)
{
  int k;
  struct value *array_ind = ada_value_ind (arr);
  struct type *type
    = check_typedef (value_enclosing_type (array_ind));

  if (TYPE_CODE (type) == TYPE_CODE_ARRAY
      && TYPE_FIELD_BITSIZE (type, 0) > 0)
    return value_subscript_packed (array_ind, arity, ind);

  for (k = 0; k < arity; k += 1)
    {
      LONGEST lwb, upb;
      struct value *lwb_value;

      if (TYPE_CODE (type) != TYPE_CODE_ARRAY)
        error (_("too many subscripts (%d expected)"), k);
      arr = value_cast (lookup_pointer_type (TYPE_TARGET_TYPE (type)),
                        value_copy (arr));
      get_discrete_bounds (TYPE_INDEX_TYPE (type), &lwb, &upb);
      lwb_value = value_from_longest (value_type(ind[k]), lwb);
      arr = value_ptradd (arr, pos_atr (ind[k]) - pos_atr (lwb_value));
      type = TYPE_TARGET_TYPE (type);
    }

  return value_ind (arr);
}

/* Given that ARRAY_PTR is a pointer or reference to an array of type TYPE (the
   actual type of ARRAY_PTR is ignored), returns the Ada slice of
   HIGH'Pos-LOW'Pos+1 elements starting at index LOW.  The lower bound of
   this array is LOW, as per Ada rules.  */
static struct value *
ada_value_slice_from_ptr (struct value *array_ptr, struct type *type,
                          int low, int high)
{
  struct type *type0 = ada_check_typedef (type);
  struct type *base_index_type = TYPE_TARGET_TYPE (TYPE_INDEX_TYPE (type0));
  struct type *index_type
    = create_static_range_type (NULL, base_index_type, low, high);
  struct type *slice_type =
    create_array_type (NULL, TYPE_TARGET_TYPE (type0), index_type);
  int base_low =  ada_discrete_type_low_bound (TYPE_INDEX_TYPE (type0));
  LONGEST base_low_pos, low_pos;
  CORE_ADDR base;

  if (!discrete_position (base_index_type, low, &low_pos)
      || !discrete_position (base_index_type, base_low, &base_low_pos))
    {
      warning (_("unable to get positions in slice, use bounds instead"));
      low_pos = low;
      base_low_pos = base_low;
    }

  base = value_as_address (array_ptr)
    + ((low_pos - base_low_pos)
       * TYPE_LENGTH (TYPE_TARGET_TYPE (type0)));
  return value_at_lazy (slice_type, base);
}


static struct value *
ada_value_slice (struct value *array, int low, int high)
{
  struct type *type = ada_check_typedef (value_type (array));
  struct type *base_index_type = TYPE_TARGET_TYPE (TYPE_INDEX_TYPE (type));
  struct type *index_type
    = create_static_range_type (NULL, TYPE_INDEX_TYPE (type), low, high);
  struct type *slice_type =
    create_array_type (NULL, TYPE_TARGET_TYPE (type), index_type);
  LONGEST low_pos, high_pos;

  if (!discrete_position (base_index_type, low, &low_pos)
      || !discrete_position (base_index_type, high, &high_pos))
    {
      warning (_("unable to get positions in slice, use bounds instead"));
      low_pos = low;
      high_pos = high;
    }

  return value_cast (slice_type,
		     value_slice (array, low, high_pos - low_pos + 1));
}

/* If type is a record type in the form of a standard GNAT array
   descriptor, returns the number of dimensions for type.  If arr is a
   simple array, returns the number of "array of"s that prefix its
   type designation.  Otherwise, returns 0.  */

int
ada_array_arity (struct type *type)
{
  int arity;

  if (type == NULL)
    return 0;

  type = desc_base_type (type);

  arity = 0;
  if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
    return desc_arity (desc_bounds_type (type));
  else
    while (TYPE_CODE (type) == TYPE_CODE_ARRAY)
      {
        arity += 1;
        type = ada_check_typedef (TYPE_TARGET_TYPE (type));
      }

  return arity;
}

/* If TYPE is a record type in the form of a standard GNAT array
   descriptor or a simple array type, returns the element type for
   TYPE after indexing by NINDICES indices, or by all indices if
   NINDICES is -1.  Otherwise, returns NULL.  */

struct type *
ada_array_element_type (struct type *type, int nindices)
{
  type = desc_base_type (type);

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT)
    {
      int k;
      struct type *p_array_type;

      p_array_type = desc_data_target_type (type);

      k = ada_array_arity (type);
      if (k == 0)
        return NULL;

      /* Initially p_array_type = elt_type(*)[]...(k times)...[].  */
      if (nindices >= 0 && k > nindices)
        k = nindices;
      while (k > 0 && p_array_type != NULL)
        {
          p_array_type = ada_check_typedef (TYPE_TARGET_TYPE (p_array_type));
          k -= 1;
        }
      return p_array_type;
    }
  else if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
    {
      while (nindices != 0 && TYPE_CODE (type) == TYPE_CODE_ARRAY)
        {
          type = TYPE_TARGET_TYPE (type);
          nindices -= 1;
        }
      return type;
    }

  return NULL;
}

/* The type of nth index in arrays of given type (n numbering from 1).
   Does not examine memory.  Throws an error if N is invalid or TYPE
   is not an array type.  NAME is the name of the Ada attribute being
   evaluated ('range, 'first, 'last, or 'length); it is used in building
   the error message.  */

static struct type *
ada_index_type (struct type *type, int n, const char *name)
{
  struct type *result_type;

  type = desc_base_type (type);

  if (n < 0 || n > ada_array_arity (type))
    error (_("invalid dimension number to '%s"), name);

  if (ada_is_simple_array_type (type))
    {
      int i;

      for (i = 1; i < n; i += 1)
        type = TYPE_TARGET_TYPE (type);
      result_type = TYPE_TARGET_TYPE (TYPE_INDEX_TYPE (type));
      /* FIXME: The stabs type r(0,0);bound;bound in an array type
         has a target type of TYPE_CODE_UNDEF.  We compensate here, but
         perhaps stabsread.c would make more sense.  */
      if (result_type && TYPE_CODE (result_type) == TYPE_CODE_UNDEF)
        result_type = NULL;
    }
  else
    {
      result_type = desc_index_type (desc_bounds_type (type), n);
      if (result_type == NULL)
	error (_("attempt to take bound of something that is not an array"));
    }

  return result_type;
}

/* Given that arr is an array type, returns the lower bound of the
   Nth index (numbering from 1) if WHICH is 0, and the upper bound if
   WHICH is 1.  This returns bounds 0 .. -1 if ARR_TYPE is an
   array-descriptor type.  It works for other arrays with bounds supplied
   by run-time quantities other than discriminants.  */

static LONGEST
ada_array_bound_from_type (struct type *arr_type, int n, int which)
{
  struct type *type, *index_type_desc, *index_type;
  int i;

  gdb_assert (which == 0 || which == 1);

  if (ada_is_constrained_packed_array_type (arr_type))
    arr_type = decode_constrained_packed_array_type (arr_type);

  if (arr_type == NULL || !ada_is_simple_array_type (arr_type))
    return (LONGEST) - which;

  if (TYPE_CODE (arr_type) == TYPE_CODE_PTR)
    type = TYPE_TARGET_TYPE (arr_type);
  else
    type = arr_type;

  if (TYPE_FIXED_INSTANCE (type))
    {
      /* The array has already been fixed, so we do not need to
	 check the parallel ___XA type again.  That encoding has
	 already been applied, so ignore it now.  */
      index_type_desc = NULL;
    }
  else
    {
      index_type_desc = ada_find_parallel_type (type, "___XA");
      ada_fixup_array_indexes_type (index_type_desc);
    }

  if (index_type_desc != NULL)
    index_type = to_fixed_range_type (TYPE_FIELD_TYPE (index_type_desc, n - 1),
				      NULL);
  else
    {
      struct type *elt_type = check_typedef (type);

      for (i = 1; i < n; i++)
	elt_type = check_typedef (TYPE_TARGET_TYPE (elt_type));

      index_type = TYPE_INDEX_TYPE (elt_type);
    }

  return
    (LONGEST) (which == 0
               ? ada_discrete_type_low_bound (index_type)
               : ada_discrete_type_high_bound (index_type));
}

/* Given that arr is an array value, returns the lower bound of the
   nth index (numbering from 1) if WHICH is 0, and the upper bound if
   WHICH is 1.  This routine will also work for arrays with bounds
   supplied by run-time quantities other than discriminants.  */

static LONGEST
ada_array_bound (struct value *arr, int n, int which)
{
  struct type *arr_type;

  if (TYPE_CODE (check_typedef (value_type (arr))) == TYPE_CODE_PTR)
    arr = value_ind (arr);
  arr_type = value_enclosing_type (arr);

  if (ada_is_constrained_packed_array_type (arr_type))
    return ada_array_bound (decode_constrained_packed_array (arr), n, which);
  else if (ada_is_simple_array_type (arr_type))
    return ada_array_bound_from_type (arr_type, n, which);
  else
    return value_as_long (desc_one_bound (desc_bounds (arr), n, which));
}

/* Given that arr is an array value, returns the length of the
   nth index.  This routine will also work for arrays with bounds
   supplied by run-time quantities other than discriminants.
   Does not work for arrays indexed by enumeration types with representation
   clauses at the moment.  */

static LONGEST
ada_array_length (struct value *arr, int n)
{
  struct type *arr_type, *index_type;
  int low, high;

  if (TYPE_CODE (check_typedef (value_type (arr))) == TYPE_CODE_PTR)
    arr = value_ind (arr);
  arr_type = value_enclosing_type (arr);

  if (ada_is_constrained_packed_array_type (arr_type))
    return ada_array_length (decode_constrained_packed_array (arr), n);

  if (ada_is_simple_array_type (arr_type))
    {
      low = ada_array_bound_from_type (arr_type, n, 0);
      high = ada_array_bound_from_type (arr_type, n, 1);
    }
  else
    {
      low = value_as_long (desc_one_bound (desc_bounds (arr), n, 0));
      high = value_as_long (desc_one_bound (desc_bounds (arr), n, 1));
    }

  arr_type = check_typedef (arr_type);
  index_type = TYPE_INDEX_TYPE (arr_type);
  if (index_type != NULL)
    {
      struct type *base_type;
      if (TYPE_CODE (index_type) == TYPE_CODE_RANGE)
	base_type = TYPE_TARGET_TYPE (index_type);
      else
	base_type = index_type;

      low = pos_atr (value_from_longest (base_type, low));
      high = pos_atr (value_from_longest (base_type, high));
    }
  return high - low + 1;
}

/* An empty array whose type is that of ARR_TYPE (an array type),
   with bounds LOW to LOW-1.  */

static struct value *
empty_array (struct type *arr_type, int low)
{
  struct type *arr_type0 = ada_check_typedef (arr_type);
  struct type *index_type
    = create_static_range_type
        (NULL, TYPE_TARGET_TYPE (TYPE_INDEX_TYPE (arr_type0)),  low, low - 1);
  struct type *elt_type = ada_array_element_type (arr_type0, 1);

  return allocate_value (create_array_type (NULL, elt_type, index_type));
}


                                /* Name resolution */

/* The "decoded" name for the user-definable Ada operator corresponding
   to OP.  */

static const char *
ada_decoded_op_name (enum exp_opcode op)
{
  int i;

  for (i = 0; ada_opname_table[i].encoded != NULL; i += 1)
    {
      if (ada_opname_table[i].op == op)
        return ada_opname_table[i].decoded;
    }
  error (_("Could not find operator name for opcode"));
}


/* Same as evaluate_type (*EXP), but resolves ambiguous symbol
   references (marked by OP_VAR_VALUE nodes in which the symbol has an
   undefined namespace) and converts operators that are
   user-defined into appropriate function calls.  If CONTEXT_TYPE is
   non-null, it provides a preferred result type [at the moment, only
   type void has any effect---causing procedures to be preferred over
   functions in calls].  A null CONTEXT_TYPE indicates that a non-void
   return type is preferred.  May change (expand) *EXP.  */

static void
resolve (struct expression **expp, int void_context_p)
{
  struct type *context_type = NULL;
  int pc = 0;

  if (void_context_p)
    context_type = builtin_type ((*expp)->gdbarch)->builtin_void;

  resolve_subexp (expp, &pc, 1, context_type);
}

/* Resolve the operator of the subexpression beginning at
   position *POS of *EXPP.  "Resolving" consists of replacing
   the symbols that have undefined namespaces in OP_VAR_VALUE nodes
   with their resolutions, replacing built-in operators with
   function calls to user-defined operators, where appropriate, and,
   when DEPROCEDURE_P is non-zero, converting function-valued variables
   into parameterless calls.  May expand *EXPP.  The CONTEXT_TYPE functions
   are as in ada_resolve, above.  */

static struct value *
resolve_subexp (struct expression **expp, int *pos, int deprocedure_p,
                struct type *context_type)
{
  int pc = *pos;
  int i;
  struct expression *exp;       /* Convenience: == *expp.  */
  enum exp_opcode op = (*expp)->elts[pc].opcode;
  struct value **argvec;        /* Vector of operand types (alloca'ed).  */
  int nargs;                    /* Number of operands.  */
  int oplen;

  argvec = NULL;
  nargs = 0;
  exp = *expp;

  /* Pass one: resolve operands, saving their types and updating *pos,
     if needed.  */
  switch (op)
    {
    case OP_FUNCALL:
      if (exp->elts[pc + 3].opcode == OP_VAR_VALUE
          && SYMBOL_DOMAIN (exp->elts[pc + 5].symbol) == UNDEF_DOMAIN)
        *pos += 7;
      else
        {
          *pos += 3;
          resolve_subexp (expp, pos, 0, NULL);
        }
      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      break;

    case UNOP_ADDR:
      *pos += 1;
      resolve_subexp (expp, pos, 0, NULL);
      break;

    case UNOP_QUAL:
      *pos += 3;
      resolve_subexp (expp, pos, 1, check_typedef (exp->elts[pc + 1].type));
      break;

    case OP_ATR_MODULUS:
    case OP_ATR_SIZE:
    case OP_ATR_TAG:
    case OP_ATR_FIRST:
    case OP_ATR_LAST:
    case OP_ATR_LENGTH:
    case OP_ATR_POS:
    case OP_ATR_VAL:
    case OP_ATR_MIN:
    case OP_ATR_MAX:
    case TERNOP_IN_RANGE:
    case BINOP_IN_BOUNDS:
    case UNOP_IN_RANGE:
    case OP_AGGREGATE:
    case OP_OTHERS:
    case OP_CHOICES:
    case OP_POSITIONAL:
    case OP_DISCRETE_RANGE:
    case OP_NAME:
      ada_forward_operator_length (exp, pc, &oplen, &nargs);
      *pos += oplen;
      break;

    case BINOP_ASSIGN:
      {
        struct value *arg1;

        *pos += 1;
        arg1 = resolve_subexp (expp, pos, 0, NULL);
        if (arg1 == NULL)
          resolve_subexp (expp, pos, 1, NULL);
        else
          resolve_subexp (expp, pos, 1, value_type (arg1));
        break;
      }

    case UNOP_CAST:
      *pos += 3;
      nargs = 1;
      break;

    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_MUL:
    case BINOP_DIV:
    case BINOP_REM:
    case BINOP_MOD:
    case BINOP_EXP:
    case BINOP_CONCAT:
    case BINOP_LOGICAL_AND:
    case BINOP_LOGICAL_OR:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:

    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
    case BINOP_LESS:
    case BINOP_GTR:
    case BINOP_LEQ:
    case BINOP_GEQ:

    case BINOP_REPEAT:
    case BINOP_SUBSCRIPT:
    case BINOP_COMMA:
      *pos += 1;
      nargs = 2;
      break;

    case UNOP_NEG:
    case UNOP_PLUS:
    case UNOP_LOGICAL_NOT:
    case UNOP_ABS:
    case UNOP_IND:
      *pos += 1;
      nargs = 1;
      break;

    case OP_LONG:
    case OP_DOUBLE:
    case OP_VAR_VALUE:
      *pos += 4;
      break;

    case OP_TYPE:
    case OP_BOOL:
    case OP_LAST:
    case OP_INTERNALVAR:
      *pos += 3;
      break;

    case UNOP_MEMVAL:
      *pos += 3;
      nargs = 1;
      break;

    case OP_REGISTER:
      *pos += 4 + BYTES_TO_EXP_ELEM (exp->elts[pc + 1].longconst + 1);
      break;

    case STRUCTOP_STRUCT:
      *pos += 4 + BYTES_TO_EXP_ELEM (exp->elts[pc + 1].longconst + 1);
      nargs = 1;
      break;

    case TERNOP_SLICE:
      *pos += 1;
      nargs = 3;
      break;

    case OP_STRING:
      break;

    default:
      error (_("Unexpected operator during name resolution"));
    }

  argvec = XALLOCAVEC (struct value *, nargs + 1);
  for (i = 0; i < nargs; i += 1)
    argvec[i] = resolve_subexp (expp, pos, 1, NULL);
  argvec[i] = NULL;
  exp = *expp;

  /* Pass two: perform any resolution on principal operator.  */
  switch (op)
    {
    default:
      break;

    case OP_VAR_VALUE:
      if (SYMBOL_DOMAIN (exp->elts[pc + 2].symbol) == UNDEF_DOMAIN)
        {
          struct block_symbol *candidates;
          int n_candidates;

          n_candidates =
            ada_lookup_symbol_list (SYMBOL_LINKAGE_NAME
                                    (exp->elts[pc + 2].symbol),
                                    exp->elts[pc + 1].block, VAR_DOMAIN,
                                    &candidates);

          if (n_candidates > 1)
            {
              /* Types tend to get re-introduced locally, so if there
                 are any local symbols that are not types, first filter
                 out all types.  */
              int j;
              for (j = 0; j < n_candidates; j += 1)
                switch (SYMBOL_CLASS (candidates[j].symbol))
                  {
                  case LOC_REGISTER:
                  case LOC_ARG:
                  case LOC_REF_ARG:
                  case LOC_REGPARM_ADDR:
                  case LOC_LOCAL:
                  case LOC_COMPUTED:
                    goto FoundNonType;
                  default:
                    break;
                  }
            FoundNonType:
              if (j < n_candidates)
                {
                  j = 0;
                  while (j < n_candidates)
                    {
                      if (SYMBOL_CLASS (candidates[j].symbol) == LOC_TYPEDEF)
                        {
                          candidates[j] = candidates[n_candidates - 1];
                          n_candidates -= 1;
                        }
                      else
                        j += 1;
                    }
                }
            }

          if (n_candidates == 0)
            error (_("No definition found for %s"),
                   SYMBOL_PRINT_NAME (exp->elts[pc + 2].symbol));
          else if (n_candidates == 1)
            i = 0;
          else if (deprocedure_p
                   && !is_nonfunction (candidates, n_candidates))
            {
              i = ada_resolve_function
                (candidates, n_candidates, NULL, 0,
                 SYMBOL_LINKAGE_NAME (exp->elts[pc + 2].symbol),
                 context_type);
              if (i < 0)
                error (_("Could not find a match for %s"),
                       SYMBOL_PRINT_NAME (exp->elts[pc + 2].symbol));
            }
          else
            {
              printf_filtered (_("Multiple matches for %s\n"),
                               SYMBOL_PRINT_NAME (exp->elts[pc + 2].symbol));
              user_select_syms (candidates, n_candidates, 1);
              i = 0;
            }

          exp->elts[pc + 1].block = candidates[i].block;
          exp->elts[pc + 2].symbol = candidates[i].symbol;
          if (innermost_block == NULL
              || contained_in (candidates[i].block, innermost_block))
            innermost_block = candidates[i].block;
        }

      if (deprocedure_p
          && (TYPE_CODE (SYMBOL_TYPE (exp->elts[pc + 2].symbol))
              == TYPE_CODE_FUNC))
        {
          replace_operator_with_call (expp, pc, 0, 0,
                                      exp->elts[pc + 2].symbol,
                                      exp->elts[pc + 1].block);
          exp = *expp;
        }
      break;

    case OP_FUNCALL:
      {
        if (exp->elts[pc + 3].opcode == OP_VAR_VALUE
            && SYMBOL_DOMAIN (exp->elts[pc + 5].symbol) == UNDEF_DOMAIN)
          {
            struct block_symbol *candidates;
            int n_candidates;

            n_candidates =
              ada_lookup_symbol_list (SYMBOL_LINKAGE_NAME
                                      (exp->elts[pc + 5].symbol),
                                      exp->elts[pc + 4].block, VAR_DOMAIN,
                                      &candidates);
            if (n_candidates == 1)
              i = 0;
            else
              {
                i = ada_resolve_function
                  (candidates, n_candidates,
                   argvec, nargs,
                   SYMBOL_LINKAGE_NAME (exp->elts[pc + 5].symbol),
                   context_type);
                if (i < 0)
                  error (_("Could not find a match for %s"),
                         SYMBOL_PRINT_NAME (exp->elts[pc + 5].symbol));
              }

            exp->elts[pc + 4].block = candidates[i].block;
            exp->elts[pc + 5].symbol = candidates[i].symbol;
            if (innermost_block == NULL
                || contained_in (candidates[i].block, innermost_block))
              innermost_block = candidates[i].block;
          }
      }
      break;
    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_MUL:
    case BINOP_DIV:
    case BINOP_REM:
    case BINOP_MOD:
    case BINOP_CONCAT:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
    case BINOP_LESS:
    case BINOP_GTR:
    case BINOP_LEQ:
    case BINOP_GEQ:
    case BINOP_EXP:
    case UNOP_NEG:
    case UNOP_PLUS:
    case UNOP_LOGICAL_NOT:
    case UNOP_ABS:
      if (possible_user_operator_p (op, argvec))
        {
          struct block_symbol *candidates;
          int n_candidates;

          n_candidates =
            ada_lookup_symbol_list (ada_encode (ada_decoded_op_name (op)),
                                    (struct block *) NULL, VAR_DOMAIN,
                                    &candidates);
          i = ada_resolve_function (candidates, n_candidates, argvec, nargs,
                                    ada_decoded_op_name (op), NULL);
          if (i < 0)
            break;

	  replace_operator_with_call (expp, pc, nargs, 1,
				      candidates[i].symbol,
				      candidates[i].block);
          exp = *expp;
        }
      break;

    case OP_TYPE:
    case OP_REGISTER:
      return NULL;
    }

  *pos = pc;
  return evaluate_subexp_type (exp, pos);
}

/* Return non-zero if formal type FTYPE matches actual type ATYPE.  If
   MAY_DEREF is non-zero, the formal may be a pointer and the actual
   a non-pointer.  */
/* The term "match" here is rather loose.  The match is heuristic and
   liberal.  */

static int
ada_type_match (struct type *ftype, struct type *atype, int may_deref)
{
  ftype = ada_check_typedef (ftype);
  atype = ada_check_typedef (atype);

  if (TYPE_CODE (ftype) == TYPE_CODE_REF)
    ftype = TYPE_TARGET_TYPE (ftype);
  if (TYPE_CODE (atype) == TYPE_CODE_REF)
    atype = TYPE_TARGET_TYPE (atype);

  switch (TYPE_CODE (ftype))
    {
    default:
      return TYPE_CODE (ftype) == TYPE_CODE (atype);
    case TYPE_CODE_PTR:
      if (TYPE_CODE (atype) == TYPE_CODE_PTR)
        return ada_type_match (TYPE_TARGET_TYPE (ftype),
                               TYPE_TARGET_TYPE (atype), 0);
      else
        return (may_deref
                && ada_type_match (TYPE_TARGET_TYPE (ftype), atype, 0));
    case TYPE_CODE_INT:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_RANGE:
      switch (TYPE_CODE (atype))
        {
        case TYPE_CODE_INT:
        case TYPE_CODE_ENUM:
        case TYPE_CODE_RANGE:
          return 1;
        default:
          return 0;
        }

    case TYPE_CODE_ARRAY:
      return (TYPE_CODE (atype) == TYPE_CODE_ARRAY
              || ada_is_array_descriptor_type (atype));

    case TYPE_CODE_STRUCT:
      if (ada_is_array_descriptor_type (ftype))
        return (TYPE_CODE (atype) == TYPE_CODE_ARRAY
                || ada_is_array_descriptor_type (atype));
      else
        return (TYPE_CODE (atype) == TYPE_CODE_STRUCT
                && !ada_is_array_descriptor_type (atype));

    case TYPE_CODE_UNION:
    case TYPE_CODE_FLT:
      return (TYPE_CODE (atype) == TYPE_CODE (ftype));
    }
}

/* Return non-zero if the formals of FUNC "sufficiently match" the
   vector of actual argument types ACTUALS of size N_ACTUALS.  FUNC
   may also be an enumeral, in which case it is treated as a 0-
   argument function.  */

static int
ada_args_match (struct symbol *func, struct value **actuals, int n_actuals)
{
  int i;
  struct type *func_type = SYMBOL_TYPE (func);

  if (SYMBOL_CLASS (func) == LOC_CONST
      && TYPE_CODE (func_type) == TYPE_CODE_ENUM)
    return (n_actuals == 0);
  else if (func_type == NULL || TYPE_CODE (func_type) != TYPE_CODE_FUNC)
    return 0;

  if (TYPE_NFIELDS (func_type) != n_actuals)
    return 0;

  for (i = 0; i < n_actuals; i += 1)
    {
      if (actuals[i] == NULL)
        return 0;
      else
        {
          struct type *ftype = ada_check_typedef (TYPE_FIELD_TYPE (func_type,
								   i));
          struct type *atype = ada_check_typedef (value_type (actuals[i]));

          if (!ada_type_match (ftype, atype, 1))
            return 0;
        }
    }
  return 1;
}

/* False iff function type FUNC_TYPE definitely does not produce a value
   compatible with type CONTEXT_TYPE.  Conservatively returns 1 if
   FUNC_TYPE is not a valid function type with a non-null return type
   or an enumerated type.  A null CONTEXT_TYPE indicates any non-void type.  */

static int
return_match (struct type *func_type, struct type *context_type)
{
  struct type *return_type;

  if (func_type == NULL)
    return 1;

  if (TYPE_CODE (func_type) == TYPE_CODE_FUNC)
    return_type = get_base_type (TYPE_TARGET_TYPE (func_type));
  else
    return_type = get_base_type (func_type);
  if (return_type == NULL)
    return 1;

  context_type = get_base_type (context_type);

  if (TYPE_CODE (return_type) == TYPE_CODE_ENUM)
    return context_type == NULL || return_type == context_type;
  else if (context_type == NULL)
    return TYPE_CODE (return_type) != TYPE_CODE_VOID;
  else
    return TYPE_CODE (return_type) == TYPE_CODE (context_type);
}


/* Returns the index in SYMS[0..NSYMS-1] that contains  the symbol for the
   function (if any) that matches the types of the NARGS arguments in
   ARGS.  If CONTEXT_TYPE is non-null and there is at least one match
   that returns that type, then eliminate matches that don't.  If
   CONTEXT_TYPE is void and there is at least one match that does not
   return void, eliminate all matches that do.

   Asks the user if there is more than one match remaining.  Returns -1
   if there is no such symbol or none is selected.  NAME is used
   solely for messages.  May re-arrange and modify SYMS in
   the process; the index returned is for the modified vector.  */

static int
ada_resolve_function (struct block_symbol syms[],
                      int nsyms, struct value **args, int nargs,
                      const char *name, struct type *context_type)
{
  int fallback;
  int k;
  int m;                        /* Number of hits */

  m = 0;
  /* In the first pass of the loop, we only accept functions matching
     context_type.  If none are found, we add a second pass of the loop
     where every function is accepted.  */
  for (fallback = 0; m == 0 && fallback < 2; fallback++)
    {
      for (k = 0; k < nsyms; k += 1)
        {
          struct type *type = ada_check_typedef (SYMBOL_TYPE (syms[k].symbol));

          if (ada_args_match (syms[k].symbol, args, nargs)
              && (fallback || return_match (type, context_type)))
            {
              syms[m] = syms[k];
              m += 1;
            }
        }
    }

  /* If we got multiple matches, ask the user which one to use.  Don't do this
     interactive thing during completion, though, as the purpose of the
     completion is providing a list of all possible matches.  Prompting the
     user to filter it down would be completely unexpected in this case.  */
  if (m == 0)
    return -1;
  else if (m > 1 && !parse_completion)
    {
      printf_filtered (_("Multiple matches for %s\n"), name);
      user_select_syms (syms, m, 1);
      return 0;
    }
  return 0;
}

/* Returns true (non-zero) iff decoded name N0 should appear before N1
   in a listing of choices during disambiguation (see sort_choices, below).
   The idea is that overloadings of a subprogram name from the
   same package should sort in their source order.  We settle for ordering
   such symbols by their trailing number (__N  or $N).  */

static int
encoded_ordered_before (const char *N0, const char *N1)
{
  if (N1 == NULL)
    return 0;
  else if (N0 == NULL)
    return 1;
  else
    {
      int k0, k1;

      for (k0 = strlen (N0) - 1; k0 > 0 && isdigit (N0[k0]); k0 -= 1)
        ;
      for (k1 = strlen (N1) - 1; k1 > 0 && isdigit (N1[k1]); k1 -= 1)
        ;
      if ((N0[k0] == '_' || N0[k0] == '$') && N0[k0 + 1] != '\000'
          && (N1[k1] == '_' || N1[k1] == '$') && N1[k1 + 1] != '\000')
        {
          int n0, n1;

          n0 = k0;
          while (N0[n0] == '_' && n0 > 0 && N0[n0 - 1] == '_')
            n0 -= 1;
          n1 = k1;
          while (N1[n1] == '_' && n1 > 0 && N1[n1 - 1] == '_')
            n1 -= 1;
          if (n0 == n1 && strncmp (N0, N1, n0) == 0)
            return (atoi (N0 + k0 + 1) < atoi (N1 + k1 + 1));
        }
      return (strcmp (N0, N1) < 0);
    }
}

/* Sort SYMS[0..NSYMS-1] to put the choices in a canonical order by the
   encoded names.  */

static void
sort_choices (struct block_symbol syms[], int nsyms)
{
  int i;

  for (i = 1; i < nsyms; i += 1)
    {
      struct block_symbol sym = syms[i];
      int j;

      for (j = i - 1; j >= 0; j -= 1)
        {
          if (encoded_ordered_before (SYMBOL_LINKAGE_NAME (syms[j].symbol),
                                      SYMBOL_LINKAGE_NAME (sym.symbol)))
            break;
          syms[j + 1] = syms[j];
        }
      syms[j + 1] = sym;
    }
}

/* Whether GDB should display formals and return types for functions in the
   overloads selection menu.  */
static int print_signatures = 1;

/* Print the signature for SYM on STREAM according to the FLAGS options.  For
   all but functions, the signature is just the name of the symbol.  For
   functions, this is the name of the function, the list of types for formals
   and the return type (if any).  */

static void
ada_print_symbol_signature (struct ui_file *stream, struct symbol *sym,
			    const struct type_print_options *flags)
{
  struct type *type = SYMBOL_TYPE (sym);

  fprintf_filtered (stream, "%s", SYMBOL_PRINT_NAME (sym));
  if (!print_signatures
      || type == NULL
      || TYPE_CODE (type) != TYPE_CODE_FUNC)
    return;

  if (TYPE_NFIELDS (type) > 0)
    {
      int i;

      fprintf_filtered (stream, " (");
      for (i = 0; i < TYPE_NFIELDS (type); ++i)
	{
	  if (i > 0)
	    fprintf_filtered (stream, "; ");
	  ada_print_type (TYPE_FIELD_TYPE (type, i), NULL, stream, -1, 0,
			  flags);
	}
      fprintf_filtered (stream, ")");
    }
  if (TYPE_TARGET_TYPE (type) != NULL
      && TYPE_CODE (TYPE_TARGET_TYPE (type)) != TYPE_CODE_VOID)
    {
      fprintf_filtered (stream, " return ");
      ada_print_type (TYPE_TARGET_TYPE (type), NULL, stream, -1, 0, flags);
    }
}

/* Given a list of NSYMS symbols in SYMS, select up to MAX_RESULTS>0 
   by asking the user (if necessary), returning the number selected, 
   and setting the first elements of SYMS items.  Error if no symbols
   selected.  */

/* NOTE: Adapted from decode_line_2 in symtab.c, with which it ought
   to be re-integrated one of these days.  */

int
user_select_syms (struct block_symbol *syms, int nsyms, int max_results)
{
  int i;
  int *chosen = XALLOCAVEC (int , nsyms);
  int n_chosen;
  int first_choice = (max_results == 1) ? 1 : 2;
  const char *select_mode = multiple_symbols_select_mode ();

  if (max_results < 1)
    error (_("Request to select 0 symbols!"));
  if (nsyms <= 1)
    return nsyms;

  if (select_mode == multiple_symbols_cancel)
    error (_("\
canceled because the command is ambiguous\n\
See set/show multiple-symbol."));
  
  /* If select_mode is "all", then return all possible symbols.
     Only do that if more than one symbol can be selected, of course.
     Otherwise, display the menu as usual.  */
  if (select_mode == multiple_symbols_all && max_results > 1)
    return nsyms;

  printf_unfiltered (_("[0] cancel\n"));
  if (max_results > 1)
    printf_unfiltered (_("[1] all\n"));

  sort_choices (syms, nsyms);

  for (i = 0; i < nsyms; i += 1)
    {
      if (syms[i].symbol == NULL)
        continue;

      if (SYMBOL_CLASS (syms[i].symbol) == LOC_BLOCK)
        {
          struct symtab_and_line sal =
            find_function_start_sal (syms[i].symbol, 1);

	  printf_unfiltered ("[%d] ", i + first_choice);
	  ada_print_symbol_signature (gdb_stdout, syms[i].symbol,
				      &type_print_raw_options);
	  if (sal.symtab == NULL)
	    printf_unfiltered (_(" at <no source file available>:%d\n"),
			       sal.line);
	  else
	    printf_unfiltered (_(" at %s:%d\n"),
			       symtab_to_filename_for_display (sal.symtab),
			       sal.line);
          continue;
        }
      else
        {
          int is_enumeral =
            (SYMBOL_CLASS (syms[i].symbol) == LOC_CONST
             && SYMBOL_TYPE (syms[i].symbol) != NULL
             && TYPE_CODE (SYMBOL_TYPE (syms[i].symbol)) == TYPE_CODE_ENUM);
	  struct symtab *symtab = NULL;

	  if (SYMBOL_OBJFILE_OWNED (syms[i].symbol))
	    symtab = symbol_symtab (syms[i].symbol);

          if (SYMBOL_LINE (syms[i].symbol) != 0 && symtab != NULL)
	    {
	      printf_unfiltered ("[%d] ", i + first_choice);
	      ada_print_symbol_signature (gdb_stdout, syms[i].symbol,
					  &type_print_raw_options);
	      printf_unfiltered (_(" at %s:%d\n"),
				 symtab_to_filename_for_display (symtab),
				 SYMBOL_LINE (syms[i].symbol));
	    }
          else if (is_enumeral
                   && TYPE_NAME (SYMBOL_TYPE (syms[i].symbol)) != NULL)
            {
              printf_unfiltered (("[%d] "), i + first_choice);
              ada_print_type (SYMBOL_TYPE (syms[i].symbol), NULL,
                              gdb_stdout, -1, 0, &type_print_raw_options);
              printf_unfiltered (_("'(%s) (enumeral)\n"),
                                 SYMBOL_PRINT_NAME (syms[i].symbol));
            }
	  else
	    {
	      printf_unfiltered ("[%d] ", i + first_choice);
	      ada_print_symbol_signature (gdb_stdout, syms[i].symbol,
					  &type_print_raw_options);

	      if (symtab != NULL)
		printf_unfiltered (is_enumeral
				   ? _(" in %s (enumeral)\n")
				   : _(" at %s:?\n"),
				   symtab_to_filename_for_display (symtab));
	      else
		printf_unfiltered (is_enumeral
				   ? _(" (enumeral)\n")
				   : _(" at ?\n"));
	    }
        }
    }

  n_chosen = get_selections (chosen, nsyms, max_results, max_results > 1,
                             "overload-choice");

  for (i = 0; i < n_chosen; i += 1)
    syms[i] = syms[chosen[i]];

  return n_chosen;
}

/* Read and validate a set of numeric choices from the user in the
   range 0 .. N_CHOICES-1.  Place the results in increasing
   order in CHOICES[0 .. N-1], and return N.

   The user types choices as a sequence of numbers on one line
   separated by blanks, encoding them as follows:

     + A choice of 0 means to cancel the selection, throwing an error.
     + If IS_ALL_CHOICE, a choice of 1 selects the entire set 0 .. N_CHOICES-1.
     + The user chooses k by typing k+IS_ALL_CHOICE+1.

   The user is not allowed to choose more than MAX_RESULTS values.

   ANNOTATION_SUFFIX, if present, is used to annotate the input
   prompts (for use with the -f switch).  */

int
get_selections (int *choices, int n_choices, int max_results,
                int is_all_choice, char *annotation_suffix)
{
  char *args;
  char *prompt;
  int n_chosen;
  int first_choice = is_all_choice ? 2 : 1;

  prompt = getenv ("PS2");
  if (prompt == NULL)
    prompt = "> ";

  args = command_line_input (prompt, 0, annotation_suffix);

  if (args == NULL)
    error_no_arg (_("one or more choice numbers"));

  n_chosen = 0;

  /* Set choices[0 .. n_chosen-1] to the users' choices in ascending
     order, as given in args.  Choices are validated.  */
  while (1)
    {
      char *args2;
      int choice, j;

      args = skip_spaces (args);
      if (*args == '\0' && n_chosen == 0)
        error_no_arg (_("one or more choice numbers"));
      else if (*args == '\0')
        break;

      choice = strtol (args, &args2, 10);
      if (args == args2 || choice < 0
          || choice > n_choices + first_choice - 1)
        error (_("Argument must be choice number"));
      args = args2;

      if (choice == 0)
        error (_("cancelled"));

      if (choice < first_choice)
        {
          n_chosen = n_choices;
          for (j = 0; j < n_choices; j += 1)
            choices[j] = j;
          break;
        }
      choice -= first_choice;

      for (j = n_chosen - 1; j >= 0 && choice < choices[j]; j -= 1)
        {
        }

      if (j < 0 || choice != choices[j])
        {
          int k;

          for (k = n_chosen - 1; k > j; k -= 1)
            choices[k + 1] = choices[k];
          choices[j + 1] = choice;
          n_chosen += 1;
        }
    }

  if (n_chosen > max_results)
    error (_("Select no more than %d of the above"), max_results);

  return n_chosen;
}

/* Replace the operator of length OPLEN at position PC in *EXPP with a call
   on the function identified by SYM and BLOCK, and taking NARGS
   arguments.  Update *EXPP as needed to hold more space.  */

static void
replace_operator_with_call (struct expression **expp, int pc, int nargs,
                            int oplen, struct symbol *sym,
                            const struct block *block)
{
  /* A new expression, with 6 more elements (3 for funcall, 4 for function
     symbol, -oplen for operator being replaced).  */
  struct expression *newexp = (struct expression *)
    xzalloc (sizeof (struct expression)
             + EXP_ELEM_TO_BYTES ((*expp)->nelts + 7 - oplen));
  struct expression *exp = *expp;

  newexp->nelts = exp->nelts + 7 - oplen;
  newexp->language_defn = exp->language_defn;
  newexp->gdbarch = exp->gdbarch;
  memcpy (newexp->elts, exp->elts, EXP_ELEM_TO_BYTES (pc));
  memcpy (newexp->elts + pc + 7, exp->elts + pc + oplen,
          EXP_ELEM_TO_BYTES (exp->nelts - pc - oplen));

  newexp->elts[pc].opcode = newexp->elts[pc + 2].opcode = OP_FUNCALL;
  newexp->elts[pc + 1].longconst = (LONGEST) nargs;

  newexp->elts[pc + 3].opcode = newexp->elts[pc + 6].opcode = OP_VAR_VALUE;
  newexp->elts[pc + 4].block = block;
  newexp->elts[pc + 5].symbol = sym;

  *expp = newexp;
  xfree (exp);
}

/* Type-class predicates */

/* True iff TYPE is numeric (i.e., an INT, RANGE (of numeric type),
   or FLOAT).  */

static int
numeric_type_p (struct type *type)
{
  if (type == NULL)
    return 0;
  else
    {
      switch (TYPE_CODE (type))
        {
        case TYPE_CODE_INT:
        case TYPE_CODE_FLT:
          return 1;
        case TYPE_CODE_RANGE:
          return (type == TYPE_TARGET_TYPE (type)
                  || numeric_type_p (TYPE_TARGET_TYPE (type)));
        default:
          return 0;
        }
    }
}

/* True iff TYPE is integral (an INT or RANGE of INTs).  */

static int
integer_type_p (struct type *type)
{
  if (type == NULL)
    return 0;
  else
    {
      switch (TYPE_CODE (type))
        {
        case TYPE_CODE_INT:
          return 1;
        case TYPE_CODE_RANGE:
          return (type == TYPE_TARGET_TYPE (type)
                  || integer_type_p (TYPE_TARGET_TYPE (type)));
        default:
          return 0;
        }
    }
}

/* True iff TYPE is scalar (INT, RANGE, FLOAT, ENUM).  */

static int
scalar_type_p (struct type *type)
{
  if (type == NULL)
    return 0;
  else
    {
      switch (TYPE_CODE (type))
        {
        case TYPE_CODE_INT:
        case TYPE_CODE_RANGE:
        case TYPE_CODE_ENUM:
        case TYPE_CODE_FLT:
          return 1;
        default:
          return 0;
        }
    }
}

/* True iff TYPE is discrete (INT, RANGE, ENUM).  */

static int
discrete_type_p (struct type *type)
{
  if (type == NULL)
    return 0;
  else
    {
      switch (TYPE_CODE (type))
        {
        case TYPE_CODE_INT:
        case TYPE_CODE_RANGE:
        case TYPE_CODE_ENUM:
        case TYPE_CODE_BOOL:
          return 1;
        default:
          return 0;
        }
    }
}

/* Returns non-zero if OP with operands in the vector ARGS could be
   a user-defined function.  Errs on the side of pre-defined operators
   (i.e., result 0).  */

static int
possible_user_operator_p (enum exp_opcode op, struct value *args[])
{
  struct type *type0 =
    (args[0] == NULL) ? NULL : ada_check_typedef (value_type (args[0]));
  struct type *type1 =
    (args[1] == NULL) ? NULL : ada_check_typedef (value_type (args[1]));

  if (type0 == NULL)
    return 0;

  switch (op)
    {
    default:
      return 0;

    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_MUL:
    case BINOP_DIV:
      return (!(numeric_type_p (type0) && numeric_type_p (type1)));

    case BINOP_REM:
    case BINOP_MOD:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
      return (!(integer_type_p (type0) && integer_type_p (type1)));

    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
    case BINOP_LESS:
    case BINOP_GTR:
    case BINOP_LEQ:
    case BINOP_GEQ:
      return (!(scalar_type_p (type0) && scalar_type_p (type1)));

    case BINOP_CONCAT:
      return !ada_is_array_type (type0) || !ada_is_array_type (type1);

    case BINOP_EXP:
      return (!(numeric_type_p (type0) && integer_type_p (type1)));

    case UNOP_NEG:
    case UNOP_PLUS:
    case UNOP_LOGICAL_NOT:
    case UNOP_ABS:
      return (!numeric_type_p (type0));

    }
}

                                /* Renaming */

/* NOTES: 

   1. In the following, we assume that a renaming type's name may
      have an ___XD suffix.  It would be nice if this went away at some
      point.
   2. We handle both the (old) purely type-based representation of 
      renamings and the (new) variable-based encoding.  At some point,
      it is devoutly to be hoped that the former goes away 
      (FIXME: hilfinger-2007-07-09).
   3. Subprogram renamings are not implemented, although the XRS
      suffix is recognized (FIXME: hilfinger-2007-07-09).  */

/* If SYM encodes a renaming, 

       <renaming> renames <renamed entity>,

   sets *LEN to the length of the renamed entity's name,
   *RENAMED_ENTITY to that name (not null-terminated), and *RENAMING_EXPR to
   the string describing the subcomponent selected from the renamed
   entity.  Returns ADA_NOT_RENAMING if SYM does not encode a renaming
   (in which case, the values of *RENAMED_ENTITY, *LEN, and *RENAMING_EXPR
   are undefined).  Otherwise, returns a value indicating the category
   of entity renamed: an object (ADA_OBJECT_RENAMING), exception
   (ADA_EXCEPTION_RENAMING), package (ADA_PACKAGE_RENAMING), or
   subprogram (ADA_SUBPROGRAM_RENAMING).  Does no allocation; the
   strings returned in *RENAMED_ENTITY and *RENAMING_EXPR should not be
   deallocated.  The values of RENAMED_ENTITY, LEN, or RENAMING_EXPR
   may be NULL, in which case they are not assigned.

   [Currently, however, GCC does not generate subprogram renamings.]  */

enum ada_renaming_category
ada_parse_renaming (struct symbol *sym,
		    const char **renamed_entity, int *len, 
		    const char **renaming_expr)
{
  enum ada_renaming_category kind;
  const char *info;
  const char *suffix;

  if (sym == NULL)
    return ADA_NOT_RENAMING;
  switch (SYMBOL_CLASS (sym)) 
    {
    default:
      return ADA_NOT_RENAMING;
    case LOC_TYPEDEF:
      return parse_old_style_renaming (SYMBOL_TYPE (sym), 
				       renamed_entity, len, renaming_expr);
    case LOC_LOCAL:
    case LOC_STATIC:
    case LOC_COMPUTED:
    case LOC_OPTIMIZED_OUT:
      info = strstr (SYMBOL_LINKAGE_NAME (sym), "___XR");
      if (info == NULL)
	return ADA_NOT_RENAMING;
      switch (info[5])
	{
	case '_':
	  kind = ADA_OBJECT_RENAMING;
	  info += 6;
	  break;
	case 'E':
	  kind = ADA_EXCEPTION_RENAMING;
	  info += 7;
	  break;
	case 'P':
	  kind = ADA_PACKAGE_RENAMING;
	  info += 7;
	  break;
	case 'S':
	  kind = ADA_SUBPROGRAM_RENAMING;
	  info += 7;
	  break;
	default:
	  return ADA_NOT_RENAMING;
	}
    }

  if (renamed_entity != NULL)
    *renamed_entity = info;
  suffix = strstr (info, "___XE");
  if (suffix == NULL || suffix == info)
    return ADA_NOT_RENAMING;
  if (len != NULL)
    *len = strlen (info) - strlen (suffix);
  suffix += 5;
  if (renaming_expr != NULL)
    *renaming_expr = suffix;
  return kind;
}

/* Assuming TYPE encodes a renaming according to the old encoding in
   exp_dbug.ads, returns details of that renaming in *RENAMED_ENTITY,
   *LEN, and *RENAMING_EXPR, as for ada_parse_renaming, above.  Returns
   ADA_NOT_RENAMING otherwise.  */
static enum ada_renaming_category
parse_old_style_renaming (struct type *type,
			  const char **renamed_entity, int *len, 
			  const char **renaming_expr)
{
  enum ada_renaming_category kind;
  const char *name;
  const char *info;
  const char *suffix;

  if (type == NULL || TYPE_CODE (type) != TYPE_CODE_ENUM 
      || TYPE_NFIELDS (type) != 1)
    return ADA_NOT_RENAMING;

  name = type_name_no_tag (type);
  if (name == NULL)
    return ADA_NOT_RENAMING;
  
  name = strstr (name, "___XR");
  if (name == NULL)
    return ADA_NOT_RENAMING;
  switch (name[5])
    {
    case '\0':
    case '_':
      kind = ADA_OBJECT_RENAMING;
      break;
    case 'E':
      kind = ADA_EXCEPTION_RENAMING;
      break;
    case 'P':
      kind = ADA_PACKAGE_RENAMING;
      break;
    case 'S':
      kind = ADA_SUBPROGRAM_RENAMING;
      break;
    default:
      return ADA_NOT_RENAMING;
    }

  info = TYPE_FIELD_NAME (type, 0);
  if (info == NULL)
    return ADA_NOT_RENAMING;
  if (renamed_entity != NULL)
    *renamed_entity = info;
  suffix = strstr (info, "___XE");
  if (renaming_expr != NULL)
    *renaming_expr = suffix + 5;
  if (suffix == NULL || suffix == info)
    return ADA_NOT_RENAMING;
  if (len != NULL)
    *len = suffix - info;
  return kind;
}

/* Compute the value of the given RENAMING_SYM, which is expected to
   be a symbol encoding a renaming expression.  BLOCK is the block
   used to evaluate the renaming.  */

static struct value *
ada_read_renaming_var_value (struct symbol *renaming_sym,
			     const struct block *block)
{
  const char *sym_name;
  struct expression *expr;
  struct value *value;
  struct cleanup *old_chain = NULL;

  sym_name = SYMBOL_LINKAGE_NAME (renaming_sym);
  expr = parse_exp_1 (&sym_name, 0, block, 0);
  old_chain = make_cleanup (free_current_contents, &expr);
  value = evaluate_expression (expr);

  do_cleanups (old_chain);
  return value;
}


                                /* Evaluation: Function Calls */

/* Return an lvalue containing the value VAL.  This is the identity on
   lvalues, and otherwise has the side-effect of allocating memory
   in the inferior where a copy of the value contents is copied.  */

static struct value *
ensure_lval (struct value *val)
{
  if (VALUE_LVAL (val) == not_lval
      || VALUE_LVAL (val) == lval_internalvar)
    {
      int len = TYPE_LENGTH (ada_check_typedef (value_type (val)));
      const CORE_ADDR addr =
        value_as_long (value_allocate_space_in_inferior (len));

      set_value_address (val, addr);
      VALUE_LVAL (val) = lval_memory;
      write_memory (addr, value_contents (val), len);
    }

  return val;
}

/* Return the value ACTUAL, converted to be an appropriate value for a
   formal of type FORMAL_TYPE.  Use *SP as a stack pointer for
   allocating any necessary descriptors (fat pointers), or copies of
   values not residing in memory, updating it as needed.  */

struct value *
ada_convert_actual (struct value *actual, struct type *formal_type0)
{
  struct type *actual_type = ada_check_typedef (value_type (actual));
  struct type *formal_type = ada_check_typedef (formal_type0);
  struct type *formal_target =
    TYPE_CODE (formal_type) == TYPE_CODE_PTR
    ? ada_check_typedef (TYPE_TARGET_TYPE (formal_type)) : formal_type;
  struct type *actual_target =
    TYPE_CODE (actual_type) == TYPE_CODE_PTR
    ? ada_check_typedef (TYPE_TARGET_TYPE (actual_type)) : actual_type;

  if (ada_is_array_descriptor_type (formal_target)
      && TYPE_CODE (actual_target) == TYPE_CODE_ARRAY)
    return make_array_descriptor (formal_type, actual);
  else if (TYPE_CODE (formal_type) == TYPE_CODE_PTR
	   || TYPE_CODE (formal_type) == TYPE_CODE_REF)
    {
      struct value *result;

      if (TYPE_CODE (formal_target) == TYPE_CODE_ARRAY
          && ada_is_array_descriptor_type (actual_target))
	result = desc_data (actual);
      else if (TYPE_CODE (actual_type) != TYPE_CODE_PTR)
        {
          if (VALUE_LVAL (actual) != lval_memory)
            {
              struct value *val;

              actual_type = ada_check_typedef (value_type (actual));
              val = allocate_value (actual_type);
              memcpy ((char *) value_contents_raw (val),
                      (char *) value_contents (actual),
                      TYPE_LENGTH (actual_type));
              actual = ensure_lval (val);
            }
          result = value_addr (actual);
        }
      else
	return actual;
      return value_cast_pointers (formal_type, result, 0);
    }
  else if (TYPE_CODE (actual_type) == TYPE_CODE_PTR)
    return ada_value_ind (actual);
  else if (ada_is_aligner_type (formal_type))
    {
      /* We need to turn this parameter into an aligner type
	 as well.  */
      struct value *aligner = allocate_value (formal_type);
      struct value *component = ada_value_struct_elt (aligner, "F", 0);

      value_assign_to_component (aligner, component, actual);
      return aligner;
    }

  return actual;
}

/* Convert VALUE (which must be an address) to a CORE_ADDR that is a pointer of
   type TYPE.  This is usually an inefficient no-op except on some targets
   (such as AVR) where the representation of a pointer and an address
   differs.  */

static CORE_ADDR
value_pointer (struct value *value, struct type *type)
{
  struct gdbarch *gdbarch = get_type_arch (type);
  unsigned len = TYPE_LENGTH (type);
  gdb_byte *buf = (gdb_byte *) alloca (len);
  CORE_ADDR addr;

  addr = value_address (value);
  gdbarch_address_to_pointer (gdbarch, type, buf, addr);
  addr = extract_unsigned_integer (buf, len, gdbarch_byte_order (gdbarch));
  return addr;
}


/* Push a descriptor of type TYPE for array value ARR on the stack at
   *SP, updating *SP to reflect the new descriptor.  Return either
   an lvalue representing the new descriptor, or (if TYPE is a pointer-
   to-descriptor type rather than a descriptor type), a struct value *
   representing a pointer to this descriptor.  */

static struct value *
make_array_descriptor (struct type *type, struct value *arr)
{
  struct type *bounds_type = desc_bounds_type (type);
  struct type *desc_type = desc_base_type (type);
  struct value *descriptor = allocate_value (desc_type);
  struct value *bounds = allocate_value (bounds_type);
  int i;

  for (i = ada_array_arity (ada_check_typedef (value_type (arr)));
       i > 0; i -= 1)
    {
      modify_field (value_type (bounds), value_contents_writeable (bounds),
		    ada_array_bound (arr, i, 0),
		    desc_bound_bitpos (bounds_type, i, 0),
		    desc_bound_bitsize (bounds_type, i, 0));
      modify_field (value_type (bounds), value_contents_writeable (bounds),
		    ada_array_bound (arr, i, 1),
		    desc_bound_bitpos (bounds_type, i, 1),
		    desc_bound_bitsize (bounds_type, i, 1));
    }

  bounds = ensure_lval (bounds);

  modify_field (value_type (descriptor),
		value_contents_writeable (descriptor),
		value_pointer (ensure_lval (arr),
			       TYPE_FIELD_TYPE (desc_type, 0)),
		fat_pntr_data_bitpos (desc_type),
		fat_pntr_data_bitsize (desc_type));

  modify_field (value_type (descriptor),
		value_contents_writeable (descriptor),
		value_pointer (bounds,
			       TYPE_FIELD_TYPE (desc_type, 1)),
		fat_pntr_bounds_bitpos (desc_type),
		fat_pntr_bounds_bitsize (desc_type));

  descriptor = ensure_lval (descriptor);

  if (TYPE_CODE (type) == TYPE_CODE_PTR)
    return value_addr (descriptor);
  else
    return descriptor;
}

                                /* Symbol Cache Module */

/* Performance measurements made as of 2010-01-15 indicate that
   this cache does bring some noticeable improvements.  Depending
   on the type of entity being printed, the cache can make it as much
   as an order of magnitude faster than without it.

   The descriptive type DWARF extension has significantly reduced
   the need for this cache, at least when DWARF is being used.  However,
   even in this case, some expensive name-based symbol searches are still
   sometimes necessary - to find an XVZ variable, mostly.  */

/* Initialize the contents of SYM_CACHE.  */

static void
ada_init_symbol_cache (struct ada_symbol_cache *sym_cache)
{
  obstack_init (&sym_cache->cache_space);
  memset (sym_cache->root, '\000', sizeof (sym_cache->root));
}

/* Free the memory used by SYM_CACHE.  */

static void
ada_free_symbol_cache (struct ada_symbol_cache *sym_cache)
{
  obstack_free (&sym_cache->cache_space, NULL);
  xfree (sym_cache);
}

/* Return the symbol cache associated to the given program space PSPACE.
   If not allocated for this PSPACE yet, allocate and initialize one.  */

static struct ada_symbol_cache *
ada_get_symbol_cache (struct program_space *pspace)
{
  struct ada_pspace_data *pspace_data = get_ada_pspace_data (pspace);

  if (pspace_data->sym_cache == NULL)
    {
      pspace_data->sym_cache = XCNEW (struct ada_symbol_cache);
      ada_init_symbol_cache (pspace_data->sym_cache);
    }

  return pspace_data->sym_cache;
}

/* Clear all entries from the symbol cache.  */

static void
ada_clear_symbol_cache (void)
{
  struct ada_symbol_cache *sym_cache
    = ada_get_symbol_cache (current_program_space);

  obstack_free (&sym_cache->cache_space, NULL);
  ada_init_symbol_cache (sym_cache);
}

/* Search our cache for an entry matching NAME and DOMAIN.
   Return it if found, or NULL otherwise.  */

static struct cache_entry **
find_entry (const char *name, domain_enum domain)
{
  struct ada_symbol_cache *sym_cache
    = ada_get_symbol_cache (current_program_space);
  int h = msymbol_hash (name) % HASH_SIZE;
  struct cache_entry **e;

  for (e = &sym_cache->root[h]; *e != NULL; e = &(*e)->next)
    {
      if (domain == (*e)->domain && strcmp (name, (*e)->name) == 0)
        return e;
    }
  return NULL;
}

/* Search the symbol cache for an entry matching NAME and DOMAIN.
   Return 1 if found, 0 otherwise.

   If an entry was found and SYM is not NULL, set *SYM to the entry's
   SYM.  Same principle for BLOCK if not NULL.  */

static int
lookup_cached_symbol (const char *name, domain_enum domain,
                      struct symbol **sym, const struct block **block)
{
  struct cache_entry **e = find_entry (name, domain);

  if (e == NULL)
    return 0;
  if (sym != NULL)
    *sym = (*e)->sym;
  if (block != NULL)
    *block = (*e)->block;
  return 1;
}

/* Assuming that (SYM, BLOCK) is the result of the lookup of NAME
   in domain DOMAIN, save this result in our symbol cache.  */

static void
cache_symbol (const char *name, domain_enum domain, struct symbol *sym,
              const struct block *block)
{
  struct ada_symbol_cache *sym_cache
    = ada_get_symbol_cache (current_program_space);
  int h;
  char *copy;
  struct cache_entry *e;

  /* Symbols for builtin types don't have a block.
     For now don't cache such symbols.  */
  if (sym != NULL && !SYMBOL_OBJFILE_OWNED (sym))
    return;

  /* If the symbol is a local symbol, then do not cache it, as a search
     for that symbol depends on the context.  To determine whether
     the symbol is local or not, we check the block where we found it
     against the global and static blocks of its associated symtab.  */
  if (sym
      && BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (symbol_symtab (sym)),
			    GLOBAL_BLOCK) != block
      && BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (symbol_symtab (sym)),
			    STATIC_BLOCK) != block)
    return;

  h = msymbol_hash (name) % HASH_SIZE;
  e = (struct cache_entry *) obstack_alloc (&sym_cache->cache_space,
					    sizeof (*e));
  e->next = sym_cache->root[h];
  sym_cache->root[h] = e;
  e->name = copy
    = (char *) obstack_alloc (&sym_cache->cache_space, strlen (name) + 1);
  strcpy (copy, name);
  e->sym = sym;
  e->domain = domain;
  e->block = block;
}

                                /* Symbol Lookup */

/* Return nonzero if wild matching should be used when searching for
   all symbols matching LOOKUP_NAME.

   LOOKUP_NAME is expected to be a symbol name after transformation
   for Ada lookups (see ada_name_for_lookup).  */

static int
should_use_wild_match (const char *lookup_name)
{
  return (strstr (lookup_name, "__") == NULL);
}

/* Return the result of a standard (literal, C-like) lookup of NAME in
   given DOMAIN, visible from lexical block BLOCK.  */

static struct symbol *
standard_lookup (const char *name, const struct block *block,
                 domain_enum domain)
{
  /* Initialize it just to avoid a GCC false warning.  */
  struct block_symbol sym = {NULL, NULL};

  if (lookup_cached_symbol (name, domain, &sym.symbol, NULL))
    return sym.symbol;
  sym = lookup_symbol_in_language (name, block, domain, language_c, 0);
  cache_symbol (name, domain, sym.symbol, sym.block);
  return sym.symbol;
}


/* Non-zero iff there is at least one non-function/non-enumeral symbol
   in the symbol fields of SYMS[0..N-1].  We treat enumerals as functions, 
   since they contend in overloading in the same way.  */
static int
is_nonfunction (struct block_symbol syms[], int n)
{
  int i;

  for (i = 0; i < n; i += 1)
    if (TYPE_CODE (SYMBOL_TYPE (syms[i].symbol)) != TYPE_CODE_FUNC
        && (TYPE_CODE (SYMBOL_TYPE (syms[i].symbol)) != TYPE_CODE_ENUM
            || SYMBOL_CLASS (syms[i].symbol) != LOC_CONST))
      return 1;

  return 0;
}

/* If true (non-zero), then TYPE0 and TYPE1 represent equivalent
   struct types.  Otherwise, they may not.  */

static int
equiv_types (struct type *type0, struct type *type1)
{
  if (type0 == type1)
    return 1;
  if (type0 == NULL || type1 == NULL
      || TYPE_CODE (type0) != TYPE_CODE (type1))
    return 0;
  if ((TYPE_CODE (type0) == TYPE_CODE_STRUCT
       || TYPE_CODE (type0) == TYPE_CODE_ENUM)
      && ada_type_name (type0) != NULL && ada_type_name (type1) != NULL
      && strcmp (ada_type_name (type0), ada_type_name (type1)) == 0)
    return 1;

  return 0;
}

/* True iff SYM0 represents the same entity as SYM1, or one that is
   no more defined than that of SYM1.  */

static int
lesseq_defined_than (struct symbol *sym0, struct symbol *sym1)
{
  if (sym0 == sym1)
    return 1;
  if (SYMBOL_DOMAIN (sym0) != SYMBOL_DOMAIN (sym1)
      || SYMBOL_CLASS (sym0) != SYMBOL_CLASS (sym1))
    return 0;

  switch (SYMBOL_CLASS (sym0))
    {
    case LOC_UNDEF:
      return 1;
    case LOC_TYPEDEF:
      {
        struct type *type0 = SYMBOL_TYPE (sym0);
        struct type *type1 = SYMBOL_TYPE (sym1);
        const char *name0 = SYMBOL_LINKAGE_NAME (sym0);
        const char *name1 = SYMBOL_LINKAGE_NAME (sym1);
        int len0 = strlen (name0);

        return
          TYPE_CODE (type0) == TYPE_CODE (type1)
          && (equiv_types (type0, type1)
              || (len0 < strlen (name1) && strncmp (name0, name1, len0) == 0
                  && startswith (name1 + len0, "___XV")));
      }
    case LOC_CONST:
      return SYMBOL_VALUE (sym0) == SYMBOL_VALUE (sym1)
        && equiv_types (SYMBOL_TYPE (sym0), SYMBOL_TYPE (sym1));
    default:
      return 0;
    }
}

/* Append (SYM,BLOCK,SYMTAB) to the end of the array of struct block_symbol
   records in OBSTACKP.  Do nothing if SYM is a duplicate.  */

static void
add_defn_to_vec (struct obstack *obstackp,
                 struct symbol *sym,
                 const struct block *block)
{
  int i;
  struct block_symbol *prevDefns = defns_collected (obstackp, 0);

  /* Do not try to complete stub types, as the debugger is probably
     already scanning all symbols matching a certain name at the
     time when this function is called.  Trying to replace the stub
     type by its associated full type will cause us to restart a scan
     which may lead to an infinite recursion.  Instead, the client
     collecting the matching symbols will end up collecting several
     matches, with at least one of them complete.  It can then filter
     out the stub ones if needed.  */

  for (i = num_defns_collected (obstackp) - 1; i >= 0; i -= 1)
    {
      if (lesseq_defined_than (sym, prevDefns[i].symbol))
        return;
      else if (lesseq_defined_than (prevDefns[i].symbol, sym))
        {
          prevDefns[i].symbol = sym;
          prevDefns[i].block = block;
          return;
        }
    }

  {
    struct block_symbol info;

    info.symbol = sym;
    info.block = block;
    obstack_grow (obstackp, &info, sizeof (struct block_symbol));
  }
}

/* Number of block_symbol structures currently collected in current vector in
   OBSTACKP.  */

static int
num_defns_collected (struct obstack *obstackp)
{
  return obstack_object_size (obstackp) / sizeof (struct block_symbol);
}

/* Vector of block_symbol structures currently collected in current vector in
   OBSTACKP.  If FINISH, close off the vector and return its final address.  */

static struct block_symbol *
defns_collected (struct obstack *obstackp, int finish)
{
  if (finish)
    return (struct block_symbol *) obstack_finish (obstackp);
  else
    return (struct block_symbol *) obstack_base (obstackp);
}

/* Return a bound minimal symbol matching NAME according to Ada
   decoding rules.  Returns an invalid symbol if there is no such
   minimal symbol.  Names prefixed with "standard__" are handled
   specially: "standard__" is first stripped off, and only static and
   global symbols are searched.  */

struct bound_minimal_symbol
ada_lookup_simple_minsym (const char *name)
{
  struct bound_minimal_symbol result;
  struct objfile *objfile;
  struct minimal_symbol *msymbol;
  const int wild_match_p = should_use_wild_match (name);

  memset (&result, 0, sizeof (result));

  /* Special case: If the user specifies a symbol name inside package
     Standard, do a non-wild matching of the symbol name without
     the "standard__" prefix.  This was primarily introduced in order
     to allow the user to specifically access the standard exceptions
     using, for instance, Standard.Constraint_Error when Constraint_Error
     is ambiguous (due to the user defining its own Constraint_Error
     entity inside its program).  */
  if (startswith (name, "standard__"))
    name += sizeof ("standard__") - 1;

  ALL_MSYMBOLS (objfile, msymbol)
  {
    if (match_name (MSYMBOL_LINKAGE_NAME (msymbol), name, wild_match_p)
        && MSYMBOL_TYPE (msymbol) != mst_solib_trampoline)
      {
	result.minsym = msymbol;
	result.objfile = objfile;
	break;
      }
  }

  return result;
}

/* For all subprograms that statically enclose the subprogram of the
   selected frame, add symbols matching identifier NAME in DOMAIN
   and their blocks to the list of data in OBSTACKP, as for
   ada_add_block_symbols (q.v.).   If WILD_MATCH_P, treat as NAME
   with a wildcard prefix.  */

static void
add_symbols_from_enclosing_procs (struct obstack *obstackp,
                                  const char *name, domain_enum domain,
                                  int wild_match_p)
{
}

/* True if TYPE is definitely an artificial type supplied to a symbol
   for which no debugging information was given in the symbol file.  */

static int
is_nondebugging_type (struct type *type)
{
  const char *name = ada_type_name (type);

  return (name != NULL && strcmp (name, "<variable, no debug info>") == 0);
}

/* Return nonzero if TYPE1 and TYPE2 are two enumeration types
   that are deemed "identical" for practical purposes.

   This function assumes that TYPE1 and TYPE2 are both TYPE_CODE_ENUM
   types and that their number of enumerals is identical (in other
   words, TYPE_NFIELDS (type1) == TYPE_NFIELDS (type2)).  */

static int
ada_identical_enum_types_p (struct type *type1, struct type *type2)
{
  int i;

  /* The heuristic we use here is fairly conservative.  We consider
     that 2 enumerate types are identical if they have the same
     number of enumerals and that all enumerals have the same
     underlying value and name.  */

  /* All enums in the type should have an identical underlying value.  */
  for (i = 0; i < TYPE_NFIELDS (type1); i++)
    if (TYPE_FIELD_ENUMVAL (type1, i) != TYPE_FIELD_ENUMVAL (type2, i))
      return 0;

  /* All enumerals should also have the same name (modulo any numerical
     suffix).  */
  for (i = 0; i < TYPE_NFIELDS (type1); i++)
    {
      const char *name_1 = TYPE_FIELD_NAME (type1, i);
      const char *name_2 = TYPE_FIELD_NAME (type2, i);
      int len_1 = strlen (name_1);
      int len_2 = strlen (name_2);

      ada_remove_trailing_digits (TYPE_FIELD_NAME (type1, i), &len_1);
      ada_remove_trailing_digits (TYPE_FIELD_NAME (type2, i), &len_2);
      if (len_1 != len_2
          || strncmp (TYPE_FIELD_NAME (type1, i),
		      TYPE_FIELD_NAME (type2, i),
		      len_1) != 0)
	return 0;
    }

  return 1;
}

/* Return nonzero if all the symbols in SYMS are all enumeral symbols
   that are deemed "identical" for practical purposes.  Sometimes,
   enumerals are not strictly identical, but their types are so similar
   that they can be considered identical.

   For instance, consider the following code:

      type Color is (Black, Red, Green, Blue, White);
      type RGB_Color is new Color range Red .. Blue;

   Type RGB_Color is a subrange of an implicit type which is a copy
   of type Color. If we call that implicit type RGB_ColorB ("B" is
   for "Base Type"), then type RGB_ColorB is a copy of type Color.
   As a result, when an expression references any of the enumeral
   by name (Eg. "print green"), the expression is technically
   ambiguous and the user should be asked to disambiguate. But
   doing so would only hinder the user, since it wouldn't matter
   what choice he makes, the outcome would always be the same.
   So, for practical purposes, we consider them as the same.  */

static int
symbols_are_identical_enums (struct block_symbol *syms, int nsyms)
{
  int i;

  /* Before performing a thorough comparison check of each type,
     we perform a series of inexpensive checks.  We expect that these
     checks will quickly fail in the vast majority of cases, and thus
     help prevent the unnecessary use of a more expensive comparison.
     Said comparison also expects us to make some of these checks
     (see ada_identical_enum_types_p).  */

  /* Quick check: All symbols should have an enum type.  */
  for (i = 0; i < nsyms; i++)
    if (TYPE_CODE (SYMBOL_TYPE (syms[i].symbol)) != TYPE_CODE_ENUM)
      return 0;

  /* Quick check: They should all have the same value.  */
  for (i = 1; i < nsyms; i++)
    if (SYMBOL_VALUE (syms[i].symbol) != SYMBOL_VALUE (syms[0].symbol))
      return 0;

  /* Quick check: They should all have the same number of enumerals.  */
  for (i = 1; i < nsyms; i++)
    if (TYPE_NFIELDS (SYMBOL_TYPE (syms[i].symbol))
        != TYPE_NFIELDS (SYMBOL_TYPE (syms[0].symbol)))
      return 0;

  /* All the sanity checks passed, so we might have a set of
     identical enumeration types.  Perform a more complete
     comparison of the type of each symbol.  */
  for (i = 1; i < nsyms; i++)
    if (!ada_identical_enum_types_p (SYMBOL_TYPE (syms[i].symbol),
                                     SYMBOL_TYPE (syms[0].symbol)))
      return 0;

  return 1;
}

/* Remove any non-debugging symbols in SYMS[0 .. NSYMS-1] that definitely
   duplicate other symbols in the list (The only case I know of where
   this happens is when object files containing stabs-in-ecoff are
   linked with files containing ordinary ecoff debugging symbols (or no
   debugging symbols)).  Modifies SYMS to squeeze out deleted entries.
   Returns the number of items in the modified list.  */

static int
remove_extra_symbols (struct block_symbol *syms, int nsyms)
{
  int i, j;

  /* We should never be called with less than 2 symbols, as there
     cannot be any extra symbol in that case.  But it's easy to
     handle, since we have nothing to do in that case.  */
  if (nsyms < 2)
    return nsyms;

  i = 0;
  while (i < nsyms)
    {
      int remove_p = 0;

      /* If two symbols have the same name and one of them is a stub type,
         the get rid of the stub.  */

      if (TYPE_STUB (SYMBOL_TYPE (syms[i].symbol))
          && SYMBOL_LINKAGE_NAME (syms[i].symbol) != NULL)
        {
          for (j = 0; j < nsyms; j++)
            {
              if (j != i
                  && !TYPE_STUB (SYMBOL_TYPE (syms[j].symbol))
                  && SYMBOL_LINKAGE_NAME (syms[j].symbol) != NULL
                  && strcmp (SYMBOL_LINKAGE_NAME (syms[i].symbol),
                             SYMBOL_LINKAGE_NAME (syms[j].symbol)) == 0)
                remove_p = 1;
            }
        }

      /* Two symbols with the same name, same class and same address
         should be identical.  */

      else if (SYMBOL_LINKAGE_NAME (syms[i].symbol) != NULL
          && SYMBOL_CLASS (syms[i].symbol) == LOC_STATIC
          && is_nondebugging_type (SYMBOL_TYPE (syms[i].symbol)))
        {
          for (j = 0; j < nsyms; j += 1)
            {
              if (i != j
                  && SYMBOL_LINKAGE_NAME (syms[j].symbol) != NULL
                  && strcmp (SYMBOL_LINKAGE_NAME (syms[i].symbol),
                             SYMBOL_LINKAGE_NAME (syms[j].symbol)) == 0
                  && SYMBOL_CLASS (syms[i].symbol)
		       == SYMBOL_CLASS (syms[j].symbol)
                  && SYMBOL_VALUE_ADDRESS (syms[i].symbol)
                  == SYMBOL_VALUE_ADDRESS (syms[j].symbol))
                remove_p = 1;
            }
        }
      
      if (remove_p)
        {
          for (j = i + 1; j < nsyms; j += 1)
            syms[j - 1] = syms[j];
          nsyms -= 1;
        }

      i += 1;
    }

  /* If all the remaining symbols are identical enumerals, then
     just keep the first one and discard the rest.

     Unlike what we did previously, we do not discard any entry
     unless they are ALL identical.  This is because the symbol
     comparison is not a strict comparison, but rather a practical
     comparison.  If all symbols are considered identical, then
     we can just go ahead and use the first one and discard the rest.
     But if we cannot reduce the list to a single element, we have
     to ask the user to disambiguate anyways.  And if we have to
     present a multiple-choice menu, it's less confusing if the list
     isn't missing some choices that were identical and yet distinct.  */
  if (symbols_are_identical_enums (syms, nsyms))
    nsyms = 1;

  return nsyms;
}

/* Given a type that corresponds to a renaming entity, use the type name
   to extract the scope (package name or function name, fully qualified,
   and following the GNAT encoding convention) where this renaming has been
   defined.  The string returned needs to be deallocated after use.  */

static char *
xget_renaming_scope (struct type *renaming_type)
{
  /* The renaming types adhere to the following convention:
     <scope>__<rename>___<XR extension>.
     So, to extract the scope, we search for the "___XR" extension,
     and then backtrack until we find the first "__".  */

  const char *name = type_name_no_tag (renaming_type);
  const char *suffix = strstr (name, "___XR");
  const char *last;
  int scope_len;
  char *scope;

  /* Now, backtrack a bit until we find the first "__".  Start looking
     at suffix - 3, as the <rename> part is at least one character long.  */

  for (last = suffix - 3; last > name; last--)
    if (last[0] == '_' && last[1] == '_')
      break;

  /* Make a copy of scope and return it.  */

  scope_len = last - name;
  scope = (char *) xmalloc ((scope_len + 1) * sizeof (char));

  strncpy (scope, name, scope_len);
  scope[scope_len] = '\0';

  return scope;
}

/* Return nonzero if NAME corresponds to a package name.  */

static int
is_package_name (const char *name)
{
  /* Here, We take advantage of the fact that no symbols are generated
     for packages, while symbols are generated for each function.
     So the condition for NAME represent a package becomes equivalent
     to NAME not existing in our list of symbols.  There is only one
     small complication with library-level functions (see below).  */

  char *fun_name;

  /* If it is a function that has not been defined at library level,
     then we should be able to look it up in the symbols.  */
  if (standard_lookup (name, NULL, VAR_DOMAIN) != NULL)
    return 0;

  /* Library-level function names start with "_ada_".  See if function
     "_ada_" followed by NAME can be found.  */

  /* Do a quick check that NAME does not contain "__", since library-level
     functions names cannot contain "__" in them.  */
  if (strstr (name, "__") != NULL)
    return 0;

  fun_name = xstrprintf ("_ada_%s", name);

  return (standard_lookup (fun_name, NULL, VAR_DOMAIN) == NULL);
}

/* Return nonzero if SYM corresponds to a renaming entity that is
   not visible from FUNCTION_NAME.  */

static int
old_renaming_is_invisible (const struct symbol *sym, const char *function_name)
{
  char *scope;
  struct cleanup *old_chain;

  if (SYMBOL_CLASS (sym) != LOC_TYPEDEF)
    return 0;

  scope = xget_renaming_scope (SYMBOL_TYPE (sym));
  old_chain = make_cleanup (xfree, scope);

  /* If the rename has been defined in a package, then it is visible.  */
  if (is_package_name (scope))
    {
      do_cleanups (old_chain);
      return 0;
    }

  /* Check that the rename is in the current function scope by checking
     that its name starts with SCOPE.  */

  /* If the function name starts with "_ada_", it means that it is
     a library-level function.  Strip this prefix before doing the
     comparison, as the encoding for the renaming does not contain
     this prefix.  */
  if (startswith (function_name, "_ada_"))
    function_name += 5;

  {
    int is_invisible = !startswith (function_name, scope);

    do_cleanups (old_chain);
    return is_invisible;
  }
}

/* Remove entries from SYMS that corresponds to a renaming entity that
   is not visible from the function associated with CURRENT_BLOCK or
   that is superfluous due to the presence of more specific renaming
   information.  Places surviving symbols in the initial entries of
   SYMS and returns the number of surviving symbols.
   
   Rationale:
   First, in cases where an object renaming is implemented as a
   reference variable, GNAT may produce both the actual reference
   variable and the renaming encoding.  In this case, we discard the
   latter.

   Second, GNAT emits a type following a specified encoding for each renaming
   entity.  Unfortunately, STABS currently does not support the definition
   of types that are local to a given lexical block, so all renamings types
   are emitted at library level.  As a consequence, if an application
   contains two renaming entities using the same name, and a user tries to
   print the value of one of these entities, the result of the ada symbol
   lookup will also contain the wrong renaming type.

   This function partially covers for this limitation by attempting to
   remove from the SYMS list renaming symbols that should be visible
   from CURRENT_BLOCK.  However, there does not seem be a 100% reliable
   method with the current information available.  The implementation
   below has a couple of limitations (FIXME: brobecker-2003-05-12):  
   
      - When the user tries to print a rename in a function while there
        is another rename entity defined in a package:  Normally, the
        rename in the function has precedence over the rename in the
        package, so the latter should be removed from the list.  This is
        currently not the case.
        
      - This function will incorrectly remove valid renames if
        the CURRENT_BLOCK corresponds to a function which symbol name
        has been changed by an "Export" pragma.  As a consequence,
        the user will be unable to print such rename entities.  */

static int
remove_irrelevant_renamings (struct block_symbol *syms,
			     int nsyms, const struct block *current_block)
{
  struct symbol *current_function;
  const char *current_function_name;
  int i;
  int is_new_style_renaming;

  /* If there is both a renaming foo___XR... encoded as a variable and
     a simple variable foo in the same block, discard the latter.
     First, zero out such symbols, then compress.  */
  is_new_style_renaming = 0;
  for (i = 0; i < nsyms; i += 1)
    {
      struct symbol *sym = syms[i].symbol;
      const struct block *block = syms[i].block;
      const char *name;
      const char *suffix;

      if (sym == NULL || SYMBOL_CLASS (sym) == LOC_TYPEDEF)
	continue;
      name = SYMBOL_LINKAGE_NAME (sym);
      suffix = strstr (name, "___XR");

      if (suffix != NULL)
	{
	  int name_len = suffix - name;
	  int j;

	  is_new_style_renaming = 1;
	  for (j = 0; j < nsyms; j += 1)
	    if (i != j && syms[j].symbol != NULL
		&& strncmp (name, SYMBOL_LINKAGE_NAME (syms[j].symbol),
			    name_len) == 0
		&& block == syms[j].block)
	      syms[j].symbol = NULL;
	}
    }
  if (is_new_style_renaming)
    {
      int j, k;

      for (j = k = 0; j < nsyms; j += 1)
	if (syms[j].symbol != NULL)
	    {
	      syms[k] = syms[j];
	      k += 1;
	    }
      return k;
    }

  /* Extract the function name associated to CURRENT_BLOCK.
     Abort if unable to do so.  */

  if (current_block == NULL)
    return nsyms;

  current_function = block_linkage_function (current_block);
  if (current_function == NULL)
    return nsyms;

  current_function_name = SYMBOL_LINKAGE_NAME (current_function);
  if (current_function_name == NULL)
    return nsyms;

  /* Check each of the symbols, and remove it from the list if it is
     a type corresponding to a renaming that is out of the scope of
     the current block.  */

  i = 0;
  while (i < nsyms)
    {
      if (ada_parse_renaming (syms[i].symbol, NULL, NULL, NULL)
          == ADA_OBJECT_RENAMING
          && old_renaming_is_invisible (syms[i].symbol, current_function_name))
        {
          int j;

          for (j = i + 1; j < nsyms; j += 1)
            syms[j - 1] = syms[j];
          nsyms -= 1;
        }
      else
        i += 1;
    }

  return nsyms;
}

/* Add to OBSTACKP all symbols from BLOCK (and its super-blocks)
   whose name and domain match NAME and DOMAIN respectively.
   If no match was found, then extend the search to "enclosing"
   routines (in other words, if we're inside a nested function,
   search the symbols defined inside the enclosing functions).
   If WILD_MATCH_P is nonzero, perform the naming matching in
   "wild" mode (see function "wild_match" for more info).

   Note: This function assumes that OBSTACKP has 0 (zero) element in it.  */

static void
ada_add_local_symbols (struct obstack *obstackp, const char *name,
                       const struct block *block, domain_enum domain,
                       int wild_match_p)
{
  int block_depth = 0;

  while (block != NULL)
    {
      block_depth += 1;
      ada_add_block_symbols (obstackp, block, name, domain, NULL,
			     wild_match_p);

      /* If we found a non-function match, assume that's the one.  */
      if (is_nonfunction (defns_collected (obstackp, 0),
                          num_defns_collected (obstackp)))
        return;

      block = BLOCK_SUPERBLOCK (block);
    }

  /* If no luck so far, try to find NAME as a local symbol in some lexically
     enclosing subprogram.  */
  if (num_defns_collected (obstackp) == 0 && block_depth > 2)
    add_symbols_from_enclosing_procs (obstackp, name, domain, wild_match_p);
}

/* An object of this type is used as the user_data argument when
   calling the map_matching_symbols method.  */

struct match_data
{
  struct objfile *objfile;
  struct obstack *obstackp;
  struct symbol *arg_sym;
  int found_sym;
};

/* A callback for add_nonlocal_symbols that adds SYM, found in BLOCK,
   to a list of symbols.  DATA0 is a pointer to a struct match_data *
   containing the obstack that collects the symbol list, the file that SYM
   must come from, a flag indicating whether a non-argument symbol has
   been found in the current block, and the last argument symbol
   passed in SYM within the current block (if any).  When SYM is null,
   marking the end of a block, the argument symbol is added if no
   other has been found.  */

static int
aux_add_nonlocal_symbols (struct block *block, struct symbol *sym, void *data0)
{
  struct match_data *data = (struct match_data *) data0;
  
  if (sym == NULL)
    {
      if (!data->found_sym && data->arg_sym != NULL) 
	add_defn_to_vec (data->obstackp,
			 fixup_symbol_section (data->arg_sym, data->objfile),
			 block);
      data->found_sym = 0;
      data->arg_sym = NULL;
    }
  else 
    {
      if (SYMBOL_CLASS (sym) == LOC_UNRESOLVED)
	return 0;
      else if (SYMBOL_IS_ARGUMENT (sym))
	data->arg_sym = sym;
      else
	{
	  data->found_sym = 1;
	  add_defn_to_vec (data->obstackp,
			   fixup_symbol_section (sym, data->objfile),
			   block);
	}
    }
  return 0;
}

/* Helper for add_nonlocal_symbols.  Find symbols in DOMAIN which are targetted
   by renamings matching NAME in BLOCK.  Add these symbols to OBSTACKP.  If
   WILD_MATCH_P is nonzero, perform the naming matching in "wild" mode (see
   function "wild_match" for more information).  Return whether we found such
   symbols.  */

static int
ada_add_block_renamings (struct obstack *obstackp,
			 const struct block *block,
			 const char *name,
			 domain_enum domain,
			 int wild_match_p)
{
  struct using_direct *renaming;
  int defns_mark = num_defns_collected (obstackp);

  for (renaming = block_using (block);
       renaming != NULL;
       renaming = renaming->next)
    {
      const char *r_name;
      int name_match;

      /* Avoid infinite recursions: skip this renaming if we are actually
	 already traversing it.

	 Currently, symbol lookup in Ada don't use the namespace machinery from
	 C++/Fortran support: skip namespace imports that use them.  */
      if (renaming->searched
	  || (renaming->import_src != NULL
	      && renaming->import_src[0] != '\0')
	  || (renaming->import_dest != NULL
	      && renaming->import_dest[0] != '\0'))
	continue;
      renaming->searched = 1;

      /* TODO: here, we perform another name-based symbol lookup, which can
	 pull its own multiple overloads.  In theory, we should be able to do
	 better in this case since, in DWARF, DW_AT_import is a DIE reference,
	 not a simple name.  But in order to do this, we would need to enhance
	 the DWARF reader to associate a symbol to this renaming, instead of a
	 name.  So, for now, we do something simpler: re-use the C++/Fortran
	 namespace machinery.  */
      r_name = (renaming->alias != NULL
		? renaming->alias
		: renaming->declaration);
      name_match
	= wild_match_p ? wild_match (r_name, name) : strcmp (r_name, name);
      if (name_match == 0)
	ada_add_all_symbols (obstackp, block, renaming->declaration, domain,
			     1, NULL);
      renaming->searched = 0;
    }
  return num_defns_collected (obstackp) != defns_mark;
}

/* Implements compare_names, but only applying the comparision using
   the given CASING.  */

static int
compare_names_with_case (const char *string1, const char *string2,
			 enum case_sensitivity casing)
{
  while (*string1 != '\0' && *string2 != '\0')
    {
      char c1, c2;

      if (isspace (*string1) || isspace (*string2))
	return strcmp_iw_ordered (string1, string2);

      if (casing == case_sensitive_off)
	{
	  c1 = tolower (*string1);
	  c2 = tolower (*string2);
	}
      else
	{
	  c1 = *string1;
	  c2 = *string2;
	}
      if (c1 != c2)
	break;

      string1 += 1;
      string2 += 1;
    }

  switch (*string1)
    {
    case '(':
      return strcmp_iw_ordered (string1, string2);
    case '_':
      if (*string2 == '\0')
	{
	  if (is_name_suffix (string1))
	    return 0;
	  else
	    return 1;
	}
      /* FALLTHROUGH */
    default:
      if (*string2 == '(')
	return strcmp_iw_ordered (string1, string2);
      else
	{
	  if (casing == case_sensitive_off)
	    return tolower (*string1) - tolower (*string2);
	  else
	    return *string1 - *string2;
	}
    }
}

/* Compare STRING1 to STRING2, with results as for strcmp.
   Compatible with strcmp_iw_ordered in that...

       strcmp_iw_ordered (STRING1, STRING2) <= 0

   ... implies...

       compare_names (STRING1, STRING2) <= 0

   (they may differ as to what symbols compare equal).  */

static int
compare_names (const char *string1, const char *string2)
{
  int result;

  /* Similar to what strcmp_iw_ordered does, we need to perform
     a case-insensitive comparison first, and only resort to
     a second, case-sensitive, comparison if the first one was
     not sufficient to differentiate the two strings.  */

  result = compare_names_with_case (string1, string2, case_sensitive_off);
  if (result == 0)
    result = compare_names_with_case (string1, string2, case_sensitive_on);

  return result;
}

/* Add to OBSTACKP all non-local symbols whose name and domain match
   NAME and DOMAIN respectively.  The search is performed on GLOBAL_BLOCK
   symbols if GLOBAL is non-zero, or on STATIC_BLOCK symbols otherwise.  */

static void
add_nonlocal_symbols (struct obstack *obstackp, const char *name,
		      domain_enum domain, int global,
		      int is_wild_match)
{
  struct objfile *objfile;
  struct compunit_symtab *cu;
  struct match_data data;

  memset (&data, 0, sizeof data);
  data.obstackp = obstackp;

  ALL_OBJFILES (objfile)
    {
      data.objfile = objfile;

      if (is_wild_match)
	objfile->sf->qf->map_matching_symbols (objfile, name, domain, global,
					       aux_add_nonlocal_symbols, &data,
					       wild_match, NULL);
      else
	objfile->sf->qf->map_matching_symbols (objfile, name, domain, global,
					       aux_add_nonlocal_symbols, &data,
					       full_match, compare_names);

      ALL_OBJFILE_COMPUNITS (objfile, cu)
	{
	  const struct block *global_block
	    = BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (cu), GLOBAL_BLOCK);

	  if (ada_add_block_renamings (obstackp, global_block , name, domain,
				       is_wild_match))
	    data.found_sym = 1;
	}
    }

  if (num_defns_collected (obstackp) == 0 && global && !is_wild_match)
    {
      ALL_OBJFILES (objfile)
        {
	  char *name1 = (char *) alloca (strlen (name) + sizeof ("_ada_"));
	  strcpy (name1, "_ada_");
	  strcpy (name1 + sizeof ("_ada_") - 1, name);
	  data.objfile = objfile;
	  objfile->sf->qf->map_matching_symbols (objfile, name1, domain,
						 global,
						 aux_add_nonlocal_symbols,
						 &data,
						 full_match, compare_names);
	}
    }      	
}

/* Find symbols in DOMAIN matching NAME, in BLOCK and, if FULL_SEARCH is
   non-zero, enclosing scope and in global scopes, returning the number of
   matches.  Add these to OBSTACKP.

   When FULL_SEARCH is non-zero, any non-function/non-enumeral
   symbol match within the nest of blocks whose innermost member is BLOCK,
   is the one match returned (no other matches in that or
   enclosing blocks is returned).  If there are any matches in or
   surrounding BLOCK, then these alone are returned.

   Names prefixed with "standard__" are handled specially: "standard__"
   is first stripped off, and only static and global symbols are searched.

   If MADE_GLOBAL_LOOKUP_P is non-null, set it before return to whether we had
   to lookup global symbols.  */

static void
ada_add_all_symbols (struct obstack *obstackp,
		     const struct block *block,
		     const char *name,
		     domain_enum domain,
		     int full_search,
		     int *made_global_lookup_p)
{
  struct symbol *sym;
  const int wild_match_p = should_use_wild_match (name);

  if (made_global_lookup_p)
    *made_global_lookup_p = 0;

  /* Special case: If the user specifies a symbol name inside package
     Standard, do a non-wild matching of the symbol name without
     the "standard__" prefix.  This was primarily introduced in order
     to allow the user to specifically access the standard exceptions
     using, for instance, Standard.Constraint_Error when Constraint_Error
     is ambiguous (due to the user defining its own Constraint_Error
     entity inside its program).  */
  if (startswith (name, "standard__"))
    {
      block = NULL;
      name = name + sizeof ("standard__") - 1;
    }

  /* Check the non-global symbols.  If we have ANY match, then we're done.  */

  if (block != NULL)
    {
      if (full_search)
	ada_add_local_symbols (obstackp, name, block, domain, wild_match_p);
      else
	{
	  /* In the !full_search case we're are being called by
	     ada_iterate_over_symbols, and we don't want to search
	     superblocks.  */
	  ada_add_block_symbols (obstackp, block, name, domain, NULL,
				 wild_match_p);
	}
      if (num_defns_collected (obstackp) > 0 || !full_search)
	return;
    }

  /* No non-global symbols found.  Check our cache to see if we have
     already performed this search before.  If we have, then return
     the same result.  */

  if (lookup_cached_symbol (name, domain, &sym, &block))
    {
      if (sym != NULL)
        add_defn_to_vec (obstackp, sym, block);
      return;
    }

  if (made_global_lookup_p)
    *made_global_lookup_p = 1;

  /* Search symbols from all global blocks.  */
 
  add_nonlocal_symbols (obstackp, name, domain, 1, wild_match_p);

  /* Now add symbols from all per-file blocks if we've gotten no hits
     (not strictly correct, but perhaps better than an error).  */

  if (num_defns_collected (obstackp) == 0)
    add_nonlocal_symbols (obstackp, name, domain, 0, wild_match_p);
}

/* Find symbols in DOMAIN matching NAME, in BLOCK and, if full_search is
   non-zero, enclosing scope and in global scopes, returning the number of
   matches.
   Sets *RESULTS to point to a vector of (SYM,BLOCK) tuples,
   indicating the symbols found and the blocks and symbol tables (if
   any) in which they were found.  This vector is transient---good only to
   the next call of ada_lookup_symbol_list.

   When full_search is non-zero, any non-function/non-enumeral
   symbol match within the nest of blocks whose innermost member is BLOCK,
   is the one match returned (no other matches in that or
   enclosing blocks is returned).  If there are any matches in or
   surrounding BLOCK, then these alone are returned.

   Names prefixed with "standard__" are handled specially: "standard__"
   is first stripped off, and only static and global symbols are searched.  */

static int
ada_lookup_symbol_list_worker (const char *name, const struct block *block,
			       domain_enum domain,
			       struct block_symbol **results,
			       int full_search)
{
  const int wild_match_p = should_use_wild_match (name);
  int syms_from_global_search;
  int ndefns;

  obstack_free (&symbol_list_obstack, NULL);
  obstack_init (&symbol_list_obstack);
  ada_add_all_symbols (&symbol_list_obstack, block, name, domain,
		       full_search, &syms_from_global_search);

  ndefns = num_defns_collected (&symbol_list_obstack);
  *results = defns_collected (&symbol_list_obstack, 1);

  ndefns = remove_extra_symbols (*results, ndefns);

  if (ndefns == 0 && full_search && syms_from_global_search)
    cache_symbol (name, domain, NULL, NULL);

  if (ndefns == 1 && full_search && syms_from_global_search)
    cache_symbol (name, domain, (*results)[0].symbol, (*results)[0].block);

  ndefns = remove_irrelevant_renamings (*results, ndefns, block);
  return ndefns;
}

/* Find symbols in DOMAIN matching NAME0, in BLOCK0 and enclosing scope and
   in global scopes, returning the number of matches, and setting *RESULTS
   to a vector of (SYM,BLOCK) tuples.
   See ada_lookup_symbol_list_worker for further details.  */

int
ada_lookup_symbol_list (const char *name0, const struct block *block0,
			domain_enum domain, struct block_symbol **results)
{
  return ada_lookup_symbol_list_worker (name0, block0, domain, results, 1);
}

/* Implementation of the la_iterate_over_symbols method.  */

static void
ada_iterate_over_symbols (const struct block *block,
			  const char *name, domain_enum domain,
			  symbol_found_callback_ftype *callback,
			  void *data)
{
  int ndefs, i;
  struct block_symbol *results;

  ndefs = ada_lookup_symbol_list_worker (name, block, domain, &results, 0);
  for (i = 0; i < ndefs; ++i)
    {
      if (! (*callback) (results[i].symbol, data))
	break;
    }
}

/* If NAME is the name of an entity, return a string that should
   be used to look that entity up in Ada units.  This string should
   be deallocated after use using xfree.

   NAME can have any form that the "break" or "print" commands might
   recognize.  In other words, it does not have to be the "natural"
   name, or the "encoded" name.  */

char *
ada_name_for_lookup (const char *name)
{
  char *canon;
  int nlen = strlen (name);

  if (name[0] == '<' && name[nlen - 1] == '>')
    {
      canon = (char *) xmalloc (nlen - 1);
      memcpy (canon, name + 1, nlen - 2);
      canon[nlen - 2] = '\0';
    }
  else
    canon = xstrdup (ada_encode (ada_fold_name (name)));
  return canon;
}

/* The result is as for ada_lookup_symbol_list with FULL_SEARCH set
   to 1, but choosing the first symbol found if there are multiple
   choices.

   The result is stored in *INFO, which must be non-NULL.
   If no match is found, INFO->SYM is set to NULL.  */

void
ada_lookup_encoded_symbol (const char *name, const struct block *block,
			   domain_enum domain,
			   struct block_symbol *info)
{
  struct block_symbol *candidates;
  int n_candidates;

  gdb_assert (info != NULL);
  memset (info, 0, sizeof (struct block_symbol));

  n_candidates = ada_lookup_symbol_list (name, block, domain, &candidates);
  if (n_candidates == 0)
    return;

  *info = candidates[0];
  info->symbol = fixup_symbol_section (info->symbol, NULL);
}

/* Return a symbol in DOMAIN matching NAME, in BLOCK0 and enclosing
   scope and in global scopes, or NULL if none.  NAME is folded and
   encoded first.  Otherwise, the result is as for ada_lookup_symbol_list,
   choosing the first symbol if there are multiple choices.
   If IS_A_FIELD_OF_THIS is not NULL, it is set to zero.  */

struct block_symbol
ada_lookup_symbol (const char *name, const struct block *block0,
                   domain_enum domain, int *is_a_field_of_this)
{
  struct block_symbol info;

  if (is_a_field_of_this != NULL)
    *is_a_field_of_this = 0;

  ada_lookup_encoded_symbol (ada_encode (ada_fold_name (name)),
			     block0, domain, &info);
  return info;
}

static struct block_symbol
ada_lookup_symbol_nonlocal (const struct language_defn *langdef,
			    const char *name,
                            const struct block *block,
                            const domain_enum domain)
{
  struct block_symbol sym;

  sym = ada_lookup_symbol (name, block_static_block (block), domain, NULL);
  if (sym.symbol != NULL)
    return sym;

  /* If we haven't found a match at this point, try the primitive
     types.  In other languages, this search is performed before
     searching for global symbols in order to short-circuit that
     global-symbol search if it happens that the name corresponds
     to a primitive type.  But we cannot do the same in Ada, because
     it is perfectly legitimate for a program to declare a type which
     has the same name as a standard type.  If looking up a type in
     that situation, we have traditionally ignored the primitive type
     in favor of user-defined types.  This is why, unlike most other
     languages, we search the primitive types this late and only after
     having searched the global symbols without success.  */

  if (domain == VAR_DOMAIN)
    {
      struct gdbarch *gdbarch;

      if (block == NULL)
	gdbarch = target_gdbarch ();
      else
	gdbarch = block_gdbarch (block);
      sym.symbol = language_lookup_primitive_type_as_symbol (langdef, gdbarch, name);
      if (sym.symbol != NULL)
	return sym;
    }

  return (struct block_symbol) {NULL, NULL};
}


/* True iff STR is a possible encoded suffix of a normal Ada name
   that is to be ignored for matching purposes.  Suffixes of parallel
   names (e.g., XVE) are not included here.  Currently, the possible suffixes
   are given by any of the regular expressions:

   [.$][0-9]+       [nested subprogram suffix, on platforms such as GNU/Linux]
   ___[0-9]+        [nested subprogram suffix, on platforms such as HP/UX]
   TKB              [subprogram suffix for task bodies]
   _E[0-9]+[bs]$    [protected object entry suffixes]
   (X[nb]*)?((\$|__)[0-9](_?[0-9]+)|___(JM|LJM|X([FDBUP].*|R[^T]?)))?$

   Also, any leading "__[0-9]+" sequence is skipped before the suffix
   match is performed.  This sequence is used to differentiate homonyms,
   is an optional part of a valid name suffix.  */

static int
is_name_suffix (const char *str)
{
  int k;
  const char *matching;
  const int len = strlen (str);

  /* Skip optional leading __[0-9]+.  */

  if (len > 3 && str[0] == '_' && str[1] == '_' && isdigit (str[2]))
    {
      str += 3;
      while (isdigit (str[0]))
        str += 1;
    }
  
  /* [.$][0-9]+ */

  if (str[0] == '.' || str[0] == '$')
    {
      matching = str + 1;
      while (isdigit (matching[0]))
        matching += 1;
      if (matching[0] == '\0')
        return 1;
    }

  /* ___[0-9]+ */

  if (len > 3 && str[0] == '_' && str[1] == '_' && str[2] == '_')
    {
      matching = str + 3;
      while (isdigit (matching[0]))
        matching += 1;
      if (matching[0] == '\0')
        return 1;
    }

  /* "TKB" suffixes are used for subprograms implementing task bodies.  */

  if (strcmp (str, "TKB") == 0)
    return 1;

#if 0
  /* FIXME: brobecker/2005-09-23: Protected Object subprograms end
     with a N at the end.  Unfortunately, the compiler uses the same
     convention for other internal types it creates.  So treating
     all entity names that end with an "N" as a name suffix causes
     some regressions.  For instance, consider the case of an enumerated
     type.  To support the 'Image attribute, it creates an array whose
     name ends with N.
     Having a single character like this as a suffix carrying some
     information is a bit risky.  Perhaps we should change the encoding
     to be something like "_N" instead.  In the meantime, do not do
     the following check.  */
  /* Protected Object Subprograms */
  if (len == 1 && str [0] == 'N')
    return 1;
#endif

  /* _E[0-9]+[bs]$ */
  if (len > 3 && str[0] == '_' && str [1] == 'E' && isdigit (str[2]))
    {
      matching = str + 3;
      while (isdigit (matching[0]))
        matching += 1;
      if ((matching[0] == 'b' || matching[0] == 's')
          && matching [1] == '\0')
        return 1;
    }

  /* ??? We should not modify STR directly, as we are doing below.  This
     is fine in this case, but may become problematic later if we find
     that this alternative did not work, and want to try matching
     another one from the begining of STR.  Since we modified it, we
     won't be able to find the begining of the string anymore!  */
  if (str[0] == 'X')
    {
      str += 1;
      while (str[0] != '_' && str[0] != '\0')
        {
          if (str[0] != 'n' && str[0] != 'b')
            return 0;
          str += 1;
        }
    }

  if (str[0] == '\000')
    return 1;

  if (str[0] == '_')
    {
      if (str[1] != '_' || str[2] == '\000')
        return 0;
      if (str[2] == '_')
        {
          if (strcmp (str + 3, "JM") == 0)
            return 1;
          /* FIXME: brobecker/2004-09-30: GNAT will soon stop using
             the LJM suffix in favor of the JM one.  But we will
             still accept LJM as a valid suffix for a reasonable
             amount of time, just to allow ourselves to debug programs
             compiled using an older version of GNAT.  */
          if (strcmp (str + 3, "LJM") == 0)
            return 1;
          if (str[3] != 'X')
            return 0;
          if (str[4] == 'F' || str[4] == 'D' || str[4] == 'B'
              || str[4] == 'U' || str[4] == 'P')
            return 1;
          if (str[4] == 'R' && str[5] != 'T')
            return 1;
          return 0;
        }
      if (!isdigit (str[2]))
        return 0;
      for (k = 3; str[k] != '\0'; k += 1)
        if (!isdigit (str[k]) && str[k] != '_')
          return 0;
      return 1;
    }
  if (str[0] == '$' && isdigit (str[1]))
    {
      for (k = 2; str[k] != '\0'; k += 1)
        if (!isdigit (str[k]) && str[k] != '_')
          return 0;
      return 1;
    }
  return 0;
}

/* Return non-zero if the string starting at NAME and ending before
   NAME_END contains no capital letters.  */

static int
is_valid_name_for_wild_match (const char *name0)
{
  const char *decoded_name = ada_decode (name0);
  int i;

  /* If the decoded name starts with an angle bracket, it means that
     NAME0 does not follow the GNAT encoding format.  It should then
     not be allowed as a possible wild match.  */
  if (decoded_name[0] == '<')
    return 0;

  for (i=0; decoded_name[i] != '\0'; i++)
    if (isalpha (decoded_name[i]) && !islower (decoded_name[i]))
      return 0;

  return 1;
}

/* Advance *NAMEP to next occurrence of TARGET0 in the string NAME0
   that could start a simple name.  Assumes that *NAMEP points into
   the string beginning at NAME0.  */

static int
advance_wild_match (const char **namep, const char *name0, int target0)
{
  const char *name = *namep;

  while (1)
    {
      int t0, t1;

      t0 = *name;
      if (t0 == '_')
	{
	  t1 = name[1];
	  if ((t1 >= 'a' && t1 <= 'z') || (t1 >= '0' && t1 <= '9'))
	    {
	      name += 1;
	      if (name == name0 + 5 && startswith (name0, "_ada"))
		break;
	      else
		name += 1;
	    }
	  else if (t1 == '_' && ((name[2] >= 'a' && name[2] <= 'z')
				 || name[2] == target0))
	    {
	      name += 2;
	      break;
	    }
	  else
	    return 0;
	}
      else if ((t0 >= 'a' && t0 <= 'z') || (t0 >= '0' && t0 <= '9'))
	name += 1;
      else
	return 0;
    }

  *namep = name;
  return 1;
}

/* Return 0 iff NAME encodes a name of the form prefix.PATN.  Ignores any
   informational suffixes of NAME (i.e., for which is_name_suffix is
   true).  Assumes that PATN is a lower-cased Ada simple name.  */

static int
wild_match (const char *name, const char *patn)
{
  const char *p;
  const char *name0 = name;

  while (1)
    {
      const char *match = name;

      if (*name == *patn)
	{
	  for (name += 1, p = patn + 1; *p != '\0'; name += 1, p += 1)
	    if (*p != *name)
	      break;
	  if (*p == '\0' && is_name_suffix (name))
	    return match != name0 && !is_valid_name_for_wild_match (name0);

	  if (name[-1] == '_')
	    name -= 1;
	}
      if (!advance_wild_match (&name, name0, *patn))
	return 1;
    }
}

/* Returns 0 iff symbol name SYM_NAME matches SEARCH_NAME, apart from
   informational suffix.  */

static int
full_match (const char *sym_name, const char *search_name)
{
  return !match_name (sym_name, search_name, 0);
}


/* Add symbols from BLOCK matching identifier NAME in DOMAIN to
   vector *defn_symbols, updating the list of symbols in OBSTACKP 
   (if necessary).  If WILD, treat as NAME with a wildcard prefix.
   OBJFILE is the section containing BLOCK.  */

static void
ada_add_block_symbols (struct obstack *obstackp,
                       const struct block *block, const char *name,
                       domain_enum domain, struct objfile *objfile,
                       int wild)
{
  struct block_iterator iter;
  int name_len = strlen (name);
  /* A matching argument symbol, if any.  */
  struct symbol *arg_sym;
  /* Set true when we find a matching non-argument symbol.  */
  int found_sym;
  struct symbol *sym;

  arg_sym = NULL;
  found_sym = 0;
  if (wild)
    {
      for (sym = block_iter_match_first (block, name, wild_match, &iter);
	   sym != NULL; sym = block_iter_match_next (name, wild_match, &iter))
      {
        if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
                                   SYMBOL_DOMAIN (sym), domain)
            && wild_match (SYMBOL_LINKAGE_NAME (sym), name) == 0)
          {
	    if (SYMBOL_CLASS (sym) == LOC_UNRESOLVED)
	      continue;
	    else if (SYMBOL_IS_ARGUMENT (sym))
	      arg_sym = sym;
	    else
	      {
                found_sym = 1;
                add_defn_to_vec (obstackp,
                                 fixup_symbol_section (sym, objfile),
                                 block);
              }
          }
      }
    }
  else
    {
     for (sym = block_iter_match_first (block, name, full_match, &iter);
	  sym != NULL; sym = block_iter_match_next (name, full_match, &iter))
      {
        if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
                                   SYMBOL_DOMAIN (sym), domain))
          {
	    if (SYMBOL_CLASS (sym) != LOC_UNRESOLVED)
	      {
		if (SYMBOL_IS_ARGUMENT (sym))
		  arg_sym = sym;
		else
		  {
		    found_sym = 1;
		    add_defn_to_vec (obstackp,
				     fixup_symbol_section (sym, objfile),
				     block);
		  }
	      }
          }
      }
    }

  /* Handle renamings.  */

  if (ada_add_block_renamings (obstackp, block, name, domain, wild))
    found_sym = 1;

  if (!found_sym && arg_sym != NULL)
    {
      add_defn_to_vec (obstackp,
                       fixup_symbol_section (arg_sym, objfile),
                       block);
    }

  if (!wild)
    {
      arg_sym = NULL;
      found_sym = 0;

      ALL_BLOCK_SYMBOLS (block, iter, sym)
      {
        if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
                                   SYMBOL_DOMAIN (sym), domain))
          {
            int cmp;

            cmp = (int) '_' - (int) SYMBOL_LINKAGE_NAME (sym)[0];
            if (cmp == 0)
              {
                cmp = !startswith (SYMBOL_LINKAGE_NAME (sym), "_ada_");
                if (cmp == 0)
                  cmp = strncmp (name, SYMBOL_LINKAGE_NAME (sym) + 5,
                                 name_len);
              }

            if (cmp == 0
                && is_name_suffix (SYMBOL_LINKAGE_NAME (sym) + name_len + 5))
              {
		if (SYMBOL_CLASS (sym) != LOC_UNRESOLVED)
		  {
		    if (SYMBOL_IS_ARGUMENT (sym))
		      arg_sym = sym;
		    else
		      {
			found_sym = 1;
			add_defn_to_vec (obstackp,
					 fixup_symbol_section (sym, objfile),
					 block);
		      }
		  }
              }
          }
      }

      /* NOTE: This really shouldn't be needed for _ada_ symbols.
         They aren't parameters, right?  */
      if (!found_sym && arg_sym != NULL)
        {
          add_defn_to_vec (obstackp,
                           fixup_symbol_section (arg_sym, objfile),
                           block);
        }
    }
}


                                /* Symbol Completion */

/* If SYM_NAME is a completion candidate for TEXT, return this symbol
   name in a form that's appropriate for the completion.  The result
   does not need to be deallocated, but is only good until the next call.

   TEXT_LEN is equal to the length of TEXT.
   Perform a wild match if WILD_MATCH_P is set.
   ENCODED_P should be set if TEXT represents the start of a symbol name
   in its encoded form.  */

static const char *
symbol_completion_match (const char *sym_name,
                         const char *text, int text_len,
                         int wild_match_p, int encoded_p)
{
  const int verbatim_match = (text[0] == '<');
  int match = 0;

  if (verbatim_match)
    {
      /* Strip the leading angle bracket.  */
      text = text + 1;
      text_len--;
    }

  /* First, test against the fully qualified name of the symbol.  */

  if (strncmp (sym_name, text, text_len) == 0)
    match = 1;

  if (match && !encoded_p)
    {
      /* One needed check before declaring a positive match is to verify
         that iff we are doing a verbatim match, the decoded version
         of the symbol name starts with '<'.  Otherwise, this symbol name
         is not a suitable completion.  */
      const char *sym_name_copy = sym_name;
      int has_angle_bracket;

      sym_name = ada_decode (sym_name);
      has_angle_bracket = (sym_name[0] == '<');
      match = (has_angle_bracket == verbatim_match);
      sym_name = sym_name_copy;
    }

  if (match && !verbatim_match)
    {
      /* When doing non-verbatim match, another check that needs to
         be done is to verify that the potentially matching symbol name
         does not include capital letters, because the ada-mode would
         not be able to understand these symbol names without the
         angle bracket notation.  */
      const char *tmp;

      for (tmp = sym_name; *tmp != '\0' && !isupper (*tmp); tmp++);
      if (*tmp != '\0')
        match = 0;
    }

  /* Second: Try wild matching...  */

  if (!match && wild_match_p)
    {
      /* Since we are doing wild matching, this means that TEXT
         may represent an unqualified symbol name.  We therefore must
         also compare TEXT against the unqualified name of the symbol.  */
      sym_name = ada_unqualified_name (ada_decode (sym_name));

      if (strncmp (sym_name, text, text_len) == 0)
        match = 1;
    }

  /* Finally: If we found a mach, prepare the result to return.  */

  if (!match)
    return NULL;

  if (verbatim_match)
    sym_name = add_angle_brackets (sym_name);

  if (!encoded_p)
    sym_name = ada_decode (sym_name);

  return sym_name;
}

/* A companion function to ada_make_symbol_completion_list().
   Check if SYM_NAME represents a symbol which name would be suitable
   to complete TEXT (TEXT_LEN is the length of TEXT), in which case
   it is appended at the end of the given string vector SV.

   ORIG_TEXT is the string original string from the user command
   that needs to be completed.  WORD is the entire command on which
   completion should be performed.  These two parameters are used to
   determine which part of the symbol name should be added to the
   completion vector.
   if WILD_MATCH_P is set, then wild matching is performed.
   ENCODED_P should be set if TEXT represents a symbol name in its
   encoded formed (in which case the completion should also be
   encoded).  */

static void
symbol_completion_add (VEC(char_ptr) **sv,
                       const char *sym_name,
                       const char *text, int text_len,
                       const char *orig_text, const char *word,
                       int wild_match_p, int encoded_p)
{
  const char *match = symbol_completion_match (sym_name, text, text_len,
                                               wild_match_p, encoded_p);
  char *completion;

  if (match == NULL)
    return;

  /* We found a match, so add the appropriate completion to the given
     string vector.  */

  if (word == orig_text)
    {
      completion = (char *) xmalloc (strlen (match) + 5);
      strcpy (completion, match);
    }
  else if (word > orig_text)
    {
      /* Return some portion of sym_name.  */
      completion = (char *) xmalloc (strlen (match) + 5);
      strcpy (completion, match + (word - orig_text));
    }
  else
    {
      /* Return some of ORIG_TEXT plus sym_name.  */
      completion = (char *) xmalloc (strlen (match) + (orig_text - word) + 5);
      strncpy (completion, word, orig_text - word);
      completion[orig_text - word] = '\0';
      strcat (completion, match);
    }

  VEC_safe_push (char_ptr, *sv, completion);
}

/* An object of this type is passed as the user_data argument to the
   expand_symtabs_matching method.  */
struct add_partial_datum
{
  VEC(char_ptr) **completions;
  const char *text;
  int text_len;
  const char *text0;
  const char *word;
  int wild_match;
  int encoded;
};

/* A callback for expand_symtabs_matching.  */

static int
ada_complete_symbol_matcher (const char *name, void *user_data)
{
  struct add_partial_datum *data = (struct add_partial_datum *) user_data;
  
  return symbol_completion_match (name, data->text, data->text_len,
                                  data->wild_match, data->encoded) != NULL;
}

/* Return a list of possible symbol names completing TEXT0.  WORD is
   the entire command on which completion is made.  */

static VEC (char_ptr) *
ada_make_symbol_completion_list (const char *text0, const char *word,
				 enum type_code code)
{
  char *text;
  int text_len;
  int wild_match_p;
  int encoded_p;
  VEC(char_ptr) *completions = VEC_alloc (char_ptr, 128);
  struct symbol *sym;
  struct compunit_symtab *s;
  struct minimal_symbol *msymbol;
  struct objfile *objfile;
  const struct block *b, *surrounding_static_block = 0;
  int i;
  struct block_iterator iter;
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);

  gdb_assert (code == TYPE_CODE_UNDEF);

  if (text0[0] == '<')
    {
      text = xstrdup (text0);
      make_cleanup (xfree, text);
      text_len = strlen (text);
      wild_match_p = 0;
      encoded_p = 1;
    }
  else
    {
      text = xstrdup (ada_encode (text0));
      make_cleanup (xfree, text);
      text_len = strlen (text);
      for (i = 0; i < text_len; i++)
        text[i] = tolower (text[i]);

      encoded_p = (strstr (text0, "__") != NULL);
      /* If the name contains a ".", then the user is entering a fully
         qualified entity name, and the match must not be done in wild
         mode.  Similarly, if the user wants to complete what looks like
         an encoded name, the match must not be done in wild mode.  */
      wild_match_p = (strchr (text0, '.') == NULL && !encoded_p);
    }

  /* First, look at the partial symtab symbols.  */
  {
    struct add_partial_datum data;

    data.completions = &completions;
    data.text = text;
    data.text_len = text_len;
    data.text0 = text0;
    data.word = word;
    data.wild_match = wild_match_p;
    data.encoded = encoded_p;
    expand_symtabs_matching (NULL, ada_complete_symbol_matcher, NULL,
			     ALL_DOMAIN, &data);
  }

  /* At this point scan through the misc symbol vectors and add each
     symbol you find to the list.  Eventually we want to ignore
     anything that isn't a text symbol (everything else will be
     handled by the psymtab code above).  */

  ALL_MSYMBOLS (objfile, msymbol)
  {
    QUIT;
    symbol_completion_add (&completions, MSYMBOL_LINKAGE_NAME (msymbol),
			   text, text_len, text0, word, wild_match_p,
			   encoded_p);
  }

  /* Search upwards from currently selected frame (so that we can
     complete on local vars.  */

  for (b = get_selected_block (0); b != NULL; b = BLOCK_SUPERBLOCK (b))
    {
      if (!BLOCK_SUPERBLOCK (b))
        surrounding_static_block = b;   /* For elmin of dups */

      ALL_BLOCK_SYMBOLS (b, iter, sym)
      {
        symbol_completion_add (&completions, SYMBOL_LINKAGE_NAME (sym),
                               text, text_len, text0, word,
                               wild_match_p, encoded_p);
      }
    }

  /* Go through the symtabs and check the externs and statics for
     symbols which match.  */

  ALL_COMPUNITS (objfile, s)
  {
    QUIT;
    b = BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (s), GLOBAL_BLOCK);
    ALL_BLOCK_SYMBOLS (b, iter, sym)
    {
      symbol_completion_add (&completions, SYMBOL_LINKAGE_NAME (sym),
                             text, text_len, text0, word,
                             wild_match_p, encoded_p);
    }
  }

  ALL_COMPUNITS (objfile, s)
  {
    QUIT;
    b = BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (s), STATIC_BLOCK);
    /* Don't do this block twice.  */
    if (b == surrounding_static_block)
      continue;
    ALL_BLOCK_SYMBOLS (b, iter, sym)
    {
      symbol_completion_add (&completions, SYMBOL_LINKAGE_NAME (sym),
                             text, text_len, text0, word,
                             wild_match_p, encoded_p);
    }
  }

  do_cleanups (old_chain);
  return completions;
}

                                /* Field Access */

/* Return non-zero if TYPE is a pointer to the GNAT dispatch table used
   for tagged types.  */

static int
ada_is_dispatch_table_ptr_type (struct type *type)
{
  const char *name;

  if (TYPE_CODE (type) != TYPE_CODE_PTR)
    return 0;

  name = TYPE_NAME (TYPE_TARGET_TYPE (type));
  if (name == NULL)
    return 0;

  return (strcmp (name, "ada__tags__dispatch_table") == 0);
}

/* Return non-zero if TYPE is an interface tag.  */

static int
ada_is_interface_tag (struct type *type)
{
  const char *name = TYPE_NAME (type);

  if (name == NULL)
    return 0;

  return (strcmp (name, "ada__tags__interface_tag") == 0);
}

/* True if field number FIELD_NUM in struct or union type TYPE is supposed
   to be invisible to users.  */

int
ada_is_ignored_field (struct type *type, int field_num)
{
  if (field_num < 0 || field_num > TYPE_NFIELDS (type))
    return 1;

  /* Check the name of that field.  */
  {
    const char *name = TYPE_FIELD_NAME (type, field_num);

    /* Anonymous field names should not be printed.
       brobecker/2007-02-20: I don't think this can actually happen
       but we don't want to print the value of annonymous fields anyway.  */
    if (name == NULL)
      return 1;

    /* Normally, fields whose name start with an underscore ("_")
       are fields that have been internally generated by the compiler,
       and thus should not be printed.  The "_parent" field is special,
       however: This is a field internally generated by the compiler
       for tagged types, and it contains the components inherited from
       the parent type.  This field should not be printed as is, but
       should not be ignored either.  */
    if (name[0] == '_' && !startswith (name, "_parent"))
      return 1;
  }

  /* If this is the dispatch table of a tagged type or an interface tag,
     then ignore.  */
  if (ada_is_tagged_type (type, 1)
      && (ada_is_dispatch_table_ptr_type (TYPE_FIELD_TYPE (type, field_num))
	  || ada_is_interface_tag (TYPE_FIELD_TYPE (type, field_num))))
    return 1;

  /* Not a special field, so it should not be ignored.  */
  return 0;
}

/* True iff TYPE has a tag field.  If REFOK, then TYPE may also be a
   pointer or reference type whose ultimate target has a tag field.  */

int
ada_is_tagged_type (struct type *type, int refok)
{
  return (ada_lookup_struct_elt_type (type, "_tag", refok, 1, NULL) != NULL);
}

/* True iff TYPE represents the type of X'Tag */

int
ada_is_tag_type (struct type *type)
{
  type = ada_check_typedef (type);

  if (type == NULL || TYPE_CODE (type) != TYPE_CODE_PTR)
    return 0;
  else
    {
      const char *name = ada_type_name (TYPE_TARGET_TYPE (type));

      return (name != NULL
              && strcmp (name, "ada__tags__dispatch_table") == 0);
    }
}

/* The type of the tag on VAL.  */

struct type *
ada_tag_type (struct value *val)
{
  return ada_lookup_struct_elt_type (value_type (val), "_tag", 1, 0, NULL);
}

/* Return 1 if TAG follows the old scheme for Ada tags (used for Ada 95,
   retired at Ada 05).  */

static int
is_ada95_tag (struct value *tag)
{
  return ada_value_struct_elt (tag, "tsd", 1) != NULL;
}

/* The value of the tag on VAL.  */

struct value *
ada_value_tag (struct value *val)
{
  return ada_value_struct_elt (val, "_tag", 0);
}

/* The value of the tag on the object of type TYPE whose contents are
   saved at VALADDR, if it is non-null, or is at memory address
   ADDRESS.  */

static struct value *
value_tag_from_contents_and_address (struct type *type,
				     const gdb_byte *valaddr,
                                     CORE_ADDR address)
{
  int tag_byte_offset;
  struct type *tag_type;

  if (find_struct_field ("_tag", type, 0, &tag_type, &tag_byte_offset,
                         NULL, NULL, NULL))
    {
      const gdb_byte *valaddr1 = ((valaddr == NULL)
				  ? NULL
				  : valaddr + tag_byte_offset);
      CORE_ADDR address1 = (address == 0) ? 0 : address + tag_byte_offset;

      return value_from_contents_and_address (tag_type, valaddr1, address1);
    }
  return NULL;
}

static struct type *
type_from_tag (struct value *tag)
{
  const char *type_name = ada_tag_name (tag);

  if (type_name != NULL)
    return ada_find_any_type (ada_encode (type_name));
  return NULL;
}

/* Given a value OBJ of a tagged type, return a value of this
   type at the base address of the object.  The base address, as
   defined in Ada.Tags, it is the address of the primary tag of
   the object, and therefore where the field values of its full
   view can be fetched.  */

struct value *
ada_tag_value_at_base_address (struct value *obj)
{
  struct value *val;
  LONGEST offset_to_top = 0;
  struct type *ptr_type, *obj_type;
  struct value *tag;
  CORE_ADDR base_address;

  obj_type = value_type (obj);

  /* It is the responsability of the caller to deref pointers.  */

  if (TYPE_CODE (obj_type) == TYPE_CODE_PTR
      || TYPE_CODE (obj_type) == TYPE_CODE_REF)
    return obj;

  tag = ada_value_tag (obj);
  if (!tag)
    return obj;

  /* Base addresses only appeared with Ada 05 and multiple inheritance.  */

  if (is_ada95_tag (tag))
    return obj;

  ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  ptr_type = lookup_pointer_type (ptr_type);
  val = value_cast (ptr_type, tag);
  if (!val)
    return obj;

  /* It is perfectly possible that an exception be raised while
     trying to determine the base address, just like for the tag;
     see ada_tag_name for more details.  We do not print the error
     message for the same reason.  */

  TRY
    {
      offset_to_top = value_as_long (value_ind (value_ptradd (val, -2)));
    }

  CATCH (e, RETURN_MASK_ERROR)
    {
      return obj;
    }
  END_CATCH

  /* If offset is null, nothing to do.  */

  if (offset_to_top == 0)
    return obj;

  /* -1 is a special case in Ada.Tags; however, what should be done
     is not quite clear from the documentation.  So do nothing for
     now.  */

  if (offset_to_top == -1)
    return obj;

  base_address = value_address (obj) - offset_to_top;
  tag = value_tag_from_contents_and_address (obj_type, NULL, base_address);

  /* Make sure that we have a proper tag at the new address.
     Otherwise, offset_to_top is bogus (which can happen when
     the object is not initialized yet).  */

  if (!tag)
    return obj;

  obj_type = type_from_tag (tag);

  if (!obj_type)
    return obj;

  return value_from_contents_and_address (obj_type, NULL, base_address);
}

/* Return the "ada__tags__type_specific_data" type.  */

static struct type *
ada_get_tsd_type (struct inferior *inf)
{
  struct ada_inferior_data *data = get_ada_inferior_data (inf);

  if (data->tsd_type == 0)
    data->tsd_type = ada_find_any_type ("ada__tags__type_specific_data");
  return data->tsd_type;
}

/* Return the TSD (type-specific data) associated to the given TAG.
   TAG is assumed to be the tag of a tagged-type entity.

   May return NULL if we are unable to get the TSD.  */

static struct value *
ada_get_tsd_from_tag (struct value *tag)
{
  struct value *val;
  struct type *type;

  /* First option: The TSD is simply stored as a field of our TAG.
     Only older versions of GNAT would use this format, but we have
     to test it first, because there are no visible markers for
     the current approach except the absence of that field.  */

  val = ada_value_struct_elt (tag, "tsd", 1);
  if (val)
    return val;

  /* Try the second representation for the dispatch table (in which
     there is no explicit 'tsd' field in the referent of the tag pointer,
     and instead the tsd pointer is stored just before the dispatch
     table.  */

  type = ada_get_tsd_type (current_inferior());
  if (type == NULL)
    return NULL;
  type = lookup_pointer_type (lookup_pointer_type (type));
  val = value_cast (type, tag);
  if (val == NULL)
    return NULL;
  return value_ind (value_ptradd (val, -1));
}

/* Given the TSD of a tag (type-specific data), return a string
   containing the name of the associated type.

   The returned value is good until the next call.  May return NULL
   if we are unable to determine the tag name.  */

static char *
ada_tag_name_from_tsd (struct value *tsd)
{
  static char name[1024];
  char *p;
  struct value *val;

  val = ada_value_struct_elt (tsd, "expanded_name", 1);
  if (val == NULL)
    return NULL;
  read_memory_string (value_as_address (val), name, sizeof (name) - 1);
  for (p = name; *p != '\0'; p += 1)
    if (isalpha (*p))
      *p = tolower (*p);
  return name;
}

/* The type name of the dynamic type denoted by the 'tag value TAG, as
   a C string.

   Return NULL if the TAG is not an Ada tag, or if we were unable to
   determine the name of that tag.  The result is good until the next
   call.  */

const char *
ada_tag_name (struct value *tag)
{
  char *name = NULL;

  if (!ada_is_tag_type (value_type (tag)))
    return NULL;

  /* It is perfectly possible that an exception be raised while trying
     to determine the TAG's name, even under normal circumstances:
     The associated variable may be uninitialized or corrupted, for
     instance. We do not let any exception propagate past this point.
     instead we return NULL.

     We also do not print the error message either (which often is very
     low-level (Eg: "Cannot read memory at 0x[...]"), but instead let
     the caller print a more meaningful message if necessary.  */
  TRY
    {
      struct value *tsd = ada_get_tsd_from_tag (tag);

      if (tsd != NULL)
	name = ada_tag_name_from_tsd (tsd);
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
    }
  END_CATCH

  return name;
}

/* The parent type of TYPE, or NULL if none.  */

struct type *
ada_parent_type (struct type *type)
{
  int i;

  type = ada_check_typedef (type);

  if (type == NULL || TYPE_CODE (type) != TYPE_CODE_STRUCT)
    return NULL;

  for (i = 0; i < TYPE_NFIELDS (type); i += 1)
    if (ada_is_parent_field (type, i))
      {
        struct type *parent_type = TYPE_FIELD_TYPE (type, i);

        /* If the _parent field is a pointer, then dereference it.  */
        if (TYPE_CODE (parent_type) == TYPE_CODE_PTR)
          parent_type = TYPE_TARGET_TYPE (parent_type);
        /* If there is a parallel XVS type, get the actual base type.  */
        parent_type = ada_get_base_type (parent_type);

        return ada_check_typedef (parent_type);
      }

  return NULL;
}

/* True iff field number FIELD_NUM of structure type TYPE contains the
   parent-type (inherited) fields of a derived type.  Assumes TYPE is
   a structure type with at least FIELD_NUM+1 fields.  */

int
ada_is_parent_field (struct type *type, int field_num)
{
  const char *name = TYPE_FIELD_NAME (ada_check_typedef (type), field_num);

  return (name != NULL
          && (startswith (name, "PARENT")
              || startswith (name, "_parent")));
}

/* True iff field number FIELD_NUM of structure type TYPE is a
   transparent wrapper field (which should be silently traversed when doing
   field selection and flattened when printing).  Assumes TYPE is a
   structure type with at least FIELD_NUM+1 fields.  Such fields are always
   structures.  */

int
ada_is_wrapper_field (struct type *type, int field_num)
{
  const char *name = TYPE_FIELD_NAME (type, field_num);

  if (name != NULL && strcmp (name, "RETVAL") == 0)
    {
      /* This happens in functions with "out" or "in out" parameters
	 which are passed by copy.  For such functions, GNAT describes
	 the function's return type as being a struct where the return
	 value is in a field called RETVAL, and where the other "out"
	 or "in out" parameters are fields of that struct.  This is not
	 a wrapper.  */
      return 0;
    }

  return (name != NULL
          && (startswith (name, "PARENT")
              || strcmp (name, "REP") == 0
              || startswith (name, "_parent")
              || name[0] == 'S' || name[0] == 'R' || name[0] == 'O'));
}

/* True iff field number FIELD_NUM of structure or union type TYPE
   is a variant wrapper.  Assumes TYPE is a structure type with at least
   FIELD_NUM+1 fields.  */

int
ada_is_variant_part (struct type *type, int field_num)
{
  struct type *field_type = TYPE_FIELD_TYPE (type, field_num);

  return (TYPE_CODE (field_type) == TYPE_CODE_UNION
          || (is_dynamic_field (type, field_num)
              && (TYPE_CODE (TYPE_TARGET_TYPE (field_type)) 
		  == TYPE_CODE_UNION)));
}

/* Assuming that VAR_TYPE is a variant wrapper (type of the variant part)
   whose discriminants are contained in the record type OUTER_TYPE,
   returns the type of the controlling discriminant for the variant.
   May return NULL if the type could not be found.  */

struct type *
ada_variant_discrim_type (struct type *var_type, struct type *outer_type)
{
  char *name = ada_variant_discrim_name (var_type);

  return ada_lookup_struct_elt_type (outer_type, name, 1, 1, NULL);
}

/* Assuming that TYPE is the type of a variant wrapper, and FIELD_NUM is a
   valid field number within it, returns 1 iff field FIELD_NUM of TYPE
   represents a 'when others' clause; otherwise 0.  */

int
ada_is_others_clause (struct type *type, int field_num)
{
  const char *name = TYPE_FIELD_NAME (type, field_num);

  return (name != NULL && name[0] == 'O');
}

/* Assuming that TYPE0 is the type of the variant part of a record,
   returns the name of the discriminant controlling the variant.
   The value is valid until the next call to ada_variant_discrim_name.  */

char *
ada_variant_discrim_name (struct type *type0)
{
  static char *result = NULL;
  static size_t result_len = 0;
  struct type *type;
  const char *name;
  const char *discrim_end;
  const char *discrim_start;

  if (TYPE_CODE (type0) == TYPE_CODE_PTR)
    type = TYPE_TARGET_TYPE (type0);
  else
    type = type0;

  name = ada_type_name (type);

  if (name == NULL || name[0] == '\000')
    return "";

  for (discrim_end = name + strlen (name) - 6; discrim_end != name;
       discrim_end -= 1)
    {
      if (startswith (discrim_end, "___XVN"))
        break;
    }
  if (discrim_end == name)
    return "";

  for (discrim_start = discrim_end; discrim_start != name + 3;
       discrim_start -= 1)
    {
      if (discrim_start == name + 1)
        return "";
      if ((discrim_start > name + 3
           && startswith (discrim_start - 3, "___"))
          || discrim_start[-1] == '.')
        break;
    }

  GROW_VECT (result, result_len, discrim_end - discrim_start + 1);
  strncpy (result, discrim_start, discrim_end - discrim_start);
  result[discrim_end - discrim_start] = '\0';
  return result;
}

/* Scan STR for a subtype-encoded number, beginning at position K.
   Put the position of the character just past the number scanned in
   *NEW_K, if NEW_K!=NULL.  Put the scanned number in *R, if R!=NULL.
   Return 1 if there was a valid number at the given position, and 0
   otherwise.  A "subtype-encoded" number consists of the absolute value
   in decimal, followed by the letter 'm' to indicate a negative number.
   Assumes 0m does not occur.  */

int
ada_scan_number (const char str[], int k, LONGEST * R, int *new_k)
{
  ULONGEST RU;

  if (!isdigit (str[k]))
    return 0;

  /* Do it the hard way so as not to make any assumption about
     the relationship of unsigned long (%lu scan format code) and
     LONGEST.  */
  RU = 0;
  while (isdigit (str[k]))
    {
      RU = RU * 10 + (str[k] - '0');
      k += 1;
    }

  if (str[k] == 'm')
    {
      if (R != NULL)
        *R = (-(LONGEST) (RU - 1)) - 1;
      k += 1;
    }
  else if (R != NULL)
    *R = (LONGEST) RU;

  /* NOTE on the above: Technically, C does not say what the results of
     - (LONGEST) RU or (LONGEST) -RU are for RU == largest positive
     number representable as a LONGEST (although either would probably work
     in most implementations).  When RU>0, the locution in the then branch
     above is always equivalent to the negative of RU.  */

  if (new_k != NULL)
    *new_k = k;
  return 1;
}

/* Assuming that TYPE is a variant part wrapper type (a VARIANTS field),
   and FIELD_NUM is a valid field number within it, returns 1 iff VAL is
   in the range encoded by field FIELD_NUM of TYPE; otherwise 0.  */

int
ada_in_variant (LONGEST val, struct type *type, int field_num)
{
  const char *name = TYPE_FIELD_NAME (type, field_num);
  int p;

  p = 0;
  while (1)
    {
      switch (name[p])
        {
        case '\0':
          return 0;
        case 'S':
          {
            LONGEST W;

            if (!ada_scan_number (name, p + 1, &W, &p))
              return 0;
            if (val == W)
              return 1;
            break;
          }
        case 'R':
          {
            LONGEST L, U;

            if (!ada_scan_number (name, p + 1, &L, &p)
                || name[p] != 'T' || !ada_scan_number (name, p + 1, &U, &p))
              return 0;
            if (val >= L && val <= U)
              return 1;
            break;
          }
        case 'O':
          return 1;
        default:
          return 0;
        }
    }
}

/* FIXME: Lots of redundancy below.  Try to consolidate.  */

/* Given a value ARG1 (offset by OFFSET bytes) of a struct or union type
   ARG_TYPE, extract and return the value of one of its (non-static)
   fields.  FIELDNO says which field.   Differs from value_primitive_field
   only in that it can handle packed values of arbitrary type.  */

static struct value *
ada_value_primitive_field (struct value *arg1, int offset, int fieldno,
                           struct type *arg_type)
{
  struct type *type;

  arg_type = ada_check_typedef (arg_type);
  type = TYPE_FIELD_TYPE (arg_type, fieldno);

  /* Handle packed fields.  */

  if (TYPE_FIELD_BITSIZE (arg_type, fieldno) != 0)
    {
      int bit_pos = TYPE_FIELD_BITPOS (arg_type, fieldno);
      int bit_size = TYPE_FIELD_BITSIZE (arg_type, fieldno);

      return ada_value_primitive_packed_val (arg1, value_contents (arg1),
                                             offset + bit_pos / 8,
                                             bit_pos % 8, bit_size, type);
    }
  else
    return value_primitive_field (arg1, offset, fieldno, arg_type);
}

/* Find field with name NAME in object of type TYPE.  If found, 
   set the following for each argument that is non-null:
    - *FIELD_TYPE_P to the field's type; 
    - *BYTE_OFFSET_P to OFFSET + the byte offset of the field within 
      an object of that type;
    - *BIT_OFFSET_P to the bit offset modulo byte size of the field; 
    - *BIT_SIZE_P to its size in bits if the field is packed, and 
      0 otherwise;
   If INDEX_P is non-null, increment *INDEX_P by the number of source-visible
   fields up to but not including the desired field, or by the total
   number of fields if not found.   A NULL value of NAME never
   matches; the function just counts visible fields in this case.
   
   Returns 1 if found, 0 otherwise.  */

static int
find_struct_field (const char *name, struct type *type, int offset,
                   struct type **field_type_p,
                   int *byte_offset_p, int *bit_offset_p, int *bit_size_p,
		   int *index_p)
{
  int i;

  type = ada_check_typedef (type);

  if (field_type_p != NULL)
    *field_type_p = NULL;
  if (byte_offset_p != NULL)
    *byte_offset_p = 0;
  if (bit_offset_p != NULL)
    *bit_offset_p = 0;
  if (bit_size_p != NULL)
    *bit_size_p = 0;

  for (i = 0; i < TYPE_NFIELDS (type); i += 1)
    {
      int bit_pos = TYPE_FIELD_BITPOS (type, i);
      int fld_offset = offset + bit_pos / 8;
      const char *t_field_name = TYPE_FIELD_NAME (type, i);

      if (t_field_name == NULL)
        continue;

      else if (name != NULL && field_name_match (t_field_name, name))
        {
          int bit_size = TYPE_FIELD_BITSIZE (type, i);

	  if (field_type_p != NULL)
	    *field_type_p = TYPE_FIELD_TYPE (type, i);
	  if (byte_offset_p != NULL)
	    *byte_offset_p = fld_offset;
	  if (bit_offset_p != NULL)
	    *bit_offset_p = bit_pos % 8;
	  if (bit_size_p != NULL)
	    *bit_size_p = bit_size;
          return 1;
        }
      else if (ada_is_wrapper_field (type, i))
        {
	  if (find_struct_field (name, TYPE_FIELD_TYPE (type, i), fld_offset,
				 field_type_p, byte_offset_p, bit_offset_p,
				 bit_size_p, index_p))
            return 1;
        }
      else if (ada_is_variant_part (type, i))
        {
	  /* PNH: Wait.  Do we ever execute this section, or is ARG always of 
	     fixed type?? */
          int j;
          struct type *field_type
	    = ada_check_typedef (TYPE_FIELD_TYPE (type, i));

          for (j = 0; j < TYPE_NFIELDS (field_type); j += 1)
            {
              if (find_struct_field (name, TYPE_FIELD_TYPE (field_type, j),
                                     fld_offset
                                     + TYPE_FIELD_BITPOS (field_type, j) / 8,
                                     field_type_p, byte_offset_p,
                                     bit_offset_p, bit_size_p, index_p))
                return 1;
            }
        }
      else if (index_p != NULL)
	*index_p += 1;
    }
  return 0;
}

/* Number of user-visible fields in record type TYPE.  */

static int
num_visible_fields (struct type *type)
{
  int n;

  n = 0;
  find_struct_field (NULL, type, 0, NULL, NULL, NULL, NULL, &n);
  return n;
}

/* Look for a field NAME in ARG.  Adjust the address of ARG by OFFSET bytes,
   and search in it assuming it has (class) type TYPE.
   If found, return value, else return NULL.

   Searches recursively through wrapper fields (e.g., '_parent').  */

static struct value *
ada_search_struct_field (const char *name, struct value *arg, int offset,
                         struct type *type)
{
  int i;

  type = ada_check_typedef (type);
  for (i = 0; i < TYPE_NFIELDS (type); i += 1)
    {
      const char *t_field_name = TYPE_FIELD_NAME (type, i);

      if (t_field_name == NULL)
        continue;

      else if (field_name_match (t_field_name, name))
        return ada_value_primitive_field (arg, offset, i, type);

      else if (ada_is_wrapper_field (type, i))
        {
          struct value *v =     /* Do not let indent join lines here.  */
            ada_search_struct_field (name, arg,
                                     offset + TYPE_FIELD_BITPOS (type, i) / 8,
                                     TYPE_FIELD_TYPE (type, i));

          if (v != NULL)
            return v;
        }

      else if (ada_is_variant_part (type, i))
        {
	  /* PNH: Do we ever get here?  See find_struct_field.  */
          int j;
          struct type *field_type = ada_check_typedef (TYPE_FIELD_TYPE (type,
									i));
          int var_offset = offset + TYPE_FIELD_BITPOS (type, i) / 8;

          for (j = 0; j < TYPE_NFIELDS (field_type); j += 1)
            {
              struct value *v = ada_search_struct_field /* Force line
							   break.  */
                (name, arg,
                 var_offset + TYPE_FIELD_BITPOS (field_type, j) / 8,
                 TYPE_FIELD_TYPE (field_type, j));

              if (v != NULL)
                return v;
            }
        }
    }
  return NULL;
}

static struct value *ada_index_struct_field_1 (int *, struct value *,
					       int, struct type *);


/* Return field #INDEX in ARG, where the index is that returned by
 * find_struct_field through its INDEX_P argument.  Adjust the address
 * of ARG by OFFSET bytes, and search in it assuming it has (class) type TYPE.
 * If found, return value, else return NULL.  */

static struct value *
ada_index_struct_field (int index, struct value *arg, int offset,
			struct type *type)
{
  return ada_index_struct_field_1 (&index, arg, offset, type);
}


/* Auxiliary function for ada_index_struct_field.  Like
 * ada_index_struct_field, but takes index from *INDEX_P and modifies
 * *INDEX_P.  */

static struct value *
ada_index_struct_field_1 (int *index_p, struct value *arg, int offset,
			  struct type *type)
{
  int i;
  type = ada_check_typedef (type);

  for (i = 0; i < TYPE_NFIELDS (type); i += 1)
    {
      if (TYPE_FIELD_NAME (type, i) == NULL)
        continue;
      else if (ada_is_wrapper_field (type, i))
        {
          struct value *v =     /* Do not let indent join lines here.  */
            ada_index_struct_field_1 (index_p, arg,
				      offset + TYPE_FIELD_BITPOS (type, i) / 8,
				      TYPE_FIELD_TYPE (type, i));

          if (v != NULL)
            return v;
        }

      else if (ada_is_variant_part (type, i))
        {
	  /* PNH: Do we ever get here?  See ada_search_struct_field,
	     find_struct_field.  */
	  error (_("Cannot assign this kind of variant record"));
        }
      else if (*index_p == 0)
        return ada_value_primitive_field (arg, offset, i, type);
      else
	*index_p -= 1;
    }
  return NULL;
}

/* Given ARG, a value of type (pointer or reference to a)*
   structure/union, extract the component named NAME from the ultimate
   target structure/union and return it as a value with its
   appropriate type.

   The routine searches for NAME among all members of the structure itself
   and (recursively) among all members of any wrapper members
   (e.g., '_parent').

   If NO_ERR, then simply return NULL in case of error, rather than 
   calling error.  */

struct value *
ada_value_struct_elt (struct value *arg, char *name, int no_err)
{
  struct type *t, *t1;
  struct value *v;

  v = NULL;
  t1 = t = ada_check_typedef (value_type (arg));
  if (TYPE_CODE (t) == TYPE_CODE_REF)
    {
      t1 = TYPE_TARGET_TYPE (t);
      if (t1 == NULL)
	goto BadValue;
      t1 = ada_check_typedef (t1);
      if (TYPE_CODE (t1) == TYPE_CODE_PTR)
        {
          arg = coerce_ref (arg);
          t = t1;
        }
    }

  while (TYPE_CODE (t) == TYPE_CODE_PTR)
    {
      t1 = TYPE_TARGET_TYPE (t);
      if (t1 == NULL)
	goto BadValue;
      t1 = ada_check_typedef (t1);
      if (TYPE_CODE (t1) == TYPE_CODE_PTR)
        {
          arg = value_ind (arg);
          t = t1;
        }
      else
        break;
    }

  if (TYPE_CODE (t1) != TYPE_CODE_STRUCT && TYPE_CODE (t1) != TYPE_CODE_UNION)
    goto BadValue;

  if (t1 == t)
    v = ada_search_struct_field (name, arg, 0, t);
  else
    {
      int bit_offset, bit_size, byte_offset;
      struct type *field_type;
      CORE_ADDR address;

      if (TYPE_CODE (t) == TYPE_CODE_PTR)
	address = value_address (ada_value_ind (arg));
      else
	address = value_address (ada_coerce_ref (arg));

      t1 = ada_to_fixed_type (ada_get_base_type (t1), NULL, address, NULL, 1);
      if (find_struct_field (name, t1, 0,
                             &field_type, &byte_offset, &bit_offset,
                             &bit_size, NULL))
        {
          if (bit_size != 0)
            {
              if (TYPE_CODE (t) == TYPE_CODE_REF)
                arg = ada_coerce_ref (arg);
              else
                arg = ada_value_ind (arg);
              v = ada_value_primitive_packed_val (arg, NULL, byte_offset,
                                                  bit_offset, bit_size,
                                                  field_type);
            }
          else
            v = value_at_lazy (field_type, address + byte_offset);
        }
    }

  if (v != NULL || no_err)
    return v;
  else
    error (_("There is no member named %s."), name);

 BadValue:
  if (no_err)
    return NULL;
  else
    error (_("Attempt to extract a component of "
	     "a value that is not a record."));
}

/* Return a string representation of type TYPE.  Caller must free
   result.  */

static char *
type_as_string (struct type *type)
{
  struct ui_file *tmp_stream = mem_fileopen ();
  struct cleanup *old_chain;
  char *str;

  tmp_stream = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (tmp_stream);

  type_print (type, "", tmp_stream, -1);
  str = ui_file_xstrdup (tmp_stream, NULL);

  do_cleanups (old_chain);
  return str;
}

/* Return a string representation of type TYPE, and install a cleanup
   that releases it.  */

static char *
type_as_string_and_cleanup (struct type *type)
{
  char *str;

  str = type_as_string (type);
  make_cleanup (xfree, str);
  return str;
}

/* Given a type TYPE, look up the type of the component of type named NAME.
   If DISPP is non-null, add its byte displacement from the beginning of a
   structure (pointed to by a value) of type TYPE to *DISPP (does not
   work for packed fields).

   Matches any field whose name has NAME as a prefix, possibly
   followed by "___".

   TYPE can be either a struct or union.  If REFOK, TYPE may also 
   be a (pointer or reference)+ to a struct or union, and the
   ultimate target type will be searched.

   Looks recursively into variant clauses and parent types.

   If NOERR is nonzero, return NULL if NAME is not suitably defined or
   TYPE is not a type of the right kind.  */

static struct type *
ada_lookup_struct_elt_type (struct type *type, char *name, int refok,
                            int noerr, int *dispp)
{
  int i;

  if (name == NULL)
    goto BadName;

  if (refok && type != NULL)
    while (1)
      {
        type = ada_check_typedef (type);
        if (TYPE_CODE (type) != TYPE_CODE_PTR
            && TYPE_CODE (type) != TYPE_CODE_REF)
          break;
        type = TYPE_TARGET_TYPE (type);
      }

  if (type == NULL
      || (TYPE_CODE (type) != TYPE_CODE_STRUCT
          && TYPE_CODE (type) != TYPE_CODE_UNION))
    {
      const char *type_str;

      if (noerr)
        return NULL;

      type_str = (type != NULL
		  ? type_as_string_and_cleanup (type)
		  : _("(null)"));
      error (_("Type %s is not a structure or union type"), type_str);
    }

  type = to_static_fixed_type (type);

  for (i = 0; i < TYPE_NFIELDS (type); i += 1)
    {
      const char *t_field_name = TYPE_FIELD_NAME (type, i);
      struct type *t;
      int disp;

      if (t_field_name == NULL)
        continue;

      else if (field_name_match (t_field_name, name))
        {
          if (dispp != NULL)
            *dispp += TYPE_FIELD_BITPOS (type, i) / 8;
          return TYPE_FIELD_TYPE (type, i);
        }

      else if (ada_is_wrapper_field (type, i))
        {
          disp = 0;
          t = ada_lookup_struct_elt_type (TYPE_FIELD_TYPE (type, i), name,
                                          0, 1, &disp);
          if (t != NULL)
            {
              if (dispp != NULL)
                *dispp += disp + TYPE_FIELD_BITPOS (type, i) / 8;
              return t;
            }
        }

      else if (ada_is_variant_part (type, i))
        {
          int j;
          struct type *field_type = ada_check_typedef (TYPE_FIELD_TYPE (type,
									i));

          for (j = TYPE_NFIELDS (field_type) - 1; j >= 0; j -= 1)
            {
	      /* FIXME pnh 2008/01/26: We check for a field that is
	         NOT wrapped in a struct, since the compiler sometimes
		 generates these for unchecked variant types.  Revisit
	         if the compiler changes this practice.  */
	      const char *v_field_name = TYPE_FIELD_NAME (field_type, j);
              disp = 0;
	      if (v_field_name != NULL 
		  && field_name_match (v_field_name, name))
		t = TYPE_FIELD_TYPE (field_type, j);
	      else
		t = ada_lookup_struct_elt_type (TYPE_FIELD_TYPE (field_type,
								 j),
						name, 0, 1, &disp);

              if (t != NULL)
                {
                  if (dispp != NULL)
                    *dispp += disp + TYPE_FIELD_BITPOS (type, i) / 8;
                  return t;
                }
            }
        }

    }

BadName:
  if (!noerr)
    {
      const char *name_str = name != NULL ? name : _("<null>");

      error (_("Type %s has no component named %s"),
	     type_as_string_and_cleanup (type), name_str);
    }

  return NULL;
}

/* Assuming that VAR_TYPE is the type of a variant part of a record (a union),
   within a value of type OUTER_TYPE, return true iff VAR_TYPE
   represents an unchecked union (that is, the variant part of a
   record that is named in an Unchecked_Union pragma).  */

static int
is_unchecked_variant (struct type *var_type, struct type *outer_type)
{
  char *discrim_name = ada_variant_discrim_name (var_type);

  return (ada_lookup_struct_elt_type (outer_type, discrim_name, 0, 1, NULL) 
	  == NULL);
}


/* Assuming that VAR_TYPE is the type of a variant part of a record (a union),
   within a value of type OUTER_TYPE that is stored in GDB at
   OUTER_VALADDR, determine which variant clause (field number in VAR_TYPE,
   numbering from 0) is applicable.  Returns -1 if none are.  */

int
ada_which_variant_applies (struct type *var_type, struct type *outer_type,
                           const gdb_byte *outer_valaddr)
{
  int others_clause;
  int i;
  char *discrim_name = ada_variant_discrim_name (var_type);
  struct value *outer;
  struct value *discrim;
  LONGEST discrim_val;

  /* Using plain value_from_contents_and_address here causes problems
     because we will end up trying to resolve a type that is currently
     being constructed.  */
  outer = value_from_contents_and_address_unresolved (outer_type,
						      outer_valaddr, 0);
  discrim = ada_value_struct_elt (outer, discrim_name, 1);
  if (discrim == NULL)
    return -1;
  discrim_val = value_as_long (discrim);

  others_clause = -1;
  for (i = 0; i < TYPE_NFIELDS (var_type); i += 1)
    {
      if (ada_is_others_clause (var_type, i))
        others_clause = i;
      else if (ada_in_variant (discrim_val, var_type, i))
        return i;
    }

  return others_clause;
}



                                /* Dynamic-Sized Records */

/* Strategy: The type ostensibly attached to a value with dynamic size
   (i.e., a size that is not statically recorded in the debugging
   data) does not accurately reflect the size or layout of the value.
   Our strategy is to convert these values to values with accurate,
   conventional types that are constructed on the fly.  */

/* There is a subtle and tricky problem here.  In general, we cannot
   determine the size of dynamic records without its data.  However,
   the 'struct value' data structure, which GDB uses to represent
   quantities in the inferior process (the target), requires the size
   of the type at the time of its allocation in order to reserve space
   for GDB's internal copy of the data.  That's why the
   'to_fixed_xxx_type' routines take (target) addresses as parameters,
   rather than struct value*s.

   However, GDB's internal history variables ($1, $2, etc.) are
   struct value*s containing internal copies of the data that are not, in
   general, the same as the data at their corresponding addresses in
   the target.  Fortunately, the types we give to these values are all
   conventional, fixed-size types (as per the strategy described
   above), so that we don't usually have to perform the
   'to_fixed_xxx_type' conversions to look at their values.
   Unfortunately, there is one exception: if one of the internal
   history variables is an array whose elements are unconstrained
   records, then we will need to create distinct fixed types for each
   element selected.  */

/* The upshot of all of this is that many routines take a (type, host
   address, target address) triple as arguments to represent a value.
   The host address, if non-null, is supposed to contain an internal
   copy of the relevant data; otherwise, the program is to consult the
   target at the target address.  */

/* Assuming that VAL0 represents a pointer value, the result of
   dereferencing it.  Differs from value_ind in its treatment of
   dynamic-sized types.  */

struct value *
ada_value_ind (struct value *val0)
{
  struct value *val = value_ind (val0);

  if (ada_is_tagged_type (value_type (val), 0))
    val = ada_tag_value_at_base_address (val);

  return ada_to_fixed_value (val);
}

/* The value resulting from dereferencing any "reference to"
   qualifiers on VAL0.  */

static struct value *
ada_coerce_ref (struct value *val0)
{
  if (TYPE_CODE (value_type (val0)) == TYPE_CODE_REF)
    {
      struct value *val = val0;

      val = coerce_ref (val);

      if (ada_is_tagged_type (value_type (val), 0))
	val = ada_tag_value_at_base_address (val);

      return ada_to_fixed_value (val);
    }
  else
    return val0;
}

/* Return OFF rounded upward if necessary to a multiple of
   ALIGNMENT (a power of 2).  */

static unsigned int
align_value (unsigned int off, unsigned int alignment)
{
  return (off + alignment - 1) & ~(alignment - 1);
}

/* Return the bit alignment required for field #F of template type TYPE.  */

static unsigned int
field_alignment (struct type *type, int f)
{
  const char *name = TYPE_FIELD_NAME (type, f);
  int len;
  int align_offset;

  /* The field name should never be null, unless the debugging information
     is somehow malformed.  In this case, we assume the field does not
     require any alignment.  */
  if (name == NULL)
    return 1;

  len = strlen (name);

  if (!isdigit (name[len - 1]))
    return 1;

  if (isdigit (name[len - 2]))
    align_offset = len - 2;
  else
    align_offset = len - 1;

  if (align_offset < 7 || !startswith (name + align_offset - 6, "___XV"))
    return TARGET_CHAR_BIT;

  return atoi (name + align_offset) * TARGET_CHAR_BIT;
}

/* Find a typedef or tag symbol named NAME.  Ignores ambiguity.  */

static struct symbol *
ada_find_any_type_symbol (const char *name)
{
  struct symbol *sym;

  sym = standard_lookup (name, get_selected_block (NULL), VAR_DOMAIN);
  if (sym != NULL && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
    return sym;

  sym = standard_lookup (name, NULL, STRUCT_DOMAIN);
  return sym;
}

/* Find a type named NAME.  Ignores ambiguity.  This routine will look
   solely for types defined by debug info, it will not search the GDB
   primitive types.  */

static struct type *
ada_find_any_type (const char *name)
{
  struct symbol *sym = ada_find_any_type_symbol (name);

  if (sym != NULL)
    return SYMBOL_TYPE (sym);

  return NULL;
}

/* Given NAME_SYM and an associated BLOCK, find a "renaming" symbol
   associated with NAME_SYM's name.  NAME_SYM may itself be a renaming
   symbol, in which case it is returned.  Otherwise, this looks for
   symbols whose name is that of NAME_SYM suffixed with  "___XR".
   Return symbol if found, and NULL otherwise.  */

struct symbol *
ada_find_renaming_symbol (struct symbol *name_sym, const struct block *block)
{
  const char *name = SYMBOL_LINKAGE_NAME (name_sym);
  struct symbol *sym;

  if (strstr (name, "___XR") != NULL)
     return name_sym;

  sym = find_old_style_renaming_symbol (name, block);

  if (sym != NULL)
    return sym;

  /* Not right yet.  FIXME pnh 7/20/2007.  */
  sym = ada_find_any_type_symbol (name);
  if (sym != NULL && strstr (SYMBOL_LINKAGE_NAME (sym), "___XR") != NULL)
    return sym;
  else
    return NULL;
}

static struct symbol *
find_old_style_renaming_symbol (const char *name, const struct block *block)
{
  const struct symbol *function_sym = block_linkage_function (block);
  char *rename;

  if (function_sym != NULL)
    {
      /* If the symbol is defined inside a function, NAME is not fully
         qualified.  This means we need to prepend the function name
         as well as adding the ``___XR'' suffix to build the name of
         the associated renaming symbol.  */
      const char *function_name = SYMBOL_LINKAGE_NAME (function_sym);
      /* Function names sometimes contain suffixes used
         for instance to qualify nested subprograms.  When building
         the XR type name, we need to make sure that this suffix is
         not included.  So do not include any suffix in the function
         name length below.  */
      int function_name_len = ada_name_prefix_len (function_name);
      const int rename_len = function_name_len + 2      /*  "__" */
        + strlen (name) + 6 /* "___XR\0" */ ;

      /* Strip the suffix if necessary.  */
      ada_remove_trailing_digits (function_name, &function_name_len);
      ada_remove_po_subprogram_suffix (function_name, &function_name_len);
      ada_remove_Xbn_suffix (function_name, &function_name_len);

      /* Library-level functions are a special case, as GNAT adds
         a ``_ada_'' prefix to the function name to avoid namespace
         pollution.  However, the renaming symbols themselves do not
         have this prefix, so we need to skip this prefix if present.  */
      if (function_name_len > 5 /* "_ada_" */
          && strstr (function_name, "_ada_") == function_name)
        {
	  function_name += 5;
	  function_name_len -= 5;
        }

      rename = (char *) alloca (rename_len * sizeof (char));
      strncpy (rename, function_name, function_name_len);
      xsnprintf (rename + function_name_len, rename_len - function_name_len,
		 "__%s___XR", name);
    }
  else
    {
      const int rename_len = strlen (name) + 6;

      rename = (char *) alloca (rename_len * sizeof (char));
      xsnprintf (rename, rename_len * sizeof (char), "%s___XR", name);
    }

  return ada_find_any_type_symbol (rename);
}

/* Because of GNAT encoding conventions, several GDB symbols may match a
   given type name.  If the type denoted by TYPE0 is to be preferred to
   that of TYPE1 for purposes of type printing, return non-zero;
   otherwise return 0.  */

int
ada_prefer_type (struct type *type0, struct type *type1)
{
  if (type1 == NULL)
    return 1;
  else if (type0 == NULL)
    return 0;
  else if (TYPE_CODE (type1) == TYPE_CODE_VOID)
    return 1;
  else if (TYPE_CODE (type0) == TYPE_CODE_VOID)
    return 0;
  else if (TYPE_NAME (type1) == NULL && TYPE_NAME (type0) != NULL)
    return 1;
  else if (ada_is_constrained_packed_array_type (type0))
    return 1;
  else if (ada_is_array_descriptor_type (type0)
           && !ada_is_array_descriptor_type (type1))
    return 1;
  else
    {
      const char *type0_name = type_name_no_tag (type0);
      const char *type1_name = type_name_no_tag (type1);

      if (type0_name != NULL && strstr (type0_name, "___XR") != NULL
	  && (type1_name == NULL || strstr (type1_name, "___XR") == NULL))
	return 1;
    }
  return 0;
}

/* The name of TYPE, which is either its TYPE_NAME, or, if that is
   null, its TYPE_TAG_NAME.  Null if TYPE is null.  */

const char *
ada_type_name (struct type *type)
{
  if (type == NULL)
    return NULL;
  else if (TYPE_NAME (type) != NULL)
    return TYPE_NAME (type);
  else
    return TYPE_TAG_NAME (type);
}

/* Search the list of "descriptive" types associated to TYPE for a type
   whose name is NAME.  */

static struct type *
find_parallel_type_by_descriptive_type (struct type *type, const char *name)
{
  struct type *result, *tmp;

  if (ada_ignore_descriptive_types_p)
    return NULL;

  /* If there no descriptive-type info, then there is no parallel type
     to be found.  */
  if (!HAVE_GNAT_AUX_INFO (type))
    return NULL;

  result = TYPE_DESCRIPTIVE_TYPE (type);
  while (result != NULL)
    {
      const char *result_name = ada_type_name (result);

      if (result_name == NULL)
        {
          warning (_("unexpected null name on descriptive type"));
          return NULL;
        }

      /* If the names match, stop.  */
      if (strcmp (result_name, name) == 0)
	break;

      /* Otherwise, look at the next item on the list, if any.  */
      if (HAVE_GNAT_AUX_INFO (result))
	tmp = TYPE_DESCRIPTIVE_TYPE (result);
      else
	tmp = NULL;

      /* If not found either, try after having resolved the typedef.  */
      if (tmp != NULL)
	result = tmp;
      else
	{
	  result = check_typedef (result);
	  if (HAVE_GNAT_AUX_INFO (result))
	    result = TYPE_DESCRIPTIVE_TYPE (result);
	  else
	    result = NULL;
	}
    }

  /* If we didn't find a match, see whether this is a packed array.  With
     older compilers, the descriptive type information is either absent or
     irrelevant when it comes to packed arrays so the above lookup fails.
     Fall back to using a parallel lookup by name in this case.  */
  if (result == NULL && ada_is_constrained_packed_array_type (type))
    return ada_find_any_type (name);

  return result;
}

/* Find a parallel type to TYPE with the specified NAME, using the
   descriptive type taken from the debugging information, if available,
   and otherwise using the (slower) name-based method.  */

static struct type *
ada_find_parallel_type_with_name (struct type *type, const char *name)
{
  struct type *result = NULL;

  if (HAVE_GNAT_AUX_INFO (type))
    result = find_parallel_type_by_descriptive_type (type, name);
  else
    result = ada_find_any_type (name);

  return result;
}

/* Same as above, but specify the name of the parallel type by appending
   SUFFIX to the name of TYPE.  */

struct type *
ada_find_parallel_type (struct type *type, const char *suffix)
{
  char *name;
  const char *type_name = ada_type_name (type);
  int len;

  if (type_name == NULL)
    return NULL;

  len = strlen (type_name);

  name = (char *) alloca (len + strlen (suffix) + 1);

  strcpy (name, type_name);
  strcpy (name + len, suffix);

  return ada_find_parallel_type_with_name (type, name);
}

/* If TYPE is a variable-size record type, return the corresponding template
   type describing its fields.  Otherwise, return NULL.  */

static struct type *
dynamic_template_type (struct type *type)
{
  type = ada_check_typedef (type);

  if (type == NULL || TYPE_CODE (type) != TYPE_CODE_STRUCT
      || ada_type_name (type) == NULL)
    return NULL;
  else
    {
      int len = strlen (ada_type_name (type));

      if (len > 6 && strcmp (ada_type_name (type) + len - 6, "___XVE") == 0)
        return type;
      else
        return ada_find_parallel_type (type, "___XVE");
    }
}

/* Assuming that TEMPL_TYPE is a union or struct type, returns
   non-zero iff field FIELD_NUM of TEMPL_TYPE has dynamic size.  */

static int
is_dynamic_field (struct type *templ_type, int field_num)
{
  const char *name = TYPE_FIELD_NAME (templ_type, field_num);

  return name != NULL
    && TYPE_CODE (TYPE_FIELD_TYPE (templ_type, field_num)) == TYPE_CODE_PTR
    && strstr (name, "___XVL") != NULL;
}

/* The index of the variant field of TYPE, or -1 if TYPE does not
   represent a variant record type.  */

static int
variant_field_index (struct type *type)
{
  int f;

  if (type == NULL || TYPE_CODE (type) != TYPE_CODE_STRUCT)
    return -1;

  for (f = 0; f < TYPE_NFIELDS (type); f += 1)
    {
      if (ada_is_variant_part (type, f))
        return f;
    }
  return -1;
}

/* A record type with no fields.  */

static struct type *
empty_record (struct type *templ)
{
  struct type *type = alloc_type_copy (templ);

  TYPE_CODE (type) = TYPE_CODE_STRUCT;
  TYPE_NFIELDS (type) = 0;
  TYPE_FIELDS (type) = NULL;
  INIT_CPLUS_SPECIFIC (type);
  TYPE_NAME (type) = "<empty>";
  TYPE_TAG_NAME (type) = NULL;
  TYPE_LENGTH (type) = 0;
  return type;
}

/* An ordinary record type (with fixed-length fields) that describes
   the value of type TYPE at VALADDR or ADDRESS (see comments at
   the beginning of this section) VAL according to GNAT conventions.
   DVAL0 should describe the (portion of a) record that contains any
   necessary discriminants.  It should be NULL if value_type (VAL) is
   an outer-level type (i.e., as opposed to a branch of a variant.)  A
   variant field (unless unchecked) is replaced by a particular branch
   of the variant.

   If not KEEP_DYNAMIC_FIELDS, then all fields whose position or
   length are not statically known are discarded.  As a consequence,
   VALADDR, ADDRESS and DVAL0 are ignored.

   NOTE: Limitations: For now, we assume that dynamic fields and
   variants occupy whole numbers of bytes.  However, they need not be
   byte-aligned.  */

struct type *
ada_template_to_fixed_record_type_1 (struct type *type,
				     const gdb_byte *valaddr,
                                     CORE_ADDR address, struct value *dval0,
                                     int keep_dynamic_fields)
{
  struct value *mark = value_mark ();
  struct value *dval;
  struct type *rtype;
  int nfields, bit_len;
  int variant_field;
  long off;
  int fld_bit_len;
  int f;

  /* Compute the number of fields in this record type that are going
     to be processed: unless keep_dynamic_fields, this includes only
     fields whose position and length are static will be processed.  */
  if (keep_dynamic_fields)
    nfields = TYPE_NFIELDS (type);
  else
    {
      nfields = 0;
      while (nfields < TYPE_NFIELDS (type)
             && !ada_is_variant_part (type, nfields)
             && !is_dynamic_field (type, nfields))
        nfields++;
    }

  rtype = alloc_type_copy (type);
  TYPE_CODE (rtype) = TYPE_CODE_STRUCT;
  INIT_CPLUS_SPECIFIC (rtype);
  TYPE_NFIELDS (rtype) = nfields;
  TYPE_FIELDS (rtype) = (struct field *)
    TYPE_ALLOC (rtype, nfields * sizeof (struct field));
  memset (TYPE_FIELDS (rtype), 0, sizeof (struct field) * nfields);
  TYPE_NAME (rtype) = ada_type_name (type);
  TYPE_TAG_NAME (rtype) = NULL;
  TYPE_FIXED_INSTANCE (rtype) = 1;

  off = 0;
  bit_len = 0;
  variant_field = -1;

  for (f = 0; f < nfields; f += 1)
    {
      off = align_value (off, field_alignment (type, f))
	+ TYPE_FIELD_BITPOS (type, f);
      SET_FIELD_BITPOS (TYPE_FIELD (rtype, f), off);
      TYPE_FIELD_BITSIZE (rtype, f) = 0;

      if (ada_is_variant_part (type, f))
        {
          variant_field = f;
          fld_bit_len = 0;
        }
      else if (is_dynamic_field (type, f))
        {
	  const gdb_byte *field_valaddr = valaddr;
	  CORE_ADDR field_address = address;
	  struct type *field_type =
	    TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (type, f));

          if (dval0 == NULL)
	    {
	      /* rtype's length is computed based on the run-time
		 value of discriminants.  If the discriminants are not
		 initialized, the type size may be completely bogus and
		 GDB may fail to allocate a value for it.  So check the
		 size first before creating the value.  */
	      ada_ensure_varsize_limit (rtype);
	      /* Using plain value_from_contents_and_address here
		 causes problems because we will end up trying to
		 resolve a type that is currently being
		 constructed.  */
	      dval = value_from_contents_and_address_unresolved (rtype,
								 valaddr,
								 address);
	      rtype = value_type (dval);
	    }
          else
            dval = dval0;

	  /* If the type referenced by this field is an aligner type, we need
	     to unwrap that aligner type, because its size might not be set.
	     Keeping the aligner type would cause us to compute the wrong
	     size for this field, impacting the offset of the all the fields
	     that follow this one.  */
	  if (ada_is_aligner_type (field_type))
	    {
	      long field_offset = TYPE_FIELD_BITPOS (field_type, f);

	      field_valaddr = cond_offset_host (field_valaddr, field_offset);
	      field_address = cond_offset_target (field_address, field_offset);
	      field_type = ada_aligned_type (field_type);
	    }

	  field_valaddr = cond_offset_host (field_valaddr,
					    off / TARGET_CHAR_BIT);
	  field_address = cond_offset_target (field_address,
					      off / TARGET_CHAR_BIT);

	  /* Get the fixed type of the field.  Note that, in this case,
	     we do not want to get the real type out of the tag: if
	     the current field is the parent part of a tagged record,
	     we will get the tag of the object.  Clearly wrong: the real
	     type of the parent is not the real type of the child.  We
	     would end up in an infinite loop.	*/
	  field_type = ada_get_base_type (field_type);
	  field_type = ada_to_fixed_type (field_type, field_valaddr,
					  field_address, dval, 0);
	  /* If the field size is already larger than the maximum
	     object size, then the record itself will necessarily
	     be larger than the maximum object size.  We need to make
	     this check now, because the size might be so ridiculously
	     large (due to an uninitialized variable in the inferior)
	     that it would cause an overflow when adding it to the
	     record size.  */
	  ada_ensure_varsize_limit (field_type);

	  TYPE_FIELD_TYPE (rtype, f) = field_type;
          TYPE_FIELD_NAME (rtype, f) = TYPE_FIELD_NAME (type, f);
	  /* The multiplication can potentially overflow.  But because
	     the field length has been size-checked just above, and
	     assuming that the maximum size is a reasonable value,
	     an overflow should not happen in practice.  So rather than
	     adding overflow recovery code to this already complex code,
	     we just assume that it's not going to happen.  */
          fld_bit_len =
            TYPE_LENGTH (TYPE_FIELD_TYPE (rtype, f)) * TARGET_CHAR_BIT;
        }
      else
        {
	  /* Note: If this field's type is a typedef, it is important
	     to preserve the typedef layer.

	     Otherwise, we might be transforming a typedef to a fat
	     pointer (encoding a pointer to an unconstrained array),
	     into a basic fat pointer (encoding an unconstrained
	     array).  As both types are implemented using the same
	     structure, the typedef is the only clue which allows us
	     to distinguish between the two options.  Stripping it
	     would prevent us from printing this field appropriately.  */
          TYPE_FIELD_TYPE (rtype, f) = TYPE_FIELD_TYPE (type, f);
          TYPE_FIELD_NAME (rtype, f) = TYPE_FIELD_NAME (type, f);
          if (TYPE_FIELD_BITSIZE (type, f) > 0)
            fld_bit_len =
              TYPE_FIELD_BITSIZE (rtype, f) = TYPE_FIELD_BITSIZE (type, f);
          else
	    {
	      struct type *field_type = TYPE_FIELD_TYPE (type, f);

	      /* We need to be careful of typedefs when computing
		 the length of our field.  If this is a typedef,
		 get the length of the target type, not the length
		 of the typedef.  */
	      if (TYPE_CODE (field_type) == TYPE_CODE_TYPEDEF)
		field_type = ada_typedef_target_type (field_type);

              fld_bit_len =
                TYPE_LENGTH (ada_check_typedef (field_type)) * TARGET_CHAR_BIT;
	    }
        }
      if (off + fld_bit_len > bit_len)
        bit_len = off + fld_bit_len;
      off += fld_bit_len;
      TYPE_LENGTH (rtype) =
        align_value (bit_len, TARGET_CHAR_BIT) / TARGET_CHAR_BIT;
    }

  /* We handle the variant part, if any, at the end because of certain
     odd cases in which it is re-ordered so as NOT to be the last field of
     the record.  This can happen in the presence of representation
     clauses.  */
  if (variant_field >= 0)
    {
      struct type *branch_type;

      off = TYPE_FIELD_BITPOS (rtype, variant_field);

      if (dval0 == NULL)
	{
	  /* Using plain value_from_contents_and_address here causes
	     problems because we will end up trying to resolve a type
	     that is currently being constructed.  */
	  dval = value_from_contents_and_address_unresolved (rtype, valaddr,
							     address);
	  rtype = value_type (dval);
	}
      else
        dval = dval0;

      branch_type =
        to_fixed_variant_branch_type
        (TYPE_FIELD_TYPE (type, variant_field),
         cond_offset_host (valaddr, off / TARGET_CHAR_BIT),
         cond_offset_target (address, off / TARGET_CHAR_BIT), dval);
      if (branch_type == NULL)
        {
          for (f = variant_field + 1; f < TYPE_NFIELDS (rtype); f += 1)
            TYPE_FIELDS (rtype)[f - 1] = TYPE_FIELDS (rtype)[f];
          TYPE_NFIELDS (rtype) -= 1;
        }
      else
        {
          TYPE_FIELD_TYPE (rtype, variant_field) = branch_type;
          TYPE_FIELD_NAME (rtype, variant_field) = "S";
          fld_bit_len =
            TYPE_LENGTH (TYPE_FIELD_TYPE (rtype, variant_field)) *
            TARGET_CHAR_BIT;
          if (off + fld_bit_len > bit_len)
            bit_len = off + fld_bit_len;
          TYPE_LENGTH (rtype) =
            align_value (bit_len, TARGET_CHAR_BIT) / TARGET_CHAR_BIT;
        }
    }

  /* According to exp_dbug.ads, the size of TYPE for variable-size records
     should contain the alignment of that record, which should be a strictly
     positive value.  If null or negative, then something is wrong, most
     probably in the debug info.  In that case, we don't round up the size
     of the resulting type.  If this record is not part of another structure,
     the current RTYPE length might be good enough for our purposes.  */
  if (TYPE_LENGTH (type) <= 0)
    {
      if (TYPE_NAME (rtype))
	warning (_("Invalid type size for `%s' detected: %d."),
		 TYPE_NAME (rtype), TYPE_LENGTH (type));
      else
	warning (_("Invalid type size for <unnamed> detected: %d."),
		 TYPE_LENGTH (type));
    }
  else
    {
      TYPE_LENGTH (rtype) = align_value (TYPE_LENGTH (rtype),
                                         TYPE_LENGTH (type));
    }

  value_free_to_mark (mark);
  if (TYPE_LENGTH (rtype) > varsize_limit)
    error (_("record type with dynamic size is larger than varsize-limit"));
  return rtype;
}

/* As for ada_template_to_fixed_record_type_1 with KEEP_DYNAMIC_FIELDS
   of 1.  */

static struct type *
template_to_fixed_record_type (struct type *type, const gdb_byte *valaddr,
                               CORE_ADDR address, struct value *dval0)
{
  return ada_template_to_fixed_record_type_1 (type, valaddr,
                                              address, dval0, 1);
}

/* An ordinary record type in which ___XVL-convention fields and
   ___XVU- and ___XVN-convention field types in TYPE0 are replaced with
   static approximations, containing all possible fields.  Uses
   no runtime values.  Useless for use in values, but that's OK,
   since the results are used only for type determinations.   Works on both
   structs and unions.  Representation note: to save space, we memorize
   the result of this function in the TYPE_TARGET_TYPE of the
   template type.  */

static struct type *
template_to_static_fixed_type (struct type *type0)
{
  struct type *type;
  int nfields;
  int f;

  /* No need no do anything if the input type is already fixed.  */
  if (TYPE_FIXED_INSTANCE (type0))
    return type0;

  /* Likewise if we already have computed the static approximation.  */
  if (TYPE_TARGET_TYPE (type0) != NULL)
    return TYPE_TARGET_TYPE (type0);

  /* Don't clone TYPE0 until we are sure we are going to need a copy.  */
  type = type0;
  nfields = TYPE_NFIELDS (type0);

  /* Whether or not we cloned TYPE0, cache the result so that we don't do
     recompute all over next time.  */
  TYPE_TARGET_TYPE (type0) = type;

  for (f = 0; f < nfields; f += 1)
    {
      struct type *field_type = TYPE_FIELD_TYPE (type0, f);
      struct type *new_type;

      if (is_dynamic_field (type0, f))
	{
	  field_type = ada_check_typedef (field_type);
          new_type = to_static_fixed_type (TYPE_TARGET_TYPE (field_type));
	}
      else
        new_type = static_unwrap_type (field_type);

      if (new_type != field_type)
	{
	  /* Clone TYPE0 only the first time we get a new field type.  */
	  if (type == type0)
	    {
	      TYPE_TARGET_TYPE (type0) = type = alloc_type_copy (type0);
	      TYPE_CODE (type) = TYPE_CODE (type0);
	      INIT_CPLUS_SPECIFIC (type);
	      TYPE_NFIELDS (type) = nfields;
	      TYPE_FIELDS (type) = (struct field *)
		TYPE_ALLOC (type, nfields * sizeof (struct field));
	      memcpy (TYPE_FIELDS (type), TYPE_FIELDS (type0),
		      sizeof (struct field) * nfields);
	      TYPE_NAME (type) = ada_type_name (type0);
	      TYPE_TAG_NAME (type) = NULL;
	      TYPE_FIXED_INSTANCE (type) = 1;
	      TYPE_LENGTH (type) = 0;
	    }
	  TYPE_FIELD_TYPE (type, f) = new_type;
	  TYPE_FIELD_NAME (type, f) = TYPE_FIELD_NAME (type0, f);
	}
    }

  return type;
}

/* Given an object of type TYPE whose contents are at VALADDR and
   whose address in memory is ADDRESS, returns a revision of TYPE,
   which should be a non-dynamic-sized record, in which the variant
   part, if any, is replaced with the appropriate branch.  Looks
   for discriminant values in DVAL0, which can be NULL if the record
   contains the necessary discriminant values.  */

static struct type *
to_record_with_fixed_variant_part (struct type *type, const gdb_byte *valaddr,
                                   CORE_ADDR address, struct value *dval0)
{
  struct value *mark = value_mark ();
  struct value *dval;
  struct type *rtype;
  struct type *branch_type;
  int nfields = TYPE_NFIELDS (type);
  int variant_field = variant_field_index (type);

  if (variant_field == -1)
    return type;

  if (dval0 == NULL)
    {
      dval = value_from_contents_and_address (type, valaddr, address);
      type = value_type (dval);
    }
  else
    dval = dval0;

  rtype = alloc_type_copy (type);
  TYPE_CODE (rtype) = TYPE_CODE_STRUCT;
  INIT_CPLUS_SPECIFIC (rtype);
  TYPE_NFIELDS (rtype) = nfields;
  TYPE_FIELDS (rtype) =
    (struct field *) TYPE_ALLOC (rtype, nfields * sizeof (struct field));
  memcpy (TYPE_FIELDS (rtype), TYPE_FIELDS (type),
          sizeof (struct field) * nfields);
  TYPE_NAME (rtype) = ada_type_name (type);
  TYPE_TAG_NAME (rtype) = NULL;
  TYPE_FIXED_INSTANCE (rtype) = 1;
  TYPE_LENGTH (rtype) = TYPE_LENGTH (type);

  branch_type = to_fixed_variant_branch_type
    (TYPE_FIELD_TYPE (type, variant_field),
     cond_offset_host (valaddr,
                       TYPE_FIELD_BITPOS (type, variant_field)
                       / TARGET_CHAR_BIT),
     cond_offset_target (address,
                         TYPE_FIELD_BITPOS (type, variant_field)
                         / TARGET_CHAR_BIT), dval);
  if (branch_type == NULL)
    {
      int f;

      for (f = variant_field + 1; f < nfields; f += 1)
        TYPE_FIELDS (rtype)[f - 1] = TYPE_FIELDS (rtype)[f];
      TYPE_NFIELDS (rtype) -= 1;
    }
  else
    {
      TYPE_FIELD_TYPE (rtype, variant_field) = branch_type;
      TYPE_FIELD_NAME (rtype, variant_field) = "S";
      TYPE_FIELD_BITSIZE (rtype, variant_field) = 0;
      TYPE_LENGTH (rtype) += TYPE_LENGTH (branch_type);
    }
  TYPE_LENGTH (rtype) -= TYPE_LENGTH (TYPE_FIELD_TYPE (type, variant_field));

  value_free_to_mark (mark);
  return rtype;
}

/* An ordinary record type (with fixed-length fields) that describes
   the value at (TYPE0, VALADDR, ADDRESS) [see explanation at
   beginning of this section].   Any necessary discriminants' values
   should be in DVAL, a record value; it may be NULL if the object
   at ADDR itself contains any necessary discriminant values.
   Additionally, VALADDR and ADDRESS may also be NULL if no discriminant
   values from the record are needed.  Except in the case that DVAL,
   VALADDR, and ADDRESS are all 0 or NULL, a variant field (unless
   unchecked) is replaced by a particular branch of the variant.

   NOTE: the case in which DVAL and VALADDR are NULL and ADDRESS is 0
   is questionable and may be removed.  It can arise during the
   processing of an unconstrained-array-of-record type where all the
   variant branches have exactly the same size.  This is because in
   such cases, the compiler does not bother to use the XVS convention
   when encoding the record.  I am currently dubious of this
   shortcut and suspect the compiler should be altered.  FIXME.  */

static struct type *
to_fixed_record_type (struct type *type0, const gdb_byte *valaddr,
                      CORE_ADDR address, struct value *dval)
{
  struct type *templ_type;

  if (TYPE_FIXED_INSTANCE (type0))
    return type0;

  templ_type = dynamic_template_type (type0);

  if (templ_type != NULL)
    return template_to_fixed_record_type (templ_type, valaddr, address, dval);
  else if (variant_field_index (type0) >= 0)
    {
      if (dval == NULL && valaddr == NULL && address == 0)
        return type0;
      return to_record_with_fixed_variant_part (type0, valaddr, address,
                                                dval);
    }
  else
    {
      TYPE_FIXED_INSTANCE (type0) = 1;
      return type0;
    }

}

/* An ordinary record type (with fixed-length fields) that describes
   the value at (VAR_TYPE0, VALADDR, ADDRESS), where VAR_TYPE0 is a
   union type.  Any necessary discriminants' values should be in DVAL,
   a record value.  That is, this routine selects the appropriate
   branch of the union at ADDR according to the discriminant value
   indicated in the union's type name.  Returns VAR_TYPE0 itself if
   it represents a variant subject to a pragma Unchecked_Union.  */

static struct type *
to_fixed_variant_branch_type (struct type *var_type0, const gdb_byte *valaddr,
                              CORE_ADDR address, struct value *dval)
{
  int which;
  struct type *templ_type;
  struct type *var_type;

  if (TYPE_CODE (var_type0) == TYPE_CODE_PTR)
    var_type = TYPE_TARGET_TYPE (var_type0);
  else
    var_type = var_type0;

  templ_type = ada_find_parallel_type (var_type, "___XVU");

  if (templ_type != NULL)
    var_type = templ_type;

  if (is_unchecked_variant (var_type, value_type (dval)))
      return var_type0;
  which =
    ada_which_variant_applies (var_type,
                               value_type (dval), value_contents (dval));

  if (which < 0)
    return empty_record (var_type);
  else if (is_dynamic_field (var_type, which))
    return to_fixed_record_type
      (TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (var_type, which)),
       valaddr, address, dval);
  else if (variant_field_index (TYPE_FIELD_TYPE (var_type, which)) >= 0)
    return
      to_fixed_record_type
      (TYPE_FIELD_TYPE (var_type, which), valaddr, address, dval);
  else
    return TYPE_FIELD_TYPE (var_type, which);
}

/* Assuming RANGE_TYPE is a TYPE_CODE_RANGE, return nonzero if
   ENCODING_TYPE, a type following the GNAT conventions for discrete
   type encodings, only carries redundant information.  */

static int
ada_is_redundant_range_encoding (struct type *range_type,
				 struct type *encoding_type)
{
  struct type *fixed_range_type;
  const char *bounds_str;
  int n;
  LONGEST lo, hi;

  gdb_assert (TYPE_CODE (range_type) == TYPE_CODE_RANGE);

  if (TYPE_CODE (get_base_type (range_type))
      != TYPE_CODE (get_base_type (encoding_type)))
    {
      /* The compiler probably used a simple base type to describe
	 the range type instead of the range's actual base type,
	 expecting us to get the real base type from the encoding
	 anyway.  In this situation, the encoding cannot be ignored
	 as redundant.  */
      return 0;
    }

  if (is_dynamic_type (range_type))
    return 0;

  if (TYPE_NAME (encoding_type) == NULL)
    return 0;

  bounds_str = strstr (TYPE_NAME (encoding_type), "___XDLU_");
  if (bounds_str == NULL)
    return 0;

  n = 8; /* Skip "___XDLU_".  */
  if (!ada_scan_number (bounds_str, n, &lo, &n))
    return 0;
  if (TYPE_LOW_BOUND (range_type) != lo)
    return 0;

  n += 2; /* Skip the "__" separator between the two bounds.  */
  if (!ada_scan_number (bounds_str, n, &hi, &n))
    return 0;
  if (TYPE_HIGH_BOUND (range_type) != hi)
    return 0;

  return 1;
}

/* Given the array type ARRAY_TYPE, return nonzero if DESC_TYPE,
   a type following the GNAT encoding for describing array type
   indices, only carries redundant information.  */

static int
ada_is_redundant_index_type_desc (struct type *array_type,
				  struct type *desc_type)
{
  struct type *this_layer = check_typedef (array_type);
  int i;

  for (i = 0; i < TYPE_NFIELDS (desc_type); i++)
    {
      if (!ada_is_redundant_range_encoding (TYPE_INDEX_TYPE (this_layer),
					    TYPE_FIELD_TYPE (desc_type, i)))
	return 0;
      this_layer = check_typedef (TYPE_TARGET_TYPE (this_layer));
    }

  return 1;
}

/* Assuming that TYPE0 is an array type describing the type of a value
   at ADDR, and that DVAL describes a record containing any
   discriminants used in TYPE0, returns a type for the value that
   contains no dynamic components (that is, no components whose sizes
   are determined by run-time quantities).  Unless IGNORE_TOO_BIG is
   true, gives an error message if the resulting type's size is over
   varsize_limit.  */

static struct type *
to_fixed_array_type (struct type *type0, struct value *dval,
                     int ignore_too_big)
{
  struct type *index_type_desc;
  struct type *result;
  int constrained_packed_array_p;
  static const char *xa_suffix = "___XA";

  type0 = ada_check_typedef (type0);
  if (TYPE_FIXED_INSTANCE (type0))
    return type0;

  constrained_packed_array_p = ada_is_constrained_packed_array_type (type0);
  if (constrained_packed_array_p)
    type0 = decode_constrained_packed_array_type (type0);

  index_type_desc = ada_find_parallel_type (type0, xa_suffix);

  /* As mentioned in exp_dbug.ads, for non bit-packed arrays an
     encoding suffixed with 'P' may still be generated.  If so,
     it should be used to find the XA type.  */

  if (index_type_desc == NULL)
    {
      const char *type_name = ada_type_name (type0);

      if (type_name != NULL)
	{
	  const int len = strlen (type_name);
	  char *name = (char *) alloca (len + strlen (xa_suffix));

	  if (type_name[len - 1] == 'P')
	    {
	      strcpy (name, type_name);
	      strcpy (name + len - 1, xa_suffix);
	      index_type_desc = ada_find_parallel_type_with_name (type0, name);
	    }
	}
    }

  ada_fixup_array_indexes_type (index_type_desc);
  if (index_type_desc != NULL
      && ada_is_redundant_index_type_desc (type0, index_type_desc))
    {
      /* Ignore this ___XA parallel type, as it does not bring any
	 useful information.  This allows us to avoid creating fixed
	 versions of the array's index types, which would be identical
	 to the original ones.  This, in turn, can also help avoid
	 the creation of fixed versions of the array itself.  */
      index_type_desc = NULL;
    }

  if (index_type_desc == NULL)
    {
      struct type *elt_type0 = ada_check_typedef (TYPE_TARGET_TYPE (type0));

      /* NOTE: elt_type---the fixed version of elt_type0---should never
         depend on the contents of the array in properly constructed
         debugging data.  */
      /* Create a fixed version of the array element type.
         We're not providing the address of an element here,
         and thus the actual object value cannot be inspected to do
         the conversion.  This should not be a problem, since arrays of
         unconstrained objects are not allowed.  In particular, all
         the elements of an array of a tagged type should all be of
         the same type specified in the debugging info.  No need to
         consult the object tag.  */
      struct type *elt_type = ada_to_fixed_type (elt_type0, 0, 0, dval, 1);

      /* Make sure we always create a new array type when dealing with
	 packed array types, since we're going to fix-up the array
	 type length and element bitsize a little further down.  */
      if (elt_type0 == elt_type && !constrained_packed_array_p)
        result = type0;
      else
        result = create_array_type (alloc_type_copy (type0),
                                    elt_type, TYPE_INDEX_TYPE (type0));
    }
  else
    {
      int i;
      struct type *elt_type0;

      elt_type0 = type0;
      for (i = TYPE_NFIELDS (index_type_desc); i > 0; i -= 1)
        elt_type0 = TYPE_TARGET_TYPE (elt_type0);

      /* NOTE: result---the fixed version of elt_type0---should never
         depend on the contents of the array in properly constructed
         debugging data.  */
      /* Create a fixed version of the array element type.
         We're not providing the address of an element here,
         and thus the actual object value cannot be inspected to do
         the conversion.  This should not be a problem, since arrays of
         unconstrained objects are not allowed.  In particular, all
         the elements of an array of a tagged type should all be of
         the same type specified in the debugging info.  No need to
         consult the object tag.  */
      result =
        ada_to_fixed_type (ada_check_typedef (elt_type0), 0, 0, dval, 1);

      elt_type0 = type0;
      for (i = TYPE_NFIELDS (index_type_desc) - 1; i >= 0; i -= 1)
        {
          struct type *range_type =
            to_fixed_range_type (TYPE_FIELD_TYPE (index_type_desc, i), dval);

          result = create_array_type (alloc_type_copy (elt_type0),
                                      result, range_type);
	  elt_type0 = TYPE_TARGET_TYPE (elt_type0);
        }
      if (!ignore_too_big && TYPE_LENGTH (result) > varsize_limit)
        error (_("array type with dynamic size is larger than varsize-limit"));
    }

  /* We want to preserve the type name.  This can be useful when
     trying to get the type name of a value that has already been
     printed (for instance, if the user did "print VAR; whatis $".  */
  TYPE_NAME (result) = TYPE_NAME (type0);

  if (constrained_packed_array_p)
    {
      /* So far, the resulting type has been created as if the original
	 type was a regular (non-packed) array type.  As a result, the
	 bitsize of the array elements needs to be set again, and the array
	 length needs to be recomputed based on that bitsize.  */
      int len = TYPE_LENGTH (result) / TYPE_LENGTH (TYPE_TARGET_TYPE (result));
      int elt_bitsize = TYPE_FIELD_BITSIZE (type0, 0);

      TYPE_FIELD_BITSIZE (result, 0) = TYPE_FIELD_BITSIZE (type0, 0);
      TYPE_LENGTH (result) = len * elt_bitsize / HOST_CHAR_BIT;
      if (TYPE_LENGTH (result) * HOST_CHAR_BIT < len * elt_bitsize)
        TYPE_LENGTH (result)++;
    }

  TYPE_FIXED_INSTANCE (result) = 1;
  return result;
}


/* A standard type (containing no dynamically sized components)
   corresponding to TYPE for the value (TYPE, VALADDR, ADDRESS)
   DVAL describes a record containing any discriminants used in TYPE0,
   and may be NULL if there are none, or if the object of type TYPE at
   ADDRESS or in VALADDR contains these discriminants.
   
   If CHECK_TAG is not null, in the case of tagged types, this function
   attempts to locate the object's tag and use it to compute the actual
   type.  However, when ADDRESS is null, we cannot use it to determine the
   location of the tag, and therefore compute the tagged type's actual type.
   So we return the tagged type without consulting the tag.  */
   
static struct type *
ada_to_fixed_type_1 (struct type *type, const gdb_byte *valaddr,
                   CORE_ADDR address, struct value *dval, int check_tag)
{
  type = ada_check_typedef (type);
  switch (TYPE_CODE (type))
    {
    default:
      return type;
    case TYPE_CODE_STRUCT:
      {
        struct type *static_type = to_static_fixed_type (type);
        struct type *fixed_record_type =
          to_fixed_record_type (type, valaddr, address, NULL);

        /* If STATIC_TYPE is a tagged type and we know the object's address,
           then we can determine its tag, and compute the object's actual
           type from there.  Note that we have to use the fixed record
           type (the parent part of the record may have dynamic fields
           and the way the location of _tag is expressed may depend on
           them).  */

        if (check_tag && address != 0 && ada_is_tagged_type (static_type, 0))
          {
	    struct value *tag =
	      value_tag_from_contents_and_address
	      (fixed_record_type,
	       valaddr,
	       address);
	    struct type *real_type = type_from_tag (tag);
	    struct value *obj =
	      value_from_contents_and_address (fixed_record_type,
					       valaddr,
					       address);
            fixed_record_type = value_type (obj);
            if (real_type != NULL)
              return to_fixed_record_type
		(real_type, NULL,
		 value_address (ada_tag_value_at_base_address (obj)), NULL);
          }

        /* Check to see if there is a parallel ___XVZ variable.
           If there is, then it provides the actual size of our type.  */
        else if (ada_type_name (fixed_record_type) != NULL)
          {
            const char *name = ada_type_name (fixed_record_type);
            char *xvz_name
	      = (char *) alloca (strlen (name) + 7 /* "___XVZ\0" */);
            int xvz_found = 0;
            LONGEST size;

            xsnprintf (xvz_name, strlen (name) + 7, "%s___XVZ", name);
            size = get_int_var_value (xvz_name, &xvz_found);
            if (xvz_found && TYPE_LENGTH (fixed_record_type) != size)
              {
                fixed_record_type = copy_type (fixed_record_type);
                TYPE_LENGTH (fixed_record_type) = size;

                /* The FIXED_RECORD_TYPE may have be a stub.  We have
                   observed this when the debugging info is STABS, and
                   apparently it is something that is hard to fix.

                   In practice, we don't need the actual type definition
                   at all, because the presence of the XVZ variable allows us
                   to assume that there must be a XVS type as well, which we
                   should be able to use later, when we need the actual type
                   definition.

                   In the meantime, pretend that the "fixed" type we are
                   returning is NOT a stub, because this can cause trouble
                   when using this type to create new types targeting it.
                   Indeed, the associated creation routines often check
                   whether the target type is a stub and will try to replace
                   it, thus using a type with the wrong size.  This, in turn,
                   might cause the new type to have the wrong size too.
                   Consider the case of an array, for instance, where the size
                   of the array is computed from the number of elements in
                   our array multiplied by the size of its element.  */
                TYPE_STUB (fixed_record_type) = 0;
              }
          }
        return fixed_record_type;
      }
    case TYPE_CODE_ARRAY:
      return to_fixed_array_type (type, dval, 1);
    case TYPE_CODE_UNION:
      if (dval == NULL)
        return type;
      else
        return to_fixed_variant_branch_type (type, valaddr, address, dval);
    }
}

/* The same as ada_to_fixed_type_1, except that it preserves the type
   if it is a TYPE_CODE_TYPEDEF of a type that is already fixed.

   The typedef layer needs be preserved in order to differentiate between
   arrays and array pointers when both types are implemented using the same
   fat pointer.  In the array pointer case, the pointer is encoded as
   a typedef of the pointer type.  For instance, considering:

	  type String_Access is access String;
	  S1 : String_Access := null;

   To the debugger, S1 is defined as a typedef of type String.  But
   to the user, it is a pointer.  So if the user tries to print S1,
   we should not dereference the array, but print the array address
   instead.

   If we didn't preserve the typedef layer, we would lose the fact that
   the type is to be presented as a pointer (needs de-reference before
   being printed).  And we would also use the source-level type name.  */

struct type *
ada_to_fixed_type (struct type *type, const gdb_byte *valaddr,
                   CORE_ADDR address, struct value *dval, int check_tag)

{
  struct type *fixed_type =
    ada_to_fixed_type_1 (type, valaddr, address, dval, check_tag);

  /*  If TYPE is a typedef and its target type is the same as the FIXED_TYPE,
      then preserve the typedef layer.

      Implementation note: We can only check the main-type portion of
      the TYPE and FIXED_TYPE, because eliminating the typedef layer
      from TYPE now returns a type that has the same instance flags
      as TYPE.  For instance, if TYPE is a "typedef const", and its
      target type is a "struct", then the typedef elimination will return
      a "const" version of the target type.  See check_typedef for more
      details about how the typedef layer elimination is done.

      brobecker/2010-11-19: It seems to me that the only case where it is
      useful to preserve the typedef layer is when dealing with fat pointers.
      Perhaps, we could add a check for that and preserve the typedef layer
      only in that situation.  But this seems unecessary so far, probably
      because we call check_typedef/ada_check_typedef pretty much everywhere.
      */
  if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF
      && (TYPE_MAIN_TYPE (ada_typedef_target_type (type))
	  == TYPE_MAIN_TYPE (fixed_type)))
    return type;

  return fixed_type;
}

/* A standard (static-sized) type corresponding as well as possible to
   TYPE0, but based on no runtime data.  */

static struct type *
to_static_fixed_type (struct type *type0)
{
  struct type *type;

  if (type0 == NULL)
    return NULL;

  if (TYPE_FIXED_INSTANCE (type0))
    return type0;

  type0 = ada_check_typedef (type0);

  switch (TYPE_CODE (type0))
    {
    default:
      return type0;
    case TYPE_CODE_STRUCT:
      type = dynamic_template_type (type0);
      if (type != NULL)
        return template_to_static_fixed_type (type);
      else
        return template_to_static_fixed_type (type0);
    case TYPE_CODE_UNION:
      type = ada_find_parallel_type (type0, "___XVU");
      if (type != NULL)
        return template_to_static_fixed_type (type);
      else
        return template_to_static_fixed_type (type0);
    }
}

/* A static approximation of TYPE with all type wrappers removed.  */

static struct type *
static_unwrap_type (struct type *type)
{
  if (ada_is_aligner_type (type))
    {
      struct type *type1 = TYPE_FIELD_TYPE (ada_check_typedef (type), 0);
      if (ada_type_name (type1) == NULL)
        TYPE_NAME (type1) = ada_type_name (type);

      return static_unwrap_type (type1);
    }
  else
    {
      struct type *raw_real_type = ada_get_base_type (type);

      if (raw_real_type == type)
        return type;
      else
        return to_static_fixed_type (raw_real_type);
    }
}

/* In some cases, incomplete and private types require
   cross-references that are not resolved as records (for example,
      type Foo;
      type FooP is access Foo;
      V: FooP;
      type Foo is array ...;
   ).  In these cases, since there is no mechanism for producing
   cross-references to such types, we instead substitute for FooP a
   stub enumeration type that is nowhere resolved, and whose tag is
   the name of the actual type.  Call these types "non-record stubs".  */

/* A type equivalent to TYPE that is not a non-record stub, if one
   exists, otherwise TYPE.  */

struct type *
ada_check_typedef (struct type *type)
{
  if (type == NULL)
    return NULL;

  /* If our type is a typedef type of a fat pointer, then we're done.
     We don't want to strip the TYPE_CODE_TYPDEF layer, because this is
     what allows us to distinguish between fat pointers that represent
     array types, and fat pointers that represent array access types
     (in both cases, the compiler implements them as fat pointers).  */
  if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF
      && is_thick_pntr (ada_typedef_target_type (type)))
    return type;

  type = check_typedef (type);
  if (type == NULL || TYPE_CODE (type) != TYPE_CODE_ENUM
      || !TYPE_STUB (type)
      || TYPE_TAG_NAME (type) == NULL)
    return type;
  else
    {
      const char *name = TYPE_TAG_NAME (type);
      struct type *type1 = ada_find_any_type (name);

      if (type1 == NULL)
        return type;

      /* TYPE1 might itself be a TYPE_CODE_TYPEDEF (this can happen with
	 stubs pointing to arrays, as we don't create symbols for array
	 types, only for the typedef-to-array types).  If that's the case,
	 strip the typedef layer.  */
      if (TYPE_CODE (type1) == TYPE_CODE_TYPEDEF)
	type1 = ada_check_typedef (type1);

      return type1;
    }
}

/* A value representing the data at VALADDR/ADDRESS as described by
   type TYPE0, but with a standard (static-sized) type that correctly
   describes it.  If VAL0 is not NULL and TYPE0 already is a standard
   type, then return VAL0 [this feature is simply to avoid redundant
   creation of struct values].  */

static struct value *
ada_to_fixed_value_create (struct type *type0, CORE_ADDR address,
                           struct value *val0)
{
  struct type *type = ada_to_fixed_type (type0, 0, address, NULL, 1);

  if (type == type0 && val0 != NULL)
    return val0;
  else
    return value_from_contents_and_address (type, 0, address);
}

/* A value representing VAL, but with a standard (static-sized) type
   that correctly describes it.  Does not necessarily create a new
   value.  */

struct value *
ada_to_fixed_value (struct value *val)
{
  val = unwrap_value (val);
  val = ada_to_fixed_value_create (value_type (val),
				      value_address (val),
				      val);
  return val;
}


/* Attributes */

/* Table mapping attribute numbers to names.
   NOTE: Keep up to date with enum ada_attribute definition in ada-lang.h.  */

static const char *attribute_names[] = {
  "<?>",

  "first",
  "last",
  "length",
  "image",
  "max",
  "min",
  "modulus",
  "pos",
  "size",
  "tag",
  "val",
  0
};

const char *
ada_attribute_name (enum exp_opcode n)
{
  if (n >= OP_ATR_FIRST && n <= (int) OP_ATR_VAL)
    return attribute_names[n - OP_ATR_FIRST + 1];
  else
    return attribute_names[0];
}

/* Evaluate the 'POS attribute applied to ARG.  */

static LONGEST
pos_atr (struct value *arg)
{
  struct value *val = coerce_ref (arg);
  struct type *type = value_type (val);
  LONGEST result;

  if (!discrete_type_p (type))
    error (_("'POS only defined on discrete types"));

  if (!discrete_position (type, value_as_long (val), &result))
    error (_("enumeration value is invalid: can't find 'POS"));

  return result;
}

static struct value *
value_pos_atr (struct type *type, struct value *arg)
{
  return value_from_longest (type, pos_atr (arg));
}

/* Evaluate the TYPE'VAL attribute applied to ARG.  */

static struct value *
value_val_atr (struct type *type, struct value *arg)
{
  if (!discrete_type_p (type))
    error (_("'VAL only defined on discrete types"));
  if (!integer_type_p (value_type (arg)))
    error (_("'VAL requires integral argument"));

  if (TYPE_CODE (type) == TYPE_CODE_ENUM)
    {
      long pos = value_as_long (arg);

      if (pos < 0 || pos >= TYPE_NFIELDS (type))
        error (_("argument to 'VAL out of range"));
      return value_from_longest (type, TYPE_FIELD_ENUMVAL (type, pos));
    }
  else
    return value_from_longest (type, value_as_long (arg));
}


                                /* Evaluation */

/* True if TYPE appears to be an Ada character type.
   [At the moment, this is true only for Character and Wide_Character;
   It is a heuristic test that could stand improvement].  */

int
ada_is_character_type (struct type *type)
{
  const char *name;

  /* If the type code says it's a character, then assume it really is,
     and don't check any further.  */
  if (TYPE_CODE (type) == TYPE_CODE_CHAR)
    return 1;
  
  /* Otherwise, assume it's a character type iff it is a discrete type
     with a known character type name.  */
  name = ada_type_name (type);
  return (name != NULL
          && (TYPE_CODE (type) == TYPE_CODE_INT
              || TYPE_CODE (type) == TYPE_CODE_RANGE)
          && (strcmp (name, "character") == 0
              || strcmp (name, "wide_character") == 0
              || strcmp (name, "wide_wide_character") == 0
              || strcmp (name, "unsigned char") == 0));
}

/* True if TYPE appears to be an Ada string type.  */

int
ada_is_string_type (struct type *type)
{
  type = ada_check_typedef (type);
  if (type != NULL
      && TYPE_CODE (type) != TYPE_CODE_PTR
      && (ada_is_simple_array_type (type)
          || ada_is_array_descriptor_type (type))
      && ada_array_arity (type) == 1)
    {
      struct type *elttype = ada_array_element_type (type, 1);

      return ada_is_character_type (elttype);
    }
  else
    return 0;
}

/* The compiler sometimes provides a parallel XVS type for a given
   PAD type.  Normally, it is safe to follow the PAD type directly,
   but older versions of the compiler have a bug that causes the offset
   of its "F" field to be wrong.  Following that field in that case
   would lead to incorrect results, but this can be worked around
   by ignoring the PAD type and using the associated XVS type instead.

   Set to True if the debugger should trust the contents of PAD types.
   Otherwise, ignore the PAD type if there is a parallel XVS type.  */
static int trust_pad_over_xvs = 1;

/* True if TYPE is a struct type introduced by the compiler to force the
   alignment of a value.  Such types have a single field with a
   distinctive name.  */

int
ada_is_aligner_type (struct type *type)
{
  type = ada_check_typedef (type);

  if (!trust_pad_over_xvs && ada_find_parallel_type (type, "___XVS") != NULL)
    return 0;

  return (TYPE_CODE (type) == TYPE_CODE_STRUCT
          && TYPE_NFIELDS (type) == 1
          && strcmp (TYPE_FIELD_NAME (type, 0), "F") == 0);
}

/* If there is an ___XVS-convention type parallel to SUBTYPE, return
   the parallel type.  */

struct type *
ada_get_base_type (struct type *raw_type)
{
  struct type *real_type_namer;
  struct type *raw_real_type;

  if (raw_type == NULL || TYPE_CODE (raw_type) != TYPE_CODE_STRUCT)
    return raw_type;

  if (ada_is_aligner_type (raw_type))
    /* The encoding specifies that we should always use the aligner type.
       So, even if this aligner type has an associated XVS type, we should
       simply ignore it.

       According to the compiler gurus, an XVS type parallel to an aligner
       type may exist because of a stabs limitation.  In stabs, aligner
       types are empty because the field has a variable-sized type, and
       thus cannot actually be used as an aligner type.  As a result,
       we need the associated parallel XVS type to decode the type.
       Since the policy in the compiler is to not change the internal
       representation based on the debugging info format, we sometimes
       end up having a redundant XVS type parallel to the aligner type.  */
    return raw_type;

  real_type_namer = ada_find_parallel_type (raw_type, "___XVS");
  if (real_type_namer == NULL
      || TYPE_CODE (real_type_namer) != TYPE_CODE_STRUCT
      || TYPE_NFIELDS (real_type_namer) != 1)
    return raw_type;

  if (TYPE_CODE (TYPE_FIELD_TYPE (real_type_namer, 0)) != TYPE_CODE_REF)
    {
      /* This is an older encoding form where the base type needs to be
	 looked up by name.  We prefer the newer enconding because it is
	 more efficient.  */
      raw_real_type = ada_find_any_type (TYPE_FIELD_NAME (real_type_namer, 0));
      if (raw_real_type == NULL)
	return raw_type;
      else
	return raw_real_type;
    }

  /* The field in our XVS type is a reference to the base type.  */
  return TYPE_TARGET_TYPE (TYPE_FIELD_TYPE (real_type_namer, 0));
}

/* The type of value designated by TYPE, with all aligners removed.  */

struct type *
ada_aligned_type (struct type *type)
{
  if (ada_is_aligner_type (type))
    return ada_aligned_type (TYPE_FIELD_TYPE (type, 0));
  else
    return ada_get_base_type (type);
}


/* The address of the aligned value in an object at address VALADDR
   having type TYPE.  Assumes ada_is_aligner_type (TYPE).  */

const gdb_byte *
ada_aligned_value_addr (struct type *type, const gdb_byte *valaddr)
{
  if (ada_is_aligner_type (type))
    return ada_aligned_value_addr (TYPE_FIELD_TYPE (type, 0),
                                   valaddr +
                                   TYPE_FIELD_BITPOS (type,
                                                      0) / TARGET_CHAR_BIT);
  else
    return valaddr;
}



/* The printed representation of an enumeration literal with encoded
   name NAME.  The value is good to the next call of ada_enum_name.  */
const char *
ada_enum_name (const char *name)
{
  static char *result;
  static size_t result_len = 0;
  const char *tmp;

  /* First, unqualify the enumeration name:
     1. Search for the last '.' character.  If we find one, then skip
     all the preceding characters, the unqualified name starts
     right after that dot.
     2. Otherwise, we may be debugging on a target where the compiler
     translates dots into "__".  Search forward for double underscores,
     but stop searching when we hit an overloading suffix, which is
     of the form "__" followed by digits.  */

  tmp = strrchr (name, '.');
  if (tmp != NULL)
    name = tmp + 1;
  else
    {
      while ((tmp = strstr (name, "__")) != NULL)
        {
          if (isdigit (tmp[2]))
            break;
          else
            name = tmp + 2;
        }
    }

  if (name[0] == 'Q')
    {
      int v;

      if (name[1] == 'U' || name[1] == 'W')
        {
          if (sscanf (name + 2, "%x", &v) != 1)
            return name;
        }
      else
        return name;

      GROW_VECT (result, result_len, 16);
      if (isascii (v) && isprint (v))
        xsnprintf (result, result_len, "'%c'", v);
      else if (name[1] == 'U')
        xsnprintf (result, result_len, "[\"%02x\"]", v);
      else
        xsnprintf (result, result_len, "[\"%04x\"]", v);

      return result;
    }
  else
    {
      tmp = strstr (name, "__");
      if (tmp == NULL)
	tmp = strstr (name, "$");
      if (tmp != NULL)
        {
          GROW_VECT (result, result_len, tmp - name + 1);
          strncpy (result, name, tmp - name);
          result[tmp - name] = '\0';
          return result;
        }

      return name;
    }
}

/* Evaluate the subexpression of EXP starting at *POS as for
   evaluate_type, updating *POS to point just past the evaluated
   expression.  */

static struct value *
evaluate_subexp_type (struct expression *exp, int *pos)
{
  return evaluate_subexp (NULL_TYPE, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
}

/* If VAL is wrapped in an aligner or subtype wrapper, return the
   value it wraps.  */

static struct value *
unwrap_value (struct value *val)
{
  struct type *type = ada_check_typedef (value_type (val));

  if (ada_is_aligner_type (type))
    {
      struct value *v = ada_value_struct_elt (val, "F", 0);
      struct type *val_type = ada_check_typedef (value_type (v));

      if (ada_type_name (val_type) == NULL)
        TYPE_NAME (val_type) = ada_type_name (type);

      return unwrap_value (v);
    }
  else
    {
      struct type *raw_real_type =
        ada_check_typedef (ada_get_base_type (type));

      /* If there is no parallel XVS or XVE type, then the value is
	 already unwrapped.  Return it without further modification.  */
      if ((type == raw_real_type)
	  && ada_find_parallel_type (type, "___XVE") == NULL)
	return val;

      return
        coerce_unspec_val_to_type
        (val, ada_to_fixed_type (raw_real_type, 0,
                                 value_address (val),
                                 NULL, 1));
    }
}

static struct value *
cast_to_fixed (struct type *type, struct value *arg)
{
  LONGEST val;

  if (type == value_type (arg))
    return arg;
  else if (ada_is_fixed_point_type (value_type (arg)))
    val = ada_float_to_fixed (type,
                              ada_fixed_to_float (value_type (arg),
                                                  value_as_long (arg)));
  else
    {
      DOUBLEST argd = value_as_double (arg);

      val = ada_float_to_fixed (type, argd);
    }

  return value_from_longest (type, val);
}

static struct value *
cast_from_fixed (struct type *type, struct value *arg)
{
  DOUBLEST val = ada_fixed_to_float (value_type (arg),
                                     value_as_long (arg));

  return value_from_double (type, val);
}

/* Given two array types T1 and T2, return nonzero iff both arrays
   contain the same number of elements.  */

static int
ada_same_array_size_p (struct type *t1, struct type *t2)
{
  LONGEST lo1, hi1, lo2, hi2;

  /* Get the array bounds in order to verify that the size of
     the two arrays match.  */
  if (!get_array_bounds (t1, &lo1, &hi1)
      || !get_array_bounds (t2, &lo2, &hi2))
    error (_("unable to determine array bounds"));

  /* To make things easier for size comparison, normalize a bit
     the case of empty arrays by making sure that the difference
     between upper bound and lower bound is always -1.  */
  if (lo1 > hi1)
    hi1 = lo1 - 1;
  if (lo2 > hi2)
    hi2 = lo2 - 1;

  return (hi1 - lo1 == hi2 - lo2);
}

/* Assuming that VAL is an array of integrals, and TYPE represents
   an array with the same number of elements, but with wider integral
   elements, return an array "casted" to TYPE.  In practice, this
   means that the returned array is built by casting each element
   of the original array into TYPE's (wider) element type.  */

static struct value *
ada_promote_array_of_integrals (struct type *type, struct value *val)
{
  struct type *elt_type = TYPE_TARGET_TYPE (type);
  LONGEST lo, hi;
  struct value *res;
  LONGEST i;

  /* Verify that both val and type are arrays of scalars, and
     that the size of val's elements is smaller than the size
     of type's element.  */
  gdb_assert (TYPE_CODE (type) == TYPE_CODE_ARRAY);
  gdb_assert (is_integral_type (TYPE_TARGET_TYPE (type)));
  gdb_assert (TYPE_CODE (value_type (val)) == TYPE_CODE_ARRAY);
  gdb_assert (is_integral_type (TYPE_TARGET_TYPE (value_type (val))));
  gdb_assert (TYPE_LENGTH (TYPE_TARGET_TYPE (type))
	      > TYPE_LENGTH (TYPE_TARGET_TYPE (value_type (val))));

  if (!get_array_bounds (type, &lo, &hi))
    error (_("unable to determine array bounds"));

  res = allocate_value (type);

  /* Promote each array element.  */
  for (i = 0; i < hi - lo + 1; i++)
    {
      struct value *elt = value_cast (elt_type, value_subscript (val, lo + i));

      memcpy (value_contents_writeable (res) + (i * TYPE_LENGTH (elt_type)),
	      value_contents_all (elt), TYPE_LENGTH (elt_type));
    }

  return res;
}

/* Coerce VAL as necessary for assignment to an lval of type TYPE, and
   return the converted value.  */

static struct value *
coerce_for_assign (struct type *type, struct value *val)
{
  struct type *type2 = value_type (val);

  if (type == type2)
    return val;

  type2 = ada_check_typedef (type2);
  type = ada_check_typedef (type);

  if (TYPE_CODE (type2) == TYPE_CODE_PTR
      && TYPE_CODE (type) == TYPE_CODE_ARRAY)
    {
      val = ada_value_ind (val);
      type2 = value_type (val);
    }

  if (TYPE_CODE (type2) == TYPE_CODE_ARRAY
      && TYPE_CODE (type) == TYPE_CODE_ARRAY)
    {
      if (!ada_same_array_size_p (type, type2))
	error (_("cannot assign arrays of different length"));

      if (is_integral_type (TYPE_TARGET_TYPE (type))
	  && is_integral_type (TYPE_TARGET_TYPE (type2))
	  && TYPE_LENGTH (TYPE_TARGET_TYPE (type2))
	       < TYPE_LENGTH (TYPE_TARGET_TYPE (type)))
	{
	  /* Allow implicit promotion of the array elements to
	     a wider type.  */
	  return ada_promote_array_of_integrals (type, val);
	}

      if (TYPE_LENGTH (TYPE_TARGET_TYPE (type2))
          != TYPE_LENGTH (TYPE_TARGET_TYPE (type)))
        error (_("Incompatible types in assignment"));
      deprecated_set_value_type (val, type);
    }
  return val;
}

static struct value *
ada_value_binop (struct value *arg1, struct value *arg2, enum exp_opcode op)
{
  struct value *val;
  struct type *type1, *type2;
  LONGEST v, v1, v2;

  arg1 = coerce_ref (arg1);
  arg2 = coerce_ref (arg2);
  type1 = get_base_type (ada_check_typedef (value_type (arg1)));
  type2 = get_base_type (ada_check_typedef (value_type (arg2)));

  if (TYPE_CODE (type1) != TYPE_CODE_INT
      || TYPE_CODE (type2) != TYPE_CODE_INT)
    return value_binop (arg1, arg2, op);

  switch (op)
    {
    case BINOP_MOD:
    case BINOP_DIV:
    case BINOP_REM:
      break;
    default:
      return value_binop (arg1, arg2, op);
    }

  v2 = value_as_long (arg2);
  if (v2 == 0)
    error (_("second operand of %s must not be zero."), op_string (op));

  if (TYPE_UNSIGNED (type1) || op == BINOP_MOD)
    return value_binop (arg1, arg2, op);

  v1 = value_as_long (arg1);
  switch (op)
    {
    case BINOP_DIV:
      v = v1 / v2;
      if (!TRUNCATION_TOWARDS_ZERO && v1 * (v1 % v2) < 0)
        v += v > 0 ? -1 : 1;
      break;
    case BINOP_REM:
      v = v1 % v2;
      if (v * v1 < 0)
        v -= v2;
      break;
    default:
      /* Should not reach this point.  */
      v = 0;
    }

  val = allocate_value (type1);
  store_unsigned_integer (value_contents_raw (val),
                          TYPE_LENGTH (value_type (val)),
			  gdbarch_byte_order (get_type_arch (type1)), v);
  return val;
}

static int
ada_value_equal (struct value *arg1, struct value *arg2)
{
  if (ada_is_direct_array_type (value_type (arg1))
      || ada_is_direct_array_type (value_type (arg2)))
    {
      /* Automatically dereference any array reference before
         we attempt to perform the comparison.  */
      arg1 = ada_coerce_ref (arg1);
      arg2 = ada_coerce_ref (arg2);
      
      arg1 = ada_coerce_to_simple_array (arg1);
      arg2 = ada_coerce_to_simple_array (arg2);
      if (TYPE_CODE (value_type (arg1)) != TYPE_CODE_ARRAY
          || TYPE_CODE (value_type (arg2)) != TYPE_CODE_ARRAY)
        error (_("Attempt to compare array with non-array"));
      /* FIXME: The following works only for types whose
         representations use all bits (no padding or undefined bits)
         and do not have user-defined equality.  */
      return
        TYPE_LENGTH (value_type (arg1)) == TYPE_LENGTH (value_type (arg2))
        && memcmp (value_contents (arg1), value_contents (arg2),
                   TYPE_LENGTH (value_type (arg1))) == 0;
    }
  return value_equal (arg1, arg2);
}

/* Total number of component associations in the aggregate starting at
   index PC in EXP.  Assumes that index PC is the start of an
   OP_AGGREGATE.  */

static int
num_component_specs (struct expression *exp, int pc)
{
  int n, m, i;

  m = exp->elts[pc + 1].longconst;
  pc += 3;
  n = 0;
  for (i = 0; i < m; i += 1)
    {
      switch (exp->elts[pc].opcode) 
	{
	default:
	  n += 1;
	  break;
	case OP_CHOICES:
	  n += exp->elts[pc + 1].longconst;
	  break;
	}
      ada_evaluate_subexp (NULL, exp, &pc, EVAL_SKIP);
    }
  return n;
}

/* Assign the result of evaluating EXP starting at *POS to the INDEXth 
   component of LHS (a simple array or a record), updating *POS past
   the expression, assuming that LHS is contained in CONTAINER.  Does
   not modify the inferior's memory, nor does it modify LHS (unless
   LHS == CONTAINER).  */

static void
assign_component (struct value *container, struct value *lhs, LONGEST index,
		  struct expression *exp, int *pos)
{
  struct value *mark = value_mark ();
  struct value *elt;

  if (TYPE_CODE (value_type (lhs)) == TYPE_CODE_ARRAY)
    {
      struct type *index_type = builtin_type (exp->gdbarch)->builtin_int;
      struct value *index_val = value_from_longest (index_type, index);

      elt = unwrap_value (ada_value_subscript (lhs, 1, &index_val));
    }
  else
    {
      elt = ada_index_struct_field (index, lhs, 0, value_type (lhs));
      elt = ada_to_fixed_value (elt);
    }

  if (exp->elts[*pos].opcode == OP_AGGREGATE)
    assign_aggregate (container, elt, exp, pos, EVAL_NORMAL);
  else
    value_assign_to_component (container, elt, 
			       ada_evaluate_subexp (NULL, exp, pos, 
						    EVAL_NORMAL));

  value_free_to_mark (mark);
}

/* Assuming that LHS represents an lvalue having a record or array
   type, and EXP->ELTS[*POS] is an OP_AGGREGATE, evaluate an assignment
   of that aggregate's value to LHS, advancing *POS past the
   aggregate.  NOSIDE is as for evaluate_subexp.  CONTAINER is an
   lvalue containing LHS (possibly LHS itself).  Does not modify
   the inferior's memory, nor does it modify the contents of 
   LHS (unless == CONTAINER).  Returns the modified CONTAINER.  */

static struct value *
assign_aggregate (struct value *container, 
		  struct value *lhs, struct expression *exp, 
		  int *pos, enum noside noside)
{
  struct type *lhs_type;
  int n = exp->elts[*pos+1].longconst;
  LONGEST low_index, high_index;
  int num_specs;
  LONGEST *indices;
  int max_indices, num_indices;
  int i;

  *pos += 3;
  if (noside != EVAL_NORMAL)
    {
      for (i = 0; i < n; i += 1)
	ada_evaluate_subexp (NULL, exp, pos, noside);
      return container;
    }

  container = ada_coerce_ref (container);
  if (ada_is_direct_array_type (value_type (container)))
    container = ada_coerce_to_simple_array (container);
  lhs = ada_coerce_ref (lhs);
  if (!deprecated_value_modifiable (lhs))
    error (_("Left operand of assignment is not a modifiable lvalue."));

  lhs_type = value_type (lhs);
  if (ada_is_direct_array_type (lhs_type))
    {
      lhs = ada_coerce_to_simple_array (lhs);
      lhs_type = value_type (lhs);
      low_index = TYPE_ARRAY_LOWER_BOUND_VALUE (lhs_type);
      high_index = TYPE_ARRAY_UPPER_BOUND_VALUE (lhs_type);
    }
  else if (TYPE_CODE (lhs_type) == TYPE_CODE_STRUCT)
    {
      low_index = 0;
      high_index = num_visible_fields (lhs_type) - 1;
    }
  else
    error (_("Left-hand side must be array or record."));

  num_specs = num_component_specs (exp, *pos - 3);
  max_indices = 4 * num_specs + 4;
  indices = XALLOCAVEC (LONGEST, max_indices);
  indices[0] = indices[1] = low_index - 1;
  indices[2] = indices[3] = high_index + 1;
  num_indices = 4;

  for (i = 0; i < n; i += 1)
    {
      switch (exp->elts[*pos].opcode)
	{
	  case OP_CHOICES:
	    aggregate_assign_from_choices (container, lhs, exp, pos, indices, 
					   &num_indices, max_indices,
					   low_index, high_index);
	    break;
	  case OP_POSITIONAL:
	    aggregate_assign_positional (container, lhs, exp, pos, indices,
					 &num_indices, max_indices,
					 low_index, high_index);
	    break;
	  case OP_OTHERS:
	    if (i != n-1)
	      error (_("Misplaced 'others' clause"));
	    aggregate_assign_others (container, lhs, exp, pos, indices, 
				     num_indices, low_index, high_index);
	    break;
	  default:
	    error (_("Internal error: bad aggregate clause"));
	}
    }

  return container;
}
	      
/* Assign into the component of LHS indexed by the OP_POSITIONAL
   construct at *POS, updating *POS past the construct, given that
   the positions are relative to lower bound LOW, where HIGH is the 
   upper bound.  Record the position in INDICES[0 .. MAX_INDICES-1]
   updating *NUM_INDICES as needed.  CONTAINER is as for
   assign_aggregate.  */
static void
aggregate_assign_positional (struct value *container,
			     struct value *lhs, struct expression *exp,
			     int *pos, LONGEST *indices, int *num_indices,
			     int max_indices, LONGEST low, LONGEST high) 
{
  LONGEST ind = longest_to_int (exp->elts[*pos + 1].longconst) + low;
  
  if (ind - 1 == high)
    warning (_("Extra components in aggregate ignored."));
  if (ind <= high)
    {
      add_component_interval (ind, ind, indices, num_indices, max_indices);
      *pos += 3;
      assign_component (container, lhs, ind, exp, pos);
    }
  else
    ada_evaluate_subexp (NULL, exp, pos, EVAL_SKIP);
}

/* Assign into the components of LHS indexed by the OP_CHOICES
   construct at *POS, updating *POS past the construct, given that
   the allowable indices are LOW..HIGH.  Record the indices assigned
   to in INDICES[0 .. MAX_INDICES-1], updating *NUM_INDICES as
   needed.  CONTAINER is as for assign_aggregate.  */
static void
aggregate_assign_from_choices (struct value *container,
			       struct value *lhs, struct expression *exp,
			       int *pos, LONGEST *indices, int *num_indices,
			       int max_indices, LONGEST low, LONGEST high) 
{
  int j;
  int n_choices = longest_to_int (exp->elts[*pos+1].longconst);
  int choice_pos, expr_pc;
  int is_array = ada_is_direct_array_type (value_type (lhs));

  choice_pos = *pos += 3;

  for (j = 0; j < n_choices; j += 1)
    ada_evaluate_subexp (NULL, exp, pos, EVAL_SKIP);
  expr_pc = *pos;
  ada_evaluate_subexp (NULL, exp, pos, EVAL_SKIP);
  
  for (j = 0; j < n_choices; j += 1)
    {
      LONGEST lower, upper;
      enum exp_opcode op = exp->elts[choice_pos].opcode;

      if (op == OP_DISCRETE_RANGE)
	{
	  choice_pos += 1;
	  lower = value_as_long (ada_evaluate_subexp (NULL, exp, pos,
						      EVAL_NORMAL));
	  upper = value_as_long (ada_evaluate_subexp (NULL, exp, pos, 
						      EVAL_NORMAL));
	}
      else if (is_array)
	{
	  lower = value_as_long (ada_evaluate_subexp (NULL, exp, &choice_pos, 
						      EVAL_NORMAL));
	  upper = lower;
	}
      else
	{
	  int ind;
	  const char *name;

	  switch (op)
	    {
	    case OP_NAME:
	      name = &exp->elts[choice_pos + 2].string;
	      break;
	    case OP_VAR_VALUE:
	      name = SYMBOL_NATURAL_NAME (exp->elts[choice_pos + 2].symbol);
	      break;
	    default:
	      error (_("Invalid record component association."));
	    }
	  ada_evaluate_subexp (NULL, exp, &choice_pos, EVAL_SKIP);
	  ind = 0;
	  if (! find_struct_field (name, value_type (lhs), 0, 
				   NULL, NULL, NULL, NULL, &ind))
	    error (_("Unknown component name: %s."), name);
	  lower = upper = ind;
	}

      if (lower <= upper && (lower < low || upper > high))
	error (_("Index in component association out of bounds."));

      add_component_interval (lower, upper, indices, num_indices,
			      max_indices);
      while (lower <= upper)
	{
	  int pos1;

	  pos1 = expr_pc;
	  assign_component (container, lhs, lower, exp, &pos1);
	  lower += 1;
	}
    }
}

/* Assign the value of the expression in the OP_OTHERS construct in
   EXP at *POS into the components of LHS indexed from LOW .. HIGH that
   have not been previously assigned.  The index intervals already assigned
   are in INDICES[0 .. NUM_INDICES-1].  Updates *POS to after the 
   OP_OTHERS clause.  CONTAINER is as for assign_aggregate.  */
static void
aggregate_assign_others (struct value *container,
			 struct value *lhs, struct expression *exp,
			 int *pos, LONGEST *indices, int num_indices,
			 LONGEST low, LONGEST high) 
{
  int i;
  int expr_pc = *pos + 1;
  
  for (i = 0; i < num_indices - 2; i += 2)
    {
      LONGEST ind;

      for (ind = indices[i + 1] + 1; ind < indices[i + 2]; ind += 1)
	{
	  int localpos;

	  localpos = expr_pc;
	  assign_component (container, lhs, ind, exp, &localpos);
	}
    }
  ada_evaluate_subexp (NULL, exp, pos, EVAL_SKIP);
}

/* Add the interval [LOW .. HIGH] to the sorted set of intervals 
   [ INDICES[0] .. INDICES[1] ],..., [ INDICES[*SIZE-2] .. INDICES[*SIZE-1] ],
   modifying *SIZE as needed.  It is an error if *SIZE exceeds
   MAX_SIZE.  The resulting intervals do not overlap.  */
static void
add_component_interval (LONGEST low, LONGEST high, 
			LONGEST* indices, int *size, int max_size)
{
  int i, j;

  for (i = 0; i < *size; i += 2) {
    if (high >= indices[i] && low <= indices[i + 1])
      {
	int kh;

	for (kh = i + 2; kh < *size; kh += 2)
	  if (high < indices[kh])
	    break;
	if (low < indices[i])
	  indices[i] = low;
	indices[i + 1] = indices[kh - 1];
	if (high > indices[i + 1])
	  indices[i + 1] = high;
	memcpy (indices + i + 2, indices + kh, *size - kh);
	*size -= kh - i - 2;
	return;
      }
    else if (high < indices[i])
      break;
  }
	
  if (*size == max_size)
    error (_("Internal error: miscounted aggregate components."));
  *size += 2;
  for (j = *size-1; j >= i+2; j -= 1)
    indices[j] = indices[j - 2];
  indices[i] = low;
  indices[i + 1] = high;
}

/* Perform and Ada cast of ARG2 to type TYPE if the type of ARG2
   is different.  */

static struct value *
ada_value_cast (struct type *type, struct value *arg2, enum noside noside)
{
  if (type == ada_check_typedef (value_type (arg2)))
    return arg2;

  if (ada_is_fixed_point_type (type))
    return (cast_to_fixed (type, arg2));

  if (ada_is_fixed_point_type (value_type (arg2)))
    return cast_from_fixed (type, arg2);

  return value_cast (type, arg2);
}

/*  Evaluating Ada expressions, and printing their result.
    ------------------------------------------------------

    1. Introduction:
    ----------------

    We usually evaluate an Ada expression in order to print its value.
    We also evaluate an expression in order to print its type, which
    happens during the EVAL_AVOID_SIDE_EFFECTS phase of the evaluation,
    but we'll focus mostly on the EVAL_NORMAL phase.  In practice, the
    EVAL_AVOID_SIDE_EFFECTS phase allows us to simplify certain aspects of
    the evaluation compared to the EVAL_NORMAL, but is otherwise very
    similar.

    Evaluating expressions is a little more complicated for Ada entities
    than it is for entities in languages such as C.  The main reason for
    this is that Ada provides types whose definition might be dynamic.
    One example of such types is variant records.  Or another example
    would be an array whose bounds can only be known at run time.

    The following description is a general guide as to what should be
    done (and what should NOT be done) in order to evaluate an expression
    involving such types, and when.  This does not cover how the semantic
    information is encoded by GNAT as this is covered separatly.  For the
    document used as the reference for the GNAT encoding, see exp_dbug.ads
    in the GNAT sources.

    Ideally, we should embed each part of this description next to its
    associated code.  Unfortunately, the amount of code is so vast right
    now that it's hard to see whether the code handling a particular
    situation might be duplicated or not.  One day, when the code is
    cleaned up, this guide might become redundant with the comments
    inserted in the code, and we might want to remove it.

    2. ``Fixing'' an Entity, the Simple Case:
    -----------------------------------------

    When evaluating Ada expressions, the tricky issue is that they may
    reference entities whose type contents and size are not statically
    known.  Consider for instance a variant record:

       type Rec (Empty : Boolean := True) is record
          case Empty is
             when True => null;
             when False => Value : Integer;
          end case;
       end record;
       Yes : Rec := (Empty => False, Value => 1);
       No  : Rec := (empty => True);

    The size and contents of that record depends on the value of the
    descriminant (Rec.Empty).  At this point, neither the debugging
    information nor the associated type structure in GDB are able to
    express such dynamic types.  So what the debugger does is to create
    "fixed" versions of the type that applies to the specific object.
    We also informally refer to this opperation as "fixing" an object,
    which means creating its associated fixed type.

    Example: when printing the value of variable "Yes" above, its fixed
    type would look like this:

       type Rec is record
          Empty : Boolean;
          Value : Integer;
       end record;

    On the other hand, if we printed the value of "No", its fixed type
    would become:

       type Rec is record
          Empty : Boolean;
       end record;

    Things become a little more complicated when trying to fix an entity
    with a dynamic type that directly contains another dynamic type,
    such as an array of variant records, for instance.  There are
    two possible cases: Arrays, and records.

    3. ``Fixing'' Arrays:
    ---------------------

    The type structure in GDB describes an array in terms of its bounds,
    and the type of its elements.  By design, all elements in the array
    have the same type and we cannot represent an array of variant elements
    using the current type structure in GDB.  When fixing an array,
    we cannot fix the array element, as we would potentially need one
    fixed type per element of the array.  As a result, the best we can do
    when fixing an array is to produce an array whose bounds and size
    are correct (allowing us to read it from memory), but without having
    touched its element type.  Fixing each element will be done later,
    when (if) necessary.

    Arrays are a little simpler to handle than records, because the same
    amount of memory is allocated for each element of the array, even if
    the amount of space actually used by each element differs from element
    to element.  Consider for instance the following array of type Rec:

       type Rec_Array is array (1 .. 2) of Rec;

    The actual amount of memory occupied by each element might be different
    from element to element, depending on the value of their discriminant.
    But the amount of space reserved for each element in the array remains
    fixed regardless.  So we simply need to compute that size using
    the debugging information available, from which we can then determine
    the array size (we multiply the number of elements of the array by
    the size of each element).

    The simplest case is when we have an array of a constrained element
    type. For instance, consider the following type declarations:

        type Bounded_String (Max_Size : Integer) is
           Length : Integer;
           Buffer : String (1 .. Max_Size);
        end record;
        type Bounded_String_Array is array (1 ..2) of Bounded_String (80);

    In this case, the compiler describes the array as an array of
    variable-size elements (identified by its XVS suffix) for which
    the size can be read in the parallel XVZ variable.

    In the case of an array of an unconstrained element type, the compiler
    wraps the array element inside a private PAD type.  This type should not
    be shown to the user, and must be "unwrap"'ed before printing.  Note
    that we also use the adjective "aligner" in our code to designate
    these wrapper types.

    In some cases, the size allocated for each element is statically
    known.  In that case, the PAD type already has the correct size,
    and the array element should remain unfixed.

    But there are cases when this size is not statically known.
    For instance, assuming that "Five" is an integer variable:

        type Dynamic is array (1 .. Five) of Integer;
        type Wrapper (Has_Length : Boolean := False) is record
           Data : Dynamic;
           case Has_Length is
              when True => Length : Integer;
              when False => null;
           end case;
        end record;
        type Wrapper_Array is array (1 .. 2) of Wrapper;

        Hello : Wrapper_Array := (others => (Has_Length => True,
                                             Data => (others => 17),
                                             Length => 1));


    The debugging info would describe variable Hello as being an
    array of a PAD type.  The size of that PAD type is not statically
    known, but can be determined using a parallel XVZ variable.
    In that case, a copy of the PAD type with the correct size should
    be used for the fixed array.

    3. ``Fixing'' record type objects:
    ----------------------------------

    Things are slightly different from arrays in the case of dynamic
    record types.  In this case, in order to compute the associated
    fixed type, we need to determine the size and offset of each of
    its components.  This, in turn, requires us to compute the fixed
    type of each of these components.

    Consider for instance the example:

        type Bounded_String (Max_Size : Natural) is record
           Str : String (1 .. Max_Size);
           Length : Natural;
        end record;
        My_String : Bounded_String (Max_Size => 10);

    In that case, the position of field "Length" depends on the size
    of field Str, which itself depends on the value of the Max_Size
    discriminant.  In order to fix the type of variable My_String,
    we need to fix the type of field Str.  Therefore, fixing a variant
    record requires us to fix each of its components.

    However, if a component does not have a dynamic size, the component
    should not be fixed.  In particular, fields that use a PAD type
    should not fixed.  Here is an example where this might happen
    (assuming type Rec above):

       type Container (Big : Boolean) is record
          First : Rec;
          After : Integer;
          case Big is
             when True => Another : Integer;
             when False => null;
          end case;
       end record;
       My_Container : Container := (Big => False,
                                    First => (Empty => True),
                                    After => 42);

    In that example, the compiler creates a PAD type for component First,
    whose size is constant, and then positions the component After just
    right after it.  The offset of component After is therefore constant
    in this case.

    The debugger computes the position of each field based on an algorithm
    that uses, among other things, the actual position and size of the field
    preceding it.  Let's now imagine that the user is trying to print
    the value of My_Container.  If the type fixing was recursive, we would
    end up computing the offset of field After based on the size of the
    fixed version of field First.  And since in our example First has
    only one actual field, the size of the fixed type is actually smaller
    than the amount of space allocated to that field, and thus we would
    compute the wrong offset of field After.

    To make things more complicated, we need to watch out for dynamic
    components of variant records (identified by the ___XVL suffix in
    the component name).  Even if the target type is a PAD type, the size
    of that type might not be statically known.  So the PAD type needs
    to be unwrapped and the resulting type needs to be fixed.  Otherwise,
    we might end up with the wrong size for our component.  This can be
    observed with the following type declarations:

        type Octal is new Integer range 0 .. 7;
        type Octal_Array is array (Positive range <>) of Octal;
        pragma Pack (Octal_Array);

        type Octal_Buffer (Size : Positive) is record
           Buffer : Octal_Array (1 .. Size);
           Length : Integer;
        end record;

    In that case, Buffer is a PAD type whose size is unset and needs
    to be computed by fixing the unwrapped type.

    4. When to ``Fix'' un-``Fixed'' sub-elements of an entity:
    ----------------------------------------------------------

    Lastly, when should the sub-elements of an entity that remained unfixed
    thus far, be actually fixed?

    The answer is: Only when referencing that element.  For instance
    when selecting one component of a record, this specific component
    should be fixed at that point in time.  Or when printing the value
    of a record, each component should be fixed before its value gets
    printed.  Similarly for arrays, the element of the array should be
    fixed when printing each element of the array, or when extracting
    one element out of that array.  On the other hand, fixing should
    not be performed on the elements when taking a slice of an array!

    Note that one of the side-effects of miscomputing the offset and
    size of each field is that we end up also miscomputing the size
    of the containing type.  This can have adverse results when computing
    the value of an entity.  GDB fetches the value of an entity based
    on the size of its type, and thus a wrong size causes GDB to fetch
    the wrong amount of memory.  In the case where the computed size is
    too small, GDB fetches too little data to print the value of our
    entiry.  Results in this case as unpredicatble, as we usually read
    past the buffer containing the data =:-o.  */

/* Implement the evaluate_exp routine in the exp_descriptor structure
   for the Ada language.  */

static struct value *
ada_evaluate_subexp (struct type *expect_type, struct expression *exp,
                     int *pos, enum noside noside)
{
  enum exp_opcode op;
  int tem;
  int pc;
  int preeval_pos;
  struct value *arg1 = NULL, *arg2 = NULL, *arg3;
  struct type *type;
  int nargs, oplen;
  struct value **argvec;

  pc = *pos;
  *pos += 1;
  op = exp->elts[pc].opcode;

  switch (op)
    {
    default:
      *pos -= 1;
      arg1 = evaluate_subexp_standard (expect_type, exp, pos, noside);

      if (noside == EVAL_NORMAL)
	arg1 = unwrap_value (arg1);

      /* If evaluating an OP_DOUBLE and an EXPECT_TYPE was provided,
         then we need to perform the conversion manually, because
         evaluate_subexp_standard doesn't do it.  This conversion is
         necessary in Ada because the different kinds of float/fixed
         types in Ada have different representations.

         Similarly, we need to perform the conversion from OP_LONG
         ourselves.  */
      if ((op == OP_DOUBLE || op == OP_LONG) && expect_type != NULL)
        arg1 = ada_value_cast (expect_type, arg1, noside);

      return arg1;

    case OP_STRING:
      {
        struct value *result;

        *pos -= 1;
        result = evaluate_subexp_standard (expect_type, exp, pos, noside);
        /* The result type will have code OP_STRING, bashed there from 
           OP_ARRAY.  Bash it back.  */
        if (TYPE_CODE (value_type (result)) == TYPE_CODE_STRING)
          TYPE_CODE (value_type (result)) = TYPE_CODE_ARRAY;
        return result;
      }

    case UNOP_CAST:
      (*pos) += 2;
      type = exp->elts[pc + 1].type;
      arg1 = evaluate_subexp (type, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      arg1 = ada_value_cast (type, arg1, noside);
      return arg1;

    case UNOP_QUAL:
      (*pos) += 2;
      type = exp->elts[pc + 1].type;
      return ada_evaluate_subexp (type, exp, pos, noside);

    case BINOP_ASSIGN:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (exp->elts[*pos].opcode == OP_AGGREGATE)
	{
	  arg1 = assign_aggregate (arg1, arg1, exp, pos, noside);
	  if (noside == EVAL_SKIP || noside == EVAL_AVOID_SIDE_EFFECTS)
	    return arg1;
	  return ada_value_assign (arg1, arg1);
	}
      /* Force the evaluation of the rhs ARG2 to the type of the lhs ARG1,
         except if the lhs of our assignment is a convenience variable.
         In the case of assigning to a convenience variable, the lhs
         should be exactly the result of the evaluation of the rhs.  */
      type = value_type (arg1);
      if (VALUE_LVAL (arg1) == lval_internalvar)
         type = NULL;
      arg2 = evaluate_subexp (type, exp, pos, noside);
      if (noside == EVAL_SKIP || noside == EVAL_AVOID_SIDE_EFFECTS)
        return arg1;
      if (ada_is_fixed_point_type (value_type (arg1)))
        arg2 = cast_to_fixed (value_type (arg1), arg2);
      else if (ada_is_fixed_point_type (value_type (arg2)))
        error
          (_("Fixed-point values must be assigned to fixed-point variables"));
      else
        arg2 = coerce_for_assign (value_type (arg1), arg2);
      return ada_value_assign (arg1, arg2);

    case BINOP_ADD:
      arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
      arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      if (TYPE_CODE (value_type (arg1)) == TYPE_CODE_PTR)
        return (value_from_longest
                 (value_type (arg1),
                  value_as_long (arg1) + value_as_long (arg2)));
      if (TYPE_CODE (value_type (arg2)) == TYPE_CODE_PTR)
        return (value_from_longest
                 (value_type (arg2),
                  value_as_long (arg1) + value_as_long (arg2)));
      if ((ada_is_fixed_point_type (value_type (arg1))
           || ada_is_fixed_point_type (value_type (arg2)))
          && value_type (arg1) != value_type (arg2))
        error (_("Operands of fixed-point addition must have the same type"));
      /* Do the addition, and cast the result to the type of the first
         argument.  We cannot cast the result to a reference type, so if
         ARG1 is a reference type, find its underlying type.  */
      type = value_type (arg1);
      while (TYPE_CODE (type) == TYPE_CODE_REF)
        type = TYPE_TARGET_TYPE (type);
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      return value_cast (type, value_binop (arg1, arg2, BINOP_ADD));

    case BINOP_SUB:
      arg1 = evaluate_subexp_with_coercion (exp, pos, noside);
      arg2 = evaluate_subexp_with_coercion (exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      if (TYPE_CODE (value_type (arg1)) == TYPE_CODE_PTR)
        return (value_from_longest
                 (value_type (arg1),
                  value_as_long (arg1) - value_as_long (arg2)));
      if (TYPE_CODE (value_type (arg2)) == TYPE_CODE_PTR)
        return (value_from_longest
                 (value_type (arg2),
                  value_as_long (arg1) - value_as_long (arg2)));
      if ((ada_is_fixed_point_type (value_type (arg1))
           || ada_is_fixed_point_type (value_type (arg2)))
          && value_type (arg1) != value_type (arg2))
        error (_("Operands of fixed-point subtraction "
		 "must have the same type"));
      /* Do the substraction, and cast the result to the type of the first
         argument.  We cannot cast the result to a reference type, so if
         ARG1 is a reference type, find its underlying type.  */
      type = value_type (arg1);
      while (TYPE_CODE (type) == TYPE_CODE_REF)
        type = TYPE_TARGET_TYPE (type);
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      return value_cast (type, value_binop (arg1, arg2, BINOP_SUB));

    case BINOP_MUL:
    case BINOP_DIV:
    case BINOP_REM:
    case BINOP_MOD:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
        {
          binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
          return value_zero (value_type (arg1), not_lval);
        }
      else
        {
          type = builtin_type (exp->gdbarch)->builtin_double;
          if (ada_is_fixed_point_type (value_type (arg1)))
            arg1 = cast_from_fixed (type, arg1);
          if (ada_is_fixed_point_type (value_type (arg2)))
            arg2 = cast_from_fixed (type, arg2);
          binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
          return ada_value_binop (arg1, arg2, op);
        }

    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (value_type (arg1), exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
        tem = 0;
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  tem = ada_value_equal (arg1, arg2);
	}
      if (op == BINOP_NOTEQUAL)
        tem = !tem;
      type = language_bool_type (exp->language_defn, exp->gdbarch);
      return value_from_longest (type, (LONGEST) tem);

    case UNOP_NEG:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      else if (ada_is_fixed_point_type (value_type (arg1)))
        return value_cast (value_type (arg1), value_neg (arg1));
      else
	{
	  unop_promote (exp->language_defn, exp->gdbarch, &arg1);
	  return value_neg (arg1);
	}

    case BINOP_LOGICAL_AND:
    case BINOP_LOGICAL_OR:
    case UNOP_LOGICAL_NOT:
      {
        struct value *val;

        *pos -= 1;
        val = evaluate_subexp_standard (expect_type, exp, pos, noside);
	type = language_bool_type (exp->language_defn, exp->gdbarch);
        return value_cast (type, val);
      }

    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
      {
        struct value *val;

        arg1 = evaluate_subexp (NULL_TYPE, exp, pos, EVAL_AVOID_SIDE_EFFECTS);
        *pos = pc;
        val = evaluate_subexp_standard (expect_type, exp, pos, noside);

        return value_cast (value_type (arg1), val);
      }

    case OP_VAR_VALUE:
      *pos -= 1;

      if (noside == EVAL_SKIP)
        {
          *pos += 4;
          goto nosideret;
        }

      if (SYMBOL_DOMAIN (exp->elts[pc + 2].symbol) == UNDEF_DOMAIN)
        /* Only encountered when an unresolved symbol occurs in a
           context other than a function call, in which case, it is
           invalid.  */
        error (_("Unexpected unresolved symbol, %s, during evaluation"),
               SYMBOL_PRINT_NAME (exp->elts[pc + 2].symbol));

      if (noside == EVAL_AVOID_SIDE_EFFECTS)
        {
          type = static_unwrap_type (SYMBOL_TYPE (exp->elts[pc + 2].symbol));
          /* Check to see if this is a tagged type.  We also need to handle
             the case where the type is a reference to a tagged type, but
             we have to be careful to exclude pointers to tagged types.
             The latter should be shown as usual (as a pointer), whereas
             a reference should mostly be transparent to the user.  */
          if (ada_is_tagged_type (type, 0)
              || (TYPE_CODE (type) == TYPE_CODE_REF
                  && ada_is_tagged_type (TYPE_TARGET_TYPE (type), 0)))
	    {
	      /* Tagged types are a little special in the fact that the real
		 type is dynamic and can only be determined by inspecting the
		 object's tag.  This means that we need to get the object's
		 value first (EVAL_NORMAL) and then extract the actual object
		 type from its tag.

		 Note that we cannot skip the final step where we extract
		 the object type from its tag, because the EVAL_NORMAL phase
		 results in dynamic components being resolved into fixed ones.
		 This can cause problems when trying to print the type
		 description of tagged types whose parent has a dynamic size:
		 We use the type name of the "_parent" component in order
		 to print the name of the ancestor type in the type description.
		 If that component had a dynamic size, the resolution into
		 a fixed type would result in the loss of that type name,
		 thus preventing us from printing the name of the ancestor
		 type in the type description.  */
	      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, EVAL_NORMAL);

	      if (TYPE_CODE (type) != TYPE_CODE_REF)
		{
		  struct type *actual_type;

		  actual_type = type_from_tag (ada_value_tag (arg1));
		  if (actual_type == NULL)
		    /* If, for some reason, we were unable to determine
		       the actual type from the tag, then use the static
		       approximation that we just computed as a fallback.
		       This can happen if the debugging information is
		       incomplete, for instance.  */
		    actual_type = type;
		  return value_zero (actual_type, not_lval);
		}
	      else
		{
		  /* In the case of a ref, ada_coerce_ref takes care
		     of determining the actual type.  But the evaluation
		     should return a ref as it should be valid to ask
		     for its address; so rebuild a ref after coerce.  */
		  arg1 = ada_coerce_ref (arg1);
		  return value_ref (arg1);
		}
	    }

	  /* Records and unions for which GNAT encodings have been
	     generated need to be statically fixed as well.
	     Otherwise, non-static fixing produces a type where
	     all dynamic properties are removed, which prevents "ptype"
	     from being able to completely describe the type.
	     For instance, a case statement in a variant record would be
	     replaced by the relevant components based on the actual
	     value of the discriminants.  */
	  if ((TYPE_CODE (type) == TYPE_CODE_STRUCT
	       && dynamic_template_type (type) != NULL)
	      || (TYPE_CODE (type) == TYPE_CODE_UNION
		  && ada_find_parallel_type (type, "___XVU") != NULL))
	    {
	      *pos += 4;
	      return value_zero (to_static_fixed_type (type), not_lval);
	    }
        }

      arg1 = evaluate_subexp_standard (expect_type, exp, pos, noside);
      return ada_to_fixed_value (arg1);

    case OP_FUNCALL:
      (*pos) += 2;

      /* Allocate arg vector, including space for the function to be
         called in argvec[0] and a terminating NULL.  */
      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      argvec = XALLOCAVEC (struct value *, nargs + 2);

      if (exp->elts[*pos].opcode == OP_VAR_VALUE
          && SYMBOL_DOMAIN (exp->elts[pc + 5].symbol) == UNDEF_DOMAIN)
        error (_("Unexpected unresolved symbol, %s, during evaluation"),
               SYMBOL_PRINT_NAME (exp->elts[pc + 5].symbol));
      else
        {
          for (tem = 0; tem <= nargs; tem += 1)
            argvec[tem] = evaluate_subexp (NULL_TYPE, exp, pos, noside);
          argvec[tem] = 0;

          if (noside == EVAL_SKIP)
            goto nosideret;
        }

      if (ada_is_constrained_packed_array_type
	  (desc_base_type (value_type (argvec[0]))))
        argvec[0] = ada_coerce_to_simple_array (argvec[0]);
      else if (TYPE_CODE (value_type (argvec[0])) == TYPE_CODE_ARRAY
               && TYPE_FIELD_BITSIZE (value_type (argvec[0]), 0) != 0)
        /* This is a packed array that has already been fixed, and
	   therefore already coerced to a simple array.  Nothing further
	   to do.  */
        ;
      else if (TYPE_CODE (value_type (argvec[0])) == TYPE_CODE_REF)
	{
	  /* Make sure we dereference references so that all the code below
	     feels like it's really handling the referenced value.  Wrapping
	     types (for alignment) may be there, so make sure we strip them as
	     well.  */
	  argvec[0] = ada_to_fixed_value (coerce_ref (argvec[0]));
	}
      else if (TYPE_CODE (value_type (argvec[0])) == TYPE_CODE_ARRAY
	       && VALUE_LVAL (argvec[0]) == lval_memory)
	argvec[0] = value_addr (argvec[0]);

      type = ada_check_typedef (value_type (argvec[0]));

      /* Ada allows us to implicitly dereference arrays when subscripting
	 them.  So, if this is an array typedef (encoding use for array
	 access types encoded as fat pointers), strip it now.  */
      if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
	type = ada_typedef_target_type (type);

      if (TYPE_CODE (type) == TYPE_CODE_PTR)
        {
          switch (TYPE_CODE (ada_check_typedef (TYPE_TARGET_TYPE (type))))
            {
            case TYPE_CODE_FUNC:
              type = ada_check_typedef (TYPE_TARGET_TYPE (type));
              break;
            case TYPE_CODE_ARRAY:
              break;
            case TYPE_CODE_STRUCT:
              if (noside != EVAL_AVOID_SIDE_EFFECTS)
                argvec[0] = ada_value_ind (argvec[0]);
              type = ada_check_typedef (TYPE_TARGET_TYPE (type));
              break;
            default:
              error (_("cannot subscript or call something of type `%s'"),
                     ada_type_name (value_type (argvec[0])));
              break;
            }
        }

      switch (TYPE_CODE (type))
        {
        case TYPE_CODE_FUNC:
          if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    {
	      struct type *rtype = TYPE_TARGET_TYPE (type);

	      if (TYPE_GNU_IFUNC (type))
		return allocate_value (TYPE_TARGET_TYPE (rtype));
	      return allocate_value (rtype);
	    }
          return call_function_by_hand (argvec[0], nargs, argvec + 1);
	case TYPE_CODE_INTERNAL_FUNCTION:
	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    /* We don't know anything about what the internal
	       function might return, but we have to return
	       something.  */
	    return value_zero (builtin_type (exp->gdbarch)->builtin_int,
			       not_lval);
	  else
	    return call_internal_function (exp->gdbarch, exp->language_defn,
					   argvec[0], nargs, argvec + 1);

        case TYPE_CODE_STRUCT:
          {
            int arity;

            arity = ada_array_arity (type);
            type = ada_array_element_type (type, nargs);
            if (type == NULL)
              error (_("cannot subscript or call a record"));
            if (arity != nargs)
              error (_("wrong number of subscripts; expecting %d"), arity);
            if (noside == EVAL_AVOID_SIDE_EFFECTS)
              return value_zero (ada_aligned_type (type), lval_memory);
            return
              unwrap_value (ada_value_subscript
                            (argvec[0], nargs, argvec + 1));
          }
        case TYPE_CODE_ARRAY:
          if (noside == EVAL_AVOID_SIDE_EFFECTS)
            {
              type = ada_array_element_type (type, nargs);
              if (type == NULL)
                error (_("element type of array unknown"));
              else
                return value_zero (ada_aligned_type (type), lval_memory);
            }
          return
            unwrap_value (ada_value_subscript
                          (ada_coerce_to_simple_array (argvec[0]),
                           nargs, argvec + 1));
        case TYPE_CODE_PTR:     /* Pointer to array */
          if (noside == EVAL_AVOID_SIDE_EFFECTS)
            {
	      type = to_fixed_array_type (TYPE_TARGET_TYPE (type), NULL, 1);
              type = ada_array_element_type (type, nargs);
              if (type == NULL)
                error (_("element type of array unknown"));
              else
                return value_zero (ada_aligned_type (type), lval_memory);
            }
          return
            unwrap_value (ada_value_ptr_subscript (argvec[0],
						   nargs, argvec + 1));

        default:
          error (_("Attempt to index or call something other than an "
		   "array or function"));
        }

    case TERNOP_SLICE:
      {
        struct value *array = evaluate_subexp (NULL_TYPE, exp, pos, noside);
        struct value *low_bound_val =
          evaluate_subexp (NULL_TYPE, exp, pos, noside);
        struct value *high_bound_val =
          evaluate_subexp (NULL_TYPE, exp, pos, noside);
        LONGEST low_bound;
        LONGEST high_bound;

        low_bound_val = coerce_ref (low_bound_val);
        high_bound_val = coerce_ref (high_bound_val);
        low_bound = value_as_long (low_bound_val);
        high_bound = value_as_long (high_bound_val);

        if (noside == EVAL_SKIP)
          goto nosideret;

        /* If this is a reference to an aligner type, then remove all
           the aligners.  */
        if (TYPE_CODE (value_type (array)) == TYPE_CODE_REF
            && ada_is_aligner_type (TYPE_TARGET_TYPE (value_type (array))))
          TYPE_TARGET_TYPE (value_type (array)) =
            ada_aligned_type (TYPE_TARGET_TYPE (value_type (array)));

        if (ada_is_constrained_packed_array_type (value_type (array)))
          error (_("cannot slice a packed array"));

        /* If this is a reference to an array or an array lvalue,
           convert to a pointer.  */
        if (TYPE_CODE (value_type (array)) == TYPE_CODE_REF
            || (TYPE_CODE (value_type (array)) == TYPE_CODE_ARRAY
                && VALUE_LVAL (array) == lval_memory))
          array = value_addr (array);

        if (noside == EVAL_AVOID_SIDE_EFFECTS
            && ada_is_array_descriptor_type (ada_check_typedef
                                             (value_type (array))))
          return empty_array (ada_type_of_array (array, 0), low_bound);

        array = ada_coerce_to_simple_array_ptr (array);

        /* If we have more than one level of pointer indirection,
           dereference the value until we get only one level.  */
        while (TYPE_CODE (value_type (array)) == TYPE_CODE_PTR
               && (TYPE_CODE (TYPE_TARGET_TYPE (value_type (array)))
                     == TYPE_CODE_PTR))
          array = value_ind (array);

        /* Make sure we really do have an array type before going further,
           to avoid a SEGV when trying to get the index type or the target
           type later down the road if the debug info generated by
           the compiler is incorrect or incomplete.  */
        if (!ada_is_simple_array_type (value_type (array)))
          error (_("cannot take slice of non-array"));

        if (TYPE_CODE (ada_check_typedef (value_type (array)))
            == TYPE_CODE_PTR)
          {
            struct type *type0 = ada_check_typedef (value_type (array));

            if (high_bound < low_bound || noside == EVAL_AVOID_SIDE_EFFECTS)
              return empty_array (TYPE_TARGET_TYPE (type0), low_bound);
            else
              {
                struct type *arr_type0 =
                  to_fixed_array_type (TYPE_TARGET_TYPE (type0), NULL, 1);

                return ada_value_slice_from_ptr (array, arr_type0,
                                                 longest_to_int (low_bound),
                                                 longest_to_int (high_bound));
              }
          }
        else if (noside == EVAL_AVOID_SIDE_EFFECTS)
          return array;
        else if (high_bound < low_bound)
          return empty_array (value_type (array), low_bound);
        else
          return ada_value_slice (array, longest_to_int (low_bound),
				  longest_to_int (high_bound));
      }

    case UNOP_IN_RANGE:
      (*pos) += 2;
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      type = check_typedef (exp->elts[pc + 1].type);

      if (noside == EVAL_SKIP)
        goto nosideret;

      switch (TYPE_CODE (type))
        {
        default:
          lim_warning (_("Membership test incompletely implemented; "
			 "always returns true"));
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_from_longest (type, (LONGEST) 1);

        case TYPE_CODE_RANGE:
	  arg2 = value_from_longest (type, TYPE_LOW_BOUND (type));
	  arg3 = value_from_longest (type, TYPE_HIGH_BOUND (type));
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg3);
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return
	    value_from_longest (type,
                                (value_less (arg1, arg3)
                                 || value_equal (arg1, arg3))
                                && (value_less (arg2, arg1)
                                    || value_equal (arg2, arg1)));
        }

    case BINOP_IN_BOUNDS:
      (*pos) += 2;
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);

      if (noside == EVAL_SKIP)
        goto nosideret;

      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  type = language_bool_type (exp->language_defn, exp->gdbarch);
	  return value_zero (type, not_lval);
	}

      tem = longest_to_int (exp->elts[pc + 1].longconst);

      type = ada_index_type (value_type (arg2), tem, "range");
      if (!type)
	type = value_type (arg1);

      arg3 = value_from_longest (type, ada_array_bound (arg2, tem, 1));
      arg2 = value_from_longest (type, ada_array_bound (arg2, tem, 0));

      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg3);
      type = language_bool_type (exp->language_defn, exp->gdbarch);
      return
        value_from_longest (type,
                            (value_less (arg1, arg3)
                             || value_equal (arg1, arg3))
                            && (value_less (arg2, arg1)
                                || value_equal (arg2, arg1)));

    case TERNOP_IN_RANGE:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg3 = evaluate_subexp (NULL_TYPE, exp, pos, noside);

      if (noside == EVAL_SKIP)
        goto nosideret;

      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg3);
      type = language_bool_type (exp->language_defn, exp->gdbarch);
      return
        value_from_longest (type,
                            (value_less (arg1, arg3)
                             || value_equal (arg1, arg3))
                            && (value_less (arg2, arg1)
                                || value_equal (arg2, arg1)));

    case OP_ATR_FIRST:
    case OP_ATR_LAST:
    case OP_ATR_LENGTH:
      {
        struct type *type_arg;

        if (exp->elts[*pos].opcode == OP_TYPE)
          {
            evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
            arg1 = NULL;
            type_arg = check_typedef (exp->elts[pc + 2].type);
          }
        else
          {
            arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
            type_arg = NULL;
          }

        if (exp->elts[*pos].opcode != OP_LONG)
          error (_("Invalid operand to '%s"), ada_attribute_name (op));
        tem = longest_to_int (exp->elts[*pos + 2].longconst);
        *pos += 4;

        if (noside == EVAL_SKIP)
          goto nosideret;

        if (type_arg == NULL)
          {
            arg1 = ada_coerce_ref (arg1);

            if (ada_is_constrained_packed_array_type (value_type (arg1)))
              arg1 = ada_coerce_to_simple_array (arg1);

            if (op == OP_ATR_LENGTH)
	      type = builtin_type (exp->gdbarch)->builtin_int;
	    else
	      {
		type = ada_index_type (value_type (arg1), tem,
				       ada_attribute_name (op));
		if (type == NULL)
		  type = builtin_type (exp->gdbarch)->builtin_int;
	      }

            if (noside == EVAL_AVOID_SIDE_EFFECTS)
              return allocate_value (type);

            switch (op)
              {
              default:          /* Should never happen.  */
                error (_("unexpected attribute encountered"));
              case OP_ATR_FIRST:
                return value_from_longest
			(type, ada_array_bound (arg1, tem, 0));
              case OP_ATR_LAST:
                return value_from_longest
			(type, ada_array_bound (arg1, tem, 1));
              case OP_ATR_LENGTH:
                return value_from_longest
			(type, ada_array_length (arg1, tem));
              }
          }
        else if (discrete_type_p (type_arg))
          {
            struct type *range_type;
            const char *name = ada_type_name (type_arg);

            range_type = NULL;
            if (name != NULL && TYPE_CODE (type_arg) != TYPE_CODE_ENUM)
              range_type = to_fixed_range_type (type_arg, NULL);
            if (range_type == NULL)
              range_type = type_arg;
            switch (op)
              {
              default:
                error (_("unexpected attribute encountered"));
              case OP_ATR_FIRST:
		return value_from_longest 
		  (range_type, ada_discrete_type_low_bound (range_type));
              case OP_ATR_LAST:
                return value_from_longest
		  (range_type, ada_discrete_type_high_bound (range_type));
              case OP_ATR_LENGTH:
                error (_("the 'length attribute applies only to array types"));
              }
          }
        else if (TYPE_CODE (type_arg) == TYPE_CODE_FLT)
          error (_("unimplemented type attribute"));
        else
          {
            LONGEST low, high;

            if (ada_is_constrained_packed_array_type (type_arg))
              type_arg = decode_constrained_packed_array_type (type_arg);

	    if (op == OP_ATR_LENGTH)
	      type = builtin_type (exp->gdbarch)->builtin_int;
	    else
	      {
		type = ada_index_type (type_arg, tem, ada_attribute_name (op));
		if (type == NULL)
		  type = builtin_type (exp->gdbarch)->builtin_int;
	      }

            if (noside == EVAL_AVOID_SIDE_EFFECTS)
              return allocate_value (type);

            switch (op)
              {
              default:
                error (_("unexpected attribute encountered"));
              case OP_ATR_FIRST:
                low = ada_array_bound_from_type (type_arg, tem, 0);
                return value_from_longest (type, low);
              case OP_ATR_LAST:
                high = ada_array_bound_from_type (type_arg, tem, 1);
                return value_from_longest (type, high);
              case OP_ATR_LENGTH:
                low = ada_array_bound_from_type (type_arg, tem, 0);
                high = ada_array_bound_from_type (type_arg, tem, 1);
                return value_from_longest (type, high - low + 1);
              }
          }
      }

    case OP_ATR_TAG:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;

      if (noside == EVAL_AVOID_SIDE_EFFECTS)
        return value_zero (ada_tag_type (arg1), not_lval);

      return ada_value_tag (arg1);

    case OP_ATR_MIN:
    case OP_ATR_MAX:
      evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
        return value_zero (value_type (arg1), not_lval);
      else
	{
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
	  return value_binop (arg1, arg2,
			      op == OP_ATR_MIN ? BINOP_MIN : BINOP_MAX);
	}

    case OP_ATR_MODULUS:
      {
        struct type *type_arg = check_typedef (exp->elts[pc + 2].type);

        evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
        if (noside == EVAL_SKIP)
          goto nosideret;

        if (!ada_is_modular_type (type_arg))
          error (_("'modulus must be applied to modular type"));

        return value_from_longest (TYPE_TARGET_TYPE (type_arg),
                                   ada_modulus (type_arg));
      }


    case OP_ATR_POS:
      evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      type = builtin_type (exp->gdbarch)->builtin_int;
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	return value_zero (type, not_lval);
      else
	return value_pos_atr (type, arg1);

    case OP_ATR_SIZE:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      type = value_type (arg1);

      /* If the argument is a reference, then dereference its type, since
         the user is really asking for the size of the actual object,
         not the size of the pointer.  */
      if (TYPE_CODE (type) == TYPE_CODE_REF)
        type = TYPE_TARGET_TYPE (type);

      if (noside == EVAL_SKIP)
        goto nosideret;
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
        return value_zero (builtin_type (exp->gdbarch)->builtin_int, not_lval);
      else
        return value_from_longest (builtin_type (exp->gdbarch)->builtin_int,
                                   TARGET_CHAR_BIT * TYPE_LENGTH (type));

    case OP_ATR_VAL:
      evaluate_subexp (NULL_TYPE, exp, pos, EVAL_SKIP);
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      type = exp->elts[pc + 2].type;
      if (noside == EVAL_SKIP)
        goto nosideret;
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
        return value_zero (type, not_lval);
      else
        return value_val_atr (type, arg1);

    case BINOP_EXP:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      arg2 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
        return value_zero (value_type (arg1), not_lval);
      else
	{
	  /* For integer exponentiation operations,
	     only promote the first argument.  */
	  if (is_integral_type (value_type (arg2)))
	    unop_promote (exp->language_defn, exp->gdbarch, &arg1);
	  else
	    binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);

	  return value_binop (arg1, arg2, op);
	}

    case UNOP_PLUS:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      else
        return arg1;

    case UNOP_ABS:
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      unop_promote (exp->language_defn, exp->gdbarch, &arg1);
      if (value_less (arg1, value_zero (value_type (arg1), not_lval)))
        return value_neg (arg1);
      else
        return arg1;

    case UNOP_IND:
      preeval_pos = *pos;
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      type = ada_check_typedef (value_type (arg1));
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
        {
          if (ada_is_array_descriptor_type (type))
            /* GDB allows dereferencing GNAT array descriptors.  */
            {
              struct type *arrType = ada_type_of_array (arg1, 0);

              if (arrType == NULL)
                error (_("Attempt to dereference null array pointer."));
              return value_at_lazy (arrType, 0);
            }
          else if (TYPE_CODE (type) == TYPE_CODE_PTR
                   || TYPE_CODE (type) == TYPE_CODE_REF
                   /* In C you can dereference an array to get the 1st elt.  */
                   || TYPE_CODE (type) == TYPE_CODE_ARRAY)
            {
            /* As mentioned in the OP_VAR_VALUE case, tagged types can
               only be determined by inspecting the object's tag.
               This means that we need to evaluate completely the
               expression in order to get its type.  */

	      if ((TYPE_CODE (type) == TYPE_CODE_REF
		   || TYPE_CODE (type) == TYPE_CODE_PTR)
		  && ada_is_tagged_type (TYPE_TARGET_TYPE (type), 0))
		{
		  arg1 = evaluate_subexp (NULL_TYPE, exp, &preeval_pos,
					  EVAL_NORMAL);
		  type = value_type (ada_value_ind (arg1));
		}
	      else
		{
		  type = to_static_fixed_type
		    (ada_aligned_type
		     (ada_check_typedef (TYPE_TARGET_TYPE (type))));
		}
	      ada_ensure_varsize_limit (type);
              return value_zero (type, lval_memory);
            }
          else if (TYPE_CODE (type) == TYPE_CODE_INT)
	    {
	      /* GDB allows dereferencing an int.  */
	      if (expect_type == NULL)
		return value_zero (builtin_type (exp->gdbarch)->builtin_int,
				   lval_memory);
	      else
		{
		  expect_type = 
		    to_static_fixed_type (ada_aligned_type (expect_type));
		  return value_zero (expect_type, lval_memory);
		}
	    }
          else
            error (_("Attempt to take contents of a non-pointer value."));
        }
      arg1 = ada_coerce_ref (arg1);     /* FIXME: What is this for??  */
      type = ada_check_typedef (value_type (arg1));

      if (TYPE_CODE (type) == TYPE_CODE_INT)
          /* GDB allows dereferencing an int.  If we were given
             the expect_type, then use that as the target type.
             Otherwise, assume that the target type is an int.  */
        {
          if (expect_type != NULL)
	    return ada_value_ind (value_cast (lookup_pointer_type (expect_type),
					      arg1));
	  else
	    return value_at_lazy (builtin_type (exp->gdbarch)->builtin_int,
				  (CORE_ADDR) value_as_address (arg1));
        }

      if (ada_is_array_descriptor_type (type))
        /* GDB allows dereferencing GNAT array descriptors.  */
        return ada_coerce_to_simple_array (arg1);
      else
        return ada_value_ind (arg1);

    case STRUCTOP_STRUCT:
      tem = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
      preeval_pos = *pos;
      arg1 = evaluate_subexp (NULL_TYPE, exp, pos, noside);
      if (noside == EVAL_SKIP)
        goto nosideret;
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
        {
          struct type *type1 = value_type (arg1);

          if (ada_is_tagged_type (type1, 1))
            {
              type = ada_lookup_struct_elt_type (type1,
                                                 &exp->elts[pc + 2].string,
                                                 1, 1, NULL);

	      /* If the field is not found, check if it exists in the
		 extension of this object's type. This means that we
		 need to evaluate completely the expression.  */

              if (type == NULL)
		{
		  arg1 = evaluate_subexp (NULL_TYPE, exp, &preeval_pos,
					  EVAL_NORMAL);
		  arg1 = ada_value_struct_elt (arg1,
					       &exp->elts[pc + 2].string,
					       0);
		  arg1 = unwrap_value (arg1);
		  type = value_type (ada_to_fixed_value (arg1));
		}
            }
          else
            type =
              ada_lookup_struct_elt_type (type1, &exp->elts[pc + 2].string, 1,
                                          0, NULL);

          return value_zero (ada_aligned_type (type), lval_memory);
        }
      else
	{
	  arg1 = ada_value_struct_elt (arg1, &exp->elts[pc + 2].string, 0);
	  arg1 = unwrap_value (arg1);
	  return ada_to_fixed_value (arg1);
	}

    case OP_TYPE:
      /* The value is not supposed to be used.  This is here to make it
         easier to accommodate expressions that contain types.  */
      (*pos) += 2;
      if (noside == EVAL_SKIP)
        goto nosideret;
      else if (noside == EVAL_AVOID_SIDE_EFFECTS)
        return allocate_value (exp->elts[pc + 1].type);
      else
        error (_("Attempt to use a type name as an expression"));

    case OP_AGGREGATE:
    case OP_CHOICES:
    case OP_OTHERS:
    case OP_DISCRETE_RANGE:
    case OP_POSITIONAL:
    case OP_NAME:
      if (noside == EVAL_NORMAL)
	switch (op) 
	  {
	  case OP_NAME:
	    error (_("Undefined name, ambiguous name, or renaming used in "
		     "component association: %s."), &exp->elts[pc+2].string);
	  case OP_AGGREGATE:
	    error (_("Aggregates only allowed on the right of an assignment"));
	  default:
	    internal_error (__FILE__, __LINE__,
			    _("aggregate apparently mangled"));
	  }

      ada_forward_operator_length (exp, pc, &oplen, &nargs);
      *pos += oplen - 1;
      for (tem = 0; tem < nargs; tem += 1) 
	ada_evaluate_subexp (NULL, exp, pos, noside);
      goto nosideret;
    }

nosideret:
  return value_from_longest (builtin_type (exp->gdbarch)->builtin_int, 1);
}


                                /* Fixed point */

/* If TYPE encodes an Ada fixed-point type, return the suffix of the
   type name that encodes the 'small and 'delta information.
   Otherwise, return NULL.  */

static const char *
fixed_type_info (struct type *type)
{
  const char *name = ada_type_name (type);
  enum type_code code = (type == NULL) ? TYPE_CODE_UNDEF : TYPE_CODE (type);

  if ((code == TYPE_CODE_INT || code == TYPE_CODE_RANGE) && name != NULL)
    {
      const char *tail = strstr (name, "___XF_");

      if (tail == NULL)
        return NULL;
      else
        return tail + 5;
    }
  else if (code == TYPE_CODE_RANGE && TYPE_TARGET_TYPE (type) != type)
    return fixed_type_info (TYPE_TARGET_TYPE (type));
  else
    return NULL;
}

/* Returns non-zero iff TYPE represents an Ada fixed-point type.  */

int
ada_is_fixed_point_type (struct type *type)
{
  return fixed_type_info (type) != NULL;
}

/* Return non-zero iff TYPE represents a System.Address type.  */

int
ada_is_system_address_type (struct type *type)
{
  return (TYPE_NAME (type)
          && strcmp (TYPE_NAME (type), "system__address") == 0);
}

/* Assuming that TYPE is the representation of an Ada fixed-point
   type, return its delta, or -1 if the type is malformed and the
   delta cannot be determined.  */

DOUBLEST
ada_delta (struct type *type)
{
  const char *encoding = fixed_type_info (type);
  DOUBLEST num, den;

  /* Strictly speaking, num and den are encoded as integer.  However,
     they may not fit into a long, and they will have to be converted
     to DOUBLEST anyway.  So scan them as DOUBLEST.  */
  if (sscanf (encoding, "_%" DOUBLEST_SCAN_FORMAT "_%" DOUBLEST_SCAN_FORMAT,
	      &num, &den) < 2)
    return -1.0;
  else
    return num / den;
}

/* Assuming that ada_is_fixed_point_type (TYPE), return the scaling
   factor ('SMALL value) associated with the type.  */

static DOUBLEST
scaling_factor (struct type *type)
{
  const char *encoding = fixed_type_info (type);
  DOUBLEST num0, den0, num1, den1;
  int n;

  /* Strictly speaking, num's and den's are encoded as integer.  However,
     they may not fit into a long, and they will have to be converted
     to DOUBLEST anyway.  So scan them as DOUBLEST.  */
  n = sscanf (encoding,
	      "_%" DOUBLEST_SCAN_FORMAT "_%" DOUBLEST_SCAN_FORMAT
	      "_%" DOUBLEST_SCAN_FORMAT "_%" DOUBLEST_SCAN_FORMAT,
	      &num0, &den0, &num1, &den1);

  if (n < 2)
    return 1.0;
  else if (n == 4)
    return num1 / den1;
  else
    return num0 / den0;
}


/* Assuming that X is the representation of a value of fixed-point
   type TYPE, return its floating-point equivalent.  */

DOUBLEST
ada_fixed_to_float (struct type *type, LONGEST x)
{
  return (DOUBLEST) x *scaling_factor (type);
}

/* The representation of a fixed-point value of type TYPE
   corresponding to the value X.  */

LONGEST
ada_float_to_fixed (struct type *type, DOUBLEST x)
{
  return (LONGEST) (x / scaling_factor (type) + 0.5);
}



                                /* Range types */

/* Scan STR beginning at position K for a discriminant name, and
   return the value of that discriminant field of DVAL in *PX.  If
   PNEW_K is not null, put the position of the character beyond the
   name scanned in *PNEW_K.  Return 1 if successful; return 0 and do
   not alter *PX and *PNEW_K if unsuccessful.  */

static int
scan_discrim_bound (const char *str, int k, struct value *dval, LONGEST * px,
                    int *pnew_k)
{
  static char *bound_buffer = NULL;
  static size_t bound_buffer_len = 0;
  const char *pstart, *pend, *bound;
  struct value *bound_val;

  if (dval == NULL || str == NULL || str[k] == '\0')
    return 0;

  pstart = str + k;
  pend = strstr (pstart, "__");
  if (pend == NULL)
    {
      bound = pstart;
      k += strlen (bound);
    }
  else
    {
      int len = pend - pstart;

      /* Strip __ and beyond.  */
      GROW_VECT (bound_buffer, bound_buffer_len, len + 1);
      strncpy (bound_buffer, pstart, len);
      bound_buffer[len] = '\0';

      bound = bound_buffer;
      k = pend - str;
    }

  bound_val = ada_search_struct_field (bound, dval, 0, value_type (dval));
  if (bound_val == NULL)
    return 0;

  *px = value_as_long (bound_val);
  if (pnew_k != NULL)
    *pnew_k = k;
  return 1;
}

/* Value of variable named NAME in the current environment.  If
   no such variable found, then if ERR_MSG is null, returns 0, and
   otherwise causes an error with message ERR_MSG.  */

static struct value *
get_var_value (char *name, char *err_msg)
{
  struct block_symbol *syms;
  int nsyms;

  nsyms = ada_lookup_symbol_list (name, get_selected_block (0), VAR_DOMAIN,
                                  &syms);

  if (nsyms != 1)
    {
      if (err_msg == NULL)
        return 0;
      else
        error (("%s"), err_msg);
    }

  return value_of_variable (syms[0].symbol, syms[0].block);
}

/* Value of integer variable named NAME in the current environment.  If
   no such variable found, returns 0, and sets *FLAG to 0.  If
   successful, sets *FLAG to 1.  */

LONGEST
get_int_var_value (char *name, int *flag)
{
  struct value *var_val = get_var_value (name, 0);

  if (var_val == 0)
    {
      if (flag != NULL)
        *flag = 0;
      return 0;
    }
  else
    {
      if (flag != NULL)
        *flag = 1;
      return value_as_long (var_val);
    }
}


/* Return a range type whose base type is that of the range type named
   NAME in the current environment, and whose bounds are calculated
   from NAME according to the GNAT range encoding conventions.
   Extract discriminant values, if needed, from DVAL.  ORIG_TYPE is the
   corresponding range type from debug information; fall back to using it
   if symbol lookup fails.  If a new type must be created, allocate it
   like ORIG_TYPE was.  The bounds information, in general, is encoded
   in NAME, the base type given in the named range type.  */

static struct type *
to_fixed_range_type (struct type *raw_type, struct value *dval)
{
  const char *name;
  struct type *base_type;
  const char *subtype_info;

  gdb_assert (raw_type != NULL);
  gdb_assert (TYPE_NAME (raw_type) != NULL);

  if (TYPE_CODE (raw_type) == TYPE_CODE_RANGE)
    base_type = TYPE_TARGET_TYPE (raw_type);
  else
    base_type = raw_type;

  name = TYPE_NAME (raw_type);
  subtype_info = strstr (name, "___XD");
  if (subtype_info == NULL)
    {
      LONGEST L = ada_discrete_type_low_bound (raw_type);
      LONGEST U = ada_discrete_type_high_bound (raw_type);

      if (L < INT_MIN || U > INT_MAX)
	return raw_type;
      else
	return create_static_range_type (alloc_type_copy (raw_type), raw_type,
					 L, U);
    }
  else
    {
      static char *name_buf = NULL;
      static size_t name_len = 0;
      int prefix_len = subtype_info - name;
      LONGEST L, U;
      struct type *type;
      const char *bounds_str;
      int n;

      GROW_VECT (name_buf, name_len, prefix_len + 5);
      strncpy (name_buf, name, prefix_len);
      name_buf[prefix_len] = '\0';

      subtype_info += 5;
      bounds_str = strchr (subtype_info, '_');
      n = 1;

      if (*subtype_info == 'L')
        {
          if (!ada_scan_number (bounds_str, n, &L, &n)
              && !scan_discrim_bound (bounds_str, n, dval, &L, &n))
            return raw_type;
          if (bounds_str[n] == '_')
            n += 2;
          else if (bounds_str[n] == '.')     /* FIXME? SGI Workshop kludge.  */
            n += 1;
          subtype_info += 1;
        }
      else
        {
          int ok;

          strcpy (name_buf + prefix_len, "___L");
          L = get_int_var_value (name_buf, &ok);
          if (!ok)
            {
              lim_warning (_("Unknown lower bound, using 1."));
              L = 1;
            }
        }

      if (*subtype_info == 'U')
        {
          if (!ada_scan_number (bounds_str, n, &U, &n)
              && !scan_discrim_bound (bounds_str, n, dval, &U, &n))
            return raw_type;
        }
      else
        {
          int ok;

          strcpy (name_buf + prefix_len, "___U");
          U = get_int_var_value (name_buf, &ok);
          if (!ok)
            {
              lim_warning (_("Unknown upper bound, using %ld."), (long) L);
              U = L;
            }
        }

      type = create_static_range_type (alloc_type_copy (raw_type),
				       base_type, L, U);
      TYPE_NAME (type) = name;
      return type;
    }
}

/* True iff NAME is the name of a range type.  */

int
ada_is_range_type_name (const char *name)
{
  return (name != NULL && strstr (name, "___XD"));
}


                                /* Modular types */

/* True iff TYPE is an Ada modular type.  */

int
ada_is_modular_type (struct type *type)
{
  struct type *subranged_type = get_base_type (type);

  return (subranged_type != NULL && TYPE_CODE (type) == TYPE_CODE_RANGE
          && TYPE_CODE (subranged_type) == TYPE_CODE_INT
          && TYPE_UNSIGNED (subranged_type));
}

/* Assuming ada_is_modular_type (TYPE), the modulus of TYPE.  */

ULONGEST
ada_modulus (struct type *type)
{
  return (ULONGEST) TYPE_HIGH_BOUND (type) + 1;
}


/* Ada exception catchpoint support:
   ---------------------------------

   We support 3 kinds of exception catchpoints:
     . catchpoints on Ada exceptions
     . catchpoints on unhandled Ada exceptions
     . catchpoints on failed assertions

   Exceptions raised during failed assertions, or unhandled exceptions
   could perfectly be caught with the general catchpoint on Ada exceptions.
   However, we can easily differentiate these two special cases, and having
   the option to distinguish these two cases from the rest can be useful
   to zero-in on certain situations.

   Exception catchpoints are a specialized form of breakpoint,
   since they rely on inserting breakpoints inside known routines
   of the GNAT runtime.  The implementation therefore uses a standard
   breakpoint structure of the BP_BREAKPOINT type, but with its own set
   of breakpoint_ops.

   Support in the runtime for exception catchpoints have been changed
   a few times already, and these changes affect the implementation
   of these catchpoints.  In order to be able to support several
   variants of the runtime, we use a sniffer that will determine
   the runtime variant used by the program being debugged.  */

/* Ada's standard exceptions.

   The Ada 83 standard also defined Numeric_Error.  But there so many
   situations where it was unclear from the Ada 83 Reference Manual
   (RM) whether Constraint_Error or Numeric_Error should be raised,
   that the ARG (Ada Rapporteur Group) eventually issued a Binding
   Interpretation saying that anytime the RM says that Numeric_Error
   should be raised, the implementation may raise Constraint_Error.
   Ada 95 went one step further and pretty much removed Numeric_Error
   from the list of standard exceptions (it made it a renaming of
   Constraint_Error, to help preserve compatibility when compiling
   an Ada83 compiler). As such, we do not include Numeric_Error from
   this list of standard exceptions.  */

static char *standard_exc[] = {
  "constraint_error",
  "program_error",
  "storage_error",
  "tasking_error"
};

typedef CORE_ADDR (ada_unhandled_exception_name_addr_ftype) (void);

/* A structure that describes how to support exception catchpoints
   for a given executable.  */

struct exception_support_info
{
   /* The name of the symbol to break on in order to insert
      a catchpoint on exceptions.  */
   const char *catch_exception_sym;

   /* The name of the symbol to break on in order to insert
      a catchpoint on unhandled exceptions.  */
   const char *catch_exception_unhandled_sym;

   /* The name of the symbol to break on in order to insert
      a catchpoint on failed assertions.  */
   const char *catch_assert_sym;

   /* Assuming that the inferior just triggered an unhandled exception
      catchpoint, this function is responsible for returning the address
      in inferior memory where the name of that exception is stored.
      Return zero if the address could not be computed.  */
   ada_unhandled_exception_name_addr_ftype *unhandled_exception_name_addr;
};

static CORE_ADDR ada_unhandled_exception_name_addr (void);
static CORE_ADDR ada_unhandled_exception_name_addr_from_raise (void);

/* The following exception support info structure describes how to
   implement exception catchpoints with the latest version of the
   Ada runtime (as of 2007-03-06).  */

static const struct exception_support_info default_exception_support_info =
{
  "__gnat_debug_raise_exception", /* catch_exception_sym */
  "__gnat_unhandled_exception", /* catch_exception_unhandled_sym */
  "__gnat_debug_raise_assert_failure", /* catch_assert_sym */
  ada_unhandled_exception_name_addr
};

/* The following exception support info structure describes how to
   implement exception catchpoints with a slightly older version
   of the Ada runtime.  */

static const struct exception_support_info exception_support_info_fallback =
{
  "__gnat_raise_nodefer_with_msg", /* catch_exception_sym */
  "__gnat_unhandled_exception", /* catch_exception_unhandled_sym */
  "system__assertions__raise_assert_failure",  /* catch_assert_sym */
  ada_unhandled_exception_name_addr_from_raise
};

/* Return nonzero if we can detect the exception support routines
   described in EINFO.

   This function errors out if an abnormal situation is detected
   (for instance, if we find the exception support routines, but
   that support is found to be incomplete).  */

static int
ada_has_this_exception_support (const struct exception_support_info *einfo)
{
  struct symbol *sym;

  /* The symbol we're looking up is provided by a unit in the GNAT runtime
     that should be compiled with debugging information.  As a result, we
     expect to find that symbol in the symtabs.  */

  sym = standard_lookup (einfo->catch_exception_sym, NULL, VAR_DOMAIN);
  if (sym == NULL)
    {
      /* Perhaps we did not find our symbol because the Ada runtime was
	 compiled without debugging info, or simply stripped of it.
	 It happens on some GNU/Linux distributions for instance, where
	 users have to install a separate debug package in order to get
	 the runtime's debugging info.  In that situation, let the user
	 know why we cannot insert an Ada exception catchpoint.

	 Note: Just for the purpose of inserting our Ada exception
	 catchpoint, we could rely purely on the associated minimal symbol.
	 But we would be operating in degraded mode anyway, since we are
	 still lacking the debugging info needed later on to extract
	 the name of the exception being raised (this name is printed in
	 the catchpoint message, and is also used when trying to catch
	 a specific exception).  We do not handle this case for now.  */
      struct bound_minimal_symbol msym
	= lookup_minimal_symbol (einfo->catch_exception_sym, NULL, NULL);

      if (msym.minsym && MSYMBOL_TYPE (msym.minsym) != mst_solib_trampoline)
	error (_("Your Ada runtime appears to be missing some debugging "
		 "information.\nCannot insert Ada exception catchpoint "
		 "in this configuration."));

      return 0;
    }

  /* Make sure that the symbol we found corresponds to a function.  */

  if (SYMBOL_CLASS (sym) != LOC_BLOCK)
    error (_("Symbol \"%s\" is not a function (class = %d)"),
           SYMBOL_LINKAGE_NAME (sym), SYMBOL_CLASS (sym));

  return 1;
}

/* Inspect the Ada runtime and determine which exception info structure
   should be used to provide support for exception catchpoints.

   This function will always set the per-inferior exception_info,
   or raise an error.  */

static void
ada_exception_support_info_sniffer (void)
{
  struct ada_inferior_data *data = get_ada_inferior_data (current_inferior ());

  /* If the exception info is already known, then no need to recompute it.  */
  if (data->exception_info != NULL)
    return;

  /* Check the latest (default) exception support info.  */
  if (ada_has_this_exception_support (&default_exception_support_info))
    {
      data->exception_info = &default_exception_support_info;
      return;
    }

  /* Try our fallback exception suport info.  */
  if (ada_has_this_exception_support (&exception_support_info_fallback))
    {
      data->exception_info = &exception_support_info_fallback;
      return;
    }

  /* Sometimes, it is normal for us to not be able to find the routine
     we are looking for.  This happens when the program is linked with
     the shared version of the GNAT runtime, and the program has not been
     started yet.  Inform the user of these two possible causes if
     applicable.  */

  if (ada_update_initial_language (language_unknown) != language_ada)
    error (_("Unable to insert catchpoint.  Is this an Ada main program?"));

  /* If the symbol does not exist, then check that the program is
     already started, to make sure that shared libraries have been
     loaded.  If it is not started, this may mean that the symbol is
     in a shared library.  */

  if (ptid_get_pid (inferior_ptid) == 0)
    error (_("Unable to insert catchpoint. Try to start the program first."));

  /* At this point, we know that we are debugging an Ada program and
     that the inferior has been started, but we still are not able to
     find the run-time symbols.  That can mean that we are in
     configurable run time mode, or that a-except as been optimized
     out by the linker...  In any case, at this point it is not worth
     supporting this feature.  */

  error (_("Cannot insert Ada exception catchpoints in this configuration."));
}

/* True iff FRAME is very likely to be that of a function that is
   part of the runtime system.  This is all very heuristic, but is
   intended to be used as advice as to what frames are uninteresting
   to most users.  */

static int
is_known_support_routine (struct frame_info *frame)
{
  struct symtab_and_line sal;
  char *func_name;
  enum language func_lang;
  int i;
  const char *fullname;

  /* If this code does not have any debugging information (no symtab),
     This cannot be any user code.  */

  find_frame_sal (frame, &sal);
  if (sal.symtab == NULL)
    return 1;

  /* If there is a symtab, but the associated source file cannot be
     located, then assume this is not user code:  Selecting a frame
     for which we cannot display the code would not be very helpful
     for the user.  This should also take care of case such as VxWorks
     where the kernel has some debugging info provided for a few units.  */

  fullname = symtab_to_fullname (sal.symtab);
  if (access (fullname, R_OK) != 0)
    return 1;

  /* Check the unit filename againt the Ada runtime file naming.
     We also check the name of the objfile against the name of some
     known system libraries that sometimes come with debugging info
     too.  */

  for (i = 0; known_runtime_file_name_patterns[i] != NULL; i += 1)
    {
      re_comp (known_runtime_file_name_patterns[i]);
      if (re_exec (lbasename (sal.symtab->filename)))
        return 1;
      if (SYMTAB_OBJFILE (sal.symtab) != NULL
          && re_exec (objfile_name (SYMTAB_OBJFILE (sal.symtab))))
        return 1;
    }

  /* Check whether the function is a GNAT-generated entity.  */

  find_frame_funname (frame, &func_name, &func_lang, NULL);
  if (func_name == NULL)
    return 1;

  for (i = 0; known_auxiliary_function_name_patterns[i] != NULL; i += 1)
    {
      re_comp (known_auxiliary_function_name_patterns[i]);
      if (re_exec (func_name))
	{
	  xfree (func_name);
	  return 1;
	}
    }

  xfree (func_name);
  return 0;
}

/* Find the first frame that contains debugging information and that is not
   part of the Ada run-time, starting from FI and moving upward.  */

void
ada_find_printable_frame (struct frame_info *fi)
{
  for (; fi != NULL; fi = get_prev_frame (fi))
    {
      if (!is_known_support_routine (fi))
        {
          select_frame (fi);
          break;
        }
    }

}

/* Assuming that the inferior just triggered an unhandled exception
   catchpoint, return the address in inferior memory where the name
   of the exception is stored.
   
   Return zero if the address could not be computed.  */

static CORE_ADDR
ada_unhandled_exception_name_addr (void)
{
  return parse_and_eval_address ("e.full_name");
}

/* Same as ada_unhandled_exception_name_addr, except that this function
   should be used when the inferior uses an older version of the runtime,
   where the exception name needs to be extracted from a specific frame
   several frames up in the callstack.  */

static CORE_ADDR
ada_unhandled_exception_name_addr_from_raise (void)
{
  int frame_level;
  struct frame_info *fi;
  struct ada_inferior_data *data = get_ada_inferior_data (current_inferior ());
  struct cleanup *old_chain;

  /* To determine the name of this exception, we need to select
     the frame corresponding to RAISE_SYM_NAME.  This frame is
     at least 3 levels up, so we simply skip the first 3 frames
     without checking the name of their associated function.  */
  fi = get_current_frame ();
  for (frame_level = 0; frame_level < 3; frame_level += 1)
    if (fi != NULL)
      fi = get_prev_frame (fi); 

  old_chain = make_cleanup (null_cleanup, NULL);
  while (fi != NULL)
    {
      char *func_name;
      enum language func_lang;

      find_frame_funname (fi, &func_name, &func_lang, NULL);
      if (func_name != NULL)
	{
	  make_cleanup (xfree, func_name);

          if (strcmp (func_name,
		      data->exception_info->catch_exception_sym) == 0)
	    break; /* We found the frame we were looking for...  */
	  fi = get_prev_frame (fi);
	}
    }
  do_cleanups (old_chain);

  if (fi == NULL)
    return 0;

  select_frame (fi);
  return parse_and_eval_address ("id.full_name");
}

/* Assuming the inferior just triggered an Ada exception catchpoint
   (of any type), return the address in inferior memory where the name
   of the exception is stored, if applicable.

   Assumes the selected frame is the current frame.

   Return zero if the address could not be computed, or if not relevant.  */

static CORE_ADDR
ada_exception_name_addr_1 (enum ada_exception_catchpoint_kind ex,
                           struct breakpoint *b)
{
  struct ada_inferior_data *data = get_ada_inferior_data (current_inferior ());

  switch (ex)
    {
      case ada_catch_exception:
        return (parse_and_eval_address ("e.full_name"));
        break;

      case ada_catch_exception_unhandled:
        return data->exception_info->unhandled_exception_name_addr ();
        break;
      
      case ada_catch_assert:
        return 0;  /* Exception name is not relevant in this case.  */
        break;

      default:
        internal_error (__FILE__, __LINE__, _("unexpected catchpoint type"));
        break;
    }

  return 0; /* Should never be reached.  */
}

/* Same as ada_exception_name_addr_1, except that it intercepts and contains
   any error that ada_exception_name_addr_1 might cause to be thrown.
   When an error is intercepted, a warning with the error message is printed,
   and zero is returned.  */

static CORE_ADDR
ada_exception_name_addr (enum ada_exception_catchpoint_kind ex,
                         struct breakpoint *b)
{
  CORE_ADDR result = 0;

  TRY
    {
      result = ada_exception_name_addr_1 (ex, b);
    }

  CATCH (e, RETURN_MASK_ERROR)
    {
      warning (_("failed to get exception name: %s"), e.message);
      return 0;
    }
  END_CATCH

  return result;
}

static char *ada_exception_catchpoint_cond_string (const char *excep_string);

/* Ada catchpoints.

   In the case of catchpoints on Ada exceptions, the catchpoint will
   stop the target on every exception the program throws.  When a user
   specifies the name of a specific exception, we translate this
   request into a condition expression (in text form), and then parse
   it into an expression stored in each of the catchpoint's locations.
   We then use this condition to check whether the exception that was
   raised is the one the user is interested in.  If not, then the
   target is resumed again.  We store the name of the requested
   exception, in order to be able to re-set the condition expression
   when symbols change.  */

/* An instance of this type is used to represent an Ada catchpoint
   breakpoint location.  It includes a "struct bp_location" as a kind
   of base class; users downcast to "struct bp_location *" when
   needed.  */

struct ada_catchpoint_location
{
  /* The base class.  */
  struct bp_location base;

  /* The condition that checks whether the exception that was raised
     is the specific exception the user specified on catchpoint
     creation.  */
  struct expression *excep_cond_expr;
};

/* Implement the DTOR method in the bp_location_ops structure for all
   Ada exception catchpoint kinds.  */

static void
ada_catchpoint_location_dtor (struct bp_location *bl)
{
  struct ada_catchpoint_location *al = (struct ada_catchpoint_location *) bl;

  xfree (al->excep_cond_expr);
}

/* The vtable to be used in Ada catchpoint locations.  */

static const struct bp_location_ops ada_catchpoint_location_ops =
{
  ada_catchpoint_location_dtor
};

/* An instance of this type is used to represent an Ada catchpoint.
   It includes a "struct breakpoint" as a kind of base class; users
   downcast to "struct breakpoint *" when needed.  */

struct ada_catchpoint
{
  /* The base class.  */
  struct breakpoint base;

  /* The name of the specific exception the user specified.  */
  char *excep_string;
};

/* Parse the exception condition string in the context of each of the
   catchpoint's locations, and store them for later evaluation.  */

static void
create_excep_cond_exprs (struct ada_catchpoint *c)
{
  struct cleanup *old_chain;
  struct bp_location *bl;
  char *cond_string;

  /* Nothing to do if there's no specific exception to catch.  */
  if (c->excep_string == NULL)
    return;

  /* Same if there are no locations... */
  if (c->base.loc == NULL)
    return;

  /* Compute the condition expression in text form, from the specific
     expection we want to catch.  */
  cond_string = ada_exception_catchpoint_cond_string (c->excep_string);
  old_chain = make_cleanup (xfree, cond_string);

  /* Iterate over all the catchpoint's locations, and parse an
     expression for each.  */
  for (bl = c->base.loc; bl != NULL; bl = bl->next)
    {
      struct ada_catchpoint_location *ada_loc
	= (struct ada_catchpoint_location *) bl;
      struct expression *exp = NULL;

      if (!bl->shlib_disabled)
	{
	  const char *s;

	  s = cond_string;
	  TRY
	    {
	      exp = parse_exp_1 (&s, bl->address,
				 block_for_pc (bl->address), 0);
	    }
	  CATCH (e, RETURN_MASK_ERROR)
	    {
	      warning (_("failed to reevaluate internal exception condition "
			 "for catchpoint %d: %s"),
		       c->base.number, e.message);
	      /* There is a bug in GCC on sparc-solaris when building with
		 optimization which causes EXP to change unexpectedly
		 (http://gcc.gnu.org/bugzilla/show_bug.cgi?id=56982).
		 The problem should be fixed starting with GCC 4.9.
		 In the meantime, work around it by forcing EXP back
		 to NULL.  */
	      exp = NULL;
	    }
	  END_CATCH
	}

      ada_loc->excep_cond_expr = exp;
    }

  do_cleanups (old_chain);
}

/* Implement the DTOR method in the breakpoint_ops structure for all
   exception catchpoint kinds.  */

static void
dtor_exception (enum ada_exception_catchpoint_kind ex, struct breakpoint *b)
{
  struct ada_catchpoint *c = (struct ada_catchpoint *) b;

  xfree (c->excep_string);

  bkpt_breakpoint_ops.dtor (b);
}

/* Implement the ALLOCATE_LOCATION method in the breakpoint_ops
   structure for all exception catchpoint kinds.  */

static struct bp_location *
allocate_location_exception (enum ada_exception_catchpoint_kind ex,
			     struct breakpoint *self)
{
  struct ada_catchpoint_location *loc;

  loc = XNEW (struct ada_catchpoint_location);
  init_bp_location (&loc->base, &ada_catchpoint_location_ops, self);
  loc->excep_cond_expr = NULL;
  return &loc->base;
}

/* Implement the RE_SET method in the breakpoint_ops structure for all
   exception catchpoint kinds.  */

static void
re_set_exception (enum ada_exception_catchpoint_kind ex, struct breakpoint *b)
{
  struct ada_catchpoint *c = (struct ada_catchpoint *) b;

  /* Call the base class's method.  This updates the catchpoint's
     locations.  */
  bkpt_breakpoint_ops.re_set (b);

  /* Reparse the exception conditional expressions.  One for each
     location.  */
  create_excep_cond_exprs (c);
}

/* Returns true if we should stop for this breakpoint hit.  If the
   user specified a specific exception, we only want to cause a stop
   if the program thrown that exception.  */

static int
should_stop_exception (const struct bp_location *bl)
{
  struct ada_catchpoint *c = (struct ada_catchpoint *) bl->owner;
  const struct ada_catchpoint_location *ada_loc
    = (const struct ada_catchpoint_location *) bl;
  int stop;

  /* With no specific exception, should always stop.  */
  if (c->excep_string == NULL)
    return 1;

  if (ada_loc->excep_cond_expr == NULL)
    {
      /* We will have a NULL expression if back when we were creating
	 the expressions, this location's had failed to parse.  */
      return 1;
    }

  stop = 1;
  TRY
    {
      struct value *mark;

      mark = value_mark ();
      stop = value_true (evaluate_expression (ada_loc->excep_cond_expr));
      value_free_to_mark (mark);
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      exception_fprintf (gdb_stderr, ex,
			 _("Error in testing exception condition:\n"));
    }
  END_CATCH

  return stop;
}

/* Implement the CHECK_STATUS method in the breakpoint_ops structure
   for all exception catchpoint kinds.  */

static void
check_status_exception (enum ada_exception_catchpoint_kind ex, bpstat bs)
{
  bs->stop = should_stop_exception (bs->bp_location_at);
}

/* Implement the PRINT_IT method in the breakpoint_ops structure
   for all exception catchpoint kinds.  */

static enum print_stop_action
print_it_exception (enum ada_exception_catchpoint_kind ex, bpstat bs)
{
  struct ui_out *uiout = current_uiout;
  struct breakpoint *b = bs->breakpoint_at;

  annotate_catchpoint (b->number);

  if (ui_out_is_mi_like_p (uiout))
    {
      ui_out_field_string (uiout, "reason",
			   async_reason_lookup (EXEC_ASYNC_BREAKPOINT_HIT));
      ui_out_field_string (uiout, "disp", bpdisp_text (b->disposition));
    }

  ui_out_text (uiout,
               b->disposition == disp_del ? "\nTemporary catchpoint "
	                                  : "\nCatchpoint ");
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, ", ");

  /* ada_exception_name_addr relies on the selected frame being the
     current frame.  Need to do this here because this function may be
     called more than once when printing a stop, and below, we'll
     select the first frame past the Ada run-time (see
     ada_find_printable_frame).  */
  select_frame (get_current_frame ());

  switch (ex)
    {
      case ada_catch_exception:
      case ada_catch_exception_unhandled:
	{
	  const CORE_ADDR addr = ada_exception_name_addr (ex, b);
	  char exception_name[256];

	  if (addr != 0)
	    {
	      read_memory (addr, (gdb_byte *) exception_name,
			   sizeof (exception_name) - 1);
	      exception_name [sizeof (exception_name) - 1] = '\0';
	    }
	  else
	    {
	      /* For some reason, we were unable to read the exception
		 name.  This could happen if the Runtime was compiled
		 without debugging info, for instance.  In that case,
		 just replace the exception name by the generic string
		 "exception" - it will read as "an exception" in the
		 notification we are about to print.  */
	      memcpy (exception_name, "exception", sizeof ("exception"));
	    }
	  /* In the case of unhandled exception breakpoints, we print
	     the exception name as "unhandled EXCEPTION_NAME", to make
	     it clearer to the user which kind of catchpoint just got
	     hit.  We used ui_out_text to make sure that this extra
	     info does not pollute the exception name in the MI case.  */
	  if (ex == ada_catch_exception_unhandled)
	    ui_out_text (uiout, "unhandled ");
	  ui_out_field_string (uiout, "exception-name", exception_name);
	}
	break;
      case ada_catch_assert:
	/* In this case, the name of the exception is not really
	   important.  Just print "failed assertion" to make it clearer
	   that his program just hit an assertion-failure catchpoint.
	   We used ui_out_text because this info does not belong in
	   the MI output.  */
	ui_out_text (uiout, "failed assertion");
	break;
    }
  ui_out_text (uiout, " at ");
  ada_find_printable_frame (get_current_frame ());

  return PRINT_SRC_AND_LOC;
}

/* Implement the PRINT_ONE method in the breakpoint_ops structure
   for all exception catchpoint kinds.  */

static void
print_one_exception (enum ada_exception_catchpoint_kind ex,
                     struct breakpoint *b, struct bp_location **last_loc)
{ 
  struct ui_out *uiout = current_uiout;
  struct ada_catchpoint *c = (struct ada_catchpoint *) b;
  struct value_print_options opts;

  get_user_print_options (&opts);
  if (opts.addressprint)
    {
      annotate_field (4);
      ui_out_field_core_addr (uiout, "addr", b->loc->gdbarch, b->loc->address);
    }

  annotate_field (5);
  *last_loc = b->loc;
  switch (ex)
    {
      case ada_catch_exception:
        if (c->excep_string != NULL)
          {
            char *msg = xstrprintf (_("`%s' Ada exception"), c->excep_string);

            ui_out_field_string (uiout, "what", msg);
            xfree (msg);
          }
        else
          ui_out_field_string (uiout, "what", "all Ada exceptions");
        
        break;

      case ada_catch_exception_unhandled:
        ui_out_field_string (uiout, "what", "unhandled Ada exceptions");
        break;
      
      case ada_catch_assert:
        ui_out_field_string (uiout, "what", "failed Ada assertions");
        break;

      default:
        internal_error (__FILE__, __LINE__, _("unexpected catchpoint type"));
        break;
    }
}

/* Implement the PRINT_MENTION method in the breakpoint_ops structure
   for all exception catchpoint kinds.  */

static void
print_mention_exception (enum ada_exception_catchpoint_kind ex,
                         struct breakpoint *b)
{
  struct ada_catchpoint *c = (struct ada_catchpoint *) b;
  struct ui_out *uiout = current_uiout;

  ui_out_text (uiout, b->disposition == disp_del ? _("Temporary catchpoint ")
                                                 : _("Catchpoint "));
  ui_out_field_int (uiout, "bkptno", b->number);
  ui_out_text (uiout, ": ");

  switch (ex)
    {
      case ada_catch_exception:
        if (c->excep_string != NULL)
	  {
	    char *info = xstrprintf (_("`%s' Ada exception"), c->excep_string);
	    struct cleanup *old_chain = make_cleanup (xfree, info);

	    ui_out_text (uiout, info);
	    do_cleanups (old_chain);
	  }
        else
          ui_out_text (uiout, _("all Ada exceptions"));
        break;

      case ada_catch_exception_unhandled:
        ui_out_text (uiout, _("unhandled Ada exceptions"));
        break;
      
      case ada_catch_assert:
        ui_out_text (uiout, _("failed Ada assertions"));
        break;

      default:
        internal_error (__FILE__, __LINE__, _("unexpected catchpoint type"));
        break;
    }
}

/* Implement the PRINT_RECREATE method in the breakpoint_ops structure
   for all exception catchpoint kinds.  */

static void
print_recreate_exception (enum ada_exception_catchpoint_kind ex,
			  struct breakpoint *b, struct ui_file *fp)
{
  struct ada_catchpoint *c = (struct ada_catchpoint *) b;

  switch (ex)
    {
      case ada_catch_exception:
	fprintf_filtered (fp, "catch exception");
	if (c->excep_string != NULL)
	  fprintf_filtered (fp, " %s", c->excep_string);
	break;

      case ada_catch_exception_unhandled:
	fprintf_filtered (fp, "catch exception unhandled");
	break;

      case ada_catch_assert:
	fprintf_filtered (fp, "catch assert");
	break;

      default:
	internal_error (__FILE__, __LINE__, _("unexpected catchpoint type"));
    }
  print_recreate_thread (b, fp);
}

/* Virtual table for "catch exception" breakpoints.  */

static void
dtor_catch_exception (struct breakpoint *b)
{
  dtor_exception (ada_catch_exception, b);
}

static struct bp_location *
allocate_location_catch_exception (struct breakpoint *self)
{
  return allocate_location_exception (ada_catch_exception, self);
}

static void
re_set_catch_exception (struct breakpoint *b)
{
  re_set_exception (ada_catch_exception, b);
}

static void
check_status_catch_exception (bpstat bs)
{
  check_status_exception (ada_catch_exception, bs);
}

static enum print_stop_action
print_it_catch_exception (bpstat bs)
{
  return print_it_exception (ada_catch_exception, bs);
}

static void
print_one_catch_exception (struct breakpoint *b, struct bp_location **last_loc)
{
  print_one_exception (ada_catch_exception, b, last_loc);
}

static void
print_mention_catch_exception (struct breakpoint *b)
{
  print_mention_exception (ada_catch_exception, b);
}

static void
print_recreate_catch_exception (struct breakpoint *b, struct ui_file *fp)
{
  print_recreate_exception (ada_catch_exception, b, fp);
}

static struct breakpoint_ops catch_exception_breakpoint_ops;

/* Virtual table for "catch exception unhandled" breakpoints.  */

static void
dtor_catch_exception_unhandled (struct breakpoint *b)
{
  dtor_exception (ada_catch_exception_unhandled, b);
}

static struct bp_location *
allocate_location_catch_exception_unhandled (struct breakpoint *self)
{
  return allocate_location_exception (ada_catch_exception_unhandled, self);
}

static void
re_set_catch_exception_unhandled (struct breakpoint *b)
{
  re_set_exception (ada_catch_exception_unhandled, b);
}

static void
check_status_catch_exception_unhandled (bpstat bs)
{
  check_status_exception (ada_catch_exception_unhandled, bs);
}

static enum print_stop_action
print_it_catch_exception_unhandled (bpstat bs)
{
  return print_it_exception (ada_catch_exception_unhandled, bs);
}

static void
print_one_catch_exception_unhandled (struct breakpoint *b,
				     struct bp_location **last_loc)
{
  print_one_exception (ada_catch_exception_unhandled, b, last_loc);
}

static void
print_mention_catch_exception_unhandled (struct breakpoint *b)
{
  print_mention_exception (ada_catch_exception_unhandled, b);
}

static void
print_recreate_catch_exception_unhandled (struct breakpoint *b,
					  struct ui_file *fp)
{
  print_recreate_exception (ada_catch_exception_unhandled, b, fp);
}

static struct breakpoint_ops catch_exception_unhandled_breakpoint_ops;

/* Virtual table for "catch assert" breakpoints.  */

static void
dtor_catch_assert (struct breakpoint *b)
{
  dtor_exception (ada_catch_assert, b);
}

static struct bp_location *
allocate_location_catch_assert (struct breakpoint *self)
{
  return allocate_location_exception (ada_catch_assert, self);
}

static void
re_set_catch_assert (struct breakpoint *b)
{
  re_set_exception (ada_catch_assert, b);
}

static void
check_status_catch_assert (bpstat bs)
{
  check_status_exception (ada_catch_assert, bs);
}

static enum print_stop_action
print_it_catch_assert (bpstat bs)
{
  return print_it_exception (ada_catch_assert, bs);
}

static void
print_one_catch_assert (struct breakpoint *b, struct bp_location **last_loc)
{
  print_one_exception (ada_catch_assert, b, last_loc);
}

static void
print_mention_catch_assert (struct breakpoint *b)
{
  print_mention_exception (ada_catch_assert, b);
}

static void
print_recreate_catch_assert (struct breakpoint *b, struct ui_file *fp)
{
  print_recreate_exception (ada_catch_assert, b, fp);
}

static struct breakpoint_ops catch_assert_breakpoint_ops;

/* Return a newly allocated copy of the first space-separated token
   in ARGSP, and then adjust ARGSP to point immediately after that
   token.

   Return NULL if ARGPS does not contain any more tokens.  */

static char *
ada_get_next_arg (char **argsp)
{
  char *args = *argsp;
  char *end;
  char *result;

  args = skip_spaces (args);
  if (args[0] == '\0')
    return NULL; /* No more arguments.  */
  
  /* Find the end of the current argument.  */

  end = skip_to_space (args);

  /* Adjust ARGSP to point to the start of the next argument.  */

  *argsp = end;

  /* Make a copy of the current argument and return it.  */

  result = (char *) xmalloc (end - args + 1);
  strncpy (result, args, end - args);
  result[end - args] = '\0';
  
  return result;
}

/* Split the arguments specified in a "catch exception" command.  
   Set EX to the appropriate catchpoint type.
   Set EXCEP_STRING to the name of the specific exception if
   specified by the user.
   If a condition is found at the end of the arguments, the condition
   expression is stored in COND_STRING (memory must be deallocated
   after use).  Otherwise COND_STRING is set to NULL.  */

static void
catch_ada_exception_command_split (char *args,
                                   enum ada_exception_catchpoint_kind *ex,
				   char **excep_string,
				   char **cond_string)
{
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);
  char *exception_name;
  char *cond = NULL;

  exception_name = ada_get_next_arg (&args);
  if (exception_name != NULL && strcmp (exception_name, "if") == 0)
    {
      /* This is not an exception name; this is the start of a condition
	 expression for a catchpoint on all exceptions.  So, "un-get"
	 this token, and set exception_name to NULL.  */
      xfree (exception_name);
      exception_name = NULL;
      args -= 2;
    }
  make_cleanup (xfree, exception_name);

  /* Check to see if we have a condition.  */

  args = skip_spaces (args);
  if (startswith (args, "if")
      && (isspace (args[2]) || args[2] == '\0'))
    {
      args += 2;
      args = skip_spaces (args);

      if (args[0] == '\0')
        error (_("Condition missing after `if' keyword"));
      cond = xstrdup (args);
      make_cleanup (xfree, cond);

      args += strlen (args);
    }

  /* Check that we do not have any more arguments.  Anything else
     is unexpected.  */

  if (args[0] != '\0')
    error (_("Junk at end of expression"));

  discard_cleanups (old_chain);

  if (exception_name == NULL)
    {
      /* Catch all exceptions.  */
      *ex = ada_catch_exception;
      *excep_string = NULL;
    }
  else if (strcmp (exception_name, "unhandled") == 0)
    {
      /* Catch unhandled exceptions.  */
      *ex = ada_catch_exception_unhandled;
      *excep_string = NULL;
    }
  else
    {
      /* Catch a specific exception.  */
      *ex = ada_catch_exception;
      *excep_string = exception_name;
    }
  *cond_string = cond;
}

/* Return the name of the symbol on which we should break in order to
   implement a catchpoint of the EX kind.  */

static const char *
ada_exception_sym_name (enum ada_exception_catchpoint_kind ex)
{
  struct ada_inferior_data *data = get_ada_inferior_data (current_inferior ());

  gdb_assert (data->exception_info != NULL);

  switch (ex)
    {
      case ada_catch_exception:
        return (data->exception_info->catch_exception_sym);
        break;
      case ada_catch_exception_unhandled:
        return (data->exception_info->catch_exception_unhandled_sym);
        break;
      case ada_catch_assert:
        return (data->exception_info->catch_assert_sym);
        break;
      default:
        internal_error (__FILE__, __LINE__,
                        _("unexpected catchpoint kind (%d)"), ex);
    }
}

/* Return the breakpoint ops "virtual table" used for catchpoints
   of the EX kind.  */

static const struct breakpoint_ops *
ada_exception_breakpoint_ops (enum ada_exception_catchpoint_kind ex)
{
  switch (ex)
    {
      case ada_catch_exception:
        return (&catch_exception_breakpoint_ops);
        break;
      case ada_catch_exception_unhandled:
        return (&catch_exception_unhandled_breakpoint_ops);
        break;
      case ada_catch_assert:
        return (&catch_assert_breakpoint_ops);
        break;
      default:
        internal_error (__FILE__, __LINE__,
                        _("unexpected catchpoint kind (%d)"), ex);
    }
}

/* Return the condition that will be used to match the current exception
   being raised with the exception that the user wants to catch.  This
   assumes that this condition is used when the inferior just triggered
   an exception catchpoint.
   
   The string returned is a newly allocated string that needs to be
   deallocated later.  */

static char *
ada_exception_catchpoint_cond_string (const char *excep_string)
{
  int i;

  /* The standard exceptions are a special case.  They are defined in
     runtime units that have been compiled without debugging info; if
     EXCEP_STRING is the not-fully-qualified name of a standard
     exception (e.g. "constraint_error") then, during the evaluation
     of the condition expression, the symbol lookup on this name would
     *not* return this standard exception.  The catchpoint condition
     may then be set only on user-defined exceptions which have the
     same not-fully-qualified name (e.g. my_package.constraint_error).

     To avoid this unexcepted behavior, these standard exceptions are
     systematically prefixed by "standard".  This means that "catch
     exception constraint_error" is rewritten into "catch exception
     standard.constraint_error".

     If an exception named contraint_error is defined in another package of
     the inferior program, then the only way to specify this exception as a
     breakpoint condition is to use its fully-qualified named:
     e.g. my_package.constraint_error.  */

  for (i = 0; i < sizeof (standard_exc) / sizeof (char *); i++)
    {
      if (strcmp (standard_exc [i], excep_string) == 0)
	{
          return xstrprintf ("long_integer (e) = long_integer (&standard.%s)",
                             excep_string);
	}
    }
  return xstrprintf ("long_integer (e) = long_integer (&%s)", excep_string);
}

/* Return the symtab_and_line that should be used to insert an exception
   catchpoint of the TYPE kind.

   EXCEP_STRING should contain the name of a specific exception that
   the catchpoint should catch, or NULL otherwise.

   ADDR_STRING returns the name of the function where the real
   breakpoint that implements the catchpoints is set, depending on the
   type of catchpoint we need to create.  */

static struct symtab_and_line
ada_exception_sal (enum ada_exception_catchpoint_kind ex, char *excep_string,
		   char **addr_string, const struct breakpoint_ops **ops)
{
  const char *sym_name;
  struct symbol *sym;

  /* First, find out which exception support info to use.  */
  ada_exception_support_info_sniffer ();

  /* Then lookup the function on which we will break in order to catch
     the Ada exceptions requested by the user.  */
  sym_name = ada_exception_sym_name (ex);
  sym = standard_lookup (sym_name, NULL, VAR_DOMAIN);

  /* We can assume that SYM is not NULL at this stage.  If the symbol
     did not exist, ada_exception_support_info_sniffer would have
     raised an exception.

     Also, ada_exception_support_info_sniffer should have already
     verified that SYM is a function symbol.  */
  gdb_assert (sym != NULL);
  gdb_assert (SYMBOL_CLASS (sym) == LOC_BLOCK);

  /* Set ADDR_STRING.  */
  *addr_string = xstrdup (sym_name);

  /* Set OPS.  */
  *ops = ada_exception_breakpoint_ops (ex);

  return find_function_start_sal (sym, 1);
}

/* Create an Ada exception catchpoint.

   EX_KIND is the kind of exception catchpoint to be created.

   If EXCEPT_STRING is NULL, this catchpoint is expected to trigger
   for all exceptions.  Otherwise, EXCEPT_STRING indicates the name
   of the exception to which this catchpoint applies.  When not NULL,
   the string must be allocated on the heap, and its deallocation
   is no longer the responsibility of the caller.

   COND_STRING, if not NULL, is the catchpoint condition.  This string
   must be allocated on the heap, and its deallocation is no longer
   the responsibility of the caller.

   TEMPFLAG, if nonzero, means that the underlying breakpoint
   should be temporary.

   FROM_TTY is the usual argument passed to all commands implementations.  */

void
create_ada_exception_catchpoint (struct gdbarch *gdbarch,
				 enum ada_exception_catchpoint_kind ex_kind,
				 char *excep_string,
				 char *cond_string,
				 int tempflag,
				 int disabled,
				 int from_tty)
{
  struct ada_catchpoint *c;
  char *addr_string = NULL;
  const struct breakpoint_ops *ops = NULL;
  struct symtab_and_line sal
    = ada_exception_sal (ex_kind, excep_string, &addr_string, &ops);

  c = XNEW (struct ada_catchpoint);
  init_ada_exception_breakpoint (&c->base, gdbarch, sal, addr_string,
				 ops, tempflag, disabled, from_tty);
  c->excep_string = excep_string;
  create_excep_cond_exprs (c);
  if (cond_string != NULL)
    set_breakpoint_condition (&c->base, cond_string, from_tty);
  install_breakpoint (0, &c->base, 1);
}

/* Implement the "catch exception" command.  */

static void
catch_ada_exception_command (char *arg, int from_tty,
			     struct cmd_list_element *command)
{
  struct gdbarch *gdbarch = get_current_arch ();
  int tempflag;
  enum ada_exception_catchpoint_kind ex_kind;
  char *excep_string = NULL;
  char *cond_string = NULL;

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  catch_ada_exception_command_split (arg, &ex_kind, &excep_string,
				     &cond_string);
  create_ada_exception_catchpoint (gdbarch, ex_kind,
				   excep_string, cond_string,
				   tempflag, 1 /* enabled */,
				   from_tty);
}

/* Split the arguments specified in a "catch assert" command.

   ARGS contains the command's arguments (or the empty string if
   no arguments were passed).

   If ARGS contains a condition, set COND_STRING to that condition
   (the memory needs to be deallocated after use).  */

static void
catch_ada_assert_command_split (char *args, char **cond_string)
{
  args = skip_spaces (args);

  /* Check whether a condition was provided.  */
  if (startswith (args, "if")
      && (isspace (args[2]) || args[2] == '\0'))
    {
      args += 2;
      args = skip_spaces (args);
      if (args[0] == '\0')
        error (_("condition missing after `if' keyword"));
      *cond_string = xstrdup (args);
    }

  /* Otherwise, there should be no other argument at the end of
     the command.  */
  else if (args[0] != '\0')
    error (_("Junk at end of arguments."));
}

/* Implement the "catch assert" command.  */

static void
catch_assert_command (char *arg, int from_tty,
		      struct cmd_list_element *command)
{
  struct gdbarch *gdbarch = get_current_arch ();
  int tempflag;
  char *cond_string = NULL;

  tempflag = get_cmd_context (command) == CATCH_TEMPORARY;

  if (!arg)
    arg = "";
  catch_ada_assert_command_split (arg, &cond_string);
  create_ada_exception_catchpoint (gdbarch, ada_catch_assert,
				   NULL, cond_string,
				   tempflag, 1 /* enabled */,
				   from_tty);
}

/* Return non-zero if the symbol SYM is an Ada exception object.  */

static int
ada_is_exception_sym (struct symbol *sym)
{
  const char *type_name = type_name_no_tag (SYMBOL_TYPE (sym));

  return (SYMBOL_CLASS (sym) != LOC_TYPEDEF
          && SYMBOL_CLASS (sym) != LOC_BLOCK
          && SYMBOL_CLASS (sym) != LOC_CONST
          && SYMBOL_CLASS (sym) != LOC_UNRESOLVED
          && type_name != NULL && strcmp (type_name, "exception") == 0);
}

/* Given a global symbol SYM, return non-zero iff SYM is a non-standard
   Ada exception object.  This matches all exceptions except the ones
   defined by the Ada language.  */

static int
ada_is_non_standard_exception_sym (struct symbol *sym)
{
  int i;

  if (!ada_is_exception_sym (sym))
    return 0;

  for (i = 0; i < ARRAY_SIZE (standard_exc); i++)
    if (strcmp (SYMBOL_LINKAGE_NAME (sym), standard_exc[i]) == 0)
      return 0;  /* A standard exception.  */

  /* Numeric_Error is also a standard exception, so exclude it.
     See the STANDARD_EXC description for more details as to why
     this exception is not listed in that array.  */
  if (strcmp (SYMBOL_LINKAGE_NAME (sym), "numeric_error") == 0)
    return 0;

  return 1;
}

/* A helper function for qsort, comparing two struct ada_exc_info
   objects.

   The comparison is determined first by exception name, and then
   by exception address.  */

static int
compare_ada_exception_info (const void *a, const void *b)
{
  const struct ada_exc_info *exc_a = (struct ada_exc_info *) a;
  const struct ada_exc_info *exc_b = (struct ada_exc_info *) b;
  int result;

  result = strcmp (exc_a->name, exc_b->name);
  if (result != 0)
    return result;

  if (exc_a->addr < exc_b->addr)
    return -1;
  if (exc_a->addr > exc_b->addr)
    return 1;

  return 0;
}

/* Sort EXCEPTIONS using compare_ada_exception_info as the comparison
   routine, but keeping the first SKIP elements untouched.

   All duplicates are also removed.  */

static void
sort_remove_dups_ada_exceptions_list (VEC(ada_exc_info) **exceptions,
				      int skip)
{
  struct ada_exc_info *to_sort
    = VEC_address (ada_exc_info, *exceptions) + skip;
  int to_sort_len
    = VEC_length (ada_exc_info, *exceptions) - skip;
  int i, j;

  qsort (to_sort, to_sort_len, sizeof (struct ada_exc_info),
	 compare_ada_exception_info);

  for (i = 1, j = 1; i < to_sort_len; i++)
    if (compare_ada_exception_info (&to_sort[i], &to_sort[j - 1]) != 0)
      to_sort[j++] = to_sort[i];
  to_sort_len = j;
  VEC_truncate(ada_exc_info, *exceptions, skip + to_sort_len);
}

/* A function intended as the "name_matcher" callback in the struct
   quick_symbol_functions' expand_symtabs_matching method.

   SEARCH_NAME is the symbol's search name.

   If USER_DATA is not NULL, it is a pointer to a regext_t object
   used to match the symbol (by natural name).  Otherwise, when USER_DATA
   is null, no filtering is performed, and all symbols are a positive
   match.  */

static int
ada_exc_search_name_matches (const char *search_name, void *user_data)
{
  regex_t *preg = (regex_t *) user_data;

  if (preg == NULL)
    return 1;

  /* In Ada, the symbol "search name" is a linkage name, whereas
     the regular expression used to do the matching refers to
     the natural name.  So match against the decoded name.  */
  return (regexec (preg, ada_decode (search_name), 0, NULL, 0) == 0);
}

/* Add all exceptions defined by the Ada standard whose name match
   a regular expression.

   If PREG is not NULL, then this regexp_t object is used to
   perform the symbol name matching.  Otherwise, no name-based
   filtering is performed.

   EXCEPTIONS is a vector of exceptions to which matching exceptions
   gets pushed.  */

static void
ada_add_standard_exceptions (regex_t *preg, VEC(ada_exc_info) **exceptions)
{
  int i;

  for (i = 0; i < ARRAY_SIZE (standard_exc); i++)
    {
      if (preg == NULL
	  || regexec (preg, standard_exc[i], 0, NULL, 0) == 0)
	{
	  struct bound_minimal_symbol msymbol
	    = ada_lookup_simple_minsym (standard_exc[i]);

	  if (msymbol.minsym != NULL)
	    {
	      struct ada_exc_info info
		= {standard_exc[i], BMSYMBOL_VALUE_ADDRESS (msymbol)};

	      VEC_safe_push (ada_exc_info, *exceptions, &info);
	    }
	}
    }
}

/* Add all Ada exceptions defined locally and accessible from the given
   FRAME.

   If PREG is not NULL, then this regexp_t object is used to
   perform the symbol name matching.  Otherwise, no name-based
   filtering is performed.

   EXCEPTIONS is a vector of exceptions to which matching exceptions
   gets pushed.  */

static void
ada_add_exceptions_from_frame (regex_t *preg, struct frame_info *frame,
			       VEC(ada_exc_info) **exceptions)
{
  const struct block *block = get_frame_block (frame, 0);

  while (block != 0)
    {
      struct block_iterator iter;
      struct symbol *sym;

      ALL_BLOCK_SYMBOLS (block, iter, sym)
	{
	  switch (SYMBOL_CLASS (sym))
	    {
	    case LOC_TYPEDEF:
	    case LOC_BLOCK:
	    case LOC_CONST:
	      break;
	    default:
	      if (ada_is_exception_sym (sym))
		{
		  struct ada_exc_info info = {SYMBOL_PRINT_NAME (sym),
					      SYMBOL_VALUE_ADDRESS (sym)};

		  VEC_safe_push (ada_exc_info, *exceptions, &info);
		}
	    }
	}
      if (BLOCK_FUNCTION (block) != NULL)
	break;
      block = BLOCK_SUPERBLOCK (block);
    }
}

/* Add all exceptions defined globally whose name name match
   a regular expression, excluding standard exceptions.

   The reason we exclude standard exceptions is that they need
   to be handled separately: Standard exceptions are defined inside
   a runtime unit which is normally not compiled with debugging info,
   and thus usually do not show up in our symbol search.  However,
   if the unit was in fact built with debugging info, we need to
   exclude them because they would duplicate the entry we found
   during the special loop that specifically searches for those
   standard exceptions.

   If PREG is not NULL, then this regexp_t object is used to
   perform the symbol name matching.  Otherwise, no name-based
   filtering is performed.

   EXCEPTIONS is a vector of exceptions to which matching exceptions
   gets pushed.  */

static void
ada_add_global_exceptions (regex_t *preg, VEC(ada_exc_info) **exceptions)
{
  struct objfile *objfile;
  struct compunit_symtab *s;

  expand_symtabs_matching (NULL, ada_exc_search_name_matches, NULL,
			   VARIABLES_DOMAIN, preg);

  ALL_COMPUNITS (objfile, s)
    {
      const struct blockvector *bv = COMPUNIT_BLOCKVECTOR (s);
      int i;

      for (i = GLOBAL_BLOCK; i <= STATIC_BLOCK; i++)
	{
	  struct block *b = BLOCKVECTOR_BLOCK (bv, i);
	  struct block_iterator iter;
	  struct symbol *sym;

	  ALL_BLOCK_SYMBOLS (b, iter, sym)
	    if (ada_is_non_standard_exception_sym (sym)
		&& (preg == NULL
		    || regexec (preg, SYMBOL_NATURAL_NAME (sym),
				0, NULL, 0) == 0))
	      {
		struct ada_exc_info info
		  = {SYMBOL_PRINT_NAME (sym), SYMBOL_VALUE_ADDRESS (sym)};

		VEC_safe_push (ada_exc_info, *exceptions, &info);
	      }
	}
    }
}

/* Implements ada_exceptions_list with the regular expression passed
   as a regex_t, rather than a string.

   If not NULL, PREG is used to filter out exceptions whose names
   do not match.  Otherwise, all exceptions are listed.  */

static VEC(ada_exc_info) *
ada_exceptions_list_1 (regex_t *preg)
{
  VEC(ada_exc_info) *result = NULL;
  struct cleanup *old_chain
    = make_cleanup (VEC_cleanup (ada_exc_info), &result);
  int prev_len;

  /* First, list the known standard exceptions.  These exceptions
     need to be handled separately, as they are usually defined in
     runtime units that have been compiled without debugging info.  */

  ada_add_standard_exceptions (preg, &result);

  /* Next, find all exceptions whose scope is local and accessible
     from the currently selected frame.  */

  if (has_stack_frames ())
    {
      prev_len = VEC_length (ada_exc_info, result);
      ada_add_exceptions_from_frame (preg, get_selected_frame (NULL),
				     &result);
      if (VEC_length (ada_exc_info, result) > prev_len)
	sort_remove_dups_ada_exceptions_list (&result, prev_len);
    }

  /* Add all exceptions whose scope is global.  */

  prev_len = VEC_length (ada_exc_info, result);
  ada_add_global_exceptions (preg, &result);
  if (VEC_length (ada_exc_info, result) > prev_len)
    sort_remove_dups_ada_exceptions_list (&result, prev_len);

  discard_cleanups (old_chain);
  return result;
}

/* Return a vector of ada_exc_info.

   If REGEXP is NULL, all exceptions are included in the result.
   Otherwise, it should contain a valid regular expression,
   and only the exceptions whose names match that regular expression
   are included in the result.

   The exceptions are sorted in the following order:
     - Standard exceptions (defined by the Ada language), in
       alphabetical order;
     - Exceptions only visible from the current frame, in
       alphabetical order;
     - Exceptions whose scope is global, in alphabetical order.  */

VEC(ada_exc_info) *
ada_exceptions_list (const char *regexp)
{
  VEC(ada_exc_info) *result = NULL;
  struct cleanup *old_chain = NULL;
  regex_t reg;

  if (regexp != NULL)
    old_chain = compile_rx_or_error (&reg, regexp,
				     _("invalid regular expression"));

  result = ada_exceptions_list_1 (regexp != NULL ? &reg : NULL);

  if (old_chain != NULL)
    do_cleanups (old_chain);
  return result;
}

/* Implement the "info exceptions" command.  */

static void
info_exceptions_command (char *regexp, int from_tty)
{
  VEC(ada_exc_info) *exceptions;
  struct cleanup *cleanup;
  struct gdbarch *gdbarch = get_current_arch ();
  int ix;
  struct ada_exc_info *info;

  exceptions = ada_exceptions_list (regexp);
  cleanup = make_cleanup (VEC_cleanup (ada_exc_info), &exceptions);

  if (regexp != NULL)
    printf_filtered
      (_("All Ada exceptions matching regular expression \"%s\":\n"), regexp);
  else
    printf_filtered (_("All defined Ada exceptions:\n"));

  for (ix = 0; VEC_iterate(ada_exc_info, exceptions, ix, info); ix++)
    printf_filtered ("%s: %s\n", info->name, paddress (gdbarch, info->addr));

  do_cleanups (cleanup);
}

                                /* Operators */
/* Information about operators given special treatment in functions
   below.  */
/* Format: OP_DEFN (<operator>, <operator length>, <# args>, <binop>).  */

#define ADA_OPERATORS \
    OP_DEFN (OP_VAR_VALUE, 4, 0, 0) \
    OP_DEFN (BINOP_IN_BOUNDS, 3, 2, 0) \
    OP_DEFN (TERNOP_IN_RANGE, 1, 3, 0) \
    OP_DEFN (OP_ATR_FIRST, 1, 2, 0) \
    OP_DEFN (OP_ATR_LAST, 1, 2, 0) \
    OP_DEFN (OP_ATR_LENGTH, 1, 2, 0) \
    OP_DEFN (OP_ATR_IMAGE, 1, 2, 0) \
    OP_DEFN (OP_ATR_MAX, 1, 3, 0) \
    OP_DEFN (OP_ATR_MIN, 1, 3, 0) \
    OP_DEFN (OP_ATR_MODULUS, 1, 1, 0) \
    OP_DEFN (OP_ATR_POS, 1, 2, 0) \
    OP_DEFN (OP_ATR_SIZE, 1, 1, 0) \
    OP_DEFN (OP_ATR_TAG, 1, 1, 0) \
    OP_DEFN (OP_ATR_VAL, 1, 2, 0) \
    OP_DEFN (UNOP_QUAL, 3, 1, 0) \
    OP_DEFN (UNOP_IN_RANGE, 3, 1, 0) \
    OP_DEFN (OP_OTHERS, 1, 1, 0) \
    OP_DEFN (OP_POSITIONAL, 3, 1, 0) \
    OP_DEFN (OP_DISCRETE_RANGE, 1, 2, 0)

static void
ada_operator_length (const struct expression *exp, int pc, int *oplenp,
		     int *argsp)
{
  switch (exp->elts[pc - 1].opcode)
    {
    default:
      operator_length_standard (exp, pc, oplenp, argsp);
      break;

#define OP_DEFN(op, len, args, binop) \
    case op: *oplenp = len; *argsp = args; break;
      ADA_OPERATORS;
#undef OP_DEFN

    case OP_AGGREGATE:
      *oplenp = 3;
      *argsp = longest_to_int (exp->elts[pc - 2].longconst);
      break;

    case OP_CHOICES:
      *oplenp = 3;
      *argsp = longest_to_int (exp->elts[pc - 2].longconst) + 1;
      break;
    }
}

/* Implementation of the exp_descriptor method operator_check.  */

static int
ada_operator_check (struct expression *exp, int pos,
		    int (*objfile_func) (struct objfile *objfile, void *data),
		    void *data)
{
  const union exp_element *const elts = exp->elts;
  struct type *type = NULL;

  switch (elts[pos].opcode)
    {
      case UNOP_IN_RANGE:
      case UNOP_QUAL:
	type = elts[pos + 1].type;
	break;

      default:
	return operator_check_standard (exp, pos, objfile_func, data);
    }

  /* Invoke callbacks for TYPE and OBJFILE if they were set as non-NULL.  */

  if (type && TYPE_OBJFILE (type)
      && (*objfile_func) (TYPE_OBJFILE (type), data))
    return 1;

  return 0;
}

static char *
ada_op_name (enum exp_opcode opcode)
{
  switch (opcode)
    {
    default:
      return op_name_standard (opcode);

#define OP_DEFN(op, len, args, binop) case op: return #op;
      ADA_OPERATORS;
#undef OP_DEFN

    case OP_AGGREGATE:
      return "OP_AGGREGATE";
    case OP_CHOICES:
      return "OP_CHOICES";
    case OP_NAME:
      return "OP_NAME";
    }
}

/* As for operator_length, but assumes PC is pointing at the first
   element of the operator, and gives meaningful results only for the 
   Ada-specific operators, returning 0 for *OPLENP and *ARGSP otherwise.  */

static void
ada_forward_operator_length (struct expression *exp, int pc,
                             int *oplenp, int *argsp)
{
  switch (exp->elts[pc].opcode)
    {
    default:
      *oplenp = *argsp = 0;
      break;

#define OP_DEFN(op, len, args, binop) \
    case op: *oplenp = len; *argsp = args; break;
      ADA_OPERATORS;
#undef OP_DEFN

    case OP_AGGREGATE:
      *oplenp = 3;
      *argsp = longest_to_int (exp->elts[pc + 1].longconst);
      break;

    case OP_CHOICES:
      *oplenp = 3;
      *argsp = longest_to_int (exp->elts[pc + 1].longconst) + 1;
      break;

    case OP_STRING:
    case OP_NAME:
      {
	int len = longest_to_int (exp->elts[pc + 1].longconst);

	*oplenp = 4 + BYTES_TO_EXP_ELEM (len + 1);
	*argsp = 0;
	break;
      }
    }
}

static int
ada_dump_subexp_body (struct expression *exp, struct ui_file *stream, int elt)
{
  enum exp_opcode op = exp->elts[elt].opcode;
  int oplen, nargs;
  int pc = elt;
  int i;

  ada_forward_operator_length (exp, elt, &oplen, &nargs);

  switch (op)
    {
      /* Ada attributes ('Foo).  */
    case OP_ATR_FIRST:
    case OP_ATR_LAST:
    case OP_ATR_LENGTH:
    case OP_ATR_IMAGE:
    case OP_ATR_MAX:
    case OP_ATR_MIN:
    case OP_ATR_MODULUS:
    case OP_ATR_POS:
    case OP_ATR_SIZE:
    case OP_ATR_TAG:
    case OP_ATR_VAL:
      break;

    case UNOP_IN_RANGE:
    case UNOP_QUAL:
      /* XXX: gdb_sprint_host_address, type_sprint */
      fprintf_filtered (stream, _("Type @"));
      gdb_print_host_address (exp->elts[pc + 1].type, stream);
      fprintf_filtered (stream, " (");
      type_print (exp->elts[pc + 1].type, NULL, stream, 0);
      fprintf_filtered (stream, ")");
      break;
    case BINOP_IN_BOUNDS:
      fprintf_filtered (stream, " (%d)",
			longest_to_int (exp->elts[pc + 2].longconst));
      break;
    case TERNOP_IN_RANGE:
      break;

    case OP_AGGREGATE:
    case OP_OTHERS:
    case OP_DISCRETE_RANGE:
    case OP_POSITIONAL:
    case OP_CHOICES:
      break;

    case OP_NAME:
    case OP_STRING:
      {
	char *name = &exp->elts[elt + 2].string;
	int len = longest_to_int (exp->elts[elt + 1].longconst);

	fprintf_filtered (stream, "Text: `%.*s'", len, name);
	break;
      }

    default:
      return dump_subexp_body_standard (exp, stream, elt);
    }

  elt += oplen;
  for (i = 0; i < nargs; i += 1)
    elt = dump_subexp (exp, stream, elt);

  return elt;
}

/* The Ada extension of print_subexp (q.v.).  */

static void
ada_print_subexp (struct expression *exp, int *pos,
                  struct ui_file *stream, enum precedence prec)
{
  int oplen, nargs, i;
  int pc = *pos;
  enum exp_opcode op = exp->elts[pc].opcode;

  ada_forward_operator_length (exp, pc, &oplen, &nargs);

  *pos += oplen;
  switch (op)
    {
    default:
      *pos -= oplen;
      print_subexp_standard (exp, pos, stream, prec);
      return;

    case OP_VAR_VALUE:
      fputs_filtered (SYMBOL_NATURAL_NAME (exp->elts[pc + 2].symbol), stream);
      return;

    case BINOP_IN_BOUNDS:
      /* XXX: sprint_subexp */
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered (" in ", stream);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered ("'range", stream);
      if (exp->elts[pc + 1].longconst > 1)
        fprintf_filtered (stream, "(%ld)",
                          (long) exp->elts[pc + 1].longconst);
      return;

    case TERNOP_IN_RANGE:
      if (prec >= PREC_EQUAL)
        fputs_filtered ("(", stream);
      /* XXX: sprint_subexp */
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered (" in ", stream);
      print_subexp (exp, pos, stream, PREC_EQUAL);
      fputs_filtered (" .. ", stream);
      print_subexp (exp, pos, stream, PREC_EQUAL);
      if (prec >= PREC_EQUAL)
        fputs_filtered (")", stream);
      return;

    case OP_ATR_FIRST:
    case OP_ATR_LAST:
    case OP_ATR_LENGTH:
    case OP_ATR_IMAGE:
    case OP_ATR_MAX:
    case OP_ATR_MIN:
    case OP_ATR_MODULUS:
    case OP_ATR_POS:
    case OP_ATR_SIZE:
    case OP_ATR_TAG:
    case OP_ATR_VAL:
      if (exp->elts[*pos].opcode == OP_TYPE)
        {
          if (TYPE_CODE (exp->elts[*pos + 1].type) != TYPE_CODE_VOID)
            LA_PRINT_TYPE (exp->elts[*pos + 1].type, "", stream, 0, 0,
			   &type_print_raw_options);
          *pos += 3;
        }
      else
        print_subexp (exp, pos, stream, PREC_SUFFIX);
      fprintf_filtered (stream, "'%s", ada_attribute_name (op));
      if (nargs > 1)
        {
          int tem;

          for (tem = 1; tem < nargs; tem += 1)
            {
              fputs_filtered ((tem == 1) ? " (" : ", ", stream);
              print_subexp (exp, pos, stream, PREC_ABOVE_COMMA);
            }
          fputs_filtered (")", stream);
        }
      return;

    case UNOP_QUAL:
      type_print (exp->elts[pc + 1].type, "", stream, 0);
      fputs_filtered ("'(", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      fputs_filtered (")", stream);
      return;

    case UNOP_IN_RANGE:
      /* XXX: sprint_subexp */
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered (" in ", stream);
      LA_PRINT_TYPE (exp->elts[pc + 1].type, "", stream, 1, 0,
		     &type_print_raw_options);
      return;

    case OP_DISCRETE_RANGE:
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered ("..", stream);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      return;

    case OP_OTHERS:
      fputs_filtered ("others => ", stream);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      return;

    case OP_CHOICES:
      for (i = 0; i < nargs-1; i += 1)
	{
	  if (i > 0)
	    fputs_filtered ("|", stream);
	  print_subexp (exp, pos, stream, PREC_SUFFIX);
	}
      fputs_filtered (" => ", stream);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      return;
      
    case OP_POSITIONAL:
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      return;

    case OP_AGGREGATE:
      fputs_filtered ("(", stream);
      for (i = 0; i < nargs; i += 1)
	{
	  if (i > 0)
	    fputs_filtered (", ", stream);
	  print_subexp (exp, pos, stream, PREC_SUFFIX);
	}
      fputs_filtered (")", stream);
      return;
    }
}

/* Table mapping opcodes into strings for printing operators
   and precedences of the operators.  */

static const struct op_print ada_op_print_tab[] = {
  {":=", BINOP_ASSIGN, PREC_ASSIGN, 1},
  {"or else", BINOP_LOGICAL_OR, PREC_LOGICAL_OR, 0},
  {"and then", BINOP_LOGICAL_AND, PREC_LOGICAL_AND, 0},
  {"or", BINOP_BITWISE_IOR, PREC_BITWISE_IOR, 0},
  {"xor", BINOP_BITWISE_XOR, PREC_BITWISE_XOR, 0},
  {"and", BINOP_BITWISE_AND, PREC_BITWISE_AND, 0},
  {"=", BINOP_EQUAL, PREC_EQUAL, 0},
  {"/=", BINOP_NOTEQUAL, PREC_EQUAL, 0},
  {"<=", BINOP_LEQ, PREC_ORDER, 0},
  {">=", BINOP_GEQ, PREC_ORDER, 0},
  {">", BINOP_GTR, PREC_ORDER, 0},
  {"<", BINOP_LESS, PREC_ORDER, 0},
  {">>", BINOP_RSH, PREC_SHIFT, 0},
  {"<<", BINOP_LSH, PREC_SHIFT, 0},
  {"+", BINOP_ADD, PREC_ADD, 0},
  {"-", BINOP_SUB, PREC_ADD, 0},
  {"&", BINOP_CONCAT, PREC_ADD, 0},
  {"*", BINOP_MUL, PREC_MUL, 0},
  {"/", BINOP_DIV, PREC_MUL, 0},
  {"rem", BINOP_REM, PREC_MUL, 0},
  {"mod", BINOP_MOD, PREC_MUL, 0},
  {"**", BINOP_EXP, PREC_REPEAT, 0},
  {"@", BINOP_REPEAT, PREC_REPEAT, 0},
  {"-", UNOP_NEG, PREC_PREFIX, 0},
  {"+", UNOP_PLUS, PREC_PREFIX, 0},
  {"not ", UNOP_LOGICAL_NOT, PREC_PREFIX, 0},
  {"not ", UNOP_COMPLEMENT, PREC_PREFIX, 0},
  {"abs ", UNOP_ABS, PREC_PREFIX, 0},
  {".all", UNOP_IND, PREC_SUFFIX, 1},
  {"'access", UNOP_ADDR, PREC_SUFFIX, 1},
  {"'size", OP_ATR_SIZE, PREC_SUFFIX, 1},
  {NULL, OP_NULL, PREC_SUFFIX, 0}
};

enum ada_primitive_types {
  ada_primitive_type_int,
  ada_primitive_type_long,
  ada_primitive_type_short,
  ada_primitive_type_char,
  ada_primitive_type_float,
  ada_primitive_type_double,
  ada_primitive_type_void,
  ada_primitive_type_long_long,
  ada_primitive_type_long_double,
  ada_primitive_type_natural,
  ada_primitive_type_positive,
  ada_primitive_type_system_address,
  nr_ada_primitive_types
};

static void
ada_language_arch_info (struct gdbarch *gdbarch,
			struct language_arch_info *lai)
{
  const struct builtin_type *builtin = builtin_type (gdbarch);

  lai->primitive_type_vector
    = GDBARCH_OBSTACK_CALLOC (gdbarch, nr_ada_primitive_types + 1,
			      struct type *);

  lai->primitive_type_vector [ada_primitive_type_int]
    = arch_integer_type (gdbarch, gdbarch_int_bit (gdbarch),
			 0, "integer");
  lai->primitive_type_vector [ada_primitive_type_long]
    = arch_integer_type (gdbarch, gdbarch_long_bit (gdbarch),
			 0, "long_integer");
  lai->primitive_type_vector [ada_primitive_type_short]
    = arch_integer_type (gdbarch, gdbarch_short_bit (gdbarch),
			 0, "short_integer");
  lai->string_char_type
    = lai->primitive_type_vector [ada_primitive_type_char]
    = arch_character_type (gdbarch, TARGET_CHAR_BIT, 0, "character");
  lai->primitive_type_vector [ada_primitive_type_float]
    = arch_float_type (gdbarch, gdbarch_float_bit (gdbarch),
		       "float", NULL);
  lai->primitive_type_vector [ada_primitive_type_double]
    = arch_float_type (gdbarch, gdbarch_double_bit (gdbarch),
		       "long_float", NULL);
  lai->primitive_type_vector [ada_primitive_type_long_long]
    = arch_integer_type (gdbarch, gdbarch_long_long_bit (gdbarch),
			 0, "long_long_integer");
  lai->primitive_type_vector [ada_primitive_type_long_double]
    = arch_float_type (gdbarch, gdbarch_double_bit (gdbarch),
		       "long_long_float", NULL);
  lai->primitive_type_vector [ada_primitive_type_natural]
    = arch_integer_type (gdbarch, gdbarch_int_bit (gdbarch),
			 0, "natural");
  lai->primitive_type_vector [ada_primitive_type_positive]
    = arch_integer_type (gdbarch, gdbarch_int_bit (gdbarch),
			 0, "positive");
  lai->primitive_type_vector [ada_primitive_type_void]
    = builtin->builtin_void;

  lai->primitive_type_vector [ada_primitive_type_system_address]
    = lookup_pointer_type (arch_type (gdbarch, TYPE_CODE_VOID, 1, "void"));
  TYPE_NAME (lai->primitive_type_vector [ada_primitive_type_system_address])
    = "system__address";

  lai->bool_type_symbol = NULL;
  lai->bool_type_default = builtin->builtin_bool;
}

				/* Language vector */

/* Not really used, but needed in the ada_language_defn.  */

static void
emit_char (int c, struct type *type, struct ui_file *stream, int quoter)
{
  ada_emit_char (c, type, stream, quoter, 1);
}

static int
parse (struct parser_state *ps)
{
  warnings_issued = 0;
  return ada_parse (ps);
}

static const struct exp_descriptor ada_exp_descriptor = {
  ada_print_subexp,
  ada_operator_length,
  ada_operator_check,
  ada_op_name,
  ada_dump_subexp_body,
  ada_evaluate_subexp
};

/* Implement the "la_get_symbol_name_cmp" language_defn method
   for Ada.  */

static symbol_name_cmp_ftype
ada_get_symbol_name_cmp (const char *lookup_name)
{
  if (should_use_wild_match (lookup_name))
    return wild_match;
  else
    return compare_names;
}

/* Implement the "la_read_var_value" language_defn method for Ada.  */

static struct value *
ada_read_var_value (struct symbol *var, const struct block *var_block,
		    struct frame_info *frame)
{
  const struct block *frame_block = NULL;
  struct symbol *renaming_sym = NULL;

  /* The only case where default_read_var_value is not sufficient
     is when VAR is a renaming...  */
  if (frame)
    frame_block = get_frame_block (frame, NULL);
  if (frame_block)
    renaming_sym = ada_find_renaming_symbol (var, frame_block);
  if (renaming_sym != NULL)
    return ada_read_renaming_var_value (renaming_sym, frame_block);

  /* This is a typical case where we expect the default_read_var_value
     function to work.  */
  return default_read_var_value (var, var_block, frame);
}

static const char *ada_extensions[] =
{
  ".adb", ".ads", ".a", ".ada", ".dg", NULL
};

const struct language_defn ada_language_defn = {
  "ada",                        /* Language name */
  "Ada",
  language_ada,
  range_check_off,
  case_sensitive_on,            /* Yes, Ada is case-insensitive, but
                                   that's not quite what this means.  */
  array_row_major,
  macro_expansion_no,
  ada_extensions,
  &ada_exp_descriptor,
  parse,
  ada_yyerror,
  resolve,
  ada_printchar,                /* Print a character constant */
  ada_printstr,                 /* Function to print string constant */
  emit_char,                    /* Function to print single char (not used) */
  ada_print_type,               /* Print a type using appropriate syntax */
  ada_print_typedef,            /* Print a typedef using appropriate syntax */
  ada_val_print,                /* Print a value using appropriate syntax */
  ada_value_print,              /* Print a top-level value */
  ada_read_var_value,		/* la_read_var_value */
  NULL,                         /* Language specific skip_trampoline */
  NULL,                         /* name_of_this */
  ada_lookup_symbol_nonlocal,   /* Looking up non-local symbols.  */
  basic_lookup_transparent_type,        /* lookup_transparent_type */
  ada_la_decode,                /* Language specific symbol demangler */
  ada_sniff_from_mangled_name,
  NULL,                         /* Language specific
				   class_name_from_physname */
  ada_op_print_tab,             /* expression operators for printing */
  0,                            /* c-style arrays */
  1,                            /* String lower bound */
  ada_get_gdb_completer_word_break_characters,
  ada_make_symbol_completion_list,
  ada_language_arch_info,
  ada_print_array_index,
  default_pass_by_reference,
  c_get_string,
  ada_get_symbol_name_cmp,	/* la_get_symbol_name_cmp */
  ada_iterate_over_symbols,
  &ada_varobj_ops,
  NULL,
  NULL,
  LANG_MAGIC
};

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_ada_language;

/* Command-list for the "set/show ada" prefix command.  */
static struct cmd_list_element *set_ada_list;
static struct cmd_list_element *show_ada_list;

/* Implement the "set ada" prefix command.  */

static void
set_ada_command (char *arg, int from_tty)
{
  printf_unfiltered (_(\
"\"set ada\" must be followed by the name of a setting.\n"));
  help_list (set_ada_list, "set ada ", all_commands, gdb_stdout);
}

/* Implement the "show ada" prefix command.  */

static void
show_ada_command (char *args, int from_tty)
{
  cmd_show_list (show_ada_list, from_tty, "");
}

static void
initialize_ada_catchpoint_ops (void)
{
  struct breakpoint_ops *ops;

  initialize_breakpoint_ops ();

  ops = &catch_exception_breakpoint_ops;
  *ops = bkpt_breakpoint_ops;
  ops->dtor = dtor_catch_exception;
  ops->allocate_location = allocate_location_catch_exception;
  ops->re_set = re_set_catch_exception;
  ops->check_status = check_status_catch_exception;
  ops->print_it = print_it_catch_exception;
  ops->print_one = print_one_catch_exception;
  ops->print_mention = print_mention_catch_exception;
  ops->print_recreate = print_recreate_catch_exception;

  ops = &catch_exception_unhandled_breakpoint_ops;
  *ops = bkpt_breakpoint_ops;
  ops->dtor = dtor_catch_exception_unhandled;
  ops->allocate_location = allocate_location_catch_exception_unhandled;
  ops->re_set = re_set_catch_exception_unhandled;
  ops->check_status = check_status_catch_exception_unhandled;
  ops->print_it = print_it_catch_exception_unhandled;
  ops->print_one = print_one_catch_exception_unhandled;
  ops->print_mention = print_mention_catch_exception_unhandled;
  ops->print_recreate = print_recreate_catch_exception_unhandled;

  ops = &catch_assert_breakpoint_ops;
  *ops = bkpt_breakpoint_ops;
  ops->dtor = dtor_catch_assert;
  ops->allocate_location = allocate_location_catch_assert;
  ops->re_set = re_set_catch_assert;
  ops->check_status = check_status_catch_assert;
  ops->print_it = print_it_catch_assert;
  ops->print_one = print_one_catch_assert;
  ops->print_mention = print_mention_catch_assert;
  ops->print_recreate = print_recreate_catch_assert;
}

/* This module's 'new_objfile' observer.  */

static void
ada_new_objfile_observer (struct objfile *objfile)
{
  ada_clear_symbol_cache ();
}

/* This module's 'free_objfile' observer.  */

static void
ada_free_objfile_observer (struct objfile *objfile)
{
  ada_clear_symbol_cache ();
}

void
_initialize_ada_language (void)
{
  add_language (&ada_language_defn);

  initialize_ada_catchpoint_ops ();

  add_prefix_cmd ("ada", no_class, set_ada_command,
                  _("Prefix command for changing Ada-specfic settings"),
                  &set_ada_list, "set ada ", 0, &setlist);

  add_prefix_cmd ("ada", no_class, show_ada_command,
                  _("Generic command for showing Ada-specific settings."),
                  &show_ada_list, "show ada ", 0, &showlist);

  add_setshow_boolean_cmd ("trust-PAD-over-XVS", class_obscure,
                           &trust_pad_over_xvs, _("\
Enable or disable an optimization trusting PAD types over XVS types"), _("\
Show whether an optimization trusting PAD types over XVS types is activated"),
                           _("\
This is related to the encoding used by the GNAT compiler.  The debugger\n\
should normally trust the contents of PAD types, but certain older versions\n\
of GNAT have a bug that sometimes causes the information in the PAD type\n\
to be incorrect.  Turning this setting \"off\" allows the debugger to\n\
work around this bug.  It is always safe to turn this option \"off\", but\n\
this incurs a slight performance penalty, so it is recommended to NOT change\n\
this option to \"off\" unless necessary."),
                            NULL, NULL, &set_ada_list, &show_ada_list);

  add_setshow_boolean_cmd ("print-signatures", class_vars,
			   &print_signatures, _("\
Enable or disable the output of formal and return types for functions in the \
overloads selection menu"), _("\
Show whether the output of formal and return types for functions in the \
overloads selection menu is activated"),
			   NULL, NULL, NULL, &set_ada_list, &show_ada_list);

  add_catch_command ("exception", _("\
Catch Ada exceptions, when raised.\n\
With an argument, catch only exceptions with the given name."),
		     catch_ada_exception_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);
  add_catch_command ("assert", _("\
Catch failed Ada assertions, when raised.\n\
With an argument, catch only exceptions with the given name."),
		     catch_assert_command,
                     NULL,
		     CATCH_PERMANENT,
		     CATCH_TEMPORARY);

  varsize_limit = 65536;

  add_info ("exceptions", info_exceptions_command,
	    _("\
List all Ada exception names.\n\
If a regular expression is passed as an argument, only those matching\n\
the regular expression are listed."));

  add_prefix_cmd ("ada", class_maintenance, maint_set_ada_cmd,
		  _("Set Ada maintenance-related variables."),
                  &maint_set_ada_cmdlist, "maintenance set ada ",
                  0/*allow-unknown*/, &maintenance_set_cmdlist);

  add_prefix_cmd ("ada", class_maintenance, maint_show_ada_cmd,
		  _("Show Ada maintenance-related variables"),
                  &maint_show_ada_cmdlist, "maintenance show ada ",
                  0/*allow-unknown*/, &maintenance_show_cmdlist);

  add_setshow_boolean_cmd
    ("ignore-descriptive-types", class_maintenance,
     &ada_ignore_descriptive_types_p,
     _("Set whether descriptive types generated by GNAT should be ignored."),
     _("Show whether descriptive types generated by GNAT should be ignored."),
     _("\
When enabled, the debugger will stop using the DW_AT_GNAT_descriptive_type\n\
DWARF attribute."),
     NULL, NULL, &maint_set_ada_cmdlist, &maint_show_ada_cmdlist);

  obstack_init (&symbol_list_obstack);

  decoded_names_store = htab_create_alloc
    (256, htab_hash_string, (int (*)(const void *, const void *)) streq,
     NULL, xcalloc, xfree);

  /* The ada-lang observers.  */
  observer_attach_new_objfile (ada_new_objfile_observer);
  observer_attach_free_objfile (ada_free_objfile_observer);
  observer_attach_inferior_exit (ada_inferior_exit);

  /* Setup various context-specific data.  */
  ada_inferior_data
    = register_inferior_data_with_cleanup (NULL, ada_inferior_data_cleanup);
  ada_pspace_data_handle
    = register_program_space_data_with_cleanup (NULL, ada_pspace_data_cleanup);
}
