/* Functions to make fuzzy comparisons between strings
   Copyright (C) 1988-1989, 1992-1993, 1995, 2001-2003, 2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


   Derived from GNU diff 2.7, analyze.c et al.

   The basic idea is to consider two strings as similar if, when
   transforming the first string into the second string through a
   sequence of edits (inserts and deletes of one character each),
   this sequence is short - or equivalently, if the ordered list
   of characters that are untouched by these edits is long.  For a
   good introduction to the subject, read about the "Levenshtein
   distance" in Wikipedia.

   The basic algorithm is described in:
   "An O(ND) Difference Algorithm and its Variations", Eugene Myers,
   Algorithmica Vol. 1 No. 2, 1986, pp. 251-266;
   see especially section 4.2, which describes the variation used below.

   The basic algorithm was independently discovered as described in:
   "Algorithms for Approximate String Matching", E. Ukkonen,
   Information and Control Vol. 64, 1985, pp. 100-118.

   Unless the 'minimal' flag is set, this code uses the TOO_EXPENSIVE
   heuristic, by Paul Eggert, to limit the cost to O(N**1.5 log N)
   at the price of producing suboptimal output for large inputs with
   many differences.

   Modified to work on strings rather than files
   by Peter Miller <pmiller@agso.gov.au>, October 1995 */

#include <config.h>

/* Specification.  */
#include "fstrcmp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "lock.h"
#include "tls.h"
#include "xalloc.h"

#ifndef uintptr_t
# define uintptr_t unsigned long
#endif


/*
 * Context of comparison operation.
 */
struct context
{
  /*
   * Data on one input string being compared.
   */
  struct string_data
  {
    /* The string to be compared. */
    const char *data;

    /* The length of the string to be compared. */
    int data_length;

    /* The number of characters inserted or deleted. */
    int edit_count;
  }
  string[2];

  #ifdef MINUS_H_FLAG

  /* This corresponds to the diff -H flag.  With this heuristic, for
     strings with a constant small density of changes, the algorithm is
     linear in the strings size.  This is unlikely in typical uses of
     fstrcmp, and so is usually compiled out.  Besides, there is no
     interface to set it true.  */
  int heuristic;

  #endif

  /* Vector, indexed by diagonal, containing 1 + the X coordinate of the
     point furthest along the given diagonal in the forward search of the
     edit matrix.  */
  int *fdiag;

  /* Vector, indexed by diagonal, containing the X coordinate of the point
     furthest along the given diagonal in the backward search of the edit
     matrix.  */
  int *bdiag;

  /* Edit scripts longer than this are too expensive to compute.  */
  int too_expensive;

  /* Snakes bigger than this are considered `big'.  */
  #define SNAKE_LIMIT	20
};

struct partition
{
  /* Midpoints of this partition.  */
  int xmid, ymid;

  /* Nonzero if low half will be analyzed minimally.  */
  int lo_minimal;

  /* Likewise for high half.  */
  int hi_minimal;
};


/* NAME
	diag - find diagonal path

   SYNOPSIS
	int diag(int xoff, int xlim, int yoff, int ylim, int minimal,
		 struct partition *part, struct context *ctxt);

   DESCRIPTION
	Find the midpoint of the shortest edit script for a specified
	portion of the two strings.

	Scan from the beginnings of the strings, and simultaneously from
	the ends, doing a breadth-first search through the space of
	edit-sequence.  When the two searches meet, we have found the
	midpoint of the shortest edit sequence.

	If MINIMAL is nonzero, find the minimal edit script regardless
	of expense.  Otherwise, if the search is too expensive, use
	heuristics to stop the search and report a suboptimal answer.

   RETURNS
	Set PART->(XMID,YMID) to the midpoint (XMID,YMID).  The diagonal
	number XMID - YMID equals the number of inserted characters
	minus the number of deleted characters (counting only characters
	before the midpoint).  Return the approximate edit cost; this is
	the total number of characters inserted or deleted (counting
	only characters before the midpoint), unless a heuristic is used
	to terminate the search prematurely.

	Set PART->LEFT_MINIMAL to nonzero iff the minimal edit script
	for the left half of the partition is known; similarly for
	PART->RIGHT_MINIMAL.

   CAVEAT
	This function assumes that the first characters of the specified
	portions of the two strings do not match, and likewise that the
	last characters do not match.  The caller must trim matching
	characters from the beginning and end of the portions it is
	going to specify.

	If we return the "wrong" partitions, the worst this can do is
	cause suboptimal diff output.  It cannot cause incorrect diff
	output.  */

