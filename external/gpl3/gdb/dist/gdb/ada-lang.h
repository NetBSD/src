/* Ada language support definitions for GDB, the GNU debugger.

   Copyright (C) 1992-2017 Free Software Foundation, Inc.

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

#if !defined (ADA_LANG_H)
#define ADA_LANG_H 1

struct frame_info;
struct inferior;
struct type_print_options;
struct parser_state;

#include "value.h"
#include "gdbtypes.h"
#include "breakpoint.h"
#include "vec.h"

/* Names of specific files known to be part of the runtime
   system and that might consider (confusing) debugging information.
   Each name (a basic regular expression string) is followed by a
   comma.  FIXME: Should be part of a configuration file.  */
#if defined (__linux__)
#define ADA_KNOWN_RUNTIME_FILE_NAME_PATTERNS \
   "^[agis]-.*\\.ad[bs]$", \
   "/lib.*/libpthread\\.so[.0-9]*$", "/lib.*/libpthread\\.a$", \
   "/lib.*/libc\\.so[.0-9]*$", "/lib.*/libc\\.a$",
#endif

#if !defined (ADA_KNOWN_RUNTIME_FILE_NAME_PATTERNS)
#define ADA_KNOWN_RUNTIME_FILE_NAME_PATTERNS \
   "^unwind-seh.c$", \
   "^[agis]-.*\\.ad[bs]$",
#endif

/* Names of compiler-generated auxiliary functions probably of no
   interest to users.  Each name (a basic regular expression string)
   is followed by a comma.  */
#define ADA_KNOWN_AUXILIARY_FUNCTION_NAME_PATTERNS \
   "___clean[.$a-zA-Z0-9_]*$", \
   "___finalizer[.$a-zA-Z0-9_]*$",

/* The maximum number of frame levels searched for non-local,
 * non-global symbols.  This limit exists as a precaution to prevent
 * infinite search loops when the stack is screwed up.  */
#define MAX_ENCLOSING_FRAME_LEVELS 7

/* Maximum number of steps followed in looking for the ultimate
   referent of a renaming.  This prevents certain infinite loops that
   can otherwise result.  */
#define MAX_RENAMING_CHAIN_LENGTH 10

struct block;

/* Corresponding encoded/decoded names and opcodes for Ada user-definable
   operators.  */
struct ada_opname_map
{
  const char *encoded;
  const char *decoded;
  enum exp_opcode op;
};

/* Table of Ada operators in encoded and decoded forms.  */
/* Defined in ada-lang.c */
extern const struct ada_opname_map ada_opname_table[];

/* Denotes a type of renaming symbol (see ada_parse_renaming).  */
enum ada_renaming_category
  {
    /* Indicates a symbol that does not encode a renaming.  */
    ADA_NOT_RENAMING,

    /* For symbols declared
         Foo : TYPE renamed OBJECT;  */
    ADA_OBJECT_RENAMING,

    /* For symbols declared
         Foo : exception renames EXCEPTION;  */
    ADA_EXCEPTION_RENAMING,
    /* For packages declared
          package Foo renames PACKAGE; */
    ADA_PACKAGE_RENAMING,
    /* For subprograms declared
          SUBPROGRAM_SPEC renames SUBPROGRAM;
       (Currently not used).  */
    ADA_SUBPROGRAM_RENAMING
  };

/* The different types of catchpoints that we introduced for catching
   Ada exceptions.  */

enum ada_exception_catchpoint_kind
{
  ada_catch_exception,
  ada_catch_exception_unhandled,
  ada_catch_assert
};

/* Ada task structures.  */

struct ada_task_info
{
  /* The PTID of the thread that this task runs on.  This ptid is computed
     in a target-dependent way from the associated Task Control Block.  */
  ptid_t ptid;

  /* The ID of the task.  */
  CORE_ADDR task_id;

  /* The name of the task.  */
  char name[257];

  /* The current state of the task.  */
  int state;

  /* The priority associated to the task.  */
  int priority;

  /* If non-zero, the task ID of the parent task.  */
  CORE_ADDR parent;

  /* If the task is waiting on a task entry, this field contains
     the ID of the other task.  Zero otherwise.  */
  CORE_ADDR called_task;

