/* Find near-matches for identifiers.
   Copyright (C) 2015-2016 Free Software Foundation, Inc.

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

/* Calculate Levenshtein distance between two identifiers.  */

edit_distance_t
levenshtein_distance (tree ident_s, tree ident_t)
{
  gcc_assert (TREE_CODE (ident_s) == IDENTIFIER_NODE);
  gcc_assert (TREE_CODE (ident_t) == IDENTIFIER_NODE);

  return levenshtein_distance (IDENTIFIER_POINTER (ident_s),
			       IDENTIFIER_LENGTH (ident_s),
			       IDENTIFIER_POINTER (ident_t),
			       IDENTIFIER_LENGTH (ident_t));
}

/* Given TARGET, an identifier, and CANDIDATES, a vec of identifiers,
   determine which element within CANDIDATES has the lowest edit
   distance to TARGET.  If there are multiple elements with the
   same minimal distance, the first in the vector wins.

   If more than half of the letters were misspelled, the suggestion is
   likely to be meaningless, so return NULL_TREE for this case.  */

tree
find_closest_identifier (tree target, const auto_vec<tree> *candidates)
{
  gcc_assert (TREE_CODE (target) == IDENTIFIER_NODE);

  int i;
  tree identifier;
  tree best_identifier = NULL_TREE;
  edit_distance_t best_distance = MAX_EDIT_DISTANCE;
  FOR_EACH_VEC_ELT (*candidates, i, identifier)
    {
      gcc_assert (TREE_CODE (identifier) == IDENTIFIER_NODE);
      edit_distance_t dist = levenshtein_distance (target, identifier);
      if (dist < best_distance)
	{
	  best_distance = dist;
	  best_identifier = identifier;
	}
    }

  /* If more than half of the letters were misspelled, the suggestion is
     likely to be meaningless.  */
  if (best_identifier)
    {
      unsigned int cutoff = MAX (IDENTIFIER_LENGTH (target),
				 IDENTIFIER_LENGTH (best_identifier)) / 2;
      if (best_distance > cutoff)
	return NULL_TREE;
    }

  return best_identifier;
}
