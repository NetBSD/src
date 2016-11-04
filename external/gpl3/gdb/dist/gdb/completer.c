/* Line completion stuff for GDB, the GNU debugger.
   Copyright (C) 2000-2016 Free Software Foundation, Inc.

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
#include "filenames.h"		/* For DOSish file names.  */
#include "language.h"
#include "gdb_signals.h"
#include "target.h"
#include "reggroups.h"
#include "user-regs.h"
#include "arch-utils.h"
#include "location.h"

#include "cli/cli-decode.h"

/* FIXME: This is needed because of lookup_cmd_1 ().  We should be
   calling a hook instead so we eliminate the CLI dependency.  */
#include "gdbcmd.h"

/* Needed for rl_completer_word_break_characters() and for
   rl_filename_completion_function.  */
#include "readline/readline.h"

/* readline defines this.  */
#undef savestring

#include "completer.h"

/* An enumeration of the various things a user might
   attempt to complete for a location.  */

enum explicit_location_match_type
{
    /* The filename of a source file.  */
    MATCH_SOURCE,

    /* The name of a function or method.  */
    MATCH_FUNCTION,

    /* The name of a label.  */
    MATCH_LABEL
};

/* Prototypes for local functions.  */
static
char *line_completion_function (const char *text, int matches, 
				char *line_buffer,
				int point);

/* readline uses the word breaks for two things:
   (1) In figuring out where to point the TEXT parameter to the
   rl_completion_entry_function.  Since we don't use TEXT for much,
   it doesn't matter a lot what the word breaks are for this purpose,
   but it does affect how much stuff M-? lists.
   (2) If one of the matches contains a word break character, readline
   will quote it.  That's why we switch between
   current_language->la_word_break_characters() and
   gdb_completer_command_word_break_characters.  I'm not sure when
   we need this behavior (perhaps for funky characters in C++ 
   symbols?).  */

/* Variables which are necessary for fancy command line editing.  */

/* When completing on command names, we remove '-' from the list of
   word break characters, since we use it in command names.  If the
   readline library sees one in any of the current completion strings,
   it thinks that the string needs to be quoted and automatically
   supplies a leading quote.  */
static char *gdb_completer_command_word_break_characters =
" \t\n!@#$%^&*()+=|~`}{[]\"';:?/>.<,";

/* When completing on file names, we remove from the list of word
   break characters any characters that are commonly used in file
   names, such as '-', '+', '~', etc.  Otherwise, readline displays
   incorrect completion candidates.  */
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
/* MS-DOS and MS-Windows use colon as part of the drive spec, and most
   programs support @foo style response files.  */
static char *gdb_completer_file_name_break_characters = " \t\n*|\"';?><@";
#else
static char *gdb_completer_file_name_break_characters = " \t\n*|\"';:?><";
#endif

/* Characters that can be used to quote completion strings.  Note that
   we can't include '"' because the gdb C parser treats such quoted
   sequences as strings.  */
static char *gdb_completer_quote_characters = "'";

/* Accessor for some completer data that may interest other files.  */

char *
get_gdb_completer_quote_characters (void)
{
  return gdb_completer_quote_characters;
}

/* Line completion interface function for readline.  */

char *
readline_line_completion_function (const char *text, int matches)
{
  return line_completion_function (text, matches, 
				   rl_line_buffer, rl_point);
}

/* This can be used for functions which don't want to complete on
   symbols but don't want to complete on anything else either.  */
VEC (char_ptr) *
noop_completer (struct cmd_list_element *ignore, 
		const char *text, const char *prefix)
{
  return NULL;
}

/* Complete on filenames.  */
VEC (char_ptr) *
filename_completer (struct cmd_list_element *ignore, 
		    const char *text, const char *word)
{
  int subsequent_name;
  VEC (char_ptr) *return_val = NULL;

  subsequent_name = 0;
  while (1)
    {
      char *p, *q;

      p = rl_filename_completion_function (text, subsequent_name);
      if (p == NULL)
	break;
      /* We need to set subsequent_name to a non-zero value before the
	 continue line below, because otherwise, if the first file
	 seen by GDB is a backup file whose name ends in a `~', we
	 will loop indefinitely.  */
      subsequent_name = 1;
      /* Like emacs, don't complete on old versions.  Especially
         useful in the "source" command.  */
      if (p[strlen (p) - 1] == '~')
	{
	  xfree (p);
	  continue;
	}

      if (word == text)
	/* Return exactly p.  */
	q = p;
      else if (word > text)
	{
	  /* Return some portion of p.  */
	  q = (char *) xmalloc (strlen (p) + 5);
	  strcpy (q, p + (word - text));
	  xfree (p);
	}
      else
	{
	  /* Return some of TEXT plus p.  */
	  q = (char *) xmalloc (strlen (p) + (text - word) + 5);
	  strncpy (q, word, text - word);
	  q[text - word] = '\0';
	  strcat (q, p);
	  xfree (p);
	}
      VEC_safe_push (char_ptr, return_val, q);
    }
#if 0
  /* There is no way to do this just long enough to affect quote
     inserting without also affecting the next completion.  This
     should be fixed in readline.  FIXME.  */
  /* Ensure that readline does the right thing
     with respect to inserting quotes.  */
  rl_completer_word_break_characters = "";
#endif
  return return_val;
}

/* Complete on linespecs, which might be of two possible forms:

       file:line
   or
       symbol+offset

   This is intended to be used in commands that set breakpoints
   etc.  */

