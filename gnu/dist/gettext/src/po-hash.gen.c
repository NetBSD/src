
/*  A Bison parser, made from ../../src/po-hash.y
 by  GNU Bison version 1.25
  */

#define po_hash_BISON 1  /* Identify Bison output.  */

#define	STRING	258
#define	NUMBER	259
#define	COLON	260
#define	COMMA	261
#define	FILE_KEYWORD	262
#define	LINE_KEYWORD	263
#define	NUMBER_KEYWORD	264

#line 20 "../../src/po-hash.y"


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <system.h>
#include "po-hash.h"
#include "po.h"


#line 42 "../../src/po-hash.y"
typedef union
{
  char *string;
  int number;
} po_hash_STYPE;
#line 51 "../../src/po-hash.y"


static const char *cur;


void po_hash_error PARAMS ((char *));
int po_hash_lex PARAMS ((void));


int
po_hash (s)
     const char *s;
{
  extern int po_hash_parse PARAMS ((void));

  cur = s;
  return po_hash_parse ();
}


void
po_hash_error (s)
     char *s;
{
  /* Do nothing, the grammar is used as a recogniser.  */
}
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	po_hash_FINAL		18
#define	po_hash_FLAG		-32768
#define	po_hash_NTBASE	10

#define po_hash_TRANSLATE(x) ((unsigned)(x) <= 264 ? po_hash_translate[x] : 12)

static const char po_hash_translate[] = {     0,
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
     6,     7,     8,     9
};

#if po_hash_DEBUG != 0
static const short po_hash_prhs[] = {     0,
     0,     1,     4,     8,    16,    25
};

static const short po_hash_rhs[] = {    -1,
    10,    11,     0,     3,     5,     4,     0,     7,     5,     3,
     6,     8,     5,     4,     0,     7,     5,     3,     6,     8,
     9,     5,     4,     0,     7,     5,     4,     0
};

#endif

#if po_hash_DEBUG != 0
static const short po_hash_rline[] = { 0,
    82,    83,    87,    93,    99,   105
};
#endif


#if po_hash_DEBUG != 0 || defined (po_hash_ERROR_VERBOSE)

static const char * const po_hash_tname[] = {   "$","error","$undefined.","STRING",
"NUMBER","COLON","COMMA","FILE_KEYWORD","LINE_KEYWORD","NUMBER_KEYWORD","filepos_line",
"filepos", NULL
};
#endif

static const short po_hash_r1[] = {     0,
    10,    10,    11,    11,    11,    11
};

static const short po_hash_r2[] = {     0,
     0,     2,     3,     7,     8,     3
};

static const short po_hash_defact[] = {     1,
     0,     0,     0,     2,     0,     0,     3,     0,     6,     0,
     0,     0,     0,     4,     0,     5,     0,     0
};

static const short po_hash_defgoto[] = {     1,
     4
};

static const short po_hash_pact[] = {-32768,
     0,    -3,    -1,-32768,     2,     5,-32768,     4,-32768,     3,
    -4,     8,     9,-32768,    11,-32768,    13,-32768
};

static const short po_hash_pgoto[] = {-32768,
-32768
};


#define	po_hash_LAST		15


static const short po_hash_table[] = {    17,
    12,     5,     2,     6,    13,     7,     3,     8,     9,    10,
    11,    14,    18,    15,    16
};

