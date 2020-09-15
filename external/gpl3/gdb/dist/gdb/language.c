/* Multiple source language support for GDB.

   Copyright (C) 1991-2020 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "gdbcmd.h"
#include "expression.h"
#include "language.h"
#include "varobj.h"
#include "target.h"
#include "parser-defs.h"
#include "demangle.h"
#include "symfile.h"
#include "cp-support.h"
#include "frame.h"
#include "c-lang.h"
#include <algorithm>
#include "gdbarch.h"

static void set_range_case (void);

/* The current (default at startup) state of type and range checking.
   (If the modes are set to "auto", though, these are changed based
   on the default language at startup, and then again based on the
   language of the first source file.  */

enum range_mode range_mode = range_mode_auto;
enum range_check range_check = range_check_off;
enum case_mode case_mode = case_mode_auto;
enum case_sensitivity case_sensitivity = case_sensitive_on;

/* The current language and language_mode (see language.h).  */

const struct language_defn *current_language = nullptr;
enum language_mode language_mode = language_mode_auto;

/* The language that the user expects to be typing in (the language
   of main(), or the last language we notified them about, or C).  */

const struct language_defn *expected_language;

/* Define the array containing all languages.  */

const struct language_defn *language_defn::languages[nr_languages];

/* The current values of the "set language/range/case-sensitive" enum
   commands.  */
static const char *language;
static const char *range;
static const char *case_sensitive;

/* See language.h.  */
const char lang_frame_mismatch_warn[] =
N_("Warning: the current language does not match this frame.");

/* This page contains the functions corresponding to GDB commands
   and their helpers.  */

/* Show command.  Display a warning if the language set
   does not match the frame.  */
static void
show_language_command (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  enum language flang;		/* The language of the frame.  */

  if (language_mode == language_mode_auto)
    fprintf_filtered (gdb_stdout,
		      _("The current source language is "
			"\"auto; currently %s\".\n"),
		      current_language->la_name);
  else
    fprintf_filtered (gdb_stdout,
		      _("The current source language is \"%s\".\n"),
		      current_language->la_name);

  if (has_stack_frames ())
    {
      struct frame_info *frame;

      frame = get_selected_frame (NULL);
      flang = get_frame_language (frame);
      if (flang != language_unknown
	  && language_mode == language_mode_manual
	  && current_language->la_language != flang)
	printf_filtered ("%s\n", _(lang_frame_mismatch_warn));
    }
}