static int
diag (int xoff, int xlim, int yoff, int ylim, int minimal,
      struct partition *part, struct context *ctxt)
{
  int *const fd = ctxt->fdiag;	/* Give the compiler a chance. */
  int *const bd = ctxt->bdiag;	/* Additional help for the compiler. */
  const char *const xv = ctxt->string[0].data;	/* Still more help for the compiler. */
  const char *const yv = ctxt->string[1].data;	/* And more and more . . . */
  const int dmin = xoff - ylim;	/* Minimum valid diagonal. */
  const int dmax = xlim - yoff;	/* Maximum valid diagonal. */
  const int fmid = xoff - yoff;	/* Center diagonal of top-down search. */
  const int bmid = xlim - ylim;	/* Center diagonal of bottom-up search. */
  int fmin = fmid;
  int fmax = fmid;		/* Limits of top-down search. */
  int bmin = bmid;
  int bmax = bmid;		/* Limits of bottom-up search. */
  int c;			/* Cost. */
  int odd = (fmid - bmid) & 1;

  /*
   * True if southeast corner is on an odd diagonal with respect
   * to the northwest.
   */
  fd[fmid] = xoff;
  bd[bmid] = xlim;
  for (c = 1;; ++c)
    {
      int d;			/* Active diagonal. */
      int big_snake;

      big_snake = 0;
      /* Extend the top-down search by an edit step in each diagonal. */
      if (fmin > dmin)
	fd[--fmin - 1] = -1;
      else
	++fmin;
      if (fmax < dmax)
	fd[++fmax + 1] = -1;
      else
	--fmax;
      for (d = fmax; d >= fmin; d -= 2)
	{
	  int x;
	  int y;
	  int oldx;
	  int tlo;
	  int thi;

	  tlo = fd[d - 1],
	    thi = fd[d + 1];

	  if (tlo >= thi)
	    x = tlo + 1;
	  else
	    x = thi;
	  oldx = x;
	  y = x - d;
	  while (x < xlim && y < ylim && xv[x] == yv[y])
	    {
	      ++x;
	      ++y;
	    }
	  if (x - oldx > SNAKE_LIMIT)
	    big_snake = 1;
	  fd[d] = x;
	  if (odd && bmin <= d && d <= bmax && bd[d] <= x)
	    {
	      part->xmid = x;
	      part->ymid = y;
	      part->lo_minimal = part->hi_minimal = 1;
	      return 2 * c - 1;
	    }
	}
      /* Similarly extend the bottom-up search.  */
      if (bmin > dmin)
	bd[--bmin - 1] = INT_MAX;
      else
	++bmin;
      if (bmax < dmax)
	bd[++bmax + 1] = INT_MAX;
      else
	--bmax;
      for (d = bmax; d >= bmin; d -= 2)
	{
	  int x;
	  int y;
	  int oldx;
	  int tlo;
	  int thi;

	  tlo = bd[d - 1],
	    thi = bd[d + 1];
	  if (tlo < thi)
	    x = tlo;
	  else
	    x = thi - 1;
	  oldx = x;
	  y = x - d;
	  while (x > xoff && y > yoff && xv[x - 1] == yv[y - 1])
	    {
	      --x;
	      --y;
	    }
	  if (oldx - x > SNAKE_LIMIT)
	    big_snake = 1;
	  bd[d] = x;
	  if (!odd && fmin <= d && d <= fmax && x <= fd[d])
	    {
	      part->xmid = x;
	      part->ymid = y;
	      part->lo_minimal = part->hi_minimal = 1;
	      return 2 * c;
	    }
	}

      if (minimal)
	continue;

#ifdef MINUS_H_FLAG
      /* Heuristic: check occasionally for a diagonal that has made lots
         of progress compared with the edit distance.  If we have any
         such, find the one that has made the most progress and return
         it as if it had succeeded.

         With this heuristic, for strings with a constant small density
         of changes, the algorithm is linear in the strings size.  */
      if (c > 200 && big_snake && ctxt->heuristic)
	{
	  int best;

	  best = 0;
	  for (d = fmax; d >= fmin; d -= 2)
	    {
	      int dd;
	      int x;
	      int y;
	      int v;

	      dd = d - fmid;
	      x = fd[d];
	      y = x - d;
	      v = (x - xoff) * 2 - dd;

	      if (v > 12 * (c + (dd < 0 ? -dd : dd)))
		{
		  if
		    (
		      v > best
		      &&
		      xoff + SNAKE_LIMIT <= x
		      &&
		      x < xlim
		      &&
		      yoff + SNAKE_LIMIT <= y
		      &&
		      y < ylim
		    )
		    {
		      /* We have a good enough best diagonal; now insist
			 that it end with a significant snake.  */
		      int k;

		      for (k = 1; xv[x - k] == yv[y - k]; k++)
			{
			  if (k == SNAKE_LIMIT)
			    {
			      best = v;
			      part->xmid = x;
			      part->ymid = y;
			      break;
			    }
			}
		    }
		}
	    }
	  if (best > 0)
	    {
	      part->lo_minimal = 1;
	      part->hi_minimal = 0;
	      return 2 * c - 1;
	    }
	  best = 0;
	  for (d = bmax; d >= bmin; d -= 2)
	    {
	      int dd;
	      int x;
	      int y;
	      int v;

	      dd = d - bmid;
	      x = bd[d];
	      y = x - d;
	      v = (xlim - x) * 2 + dd;

	      if (v > 12 * (c + (dd < 0 ? -dd : dd)))
		{
		  if (v > best && xoff < x && x <= xlim - SNAKE_LIMIT &&
		      yoff < y && y <= ylim - SNAKE_LIMIT)
		    {
		      /* We have a good enough best diagonal; now insist
			 that it end with a significant snake.  */
		      int k;

		      for (k = 0; xv[x + k] == yv[y + k]; k++)
			{
			  if (k == SNAKE_LIMIT - 1)
			    {
			      best = v;
			      part->xmid = x;
			      part->ymid = y;
			      break;
			    }
			}
		    }
		}
	    }
	  if (best > 0)
	    {
	      part->lo_minimal = 0;
	      part->hi_minimal = 1;
	      return 2 * c - 1;
	    }
	}
#endif /* MINUS_H_FLAG */

      /* Heuristic: if we've gone well beyond the call of duty, give up
	 and report halfway between our best results so far.  */
      if (c >= ctxt->too_expensive)
	{
	  int fxybest;
	  int fxbest;
	  int bxybest;
	  int bxbest;

	  /* Pacify `gcc -Wall'. */
	  fxbest = 0;
	  bxbest = 0;

	  /* Find forward diagonal that maximizes X + Y.  */
	  fxybest = -1;
	  for (d = fmax; d >= fmin; d -= 2)
	    {
	      int x;
	      int y;

	      x = fd[d] < xlim ? fd[d] : xlim;
	      y = x - d;

	      if (ylim < y)
		{
		  x = ylim + d;
		  y = ylim;
		}
	      if (fxybest < x + y)
		{
		  fxybest = x + y;
		  fxbest = x;
		}
	    }
	  /* Find backward diagonal that minimizes X + Y.  */
	  bxybest = INT_MAX;
	  for (d = bmax; d >= bmin; d -= 2)
	    {
	      int x;
	      int y;

	      x = xoff > bd[d] ? xoff : bd[d];
	      y = x - d;

	      if (y < yoff)
		{
		  x = yoff + d;
		  y = yoff;
		}
	      if (x + y < bxybest)
		{
		  bxybest = x + y;
		  bxbest = x;
		}
	    }
	  /* Use the better of the two diagonals.  */
	  if ((xlim + ylim) - bxybest < fxybest - (xoff + yoff))
	    {
	      part->xmid = fxbest;
	      part->ymid = fxybest - fxbest;
	      part->lo_minimal = 1;
	      part->hi_minimal = 0;
	    }
	  else
	    {
	      part->xmid = bxbest;
	      part->ymid = bxybest - bxbest;
	      part->lo_minimal = 0;
	      part->hi_minimal = 1;
	    }
	  return 2 * c - 1;
	}
    }
}


