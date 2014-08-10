/* Multiple source language support for GDB.

   Copyright (C) 1991-2014 Free Software Foundation, Inc.

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

/* This file contains functions that return things that are specific
   to languages.  Each function should examine current_language if necessary,
   and return the appropriate result.  */

/* FIXME:  Most of these would be better organized as macros which
   return data out of a "language-specific" struct pointer that is set
   whenever the working language changes.  That would be a lot faster.  */

#include "defs.h"
#include <ctype.h>
#include <string.h>

#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "gdbcmd.h"
#include "expression.h"
#include "language.h"
#include "varobj.h"
#include "target.h"
#include "parser-defs.h"
#include "jv-lang.h"
#include "demangle.h"
#include "symfile.h"
#include "cp-support.h"

extern void _initialize_language (void);

static void unk_lang_error (char *);

static int unk_lang_parser (void);

static void show_check (char *, int);

static void set_check (char *, int);

static void set_range_case (void);

static void unk_lang_emit_char (int c, struct type *type,
				struct ui_file *stream, int quoter);

static void unk_lang_printchar (int c, struct type *type,
				struct ui_file *stream);

static void unk_lang_value_print (struct value *, struct ui_file *,
				  const struct value_print_options *);

static CORE_ADDR unk_lang_trampoline (struct frame_info *, CORE_ADDR pc);

/* Forward declaration */
extern const struct language_defn unknown_language_defn;

/* The current (default at startup) state of type and range checking.
   (If the modes are set to "auto", though, these are changed based
   on the default language at startup, and then again based on the
   language of the first source file.  */

enum range_mode range_mode = range_mode_auto;
enum range_check range_check = range_check_off;
enum case_mode case_mode = case_mode_auto;
enum case_sensitivity case_sensitivity = case_sensitive_on;

/* The current language and language_mode (see language.h).  */

const struct language_defn *current_language = &unknown_language_defn;
enum language_mode language_mode = language_mode_auto;

/* The language that the user expects to be typing in (the language
   of main(), or the last language we notified them about, or C).  */

const struct language_defn *expected_language;

/* The list of supported languages.  The list itself is malloc'd.  */

static const struct language_defn **languages;
static unsigned languages_size;
static unsigned languages_allocsize;
#define	DEFAULT_ALLOCSIZE 4

/* The current values of the "set language/type/range" enum
   commands.  */
static const char *language;
static const char *type;
static const char *range;
static const char *case_sensitive;

/* Warning issued when current_language and the language of the current
   frame do not match.  */
char lang_frame_mismatch_warn[] =
"Warning: the current language does not match this frame.";

/* This page contains the functions corresponding to GDB commands
   and their helpers.  */

/* Show command.  Display a warning if the language set
   does not match the frame.  */
static void
show_language_command (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  enum language flang;		/* The language of the current frame.  */

  if (language_mode == language_mode_auto)
    fprintf_filtered (gdb_stdout,
		      _("The current source language is "
			"\"auto; currently %s\".\n"),
		      current_language->la_name);
  else
    fprintf_filtered (gdb_stdout,
		      _("The current source language is \"%s\".\n"),
		      current_language->la_name);

  flang = get_frame_language ();
  if (flang != language_unknown &&
      language_mode == language_mode_manual &&
      current_language->la_language != flang)
    printf_filtered ("%s\n", lang_frame_mismatch_warn);
}

/* Set command.  Change the current working language.  */
static void
set_language_command (char *ignore, int from_tty, struct cmd_list_element *c)
{
  int i;
  enum language flang;

  /* Search the list of languages for a match.  */
  for (i = 0; i < languages_size; i++)
    {
      if (strcmp (languages[i]->la_name, language) == 0)
	{
	  /* Found it!  Go into manual mode, and use this language.  */
	  if (languages[i]->la_language == language_auto)
	    {
	      /* Enter auto mode.  Set to the current frame's language, if
                 known, or fallback to the initial language.  */
	      language_mode = language_mode_auto;
	      flang = get_frame_language ();
	      if (flang != language_unknown)
		set_language (flang);
	      else
		set_initial_language ();
	      expected_language = current_language;
	      return;
	    }
	  else
	    {
	      /* Enter manual mode.  Set the specified language.  */
	      language_mode = language_mode_manual;
	      current_language = languages[i];
	      set_range_case ();
	      expected_language = current_language;
	      return;
	    }
	}
    }

  internal_error (__FILE__, __LINE__,
		  "Couldn't find language `%s' in known languages list.",
		  language);
}

