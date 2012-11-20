/* macro.c - macro support for gas
   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005 Free Software Foundation, Inc.

   Written by Steve and Judy Chamberlain of Cygnus Support,
      sac@cygnus.com

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include "config.h"

#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
/* Indented so that pre-ansi C compilers will ignore it, rather than
   choke on it.  Some versions of AIX require this to be the first
   thing in the file.  */
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
#    if !defined (__STDC__) && !defined (__hpux)
extern char *alloca ();
#    else
extern void *alloca ();
#    endif /* __STDC__, __hpux */
#   endif /* alloca */
#  endif /* _AIX */
# endif /* HAVE_ALLOCA_H */
#endif /* __GNUC__ */

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include "as.h"
#include "libiberty.h"
#include "safe-ctype.h"
#include "sb.h"
#include "hash.h"
#include "macro.h"

#include "asintl.h"

/* The routines in this file handle macro definition and expansion.
   They are called by gas.  */

/* Internal functions.  */

static int get_token (int, sb *, sb *);
static int getstring (int, sb *, sb *);
static int get_any_string (int, sb *, sb *, int, int);
static int do_formals (macro_entry *, int, sb *);
static int get_apost_token (int, sb *, sb *, int);
static int sub_actual (int, sb *, sb *, struct hash_control *, int, sb *, int);
static const char *macro_expand_body
  (sb *, sb *, formal_entry *, struct hash_control *, int);
static const char *macro_expand (int, sb *, macro_entry *, sb *);

#define ISWHITE(x) ((x) == ' ' || (x) == '\t')

#define ISSEP(x) \
 ((x) == ' ' || (x) == '\t' || (x) == ',' || (x) == '"' || (x) == ';' \
  || (x) == ')' || (x) == '(' \
  || ((macro_alternate || macro_mri) && ((x) == '<' || (x) == '>')))

#define ISBASE(x) \
  ((x) == 'b' || (x) == 'B' \
   || (x) == 'q' || (x) == 'Q' \
   || (x) == 'h' || (x) == 'H' \
   || (x) == 'd' || (x) == 'D')

/* The macro hash table.  */

struct hash_control *macro_hash;

/* Whether any macros have been defined.  */

int macro_defined;

/* Whether we are in alternate syntax mode.  */

static int macro_alternate;

/* Whether we are in MRI mode.  */

static int macro_mri;

/* Whether we should strip '@' characters.  */

static int macro_strip_at;

/* Function to use to parse an expression.  */

static int (*macro_expr) (const char *, int, sb *, int *);

/* Number of macro expansions that have been done.  */

static int macro_number;

/* Initialize macro processing.  */

void
macro_init (int alternate, int mri, int strip_at,
	    int (*expr) (const char *, int, sb *, int *))
{
  macro_hash = hash_new ();
  macro_defined = 0;
  macro_alternate = alternate;
  macro_mri = mri;
  macro_strip_at = strip_at;
  macro_expr = expr;
}

/* Switch in and out of alternate mode on the fly.  */

void
macro_set_alternate (int alternate)
{
  macro_alternate = alternate;
}

/* Switch in and out of MRI mode on the fly.  */

void
macro_mri_mode (int mri)
{
  macro_mri = mri;
}

/* Read input lines till we get to a TO string.
   Increase nesting depth if we get a FROM string.
   Put the results into sb at PTR.
   FROM may be NULL (or will be ignored) if TO is "ENDR".
   Add a new input line to an sb using GET_LINE.
   Return 1 on success, 0 on unexpected EOF.  */

