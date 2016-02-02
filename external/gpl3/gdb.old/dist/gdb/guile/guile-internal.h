/* Internal header for GDB/Scheme code.

   Copyright (C) 2014-2015 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#ifndef GDB_GUILE_INTERNAL_H
#define GDB_GUILE_INTERNAL_H

#include "hashtab.h"
#include "extension-priv.h"
#include "symtab.h"
#include "libguile.h"

struct block;
struct frame_info;
struct objfile;
struct symbol;

/* A function to pass to the safe-call routines to ignore things like
   memory errors.  */
typedef int excp_matcher_func (SCM key);

/* Scheme variables to define during initialization.  */

typedef struct
{
  const char *name;
  SCM value;
  const char *doc_string;
} scheme_variable;

/* End of scheme_variable table mark.  */

#define END_VARIABLES { NULL, SCM_BOOL_F, NULL }

/* Scheme functions to define during initialization.  */

typedef struct
{
  const char *name;
  int required;
  int optional;
  int rest;
  scm_t_subr func;
  const char *doc_string;
} scheme_function;

/* End of scheme_function table mark.  */

#define END_FUNCTIONS { NULL, 0, 0, 0, NULL, NULL }

/* Useful for defining a set of constants.  */

typedef struct
{
  const char *name;
  int value;
} scheme_integer_constant;

#define END_INTEGER_CONSTANTS { NULL, 0 }

/* Pass this instead of 0 to routines like SCM_ASSERT to indicate the value
   is not a function argument.  */
#define GDBSCM_ARG_NONE 0

/* Ensure new code doesn't accidentally try to use this.  */
#undef scm_make_smob_type
#define scm_make_smob_type USE_gdbscm_make_smob_type_INSTEAD

/* They brought over () == #f from lisp.
   Let's avoid that for now.  */
#undef scm_is_bool
#undef scm_is_false
#undef scm_is_true
#define scm_is_bool USE_gdbscm_is_bool_INSTEAD
#define scm_is_false USE_gdbscm_is_false_INSTEAD
#define scm_is_true USE_gdbscm_is_true_INSTEAD
#define gdbscm_is_bool(scm) \
  (scm_is_eq ((scm), SCM_BOOL_F) || scm_is_eq ((scm), SCM_BOOL_T))
#define gdbscm_is_false(scm) scm_is_eq ((scm), SCM_BOOL_F)
#define gdbscm_is_true(scm) (!gdbscm_is_false (scm))

#ifndef HAVE_SCM_NEW_SMOB

/* Guile <= 2.0.5 did not provide this function, so provide it here.  */

static inline SCM
scm_new_smob (scm_t_bits tc, scm_t_bits data)
{
  SCM_RETURN_NEWSMOB (tc, data);
}

#endif

/* Function name that is passed around in case an error needs to be reported.
   __func is in C99, but we provide a wrapper "just in case",
   and because FUNC_NAME is the canonical value used in guile sources.
   IWBN to use the Scheme version of the name (e.g. foo-bar vs foo_bar),
   but let's KISS for now.  */
#define FUNC_NAME __func__

extern const char gdbscm_module_name[];
extern const char gdbscm_init_module_name[];

extern int gdb_scheme_initialized;

extern int gdbscm_guile_major_version;
extern int gdbscm_guile_minor_version;
extern int gdbscm_guile_micro_version;

extern const char gdbscm_print_excp_none[];
extern const char gdbscm_print_excp_full[];
extern const char gdbscm_print_excp_message[];
extern const char *gdbscm_print_excp;

extern SCM gdbscm_documentation_symbol;
extern SCM gdbscm_invalid_object_error_symbol;

extern SCM gdbscm_map_string;
extern SCM gdbscm_array_string;
extern SCM gdbscm_string_string;

/* scm-utils.c */

extern void gdbscm_define_variables (const scheme_variable *, int public);

extern void gdbscm_define_functions (const scheme_function *, int public);

extern void gdbscm_define_integer_constants (const scheme_integer_constant *,
					     int public);

extern void gdbscm_printf (SCM port, const char *format, ...);

extern void gdbscm_debug_display (SCM obj);

extern void gdbscm_debug_write (SCM obj);

