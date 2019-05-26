/* Source-language-related definitions for GDB.

   Copyright (C) 1991-2019 Free Software Foundation, Inc.

   Contributed by the Department of Computer Science at the State University
   of New York at Buffalo.

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

#if !defined (LANGUAGE_H)
#define LANGUAGE_H 1

#include "symtab.h"
#include "common/function-view.h"
#include "expression.h"

/* Forward decls for prototypes.  */
struct value;
struct objfile;
struct frame_info;
struct ui_file;
struct value_print_options;
struct type_print_options;
struct lang_varobj_ops;
struct parser_state;
class compile_instance;
struct completion_match_for_lcd;

#define MAX_FORTRAN_DIMS  7	/* Maximum number of F77 array dims.  */

/* range_mode ==
   range_mode_auto:   range_check set automatically to default of language.
   range_mode_manual: range_check set manually by user.  */

extern enum range_mode
  {
    range_mode_auto, range_mode_manual
  }
range_mode;

/* range_check ==
   range_check_on:    Ranges are checked in GDB expressions, producing errors.
   range_check_warn:  Ranges are checked, producing warnings.
   range_check_off:   Ranges are not checked in GDB expressions.  */

extern enum range_check
  {
    range_check_off, range_check_warn, range_check_on
  }
range_check;

/* case_mode ==
   case_mode_auto:   case_sensitivity set upon selection of scope.
   case_mode_manual: case_sensitivity set only by user.  */

extern enum case_mode
  {
    case_mode_auto, case_mode_manual
  }
case_mode;

/* array_ordering ==
   array_row_major:     Arrays are in row major order.
   array_column_major:  Arrays are in column major order.  */

extern enum array_ordering
  {
    array_row_major, array_column_major
  } 
array_ordering;


/* case_sensitivity ==
   case_sensitive_on:   Case sensitivity in name matching is used.
   case_sensitive_off:  Case sensitivity in name matching is not used.  */

extern enum case_sensitivity
  {
    case_sensitive_on, case_sensitive_off
  }
case_sensitivity;


/* macro_expansion ==
   macro_expansion_no:  No macro expansion is available.
   macro_expansion_c:   C-like macro expansion is available.  */

enum macro_expansion
  {
    macro_expansion_no, macro_expansion_c
  };


/* Per architecture (OS/ABI) language information.  */

struct language_arch_info
{
  /* Its primitive types.  This is a vector ended by a NULL pointer.
     These types can be specified by name in parsing types in
     expressions, regardless of whether the program being debugged
     actually defines such a type.  */
  struct type **primitive_type_vector;

  /* Symbol wrappers around primitive_type_vector, so that the symbol lookup
     machinery can return them.  */
  struct symbol **primitive_type_symbols;

  /* Type of elements of strings.  */
  struct type *string_char_type;

  /* Symbol name of type to use as boolean type, if defined.  */
  const char *bool_type_symbol;
  /* Otherwise, this is the default boolean builtin type.  */
  struct type *bool_type_default;
};

/* Structure tying together assorted information about a language.  */

