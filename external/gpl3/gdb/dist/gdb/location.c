/* Data structures and API for event locations in GDB.
   Copyright (C) 2013-2016 Free Software Foundation, Inc.

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
#include "gdb_assert.h"
#include "location.h"
#include "symtab.h"
#include "language.h"
#include "linespec.h"
#include "cli/cli-utils.h"
#include "probe.h"

#include <ctype.h>
#include <string.h>

/* An event location used to set a stop event in the inferior.
   This structure is an amalgam of the various ways
   to specify where a stop event should be set.  */

struct event_location
{
  /* The type of this breakpoint specification.  */
  enum event_location_type type;
#define EL_TYPE(P) (P)->type

  union
  {
    /* A generic "this is a string specification" for a location.
       This representation is used by both "normal" linespecs and
       probes.  */
    char *addr_string;
#define EL_LINESPEC(P) ((P)->u.addr_string)
#define EL_PROBE(P) ((P)->u.addr_string)

    /* An address in the inferior.  */
    CORE_ADDR address;
#define EL_ADDRESS(P) (P)->u.address

    /* An explicit location.  */
    struct explicit_location explicit_loc;
#define EL_EXPLICIT(P) (&((P)->u.explicit_loc))
  } u;

  /* Cached string representation of this location.  This is used, e.g., to
     save stop event locations to file.  Malloc'd.  */
  char *as_string;
#define EL_STRING(P) ((P)->as_string)
};

/* See description in location.h.  */

enum event_location_type
event_location_type (const struct event_location *location)
{
  return EL_TYPE (location);
}

/* See description in location.h.  */

void
initialize_explicit_location (struct explicit_location *explicit_loc)
{
  memset (explicit_loc, 0, sizeof (struct explicit_location));
  explicit_loc->line_offset.sign = LINE_OFFSET_UNKNOWN;
}

/* See description in location.h.  */

struct event_location *
new_linespec_location (char **linespec)
{
  struct event_location *location;

  location = XCNEW (struct event_location);
  EL_TYPE (location) = LINESPEC_LOCATION;
  if (*linespec != NULL)
    {
      char *p;
      char *orig = *linespec;

      linespec_lex_to_end (linespec);
      p = remove_trailing_whitespace (orig, *linespec);
      if ((p - orig) > 0)
	EL_LINESPEC (location) = savestring (orig, p - orig);
    }
  return location;
}

/* See description in location.h.  */

const char *
get_linespec_location (const struct event_location *location)
{
  gdb_assert (EL_TYPE (location) == LINESPEC_LOCATION);
  return EL_LINESPEC (location);
}

/* See description in location.h.  */

struct event_location *
new_address_location (CORE_ADDR addr, const char *addr_string,
		      int addr_string_len)
{
  struct event_location *location;

  location = XCNEW (struct event_location);
  EL_TYPE (location) = ADDRESS_LOCATION;
  EL_ADDRESS (location) = addr;
  if (addr_string != NULL)
    EL_STRING (location) = xstrndup (addr_string, addr_string_len);
  return location;
}

/* See description in location.h.  */

CORE_ADDR
get_address_location (const struct event_location *location)
{
  gdb_assert (EL_TYPE (location) == ADDRESS_LOCATION);
  return EL_ADDRESS (location);
}

/* See description in location.h.  */

const char *
get_address_string_location (const struct event_location *location)
{
  gdb_assert (EL_TYPE (location) == ADDRESS_LOCATION);
  return EL_STRING (location);
}

/* See description in location.h.  */

struct event_location *
new_probe_location (const char *probe)
{
  struct event_location *location;

  location = XCNEW (struct event_location);
  EL_TYPE (location) = PROBE_LOCATION;
  if (probe != NULL)
    EL_PROBE (location) = xstrdup (probe);
  return location;
}

/* See description in location.h.  */

const char *
get_probe_location (const struct event_location *location)
{
  gdb_assert (EL_TYPE (location) == PROBE_LOCATION);
  return EL_PROBE (location);
}

/* See description in location.h.  */