/* NAME
	compareseq - find edit sequence

   SYNOPSIS
	void compareseq(int xoff, int xlim, int yoff, int ylim, int minimal,
			struct context *ctxt);

   DESCRIPTION
	Compare in detail contiguous subsequences of the two strings
	which are known, as a whole, to match each other.

	The subsequence of string 0 is [XOFF, XLIM) and likewise for
	string 1.

	Note that XLIM, YLIM are exclusive bounds.  All character
	numbers are origin-0.

	If MINIMAL is nonzero, find a minimal difference no matter how
	expensive it is.  */

static void
compareseq (int xoff, int xlim, int yoff, int ylim, int minimal,
	    struct context *ctxt)
{
  const char *const xv = ctxt->string[0].data;	/* Help the compiler.  */
  const char *const yv = ctxt->string[1].data;

  /* Slide down the bottom initial diagonal. */
  while (xoff < xlim && yoff < ylim && xv[xoff] == yv[yoff])
    {
      ++xoff;
      ++yoff;
    }

  /* Slide up the top initial diagonal. */
  while (xlim > xoff && ylim > yoff && xv[xlim - 1] == yv[ylim - 1])
    {
      --xlim;
      --ylim;
    }

  /* Handle simple cases. */
  if (xoff == xlim)
    {
      while (yoff < ylim)
	{
	  ctxt->string[1].edit_count++;
	  ++yoff;
	}
    }
  else if (yoff == ylim)
    {
      while (xoff < xlim)
	{
	  ctxt->string[0].edit_count++;
	  ++xoff;
	}
    }
  else
    {
      int c;
      struct partition part;

      /* Find a point of correspondence in the middle of the strings.  */
      c = diag (xoff, xlim, yoff, ylim, minimal, &part, ctxt);
      if (c == 1)
	{
#if 0
	  /* This should be impossible, because it implies that one of
	     the two subsequences is empty, and that case was handled
	     above without calling `diag'.  Let's verify that this is
	     true.  */
	  abort ();
#else
	  /* The two subsequences differ by a single insert or delete;
	     record it and we are done.  */
	  if (part.xmid - part.ymid < xoff - yoff)
	    ctxt->string[1].edit_count++;
	  else
	    ctxt->string[0].edit_count++;
#endif
	}
      else
	{
	  /* Use the partitions to split this problem into subproblems.  */
	  compareseq (xoff, part.xmid, yoff, part.ymid, part.lo_minimal, ctxt);
	  compareseq (part.xmid, xlim, part.ymid, ylim, part.hi_minimal, ctxt);
	}
    }
}