int
buffer_and_nest (const char *from, const char *to, sb *ptr,
		 int (*get_line) (sb *))
{
  int from_len;
  int to_len = strlen (to);
  int depth = 1;
  int line_start = ptr->len;

  int more = get_line (ptr);

  if (to_len == 4 && strcasecmp(to, "ENDR") == 0)
    {
      from = NULL;
      from_len = 0;
    }
  else
    from_len = strlen (from);

  while (more)
    {
      /* Try and find the first pseudo op on the line.  */
      int i = line_start;

      if (! NO_PSEUDO_DOT && ! flag_m68k_mri)
	{
	  /* With normal syntax we can suck what we want till we get
	     to the dot.  With the alternate, labels have to start in
	     the first column, since we can't tell what's a label and
	     whats a pseudoop.  */

	  /* Skip leading whitespace.  */
	  while (i < ptr->len && ISWHITE (ptr->ptr[i]))
	    i++;

	  /* Skip over a label.  */
	  while (i < ptr->len
		 && (ISALNUM (ptr->ptr[i])
		     || ptr->ptr[i] == '_'
		     || ptr->ptr[i] == '$'))
	    i++;

	  /* And a colon.  */
	  if (i < ptr->len
	      && ptr->ptr[i] == ':')
	    i++;

	}
      /* Skip trailing whitespace.  */
      while (i < ptr->len && ISWHITE (ptr->ptr[i]))
	i++;

      if (i < ptr->len && (ptr->ptr[i] == '.'
			   || NO_PSEUDO_DOT
			   || macro_mri))
	{
	  if (! flag_m68k_mri && ptr->ptr[i] == '.')
	    i++;
	  if (from == NULL
	     && strncasecmp (ptr->ptr + i, "IRPC", from_len = 4) != 0
	     && strncasecmp (ptr->ptr + i, "IRP", from_len = 3) != 0
	     && strncasecmp (ptr->ptr + i, "IREPC", from_len = 5) != 0
	     && strncasecmp (ptr->ptr + i, "IREP", from_len = 4) != 0
	     && strncasecmp (ptr->ptr + i, "REPT", from_len = 4) != 0
	     && strncasecmp (ptr->ptr + i, "REP", from_len = 3) != 0)
	    from_len = 0;
	  if ((from != NULL
	       ? strncasecmp (ptr->ptr + i, from, from_len) == 0
	       : from_len > 0)
	      && (ptr->len == (i + from_len)
		  || ! ISALNUM (ptr->ptr[i + from_len])))
	    depth++;
	  if (strncasecmp (ptr->ptr + i, to, to_len) == 0
	      && (ptr->len == (i + to_len)
		  || ! ISALNUM (ptr->ptr[i + to_len])))
	    {
	      depth--;
	      if (depth == 0)
		{
		  /* Reset the string to not include the ending rune.  */
		  ptr->len = line_start;
		  break;
		}
	    }
	}

      /* Add the original end-of-line char to the end and keep running.  */
      sb_add_char (ptr, more);
      line_start = ptr->len;
      more = get_line (ptr);
    }

  /* Return 1 on success, 0 on unexpected EOF.  */
  return depth == 0;
}

/* Pick up a token.  */

static int
get_token (int idx, sb *in, sb *name)
{
  if (idx < in->len
      && (ISALPHA (in->ptr[idx])
	  || in->ptr[idx] == '_'
	  || in->ptr[idx] == '$'))
    {
      sb_add_char (name, in->ptr[idx++]);
      while (idx < in->len
	     && (ISALNUM (in->ptr[idx])
		 || in->ptr[idx] == '_'
		 || in->ptr[idx] == '$'))
	{
	  sb_add_char (name, in->ptr[idx++]);
	}
    }
  /* Ignore trailing &.  */
  if (macro_alternate && idx < in->len && in->ptr[idx] == '&')
    idx++;
  return idx;
}

/* Pick up a string.  */