struct language_defn
  {
    /* Name of the language.  */

    const char *la_name;

    /* Natural or official name of the language.  */

    const char *la_natural_name;

    /* its symtab language-enum (defs.h).  */

    enum language la_language;

    /* Default range checking.  */

    enum range_check la_range_check;

    /* Default case sensitivity.  */
    enum case_sensitivity la_case_sensitivity;

    /* Multi-dimensional array ordering.  */
    enum array_ordering la_array_ordering;

    /* Style of macro expansion, if any, supported by this language.  */
    enum macro_expansion la_macro_expansion;

    /* A NULL-terminated array of file extensions for this language.
       The extension must include the ".", like ".c".  If this
       language doesn't need to provide any filename extensions, this
       may be NULL.  */

    const char *const *la_filename_extensions;

    /* Definitions related to expression printing, prefixifying, and
       dumping.  */

    const struct exp_descriptor *la_exp_desc;

    /* Parser function.  */

    int (*la_parser) (struct parser_state *);

    /* Given an expression *EXPP created by prefixifying the result of
       la_parser, perform any remaining processing necessary to complete
       its translation.  *EXPP may change; la_post_parser is responsible 
       for releasing its previous contents, if necessary.  If 
       VOID_CONTEXT_P, then no value is expected from the expression.  */

    void (*la_post_parser) (expression_up *expp, int void_context_p);

    void (*la_printchar) (int ch, struct type *chtype,
			  struct ui_file * stream);

    void (*la_printstr) (struct ui_file * stream, struct type *elttype,
			 const gdb_byte *string, unsigned int length,
			 const char *encoding, int force_ellipses,
			 const struct value_print_options *);

    void (*la_emitchar) (int ch, struct type *chtype,
			 struct ui_file * stream, int quoter);

    /* Print a type using syntax appropriate for this language.  */

    void (*la_print_type) (struct type *, const char *, struct ui_file *, int,
			   int, const struct type_print_options *);

    /* Print a typedef using syntax appropriate for this language.
       TYPE is the underlying type.  NEW_SYMBOL is the symbol naming
       the type.  STREAM is the output stream on which to print.  */

    void (*la_print_typedef) (struct type *type, struct symbol *new_symbol,
			      struct ui_file *stream);

    /* Print a value using syntax appropriate for this language.
       
       TYPE is the type of the sub-object to be printed.

       EMBEDDED_OFFSET is the offset into the outermost object of the
       sub-object represented by TYPE.  This is the object which this
       call should print.  Note that the enclosing type is not
       available.

       ADDRESS is the address in the inferior of the enclosing object.

       STREAM is the stream on which the value is to be printed.

       RECURSE is the recursion depth.  It is zero-based.

       OPTIONS are the formatting options to be used when
       printing.  */

    void (*la_val_print) (struct type *type,
			  int embedded_offset, CORE_ADDR address,
			  struct ui_file *stream, int recurse,
			  struct value *val,
			  const struct value_print_options *options);

    /* Print a top-level value using syntax appropriate for this language.  */

    void (*la_value_print) (struct value *, struct ui_file *,
			    const struct value_print_options *);

    /* Given a symbol VAR, the corresponding block VAR_BLOCK (if any) and a
       stack frame id FRAME, read the value of the variable and return (pointer
       to a) struct value containing the value.

       VAR_BLOCK is needed if there's a possibility for VAR to be outside
       FRAME.  This is what happens if FRAME correspond to a nested function
       and VAR is defined in the outer function.  If callers know that VAR is
       located in FRAME or is global/static, NULL can be passed as VAR_BLOCK.

       Throw an error if the variable cannot be found.  */

    struct value *(*la_read_var_value) (struct symbol *var,
					const struct block *var_block,
					struct frame_info *frame);

    /* PC is possibly an unknown languages trampoline.
       If that PC falls in a trampoline belonging to this language,
       return the address of the first pc in the real function, or 0
       if it isn't a language tramp for this language.  */
    CORE_ADDR (*skip_trampoline) (struct frame_info *, CORE_ADDR);

    /* Now come some hooks for lookup_symbol.  */

    /* If this is non-NULL, specifies the name that of the implicit
       local variable that refers to the current object instance.  */

    const char *la_name_of_this;

    /* True if the symbols names should be stored in GDB's data structures
       for minimal/partial/full symbols using their linkage (aka mangled)
       form; false if the symbol names should be demangled first.

       Most languages implement symbol lookup by comparing the demangled
       names, in which case it is advantageous to store that information
       already demangled, and so would set this field to false.

       On the other hand, some languages have opted for doing symbol
       lookups by comparing mangled names instead, for reasons usually
       specific to the language.  Those languages should set this field
       to true.

       And finally, other languages such as C or Asm do not have
       the concept of mangled vs demangled name, so those languages
       should set this field to true as well, to prevent any accidental
       demangling through an unrelated language's demangler.  */

    const bool la_store_sym_names_in_linkage_form_p;

    /* This is a function that lookup_symbol will call when it gets to
       the part of symbol lookup where C looks up static and global
       variables.  */

    struct block_symbol (*la_lookup_symbol_nonlocal)
      (const struct language_defn *,
       const char *,
       const struct block *,
       const domain_enum);

    /* Find the definition of the type with the given name.  */
    struct type *(*la_lookup_transparent_type) (const char *);

    /* Return demangled language symbol, or NULL.  */
    char *(*la_demangle) (const char *mangled, int options);

    /* Demangle a symbol according to this language's rules.  Unlike
       la_demangle, this does not take any options.

       *DEMANGLED will be set by this function.
       
       If this function returns 0, then *DEMANGLED must always be set
       to NULL.

       If this function returns 1, the implementation may set this to
       a xmalloc'd string holding the demangled form.  However, it is
       not required to.  The string, if any, is owned by the caller.

       The resulting string should be of the form that will be
       installed into a symbol.  */
    int (*la_sniff_from_mangled_name) (const char *mangled, char **demangled);

    /* Return class name of a mangled method name or NULL.  */
    char *(*la_class_name_from_physname) (const char *physname);

    /* Table for printing expressions.  */

    const struct op_print *la_op_print_tab;

    /* Zero if the language has first-class arrays.  True if there are no
       array values, and array objects decay to pointers, as in C.  */

    char c_style_arrays;

    /* Index to use for extracting the first element of a string.  */
    char string_lower_bound;

    /* The list of characters forming word boundaries.  */
    const char *(*la_word_break_characters) (void);

    /* Add to the completion tracker all symbols which are possible
       completions for TEXT.  WORD is the entire command on which the
       completion is being made.  If CODE is TYPE_CODE_UNDEF, then all
       symbols should be examined; otherwise, only STRUCT_DOMAIN
       symbols whose type has a code of CODE should be matched.  */
    void (*la_collect_symbol_completion_matches)
      (completion_tracker &tracker,
       complete_symbol_mode mode,
       symbol_name_match_type match_type,
       const char *text,
       const char *word,
       enum type_code code);

    /* The per-architecture (OS/ABI) language information.  */
    void (*la_language_arch_info) (struct gdbarch *,
				   struct language_arch_info *);

    /* Print the index of an element of an array.  */
    void (*la_print_array_index) (struct value *index_value,
                                  struct ui_file *stream,
                                  const struct value_print_options *options);

    /* Return non-zero if TYPE should be passed (and returned) by
       reference at the language level.  */
    int (*la_pass_by_reference) (struct type *type);

    /* Obtain a string from the inferior, storing it in a newly allocated
       buffer in BUFFER, which should be freed by the caller.  If the
       in- and out-parameter *LENGTH is specified at -1, the string is
       read until a null character of the appropriate width is found -
       otherwise the string is read to the length of characters specified.
       On completion, *LENGTH will hold the size of the string in characters.
       If a *LENGTH of -1 was specified it will count only actual
       characters, excluding any eventual terminating null character.
       Otherwise *LENGTH will include all characters - including any nulls.
       CHARSET will hold the encoding used in the string.  */
    void (*la_get_string) (struct value *value,
			   gdb::unique_xmalloc_ptr<gdb_byte> *buffer,
			   int *length, struct type **chartype,
			   const char **charset);

    /* Return an expression that can be used for a location
       watchpoint.  TYPE is a pointer type that points to the memory
       to watch, and ADDR is the address of the watched memory.  */
    gdb::unique_xmalloc_ptr<char> (*la_watch_location_expression)
         (struct type *type, CORE_ADDR addr);

    /* Return a pointer to the function that should be used to match a
       symbol name against LOOKUP_NAME, according to this language's
       rules.  The matching algorithm depends on LOOKUP_NAME.  For
       example, on Ada, the matching algorithm depends on the symbol
       name (wild/full/verbatim matching), and on whether we're doing
       a normal lookup or a completion match lookup.

       This field may be NULL, in which case
       default_symbol_name_matcher is used to perform the
       matching.  */
    symbol_name_matcher_ftype *(*la_get_symbol_name_matcher)
      (const lookup_name_info &);

    /* Find all symbols in the current program space matching NAME in
       DOMAIN, according to this language's rules.

       The search is done in BLOCK only.
       The caller is responsible for iterating up through superblocks
       if desired.

       For each one, call CALLBACK with the symbol.  If CALLBACK
       returns false, the iteration ends at that point.

       This field may not be NULL.  If the language does not need any
       special processing here, 'iterate_over_symbols' should be
       used as the definition.  */
    void (*la_iterate_over_symbols)
      (const struct block *block, const lookup_name_info &name,
       domain_enum domain,
       gdb::function_view<symbol_found_callback_ftype> callback);

    /* Hash the given symbol search name.  Use
       default_search_name_hash if no special treatment is
       required.  */
    unsigned int (*la_search_name_hash) (const char *name);

    /* Various operations on varobj.  */
    const struct lang_varobj_ops *la_varobj_ops;

    /* If this language allows compilation from the gdb command line,
       this method should be non-NULL.  When called it should return
       an instance of struct gcc_context appropriate to the language.
       When defined this method must never return NULL; instead it
       should throw an exception on failure.  The returned compiler
       instance is owned by its caller and must be deallocated by
       calling its 'destroy' method.  */

    compile_instance *(*la_get_compile_instance) (void);

    /* This method must be defined if 'la_get_gcc_context' is defined.
       If 'la_get_gcc_context' is not defined, then this method is
       ignored.

       This takes the user-supplied text and returns a new bit of code
       to compile.

       INST is the compiler instance being used.
       INPUT is the user's input text.
       GDBARCH is the architecture to use.
       EXPR_BLOCK is the block in which the expression is being
       parsed.
       EXPR_PC is the PC at which the expression is being parsed.  */

    std::string (*la_compute_program) (compile_instance *inst,
				       const char *input,
				       struct gdbarch *gdbarch,
				       const struct block *expr_block,
				       CORE_ADDR expr_pc);

    /* Add fields above this point, so the magic number is always last.  */
    /* Magic number for compat checking.  */

    long la_magic;

  };

