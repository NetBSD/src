/*
 * Copyright (c) 1983, 1993, 2001
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "util.h"
#include "bfd.h"
#include "gp-gmon.h"

#include "symtab.h"
#include "cg_arcs.h"

Sym *cycle_header;
unsigned int num_cycles;
Arc **arcs;
unsigned int numarcs;

/*
 * Return TRUE iff PARENT has an arc to covers the address
 * range covered by CHILD.
 */
Arc *
arc_lookup (Sym *parent, Sym *child)
{
  Arc *arc;

  if (!parent || !child)
    {
      printf ("[arc_lookup] parent == 0 || child == 0\n");
      return 0;
    }
  DBG (LOOKUPDEBUG, printf ("[arc_lookup] parent %s child %s\n",
			    parent->name, child->name));
  for (arc = parent->cg.children; arc; arc = arc->next_child)
    {
      DBG (LOOKUPDEBUG, printf ("[arc_lookup]\t parent %s child %s\n",
				arc->parent->name, arc->child->name));
      if (child->addr >= arc->child->addr
	  && child->end_addr <= arc->child->end_addr)
	{
	  return arc;
	}
    }
  return 0;
}


/*
 * Add (or just increment) an arc:
 */
void
arc_add (Sym *parent, Sym *child, unsigned long count)
{
  static unsigned int maxarcs = 0;
  Arc *arc;

  DBG (TALLYDEBUG, printf ("[arc_add] %lu arcs from %s to %s\n",
			   count, parent->name, child->name));
  arc = arc_lookup (parent, child);
  if (arc)
    {
      /*
       * A hit: just increment the count.
       */
      DBG (TALLYDEBUG, printf ("[tally] hit %lu += %lu\n",
			       arc->count, count));
      arc->count += count;
      return;
    }
  arc = (Arc *) xmalloc (sizeof (*arc));
  memset (arc, 0, sizeof (*arc));
  arc->parent = parent;
  arc->child = child;
  arc->count = count;

  /* If this isn't an arc for a recursive call to parent, then add it
     to the array of arcs.  */
  if (parent != child)
    {
      /* If we've exhausted space in our current array, get a new one
	 and copy the contents.   We might want to throttle the doubling
	 factor one day.  */
      if (numarcs == maxarcs)
	{
	  /* Determine how much space we want to allocate.  */
	  if (maxarcs == 0)
	    maxarcs = 1;
	  maxarcs *= 2;

	  arcs = (Arc**) xrealloc (arcs, sizeof (*arcs) * maxarcs);
	}

      /* Place this arc in the arc array.  */
      arcs[numarcs++] = arc;
    }

  /* prepend this child to the children of this parent: */
  arc->next_child = parent->cg.children;
  parent->cg.children = arc;

  /* prepend this parent to the parents of this child: */
  arc->next_parent = child->cg.parents;
  child->cg.parents = arc;
}