static int
getstring (int idx, sb *in, sb *acc)
{
  idx = sb_skip_white (idx, in);

  while (idx < in->len
	 && (in->ptr[idx] == '"'
	     || (in->ptr[idx] == '<' && (macro_alternate || macro_mri))
	     || (in->ptr[idx] == '\'' && macro_alternate)))
    {
      if (in->ptr[idx] == '<')
	{
	  int nest = 0;
	  idx++;
	  while ((in->ptr[idx] != '>' || nest)
		 && idx < in->len)
	    {
	      if (in->ptr[idx] == '!')
		{
		  idx++;
		  sb_add_char (acc, in->ptr[idx++]);
		}
	      else
		{
		  if (in->ptr[idx] == '>')
		    nest--;
		  if (in->ptr[idx] == '<')
		    nest++;
		  sb_add_char (acc, in->ptr[idx++]);
		}
	    }
	  idx++;
	}
      else if (in->ptr[idx] == '"' || in->ptr[idx] == '\'')
	{
	  char tchar = in->ptr[idx];
	  int escaped = 0;

	  idx++;

	  while (idx < in->len)
	    {
	      if (in->ptr[idx - 1] == '\\')
		escaped ^= 1;
	      else
		escaped = 0;

	      if (macro_alternate && in->ptr[idx] == '!')
		{
		  idx ++;

		  sb_add_char (acc, in->ptr[idx]);

		  idx ++;
		}
	      else if (escaped && in->ptr[idx] == tchar)
		{
		  sb_add_char (acc, tchar);
		  idx ++;
		}
	      else
		{
		  if (in->ptr[idx] == tchar)
		    {
		      idx ++;

		      if (idx >= in->len || in->ptr[idx] != tchar)
			break;
		    }

		  sb_add_char (acc, in->ptr[idx]);
		  idx ++;
		}
	    }
	}
    }

  return idx;
}

/* Fetch string from the input stream,
   rules:
    'Bxyx<whitespace>  	-> return 'Bxyza
    %<char>		-> return string of decimal value of x
    "<string>"		-> return string
    xyx<whitespace>     -> return xyz
*/

static int
get_any_string (int idx, sb *in, sb *out, int expand, int pretend_quoted)
{
  sb_reset (out);
  idx = sb_skip_white (idx, in);

  if (idx < in->len)
    {
      if (in->len > idx + 2 && in->ptr[idx + 1] == '\'' && ISBASE (in->ptr[idx]))
	{
	  while (!ISSEP (in->ptr[idx]))
	    sb_add_char (out, in->ptr[idx++]);
	}
      else if (in->ptr[idx] == '%'
	       && macro_alternate
	       && expand)
	{
	  int val;
	  char buf[20];
	  /* Turns the next expression into a string.  */
	  /* xgettext: no-c-format */
	  idx = (*macro_expr) (_("% operator needs absolute expression"),
			       idx + 1,
			       in,
			       &val);
	  sprintf (buf, "%d", val);
	  sb_add_string (out, buf);
	}
      else if (in->ptr[idx] == '"'
	       || (in->ptr[idx] == '<' && (macro_alternate || macro_mri))
	       || (macro_alternate && in->ptr[idx] == '\''))
	{
	  if (macro_alternate
	      && ! macro_strip_at
	      && expand)
	    {
	      /* Keep the quotes.  */
	      sb_add_char (out, '\"');

	      idx = getstring (idx, in, out);
	      sb_add_char (out, '\"');
	    }
	  else
	    {
	      idx = getstring (idx, in, out);
	    }
	}
      else
	{
	  while (idx < in->len
		 && (in->ptr[idx] == '"'
		     || in->ptr[idx] == '\''
		     || pretend_quoted
		     || (in->ptr[idx] != ' '
			 && in->ptr[idx] != '\t'
			 && in->ptr[idx] != ','
			 && (in->ptr[idx] != '<'
			     || (! macro_alternate && ! macro_mri)))))
	    {
	      if (in->ptr[idx] == '"'
		  || in->ptr[idx] == '\'')
		{
		  char tchar = in->ptr[idx];
		  sb_add_char (out, in->ptr[idx++]);
		  while (idx < in->len
			 && in->ptr[idx] != tchar)
		    sb_add_char (out, in->ptr[idx++]);
		  if (idx == in->len)
		    return idx;
		}
	      sb_add_char (out, in->ptr[idx++]);
	    }
	}
    }

  return idx;
}

/* Pick up the formal parameters of a macro definition.  */

