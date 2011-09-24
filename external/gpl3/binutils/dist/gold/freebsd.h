// freebsd.h -- FreeBSD support for gold    -*- C++ -*-

// Copyright 2009 Free Software Foundation, Inc.
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

#include "target.h"
#include "target-select.h"

#ifndef GOLD_FREEBSD_H
#define GOLD_FREEBSD_H

namespace gold
{

// FreeBSD 4.1 and later wants the EI_OSABI field in the ELF header to
// be set to ELFOSABI_FREEBSD.  This is a subclass of Sized_target
// which supports that.  The real target would be a subclass of this
// one.  We permit combining FreeBSD and non-FreeBSD object files.
// The effect of this target is to set the code in the output file.

template<int size, bool big_endian>
class Target_freebsd : public Sized_target<size, big_endian>
{
 public:
  // Set the value to use for the EI_OSABI field in the ELF header.
  void
  set_osabi(elfcpp::ELFOSABI osabi)
  { this->osabi_ = osabi; }

 protected:
  Target_freebsd(const Target::Target_info* pti)
    : Sized_target<size, big_endian>(pti),
      osabi_(elfcpp::ELFOSABI_NONE)
  { }

  virtual void
  do_adjust_elf_header(unsigned char* view, int len) const;

 private:
  // Value to store in the EI_OSABI field of the ELF file header.
  elfcpp::ELFOSABI osabi_;
};

// Adjust the ELF file header by storing the requested value in the
// OSABI field.  This is for FreeBSD support.

template<int size, bool big_endian>
inline void
Target_freebsd<size, big_endian>::do_adjust_elf_header(unsigned char* view,
						       int len) const
{
  if (this->osabi_ != elfcpp::ELFOSABI_NONE)
    {
      gold_assert(len == elfcpp::Elf_sizes<size>::ehdr_size);

      elfcpp::Ehdr<size, false> ehdr(view);
      unsigned char e_ident[elfcpp::EI_NIDENT];
      memcpy(e_ident, ehdr.get_e_ident(), elfcpp::EI_NIDENT);

      e_ident[elfcpp::EI_OSABI] = this->osabi_;

      elfcpp::Ehdr_write<size, false> oehdr(view);
      oehdr.put_e_ident(e_ident);
    }
}

// A target selector for targets which permit combining both FreeBSD
// and non-FreeBSD object files.

class Target_selector_freebsd : public Target_selector
{
 public:
  Target_selector_freebsd(int machine, int size, bool is_big_endian,
			  const char* bfd_name,
			  const char* freebsd_bfd_name)
    : Target_selector(machine, size, is_big_endian, NULL),
      bfd_name_(bfd_name), freebsd_bfd_name_(freebsd_bfd_name)
  { }

 protected:
  // If we see a FreeBSD input file, mark the output file as using
  // FreeBSD.
  virtual Target*
  do_recognize(int, int osabi, int)
  {
    Target* ret = this->instantiate_target();
    if (osabi == elfcpp::ELFOSABI_FREEBSD)
      this->set_osabi(ret);
    return ret;
  }
  
  // Recognize two names.
  virtual Target*
  do_recognize_by_name(const char* name)
  {
    if (strcmp(name, this->bfd_name_) == 0)
      return this->instantiate_target();
    else if (strcmp(name, this->freebsd_bfd_name_) == 0)
      {
	Target* ret = this->instantiate_target();
	this->set_osabi(ret);
	return ret;
      }
    else
      return NULL;
  }

  // Print both names in --help output.
  virtual void
  do_supported_names(std::vector<const char*>* names)
  {
    names->push_back(this->bfd_name_);
    names->push_back(this->freebsd_bfd_name_);
  }

 private:
  // Set the OSABI field.  This is quite ugly.
  void
  set_osabi(Target* target)
  {
    if (this->get_size() == 32)
      {
	if (this->is_big_endian())
	  static_cast<Target_freebsd<32, true>*>(target)->
	    set_osabi(elfcpp::ELFOSABI_FREEBSD);
	else
	  static_cast<Target_freebsd<32, false>*>(target)->
	    set_osabi(elfcpp::ELFOSABI_FREEBSD);
      }
    else if (this->get_size() == 64)
      {
	if (this->is_big_endian())
	  static_cast<Target_freebsd<64, true>*>(target)->
	    set_osabi(elfcpp::ELFOSABI_FREEBSD);
	else
	  static_cast<Target_freebsd<64, false>*>(target)->
	    set_osabi(elfcpp::ELFOSABI_FREEBSD);
      }
    else
      gold_unreachable();
  }

  // The BFD name for the non-Freebsd target.
  const char* bfd_name_;
  // The BFD name for the Freebsd target.
  const char* freebsd_bfd_name_;
};

} // end namespace gold

#endif // !defined(GOLD_FREEBSD_H)