static VEC (char_ptr) *
linespec_location_completer (struct cmd_list_element *ignore,
			     const char *text, const char *word)
{
  int n_syms, n_files, ix;
  VEC (char_ptr) *fn_list = NULL;
  VEC (char_ptr) *list = NULL;
  const char *p;
  int quote_found = 0;
  int quoted = *text == '\'' || *text == '"';
  int quote_char = '\0';
  const char *colon = NULL;
  char *file_to_match = NULL;
  const char *symbol_start = text;
  const char *orig_text = text;
  size_t text_len;

  /* Do we have an unquoted colon, as in "break foo.c:bar"?  */
  for (p = text; *p != '\0'; ++p)
    {
      if (*p == '\\' && p[1] == '\'')
	p++;
      else if (*p == '\'' || *p == '"')
	{
	  quote_found = *p;
	  quote_char = *p++;
	  while (*p != '\0' && *p != quote_found)
	    {
	      if (*p == '\\' && p[1] == quote_found)
		p++;
	      p++;
	    }

	  if (*p == quote_found)
	    quote_found = 0;
	  else
	    break;		/* Hit the end of text.  */
	}
#if HAVE_DOS_BASED_FILE_SYSTEM
      /* If we have a DOS-style absolute file name at the beginning of
	 TEXT, and the colon after the drive letter is the only colon
	 we found, pretend the colon is not there.  */
      else if (p < text + 3 && *p == ':' && p == text + 1 + quoted)
	;
#endif
      else if (*p == ':' && !colon)
	{
	  colon = p;
	  symbol_start = p + 1;
	}
      else if (strchr (current_language->la_word_break_characters(), *p))
	symbol_start = p + 1;
    }

  if (quoted)
    text++;
  text_len = strlen (text);

  /* Where is the file name?  */
  if (colon)
    {
      char *s;

      file_to_match = (char *) xmalloc (colon - text + 1);
      strncpy (file_to_match, text, colon - text);
      file_to_match[colon - text] = '\0';
      /* Remove trailing colons and quotes from the file name.  */
      for (s = file_to_match + (colon - text);
	   s > file_to_match;
	   s--)
	if (*s == ':' || *s == quote_char)
	  *s = '\0';
    }
  /* If the text includes a colon, they want completion only on a
     symbol name after the colon.  Otherwise, we need to complete on
     symbols as well as on files.  */
  if (colon)
    {
      list = make_file_symbol_completion_list (symbol_start, word,
					       file_to_match);
      xfree (file_to_match);
    }
  else
    {
      list = make_symbol_completion_list (symbol_start, word);
      /* If text includes characters which cannot appear in a file
	 name, they cannot be asking for completion on files.  */
      if (strcspn (text, 
		   gdb_completer_file_name_break_characters) == text_len)
	fn_list = make_source_files_completion_list (text, text);
    }

  n_syms = VEC_length (char_ptr, list);
  n_files = VEC_length (char_ptr, fn_list);

  /* Catenate fn_list[] onto the end of list[].  */
  if (!n_syms)
    {
      VEC_free (char_ptr, list); /* Paranoia.  */
      list = fn_list;
      fn_list = NULL;
    }
  else
    {
      char *fn;

      for (ix = 0; VEC_iterate (char_ptr, fn_list, ix, fn); ++ix)
	VEC_safe_push (char_ptr, list, fn);
      VEC_free (char_ptr, fn_list);
    }

  if (n_syms && n_files)
    {
      /* Nothing.  */
    }
  else if (n_files)
    {
      char *fn;

      /* If we only have file names as possible completion, we should
	 bring them in sync with what rl_complete expects.  The
	 problem is that if the user types "break /foo/b TAB", and the
	 possible completions are "/foo/bar" and "/foo/baz"
	 rl_complete expects us to return "bar" and "baz", without the
	 leading directories, as possible completions, because `word'
	 starts at the "b".  But we ignore the value of `word' when we
	 call make_source_files_completion_list above (because that
	 would not DTRT when the completion results in both symbols
	 and file names), so make_source_files_completion_list returns
	 the full "/foo/bar" and "/foo/baz" strings.  This produces
	 wrong results when, e.g., there's only one possible
	 completion, because rl_complete will prepend "/foo/" to each
	 candidate completion.  The loop below removes that leading
	 part.  */
      for (ix = 0; VEC_iterate (char_ptr, list, ix, fn); ++ix)
	{
	  memmove (fn, fn + (word - text),
		   strlen (fn) + 1 - (word - text));
	}
    }
  else if (!n_syms)
    {
      /* No completions at all.  As the final resort, try completing
	 on the entire text as a symbol.  */
      list = make_symbol_completion_list (orig_text, word);
    }

  return list;
}

/* A helper function to collect explicit location matches for the given
   LOCATION, which is attempting to match on WORD.  */

static VEC (char_ptr) *
collect_explicit_location_matches (struct event_location *location,
				   enum explicit_location_match_type what,
				   const char *word)
{
  VEC (char_ptr) *matches = NULL;
  const struct explicit_location *explicit_loc
    = get_explicit_location (location);

  switch (what)
    {
    case MATCH_SOURCE:
      {
	const char *text = (explicit_loc->source_filename == NULL
			    ? "" : explicit_loc->source_filename);

	matches = make_source_files_completion_list (text, word);
      }
      break;

    case MATCH_FUNCTION:
      {
	const char *text = (explicit_loc->function_name == NULL
			    ? "" : explicit_loc->function_name);

	if (explicit_loc->source_filename != NULL)
	  {
	    const char *filename = explicit_loc->source_filename;

	    matches = make_file_symbol_completion_list (text, word, filename);
	  }
	else
	  matches = make_symbol_completion_list (text, word);
      }
      break;

    case MATCH_LABEL:
      /* Not supported.  */
      break;

    default:
      gdb_assert_not_reached ("unhandled explicit_location_match_type");
    }

  return matches;
}

/* A convenience macro to (safely) back up P to the previous word.  */

static const char *
backup_text_ptr (const char *p, const char *text)
{
  while (p > text && isspace (*p))
    --p;
  for (; p > text && !isspace (p[-1]); --p)
    ;

  return p;
}

/* A completer function for explicit locations.  This function
   completes both options ("-source", "-line", etc) and values.  */

static VEC (char_ptr) *
explicit_location_completer (struct cmd_list_element *ignore,
			     struct event_location *location,
			     const char *text, const char *word)
{
  const char *p;
  VEC (char_ptr) *matches = NULL;

  /* Find the beginning of the word.  This is necessary because
     we need to know if we are completing an option name or value.  We
     don't get the leading '-' from the completer.  */
  p = backup_text_ptr (word, text);

  if (*p == '-')
    {
      /* Completing on option name.  */
      static const char *const keywords[] =
	{
	  "source",
	  "function",
	  "line",
	  "label",
	  NULL
	};

      /* Skip over the '-'.  */
      ++p;

      return complete_on_enum (keywords, p, p);
    }
  else
    {
      /* Completing on value (or unknown).  Get the previous word to see what
	 the user is completing on.  */
      size_t len, offset;
      const char *new_word, *end;
      enum explicit_location_match_type what;
      struct explicit_location *explicit_loc
	= get_explicit_location (location);

      /* Backup P to the previous word, which should be the option
	 the user is attempting to complete.  */
      offset = word - p;
      end = --p;
      p = backup_text_ptr (p, text);
      len = end - p;

      if (strncmp (p, "-source", len) == 0)
	{
	  what = MATCH_SOURCE;
	  new_word = explicit_loc->source_filename + offset;
	}
      else if (strncmp (p, "-function", len) == 0)
	{
	  what = MATCH_FUNCTION;
	  new_word = explicit_loc->function_name + offset;
	}
      else if (strncmp (p, "-label", len) == 0)
	{
	  what = MATCH_LABEL;
	  new_word = explicit_loc->label_name + offset;
	}
      else
	{
	  /* The user isn't completing on any valid option name,
	     e.g., "break -source foo.c [tab]".  */
	  return NULL;
	}

      /* If the user hasn't entered a search expression, e.g.,
	 "break -function <TAB><TAB>", new_word will be NULL, but
	 search routines require non-NULL search words.  */
      if (new_word == NULL)
	new_word = "";

      /* Now gather matches  */
      matches = collect_explicit_location_matches (location, what, new_word);
    }

  return matches;
}

/* A completer for locations.  */

VEC (char_ptr) *
location_completer (struct cmd_list_element *ignore,
		    const char *text, const char *word)
{
  VEC (char_ptr) *matches = NULL;
  const char *copy = text;
  struct event_location *location;

  location = string_to_explicit_location (&copy, current_language, 1);
  if (location != NULL)
    {
      struct cleanup *cleanup;

      cleanup = make_cleanup_delete_event_location (location);
      matches = explicit_location_completer (ignore, location, text, word);
      do_cleanups (cleanup);
    }
  else
    {
      /* This is an address or linespec location.
	 Right now both of these are handled by the (old) linespec
	 completer.  */
      matches = linespec_location_completer (ignore, text, word);
    }

  return matches;
}