static int
do_formals (macro_entry *macro, int idx, sb *in)
{
  formal_entry **p = &macro->formals;

  macro->formal_count = 0;
  macro->formal_hash = hash_new ();
  idx = sb_skip_white (idx, in);
  while (idx < in->len)
    {
      formal_entry *formal;
      int cidx;

      formal = (formal_entry *) xmalloc (sizeof (formal_entry));

      sb_new (&formal->name);
      sb_new (&formal->def);
      sb_new (&formal->actual);

      idx = get_token (idx, in, &formal->name);
      if (formal->name.len == 0)
	{
	  if (macro->formal_count)
	    --idx;
	  break;
	}
      idx = sb_skip_white (idx, in);
      /* This is a formal.  */
      if (idx < in->len && in->ptr[idx] == '=')
	{
	  /* Got a default.  */
	  idx = get_any_string (idx + 1, in, &formal->def, 1, 0);
	  idx = sb_skip_white (idx, in);
	}

      /* Add to macro's hash table.  */
      hash_jam (macro->formal_hash, sb_terminate (&formal->name), formal);

      formal->index = macro->formal_count++;
      cidx = idx;
      idx = sb_skip_comma (idx, in);
      if (idx != cidx && idx >= in->len)
	{
	  idx = cidx;
	  break;
	}
      *p = formal;
      p = &formal->next;
      *p = NULL;
    }

  if (macro_mri)
    {
      formal_entry *formal;
      const char *name;

      /* Add a special NARG formal, which macro_expand will set to the
         number of arguments.  */
      formal = (formal_entry *) xmalloc (sizeof (formal_entry));

      sb_new (&formal->name);
      sb_new (&formal->def);
      sb_new (&formal->actual);

      /* The same MRI assemblers which treat '@' characters also use
         the name $NARG.  At least until we find an exception.  */
      if (macro_strip_at)
	name = "$NARG";
      else
	name = "NARG";

      sb_add_string (&formal->name, name);

      /* Add to macro's hash table.  */
      hash_jam (macro->formal_hash, name, formal);

      formal->index = NARG_INDEX;
      *p = formal;
      formal->next = NULL;
    }

  return idx;
}

/* Define a new macro.  Returns NULL on success, otherwise returns an
   error message.  If NAMEP is not NULL, *NAMEP is set to the name of
   the macro which was defined.  */

const char *
define_macro (int idx, sb *in, sb *label,
	      int (*get_line) (sb *), const char **namep)
{
  macro_entry *macro;
  sb name;
  const char *namestr;

  macro = (macro_entry *) xmalloc (sizeof (macro_entry));
  sb_new (&macro->sub);
  sb_new (&name);

  macro->formal_count = 0;
  macro->formals = 0;

  idx = sb_skip_white (idx, in);
  if (! buffer_and_nest ("MACRO", "ENDM", &macro->sub, get_line))
    return _("unexpected end of file in macro definition");
  if (label != NULL && label->len != 0)
    {
      sb_add_sb (&name, label);
      if (idx < in->len && in->ptr[idx] == '(')
	{
	  /* It's the label: MACRO (formals,...)  sort  */
	  idx = do_formals (macro, idx + 1, in);
	  if (idx >= in->len || in->ptr[idx] != ')')
	    return _("missing ) after formals");
	  idx = sb_skip_white (idx + 1, in);
	}
      else
	{
	  /* It's the label: MACRO formals,...  sort  */
	  idx = do_formals (macro, idx, in);
	}
    }
  else
    {
      int cidx;

      idx = get_token (idx, in, &name);
      if (name.len == 0)
	return _("Missing macro name");
      cidx = sb_skip_white (idx, in);
      idx = sb_skip_comma (cidx, in);
      if (idx == cidx || idx < in->len)
	idx = do_formals (macro, idx, in);
      else
	idx = cidx;
    }
  if (idx < in->len)
    return _("Bad macro parameter list");

  /* And stick it in the macro hash table.  */
  for (idx = 0; idx < name.len; idx++)
    name.ptr[idx] = TOLOWER (name.ptr[idx]);
  namestr = sb_terminate (&name);
  if (hash_find (macro_hash, namestr))
    return _("Macro with this name was already defined");
  hash_jam (macro_hash, namestr, (PTR) macro);

  macro_defined = 1;

  if (namep != NULL)
    *namep = namestr;

  return NULL;
}

/* Scan a token, and then skip KIND.  */

static int
get_apost_token (int idx, sb *in, sb *name, int kind)
{
  idx = get_token (idx, in, name);
  if (idx < in->len
      && in->ptr[idx] == kind
      && (! macro_mri || macro_strip_at)
      && (! macro_strip_at || kind == '@'))
    idx++;
  return idx;
}