struct event_location *
new_explicit_location (const struct explicit_location *explicit_loc)
{
  struct event_location tmp;

  memset (&tmp, 0, sizeof (struct event_location));
  EL_TYPE (&tmp) = EXPLICIT_LOCATION;
  initialize_explicit_location (EL_EXPLICIT (&tmp));
  if (explicit_loc != NULL)
    {
      if (explicit_loc->source_filename != NULL)
	{
	  EL_EXPLICIT (&tmp)->source_filename
	    = explicit_loc->source_filename;
	}

      if (explicit_loc->function_name != NULL)
	EL_EXPLICIT (&tmp)->function_name
	  = explicit_loc->function_name;

      if (explicit_loc->label_name != NULL)
	EL_EXPLICIT (&tmp)->label_name = explicit_loc->label_name;

      if (explicit_loc->line_offset.sign != LINE_OFFSET_UNKNOWN)
	EL_EXPLICIT (&tmp)->line_offset = explicit_loc->line_offset;
    }

  return copy_event_location (&tmp);
}

/* See description in location.h.  */

struct explicit_location *
get_explicit_location (struct event_location *location)
{
  gdb_assert (EL_TYPE (location) == EXPLICIT_LOCATION);
  return EL_EXPLICIT (location);
}

/* See description in location.h.  */

const struct explicit_location *
get_explicit_location_const (const struct event_location *location)
{
  gdb_assert (EL_TYPE (location) == EXPLICIT_LOCATION);
  return EL_EXPLICIT (location);
}

/* This convenience function returns a malloc'd string which
   represents the location in EXPLICIT_LOC.

   AS_LINESPEC is non-zero if this string should be a linespec.
   Otherwise it will be output in explicit form.  */

static char *
explicit_to_string_internal (int as_linespec,
			     const struct explicit_location *explicit_loc)
{
  struct ui_file *buf;
  char space, *result;
  int need_space = 0;
  struct cleanup *cleanup;

  space = as_linespec ? ':' : ' ';
  buf = mem_fileopen ();
  cleanup = make_cleanup_ui_file_delete (buf);

  if (explicit_loc->source_filename != NULL)
    {
      if (!as_linespec)
	fputs_unfiltered ("-source ", buf);
      fputs_unfiltered (explicit_loc->source_filename, buf);
      need_space = 1;
    }

  if (explicit_loc->function_name != NULL)
    {
      if (need_space)
	fputc_unfiltered (space, buf);
      if (!as_linespec)
	fputs_unfiltered ("-function ", buf);
      fputs_unfiltered (explicit_loc->function_name, buf);
      need_space = 1;
    }

  if (explicit_loc->label_name != NULL)
    {
      if (need_space)
	fputc_unfiltered (space, buf);
      if (!as_linespec)
	fputs_unfiltered ("-label ", buf);
      fputs_unfiltered (explicit_loc->label_name, buf);
      need_space = 1;
    }

  if (explicit_loc->line_offset.sign != LINE_OFFSET_UNKNOWN)
    {
      if (need_space)
	fputc_unfiltered (space, buf);
      if (!as_linespec)
	fputs_unfiltered ("-line ", buf);
      fprintf_filtered (buf, "%s%d",
			(explicit_loc->line_offset.sign == LINE_OFFSET_NONE ? ""
			 : (explicit_loc->line_offset.sign
			    == LINE_OFFSET_PLUS ? "+" : "-")),
			explicit_loc->line_offset.offset);
    }

  result = ui_file_xstrdup (buf, NULL);
  do_cleanups (cleanup);
  return result;
}

/* See description in location.h.  */

char *
explicit_location_to_string (const struct explicit_location *explicit_loc)
{
  return explicit_to_string_internal (0, explicit_loc);
}

/* See description in location.h.  */

char *
explicit_location_to_linespec (const struct explicit_location *explicit_loc)
{
  return explicit_to_string_internal (1, explicit_loc);
}

/* See description in location.h.  */