extern void gdbscm_parse_function_args (const char *function_name,
					int beginning_arg_pos,
					const SCM *keywords,
					const char *format, ...);

extern SCM gdbscm_scm_from_longest (LONGEST l);

extern LONGEST gdbscm_scm_to_longest (SCM l);

extern SCM gdbscm_scm_from_ulongest (ULONGEST l);

extern ULONGEST gdbscm_scm_to_ulongest (SCM u);

extern void gdbscm_dynwind_xfree (void *ptr);

extern int gdbscm_is_procedure (SCM proc);

extern char *gdbscm_gc_xstrdup (const char *);

extern const char * const *gdbscm_gc_dup_argv (char **argv);

extern int gdbscm_guile_version_is_at_least (int major, int minor, int micro);

/* GDB smobs, from scm-gsmob.c */

/* All gdb smobs must contain one of the following as the first member:
   gdb_smob, chained_gdb_smob, or eqable_gdb_smob.

   Chained GDB smobs should have chained_gdb_smob as their first member.  The
   next,prev members of chained_gdb_smob allow for chaining gsmobs together so
   that, for example, when an objfile is deleted we can clean up all smobs that
   reference it.

   Eq-able GDB smobs should have eqable_gdb_smob as their first member.  The
   containing_scm member of eqable_gdb_smob allows for returning the same gsmob
   instead of creating a new one, allowing them to be eq?-able.

   All other smobs should have gdb_smob as their first member.
   FIXME: dje/2014-05-26: gdb_smob was useful during early development as a
   "baseclass" for all gdb smobs.  If it's still unused by gdb 8.0 delete it.

   IMPORTANT: chained_gdb_smob and eqable_gdb-smob are "subclasses" of
   gdb_smob.  The layout of chained_gdb_smob,eqable_gdb_smob must match
   gdb_smob as if it is a subclass.  To that end we use macro GDB_SMOB_HEAD
   to ensure this.  */

#define GDB_SMOB_HEAD \
  int empty_base_class;

typedef struct
{
  GDB_SMOB_HEAD
} gdb_smob;

typedef struct _chained_gdb_smob
{
  GDB_SMOB_HEAD

  struct _chained_gdb_smob *prev;
  struct _chained_gdb_smob *next;
} chained_gdb_smob;

typedef struct _eqable_gdb_smob
{
  GDB_SMOB_HEAD

  /* The object we are contained in.
     This can be used for several purposes.
     This is used by the eq? machinery:  We need to be able to see if we have
     already created an object for a symbol, and if so use that SCM.
     This may also be used to protect the smob from GC if there is
     a reference to this smob from outside of GC space (i.e., from gdb).
     This can also be used in place of chained_gdb_smob where we need to
     keep track of objfile referencing objects.  When the objfile is deleted
     we need to invalidate the objects: we can do that using the same hashtab
     used to record the smob for eq-ability.  */
  SCM containing_scm;
} eqable_gdb_smob;

#undef GDB_SMOB_HEAD

struct objfile;
struct objfile_data;

/* A predicate that returns non-zero if an object is a particular kind
   of gsmob.  */
typedef int (gsmob_pred_func) (SCM);

extern scm_t_bits gdbscm_make_smob_type (const char *name, size_t size);

extern void gdbscm_init_gsmob (gdb_smob *base);

extern void gdbscm_init_chained_gsmob (chained_gdb_smob *base);

extern void gdbscm_init_eqable_gsmob (eqable_gdb_smob *base,
				      SCM containing_scm);

extern void gdbscm_add_objfile_ref (struct objfile *objfile,
				    const struct objfile_data *data_key,
				    chained_gdb_smob *g_smob);

extern void gdbscm_remove_objfile_ref (struct objfile *objfile,
				       const struct objfile_data *data_key,
				       chained_gdb_smob *g_smob);

extern htab_t gdbscm_create_eqable_gsmob_ptr_map (htab_hash hash_fn,
						  htab_eq eq_fn);

extern eqable_gdb_smob **gdbscm_find_eqable_gsmob_ptr_slot
  (htab_t htab, eqable_gdb_smob *base);

extern void gdbscm_fill_eqable_gsmob_ptr_slot (eqable_gdb_smob **slot,
					       eqable_gdb_smob *base);

