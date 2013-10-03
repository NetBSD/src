/* Source-language-related definitions for GDB.

   Copyright (C) 1991-2013 Free Software Foundation, Inc.

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

/* Forward decls for prototypes.  */
struct value;
struct objfile;
struct frame_info;
struct expression;
struct ui_file;
struct value_print_options;
struct type_print_options;

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
  /* Type of elements of strings.  */
  struct type *string_char_type;

  /* Symbol name of type to use as boolean type, if defined.  */
  const char *bool_type_symbol;
  /* Otherwise, this is the default boolean builtin type.  */
  struct type *bool_type_default;
};

/* A pointer to a function expected to return nonzero if
   SYMBOL_SEARCH_NAME matches the given LOOKUP_NAME.

   SYMBOL_SEARCH_NAME should be a symbol's "search" name.
   LOOKUP_NAME should be the name of an entity after it has been
   transformed for lookup.  */

typedef int (*symbol_name_cmp_ftype) (const char *symbol_search_name,
					  const char *lookup_name);

/* Structure tying together assorted information about a language.  */

struct language_defn
  {
    /* Name of the language.  */

    char *la_name;

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

    /* Definitions related to expression printing, prefixifying, and
       dumping.  */

    const struct exp_descriptor *la_exp_desc;

    /* Parser function.  */

    int (*la_parser) (void);

    /* Parser error function.  */

    void (*la_error) (char *);

    /* Given an expression *EXPP created by prefixifying the result of
       la_parser, perform any remaining processing necessary to complete
       its translation.  *EXPP may change; la_post_parser is responsible 
       for releasing its previous contents, if necessary.  If 
       VOID_CONTEXT_P, then no value is expected from the expression.  */

    void (*la_post_parser) (struct expression ** expp, int void_context_p);

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

       CONTENTS holds the bits of the value.  This holds the entire
       enclosing object.

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
			  const gdb_byte *contents,
			  int embedded_offset, CORE_ADDR address,
			  struct ui_file *stream, int recurse,
			  const struct value *val,
			  const struct value_print_options *options);

    /* Print a top-level value using syntax appropriate for this language.  */

    void (*la_value_print) (struct value *, struct ui_file *,
			    const struct value_print_options *);

    /* Given a symbol VAR, and a stack frame id FRAME, read the value
       of the variable an return (pointer to a) struct value containing
       the value.

       Throw an error if the variable cannot be found.  */

    struct value *(*la_read_var_value) (struct symbol *var,
					struct frame_info *frame);

    /* PC is possibly an unknown languages trampoline.
       If that PC falls in a trampoline belonging to this language,
       return the address of the first pc in the real function, or 0
       if it isn't a language tramp for this language.  */
    CORE_ADDR (*skip_trampoline) (struct frame_info *, CORE_ADDR);

    /* Now come some hooks for lookup_symbol.  */

    /* If this is non-NULL, specifies the name that of the implicit
       local variable that refers to the current object instance.  */

    char *la_name_of_this;

    /* This is a function that lookup_symbol will call when it gets to
       the part of symbol lookup where C looks up static and global
       variables.  */

    struct symbol *(*la_lookup_symbol_nonlocal) (const char *,
						 const struct block *,
						 const domain_enum);

    /* Find the definition of the type with the given name.  */
    struct type *(*la_lookup_transparent_type) (const char *);

    /* Return demangled language symbol, or NULL.  */
    char *(*la_demangle) (const char *mangled, int options);

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
    char *(*la_word_break_characters) (void);

    /* Should return a vector of all symbols which are possible
       completions for TEXT.  WORD is the entire command on which the
       completion is being made.  If CODE is TYPE_CODE_UNDEF, then all
       symbols should be examined; otherwise, only STRUCT_DOMAIN
       symbols whose type has a code of CODE should be matched.  */
    VEC (char_ptr) *(*la_make_symbol_completion_list) (char *text, char *word,
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
    void (*la_get_string) (struct value *value, gdb_byte **buffer, int *length,
			   struct type **chartype, const char **charset);

    /* Return a pointer to the function that should be used to match
       a symbol name against LOOKUP_NAME. This is mostly for languages
       such as Ada where the matching algorithm depends on LOOKUP_NAME.

       This field may be NULL, in which case strcmp_iw will be used
       to perform the matching.  */
    symbol_name_cmp_ftype (*la_get_symbol_name_cmp) (const char *lookup_name);

    /* Find all symbols in the current program space matching NAME in
       DOMAIN, according to this language's rules.

       The search is done in BLOCK only.
       The caller is responsible for iterating up through superblocks
       if desired.

       For each one, call CALLBACK with the symbol and the DATA
       argument.  If CALLBACK returns zero, the iteration ends at that
       point.

       This field may not be NULL.  If the language does not need any
       special processing here, 'iterate_over_symbols' should be
       used as the definition.  */
    void (*la_iterate_over_symbols) (const struct block *block,
				     const char *name,
				     domain_enum domain,
				     symbol_found_callback_ftype *callback,
				     void *data);

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

struct type *language_lookup_primitive_type_by_name (const struct language_defn *l,
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

#define LA_VAL_PRINT(type,valaddr,offset,addr,stream,val,recurse,options) \
  (current_language->la_val_print(type,valaddr,offset,addr,stream, \
				  val,recurse,options))
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

#define LA_ITERATE_OVER_SYMBOLS(BLOCK, NAME, DOMAIN, CALLBACK, DATA) \
  (current_language->la_iterate_over_symbols (BLOCK, NAME, DOMAIN, CALLBACK, \
					      DATA))

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

/* Checks Binary and Unary operations for semantic type correctness.  */
/* FIXME:  Does not appear to be used.  */
#define unop_type_check(v,o) binop_type_check((v),NULL,(o))

extern void binop_type_check (struct value *, struct value *, int);

/* Error messages */

extern void range_error (const char *, ...) ATTRIBUTE_PRINTF (1, 2);

/* Data:  Does this value represent "truth" to the current language?  */

extern int value_true (struct value *);

/* Misc:  The string representing a particular enum language.  */

extern enum language language_enum (char *str);

extern const struct language_defn *language_def (enum language);

extern char *language_str (enum language);

/* Add a language to the set known by GDB (at initialization time).  */

extern void add_language (const struct language_defn *);

extern enum language get_frame_language (void);	/* In stack.c */

/* Check for a language-specific trampoline.  */

extern CORE_ADDR skip_language_trampoline (struct frame_info *, CORE_ADDR pc);

/* Return demangled language symbol, or NULL.  */
extern char *language_demangle (const struct language_defn *current_language, 
				const char *mangled, int options);

/* Return class name from physname, or NULL.  */
extern char *language_class_name_from_physname (const struct language_defn *,
					        const char *physname);

/* Splitting strings into words.  */
extern char *default_word_break_characters (void);

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

void default_get_string (struct value *value, gdb_byte **buffer, int *length,
			 struct type **char_type, const char **charset);

void c_get_string (struct value *value, gdb_byte **buffer, int *length,
		   struct type **char_type, const char **charset);

#endif /* defined (LANGUAGE_H) */
