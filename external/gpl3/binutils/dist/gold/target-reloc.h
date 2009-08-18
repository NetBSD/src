// target-reloc.h -- target specific relocation support  -*- C++ -*-

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

#ifndef GOLD_TARGET_RELOC_H
#define GOLD_TARGET_RELOC_H

#include "elfcpp.h"
#include "symtab.h"
#include "reloc.h"
#include "reloc-types.h"

namespace gold
{

// This function implements the generic part of reloc scanning.  The
// template parameter Scan must be a class type which provides two
// functions: local() and global().  Those functions implement the
// machine specific part of scanning.  We do it this way to
// avoidmaking a function call for each relocation, and to avoid
// repeating the generic code for each target.

template<int size, bool big_endian, typename Target_type, int sh_type,
	 typename Scan>
inline void
scan_relocs(
    const General_options& options,
    Symbol_table* symtab,
    Layout* layout,
    Target_type* target,
    Sized_relobj<size, big_endian>* object,
    unsigned int data_shndx,
    const unsigned char* prelocs,
    size_t reloc_count,
    Output_section* output_section,
    bool needs_special_offset_handling,
    size_t local_count,
    const unsigned char* plocal_syms)
{
  typedef typename Reloc_types<sh_type, size, big_endian>::Reloc Reltype;
  const int reloc_size = Reloc_types<sh_type, size, big_endian>::reloc_size;
  const int sym_size = elfcpp::Elf_sizes<size>::sym_size;
  Scan scan;

  for (size_t i = 0; i < reloc_count; ++i, prelocs += reloc_size)
    {
      Reltype reloc(prelocs);

      if (needs_special_offset_handling
	  && !output_section->is_input_address_mapped(object, data_shndx,
						      reloc.get_r_offset()))
	continue;

      typename elfcpp::Elf_types<size>::Elf_WXword r_info = reloc.get_r_info();
      unsigned int r_sym = elfcpp::elf_r_sym<size>(r_info);
      unsigned int r_type = elfcpp::elf_r_type<size>(r_info);

      if (r_sym < local_count)
	{
	  gold_assert(plocal_syms != NULL);
	  typename elfcpp::Sym<size, big_endian> lsym(plocal_syms
						      + r_sym * sym_size);
	  unsigned int shndx = lsym.get_st_shndx();
	  bool is_ordinary;
	  shndx = object->adjust_sym_shndx(r_sym, shndx, &is_ordinary);
	  if (is_ordinary
	      && shndx != elfcpp::SHN_UNDEF
	      && !object->is_section_included(shndx))
	    {
	      // RELOC is a relocation against a local symbol in a
	      // section we are discarding.  We can ignore this
	      // relocation.  It will eventually become a reloc
	      // against the value zero.
	      //
	      // FIXME: We should issue a warning if this is an
	      // allocated section; is this the best place to do it?
	      // 
	      // FIXME: The old GNU linker would in some cases look
	      // for the linkonce section which caused this section to
	      // be discarded, and, if the other section was the same
	      // size, change the reloc to refer to the other section.
	      // That seems risky and weird to me, and I don't know of
	      // any case where it is actually required.

	      continue;
	    }

	  scan.local(options, symtab, layout, target, object, data_shndx,
		     output_section, reloc, r_type, lsym);
	}
      else
	{
	  Symbol* gsym = object->global_symbol(r_sym);
	  gold_assert(gsym != NULL);
	  if (gsym->is_forwarder())
	    gsym = symtab->resolve_forwards(gsym);

	  scan.global(options, symtab, layout, target, object, data_shndx,
		      output_section, reloc, r_type, gsym);
	}
    }
}

// Behavior for relocations to discarded comdat sections.

enum Comdat_behavior
{
  CB_UNDETERMINED,   // Not yet determined -- need to look at section name.
  CB_PRETEND,        // Attempt to map to the corresponding kept section.
  CB_IGNORE,         // Ignore the relocation.
  CB_WARNING         // Print a warning.
};

// Decide what the linker should do for relocations that refer to discarded
// comdat sections.  This decision is based on the name of the section being
// relocated.

inline Comdat_behavior
get_comdat_behavior(const char* name)
{
  if (Layout::is_debug_info_section(name))
    return CB_PRETEND;
  if (strcmp(name, ".eh_frame") == 0
      || strcmp(name, ".gcc_except_table") == 0)
    return CB_IGNORE;
  return CB_WARNING;
}

// This function implements the generic part of relocation processing.
// The template parameter Relocate must be a class type which provides
// a single function, relocate(), which implements the machine
// specific part of a relocation.

// SIZE is the ELF size: 32 or 64.  BIG_ENDIAN is the endianness of
// the data.  SH_TYPE is the section type: SHT_REL or SHT_RELA.
// RELOCATE implements operator() to do a relocation.

// PRELOCS points to the relocation data.  RELOC_COUNT is the number
// of relocs.  OUTPUT_SECTION is the output section.
// NEEDS_SPECIAL_OFFSET_HANDLING is true if input offsets need to be
// mapped to output offsets.

// VIEW is the section data, VIEW_ADDRESS is its memory address, and
// VIEW_SIZE is the size.  These refer to the input section, unless
// NEEDS_SPECIAL_OFFSET_HANDLING is true, in which case they refer to
// the output section.

template<int size, bool big_endian, typename Target_type, int sh_type,
	 typename Relocate>
inline void
relocate_section(
    const Relocate_info<size, big_endian>* relinfo,
    Target_type* target,
    const unsigned char* prelocs,
    size_t reloc_count,
    Output_section* output_section,
    bool needs_special_offset_handling,
    unsigned char* view,
    typename elfcpp::Elf_types<size>::Elf_Addr view_address,
    section_size_type view_size)
{
  typedef typename Reloc_types<sh_type, size, big_endian>::Reloc Reltype;
  const int reloc_size = Reloc_types<sh_type, size, big_endian>::reloc_size;
  Relocate relocate;

  Sized_relobj<size, big_endian>* object = relinfo->object;
  unsigned int local_count = object->local_symbol_count();

  Comdat_behavior comdat_behavior = CB_UNDETERMINED;

  for (size_t i = 0; i < reloc_count; ++i, prelocs += reloc_size)
    {
      Reltype reloc(prelocs);

      section_offset_type offset =
	convert_to_section_size_type(reloc.get_r_offset());

      if (needs_special_offset_handling)
	{
	  offset = output_section->output_offset(relinfo->object,
						 relinfo->data_shndx,
						 offset);
	  if (offset == -1)
	    continue;
	}

      typename elfcpp::Elf_types<size>::Elf_WXword r_info = reloc.get_r_info();
      unsigned int r_sym = elfcpp::elf_r_sym<size>(r_info);
      unsigned int r_type = elfcpp::elf_r_type<size>(r_info);

      const Sized_symbol<size>* sym;

      Symbol_value<size> symval;
      const Symbol_value<size> *psymval;
      if (r_sym < local_count)
	{
	  sym = NULL;
	  psymval = object->local_symbol(r_sym);

          // If the local symbol belongs to a section we are discarding,
          // and that section is a debug section, try to find the
          // corresponding kept section and map this symbol to its
          // counterpart in the kept section.
	  bool is_ordinary;
	  unsigned int shndx = psymval->input_shndx(&is_ordinary);
	  if (is_ordinary
	      && shndx != elfcpp::SHN_UNDEF
	      && !object->is_section_included(shndx))
	    {
	      if (comdat_behavior == CB_UNDETERMINED)
	        {
	          std::string name = object->section_name(relinfo->data_shndx);
	          comdat_behavior = get_comdat_behavior(name.c_str());
	        }
	      if (comdat_behavior == CB_PRETEND)
	        {
                  bool found;
	          typename elfcpp::Elf_types<size>::Elf_Addr value =
	            object->map_to_kept_section(shndx, &found);
	          if (found)
	            symval.set_output_value(value + psymval->input_value());
                  else
                    symval.set_output_value(0);
	        }
	      else
	        {
	          if (comdat_behavior == CB_WARNING)
                    gold_warning_at_location(relinfo, i, offset,
                                             _("Relocation refers to discarded "
                                               "comdat section"));
                  symval.set_output_value(0);
	        }
	      symval.set_no_output_symtab_entry();
	      psymval = &symval;
	    }
	}
      else
	{
	  const Symbol* gsym = object->global_symbol(r_sym);
	  gold_assert(gsym != NULL);
	  if (gsym->is_forwarder())
	    gsym = relinfo->symtab->resolve_forwards(gsym);

	  sym = static_cast<const Sized_symbol<size>*>(gsym);
	  if (sym->has_symtab_index())
	    symval.set_output_symtab_index(sym->symtab_index());
	  else
	    symval.set_no_output_symtab_entry();
	  symval.set_output_value(sym->value());
	  psymval = &symval;
	}

      if (!relocate.relocate(relinfo, target, i, reloc, r_type, sym, psymval,
			     view + offset, view_address + offset, view_size))
	continue;

      if (offset < 0 || static_cast<section_size_type>(offset) >= view_size)
	{
	  gold_error_at_location(relinfo, i, offset,
				 _("reloc has bad offset %zu"),
				 static_cast<size_t>(offset));
	  continue;
	}

      if (sym != NULL
	  && sym->is_undefined()
	  && sym->binding() != elfcpp::STB_WEAK
	  && (!parameters->options().shared()       // -shared
              || parameters->options().defs()))     // -z defs
	gold_undefined_symbol(sym, relinfo, i, offset);

      if (sym != NULL && sym->has_warning())
	relinfo->symtab->issue_warning(sym, relinfo, i, offset);
    }
}

// This class may be used as a typical class for the
// Scan_relocatable_reloc parameter to scan_relocatable_relocs.  The
// template parameter Classify_reloc must be a class type which
// provides a function get_size_for_reloc which returns the number of
// bytes to which a reloc applies.  This class is intended to capture
// the most typical target behaviour, while still permitting targets
// to define their own independent class for Scan_relocatable_reloc.

template<int sh_type, typename Classify_reloc>
class Default_scan_relocatable_relocs
{
 public:
  // Return the strategy to use for a local symbol which is not a
  // section symbol, given the relocation type.
  inline Relocatable_relocs::Reloc_strategy
  local_non_section_strategy(unsigned int, Relobj*)
  { return Relocatable_relocs::RELOC_COPY; }

