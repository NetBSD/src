
/*  A Bison parser, made from ../../src/po-gram.y
 by  GNU Bison version 1.25
  */

#define po_gram_BISON 1  /* Identify Bison output.  */

#define	COMMENT	258
#define	DOMAIN	259
#define	JUNK	260
#define	MSGID	261
#define	MSGSTR	262
#define	NAME	263
#define	NUMBER	264
#define	STRING	265

#line 20 "../../src/po-gram.y"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include "po-lex.h"
#include "po-gram.h"
#include "error.h"
#include "system.h"
#include <libintl.h>
#include "po.h"

#define _(str) gettext (str)

#line 46 "../../src/po-gram.y"
typedef union
{
  char *string;
  long number;
  lex_pos_ty pos;
} po_gram_STYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	po_gram_FINAL		18
#define	po_gram_FLAG		-32768
#define	po_gram_NTBASE	11

#define po_gram_TRANSLATE(x) ((unsigned)(x) <= 265 ? po_gram_translate[x] : 18)

static const char po_gram_translate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10
};

#if po_gram_DEBUG != 0
static const short po_gram_prhs[] = {     0,
     0,     1,     4,     7,    10,    13,    16,    21,    24,    26,
    28,    30,    33
};

static const short po_gram_rhs[] = {    -1,
    11,    17,     0,    11,    12,     0,    11,    13,     0,    11,
     1,     0,     4,    10,     0,    14,    16,    15,    16,     0,
    14,    16,     0,     6,     0,     7,     0,    10,     0,    16,
    10,     0,     3,     0
};

#endif

#if po_gram_DEBUG != 0
static const short po_gram_rline[] = { 0,
    62,    63,    64,    65,    66,    70,    77,    81,    89,    96,
   103,   107,   122
};
#endif


#if po_gram_DEBUG != 0 || defined (po_gram_ERROR_VERBOSE)

static const char * const po_gram_tname[] = {   "$","error","$undefined.","COMMENT",
"DOMAIN","JUNK","MSGID","MSGSTR","NAME","NUMBER","STRING","msgfmt","domain",
"message","msgid","msgstr","string_list","comment", NULL
};
#endif

static const short po_gram_r1[] = {     0,
    11,    11,    11,    11,    11,    12,    13,    13,    14,    15,
    16,    16,    17
};

static const short po_gram_r2[] = {     0,
     0,     2,     2,     2,     2,     2,     4,     2,     1,     1,
     1,     2,     1
};

static const short po_gram_defact[] = {     1,
     0,     5,    13,     0,     9,     3,     4,     0,     2,     6,
    11,     8,    10,    12,     0,     7,     0,     0
};

static const short po_gram_defgoto[] = {     1,
     6,     7,     8,    15,    12,     9
};

static const short po_gram_pact[] = {-32768,
     0,-32768,-32768,    -3,-32768,-32768,-32768,    -2,-32768,-32768,
-32768,    -5,-32768,-32768,    -2,    -1,    10,-32768
};

static const short po_gram_pgoto[] = {-32768,
-32768,-32768,-32768,-32768,    -4,-32768
};


#define	po_gram_LAST		11


static const short po_gram_table[] = {    17,
     2,    13,     3,     4,    14,     5,    10,    11,    14,    18,
    16
};