/* Substitute the actual value for a formal parameter.  */

static int
sub_actual (int start, sb *in, sb *t, struct hash_control *formal_hash,
	    int kind, sb *out, int copyifnotthere)
{
  int src;
  formal_entry *ptr;

  src = get_apost_token (start, in, t, kind);
  /* See if it's in the macro's hash table, unless this is
     macro_strip_at and kind is '@' and the token did not end in '@'.  */
  if (macro_strip_at
      && kind == '@'
      && (src == start || in->ptr[src - 1] != '@'))
    ptr = NULL;
  else
    ptr = (formal_entry *) hash_find (formal_hash, sb_terminate (t));
  if (ptr)
    {
      if (ptr->actual.len)
	{
	  sb_add_sb (out, &ptr->actual);
	}
      else
	{
	  sb_add_sb (out, &ptr->def);
	}
    }
  else if (kind == '&')
    {
      /* Doing this permits people to use & in macro bodies.  */
      sb_add_char (out, '&');
      sb_add_sb (out, t);
    }
  else if (copyifnotthere)
    {
      sb_add_sb (out, t);
    }
  else
    {
      sb_add_char (out, '\\');
      sb_add_sb (out, t);
    }
  return src;
}

/* Expand the body of a macro.  */

static const char *
macro_expand_body (sb *in, sb *out, formal_entry *formals,
		   struct hash_control *formal_hash, int locals)
{
  sb t;
  int src = 0;
  int inquote = 0;
  formal_entry *loclist = NULL;

  sb_new (&t);