extern void gdbscm_clear_eqable_gsmob_ptr_slot (htab_t htab,
						eqable_gdb_smob *base);

/* Exceptions and calling out to Guile.  */

/* scm-exception.c */

extern SCM gdbscm_make_exception (SCM tag, SCM args);

extern int gdbscm_is_exception (SCM scm);

extern SCM gdbscm_exception_key (SCM excp);

extern SCM gdbscm_exception_args (SCM excp);

extern SCM gdbscm_make_exception_with_stack (SCM key, SCM args, SCM stack);

extern SCM gdbscm_make_error_scm (SCM key, SCM subr, SCM message,
				  SCM args, SCM data);

extern SCM gdbscm_make_error (SCM key, const char *subr, const char *message,
			      SCM args, SCM data);

extern SCM gdbscm_make_type_error (const char *subr, int arg_pos,
				   SCM bad_value, const char *expected_type);

extern SCM gdbscm_make_invalid_object_error (const char *subr, int arg_pos,
					     SCM bad_value, const char *error);

extern void gdbscm_invalid_object_error (const char *subr, int arg_pos,
					 SCM bad_value, const char *error)
   ATTRIBUTE_NORETURN;

extern SCM gdbscm_make_out_of_range_error (const char *subr, int arg_pos,
					   SCM bad_value, const char *error);

extern void gdbscm_out_of_range_error (const char *subr, int arg_pos,
				       SCM bad_value, const char *error)
   ATTRIBUTE_NORETURN;

extern SCM gdbscm_make_misc_error (const char *subr, int arg_pos,
				   SCM bad_value, const char *error);

extern void gdbscm_misc_error (const char *subr, int arg_pos,
			       SCM bad_value, const char *error)
   ATTRIBUTE_NORETURN;

extern void gdbscm_throw (SCM exception) ATTRIBUTE_NORETURN;

extern SCM gdbscm_scm_from_gdb_exception (struct gdb_exception exception);

extern void gdbscm_throw_gdb_exception (struct gdb_exception exception)
  ATTRIBUTE_NORETURN;

extern void gdbscm_print_exception_with_stack (SCM port, SCM stack,
					       SCM key, SCM args);

extern void gdbscm_print_gdb_exception (SCM port, SCM exception);

extern char *gdbscm_exception_message_to_string (SCM exception);

extern excp_matcher_func gdbscm_memory_error_p;

extern excp_matcher_func gdbscm_user_error_p;

extern SCM gdbscm_make_memory_error (const char *subr, const char *msg,
				     SCM args);

extern void gdbscm_memory_error (const char *subr, const char *msg, SCM args)
  ATTRIBUTE_NORETURN;

/* scm-safe-call.c */

extern void *gdbscm_with_guile (void *(*func) (void *), void *data);

extern SCM gdbscm_call_guile (SCM (*func) (void *), void *data,
			      excp_matcher_func *ok_excps);

extern SCM gdbscm_safe_call_0 (SCM proc, excp_matcher_func *ok_excps);

extern SCM gdbscm_safe_call_1 (SCM proc, SCM arg0,
			       excp_matcher_func *ok_excps);

extern SCM gdbscm_safe_call_2 (SCM proc, SCM arg0, SCM arg1,
			       excp_matcher_func *ok_excps);

extern SCM gdbscm_safe_call_3 (SCM proc, SCM arg0, SCM arg1, SCM arg2,
			       excp_matcher_func *ok_excps);

extern SCM gdbscm_safe_call_4 (SCM proc, SCM arg0, SCM arg1, SCM arg2,
			       SCM arg3,
			       excp_matcher_func *ok_excps);

extern SCM gdbscm_safe_apply_1 (SCM proc, SCM arg0, SCM args,
				excp_matcher_func *ok_excps);

extern SCM gdbscm_unsafe_call_1 (SCM proc, SCM arg0);

extern char *gdbscm_safe_eval_string (const char *string, int display_result);

extern char *gdbscm_safe_source_script (const char *filename);

extern void gdbscm_enter_repl (void);

/* Interface to various GDB objects, in alphabetical order.  */

/* scm-arch.c */

typedef struct _arch_smob arch_smob;

extern struct gdbarch *arscm_get_gdbarch (arch_smob *a_smob);

