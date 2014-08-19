// copy-relocs.cc -- handle COPY relocations for gold.

// Copyright 2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
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

#include "symtab.h"
#include "copy-relocs.h"

namespace gold
{

// Copy_relocs::Copy_reloc_entry methods.

// Emit the reloc if appropriate.

template<int sh_type, int size, bool big_endian>
void
Copy_relocs<sh_type, size, big_endian>::Copy_reloc_entry::emit(
    Output_data_reloc<sh_type, true, size, big_endian>* reloc_section)
{
  // If the symbol is no longer defined in a dynamic object, then we
  // emitted a COPY relocation, and we do not want to emit this
  // dynamic relocation.
  if (this->sym_->is_from_dynobj())
    reloc_section->add_global_generic(this->sym_, this->reloc_type_,
				      this->output_section_, this->relobj_,
				      this->shndx_, this->address_,
				      this->addend_);
}

// Copy_relocs methods.

// Handle a relocation against a symbol which may force us to generate
// a COPY reloc.

template<int sh_type, int size, bool big_endian>
void
Copy_relocs<sh_type, size, big_endian>::copy_reloc(
    Symbol_table* symtab,
    Layout* layout,
    Sized_symbol<size>* sym,
    Sized_relobj_file<size, big_endian>* object,
    unsigned int shndx,
    Output_section* output_section,
    const Reloc& rel,
    Output_data_reloc<sh_type, true, size, big_endian>* reloc_section)
{
  if (this->need_copy_reloc(sym, object, shndx))
    this->make_copy_reloc(symtab, layout, sym, reloc_section);
  else
    {
      // We may not need a COPY relocation.  Save this relocation to
      // possibly be emitted later.
      this->save(sym, object, shndx, output_section, rel);
    }
}

// Return whether we need a COPY reloc for a relocation against SYM.
// The relocation is begin applied to section SHNDX in OBJECT.

template<int sh_type, int size, bool big_endian>
bool
Copy_relocs<sh_type, size, big_endian>::need_copy_reloc(
    Sized_symbol<size>* sym,
    Sized_relobj_file<size, big_endian>* object,
    unsigned int shndx) const
{
  if (!parameters->options().copyreloc())
    return false;

  if (sym->symsize() == 0)
    return false;

  // If this is a readonly section, then we need a COPY reloc.
  // Otherwise we can use a dynamic reloc.  Note that calling
  // section_flags here can be slow, as the information is not cached;
  // fortunately we shouldn't see too many potential COPY relocs.
  if ((object->section_flags(shndx) & elfcpp::SHF_WRITE) == 0)
    return true;

  return false;
}

// Emit a COPY relocation for SYM.

template<int sh_type, int size, bool big_endian>
void
Copy_relocs<sh_type, size, big_endian>::emit_copy_reloc(
    Symbol_table* symtab,
    Sized_symbol<size>* sym,
    Output_data* posd,
    off_t offset,
    Output_data_reloc<sh_type, true, size, big_endian>* reloc_section)
{
  // Define the symbol as being copied.
  symtab->define_with_copy_reloc(sym, posd, offset);

  // Add the COPY relocation to the dynamic reloc section.
  reloc_section->add_global_generic(sym, this->copy_reloc_type_, posd,
				    offset, 0);
}

// Make a COPY relocation for SYM and emit it.

template<int sh_type, int size, bool big_endian>
void
Copy_relocs<sh_type, size, big_endian>::make_copy_reloc(
    Symbol_table* symtab,
    Layout* layout,
    Sized_symbol<size>* sym,
    Output_data_reloc<sh_type, true, size, big_endian>* reloc_section)
{
  // We should not be here if -z nocopyreloc is given.
  gold_assert(parameters->options().copyreloc());

  typename elfcpp::Elf_types<size>::Elf_WXword symsize = sym->symsize();

  // There is no defined way to determine the required alignment of
  // the symbol.  We know that the symbol is defined in a dynamic
  // object.  We start with the alignment of the section in which it
  // is defined; presumably we do not require an alignment larger than
  // that.  Then we reduce that alignment if the symbol is not aligned
  // within the section.
  gold_assert(sym->is_from_dynobj());
  bool is_ordinary;
  unsigned int shndx = sym->shndx(&is_ordinary);
  gold_assert(is_ordinary);
  typename elfcpp::Elf_types<size>::Elf_WXword addralign;

  {
    // Lock the object so we can read from it.  This is only called
    // single-threaded from scan_relocs, so it is OK to lock.
    // Unfortunately we have no way to pass in a Task token.
    const Task* dummy_task = reinterpret_cast<const Task*>(-1);
    Object* obj = sym->object();
    Task_lock_obj<Object> tl(dummy_task, obj);
    addralign = obj->section_addralign(shndx);
  }

  typename Sized_symbol<size>::Value_type value = sym->value();
  while ((value & (addralign - 1)) != 0)
    addralign >>= 1;

  // Mark the dynamic object as needed for the --as-needed option.
  sym->object()->set_is_needed();

  if (this->dynbss_ == NULL)
    {
      this->dynbss_ = new Output_data_space(addralign, "** dynbss");
      layout->add_output_section_data(".bss",
				      elfcpp::SHT_NOBITS,
				      elfcpp::SHF_ALLOC | elfcpp::SHF_WRITE,
				      this->dynbss_, ORDER_BSS, false);
    }

  Output_data_space* dynbss = this->dynbss_;

  if (addralign > dynbss->addralign())
    dynbss->set_space_alignment(addralign);

  section_size_type dynbss_size =
    convert_to_section_size_type(dynbss->current_data_size());
  dynbss_size = align_address(dynbss_size, addralign);
  section_size_type offset = dynbss_size;
  dynbss->set_current_data_size(dynbss_size + symsize);

  this->emit_copy_reloc(symtab, sym, dynbss, offset, reloc_section);
}

// Save a relocation to possibly be emitted later.

template<int sh_type, int size, bool big_endian>
void
Copy_relocs<sh_type, size, big_endian>::save(
    Symbol* sym,
    Sized_relobj_file<size, big_endian>* object,
    unsigned int shndx,
    Output_section* output_section,
    const Reloc& rel)
{
  unsigned int reloc_type = elfcpp::elf_r_type<size>(rel.get_r_info());
  typename elfcpp::Elf_types<size>::Elf_Addr addend =
    Reloc_types<sh_type, size, big_endian>::get_reloc_addend_noerror(&rel);
  this->entries_.push_back(Copy_reloc_entry(sym, reloc_type, object, shndx,
					    output_section, rel.get_r_offset(),
					    addend));
}

// Emit any saved relocs.

template<int sh_type, int size, bool big_endian>
void
Copy_relocs<sh_type, size, big_endian>::emit(
    Output_data_reloc<sh_type, true, size, big_endian>* reloc_section)
{
  for (typename Copy_reloc_entries::iterator p = this->entries_.begin();
       p != this->entries_.end();
       ++p)
    p->emit(reloc_section);

  // We no longer need the saved information.
  this->entries_.clear();
}

// Instantiate the templates we need.

#ifdef HAVE_TARGET_32_LITTLE
template
class Copy_relocs<elfcpp::SHT_REL, 32, false>;

template
class Copy_relocs<elfcpp::SHT_RELA, 32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Copy_relocs<elfcpp::SHT_REL, 32, true>;

template
class Copy_relocs<elfcpp::SHT_RELA, 32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Copy_relocs<elfcpp::SHT_REL, 64, false>;

template
class Copy_relocs<elfcpp::SHT_RELA, 64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Copy_relocs<elfcpp::SHT_REL, 64, true>;

template
class Copy_relocs<elfcpp::SHT_RELA, 64, true>;
#endif

} // End namespace gold.
