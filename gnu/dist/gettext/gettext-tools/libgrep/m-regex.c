/* Pattern Matchers for Regular Expressions.
   Copyright (C) 1992, 1998, 2000, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "libgrep.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "exitfail.h"
#include "xalloc.h"
#include "m-common.h"

/* This must be included _after_ m-common.h: It depends on MBS_SUPPORT.  */
#include "dfa.h"

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(c) 1 
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif
#define ISALNUM(C) (IN_CTYPE_DOMAIN (C) && isalnum (C))

struct compiled_regex {
  bool match_words;
  bool match_lines;
  char eolbyte;

  /* DFA compiled regexp. */
  struct dfa dfa;

  /* The Regex compiled patterns.  */
  struct patterns
  {
    /* Regex compiled regexp. */
    struct re_pattern_buffer regexbuf;
    struct re_registers regs; /* This is here on account of a BRAIN-DEAD
				 Q@#%!# library interface in regex.c.  */
  } *patterns;
  size_t pcount;

  /* KWset compiled pattern.  We compile a list of strings, at least one of
     which is known to occur in any string matching the regexp. */
  struct compiled_kwset ckwset;

  /* Number of compiled fixed strings known to exactly match the regexp.
     If kwsexec returns < kwset_exact_matches, then we don't need to
     call the regexp matcher at all. */
  int kwset_exact_matches;
};

/* Callback from dfa.c.  */
void
dfaerror (const char *mesg)
{
  error (exit_failure, 0, mesg);
}

/* If the DFA turns out to have some set of fixed strings one of
   which must occur in the match, then we build a kwset matcher
   to find those strings, and thus quickly filter out impossible
   matches. */
static void
kwsmusts (struct compiled_regex *cregex,
	  bool match_icase, bool match_words, bool match_lines, char eolbyte)
{
  struct dfamust const *dm;
  const char *err;

  if (cregex->dfa.musts)
    {
      kwsinit (&cregex->ckwset, match_icase, match_words, match_lines, eolbyte);
      /* First, we compile in the substrings known to be exact
	 matches.  The kwset matcher will return the index
	 of the matching string that it chooses. */
      for (dm = cregex->dfa.musts; dm; dm = dm->next)
	{
	  if (!dm->exact)
	    continue;
	  cregex->kwset_exact_matches++;
	  if ((err = kwsincr (cregex->ckwset.kwset, dm->must, strlen (dm->must))) != NULL)
	    error (exit_failure, 0, err);
	}
      /* Now, we compile the substrings that will require
	 the use of the regexp matcher.  */
      for (dm = cregex->dfa.musts; dm; dm = dm->next)
	{
	  if (dm->exact)
	    continue;
	  if ((err = kwsincr (cregex->ckwset.kwset, dm->must, strlen (dm->must))) != NULL)
	    error (exit_failure, 0, err);
	}
      if ((err = kwsprep (cregex->ckwset.kwset)) != NULL)
	error (exit_failure, 0, err);
    }
}

