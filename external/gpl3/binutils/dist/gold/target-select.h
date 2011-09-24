// target-select.h -- select a target for an object file  -*- C++ -*-

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

#ifndef GOLD_TARGET_SELECT_H
#define GOLD_TARGET_SELECT_H

#include <vector>

#include "gold-threads.h"

namespace gold
{

class Target;
class Target_selector;

// Used to set the target only once.

class Set_target_once : public Once
{
 public:
  Set_target_once(Target_selector* target_selector)
    : target_selector_(target_selector)
  { }

 protected:
  void
  do_run_once(void*);

 private:
  Target_selector* target_selector_;
};

// We want to avoid a master list of targets, which implies using a
// global constructor.  And we also want the program to start up as
// quickly as possible, which implies avoiding global constructors.
// We compromise on a very simple global constructor.  We use a target
// selector, which specifies an ELF machine number and a recognition
// function.  We use global constructors to build a linked list of
// target selectors--a simple pointer list, not a std::list.

class Target_selector
{
 public:
  // Create a target selector for a specific machine number, size (32
  // or 64), and endianness.  The machine number can be EM_NONE to
  // test for any machine number.  BFD_NAME is the name of the target
  // used by the GNU linker, for backward compatibility; it may be
  // NULL.
  Target_selector(int machine, int size, bool is_big_endian,
		  const char* bfd_name);

  virtual ~Target_selector()
  { }

  // If we can handle this target, return a pointer to a target
  // structure.  The size and endianness are known.
  Target*
  recognize(int machine, int osabi, int abiversion)
  { return this->do_recognize(machine, osabi, abiversion); }

  // If NAME matches the target, return a pointer to a target
  // structure.
  Target*
  recognize_by_name(const char* name)
  { return this->do_recognize_by_name(name); }

  // Push all supported names onto the vector.  This is only used for
  // help output.
  void
  supported_names(std::vector<const char*>* names)
  { this->do_supported_names(names); }

  // Return the next Target_selector in the linked list.
  Target_selector*
  next() const
  { return this->next_; }

  // Return the machine number this selector is looking for.  This can
  // be EM_NONE to match any machine number, in which case the
  // do_recognize hook will be responsible for matching the machine
  // number.
  int
  machine() const
  { return this->machine_; }

  // Return the size this is looking for (32 or 64).
  int
  get_size() const
  { return this->size_; }

  // Return the endianness this is looking for.
  bool
  is_big_endian() const
  { return this->is_big_endian_; }

  // Return the BFD name.  This may return NULL, in which case the
  // do_recognize_by_name hook will be responsible for matching the
  // BFD name.
  const char*
  bfd_name() const
  { return this->bfd_name_; }

 protected:
  // Return an instance of the real target.  This must be implemented
  // by the child class.
  virtual Target*
  do_instantiate_target() = 0;

  // Recognize an object file given a machine code, OSABI code, and
  // ELF version value.  When this is called we already know that they
  // match the machine_, size_, and is_big_endian_ fields.  The child
  // class may implement a different version of this to do additional
  // checks, or to check for multiple machine codes if the machine_
  // field is EM_NONE.
  virtual Target*
  do_recognize(int, int, int)
  { return this->instantiate_target(); }

  // Recognize a target by name.  When this is called we already know
  // that the name matches (or that the bfd_name_ field is NULL).  The
  // child class may implement a different version of this to
  // recognize more than one name.
  virtual Target*
  do_recognize_by_name(const char*)
  { return this->instantiate_target(); }

  // Return a list of supported BFD names.  The child class may
  // implement a different version of this to handle more than one
  // name.
  virtual void
  do_supported_names(std::vector<const char*>* names)
  {
    gold_assert(this->bfd_name_ != NULL);
    names->push_back(this->bfd_name_);
  }

  // Instantiate the target and return it.
  Target*
  instantiate_target();

 private:
  // Set the target.
  void
  set_target();

  friend class Set_target_once;

  // ELF machine code.
  const int machine_;
  // Target size--32 or 64.
  const int size_;
  // Whether the target is big endian.
  const bool is_big_endian_;
  // BFD name of target, for compatibility.
  const char* const bfd_name_;
  // Next entry in list built at global constructor time.
  Target_selector* next_;
  // The singleton Target structure--this points to an instance of the
  // real implementation.
  Target* instantiated_target_;
  // Used to set the target only once.
  Set_target_once set_target_once_;
};

// Select the target for an ELF file.

extern Target*
select_target(int machine, int size, bool big_endian, int osabi,
	      int abiversion);

// Select a target using a BFD name.

extern Target*
select_target_by_name(const char* name);

// Fill in a vector with the list of supported targets.  This returns
// a list of BFD names.

extern void
supported_target_names(std::vector<const char*>*);

} // End namespace gold.

#endif // !defined(GOLD_TARGET_SELECT_H)