static const short po_hash_check[] = {     0,
     5,     5,     3,     5,     9,     4,     7,     3,     4,     6,
     8,     4,     0,     5,     4
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

#define po_hash_errok		(po_hash_errstatus = 0)
#define po_hash_clearin	(po_hash_char = po_hash_EMPTY)
#define po_hash_EMPTY		-2
#define po_hash_EOF		0
#define po_hash_ACCEPT	return(0)
#define po_hash_ABORT 	return(1)
#define po_hash_ERROR		goto po_hash_errlab1
/* Like po_hash_ERROR except do call po_hash_error.
   This remains here temporarily to ease the
   transition to the new meaning of po_hash_ERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define po_hash_FAIL		goto po_hash_errlab
#define po_hash_RECOVERING()  (!!po_hash_errstatus)
#define po_hash_BACKUP(token, value) \
do								\
  if (po_hash_char == po_hash_EMPTY && po_hash_len == 1)				\
    { po_hash_char = (token), po_hash_lval = (value);			\
      po_hash_char1 = po_hash_TRANSLATE (po_hash_char);				\
      po_hash_POPSTACK;						\
      goto po_hash_backup;						\
    }								\
  else								\
    { po_hash_error ("syntax error: cannot back up"); po_hash_ERROR; }	\
while (0)

#define po_hash_TERROR	1
#define po_hash_ERRCODE	256

#ifndef po_hash_PURE
#define po_hash_LEX		po_hash_lex()
#endif

#ifdef po_hash_PURE
#ifdef po_hash_LSP_NEEDED
#ifdef po_hash_LEX_PARAM
#define po_hash_LEX		po_hash_lex(&po_hash_lval, &po_hash_lloc, po_hash_LEX_PARAM)
#else
#define po_hash_LEX		po_hash_lex(&po_hash_lval, &po_hash_lloc)
#endif
#else /* not po_hash_LSP_NEEDED */
#ifdef po_hash_LEX_PARAM
#define po_hash_LEX		po_hash_lex(&po_hash_lval, po_hash_LEX_PARAM)
#else
#define po_hash_LEX		po_hash_lex(&po_hash_lval)
#endif
#endif /* not po_hash_LSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef po_hash_PURE

int	po_hash_char;			/*  the lookahead symbol		*/
po_hash_STYPE	po_hash_lval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef po_hash_LSP_NEEDED
po_hash_LTYPE po_hash_lloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int po_hash_nerrs;			/*  number of parse errors so far       */
#endif  /* not po_hash_PURE */

#if po_hash_DEBUG != 0
int po_hash_debug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  po_hash_INITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	po_hash_INITDEPTH
#define po_hash_INITDEPTH 200
#endif

/*  po_hash_MAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if po_hash_MAXDEPTH == 0
#undef po_hash_MAXDEPTH
#endif

#ifndef po_hash_MAXDEPTH
#define po_hash_MAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int po_hash_parse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __po_hash__memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__po_hash__memcpy (to, from, count)
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
__po_hash__memcpy (char *to, char *from, int count)
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

/* The user can define po_hash_PARSE_PARAM as the name of an argument to be passed
   into po_hash_parse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef po_hash_PARSE_PARAM
#ifdef __cplusplus
#define po_hash_PARSE_PARAM_ARG void *po_hash_PARSE_PARAM
#define po_hash_PARSE_PARAM_DECL
#else /* not __cplusplus */
#define po_hash_PARSE_PARAM_ARG po_hash_PARSE_PARAM
#define po_hash_PARSE_PARAM_DECL void *po_hash_PARSE_PARAM;
#endif /* not __cplusplus */
#else /* not po_hash_PARSE_PARAM */
#define po_hash_PARSE_PARAM_ARG
#define po_hash_PARSE_PARAM_DECL
#endif /* not po_hash_PARSE_PARAM */

int
po_hash_parse(po_hash_PARSE_PARAM_ARG)
     po_hash_PARSE_PARAM_DECL
{
  register int po_hash_state;
  register int po_hash_n;
  register short *po_hash_ssp;
  register po_hash_STYPE *po_hash_vsp;
  int po_hash_errstatus;	/*  number of tokens to shift before error messages enabled */
  int po_hash_char1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	po_hash_ssa[po_hash_INITDEPTH];	/*  the state stack			*/
  po_hash_STYPE po_hash_vsa[po_hash_INITDEPTH];	/*  the semantic value stack		*/

  short *po_hash_ss = po_hash_ssa;		/*  refer to the stacks thru separate pointers */
  po_hash_STYPE *po_hash_vs = po_hash_vsa;	/*  to allow po_hash_overflow to reallocate them elsewhere */

#ifdef po_hash_LSP_NEEDED
  po_hash_LTYPE po_hash_lsa[po_hash_INITDEPTH];	/*  the location stack			*/
  po_hash_LTYPE *po_hash_ls = po_hash_lsa;
  po_hash_LTYPE *po_hash_lsp;

#define po_hash_POPSTACK   (po_hash_vsp--, po_hash_ssp--, po_hash_lsp--)
#else
#define po_hash_POPSTACK   (po_hash_vsp--, po_hash_ssp--)
#endif

  int po_hash_stacksize = po_hash_INITDEPTH;

#ifdef po_hash_PURE
  int po_hash_char;
  po_hash_STYPE po_hash_lval;
  int po_hash_nerrs;
#ifdef po_hash_LSP_NEEDED
  po_hash_LTYPE po_hash_lloc;
#endif
#endif

  po_hash_STYPE po_hash_val;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int po_hash_len;

#if po_hash_DEBUG != 0
  if (po_hash_debug)
    fprintf(stderr, "Starting parse\n");
#endif

  po_hash_state = 0;
  po_hash_errstatus = 0;
  po_hash_nerrs = 0;
  po_hash_char = po_hash_EMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  po_hash_ssp = po_hash_ss - 1;
  po_hash_vsp = po_hash_vs;
#ifdef po_hash_LSP_NEEDED
  po_hash_lsp = po_hash_ls;
#endif

/* Push a new state, which is found in  po_hash_state  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
po_hash_newstate:

  *++po_hash_ssp = po_hash_state;

  if (po_hash_ssp >= po_hash_ss + po_hash_stacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      po_hash_STYPE *po_hash_vs1 = po_hash_vs;
      short *po_hash_ss1 = po_hash_ss;
#ifdef po_hash_LSP_NEEDED
      po_hash_LTYPE *po_hash_ls1 = po_hash_ls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = po_hash_ssp - po_hash_ss + 1;

#ifdef po_hash_overflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef po_hash_LSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if po_hash_overflow is a macro.  */
      po_hash_overflow("parser stack overflow",
		 &po_hash_ss1, size * sizeof (*po_hash_ssp),
		 &po_hash_vs1, size * sizeof (*po_hash_vsp),
		 &po_hash_ls1, size * sizeof (*po_hash_lsp),
		 &po_hash_stacksize);
#else
      po_hash_overflow("parser stack overflow",
		 &po_hash_ss1, size * sizeof (*po_hash_ssp),
		 &po_hash_vs1, size * sizeof (*po_hash_vsp),
		 &po_hash_stacksize);
#endif

      po_hash_ss = po_hash_ss1; po_hash_vs = po_hash_vs1;
#ifdef po_hash_LSP_NEEDED
      po_hash_ls = po_hash_ls1;
#endif
#else /* no po_hash_overflow */
      /* Extend the stack our own way.  */
      if (po_hash_stacksize >= po_hash_MAXDEPTH)
	{
	  po_hash_error("parser stack overflow");
	  return 2;
	}
      po_hash_stacksize *= 2;
      if (po_hash_stacksize > po_hash_MAXDEPTH)
	po_hash_stacksize = po_hash_MAXDEPTH;
      po_hash_ss = (short *) alloca (po_hash_stacksize * sizeof (*po_hash_ssp));
      __po_hash__memcpy ((char *)po_hash_ss, (char *)po_hash_ss1, size * sizeof (*po_hash_ssp));
      po_hash_vs = (po_hash_STYPE *) alloca (po_hash_stacksize * sizeof (*po_hash_vsp));
      __po_hash__memcpy ((char *)po_hash_vs, (char *)po_hash_vs1, size * sizeof (*po_hash_vsp));
#ifdef po_hash_LSP_NEEDED
      po_hash_ls = (po_hash_LTYPE *) alloca (po_hash_stacksize * sizeof (*po_hash_lsp));
      __po_hash__memcpy ((char *)po_hash_ls, (char *)po_hash_ls1, size * sizeof (*po_hash_lsp));
#endif
#endif /* no po_hash_overflow */

      po_hash_ssp = po_hash_ss + size - 1;
      po_hash_vsp = po_hash_vs + size - 1;
#ifdef po_hash_LSP_NEEDED
      po_hash_lsp = po_hash_ls + size - 1;
#endif

#if po_hash_DEBUG != 0
      if (po_hash_debug)
	fprintf(stderr, "Stack size increased to %d\n", po_hash_stacksize);
#endif

      if (po_hash_ssp >= po_hash_ss + po_hash_stacksize - 1)
	po_hash_ABORT;
    }

#if po_hash_DEBUG != 0
  if (po_hash_debug)
    fprintf(stderr, "Entering state %d\n", po_hash_state);
#endif

  goto po_hash_backup;
 po_hash_backup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* po_hash_resume: */

  /* First try to decide what to do without reference to lookahead token.  */

  po_hash_n = po_hash_pact[po_hash_state];
  if (po_hash_n == po_hash_FLAG)
    goto po_hash_default;

  /* Not known => get a lookahead token if don't already have one.  */

  /* po_hash_char is either po_hash_EMPTY or po_hash_EOF
     or a valid token in external form.  */

  if (po_hash_char == po_hash_EMPTY)
    {
#if po_hash_DEBUG != 0
      if (po_hash_debug)
	fprintf(stderr, "Reading a token: ");
#endif
      po_hash_char = po_hash_LEX;
    }

  /* Convert token to internal form (in po_hash_char1) for indexing tables with */

  if (po_hash_char <= 0)		/* This means end of input. */
    {
      po_hash_char1 = 0;
      po_hash_char = po_hash_EOF;		/* Don't call po_hash_LEX any more */

#if po_hash_DEBUG != 0
      if (po_hash_debug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      po_hash_char1 = po_hash_TRANSLATE(po_hash_char);

#if po_hash_DEBUG != 0
      if (po_hash_debug)
	{
	  fprintf (stderr, "Next token is %d (%s", po_hash_char, po_hash_tname[po_hash_char1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef po_hash_PRINT
	  po_hash_PRINT (stderr, po_hash_char, po_hash_lval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  po_hash_n += po_hash_char1;
  if (po_hash_n < 0 || po_hash_n > po_hash_LAST || po_hash_check[po_hash_n] != po_hash_char1)
    goto po_hash_default;

  po_hash_n = po_hash_table[po_hash_n];

  /* po_hash_n is what to do for this token type in this state.
     Negative => reduce, -po_hash_n is rule number.
     Positive => shift, po_hash_n is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (po_hash_n < 0)
    {
      if (po_hash_n == po_hash_FLAG)
	goto po_hash_errlab;
      po_hash_n = -po_hash_n;
      goto po_hash_reduce;
    }
  else if (po_hash_n == 0)
    goto po_hash_errlab;

  if (po_hash_n == po_hash_FINAL)
    po_hash_ACCEPT;

  /* Shift the lookahead token.  */

#if po_hash_DEBUG != 0
  if (po_hash_debug)
    fprintf(stderr, "Shifting token %d (%s), ", po_hash_char, po_hash_tname[po_hash_char1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (po_hash_char != po_hash_EOF)
    po_hash_char = po_hash_EMPTY;

  *++po_hash_vsp = po_hash_lval;
#ifdef po_hash_LSP_NEEDED
  *++po_hash_lsp = po_hash_lloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (po_hash_errstatus) po_hash_errstatus--;

  po_hash_state = po_hash_n;
  goto po_hash_newstate;

/* Do the default action for the current state.  */
po_hash_default:

  po_hash_n = po_hash_defact[po_hash_state];
  if (po_hash_n == 0)
    goto po_hash_errlab;

/* Do a reduction.  po_hash_n is the number of a rule to reduce with.  */
po_hash_reduce:
  po_hash_len = po_hash_r2[po_hash_n];
  if (po_hash_len > 0)
    po_hash_val = po_hash_vsp[1-po_hash_len]; /* implement default value of the action */

#if po_hash_DEBUG != 0
  if (po_hash_debug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       po_hash_n, po_hash_rline[po_hash_n]);

      /* Print the symbols being reduced, and their result.  */
      for (i = po_hash_prhs[po_hash_n]; po_hash_rhs[i] > 0; i++)
	fprintf (stderr, "%s ", po_hash_tname[po_hash_rhs[i]]);
      fprintf (stderr, " -> %s\n", po_hash_tname[po_hash_r1[po_hash_n]]);
    }
#endif


  switch (po_hash_n) {

case 3:
#line 88 "../../src/po-hash.y"
{
		  /* GNU style */
		  po_callback_comment_filepos (po_hash_vsp[-2].string, po_hash_vsp[0].number);
		  free (po_hash_vsp[-2].string);
		;
    break;}
case 4:
#line 94 "../../src/po-hash.y"
{
		  /* SunOS style */
		  po_callback_comment_filepos (po_hash_vsp[-4].string, po_hash_vsp[0].number);
		  free (po_hash_vsp[-4].string);
		;
    break;}
case 5:
#line 100 "../../src/po-hash.y"
{
		  /* Solaris style */
		  po_callback_comment_filepos (po_hash_vsp[-5].string, po_hash_vsp[0].number);
		  free (po_hash_vsp[-5].string);
		;
    break;}
case 6:
#line 106 "../../src/po-hash.y"
{
		  /* GNU style, but STRING is `file'.  Esoteric, but it
		     happened.  */
		  po_callback_comment_filepos ("file", po_hash_vsp[0].number);
		;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "/usr/lib/bison.simple"

  po_hash_vsp -= po_hash_len;
  po_hash_ssp -= po_hash_len;
#ifdef po_hash_LSP_NEEDED
  po_hash_lsp -= po_hash_len;
#endif

#if po_hash_DEBUG != 0
  if (po_hash_debug)
    {
      short *ssp1 = po_hash_ss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != po_hash_ssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++po_hash_vsp = po_hash_val;

#ifdef po_hash_LSP_NEEDED
  po_hash_lsp++;
  if (po_hash_len == 0)
    {
      po_hash_lsp->first_line = po_hash_lloc.first_line;
      po_hash_lsp->first_column = po_hash_lloc.first_column;
      po_hash_lsp->last_line = (po_hash_lsp-1)->last_line;
      po_hash_lsp->last_column = (po_hash_lsp-1)->last_column;
      po_hash_lsp->text = 0;
    }
  else
    {
      po_hash_lsp->last_line = (po_hash_lsp+po_hash_len-1)->last_line;
      po_hash_lsp->last_column = (po_hash_lsp+po_hash_len-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  po_hash_n = po_hash_r1[po_hash_n];

  po_hash_state = po_hash_pgoto[po_hash_n - po_hash_NTBASE] + *po_hash_ssp;
  if (po_hash_state >= 0 && po_hash_state <= po_hash_LAST && po_hash_check[po_hash_state] == *po_hash_ssp)
    po_hash_state = po_hash_table[po_hash_state];
  else
    po_hash_state = po_hash_defgoto[po_hash_n - po_hash_NTBASE];

  goto po_hash_newstate;

po_hash_errlab:   /* here on detecting error */

  if (! po_hash_errstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++po_hash_nerrs;

#ifdef po_hash_ERROR_VERBOSE
      po_hash_n = po_hash_pact[po_hash_state];

      if (po_hash_n > po_hash_FLAG && po_hash_n < po_hash_LAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -po_hash_n if nec to avoid negative indexes in po_hash_check.  */
	  for (x = (po_hash_n < 0 ? -po_hash_n : 0);
	       x < (sizeof(po_hash_tname) / sizeof(char *)); x++)
	    if (po_hash_check[x + po_hash_n] == x)
	      size += strlen(po_hash_tname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (po_hash_n < 0 ? -po_hash_n : 0);
		       x < (sizeof(po_hash_tname) / sizeof(char *)); x++)
		    if (po_hash_check[x + po_hash_n] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, po_hash_tname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      po_hash_error(msg);
	      free(msg);
	    }
	  else
	    po_hash_error ("parse error; also virtual memory exceeded");
	}
      else
#endif /* po_hash_ERROR_VERBOSE */
	po_hash_error("parse error");
    }

  goto po_hash_errlab1;
po_hash_errlab1:   /* here on error raised explicitly by an action */

  if (po_hash_errstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (po_hash_char == po_hash_EOF)
	po_hash_ABORT;

#if po_hash_DEBUG != 0
      if (po_hash_debug)
	fprintf(stderr, "Discarding token %d (%s).\n", po_hash_char, po_hash_tname[po_hash_char1]);
#endif

      po_hash_char = po_hash_EMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  po_hash_errstatus = 3;		/* Each real token shifted decrements this */

  goto po_hash_errhandle;

po_hash_errdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  po_hash_n = po_hash_defact[po_hash_state];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (po_hash_n) goto po_hash_default;
#endif

po_hash_errpop:   /* pop the current state because it cannot handle the error token */

  if (po_hash_ssp == po_hash_ss) po_hash_ABORT;
  po_hash_vsp--;
  po_hash_state = *--po_hash_ssp;
#ifdef po_hash_LSP_NEEDED
  po_hash_lsp--;
#endif

#if po_hash_DEBUG != 0
  if (po_hash_debug)
    {
      short *ssp1 = po_hash_ss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != po_hash_ssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

po_hash_errhandle:

  po_hash_n = po_hash_pact[po_hash_state];
  if (po_hash_n == po_hash_FLAG)
    goto po_hash_errdefault;

  po_hash_n += po_hash_TERROR;
  if (po_hash_n < 0 || po_hash_n > po_hash_LAST || po_hash_check[po_hash_n] != po_hash_TERROR)
    goto po_hash_errdefault;

  po_hash_n = po_hash_table[po_hash_n];
  if (po_hash_n < 0)
    {
      if (po_hash_n == po_hash_FLAG)
	goto po_hash_errpop;
      po_hash_n = -po_hash_n;
      goto po_hash_reduce;
    }
  else if (po_hash_n == 0)
    goto po_hash_errpop;

  if (po_hash_n == po_hash_FINAL)
    po_hash_ACCEPT;

#if po_hash_DEBUG != 0
  if (po_hash_debug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++po_hash_vsp = po_hash_lval;
#ifdef po_hash_LSP_NEEDED
  *++po_hash_lsp = po_hash_lloc;
#endif

  po_hash_state = po_hash_n;
  goto po_hash_newstate;
}
#line 113 "../../src/po-hash.y"



int
po_hash_lex ()
{
  static char *buf;
  static size_t bufmax;
  size_t bufpos;
  int n;
  int c;

  for (;;)
    {
      c = *cur++;
      switch (c)
	{
	case 0:
	  --cur;
	  return 0;

	case ' ':
	case '\t':
	case '\n':
	  break;

	case ':':
	  return COLON;

	case ',':
	  return COMMA;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  /* Accumulate a number.  */
	  n = 0;
	  for (;;)
	    {
	      n = n * 10 + c - '0';
	      c = *cur++;
	      switch (c)
		{
		default:
		  break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		  continue;
		}
	      break;
	    }
	  --cur;
	  po_hash_lval.number = n;
	  return NUMBER;

	default:
	  /* Accumulate a string.  */
	  bufpos = 0;
	  for (;;)
	    {
	      if (bufpos >= bufmax)
		{
		  bufmax += 100;
		  buf = xrealloc (buf, bufmax);
		}
	      buf[bufpos++] = c;

	      c = *cur++;
	      switch (c)
	        {
	        default:
	          continue;

	        case 0:
	        case ':':
	        case ',':
	        case ' ':
	        case '\t':
	          --cur;
	          break;
	        }
	      break;
	    }

	  if (bufpos >= bufmax)
	    {
	      bufmax += 100;
	      buf = xrealloc (buf, bufmax);
	    }
	  buf[bufpos] = 0;

	  if (strcmp (buf, "file") == 0 || strcmp (buf, "File") == 0)
	    return FILE_KEYWORD;
	  if (strcmp (buf, "line") == 0)
	    return LINE_KEYWORD;
	  if (strcmp (buf, "number") == 0)
	    return NUMBER_KEYWORD;
	  po_hash_lval.string = xstrdup (buf);
	  return STRING;
	}
    }
}
