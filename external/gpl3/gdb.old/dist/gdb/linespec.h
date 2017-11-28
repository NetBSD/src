/* Header for GDB line completion.
   Copyright (C) 2000-2016 Free Software Foundation, Inc.

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

#if !defined (LINESPEC_H)
#define LINESPEC_H 1

struct symtab;

#include "vec.h"

/* Flags to pass to decode_line_1 and decode_line_full.  */

enum decode_line_flags
  {
    /* Set this flag if you want the resulting SALs to describe the
       first line of indicated functions.  */
    DECODE_LINE_FUNFIRSTLINE = 1,

    /* Set this flag if you want "list mode".  In this mode, a
       FILE:LINE linespec will always return a result, and such
       linespecs will not be expanded to all matches.  */
    DECODE_LINE_LIST_MODE = 2
  };

/* decode_line_full returns a vector of these.  */

struct linespec_sals
{
  /* This is the location corresponding to the sals contained in this
     object.  It can be passed as the FILTER argument to future calls
     to decode_line_full.  This is freed by
     destroy_linespec_result.  */
  char *canonical;

  /* Sals.  The 'sals' field is destroyed by
     destroy_linespec_result.  */
  struct symtabs_and_lines sals;
};

typedef struct linespec_sals linespec_sals;
DEF_VEC_O (linespec_sals);

/* An instance of this may be filled in by decode_line_1.  The caller
   must call init_linespec_result to initialize it and
   destroy_linespec_result to destroy it.  The caller must make copies
   of any data that it needs to keep.  */

struct linespec_result
{
  /* If non-zero, the linespec should be displayed to the user.  This
     is used by "unusual" linespecs where the ordinary `info break'
     display mechanism would do the wrong thing.  */
  int special_display;

  /* If non-zero, the linespec result should be considered to be a
     "pre-expanded" multi-location linespec.  A pre-expanded linespec
     holds all matching locations in a single linespec_sals
     object.  */
  int pre_expanded;

  /* If PRE_EXPANDED is non-zero, this is set to the location entered
     by the user.  This will be freed by destroy_linespec_result.  */
  struct event_location *location;

  /* The sals.  The vector will be freed by
     destroy_linespec_result.  */
  VEC (linespec_sals) *sals;
};

/* Initialize a linespec_result.  */

extern void init_linespec_result (struct linespec_result *);

/* Destroy a linespec_result.  */

extern void destroy_linespec_result (struct linespec_result *);

/* Return a cleanup that destroys a linespec_result.  */

extern struct cleanup *
        make_cleanup_destroy_linespec_result (struct linespec_result *);

/* Decode a linespec using the provided default symtab and line.  */

extern struct symtabs_and_lines
	decode_line_1 (const struct event_location *location, int flags,
		       struct program_space *search_pspace,
		       struct symtab *default_symtab, int default_line);

/* Parse LOCATION and return results.  This is the "full"
   interface to this module, which handles multiple results
   properly.

   For FLAGS, see decode_line_flags.  DECODE_LINE_LIST_MODE is not
   valid for this function.

   If SEARCH_PSPACE is not NULL, symbol search is restricted to just
   that program space.

   DEFAULT_SYMTAB and DEFAULT_LINE describe the default location.
   DEFAULT_SYMTAB can be NULL, in which case the current symtab and
   line are used.

   CANONICAL is where the results are stored.  It must not be NULL.

   SELECT_MODE must be one of the multiple_symbols_* constants, or
   NULL.  It determines how multiple results will be handled.  If
   NULL, the appropriate CLI value will be used.

   FILTER can either be NULL or a string holding a canonical name.
   This is only valid when SELECT_MODE is multiple_symbols_all.

   Multiple results are handled differently depending on the
   arguments:

   . With multiple_symbols_cancel, an exception is thrown.

   . With multiple_symbols_ask, a menu is presented to the user.  The
   user may select none, in which case an exception is thrown; or all,
   which is handled like multiple_symbols_all, below.  Otherwise,
   CANONICAL->SALS will have one entry for each name the user chose.

   . With multiple_symbols_all, CANONICAL->SALS will have a single
   entry describing all the matching locations.  If FILTER is
   non-NULL, then only locations whose canonical name is equal (in the
   strcmp sense) to FILTER will be returned; all others will be
   filtered out.  */

extern void decode_line_full (const struct event_location *location, int flags,
			      struct program_space *search_pspace,
			      struct symtab *default_symtab, int default_line,
			      struct linespec_result *canonical,
			      const char *select_mode,
			      const char *filter);

/* Given a string, return the line specified by it, using the current
   source symtab and line as defaults.
   This is for commands like "list" and "breakpoint".  */

extern struct symtabs_and_lines decode_line_with_current_source (char *, int);

/* Given a string, return the line specified by it, using the last displayed
   codepoint's values as defaults, or nothing if they aren't valid.  */

extern struct symtabs_and_lines decode_line_with_last_displayed (char *, int);

/* Does P represent one of the keywords?  If so, return
   the keyword.  If not, return NULL.  */

extern const char *linespec_lexer_lex_keyword (const char *p);

/* Parse a line offset from STRING.  */

extern struct line_offset linespec_parse_line_offset (const char *string);

/* Return the quote characters permitted by the linespec parser.  */

extern const char *get_gdb_linespec_parser_quote_characters (void);

/* Does STRING represent an Ada operator?  If so, return the length
   of the decoded operator name.  If not, return 0.  */

extern int is_ada_operator (const char *string);

/* Find an instance of the character C in the string S that is outside
   of all parenthesis pairs, single-quoted strings, and double-quoted
   strings.  Also, ignore the char within a template name, like a ','
   within foo<int, int>.  */

extern const char *find_toplevel_char (const char *s, char c);

/* Find the end of the (first) linespec pointed to by *STRINGP.
   STRINGP will be advanced to this point.  */

extern void linespec_lex_to_end (char **stringp);

/* Evaluate the expression pointed to by EXP_PTR into a CORE_ADDR,
   advancing EXP_PTR past any parsed text.  */

extern CORE_ADDR linespec_expression_to_pc (const char **exp_ptr);
#endif /* defined (LINESPEC_H) */