  // Return the strategy to use for a local symbol which is a section
  // symbol, given the relocation type.
  inline Relocatable_relocs::Reloc_strategy
  local_section_strategy(unsigned int r_type, Relobj* object)
  {
    if (sh_type == elfcpp::SHT_RELA)
      return Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_RELA;
    else
      {
	Classify_reloc classify;
	switch (classify.get_size_for_reloc(r_type, object))
	  {
	  case 0:
	    return Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_0;
	  case 1:
	    return Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_1;
	  case 2:
	    return Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_2;
	  case 4:
	    return Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_4;
	  case 8:
	    return Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_8;
	  default:
	    gold_unreachable();
	  }
      }
  }

  // Return the strategy to use for a global symbol, given the
  // relocation type, the object, and the symbol index.
  inline Relocatable_relocs::Reloc_strategy
  global_strategy(unsigned int, Relobj*, unsigned int)
  { return Relocatable_relocs::RELOC_COPY; }
};

// Scan relocs during a relocatable link.  This is a default
// definition which should work for most targets.
// Scan_relocatable_reloc must name a class type which provides three
// functions which return a Relocatable_relocs::Reloc_strategy code:
// global_strategy, local_non_section_strategy, and
// local_section_strategy.  Most targets should be able to use
// Default_scan_relocatable_relocs as this class.

template<int size, bool big_endian, int sh_type,
	 typename Scan_relocatable_reloc>
void
scan_relocatable_relocs(
    const General_options&,
    Symbol_table*,
    Layout*,
    Sized_relobj<size, big_endian>* object,
    unsigned int data_shndx,
    const unsigned char* prelocs,
    size_t reloc_count,
    Output_section* output_section,
    bool needs_special_offset_handling,
    size_t local_symbol_count,
    const unsigned char* plocal_syms,
    Relocatable_relocs* rr)
{
  typedef typename Reloc_types<sh_type, size, big_endian>::Reloc Reltype;
  const int reloc_size = Reloc_types<sh_type, size, big_endian>::reloc_size;
  const int sym_size = elfcpp::Elf_sizes<size>::sym_size;
  Scan_relocatable_reloc scan;

  for (size_t i = 0; i < reloc_count; ++i, prelocs += reloc_size)
    {
      Reltype reloc(prelocs);

      Relocatable_relocs::Reloc_strategy strategy;

      if (needs_special_offset_handling
	  && !output_section->is_input_address_mapped(object, data_shndx,
						      reloc.get_r_offset()))
	strategy = Relocatable_relocs::RELOC_DISCARD;
      else
	{
	  typename elfcpp::Elf_types<size>::Elf_WXword r_info =
	    reloc.get_r_info();
	  const unsigned int r_sym = elfcpp::elf_r_sym<size>(r_info);
	  const unsigned int r_type = elfcpp::elf_r_type<size>(r_info);

	  if (r_sym >= local_symbol_count)
	    strategy = scan.global_strategy(r_type, object, r_sym);
	  else
	    {
	      gold_assert(plocal_syms != NULL);
	      typename elfcpp::Sym<size, big_endian> lsym(plocal_syms
							  + r_sym * sym_size);
	      unsigned int shndx = lsym.get_st_shndx();
	      bool is_ordinary;
	      shndx = object->adjust_sym_shndx(r_sym, shndx, &is_ordinary);
	      if (is_ordinary
		  && shndx != elfcpp::SHN_UNDEF
		  && !object->is_section_included(shndx))
		{
		  // RELOC is a relocation against a local symbol
		  // defined in a section we are discarding.  Discard
		  // the reloc.  FIXME: Should we issue a warning?
		  strategy = Relocatable_relocs::RELOC_DISCARD;
		}
	      else if (lsym.get_st_type() != elfcpp::STT_SECTION)
		strategy = scan.local_non_section_strategy(r_type, object);
	      else
		{
		  strategy = scan.local_section_strategy(r_type, object);
		  if (strategy != Relocatable_relocs::RELOC_DISCARD)
                    object->output_section(shndx)->set_needs_symtab_index();
		}
	    }
	}

      rr->set_next_reloc_strategy(strategy);
    }
}

// Relocate relocs during a relocatable link.  This is a default
// definition which should work for most targets.

template<int size, bool big_endian, int sh_type>
void
relocate_for_relocatable(
    const Relocate_info<size, big_endian>* relinfo,
    const unsigned char* prelocs,
    size_t reloc_count,
    Output_section* output_section,
    typename elfcpp::Elf_types<size>::Elf_Addr offset_in_output_section,
    const Relocatable_relocs* rr,
    unsigned char* view,
    typename elfcpp::Elf_types<size>::Elf_Addr view_address,
    section_size_type,
    unsigned char* reloc_view,
    section_size_type reloc_view_size)
{
  typedef typename elfcpp::Elf_types<size>::Elf_Addr Address;
  typedef typename Reloc_types<sh_type, size, big_endian>::Reloc Reltype;
  typedef typename Reloc_types<sh_type, size, big_endian>::Reloc_write
    Reltype_write;
  const int reloc_size = Reloc_types<sh_type, size, big_endian>::reloc_size;

  Sized_relobj<size, big_endian>* const object = relinfo->object;
  const unsigned int local_count = object->local_symbol_count();

  unsigned char* pwrite = reloc_view;

  for (size_t i = 0; i < reloc_count; ++i, prelocs += reloc_size)
    {
      Relocatable_relocs::Reloc_strategy strategy = rr->strategy(i);
      if (strategy == Relocatable_relocs::RELOC_DISCARD)
	continue;

      Reltype reloc(prelocs);
      Reltype_write reloc_write(pwrite);

      typename elfcpp::Elf_types<size>::Elf_WXword r_info = reloc.get_r_info();
      const unsigned int r_sym = elfcpp::elf_r_sym<size>(r_info);
      const unsigned int r_type = elfcpp::elf_r_type<size>(r_info);

      // Get the new symbol index.

      unsigned int new_symndx;
      if (r_sym < local_count)
	{
	  switch (strategy)
	    {
	    case Relocatable_relocs::RELOC_COPY:
	      new_symndx = object->symtab_index(r_sym);
	      gold_assert(new_symndx != -1U);
	      break;

	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_RELA:
	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_0:
	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_1:
	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_2:
	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_4:
	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_8:
	      {
		// We are adjusting a section symbol.  We need to find
		// the symbol table index of the section symbol for
		// the output section corresponding to input section
		// in which this symbol is defined.
		gold_assert(r_sym < local_count);
		bool is_ordinary;
		unsigned int shndx =
		  object->local_symbol_input_shndx(r_sym, &is_ordinary);
		gold_assert(is_ordinary);
		Output_section* os = object->output_section(shndx);
		gold_assert(os != NULL);
		gold_assert(os->needs_symtab_index());
		new_symndx = os->symtab_index();
	      }
	      break;

	    default:
	      gold_unreachable();
	    }
	}
      else
	{
	  const Symbol* gsym = object->global_symbol(r_sym);
	  gold_assert(gsym != NULL);
	  if (gsym->is_forwarder())
	    gsym = relinfo->symtab->resolve_forwards(gsym);

	  gold_assert(gsym->has_symtab_index());
	  new_symndx = gsym->symtab_index();
	}

      // Get the new offset--the location in the output section where
      // this relocation should be applied.

      Address offset = reloc.get_r_offset();
      Address new_offset;
      if (offset_in_output_section != -1U)
	new_offset = offset + offset_in_output_section;
      else
	{
          section_offset_type sot_offset =
              convert_types<section_offset_type, Address>(offset);
	  section_offset_type new_sot_offset =
              output_section->output_offset(object, relinfo->data_shndx,
                                            sot_offset);
	  gold_assert(new_sot_offset != -1);
          new_offset = new_sot_offset;
	}

      // In an object file, r_offset is an offset within the section.
      // In an executable or dynamic object, generated by
      // --emit-relocs, r_offset is an absolute address.
      if (!parameters->options().relocatable())
	{
	  new_offset += view_address;
	  if (offset_in_output_section != -1U)
	    new_offset -= offset_in_output_section;
	}

      reloc_write.put_r_offset(new_offset);
      reloc_write.put_r_info(elfcpp::elf_r_info<size>(new_symndx, r_type));

      // Handle the reloc addend based on the strategy.

      if (strategy == Relocatable_relocs::RELOC_COPY)
	{
	  if (sh_type == elfcpp::SHT_RELA)
	    Reloc_types<sh_type, size, big_endian>::
	      copy_reloc_addend(&reloc_write,
				&reloc);
	}
      else
	{
	  // The relocation uses a section symbol in the input file.
	  // We are adjusting it to use a section symbol in the output
	  // file.  The input section symbol refers to some address in
	  // the input section.  We need the relocation in the output
	  // file to refer to that same address.  This adjustment to
	  // the addend is the same calculation we use for a simple
	  // absolute relocation for the input section symbol.

	  const Symbol_value<size>* psymval = object->local_symbol(r_sym);

	  unsigned char* padd = view + offset;
	  switch (strategy)
	    {
	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_RELA:
	      {
		typename elfcpp::Elf_types<size>::Elf_Swxword addend;
		addend = Reloc_types<sh_type, size, big_endian>::
			   get_reloc_addend(&reloc);
		addend = psymval->value(object, addend);
		Reloc_types<sh_type, size, big_endian>::
		  set_reloc_addend(&reloc_write, addend);
	      }
	      break;

	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_0:
	      break;

	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_1:
	      Relocate_functions<size, big_endian>::rel8(padd, object,
							 psymval);
	      break;

	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_2:
	      Relocate_functions<size, big_endian>::rel16(padd, object,
							  psymval);
	      break;

	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_4:
	      Relocate_functions<size, big_endian>::rel32(padd, object,
							  psymval);
	      break;

	    case Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_8:
	      Relocate_functions<size, big_endian>::rel64(padd, object,
							  psymval);
	      break;

	    default:
	      gold_unreachable();
	    }
	}

      pwrite += reloc_size;
    }

  gold_assert(static_cast<section_size_type>(pwrite - reloc_view)
	      == reloc_view_size);
}

} // End namespace gold.

#endif // !defined(GOLD_TARGET_RELOC_H)