static void *
Gcompile (const char *pattern, size_t pattern_size,
	  bool match_icase, bool match_words, bool match_lines, char eolbyte)
{
  struct compiled_regex *cregex;
  const char *err;
  const char *sep;
  size_t total = pattern_size;
  const char *motif = pattern;

  cregex = (struct compiled_regex *) xmalloc (sizeof (struct compiled_regex));
  memset (cregex, '\0', sizeof (struct compiled_regex));
  cregex->match_words = match_words;
  cregex->match_lines = match_lines;
  cregex->eolbyte = eolbyte;
  cregex->patterns = NULL;
  cregex->pcount = 0;

  re_set_syntax (RE_SYNTAX_GREP | RE_HAT_LISTS_NOT_NEWLINE);
  dfasyntax (RE_SYNTAX_GREP | RE_HAT_LISTS_NOT_NEWLINE, match_icase, eolbyte);

  /* For GNU regex compiler we have to pass the patterns separately to detect
     errors like "[\nallo\n]\n".  The patterns here are "[", "allo" and "]"
     GNU regex should have raise a syntax error.  The same for backref, where
     the backref should have been local to each pattern.  */
  do
    {
      size_t len;
      sep = memchr (motif, '\n', total);
      if (sep)
	{
	  len = sep - motif;
	  sep++;
	  total -= (len + 1);
	}
      else
	{
	  len = total;
	  total = 0;
	}

      cregex->patterns = xrealloc (cregex->patterns, (cregex->pcount + 1) * sizeof (struct patterns));
      memset (&cregex->patterns[cregex->pcount], '\0', sizeof (struct patterns));

      if ((err = re_compile_pattern (motif, len,
				     &(cregex->patterns[cregex->pcount].regexbuf))) != NULL)
	error (exit_failure, 0, err);
      cregex->pcount++;

      motif = sep;
    } while (sep && total != 0);

  /* In the match_words and match_lines cases, we use a different pattern
     for the DFA matcher that will quickly throw out cases that won't work.
     Then if DFA succeeds we do some hairy stuff using the regex matcher
     to decide whether the match should really count. */
  if (match_words || match_lines)
    {
      /* In the whole-word case, we use the pattern:
	 \(^\|[^[:alnum:]_]\)\(userpattern\)\([^[:alnum:]_]|$\).
	 In the whole-line case, we use the pattern:
	 ^\(userpattern\)$.  */

      static const char line_beg[] = "^\\(";
      static const char line_end[] = "\\)$";
      static const char word_beg[] = "\\(^\\|[^[:alnum:]_]\\)\\(";
      static const char word_end[] = "\\)\\([^[:alnum:]_]\\|$\\)";
      char *n = (char *) xmalloc (sizeof word_beg - 1 + pattern_size + sizeof word_end);
      size_t i;
      strcpy (n, match_lines ? line_beg : word_beg);
      i = strlen (n);
      memcpy (n + i, pattern, pattern_size);
      i += pattern_size;
      strcpy (n + i, match_lines ? line_end : word_end);
      i += strlen (n + i);
      pattern = n;
      pattern_size = i;
    }

  dfacomp (pattern, pattern_size, &cregex->dfa, 1);
  kwsmusts (cregex, match_icase, match_words, match_lines, eolbyte);

  return cregex;
}

static void *
compile (const char *pattern, size_t pattern_size,
	 bool match_icase, bool match_words, bool match_lines, char eolbyte,
	 reg_syntax_t syntax)
{
  struct compiled_regex *cregex;
  const char *err;
  const char *sep;
  size_t total = pattern_size;
  const char *motif = pattern;

  cregex = (struct compiled_regex *) xmalloc (sizeof (struct compiled_regex));
  memset (cregex, '\0', sizeof (struct compiled_regex));
  cregex->match_words = match_words;
  cregex->match_lines = match_lines;
  cregex->eolbyte = eolbyte;
  cregex->patterns = NULL;
  cregex->pcount = 0;

  re_set_syntax (syntax);
  dfasyntax (syntax, match_icase, eolbyte);

  /* For GNU regex compiler we have to pass the patterns separately to detect
     errors like "[\nallo\n]\n".  The patterns here are "[", "allo" and "]"
     GNU regex should have raise a syntax error.  The same for backref, where
     the backref should have been local to each pattern.  */
  do
    {
      size_t len;
      sep = memchr (motif, '\n', total);
      if (sep)
	{
	  len = sep - motif;
	  sep++;
	  total -= (len + 1);
	}
      else
	{
	  len = total;
	  total = 0;
	}

      cregex->patterns = xrealloc (cregex->patterns, (cregex->pcount + 1) * sizeof (struct patterns));
      memset (&cregex->patterns[cregex->pcount], '\0', sizeof (struct patterns));

      if ((err = re_compile_pattern (motif, len,
				     &(cregex->patterns[cregex->pcount].regexbuf))) != NULL)
	error (exit_failure, 0, err);
      cregex->pcount++;

      motif = sep;
    } while (sep && total != 0);

  /* In the match_words and match_lines cases, we use a different pattern
     for the DFA matcher that will quickly throw out cases that won't work.
     Then if DFA succeeds we do some hairy stuff using the regex matcher
     to decide whether the match should really count. */
  if (match_words || match_lines)
    {
      /* In the whole-word case, we use the pattern:
	 (^|[^[:alnum:]_])(userpattern)([^[:alnum:]_]|$).
	 In the whole-line case, we use the pattern:
	 ^(userpattern)$.  */

      static const char line_beg[] = "^(";
      static const char line_end[] = ")$";
      static const char word_beg[] = "(^|[^[:alnum:]_])(";
      static const char word_end[] = ")([^[:alnum:]_]|$)";
      char *n = (char *) xmalloc (sizeof word_beg - 1 + pattern_size + sizeof word_end);
      size_t i;
      strcpy (n, match_lines ? line_beg : word_beg);
      i = strlen(n);
      memcpy (n + i, pattern, pattern_size);
      i += pattern_size;
      strcpy (n + i, match_lines ? line_end : word_end);
      i += strlen (n + i);
      pattern = n;
      pattern_size = i;
    }

  dfacomp (pattern, pattern_size, &cregex->dfa, 1);
  kwsmusts (cregex, match_icase, match_words, match_lines, eolbyte);

  return cregex;
}