/* Helper for expression_completer which recursively adds field and
   method names from TYPE, a struct or union type, to the array
   OUTPUT.  */
static void
add_struct_fields (struct type *type, VEC (char_ptr) **output,
		   char *fieldname, int namelen)
{
  int i;
  int computed_type_name = 0;
  const char *type_name = NULL;

  type = check_typedef (type);
  for (i = 0; i < TYPE_NFIELDS (type); ++i)
    {
      if (i < TYPE_N_BASECLASSES (type))
	add_struct_fields (TYPE_BASECLASS (type, i),
			   output, fieldname, namelen);
      else if (TYPE_FIELD_NAME (type, i))
	{
	  if (TYPE_FIELD_NAME (type, i)[0] != '\0')
	    {
	      if (! strncmp (TYPE_FIELD_NAME (type, i), 
			     fieldname, namelen))
		VEC_safe_push (char_ptr, *output,
			       xstrdup (TYPE_FIELD_NAME (type, i)));
	    }
	  else if (TYPE_CODE (TYPE_FIELD_TYPE (type, i)) == TYPE_CODE_UNION)
	    {
	      /* Recurse into anonymous unions.  */
	      add_struct_fields (TYPE_FIELD_TYPE (type, i),
				 output, fieldname, namelen);
	    }
	}
    }

  for (i = TYPE_NFN_FIELDS (type) - 1; i >= 0; --i)
    {
      const char *name = TYPE_FN_FIELDLIST_NAME (type, i);

      if (name && ! strncmp (name, fieldname, namelen))
	{
	  if (!computed_type_name)
	    {
	      type_name = type_name_no_tag (type);
	      computed_type_name = 1;
	    }
	  /* Omit constructors from the completion list.  */
	  if (!type_name || strcmp (type_name, name))
	    VEC_safe_push (char_ptr, *output, xstrdup (name));
	}
    }
}

/* Complete on expressions.  Often this means completing on symbol
   names, but some language parsers also have support for completing
   field names.  */
VEC (char_ptr) *
expression_completer (struct cmd_list_element *ignore, 
		      const char *text, const char *word)
{
  struct type *type = NULL;
  char *fieldname;
  const char *p;
  enum type_code code = TYPE_CODE_UNDEF;

  /* Perform a tentative parse of the expression, to see whether a
     field completion is required.  */
  fieldname = NULL;
  TRY
    {
      type = parse_expression_for_completion (text, &fieldname, &code);
    }
  CATCH (except, RETURN_MASK_ERROR)
    {
      return NULL;
    }
  END_CATCH

  if (fieldname && type)
    {
      for (;;)
	{
	  type = check_typedef (type);
	  if (TYPE_CODE (type) != TYPE_CODE_PTR
	      && TYPE_CODE (type) != TYPE_CODE_REF)
	    break;
	  type = TYPE_TARGET_TYPE (type);
	}

      if (TYPE_CODE (type) == TYPE_CODE_UNION
	  || TYPE_CODE (type) == TYPE_CODE_STRUCT)
	{
	  int flen = strlen (fieldname);
	  VEC (char_ptr) *result = NULL;

	  add_struct_fields (type, &result, fieldname, flen);
	  xfree (fieldname);
	  return result;
	}
    }
  else if (fieldname && code != TYPE_CODE_UNDEF)
    {
      VEC (char_ptr) *result;
      struct cleanup *cleanup = make_cleanup (xfree, fieldname);

      result = make_symbol_completion_type (fieldname, fieldname, code);
      do_cleanups (cleanup);
      return result;
    }
  xfree (fieldname);

  /* Commands which complete on locations want to see the entire
     argument.  */
  for (p = word;
       p > text && p[-1] != ' ' && p[-1] != '\t';
       p--)
    ;

  /* Not ideal but it is what we used to do before...  */
  return location_completer (ignore, p, word);
}

/* See definition in completer.h.  */

void
set_gdb_completion_word_break_characters (completer_ftype *fn)
{
  /* So far we are only interested in differentiating filename
     completers from everything else.  */
  if (fn == filename_completer)
    rl_completer_word_break_characters
      = gdb_completer_file_name_break_characters;
  else
    rl_completer_word_break_characters
      = gdb_completer_command_word_break_characters;
}

/* Here are some useful test cases for completion.  FIXME: These
   should be put in the test suite.  They should be tested with both
   M-? and TAB.

   "show output-" "radix"
   "show output" "-radix"
   "p" ambiguous (commands starting with p--path, print, printf, etc.)
   "p "  ambiguous (all symbols)
   "info t foo" no completions
   "info t " no completions
   "info t" ambiguous ("info target", "info terminal", etc.)
   "info ajksdlfk" no completions
   "info ajksdlfk " no completions
   "info" " "
   "info " ambiguous (all info commands)
   "p \"a" no completions (string constant)
   "p 'a" ambiguous (all symbols starting with a)
   "p b-a" ambiguous (all symbols starting with a)
   "p b-" ambiguous (all symbols)
   "file Make" "file" (word break hard to screw up here)
   "file ../gdb.stabs/we" "ird" (needs to not break word at slash)
 */

typedef enum
{
  handle_brkchars,
  handle_completions,
  handle_help
}
complete_line_internal_reason;


/* Internal function used to handle completions.


   TEXT is the caller's idea of the "word" we are looking at.

   LINE_BUFFER is available to be looked at; it contains the entire
   text of the line.  POINT is the offset in that line of the cursor.
   You should pretend that the line ends at POINT.

   REASON is of type complete_line_internal_reason.

   If REASON is handle_brkchars:
   Preliminary phase, called by gdb_completion_word_break_characters
   function, is used to determine the correct set of chars that are
   word delimiters depending on the current command in line_buffer.
   No completion list should be generated; the return value should be
   NULL.  This is checked by an assertion in that function.

   If REASON is handle_completions:
   Main phase, called by complete_line function, is used to get the list
   of posible completions.

   If REASON is handle_help:
   Special case when completing a 'help' command.  In this case,
   once sub-command completions are exhausted, we simply return NULL.
 */