/* Show command.  Display a warning if the range setting does
   not match the current language.  */
static void
show_range_command (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  if (range_mode == range_mode_auto)
    {
      char *tmp;

      switch (range_check)
	{
	case range_check_on:
	  tmp = "on";
	  break;
	case range_check_off:
	  tmp = "off";
	  break;
	case range_check_warn:
	  tmp = "warn";
	  break;
	default:
	  internal_error (__FILE__, __LINE__,
			  "Unrecognized range check setting.");
	}

      fprintf_filtered (gdb_stdout,
			_("Range checking is \"auto; currently %s\".\n"),
			tmp);
    }
  else
    fprintf_filtered (gdb_stdout, _("Range checking is \"%s\".\n"),
		      value);

  if (range_check != current_language->la_range_check)
    warning (_("the current range check setting "
	       "does not match the language.\n"));
}

/* Set command.  Change the setting for range checking.  */
static void
set_range_command (char *ignore, int from_tty, struct cmd_list_element *c)
{
  if (strcmp (range, "on") == 0)
    {
      range_check = range_check_on;
      range_mode = range_mode_manual;
    }
  else if (strcmp (range, "warn") == 0)
    {
      range_check = range_check_warn;
      range_mode = range_mode_manual;
    }
  else if (strcmp (range, "off") == 0)
    {
      range_check = range_check_off;
      range_mode = range_mode_manual;
    }
  else if (strcmp (range, "auto") == 0)
    {
      range_mode = range_mode_auto;
      set_range_case ();
      return;
    }
  else
    {
      internal_error (__FILE__, __LINE__,
		      _("Unrecognized range check setting: \"%s\""), range);
    }
  if (range_check != current_language->la_range_check)
    warning (_("the current range check setting "
	       "does not match the language.\n"));
}

/* Show command.  Display a warning if the case sensitivity setting does
   not match the current language.  */
static void
show_case_command (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  if (case_mode == case_mode_auto)
    {
      char *tmp = NULL;

      switch (case_sensitivity)
	{
	case case_sensitive_on:
	  tmp = "on";
	  break;
	case case_sensitive_off:
	  tmp = "off";
	  break;
	default:
	  internal_error (__FILE__, __LINE__,
			  "Unrecognized case-sensitive setting.");
	}

      fprintf_filtered (gdb_stdout,
			_("Case sensitivity in "
			  "name search is \"auto; currently %s\".\n"),
			tmp);
    }
  else
    fprintf_filtered (gdb_stdout,
		      _("Case sensitivity in name search is \"%s\".\n"),
		      value);

  if (case_sensitivity != current_language->la_case_sensitivity)
    warning (_("the current case sensitivity setting does not match "
	       "the language.\n"));
}

/* Set command.  Change the setting for case sensitivity.  */

static void
set_case_command (char *ignore, int from_tty, struct cmd_list_element *c)
{
   if (strcmp (case_sensitive, "on") == 0)
     {
       case_sensitivity = case_sensitive_on;
       case_mode = case_mode_manual;
     }
   else if (strcmp (case_sensitive, "off") == 0)
     {
       case_sensitivity = case_sensitive_off;
       case_mode = case_mode_manual;
     }
   else if (strcmp (case_sensitive, "auto") == 0)
     {
       case_mode = case_mode_auto;
       set_range_case ();
       return;
     }
   else
     {
       internal_error (__FILE__, __LINE__,
		       "Unrecognized case-sensitive setting: \"%s\"",
		       case_sensitive);
     }

   if (case_sensitivity != current_language->la_case_sensitivity)
     warning (_("the current case sensitivity setting does not match "
		"the language.\n"));
}

/* Set the status of range and type checking and case sensitivity based on
   the current modes and the current language.
   If SHOW is non-zero, then print out the current language,
   type and range checking status.  */