extern arch_smob *arscm_get_arch_smob_arg_unsafe (SCM arch_scm, int arg_pos,
						  const char *func_name);

extern SCM arscm_scm_from_arch (struct gdbarch *gdbarch);

/* scm-block.c */

extern SCM bkscm_scm_from_block (const struct block *block,
				 struct objfile *objfile);

extern const struct block *bkscm_scm_to_block
  (SCM block_scm, int arg_pos, const char *func_name, SCM *excp);

/* scm-cmd.c */

extern char *gdbscm_parse_command_name (const char *name,
					const char *func_name, int arg_pos,
					struct cmd_list_element ***base_list,
					struct cmd_list_element **start_list);

extern int gdbscm_valid_command_class_p (int command_class);

extern char *gdbscm_canonicalize_command_name (const char *name,
					       int want_trailing_space);

/* scm-frame.c */

typedef struct _frame_smob frame_smob;

extern int frscm_is_frame (SCM scm);

extern frame_smob *frscm_get_frame_smob_arg_unsafe (SCM frame_scm, int arg_pos,
						    const char *func_name);

extern struct frame_info *frscm_frame_smob_to_frame (frame_smob *);

/* scm-iterator.c */

typedef struct _iterator_smob iterator_smob;

extern SCM itscm_iterator_smob_object (iterator_smob *i_smob);

extern SCM itscm_iterator_smob_progress (iterator_smob *i_smob);

extern void itscm_set_iterator_smob_progress_x (iterator_smob *i_smob,
						SCM progress);

extern const char *itscm_iterator_smob_name (void);

extern SCM gdbscm_make_iterator (SCM object, SCM progress, SCM next);

extern int itscm_is_iterator (SCM scm);

extern SCM gdbscm_end_of_iteration (void);

extern int itscm_is_end_of_iteration (SCM obj);

extern SCM itscm_safe_call_next_x (SCM iter, excp_matcher_func *ok_excps);

extern SCM itscm_get_iterator_arg_unsafe (SCM self, int arg_pos,
					  const char *func_name);

/* scm-lazy-string.c */

extern int lsscm_is_lazy_string (SCM scm);

extern SCM lsscm_make_lazy_string (CORE_ADDR address, int length,
				   const char *encoding, struct type *type);

extern struct value *lsscm_safe_lazy_string_to_value (SCM string,
						      int arg_pos,
						      const char *func_name,
						      SCM *except_scmp);

extern void lsscm_val_print_lazy_string
  (SCM string, struct ui_file *stream,
   const struct value_print_options *options);

/* scm-objfile.c */

typedef struct _objfile_smob objfile_smob;

extern SCM ofscm_objfile_smob_pretty_printers (objfile_smob *o_smob);

extern objfile_smob *ofscm_objfile_smob_from_objfile (struct objfile *objfile);

extern SCM ofscm_scm_from_objfile (struct objfile *objfile);

/* scm-progspace.c */

typedef struct _pspace_smob pspace_smob;

extern SCM psscm_pspace_smob_pretty_printers (const pspace_smob *);

extern pspace_smob *psscm_pspace_smob_from_pspace (struct program_space *);

extern SCM psscm_scm_from_pspace (struct program_space *);

/* scm-string.c */

extern int gdbscm_scm_string_to_int (SCM string);

extern char *gdbscm_scm_to_c_string (SCM string);

extern SCM gdbscm_scm_from_c_string (const char *string);

extern SCM gdbscm_scm_from_printf (const char *format, ...);

extern char *gdbscm_scm_to_string (SCM string, size_t *lenp,
				   const char *charset,
				   int strict, SCM *except_scmp);

extern SCM gdbscm_scm_from_string (const char *string, size_t len,
				   const char *charset, int strict);

extern char *gdbscm_scm_to_host_string (SCM string, size_t *lenp, SCM *except);

extern SCM gdbscm_scm_from_host_string (const char *string, size_t len);

/* scm-symbol.c */

extern int syscm_is_symbol (SCM scm);

extern SCM syscm_scm_from_symbol (struct symbol *symbol);

extern struct symbol *syscm_get_valid_symbol_arg_unsafe
  (SCM self, int arg_pos, const char *func_name);

/* scm-symtab.c */

extern SCM stscm_scm_from_symtab (struct symtab *symtab);