static VEC (char_ptr) *
complete_line_internal (const char *text, 
			const char *line_buffer, int point,
			complete_line_internal_reason reason)
{
  VEC (char_ptr) *list = NULL;
  char *tmp_command;
  const char *p;
  int ignore_help_classes;
  /* Pointer within tmp_command which corresponds to text.  */
  char *word;
  struct cmd_list_element *c, *result_list;

  /* Choose the default set of word break characters to break
     completions.  If we later find out that we are doing completions
     on command strings (as opposed to strings supplied by the
     individual command completer functions, which can be any string)
     then we will switch to the special word break set for command
     strings, which leaves out the '-' character used in some
     commands.  */
  rl_completer_word_break_characters =
    current_language->la_word_break_characters();

  /* Decide whether to complete on a list of gdb commands or on
     symbols.  */
  tmp_command = (char *) alloca (point + 1);
  p = tmp_command;

  /* The help command should complete help aliases.  */
  ignore_help_classes = reason != handle_help;

  strncpy (tmp_command, line_buffer, point);
  tmp_command[point] = '\0';
  /* Since text always contains some number of characters leading up
     to point, we can find the equivalent position in tmp_command
     by subtracting that many characters from the end of tmp_command.  */
  word = tmp_command + point - strlen (text);

  if (point == 0)
    {
      /* An empty line we want to consider ambiguous; that is, it
	 could be any command.  */
      c = CMD_LIST_AMBIGUOUS;
      result_list = 0;
    }
  else
    {
      c = lookup_cmd_1 (&p, cmdlist, &result_list, ignore_help_classes);
    }

  /* Move p up to the next interesting thing.  */
  while (*p == ' ' || *p == '\t')
    {
      p++;
    }

  if (!c)
    {
      /* It is an unrecognized command.  So there are no
	 possible completions.  */
      list = NULL;
    }
  else if (c == CMD_LIST_AMBIGUOUS)
    {
      const char *q;

      /* lookup_cmd_1 advances p up to the first ambiguous thing, but
	 doesn't advance over that thing itself.  Do so now.  */
      q = p;
      while (*q && (isalnum (*q) || *q == '-' || *q == '_'))
	++q;
      if (q != tmp_command + point)
	{
	  /* There is something beyond the ambiguous
	     command, so there are no possible completions.  For
	     example, "info t " or "info t foo" does not complete
	     to anything, because "info t" can be "info target" or
	     "info terminal".  */
	  list = NULL;
	}
      else
	{
	  /* We're trying to complete on the command which was ambiguous.
	     This we can deal with.  */
	  if (result_list)
	    {
	      if (reason != handle_brkchars)
		list = complete_on_cmdlist (*result_list->prefixlist, p,
					    word, ignore_help_classes);
	    }
	  else
	    {
	      if (reason != handle_brkchars)
		list = complete_on_cmdlist (cmdlist, p, word,
					    ignore_help_classes);
	    }
	  /* Ensure that readline does the right thing with respect to
	     inserting quotes.  */
	  rl_completer_word_break_characters =
	    gdb_completer_command_word_break_characters;
	}
    }
  else
    {
      /* We've recognized a full command.  */

      if (p == tmp_command + point)
	{
	  /* There is no non-whitespace in the line beyond the
	     command.  */

	  if (p[-1] == ' ' || p[-1] == '\t')
	    {
	      /* The command is followed by whitespace; we need to
		 complete on whatever comes after command.  */
	      if (c->prefixlist)
		{
		  /* It is a prefix command; what comes after it is
		     a subcommand (e.g. "info ").  */
		  if (reason != handle_brkchars)
		    list = complete_on_cmdlist (*c->prefixlist, p, word,
						ignore_help_classes);

		  /* Ensure that readline does the right thing
		     with respect to inserting quotes.  */
		  rl_completer_word_break_characters =
		    gdb_completer_command_word_break_characters;
		}
	      else if (reason == handle_help)
		list = NULL;
	      else if (c->enums)
		{
		  if (reason != handle_brkchars)
		    list = complete_on_enum (c->enums, p, word);
		  rl_completer_word_break_characters =
		    gdb_completer_command_word_break_characters;
		}
	      else
		{
		  /* It is a normal command; what comes after it is
		     completed by the command's completer function.  */
		  if (c->completer == filename_completer)
		    {
		      /* Many commands which want to complete on
			 file names accept several file names, as
			 in "run foo bar >>baz".  So we don't want
			 to complete the entire text after the
			 command, just the last word.  To this
			 end, we need to find the beginning of the
			 file name by starting at `word' and going
			 backwards.  */
		      for (p = word;
			   p > tmp_command
			     && strchr (gdb_completer_file_name_break_characters, p[-1]) == NULL;
			   p--)
			;
		      rl_completer_word_break_characters =
			gdb_completer_file_name_break_characters;
		    }
		  if (reason == handle_brkchars
		      && c->completer_handle_brkchars != NULL)
		    (*c->completer_handle_brkchars) (c, p, word);
		  if (reason != handle_brkchars && c->completer != NULL)
		    list = (*c->completer) (c, p, word);
		}
	    }
	  else
	    {
	      /* The command is not followed by whitespace; we need to
		 complete on the command itself, e.g. "p" which is a
		 command itself but also can complete to "print", "ptype"
		 etc.  */
	      const char *q;

	      /* Find the command we are completing on.  */
	      q = p;
	      while (q > tmp_command)
		{
		  if (isalnum (q[-1]) || q[-1] == '-' || q[-1] == '_')
		    --q;
		  else
		    break;
		}

	      if (reason != handle_brkchars)
		list = complete_on_cmdlist (result_list, q, word,
					    ignore_help_classes);

	      /* Ensure that readline does the right thing
		 with respect to inserting quotes.  */
	      rl_completer_word_break_characters =
		gdb_completer_command_word_break_characters;
	    }
	}
      else if (reason == handle_help)
	list = NULL;
      else
	{
	  /* There is non-whitespace beyond the command.  */

	  if (c->prefixlist && !c->allow_unknown)
	    {
	      /* It is an unrecognized subcommand of a prefix command,
		 e.g. "info adsfkdj".  */
	      list = NULL;
	    }
	  else if (c->enums)
	    {
	      if (reason != handle_brkchars)
		list = complete_on_enum (c->enums, p, word);
	    }
	  else
	    {
	      /* It is a normal command.  */
	      if (c->completer == filename_completer)
		{
		  /* See the commentary above about the specifics
		     of file-name completion.  */
		  for (p = word;
		       p > tmp_command
			 && strchr (gdb_completer_file_name_break_characters, 
				    p[-1]) == NULL;
		       p--)
		    ;
		  rl_completer_word_break_characters =
		    gdb_completer_file_name_break_characters;
		}
	      if (reason == handle_brkchars
		  && c->completer_handle_brkchars != NULL)
		(*c->completer_handle_brkchars) (c, p, word);
	      if (reason != handle_brkchars && c->completer != NULL)
		list = (*c->completer) (c, p, word);
	    }
	}
    }

  return list;
}

/* See completer.h.  */

int max_completions = 200;

/* See completer.h.  */

completion_tracker_t
new_completion_tracker (void)
{
  if (max_completions <= 0)
    return NULL;

  return htab_create_alloc (max_completions,
			    htab_hash_string, (htab_eq) streq,
			    NULL, xcalloc, xfree);
}

/* Cleanup routine to free a completion tracker and reset the pointer
   to NULL.  */

static void
free_completion_tracker (void *p)
{
  completion_tracker_t *tracker_ptr = (completion_tracker_t *) p;

  htab_delete (*tracker_ptr);
  *tracker_ptr = NULL;
}

/* See completer.h.  */

struct cleanup *
make_cleanup_free_completion_tracker (completion_tracker_t *tracker_ptr)
{
  if (*tracker_ptr == NULL)
    return make_cleanup (null_cleanup, NULL);

  return make_cleanup (free_completion_tracker, tracker_ptr);
}

/* See completer.h.  */

