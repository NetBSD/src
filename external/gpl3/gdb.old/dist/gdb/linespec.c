/* Parser for linespec for the GNU debugger, GDB.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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
#include "frame.h"
#include "command.h"
#include "symfile.h"
#include "objfiles.h"
#include "source.h"
#include "demangle.h"
#include "value.h"
#include "completer.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "parser-defs.h"
#include "block.h"
#include "objc-lang.h"
#include "linespec.h"
#include "language.h"
#include "interps.h"
#include "mi/mi-cmds.h"
#include "target.h"
#include "arch-utils.h"
#include <ctype.h>
#include "cli/cli-utils.h"
#include "filenames.h"
#include "ada-lang.h"
#include "stack.h"
#include "location.h"
#include "common/function-view.h"

typedef struct symbol *symbolp;
DEF_VEC_P (symbolp);

typedef struct type *typep;
DEF_VEC_P (typep);

/* An address entry is used to ensure that any given location is only
   added to the result a single time.  It holds an address and the
   program space from which the address came.  */

struct address_entry
{
  struct program_space *pspace;
  CORE_ADDR addr;
};

typedef struct bound_minimal_symbol bound_minimal_symbol_d;

DEF_VEC_O (bound_minimal_symbol_d);

/* A linespec.  Elements of this structure are filled in by a parser
   (either parse_linespec or some other function).  The structure is
   then converted into SALs by convert_linespec_to_sals.  */

struct linespec
{
  /* An explicit location describing the SaLs.  */
  struct explicit_location explicit_loc;

  /* The list of symtabs to search to which to limit the search.  May not
     be NULL.  If explicit.SOURCE_FILENAME is NULL (no user-specified
     filename), FILE_SYMTABS should contain one single NULL member.  This
     will cause the code to use the default symtab.  */
  VEC (symtab_ptr) *file_symtabs;

  /* A list of matching function symbols and minimal symbols.  Both lists
     may be NULL if no matching symbols were found.  */
  VEC (symbolp) *function_symbols;
  VEC (bound_minimal_symbol_d) *minimal_symbols;

  /* A structure of matching label symbols and the corresponding
     function symbol in which the label was found.  Both may be NULL
     or both must be non-NULL.  */
  struct
  {
    VEC (symbolp) *label_symbols;
    VEC (symbolp) *function_symbols;
  } labels;
};
typedef struct linespec *linespec_p;

/* A canonical linespec represented as a symtab-related string.

   Each entry represents the "SYMTAB:SUFFIX" linespec string.
   SYMTAB can be converted for example by symtab_to_fullname or
   symtab_to_filename_for_display as needed.  */

struct linespec_canonical_name
{
  /* Remaining text part of the linespec string.  */
  char *suffix;

  /* If NULL then SUFFIX is the whole linespec string.  */
  struct symtab *symtab;
};

/* An instance of this is used to keep all state while linespec
   operates.  This instance is passed around as a 'this' pointer to
   the various implementation methods.  */

struct linespec_state
{
  /* The language in use during linespec processing.  */
  const struct language_defn *language;

  /* The program space as seen when the module was entered.  */
  struct program_space *program_space;

  /* If not NULL, the search is restricted to just this program
     space.  */
  struct program_space *search_pspace;

  /* The default symtab to use, if no other symtab is specified.  */
  struct symtab *default_symtab;

  /* The default line to use.  */
  int default_line;

  /* The 'funfirstline' value that was passed in to decode_line_1 or
     decode_line_full.  */
  int funfirstline;

  /* Nonzero if we are running in 'list' mode; see decode_line_list.  */
  int list_mode;

  /* The 'canonical' value passed to decode_line_full, or NULL.  */
  struct linespec_result *canonical;

  /* Canonical strings that mirror the symtabs_and_lines result.  */
  struct linespec_canonical_name *canonical_names;

  /* This is a set of address_entry objects which is used to prevent
     duplicate symbols from being entered into the result.  */
  htab_t addr_set;

  /* Are we building a linespec?  */
  int is_linespec;
};

/* This is a helper object that is used when collecting symbols into a
   result.  */

struct collect_info
{
  /* The linespec object in use.  */
  struct linespec_state *state;

  /* A list of symtabs to which to restrict matches.  */
  VEC (symtab_ptr) *file_symtabs;

  /* The result being accumulated.  */
  struct
  {
    VEC (symbolp) *symbols;
    VEC (bound_minimal_symbol_d) *minimal_symbols;
  } result;

  /* Possibly add a symbol to the results.  */
  bool add_symbol (symbol *sym);
};

bool
collect_info::add_symbol (symbol *sym)
{
  /* In list mode, add all matching symbols, regardless of class.
     This allows the user to type "list a_global_variable".  */
  if (SYMBOL_CLASS (sym) == LOC_BLOCK || this->state->list_mode)
    VEC_safe_push (symbolp, this->result.symbols, sym);

  /* Continue iterating.  */
  return true;
}

/* Token types  */

enum ls_token_type
{
  /* A keyword  */
  LSTOKEN_KEYWORD = 0,

  /* A colon "separator"  */
  LSTOKEN_COLON,

  /* A string  */
  LSTOKEN_STRING,

  /* A number  */
  LSTOKEN_NUMBER,

  /* A comma  */
  LSTOKEN_COMMA,

  /* EOI (end of input)  */
  LSTOKEN_EOI,

  /* Consumed token  */
  LSTOKEN_CONSUMED
};
typedef enum ls_token_type linespec_token_type;

/* List of keywords  */

static const char * const linespec_keywords[] = { "if", "thread", "task" };
#define IF_KEYWORD_INDEX 0

/* A token of the linespec lexer  */

struct ls_token
{
  /* The type of the token  */
  linespec_token_type type;

  /* Data for the token  */
  union
  {
    /* A string, given as a stoken  */
    struct stoken string;

    /* A keyword  */
    const char *keyword;
  } data;
};
typedef struct ls_token linespec_token;

#define LS_TOKEN_STOKEN(TOK) (TOK).data.string
#define LS_TOKEN_KEYWORD(TOK) (TOK).data.keyword

/* An instance of the linespec parser.  */

struct ls_parser
{
  /* Lexer internal data  */
  struct
  {
    /* Save head of input stream.  */
    const char *saved_arg;

    /* Head of the input stream.  */
    const char *stream;
#define PARSER_STREAM(P) ((P)->lexer.stream)

    /* The current token.  */
    linespec_token current;
  } lexer;

  /* Is the entire linespec quote-enclosed?  */
  int is_quote_enclosed;

  /* The state of the parse.  */
  struct linespec_state state;
#define PARSER_STATE(PPTR) (&(PPTR)->state)

  /* The result of the parse.  */
  struct linespec result;
#define PARSER_RESULT(PPTR) (&(PPTR)->result)
};
typedef struct ls_parser linespec_parser;

/* A convenience macro for accessing the explicit location result of
   the parser.  */
#define PARSER_EXPLICIT(PPTR) (&PARSER_RESULT ((PPTR))->explicit_loc)

/* Prototypes for local functions.  */

static void iterate_over_file_blocks
  (struct symtab *symtab, const char *name, domain_enum domain,
   gdb::function_view<symbol_found_callback_ftype> callback);

static void initialize_defaults (struct symtab **default_symtab,
				 int *default_line);

CORE_ADDR linespec_expression_to_pc (const char **exp_ptr);

static struct symtabs_and_lines decode_objc (struct linespec_state *self,
					     linespec_p ls,
					     const char *arg);

static VEC (symtab_ptr) *symtabs_from_filename (const char *,
						struct program_space *pspace);

static VEC (symbolp) *find_label_symbols (struct linespec_state *self,
					  VEC (symbolp) *function_symbols,
					  VEC (symbolp) **label_funcs_ret,
					  const char *name);

static void find_linespec_symbols (struct linespec_state *self,
				   VEC (symtab_ptr) *file_symtabs,
				   const char *name,
				   VEC (symbolp) **symbols,
				   VEC (bound_minimal_symbol_d) **minsyms);

static struct line_offset
     linespec_parse_variable (struct linespec_state *self,
			      const char *variable);

static int symbol_to_sal (struct symtab_and_line *result,
			  int funfirstline, struct symbol *sym);

static void add_matching_symbols_to_info (const char *name,
					  struct collect_info *info,
					  struct program_space *pspace);

static void add_all_symbol_names_from_pspace (struct collect_info *info,
					      struct program_space *pspace,
					      VEC (const_char_ptr) *names);

static VEC (symtab_ptr) *
  collect_symtabs_from_filename (const char *file,
				 struct program_space *pspace);

static void decode_digits_ordinary (struct linespec_state *self,
				    linespec_p ls,
				    int line,
				    struct symtabs_and_lines *sals,
				    struct linetable_entry **best_entry);

static void decode_digits_list_mode (struct linespec_state *self,
				     linespec_p ls,
				     struct symtabs_and_lines *values,
				     struct symtab_and_line val);

static void minsym_found (struct linespec_state *self, struct objfile *objfile,
			  struct minimal_symbol *msymbol,
			  struct symtabs_and_lines *result);

static int compare_symbols (const void *a, const void *b);

static int compare_msymbols (const void *a, const void *b);

/* Permitted quote characters for the parser.  This is different from the
   completer's quote characters to allow backward compatibility with the
   previous parser.  */
static const char *const linespec_quote_characters = "\"\'";

/* Lexer functions.  */

/* Lex a number from the input in PARSER.  This only supports
   decimal numbers.

   Return true if input is decimal numbers.  Return false if not.  */

static int
linespec_lexer_lex_number (linespec_parser *parser, linespec_token *tokenp)
{
  tokenp->type = LSTOKEN_NUMBER;
  LS_TOKEN_STOKEN (*tokenp).length = 0;
  LS_TOKEN_STOKEN (*tokenp).ptr = PARSER_STREAM (parser);

  /* Keep any sign at the start of the stream.  */
  if (*PARSER_STREAM (parser) == '+' || *PARSER_STREAM (parser) == '-')
    {
      ++LS_TOKEN_STOKEN (*tokenp).length;
      ++(PARSER_STREAM (parser));
    }

  while (isdigit (*PARSER_STREAM (parser)))
    {
      ++LS_TOKEN_STOKEN (*tokenp).length;
      ++(PARSER_STREAM (parser));
    }

  /* If the next character in the input buffer is not a space, comma,
     quote, or colon, this input does not represent a number.  */
  if (*PARSER_STREAM (parser) != '\0'
      && !isspace (*PARSER_STREAM (parser)) && *PARSER_STREAM (parser) != ','
      && *PARSER_STREAM (parser) != ':'
      && !strchr (linespec_quote_characters, *PARSER_STREAM (parser)))
    {
      PARSER_STREAM (parser) = LS_TOKEN_STOKEN (*tokenp).ptr;
      return 0;
    }

  return 1;
}

/* See linespec.h.  */

const char *
linespec_lexer_lex_keyword (const char *p)
{
  int i;

  if (p != NULL)
    {
      for (i = 0; i < ARRAY_SIZE (linespec_keywords); ++i)
	{
	  int len = strlen (linespec_keywords[i]);

	  /* If P begins with one of the keywords and the next
	     character is whitespace, we may have found a keyword.
	     It is only a keyword if it is not followed by another
	     keyword.  */
	  if (strncmp (p, linespec_keywords[i], len) == 0
	      && isspace (p[len]))
	    {
	      int j;

	      /* Special case: "if" ALWAYS stops the lexer, since it
		 is not possible to predict what is going to appear in
		 the condition, which can only be parsed after SaLs have
		 been found.  */
	      if (i != IF_KEYWORD_INDEX)
		{
		  p += len;
		  p = skip_spaces_const (p);
		  for (j = 0; j < ARRAY_SIZE (linespec_keywords); ++j)
		    {
		      int nextlen = strlen (linespec_keywords[j]);

		      if (strncmp (p, linespec_keywords[j], nextlen) == 0
			  && isspace (p[nextlen]))
			return NULL;
		    }
		}

	      return linespec_keywords[i];
	    }
	}
    }

  return NULL;
}

/*  See description in linespec.h.  */

int
is_ada_operator (const char *string)
{
  const struct ada_opname_map *mapping;

  for (mapping = ada_opname_table;
       mapping->encoded != NULL
	 && !startswith (string, mapping->decoded); ++mapping)
    ;

  return mapping->decoded == NULL ? 0 : strlen (mapping->decoded);
}

/* Find QUOTE_CHAR in STRING, accounting for the ':' terminal.  Return
   the location of QUOTE_CHAR, or NULL if not found.  */

static const char *
skip_quote_char (const char *string, char quote_char)
{
  const char *p, *last;

  p = last = find_toplevel_char (string, quote_char);
  while (p && *p != '\0' && *p != ':')
    {
      p = find_toplevel_char (p, quote_char);
      if (p != NULL)
	last = p++;
    }

  return last;
}

/* Make a writable copy of the string given in TOKEN, trimming
   any trailing whitespace.  */

static char *
copy_token_string (linespec_token token)
{
  char *str, *s;

  if (token.type == LSTOKEN_KEYWORD)
    return xstrdup (LS_TOKEN_KEYWORD (token));

  str = savestring (LS_TOKEN_STOKEN (token).ptr,
		    LS_TOKEN_STOKEN (token).length);
  s = remove_trailing_whitespace (str, str + LS_TOKEN_STOKEN (token).length);
  *s = '\0';

  return str;
}

/* Does P represent the end of a quote-enclosed linespec?  */

static int
is_closing_quote_enclosed (const char *p)
{
  if (strchr (linespec_quote_characters, *p))
    ++p;
  p = skip_spaces ((char *) p);
  return (*p == '\0' || linespec_lexer_lex_keyword (p));
}

/* Find the end of the parameter list that starts with *INPUT.
   This helper function assists with lexing string segments
   which might contain valid (non-terminating) commas.  */

static const char *
find_parameter_list_end (const char *input)
{
  char end_char, start_char;
  int depth;
  const char *p;

  start_char = *input;
  if (start_char == '(')
    end_char = ')';
  else if (start_char == '<')
    end_char = '>';
  else
    return NULL;

  p = input;
  depth = 0;
  while (*p)
    {
      if (*p == start_char)
	++depth;
      else if (*p == end_char)
	{
	  if (--depth == 0)
	    {
	      ++p;
	      break;
	    }
	}
      ++p;
    }

  return p;
}


/* Lex a string from the input in PARSER.  */

