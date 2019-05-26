/* FLEX lexer for Ada expressions, for GDB.
   Copyright (C) 1994-2017 Free Software Foundation, Inc.

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

/*----------------------------------------------------------------------*/

/* The converted version of this file is to be included in ada-exp.y, */
/* the Ada parser for gdb.  The function yylex obtains characters from */
/* the global pointer lexptr.  It returns a syntactic category for */
/* each successive token and places a semantic value into yylval */
/* (ada-lval), defined by the parser.   */

DIG	[0-9]
NUM10	({DIG}({DIG}|_)*)
HEXDIG	[0-9a-f]
NUM16	({HEXDIG}({HEXDIG}|_)*)
OCTDIG	[0-7]
LETTER	[a-z_]
ID	({LETTER}({LETTER}|{DIG})*|"<"{LETTER}({LETTER}|{DIG})*">")
WHITE	[ \t\n]
TICK	("'"{WHITE}*)
GRAPHIC [a-z0-9 #&'()*+,-./:;<>=_|!$%?@\[\]\\^`{}~]
OPER    ([-+*/=<>&]|"<="|">="|"**"|"/="|"and"|"or"|"xor"|"not"|"mod"|"rem"|"abs")

EXP	(e[+-]{NUM10})
POSEXP  (e"+"?{NUM10})

%{

#define NUMERAL_WIDTH 256
#define LONGEST_SIGN ((ULONGEST) 1 << (sizeof(LONGEST) * HOST_CHAR_BIT - 1))

/* Temporary staging for numeric literals.  */
static char numbuf[NUMERAL_WIDTH];
 static void canonicalizeNumeral (char *s1, const char *);
static struct stoken processString (const char*, int);
static int processInt (struct parser_state *, const char *, const char *,
		       const char *);
static int processReal (struct parser_state *, const char *);
static struct stoken processId (const char *, int);
static int processAttribute (const char *);
static int find_dot_all (const char *);
static void rewind_to_char (int);

#undef YY_DECL
#define YY_DECL static int yylex ( void )

/* Flex generates a static function "input" which is not used.
   Defining YY_NO_INPUT comments it out.  */
#define YY_NO_INPUT

#undef YY_INPUT
#define YY_INPUT(BUF, RESULT, MAX_SIZE) \
    if ( *lexptr == '\000' ) \
      (RESULT) = YY_NULL; \
    else \
      { \
        *(BUF) = *lexptr; \
        (RESULT) = 1; \
	lexptr += 1; \
      }

static int find_dot_all (const char *);

%}

%option case-insensitive interactive nodefault

%s BEFORE_QUAL_QUOTE

%%

{WHITE}		 { }

"--".*		 { yyterminate(); }

{NUM10}{POSEXP}  {
		   canonicalizeNumeral (numbuf, yytext);
		   return processInt (pstate, NULL, numbuf,
				      strrchr (numbuf, 'e') + 1);
		 }

{NUM10}          {
		   canonicalizeNumeral (numbuf, yytext);
		   return processInt (pstate, NULL, numbuf, NULL);
		 }

{NUM10}"#"{HEXDIG}({HEXDIG}|_)*"#"{POSEXP} {
		   canonicalizeNumeral (numbuf, yytext);
		   return processInt (pstate, numbuf,
				      strchr (numbuf, '#') + 1,
				      strrchr(numbuf, '#') + 1);
		 }

{NUM10}"#"{HEXDIG}({HEXDIG}|_)*"#" {
		   canonicalizeNumeral (numbuf, yytext);
		   return processInt (pstate, numbuf, strchr (numbuf, '#') + 1,
				      NULL);
		 }

"0x"{HEXDIG}+	{
		  canonicalizeNumeral (numbuf, yytext+2);
		  return processInt (pstate, "16#", numbuf, NULL);
		}


{NUM10}"."{NUM10}{EXP} {
		   canonicalizeNumeral (numbuf, yytext);
		   return processReal (pstate, numbuf);
		}

{NUM10}"."{NUM10} {
		   canonicalizeNumeral (numbuf, yytext);
		   return processReal (pstate, numbuf);
		}

{NUM10}"#"{NUM16}"."{NUM16}"#"{EXP} {
                   error (_("Based real literals not implemented yet."));
		}

{NUM10}"#"{NUM16}"."{NUM16}"#" {
                   error (_("Based real literals not implemented yet."));
		}

<INITIAL>"'"({GRAPHIC}|\")"'" {
		   yylval.typed_val.type = type_char (pstate);
		   yylval.typed_val.val = yytext[1];
		   return CHARLIT;
		}

<INITIAL>"'[\""{HEXDIG}{2}"\"]'"   {
                   int v;
                   yylval.typed_val.type = type_char (pstate);
		   sscanf (yytext+3, "%2x", &v);
		   yylval.typed_val.val = v;
		   return CHARLIT;
		}

\"({GRAPHIC}|"[\""({HEXDIG}{2}|\")"\"]")*\"   {
	           yylval.sval = processString (yytext+1, yyleng-2);
		   return STRING;
		}

\"              {
                   error (_("ill-formed or non-terminated string literal"));
		}