static void
set_range_case (void)
{
  if (range_mode == range_mode_auto)
    range_check = current_language->la_range_check;

  if (case_mode == case_mode_auto)
    case_sensitivity = current_language->la_case_sensitivity;
}

/* Set current language to (enum language) LANG.  Returns previous
   language.  */

enum language
set_language (enum language lang)
{
  int i;
  enum language prev_language;

  prev_language = current_language->la_language;

  for (i = 0; i < languages_size; i++)
    {
      if (languages[i]->la_language == lang)
	{
	  current_language = languages[i];
	  set_range_case ();
	  break;
	}
    }

  return prev_language;
}


/* Print out the current language settings: language, range and
   type checking.  If QUIETLY, print only what has changed.  */

void
language_info (int quietly)
{
  if (quietly && expected_language == current_language)
    return;

  expected_language = current_language;
  printf_unfiltered (_("Current language:  %s\n"), language);
  show_language_command (NULL, 1, NULL, NULL);

  if (!quietly)
    {
      printf_unfiltered (_("Range checking:    %s\n"), range);
      show_range_command (NULL, 1, NULL, NULL);
      printf_unfiltered (_("Case sensitivity:  %s\n"), case_sensitive);
      show_case_command (NULL, 1, NULL, NULL);
    }
}


/* Returns non-zero if the value is a pointer type.  */
int
pointer_type (struct type *type)
{
  return TYPE_CODE (type) == TYPE_CODE_PTR ||
    TYPE_CODE (type) == TYPE_CODE_REF;
}


/* This page contains functions that return info about
   (struct value) values used in GDB.  */

/* Returns non-zero if the value VAL represents a true value.  */
int
value_true (struct value *val)
{
  /* It is possible that we should have some sort of error if a non-boolean
     value is used in this context.  Possibly dependent on some kind of
     "boolean-checking" option like range checking.  But it should probably
     not depend on the language except insofar as is necessary to identify
     a "boolean" value (i.e. in C using a float, pointer, etc., as a boolean
     should be an error, probably).  */
  return !value_logical_not (val);
}

/* This page contains functions for the printing out of
   error messages that occur during type- and range-
   checking.  */

/* This is called when a language fails a range-check.  The
   first argument should be a printf()-style format string, and the
   rest of the arguments should be its arguments.  If range_check is
   range_check_on, an error is printed;  if range_check_warn, a warning;
   otherwise just the message.  */

void
range_error (const char *string,...)
{
  va_list args;

  va_start (args, string);
  switch (range_check)
    {
    case range_check_warn:
      vwarning (string, args);
      break;
    case range_check_on:
      verror (string, args);
      break;
    case range_check_off:
      /* FIXME: cagney/2002-01-30: Should this function print anything
         when range error is off?  */
      vfprintf_filtered (gdb_stderr, string, args);
      fprintf_filtered (gdb_stderr, "\n");
      break;
    default:
      internal_error (__FILE__, __LINE__, _("bad switch"));
    }
  va_end (args);
}


/* This page contains miscellaneous functions.  */

/* Return the language enum for a given language string.  */

enum language
language_enum (char *str)
{
  int i;

  for (i = 0; i < languages_size; i++)
    if (strcmp (languages[i]->la_name, str) == 0)
      return languages[i]->la_language;

  return language_unknown;
}

/* Return the language struct for a given language enum.  */

const struct language_defn *
language_def (enum language lang)
{
  int i;

  for (i = 0; i < languages_size; i++)
    {
      if (languages[i]->la_language == lang)
	{
	  return languages[i];
	}
    }
  return NULL;
}

/* Return the language as a string.  */
const char *
language_str (enum language lang)
{
  int i;

  for (i = 0; i < languages_size; i++)
    {
      if (languages[i]->la_language == lang)
	{
	  return languages[i]->la_name;
	}
    }
  return "Unknown";
}

static void
set_check (char *ignore, int from_tty)
{
  printf_unfiltered (
     "\"set check\" must be followed by the name of a check subcommand.\n");
  help_list (setchecklist, "set check ", -1, gdb_stdout);
}

static void
show_check (char *ignore, int from_tty)
{
  cmd_show_list (showchecklist, from_tty, "");
}

/* Add a language to the set of known languages.  */