static linespec_token
linespec_lexer_lex_string (linespec_parser *parser)
{
  linespec_token token;
  const char *start = PARSER_STREAM (parser);

  token.type = LSTOKEN_STRING;

  /* If the input stream starts with a quote character, skip to the next
     quote character, regardless of the content.  */
  if (strchr (linespec_quote_characters, *PARSER_STREAM (parser)))
    {
      const char *end;
      char quote_char = *PARSER_STREAM (parser);

      /* Special case: Ada operators.  */
      if (PARSER_STATE (parser)->language->la_language == language_ada
	  && quote_char == '\"')
	{
	  int len = is_ada_operator (PARSER_STREAM (parser));

	  if (len != 0)
	    {
	      /* The input is an Ada operator.  Return the quoted string
		 as-is.  */
	      LS_TOKEN_STOKEN (token).ptr = PARSER_STREAM (parser);
	      LS_TOKEN_STOKEN (token).length = len;
	      PARSER_STREAM (parser) += len;
	      return token;
	    }

	  /* The input does not represent an Ada operator -- fall through
	     to normal quoted string handling.  */
	}

      /* Skip past the beginning quote.  */
      ++(PARSER_STREAM (parser));

      /* Mark the start of the string.  */
      LS_TOKEN_STOKEN (token).ptr = PARSER_STREAM (parser);

      /* Skip to the ending quote.  */
      end = skip_quote_char (PARSER_STREAM (parser), quote_char);

      /* Error if the input did not terminate properly.  */
      if (end == NULL)
	error (_("unmatched quote"));

      /* Skip over the ending quote and mark the length of the string.  */
      PARSER_STREAM (parser) = (char *) ++end;
      LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - 2 - start;
    }
  else
    {
      const char *p;

      /* Otherwise, only identifier characters are permitted.
	 Spaces are the exception.  In general, we keep spaces,
	 but only if the next characters in the input do not resolve
	 to one of the keywords.

	 This allows users to forgo quoting CV-qualifiers, template arguments,
	 and similar common language constructs.  */

      while (1)
	{
	  if (isspace (*PARSER_STREAM (parser)))
	    {
	      p = skip_spaces_const (PARSER_STREAM (parser));
	      /* When we get here we know we've found something followed by
		 a space (we skip over parens and templates below).
		 So if we find a keyword now, we know it is a keyword and not,
		 say, a function name.  */
	      if (linespec_lexer_lex_keyword (p) != NULL)
		{
		  LS_TOKEN_STOKEN (token).ptr = start;
		  LS_TOKEN_STOKEN (token).length
		    = PARSER_STREAM (parser) - start;
		  return token;
		}

	      /* Advance past the whitespace.  */
	      PARSER_STREAM (parser) = p;
	    }

	  /* If the next character is EOI or (single) ':', the
	     string is complete;  return the token.  */
	  if (*PARSER_STREAM (parser) == 0)
	    {
	      LS_TOKEN_STOKEN (token).ptr = start;
	      LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - start;
	      return token;
	    }
	  else if (PARSER_STREAM (parser)[0] == ':')
	    {
	      /* Do not tokenize the C++ scope operator. */
	      if (PARSER_STREAM (parser)[1] == ':')
		++(PARSER_STREAM (parser));

	      /* Do not tokenify if the input length so far is one
		 (i.e, a single-letter drive name) and the next character
		 is a directory separator.  This allows Windows-style
		 paths to be recognized as filenames without quoting it.  */
	      else if ((PARSER_STREAM (parser) - start) != 1
		       || !IS_DIR_SEPARATOR (PARSER_STREAM (parser)[1]))
		{
		  LS_TOKEN_STOKEN (token).ptr = start;
		  LS_TOKEN_STOKEN (token).length
		    = PARSER_STREAM (parser) - start;
		  return token;
		}
	    }
	  /* Special case: permit quote-enclosed linespecs.  */
	  else if (parser->is_quote_enclosed
		   && strchr (linespec_quote_characters,
			      *PARSER_STREAM (parser))
		   && is_closing_quote_enclosed (PARSER_STREAM (parser)))
	    {
	      LS_TOKEN_STOKEN (token).ptr = start;
	      LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - start;
	      return token;
	    }
	  /* Because commas may terminate a linespec and appear in
	     the middle of valid string input, special cases for
	     '<' and '(' are necessary.  */
	  else if (*PARSER_STREAM (parser) == '<'
		   || *PARSER_STREAM (parser) == '(')
	    {
	      const char *p;

	      p = find_parameter_list_end (PARSER_STREAM (parser));
	      if (p != NULL)
		{
		  PARSER_STREAM (parser) = p;
		  continue;
		}
	    }
	  /* Commas are terminators, but not if they are part of an
	     operator name.  */
	  else if (*PARSER_STREAM (parser) == ',')
	    {
	      if ((PARSER_STATE (parser)->language->la_language
		   == language_cplus)
		  && (PARSER_STREAM (parser) - start) > 8
		  /* strlen ("operator") */)
		{
		  const char *p = strstr (start, "operator");

		  if (p != NULL && is_operator_name (p))
		    {
		      /* This is an operator name.  Keep going.  */
		      ++(PARSER_STREAM (parser));
		      continue;
		    }
		}

	      /* Comma terminates the string.  */
	      LS_TOKEN_STOKEN (token).ptr = start;
	      LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - start;
	      return token;
	    }

	  /* Advance the stream.  */
	  ++(PARSER_STREAM (parser));
	}
    }

  return token;
}

/* Lex a single linespec token from PARSER.  */

static linespec_token
linespec_lexer_lex_one (linespec_parser *parser)
{
  const char *keyword;

  if (parser->lexer.current.type == LSTOKEN_CONSUMED)
    {
      /* Skip any whitespace.  */
      PARSER_STREAM (parser) = skip_spaces_const (PARSER_STREAM (parser));

      /* Check for a keyword, they end the linespec.  */
      keyword = linespec_lexer_lex_keyword (PARSER_STREAM (parser));
      if (keyword != NULL)
	{
	  parser->lexer.current.type = LSTOKEN_KEYWORD;
	  LS_TOKEN_KEYWORD (parser->lexer.current) = keyword;
	  /* We do not advance the stream here intentionally:
	     we would like lexing to stop when a keyword is seen.

	     PARSER_STREAM (parser) +=  strlen (keyword);  */

	  return parser->lexer.current;
	}

      /* Handle other tokens.  */
      switch (*PARSER_STREAM (parser))
	{
	case 0:
	  parser->lexer.current.type = LSTOKEN_EOI;
	  break;

	case '+': case '-':
	case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
           if (!linespec_lexer_lex_number (parser, &(parser->lexer.current)))
	     parser->lexer.current = linespec_lexer_lex_string (parser);
          break;

	case ':':
	  /* If we have a scope operator, lex the input as a string.
	     Otherwise, return LSTOKEN_COLON.  */
	  if (PARSER_STREAM (parser)[1] == ':')
	    parser->lexer.current = linespec_lexer_lex_string (parser);
	  else
	    {
	      parser->lexer.current.type = LSTOKEN_COLON;
	      ++(PARSER_STREAM (parser));
	    }
	  break;

	case '\'': case '\"':
	  /* Special case: permit quote-enclosed linespecs.  */
	  if (parser->is_quote_enclosed
	      && is_closing_quote_enclosed (PARSER_STREAM (parser)))
	    {
	      ++(PARSER_STREAM (parser));
	      parser->lexer.current.type = LSTOKEN_EOI;
	    }
	  else
	    parser->lexer.current = linespec_lexer_lex_string (parser);
	  break;

	case ',':
	  parser->lexer.current.type = LSTOKEN_COMMA;
	  LS_TOKEN_STOKEN (parser->lexer.current).ptr
	    = PARSER_STREAM (parser);
	  LS_TOKEN_STOKEN (parser->lexer.current).length = 1;
	  ++(PARSER_STREAM (parser));
	  break;

	default:
	  /* If the input is not a number, it must be a string.
	     [Keywords were already considered above.]  */
	  parser->lexer.current = linespec_lexer_lex_string (parser);
	  break;
	}
    }

  return parser->lexer.current;
}

/* Consume the current token and return the next token in PARSER's
   input stream.  */

static linespec_token
linespec_lexer_consume_token (linespec_parser *parser)
{
  parser->lexer.current.type = LSTOKEN_CONSUMED;
  return linespec_lexer_lex_one (parser);
}

/* Return the next token without consuming the current token.  */

static linespec_token
linespec_lexer_peek_token (linespec_parser *parser)
{
  linespec_token next;
  const char *saved_stream = PARSER_STREAM (parser);
  linespec_token saved_token = parser->lexer.current;

  next = linespec_lexer_consume_token (parser);
  PARSER_STREAM (parser) = saved_stream;
  parser->lexer.current = saved_token;
  return next;
}

/* Helper functions.  */

/* Add SAL to SALS.  */

static void
add_sal_to_sals_basic (struct symtabs_and_lines *sals,
		       struct symtab_and_line *sal)
{
  ++sals->nelts;
  sals->sals = XRESIZEVEC (struct symtab_and_line, sals->sals, sals->nelts);
  sals->sals[sals->nelts - 1] = *sal;
}

/* Add SAL to SALS, and also update SELF->CANONICAL_NAMES to reflect
   the new sal, if needed.  If not NULL, SYMNAME is the name of the
   symbol to use when constructing the new canonical name.

   If LITERAL_CANONICAL is non-zero, SYMNAME will be used as the
   canonical name for the SAL.  */

static void
add_sal_to_sals (struct linespec_state *self,
		 struct symtabs_and_lines *sals,
		 struct symtab_and_line *sal,
		 const char *symname, int literal_canonical)
{
  add_sal_to_sals_basic (sals, sal);

  if (self->canonical)
    {
      struct linespec_canonical_name *canonical;

      self->canonical_names = XRESIZEVEC (struct linespec_canonical_name,
					  self->canonical_names, sals->nelts);
      canonical = &self->canonical_names[sals->nelts - 1];
      if (!literal_canonical && sal->symtab)
	{
	  symtab_to_fullname (sal->symtab);

	  /* Note that the filter doesn't have to be a valid linespec
	     input.  We only apply the ":LINE" treatment to Ada for
	     the time being.  */
	  if (symname != NULL && sal->line != 0
	      && self->language->la_language == language_ada)
	    canonical->suffix = xstrprintf ("%s:%d", symname, sal->line);
	  else if (symname != NULL)
	    canonical->suffix = xstrdup (symname);
	  else
	    canonical->suffix = xstrprintf ("%d", sal->line);
	  canonical->symtab = sal->symtab;
	}
      else
	{
	  if (symname != NULL)
	    canonical->suffix = xstrdup (symname);
	  else
	    canonical->suffix = xstrdup ("<unknown>");
	  canonical->symtab = NULL;
	}
    }
}

/* A hash function for address_entry.  */

static hashval_t
hash_address_entry (const void *p)
{
  const struct address_entry *aep = (const struct address_entry *) p;
  hashval_t hash;

  hash = iterative_hash_object (aep->pspace, 0);
  return iterative_hash_object (aep->addr, hash);
}

/* An equality function for address_entry.  */

static int
eq_address_entry (const void *a, const void *b)
{
  const struct address_entry *aea = (const struct address_entry *) a;
  const struct address_entry *aeb = (const struct address_entry *) b;

  return aea->pspace == aeb->pspace && aea->addr == aeb->addr;
}

/* Check whether the address, represented by PSPACE and ADDR, is
   already in the set.  If so, return 0.  Otherwise, add it and return
   1.  */

static int
maybe_add_address (htab_t set, struct program_space *pspace, CORE_ADDR addr)
{
  struct address_entry e, *p;
  void **slot;

  e.pspace = pspace;
  e.addr = addr;
  slot = htab_find_slot (set, &e, INSERT);
  if (*slot)
    return 0;

  p = XNEW (struct address_entry);
  memcpy (p, &e, sizeof (struct address_entry));
  *slot = p;

  return 1;
}

/* A helper that walks over all matching symtabs in all objfiles and
   calls CALLBACK for each symbol matching NAME.  If SEARCH_PSPACE is
   not NULL, then the search is restricted to just that program
   space.  If INCLUDE_INLINE is true then symbols representing
   inlined instances of functions will be included in the result.  */

static void
iterate_over_all_matching_symtabs
  (struct linespec_state *state, const char *name, const domain_enum domain,
   struct program_space *search_pspace, bool include_inline,
   gdb::function_view<symbol_found_callback_ftype> callback)
{
  struct objfile *objfile;
  struct program_space *pspace;

  /* The routine to be used for comparison.  */
  symbol_name_cmp_ftype symbol_name_cmp
    = (state->language->la_get_symbol_name_cmp != NULL
       ? state->language->la_get_symbol_name_cmp (name)
       : strcmp_iw);

  ALL_PSPACES (pspace)
  {
    if (search_pspace != NULL && search_pspace != pspace)
      continue;
    if (pspace->executing_startup)
      continue;

    set_current_program_space (pspace);

    ALL_OBJFILES (objfile)
    {
      struct compunit_symtab *cu;

      if (objfile->sf)
	objfile->sf->qf->expand_symtabs_matching
	  (objfile,
	   NULL,
	   [&] (const char *symbol_name)
	   {
	     return symbol_name_cmp (symbol_name, name) == 0;
	   },
	   NULL,
	   ALL_DOMAIN);

      ALL_OBJFILE_COMPUNITS (objfile, cu)
	{
	  struct symtab *symtab = COMPUNIT_FILETABS (cu);

	  iterate_over_file_blocks (symtab, name, domain, callback);

	  if (include_inline)
	    {
	      struct block *block;
	      int i;

	      for (i = FIRST_LOCAL_BLOCK;
		   i < BLOCKVECTOR_NBLOCKS (SYMTAB_BLOCKVECTOR (symtab));
		   i++)
		{
		  block = BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (symtab), i);
		  state->language->la_iterate_over_symbols
		    (block, name, domain, [&] (symbol *sym)
		     {
		       /* Restrict calls to CALLBACK to symbols
			  representing inline symbols only.  */
		       if (SYMBOL_INLINED (sym))
			 return callback (sym);
		       return true;
		     });
		}
	    }
	}
    }
  }
}

/* Returns the block to be used for symbol searches from
   the current location.  */

static const struct block *
get_current_search_block (void)
{
  const struct block *block;
  enum language save_language;

  /* get_selected_block can change the current language when there is
     no selected frame yet.  */
  save_language = current_language->la_language;
  block = get_selected_block (0);
  set_language (save_language);

  return block;
}

/* Iterate over static and global blocks.  */

static void
iterate_over_file_blocks
  (struct symtab *symtab, const char *name, domain_enum domain,
   gdb::function_view<symbol_found_callback_ftype> callback)
{
  struct block *block;

  for (block = BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (symtab), STATIC_BLOCK);
       block != NULL;
       block = BLOCK_SUPERBLOCK (block))
    LA_ITERATE_OVER_SYMBOLS (block, name, domain, callback);
}

/* A helper for find_method.  This finds all methods in type T which
   match NAME.  It adds matching symbol names to RESULT_NAMES, and
   adds T's direct superclasses to SUPERCLASSES.  */