#define LANG_MAGIC	910823L

/* Pointer to the language_defn for our current language.  This pointer
   always points to *some* valid struct; it can be used without checking
   it for validity.

   The current language affects expression parsing and evaluation
   (FIXME: it might be cleaner to make the evaluation-related stuff
   separate exp_opcodes for each different set of semantics.  We
   should at least think this through more clearly with respect to
   what happens if the language is changed between parsing and
   evaluation) and printing of things like types and arrays.  It does
   *not* affect symbol-reading-- each source file in a symbol-file has
   its own language and we should keep track of that regardless of the
   language when symbols are read.  If we want some manual setting for
   the language of symbol files (e.g. detecting when ".c" files are
   C++), it should be a separate setting from the current_language.  */

extern const struct language_defn *current_language;

/* Pointer to the language_defn expected by the user, e.g. the language
   of main(), or the language we last mentioned in a message, or C.  */

extern const struct language_defn *expected_language;

/* language_mode == 
   language_mode_auto:   current_language automatically set upon selection
   of scope (e.g. stack frame)
   language_mode_manual: current_language set only by user.  */

extern enum language_mode
  {
    language_mode_auto, language_mode_manual
  }
language_mode;

struct type *language_bool_type (const struct language_defn *l,
				 struct gdbarch *gdbarch);