enum maybe_add_completion_enum
maybe_add_completion (completion_tracker_t tracker, char *name)
{
  void **slot;

  if (max_completions < 0)
    return MAYBE_ADD_COMPLETION_OK;
  if (max_completions == 0)
    return MAYBE_ADD_COMPLETION_MAX_REACHED;

  gdb_assert (tracker != NULL);

  if (htab_elements (tracker) >= max_completions)
    return MAYBE_ADD_COMPLETION_MAX_REACHED;

  slot = htab_find_slot (tracker, name, INSERT);

  if (*slot != HTAB_EMPTY_ENTRY)
    return MAYBE_ADD_COMPLETION_DUPLICATE;

  *slot = name;

  return (htab_elements (tracker) < max_completions
	  ? MAYBE_ADD_COMPLETION_OK
	  : MAYBE_ADD_COMPLETION_OK_MAX_REACHED);
}

void
throw_max_completions_reached_error (void)
{
  throw_error (MAX_COMPLETIONS_REACHED_ERROR, _("Max completions reached."));
}

/* Generate completions all at once.  Returns a vector of unique strings
   allocated with xmalloc.  Returns NULL if there are no completions
   or if max_completions is 0.  If max_completions is non-negative, this will
   return at most max_completions strings.

   TEXT is the caller's idea of the "word" we are looking at.

   LINE_BUFFER is available to be looked at; it contains the entire
   text of the line.

   POINT is the offset in that line of the cursor.  You
   should pretend that the line ends at POINT.  */

VEC (char_ptr) *
complete_line (const char *text, const char *line_buffer, int point)
{
  VEC (char_ptr) *list;
  VEC (char_ptr) *result = NULL;
  struct cleanup *cleanups;
  completion_tracker_t tracker;
  char *candidate;
  int ix, max_reached;

  if (max_completions == 0)
    return NULL;
  list = complete_line_internal (text, line_buffer, point,
				 handle_completions);
  if (max_completions < 0)
    return list;

  tracker = new_completion_tracker ();
  cleanups = make_cleanup_free_completion_tracker (&tracker);
  make_cleanup_free_char_ptr_vec (list);

  /* Do a final test for too many completions.  Individual completers may
     do some of this, but are not required to.  Duplicates are also removed
     here.  Otherwise the user is left scratching his/her head: readline and
     complete_command will remove duplicates, and if removal of duplicates
     there brings the total under max_completions the user may think gdb quit
     searching too early.  */

  for (ix = 0, max_reached = 0;
       !max_reached && VEC_iterate (char_ptr, list, ix, candidate);
       ++ix)
    {
      enum maybe_add_completion_enum add_status;

      add_status = maybe_add_completion (tracker, candidate);

      switch (add_status)
	{
	  case MAYBE_ADD_COMPLETION_OK:
	    VEC_safe_push (char_ptr, result, xstrdup (candidate));
	    break;
	  case MAYBE_ADD_COMPLETION_OK_MAX_REACHED:
	    VEC_safe_push (char_ptr, result, xstrdup (candidate));
	    max_reached = 1;
	    break;
      	  case MAYBE_ADD_COMPLETION_MAX_REACHED:
	    gdb_assert_not_reached ("more than max completions reached");
	  case MAYBE_ADD_COMPLETION_DUPLICATE:
	    break;
	}
    }

  do_cleanups (cleanups);

  return result;
}

/* Complete on command names.  Used by "help".  */
VEC (char_ptr) *
command_completer (struct cmd_list_element *ignore, 
		   const char *text, const char *word)
{
  return complete_line_internal (word, text, 
				 strlen (text), handle_help);
}

/* Complete on signals.  */

VEC (char_ptr) *
signal_completer (struct cmd_list_element *ignore,
		  const char *text, const char *word)
{
  VEC (char_ptr) *return_val = NULL;
  size_t len = strlen (word);
  int signum;
  const char *signame;

  for (signum = GDB_SIGNAL_FIRST; signum != GDB_SIGNAL_LAST; ++signum)
    {
      /* Can't handle this, so skip it.  */
      if (signum == GDB_SIGNAL_0)
	continue;

      signame = gdb_signal_to_name ((enum gdb_signal) signum);

      /* Ignore the unknown signal case.  */
      if (!signame || strcmp (signame, "?") == 0)
	continue;

      if (strncasecmp (signame, word, len) == 0)
	VEC_safe_push (char_ptr, return_val, xstrdup (signame));
    }

  return return_val;
}

/* Bit-flags for selecting what the register and/or register-group
   completer should complete on.  */

enum reg_completer_target
  {
    complete_register_names = 0x1,
    complete_reggroup_names = 0x2
  };
DEF_ENUM_FLAGS_TYPE (enum reg_completer_target, reg_completer_targets);

/* Complete register names and/or reggroup names based on the value passed
   in TARGETS.  At least one bit in TARGETS must be set.  */

static VEC (char_ptr) *
reg_or_group_completer_1 (struct cmd_list_element *ignore,
			  const char *text, const char *word,
			  reg_completer_targets targets)
{
  VEC (char_ptr) *result = NULL;
  size_t len = strlen (word);
  struct gdbarch *gdbarch;
  const char *name;

  gdb_assert ((targets & (complete_register_names
			  | complete_reggroup_names)) != 0);
  gdbarch = get_current_arch ();

  if ((targets & complete_register_names) != 0)
    {
      int i;

      for (i = 0;
	   (name = user_reg_map_regnum_to_name (gdbarch, i)) != NULL;
	   i++)
	{
	  if (*name != '\0' && strncmp (word, name, len) == 0)
	    VEC_safe_push (char_ptr, result, xstrdup (name));
	}
    }

  if ((targets & complete_reggroup_names) != 0)
    {
      struct reggroup *group;

      for (group = reggroup_next (gdbarch, NULL);
	   group != NULL;
	   group = reggroup_next (gdbarch, group))
	{
	  name = reggroup_name (group);
	  if (strncmp (word, name, len) == 0)
	    VEC_safe_push (char_ptr, result, xstrdup (name));
	}
    }

  return result;
}

/* Perform completion on register and reggroup names.  */

VEC (char_ptr) *
reg_or_group_completer (struct cmd_list_element *ignore,
			const char *text, const char *word)
{
  return reg_or_group_completer_1 (ignore, text, word,
				   (complete_register_names
				    | complete_reggroup_names));
}

/* Perform completion on reggroup names.  */

VEC (char_ptr) *
reggroup_completer (struct cmd_list_element *ignore,
		    const char *text, const char *word)
{
  return reg_or_group_completer_1 (ignore, text, word,
				   complete_reggroup_names);
}

/* Get the list of chars that are considered as word breaks
   for the current command.  */

char *
gdb_completion_word_break_characters (void)
{
  VEC (char_ptr) *list;

  list = complete_line_internal (rl_line_buffer, rl_line_buffer, rl_point,
				 handle_brkchars);
  gdb_assert (list == NULL);
  return rl_completer_word_break_characters;
}

