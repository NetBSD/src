/* Find near-matches for strings.
   Copyright (C) 2015-2018 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "tree.h"
#include "spellcheck.h"
#include "selftest.h"

/* The Levenshtein distance is an "edit-distance": the minimal
   number of one-character insertions, removals or substitutions
   that are needed to change one string into another.

   This implementation uses the Wagner-Fischer algorithm.  */

edit_distance_t
levenshtein_distance (const char *s, int len_s,
		      const char *t, int len_t)
{
  const bool debug = false;

  if (debug)
    {
      printf ("s: \"%s\" (len_s=%i)\n", s, len_s);
      printf ("t: \"%s\" (len_t=%i)\n", t, len_t);
    }

  if (len_s == 0)
    return len_t;
  if (len_t == 0)
    return len_s;

  /* We effectively build a matrix where each (i, j) contains the
     Levenshtein distance between the prefix strings s[0:j]
     and t[0:i].
     Rather than actually build an (len_t + 1) * (len_s + 1) matrix,
     we simply keep track of the last row, v0 and a new row, v1,
     which avoids an (len_t + 1) * (len_s + 1) allocation and memory accesses
     in favor of two (len_s + 1) allocations.  These could potentially be
     statically-allocated if we impose a maximum length on the
     strings of interest.  */
  edit_distance_t *v0 = new edit_distance_t[len_s + 1];
  edit_distance_t *v1 = new edit_distance_t[len_s + 1];

  /* The first row is for the case of an empty target string, which
     we can reach by deleting every character in the source string.  */
  for (int i = 0; i < len_s + 1; i++)
    v0[i] = i;

  /* Build successive rows.  */
  for (int i = 0; i < len_t; i++)
    {
      if (debug)
	{
	  printf ("i:%i v0 = ", i);
	  for (int j = 0; j < len_s + 1; j++)
	    printf ("%i ", v0[j]);
	  printf ("\n");
	}

      /* The initial column is for the case of an empty source string; we
	 can reach prefixes of the target string of length i
	 by inserting i characters.  */
      v1[0] = i + 1;

      /* Build the rest of the row by considering neighbors to
	 the north, west and northwest.  */
      for (int j = 0; j < len_s; j++)
	{
	  edit_distance_t cost = (s[j] == t[i] ? 0 : 1);
	  edit_distance_t deletion     = v1[j] + 1;
	  edit_distance_t insertion    = v0[j + 1] + 1;
	  edit_distance_t substitution = v0[j] + cost;
	  edit_distance_t cheapest = MIN (deletion, insertion);
	  cheapest = MIN (cheapest, substitution);
	  v1[j + 1] = cheapest;
	}

      /* Prepare to move on to next row.  */
      for (int j = 0; j < len_s + 1; j++)
	v0[j] = v1[j];
    }

  if (debug)
    {
      printf ("final v1 = ");
      for (int j = 0; j < len_s + 1; j++)
	printf ("%i ", v1[j]);
      printf ("\n");
    }

  edit_distance_t result = v1[len_s];
  delete[] v0;
  delete[] v1;
  return result;
}

/* Calculate Levenshtein distance between two nil-terminated strings.  */

edit_distance_t
levenshtein_distance (const char *s, const char *t)
{
  return levenshtein_distance (s, strlen (s), t, strlen (t));
}

/* Given TARGET, a non-NULL string, and CANDIDATES, a non-NULL ptr to
   an autovec of non-NULL strings, determine which element within
   CANDIDATES has the lowest edit distance to TARGET.  If there are
   multiple elements with the same minimal distance, the first in the
   vector wins.

   If more than half of the letters were misspelled, the suggestion is
   likely to be meaningless, so return NULL for this case.  */

const char *
find_closest_string (const char *target,
		     const auto_vec<const char *> *candidates)
{
  gcc_assert (target);
  gcc_assert (candidates);

  int i;
  const char *candidate;
  best_match<const char *, const char *> bm (target);
  FOR_EACH_VEC_ELT (*candidates, i, candidate)
    {
      gcc_assert (candidate);
      bm.consider (candidate);
    }

  return bm.get_best_meaningful_candidate ();
}

#if CHECKING_P