struct type *language_string_char_type (const struct language_defn *l,
					struct gdbarch *gdbarch);

/* Look up type NAME in language L, and return its definition for architecture
   GDBARCH.  Returns NULL if not found.  */

struct type *language_lookup_primitive_type (const struct language_defn *l,
					     struct gdbarch *gdbarch,
					     const char *name);

/* Wrapper around language_lookup_primitive_type to return the
   corresponding symbol.  */

struct symbol *
  language_lookup_primitive_type_as_symbol (const struct language_defn *l,
					    struct gdbarch *gdbarch,
					    const char *name);


/* These macros define the behaviour of the expression 
   evaluator.  */

/* Should we range check values against the domain of their type?  */
#define RANGE_CHECK (range_check != range_check_off)

/* "cast" really means conversion.  */
/* FIXME -- should be a setting in language_defn.  */
#define CAST_IS_CONVERSION(LANG) ((LANG)->la_language == language_c  || \
				  (LANG)->la_language == language_cplus || \
				  (LANG)->la_language == language_objc)

extern void language_info (int);

extern enum language set_language (enum language);


/* This page contains functions that return things that are
   specific to languages.  Each of these functions is based on
   the current setting of working_lang, which the user sets
   with the "set language" command.  */

#define LA_PRINT_TYPE(type,varstring,stream,show,level,flags)		\
  (current_language->la_print_type(type,varstring,stream,show,level,flags))

