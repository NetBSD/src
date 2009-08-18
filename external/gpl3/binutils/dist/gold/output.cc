// output.cc -- manage the output file for gold

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

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <algorithm>
#include "libiberty.h"   // for unlink_if_ordinary()

#include "parameters.h"
#include "object.h"
#include "symtab.h"
#include "reloc.h"
#include "merge.h"
#include "descriptors.h"
#include "output.h"

// Some BSD systems still use MAP_ANON instead of MAP_ANONYMOUS
#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS  MAP_ANON
#endif

namespace gold
{

// Output_data variables.

bool Output_data::allocated_sizes_are_fixed;

// Output_data methods.

Output_data::~Output_data()
{
}

// Return the default alignment for the target size.

uint64_t
Output_data::default_alignment()
{
  return Output_data::default_alignment_for_size(
      parameters->target().get_size());
}

// Return the default alignment for a size--32 or 64.

uint64_t
Output_data::default_alignment_for_size(int size)
{
  if (size == 32)
    return 4;
  else if (size == 64)
    return 8;
  else
    gold_unreachable();
}

// Output_section_header methods.  This currently assumes that the
// segment and section lists are complete at construction time.

Output_section_headers::Output_section_headers(
    const Layout* layout,
    const Layout::Segment_list* segment_list,
    const Layout::Section_list* section_list,
    const Layout::Section_list* unattached_section_list,
    const Stringpool* secnamepool,
    const Output_section* shstrtab_section)
  : layout_(layout),
    segment_list_(segment_list),
    section_list_(section_list),
    unattached_section_list_(unattached_section_list),
    secnamepool_(secnamepool),
    shstrtab_section_(shstrtab_section)
{
  // Count all the sections.  Start with 1 for the null section.
  off_t count = 1;
  if (!parameters->options().relocatable())
    {
      for (Layout::Segment_list::const_iterator p = segment_list->begin();
	   p != segment_list->end();
	   ++p)
	if ((*p)->type() == elfcpp::PT_LOAD)
	  count += (*p)->output_section_count();
    }
  else
    {
      for (Layout::Section_list::const_iterator p = section_list->begin();
	   p != section_list->end();
	   ++p)
	if (((*p)->flags() & elfcpp::SHF_ALLOC) != 0)
	  ++count;
    }
  count += unattached_section_list->size();

  const int size = parameters->target().get_size();
  int shdr_size;
  if (size == 32)
    shdr_size = elfcpp::Elf_sizes<32>::shdr_size;
  else if (size == 64)
    shdr_size = elfcpp::Elf_sizes<64>::shdr_size;
  else
    gold_unreachable();

  this->set_data_size(count * shdr_size);
}

// Write out the section headers.

void
Output_section_headers::do_write(Output_file* of)
{
  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->do_sized_write<32, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->do_sized_write<32, true>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->do_sized_write<64, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->do_sized_write<64, true>(of);
      break;
#endif
    default:
      gold_unreachable();
    }
}

template<int size, bool big_endian>
void
Output_section_headers::do_sized_write(Output_file* of)
{
  off_t all_shdrs_size = this->data_size();
  unsigned char* view = of->get_output_view(this->offset(), all_shdrs_size);

  const int shdr_size = elfcpp::Elf_sizes<size>::shdr_size;
  unsigned char* v = view;

  {
    typename elfcpp::Shdr_write<size, big_endian> oshdr(v);
    oshdr.put_sh_name(0);
    oshdr.put_sh_type(elfcpp::SHT_NULL);
    oshdr.put_sh_flags(0);
    oshdr.put_sh_addr(0);
    oshdr.put_sh_offset(0);

    size_t section_count = (this->data_size()
			    / elfcpp::Elf_sizes<size>::shdr_size);
    if (section_count < elfcpp::SHN_LORESERVE)
      oshdr.put_sh_size(0);
    else
      oshdr.put_sh_size(section_count);

    unsigned int shstrndx = this->shstrtab_section_->out_shndx();
    if (shstrndx < elfcpp::SHN_LORESERVE)
      oshdr.put_sh_link(0);
    else
      oshdr.put_sh_link(shstrndx);

    oshdr.put_sh_info(0);
    oshdr.put_sh_addralign(0);
    oshdr.put_sh_entsize(0);
  }

  v += shdr_size;

  unsigned int shndx = 1;
  if (!parameters->options().relocatable())
    {
      for (Layout::Segment_list::const_iterator p =
	     this->segment_list_->begin();
	   p != this->segment_list_->end();
	   ++p)
	v = (*p)->write_section_headers<size, big_endian>(this->layout_,
							  this->secnamepool_,
							  v,
							  &shndx);
    }
  else
    {
      for (Layout::Section_list::const_iterator p =
	     this->section_list_->begin();
	   p != this->section_list_->end();
	   ++p)
	{
	  // We do unallocated sections below, except that group
	  // sections have to come first.
	  if (((*p)->flags() & elfcpp::SHF_ALLOC) == 0
	      && (*p)->type() != elfcpp::SHT_GROUP)
	    continue;
	  gold_assert(shndx == (*p)->out_shndx());
	  elfcpp::Shdr_write<size, big_endian> oshdr(v);
	  (*p)->write_header(this->layout_, this->secnamepool_, &oshdr);
	  v += shdr_size;
	  ++shndx;
	}
    }

  for (Layout::Section_list::const_iterator p =
	 this->unattached_section_list_->begin();
       p != this->unattached_section_list_->end();
       ++p)
    {
      // For a relocatable link, we did unallocated group sections
      // above, since they have to come first.
      if ((*p)->type() == elfcpp::SHT_GROUP
	  && parameters->options().relocatable())
	continue;
      gold_assert(shndx == (*p)->out_shndx());
      elfcpp::Shdr_write<size, big_endian> oshdr(v);
      (*p)->write_header(this->layout_, this->secnamepool_, &oshdr);
      v += shdr_size;
      ++shndx;
    }

  of->write_output_view(this->offset(), all_shdrs_size, view);
}

// Output_segment_header methods.

Output_segment_headers::Output_segment_headers(
    const Layout::Segment_list& segment_list)
  : segment_list_(segment_list)
{
  const int size = parameters->target().get_size();
  int phdr_size;
  if (size == 32)
    phdr_size = elfcpp::Elf_sizes<32>::phdr_size;
  else if (size == 64)
    phdr_size = elfcpp::Elf_sizes<64>::phdr_size;
  else
    gold_unreachable();

  this->set_data_size(segment_list.size() * phdr_size);
}

void
Output_segment_headers::do_write(Output_file* of)
{
  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->do_sized_write<32, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->do_sized_write<32, true>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->do_sized_write<64, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->do_sized_write<64, true>(of);
      break;
#endif
    default:
      gold_unreachable();
    }
}

template<int size, bool big_endian>
void
Output_segment_headers::do_sized_write(Output_file* of)
{
  const int phdr_size = elfcpp::Elf_sizes<size>::phdr_size;
  off_t all_phdrs_size = this->segment_list_.size() * phdr_size;
  gold_assert(all_phdrs_size == this->data_size());
  unsigned char* view = of->get_output_view(this->offset(),
					    all_phdrs_size);
  unsigned char* v = view;
  for (Layout::Segment_list::const_iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    {
      elfcpp::Phdr_write<size, big_endian> ophdr(v);
      (*p)->write_header(&ophdr);
      v += phdr_size;
    }

  gold_assert(v - view == all_phdrs_size);

  of->write_output_view(this->offset(), all_phdrs_size, view);
}

// Output_file_header methods.

Output_file_header::Output_file_header(const Target* target,
				       const Symbol_table* symtab,
				       const Output_segment_headers* osh,
				       const char* entry)
  : target_(target),
    symtab_(symtab),
    segment_header_(osh),
    section_header_(NULL),
    shstrtab_(NULL),
    entry_(entry)
{
  const int size = parameters->target().get_size();
  int ehdr_size;
  if (size == 32)
    ehdr_size = elfcpp::Elf_sizes<32>::ehdr_size;
  else if (size == 64)
    ehdr_size = elfcpp::Elf_sizes<64>::ehdr_size;
  else
    gold_unreachable();

  this->set_data_size(ehdr_size);
}

// Set the section table information for a file header.

void
Output_file_header::set_section_info(const Output_section_headers* shdrs,
				     const Output_section* shstrtab)
{
  this->section_header_ = shdrs;
  this->shstrtab_ = shstrtab;
}

// Write out the file header.

void
Output_file_header::do_write(Output_file* of)
{
  gold_assert(this->offset() == 0);

  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->do_sized_write<32, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->do_sized_write<32, true>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->do_sized_write<64, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->do_sized_write<64, true>(of);
      break;
#endif
    default:
      gold_unreachable();
    }
}

// Write out the file header with appropriate size and endianess.

template<int size, bool big_endian>
void
Output_file_header::do_sized_write(Output_file* of)
{
  gold_assert(this->offset() == 0);

  int ehdr_size = elfcpp::Elf_sizes<size>::ehdr_size;
  unsigned char* view = of->get_output_view(0, ehdr_size);
  elfcpp::Ehdr_write<size, big_endian> oehdr(view);

  unsigned char e_ident[elfcpp::EI_NIDENT];
  memset(e_ident, 0, elfcpp::EI_NIDENT);
  e_ident[elfcpp::EI_MAG0] = elfcpp::ELFMAG0;
  e_ident[elfcpp::EI_MAG1] = elfcpp::ELFMAG1;
  e_ident[elfcpp::EI_MAG2] = elfcpp::ELFMAG2;
  e_ident[elfcpp::EI_MAG3] = elfcpp::ELFMAG3;
  if (size == 32)
    e_ident[elfcpp::EI_CLASS] = elfcpp::ELFCLASS32;
  else if (size == 64)
    e_ident[elfcpp::EI_CLASS] = elfcpp::ELFCLASS64;
  else
    gold_unreachable();
  e_ident[elfcpp::EI_DATA] = (big_endian
			      ? elfcpp::ELFDATA2MSB
			      : elfcpp::ELFDATA2LSB);
  e_ident[elfcpp::EI_VERSION] = elfcpp::EV_CURRENT;
  // FIXME: Some targets may need to set EI_OSABI and EI_ABIVERSION.
  oehdr.put_e_ident(e_ident);

  elfcpp::ET e_type;
  if (parameters->options().relocatable())
    e_type = elfcpp::ET_REL;
  else if (parameters->options().shared())
    e_type = elfcpp::ET_DYN;
  else
    e_type = elfcpp::ET_EXEC;
  oehdr.put_e_type(e_type);

  oehdr.put_e_machine(this->target_->machine_code());
  oehdr.put_e_version(elfcpp::EV_CURRENT);

  oehdr.put_e_entry(this->entry<size>());

  if (this->segment_header_ == NULL)
    oehdr.put_e_phoff(0);
  else
    oehdr.put_e_phoff(this->segment_header_->offset());

  oehdr.put_e_shoff(this->section_header_->offset());

  // FIXME: The target needs to set the flags.
  oehdr.put_e_flags(0);

  oehdr.put_e_ehsize(elfcpp::Elf_sizes<size>::ehdr_size);

  if (this->segment_header_ == NULL)
    {
      oehdr.put_e_phentsize(0);
      oehdr.put_e_phnum(0);
    }
  else
    {
      oehdr.put_e_phentsize(elfcpp::Elf_sizes<size>::phdr_size);
      oehdr.put_e_phnum(this->segment_header_->data_size()
			/ elfcpp::Elf_sizes<size>::phdr_size);
    }

  oehdr.put_e_shentsize(elfcpp::Elf_sizes<size>::shdr_size);
  size_t section_count = (this->section_header_->data_size()
			  / elfcpp::Elf_sizes<size>::shdr_size);

  if (section_count < elfcpp::SHN_LORESERVE)
    oehdr.put_e_shnum(this->section_header_->data_size()
		      / elfcpp::Elf_sizes<size>::shdr_size);
  else
    oehdr.put_e_shnum(0);

  unsigned int shstrndx = this->shstrtab_->out_shndx();
  if (shstrndx < elfcpp::SHN_LORESERVE)
    oehdr.put_e_shstrndx(this->shstrtab_->out_shndx());
  else
    oehdr.put_e_shstrndx(elfcpp::SHN_XINDEX);

  of->write_output_view(0, ehdr_size, view);
}