static void
find_methods (struct type *t, const char *name,
	      VEC (const_char_ptr) **result_names,
	      VEC (typep) **superclasses)
{
  int ibase;
  const char *class_name = type_name_no_tag (t);

  /* Ignore this class if it doesn't have a name.  This is ugly, but
     unless we figure out how to get the physname without the name of
     the class, then the loop can't do any good.  */
  if (class_name)
    {
      int method_counter;

      t = check_typedef (t);

      /* Loop over each method name.  At this level, all overloads of a name
         are counted as a single name.  There is an inner loop which loops over
         each overload.  */

      for (method_counter = TYPE_NFN_FIELDS (t) - 1;
	   method_counter >= 0;
	   --method_counter)
	{
	  const char *method_name = TYPE_FN_FIELDLIST_NAME (t, method_counter);
	  char dem_opname[64];

	  if (startswith (method_name, "__") ||
	      startswith (method_name, "op") ||
	      startswith (method_name, "type"))
	    {
	      if (cplus_demangle_opname (method_name, dem_opname, DMGL_ANSI))
		method_name = dem_opname;
	      else if (cplus_demangle_opname (method_name, dem_opname, 0))
		method_name = dem_opname;
	    }

	  if (strcmp_iw (method_name, name) == 0)
	    {
	      int field_counter;

	      for (field_counter = (TYPE_FN_FIELDLIST_LENGTH (t, method_counter)
				    - 1);
		   field_counter >= 0;
		   --field_counter)
		{
		  struct fn_field *f;
		  const char *phys_name;

		  f = TYPE_FN_FIELDLIST1 (t, method_counter);
		  if (TYPE_FN_FIELD_STUB (f, field_counter))
		    continue;
		  phys_name = TYPE_FN_FIELD_PHYSNAME (f, field_counter);
		  VEC_safe_push (const_char_ptr, *result_names, phys_name);
		}
	    }
	}
    }

  for (ibase = 0; ibase < TYPE_N_BASECLASSES (t); ibase++)
    VEC_safe_push (typep, *superclasses, TYPE_BASECLASS (t, ibase));
}

/* Find an instance of the character C in the string S that is outside
   of all parenthesis pairs, single-quoted strings, and double-quoted
   strings.  Also, ignore the char within a template name, like a ','
   within foo<int, int>.  */

const char *
find_toplevel_char (const char *s, char c)
{
  int quoted = 0;		/* zero if we're not in quotes;
				   '"' if we're in a double-quoted string;
				   '\'' if we're in a single-quoted string.  */
  int depth = 0;		/* Number of unclosed parens we've seen.  */
  const char *scan;

  for (scan = s; *scan; scan++)
    {
      if (quoted)
	{
	  if (*scan == quoted)
	    quoted = 0;
	  else if (*scan == '\\' && *(scan + 1))
	    scan++;
	}
      else if (*scan == c && ! quoted && depth == 0)
	return scan;
      else if (*scan == '"' || *scan == '\'')
	quoted = *scan;
      else if (*scan == '(' || *scan == '<')
	depth++;
      else if ((*scan == ')' || *scan == '>') && depth > 0)
	depth--;
    }

  return 0;
}

/* The string equivalent of find_toplevel_char.  Returns a pointer
   to the location of NEEDLE in HAYSTACK, ignoring any occurrences
   inside "()" and "<>".  Returns NULL if NEEDLE was not found.  */

static const char *
find_toplevel_string (const char *haystack, const char *needle)
{
  const char *s = haystack;

  do
    {
      s = find_toplevel_char (s, *needle);

      if (s != NULL)
	{
	  /* Found first char in HAYSTACK;  check rest of string.  */
	  if (startswith (s, needle))
	    return s;

	  /* Didn't find it; loop over HAYSTACK, looking for the next
	     instance of the first character of NEEDLE.  */
	  ++s;
	}
    }
  while (s != NULL && *s != '\0');

  /* NEEDLE was not found in HAYSTACK.  */
  return NULL;
}

/* Convert CANONICAL to its string representation using
   symtab_to_fullname for SYMTAB.  The caller must xfree the result.  */

static char *
canonical_to_fullform (const struct linespec_canonical_name *canonical)
{
  if (canonical->symtab == NULL)
    return xstrdup (canonical->suffix);
  else
    return xstrprintf ("%s:%s", symtab_to_fullname (canonical->symtab),
		       canonical->suffix);
}

/* Given FILTERS, a list of canonical names, filter the sals in RESULT
   and store the result in SELF->CANONICAL.  */

static void
filter_results (struct linespec_state *self,
		struct symtabs_and_lines *result,
		VEC (const_char_ptr) *filters)
{
  int i;
  const char *name;

  for (i = 0; VEC_iterate (const_char_ptr, filters, i, name); ++i)
    {
      struct linespec_sals lsal;
      int j;

      memset (&lsal, 0, sizeof (lsal));

      for (j = 0; j < result->nelts; ++j)
	{
	  const struct linespec_canonical_name *canonical;
	  char *fullform;
	  struct cleanup *cleanup;

	  canonical = &self->canonical_names[j];
	  fullform = canonical_to_fullform (canonical);
	  cleanup = make_cleanup (xfree, fullform);

	  if (strcmp (name, fullform) == 0)
	    add_sal_to_sals_basic (&lsal.sals, &result->sals[j]);

	  do_cleanups (cleanup);
	}

      if (lsal.sals.nelts > 0)
	{
	  lsal.canonical = xstrdup (name);
	  VEC_safe_push (linespec_sals, self->canonical->sals, &lsal);
	}
    }

  self->canonical->pre_expanded = 0;
}

/* Store RESULT into SELF->CANONICAL.  */

static void
convert_results_to_lsals (struct linespec_state *self,
			  struct symtabs_and_lines *result)
{
  struct linespec_sals lsal;

  lsal.canonical = NULL;
  lsal.sals = *result;
  VEC_safe_push (linespec_sals, self->canonical->sals, &lsal);
}

/* A structure that contains two string representations of a struct
   linespec_canonical_name:
     - one where the the symtab's fullname is used;
     - one where the filename followed the "set filename-display"
       setting.  */

struct decode_line_2_item
{
  /* The form using symtab_to_fullname.
     It must be xfree'ed after use.  */
  char *fullform;

  /* The form using symtab_to_filename_for_display.
     It must be xfree'ed after use.  */
  char *displayform;

  /* Field is initialized to zero and it is set to one if the user
     requested breakpoint for this entry.  */
  unsigned int selected : 1;
};

/* Helper for qsort to sort decode_line_2_item entries by DISPLAYFORM and
   secondarily by FULLFORM.  */

static int
decode_line_2_compare_items (const void *ap, const void *bp)
{
  const struct decode_line_2_item *a = (const struct decode_line_2_item *) ap;
  const struct decode_line_2_item *b = (const struct decode_line_2_item *) bp;
  int retval;

  retval = strcmp (a->displayform, b->displayform);
  if (retval != 0)
    return retval;

  return strcmp (a->fullform, b->fullform);
}

/* Handle multiple results in RESULT depending on SELECT_MODE.  This
   will either return normally, throw an exception on multiple
   results, or present a menu to the user.  On return, the SALS vector
   in SELF->CANONICAL is set up properly.  */

static void
decode_line_2 (struct linespec_state *self,
	       struct symtabs_and_lines *result,
	       const char *select_mode)
{
  char *args;
  const char *prompt;
  int i;
  struct cleanup *old_chain;
  VEC (const_char_ptr) *filters = NULL;
  struct decode_line_2_item *items;
  int items_count;

  gdb_assert (select_mode != multiple_symbols_all);
  gdb_assert (self->canonical != NULL);
  gdb_assert (result->nelts >= 1);

  old_chain = make_cleanup (VEC_cleanup (const_char_ptr), &filters);

  /* Prepare ITEMS array.  */
  items_count = result->nelts;
  items = XNEWVEC (struct decode_line_2_item, items_count);
  make_cleanup (xfree, items);
  for (i = 0; i < items_count; ++i)
    {
      const struct linespec_canonical_name *canonical;
      struct decode_line_2_item *item;

      canonical = &self->canonical_names[i];
      gdb_assert (canonical->suffix != NULL);
      item = &items[i];

      item->fullform = canonical_to_fullform (canonical);
      make_cleanup (xfree, item->fullform);

      if (canonical->symtab == NULL)
	item->displayform = canonical->suffix;
      else
	{
	  const char *fn_for_display;

	  fn_for_display = symtab_to_filename_for_display (canonical->symtab);
	  item->displayform = xstrprintf ("%s:%s", fn_for_display,
					  canonical->suffix);
	  make_cleanup (xfree, item->displayform);
	}

      item->selected = 0;
    }

  /* Sort the list of method names.  */
  qsort (items, items_count, sizeof (*items), decode_line_2_compare_items);

  /* Remove entries with the same FULLFORM.  */
  if (items_count >= 2)
    {
      struct decode_line_2_item *dst, *src;

      dst = items;
      for (src = &items[1]; src < &items[items_count]; src++)
	if (strcmp (src->fullform, dst->fullform) != 0)
	  *++dst = *src;
      items_count = dst + 1 - items;
    }

  if (select_mode == multiple_symbols_cancel && items_count > 1)
    error (_("canceled because the command is ambiguous\n"
	     "See set/show multiple-symbol."));
  
  if (select_mode == multiple_symbols_all || items_count == 1)
    {
      do_cleanups (old_chain);
      convert_results_to_lsals (self, result);
      return;
    }

  printf_unfiltered (_("[0] cancel\n[1] all\n"));
  for (i = 0; i < items_count; i++)
    printf_unfiltered ("[%d] %s\n", i + 2, items[i].displayform);

  prompt = getenv ("PS2");
  if (prompt == NULL)
    {
      prompt = "> ";
    }
  args = command_line_input (prompt, 0, "overload-choice");

  if (args == 0 || *args == 0)
    error_no_arg (_("one or more choice numbers"));

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();

      if (num == 0)
	error (_("canceled"));
      else if (num == 1)
	{
	  /* We intentionally make this result in a single breakpoint,
	     contrary to what older versions of gdb did.  The
	     rationale is that this lets a user get the
	     multiple_symbols_all behavior even with the 'ask'
	     setting; and he can get separate breakpoints by entering
	     "2-57" at the query.  */
	  do_cleanups (old_chain);
	  convert_results_to_lsals (self, result);
	  return;
	}

      num -= 2;
      if (num >= items_count)
	printf_unfiltered (_("No choice number %d.\n"), num);
      else
	{
	  struct decode_line_2_item *item = &items[num];

	  if (!item->selected)
	    {
	      VEC_safe_push (const_char_ptr, filters, item->fullform);
	      item->selected = 1;
	    }
	  else
	    {
	      printf_unfiltered (_("duplicate request for %d ignored.\n"),
				 num + 2);
	    }
	}
    }

  filter_results (self, result, filters);
  do_cleanups (old_chain);
}



/* The parser of linespec itself.  */

/* Throw an appropriate error when SYMBOL is not found (optionally in
   FILENAME).  */

static void ATTRIBUTE_NORETURN
symbol_not_found_error (const char *symbol, const char *filename)
{
  if (symbol == NULL)
    symbol = "";

  if (!have_full_symbols ()
      && !have_partial_symbols ()
      && !have_minimal_symbols ())
    throw_error (NOT_FOUND_ERROR,
		 _("No symbol table is loaded.  Use the \"file\" command."));

  /* If SYMBOL starts with '$', the user attempted to either lookup
     a function/variable in his code starting with '$' or an internal
     variable of that name.  Since we do not know which, be concise and
     explain both possibilities.  */
  if (*symbol == '$')
    {
      if (filename)
	throw_error (NOT_FOUND_ERROR,
		     _("Undefined convenience variable or function \"%s\" "
		       "not defined in \"%s\"."), symbol, filename);
      else
	throw_error (NOT_FOUND_ERROR,
		     _("Undefined convenience variable or function \"%s\" "
		       "not defined."), symbol);
    }
  else
    {
      if (filename)
	throw_error (NOT_FOUND_ERROR,
		     _("Function \"%s\" not defined in \"%s\"."),
		     symbol, filename);
      else
	throw_error (NOT_FOUND_ERROR,
		     _("Function \"%s\" not defined."), symbol);
    }
}

/* Throw an appropriate error when an unexpected token is encountered 
   in the input.  */

static void ATTRIBUTE_NORETURN
unexpected_linespec_error (linespec_parser *parser)
{
  linespec_token token;
  static const char * token_type_strings[]
    = {"keyword", "colon", "string", "number", "comma", "end of input"};

  /* Get the token that generated the error.  */
  token = linespec_lexer_lex_one (parser);

  /* Finally, throw the error.  */
  if (token.type == LSTOKEN_STRING || token.type == LSTOKEN_NUMBER
      || token.type == LSTOKEN_KEYWORD)
    {
      char *string;

      string = copy_token_string (token);
      make_cleanup (xfree, string);
      throw_error (GENERIC_ERROR,
		   _("malformed linespec error: unexpected %s, \"%s\""),
		   token_type_strings[token.type], string);
    }
  else
    throw_error (GENERIC_ERROR,
		 _("malformed linespec error: unexpected %s"),
		 token_type_strings[token.type]);
}

/* Throw an undefined label error.  */

static void ATTRIBUTE_NORETURN
undefined_label_error (const char *function, const char *label)
{
  if (function != NULL)
    throw_error (NOT_FOUND_ERROR,
                _("No label \"%s\" defined in function \"%s\"."),
                label, function);
  else
    throw_error (NOT_FOUND_ERROR,
                _("No label \"%s\" defined in current function."),
                label);
}

/* Throw a source file not found error.  */

static void ATTRIBUTE_NORETURN
source_file_not_found_error (const char *name)
{
  throw_error (NOT_FOUND_ERROR, _("No source file named %s."), name);
}

/* See description in linespec.h.  */

struct line_offset
linespec_parse_line_offset (const char *string)
{
  const char *start = string;
  struct line_offset line_offset = {0, LINE_OFFSET_NONE};

  if (*string == '+')
    {
      line_offset.sign = LINE_OFFSET_PLUS;
      ++string;
    }
  else if (*string == '-')
    {
      line_offset.sign = LINE_OFFSET_MINUS;
      ++string;
    }

  if (*string != '\0' && !isdigit (*string))
    error (_("malformed line offset: \"%s\""), start);

  /* Right now, we only allow base 10 for offsets.  */
  line_offset.offset = atoi (string);
  return line_offset;
}

/* Parse the basic_spec in PARSER's input.  */