/* Set command.  Change the current working language.  */
static void
set_language_command (const char *ignore,
		      int from_tty, struct cmd_list_element *c)
{
  enum language flang = language_unknown;

  /* "local" is a synonym of "auto".  */
  if (strcmp (language, "local") == 0)
    language = "auto";

  /* Search the list of languages for a match.  */
  for (const auto &lang : language_defn::languages)
    {
      if (strcmp (lang->la_name, language) == 0)
	{
	  /* Found it!  Go into manual mode, and use this language.  */
	  if (lang->la_language == language_auto)
	    {
	      /* Enter auto mode.  Set to the current frame's language, if
                 known, or fallback to the initial language.  */
	      language_mode = language_mode_auto;
	      try
		{
		  struct frame_info *frame;

		  frame = get_selected_frame (NULL);
		  flang = get_frame_language (frame);
		}
	      catch (const gdb_exception_error &ex)
		{
		  flang = language_unknown;
		}

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
	      current_language = lang;
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
      const char *tmp;

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
set_range_command (const char *ignore,
		   int from_tty, struct cmd_list_element *c)
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
      const char *tmp = NULL;

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
set_case_command (const char *ignore, int from_tty, struct cmd_list_element *c)
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
  enum language prev_language;

  prev_language = current_language->la_language;
  current_language = language_def (lang);
  set_range_case ();
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
  return type->code () == TYPE_CODE_PTR || TYPE_IS_REFERENCE (type);
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
language_enum (const char *str)
{
  for (const auto &lang : language_defn::languages)
    if (strcmp (lang->la_name, str) == 0)
      return lang->la_language;

  if (strcmp (str, "local") == 0)
    return language_auto;

  return language_unknown;
}

/* Return the language struct for a given language enum.  */

const struct language_defn *
language_def (enum language lang)
{
  const struct language_defn *l = language_defn::languages[lang];
  gdb_assert (l != nullptr);
  return l;
}

/* Return the language as a string.  */

const char *
language_str (enum language lang)
{
  return language_def (lang)->la_name;
}



/* Build and install the "set language LANG" command.  */

static void
add_set_language_command ()
{
  static const char **language_names;

  /* Build the language names array, to be used as enumeration in the
     "set language" enum command.  +1 for "local" and +1 for NULL
     termination.  */
  language_names = new const char *[ARRAY_SIZE (language_defn::languages) + 2];

  /* Display "auto", "local" and "unknown" first, and then the rest,
     alpha sorted.  */
  const char **language_names_p = language_names;
  *language_names_p++ = language_def (language_auto)->la_name;
  *language_names_p++ = "local";
  *language_names_p++ = language_def (language_unknown)->la_name;
  const char **sort_begin = language_names_p;
  for (const auto &lang : language_defn::languages)
    {
      /* Already handled above.  */
      if (lang->la_language == language_auto
	  || lang->la_language == language_unknown)
	continue;
      *language_names_p++ = lang->la_name;
    }
  *language_names_p = NULL;
  std::sort (sort_begin, language_names_p, compare_cstrings);

  /* Add the filename extensions.  */
  for (const auto &lang : language_defn::languages)
    if (lang->la_filename_extensions != NULL)
      {
	for (size_t i = 0; lang->la_filename_extensions[i] != NULL; ++i)
	  add_filename_language (lang->la_filename_extensions[i],
				 lang->la_language);
      }

  /* Build the "help set language" docs.  */
  string_file doc;

  doc.printf (_("Set the current source language.\n"
		"The currently understood settings are:\n\nlocal or "
		"auto    Automatic setting based on source file"));

  for (const auto &lang : language_defn::languages)
    {
      /* Already dealt with these above.  */
      if (lang->la_language == language_unknown
	  || lang->la_language == language_auto)
	continue;

      /* FIXME: i18n: for now assume that the human-readable name is
	 just a capitalization of the internal name.  */
      /* Note that we add the newline at the front, so we don't wind
	 up with a trailing newline.  */
      doc.printf ("\n%-16s Use the %c%s language",
		  lang->la_name,
		  /* Capitalize first letter of language name.  */
		  toupper (lang->la_name[0]),
		  lang->la_name + 1);
    }

  add_setshow_enum_cmd ("language", class_support,
			language_names,
			&language,
			doc.c_str (),
			_("Show the current source language."),
			NULL, set_language_command,
			show_language_command,
			&setlist, &showlist);
}

/* Iterate through all registered languages looking for and calling
   any non-NULL struct language_defn.skip_trampoline() functions.
   Return the result from the first that returns non-zero, or 0 if all
   `fail'.  */
CORE_ADDR 
skip_language_trampoline (struct frame_info *frame, CORE_ADDR pc)
{
  for (const auto &lang : language_defn::languages)
    {
      CORE_ADDR real_pc = lang->skip_trampoline (frame, pc);

      if (real_pc != 0)
	return real_pc;
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
  if (current_language != NULL)
    return current_language->demangle (mangled, options);
  return NULL;
}

/* Return information about whether TYPE should be passed
   (and returned) by reference at the language level.  */

struct language_pass_by_ref_info
language_pass_by_reference (struct type *type)
{
  return current_language->pass_by_reference_info (type);
}

/* Return the default string containing the list of characters
   delimiting words.  This is a reasonable default value that
   most languages should be able to use.  */

const char *
default_word_break_characters (void)
{
  return " \t\n!@#$%^&*()+=|~`}{[]\"';:?/>.<,-";
}

/* See language.h.  */

void
language_defn::print_array_index (struct type *index_type, LONGEST index,
				  struct ui_file *stream,
				  const value_print_options *options) const
{
  struct value *index_value = value_from_longest (index_type, index);

  fprintf_filtered (stream, "[");
  LA_VALUE_PRINT (index_value, stream, options);
  fprintf_filtered (stream, "] = ");
}

/* See language.h.  */

gdb::unique_xmalloc_ptr<char>
language_defn::watch_location_expression (struct type *type,
					  CORE_ADDR addr) const
{
  /* Generates an expression that assumes a C like syntax is valid.  */
  type = check_typedef (TYPE_TARGET_TYPE (check_typedef (type)));
  std::string name = type_to_string (type);
  return gdb::unique_xmalloc_ptr<char>
    (xstrprintf ("* (%s *) %s", name.c_str (), core_addr_to_string (addr)));
}

/* See language.h.  */

void
language_defn::value_print (struct value *val, struct ui_file *stream,
	       const struct value_print_options *options) const
{
  return c_value_print (val, stream, options);
}

/* See language.h.  */

int
language_defn::parser (struct parser_state *ps) const
{
  return c_parse (ps);
}

/* See language.h.  */

void
language_defn::value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const
{
  return c_value_print_inner (val, stream, recurse, options);
}

/* See language.h.  */

void
language_defn::emitchar (int ch, struct type *chtype,
			 struct ui_file * stream, int quoter) const
{
  c_emit_char (ch, chtype, stream, quoter);
}

/* See language.h.  */

void
language_defn::printchar (int ch, struct type *chtype,
			  struct ui_file * stream) const
{
  c_printchar (ch, chtype, stream);
}

/* See language.h.  */

void
language_defn::printstr (struct ui_file *stream, struct type *elttype,
			 const gdb_byte *string, unsigned int length,
			 const char *encoding, int force_ellipses,
			 const struct value_print_options *options) const
{
  c_printstr (stream, elttype, string, length, encoding, force_ellipses,
	      options);
}

/* See language.h.  */

void
language_defn::print_typedef (struct type *type, struct symbol *new_symbol,
			      struct ui_file *stream) const
{
  c_print_typedef (type, new_symbol, stream);
}

/* See language.h.  */

bool
language_defn::is_string_type_p (struct type *type) const
{
  return c_is_string_type_p (type);
}

/* The default implementation of the get_symbol_name_matcher_inner method
   from the language_defn class.  Matches with strncmp_iw.  */

static bool
default_symbol_name_matcher (const char *symbol_search_name,
			     const lookup_name_info &lookup_name,
			     completion_match_result *comp_match_res)
{
  gdb::string_view name = lookup_name.name ();
  completion_match_for_lcd *match_for_lcd
    = (comp_match_res != NULL ? &comp_match_res->match_for_lcd : NULL);
  strncmp_iw_mode mode = (lookup_name.completion_mode ()
			  ? strncmp_iw_mode::NORMAL
			  : strncmp_iw_mode::MATCH_PARAMS);

  if (strncmp_iw_with_mode (symbol_search_name, name.data (), name.size (),
			    mode, language_minimal, match_for_lcd) == 0)
    {
      if (comp_match_res != NULL)
	comp_match_res->set_match (symbol_search_name);
      return true;
    }
  else
    return false;
}

/* See language.h.  */

symbol_name_matcher_ftype *
language_defn::get_symbol_name_matcher
	(const lookup_name_info &lookup_name) const
{
  /* If currently in Ada mode, and the lookup name is wrapped in
     '<...>', hijack all symbol name comparisons using the Ada
     matcher, which handles the verbatim matching.  */
  if (current_language->la_language == language_ada
      && lookup_name.ada ().verbatim_p ())
    return current_language->get_symbol_name_matcher_inner (lookup_name);

  return this->get_symbol_name_matcher_inner (lookup_name);
}

/* See language.h.  */

symbol_name_matcher_ftype *
language_defn::get_symbol_name_matcher_inner
	(const lookup_name_info &lookup_name) const
{
  return default_symbol_name_matcher;
}

/* Return true if TYPE is a string type, otherwise return false.  This
   default implementation only detects TYPE_CODE_STRING.  */

static bool
default_is_string_type_p (struct type *type)
{
  type = check_typedef (type);
  while (type->code () == TYPE_CODE_REF)
    {
      type = TYPE_TARGET_TYPE (type);
      type = check_typedef (type);
    }
  return (type->code ()  == TYPE_CODE_STRING);
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

/* Constant data that describes the unknown language.  */

extern const struct language_data unknown_language_data =
{
  "unknown",
  "Unknown",
  language_unknown,
  range_check_off,
  case_sensitive_on,
  array_row_major,
  macro_expansion_no,
  NULL,
  &exp_descriptor_standard,
  "this",        	    	/* name_of_this */
  true,				/* store_sym_names_in_linkage_form_p */
  unk_op_print_tab,		/* expression operators for printing */
  1,				/* c-style arrays */
  0,				/* String lower bound */
  &default_varobj_ops,
  "{...}"			/* la_struct_too_deep_ellipsis */
};

/* Class representing the unknown language.  */

class unknown_language : public language_defn
{
public:
  unknown_language ()
    : language_defn (language_unknown, unknown_language_data)
  { /* Nothing.  */ }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    unknown_language_arch_info (gdbarch, lai);
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    error (_("unimplemented unknown_language::print_type called"));
  }

  /* See language.h.  */

  char *demangle (const char *mangled, int options) const override
  {
    /* The unknown language just uses the C++ demangler.  */
    return gdb_demangle (mangled, options);
  }

  /* See language.h.  */

  void value_print (struct value *val, struct ui_file *stream,
		    const struct value_print_options *options) const override
  {
    error (_("unimplemented unknown_language::value_print called"));
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    error (_("unimplemented unknown_language::value_print_inner called"));
  }

  /* See language.h.  */

  int parser (struct parser_state *ps) const override
  {
    /* No parsing is done, just claim success.  */
    return 1;
  }

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override
  {
    error (_("unimplemented unknown_language::emitchar called"));
  }

  /* See language.h.  */

  void printchar (int ch, struct type *chtype,
		  struct ui_file *stream) const override
  {
    error (_("unimplemented unknown_language::printchar called"));
  }

  /* See language.h.  */

  void printstr (struct ui_file *stream, struct type *elttype,
		 const gdb_byte *string, unsigned int length,
		 const char *encoding, int force_ellipses,
		 const struct value_print_options *options) const override
  {
    error (_("unimplemented unknown_language::printstr called"));
  }

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override
  {
    error (_("unimplemented unknown_language::print_typedef called"));
  }

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    return default_is_string_type_p (type);
  }
};

/* Single instance of the unknown language class.  */

static unknown_language unknown_language_defn;

/* Constant data for the fake "auto" language.  */

extern const struct language_data auto_language_data =
{
  "auto",
  "Auto",
  language_auto,
  range_check_off,
  case_sensitive_on,
  array_row_major,
  macro_expansion_no,
  NULL,
  &exp_descriptor_standard,
  "this",		        /* name_of_this */
  false,			/* store_sym_names_in_linkage_form_p */
  unk_op_print_tab,		/* expression operators for printing */
  1,				/* c-style arrays */
  0,				/* String lower bound */
  &default_varobj_ops,
  "{...}"			/* la_struct_too_deep_ellipsis */
};

/* Class representing the fake "auto" language.  */

class auto_language : public language_defn
{
public:
  auto_language ()
    : language_defn (language_auto, auto_language_data)
  { /* Nothing.  */ }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    unknown_language_arch_info (gdbarch, lai);
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    error (_("unimplemented auto_language::print_type called"));
  }

  /* See language.h.  */

  char *demangle (const char *mangled, int options) const override
  {
    /* The auto language just uses the C++ demangler.  */
    return gdb_demangle (mangled, options);
  }

  /* See language.h.  */

  void value_print (struct value *val, struct ui_file *stream,
		    const struct value_print_options *options) const override
  {
    error (_("unimplemented auto_language::value_print called"));
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    error (_("unimplemented auto_language::value_print_inner called"));
  }

  /* See language.h.  */

  int parser (struct parser_state *ps) const override
  {
    /* No parsing is done, just claim success.  */
    return 1;
  }

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override
  {
    error (_("unimplemented auto_language::emitchar called"));
  }

  /* See language.h.  */

  void printchar (int ch, struct type *chtype,
		  struct ui_file *stream) const override
  {
    error (_("unimplemented auto_language::printchar called"));
  }

  /* See language.h.  */

  void printstr (struct ui_file *stream, struct type *elttype,
		 const gdb_byte *string, unsigned int length,
		 const char *encoding, int force_ellipses,
		 const struct value_print_options *options) const override
  {
    error (_("unimplemented auto_language::printstr called"));
  }

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override
  {
    error (_("unimplemented auto_language::print_typedef called"));
  }

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    return default_is_string_type_p (type);
  }
};

/* Single instance of the fake "auto" language.  */

static auto_language auto_language_defn;


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

  l = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct language_gdbarch);
  for (const auto &lang : language_defn::languages)
    {
      gdb_assert (lang != nullptr);
      lang->language_arch_info (gdbarch,
				l->arch_info + lang->la_language);
    }

  return l;
}