  /* If the task is accepting a rendezvous with another task, this field
     contains the ID of the calling task.  Zero otherwise.  */
  CORE_ADDR caller_task;
};

/* Assuming V points to an array of S objects,  make sure that it contains at
   least M objects, updating V and S as necessary.  */

#define GROW_VECT(v, s, m)                                    \
   if ((s) < (m)) (v) = (char *) grow_vect (v, &(s), m, sizeof *(v));

extern void *grow_vect (void *, size_t *, size_t, int);

extern void ada_ensure_varsize_limit (const struct type *type);

extern int ada_get_field_index (const struct type *type,
                                const char *field_name,
                                int maybe_missing);

extern int ada_parse (struct parser_state *);    /* Defined in ada-exp.y */

extern void ada_yyerror (const char *); /* Defined in ada-exp.y */

                        /* Defined in ada-typeprint.c */
extern void ada_print_type (struct type *, const char *, struct ui_file *, int,
                            int, const struct type_print_options *);

extern void ada_print_typedef (struct type *type, struct symbol *new_symbol,
			       struct ui_file *stream);

extern void ada_val_print (struct type *, int, CORE_ADDR,
			   struct ui_file *, int,
			   struct value *,
			   const struct value_print_options *);

extern void ada_value_print (struct value *, struct ui_file *,
			     const struct value_print_options *);

                                /* Defined in ada-lang.c */

extern void ada_emit_char (int, struct type *, struct ui_file *, int, int);

extern void ada_printchar (int, struct type *, struct ui_file *);

extern void ada_printstr (struct ui_file *, struct type *, const gdb_byte *,
			  unsigned int, const char *, int,
			  const struct value_print_options *);

struct value *ada_convert_actual (struct value *actual,
                                  struct type *formal_type0);

extern struct value *ada_value_subscript (struct value *, int,
                                          struct value **);

extern void ada_fixup_array_indexes_type (struct type *index_desc_type);

extern struct type *ada_array_element_type (struct type *, int);

extern int ada_array_arity (struct type *);

struct type *ada_type_of_array (struct value *, int);

extern struct value *ada_coerce_to_simple_array_ptr (struct value *);

struct value *ada_coerce_to_simple_array (struct value *);

extern int ada_is_simple_array_type (struct type *);

extern int ada_is_array_descriptor_type (struct type *);

extern int ada_is_bogus_array_descriptor (struct type *);

extern LONGEST ada_discrete_type_low_bound (struct type *);

extern LONGEST ada_discrete_type_high_bound (struct type *);

extern struct value *ada_get_decoded_value (struct value *value);

extern struct type *ada_get_decoded_type (struct type *type);

extern const char *ada_decode_symbol (const struct general_symbol_info *);

extern const char *ada_decode (const char*);

extern enum language ada_update_initial_language (enum language);

extern int ada_lookup_symbol_list (const char *, const struct block *,
                                   domain_enum, struct block_symbol**);

extern char *ada_fold_name (const char *);

extern struct block_symbol ada_lookup_symbol (const char *,
					      const struct block *,
					      domain_enum, int *);

extern void ada_lookup_encoded_symbol
  (const char *name, const struct block *block, domain_enum domain,
   struct block_symbol *symbol_info);

extern struct bound_minimal_symbol ada_lookup_simple_minsym (const char *);

extern void ada_fill_in_ada_prototype (struct symbol *);

extern int user_select_syms (struct block_symbol *, int, int);

extern int get_selections (int *, int, int, int, const char *);

extern int ada_scan_number (const char *, int, LONGEST *, int *);

extern struct type *ada_parent_type (struct type *);

extern int ada_is_ignored_field (struct type *, int);

extern int ada_is_constrained_packed_array_type (struct type *);

extern struct value *ada_value_primitive_packed_val (struct value *,
						     const gdb_byte *,
                                                     long, int, int,
                                                     struct type *);

extern struct type *ada_coerce_to_simple_array_type (struct type *);

extern int ada_is_character_type (struct type *);

extern int ada_is_string_type (struct type *);

extern int ada_is_tagged_type (struct type *, int);

extern int ada_is_tag_type (struct type *);