static void
linespec_parse_basic (linespec_parser *parser)
{
  char *name;
  linespec_token token;
  VEC (symbolp) *symbols, *labels;
  VEC (bound_minimal_symbol_d) *minimal_symbols;
  struct cleanup *cleanup;

  /* Get the next token.  */
  token = linespec_lexer_lex_one (parser);

  /* If it is EOI or KEYWORD, issue an error.  */
  if (token.type == LSTOKEN_KEYWORD || token.type == LSTOKEN_EOI)
    unexpected_linespec_error (parser);
  /* If it is a LSTOKEN_NUMBER, we have an offset.  */
  else if (token.type == LSTOKEN_NUMBER)
    {
      /* Record the line offset and get the next token.  */
      name = copy_token_string (token);
      cleanup = make_cleanup (xfree, name);
      PARSER_EXPLICIT (parser)->line_offset = linespec_parse_line_offset (name);
      do_cleanups (cleanup);

      /* Get the next token.  */
      token = linespec_lexer_consume_token (parser);

      /* If the next token is a comma, stop parsing and return.  */
      if (token.type == LSTOKEN_COMMA)
	return;

      /* If the next token is anything but EOI or KEYWORD, issue
	 an error.  */
      if (token.type != LSTOKEN_KEYWORD && token.type != LSTOKEN_EOI)
	unexpected_linespec_error (parser);
    }

  if (token.type == LSTOKEN_KEYWORD || token.type == LSTOKEN_EOI)
    return;

  /* Next token must be LSTOKEN_STRING.  */
  if (token.type != LSTOKEN_STRING)
    unexpected_linespec_error (parser);

  /* The current token will contain the name of a function, method,
     or label.  */
  name  = copy_token_string (token);
  cleanup = make_cleanup (xfree, name);

  /* Try looking it up as a function/method.  */
  find_linespec_symbols (PARSER_STATE (parser),
			 PARSER_RESULT (parser)->file_symtabs, name,
			 &symbols, &minimal_symbols);

  if (symbols != NULL || minimal_symbols != NULL)
    {
      PARSER_RESULT (parser)->function_symbols = symbols;
      PARSER_RESULT (parser)->minimal_symbols = minimal_symbols;
      PARSER_EXPLICIT (parser)->function_name = name;
      symbols = NULL;
      discard_cleanups (cleanup);
    }
  else
    {
      /* NAME was not a function or a method.  So it must be a label
	 name or user specified variable like "break foo.c:$zippo".  */
      labels = find_label_symbols (PARSER_STATE (parser), NULL,
				   &symbols, name);
      if (labels != NULL)
	{
	  PARSER_RESULT (parser)->labels.label_symbols = labels;
	  PARSER_RESULT (parser)->labels.function_symbols = symbols;
	  PARSER_EXPLICIT (parser)->label_name = name;
	  symbols = NULL;
	  discard_cleanups (cleanup);
	}
      else if (token.type == LSTOKEN_STRING
	       && *LS_TOKEN_STOKEN (token).ptr == '$')
	{
	  /* User specified a convenience variable or history value.  */
	  PARSER_EXPLICIT (parser)->line_offset
	    = linespec_parse_variable (PARSER_STATE (parser), name);

	  if (PARSER_EXPLICIT (parser)->line_offset.sign == LINE_OFFSET_UNKNOWN)
	    {
	      /* The user-specified variable was not valid.  Do not
		 throw an error here.  parse_linespec will do it for us.  */
	      PARSER_EXPLICIT (parser)->function_name = name;
	      discard_cleanups (cleanup);
	      return;
	    }

	  /* The convenience variable/history value parsed correctly.
	     NAME is no longer needed.  */
	  do_cleanups (cleanup);
	}
      else
	{
	  /* The name is also not a label.  Abort parsing.  Do not throw
	     an error here.  parse_linespec will do it for us.  */

	  /* Save a copy of the name we were trying to lookup.  */
	  PARSER_EXPLICIT (parser)->function_name = name;
	  discard_cleanups (cleanup);
	  return;
	}
    }

  /* Get the next token.  */
  token = linespec_lexer_consume_token (parser);

  if (token.type == LSTOKEN_COLON)
    {
      /* User specified a label or a lineno.  */
      token = linespec_lexer_consume_token (parser);

      if (token.type == LSTOKEN_NUMBER)
	{
	  /* User specified an offset.  Record the line offset and
	     get the next token.  */
	  name = copy_token_string (token);
	  cleanup = make_cleanup (xfree, name);
	  PARSER_EXPLICIT (parser)->line_offset
	    = linespec_parse_line_offset (name);
	  do_cleanups (cleanup);

	  /* Ge the next token.  */
	  token = linespec_lexer_consume_token (parser);
	}
      else if (token.type == LSTOKEN_STRING)
	{
	  /* Grab a copy of the label's name and look it up.  */
	  name = copy_token_string (token);
	  cleanup = make_cleanup (xfree, name);
	  labels = find_label_symbols (PARSER_STATE (parser),
				       PARSER_RESULT (parser)->function_symbols,
				       &symbols, name);

	  if (labels != NULL)
	    {
	      PARSER_RESULT (parser)->labels.label_symbols = labels;
	      PARSER_RESULT (parser)->labels.function_symbols = symbols;
	      PARSER_EXPLICIT (parser)->label_name = name;
	      symbols = NULL;
	      discard_cleanups (cleanup);
	    }
	  else
	    {
	      /* We don't know what it was, but it isn't a label.  */
	      undefined_label_error (PARSER_EXPLICIT (parser)->function_name,
				     name);
	    }

	  /* Check for a line offset.  */
	  token = linespec_lexer_consume_token (parser);
	  if (token.type == LSTOKEN_COLON)
	    {
	      /* Get the next token.  */
	      token = linespec_lexer_consume_token (parser);

	      /* It must be a line offset.  */
	      if (token.type != LSTOKEN_NUMBER)
		unexpected_linespec_error (parser);

	      /* Record the lione offset and get the next token.  */
	      name = copy_token_string (token);
	      cleanup = make_cleanup (xfree, name);

	      PARSER_EXPLICIT (parser)->line_offset
		= linespec_parse_line_offset (name);
	      do_cleanups (cleanup);

	      /* Get the next token.  */
	      token = linespec_lexer_consume_token (parser);
	    }
	}
      else
	{
	  /* Trailing ':' in the input. Issue an error.  */
	  unexpected_linespec_error (parser);
	}
    }
}

/* Canonicalize the linespec contained in LS.  The result is saved into
   STATE->canonical.  This function handles both linespec and explicit
   locations.  */

static void
canonicalize_linespec (struct linespec_state *state, const linespec_p ls)
{
  struct event_location *canon;
  struct explicit_location *explicit_loc;

  /* If canonicalization was not requested, no need to do anything.  */
  if (!state->canonical)
    return;

  /* Save everything as an explicit location.  */
  state->canonical->location
    = new_explicit_location (&ls->explicit_loc);
  canon = state->canonical->location.get ();
  explicit_loc = get_explicit_location (canon);

  if (explicit_loc->label_name != NULL)
    {
      state->canonical->special_display = 1;

      if (explicit_loc->function_name == NULL)
	{
	  struct symbol *s;

	  /* No function was specified, so add the symbol name.  */
	  gdb_assert (ls->labels.function_symbols != NULL
		      && (VEC_length (symbolp, ls->labels.function_symbols)
			  == 1));
	  s = VEC_index (symbolp, ls->labels.function_symbols, 0);
	  explicit_loc->function_name = xstrdup (SYMBOL_NATURAL_NAME (s));
	}
    }

  /* If this location originally came from a linespec, save a string
     representation of it for display and saving to file.  */
  if (state->is_linespec)
    {
      char *linespec = explicit_location_to_linespec (explicit_loc);

      set_event_location_string (canon, linespec);
      xfree (linespec);
    }
}

/* Given a line offset in LS, construct the relevant SALs.  */

static struct symtabs_and_lines
create_sals_line_offset (struct linespec_state *self,
			 linespec_p ls)
{
  struct symtabs_and_lines values;
  struct symtab_and_line val;
  int use_default = 0;

  init_sal (&val);
  values.sals = NULL;
  values.nelts = 0;

  /* This is where we need to make sure we have good defaults.
     We must guarantee that this section of code is never executed
     when we are called with just a function name, since
     set_default_source_symtab_and_line uses
     select_source_symtab that calls us with such an argument.  */

  if (VEC_length (symtab_ptr, ls->file_symtabs) == 1
      && VEC_index (symtab_ptr, ls->file_symtabs, 0) == NULL)
    {
      const char *fullname;

      set_current_program_space (self->program_space);

      /* Make sure we have at least a default source line.  */
      set_default_source_symtab_and_line ();
      initialize_defaults (&self->default_symtab, &self->default_line);
      fullname = symtab_to_fullname (self->default_symtab);
      VEC_pop (symtab_ptr, ls->file_symtabs);
      VEC_free (symtab_ptr, ls->file_symtabs);
      ls->file_symtabs = collect_symtabs_from_filename (fullname,
							self->search_pspace);
      use_default = 1;
    }

  val.line = ls->explicit_loc.line_offset.offset;
  switch (ls->explicit_loc.line_offset.sign)
    {
    case LINE_OFFSET_PLUS:
      if (ls->explicit_loc.line_offset.offset == 0)
	val.line = 5;
      if (use_default)
	val.line = self->default_line + val.line;
      break;

    case LINE_OFFSET_MINUS:
      if (ls->explicit_loc.line_offset.offset == 0)
	val.line = 15;
      if (use_default)
	val.line = self->default_line - val.line;
      else
	val.line = -val.line;
      break;

    case LINE_OFFSET_NONE:
      break;			/* No need to adjust val.line.  */
    }

  if (self->list_mode)
    decode_digits_list_mode (self, ls, &values, val);
  else
    {
      struct linetable_entry *best_entry = NULL;
      int *filter;
      const struct block **blocks;
      struct cleanup *cleanup;
      struct symtabs_and_lines intermediate_results;
      int i, j;

      intermediate_results.sals = NULL;
      intermediate_results.nelts = 0;

      decode_digits_ordinary (self, ls, val.line, &intermediate_results,
			      &best_entry);
      if (intermediate_results.nelts == 0 && best_entry != NULL)
	decode_digits_ordinary (self, ls, best_entry->line,
				&intermediate_results, &best_entry);

      cleanup = make_cleanup (xfree, intermediate_results.sals);

      /* For optimized code, the compiler can scatter one source line
	 across disjoint ranges of PC values, even when no duplicate
	 functions or inline functions are involved.  For example,
	 'for (;;)' inside a non-template, non-inline, and non-ctor-or-dtor
	 function can result in two PC ranges.  In this case, we don't
	 want to set a breakpoint on the first PC of each range.  To filter
	 such cases, we use containing blocks -- for each PC found
	 above, we see if there are other PCs that are in the same
	 block.  If yes, the other PCs are filtered out.  */

      filter = XNEWVEC (int, intermediate_results.nelts);
      make_cleanup (xfree, filter);
      blocks = XNEWVEC (const struct block *, intermediate_results.nelts);
      make_cleanup (xfree, blocks);

      for (i = 0; i < intermediate_results.nelts; ++i)
	{
	  set_current_program_space (intermediate_results.sals[i].pspace);

	  filter[i] = 1;
	  blocks[i] = block_for_pc_sect (intermediate_results.sals[i].pc,
					 intermediate_results.sals[i].section);
	}

      for (i = 0; i < intermediate_results.nelts; ++i)
	{
	  if (blocks[i] != NULL)
	    for (j = i + 1; j < intermediate_results.nelts; ++j)
	      {
		if (blocks[j] == blocks[i])
		  {
		    filter[j] = 0;
		    break;
		  }
	      }
	}

      for (i = 0; i < intermediate_results.nelts; ++i)
	if (filter[i])
	  {
	    struct symbol *sym = (blocks[i]
				  ? block_containing_function (blocks[i])
				  : NULL);

	    if (self->funfirstline)
	      skip_prologue_sal (&intermediate_results.sals[i]);
	    /* Make sure the line matches the request, not what was
	       found.  */
	    intermediate_results.sals[i].line = val.line;
	    add_sal_to_sals (self, &values, &intermediate_results.sals[i],
			     sym ? SYMBOL_NATURAL_NAME (sym) : NULL, 0);
	  }

      do_cleanups (cleanup);
    }

  if (values.nelts == 0)
    {
      if (ls->explicit_loc.source_filename)
	throw_error (NOT_FOUND_ERROR, _("No line %d in file \"%s\"."),
		     val.line, ls->explicit_loc.source_filename);
      else
	throw_error (NOT_FOUND_ERROR, _("No line %d in the current file."),
		     val.line);
    }

  return values;
}

/* Convert the given ADDRESS into SaLs.  */

static struct symtabs_and_lines
convert_address_location_to_sals (struct linespec_state *self,
				  CORE_ADDR address)
{
  struct symtab_and_line sal;
  struct symtabs_and_lines sals = {NULL, 0};

  sal = find_pc_line (address, 0);
  sal.pc = address;
  sal.section = find_pc_overlay (address);
  sal.explicit_pc = 1;
  add_sal_to_sals (self, &sals, &sal, core_addr_to_string (address), 1);

  return sals;
}

/* Create and return SALs from the linespec LS.  */

static struct symtabs_and_lines
convert_linespec_to_sals (struct linespec_state *state, linespec_p ls)
{
  struct symtabs_and_lines sals = {NULL, 0};

  if (ls->labels.label_symbols != NULL)
    {
      /* We have just a bunch of functions/methods or labels.  */
      int i;
      struct symtab_and_line sal;
      struct symbol *sym;

      for (i = 0; VEC_iterate (symbolp, ls->labels.label_symbols, i, sym); ++i)
	{
	  struct program_space *pspace = SYMTAB_PSPACE (symbol_symtab (sym));

	  if (symbol_to_sal (&sal, state->funfirstline, sym)
	      && maybe_add_address (state->addr_set, pspace, sal.pc))
	    add_sal_to_sals (state, &sals, &sal,
			     SYMBOL_NATURAL_NAME (sym), 0);
	}
    }
  else if (ls->function_symbols != NULL || ls->minimal_symbols != NULL)
    {
      /* We have just a bunch of functions and/or methods.  */
      int i;
      struct symtab_and_line sal;
      struct symbol *sym;
      bound_minimal_symbol_d *elem;
      struct program_space *pspace;

      if (ls->function_symbols != NULL)
	{
	  /* Sort symbols so that symbols with the same program space are next
	     to each other.  */
	  qsort (VEC_address (symbolp, ls->function_symbols),
		 VEC_length (symbolp, ls->function_symbols),
		 sizeof (symbolp), compare_symbols);

	  for (i = 0; VEC_iterate (symbolp, ls->function_symbols, i, sym); ++i)
	    {
	      pspace = SYMTAB_PSPACE (symbol_symtab (sym));
	      set_current_program_space (pspace);
	      if (symbol_to_sal (&sal, state->funfirstline, sym)
		  && maybe_add_address (state->addr_set, pspace, sal.pc))
		add_sal_to_sals (state, &sals, &sal,
				 SYMBOL_NATURAL_NAME (sym), 0);
	    }
	}

      if (ls->minimal_symbols != NULL)
	{
	  /* Sort minimal symbols by program space, too.  */
	  qsort (VEC_address (bound_minimal_symbol_d, ls->minimal_symbols),
		 VEC_length (bound_minimal_symbol_d, ls->minimal_symbols),
		 sizeof (bound_minimal_symbol_d), compare_msymbols);

	  for (i = 0;
	       VEC_iterate (bound_minimal_symbol_d, ls->minimal_symbols,
			    i, elem);
	       ++i)
	    {
	      pspace = elem->objfile->pspace;
	      set_current_program_space (pspace);
	      minsym_found (state, elem->objfile, elem->minsym, &sals);
	    }
	}
    }
  else if (ls->explicit_loc.line_offset.sign != LINE_OFFSET_UNKNOWN)
    {
      /* Only an offset was specified.  */
	sals = create_sals_line_offset (state, ls);

	/* Make sure we have a filename for canonicalization.  */
	if (ls->explicit_loc.source_filename == NULL)
	  {
	    const char *fullname = symtab_to_fullname (state->default_symtab);

	    /* It may be more appropriate to keep DEFAULT_SYMTAB in its symtab
	       form so that displaying SOURCE_FILENAME can follow the current
	       FILENAME_DISPLAY_STRING setting.  But as it is used only rarely
	       it has been kept for code simplicity only in absolute form.  */
	    ls->explicit_loc.source_filename = xstrdup (fullname);
	  }
    }
  else
    {
      /* We haven't found any results...  */
      return sals;
    }

  canonicalize_linespec (state, ls);

  if (sals.nelts > 0 && state->canonical != NULL)
    state->canonical->pre_expanded = 1;

  return sals;
}