#define LA_PRINT_TYPEDEF(type,new_symbol,stream) \
  (current_language->la_print_typedef(type,new_symbol,stream))

#define LA_VALUE_PRINT(val,stream,options) \
  (current_language->la_value_print(val,stream,options))

#define LA_PRINT_CHAR(ch, type, stream) \
  (current_language->la_printchar(ch, type, stream))
#define LA_PRINT_STRING(stream, elttype, string, length, encoding, force_ellipses, options) \
  (current_language->la_printstr(stream, elttype, string, length, \
				 encoding, force_ellipses,options))
#define LA_EMIT_CHAR(ch, type, stream, quoter) \
  (current_language->la_emitchar(ch, type, stream, quoter))
#define LA_GET_STRING(value, buffer, length, chartype, encoding) \
  (current_language->la_get_string(value, buffer, length, chartype, encoding))

#define LA_PRINT_ARRAY_INDEX(index_value, stream, options) \
  (current_language->la_print_array_index(index_value, stream, options))

#define LA_ITERATE_OVER_SYMBOLS(BLOCK, NAME, DOMAIN, CALLBACK) \
  (current_language->la_iterate_over_symbols (BLOCK, NAME, DOMAIN, CALLBACK))

/* Test a character to decide whether it can be printed in literal form
   or needs to be printed in another representation.  For example,
   in C the literal form of the character with octal value 141 is 'a'
   and the "other representation" is '\141'.  The "other representation"
   is program language dependent.  */

#define PRINT_LITERAL_FORM(c)		\
  ((c) >= 0x20				\
   && ((c) < 0x7F || (c) >= 0xA0)	\
   && (!sevenbit_strings || (c) < 0x80))

/* Type predicates */

extern int pointer_type (struct type *);

/* Error messages */