/* Because fstrcmp is typically called multiple times, attempt to minimize
   the number of memory allocations performed.  Thus, let a call reuse the
   memory already allocated by the previous call, if it is sufficient.
   To make it multithread-safe, without need for a lock that protects the
   already allocated memory, store the allocated memory per thread.  Free
   it only when the thread exits.  */

static gl_tls_key_t buffer_key;	/* TLS key for a 'int *' */
static gl_tls_key_t bufmax_key;	/* TLS key for a 'size_t' */

static void
keys_init (void)
{
  gl_tls_key_init (buffer_key, free);
  gl_tls_key_init (bufmax_key, NULL);
  /* The per-thread initial values are NULL and 0, respectively.  */
}

/* Ensure that keys_init is called once only.  */
gl_once_define(static, keys_init_once)


/* NAME
	fstrcmp - fuzzy string compare

   SYNOPSIS
	double fstrcmp(const char *, const char *);

   DESCRIPTION
	The fstrcmp function may be used to compare two string for
	similarity.  It is very useful in reducing "cascade" or
	"secondary" errors in compilers or other situations where
	symbol tables occur.

   RETURNS
	double; 0 if the strings are entirly dissimilar, 1 if the
	strings are identical, and a number in between if they are
	similar.  */

double
fstrcmp (const char *string1, const char *string2)
{
  struct context ctxt;
  int i;

  size_t fdiag_len;
  int *buffer;
  size_t bufmax;

  /* set the info for each string.  */
  ctxt.string[0].data = string1;
  ctxt.string[0].data_length = strlen (string1);
  ctxt.string[1].data = string2;
  ctxt.string[1].data_length = strlen (string2);

  /* short-circuit obvious comparisons */
  if (ctxt.string[0].data_length == 0 && ctxt.string[1].data_length == 0)
    return 1.0;
  if (ctxt.string[0].data_length == 0 || ctxt.string[1].data_length == 0)
    return 0.0;

  /* Set TOO_EXPENSIVE to be approximate square root of input size,
     bounded below by 256.  */
  ctxt.too_expensive = 1;
  for (i = ctxt.string[0].data_length + ctxt.string[1].data_length;
       i != 0;
       i >>= 2)
    ctxt.too_expensive <<= 1;
  if (ctxt.too_expensive < 256)
    ctxt.too_expensive = 256;

  /* Allocate memory for fdiag and bdiag from a thread-local pool.  */
  fdiag_len = ctxt.string[0].data_length + ctxt.string[1].data_length + 3;
  gl_once (keys_init_once, keys_init);
  buffer = (int *) gl_tls_get (buffer_key);
  bufmax = (size_t) (uintptr_t) gl_tls_get (bufmax_key);
  if (fdiag_len > bufmax)
    {
      /* Need more memory.  */
      bufmax = 2 * bufmax;
      if (fdiag_len > bufmax)
	bufmax = fdiag_len;
      /* Calling xrealloc would be a waste: buffer's contents does not need
	 to be preserved.  */
      if (buffer != NULL)
	free (buffer);
      buffer = (int *) xmalloc (bufmax * (2 * sizeof (int)));
      gl_tls_set (buffer_key, buffer);
      gl_tls_set (bufmax_key, (void *) (uintptr_t) bufmax);
    }
  ctxt.fdiag = buffer + ctxt.string[1].data_length + 1;
  ctxt.bdiag = ctxt.fdiag + fdiag_len;

  /* Now do the main comparison algorithm */
  ctxt.string[0].edit_count = 0;
  ctxt.string[1].edit_count = 0;
  compareseq (0, ctxt.string[0].data_length, 0, ctxt.string[1].data_length, 0,
	      &ctxt);

  /* The result is
	((number of chars in common) / (average length of the strings)).
     This is admittedly biased towards finding that the strings are
     similar, however it does produce meaningful results.  */
  return ((double) (ctxt.string[0].data_length + ctxt.string[1].data_length
		    - ctxt.string[1].edit_count - ctxt.string[0].edit_count)
	  / (ctxt.string[0].data_length + ctxt.string[1].data_length));
}