/* Convert the explicit location EXPLICIT_LOC into SaLs.  */

static struct symtabs_and_lines
convert_explicit_location_to_sals (struct linespec_state *self,
				   linespec_p result,
				   const struct explicit_location *explicit_loc)
{
  VEC (symbolp) *symbols, *labels;
  VEC (bound_minimal_symbol_d) *minimal_symbols;

  if (explicit_loc->source_filename != NULL)
    {
      TRY
	{
	  result->file_symtabs
	    = symtabs_from_filename (explicit_loc->source_filename,
				     self->search_pspace);
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  source_file_not_found_error (explicit_loc->source_filename);
	}
      END_CATCH
      result->explicit_loc.source_filename
	= xstrdup (explicit_loc->source_filename);
    }
  else
    {
      /* A NULL entry means to use the default symtab.  */
      VEC_safe_push (symtab_ptr, result->file_symtabs, NULL);
    }

  if (explicit_loc->function_name != NULL)
    {
      find_linespec_symbols (self, result->file_symtabs,
			     explicit_loc->function_name, &symbols,
			     &minimal_symbols);

      if (symbols == NULL && minimal_symbols == NULL)
	symbol_not_found_error (explicit_loc->function_name,
				result->explicit_loc.source_filename);

      result->explicit_loc.function_name
	= xstrdup (explicit_loc->function_name);
      result->function_symbols = symbols;
      result->minimal_symbols = minimal_symbols;
    }

  if (explicit_loc->label_name != NULL)
    {
      symbols = NULL;
      labels = find_label_symbols (self, result->function_symbols,
				   &symbols, explicit_loc->label_name);

      if (labels == NULL)
	undefined_label_error (result->explicit_loc.function_name,
			       explicit_loc->label_name);

      result->explicit_loc.label_name = xstrdup (explicit_loc->label_name);
      result->labels.label_symbols = labels;
      result->labels.function_symbols = symbols;
    }

  if (explicit_loc->line_offset.sign != LINE_OFFSET_UNKNOWN)
    result->explicit_loc.line_offset = explicit_loc->line_offset;

   return convert_linespec_to_sals (self, result);
}

/* Parse a string that specifies a linespec.

   The basic grammar of linespecs:

   linespec -> var_spec | basic_spec
   var_spec -> '$' (STRING | NUMBER)

   basic_spec -> file_offset_spec | function_spec | label_spec
   file_offset_spec -> opt_file_spec offset_spec
   function_spec -> opt_file_spec function_name_spec opt_label_spec
   label_spec -> label_name_spec

   opt_file_spec -> "" | file_name_spec ':'
   opt_label_spec -> "" | ':' label_name_spec

   file_name_spec -> STRING
   function_name_spec -> STRING
   label_name_spec -> STRING
   function_name_spec -> STRING
   offset_spec -> NUMBER
               -> '+' NUMBER
	       -> '-' NUMBER

   This may all be followed by several keywords such as "if EXPR",
   which we ignore.

   A comma will terminate parsing.

   The function may be an undebuggable function found in minimal symbol table.

   If the argument FUNFIRSTLINE is nonzero, we want the first line
   of real code inside a function when a function is specified, and it is
   not OK to specify a variable or type to get its line number.

   DEFAULT_SYMTAB specifies the file to use if none is specified.
   It defaults to current_source_symtab.
   DEFAULT_LINE specifies the line number to use for relative
   line numbers (that start with signs).  Defaults to current_source_line.
   If CANONICAL is non-NULL, store an array of strings containing the canonical
   line specs there if necessary.  Currently overloaded member functions and
   line numbers or static functions without a filename yield a canonical
   line spec.  The array and the line spec strings are allocated on the heap,
   it is the callers responsibility to free them.

   Note that it is possible to return zero for the symtab
   if no file is validly specified.  Callers must check that.
   Also, the line number returned may be invalid.  */

/* Parse the linespec in ARG.  */

static struct symtabs_and_lines
parse_linespec (linespec_parser *parser, const char *arg)
{
  linespec_token token;
  struct symtabs_and_lines values;
  struct gdb_exception file_exception = exception_none;
  struct cleanup *cleanup;

  /* A special case to start.  It has become quite popular for
     IDEs to work around bugs in the previous parser by quoting
     the entire linespec, so we attempt to deal with this nicely.  */
  parser->is_quote_enclosed = 0;
  if (!is_ada_operator (arg)
      && strchr (linespec_quote_characters, *arg) != NULL)
    {
      const char *end;

      end = skip_quote_char (arg + 1, *arg);
      if (end != NULL && is_closing_quote_enclosed (end))
	{
	  /* Here's the special case.  Skip ARG past the initial
	     quote.  */
	  ++arg;
	  parser->is_quote_enclosed = 1;
	}
    }

  parser->lexer.saved_arg = arg;
  parser->lexer.stream = arg;

  /* Initialize the default symtab and line offset.  */
  initialize_defaults (&PARSER_STATE (parser)->default_symtab,
		       &PARSER_STATE (parser)->default_line);

  /* Objective-C shortcut.  */
  values = decode_objc (PARSER_STATE (parser), PARSER_RESULT (parser), arg);
  if (values.sals != NULL)
    return values;

  /* Start parsing.  */

  /* Get the first token.  */
  token = linespec_lexer_lex_one (parser);

  /* It must be either LSTOKEN_STRING or LSTOKEN_NUMBER.  */
  if (token.type == LSTOKEN_STRING && *LS_TOKEN_STOKEN (token).ptr == '$')
    {
      char *var;

      /* A NULL entry means to use GLOBAL_DEFAULT_SYMTAB.  */
      VEC_safe_push (symtab_ptr, PARSER_RESULT (parser)->file_symtabs, NULL);

      /* User specified a convenience variable or history value.  */
      var = copy_token_string (token);
      cleanup = make_cleanup (xfree, var);
      PARSER_EXPLICIT (parser)->line_offset
	= linespec_parse_variable (PARSER_STATE (parser), var);
      do_cleanups (cleanup);

      /* If a line_offset wasn't found (VAR is the name of a user
	 variable/function), then skip to normal symbol processing.  */
      if (PARSER_EXPLICIT (parser)->line_offset.sign != LINE_OFFSET_UNKNOWN)
	{
	  /* Consume this token.  */
	  linespec_lexer_consume_token (parser);

	  goto convert_to_sals;
	}
    }
  else if (token.type != LSTOKEN_STRING && token.type != LSTOKEN_NUMBER)
    unexpected_linespec_error (parser);

  /* Shortcut: If the next token is not LSTOKEN_COLON, we know that
     this token cannot represent a filename.  */
  token = linespec_lexer_peek_token (parser);

  if (token.type == LSTOKEN_COLON)
    {
      char *user_filename;

      /* Get the current token again and extract the filename.  */
      token = linespec_lexer_lex_one (parser);
      user_filename = copy_token_string (token);

      /* Check if the input is a filename.  */
      TRY
	{
	  PARSER_RESULT (parser)->file_symtabs
	    = symtabs_from_filename (user_filename,
				     PARSER_STATE (parser)->search_pspace);
	}
      CATCH (ex, RETURN_MASK_ERROR)
	{
	  file_exception = ex;
	}
      END_CATCH

      if (file_exception.reason >= 0)
	{
	  /* Symtabs were found for the file.  Record the filename.  */
	  PARSER_EXPLICIT (parser)->source_filename = user_filename;

	  /* Get the next token.  */
	  token = linespec_lexer_consume_token (parser);

	  /* This is LSTOKEN_COLON; consume it.  */
	  linespec_lexer_consume_token (parser);
	}
      else
	{
	  /* No symtabs found -- discard user_filename.  */
	  xfree (user_filename);

	  /* A NULL entry means to use GLOBAL_DEFAULT_SYMTAB.  */
	  VEC_safe_push (symtab_ptr, PARSER_RESULT (parser)->file_symtabs, NULL);
	}
    }
  /* If the next token is not EOI, KEYWORD, or COMMA, issue an error.  */
  else if (token.type != LSTOKEN_EOI && token.type != LSTOKEN_KEYWORD
	   && token.type != LSTOKEN_COMMA)
    {
      /* TOKEN is the _next_ token, not the one currently in the parser.
	 Consuming the token will give the correct error message.  */
      linespec_lexer_consume_token (parser);
      unexpected_linespec_error (parser);
    }
  else
    {
      /* A NULL entry means to use GLOBAL_DEFAULT_SYMTAB.  */
      VEC_safe_push (symtab_ptr, PARSER_RESULT (parser)->file_symtabs, NULL);
    }

  /* Parse the rest of the linespec.  */
  linespec_parse_basic (parser);

  if (PARSER_RESULT (parser)->function_symbols == NULL
      && PARSER_RESULT (parser)->labels.label_symbols == NULL
      && PARSER_EXPLICIT (parser)->line_offset.sign == LINE_OFFSET_UNKNOWN
      && PARSER_RESULT (parser)->minimal_symbols == NULL)
    {
      /* The linespec didn't parse.  Re-throw the file exception if
	 there was one.  */
      if (file_exception.reason < 0)
	throw_exception (file_exception);

      /* Otherwise, the symbol is not found.  */
      symbol_not_found_error (PARSER_EXPLICIT (parser)->function_name,
			      PARSER_EXPLICIT (parser)->source_filename);
    }

 convert_to_sals:

  /* Get the last token and record how much of the input was parsed,
     if necessary.  */
  token = linespec_lexer_lex_one (parser);
  if (token.type != LSTOKEN_EOI && token.type != LSTOKEN_KEYWORD)
    PARSER_STREAM (parser) = LS_TOKEN_STOKEN (token).ptr;

  /* Convert the data in PARSER_RESULT to SALs.  */
  values = convert_linespec_to_sals (PARSER_STATE (parser),
				     PARSER_RESULT (parser));

  return values;
}


/* A constructor for linespec_state.  */

static void
linespec_state_constructor (struct linespec_state *self,
			    int flags, const struct language_defn *language,
			    struct program_space *search_pspace,
			    struct symtab *default_symtab,
			    int default_line,
			    struct linespec_result *canonical)
{
  memset (self, 0, sizeof (*self));
  self->language = language;
  self->funfirstline = (flags & DECODE_LINE_FUNFIRSTLINE) ? 1 : 0;
  self->list_mode = (flags & DECODE_LINE_LIST_MODE) ? 1 : 0;
  self->search_pspace = search_pspace;
  self->default_symtab = default_symtab;
  self->default_line = default_line;
  self->canonical = canonical;
  self->program_space = current_program_space;
  self->addr_set = htab_create_alloc (10, hash_address_entry, eq_address_entry,
				      xfree, xcalloc, xfree);
  self->is_linespec = 0;
}

/* Initialize a new linespec parser.  */

static void
linespec_parser_new (linespec_parser *parser,
		     int flags, const struct language_defn *language,
		     struct program_space *search_pspace,
		     struct symtab *default_symtab,
		     int default_line,
		     struct linespec_result *canonical)
{
  memset (parser, 0, sizeof (linespec_parser));
  parser->lexer.current.type = LSTOKEN_CONSUMED;
  memset (PARSER_RESULT (parser), 0, sizeof (struct linespec));
  PARSER_EXPLICIT (parser)->line_offset.sign = LINE_OFFSET_UNKNOWN;
  linespec_state_constructor (PARSER_STATE (parser), flags, language,
			      search_pspace,
			      default_symtab, default_line, canonical);
}

/* A destructor for linespec_state.  */

static void
linespec_state_destructor (struct linespec_state *self)
{
  htab_delete (self->addr_set);
}

/* Delete a linespec parser.  */

static void
linespec_parser_delete (void *arg)
{
  linespec_parser *parser = (linespec_parser *) arg;

  xfree (PARSER_EXPLICIT (parser)->source_filename);
  xfree (PARSER_EXPLICIT (parser)->label_name);
  xfree (PARSER_EXPLICIT (parser)->function_name);

  if (PARSER_RESULT (parser)->file_symtabs != NULL)
    VEC_free (symtab_ptr, PARSER_RESULT (parser)->file_symtabs);

  if (PARSER_RESULT (parser)->function_symbols != NULL)
    VEC_free (symbolp, PARSER_RESULT (parser)->function_symbols);

  if (PARSER_RESULT (parser)->minimal_symbols != NULL)
    VEC_free (bound_minimal_symbol_d, PARSER_RESULT (parser)->minimal_symbols);

  if (PARSER_RESULT (parser)->labels.label_symbols != NULL)
    VEC_free (symbolp, PARSER_RESULT (parser)->labels.label_symbols);

  if (PARSER_RESULT (parser)->labels.function_symbols != NULL)
    VEC_free (symbolp, PARSER_RESULT (parser)->labels.function_symbols);

  linespec_state_destructor (PARSER_STATE (parser));
}

/* See description in linespec.h.  */