// Return the value to use for the entry address.  THIS->ENTRY_ is the
// symbol specified on the command line, if any.

template<int size>
typename elfcpp::Elf_types<size>::Elf_Addr
Output_file_header::entry()
{
  const bool should_issue_warning = (this->entry_ != NULL
				     && !parameters->options().relocatable()
                                     && !parameters->options().shared());

  // FIXME: Need to support target specific entry symbol.
  const char* entry = this->entry_;
  if (entry == NULL)
    entry = "_start";

  Symbol* sym = this->symtab_->lookup(entry);

  typename Sized_symbol<size>::Value_type v;
  if (sym != NULL)
    {
      Sized_symbol<size>* ssym;
      ssym = this->symtab_->get_sized_symbol<size>(sym);
      if (!ssym->is_defined() && should_issue_warning)
	gold_warning("entry symbol '%s' exists but is not defined", entry);
      v = ssym->value();
    }
  else
    {
      // We couldn't find the entry symbol.  See if we can parse it as
      // a number.  This supports, e.g., -e 0x1000.
      char* endptr;
      v = strtoull(entry, &endptr, 0);
      if (*endptr != '\0')
	{
	  if (should_issue_warning)
	    gold_warning("cannot find entry symbol '%s'", entry);
	  v = 0;
	}
    }

  return v;
}

// Output_data_const methods.

void
Output_data_const::do_write(Output_file* of)
{
  of->write(this->offset(), this->data_.data(), this->data_.size());
}

// Output_data_const_buffer methods.

void
Output_data_const_buffer::do_write(Output_file* of)
{
  of->write(this->offset(), this->p_, this->data_size());
}

// Output_section_data methods.

// Record the output section, and set the entry size and such.

void
Output_section_data::set_output_section(Output_section* os)
{
  gold_assert(this->output_section_ == NULL);
  this->output_section_ = os;
  this->do_adjust_output_section(os);
}

// Return the section index of the output section.

unsigned int
Output_section_data::do_out_shndx() const
{
  gold_assert(this->output_section_ != NULL);
  return this->output_section_->out_shndx();
}

// Set the alignment, which means we may need to update the alignment
// of the output section.

void
Output_section_data::set_addralign(uint64_t addralign)
{
  this->addralign_ = addralign;
  if (this->output_section_ != NULL
      && this->output_section_->addralign() < addralign)
    this->output_section_->set_addralign(addralign);
}

// Output_data_strtab methods.

// Set the final data size.

void
Output_data_strtab::set_final_data_size()
{
  this->strtab_->set_string_offsets();
  this->set_data_size(this->strtab_->get_strtab_size());
}

// Write out a string table.

void
Output_data_strtab::do_write(Output_file* of)
{
  this->strtab_->write(of, this->offset());
}

// Output_reloc methods.

// A reloc against a global symbol.

template<bool dynamic, int size, bool big_endian>
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::Output_reloc(
    Symbol* gsym,
    unsigned int type,
    Output_data* od,
    Address address,
    bool is_relative)
  : address_(address), local_sym_index_(GSYM_CODE), type_(type),
    is_relative_(is_relative), is_section_symbol_(false), shndx_(INVALID_CODE)
{
  // this->type_ is a bitfield; make sure TYPE fits.
  gold_assert(this->type_ == type);
  this->u1_.gsym = gsym;
  this->u2_.od = od;
  if (dynamic)
    this->set_needs_dynsym_index();
}

template<bool dynamic, int size, bool big_endian>
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::Output_reloc(
    Symbol* gsym,
    unsigned int type,
    Sized_relobj<size, big_endian>* relobj,
    unsigned int shndx,
    Address address,
    bool is_relative)
  : address_(address), local_sym_index_(GSYM_CODE), type_(type),
    is_relative_(is_relative), is_section_symbol_(false), shndx_(shndx)
{
  gold_assert(shndx != INVALID_CODE);
  // this->type_ is a bitfield; make sure TYPE fits.
  gold_assert(this->type_ == type);
  this->u1_.gsym = gsym;
  this->u2_.relobj = relobj;
  if (dynamic)
    this->set_needs_dynsym_index();
}

// A reloc against a local symbol.

template<bool dynamic, int size, bool big_endian>
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::Output_reloc(
    Sized_relobj<size, big_endian>* relobj,
    unsigned int local_sym_index,
    unsigned int type,
    Output_data* od,
    Address address,
    bool is_relative,
    bool is_section_symbol)
  : address_(address), local_sym_index_(local_sym_index), type_(type),
    is_relative_(is_relative), is_section_symbol_(is_section_symbol),
    shndx_(INVALID_CODE)
{
  gold_assert(local_sym_index != GSYM_CODE
              && local_sym_index != INVALID_CODE);
  // this->type_ is a bitfield; make sure TYPE fits.
  gold_assert(this->type_ == type);
  this->u1_.relobj = relobj;
  this->u2_.od = od;
  if (dynamic)
    this->set_needs_dynsym_index();
}

template<bool dynamic, int size, bool big_endian>
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::Output_reloc(
    Sized_relobj<size, big_endian>* relobj,
    unsigned int local_sym_index,
    unsigned int type,
    unsigned int shndx,
    Address address,
    bool is_relative,
    bool is_section_symbol)
  : address_(address), local_sym_index_(local_sym_index), type_(type),
    is_relative_(is_relative), is_section_symbol_(is_section_symbol),
    shndx_(shndx)
{
  gold_assert(local_sym_index != GSYM_CODE
              && local_sym_index != INVALID_CODE);
  gold_assert(shndx != INVALID_CODE);
  // this->type_ is a bitfield; make sure TYPE fits.
  gold_assert(this->type_ == type);
  this->u1_.relobj = relobj;
  this->u2_.relobj = relobj;
  if (dynamic)
    this->set_needs_dynsym_index();
}

// A reloc against the STT_SECTION symbol of an output section.

template<bool dynamic, int size, bool big_endian>
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::Output_reloc(
    Output_section* os,
    unsigned int type,
    Output_data* od,
    Address address)
  : address_(address), local_sym_index_(SECTION_CODE), type_(type),
    is_relative_(false), is_section_symbol_(true), shndx_(INVALID_CODE)
{
  // this->type_ is a bitfield; make sure TYPE fits.
  gold_assert(this->type_ == type);
  this->u1_.os = os;
  this->u2_.od = od;
  if (dynamic)
    this->set_needs_dynsym_index();
  else
    os->set_needs_symtab_index();
}

template<bool dynamic, int size, bool big_endian>
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::Output_reloc(
    Output_section* os,
    unsigned int type,
    Sized_relobj<size, big_endian>* relobj,
    unsigned int shndx,
    Address address)
  : address_(address), local_sym_index_(SECTION_CODE), type_(type),
    is_relative_(false), is_section_symbol_(true), shndx_(shndx)
{
  gold_assert(shndx != INVALID_CODE);
  // this->type_ is a bitfield; make sure TYPE fits.
  gold_assert(this->type_ == type);
  this->u1_.os = os;
  this->u2_.relobj = relobj;
  if (dynamic)
    this->set_needs_dynsym_index();
  else
    os->set_needs_symtab_index();
}

// Record that we need a dynamic symbol index for this relocation.

template<bool dynamic, int size, bool big_endian>
void
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::
set_needs_dynsym_index()
{
  if (this->is_relative_)
    return;
  switch (this->local_sym_index_)
    {
    case INVALID_CODE:
      gold_unreachable();

    case GSYM_CODE:
      this->u1_.gsym->set_needs_dynsym_entry();
      break;

    case SECTION_CODE:
      this->u1_.os->set_needs_dynsym_index();
      break;

    case 0:
      break;

    default:
      {
        const unsigned int lsi = this->local_sym_index_;
        if (!this->is_section_symbol_)
          this->u1_.relobj->set_needs_output_dynsym_entry(lsi);
        else
          this->u1_.relobj->output_section(lsi)->set_needs_dynsym_index();
      }
      break;
    }
}

// Get the symbol index of a relocation.

template<bool dynamic, int size, bool big_endian>
unsigned int
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::get_symbol_index()
  const
{
  unsigned int index;
  switch (this->local_sym_index_)
    {
    case INVALID_CODE:
      gold_unreachable();

    case GSYM_CODE:
      if (this->u1_.gsym == NULL)
	index = 0;
      else if (dynamic)
	index = this->u1_.gsym->dynsym_index();
      else
	index = this->u1_.gsym->symtab_index();
      break;

    case SECTION_CODE:
      if (dynamic)
	index = this->u1_.os->dynsym_index();
      else
	index = this->u1_.os->symtab_index();
      break;

    case 0:
      // Relocations without symbols use a symbol index of 0.
      index = 0;
      break;

    default:
      {
        const unsigned int lsi = this->local_sym_index_;
        if (!this->is_section_symbol_)
          {
            if (dynamic)
              index = this->u1_.relobj->dynsym_index(lsi);
            else
              index = this->u1_.relobj->symtab_index(lsi);
          }
        else
          {
            Output_section* os = this->u1_.relobj->output_section(lsi);
            gold_assert(os != NULL);
            if (dynamic)
              index = os->dynsym_index();
            else
              index = os->symtab_index();
          }
      }
      break;
    }
  gold_assert(index != -1U);
  return index;
}

// For a local section symbol, get the address of the offset ADDEND
// within the input section.

template<bool dynamic, int size, bool big_endian>
typename elfcpp::Elf_types<size>::Elf_Addr
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::
  local_section_offset(Addend addend) const
{
  gold_assert(this->local_sym_index_ != GSYM_CODE
              && this->local_sym_index_ != SECTION_CODE
              && this->local_sym_index_ != INVALID_CODE
              && this->is_section_symbol_);
  const unsigned int lsi = this->local_sym_index_;
  Output_section* os = this->u1_.relobj->output_section(lsi);
  gold_assert(os != NULL);
  Address offset = this->u1_.relobj->get_output_section_offset(lsi);
  if (offset != -1U)
    return offset + addend;
  // This is a merge section.
  offset = os->output_address(this->u1_.relobj, lsi, addend);
  gold_assert(offset != -1U);
  return offset;
}

// Get the output address of a relocation.

template<bool dynamic, int size, bool big_endian>
typename elfcpp::Elf_types<size>::Elf_Addr
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::get_address() const
{
  Address address = this->address_;
  if (this->shndx_ != INVALID_CODE)
    {
      Output_section* os = this->u2_.relobj->output_section(this->shndx_);
      gold_assert(os != NULL);
      Address off = this->u2_.relobj->get_output_section_offset(this->shndx_);
      if (off != -1U)
	address += os->address() + off;
      else
	{
	  address = os->output_address(this->u2_.relobj, this->shndx_,
				       address);
	  gold_assert(address != -1U);
	}
    }
  else if (this->u2_.od != NULL)
    address += this->u2_.od->address();
  return address;
}

// Write out the offset and info fields of a Rel or Rela relocation
// entry.

template<bool dynamic, int size, bool big_endian>
template<typename Write_rel>
void
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::write_rel(
    Write_rel* wr) const
{
  wr->put_r_offset(this->get_address());
  unsigned int sym_index = this->is_relative_ ? 0 : this->get_symbol_index();
  wr->put_r_info(elfcpp::elf_r_info<size>(sym_index, this->type_));
}

// Write out a Rel relocation.

template<bool dynamic, int size, bool big_endian>
void
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::write(
    unsigned char* pov) const
{
  elfcpp::Rel_write<size, big_endian> orel(pov);
  this->write_rel(&orel);
}

// Get the value of the symbol referred to by a Rel relocation.

template<bool dynamic, int size, bool big_endian>
typename elfcpp::Elf_types<size>::Elf_Addr
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::symbol_value(
    Addend addend) const
{
  if (this->local_sym_index_ == GSYM_CODE)
    {
      const Sized_symbol<size>* sym;
      sym = static_cast<const Sized_symbol<size>*>(this->u1_.gsym);
      return sym->value() + addend;
    }
  gold_assert(this->local_sym_index_ != SECTION_CODE
              && this->local_sym_index_ != INVALID_CODE
              && !this->is_section_symbol_);
  const unsigned int lsi = this->local_sym_index_;
  const Symbol_value<size>* symval = this->u1_.relobj->local_symbol(lsi);
  return symval->value(this->u1_.relobj, addend);
}