struct event_location *
copy_event_location (const struct event_location *src)
{
  struct event_location *dst;

  dst = XCNEW (struct event_location);
  EL_TYPE (dst) = EL_TYPE (src);
  if (EL_STRING (src) != NULL)
    EL_STRING (dst) = xstrdup (EL_STRING (src));

  switch (EL_TYPE (src))
    {
    case LINESPEC_LOCATION:
      if (EL_LINESPEC (src) != NULL)
	EL_LINESPEC (dst) = xstrdup (EL_LINESPEC (src));
      break;

    case ADDRESS_LOCATION:
      EL_ADDRESS (dst) = EL_ADDRESS (src);
      break;

    case EXPLICIT_LOCATION:
      if (EL_EXPLICIT (src)->source_filename != NULL)
	EL_EXPLICIT (dst)->source_filename
	  = xstrdup (EL_EXPLICIT (src)->source_filename);

      if (EL_EXPLICIT (src)->function_name != NULL)
	EL_EXPLICIT (dst)->function_name
	  = xstrdup (EL_EXPLICIT (src)->function_name);

      if (EL_EXPLICIT (src)->label_name != NULL)
	EL_EXPLICIT (dst)->label_name = xstrdup (EL_EXPLICIT (src)->label_name);

      EL_EXPLICIT (dst)->line_offset = EL_EXPLICIT (src)->line_offset;
      break;


    case PROBE_LOCATION:
      if (EL_PROBE (src) != NULL)
	EL_PROBE (dst) = xstrdup (EL_PROBE (src));
      break;

    default:
      gdb_assert_not_reached ("unknown event location type");
    }

  return dst;
}

/* A cleanup function for struct event_location.  */

static void
delete_event_location_cleanup (void *data)
{
  struct event_location *location = (struct event_location *) data;

  delete_event_location (location);
}

/* See description in location.h.  */

struct cleanup *
make_cleanup_delete_event_location (struct event_location *location)
{
  return make_cleanup (delete_event_location_cleanup, location);
}

/* See description in location.h.  */

void
delete_event_location (struct event_location *location)
{
  if (location != NULL)
    {
      xfree (EL_STRING (location));

      switch (EL_TYPE (location))
	{
	case LINESPEC_LOCATION:
	  xfree (EL_LINESPEC (location));
	  break;

	case ADDRESS_LOCATION:
	  /* Nothing to do.  */
	  break;

	case EXPLICIT_LOCATION:
	  xfree (EL_EXPLICIT (location)->source_filename);
	  xfree (EL_EXPLICIT (location)->function_name);
	  xfree (EL_EXPLICIT (location)->label_name);
	  break;

	case PROBE_LOCATION:
	  xfree (EL_PROBE (location));
	  break;

	default:
	  gdb_assert_not_reached ("unknown event location type");
	}

      xfree (location);
    }
}

/* See description in location.h.  */

const char *
event_location_to_string (struct event_location *location)
{
  if (EL_STRING (location) == NULL)
    {
      switch (EL_TYPE (location))
	{
	case LINESPEC_LOCATION:
	  if (EL_LINESPEC (location) != NULL)
	    EL_STRING (location) = xstrdup (EL_LINESPEC (location));
	  break;

	case ADDRESS_LOCATION:
	  EL_STRING (location)
	    = xstrprintf ("*%s",
			  core_addr_to_string (EL_ADDRESS (location)));
	  break;

	case EXPLICIT_LOCATION:
	  EL_STRING (location)
	    = explicit_location_to_string (EL_EXPLICIT (location));
	  break;

	case PROBE_LOCATION:
	  EL_STRING (location) = xstrdup (EL_PROBE (location));
	  break;

	default:
	  gdb_assert_not_reached ("unknown event location type");
	}
    }

  return EL_STRING (location);
}

/* A lexer for explicit locations.  This function will advance INP
   past any strings that it lexes.  Returns a malloc'd copy of the
   lexed string or NULL if no lexing was done.  */