namespace selftest {

/* Selftests.  */

/* Verify that the levenshtein_distance (A, B) equals the expected
   value.  */

static void
levenshtein_distance_unit_test_oneway (const char *a, const char *b,
				       edit_distance_t expected)
{
  edit_distance_t actual = levenshtein_distance (a, b);
  ASSERT_EQ (actual, expected);
}

/* Verify that both
     levenshtein_distance (A, B)
   and
     levenshtein_distance (B, A)
   equal the expected value, to ensure that the function is symmetric.  */

static void
levenshtein_distance_unit_test (const char *a, const char *b,
				edit_distance_t expected)
{
  levenshtein_distance_unit_test_oneway (a, b, expected);
  levenshtein_distance_unit_test_oneway (b, a, expected);
}

/* Verify that find_closest_string is sane.  */

static void
test_find_closest_string ()
{
  auto_vec<const char *> candidates;

  /* Verify that it can handle an empty vec.  */
  ASSERT_EQ (NULL, find_closest_string ("", &candidates));

  /* Verify that it works sanely for non-empty vecs.  */
  candidates.safe_push ("apple");
  candidates.safe_push ("banana");
  candidates.safe_push ("cherry");

  ASSERT_STREQ ("apple", find_closest_string ("app", &candidates));
  ASSERT_STREQ ("banana", find_closest_string ("banyan", &candidates));
  ASSERT_STREQ ("cherry", find_closest_string ("berry", &candidates));
  ASSERT_EQ (NULL, find_closest_string ("not like the others", &candidates));

  /* The order of the vec can matter, but it should not matter for these
     inputs.  */
  candidates.truncate (0);
  candidates.safe_push ("cherry");
  candidates.safe_push ("banana");
  candidates.safe_push ("apple");
  ASSERT_STREQ ("apple", find_closest_string ("app", &candidates));
  ASSERT_STREQ ("banana", find_closest_string ("banyan", &candidates));
  ASSERT_STREQ ("cherry", find_closest_string ("berry", &candidates));
  ASSERT_EQ (NULL, find_closest_string ("not like the others", &candidates));

  /* If the goal string somehow makes it into the candidate list, offering
     it as a suggestion will be nonsensical.  Verify that we don't offer such
     suggestions.  */
  ASSERT_EQ (NULL, find_closest_string ("banana", &candidates));
}

/* Test data for test_metric_conditions.  */

static const char * const test_data[] = {
  "",
  "foo",
  "food",
  "boo",
  "1234567890123456789012345678901234567890123456789012345678901234567890"
};

/* Verify that levenshtein_distance appears to be a sane distance function,
   i.e. the conditions for being a metric.  This is done directly for a
   small set of examples, using test_data above.  This is O(N^3) in the size
   of the array, due to the test for the triangle inequality, so we keep the
   array small.  */

static void
test_metric_conditions ()
{
  const int num_test_cases = sizeof (test_data) / sizeof (test_data[0]);

  for (int i = 0; i < num_test_cases; i++)
    {
      for (int j = 0; j < num_test_cases; j++)
	{
	  edit_distance_t dist_ij
	    = levenshtein_distance (test_data[i], test_data[j]);

	  /* Identity of indiscernibles: d(i, j) > 0 iff i == j.  */
	  if (i == j)
	    ASSERT_EQ (dist_ij, 0);
	  else
	    ASSERT_TRUE (dist_ij > 0);

	  /* Symmetry: d(i, j) == d(j, i).  */
	  edit_distance_t dist_ji
	    = levenshtein_distance (test_data[j], test_data[i]);
	  ASSERT_EQ (dist_ij, dist_ji);

	  /* Triangle inequality.  */
	  for (int k = 0; k < num_test_cases; k++)
	    {
	      edit_distance_t dist_ik
		= levenshtein_distance (test_data[i], test_data[k]);
	      edit_distance_t dist_jk
		= levenshtein_distance (test_data[j], test_data[k]);
	      ASSERT_TRUE (dist_ik <= dist_ij + dist_jk);
	    }
	}
    }
}

/* Verify levenshtein_distance for a variety of pairs of pre-canned
   inputs, comparing against known-good values.  */

void
spellcheck_c_tests ()
{
  levenshtein_distance_unit_test ("", "nonempty", strlen ("nonempty"));
  levenshtein_distance_unit_test ("saturday", "sunday", 3);
  levenshtein_distance_unit_test ("foo", "m_foo", 2);
  levenshtein_distance_unit_test ("hello_world", "HelloWorld", 3);
  levenshtein_distance_unit_test
    ("the quick brown fox jumps over the lazy dog", "dog", 40);
  levenshtein_distance_unit_test
    ("the quick brown fox jumps over the lazy dog",
     "the quick brown dog jumps over the lazy fox",
     4);
  levenshtein_distance_unit_test
    ("Lorem ipsum dolor sit amet, consectetur adipiscing elit,",
     "All your base are belong to us",
     44);
  levenshtein_distance_unit_test ("foo", "FOO", 3);

  test_find_closest_string ();
  test_metric_conditions ();
}

} // namespace selftest

#endif /* #if CHECKING_P */