extern void range_error (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

/* Data:  Does this value represent "truth" to the current language?  */

extern int value_true (struct value *);

/* Misc:  The string representing a particular enum language.  */

extern enum language language_enum (const char *str);

extern const struct language_defn *language_def (enum language);

extern const char *language_str (enum language);

/* Check for a language-specific trampoline.  */

extern CORE_ADDR skip_language_trampoline (struct frame_info *, CORE_ADDR pc);

/* Return demangled language symbol, or NULL.  */
extern char *language_demangle (const struct language_defn *current_language, 
				const char *mangled, int options);

/* A wrapper for la_sniff_from_mangled_name.  The arguments and result
   are as for the method.  */

extern int language_sniff_from_mangled_name (const struct language_defn *lang,
					     const char *mangled,
					     char **demangled);

/* Return class name from physname, or NULL.  */
extern char *language_class_name_from_physname (const struct language_defn *,
					        const char *physname);

/* Splitting strings into words.  */
extern const char *default_word_break_characters (void);

/* Print the index of an array element using the C99 syntax.  */
extern void default_print_array_index (struct value *index_value,
                                       struct ui_file *stream,
				       const struct value_print_options *options);

/* Return non-zero if TYPE should be passed (and returned) by
   reference at the language level.  */
int language_pass_by_reference (struct type *type);

/* Return zero; by default, types are passed by value at the language
   level.  The target ABI may pass or return some structs by reference
   independent of this.  */
int default_pass_by_reference (struct type *type);

/* The default implementation of la_print_typedef.  */
void default_print_typedef (struct type *type, struct symbol *new_symbol,
			    struct ui_file *stream);

void default_get_string (struct value *value,
			 gdb::unique_xmalloc_ptr<gdb_byte> *buffer,
			 int *length, struct type **char_type,
			 const char **charset);

/* Default name hashing function.  */

/* Produce an unsigned hash value from SEARCH_NAME that is consistent
   with strcmp_iw, strcmp, and, at least on Ada symbols, wild_match.
   That is, two identifiers equivalent according to any of those three
   comparison operators hash to the same value.  */
extern unsigned int default_search_name_hash (const char *search_name);

void c_get_string (struct value *value,
		   gdb::unique_xmalloc_ptr<gdb_byte> *buffer,
		   int *length, struct type **char_type,
		   const char **charset);

/* The default implementation of la_symbol_name_matcher.  Matches with
   strncmp_iw.  */
extern bool default_symbol_name_matcher
  (const char *symbol_search_name,
   const lookup_name_info &lookup_name,
   completion_match_result *comp_match_res);

/* Get LANG's symbol_name_matcher method for LOOKUP_NAME.  Returns
   default_symbol_name_matcher if not set.  LANG is used as a hint;
   the function may ignore it depending on the current language and
   LOOKUP_NAME.  Specifically, if the current language is Ada, this
   may return an Ada matcher regardless of LANG.  */
symbol_name_matcher_ftype *get_symbol_name_matcher
  (const language_defn *lang, const lookup_name_info &lookup_name);

/* The languages supported by GDB.  */

extern const struct language_defn auto_language_defn;
extern const struct language_defn unknown_language_defn;
extern const struct language_defn minimal_language_defn;

extern const struct language_defn ada_language_defn;
extern const struct language_defn asm_language_defn;
extern const struct language_defn c_language_defn;
extern const struct language_defn cplus_language_defn;
extern const struct language_defn d_language_defn;
extern const struct language_defn f_language_defn;
extern const struct language_defn go_language_defn;
extern const struct language_defn m2_language_defn;
extern const struct language_defn objc_language_defn;
extern const struct language_defn opencl_language_defn;
extern const struct language_defn pascal_language_defn;
extern const struct language_defn rust_language_defn;

/* Save the current language and restore it upon destruction.  */

class scoped_restore_current_language
{
public:

  explicit scoped_restore_current_language ()
    : m_lang (current_language->la_language)
  {
  }

  ~scoped_restore_current_language ()
  {
    set_language (m_lang);
  }

  scoped_restore_current_language (const scoped_restore_current_language &)
      = delete;
  scoped_restore_current_language &operator=
      (const scoped_restore_current_language &) = delete;

private:

  enum language m_lang;
};

/* If language_mode is language_mode_auto,
   then switch current language to the language of SYM
   and restore current language upon destruction.

   Else do nothing.  */

class scoped_switch_to_sym_language_if_auto
{
public:

  explicit scoped_switch_to_sym_language_if_auto (const struct symbol *sym)
  {
    if (language_mode == language_mode_auto)
      {
	m_lang = current_language->la_language;
	m_switched = true;
	set_language (SYMBOL_LANGUAGE (sym));
      }
    else
      {
	m_switched = false;
	/* Assign to m_lang to silence a GCC warning.  See
	   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80635.  */
	m_lang = language_unknown;
      }
  }

  ~scoped_switch_to_sym_language_if_auto ()
  {
    if (m_switched)
      set_language (m_lang);
  }

  DISABLE_COPY_AND_ASSIGN (scoped_switch_to_sym_language_if_auto);

private:
  bool m_switched;
  enum language m_lang;
};

#endif /* defined (LANGUAGE_H) */