void
linespec_lex_to_end (char **stringp)
{
  linespec_parser parser;
  struct cleanup *cleanup;
  linespec_token token;
  const char *orig;

  if (stringp == NULL || *stringp == NULL)
    return;

  linespec_parser_new (&parser, 0, current_language, NULL, NULL, 0, NULL);
  cleanup = make_cleanup (linespec_parser_delete, &parser);
  parser.lexer.saved_arg = *stringp;
  PARSER_STREAM (&parser) = orig = *stringp;

  do
    {
      /* Stop before any comma tokens;  we need it to keep it
	 as the next token in the string.  */
      token = linespec_lexer_peek_token (&parser);
      if (token.type == LSTOKEN_COMMA)
	break;
      token = linespec_lexer_consume_token (&parser);
    }
  while (token.type != LSTOKEN_EOI && token.type != LSTOKEN_KEYWORD);

  *stringp += PARSER_STREAM (&parser) - orig;
  do_cleanups (cleanup);
}

/* A helper function for decode_line_full and decode_line_1 to
   turn LOCATION into symtabs_and_lines.  */

static struct symtabs_and_lines
event_location_to_sals (linespec_parser *parser,
			const struct event_location *location)
{
  struct symtabs_and_lines result = {NULL, 0};

  switch (event_location_type (location))
    {
    case LINESPEC_LOCATION:
      {
	PARSER_STATE (parser)->is_linespec = 1;
	TRY
	  {
	    result = parse_linespec (parser, get_linespec_location (location));
	  }
	CATCH (except, RETURN_MASK_ERROR)
	  {
	    throw_exception (except);
	  }
	END_CATCH
      }
      break;

    case ADDRESS_LOCATION:
      {
	const char *addr_string = get_address_string_location (location);
	CORE_ADDR addr = get_address_location (location);

	if (addr_string != NULL)
	  {
	    char *expr = xstrdup (addr_string);
	    const char *const_expr = expr;
	    struct cleanup *cleanup = make_cleanup (xfree, expr);

	    addr = linespec_expression_to_pc (&const_expr);
	    if (PARSER_STATE (parser)->canonical != NULL)
	      PARSER_STATE (parser)->canonical->location
		= copy_event_location (location);

	    do_cleanups (cleanup);
	  }

	result = convert_address_location_to_sals (PARSER_STATE (parser),
						   addr);
      }
      break;

    case EXPLICIT_LOCATION:
      {
	const struct explicit_location *explicit_loc;

	explicit_loc = get_explicit_location_const (location);
	result = convert_explicit_location_to_sals (PARSER_STATE (parser),
						    PARSER_RESULT (parser),
						    explicit_loc);
      }
      break;

    case PROBE_LOCATION:
      /* Probes are handled by their own decoders.  */
      gdb_assert_not_reached ("attempt to decode probe location");
      break;

    default:
      gdb_assert_not_reached ("unhandled event location type");
    }

  return result;
}

/* See linespec.h.  */

void
decode_line_full (const struct event_location *location, int flags,
		  struct program_space *search_pspace,
		  struct symtab *default_symtab,
		  int default_line, struct linespec_result *canonical,
		  const char *select_mode,
		  const char *filter)
{
  struct symtabs_and_lines result;
  struct cleanup *cleanups;
  VEC (const_char_ptr) *filters = NULL;
  linespec_parser parser;
  struct linespec_state *state;

  gdb_assert (canonical != NULL);
  /* The filter only makes sense for 'all'.  */
  gdb_assert (filter == NULL || select_mode == multiple_symbols_all);
  gdb_assert (select_mode == NULL
	      || select_mode == multiple_symbols_all
	      || select_mode == multiple_symbols_ask
	      || select_mode == multiple_symbols_cancel);
  gdb_assert ((flags & DECODE_LINE_LIST_MODE) == 0);

  linespec_parser_new (&parser, flags, current_language,
		       search_pspace, default_symtab,
		       default_line, canonical);
  cleanups = make_cleanup (linespec_parser_delete, &parser);
  save_current_program_space ();

  result = event_location_to_sals (&parser, location);
  state = PARSER_STATE (&parser);

  gdb_assert (result.nelts == 1 || canonical->pre_expanded);
  canonical->pre_expanded = 1;

  /* Arrange for allocated canonical names to be freed.  */
  if (result.nelts > 0)
    {
      int i;

      make_cleanup (xfree, state->canonical_names);
      for (i = 0; i < result.nelts; ++i)
	{
	  gdb_assert (state->canonical_names[i].suffix != NULL);
	  make_cleanup (xfree, state->canonical_names[i].suffix);
	}
    }

  if (select_mode == NULL)
    {
      if (interp_ui_out (top_level_interpreter ())->is_mi_like_p ())
	select_mode = multiple_symbols_all;
      else
	select_mode = multiple_symbols_select_mode ();
    }

  if (select_mode == multiple_symbols_all)
    {
      if (filter != NULL)
	{
	  make_cleanup (VEC_cleanup (const_char_ptr), &filters);
	  VEC_safe_push (const_char_ptr, filters, filter);
	  filter_results (state, &result, filters);
	}
      else
	convert_results_to_lsals (state, &result);
    }
  else
    decode_line_2 (state, &result, select_mode);

  do_cleanups (cleanups);
}

/* See linespec.h.  */

struct symtabs_and_lines
decode_line_1 (const struct event_location *location, int flags,
	       struct program_space *search_pspace,
	       struct symtab *default_symtab,
	       int default_line)
{
  struct symtabs_and_lines result;
  linespec_parser parser;
  struct cleanup *cleanups;

  linespec_parser_new (&parser, flags, current_language,
		       search_pspace, default_symtab,
		       default_line, NULL);
  cleanups = make_cleanup (linespec_parser_delete, &parser);
  save_current_program_space ();

  result = event_location_to_sals (&parser, location);

  do_cleanups (cleanups);
  return result;
}

/* See linespec.h.  */

struct symtabs_and_lines
decode_line_with_current_source (char *string, int flags)
{
  struct symtabs_and_lines sals;
  struct symtab_and_line cursal;

  if (string == 0)
    error (_("Empty line specification."));

  /* We use whatever is set as the current source line.  We do not try
     and get a default source symtab+line or it will recursively call us!  */
  cursal = get_current_source_symtab_and_line ();

  event_location_up location = string_to_event_location (&string,
							 current_language);
  sals = decode_line_1 (location.get (), flags, NULL,
			cursal.symtab, cursal.line);

  if (*string)
    error (_("Junk at end of line specification: %s"), string);

  return sals;
}

/* See linespec.h.  */

struct symtabs_and_lines
decode_line_with_last_displayed (char *string, int flags)
{
  struct symtabs_and_lines sals;

  if (string == 0)
    error (_("Empty line specification."));

  event_location_up location = string_to_event_location (&string,
							 current_language);
  if (last_displayed_sal_is_valid ())
    sals = decode_line_1 (location.get (), flags, NULL,
			  get_last_displayed_symtab (),
			  get_last_displayed_line ());
  else
    sals = decode_line_1 (location.get (), flags, NULL,
			  (struct symtab *) NULL, 0);

  if (*string)
    error (_("Junk at end of line specification: %s"), string);

  return sals;
}



/* First, some functions to initialize stuff at the beggining of the
   function.  */

static void
initialize_defaults (struct symtab **default_symtab, int *default_line)
{
  if (*default_symtab == 0)
    {
      /* Use whatever we have for the default source line.  We don't use
         get_current_or_default_symtab_and_line as it can recurse and call
	 us back!  */
      struct symtab_and_line cursal = 
	get_current_source_symtab_and_line ();
      
      *default_symtab = cursal.symtab;
      *default_line = cursal.line;
    }
}



/* Evaluate the expression pointed to by EXP_PTR into a CORE_ADDR,
   advancing EXP_PTR past any parsed text.  */

CORE_ADDR
linespec_expression_to_pc (const char **exp_ptr)
{
  if (current_program_space->executing_startup)
    /* The error message doesn't really matter, because this case
       should only hit during breakpoint reset.  */
    throw_error (NOT_FOUND_ERROR, _("cannot evaluate expressions while "
				    "program space is in startup"));

  (*exp_ptr)++;
  return value_as_address (parse_to_comma_and_eval (exp_ptr));
}



/* Here's where we recognise an Objective-C Selector.  An Objective C
   selector may be implemented by more than one class, therefore it
   may represent more than one method/function.  This gives us a
   situation somewhat analogous to C++ overloading.  If there's more
   than one method that could represent the selector, then use some of
   the existing C++ code to let the user choose one.  */

static struct symtabs_and_lines
decode_objc (struct linespec_state *self, linespec_p ls, const char *arg)
{
  struct collect_info info;
  VEC (const_char_ptr) *symbol_names = NULL;
  struct symtabs_and_lines values;
  const char *new_argptr;
  struct cleanup *cleanup = make_cleanup (VEC_cleanup (const_char_ptr),
					  &symbol_names);

  info.state = self;
  info.file_symtabs = NULL;
  VEC_safe_push (symtab_ptr, info.file_symtabs, NULL);
  make_cleanup (VEC_cleanup (symtab_ptr), &info.file_symtabs);
  info.result.symbols = NULL;
  info.result.minimal_symbols = NULL;
  values.nelts = 0;
  values.sals = NULL;

  new_argptr = find_imps (arg, &symbol_names);
  if (VEC_empty (const_char_ptr, symbol_names))
    {
      do_cleanups (cleanup);
      return values;
    }

  add_all_symbol_names_from_pspace (&info, NULL, symbol_names);

  if (!VEC_empty (symbolp, info.result.symbols)
      || !VEC_empty (bound_minimal_symbol_d, info.result.minimal_symbols))
    {
      char *saved_arg;

      saved_arg = (char *) alloca (new_argptr - arg + 1);
      memcpy (saved_arg, arg, new_argptr - arg);
      saved_arg[new_argptr - arg] = '\0';

      ls->explicit_loc.function_name = xstrdup (saved_arg);
      ls->function_symbols = info.result.symbols;
      ls->minimal_symbols = info.result.minimal_symbols;
      values = convert_linespec_to_sals (self, ls);

      if (self->canonical)
	{
	  char *str;

	  self->canonical->pre_expanded = 1;

	  if (ls->explicit_loc.source_filename)
	    {
	      str = xstrprintf ("%s:%s",
				ls->explicit_loc.source_filename, saved_arg);
	    }
	  else
	    str = xstrdup (saved_arg);

	  make_cleanup (xfree, str);
	  self->canonical->location = new_linespec_location (&str);
	}
    }

  do_cleanups (cleanup);

  return values;
}

namespace {

/* A function object that serves as symbol_found_callback_ftype
   callback for iterate_over_symbols.  This is used by
   lookup_prefix_sym to collect type symbols.  */
class decode_compound_collector
{
public:
  decode_compound_collector ()
    : m_symbols (NULL)
  {
    m_unique_syms = htab_create_alloc (1, htab_hash_pointer,
				       htab_eq_pointer, NULL,
				       xcalloc, xfree);
  }

  ~decode_compound_collector ()
  {
    if (m_unique_syms != NULL)
      htab_delete (m_unique_syms);
  }

  /* Releases ownership of the collected symbols and returns them.  */
  VEC (symbolp) *release_symbols ()
  {
    VEC (symbolp) *res = m_symbols;
    m_symbols = NULL;
    return res;
  }

  /* Callable as a symbol_found_callback_ftype callback.  */
  bool operator () (symbol *sym);

private:
  /* A hash table of all symbols we found.  We use this to avoid
     adding any symbol more than once.  */
  htab_t m_unique_syms;

  /* The result vector.  */
  VEC (symbolp) *m_symbols;
};

bool
decode_compound_collector::operator () (symbol *sym)
{
  void **slot;
  struct type *t;

  if (SYMBOL_CLASS (sym) != LOC_TYPEDEF)
    return true; /* Continue iterating.  */

  t = SYMBOL_TYPE (sym);
  t = check_typedef (t);
  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION
      && TYPE_CODE (t) != TYPE_CODE_NAMESPACE)
    return true; /* Continue iterating.  */

  slot = htab_find_slot (m_unique_syms, sym, INSERT);
  if (!*slot)
    {
      *slot = sym;
      VEC_safe_push (symbolp, m_symbols, sym);
    }

  return true; /* Continue iterating.  */
}

} // namespace

/* Return any symbols corresponding to CLASS_NAME in FILE_SYMTABS.  */

static VEC (symbolp) *
lookup_prefix_sym (struct linespec_state *state, VEC (symtab_ptr) *file_symtabs,
		   const char *class_name)
{
  int ix;
  struct symtab *elt;
  decode_compound_collector collector;

  for (ix = 0; VEC_iterate (symtab_ptr, file_symtabs, ix, elt); ++ix)
    {
      if (elt == NULL)
	{
	  iterate_over_all_matching_symtabs (state, class_name, STRUCT_DOMAIN,
					     NULL, false, collector);
	  iterate_over_all_matching_symtabs (state, class_name, VAR_DOMAIN,
					     NULL, false, collector);
	}
      else
	{
	  /* Program spaces that are executing startup should have
	     been filtered out earlier.  */
	  gdb_assert (!SYMTAB_PSPACE (elt)->executing_startup);
	  set_current_program_space (SYMTAB_PSPACE (elt));
	  iterate_over_file_blocks (elt, class_name, STRUCT_DOMAIN, collector);
	  iterate_over_file_blocks (elt, class_name, VAR_DOMAIN, collector);
	}
    }

  return collector.release_symbols ();
}

/* A qsort comparison function for symbols.  The resulting order does
   not actually matter; we just need to be able to sort them so that
   symbols with the same program space end up next to each other.  */

static int
compare_symbols (const void *a, const void *b)
{
  struct symbol * const *sa = (struct symbol * const*) a;
  struct symbol * const *sb = (struct symbol * const*) b;
  uintptr_t uia, uib;

  uia = (uintptr_t) SYMTAB_PSPACE (symbol_symtab (*sa));
  uib = (uintptr_t) SYMTAB_PSPACE (symbol_symtab (*sb));

  if (uia < uib)
    return -1;
  if (uia > uib)
    return 1;

  uia = (uintptr_t) *sa;
  uib = (uintptr_t) *sb;

  if (uia < uib)
    return -1;
  if (uia > uib)
    return 1;

  return 0;
}

/* Like compare_symbols but for minimal symbols.  */

static int
compare_msymbols (const void *a, const void *b)
{
  const struct bound_minimal_symbol *sa
    = (const struct bound_minimal_symbol *) a;
  const struct bound_minimal_symbol *sb
    = (const struct bound_minimal_symbol *) b;
  uintptr_t uia, uib;

  uia = (uintptr_t) sa->objfile->pspace;
  uib = (uintptr_t) sa->objfile->pspace;

  if (uia < uib)
    return -1;
  if (uia > uib)
    return 1;

  uia = (uintptr_t) sa->minsym;
  uib = (uintptr_t) sb->minsym;

  if (uia < uib)
    return -1;
  if (uia > uib)
    return 1;

  return 0;
}