// Reloc comparison.  This function sorts the dynamic relocs for the
// benefit of the dynamic linker.  First we sort all relative relocs
// to the front.  Among relative relocs, we sort by output address.
// Among non-relative relocs, we sort by symbol index, then by output
// address.

template<bool dynamic, int size, bool big_endian>
int
Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>::
  compare(const Output_reloc<elfcpp::SHT_REL, dynamic, size, big_endian>& r2)
    const
{
  if (this->is_relative_)
    {
      if (!r2.is_relative_)
	return -1;
      // Otherwise sort by reloc address below.
    }
  else if (r2.is_relative_)
    return 1;
  else
    {
      unsigned int sym1 = this->get_symbol_index();
      unsigned int sym2 = r2.get_symbol_index();
      if (sym1 < sym2)
	return -1;
      else if (sym1 > sym2)
	return 1;
      // Otherwise sort by reloc address.
    }

  section_offset_type addr1 = this->get_address();
  section_offset_type addr2 = r2.get_address();
  if (addr1 < addr2)
    return -1;
  else if (addr1 > addr2)
    return 1;

  // Final tie breaker, in order to generate the same output on any
  // host: reloc type.
  unsigned int type1 = this->type_;
  unsigned int type2 = r2.type_;
  if (type1 < type2)
    return -1;
  else if (type1 > type2)
    return 1;

  // These relocs appear to be exactly the same.
  return 0;
}

// Write out a Rela relocation.

template<bool dynamic, int size, bool big_endian>
void
Output_reloc<elfcpp::SHT_RELA, dynamic, size, big_endian>::write(
    unsigned char* pov) const
{
  elfcpp::Rela_write<size, big_endian> orel(pov);
  this->rel_.write_rel(&orel);
  Addend addend = this->addend_;
  if (this->rel_.is_relative())
    addend = this->rel_.symbol_value(addend);
  else if (this->rel_.is_local_section_symbol())
    addend = this->rel_.local_section_offset(addend);
  orel.put_r_addend(addend);
}

// Output_data_reloc_base methods.

// Adjust the output section.

template<int sh_type, bool dynamic, int size, bool big_endian>
void
Output_data_reloc_base<sh_type, dynamic, size, big_endian>
    ::do_adjust_output_section(Output_section* os)
{
  if (sh_type == elfcpp::SHT_REL)
    os->set_entsize(elfcpp::Elf_sizes<size>::rel_size);
  else if (sh_type == elfcpp::SHT_RELA)
    os->set_entsize(elfcpp::Elf_sizes<size>::rela_size);
  else
    gold_unreachable();
  if (dynamic)
    os->set_should_link_to_dynsym();
  else
    os->set_should_link_to_symtab();
}

// Write out relocation data.

template<int sh_type, bool dynamic, int size, bool big_endian>
void
Output_data_reloc_base<sh_type, dynamic, size, big_endian>::do_write(
    Output_file* of)
{
  const off_t off = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(off, oview_size);

  if (this->sort_relocs_)
    {
      gold_assert(dynamic);
      std::sort(this->relocs_.begin(), this->relocs_.end(),
		Sort_relocs_comparison());
    }

  unsigned char* pov = oview;
  for (typename Relocs::const_iterator p = this->relocs_.begin();
       p != this->relocs_.end();
       ++p)
    {
      p->write(pov);
      pov += reloc_size;
    }

  gold_assert(pov - oview == oview_size);

  of->write_output_view(off, oview_size, oview);

  // We no longer need the relocation entries.
  this->relocs_.clear();
}

// Class Output_relocatable_relocs.

template<int sh_type, int size, bool big_endian>
void
Output_relocatable_relocs<sh_type, size, big_endian>::set_final_data_size()
{
  this->set_data_size(this->rr_->output_reloc_count()
		      * Reloc_types<sh_type, size, big_endian>::reloc_size);
}

// class Output_data_group.

template<int size, bool big_endian>
Output_data_group<size, big_endian>::Output_data_group(
    Sized_relobj<size, big_endian>* relobj,
    section_size_type entry_count,
    elfcpp::Elf_Word flags,
    std::vector<unsigned int>* input_shndxes)
  : Output_section_data(entry_count * 4, 4),
    relobj_(relobj),
    flags_(flags)
{
  this->input_shndxes_.swap(*input_shndxes);
}

// Write out the section group, which means translating the section
// indexes to apply to the output file.

template<int size, bool big_endian>
void
Output_data_group<size, big_endian>::do_write(Output_file* of)
{
  const off_t off = this->offset();
  const section_size_type oview_size =
    convert_to_section_size_type(this->data_size());
  unsigned char* const oview = of->get_output_view(off, oview_size);

  elfcpp::Elf_Word* contents = reinterpret_cast<elfcpp::Elf_Word*>(oview);
  elfcpp::Swap<32, big_endian>::writeval(contents, this->flags_);
  ++contents;

  for (std::vector<unsigned int>::const_iterator p =
	 this->input_shndxes_.begin();
       p != this->input_shndxes_.end();
       ++p, ++contents)
    {
      Output_section* os = this->relobj_->output_section(*p);

      unsigned int output_shndx;
      if (os != NULL)
	output_shndx = os->out_shndx();
      else
	{
	  this->relobj_->error(_("section group retained but "
				 "group element discarded"));
	  output_shndx = 0;
	}

      elfcpp::Swap<32, big_endian>::writeval(contents, output_shndx);
    }

  size_t wrote = reinterpret_cast<unsigned char*>(contents) - oview;
  gold_assert(wrote == oview_size);

  of->write_output_view(off, oview_size, oview);

  // We no longer need this information.
  this->input_shndxes_.clear();
}

// Output_data_got::Got_entry methods.

// Write out the entry.

template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::Got_entry::write(unsigned char* pov) const
{
  Valtype val = 0;

  switch (this->local_sym_index_)
    {
    case GSYM_CODE:
      {
	// If the symbol is resolved locally, we need to write out the
	// link-time value, which will be relocated dynamically by a
	// RELATIVE relocation.
	Symbol* gsym = this->u_.gsym;
	Sized_symbol<size>* sgsym;
	// This cast is a bit ugly.  We don't want to put a
	// virtual method in Symbol, because we want Symbol to be
	// as small as possible.
	sgsym = static_cast<Sized_symbol<size>*>(gsym);
	val = sgsym->value();
      }
      break;

    case CONSTANT_CODE:
      val = this->u_.constant;
      break;

    default:
      {
        const unsigned int lsi = this->local_sym_index_;
        const Symbol_value<size>* symval = this->u_.object->local_symbol(lsi);
        val = symval->value(this->u_.object, 0);
      }
      break;
    }

  elfcpp::Swap<size, big_endian>::writeval(pov, val);
}

// Output_data_got methods.

// Add an entry for a global symbol to the GOT.  This returns true if
// this is a new GOT entry, false if the symbol already had a GOT
// entry.

template<int size, bool big_endian>
bool
Output_data_got<size, big_endian>::add_global(
    Symbol* gsym,
    unsigned int got_type)
{
  if (gsym->has_got_offset(got_type))
    return false;

  this->entries_.push_back(Got_entry(gsym));
  this->set_got_size();
  gsym->set_got_offset(got_type, this->last_got_offset());
  return true;
}

// Add an entry for a global symbol to the GOT, and add a dynamic
// relocation of type R_TYPE for the GOT entry.
template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::add_global_with_rel(
    Symbol* gsym,
    unsigned int got_type,
    Rel_dyn* rel_dyn,
    unsigned int r_type)
{
  if (gsym->has_got_offset(got_type))
    return;

  this->entries_.push_back(Got_entry());
  this->set_got_size();
  unsigned int got_offset = this->last_got_offset();
  gsym->set_got_offset(got_type, got_offset);
  rel_dyn->add_global(gsym, r_type, this, got_offset);
}

template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::add_global_with_rela(
    Symbol* gsym,
    unsigned int got_type,
    Rela_dyn* rela_dyn,
    unsigned int r_type)
{
  if (gsym->has_got_offset(got_type))
    return;

  this->entries_.push_back(Got_entry());
  this->set_got_size();
  unsigned int got_offset = this->last_got_offset();
  gsym->set_got_offset(got_type, got_offset);
  rela_dyn->add_global(gsym, r_type, this, got_offset, 0);
}

// Add a pair of entries for a global symbol to the GOT, and add
// dynamic relocations of type R_TYPE_1 and R_TYPE_2, respectively.
// If R_TYPE_2 == 0, add the second entry with no relocation.
template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::add_global_pair_with_rel(
    Symbol* gsym,
    unsigned int got_type,
    Rel_dyn* rel_dyn,
    unsigned int r_type_1,
    unsigned int r_type_2)
{
  if (gsym->has_got_offset(got_type))
    return;

  this->entries_.push_back(Got_entry());
  unsigned int got_offset = this->last_got_offset();
  gsym->set_got_offset(got_type, got_offset);
  rel_dyn->add_global(gsym, r_type_1, this, got_offset);

  this->entries_.push_back(Got_entry());
  if (r_type_2 != 0)
    {
      got_offset = this->last_got_offset();
      rel_dyn->add_global(gsym, r_type_2, this, got_offset);
    }

  this->set_got_size();
}

template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::add_global_pair_with_rela(
    Symbol* gsym,
    unsigned int got_type,
    Rela_dyn* rela_dyn,
    unsigned int r_type_1,
    unsigned int r_type_2)
{
  if (gsym->has_got_offset(got_type))
    return;

  this->entries_.push_back(Got_entry());
  unsigned int got_offset = this->last_got_offset();
  gsym->set_got_offset(got_type, got_offset);
  rela_dyn->add_global(gsym, r_type_1, this, got_offset, 0);

  this->entries_.push_back(Got_entry());
  if (r_type_2 != 0)
    {
      got_offset = this->last_got_offset();
      rela_dyn->add_global(gsym, r_type_2, this, got_offset, 0);
    }

  this->set_got_size();
}

// Add an entry for a local symbol to the GOT.  This returns true if
// this is a new GOT entry, false if the symbol already has a GOT
// entry.

template<int size, bool big_endian>
bool
Output_data_got<size, big_endian>::add_local(
    Sized_relobj<size, big_endian>* object,
    unsigned int symndx,
    unsigned int got_type)
{
  if (object->local_has_got_offset(symndx, got_type))
    return false;

  this->entries_.push_back(Got_entry(object, symndx));
  this->set_got_size();
  object->set_local_got_offset(symndx, got_type, this->last_got_offset());
  return true;
}

// Add an entry for a local symbol to the GOT, and add a dynamic
// relocation of type R_TYPE for the GOT entry.
template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::add_local_with_rel(
    Sized_relobj<size, big_endian>* object,
    unsigned int symndx,
    unsigned int got_type,
    Rel_dyn* rel_dyn,
    unsigned int r_type)
{
  if (object->local_has_got_offset(symndx, got_type))
    return;

  this->entries_.push_back(Got_entry());
  this->set_got_size();
  unsigned int got_offset = this->last_got_offset();
  object->set_local_got_offset(symndx, got_type, got_offset);
  rel_dyn->add_local(object, symndx, r_type, this, got_offset);
}

template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::add_local_with_rela(
    Sized_relobj<size, big_endian>* object,
    unsigned int symndx,
    unsigned int got_type,
    Rela_dyn* rela_dyn,
    unsigned int r_type)
{
  if (object->local_has_got_offset(symndx, got_type))
    return;

  this->entries_.push_back(Got_entry());
  this->set_got_size();
  unsigned int got_offset = this->last_got_offset();
  object->set_local_got_offset(symndx, got_type, got_offset);
  rela_dyn->add_local(object, symndx, r_type, this, got_offset, 0);
}