struct type *
language_string_char_type (const struct language_defn *la,
			   struct gdbarch *gdbarch)
{
  struct language_gdbarch *ld
    = (struct language_gdbarch *) gdbarch_data (gdbarch, language_gdbarch_data);

  return ld->arch_info[la->la_language].string_char_type;
}

struct type *
language_bool_type (const struct language_defn *la,
		    struct gdbarch *gdbarch)
{
  struct language_gdbarch *ld
    = (struct language_gdbarch *) gdbarch_data (gdbarch, language_gdbarch_data);

  if (ld->arch_info[la->la_language].bool_type_symbol)
    {
      struct symbol *sym;

      sym = lookup_symbol (ld->arch_info[la->la_language].bool_type_symbol,
			   NULL, VAR_DOMAIN, NULL).symbol;
      if (sym)
	{
	  struct type *type = SYMBOL_TYPE (sym);

	  if (type && type->code () == TYPE_CODE_BOOL)
	    return type;
	}
    }

  return ld->arch_info[la->la_language].bool_type_default;
}

/* Helper function for primitive type lookup.  */

static struct type **
language_lookup_primitive_type_1 (const struct language_arch_info *lai,
				  const char *name)
{
  struct type **p;

  for (p = lai->primitive_type_vector; (*p) != NULL; p++)
    {
      if (strcmp ((*p)->name (), name) == 0)
	return p;
    }
  return NULL;
}