void
add_language (const struct language_defn *lang)
{
  /* For the "set language" command.  */
  static const char **language_names = NULL;
  /* For the "help set language" command.  */
  char *language_set_doc = NULL;

  int i;
  struct ui_file *tmp_stream;

  if (lang->la_magic != LANG_MAGIC)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Magic number of %s language struct wrong\n",
			  lang->la_name);
      internal_error (__FILE__, __LINE__,
		      _("failed internal consistency check"));
    }

  if (!languages)
    {
      languages_allocsize = DEFAULT_ALLOCSIZE;
      languages = (const struct language_defn **) xmalloc
	(languages_allocsize * sizeof (*languages));
    }
  if (languages_size >= languages_allocsize)
    {
      languages_allocsize *= 2;
      languages = (const struct language_defn **) xrealloc ((char *) languages,
				 languages_allocsize * sizeof (*languages));
    }
  languages[languages_size++] = lang;

  /* Build the language names array, to be used as enumeration in the
     set language" enum command.  */
  language_names = xrealloc (language_names,
			     (languages_size + 1) * sizeof (const char *));
  for (i = 0; i < languages_size; ++i)
    language_names[i] = languages[i]->la_name;
  language_names[i] = NULL;

  /* Build the "help set language" docs.  */
  tmp_stream = mem_fileopen ();

  fprintf_unfiltered (tmp_stream,
		      _("Set the current source language.\n"
			"The currently understood settings are:\n\nlocal or "
			"auto    Automatic setting based on source file\n"));

  for (i = 0; i < languages_size; ++i)
    {
      /* Already dealt with these above.  */
      if (languages[i]->la_language == language_unknown
	  || languages[i]->la_language == language_auto)
	continue;

      /* FIXME: i18n: for now assume that the human-readable name
	 is just a capitalization of the internal name.  */
      fprintf_unfiltered (tmp_stream, "%-16s Use the %c%s language\n",
			  languages[i]->la_name,
			  /* Capitalize first letter of language
			     name.  */
			  toupper (languages[i]->la_name[0]),
			  languages[i]->la_name + 1);
    }

  language_set_doc = ui_file_xstrdup (tmp_stream, NULL);
  ui_file_delete (tmp_stream);

  add_setshow_enum_cmd ("language", class_support,
			(const char **) language_names,
			&language,
			language_set_doc,
			_("Show the current source language."),
			NULL, set_language_command,
			show_language_command,
			&setlist, &showlist);

  xfree (language_set_doc);
}

/* Iterate through all registered languages looking for and calling
   any non-NULL struct language_defn.skip_trampoline() functions.
   Return the result from the first that returns non-zero, or 0 if all
   `fail'.  */
CORE_ADDR 
skip_language_trampoline (struct frame_info *frame, CORE_ADDR pc)
{
  int i;

  for (i = 0; i < languages_size; i++)
    {
      if (languages[i]->skip_trampoline)
	{
	  CORE_ADDR real_pc = (languages[i]->skip_trampoline) (frame, pc);

	  if (real_pc)
	    return real_pc;
	}
    }

  return 0;
}

/* Return demangled language symbol, or NULL.
   FIXME: Options are only useful for certain languages and ignored
   by others, so it would be better to remove them here and have a
   more flexible demangler for the languages that need it.
   FIXME: Sometimes the demangler is invoked when we don't know the
   language, so we can't use this everywhere.  */
char *
language_demangle (const struct language_defn *current_language, 
				const char *mangled, int options)
{
  if (current_language != NULL && current_language->la_demangle)
    return current_language->la_demangle (mangled, options);
  return NULL;
}

/* Return class name from physname or NULL.  */
char *
language_class_name_from_physname (const struct language_defn *lang,
				   const char *physname)
{
  if (lang != NULL && lang->la_class_name_from_physname)
    return lang->la_class_name_from_physname (physname);
  return NULL;
}

/* Return non-zero if TYPE should be passed (and returned) by
   reference at the language level.  */
int
language_pass_by_reference (struct type *type)
{
  return current_language->la_pass_by_reference (type);
}

/* Return zero; by default, types are passed by value at the language
   level.  The target ABI may pass or return some structs by reference
   independent of this.  */
int
default_pass_by_reference (struct type *type)
{
  return 0;
}