if		{
                  rewind_to_char ('i');
		  return 0;
		}

task            {
                  rewind_to_char ('t');
		  return 0;
		}

thread{WHITE}+{DIG} {
                  /* This keyword signals the end of the expression and
                     will be processed separately.  */
                  rewind_to_char ('t');
		  return 0;
		}

	/* ADA KEYWORDS */

abs		{ return ABS; }
and		{ return _AND_; }
else		{ return ELSE; }
in		{ return IN; }
mod		{ return MOD; }
new		{ return NEW; }
not		{ return NOT; }
null		{ return NULL_PTR; }
or		{ return OR; }
others          { return OTHERS; }
rem		{ return REM; }
then		{ return THEN; }
xor		{ return XOR; }

	/* BOOLEAN "KEYWORDS" */

 /* True and False are not keywords in Ada, but rather enumeration constants.
    However, the boolean type is no longer represented as an enum, so True
    and False are no longer defined in symbol tables.  We compromise by
    making them keywords (when bare). */

true		{ return TRUEKEYWORD; }
false		{ return FALSEKEYWORD; }

        /* ATTRIBUTES */

{TICK}[a-zA-Z][a-zA-Z]+ { BEGIN INITIAL; return processAttribute (yytext+1); }

	/* PUNCTUATION */

"=>"		{ return ARROW; }
".."		{ return DOTDOT; }
"**"		{ return STARSTAR; }
":="		{ return ASSIGN; }
"/="		{ return NOTEQUAL; }
"<="		{ return LEQ; }
">="		{ return GEQ; }

<BEFORE_QUAL_QUOTE>"'" { BEGIN INITIAL; return '\''; }

[-&*+./:<>=|;\[\]] { return yytext[0]; }

","		{ if (paren_depth == 0 && comma_terminates)
		    {
		      rewind_to_char (',');
		      return 0;
		    }
		  else
		    return ',';
		}

"("		{ paren_depth += 1; return '('; }
")"		{ if (paren_depth == 0)
		    {
		      rewind_to_char (')');
		      return 0;
		    }
		  else
 		    {
		      paren_depth -= 1;
		      return ')';
		    }
		}

"."{WHITE}*all  { return DOT_ALL; }

"."{WHITE}*{ID} {
	 	  yylval.sval = processId (yytext+1, yyleng-1);
	          return DOT_ID;
		}