static const short po_gram_check[] = {     0,
     1,     7,     3,     4,    10,     6,    10,    10,    10,     0,
    15
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define po_gram_errok		(po_gram_errstatus = 0)
#define po_gram_clearin	(po_gram_char = po_gram_EMPTY)
#define po_gram_EMPTY		-2
#define po_gram_EOF		0
#define po_gram_ACCEPT	return(0)
#define po_gram_ABORT 	return(1)
#define po_gram_ERROR		goto po_gram_errlab1
/* Like po_gram_ERROR except do call po_gram_error.
   This remains here temporarily to ease the
   transition to the new meaning of po_gram_ERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define po_gram_FAIL		goto po_gram_errlab
#define po_gram_RECOVERING()  (!!po_gram_errstatus)
#define po_gram_BACKUP(token, value) \
do								\
  if (po_gram_char == po_gram_EMPTY && po_gram_len == 1)				\
    { po_gram_char = (token), po_gram_lval = (value);			\
      po_gram_char1 = po_gram_TRANSLATE (po_gram_char);				\
      po_gram_POPSTACK;						\
      goto po_gram_backup;						\
    }								\
  else								\
    { po_gram_error ("syntax error: cannot back up"); po_gram_ERROR; }	\
while (0)

#define po_gram_TERROR	1
#define po_gram_ERRCODE	256

#ifndef po_gram_PURE
#define po_gram_LEX		po_gram_lex()
#endif

#ifdef po_gram_PURE
#ifdef po_gram_LSP_NEEDED
#ifdef po_gram_LEX_PARAM
#define po_gram_LEX		po_gram_lex(&po_gram_lval, &po_gram_lloc, po_gram_LEX_PARAM)
#else
#define po_gram_LEX		po_gram_lex(&po_gram_lval, &po_gram_lloc)
#endif
#else /* not po_gram_LSP_NEEDED */
#ifdef po_gram_LEX_PARAM
#define po_gram_LEX		po_gram_lex(&po_gram_lval, po_gram_LEX_PARAM)
#else
#define po_gram_LEX		po_gram_lex(&po_gram_lval)
#endif
#endif /* not po_gram_LSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef po_gram_PURE

int	po_gram_char;			/*  the lookahead symbol		*/
po_gram_STYPE	po_gram_lval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef po_gram_LSP_NEEDED
po_gram_LTYPE po_gram_lloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int po_gram_nerrs;			/*  number of parse errors so far       */
#endif  /* not po_gram_PURE */

#if po_gram_DEBUG != 0
int po_gram_debug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  po_gram_INITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	po_gram_INITDEPTH
#define po_gram_INITDEPTH 200
#endif

/*  po_gram_MAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if po_gram_MAXDEPTH == 0
#undef po_gram_MAXDEPTH
#endif

#ifndef po_gram_MAXDEPTH
#define po_gram_MAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int po_gram_parse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __po_gram__memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__po_gram__memcpy (to, from, count)
     char *to;
     char *from;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__po_gram__memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "/usr/lib/bison.simple"

/* The user can define po_gram_PARSE_PARAM as the name of an argument to be passed
   into po_gram_parse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef po_gram_PARSE_PARAM
#ifdef __cplusplus
#define po_gram_PARSE_PARAM_ARG void *po_gram_PARSE_PARAM
#define po_gram_PARSE_PARAM_DECL
#else /* not __cplusplus */
#define po_gram_PARSE_PARAM_ARG po_gram_PARSE_PARAM
#define po_gram_PARSE_PARAM_DECL void *po_gram_PARSE_PARAM;
#endif /* not __cplusplus */
#else /* not po_gram_PARSE_PARAM */
#define po_gram_PARSE_PARAM_ARG
#define po_gram_PARSE_PARAM_DECL
#endif /* not po_gram_PARSE_PARAM */

int
po_gram_parse(po_gram_PARSE_PARAM_ARG)
     po_gram_PARSE_PARAM_DECL
{
  register int po_gram_state;
  register int po_gram_n;
  register short *po_gram_ssp;
  register po_gram_STYPE *po_gram_vsp;
  int po_gram_errstatus;	/*  number of tokens to shift before error messages enabled */
  int po_gram_char1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	po_gram_ssa[po_gram_INITDEPTH];	/*  the state stack			*/
  po_gram_STYPE po_gram_vsa[po_gram_INITDEPTH];	/*  the semantic value stack		*/

  short *po_gram_ss = po_gram_ssa;		/*  refer to the stacks thru separate pointers */
  po_gram_STYPE *po_gram_vs = po_gram_vsa;	/*  to allow po_gram_overflow to reallocate them elsewhere */

#ifdef po_gram_LSP_NEEDED
  po_gram_LTYPE po_gram_lsa[po_gram_INITDEPTH];	/*  the location stack			*/
  po_gram_LTYPE *po_gram_ls = po_gram_lsa;
  po_gram_LTYPE *po_gram_lsp;

#define po_gram_POPSTACK   (po_gram_vsp--, po_gram_ssp--, po_gram_lsp--)
#else
#define po_gram_POPSTACK   (po_gram_vsp--, po_gram_ssp--)
#endif

  int po_gram_stacksize = po_gram_INITDEPTH;

#ifdef po_gram_PURE
  int po_gram_char;
  po_gram_STYPE po_gram_lval;
  int po_gram_nerrs;
#ifdef po_gram_LSP_NEEDED
  po_gram_LTYPE po_gram_lloc;
#endif
#endif

  po_gram_STYPE po_gram_val;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int po_gram_len;

#if po_gram_DEBUG != 0
  if (po_gram_debug)
    fprintf(stderr, "Starting parse\n");
#endif

  po_gram_state = 0;
  po_gram_errstatus = 0;
  po_gram_nerrs = 0;
  po_gram_char = po_gram_EMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  po_gram_ssp = po_gram_ss - 1;
  po_gram_vsp = po_gram_vs;
#ifdef po_gram_LSP_NEEDED
  po_gram_lsp = po_gram_ls;
#endif

/* Push a new state, which is found in  po_gram_state  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
po_gram_newstate:

  *++po_gram_ssp = po_gram_state;

  if (po_gram_ssp >= po_gram_ss + po_gram_stacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      po_gram_STYPE *po_gram_vs1 = po_gram_vs;
      short *po_gram_ss1 = po_gram_ss;
#ifdef po_gram_LSP_NEEDED
      po_gram_LTYPE *po_gram_ls1 = po_gram_ls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = po_gram_ssp - po_gram_ss + 1;

#ifdef po_gram_overflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef po_gram_LSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if po_gram_overflow is a macro.  */
      po_gram_overflow("parser stack overflow",
		 &po_gram_ss1, size * sizeof (*po_gram_ssp),
		 &po_gram_vs1, size * sizeof (*po_gram_vsp),
		 &po_gram_ls1, size * sizeof (*po_gram_lsp),
		 &po_gram_stacksize);
#else
      po_gram_overflow("parser stack overflow",
		 &po_gram_ss1, size * sizeof (*po_gram_ssp),
		 &po_gram_vs1, size * sizeof (*po_gram_vsp),
		 &po_gram_stacksize);
#endif

      po_gram_ss = po_gram_ss1; po_gram_vs = po_gram_vs1;
#ifdef po_gram_LSP_NEEDED
      po_gram_ls = po_gram_ls1;
#endif
#else /* no po_gram_overflow */
      /* Extend the stack our own way.  */
      if (po_gram_stacksize >= po_gram_MAXDEPTH)
	{
	  po_gram_error("parser stack overflow");
	  return 2;
	}
      po_gram_stacksize *= 2;
      if (po_gram_stacksize > po_gram_MAXDEPTH)
	po_gram_stacksize = po_gram_MAXDEPTH;
      po_gram_ss = (short *) alloca (po_gram_stacksize * sizeof (*po_gram_ssp));
      __po_gram__memcpy ((char *)po_gram_ss, (char *)po_gram_ss1, size * sizeof (*po_gram_ssp));
      po_gram_vs = (po_gram_STYPE *) alloca (po_gram_stacksize * sizeof (*po_gram_vsp));
      __po_gram__memcpy ((char *)po_gram_vs, (char *)po_gram_vs1, size * sizeof (*po_gram_vsp));
#ifdef po_gram_LSP_NEEDED
      po_gram_ls = (po_gram_LTYPE *) alloca (po_gram_stacksize * sizeof (*po_gram_lsp));
      __po_gram__memcpy ((char *)po_gram_ls, (char *)po_gram_ls1, size * sizeof (*po_gram_lsp));
#endif
#endif /* no po_gram_overflow */

      po_gram_ssp = po_gram_ss + size - 1;
      po_gram_vsp = po_gram_vs + size - 1;
#ifdef po_gram_LSP_NEEDED
      po_gram_lsp = po_gram_ls + size - 1;
#endif

#if po_gram_DEBUG != 0
      if (po_gram_debug)
	fprintf(stderr, "Stack size increased to %d\n", po_gram_stacksize);
#endif

      if (po_gram_ssp >= po_gram_ss + po_gram_stacksize - 1)
	po_gram_ABORT;
    }

#if po_gram_DEBUG != 0
  if (po_gram_debug)
    fprintf(stderr, "Entering state %d\n", po_gram_state);
#endif

  goto po_gram_backup;
 po_gram_backup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* po_gram_resume: */

  /* First try to decide what to do without reference to lookahead token.  */

  po_gram_n = po_gram_pact[po_gram_state];
  if (po_gram_n == po_gram_FLAG)
    goto po_gram_default;

  /* Not known => get a lookahead token if don't already have one.  */

  /* po_gram_char is either po_gram_EMPTY or po_gram_EOF
     or a valid token in external form.  */

  if (po_gram_char == po_gram_EMPTY)
    {
#if po_gram_DEBUG != 0
      if (po_gram_debug)
	fprintf(stderr, "Reading a token: ");
#endif
      po_gram_char = po_gram_LEX;
    }

  /* Convert token to internal form (in po_gram_char1) for indexing tables with */

  if (po_gram_char <= 0)		/* This means end of input. */
    {
      po_gram_char1 = 0;
      po_gram_char = po_gram_EOF;		/* Don't call po_gram_LEX any more */

#if po_gram_DEBUG != 0
      if (po_gram_debug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      po_gram_char1 = po_gram_TRANSLATE(po_gram_char);

#if po_gram_DEBUG != 0
      if (po_gram_debug)
	{
	  fprintf (stderr, "Next token is %d (%s", po_gram_char, po_gram_tname[po_gram_char1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef po_gram_PRINT
	  po_gram_PRINT (stderr, po_gram_char, po_gram_lval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  po_gram_n += po_gram_char1;
  if (po_gram_n < 0 || po_gram_n > po_gram_LAST || po_gram_check[po_gram_n] != po_gram_char1)
    goto po_gram_default;

  po_gram_n = po_gram_table[po_gram_n];

  /* po_gram_n is what to do for this token type in this state.
     Negative => reduce, -po_gram_n is rule number.
     Positive => shift, po_gram_n is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (po_gram_n < 0)
    {
      if (po_gram_n == po_gram_FLAG)
	goto po_gram_errlab;
      po_gram_n = -po_gram_n;
      goto po_gram_reduce;
    }
  else if (po_gram_n == 0)
    goto po_gram_errlab;

  if (po_gram_n == po_gram_FINAL)
    po_gram_ACCEPT;

  /* Shift the lookahead token.  */

#if po_gram_DEBUG != 0
  if (po_gram_debug)
    fprintf(stderr, "Shifting token %d (%s), ", po_gram_char, po_gram_tname[po_gram_char1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (po_gram_char != po_gram_EOF)
    po_gram_char = po_gram_EMPTY;

  *++po_gram_vsp = po_gram_lval;
#ifdef po_gram_LSP_NEEDED
  *++po_gram_lsp = po_gram_lloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (po_gram_errstatus) po_gram_errstatus--;

  po_gram_state = po_gram_n;
  goto po_gram_newstate;

/* Do the default action for the current state.  */
po_gram_default:

  po_gram_n = po_gram_defact[po_gram_state];
  if (po_gram_n == 0)
    goto po_gram_errlab;

/* Do a reduction.  po_gram_n is the number of a rule to reduce with.  */
po_gram_reduce:
  po_gram_len = po_gram_r2[po_gram_n];
  if (po_gram_len > 0)
    po_gram_val = po_gram_vsp[1-po_gram_len]; /* implement default value of the action */

#if po_gram_DEBUG != 0
  if (po_gram_debug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       po_gram_n, po_gram_rline[po_gram_n]);

      /* Print the symbols being reduced, and their result.  */
      for (i = po_gram_prhs[po_gram_n]; po_gram_rhs[i] > 0; i++)
	fprintf (stderr, "%s ", po_gram_tname[po_gram_rhs[i]]);
      fprintf (stderr, " -> %s\n", po_gram_tname[po_gram_r1[po_gram_n]]);
    }
#endif


  switch (po_gram_n) {

case 6:
#line 71 "../../src/po-gram.y"
{
		   po_callback_domain (po_gram_vsp[0].string);
		;
    break;}
case 7:
#line 78 "../../src/po-gram.y"
{
		  po_callback_message (po_gram_vsp[-2].string, &po_gram_vsp[-3].pos, po_gram_vsp[0].string, &po_gram_vsp[-1].pos);
		;
    break;}
case 8:
#line 82 "../../src/po-gram.y"
{
		  gram_error_at_line (&po_gram_vsp[-1].pos, _("missing `msgstr' section"));
		  free (po_gram_vsp[0].string);
		;
    break;}
case 9:
#line 90 "../../src/po-gram.y"
{
		  po_gram_val.pos = gram_pos;
		;
    break;}
case 10:
#line 97 "../../src/po-gram.y"
{
		  po_gram_val.pos = gram_pos;
		;
    break;}
case 11:
#line 104 "../../src/po-gram.y"
{
		  po_gram_val.string = po_gram_vsp[0].string;
		;
    break;}
case 12:
#line 108 "../../src/po-gram.y"
{
		  size_t len1;
		  size_t len2;

		  len1 = strlen (po_gram_vsp[-1].string);
		  len2 = strlen (po_gram_vsp[0].string);
		  po_gram_val.string = (char *) xmalloc (len1 + len2 + 1);
		  stpcpy (stpcpy (po_gram_val.string, po_gram_vsp[-1].string), po_gram_vsp[0].string);
		  free (po_gram_vsp[-1].string);
		  free (po_gram_vsp[0].string);
		;
    break;}
case 13:
#line 123 "../../src/po-gram.y"
{
		  po_callback_comment (po_gram_vsp[0].string);
		;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "/usr/lib/bison.simple"

  po_gram_vsp -= po_gram_len;
  po_gram_ssp -= po_gram_len;
#ifdef po_gram_LSP_NEEDED
  po_gram_lsp -= po_gram_len;
#endif

#if po_gram_DEBUG != 0
  if (po_gram_debug)
    {
      short *ssp1 = po_gram_ss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != po_gram_ssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++po_gram_vsp = po_gram_val;

#ifdef po_gram_LSP_NEEDED
  po_gram_lsp++;
  if (po_gram_len == 0)
    {
      po_gram_lsp->first_line = po_gram_lloc.first_line;
      po_gram_lsp->first_column = po_gram_lloc.first_column;
      po_gram_lsp->last_line = (po_gram_lsp-1)->last_line;
      po_gram_lsp->last_column = (po_gram_lsp-1)->last_column;
      po_gram_lsp->text = 0;
    }
  else
    {
      po_gram_lsp->last_line = (po_gram_lsp+po_gram_len-1)->last_line;
      po_gram_lsp->last_column = (po_gram_lsp+po_gram_len-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  po_gram_n = po_gram_r1[po_gram_n];

  po_gram_state = po_gram_pgoto[po_gram_n - po_gram_NTBASE] + *po_gram_ssp;
  if (po_gram_state >= 0 && po_gram_state <= po_gram_LAST && po_gram_check[po_gram_state] == *po_gram_ssp)
    po_gram_state = po_gram_table[po_gram_state];
  else
    po_gram_state = po_gram_defgoto[po_gram_n - po_gram_NTBASE];

  goto po_gram_newstate;

po_gram_errlab:   /* here on detecting error */

  if (! po_gram_errstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++po_gram_nerrs;

#ifdef po_gram_ERROR_VERBOSE
      po_gram_n = po_gram_pact[po_gram_state];

      if (po_gram_n > po_gram_FLAG && po_gram_n < po_gram_LAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -po_gram_n if nec to avoid negative indexes in po_gram_check.  */
	  for (x = (po_gram_n < 0 ? -po_gram_n : 0);
	       x < (sizeof(po_gram_tname) / sizeof(char *)); x++)
	    if (po_gram_check[x + po_gram_n] == x)
	      size += strlen(po_gram_tname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (po_gram_n < 0 ? -po_gram_n : 0);
		       x < (sizeof(po_gram_tname) / sizeof(char *)); x++)
		    if (po_gram_check[x + po_gram_n] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, po_gram_tname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      po_gram_error(msg);
	      free(msg);
	    }
	  else
	    po_gram_error ("parse error; also virtual memory exceeded");
	}
      else
#endif /* po_gram_ERROR_VERBOSE */
	po_gram_error("parse error");
    }

  goto po_gram_errlab1;
po_gram_errlab1:   /* here on error raised explicitly by an action */

  if (po_gram_errstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (po_gram_char == po_gram_EOF)
	po_gram_ABORT;

#if po_gram_DEBUG != 0
      if (po_gram_debug)
	fprintf(stderr, "Discarding token %d (%s).\n", po_gram_char, po_gram_tname[po_gram_char1]);
#endif

      po_gram_char = po_gram_EMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  po_gram_errstatus = 3;		/* Each real token shifted decrements this */

  goto po_gram_errhandle;

po_gram_errdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  po_gram_n = po_gram_defact[po_gram_state];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (po_gram_n) goto po_gram_default;
#endif

po_gram_errpop:   /* pop the current state because it cannot handle the error token */

  if (po_gram_ssp == po_gram_ss) po_gram_ABORT;
  po_gram_vsp--;
  po_gram_state = *--po_gram_ssp;
#ifdef po_gram_LSP_NEEDED
  po_gram_lsp--;
#endif

#if po_gram_DEBUG != 0
  if (po_gram_debug)
    {
      short *ssp1 = po_gram_ss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != po_gram_ssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

po_gram_errhandle:

  po_gram_n = po_gram_pact[po_gram_state];
  if (po_gram_n == po_gram_FLAG)
    goto po_gram_errdefault;

  po_gram_n += po_gram_TERROR;
  if (po_gram_n < 0 || po_gram_n > po_gram_LAST || po_gram_check[po_gram_n] != po_gram_TERROR)
    goto po_gram_errdefault;

  po_gram_n = po_gram_table[po_gram_n];
  if (po_gram_n < 0)
    {
      if (po_gram_n == po_gram_FLAG)
	goto po_gram_errpop;
      po_gram_n = -po_gram_n;
      goto po_gram_reduce;
    }
  else if (po_gram_n == 0)
    goto po_gram_errpop;

  if (po_gram_n == po_gram_FINAL)
    po_gram_ACCEPT;

#if po_gram_DEBUG != 0
  if (po_gram_debug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++po_gram_vsp = po_gram_lval;
#ifdef po_gram_LSP_NEEDED
  *++po_gram_lsp = po_gram_lloc;
#endif

  po_gram_state = po_gram_n;
  goto po_gram_newstate;
}
#line 127 "../../src/po-gram.y"