extern SCM stscm_scm_from_sal (struct symtab_and_line sal);

/* scm-type.c */

typedef struct _type_smob type_smob;

extern int tyscm_is_type (SCM scm);

extern SCM tyscm_scm_from_type (struct type *type);

extern type_smob *tyscm_get_type_smob_arg_unsafe (SCM type_scm, int arg_pos,
						  const char *func_name);

extern struct type *tyscm_type_smob_type (type_smob *t_smob);

extern SCM tyscm_scm_from_field (SCM type_scm, int field_num);

/* scm-value.c */

extern struct value *vlscm_scm_to_value (SCM scm);

extern int vlscm_is_value (SCM scm);

extern SCM vlscm_scm_from_value (struct value *value);

extern SCM vlscm_scm_from_value_unsafe (struct value *value);

extern struct value *vlscm_convert_typed_value_from_scheme
  (const char *func_name, int obj_arg_pos, SCM obj,
   int type_arg_pos, SCM type_scm, struct type *type, SCM *except_scmp,
   struct gdbarch *gdbarch, const struct language_defn *language);

extern struct value *vlscm_convert_value_from_scheme
  (const char *func_name, int obj_arg_pos, SCM obj, SCM *except_scmp,
   struct gdbarch *gdbarch, const struct language_defn *language);

/* stript_lang methods */

extern objfile_script_sourcer_func gdbscm_source_objfile_script;

extern int gdbscm_auto_load_enabled (const struct extension_language_defn *);

extern void gdbscm_preserve_values
  (const struct extension_language_defn *,
   struct objfile *, htab_t copied_types);

extern enum ext_lang_rc gdbscm_apply_val_pretty_printer
  (const struct extension_language_defn *,
   struct type *type, const gdb_byte *valaddr,
   int embedded_offset, CORE_ADDR address,
   struct ui_file *stream, int recurse,
   const struct value *val,
   const struct value_print_options *options,
   const struct language_defn *language);

extern int gdbscm_breakpoint_has_cond (const struct extension_language_defn *,
				       struct breakpoint *b);

extern enum ext_lang_bp_stop gdbscm_breakpoint_cond_says_stop
  (const struct extension_language_defn *, struct breakpoint *b);

/* Initializers for each piece of Scheme support, in alphabetical order.  */

extern void gdbscm_initialize_arches (void);
extern void gdbscm_initialize_auto_load (void);
extern void gdbscm_initialize_blocks (void);
extern void gdbscm_initialize_breakpoints (void);
extern void gdbscm_initialize_commands (void);
extern void gdbscm_initialize_disasm (void);
extern void gdbscm_initialize_exceptions (void);
extern void gdbscm_initialize_frames (void);
extern void gdbscm_initialize_iterators (void);
extern void gdbscm_initialize_lazy_strings (void);
extern void gdbscm_initialize_math (void);
extern void gdbscm_initialize_objfiles (void);
extern void gdbscm_initialize_pretty_printers (void);
extern void gdbscm_initialize_parameters (void);
extern void gdbscm_initialize_ports (void);
extern void gdbscm_initialize_pspaces (void);
extern void gdbscm_initialize_smobs (void);
extern void gdbscm_initialize_strings (void);
extern void gdbscm_initialize_symbols (void);
extern void gdbscm_initialize_symtabs (void);
extern void gdbscm_initialize_types (void);
extern void gdbscm_initialize_values (void);

/* Use these after a TRY_CATCH to throw the appropriate Scheme exception
   if a GDB error occurred.  */

#define GDBSCM_HANDLE_GDB_EXCEPTION(exception)		\
  do {							\
    if (exception.reason < 0)				\
      {							\
	gdbscm_throw_gdb_exception (exception);		\
        /*NOTREACHED */					\
      }							\
  } while (0)

/* If cleanups are establish outside the TRY_CATCH block, use this version.  */

#define GDBSCM_HANDLE_GDB_EXCEPTION_WITH_CLEANUPS(exception, cleanups)	\
  do {									\
    if (exception.reason < 0)						\
      {									\
	do_cleanups (cleanups);						\
	gdbscm_throw_gdb_exception (exception);				\
        /*NOTREACHED */							\
      }									\
  } while (0)

#endif /* GDB_GUILE_INTERNAL_H */