{ID}({WHITE}*"."{WHITE}*({ID}|\"{OPER}\"))*(" "*"'")?  {
                  int all_posn = find_dot_all (yytext);

                  if (all_posn == -1 && yytext[yyleng-1] == '\'')
		    {
		      BEGIN BEFORE_QUAL_QUOTE;
		      yyless (yyleng-1);
		    }
                  else if (all_posn >= 0)
		    yyless (all_posn);
                  yylval.sval = processId (yytext, yyleng);
                  return NAME;
               }


	/* GDB EXPRESSION CONSTRUCTS  */

"'"[^']+"'"{WHITE}*:: {
                  yyless (yyleng - 2);
		  yylval.sval = processId (yytext, yyleng);
		  return NAME;
		}

"::"            { return COLONCOLON; }

[{}@]		{ return yytext[0]; }

	/* REGISTERS AND GDB CONVENIENCE VARIABLES */

"$"({LETTER}|{DIG}|"$")*  {
		  yylval.sval.ptr = yytext;
		  yylval.sval.length = yyleng;
		  return SPECIAL_VARIABLE;
		}

	/* CATCH-ALL ERROR CASE */

.		{ error (_("Invalid character '%s' in expression."), yytext); }
%%

#include <ctype.h>
/* Initialize the lexer for processing new expression. */

static void
lexer_init (FILE *inp)
{
  BEGIN INITIAL;
  yyrestart (inp);
}


/* Copy S2 to S1, removing all underscores, and downcasing all letters.  */

static void
canonicalizeNumeral (char *s1, const char *s2)
{
  for (; *s2 != '\000'; s2 += 1)
    {
      if (*s2 != '_')
	{
	  *s1 = tolower(*s2);
	  s1 += 1;
	}
    }
  s1[0] = '\000';
}

/* Interprets the prefix of NUM that consists of digits of the given BASE
   as an integer of that BASE, with the string EXP as an exponent.
   Puts value in yylval, and returns INT, if the string is valid.  Causes
   an error if the number is improperly formated.   BASE, if NULL, defaults
   to "10", and EXP to "1".  The EXP does not contain a leading 'e' or 'E'.
 */

static int
processInt (struct parser_state *par_state, const char *base0,
	    const char *num0, const char *exp0)
{
  ULONGEST result;
  long exp;
  int base;
  const char *trailer;

  if (base0 == NULL)
    base = 10;
  else
    {
      base = strtol (base0, (char **) NULL, 10);
      if (base < 2 || base > 16)
	error (_("Invalid base: %d."), base);
    }

  if (exp0 == NULL)
    exp = 0;
  else
    exp = strtol(exp0, (char **) NULL, 10);

  errno = 0;
  result = strtoulst (num0, &trailer, base);
  if (errno == ERANGE)
    error (_("Integer literal out of range"));
  if (isxdigit(*trailer))
    error (_("Invalid digit `%c' in based literal"), *trailer);

  while (exp > 0)
    {
      if (result > (ULONG_MAX / base))
	error (_("Integer literal out of range"));
      result *= base;
      exp -= 1;
    }

  if ((result >> (gdbarch_int_bit (parse_gdbarch (par_state))-1)) == 0)
    yylval.typed_val.type = type_int (par_state);
  else if ((result >> (gdbarch_long_bit (parse_gdbarch (par_state))-1)) == 0)
    yylval.typed_val.type = type_long (par_state);
  else if (((result >> (gdbarch_long_bit (parse_gdbarch (par_state))-1)) >> 1) == 0)
    {
      /* We have a number representable as an unsigned integer quantity.
         For consistency with the C treatment, we will treat it as an
	 anonymous modular (unsigned) quantity.  Alas, the types are such
	 that we need to store .val as a signed quantity.  Sorry
         for the mess, but C doesn't officially guarantee that a simple
         assignment does the trick (no, it doesn't; read the reference manual).
       */
      yylval.typed_val.type
	= builtin_type (parse_gdbarch (par_state))->builtin_unsigned_long;
      if (result & LONGEST_SIGN)
	yylval.typed_val.val =
	  (LONGEST) (result & ~LONGEST_SIGN)
	  - (LONGEST_SIGN>>1) - (LONGEST_SIGN>>1);
      else
	yylval.typed_val.val = (LONGEST) result;
      return INT;
    }
  else
    yylval.typed_val.type = type_long_long (par_state);

  yylval.typed_val.val = (LONGEST) result;
  return INT;
}

static int
processReal (struct parser_state *par_state, const char *num0)
{
  sscanf (num0, "%" DOUBLEST_SCAN_FORMAT, &yylval.typed_val_float.dval);

  yylval.typed_val_float.type = type_float (par_state);
  if (sizeof(DOUBLEST) >= gdbarch_double_bit (parse_gdbarch (par_state))
			    / TARGET_CHAR_BIT)
    yylval.typed_val_float.type = type_double (par_state);
  if (sizeof(DOUBLEST) >= gdbarch_long_double_bit (parse_gdbarch (par_state))
			    / TARGET_CHAR_BIT)
    yylval.typed_val_float.type = type_long_double (par_state);

  return FLOAT;
}


/* Store a canonicalized version of NAME0[0..LEN-1] in yylval.ssym.  The
   resulting string is valid until the next call to ada_parse.  If
   NAME0 contains the substring "___", it is assumed to be already
   encoded and the resulting name is equal to it.  Otherwise, it differs
   from NAME0 in that:
    + Characters between '...' or <...> are transfered verbatim to 
      yylval.ssym.
    + <, >, and trailing "'" characters in quoted sequences are removed
      (a leading quote is preserved to indicate that the name is not to be
      GNAT-encoded).
    + Unquoted whitespace is removed.
    + Unquoted alphabetic characters are mapped to lower case.
   Result is returned as a struct stoken, but for convenience, the string
   is also null-terminated.  Result string valid until the next call of
   ada_parse.
 */
static struct stoken
processId (const char *name0, int len)
{
  char *name = (char *) obstack_alloc (&temp_parse_space, len + 11);
  int i0, i;
  struct stoken result;

  result.ptr = name;
  while (len > 0 && isspace (name0[len-1]))
    len -= 1;

  if (strstr (name0, "___") != NULL)
    {
      strncpy (name, name0, len);
      name[len] = '\000';
      result.length = len;
      return result;
    }

  i = i0 = 0;
  while (i0 < len)
    {
      if (isalnum (name0[i0]))
	{
	  name[i] = tolower (name0[i0]);
	  i += 1; i0 += 1;
	}
      else switch (name0[i0])
	{
	default:
	  name[i] = name0[i0];
	  i += 1; i0 += 1;
	  break;
	case ' ': case '\t':
	  i0 += 1;
	  break;
	case '\'':
	  do
	    {
	      name[i] = name0[i0];
	      i += 1; i0 += 1;
	    }
	  while (i0 < len && name0[i0] != '\'');
	  i0 += 1;
	  break;
	case '<':
	  i0 += 1;
	  while (i0 < len && name0[i0] != '>')
	    {
	      name[i] = name0[i0];
	      i += 1; i0 += 1;
	    }
	  i0 += 1;
	  break;
	}
    }
  name[i] = '\000';

  result.length = i;
  return result;
}

/* Return TEXT[0..LEN-1], a string literal without surrounding quotes,
   with special hex character notations replaced with characters. 
   Result valid until the next call to ada_parse.  */

static struct stoken
processString (const char *text, int len)
{
  const char *p;
  char *q;
  const char *lim = text + len;
  struct stoken result;

  q = (char *) obstack_alloc (&temp_parse_space, len);
  result.ptr = q;
  p = text;
  while (p < lim)
    {
      if (p[0] == '[' && p[1] == '"' && p+2 < lim)
         {
           if (p[2] == '"')  /* "...["""]... */
             {
               *q = '"';
	       p += 4;
	     }
           else
	     {
               int chr;
	       sscanf (p+2, "%2x", &chr);
	       *q = (char) chr;
	       p += 5;
	     }
         }
       else
         *q = *p;
       q += 1;
       p += 1;
     }
  result.length = q - result.ptr;
  return result;
}

/* Returns the position within STR of the '.' in a
   '.{WHITE}*all' component of a dotted name, or -1 if there is none.
   Note: we actually don't need this routine, since 'all' can never be an
   Ada identifier.  Thus, looking up foo.all or foo.all.x as a name
   must fail, and will eventually be interpreted as (foo).all or
   (foo).all.x.  However, this does avoid an extraneous lookup. */

static int
find_dot_all (const char *str)
{
  int i;

  for (i = 0; str[i] != '\000'; i++)
    if (str[i] == '.')
      {
	int i0 = i;

	do
	  i += 1;
	while (isspace (str[i]));

	if (strncasecmp (str + i, "all", 3) == 0
	    && !isalnum (str[i + 3]) && str[i + 3] != '_')
	  return i0;
      }
  return -1;
}

/* Returns non-zero iff string SUBSEQ matches a subsequence of STR, ignoring
   case.  */

static int
subseqMatch (const char *subseq, const char *str)
{
  if (subseq[0] == '\0')
    return 1;
  else if (str[0] == '\0')
    return 0;
  else if (tolower (subseq[0]) == tolower (str[0]))
    return subseqMatch (subseq+1, str+1) || subseqMatch (subseq, str+1);
  else
    return subseqMatch (subseq, str+1);
}


static struct { const char *name; int code; }
attributes[] = {
  { "address", TICK_ADDRESS },
  { "unchecked_access", TICK_ACCESS },
  { "unrestricted_access", TICK_ACCESS },
  { "access", TICK_ACCESS },
  { "first", TICK_FIRST },
  { "last", TICK_LAST },
  { "length", TICK_LENGTH },
  { "max", TICK_MAX },
  { "min", TICK_MIN },
  { "modulus", TICK_MODULUS },
  { "pos", TICK_POS },
  { "range", TICK_RANGE },
  { "size", TICK_SIZE },
  { "tag", TICK_TAG },
  { "val", TICK_VAL },
  { NULL, -1 }
};

/* Return the syntactic code corresponding to the attribute name or
   abbreviation STR.  */

static int
processAttribute (const char *str)
{
  int i, k;

  for (i = 0; attributes[i].code != -1; i += 1)
    if (strcasecmp (str, attributes[i].name) == 0)
      return attributes[i].code;

  for (i = 0, k = -1; attributes[i].code != -1; i += 1)
    if (subseqMatch (str, attributes[i].name))
      {
	if (k == -1)
	  k = i;
	else
	  error (_("ambiguous attribute name: `%s'"), str);
      }
  if (k == -1)
    error (_("unrecognized attribute: `%s'"), str);

  return attributes[k].code;
}

/* Back up lexptr by yyleng and then to the rightmost occurrence of
   character CH, case-folded (there must be one).  WARNING: since
   lexptr points to the next input character that Flex has not yet
   transferred to its internal buffer, the use of this function
   depends on the assumption that Flex calls YY_INPUT only when it is
   logically necessary to do so (thus, there is no reading ahead
   farther than needed to identify the next token.)  */

static void
rewind_to_char (int ch)
{
  lexptr -= yyleng;
  while (toupper (*lexptr) != toupper (ch))
    lexptr -= 1;
  yyrestart (NULL);
}

int
yywrap(void)
{
  return 1;
}

/* Dummy definition to suppress warnings about unused static definitions. */
typedef void (*dummy_function) ();
dummy_function ada_flex_use[] = 
{ 
  (dummy_function) yyunput
};