/* Return the default string containing the list of characters
   delimiting words.  This is a reasonable default value that
   most languages should be able to use.  */

char *
default_word_break_characters (void)
{
  return " \t\n!@#$%^&*()+=|~`}{[]\"';:?/>.<,-";
}

/* Print the index of array elements using the C99 syntax.  */

void
default_print_array_index (struct value *index_value, struct ui_file *stream,
			   const struct value_print_options *options)
{
  fprintf_filtered (stream, "[");
  LA_VALUE_PRINT (index_value, stream, options);
  fprintf_filtered (stream, "] = ");
}

void
default_get_string (struct value *value, gdb_byte **buffer, int *length,
		    struct type **char_type, const char **charset)
{
  error (_("Getting a string is unsupported in this language."));
}

/* Define the language that is no language.  */

static int
unk_lang_parser (void)
{
  return 1;
}

static void
unk_lang_error (char *msg)
{
  error (_("Attempted to parse an expression with unknown language"));
}

static void
unk_lang_emit_char (int c, struct type *type, struct ui_file *stream,
		    int quoter)
{
  error (_("internal error - unimplemented "
	   "function unk_lang_emit_char called."));
}

static void
unk_lang_printchar (int c, struct type *type, struct ui_file *stream)
{
  error (_("internal error - unimplemented "
	   "function unk_lang_printchar called."));
}

static void
unk_lang_printstr (struct ui_file *stream, struct type *type,
		   const gdb_byte *string, unsigned int length,
		   const char *encoding, int force_ellipses,
		   const struct value_print_options *options)
{
  error (_("internal error - unimplemented "
	   "function unk_lang_printstr called."));
}

static void
unk_lang_print_type (struct type *type, const char *varstring,
		     struct ui_file *stream, int show, int level,
		     const struct type_print_options *flags)
{
  error (_("internal error - unimplemented "
	   "function unk_lang_print_type called."));
}

static void
unk_lang_val_print (struct type *type, const gdb_byte *valaddr,
		    int embedded_offset, CORE_ADDR address,
		    struct ui_file *stream, int recurse,
		    const struct value *val,
		    const struct value_print_options *options)
{
  error (_("internal error - unimplemented "
	   "function unk_lang_val_print called."));
}

static void
unk_lang_value_print (struct value *val, struct ui_file *stream,
		      const struct value_print_options *options)
{
  error (_("internal error - unimplemented "
	   "function unk_lang_value_print called."));
}

static CORE_ADDR unk_lang_trampoline (struct frame_info *frame, CORE_ADDR pc)
{
  return 0;
}

/* Unknown languages just use the cplus demangler.  */
static char *unk_lang_demangle (const char *mangled, int options)
{
  return gdb_demangle (mangled, options);
}

static char *unk_lang_class_name (const char *mangled)
{
  return NULL;
}

static const struct op_print unk_op_print_tab[] =
{
  {NULL, OP_NULL, PREC_NULL, 0}
};

static void
unknown_language_arch_info (struct gdbarch *gdbarch,
			    struct language_arch_info *lai)
{
  lai->string_char_type = builtin_type (gdbarch)->builtin_char;
  lai->bool_type_default = builtin_type (gdbarch)->builtin_int;
  lai->primitive_type_vector = GDBARCH_OBSTACK_CALLOC (gdbarch, 1,
						       struct type *);
}

const struct language_defn unknown_language_defn =
{
  "unknown",
  "Unknown",
  language_unknown,
  range_check_off,
  case_sensitive_on,
  array_row_major,
  macro_expansion_no,
  &exp_descriptor_standard,
  unk_lang_parser,
  unk_lang_error,
  null_post_parser,
  unk_lang_printchar,		/* Print character constant */
  unk_lang_printstr,
  unk_lang_emit_char,
  unk_lang_print_type,		/* Print a type using appropriate syntax */
  default_print_typedef,	/* Print a typedef using appropriate syntax */
  unk_lang_val_print,		/* Print a value using appropriate syntax */
  unk_lang_value_print,		/* Print a top-level value */
  default_read_var_value,	/* la_read_var_value */
  unk_lang_trampoline,		/* Language specific skip_trampoline */
  "this",        	    	/* name_of_this */
  basic_lookup_symbol_nonlocal, /* lookup_symbol_nonlocal */
  basic_lookup_transparent_type,/* lookup_transparent_type */
  unk_lang_demangle,		/* Language specific symbol demangler */
  unk_lang_class_name,		/* Language specific
				   class_name_from_physname */
  unk_op_print_tab,		/* expression operators for printing */
  1,				/* c-style arrays */
  0,				/* String lower bound */
  default_word_break_characters,
  default_make_symbol_completion_list,
  unknown_language_arch_info,	/* la_language_arch_info.  */
  default_print_array_index,
  default_pass_by_reference,
  default_get_string,
  NULL,				/* la_get_symbol_name_cmp */
  iterate_over_symbols,
  &default_varobj_ops,
  LANG_MAGIC
};