  while (src < in->len)
    {
      if (in->ptr[src] == '&')
	{
	  sb_reset (&t);
	  if (macro_mri)
	    {
	      if (src + 1 < in->len && in->ptr[src + 1] == '&')
		src = sub_actual (src + 2, in, &t, formal_hash, '\'', out, 1);
	      else
		sb_add_char (out, in->ptr[src++]);
	    }
	  else
	    {
	      /* FIXME: Why do we do this?  */
	      src = sub_actual (src + 1, in, &t, formal_hash, '&', out, 0);
	    }
	}
      else if (in->ptr[src] == '\\')
	{
	  src++;
	  if (in->ptr[src] == '(')
	    {
	      /* Sub in till the next ')' literally.  */
	      src++;
	      while (src < in->len && in->ptr[src] != ')')
		{
		  sb_add_char (out, in->ptr[src++]);
		}
	      if (in->ptr[src] == ')')
		src++;
	      else
		return _("missplaced )");
	    }
	  else if (in->ptr[src] == '@')
	    {
	      /* Sub in the macro invocation number.  */

	      char buffer[10];
	      src++;
	      sprintf (buffer, "%d", macro_number);
	      sb_add_string (out, buffer);
	    }
	  else if (in->ptr[src] == '&')
	    {
	      /* This is a preprocessor variable name, we don't do them
		 here.  */
	      sb_add_char (out, '\\');
	      sb_add_char (out, '&');
	      src++;
	    }
	  else if (macro_mri && ISALNUM (in->ptr[src]))
	    {
	      int ind;
	      formal_entry *f;

	      if (ISDIGIT (in->ptr[src]))
		ind = in->ptr[src] - '0';
	      else if (ISUPPER (in->ptr[src]))
		ind = in->ptr[src] - 'A' + 10;
	      else
		ind = in->ptr[src] - 'a' + 10;
	      ++src;
	      for (f = formals; f != NULL; f = f->next)
		{
		  if (f->index == ind - 1)
		    {
		      if (f->actual.len != 0)
			sb_add_sb (out, &f->actual);
		      else
			sb_add_sb (out, &f->def);
		      break;
		    }
		}
	    }
	  else
	    {
	      sb_reset (&t);
	      src = sub_actual (src, in, &t, formal_hash, '\'', out, 0);
	    }
	}
      else if ((macro_alternate || macro_mri)
	       && (ISALPHA (in->ptr[src])
		   || in->ptr[src] == '_'
		   || in->ptr[src] == '$')
	       && (! inquote
		   || ! macro_strip_at
		   || (src > 0 && in->ptr[src - 1] == '@')))
	{
	  if (! locals
	      || src + 5 >= in->len
	      || strncasecmp (in->ptr + src, "LOCAL", 5) != 0
	      || ! ISWHITE (in->ptr[src + 5]))
	    {
	      sb_reset (&t);
	      src = sub_actual (src, in, &t, formal_hash,
				(macro_strip_at && inquote) ? '@' : '\'',
				out, 1);
	    }
	  else
	    {
	      formal_entry *f;

	      src = sb_skip_white (src + 5, in);
	      while (in->ptr[src] != '\n')
		{
		  static int loccnt;
		  char buf[20];
		  const char *err;

		  f = (formal_entry *) xmalloc (sizeof (formal_entry));
		  sb_new (&f->name);
		  sb_new (&f->def);
		  sb_new (&f->actual);
		  f->index = LOCAL_INDEX;
		  f->next = loclist;
		  loclist = f;

		  src = get_token (src, in, &f->name);
		  ++loccnt;
		  sprintf (buf, IS_ELF ? ".LL%04x" : "LL%04x", loccnt);
		  sb_add_string (&f->actual, buf);

		  err = hash_jam (formal_hash, sb_terminate (&f->name), f);
		  if (err != NULL)
		    return err;

		  src = sb_skip_comma (src, in);
		}
	    }
	}
      else if (in->ptr[src] == '"'
	       || (macro_mri && in->ptr[src] == '\''))
	{
	  inquote = !inquote;
	  sb_add_char (out, in->ptr[src++]);
	}
      else if (in->ptr[src] == '@' && macro_strip_at)
	{
	  ++src;
	  if (src < in->len
	      && in->ptr[src] == '@')
	    {
	      sb_add_char (out, '@');
	      ++src;
	    }
	}
      else if (macro_mri
	       && in->ptr[src] == '='
	       && src + 1 < in->len
	       && in->ptr[src + 1] == '=')
	{
	  formal_entry *ptr;

	  sb_reset (&t);
	  src = get_token (src + 2, in, &t);
	  ptr = (formal_entry *) hash_find (formal_hash, sb_terminate (&t));
	  if (ptr == NULL)
	    {
	      /* FIXME: We should really return a warning string here,
                 but we can't, because the == might be in the MRI
                 comment field, and, since the nature of the MRI
                 comment field depends upon the exact instruction
                 being used, we don't have enough information here to
                 figure out whether it is or not.  Instead, we leave
                 the == in place, which should cause a syntax error if
                 it is not in a comment.  */
	      sb_add_char (out, '=');
	      sb_add_char (out, '=');
	      sb_add_sb (out, &t);
	    }
	  else
	    {
	      if (ptr->actual.len)
		{
		  sb_add_string (out, "-1");
		}
	      else
		{
		  sb_add_char (out, '0');
		}
	    }
	}
      else
	{
	  sb_add_char (out, in->ptr[src++]);
	}
    }

  sb_kill (&t);

  while (loclist != NULL)
    {
      formal_entry *f;

      f = loclist->next;
      /* Setting the value to NULL effectively deletes the entry.  We
         avoid calling hash_delete because it doesn't reclaim memory.  */
      hash_jam (formal_hash, sb_terminate (&loclist->name), NULL);
      sb_kill (&loclist->name);
      sb_kill (&loclist->def);
      sb_kill (&loclist->actual);
      free (loclist);
      loclist = f;
    }

  return NULL;
}

/* Assign values to the formal parameters of a macro, and expand the
   body.  */