static void *
Ecompile (const char *pattern, size_t pattern_size,
	  bool match_icase, bool match_words, bool match_lines, char eolbyte)
{
  return compile (pattern, pattern_size,
		  match_icase, match_words, match_lines, eolbyte,
		  RE_SYNTAX_POSIX_EGREP);
}

static void *
AWKcompile (const char *pattern, size_t pattern_size,
	    bool match_icase, bool match_words, bool match_lines, char eolbyte)
{
  return compile (pattern, pattern_size,
		  match_icase, match_words, match_lines, eolbyte,
		  RE_SYNTAX_AWK);
}

static size_t
EGexecute (const void *compiled_pattern,
	   const char *buf, size_t buf_size,
	   size_t *match_size, bool exact)
{
  struct compiled_regex *cregex = (struct compiled_regex *) compiled_pattern;
  register const char *buflim, *beg, *end;
  char eol = cregex->eolbyte;
  int backref, start, len;
  struct kwsmatch kwsm;
  size_t i;
#ifdef MBS_SUPPORT
  char *mb_properties = NULL;
#endif /* MBS_SUPPORT */

#ifdef MBS_SUPPORT
  if (MB_CUR_MAX > 1 && cregex->ckwset.kwset)
    mb_properties = check_multibyte_string (buf, buf_size);
#endif /* MBS_SUPPORT */

  buflim = buf + buf_size;

  for (beg = end = buf; end < buflim; beg = end)
    {
      if (!exact)
	{
	  if (cregex->ckwset.kwset)
	    {
	      /* Find a possible match using the KWset matcher. */
	      size_t offset = kwsexec (cregex->ckwset.kwset, beg, buflim - beg, &kwsm);
	      if (offset == (size_t) -1)
		{
#ifdef MBS_SUPPORT
		  if (MB_CUR_MAX > 1)
		    free (mb_properties);
#endif
		  return (size_t)-1;
		}
	      beg += offset;
	      /* Narrow down to the line containing the candidate, and
		 run it through DFA. */
	      end = memchr (beg, eol, buflim - beg);
	      if (end != NULL)
		end++;
	      else
		end = buflim;
#ifdef MBS_SUPPORT
	      if (MB_CUR_MAX > 1 && mb_properties[beg - buf] == 0)
		continue;
#endif
	      while (beg > buf && beg[-1] != eol)
		--beg;
	      if (kwsm.index < cregex->kwset_exact_matches)
		goto success;
	      if (dfaexec (&cregex->dfa, beg, end - beg, &backref) == (size_t) -1)
		continue;
	    }
	  else
	    {
	      /* No good fixed strings; start with DFA. */
	      size_t offset = dfaexec (&cregex->dfa, beg, buflim - beg, &backref);
	      if (offset == (size_t) -1)
		break;
	      /* Narrow down to the line we've found. */
	      beg += offset;
	      end = memchr (beg, eol, buflim - beg);
	      if (end != NULL)
		end++;
	      else
		end = buflim;
	      while (beg > buf && beg[-1] != eol)
		--beg;
	    }
	  /* Successful, no backreferences encountered! */
	  if (!backref)
	    goto success;
	}
      else
	end = beg + buf_size;

      /* If we've made it to this point, this means DFA has seen
	 a probable match, and we need to run it through Regex. */
      for (i = 0; i < cregex->pcount; i++)
	{
	  cregex->patterns[i].regexbuf.not_eol = 0;
	  if (0 <= (start = re_search (&(cregex->patterns[i].regexbuf), beg,
				       end - beg - 1, 0,
				       end - beg - 1, &(cregex->patterns[i].regs))))
	    {
	      len = cregex->patterns[i].regs.end[0] - start;
	      if (exact)
		{
		  *match_size = len;
		  return start;
		}
	      if ((!cregex->match_lines && !cregex->match_words)
		  || (cregex->match_lines && len == end - beg - 1))
		goto success;
	      /* If -w, check if the match aligns with word boundaries.
		 We do this iteratively because:
		 (a) the line may contain more than one occurence of the
		 pattern, and
		 (b) Several alternatives in the pattern might be valid at a
		 given point, and we may need to consider a shorter one to
		 find a word boundary.  */
	      if (cregex->match_words)
		while (start >= 0)
		  {
		    if ((start == 0 || !IS_WORD_CONSTITUENT ((unsigned char) beg[start - 1]))
			&& (len == end - beg - 1
			    || !IS_WORD_CONSTITUENT ((unsigned char) beg[start + len])))
		      goto success;
		    if (len > 0)
		      {
			/* Try a shorter length anchored at the same place. */
			--len;
			cregex->patterns[i].regexbuf.not_eol = 1;
			len = re_match (&(cregex->patterns[i].regexbuf), beg,
					start + len, start,
					&(cregex->patterns[i].regs));
		      }
		    if (len <= 0)
		      {
			/* Try looking further on. */
			if (start == end - beg - 1)
			  break;
			++start;
			cregex->patterns[i].regexbuf.not_eol = 0;
			start = re_search (&(cregex->patterns[i].regexbuf), beg,
					   end - beg - 1,
					   start, end - beg - 1 - start,
					   &(cregex->patterns[i].regs));
			len = cregex->patterns[i].regs.end[0] - start;
		      }
		  }
	    }
	} /* for Regex patterns.  */
    } /* for (beg = end ..) */
#ifdef MBS_SUPPORT
  if (MB_CUR_MAX > 1 && mb_properties)
    free (mb_properties);
#endif /* MBS_SUPPORT */
  return (size_t) -1;

 success:
#ifdef MBS_SUPPORT
  if (MB_CUR_MAX > 1 && mb_properties)
    free (mb_properties);
#endif /* MBS_SUPPORT */
  *match_size = end - beg;
  return beg - buf;
}

static void
EGfree (void *compiled_pattern)
{
  struct compiled_regex *cregex = (struct compiled_regex *) compiled_pattern;

  dfafree (&cregex->dfa);
  free (cregex->patterns);
  free (cregex->ckwset.trans);
  free (cregex);
}

/* POSIX Basic Regular Expressions */
matcher_t matcher_grep =
  {
    Gcompile,
    EGexecute,
    EGfree
  };

/* POSIX Extended Regular Expressions */
matcher_t matcher_egrep =
  {
    Ecompile,
    EGexecute,
    EGfree
  };

/* AWK Regular Expressions */
matcher_t matcher_awk =
  {
    AWKcompile,
    EGexecute,
    EGfree
  };