/* Generate completions one by one for the completer.  Each time we
   are called return another potential completion to the caller.
   line_completion just completes on commands or passes the buck to
   the command's completer function, the stuff specific to symbol
   completion is in make_symbol_completion_list.

   TEXT is the caller's idea of the "word" we are looking at.

   MATCHES is the number of matches that have currently been collected
   from calling this completion function.  When zero, then we need to
   initialize, otherwise the initialization has already taken place
   and we can just return the next potential completion string.

   LINE_BUFFER is available to be looked at; it contains the entire
   text of the line.  POINT is the offset in that line of the cursor.
   You should pretend that the line ends at POINT.

   Returns NULL if there are no more completions, else a pointer to a
   string which is a possible completion, it is the caller's
   responsibility to free the string.  */

static char *
line_completion_function (const char *text, int matches, 
			  char *line_buffer, int point)
{
  static VEC (char_ptr) *list = NULL;	/* Cache of completions.  */
  static int index;			/* Next cached completion.  */
  char *output = NULL;

  if (matches == 0)
    {
      /* The caller is beginning to accumulate a new set of
         completions, so we need to find all of them now, and cache
         them for returning one at a time on future calls.  */

      if (list)
	{
	  /* Free the storage used by LIST, but not by the strings
	     inside.  This is because rl_complete_internal () frees
	     the strings.  As complete_line may abort by calling
	     `error' clear LIST now.  */
	  VEC_free (char_ptr, list);
	}
      index = 0;
      list = complete_line (text, line_buffer, point);
    }

  /* If we found a list of potential completions during initialization
     then dole them out one at a time.  After returning the last one,
     return NULL (and continue to do so) each time we are called after
     that, until a new list is available.  */

  if (list)
    {
      if (index < VEC_length (char_ptr, list))
	{
	  output = VEC_index (char_ptr, list, index);
	  index++;
	}
    }

#if 0
  /* Can't do this because readline hasn't yet checked the word breaks
     for figuring out whether to insert a quote.  */
  if (output == NULL)
    /* Make sure the word break characters are set back to normal for
       the next time that readline tries to complete something.  */
    rl_completer_word_break_characters =
      current_language->la_word_break_characters();
#endif

  return (output);
}

/* Skip over the possibly quoted word STR (as defined by the quote
   characters QUOTECHARS and the word break characters BREAKCHARS).
   Returns pointer to the location after the "word".  If either
   QUOTECHARS or BREAKCHARS is NULL, use the same values used by the
   completer.  */

const char *
skip_quoted_chars (const char *str, const char *quotechars,
		   const char *breakchars)
{
  char quote_char = '\0';
  const char *scan;

  if (quotechars == NULL)
    quotechars = gdb_completer_quote_characters;

  if (breakchars == NULL)
    breakchars = current_language->la_word_break_characters();

  for (scan = str; *scan != '\0'; scan++)
    {
      if (quote_char != '\0')
	{
	  /* Ignore everything until the matching close quote char.  */
	  if (*scan == quote_char)
	    {
	      /* Found matching close quote.  */
	      scan++;
	      break;
	    }
	}
      else if (strchr (quotechars, *scan))
	{
	  /* Found start of a quoted string.  */
	  quote_char = *scan;
	}
      else if (strchr (breakchars, *scan))
	{
	  break;
	}
    }

  return (scan);
}

/* Skip over the possibly quoted word STR (as defined by the quote
   characters and word break characters used by the completer).
   Returns pointer to the location after the "word".  */

const char *
skip_quoted (const char *str)
{
  return skip_quoted_chars (str, NULL, NULL);
}

/* Return a message indicating that the maximum number of completions
   has been reached and that there may be more.  */

const char *
get_max_completions_reached_message (void)
{
  return _("*** List may be truncated, max-completions reached. ***");
}

/* GDB replacement for rl_display_match_list.
   Readline doesn't provide a clean interface for TUI(curses).
   A hack previously used was to send readline's rl_outstream through a pipe
   and read it from the event loop.  Bleah.  IWBN if readline abstracted
   away all the necessary bits, and this is what this code does.  It
   replicates the parts of readline we need and then adds an abstraction
   layer, currently implemented as struct match_list_displayer, so that both
   CLI and TUI can use it.  We copy all this readline code to minimize
   GDB-specific mods to readline.  Once this code performs as desired then
   we can submit it to the readline maintainers.

   N.B. A lot of the code is the way it is in order to minimize differences
   from readline's copy.  */

/* Not supported here.  */
#undef VISIBLE_STATS

#if defined (HANDLE_MULTIBYTE)
#define MB_INVALIDCH(x) ((x) == (size_t)-1 || (x) == (size_t)-2)
#define MB_NULLWCH(x)   ((x) == 0)
#endif

#define ELLIPSIS_LEN	3

/* gdb version of readline/complete.c:get_y_or_n.
   'y' -> returns 1, and 'n' -> returns 0.
   Also supported: space == 'y', RUBOUT == 'n', ctrl-g == start over.
   If FOR_PAGER is non-zero, then also supported are:
   NEWLINE or RETURN -> returns 2, and 'q' -> returns 0.  */

static int
gdb_get_y_or_n (int for_pager, const struct match_list_displayer *displayer)
{
  int c;

  for (;;)
    {
      RL_SETSTATE (RL_STATE_MOREINPUT);
      c = displayer->read_key (displayer);
      RL_UNSETSTATE (RL_STATE_MOREINPUT);

      if (c == 'y' || c == 'Y' || c == ' ')
	return 1;
      if (c == 'n' || c == 'N' || c == RUBOUT)
	return 0;
      if (c == ABORT_CHAR || c < 0)
	{
	  /* Readline doesn't erase_entire_line here, but without it the
	     --More-- prompt isn't erased and neither is the text entered
	     thus far redisplayed.  */
	  displayer->erase_entire_line (displayer);
	  /* Note: The arguments to rl_abort are ignored.  */
	  rl_abort (0, 0);
	}
      if (for_pager && (c == NEWLINE || c == RETURN))
	return 2;
      if (for_pager && (c == 'q' || c == 'Q'))
	return 0;
      displayer->beep (displayer);
    }
}

/* Pager function for tab-completion.
   This is based on readline/complete.c:_rl_internal_pager.
   LINES is the number of lines of output displayed thus far.
   Returns:
   -1 -> user pressed 'n' or equivalent,
   0 -> user pressed 'y' or equivalent,
   N -> user pressed NEWLINE or equivalent and N is LINES - 1.  */

static int
gdb_display_match_list_pager (int lines,
			      const struct match_list_displayer *displayer)
{
  int i;

  displayer->puts (displayer, "--More--");
  displayer->flush (displayer);
  i = gdb_get_y_or_n (1, displayer);
  displayer->erase_entire_line (displayer);
  if (i == 0)
    return -1;
  else if (i == 2)
    return (lines - 1);
  else
    return 0;
}

/* Return non-zero if FILENAME is a directory.
   Based on readline/complete.c:path_isdir.  */

static int
gdb_path_isdir (const char *filename)
{
  struct stat finfo;

  return (stat (filename, &finfo) == 0 && S_ISDIR (finfo.st_mode));
}

/* Return the portion of PATHNAME that should be output when listing
   possible completions.  If we are hacking filename completion, we
   are only interested in the basename, the portion following the
   final slash.  Otherwise, we return what we were passed.  Since
   printing empty strings is not very informative, if we're doing
   filename completion, and the basename is the empty string, we look
   for the previous slash and return the portion following that.  If
   there's no previous slash, we just return what we were passed.

   Based on readline/complete.c:printable_part.  */