static char *
explicit_location_lex_one (const char **inp,
			   const struct language_defn *language)
{
  const char *start = *inp;

  if (*start == '\0')
    return NULL;

  /* If quoted, skip to the ending quote.  */
  if (strchr (get_gdb_linespec_parser_quote_characters (), *start))
    {
      char quote_char = *start;

      /* If the input is not an Ada operator, skip to the matching
	 closing quote and return the string.  */
      if (!(language->la_language == language_ada
	    && quote_char == '\"' && is_ada_operator (start)))
	{
	  const char *end = find_toplevel_char (start + 1, quote_char);

	  if (end == NULL)
	    error (_("Unmatched quote, %s."), start);
	  *inp = end + 1;
	  return savestring (start + 1, *inp - start - 2);
	}
    }

  /* If the input starts with '-' or '+', the string ends with the next
     whitespace or comma.  */
  if (*start == '-' || *start == '+')
    {
      while (*inp[0] != '\0' && *inp[0] != ',' && !isspace (*inp[0]))
	++(*inp);
    }
  else
    {
      /* Handle numbers first, stopping at the next whitespace or ','.  */
      while (isdigit (*inp[0]))
	++(*inp);
      if (*inp[0] == '\0' || isspace (*inp[0]) || *inp[0] == ',')
	return savestring (start, *inp - start);

      /* Otherwise stop at the next occurrence of whitespace, '\0',
	 keyword, or ','.  */
      *inp = start;
      while ((*inp)[0]
	     && (*inp)[0] != ','
	     && !(isspace ((*inp)[0])
		  || linespec_lexer_lex_keyword (&(*inp)[1])))
	{
	  /* Special case: C++ operator,.  */
	  if (language->la_language == language_cplus
	      && strncmp (*inp, "operator", 8) == 0)
	    (*inp) += 8;
	  ++(*inp);
	}
    }

  if (*inp - start > 0)
    return savestring (start, *inp - start);

  return NULL;
}

/* See description in location.h.  */

struct event_location *
string_to_explicit_location (const char **argp,
			     const struct language_defn *language,
			     int dont_throw)
{
  struct cleanup *cleanup;
  struct event_location *location;

  /* It is assumed that input beginning with '-' and a non-digit
     character is an explicit location.  "-p" is reserved, though,
     for probe locations.  */
  if (argp == NULL
      || *argp == NULL
      || *argp[0] != '-'
      || !isalpha ((*argp)[1])
      || ((*argp)[0] == '-' && (*argp)[1] == 'p'))
    return NULL;

  location = new_explicit_location (NULL);
  cleanup = make_cleanup_delete_event_location (location);

  /* Process option/argument pairs.  dprintf_command
     requires that processing stop on ','.  */
  while ((*argp)[0] != '\0' && (*argp)[0] != ',')
    {
      int len;
      char *opt, *oarg;
      const char *start;
      struct cleanup *opt_cleanup, *oarg_cleanup;

      /* If *ARGP starts with a keyword, stop processing
	 options.  */
      if (linespec_lexer_lex_keyword (*argp) != NULL)
	break;

      /* Mark the start of the string in case we need to rewind.  */
      start = *argp;

      /* Get the option string.  */
      opt = explicit_location_lex_one (argp, language);
      opt_cleanup = make_cleanup (xfree, opt);

      *argp = skip_spaces_const (*argp);

      /* Get the argument string.  */
      oarg = explicit_location_lex_one (argp, language);
      oarg_cleanup = make_cleanup (xfree, oarg);
      *argp = skip_spaces_const (*argp);

      /* Use the length of the option to allow abbreviations.  */
      len = strlen (opt);

      /* All options have a required argument.  Checking for this required
	 argument is deferred until later.  */
      if (strncmp (opt, "-source", len) == 0)
	EL_EXPLICIT (location)->source_filename = oarg;
      else if (strncmp (opt, "-function", len) == 0)
	EL_EXPLICIT (location)->function_name = oarg;
      else if (strncmp (opt, "-line", len) == 0)
	{
	  if (oarg != NULL)
	    {
	      EL_EXPLICIT (location)->line_offset
		= linespec_parse_line_offset (oarg);
	      do_cleanups (oarg_cleanup);
	      do_cleanups (opt_cleanup);
	      continue;
	    }
	}
      else if (strncmp (opt, "-label", len) == 0)
	EL_EXPLICIT (location)->label_name = oarg;
      /* Only emit an "invalid argument" error for options
	 that look like option strings.  */
      else if (opt[0] == '-' && !isdigit (opt[1]))
	{
	  if (!dont_throw)
	    error (_("invalid explicit location argument, \"%s\""), opt);
	}
      else
	{
	  /* End of the explicit location specification.
	     Stop parsing and return whatever explicit location was
	     parsed.  */
	  *argp = start;
	  discard_cleanups (oarg_cleanup);
	  do_cleanups (opt_cleanup);
	  discard_cleanups (cleanup);
	  return location;
	}

      /* It's a little lame to error after the fact, but in this
	 case, it provides a much better user experience to issue
	 the "invalid argument" error before any missing
	 argument error.  */
      if (oarg == NULL && !dont_throw)
	error (_("missing argument for \"%s\""), opt);

      /* The option/argument pair was successfully processed;
	 oarg belongs to the explicit location, and opt should
	 be freed.  */
      discard_cleanups (oarg_cleanup);
      do_cleanups (opt_cleanup);
    }

  /* One special error check:  If a source filename was given
     without offset, function, or label, issue an error.  */
  if (EL_EXPLICIT (location)->source_filename != NULL
      && EL_EXPLICIT (location)->function_name == NULL
      && EL_EXPLICIT (location)->label_name == NULL
      && (EL_EXPLICIT (location)->line_offset.sign == LINE_OFFSET_UNKNOWN)
      && !dont_throw)
    {
      error (_("Source filename requires function, label, or "
	       "line offset."));
    }

  discard_cleanups (cleanup);
  return location;
}