// Add a pair of entries for a local symbol to the GOT, and add
// dynamic relocations of type R_TYPE_1 and R_TYPE_2, respectively.
// If R_TYPE_2 == 0, add the second entry with no relocation.
template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::add_local_pair_with_rel(
    Sized_relobj<size, big_endian>* object,
    unsigned int symndx,
    unsigned int shndx,
    unsigned int got_type,
    Rel_dyn* rel_dyn,
    unsigned int r_type_1,
    unsigned int r_type_2)
{
  if (object->local_has_got_offset(symndx, got_type))
    return;

  this->entries_.push_back(Got_entry());
  unsigned int got_offset = this->last_got_offset();
  object->set_local_got_offset(symndx, got_type, got_offset);
  Output_section* os = object->output_section(shndx);
  rel_dyn->add_output_section(os, r_type_1, this, got_offset);

  this->entries_.push_back(Got_entry(object, symndx));
  if (r_type_2 != 0)
    {
      got_offset = this->last_got_offset();
      rel_dyn->add_output_section(os, r_type_2, this, got_offset);
    }

  this->set_got_size();
}

template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::add_local_pair_with_rela(
    Sized_relobj<size, big_endian>* object,
    unsigned int symndx,
    unsigned int shndx,
    unsigned int got_type,
    Rela_dyn* rela_dyn,
    unsigned int r_type_1,
    unsigned int r_type_2)
{
  if (object->local_has_got_offset(symndx, got_type))
    return;

  this->entries_.push_back(Got_entry());
  unsigned int got_offset = this->last_got_offset();
  object->set_local_got_offset(symndx, got_type, got_offset);
  Output_section* os = object->output_section(shndx);
  rela_dyn->add_output_section(os, r_type_1, this, got_offset, 0);

  this->entries_.push_back(Got_entry(object, symndx));
  if (r_type_2 != 0)
    {
      got_offset = this->last_got_offset();
      rela_dyn->add_output_section(os, r_type_2, this, got_offset, 0);
    }

  this->set_got_size();
}

// Write out the GOT.

template<int size, bool big_endian>
void
Output_data_got<size, big_endian>::do_write(Output_file* of)
{
  const int add = size / 8;

  const off_t off = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(off, oview_size);

  unsigned char* pov = oview;
  for (typename Got_entries::const_iterator p = this->entries_.begin();
       p != this->entries_.end();
       ++p)
    {
      p->write(pov);
      pov += add;
    }

  gold_assert(pov - oview == oview_size);

  of->write_output_view(off, oview_size, oview);

  // We no longer need the GOT entries.
  this->entries_.clear();
}

// Output_data_dynamic::Dynamic_entry methods.

// Write out the entry.

template<int size, bool big_endian>
void
Output_data_dynamic::Dynamic_entry::write(
    unsigned char* pov,
    const Stringpool* pool) const
{
  typename elfcpp::Elf_types<size>::Elf_WXword val;
  switch (this->offset_)
    {
    case DYNAMIC_NUMBER:
      val = this->u_.val;
      break;

    case DYNAMIC_SECTION_SIZE:
      val = this->u_.od->data_size();
      break;

    case DYNAMIC_SYMBOL:
      {
	const Sized_symbol<size>* s =
	  static_cast<const Sized_symbol<size>*>(this->u_.sym);
	val = s->value();
      }
      break;

    case DYNAMIC_STRING:
      val = pool->get_offset(this->u_.str);
      break;

    default:
      val = this->u_.od->address() + this->offset_;
      break;
    }

  elfcpp::Dyn_write<size, big_endian> dw(pov);
  dw.put_d_tag(this->tag_);
  dw.put_d_val(val);
}

// Output_data_dynamic methods.

// Adjust the output section to set the entry size.

void
Output_data_dynamic::do_adjust_output_section(Output_section* os)
{
  if (parameters->target().get_size() == 32)
    os->set_entsize(elfcpp::Elf_sizes<32>::dyn_size);
  else if (parameters->target().get_size() == 64)
    os->set_entsize(elfcpp::Elf_sizes<64>::dyn_size);
  else
    gold_unreachable();
}

// Set the final data size.

void
Output_data_dynamic::set_final_data_size()
{
  // Add the terminating entry.
  this->add_constant(elfcpp::DT_NULL, 0);

  int dyn_size;
  if (parameters->target().get_size() == 32)
    dyn_size = elfcpp::Elf_sizes<32>::dyn_size;
  else if (parameters->target().get_size() == 64)
    dyn_size = elfcpp::Elf_sizes<64>::dyn_size;
  else
    gold_unreachable();
  this->set_data_size(this->entries_.size() * dyn_size);
}

// Write out the dynamic entries.

void
Output_data_dynamic::do_write(Output_file* of)
{
  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->sized_write<32, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->sized_write<32, true>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->sized_write<64, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->sized_write<64, true>(of);
      break;
#endif
    default:
      gold_unreachable();
    }
}

template<int size, bool big_endian>
void
Output_data_dynamic::sized_write(Output_file* of)
{
  const int dyn_size = elfcpp::Elf_sizes<size>::dyn_size;

  const off_t offset = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(offset, oview_size);

  unsigned char* pov = oview;
  for (typename Dynamic_entries::const_iterator p = this->entries_.begin();
       p != this->entries_.end();
       ++p)
    {
      p->write<size, big_endian>(pov, this->pool_);
      pov += dyn_size;
    }

  gold_assert(pov - oview == oview_size);

  of->write_output_view(offset, oview_size, oview);

  // We no longer need the dynamic entries.
  this->entries_.clear();
}

// Class Output_symtab_xindex.

void
Output_symtab_xindex::do_write(Output_file* of)
{
  const off_t offset = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(offset, oview_size);

  memset(oview, 0, oview_size);

  if (parameters->target().is_big_endian())
    this->endian_do_write<true>(oview);
  else
    this->endian_do_write<false>(oview);

  of->write_output_view(offset, oview_size, oview);

  // We no longer need the data.
  this->entries_.clear();
}

template<bool big_endian>
void
Output_symtab_xindex::endian_do_write(unsigned char* const oview)
{
  for (Xindex_entries::const_iterator p = this->entries_.begin();
       p != this->entries_.end();
       ++p)
    elfcpp::Swap<32, big_endian>::writeval(oview + p->first * 4, p->second);
}

// Output_section::Input_section methods.

// Return the data size.  For an input section we store the size here.
// For an Output_section_data, we have to ask it for the size.

off_t
Output_section::Input_section::data_size() const
{
  if (this->is_input_section())
    return this->u1_.data_size;
  else
    return this->u2_.posd->data_size();
}

// Set the address and file offset.

void
Output_section::Input_section::set_address_and_file_offset(
    uint64_t address,
    off_t file_offset,
    off_t section_file_offset)
{
  if (this->is_input_section())
    this->u2_.object->set_section_offset(this->shndx_,
					 file_offset - section_file_offset);
  else
    this->u2_.posd->set_address_and_file_offset(address, file_offset);
}

// Reset the address and file offset.

void
Output_section::Input_section::reset_address_and_file_offset()
{
  if (!this->is_input_section())
    this->u2_.posd->reset_address_and_file_offset();
}

// Finalize the data size.

void
Output_section::Input_section::finalize_data_size()
{
  if (!this->is_input_section())
    this->u2_.posd->finalize_data_size();
}

// Try to turn an input offset into an output offset.  We want to
// return the output offset relative to the start of this
// Input_section in the output section.

inline bool
Output_section::Input_section::output_offset(
    const Relobj* object,
    unsigned int shndx,
    section_offset_type offset,
    section_offset_type *poutput) const
{
  if (!this->is_input_section())
    return this->u2_.posd->output_offset(object, shndx, offset, poutput);
  else
    {
      if (this->shndx_ != shndx || this->u2_.object != object)
	return false;
      *poutput = offset;
      return true;
    }
}

// Return whether this is the merge section for the input section
// SHNDX in OBJECT.

inline bool
Output_section::Input_section::is_merge_section_for(const Relobj* object,
						    unsigned int shndx) const
{
  if (this->is_input_section())
    return false;
  return this->u2_.posd->is_merge_section_for(object, shndx);
}

// Write out the data.  We don't have to do anything for an input
// section--they are handled via Object::relocate--but this is where
// we write out the data for an Output_section_data.

void
Output_section::Input_section::write(Output_file* of)
{
  if (!this->is_input_section())
    this->u2_.posd->write(of);
}

// Write the data to a buffer.  As for write(), we don't have to do
// anything for an input section.

void
Output_section::Input_section::write_to_buffer(unsigned char* buffer)
{
  if (!this->is_input_section())
    this->u2_.posd->write_to_buffer(buffer);
}

// Print to a map file.

void
Output_section::Input_section::print_to_mapfile(Mapfile* mapfile) const
{
  switch (this->shndx_)
    {
    case OUTPUT_SECTION_CODE:
    case MERGE_DATA_SECTION_CODE:
    case MERGE_STRING_SECTION_CODE:
      this->u2_.posd->print_to_mapfile(mapfile);
      break;

    default:
      mapfile->print_input_section(this->u2_.object, this->shndx_);
      break;
    }
}

// Output_section methods.

// Construct an Output_section.  NAME will point into a Stringpool.

Output_section::Output_section(const char* name, elfcpp::Elf_Word type,
			       elfcpp::Elf_Xword flags)
  : name_(name),
    addralign_(0),
    entsize_(0),
    load_address_(0),
    link_section_(NULL),
    link_(0),
    info_section_(NULL),
    info_symndx_(NULL),
    info_(0),
    type_(type),
    flags_(flags),
    out_shndx_(-1U),
    symtab_index_(0),
    dynsym_index_(0),
    input_sections_(),
    first_input_offset_(0),
    fills_(),
    postprocessing_buffer_(NULL),
    needs_symtab_index_(false),
    needs_dynsym_index_(false),
    should_link_to_symtab_(false),
    should_link_to_dynsym_(false),
    after_input_sections_(false),
    requires_postprocessing_(false),
    found_in_sections_clause_(false),
    has_load_address_(false),
    info_uses_section_index_(false),
    may_sort_attached_input_sections_(false),
    must_sort_attached_input_sections_(false),
    attached_input_sections_are_sorted_(false),
    is_relro_(false),
    is_relro_local_(false),
    tls_offset_(0)
{
  // An unallocated section has no address.  Forcing this means that
  // we don't need special treatment for symbols defined in debug
  // sections.
  if ((flags & elfcpp::SHF_ALLOC) == 0)
    this->set_address(0);
}

Output_section::~Output_section()
{
}

// Set the entry size.

void
Output_section::set_entsize(uint64_t v)
{
  if (this->entsize_ == 0)
    this->entsize_ = v;
  else
    gold_assert(this->entsize_ == v);
}

// Add the input section SHNDX, with header SHDR, named SECNAME, in
// OBJECT, to the Output_section.  RELOC_SHNDX is the index of a
// relocation section which applies to this section, or 0 if none, or
// -1U if more than one.  Return the offset of the input section
// within the output section.  Return -1 if the input section will
// receive special handling.  In the normal case we don't always keep
// track of input sections for an Output_section.  Instead, each
// Object keeps track of the Output_section for each of its input
// sections.  However, if HAVE_SECTIONS_SCRIPT is true, we do keep
// track of input sections here; this is used when SECTIONS appears in
// a linker script.