static const char *
macro_expand (int idx, sb *in, macro_entry *m, sb *out)
{
  sb t;
  formal_entry *ptr;
  formal_entry *f;
  int is_positional = 0;
  int is_keyword = 0;
  int narg = 0;
  const char *err;

  sb_new (&t);

  /* Reset any old value the actuals may have.  */
  for (f = m->formals; f; f = f->next)
    sb_reset (&f->actual);
  f = m->formals;
  while (f != NULL && f->index < 0)
    f = f->next;

  if (macro_mri)
    {
      /* The macro may be called with an optional qualifier, which may
         be referred to in the macro body as \0.  */
      if (idx < in->len && in->ptr[idx] == '.')
	{
	  /* The Microtec assembler ignores this if followed by a white space.
	     (Macro invocation with empty extension) */
	  idx++;
	  if (    idx < in->len
		  && in->ptr[idx] != ' '
		  && in->ptr[idx] != '\t')
	    {
	      formal_entry *n;

	      n = (formal_entry *) xmalloc (sizeof (formal_entry));
	      sb_new (&n->name);
	      sb_new (&n->def);
	      sb_new (&n->actual);
	      n->index = QUAL_INDEX;

	      n->next = m->formals;
	      m->formals = n;

	      idx = get_any_string (idx, in, &n->actual, 1, 0);
	    }
	}
    }

  /* Peel off the actuals and store them away in the hash tables' actuals.  */
  idx = sb_skip_white (idx, in);
  while (idx < in->len)
    {
      int scan;

      /* Look and see if it's a positional or keyword arg.  */
      scan = idx;
      while (scan < in->len
	     && !ISSEP (in->ptr[scan])
	     && !(macro_mri && in->ptr[scan] == '\'')
	     && (!macro_alternate && in->ptr[scan] != '='))
	scan++;
      if (scan < in->len && !macro_alternate && in->ptr[scan] == '=')
	{
	  is_keyword = 1;

	  /* It's OK to go from positional to keyword.  */

	  /* This is a keyword arg, fetch the formal name and
	     then the actual stuff.  */
	  sb_reset (&t);
	  idx = get_token (idx, in, &t);
	  if (in->ptr[idx] != '=')
	    return _("confusion in formal parameters");

	  /* Lookup the formal in the macro's list.  */
	  ptr = (formal_entry *) hash_find (m->formal_hash, sb_terminate (&t));
	  if (!ptr)
	    return _("macro formal argument does not exist");
	  else
	    {
	      /* Insert this value into the right place.  */
	      sb_reset (&ptr->actual);
	      idx = get_any_string (idx + 1, in, &ptr->actual, 0, 0);
	      if (ptr->actual.len > 0)
		++narg;
	    }
	}
      else
	{
	  /* This is a positional arg.  */
	  is_positional = 1;
	  if (is_keyword)
	    return _("can't mix positional and keyword arguments");

	  if (!f)
	    {
	      formal_entry **pf;
	      int c;

	      if (!macro_mri)
		return _("too many positional arguments");

	      f = (formal_entry *) xmalloc (sizeof (formal_entry));
	      sb_new (&f->name);
	      sb_new (&f->def);
	      sb_new (&f->actual);
	      f->next = NULL;

	      c = -1;
	      for (pf = &m->formals; *pf != NULL; pf = &(*pf)->next)
		if ((*pf)->index >= c)
		  c = (*pf)->index + 1;
	      if (c == -1)
		c = 0;
	      *pf = f;
	      f->index = c;
	    }

	  sb_reset (&f->actual);
	  idx = get_any_string (idx, in, &f->actual, 1, 0);
	  if (f->actual.len > 0)
	    ++narg;
	  do
	    {
	      f = f->next;
	    }
	  while (f != NULL && f->index < 0);
	}

      if (! macro_mri)
	idx = sb_skip_comma (idx, in);
      else
	{
	  if (in->ptr[idx] == ',')
	    ++idx;
	  if (ISWHITE (in->ptr[idx]))
	    break;
	}
    }

  if (macro_mri)
    {
      char buffer[20];

      sb_reset (&t);
      sb_add_string (&t, macro_strip_at ? "$NARG" : "NARG");
      ptr = (formal_entry *) hash_find (m->formal_hash, sb_terminate (&t));
      sb_reset (&ptr->actual);
      sprintf (buffer, "%d", narg);
      sb_add_string (&ptr->actual, buffer);
    }

  err = macro_expand_body (&m->sub, out, m->formals, m->formal_hash, 1);
  if (err != NULL)
    return err;

  /* Discard any unnamed formal arguments.  */
  if (macro_mri)
    {
      formal_entry **pf;

      pf = &m->formals;
      while (*pf != NULL)
	{
	  if ((*pf)->name.len != 0)
	    pf = &(*pf)->next;
	  else
	    {
	      sb_kill (&(*pf)->name);
	      sb_kill (&(*pf)->def);
	      sb_kill (&(*pf)->actual);
	      f = (*pf)->next;
	      free (*pf);
	      *pf = f;
	    }
	}
    }

  sb_kill (&t);
  macro_number++;

  return NULL;
}