/* See language.h.  */

struct type *
language_lookup_primitive_type (const struct language_defn *la,
				struct gdbarch *gdbarch,
				const char *name)
{
  struct language_gdbarch *ld =
    (struct language_gdbarch *) gdbarch_data (gdbarch, language_gdbarch_data);
  struct type **typep;

  typep = language_lookup_primitive_type_1 (&ld->arch_info[la->la_language],
					    name);
  if (typep == NULL)
    return NULL;
  return *typep;
}

/* Helper function for type lookup as a symbol.
   Create the symbol corresponding to type TYPE in language LANG.  */

static struct symbol *
language_alloc_type_symbol (enum language lang, struct type *type)
{
  struct symbol *symbol;
  struct gdbarch *gdbarch;

  gdb_assert (!TYPE_OBJFILE_OWNED (type));

  gdbarch = TYPE_OWNER (type).gdbarch;
  symbol = new (gdbarch_obstack (gdbarch)) struct symbol ();

  symbol->m_name = type->name ();
  symbol->set_language (lang, nullptr);
  symbol->owner.arch = gdbarch;
  SYMBOL_OBJFILE_OWNED (symbol) = 0;
  SYMBOL_SECTION (symbol) = 0;
  SYMBOL_TYPE (symbol) = type;
  SYMBOL_DOMAIN (symbol) = VAR_DOMAIN;
  SYMBOL_ACLASS_INDEX (symbol) = LOC_TYPEDEF;

  return symbol;
}