template<int size, bool big_endian>
off_t
Output_section::add_input_section(Sized_relobj<size, big_endian>* object,
				  unsigned int shndx,
				  const char* secname,
				  const elfcpp::Shdr<size, big_endian>& shdr,
				  unsigned int reloc_shndx,
				  bool have_sections_script)
{
  elfcpp::Elf_Xword addralign = shdr.get_sh_addralign();
  if ((addralign & (addralign - 1)) != 0)
    {
      object->error(_("invalid alignment %lu for section \"%s\""),
		    static_cast<unsigned long>(addralign), secname);
      addralign = 1;
    }

  if (addralign > this->addralign_)
    this->addralign_ = addralign;

  typename elfcpp::Elf_types<size>::Elf_WXword sh_flags = shdr.get_sh_flags();
  this->update_flags_for_input_section(sh_flags);

  uint64_t entsize = shdr.get_sh_entsize();

  // .debug_str is a mergeable string section, but is not always so
  // marked by compilers.  Mark manually here so we can optimize.
  if (strcmp(secname, ".debug_str") == 0)
    {
      sh_flags |= (elfcpp::SHF_MERGE | elfcpp::SHF_STRINGS);
      entsize = 1;
    }

  // If this is a SHF_MERGE section, we pass all the input sections to
  // a Output_data_merge.  We don't try to handle relocations for such
  // a section.  We don't try to handle empty merge sections--they
  // mess up the mappings, and are useless anyhow.
  if ((sh_flags & elfcpp::SHF_MERGE) != 0
      && reloc_shndx == 0
      && shdr.get_sh_size() > 0)
    {
      if (this->add_merge_input_section(object, shndx, sh_flags,
					entsize, addralign))
	{
	  // Tell the relocation routines that they need to call the
	  // output_offset method to determine the final address.
	  return -1;
	}
    }

  off_t offset_in_section = this->current_data_size_for_child();
  off_t aligned_offset_in_section = align_address(offset_in_section,
                                                  addralign);

  if (aligned_offset_in_section > offset_in_section
      && !have_sections_script
      && (sh_flags & elfcpp::SHF_EXECINSTR) != 0
      && object->target()->has_code_fill())
    {
      // We need to add some fill data.  Using fill_list_ when
      // possible is an optimization, since we will often have fill
      // sections without input sections.
      off_t fill_len = aligned_offset_in_section - offset_in_section;
      if (this->input_sections_.empty())
        this->fills_.push_back(Fill(offset_in_section, fill_len));
      else
        {
          // FIXME: When relaxing, the size needs to adjust to
          // maintain a constant alignment.
          std::string fill_data(object->target()->code_fill(fill_len));
          Output_data_const* odc = new Output_data_const(fill_data, 1);
          this->input_sections_.push_back(Input_section(odc));
        }
    }

  this->set_current_data_size_for_child(aligned_offset_in_section
					+ shdr.get_sh_size());

  // We need to keep track of this section if we are already keeping
  // track of sections, or if we are relaxing.  Also, if this is a
  // section which requires sorting, or which may require sorting in
  // the future, we keep track of the sections.  FIXME: Add test for
  // relaxing.
  if (have_sections_script
      || !this->input_sections_.empty()
      || this->may_sort_attached_input_sections()
      || this->must_sort_attached_input_sections()
      || parameters->options().user_set_Map())
    this->input_sections_.push_back(Input_section(object, shndx,
						  shdr.get_sh_size(),
						  addralign));

  return aligned_offset_in_section;
}

// Add arbitrary data to an output section.

void
Output_section::add_output_section_data(Output_section_data* posd)
{
  Input_section inp(posd);
  this->add_output_section_data(&inp);

  if (posd->is_data_size_valid())
    {
      off_t offset_in_section = this->current_data_size_for_child();
      off_t aligned_offset_in_section = align_address(offset_in_section,
						      posd->addralign());
      this->set_current_data_size_for_child(aligned_offset_in_section
					    + posd->data_size());
    }
}

// Add arbitrary data to an output section by Input_section.

void
Output_section::add_output_section_data(Input_section* inp)
{
  if (this->input_sections_.empty())
    this->first_input_offset_ = this->current_data_size_for_child();

  this->input_sections_.push_back(*inp);

  uint64_t addralign = inp->addralign();
  if (addralign > this->addralign_)
    this->addralign_ = addralign;

  inp->set_output_section(this);
}

// Add a merge section to an output section.

void
Output_section::add_output_merge_section(Output_section_data* posd,
					 bool is_string, uint64_t entsize)
{
  Input_section inp(posd, is_string, entsize);
  this->add_output_section_data(&inp);
}

// Add an input section to a SHF_MERGE section.

