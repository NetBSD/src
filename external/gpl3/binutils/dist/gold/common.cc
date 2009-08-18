// common.cc -- handle common symbols for gold

// Copyright 2006, 2007, 2008 Free Software Foundation, Inc.
// Written by Ian Lance Taylor <iant@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

#include "gold.h"

#include <algorithm>

#include "workqueue.h"
#include "mapfile.h"
#include "layout.h"
#include "output.h"
#include "symtab.h"
#include "common.h"

namespace gold
{

// Allocate_commons_task methods.

// This task allocates the common symbols.  We need a lock on the
// symbol table.

Task_token*
Allocate_commons_task::is_runnable()
{
  if (!this->symtab_lock_->is_writable())
    return this->symtab_lock_;
  return NULL;
}

// Return the locks we hold: one on the symbol table, and one blocker.

void
Allocate_commons_task::locks(Task_locker* tl)
{
  tl->add(this, this->blocker_);
  tl->add(this, this->symtab_lock_);
}

// Allocate the common symbols.

void
Allocate_commons_task::run(Workqueue*)
{
  this->symtab_->allocate_commons(this->layout_, this->mapfile_);
}

// This class is used to sort the common symbol by size.  We put the
// larger common symbols first.

template<int size>
class Sort_commons
{
 public:
  Sort_commons(const Symbol_table* symtab)
    : symtab_(symtab)
  { }

  bool operator()(const Symbol* a, const Symbol* b) const;

 private:
  const Symbol_table* symtab_;
};

template<int size>
bool
Sort_commons<size>::operator()(const Symbol* pa, const Symbol* pb) const
{
  if (pa == NULL)
    return false;
  if (pb == NULL)
    return true;

  const Symbol_table* symtab = this->symtab_;
  const Sized_symbol<size>* psa = symtab->get_sized_symbol<size>(pa);
  const Sized_symbol<size>* psb = symtab->get_sized_symbol<size>(pb);

  // Sort by largest size first.
  typename Sized_symbol<size>::Size_type sa = psa->symsize();
  typename Sized_symbol<size>::Size_type sb = psb->symsize();
  if (sa < sb)
    return false;
  else if (sb < sa)
    return true;

  // When the symbols are the same size, we sort them by alignment,
  // largest alignment first.
  typename Sized_symbol<size>::Value_type va = psa->value();
  typename Sized_symbol<size>::Value_type vb = psb->value();
  if (va < vb)
    return false;
  else if (vb < va)
    return true;

  // Otherwise we stabilize the sort by sorting by name.
  return strcmp(psa->name(), psb->name()) < 0;
}

// Allocate the common symbols.

void
Symbol_table::allocate_commons(Layout* layout, Mapfile* mapfile)
{
  if (parameters->target().get_size() == 32)
    {
#if defined(HAVE_TARGET_32_LITTLE) || defined(HAVE_TARGET_32_BIG)
      this->do_allocate_commons<32>(layout, mapfile);
#else
      gold_unreachable();
#endif
    }
  else if (parameters->target().get_size() == 64)
    {
#if defined(HAVE_TARGET_64_LITTLE) || defined(HAVE_TARGET_64_BIG)
      this->do_allocate_commons<64>(layout, mapfile);
#else
      gold_unreachable();
#endif
    }
  else
    gold_unreachable();
}

// Allocated the common symbols, sized version.

template<int size>
void
Symbol_table::do_allocate_commons(Layout* layout, Mapfile* mapfile)
{
  this->do_allocate_commons_list<size>(layout, false, &this->commons_,
				       mapfile);
  this->do_allocate_commons_list<size>(layout, true, &this->tls_commons_,
				       mapfile);
}

// Allocate the common symbols in a list.  IS_TLS indicates whether
// these are TLS common symbols.

template<int size>
void
Symbol_table::do_allocate_commons_list(Layout* layout, bool is_tls,
				       Commons_type* commons,
				       Mapfile* mapfile)
{
  typedef typename Sized_symbol<size>::Value_type Value_type;
  typedef typename Sized_symbol<size>::Size_type Size_type;

  // We've kept a list of all the common symbols.  But the symbol may
  // have been resolved to a defined symbol by now.  And it may be a
  // forwarder.  First remove all non-common symbols.
  bool any = false;
  uint64_t addralign = 0;
  for (Commons_type::iterator p = commons->begin();
       p != commons->end();
       ++p)
    {
      Symbol* sym = *p;
      if (sym->is_forwarder())
	{
	  sym = this->resolve_forwards(sym);
	  *p = sym;
	}
      if (!sym->is_common())
	*p = NULL;
      else
	{
	  any = true;
	  Sized_symbol<size>* ssym = this->get_sized_symbol<size>(sym);
	  if (ssym->value() > addralign)
	    addralign = ssym->value();
	}
    }
  if (!any)
    return;

  // Sort the common symbols by size, so that they pack better into
  // memory.
  std::sort(commons->begin(), commons->end(),
	    Sort_commons<size>(this));

  // Place them in a newly allocated BSS section.

  Output_data_space *poc = new Output_data_space(addralign,
						 (is_tls
						  ? "** tls common"
						  : "** common"));

  const char* name = ".bss";
  elfcpp::Elf_Xword flags = elfcpp::SHF_WRITE | elfcpp::SHF_ALLOC;
  if (is_tls)
    {
      name = ".tbss";
      flags |= elfcpp::SHF_TLS;
    }
  layout->add_output_section_data(name, elfcpp::SHT_NOBITS, flags, poc);

  // Allocate them all.

  off_t off = 0;
  for (Commons_type::iterator p = commons->begin();
       p != commons->end();
       ++p)
    {
      Symbol* sym = *p;
      if (sym == NULL)
	break;
      Sized_symbol<size>* ssym = this->get_sized_symbol<size>(sym);

      // Record the symbol in the map file now, before we change its
      // value.  Pass the size in separately so that we don't have to
      // templatize the map code, which is not performance sensitive.
      if (mapfile != NULL)
	mapfile->report_allocate_common(sym, ssym->symsize());

      off = align_address(off, ssym->value());
      ssym->allocate_common(poc, off);
      off += ssym->symsize();
    }

  poc->set_current_data_size(off);

  commons->clear();
}

} // End namespace gold.