/* Initialize the primitive type symbols of language LD.
   The primitive type vector must have already been initialized.  */

static void
language_init_primitive_type_symbols (struct language_arch_info *lai,
				      const struct language_defn *la,
				      struct gdbarch *gdbarch)
{
  int n;

  gdb_assert (lai->primitive_type_vector != NULL);

  for (n = 0; lai->primitive_type_vector[n] != NULL; ++n)
    continue;

  lai->primitive_type_symbols
    = GDBARCH_OBSTACK_CALLOC (gdbarch, n + 1, struct symbol *);

  for (n = 0; lai->primitive_type_vector[n] != NULL; ++n)
    {
      lai->primitive_type_symbols[n]
	= language_alloc_type_symbol (la->la_language,
				      lai->primitive_type_vector[n]);
    }

  /* Note: The result of symbol lookup is normally a symbol *and* the block
     it was found in.  Builtin types don't live in blocks.  We *could* give
     them one, but there is no current need so to keep things simple symbol
     lookup is extended to allow for BLOCK_FOUND to be NULL.  */
}

/* See language.h.  */

struct symbol *
language_lookup_primitive_type_as_symbol (const struct language_defn *la,
					  struct gdbarch *gdbarch,
					  const char *name)
{
  struct language_gdbarch *ld
    = (struct language_gdbarch *) gdbarch_data (gdbarch, language_gdbarch_data);
  struct language_arch_info *lai = &ld->arch_info[la->la_language];
  struct type **typep;
  struct symbol *sym;

  if (symbol_lookup_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "language_lookup_primitive_type_as_symbol"
			  " (%s, %s, %s)",
			  la->la_name, host_address_to_string (gdbarch), name);
    }

  typep = language_lookup_primitive_type_1 (lai, name);
  if (typep == NULL)
    {
      if (symbol_lookup_debug)
	fprintf_unfiltered (gdb_stdlog, " = NULL\n");
      return NULL;
    }

  /* The set of symbols is lazily initialized.  */
  if (lai->primitive_type_symbols == NULL)
    language_init_primitive_type_symbols (lai, la, gdbarch);

  sym = lai->primitive_type_symbols[typep - lai->primitive_type_vector];

  if (symbol_lookup_debug)
    fprintf_unfiltered (gdb_stdlog, " = %s\n", host_address_to_string (sym));
  return sym;
}