/* Look for all the matching instances of each symbol in NAMES.  Only
   instances from PSPACE are considered; other program spaces are
   handled by our caller.  If PSPACE is NULL, then all program spaces
   are considered.  Results are stored into INFO.  */

static void
add_all_symbol_names_from_pspace (struct collect_info *info,
				  struct program_space *pspace,
				  VEC (const_char_ptr) *names)
{
  int ix;
  const char *iter;

  for (ix = 0; VEC_iterate (const_char_ptr, names, ix, iter); ++ix)
    add_matching_symbols_to_info (iter, info, pspace);
}

static void
find_superclass_methods (VEC (typep) *superclasses,
			 const char *name,
			 VEC (const_char_ptr) **result_names)
{
  int old_len = VEC_length (const_char_ptr, *result_names);
  VEC (typep) *iter_classes;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);

  iter_classes = superclasses;
  while (1)
    {
      VEC (typep) *new_supers = NULL;
      int ix;
      struct type *t;

      make_cleanup (VEC_cleanup (typep), &new_supers);
      for (ix = 0; VEC_iterate (typep, iter_classes, ix, t); ++ix)
	find_methods (t, name, result_names, &new_supers);

      if (VEC_length (const_char_ptr, *result_names) != old_len
	  || VEC_empty (typep, new_supers))
	break;

      iter_classes = new_supers;
    }

  do_cleanups (cleanup);
}

/* This finds the method METHOD_NAME in the class CLASS_NAME whose type is
   given by one of the symbols in SYM_CLASSES.  Matches are returned
   in SYMBOLS (for debug symbols) and MINSYMS (for minimal symbols).  */

static void
find_method (struct linespec_state *self, VEC (symtab_ptr) *file_symtabs,
	     const char *class_name, const char *method_name,
	     VEC (symbolp) *sym_classes, VEC (symbolp) **symbols,
	     VEC (bound_minimal_symbol_d) **minsyms)
{
  struct symbol *sym;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);
  int ix;
  int last_result_len;
  VEC (typep) *superclass_vec;
  VEC (const_char_ptr) *result_names;
  struct collect_info info;

  /* Sort symbols so that symbols with the same program space are next
     to each other.  */
  qsort (VEC_address (symbolp, sym_classes),
	 VEC_length (symbolp, sym_classes),
	 sizeof (symbolp),
	 compare_symbols);

  info.state = self;
  info.file_symtabs = file_symtabs;
  info.result.symbols = NULL;
  info.result.minimal_symbols = NULL;

  /* Iterate over all the types, looking for the names of existing
     methods matching METHOD_NAME.  If we cannot find a direct method in a
     given program space, then we consider inherited methods; this is
     not ideal (ideal would be to respect C++ hiding rules), but it
     seems good enough and is what GDB has historically done.  We only
     need to collect the names because later we find all symbols with
     those names.  This loop is written in a somewhat funny way
     because we collect data across the program space before deciding
     what to do.  */
  superclass_vec = NULL;
  make_cleanup (VEC_cleanup (typep), &superclass_vec);
  result_names = NULL;
  make_cleanup (VEC_cleanup (const_char_ptr), &result_names);
  last_result_len = 0;
  for (ix = 0; VEC_iterate (symbolp, sym_classes, ix, sym); ++ix)
    {
      struct type *t;
      struct program_space *pspace;

      /* Program spaces that are executing startup should have
	 been filtered out earlier.  */
      pspace = SYMTAB_PSPACE (symbol_symtab (sym));
      gdb_assert (!pspace->executing_startup);
      set_current_program_space (pspace);
      t = check_typedef (SYMBOL_TYPE (sym));
      find_methods (t, method_name, &result_names, &superclass_vec);

      /* Handle all items from a single program space at once; and be
	 sure not to miss the last batch.  */
      if (ix == VEC_length (symbolp, sym_classes) - 1
	  || (pspace
	      != SYMTAB_PSPACE (symbol_symtab (VEC_index (symbolp, sym_classes,
							  ix + 1)))))
	{
	  /* If we did not find a direct implementation anywhere in
	     this program space, consider superclasses.  */
	  if (VEC_length (const_char_ptr, result_names) == last_result_len)
	    find_superclass_methods (superclass_vec, method_name,
				     &result_names);

	  /* We have a list of candidate symbol names, so now we
	     iterate over the symbol tables looking for all
	     matches in this pspace.  */
	  add_all_symbol_names_from_pspace (&info, pspace, result_names);

	  VEC_truncate (typep, superclass_vec, 0);
	  last_result_len = VEC_length (const_char_ptr, result_names);
	}
    }

  if (!VEC_empty (symbolp, info.result.symbols)
      || !VEC_empty (bound_minimal_symbol_d, info.result.minimal_symbols))
    {
      *symbols = info.result.symbols;
      *minsyms = info.result.minimal_symbols;
      do_cleanups (cleanup);
      return;
    }

  /* Throw an NOT_FOUND_ERROR.  This will be caught by the caller
     and other attempts to locate the symbol will be made.  */
  throw_error (NOT_FOUND_ERROR, _("see caller, this text doesn't matter"));
}



namespace {

/* This function object is a callback for iterate_over_symtabs, used
   when collecting all matching symtabs.  */

class symtab_collector
{
public:
  symtab_collector ()
  {
    m_symtabs = NULL;
    m_symtab_table = htab_create (1, htab_hash_pointer, htab_eq_pointer,
				  NULL);
  }

  ~symtab_collector ()
  {
    if (m_symtab_table != NULL)
      htab_delete (m_symtab_table);
  }

  /* Callable as a symbol_found_callback_ftype callback.  */
  bool operator () (symtab *sym);

  /* Releases ownership of the collected symtabs and returns them.  */
  VEC (symtab_ptr) *release_symtabs ()
  {
    VEC (symtab_ptr) *res = m_symtabs;
    m_symtabs = NULL;
    return res;
  }

private:
  /* The result vector of symtabs.  */
  VEC (symtab_ptr) *m_symtabs;

  /* This is used to ensure the symtabs are unique.  */
  htab_t m_symtab_table;
};

bool
symtab_collector::operator () (struct symtab *symtab)
{
  void **slot;

  slot = htab_find_slot (m_symtab_table, symtab, INSERT);
  if (!*slot)
    {
      *slot = symtab;
      VEC_safe_push (symtab_ptr, m_symtabs, symtab);
    }

  return false;
}

} // namespace

/* Given a file name, return a VEC of all matching symtabs.  If
   SEARCH_PSPACE is not NULL, the search is restricted to just that
   program space.  */

static VEC (symtab_ptr) *
collect_symtabs_from_filename (const char *file,
			       struct program_space *search_pspace)
{
  symtab_collector collector;

  /* Find that file's data.  */
  if (search_pspace == NULL)
    {
      struct program_space *pspace;

      ALL_PSPACES (pspace)
        {
	  if (pspace->executing_startup)
	    continue;

	  set_current_program_space (pspace);
	  iterate_over_symtabs (file, collector);
	}
    }
  else
    {
      set_current_program_space (search_pspace);
      iterate_over_symtabs (file, collector);
    }

  return collector.release_symtabs ();
}

/* Return all the symtabs associated to the FILENAME.  If SEARCH_PSPACE is
   not NULL, the search is restricted to just that program space.  */

static VEC (symtab_ptr) *
symtabs_from_filename (const char *filename,
		       struct program_space *search_pspace)
{
  VEC (symtab_ptr) *result;
  
  result = collect_symtabs_from_filename (filename, search_pspace);

  if (VEC_empty (symtab_ptr, result))
    {
      if (!have_full_symbols () && !have_partial_symbols ())
	throw_error (NOT_FOUND_ERROR,
		     _("No symbol table is loaded.  "
		       "Use the \"file\" command."));
      source_file_not_found_error (filename);
    }

  return result;
}

/* Look up a function symbol named NAME in symtabs FILE_SYMTABS.  Matching
   debug symbols are returned in SYMBOLS.  Matching minimal symbols are
   returned in MINSYMS.  */

static void
find_function_symbols (struct linespec_state *state,
		       VEC (symtab_ptr) *file_symtabs, const char *name,
		       VEC (symbolp) **symbols,
		       VEC (bound_minimal_symbol_d) **minsyms)
{
  struct collect_info info;
  VEC (const_char_ptr) *symbol_names = NULL;
  struct cleanup *cleanup = make_cleanup (VEC_cleanup (const_char_ptr),
					  &symbol_names);

  info.state = state;
  info.result.symbols = NULL;
  info.result.minimal_symbols = NULL;
  info.file_symtabs = file_symtabs;

  /* Try NAME as an Objective-C selector.  */
  find_imps (name, &symbol_names);
  if (!VEC_empty (const_char_ptr, symbol_names))
    add_all_symbol_names_from_pspace (&info, state->search_pspace,
				      symbol_names);
  else
    add_matching_symbols_to_info (name, &info, state->search_pspace);

  do_cleanups (cleanup);

  if (VEC_empty (symbolp, info.result.symbols))
    {
      VEC_free (symbolp, info.result.symbols);
      *symbols = NULL;
    }
  else
    *symbols = info.result.symbols;

  if (VEC_empty (bound_minimal_symbol_d, info.result.minimal_symbols))
    {
      VEC_free (bound_minimal_symbol_d, info.result.minimal_symbols);
      *minsyms = NULL;
    }
  else
    *minsyms = info.result.minimal_symbols;
}

/* Find all symbols named NAME in FILE_SYMTABS, returning debug symbols
   in SYMBOLS and minimal symbols in MINSYMS.  */

static void
find_linespec_symbols (struct linespec_state *state,
		       VEC (symtab_ptr) *file_symtabs,
		       const char *name,
		       VEC (symbolp) **symbols,
		       VEC (bound_minimal_symbol_d) **minsyms)
{
  demangle_result_storage demangle_storage;
  std::string ada_lookup_storage;
  const char *lookup_name;

  if (state->language->la_language == language_ada)
    {
      /* In Ada, the symbol lookups are performed using the encoded
         name rather than the demangled name.  */
      ada_lookup_storage = ada_name_for_lookup (name);
      lookup_name = ada_lookup_storage.c_str ();
    }
  else
    {
      lookup_name = demangle_for_lookup (name,
					 state->language->la_language,
					 demangle_storage);
    }

  std::string canon = cp_canonicalize_string_no_typedefs (lookup_name);
  if (!canon.empty ())
    lookup_name = canon.c_str ();

  /* It's important to not call expand_symtabs_matching unnecessarily
     as it can really slow things down (by unnecessarily expanding
     potentially 1000s of symtabs, which when debugging some apps can
     cost 100s of seconds).  Avoid this to some extent by *first* calling
     find_function_symbols, and only if that doesn't find anything
     *then* call find_method.  This handles two important cases:
     1) break (anonymous namespace)::foo
     2) break class::method where method is in class (and not a baseclass)  */

  find_function_symbols (state, file_symtabs, lookup_name,
			 symbols, minsyms);

  /* If we were unable to locate a symbol of the same name, try dividing
     the name into class and method names and searching the class and its
     baseclasses.  */
  if (VEC_empty (symbolp, *symbols)
      && VEC_empty (bound_minimal_symbol_d, *minsyms))
    {
      std::string klass, method;
      const char *last, *p, *scope_op;
      VEC (symbolp) *classes;

      /* See if we can find a scope operator and break this symbol
	 name into namespaces${SCOPE_OPERATOR}class_name and method_name.  */
      scope_op = "::";
      p = find_toplevel_string (lookup_name, scope_op);

      last = NULL;
      while (p != NULL)
	{
	  last = p;
	  p = find_toplevel_string (p + strlen (scope_op), scope_op);
	}

      /* If no scope operator was found, there is nothing more we can do;
	 we already attempted to lookup the entire name as a symbol
	 and failed.  */
      if (last == NULL)
	return;

      /* LOOKUP_NAME points to the class name.
	 LAST points to the method name.  */
      klass = std::string (lookup_name, last - lookup_name);

      /* Skip past the scope operator.  */
      last += strlen (scope_op);
      method = last;

      /* Find a list of classes named KLASS.  */
      classes = lookup_prefix_sym (state, file_symtabs, klass.c_str ());
      struct cleanup *old_chain
	= make_cleanup (VEC_cleanup (symbolp), &classes);

      if (!VEC_empty (symbolp, classes))
	{
	  /* Now locate a list of suitable methods named METHOD.  */
	  TRY
	    {
	      find_method (state, file_symtabs,
			   klass.c_str (), method.c_str (),
			   classes, symbols, minsyms);
	    }

	  /* If successful, we're done.  If NOT_FOUND_ERROR
	     was not thrown, rethrow the exception that we did get.  */
	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      if (except.error != NOT_FOUND_ERROR)
		throw_exception (except);
	    }
	  END_CATCH
	}

      do_cleanups (old_chain);
    }
}

/* Return all labels named NAME in FUNCTION_SYMBOLS.  Return the
   actual function symbol in which the label was found in LABEL_FUNC_RET.  */

static VEC (symbolp) *
find_label_symbols (struct linespec_state *self,
		    VEC (symbolp) *function_symbols,
		    VEC (symbolp) **label_funcs_ret, const char *name)
{
  int ix;
  const struct block *block;
  struct symbol *sym;
  struct symbol *fn_sym;
  VEC (symbolp) *result = NULL;

  if (function_symbols == NULL)
    {
      set_current_program_space (self->program_space);
      block = get_current_search_block ();

      for (;
	   block && !BLOCK_FUNCTION (block);
	   block = BLOCK_SUPERBLOCK (block))
	;
      if (!block)
	return NULL;
      fn_sym = BLOCK_FUNCTION (block);

      sym = lookup_symbol (name, block, LABEL_DOMAIN, 0).symbol;

      if (sym != NULL)
	{
	  VEC_safe_push (symbolp, result, sym);
	  VEC_safe_push (symbolp, *label_funcs_ret, fn_sym);
	}
    }
  else
    {
      for (ix = 0;
	   VEC_iterate (symbolp, function_symbols, ix, fn_sym); ++ix)
	{
	  set_current_program_space (SYMTAB_PSPACE (symbol_symtab (fn_sym)));
	  block = SYMBOL_BLOCK_VALUE (fn_sym);
	  sym = lookup_symbol (name, block, LABEL_DOMAIN, 0).symbol;

	  if (sym != NULL)
	    {
	      VEC_safe_push (symbolp, result, sym);
	      VEC_safe_push (symbolp, *label_funcs_ret, fn_sym);
	    }
	}
    }

  return result;
}



/* A helper for create_sals_line_offset that handles the 'list_mode' case.  */

static void
decode_digits_list_mode (struct linespec_state *self,
			 linespec_p ls,
			 struct symtabs_and_lines *values,
			 struct symtab_and_line val)
{
  int ix;
  struct symtab *elt;

  gdb_assert (self->list_mode);