extern struct type *ada_tag_type (struct value *);

extern struct value *ada_value_tag (struct value *);

extern const char *ada_tag_name (struct value *);

extern struct value *ada_tag_value_at_base_address (struct value *obj);

extern int ada_is_parent_field (struct type *, int);

extern int ada_is_wrapper_field (struct type *, int);

extern int ada_is_variant_part (struct type *, int);

extern struct type *ada_variant_discrim_type (struct type *, struct type *);

extern int ada_is_others_clause (struct type *, int);

extern int ada_in_variant (LONGEST, struct type *, int);

extern const char *ada_variant_discrim_name (struct type *);

extern struct value *ada_value_struct_elt (struct value *, const char *, int);

extern int ada_is_aligner_type (struct type *);

extern struct type *ada_aligned_type (struct type *);

extern const gdb_byte *ada_aligned_value_addr (struct type *,
					       const gdb_byte *);

extern const char *ada_attribute_name (enum exp_opcode);

extern int ada_is_fixed_point_type (struct type *);

extern int ada_is_system_address_type (struct type *);

extern DOUBLEST ada_delta (struct type *);

extern DOUBLEST ada_fixed_to_float (struct type *, LONGEST);

extern LONGEST ada_float_to_fixed (struct type *, DOUBLEST);

extern struct type *ada_system_address_type (void);

extern int ada_which_variant_applies (struct type *, struct type *,
				      const gdb_byte *);

extern struct type *ada_to_fixed_type (struct type *, const gdb_byte *,
				       CORE_ADDR, struct value *,
                                       int check_tag);

extern struct value *ada_to_fixed_value (struct value *val);

extern struct type *ada_template_to_fixed_record_type_1 (struct type *type,
							 const gdb_byte *valaddr,
							 CORE_ADDR address,
							 struct value *dval0,
							 int keep_dynamic_fields);

extern int ada_name_prefix_len (const char *);

extern const char *ada_type_name (struct type *);

extern struct type *ada_find_parallel_type (struct type *,
                                            const char *suffix);

extern LONGEST get_int_var_value (char *, int *);

extern struct symbol *ada_find_renaming_symbol (struct symbol *name_sym,
                                                const struct block *block);

extern int ada_prefer_type (struct type *, struct type *);

extern struct type *ada_get_base_type (struct type *);

extern struct type *ada_check_typedef (struct type *);

extern char *ada_encode (const char *);

extern const char *ada_enum_name (const char *);

extern int ada_is_modular_type (struct type *);

extern ULONGEST ada_modulus (struct type *);

extern struct value *ada_value_ind (struct value *);

extern void ada_print_scalar (struct type *, LONGEST, struct ui_file *);

extern int ada_is_range_type_name (const char *);

extern enum ada_renaming_category ada_parse_renaming (struct symbol *,
						      const char **,
						      int *, const char **);

extern void ada_find_printable_frame (struct frame_info *fi);

extern char *ada_breakpoint_rewrite (char *, int *);

extern char *ada_main_name (void);

extern std::string ada_name_for_lookup (const char *name);

extern void create_ada_exception_catchpoint
  (struct gdbarch *gdbarch, enum ada_exception_catchpoint_kind ex_kind,
   char *excep_string, char *cond_string, int tempflag, int disabled,
   int from_tty);

/* Some information about a given Ada exception.  */

typedef struct ada_exc_info
{
  /* The name of the exception.  */
  const char *name;

  /* The address of the symbol corresponding to that exception.  */
  CORE_ADDR addr;
} ada_exc_info;

DEF_VEC_O(ada_exc_info);

extern VEC(ada_exc_info) *ada_exceptions_list (const char *regexp);

/* Tasking-related: ada-tasks.c */

extern int valid_task_id (int);

extern int ada_get_task_number (ptid_t);

typedef void (ada_task_list_iterator_ftype) (struct ada_task_info *task);
extern void iterate_over_live_ada_tasks
  (ada_task_list_iterator_ftype *iterator);

extern int ada_build_task_list (void);

extern void print_ada_task_info (struct ui_out *uiout,
				 char *taskno_str,
				 struct inferior *inf);

#endif