/* See description in location.h.  */

struct event_location *
string_to_event_location_basic (char **stringp,
				const struct language_defn *language)
{
  struct event_location *location;
  const char *cs;

  /* Try the input as a probe spec.  */
  cs = *stringp;
  if (cs != NULL && probe_linespec_to_ops (&cs) != NULL)
    {
      location = new_probe_location (*stringp);
      *stringp += strlen (*stringp);
    }
  else
    {
      /* Try an address location.  */
      if (*stringp != NULL && **stringp == '*')
	{
	  const char *arg, *orig;
	  CORE_ADDR addr;

	  orig = arg = *stringp;
	  addr = linespec_expression_to_pc (&arg);
	  location = new_address_location (addr, orig, arg - orig);
	  *stringp += arg - orig;
	}
      else
	{
	  /* Everything else is a linespec.  */
	  location = new_linespec_location (stringp);
	}
    }

  return location;
}

/* See description in location.h.  */

struct event_location *
string_to_event_location (char **stringp,
			  const struct language_defn *language)
{
  struct event_location *location;
  const char *arg, *orig;

  /* Try an explicit location.  */
  orig = arg = *stringp;
  location = string_to_explicit_location (&arg, language, 0);
  if (location != NULL)
    {
      /* It was a valid explicit location.  Advance STRINGP to
	 the end of input.  */
      *stringp += arg - orig;
    }
  else
    {
      /* Everything else is a "basic" linespec, address, or probe
	 location.  */
      location = string_to_event_location_basic (stringp, language);
    }

  return location;
}

/* See description in location.h.  */

int
event_location_empty_p (const struct event_location *location)
{
  switch (EL_TYPE (location))
    {
    case LINESPEC_LOCATION:
      /* Linespecs are never "empty."  (NULL is a valid linespec)  */
      return 0;

    case ADDRESS_LOCATION:
      return 0;

    case EXPLICIT_LOCATION:
      return (EL_EXPLICIT (location) == NULL
	      || (EL_EXPLICIT (location)->source_filename == NULL
		  && EL_EXPLICIT (location)->function_name == NULL
		  && EL_EXPLICIT (location)->label_name == NULL
		  && (EL_EXPLICIT (location)->line_offset.sign
		      == LINE_OFFSET_UNKNOWN)));

    case PROBE_LOCATION:
      return EL_PROBE (location) == NULL;

    default:
      gdb_assert_not_reached ("unknown event location type");
    }
}

/* See description in location.h.  */

void
set_event_location_string (struct event_location *location,
			   const char *string)
{
  xfree (EL_STRING (location));
  EL_STRING (location) = string == NULL ?  NULL : xstrdup (string);
}