/* Check for a macro.  If one is found, put the expansion into
   *EXPAND.  Return 1 if a macro is found, 0 otherwise.  */

int
check_macro (const char *line, sb *expand,
	     const char **error, macro_entry **info)
{
  const char *s;
  char *copy, *cs;
  macro_entry *macro;
  sb line_sb;

  if (! ISALPHA (*line)
      && *line != '_'
      && *line != '$'
      && (! macro_mri || *line != '.'))
    return 0;

  s = line + 1;
  while (ISALNUM (*s)
	 || *s == '_'
	 || *s == '$')
    ++s;

  copy = (char *) alloca (s - line + 1);
  memcpy (copy, line, s - line);
  copy[s - line] = '\0';
  for (cs = copy; *cs != '\0'; cs++)
    *cs = TOLOWER (*cs);

  macro = (macro_entry *) hash_find (macro_hash, copy);

  if (macro == NULL)
    return 0;

  /* Wrap the line up in an sb.  */
  sb_new (&line_sb);
  while (*s != '\0' && *s != '\n' && *s != '\r')
    sb_add_char (&line_sb, *s++);

  sb_new (expand);
  *error = macro_expand (0, &line_sb, macro, expand);

  sb_kill (&line_sb);

  /* Export the macro information if requested.  */
  if (info)
    *info = macro;

  return 1;
}

/* Delete a macro.  */

void
delete_macro (const char *name)
{
  hash_delete (macro_hash, name);
}

/* Handle the MRI IRP and IRPC pseudo-ops.  These are handled as a
   combined macro definition and execution.  This returns NULL on
   success, or an error message otherwise.  */

const char *
expand_irp (int irpc, int idx, sb *in, sb *out, int (*get_line) (sb *))
{
  sb sub;
  formal_entry f;
  struct hash_control *h;
  const char *err;

  idx = sb_skip_white (idx, in);

  sb_new (&sub);
  if (! buffer_and_nest (NULL, "ENDR", &sub, get_line))
    return _("unexpected end of file in irp or irpc");

  sb_new (&f.name);
  sb_new (&f.def);
  sb_new (&f.actual);

  idx = get_token (idx, in, &f.name);
  if (f.name.len == 0)
    return _("missing model parameter");

  h = hash_new ();
  err = hash_jam (h, sb_terminate (&f.name), &f);
  if (err != NULL)
    return err;

  f.index = 1;
  f.next = NULL;

  sb_reset (out);

  idx = sb_skip_comma (idx, in);
  if (idx >= in->len)
    {
      /* Expand once with a null string.  */
      err = macro_expand_body (&sub, out, &f, h, 0);
      if (err != NULL)
	return err;
    }
  else
    {
      if (irpc && in->ptr[idx] == '"')
	++idx;
      while (idx < in->len)
	{
	  if (!irpc)
	    idx = get_any_string (idx, in, &f.actual, 1, 0);
	  else
	    {
	      if (in->ptr[idx] == '"')
		{
		  int nxt;

		  nxt = sb_skip_white (idx + 1, in);
		  if (nxt >= in->len)
		    {
		      idx = nxt;
		      break;
		    }
		}
	      sb_reset (&f.actual);
	      sb_add_char (&f.actual, in->ptr[idx]);
	      ++idx;
	    }
	  err = macro_expand_body (&sub, out, &f, h, 0);
	  if (err != NULL)
	    return err;
	  if (!irpc)
	    idx = sb_skip_comma (idx, in);
	  else
	    idx = sb_skip_white (idx, in);
	}
    }

  hash_die (h);
  sb_kill (&sub);

  return NULL;
}