/* These two structs define fake entries for the "local" and "auto"
   options.  */
const struct language_defn auto_language_defn =
{
  "auto",
  "Auto",
  language_auto,
  range_check_off,
  case_sensitive_on,
  array_row_major,
  macro_expansion_no,
  &exp_descriptor_standard,
  unk_lang_parser,
  unk_lang_error,
  null_post_parser,
  unk_lang_printchar,		/* Print character constant */
  unk_lang_printstr,
  unk_lang_emit_char,
  unk_lang_print_type,		/* Print a type using appropriate syntax */
  default_print_typedef,	/* Print a typedef using appropriate syntax */
  unk_lang_val_print,		/* Print a value using appropriate syntax */
  unk_lang_value_print,		/* Print a top-level value */
  default_read_var_value,	/* la_read_var_value */
  unk_lang_trampoline,		/* Language specific skip_trampoline */
  "this",		        /* name_of_this */
  basic_lookup_symbol_nonlocal,	/* lookup_symbol_nonlocal */
  basic_lookup_transparent_type,/* lookup_transparent_type */
  unk_lang_demangle,		/* Language specific symbol demangler */
  unk_lang_class_name,		/* Language specific
				   class_name_from_physname */
  unk_op_print_tab,		/* expression operators for printing */
  1,				/* c-style arrays */
  0,				/* String lower bound */
  default_word_break_characters,
  default_make_symbol_completion_list,
  unknown_language_arch_info,	/* la_language_arch_info.  */
  default_print_array_index,
  default_pass_by_reference,
  default_get_string,
  NULL,				/* la_get_symbol_name_cmp */
  iterate_over_symbols,
  &default_varobj_ops,
  LANG_MAGIC
};

const struct language_defn local_language_defn =
{
  "local",
  "Local",
  language_auto,
  range_check_off,
  case_sensitive_on,
  array_row_major,
  macro_expansion_no,
  &exp_descriptor_standard,
  unk_lang_parser,
  unk_lang_error,
  null_post_parser,
  unk_lang_printchar,		/* Print character constant */
  unk_lang_printstr,
  unk_lang_emit_char,
  unk_lang_print_type,		/* Print a type using appropriate syntax */
  default_print_typedef,	/* Print a typedef using appropriate syntax */
  unk_lang_val_print,		/* Print a value using appropriate syntax */
  unk_lang_value_print,		/* Print a top-level value */
  default_read_var_value,	/* la_read_var_value */
  unk_lang_trampoline,		/* Language specific skip_trampoline */
  "this", 		        /* name_of_this */
  basic_lookup_symbol_nonlocal,	/* lookup_symbol_nonlocal */
  basic_lookup_transparent_type,/* lookup_transparent_type */
  unk_lang_demangle,		/* Language specific symbol demangler */
  unk_lang_class_name,		/* Language specific
				   class_name_from_physname */
  unk_op_print_tab,		/* expression operators for printing */
  1,				/* c-style arrays */
  0,				/* String lower bound */
  default_word_break_characters,
  default_make_symbol_completion_list,
  unknown_language_arch_info,	/* la_language_arch_info.  */
  default_print_array_index,
  default_pass_by_reference,
  default_get_string,
  NULL,				/* la_get_symbol_name_cmp */
  iterate_over_symbols,
  &default_varobj_ops,
  LANG_MAGIC
};

/* Per-architecture language information.  */

