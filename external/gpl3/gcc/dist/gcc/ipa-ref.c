/* Interprocedural reference lists.
   Copyright (C) 2010-2013 Free Software Foundation, Inc.
   Contributed by Jan Hubicka

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
#include "tree.h"
#include "ggc.h"
#include "target.h"
#include "cgraph.h"

static const char *ipa_ref_use_name[] = {"read","write","addr","alias"};

/* Return ipa reference from REFERING_NODE or REFERING_VARPOOL_NODE
   to REFERED_NODE or REFERED_VARPOOL_NODE. USE_TYPE specify type
   of the use and STMT the statement (if it exists).  */

struct ipa_ref *
ipa_record_reference (symtab_node referring_node,
		      symtab_node referred_node,
		      enum ipa_ref_use use_type, gimple stmt)
{
  struct ipa_ref *ref;
  struct ipa_ref_list *list, *list2;
  ipa_ref_t *old_references;

  gcc_checking_assert (!stmt || is_a <cgraph_node> (referring_node));
  gcc_checking_assert (use_type != IPA_REF_ALIAS || !stmt);

  list = &referring_node->symbol.ref_list;
  old_references = vec_safe_address (list->references);
  vec_safe_grow (list->references, vec_safe_length (list->references) + 1);
  ref = &list->references->last ();

  list2 = &referred_node->symbol.ref_list;
  list2->referring.safe_push (ref);
  ref->referred_index = list2->referring.length () - 1;
  ref->referring = referring_node;
  ref->referred = referred_node;
  ref->stmt = stmt;
  ref->use = use_type;

  /* If vector was moved in memory, update pointers.  */
  if (old_references != list->references->address ())
    {
      int i;
      for (i = 0; ipa_ref_list_reference_iterate (list, i, ref); i++)
	ipa_ref_referred_ref_list (ref)->referring[ref->referred_index] = ref;
    }
  return ref;
}

/* Remove reference REF.  */

void
ipa_remove_reference (struct ipa_ref *ref)
{
  struct ipa_ref_list *list = ipa_ref_referred_ref_list (ref);
  struct ipa_ref_list *list2 = ipa_ref_referring_ref_list (ref);
  vec<ipa_ref_t, va_gc> *old_references = list2->references;
  struct ipa_ref *last;

  gcc_assert (list->referring[ref->referred_index] == ref);
  last = list->referring.last ();
  if (ref != last)
    {
      list->referring[ref->referred_index] = list->referring.last ();
      list->referring[ref->referred_index]->referred_index 
	  = ref->referred_index;
    }
  list->referring.pop ();

  last = &list2->references->last ();
  if (ref != last)
    {
      *ref = *last;
      ipa_ref_referred_ref_list (ref)->referring[ref->referred_index] = ref;
    }
  list2->references->pop ();
  gcc_assert (list2->references == old_references);
}

/* Remove all references in ref list LIST.  */

void
ipa_remove_all_references (struct ipa_ref_list *list)
{
  while (vec_safe_length (list->references))
    ipa_remove_reference (&list->references->last ());
  vec_free (list->references);
}

/* Remove all references in ref list LIST.  */

void
ipa_remove_all_referring (struct ipa_ref_list *list)
{
  while (list->referring.length ())
    ipa_remove_reference (list->referring.last ());
  list->referring.release ();
}

/* Dump references in LIST to FILE.  */

void
ipa_dump_references (FILE * file, struct ipa_ref_list *list)
{
  struct ipa_ref *ref;
  int i;
  for (i = 0; ipa_ref_list_reference_iterate (list, i, ref); i++)
    {
      fprintf (file, "%s/%i (%s)",
               symtab_node_asm_name (ref->referred),
               ref->referred->symbol.order,
	       ipa_ref_use_name [ref->use]);
    }
  fprintf (file, "\n");
}

/* Dump referring in LIST to FILE.  */

void
ipa_dump_referring (FILE * file, struct ipa_ref_list *list)
{
  struct ipa_ref *ref;
  int i;
  for (i = 0; ipa_ref_list_referring_iterate (list, i, ref); i++)
    {
      fprintf (file, "%s/%i (%s)",
               symtab_node_asm_name (ref->referring),
               ref->referring->symbol.order,
	       ipa_ref_use_name [ref->use]);
    }
  fprintf (file, "\n");
}

/* Clone all references from SRC to DEST_NODE or DEST_VARPOOL_NODE.  */

void
ipa_clone_references (symtab_node dest_node,
		      struct ipa_ref_list *src)
{
  struct ipa_ref *ref;
  int i;
  for (i = 0; ipa_ref_list_reference_iterate (src, i, ref); i++)
    ipa_record_reference (dest_node,
			  ref->referred,
			  ref->use, ref->stmt);
}

/* Clone all referring from SRC to DEST_NODE or DEST_VARPOOL_NODE.  */

void
ipa_clone_referring (symtab_node dest_node,
		    struct ipa_ref_list *src)
{
  struct ipa_ref *ref;
  int i;
  for (i = 0; ipa_ref_list_referring_iterate (src, i, ref); i++)
    ipa_record_reference (ref->referring,
			  dest_node,
			  ref->use, ref->stmt);
}

/* Return true when execution of REF can lead to return from
   function. */
bool
ipa_ref_cannot_lead_to_return (struct ipa_ref *ref)
{
  return cgraph_node_cannot_return (ipa_ref_referring_node (ref));
}

/* Return true if list contains an alias.  */
bool
ipa_ref_has_aliases_p (struct ipa_ref_list *ref_list)
{
  struct ipa_ref *ref;
  int i;
  for (i = 0; ipa_ref_list_referring_iterate (ref_list, i, ref); i++)
    if (ref->use == IPA_REF_ALIAS)
      return true;
  return false;
}