  for (ix = 0; VEC_iterate (symtab_ptr, ls->file_symtabs, ix, elt);
       ++ix)
    {
      /* The logic above should ensure this.  */
      gdb_assert (elt != NULL);

      set_current_program_space (SYMTAB_PSPACE (elt));

      /* Simplistic search just for the list command.  */
      val.symtab = find_line_symtab (elt, val.line, NULL, NULL);
      if (val.symtab == NULL)
	val.symtab = elt;
      val.pspace = SYMTAB_PSPACE (elt);
      val.pc = 0;
      val.explicit_line = 1;

      add_sal_to_sals (self, values, &val, NULL, 0);
    }
}

/* A helper for create_sals_line_offset that iterates over the symtabs,
   adding lines to the VEC.  */

static void
decode_digits_ordinary (struct linespec_state *self,
			linespec_p ls,
			int line,
			struct symtabs_and_lines *sals,
			struct linetable_entry **best_entry)
{
  int ix;
  struct symtab *elt;

  for (ix = 0; VEC_iterate (symtab_ptr, ls->file_symtabs, ix, elt); ++ix)
    {
      std::vector<CORE_ADDR> pcs;

      /* The logic above should ensure this.  */
      gdb_assert (elt != NULL);

      set_current_program_space (SYMTAB_PSPACE (elt));

      pcs = find_pcs_for_symtab_line (elt, line, best_entry);
      for (CORE_ADDR pc : pcs)
	{
	  struct symtab_and_line sal;

	  init_sal (&sal);
	  sal.pspace = SYMTAB_PSPACE (elt);
	  sal.symtab = elt;
	  sal.line = line;
	  sal.pc = pc;
	  add_sal_to_sals_basic (sals, &sal);
	}
    }
}



/* Return the line offset represented by VARIABLE.  */

static struct line_offset
linespec_parse_variable (struct linespec_state *self, const char *variable)
{
  int index = 0;
  const char *p;
  struct line_offset offset = {0, LINE_OFFSET_NONE};

  p = (variable[1] == '$') ? variable + 2 : variable + 1;
  if (*p == '$')
    ++p;
  while (*p >= '0' && *p <= '9')
    ++p;
  if (!*p)		/* Reached end of token without hitting non-digit.  */
    {
      /* We have a value history reference.  */
      struct value *val_history;

      sscanf ((variable[1] == '$') ? variable + 2 : variable + 1, "%d", &index);
      val_history
	= access_value_history ((variable[1] == '$') ? -index : index);
      if (TYPE_CODE (value_type (val_history)) != TYPE_CODE_INT)
	error (_("History values used in line "
		 "specs must have integer values."));
      offset.offset = value_as_long (val_history);
    }
  else
    {
      /* Not all digits -- may be user variable/function or a
	 convenience variable.  */
      LONGEST valx;
      struct internalvar *ivar;

      /* Try it as a convenience variable.  If it is not a convenience
	 variable, return and allow normal symbol lookup to occur.  */
      ivar = lookup_only_internalvar (variable + 1);
      if (ivar == NULL)
	/* No internal variable with that name.  Mark the offset
	   as unknown to allow the name to be looked up as a symbol.  */
	offset.sign = LINE_OFFSET_UNKNOWN;
      else
	{
	  /* We found a valid variable name.  If it is not an integer,
	     throw an error.  */
	  if (!get_internalvar_integer (ivar, &valx))
	    error (_("Convenience variables used in line "
		     "specs must have integer values."));
	  else
	    offset.offset = valx;
	}
    }

  return offset;
}


/* We've found a minimal symbol MSYMBOL in OBJFILE to associate with our
   linespec; return the SAL in RESULT.  This function should return SALs
   matching those from find_function_start_sal, otherwise false
   multiple-locations breakpoints could be placed.  */

static void
minsym_found (struct linespec_state *self, struct objfile *objfile,
	      struct minimal_symbol *msymbol,
	      struct symtabs_and_lines *result)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  CORE_ADDR pc;
  struct symtab_and_line sal;

  sal = find_pc_sect_line (MSYMBOL_VALUE_ADDRESS (objfile, msymbol),
			   (struct obj_section *) 0, 0);
  sal.section = MSYMBOL_OBJ_SECTION (objfile, msymbol);

  /* The minimal symbol might point to a function descriptor;
     resolve it to the actual code address instead.  */
  pc = gdbarch_convert_from_func_ptr_addr (gdbarch, sal.pc, &current_target);
  if (pc != sal.pc)
    sal = find_pc_sect_line (pc, NULL, 0);

  if (self->funfirstline)
    {
      if (sal.symtab != NULL
	  && (COMPUNIT_LOCATIONS_VALID (SYMTAB_COMPUNIT (sal.symtab))
	      || SYMTAB_LANGUAGE (sal.symtab) == language_asm))
	{
	  /* If gdbarch_convert_from_func_ptr_addr does not apply then
	     sal.SECTION, sal.LINE&co. will stay correct from above.
	     If gdbarch_convert_from_func_ptr_addr applies then
	     sal.SECTION is cleared from above and sal.LINE&co. will
	     stay correct from the last find_pc_sect_line above.  */
	  sal.pc = MSYMBOL_VALUE_ADDRESS (objfile, msymbol);
	  sal.pc = gdbarch_convert_from_func_ptr_addr (gdbarch, sal.pc,
						       &current_target);
	  if (gdbarch_skip_entrypoint_p (gdbarch))
	    sal.pc = gdbarch_skip_entrypoint (gdbarch, sal.pc);
	}
      else
	skip_prologue_sal (&sal);
    }

  if (maybe_add_address (self->addr_set, objfile->pspace, sal.pc))
    add_sal_to_sals (self, result, &sal, MSYMBOL_NATURAL_NAME (msymbol), 0);
}

/* A helper struct to pass some data through
   iterate_over_minimal_symbols.  */

struct collect_minsyms
{
  /* The objfile we're examining.  */
  struct objfile *objfile;

  /* Only search the given symtab, or NULL to search for all symbols.  */
  struct symtab *symtab;

  /* The funfirstline setting from the initial call.  */
  int funfirstline;

  /* The list_mode setting from the initial call.  */
  int list_mode;

  /* The resulting symbols.  */
  VEC (bound_minimal_symbol_d) *msyms;
};

/* A helper function to classify a minimal_symbol_type according to
   priority.  */

static int
classify_mtype (enum minimal_symbol_type t)
{
  switch (t)
    {
    case mst_file_text:
    case mst_file_data:
    case mst_file_bss:
      /* Intermediate priority.  */
      return 1;

    case mst_solib_trampoline:
      /* Lowest priority.  */
      return 2;

    default:
      /* Highest priority.  */
      return 0;
    }
}

/* Callback for qsort that sorts symbols by priority.  */

static int
compare_msyms (const void *a, const void *b)
{
  const bound_minimal_symbol_d *moa = (const bound_minimal_symbol_d *) a;
  const bound_minimal_symbol_d *mob = (const bound_minimal_symbol_d *) b;
  enum minimal_symbol_type ta = MSYMBOL_TYPE (moa->minsym);
  enum minimal_symbol_type tb = MSYMBOL_TYPE (mob->minsym);

  return classify_mtype (ta) - classify_mtype (tb);
}

/* Callback for iterate_over_minimal_symbols that adds the symbol to
   the result.  */

static void
add_minsym (struct minimal_symbol *minsym, void *d)
{
  struct collect_minsyms *info = (struct collect_minsyms *) d;
  bound_minimal_symbol_d mo;

  mo.minsym = minsym;
  mo.objfile = info->objfile;

  if (info->symtab != NULL)
    {
      CORE_ADDR pc;
      struct symtab_and_line sal;
      struct gdbarch *gdbarch = get_objfile_arch (info->objfile);

      sal = find_pc_sect_line (MSYMBOL_VALUE_ADDRESS (info->objfile, minsym),
			       NULL, 0);
      sal.section = MSYMBOL_OBJ_SECTION (info->objfile, minsym);
      pc
	= gdbarch_convert_from_func_ptr_addr (gdbarch, sal.pc, &current_target);
      if (pc != sal.pc)
	sal = find_pc_sect_line (pc, NULL, 0);

      if (info->symtab != sal.symtab)
	return;
    }

  /* Exclude data symbols when looking for breakpoint locations.   */
  if (!info->list_mode)
    switch (minsym->type)
      {
	case mst_slot_got_plt:
	case mst_data:
	case mst_bss:
	case mst_abs:
	case mst_file_data:
	case mst_file_bss:
	  {
	    /* Make sure this minsym is not a function descriptor
	       before we decide to discard it.  */
	    struct gdbarch *gdbarch = get_objfile_arch (info->objfile);
	    CORE_ADDR addr = gdbarch_convert_from_func_ptr_addr
			       (gdbarch, BMSYMBOL_VALUE_ADDRESS (mo),
				&current_target);

	    if (addr == BMSYMBOL_VALUE_ADDRESS (mo))
	      return;
	  }
      }

  VEC_safe_push (bound_minimal_symbol_d, info->msyms, &mo);
}

/* Search for minimal symbols called NAME.  If SEARCH_PSPACE
   is not NULL, the search is restricted to just that program
   space.

   If SYMTAB is NULL, search all objfiles, otherwise
   restrict results to the given SYMTAB.  */

static void
search_minsyms_for_name (struct collect_info *info, const char *name,
			 struct program_space *search_pspace,
			 struct symtab *symtab)
{
  struct collect_minsyms local;
  struct cleanup *cleanup;

  memset (&local, 0, sizeof (local));
  local.funfirstline = info->state->funfirstline;
  local.list_mode = info->state->list_mode;
  local.symtab = symtab;

  cleanup = make_cleanup (VEC_cleanup (bound_minimal_symbol_d), &local.msyms);

  if (symtab == NULL)
    {
      struct program_space *pspace;

      ALL_PSPACES (pspace)
      {
	struct objfile *objfile;

	if (search_pspace != NULL && search_pspace != pspace)
	  continue;
	if (pspace->executing_startup)
	  continue;

	set_current_program_space (pspace);

	ALL_OBJFILES (objfile)
	{
	  local.objfile = objfile;
	  iterate_over_minimal_symbols (objfile, name, add_minsym, &local);
	}
      }
    }
  else
    {
      if (search_pspace == NULL || SYMTAB_PSPACE (symtab) == search_pspace)
	{
	  set_current_program_space (SYMTAB_PSPACE (symtab));
	  local.objfile = SYMTAB_OBJFILE(symtab);
	  iterate_over_minimal_symbols (local.objfile, name, add_minsym,
					&local);
	}
    }

    if (!VEC_empty (bound_minimal_symbol_d, local.msyms))
      {
	int classification;
	int ix;
	bound_minimal_symbol_d *item;

	qsort (VEC_address (bound_minimal_symbol_d, local.msyms),
	       VEC_length (bound_minimal_symbol_d, local.msyms),
	       sizeof (bound_minimal_symbol_d),
	       compare_msyms);

	/* Now the minsyms are in classification order.  So, we walk
	   over them and process just the minsyms with the same
	   classification as the very first minsym in the list.  */
	item = VEC_index (bound_minimal_symbol_d, local.msyms, 0);
	classification = classify_mtype (MSYMBOL_TYPE (item->minsym));

	for (ix = 0;
	     VEC_iterate (bound_minimal_symbol_d, local.msyms, ix, item);
	     ++ix)
	  {
	    if (classify_mtype (MSYMBOL_TYPE (item->minsym)) != classification)
	      break;

	    VEC_safe_push (bound_minimal_symbol_d,
			   info->result.minimal_symbols, item);
	  }
      }

    do_cleanups (cleanup);
}

/* A helper function to add all symbols matching NAME to INFO.  If
   PSPACE is not NULL, the search is restricted to just that program
   space.  */

static void
add_matching_symbols_to_info (const char *name,
			      struct collect_info *info,
			      struct program_space *pspace)
{
  int ix;
  struct symtab *elt;

  for (ix = 0; VEC_iterate (symtab_ptr, info->file_symtabs, ix, elt); ++ix)
    {
      if (elt == NULL)
	{
	  iterate_over_all_matching_symtabs (info->state, name, VAR_DOMAIN,
					     pspace, true, [&] (symbol *sym)
	    { return info->add_symbol (sym); });
	  search_minsyms_for_name (info, name, pspace, NULL);
	}
      else if (pspace == NULL || pspace == SYMTAB_PSPACE (elt))
	{
	  int prev_len = VEC_length (symbolp, info->result.symbols);

	  /* Program spaces that are executing startup should have
	     been filtered out earlier.  */
	  gdb_assert (!SYMTAB_PSPACE (elt)->executing_startup);
	  set_current_program_space (SYMTAB_PSPACE (elt));
	  iterate_over_file_blocks (elt, name, VAR_DOMAIN, [&] (symbol *sym)
	    { return info->add_symbol (sym); });

	  /* If no new symbols were found in this iteration and this symtab
	     is in assembler, we might actually be looking for a label for
	     which we don't have debug info.  Check for a minimal symbol in
	     this case.  */
	  if (prev_len == VEC_length (symbolp, info->result.symbols)
	      && elt->language == language_asm)
	    search_minsyms_for_name (info, name, pspace, elt);
	}
    }
}



/* Now come some functions that are called from multiple places within
   decode_line_1.  */

static int
symbol_to_sal (struct symtab_and_line *result,
	       int funfirstline, struct symbol *sym)
{
  if (SYMBOL_CLASS (sym) == LOC_BLOCK)
    {
      *result = find_function_start_sal (sym, funfirstline);
      return 1;
    }
  else
    {
      if (SYMBOL_CLASS (sym) == LOC_LABEL && SYMBOL_VALUE_ADDRESS (sym) != 0)
	{
	  init_sal (result);
	  result->symtab = symbol_symtab (sym);
	  result->line = SYMBOL_LINE (sym);
	  result->pc = SYMBOL_VALUE_ADDRESS (sym);
	  result->pspace = SYMTAB_PSPACE (result->symtab);
	  result->explicit_pc = 1;
	  return 1;
	}
      else if (funfirstline)
	{
	  /* Nothing.  */
	}
      else if (SYMBOL_LINE (sym) != 0)
	{
	  /* We know its line number.  */
	  init_sal (result);
	  result->symtab = symbol_symtab (sym);
	  result->line = SYMBOL_LINE (sym);
	  result->pspace = SYMTAB_PSPACE (result->symtab);
	  return 1;
	}
    }

  return 0;
}

linespec_result::~linespec_result ()
{
  int i;
  struct linespec_sals *lsal;

  for (i = 0; VEC_iterate (linespec_sals, sals, i, lsal); ++i)
    {
      xfree (lsal->canonical);
      xfree (lsal->sals.sals);
    }
  VEC_free (linespec_sals, sals);
}

/* Return the quote characters permitted by the linespec parser.  */

const char *
get_gdb_linespec_parser_quote_characters (void)
{
  return linespec_quote_characters;
}