static struct gdbarch_data *language_gdbarch_data;

struct language_gdbarch
{
  /* A vector of per-language per-architecture info.  Indexed by "enum
     language".  */
  struct language_arch_info arch_info[nr_languages];
};

static void *
language_gdbarch_post_init (struct gdbarch *gdbarch)
{
  struct language_gdbarch *l;
  int i;

  l = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct language_gdbarch);
  for (i = 0; i < languages_size; i++)
    {
      if (languages[i] != NULL
	  && languages[i]->la_language_arch_info != NULL)
	languages[i]->la_language_arch_info
	  (gdbarch, l->arch_info + languages[i]->la_language);
    }
  return l;
}

struct type *
language_string_char_type (const struct language_defn *la,
			   struct gdbarch *gdbarch)
{
  struct language_gdbarch *ld = gdbarch_data (gdbarch,
					      language_gdbarch_data);

  return ld->arch_info[la->la_language].string_char_type;
}

struct type *
language_bool_type (const struct language_defn *la,
		    struct gdbarch *gdbarch)
{
  struct language_gdbarch *ld = gdbarch_data (gdbarch,
					      language_gdbarch_data);

  if (ld->arch_info[la->la_language].bool_type_symbol)
    {
      struct symbol *sym;

      sym = lookup_symbol (ld->arch_info[la->la_language].bool_type_symbol,
			   NULL, VAR_DOMAIN, NULL);
      if (sym)
	{
	  struct type *type = SYMBOL_TYPE (sym);

	  if (type && TYPE_CODE (type) == TYPE_CODE_BOOL)
	    return type;
	}
    }

  return ld->arch_info[la->la_language].bool_type_default;
}

struct type *
language_lookup_primitive_type_by_name (const struct language_defn *la,
					struct gdbarch *gdbarch,
					const char *name)
{
  struct language_gdbarch *ld = gdbarch_data (gdbarch,
					      language_gdbarch_data);
  struct type *const *p;

  for (p = ld->arch_info[la->la_language].primitive_type_vector;
       (*p) != NULL;
       p++)
    {
      if (strcmp (TYPE_NAME (*p), name) == 0)
	return (*p);
    }
  return (NULL);
}

/* Initialize the language routines.  */

void
_initialize_language (void)
{
  static const char *const type_or_range_names[]
    = { "on", "off", "warn", "auto", NULL };

  static const char *const case_sensitive_names[]
    = { "on", "off", "auto", NULL };

  language_gdbarch_data
    = gdbarch_data_register_post_init (language_gdbarch_post_init);

  /* GDB commands for language specific stuff.  */

  add_prefix_cmd ("check", no_class, set_check,
		  _("Set the status of the type/range checker."),
		  &setchecklist, "set check ", 0, &setlist);
  add_alias_cmd ("c", "check", no_class, 1, &setlist);
  add_alias_cmd ("ch", "check", no_class, 1, &setlist);

  add_prefix_cmd ("check", no_class, show_check,
		  _("Show the status of the type/range checker."),
		  &showchecklist, "show check ", 0, &showlist);
  add_alias_cmd ("c", "check", no_class, 1, &showlist);
  add_alias_cmd ("ch", "check", no_class, 1, &showlist);

  add_setshow_enum_cmd ("range", class_support, type_or_range_names,
			&range,
			_("Set range checking.  (on/warn/off/auto)"),
			_("Show range checking.  (on/warn/off/auto)"),
			NULL, set_range_command,
			show_range_command,
			&setchecklist, &showchecklist);

  add_setshow_enum_cmd ("case-sensitive", class_support, case_sensitive_names,
			&case_sensitive, _("\
Set case sensitivity in name search.  (on/off/auto)"), _("\
Show case sensitivity in name search.  (on/off/auto)"), _("\
For Fortran the default is off; for other languages the default is on."),
			set_case_command,
			show_case_command,
			&setlist, &showlist);

  add_language (&auto_language_defn);
  add_language (&local_language_defn);
  add_language (&unknown_language_defn);

  language = xstrdup ("auto");
  type = xstrdup ("auto");
  range = xstrdup ("auto");
  case_sensitive = xstrdup ("auto");

  /* Have the above take effect.  */
  set_language (language_auto);
}