/* Initialize the language routines.  */

void _initialize_language ();
void
_initialize_language ()
{
  static const char *const type_or_range_names[]
    = { "on", "off", "warn", "auto", NULL };

  static const char *const case_sensitive_names[]
    = { "on", "off", "auto", NULL };

  language_gdbarch_data
    = gdbarch_data_register_post_init (language_gdbarch_post_init);

  /* GDB commands for language specific stuff.  */

  add_basic_prefix_cmd ("check", no_class,
			_("Set the status of the type/range checker."),
			&setchecklist, "set check ", 0, &setlist);
  add_alias_cmd ("c", "check", no_class, 1, &setlist);
  add_alias_cmd ("ch", "check", no_class, 1, &setlist);

  add_show_prefix_cmd ("check", no_class,
		       _("Show the status of the type/range checker."),
		       &showchecklist, "show check ", 0, &showlist);
  add_alias_cmd ("c", "check", no_class, 1, &showlist);
  add_alias_cmd ("ch", "check", no_class, 1, &showlist);

  add_setshow_enum_cmd ("range", class_support, type_or_range_names,
			&range,
			_("Set range checking (on/warn/off/auto)."),
			_("Show range checking (on/warn/off/auto)."),
			NULL, set_range_command,
			show_range_command,
			&setchecklist, &showchecklist);

  add_setshow_enum_cmd ("case-sensitive", class_support, case_sensitive_names,
			&case_sensitive, _("\
Set case sensitivity in name search (on/off/auto)."), _("\
Show case sensitivity in name search (on/off/auto)."), _("\
For Fortran the default is off; for other languages the default is on."),
			set_case_command,
			show_case_command,
			&setlist, &showlist);

  /* In order to call SET_LANGUAGE (below) we need to make sure that
     CURRENT_LANGUAGE is not NULL.  So first set the language to unknown,
     then we can change the language to 'auto'.  */
  current_language = language_def (language_unknown);

  add_set_language_command ();

  language = "auto";
  range = "auto";
  case_sensitive = "auto";

  /* Have the above take effect.  */
  set_language (language_auto);
}