static char *
gdb_printable_part (char *pathname)
{
  char *temp, *x;

  if (rl_filename_completion_desired == 0)	/* don't need to do anything */
    return (pathname);

  temp = strrchr (pathname, '/');
#if defined (__MSDOS__)
  if (temp == 0 && ISALPHA ((unsigned char)pathname[0]) && pathname[1] == ':')
    temp = pathname + 1;
#endif

  if (temp == 0 || *temp == '\0')
    return (pathname);
  /* If the basename is NULL, we might have a pathname like '/usr/src/'.
     Look for a previous slash and, if one is found, return the portion
     following that slash.  If there's no previous slash, just return the
     pathname we were passed. */
  else if (temp[1] == '\0')
    {
      for (x = temp - 1; x > pathname; x--)
        if (*x == '/')
          break;
      return ((*x == '/') ? x + 1 : pathname);
    }
  else
    return ++temp;
}

/* Compute width of STRING when displayed on screen by print_filename.
   Based on readline/complete.c:fnwidth.  */

static int
gdb_fnwidth (const char *string)
{
  int width, pos;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps;
  int left, w;
  size_t clen;
  wchar_t wc;

  left = strlen (string) + 1;
  memset (&ps, 0, sizeof (mbstate_t));
#endif

  width = pos = 0;
  while (string[pos])
    {
      if (CTRL_CHAR (string[pos]) || string[pos] == RUBOUT)
	{
	  width += 2;
	  pos++;
	}
      else
	{
#if defined (HANDLE_MULTIBYTE)
	  clen = mbrtowc (&wc, string + pos, left - pos, &ps);
	  if (MB_INVALIDCH (clen))
	    {
	      width++;
	      pos++;
	      memset (&ps, 0, sizeof (mbstate_t));
	    }
	  else if (MB_NULLWCH (clen))
	    break;
	  else
	    {
	      pos += clen;
	      w = wcwidth (wc);
	      width += (w >= 0) ? w : 1;
	    }
#else
	  width++;
	  pos++;
#endif
	}
    }

  return width;
}

/* Print TO_PRINT, one matching completion.
   PREFIX_BYTES is number of common prefix bytes.
   Based on readline/complete.c:fnprint.  */

static int
gdb_fnprint (const char *to_print, int prefix_bytes,
	     const struct match_list_displayer *displayer)
{
  int printed_len, w;
  const char *s;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps;
  const char *end;
  size_t tlen;
  int width;
  wchar_t wc;

  end = to_print + strlen (to_print) + 1;
  memset (&ps, 0, sizeof (mbstate_t));
#endif

  printed_len = 0;

  /* Don't print only the ellipsis if the common prefix is one of the
     possible completions */
  if (to_print[prefix_bytes] == '\0')
    prefix_bytes = 0;

  if (prefix_bytes)
    {
      char ellipsis;

      ellipsis = (to_print[prefix_bytes] == '.') ? '_' : '.';
      for (w = 0; w < ELLIPSIS_LEN; w++)
	displayer->putch (displayer, ellipsis);
      printed_len = ELLIPSIS_LEN;
    }

  s = to_print + prefix_bytes;
  while (*s)
    {
      if (CTRL_CHAR (*s))
        {
          displayer->putch (displayer, '^');
          displayer->putch (displayer, UNCTRL (*s));
          printed_len += 2;
          s++;
#if defined (HANDLE_MULTIBYTE)
	  memset (&ps, 0, sizeof (mbstate_t));
#endif
        }
      else if (*s == RUBOUT)
	{
	  displayer->putch (displayer, '^');
	  displayer->putch (displayer, '?');
	  printed_len += 2;
	  s++;
#if defined (HANDLE_MULTIBYTE)
	  memset (&ps, 0, sizeof (mbstate_t));
#endif
	}
      else
	{
#if defined (HANDLE_MULTIBYTE)
	  tlen = mbrtowc (&wc, s, end - s, &ps);
	  if (MB_INVALIDCH (tlen))
	    {
	      tlen = 1;
	      width = 1;
	      memset (&ps, 0, sizeof (mbstate_t));
	    }
	  else if (MB_NULLWCH (tlen))
	    break;
	  else
	    {
	      w = wcwidth (wc);
	      width = (w >= 0) ? w : 1;
	    }
	  for (w = 0; w < tlen; ++w)
	    displayer->putch (displayer, s[w]);
	  s += tlen;
	  printed_len += width;
#else
	  displayer->putch (displayer, *s);
	  s++;
	  printed_len++;
#endif
	}
    }

  return printed_len;
}

/* Output TO_PRINT to rl_outstream.  If VISIBLE_STATS is defined and we
   are using it, check for and output a single character for `special'
   filenames.  Return the number of characters we output.
   Based on readline/complete.c:print_filename.  */

static int
gdb_print_filename (char *to_print, char *full_pathname, int prefix_bytes,
		    const struct match_list_displayer *displayer)
{
  int printed_len, extension_char, slen, tlen;
  char *s, c, *new_full_pathname, *dn;
  extern int _rl_complete_mark_directories;

  extension_char = 0;
  printed_len = gdb_fnprint (to_print, prefix_bytes, displayer);

#if defined (VISIBLE_STATS)
 if (rl_filename_completion_desired && (rl_visible_stats || _rl_complete_mark_directories))
#else
 if (rl_filename_completion_desired && _rl_complete_mark_directories)
#endif
    {
      /* If to_print != full_pathname, to_print is the basename of the
	 path passed.  In this case, we try to expand the directory
	 name before checking for the stat character. */
      if (to_print != full_pathname)
	{
	  /* Terminate the directory name. */
	  c = to_print[-1];
	  to_print[-1] = '\0';

	  /* If setting the last slash in full_pathname to a NUL results in
	     full_pathname being the empty string, we are trying to complete
	     files in the root directory.  If we pass a null string to the
	     bash directory completion hook, for example, it will expand it
	     to the current directory.  We just want the `/'. */
	  if (full_pathname == 0 || *full_pathname == 0)
	    dn = "/";
	  else if (full_pathname[0] != '/')
	    dn = full_pathname;
	  else if (full_pathname[1] == 0)
	    dn = "//";		/* restore trailing slash to `//' */
	  else if (full_pathname[1] == '/' && full_pathname[2] == 0)
	    dn = "/";		/* don't turn /// into // */
	  else
	    dn = full_pathname;
	  s = tilde_expand (dn);
	  if (rl_directory_completion_hook)
	    (*rl_directory_completion_hook) (&s);

	  slen = strlen (s);
	  tlen = strlen (to_print);
	  new_full_pathname = (char *)xmalloc (slen + tlen + 2);
	  strcpy (new_full_pathname, s);
	  if (s[slen - 1] == '/')
	    slen--;
	  else
	    new_full_pathname[slen] = '/';
	  new_full_pathname[slen] = '/';
	  strcpy (new_full_pathname + slen + 1, to_print);

#if defined (VISIBLE_STATS)
	  if (rl_visible_stats)
	    extension_char = stat_char (new_full_pathname);
	  else
#endif
	  if (gdb_path_isdir (new_full_pathname))
	    extension_char = '/';

	  xfree (new_full_pathname);
	  to_print[-1] = c;
	}
      else
	{
	  s = tilde_expand (full_pathname);
#if defined (VISIBLE_STATS)
	  if (rl_visible_stats)
	    extension_char = stat_char (s);
	  else
#endif
	    if (gdb_path_isdir (s))
	      extension_char = '/';
	}

      xfree (s);
      if (extension_char)
	{
	  displayer->putch (displayer, extension_char);
	  printed_len++;
	}
    }

  return printed_len;
}