bool
Output_section::add_merge_input_section(Relobj* object, unsigned int shndx,
					uint64_t flags, uint64_t entsize,
					uint64_t addralign)
{
  bool is_string = (flags & elfcpp::SHF_STRINGS) != 0;

  // We only merge strings if the alignment is not more than the
  // character size.  This could be handled, but it's unusual.
  if (is_string && addralign > entsize)
    return false;

  Input_section_list::iterator p;
  for (p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    if (p->is_merge_section(is_string, entsize, addralign))
      {
        p->add_input_section(object, shndx);
        return true;
      }

  // We handle the actual constant merging in Output_merge_data or
  // Output_merge_string_data.
  Output_section_data* posd;
  if (!is_string)
    posd = new Output_merge_data(entsize, addralign);
  else
    {
      switch (entsize)
	{
        case 1:
	  posd = new Output_merge_string<char>(addralign);
	  break;
        case 2:
	  posd = new Output_merge_string<uint16_t>(addralign);
	  break;
        case 4:
	  posd = new Output_merge_string<uint32_t>(addralign);
	  break;
        default:
	  return false;
	}
    }

  this->add_output_merge_section(posd, is_string, entsize);
  posd->add_input_section(object, shndx);

  return true;
}

// Given an address OFFSET relative to the start of input section
// SHNDX in OBJECT, return whether this address is being included in
// the final link.  This should only be called if SHNDX in OBJECT has
// a special mapping.

bool
Output_section::is_input_address_mapped(const Relobj* object,
					unsigned int shndx,
					off_t offset) const
{
  for (Input_section_list::const_iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    {
      section_offset_type output_offset;
      if (p->output_offset(object, shndx, offset, &output_offset))
	return output_offset != -1;
    }

  // By default we assume that the address is mapped.  This should
  // only be called after we have passed all sections to Layout.  At
  // that point we should know what we are discarding.
  return true;
}

// Given an address OFFSET relative to the start of input section
// SHNDX in object OBJECT, return the output offset relative to the
// start of the input section in the output section.  This should only
// be called if SHNDX in OBJECT has a special mapping.

section_offset_type
Output_section::output_offset(const Relobj* object, unsigned int shndx,
			      section_offset_type offset) const
{
  // This can only be called meaningfully when layout is complete.
  gold_assert(Output_data::is_layout_complete());

  for (Input_section_list::const_iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    {
      section_offset_type output_offset;
      if (p->output_offset(object, shndx, offset, &output_offset))
	return output_offset;
    }
  gold_unreachable();
}

// Return the output virtual address of OFFSET relative to the start
// of input section SHNDX in object OBJECT.

uint64_t
Output_section::output_address(const Relobj* object, unsigned int shndx,
			       off_t offset) const
{
  uint64_t addr = this->address() + this->first_input_offset_;
  for (Input_section_list::const_iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    {
      addr = align_address(addr, p->addralign());
      section_offset_type output_offset;
      if (p->output_offset(object, shndx, offset, &output_offset))
	{
	  if (output_offset == -1)
	    return -1U;
	  return addr + output_offset;
	}
      addr += p->data_size();
    }

  // If we get here, it means that we don't know the mapping for this
  // input section.  This might happen in principle if
  // add_input_section were called before add_output_section_data.
  // But it should never actually happen.

  gold_unreachable();
}

// Return the output address of the start of the merged section for
// input section SHNDX in object OBJECT.

uint64_t
Output_section::starting_output_address(const Relobj* object,
					unsigned int shndx) const
{
  uint64_t addr = this->address() + this->first_input_offset_;
  for (Input_section_list::const_iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    {
      addr = align_address(addr, p->addralign());

      // It would be nice if we could use the existing output_offset
      // method to get the output offset of input offset 0.
      // Unfortunately we don't know for sure that input offset 0 is
      // mapped at all.
      if (p->is_merge_section_for(object, shndx))
	return addr;

      addr += p->data_size();
    }
  gold_unreachable();
}

// Set the data size of an Output_section.  This is where we handle
// setting the addresses of any Output_section_data objects.

void
Output_section::set_final_data_size()
{
  if (this->input_sections_.empty())
    {
      this->set_data_size(this->current_data_size_for_child());
      return;
    }

  if (this->must_sort_attached_input_sections())
    this->sort_attached_input_sections();

  uint64_t address = this->address();
  off_t startoff = this->offset();
  off_t off = startoff + this->first_input_offset_;
  for (Input_section_list::iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    {
      off = align_address(off, p->addralign());
      p->set_address_and_file_offset(address + (off - startoff), off,
				     startoff);
      off += p->data_size();
    }

  this->set_data_size(off - startoff);
}

// Reset the address and file offset.

void
Output_section::do_reset_address_and_file_offset()
{
  for (Input_section_list::iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    p->reset_address_and_file_offset();
}

// Set the TLS offset.  Called only for SHT_TLS sections.

void
Output_section::do_set_tls_offset(uint64_t tls_base)
{
  this->tls_offset_ = this->address() - tls_base;
}

// In a few cases we need to sort the input sections attached to an
// output section.  This is used to implement the type of constructor
// priority ordering implemented by the GNU linker, in which the
// priority becomes part of the section name and the sections are
// sorted by name.  We only do this for an output section if we see an
// attached input section matching ".ctor.*", ".dtor.*",
// ".init_array.*" or ".fini_array.*".

class Output_section::Input_section_sort_entry
{
 public:
  Input_section_sort_entry()
    : input_section_(), index_(-1U), section_has_name_(false),
      section_name_()
  { }

  Input_section_sort_entry(const Input_section& input_section,
			   unsigned int index)
    : input_section_(input_section), index_(index),
      section_has_name_(input_section.is_input_section())
  {
    if (this->section_has_name_)
      {
	// This is only called single-threaded from Layout::finalize,
	// so it is OK to lock.  Unfortunately we have no way to pass
	// in a Task token.
	const Task* dummy_task = reinterpret_cast<const Task*>(-1);
	Object* obj = input_section.relobj();
	Task_lock_obj<Object> tl(dummy_task, obj);

	// This is a slow operation, which should be cached in
	// Layout::layout if this becomes a speed problem.
	this->section_name_ = obj->section_name(input_section.shndx());
      }
  }

  // Return the Input_section.
  const Input_section&
  input_section() const
  {
    gold_assert(this->index_ != -1U);
    return this->input_section_;
  }

  // The index of this entry in the original list.  This is used to
  // make the sort stable.
  unsigned int
  index() const
  {
    gold_assert(this->index_ != -1U);
    return this->index_;
  }

  // Whether there is a section name.
  bool
  section_has_name() const
  { return this->section_has_name_; }

  // The section name.
  const std::string&
  section_name() const
  {
    gold_assert(this->section_has_name_);
    return this->section_name_;
  }

  // Return true if the section name has a priority.  This is assumed
  // to be true if it has a dot after the initial dot.
  bool
  has_priority() const
  {
    gold_assert(this->section_has_name_);
    return this->section_name_.find('.', 1);
  }

  // Return true if this an input file whose base name matches
  // FILE_NAME.  The base name must have an extension of ".o", and
  // must be exactly FILE_NAME.o or FILE_NAME, one character, ".o".
  // This is to match crtbegin.o as well as crtbeginS.o without
  // getting confused by other possibilities.  Overall matching the
  // file name this way is a dreadful hack, but the GNU linker does it
  // in order to better support gcc, and we need to be compatible.
  bool
  match_file_name(const char* match_file_name) const
  {
    const std::string& file_name(this->input_section_.relobj()->name());
    const char* base_name = lbasename(file_name.c_str());
    size_t match_len = strlen(match_file_name);
    if (strncmp(base_name, match_file_name, match_len) != 0)
      return false;
    size_t base_len = strlen(base_name);
    if (base_len != match_len + 2 && base_len != match_len + 3)
      return false;
    return memcmp(base_name + base_len - 2, ".o", 2) == 0;
  }

 private:
  // The Input_section we are sorting.
  Input_section input_section_;
  // The index of this Input_section in the original list.
  unsigned int index_;
  // Whether this Input_section has a section name--it won't if this
  // is some random Output_section_data.
  bool section_has_name_;
  // The section name if there is one.
  std::string section_name_;
};

// Return true if S1 should come before S2 in the output section.

bool
Output_section::Input_section_sort_compare::operator()(
    const Output_section::Input_section_sort_entry& s1,
    const Output_section::Input_section_sort_entry& s2) const
{
  // crtbegin.o must come first.
  bool s1_begin = s1.match_file_name("crtbegin");
  bool s2_begin = s2.match_file_name("crtbegin");
  if (s1_begin || s2_begin)
    {
      if (!s1_begin)
	return false;
      if (!s2_begin)
	return true;
      return s1.index() < s2.index();
    }

  // crtend.o must come last.
  bool s1_end = s1.match_file_name("crtend");
  bool s2_end = s2.match_file_name("crtend");
  if (s1_end || s2_end)
    {
      if (!s1_end)
	return true;
      if (!s2_end)
	return false;
      return s1.index() < s2.index();
    }

  // We sort all the sections with no names to the end.
  if (!s1.section_has_name() || !s2.section_has_name())
    {
      if (s1.section_has_name())
	return true;
      if (s2.section_has_name())
	return false;
      return s1.index() < s2.index();
    }

  // A section with a priority follows a section without a priority.
  // The GNU linker does this for all but .init_array sections; until
  // further notice we'll assume that that is an mistake.
  bool s1_has_priority = s1.has_priority();
  bool s2_has_priority = s2.has_priority();
  if (s1_has_priority && !s2_has_priority)
    return false;
  if (!s1_has_priority && s2_has_priority)
    return true;

  // Otherwise we sort by name.
  int compare = s1.section_name().compare(s2.section_name());
  if (compare != 0)
    return compare < 0;

  // Otherwise we keep the input order.
  return s1.index() < s2.index();
}

// Sort the input sections attached to an output section.

void
Output_section::sort_attached_input_sections()
{
  if (this->attached_input_sections_are_sorted_)
    return;

  // The only thing we know about an input section is the object and
  // the section index.  We need the section name.  Recomputing this
  // is slow but this is an unusual case.  If this becomes a speed
  // problem we can cache the names as required in Layout::layout.

  // We start by building a larger vector holding a copy of each
  // Input_section, plus its current index in the list and its name.
  std::vector<Input_section_sort_entry> sort_list;

  unsigned int i = 0;
  for (Input_section_list::iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p, ++i)
    sort_list.push_back(Input_section_sort_entry(*p, i));

  // Sort the input sections.
  std::sort(sort_list.begin(), sort_list.end(), Input_section_sort_compare());

  // Copy the sorted input sections back to our list.
  this->input_sections_.clear();
  for (std::vector<Input_section_sort_entry>::iterator p = sort_list.begin();
       p != sort_list.end();
       ++p)
    this->input_sections_.push_back(p->input_section());

  // Remember that we sorted the input sections, since we might get
  // called again.
  this->attached_input_sections_are_sorted_ = true;
}

// Write the section header to *OSHDR.

template<int size, bool big_endian>
void
Output_section::write_header(const Layout* layout,
			     const Stringpool* secnamepool,
			     elfcpp::Shdr_write<size, big_endian>* oshdr) const
{
  oshdr->put_sh_name(secnamepool->get_offset(this->name_));
  oshdr->put_sh_type(this->type_);

  elfcpp::Elf_Xword flags = this->flags_;
  if (this->info_section_ != NULL && this->info_uses_section_index_)
    flags |= elfcpp::SHF_INFO_LINK;
  oshdr->put_sh_flags(flags);

  oshdr->put_sh_addr(this->address());
  oshdr->put_sh_offset(this->offset());
  oshdr->put_sh_size(this->data_size());
  if (this->link_section_ != NULL)
    oshdr->put_sh_link(this->link_section_->out_shndx());
  else if (this->should_link_to_symtab_)
    oshdr->put_sh_link(layout->symtab_section()->out_shndx());
  else if (this->should_link_to_dynsym_)
    oshdr->put_sh_link(layout->dynsym_section()->out_shndx());
  else
    oshdr->put_sh_link(this->link_);

  elfcpp::Elf_Word info;
  if (this->info_section_ != NULL)
    {
      if (this->info_uses_section_index_)
	info = this->info_section_->out_shndx();
      else
	info = this->info_section_->symtab_index();
    }
  else if (this->info_symndx_ != NULL)
    info = this->info_symndx_->symtab_index();
  else
    info = this->info_;
  oshdr->put_sh_info(info);

  oshdr->put_sh_addralign(this->addralign_);
  oshdr->put_sh_entsize(this->entsize_);
}

// Write out the data.  For input sections the data is written out by
// Object::relocate, but we have to handle Output_section_data objects
// here.

void
Output_section::do_write(Output_file* of)
{
  gold_assert(!this->requires_postprocessing());

  off_t output_section_file_offset = this->offset();
  for (Fill_list::iterator p = this->fills_.begin();
       p != this->fills_.end();
       ++p)
    {
      std::string fill_data(parameters->target().code_fill(p->length()));
      of->write(output_section_file_offset + p->section_offset(),
		fill_data.data(), fill_data.size());
    }

  for (Input_section_list::iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    p->write(of);
}

// If a section requires postprocessing, create the buffer to use.

void
Output_section::create_postprocessing_buffer()
{
  gold_assert(this->requires_postprocessing());

  if (this->postprocessing_buffer_ != NULL)
    return;

  if (!this->input_sections_.empty())
    {
      off_t off = this->first_input_offset_;
      for (Input_section_list::iterator p = this->input_sections_.begin();
	   p != this->input_sections_.end();
	   ++p)
	{
	  off = align_address(off, p->addralign());
	  p->finalize_data_size();
	  off += p->data_size();
	}
      this->set_current_data_size_for_child(off);
    }

  off_t buffer_size = this->current_data_size_for_child();
  this->postprocessing_buffer_ = new unsigned char[buffer_size];
}

// Write all the data of an Output_section into the postprocessing
// buffer.  This is used for sections which require postprocessing,
// such as compression.  Input sections are handled by
// Object::Relocate.

void
Output_section::write_to_postprocessing_buffer()
{
  gold_assert(this->requires_postprocessing());

  unsigned char* buffer = this->postprocessing_buffer();
  for (Fill_list::iterator p = this->fills_.begin();
       p != this->fills_.end();
       ++p)
    {
      std::string fill_data(parameters->target().code_fill(p->length()));
      memcpy(buffer + p->section_offset(), fill_data.data(),
	     fill_data.size());
    }

  off_t off = this->first_input_offset_;
  for (Input_section_list::iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    {
      off = align_address(off, p->addralign());
      p->write_to_buffer(buffer + off);
      off += p->data_size();
    }
}

// Get the input sections for linker script processing.  We leave
// behind the Output_section_data entries.  Note that this may be
// slightly incorrect for merge sections.  We will leave them behind,
// but it is possible that the script says that they should follow
// some other input sections, as in:
//    .rodata { *(.rodata) *(.rodata.cst*) }
// For that matter, we don't handle this correctly:
//    .rodata { foo.o(.rodata.cst*) *(.rodata.cst*) }
// With luck this will never matter.

uint64_t
Output_section::get_input_sections(
    uint64_t address,
    const std::string& fill,
    std::list<std::pair<Relobj*, unsigned int> >* input_sections)
{
  uint64_t orig_address = address;

  address = align_address(address, this->addralign());

  Input_section_list remaining;
  for (Input_section_list::iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    {
      if (p->is_input_section())
	input_sections->push_back(std::make_pair(p->relobj(), p->shndx()));
      else
	{
	  uint64_t aligned_address = align_address(address, p->addralign());
	  if (aligned_address != address && !fill.empty())
	    {
	      section_size_type length =
		convert_to_section_size_type(aligned_address - address);
	      std::string this_fill;
	      this_fill.reserve(length);
	      while (this_fill.length() + fill.length() <= length)
		this_fill += fill;
	      if (this_fill.length() < length)
		this_fill.append(fill, 0, length - this_fill.length());

	      Output_section_data* posd = new Output_data_const(this_fill, 0);
	      remaining.push_back(Input_section(posd));
	    }
	  address = aligned_address;

	  remaining.push_back(*p);

	  p->finalize_data_size();
	  address += p->data_size();
	}
    }

  this->input_sections_.swap(remaining);
  this->first_input_offset_ = 0;

  uint64_t data_size = address - orig_address;
  this->set_current_data_size_for_child(data_size);
  return data_size;
}

// Add an input section from a script.

void
Output_section::add_input_section_for_script(Relobj* object,
					     unsigned int shndx,
					     off_t data_size,
					     uint64_t addralign)
{
  if (addralign > this->addralign_)
    this->addralign_ = addralign;

  off_t offset_in_section = this->current_data_size_for_child();
  off_t aligned_offset_in_section = align_address(offset_in_section,
						  addralign);

  this->set_current_data_size_for_child(aligned_offset_in_section
					+ data_size);

  this->input_sections_.push_back(Input_section(object, shndx,
						data_size, addralign));
}

// Print to the map file.

void
Output_section::do_print_to_mapfile(Mapfile* mapfile) const
{
  mapfile->print_output_section(this);

  for (Input_section_list::const_iterator p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    p->print_to_mapfile(mapfile);
}

// Print stats for merge sections to stderr.

void
Output_section::print_merge_stats()
{
  Input_section_list::iterator p;
  for (p = this->input_sections_.begin();
       p != this->input_sections_.end();
       ++p)
    p->print_merge_stats(this->name_);
}

// Output segment methods.

Output_segment::Output_segment(elfcpp::Elf_Word type, elfcpp::Elf_Word flags)
  : output_data_(),
    output_bss_(),
    vaddr_(0),
    paddr_(0),
    memsz_(0),
    max_align_(0),
    min_p_align_(0),
    offset_(0),
    filesz_(0),
    type_(type),
    flags_(flags),
    is_max_align_known_(false),
    are_addresses_set_(false)
{
}

// Add an Output_section to an Output_segment.

void
Output_segment::add_output_section(Output_section* os,
				   elfcpp::Elf_Word seg_flags)
{
  gold_assert((os->flags() & elfcpp::SHF_ALLOC) != 0);
  gold_assert(!this->is_max_align_known_);

  // Update the segment flags.
  this->flags_ |= seg_flags;

  Output_segment::Output_data_list* pdl;
  if (os->type() == elfcpp::SHT_NOBITS)
    pdl = &this->output_bss_;
  else
    pdl = &this->output_data_;

  // So that PT_NOTE segments will work correctly, we need to ensure
  // that all SHT_NOTE sections are adjacent.  This will normally
  // happen automatically, because all the SHT_NOTE input sections
  // will wind up in the same output section.  However, it is possible
  // for multiple SHT_NOTE input sections to have different section
  // flags, and thus be in different output sections, but for the
  // different section flags to map into the same segment flags and
  // thus the same output segment.

  // Note that while there may be many input sections in an output
  // section, there are normally only a few output sections in an
  // output segment.  This loop is expected to be fast.

  if (os->type() == elfcpp::SHT_NOTE && !pdl->empty())
    {
      Output_segment::Output_data_list::iterator p = pdl->end();
      do
	{
	  --p;
	  if ((*p)->is_section_type(elfcpp::SHT_NOTE))
	    {
	      ++p;
	      pdl->insert(p, os);
	      return;
	    }
	}
      while (p != pdl->begin());
    }

  // Similarly, so that PT_TLS segments will work, we need to group
  // SHF_TLS sections.  An SHF_TLS/SHT_NOBITS section is a special
  // case: we group the SHF_TLS/SHT_NOBITS sections right after the
  // SHF_TLS/SHT_PROGBITS sections.  This lets us set up PT_TLS
  // correctly.  SHF_TLS sections get added to both a PT_LOAD segment
  // and the PT_TLS segment -- we do this grouping only for the
  // PT_LOAD segment.
  if (this->type_ != elfcpp::PT_TLS
      && (os->flags() & elfcpp::SHF_TLS) != 0)
    {
      pdl = &this->output_data_;
      bool nobits = os->type() == elfcpp::SHT_NOBITS;
      bool sawtls = false;
      Output_segment::Output_data_list::iterator p = pdl->end();
      do
	{
	  --p;
	  bool insert;
	  if ((*p)->is_section_flag_set(elfcpp::SHF_TLS))
	    {
	      sawtls = true;
	      // Put a NOBITS section after the first TLS section.
	      // Put a PROGBITS section after the first TLS/PROGBITS
	      // section.
	      insert = nobits || !(*p)->is_section_type(elfcpp::SHT_NOBITS);
	    }
	  else
	    {
	      // If we've gone past the TLS sections, but we've seen a
	      // TLS section, then we need to insert this section now.
	      insert = sawtls;
	    }

	  if (insert)
	    {
	      ++p;
	      pdl->insert(p, os);
	      return;
	    }
	}
      while (p != pdl->begin());

      // There are no TLS sections yet; put this one at the requested
      // location in the section list.
    }

  // For the PT_GNU_RELRO segment, we need to group relro sections,
  // and we need to put them before any non-relro sections.  Also,
  // relro local sections go before relro non-local sections.
  if (parameters->options().relro() && os->is_relro())
    {
      gold_assert(pdl == &this->output_data_);
      Output_segment::Output_data_list::iterator p;
      for (p = pdl->begin(); p != pdl->end(); ++p)
	{
	  if (!(*p)->is_section())
	    break;

	  Output_section* pos = (*p)->output_section();
	  if (!pos->is_relro()
	      || (os->is_relro_local() && !pos->is_relro_local()))
	    break;
	}

      pdl->insert(p, os);
      return;
    }

  pdl->push_back(os);
}

// Remove an Output_section from this segment.  It is an error if it
// is not present.

void
Output_segment::remove_output_section(Output_section* os)
{
  // We only need this for SHT_PROGBITS.
  gold_assert(os->type() == elfcpp::SHT_PROGBITS);
  for (Output_data_list::iterator p = this->output_data_.begin();
       p != this->output_data_.end();
       ++p)
   {
     if (*p == os)
       {
         this->output_data_.erase(p);
         return;
       }
   }
  gold_unreachable();
}

// Add an Output_data (which is not an Output_section) to the start of
// a segment.

void
Output_segment::add_initial_output_data(Output_data* od)
{
  gold_assert(!this->is_max_align_known_);
  this->output_data_.push_front(od);
}

// Return whether the first data section is a relro section.

bool
Output_segment::is_first_section_relro() const
{
  return (!this->output_data_.empty()
	  && this->output_data_.front()->is_section()
	  && this->output_data_.front()->output_section()->is_relro());
}

// Return the maximum alignment of the Output_data in Output_segment.

uint64_t
Output_segment::maximum_alignment()
{
  if (!this->is_max_align_known_)
    {
      uint64_t addralign;

      addralign = Output_segment::maximum_alignment_list(&this->output_data_);
      if (addralign > this->max_align_)
	this->max_align_ = addralign;

      addralign = Output_segment::maximum_alignment_list(&this->output_bss_);
      if (addralign > this->max_align_)
	this->max_align_ = addralign;

      // If -z relro is in effect, and the first section in this
      // segment is a relro section, then the segment must be aligned
      // to at least the common page size.  This ensures that the
      // PT_GNU_RELRO segment will start at a page boundary.
      if (this->type_ == elfcpp::PT_LOAD
	  && parameters->options().relro()
	  && this->is_first_section_relro())
	{
	  addralign = parameters->target().common_pagesize();
	  if (addralign > this->max_align_)
	    this->max_align_ = addralign;
	}

      this->is_max_align_known_ = true;
    }

  return this->max_align_;
}

// Return the maximum alignment of a list of Output_data.

uint64_t
Output_segment::maximum_alignment_list(const Output_data_list* pdl)
{
  uint64_t ret = 0;
  for (Output_data_list::const_iterator p = pdl->begin();
       p != pdl->end();
       ++p)
    {
      uint64_t addralign = (*p)->addralign();
      if (addralign > ret)
	ret = addralign;
    }
  return ret;
}

// Return the number of dynamic relocs applied to this segment.

unsigned int
Output_segment::dynamic_reloc_count() const
{
  return (this->dynamic_reloc_count_list(&this->output_data_)
	  + this->dynamic_reloc_count_list(&this->output_bss_));
}

// Return the number of dynamic relocs applied to an Output_data_list.

unsigned int
Output_segment::dynamic_reloc_count_list(const Output_data_list* pdl) const
{
  unsigned int count = 0;
  for (Output_data_list::const_iterator p = pdl->begin();
       p != pdl->end();
       ++p)
    count += (*p)->dynamic_reloc_count();
  return count;
}

// Set the section addresses for an Output_segment.  If RESET is true,
// reset the addresses first.  ADDR is the address and *POFF is the
// file offset.  Set the section indexes starting with *PSHNDX.
// Return the address of the immediately following segment.  Update
// *POFF and *PSHNDX.

uint64_t
Output_segment::set_section_addresses(const Layout* layout, bool reset,
                                      uint64_t addr, off_t* poff,
				      unsigned int* pshndx)
{
  gold_assert(this->type_ == elfcpp::PT_LOAD);

  if (!reset && this->are_addresses_set_)
    {
      gold_assert(this->paddr_ == addr);
      addr = this->vaddr_;
    }
  else
    {
      this->vaddr_ = addr;
      this->paddr_ = addr;
      this->are_addresses_set_ = true;
    }

  bool in_tls = false;

  bool in_relro = (parameters->options().relro()
		   && this->is_first_section_relro());

  off_t orig_off = *poff;
  this->offset_ = orig_off;

  addr = this->set_section_list_addresses(layout, reset, &this->output_data_,
					  addr, poff, pshndx, &in_tls,
					  &in_relro);
  this->filesz_ = *poff - orig_off;

  off_t off = *poff;

  uint64_t ret = this->set_section_list_addresses(layout, reset,
                                                  &this->output_bss_,
						  addr, poff, pshndx,
                                                  &in_tls, &in_relro);

  // If the last section was a TLS section, align upward to the
  // alignment of the TLS segment, so that the overall size of the TLS
  // segment is aligned.
  if (in_tls)
    {
      uint64_t segment_align = layout->tls_segment()->maximum_alignment();
      *poff = align_address(*poff, segment_align);
    }

  // If all the sections were relro sections, align upward to the
  // common page size.
  if (in_relro)
    {
      uint64_t page_align = parameters->target().common_pagesize();
      *poff = align_address(*poff, page_align);
    }

  this->memsz_ = *poff - orig_off;

  // Ignore the file offset adjustments made by the BSS Output_data
  // objects.
  *poff = off;

  return ret;
}

// Set the addresses and file offsets in a list of Output_data
// structures.

uint64_t
Output_segment::set_section_list_addresses(const Layout* layout, bool reset,
                                           Output_data_list* pdl,
					   uint64_t addr, off_t* poff,
					   unsigned int* pshndx,
                                           bool* in_tls, bool* in_relro)
{
  off_t startoff = *poff;

  off_t off = startoff;
  for (Output_data_list::iterator p = pdl->begin();
       p != pdl->end();
       ++p)
    {
      if (reset)
	(*p)->reset_address_and_file_offset();

      // When using a linker script the section will most likely
      // already have an address.
      if (!(*p)->is_address_valid())
	{
          uint64_t align = (*p)->addralign();

          if ((*p)->is_section_flag_set(elfcpp::SHF_TLS))
            {
              // Give the first TLS section the alignment of the
              // entire TLS segment.  Otherwise the TLS segment as a
              // whole may be misaligned.
              if (!*in_tls)
                {
                  Output_segment* tls_segment = layout->tls_segment();
                  gold_assert(tls_segment != NULL);
                  uint64_t segment_align = tls_segment->maximum_alignment();
                  gold_assert(segment_align >= align);
                  align = segment_align;

                  *in_tls = true;
                }
            }
          else
            {
              // If this is the first section after the TLS segment,
              // align it to at least the alignment of the TLS
              // segment, so that the size of the overall TLS segment
              // is aligned.
              if (*in_tls)
                {
                  uint64_t segment_align =
                      layout->tls_segment()->maximum_alignment();
                  if (segment_align > align)
                    align = segment_align;

                  *in_tls = false;
                }
            }

	  // If this is a non-relro section after a relro section,
	  // align it to a common page boundary so that the dynamic
	  // linker has a page to mark as read-only.
	  if (*in_relro
	      && (!(*p)->is_section()
		  || !(*p)->output_section()->is_relro()))
	    {
	      uint64_t page_align = parameters->target().common_pagesize();
	      if (page_align > align)
		align = page_align;
	      *in_relro = false;
	    }

	  off = align_address(off, align);
	  (*p)->set_address_and_file_offset(addr + (off - startoff), off);
	}
      else
	{
	  // The script may have inserted a skip forward, but it
	  // better not have moved backward.
	  gold_assert((*p)->address() >= addr + (off - startoff));
	  off += (*p)->address() - (addr + (off - startoff));
	  (*p)->set_file_offset(off);
	  (*p)->finalize_data_size();
	}

      // We want to ignore the size of a SHF_TLS or SHT_NOBITS
      // section.  Such a section does not affect the size of a
      // PT_LOAD segment.
      if (!(*p)->is_section_flag_set(elfcpp::SHF_TLS)
	  || !(*p)->is_section_type(elfcpp::SHT_NOBITS))
	off += (*p)->data_size();

      if ((*p)->is_section())
	{
	  (*p)->set_out_shndx(*pshndx);
	  ++*pshndx;
	}
    }

  *poff = off;
  return addr + (off - startoff);
}

// For a non-PT_LOAD segment, set the offset from the sections, if
// any.

void
Output_segment::set_offset()
{
  gold_assert(this->type_ != elfcpp::PT_LOAD);

  gold_assert(!this->are_addresses_set_);

  if (this->output_data_.empty() && this->output_bss_.empty())
    {
      this->vaddr_ = 0;
      this->paddr_ = 0;
      this->are_addresses_set_ = true;
      this->memsz_ = 0;
      this->min_p_align_ = 0;
      this->offset_ = 0;
      this->filesz_ = 0;
      return;
    }

  const Output_data* first;
  if (this->output_data_.empty())
    first = this->output_bss_.front();
  else
    first = this->output_data_.front();
  this->vaddr_ = first->address();
  this->paddr_ = (first->has_load_address()
		  ? first->load_address()
		  : this->vaddr_);
  this->are_addresses_set_ = true;
  this->offset_ = first->offset();

  if (this->output_data_.empty())
    this->filesz_ = 0;
  else
    {
      const Output_data* last_data = this->output_data_.back();
      this->filesz_ = (last_data->address()
		       + last_data->data_size()
		       - this->vaddr_);
    }

  const Output_data* last;
  if (this->output_bss_.empty())
    last = this->output_data_.back();
  else
    last = this->output_bss_.back();
  this->memsz_ = (last->address()
		  + last->data_size()
		  - this->vaddr_);

  // If this is a TLS segment, align the memory size.  The code in
  // set_section_list ensures that the section after the TLS segment
  // is aligned to give us room.
  if (this->type_ == elfcpp::PT_TLS)
    {
      uint64_t segment_align = this->maximum_alignment();
      gold_assert(this->vaddr_ == align_address(this->vaddr_, segment_align));
      this->memsz_ = align_address(this->memsz_, segment_align);
    }

  // If this is a RELRO segment, align the memory size.  The code in
  // set_section_list ensures that the section after the RELRO segment
  // is aligned to give us room.
  if (this->type_ == elfcpp::PT_GNU_RELRO)
    {
      uint64_t page_align = parameters->target().common_pagesize();
      gold_assert(this->vaddr_ == align_address(this->vaddr_, page_align));
      this->memsz_ = align_address(this->memsz_, page_align);
    }
}

// Set the TLS offsets of the sections in the PT_TLS segment.

void
Output_segment::set_tls_offsets()
{
  gold_assert(this->type_ == elfcpp::PT_TLS);

  for (Output_data_list::iterator p = this->output_data_.begin();
       p != this->output_data_.end();
       ++p)
    (*p)->set_tls_offset(this->vaddr_);

  for (Output_data_list::iterator p = this->output_bss_.begin();
       p != this->output_bss_.end();
       ++p)
    (*p)->set_tls_offset(this->vaddr_);
}

// Return the address of the first section.

uint64_t
Output_segment::first_section_load_address() const
{
  for (Output_data_list::const_iterator p = this->output_data_.begin();
       p != this->output_data_.end();
       ++p)
    if ((*p)->is_section())
      return (*p)->has_load_address() ? (*p)->load_address() : (*p)->address();

  for (Output_data_list::const_iterator p = this->output_bss_.begin();
       p != this->output_bss_.end();
       ++p)
    if ((*p)->is_section())
      return (*p)->has_load_address() ? (*p)->load_address() : (*p)->address();

  gold_unreachable();
}

// Return the number of Output_sections in an Output_segment.

unsigned int
Output_segment::output_section_count() const
{
  return (this->output_section_count_list(&this->output_data_)
	  + this->output_section_count_list(&this->output_bss_));
}

// Return the number of Output_sections in an Output_data_list.

unsigned int
Output_segment::output_section_count_list(const Output_data_list* pdl) const
{
  unsigned int count = 0;
  for (Output_data_list::const_iterator p = pdl->begin();
       p != pdl->end();
       ++p)
    {
      if ((*p)->is_section())
	++count;
    }
  return count;
}

// Return the section attached to the list segment with the lowest
// load address.  This is used when handling a PHDRS clause in a
// linker script.

Output_section*
Output_segment::section_with_lowest_load_address() const
{
  Output_section* found = NULL;
  uint64_t found_lma = 0;
  this->lowest_load_address_in_list(&this->output_data_, &found, &found_lma);

  Output_section* found_data = found;
  this->lowest_load_address_in_list(&this->output_bss_, &found, &found_lma);
  if (found != found_data && found_data != NULL)
    {
      gold_error(_("nobits section %s may not precede progbits section %s "
		   "in same segment"),
		 found->name(), found_data->name());
      return NULL;
    }

  return found;
}

// Look through a list for a section with a lower load address.

void
Output_segment::lowest_load_address_in_list(const Output_data_list* pdl,
					    Output_section** found,
					    uint64_t* found_lma) const
{
  for (Output_data_list::const_iterator p = pdl->begin();
       p != pdl->end();
       ++p)
    {
      if (!(*p)->is_section())
	continue;
      Output_section* os = static_cast<Output_section*>(*p);
      uint64_t lma = (os->has_load_address()
		      ? os->load_address()
		      : os->address());
      if (*found == NULL || lma < *found_lma)
	{
	  *found = os;
	  *found_lma = lma;
	}
    }
}

// Write the segment data into *OPHDR.

template<int size, bool big_endian>
void
Output_segment::write_header(elfcpp::Phdr_write<size, big_endian>* ophdr)
{
  ophdr->put_p_type(this->type_);
  ophdr->put_p_offset(this->offset_);
  ophdr->put_p_vaddr(this->vaddr_);
  ophdr->put_p_paddr(this->paddr_);
  ophdr->put_p_filesz(this->filesz_);
  ophdr->put_p_memsz(this->memsz_);
  ophdr->put_p_flags(this->flags_);
  ophdr->put_p_align(std::max(this->min_p_align_, this->maximum_alignment()));
}

// Write the section headers into V.

template<int size, bool big_endian>
unsigned char*
Output_segment::write_section_headers(const Layout* layout,
				      const Stringpool* secnamepool,
				      unsigned char* v,
				      unsigned int *pshndx) const
{
  // Every section that is attached to a segment must be attached to a
  // PT_LOAD segment, so we only write out section headers for PT_LOAD
  // segments.
  if (this->type_ != elfcpp::PT_LOAD)
    return v;

  v = this->write_section_headers_list<size, big_endian>(layout, secnamepool,
							 &this->output_data_,
							 v, pshndx);
  v = this->write_section_headers_list<size, big_endian>(layout, secnamepool,
							 &this->output_bss_,
							 v, pshndx);
  return v;
}

template<int size, bool big_endian>
unsigned char*
Output_segment::write_section_headers_list(const Layout* layout,
					   const Stringpool* secnamepool,
					   const Output_data_list* pdl,
					   unsigned char* v,
					   unsigned int* pshndx) const
{
  const int shdr_size = elfcpp::Elf_sizes<size>::shdr_size;
  for (Output_data_list::const_iterator p = pdl->begin();
       p != pdl->end();
       ++p)
    {
      if ((*p)->is_section())
	{
	  const Output_section* ps = static_cast<const Output_section*>(*p);
	  gold_assert(*pshndx == ps->out_shndx());
	  elfcpp::Shdr_write<size, big_endian> oshdr(v);
	  ps->write_header(layout, secnamepool, &oshdr);
	  v += shdr_size;
	  ++*pshndx;
	}
    }
  return v;
}

// Print the output sections to the map file.

void
Output_segment::print_sections_to_mapfile(Mapfile* mapfile) const
{
  if (this->type() != elfcpp::PT_LOAD)
    return;
  this->print_section_list_to_mapfile(mapfile, &this->output_data_);
  this->print_section_list_to_mapfile(mapfile, &this->output_bss_);
}

// Print an output section list to the map file.

void
Output_segment::print_section_list_to_mapfile(Mapfile* mapfile,
					      const Output_data_list* pdl) const
{
  for (Output_data_list::const_iterator p = pdl->begin();
       p != pdl->end();
       ++p)
    (*p)->print_to_mapfile(mapfile);
}

// Output_file methods.

Output_file::Output_file(const char* name)
  : name_(name),
    o_(-1),
    file_size_(0),
    base_(NULL),
    map_is_anonymous_(false),
    is_temporary_(false)
{
}

// Open the output file.

void
Output_file::open(off_t file_size)
{
  this->file_size_ = file_size;

  // Unlink the file first; otherwise the open() may fail if the file
  // is busy (e.g. it's an executable that's currently being executed).
  //
  // However, the linker may be part of a system where a zero-length
  // file is created for it to write to, with tight permissions (gcc
  // 2.95 did something like this).  Unlinking the file would work
  // around those permission controls, so we only unlink if the file
  // has a non-zero size.  We also unlink only regular files to avoid
  // trouble with directories/etc.
  //
  // If we fail, continue; this command is merely a best-effort attempt
  // to improve the odds for open().

  // We let the name "-" mean "stdout"
  if (!this->is_temporary_)
    {
      if (strcmp(this->name_, "-") == 0)
	this->o_ = STDOUT_FILENO;
      else
	{
	  struct stat s;
	  if (::stat(this->name_, &s) == 0 && s.st_size != 0)
	    unlink_if_ordinary(this->name_);

	  int mode = parameters->options().relocatable() ? 0666 : 0777;
	  int o = open_descriptor(-1, this->name_, O_RDWR | O_CREAT | O_TRUNC,
				  mode);
	  if (o < 0)
	    gold_fatal(_("%s: open: %s"), this->name_, strerror(errno));
	  this->o_ = o;
	}
    }

  this->map();
}

// Resize the output file.

void
Output_file::resize(off_t file_size)
{
  // If the mmap is mapping an anonymous memory buffer, this is easy:
  // just mremap to the new size.  If it's mapping to a file, we want
  // to unmap to flush to the file, then remap after growing the file.
  if (this->map_is_anonymous_)
    {
      void* base = ::mremap(this->base_, this->file_size_, file_size,
                            MREMAP_MAYMOVE);
      if (base == MAP_FAILED)
        gold_fatal(_("%s: mremap: %s"), this->name_, strerror(errno));
      this->base_ = static_cast<unsigned char*>(base);
      this->file_size_ = file_size;
    }
  else
    {
      this->unmap();
      this->file_size_ = file_size;
      this->map();
    }
}

// Map the file into memory.

void
Output_file::map()
{
  const int o = this->o_;

  // If the output file is not a regular file, don't try to mmap it;
  // instead, we'll mmap a block of memory (an anonymous buffer), and
  // then later write the buffer to the file.
  void* base;
  struct stat statbuf;
  if (o == STDOUT_FILENO || o == STDERR_FILENO
      || ::fstat(o, &statbuf) != 0
      || !S_ISREG(statbuf.st_mode)
      || this->is_temporary_)
    {
      this->map_is_anonymous_ = true;
      base = ::mmap(NULL, this->file_size_, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
  else
    {
      // Write out one byte to make the file the right size.
      if (::lseek(o, this->file_size_ - 1, SEEK_SET) < 0)
        gold_fatal(_("%s: lseek: %s"), this->name_, strerror(errno));
      char b = 0;
      if (::write(o, &b, 1) != 1)
        gold_fatal(_("%s: write: %s"), this->name_, strerror(errno));

      // Map the file into memory.
      this->map_is_anonymous_ = false;
      base = ::mmap(NULL, this->file_size_, PROT_READ | PROT_WRITE,
                    MAP_SHARED, o, 0);
    }
  if (base == MAP_FAILED)
    gold_fatal(_("%s: mmap: %s"), this->name_, strerror(errno));
  this->base_ = static_cast<unsigned char*>(base);
}

// Unmap the file from memory.

void
Output_file::unmap()
{
  if (::munmap(this->base_, this->file_size_) < 0)
    gold_error(_("%s: munmap: %s"), this->name_, strerror(errno));
  this->base_ = NULL;
}

// Close the output file.

void
Output_file::close()
{
  // If the map isn't file-backed, we need to write it now.
  if (this->map_is_anonymous_ && !this->is_temporary_)
    {
      size_t bytes_to_write = this->file_size_;
      while (bytes_to_write > 0)
        {
          ssize_t bytes_written = ::write(this->o_, this->base_, bytes_to_write);
          if (bytes_written == 0)
            gold_error(_("%s: write: unexpected 0 return-value"), this->name_);
          else if (bytes_written < 0)
            gold_error(_("%s: write: %s"), this->name_, strerror(errno));
          else
            bytes_to_write -= bytes_written;
        }
    }
  this->unmap();

  // We don't close stdout or stderr
  if (this->o_ != STDOUT_FILENO
      && this->o_ != STDERR_FILENO
      && !this->is_temporary_)
    if (::close(this->o_) < 0)
      gold_error(_("%s: close: %s"), this->name_, strerror(errno));
  this->o_ = -1;
}

// Instantiate the templates we need.  We could use the configure
// script to restrict this to only the ones for implemented targets.

#ifdef HAVE_TARGET_32_LITTLE
template
off_t
Output_section::add_input_section<32, false>(
    Sized_relobj<32, false>* object,
    unsigned int shndx,
    const char* secname,
    const elfcpp::Shdr<32, false>& shdr,
    unsigned int reloc_shndx,
    bool have_sections_script);
#endif

#ifdef HAVE_TARGET_32_BIG
template
off_t
Output_section::add_input_section<32, true>(
    Sized_relobj<32, true>* object,
    unsigned int shndx,
    const char* secname,
    const elfcpp::Shdr<32, true>& shdr,
    unsigned int reloc_shndx,
    bool have_sections_script);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
off_t
Output_section::add_input_section<64, false>(
    Sized_relobj<64, false>* object,
    unsigned int shndx,
    const char* secname,
    const elfcpp::Shdr<64, false>& shdr,
    unsigned int reloc_shndx,
    bool have_sections_script);
#endif

#ifdef HAVE_TARGET_64_BIG
template
off_t
Output_section::add_input_section<64, true>(
    Sized_relobj<64, true>* object,
    unsigned int shndx,
    const char* secname,
    const elfcpp::Shdr<64, true>& shdr,
    unsigned int reloc_shndx,
    bool have_sections_script);
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Output_data_reloc<elfcpp::SHT_REL, false, 32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Output_data_reloc<elfcpp::SHT_REL, false, 32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Output_data_reloc<elfcpp::SHT_REL, false, 64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Output_data_reloc<elfcpp::SHT_REL, false, 64, true>;
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Output_data_reloc<elfcpp::SHT_REL, true, 32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Output_data_reloc<elfcpp::SHT_REL, true, 32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Output_data_reloc<elfcpp::SHT_REL, true, 64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Output_data_reloc<elfcpp::SHT_REL, true, 64, true>;
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Output_data_reloc<elfcpp::SHT_RELA, false, 32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Output_data_reloc<elfcpp::SHT_RELA, false, 32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Output_data_reloc<elfcpp::SHT_RELA, false, 64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Output_data_reloc<elfcpp::SHT_RELA, false, 64, true>;
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Output_data_reloc<elfcpp::SHT_RELA, true, 32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Output_data_reloc<elfcpp::SHT_RELA, true, 32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Output_data_reloc<elfcpp::SHT_RELA, true, 64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Output_data_reloc<elfcpp::SHT_RELA, true, 64, true>;
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Output_relocatable_relocs<elfcpp::SHT_REL, 32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Output_relocatable_relocs<elfcpp::SHT_REL, 32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Output_relocatable_relocs<elfcpp::SHT_REL, 64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Output_relocatable_relocs<elfcpp::SHT_REL, 64, true>;
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Output_relocatable_relocs<elfcpp::SHT_RELA, 32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Output_relocatable_relocs<elfcpp::SHT_RELA, 32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Output_relocatable_relocs<elfcpp::SHT_RELA, 64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Output_relocatable_relocs<elfcpp::SHT_RELA, 64, true>;
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Output_data_group<32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Output_data_group<32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Output_data_group<64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Output_data_group<64, true>;
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Output_data_got<32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Output_data_got<32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Output_data_got<64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Output_data_got<64, true>;
#endif

} // End namespace gold.
