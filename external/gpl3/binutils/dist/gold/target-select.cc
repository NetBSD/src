// target-select.cc -- select a target for an object file

// Copyright 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#include <cstring>

#include "elfcpp.h"
#include "target-select.h"

namespace
{

// The start of the list of target selectors.

gold::Target_selector* target_selectors;

} // End anonymous namespace.

namespace gold
{

// Class Set_target_once.

void
Set_target_once::do_run_once(void*)
{
  this->target_selector_->set_target();
}

// Construct a Target_selector, which means adding it to the linked
// list.  This runs at global constructor time, so we want it to be
// fast.

Target_selector::Target_selector(int machine, int size, bool is_big_endian,
				 const char* bfd_name)
  : machine_(machine), size_(size), is_big_endian_(is_big_endian),
    bfd_name_(bfd_name), instantiated_target_(NULL), set_target_once_(this)
{
  this->next_ = target_selectors;
  target_selectors = this;
}

// Instantiate the target and return it.  Use SET_TARGET_ONCE_ to
// avoid instantiating two instances of the same target.

Target*
Target_selector::instantiate_target()
{
  this->set_target_once_.run_once(NULL);
  return this->instantiated_target_;
}

// Instantiate the target.  This is called at most once.

void
Target_selector::set_target()
{
  gold_assert(this->instantiated_target_ == NULL);
  this->instantiated_target_ = this->do_instantiate_target();
}

// Find the target for an ELF file.

Target*
select_target(int machine, int size, bool is_big_endian, int osabi,
	      int abiversion)
{
  for (Target_selector* p = target_selectors; p != NULL; p = p->next())
    {
      int pmach = p->machine();
      if ((pmach == machine || pmach == elfcpp::EM_NONE)
	  && p->get_size() == size
	  && (p->is_big_endian() ? is_big_endian : !is_big_endian))
	{
	  Target* ret = p->recognize(machine, osabi, abiversion);
	  if (ret != NULL)
	    return ret;
	}
    }
  return NULL;
}

// Find a target using a BFD name.  This is used to support the
// --oformat option.

Target*
select_target_by_name(const char* name)
{
  for (Target_selector* p = target_selectors; p != NULL; p = p->next())
    {
      const char* pname = p->bfd_name();
      if (pname == NULL || strcmp(pname, name) == 0)
	{
	  Target* ret = p->recognize_by_name(name);
	  if (ret != NULL)
	    return ret;
	}
    }
  return NULL;
}

// Push all the supported BFD names onto a vector.

void
supported_target_names(std::vector<const char*>* names)
{
  for (Target_selector* p = target_selectors; p != NULL; p = p->next())
    p->supported_names(names);
}

} // End namespace gold.