/* GDB version of readline/complete.c:complete_get_screenwidth.  */

static int
gdb_complete_get_screenwidth (const struct match_list_displayer *displayer)
{
  /* Readline has other stuff here which it's not clear we need.  */
  return displayer->width;
}

extern int _rl_completion_prefix_display_length;
extern int _rl_print_completions_horizontally;

EXTERN_C int _rl_qsort_string_compare (const void *, const void *);
typedef int QSFUNC (const void *, const void *);

/* GDB version of readline/complete.c:rl_display_match_list.
   See gdb_display_match_list for a description of MATCHES, LEN, MAX.
   Returns non-zero if all matches are displayed.  */

static int
gdb_display_match_list_1 (char **matches, int len, int max,
			  const struct match_list_displayer *displayer)
{
  int count, limit, printed_len, lines, cols;
  int i, j, k, l, common_length, sind;
  char *temp, *t;
  int page_completions = displayer->height != INT_MAX && pagination_enabled;

  /* Find the length of the prefix common to all items: length as displayed
     characters (common_length) and as a byte index into the matches (sind) */
  common_length = sind = 0;
  if (_rl_completion_prefix_display_length > 0)
    {
      t = gdb_printable_part (matches[0]);
      temp = strrchr (t, '/');
      common_length = temp ? gdb_fnwidth (temp) : gdb_fnwidth (t);
      sind = temp ? strlen (temp) : strlen (t);

      if (common_length > _rl_completion_prefix_display_length && common_length > ELLIPSIS_LEN)
	max -= common_length - ELLIPSIS_LEN;
      else
	common_length = sind = 0;
    }

  /* How many items of MAX length can we fit in the screen window? */
  cols = gdb_complete_get_screenwidth (displayer);
  max += 2;
  limit = cols / max;
  if (limit != 1 && (limit * max == cols))
    limit--;

  /* If cols == 0, limit will end up -1 */
  if (cols < displayer->width && limit < 0)
    limit = 1;

  /* Avoid a possible floating exception.  If max > cols,
     limit will be 0 and a divide-by-zero fault will result. */
  if (limit == 0)
    limit = 1;

  /* How many iterations of the printing loop? */
  count = (len + (limit - 1)) / limit;

  /* Watch out for special case.  If LEN is less than LIMIT, then
     just do the inner printing loop.
	   0 < len <= limit  implies  count = 1. */

  /* Sort the items if they are not already sorted. */
  if (rl_ignore_completion_duplicates == 0 && rl_sort_completion_matches)
    qsort (matches + 1, len, sizeof (char *), (QSFUNC *)_rl_qsort_string_compare);

  displayer->crlf (displayer);

  lines = 0;
  if (_rl_print_completions_horizontally == 0)
    {
      /* Print the sorted items, up-and-down alphabetically, like ls. */
      for (i = 1; i <= count; i++)
	{
	  for (j = 0, l = i; j < limit; j++)
	    {
	      if (l > len || matches[l] == 0)
		break;
	      else
		{
		  temp = gdb_printable_part (matches[l]);
		  printed_len = gdb_print_filename (temp, matches[l], sind,
						    displayer);

		  if (j + 1 < limit)
		    for (k = 0; k < max - printed_len; k++)
		      displayer->putch (displayer, ' ');
		}
	      l += count;
	    }
	  displayer->crlf (displayer);
	  lines++;
	  if (page_completions && lines >= (displayer->height - 1) && i < count)
	    {
	      lines = gdb_display_match_list_pager (lines, displayer);
	      if (lines < 0)
		return 0;
	    }
	}
    }
  else
    {
      /* Print the sorted items, across alphabetically, like ls -x. */
      for (i = 1; matches[i]; i++)
	{
	  temp = gdb_printable_part (matches[i]);
	  printed_len = gdb_print_filename (temp, matches[i], sind, displayer);
	  /* Have we reached the end of this line? */
	  if (matches[i+1])
	    {
	      if (i && (limit > 1) && (i % limit) == 0)
		{
		  displayer->crlf (displayer);
		  lines++;
		  if (page_completions && lines >= displayer->height - 1)
		    {
		      lines = gdb_display_match_list_pager (lines, displayer);
		      if (lines < 0)
			return 0;
		    }
		}
	      else
		for (k = 0; k < max - printed_len; k++)
		  displayer->putch (displayer, ' ');
	    }
	}
      displayer->crlf (displayer);
    }

  return 1;
}

/* Utility for displaying completion list matches, used by both CLI and TUI.

   MATCHES is the list of strings, in argv format, LEN is the number of
   strings in MATCHES, and MAX is the length of the longest string in
   MATCHES.  */

void
gdb_display_match_list (char **matches, int len, int max,
			const struct match_list_displayer *displayer)
{
  /* Readline will never call this if complete_line returned NULL.  */
  gdb_assert (max_completions != 0);

  /* complete_line will never return more than this.  */
  if (max_completions > 0)
    gdb_assert (len <= max_completions);

  if (rl_completion_query_items > 0 && len >= rl_completion_query_items)
    {
      char msg[100];

      /* We can't use *query here because they wait for <RET> which is
	 wrong here.  This follows the readline version as closely as possible
	 for compatibility's sake.  See readline/complete.c.  */

      displayer->crlf (displayer);

      xsnprintf (msg, sizeof (msg),
		 "Display all %d possibilities? (y or n)", len);
      displayer->puts (displayer, msg);
      displayer->flush (displayer);

      if (gdb_get_y_or_n (0, displayer) == 0)
	{
	  displayer->crlf (displayer);
	  return;
	}
    }

  if (gdb_display_match_list_1 (matches, len, max, displayer))
    {
      /* Note: MAX_COMPLETIONS may be -1 or zero, but LEN is always > 0.  */
      if (len == max_completions)
	{
	  /* The maximum number of completions has been reached.  Warn the user
	     that there may be more.  */
	  const char *message = get_max_completions_reached_message ();

	  displayer->puts (displayer, message);
	  displayer->crlf (displayer);
	}
    }
}

extern initialize_file_ftype _initialize_completer; /* -Wmissing-prototypes */

void
_initialize_completer (void)
{
  add_setshow_zuinteger_unlimited_cmd ("max-completions", no_class,
				       &max_completions, _("\
Set maximum number of completion candidates."), _("\
Show maximum number of completion candidates."), _("\
Use this to limit the number of candidates considered\n\
during completion.  Specifying \"unlimited\" or -1\n\
disables limiting.  Note that setting either no limit or\n\
a very large limit can make completion slow."),
				       NULL, NULL, &setlist, &showlist);
}
