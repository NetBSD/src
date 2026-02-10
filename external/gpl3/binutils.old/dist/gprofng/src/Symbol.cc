/* Copyright (C) 2025 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "config.h"

#include "DbeSession.h"
#include "Function.h"
#include "LoadObject.h"
#include "Module.h"
#include "Symbol.h"


template<> void Vector<Symbol *>::dump (const char *msg)
{
  if (msg == NULL)
    msg = "#";
  Dprintf (1, NTXT ("\n%s Vector<Symbol *> [%lld]\n"), msg, (long long) size ());
  if (size () > 0)
    Dprintf (1, "        value  | img_offset |  size | flags|local_ind|\n");
  for (long i = 0, sz = size (); i < sz; i++)
    {
      Symbol *sp = get (i);
      Dprintf (1, "  %3ld ", i);
      sp->dump ();
    }
  if (size () > 0)
    Dprintf (1, "===== END of Symbol::dump: %s =========\n\n", msg);
}

void
Symbol::dump (const char *msg)
{
  if (msg)
    Dprintf (1, "%s\n", msg);
  Dprintf (1, "%8lld |%11lld |%6d |%5d |%8d |%s\n", (long long) value,
	  (long long) img_offset, (int) size, flags, local_ind,
	  name ? name : "NULL");
}

Symbol::Symbol (Vector<Symbol*> *vec)
{
  func = NULL;
  lang_code = Sp_lang_unknown;
  value = 0;
  save = 0;
  size = 0;
  img_offset = 0;
  name = NULL;
  alias = NULL;
  local_ind = -1;
  flags = 0;
  defined = false;
  if (vec)
    vec->append (this);
}

Symbol::~Symbol ()
{
  free (name);
}

static int
cmpSym (const void *a, const void *b)
{
  Symbol *item1 = *((Symbol **) a);
  Symbol *item2 = *((Symbol **) b);
  return (item1->value > item2->value) ? 1 :
	  (item1->value == item2->value) ? 0 : -1;
}

static int
cmpSymName (const void *a, const void *b)
{
  Symbol *item1 = *((Symbol **) a);
  Symbol *item2 = *((Symbol **) b);
  return strcmp (item1->name, item2->name);
}

Symbol *
Symbol::get_symbol (Vector<Symbol*> *syms, uint64_t pc)
{
  if (syms != NULL && pc != 0)
    {
      Symbol *sp = new Symbol;
      sp->value = pc;
      long i = syms->bisearch (0, -1, &sp, cmpSym);
      delete sp;
      if (i != -1)
	return syms->get (i)->cardinal ();
    }
  return NULL;
}

Symbol *
Symbol::get_symbol (Vector<Symbol*> *syms, char *linker_name)
{
  if (syms != NULL && linker_name != NULL)
    {
      Symbol *sp = new Symbol;
      sp->name = linker_name;
      long i = syms->bisearch (0, -1, &sp, cmpSymName);
      sp->name = NULL;
      delete sp;
      if (i != -1)
	return syms->get (i)->cardinal ();
    }
  return NULL;
}

Vector<Symbol *> *
Symbol::sort_by_name (Vector<Symbol *> *syms)
{
  if (VecSize (syms) == 0)
    return NULL;
  Vector<Symbol *> *symbols = syms->copy ();
  symbols->sort (cmpSymName);
  return symbols;
}

Vector<Symbol *> *
Symbol::find_symbols (Vector<Symbol*> *syms, Vector<Range *> *ranges,
		      Vector<Symbol *> *symbols)
{
  // 'syms' and 'ranges' must already be sorted.
  // return symbols matched by 'ranges'
  if (VecSize (syms) == 0 || VecSize (ranges) == 0)
    return NULL;

  // Use binary search to find a suitable index in 'syms'
  int ind = 0;
  uint64_t addr = ranges->get (0)->low;
  for (int lo = 0, hi = syms->size (); lo < hi;)
    {
      int mid = (hi + lo) >> 1;
      Symbol *sym = syms->get (mid);
      if (sym->value == addr)
	{
	  ind = mid;
	  break;
	}
      else if (sym->value > addr)
	hi = mid - 1;
      else
	{
	  ind = mid;
	  lo = mid + 1;
	}
    }

  for (int i = 0, r_sz = ranges->size (), sz = syms->size (); ind < sz; ind++)
    {
      Symbol *sym = syms->get (ind);
      while (i < r_sz)
	{
	  Range *r = ranges->get (i);
	  if (sym->value < r->low)
	    break;
	  if (sym->value <= r->high)
	    {
	      symbols->append (sym);
	      break;
	    }
	  i++;
	}
      if (i >= r_sz)
	break;
    }
  return symbols;
}

/* Create and append a new function to the 'module'.
 * Copy attributes (size, name, etc) from Simbol,  */
Function *
Symbol::createFunction (Module *module)
{
  if (func)
    return func;
  func = dbeSession->createFunction ();
  func->img_fname = module->file_name;
  func->img_offset = img_offset;
  func->save_addr = save;
  func->size = size;
  func->module = module;
  func->set_name (name);
  module->functions->append (func);
  module->loadobject->functions->append (func);
  return func;
}

template<> void Vector<Range *>::dump (const char *msg)
{
  Dprintf (1, NTXT ("%s Vector<Range *> [%lld]\n"),
	   msg ? msg : "#", (long long) size ());
  for (long i = 0, sz = size (); i < sz; i++)
    {
      Range *p = get (i);
      Dprintf (1, "%3ld 0x%08llx 0x%08llx  (%lld - %lld)\n", i,
	       (long long) p->low, (long long) p->high,
	       (long long) p->low, (long long) p->high);
    }
}