// layout.cc -- lay out output file sections for gold

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

#include <cerrno>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <utility>
#include <fcntl.h>
#include <unistd.h>
#include "libiberty.h"
#include "md5.h"
#include "sha1.h"

#include "parameters.h"
#include "options.h"
#include "mapfile.h"
#include "script.h"
#include "script-sections.h"
#include "output.h"
#include "symtab.h"
#include "dynobj.h"
#include "ehframe.h"
#include "compressed_output.h"
#include "reduced_debug_output.h"
#include "reloc.h"
#include "descriptors.h"
#include "layout.h"

namespace gold
{

// Layout_task_runner methods.

// Lay out the sections.  This is called after all the input objects
// have been read.

void
Layout_task_runner::run(Workqueue* workqueue, const Task* task)
{
  off_t file_size = this->layout_->finalize(this->input_objects_,
					    this->symtab_,
                                            this->target_,
					    task);

  // Now we know the final size of the output file and we know where
  // each piece of information goes.

  if (this->mapfile_ != NULL)
    {
      this->mapfile_->print_discarded_sections(this->input_objects_);
      this->layout_->print_to_mapfile(this->mapfile_);
    }

  Output_file* of = new Output_file(parameters->options().output_file_name());
  if (this->options_.oformat_enum() != General_options::OBJECT_FORMAT_ELF)
    of->set_is_temporary();
  of->open(file_size);

  // Queue up the final set of tasks.
  gold::queue_final_tasks(this->options_, this->input_objects_,
			  this->symtab_, this->layout_, workqueue, of);
}

// Layout methods.

Layout::Layout(const General_options& options, Script_options* script_options)
  : options_(options),
    script_options_(script_options),
    namepool_(),
    sympool_(),
    dynpool_(),
    signatures_(),
    section_name_map_(),
    segment_list_(),
    section_list_(),
    unattached_section_list_(),
    sections_are_attached_(false),
    special_output_list_(),
    section_headers_(NULL),
    tls_segment_(NULL),
    relro_segment_(NULL),
    symtab_section_(NULL),
    symtab_xindex_(NULL),
    dynsym_section_(NULL),
    dynsym_xindex_(NULL),
    dynamic_section_(NULL),
    dynamic_data_(NULL),
    eh_frame_section_(NULL),
    eh_frame_data_(NULL),
    added_eh_frame_data_(false),
    eh_frame_hdr_section_(NULL),
    build_id_note_(NULL),
    debug_abbrev_(NULL),
    debug_info_(NULL),
    group_signatures_(),
    output_file_size_(-1),
    input_requires_executable_stack_(false),
    input_with_gnu_stack_note_(false),
    input_without_gnu_stack_note_(false),
    has_static_tls_(false),
    any_postprocessing_sections_(false)
{
  // Make space for more than enough segments for a typical file.
  // This is just for efficiency--it's OK if we wind up needing more.
  this->segment_list_.reserve(12);

  // We expect two unattached Output_data objects: the file header and
  // the segment headers.
  this->special_output_list_.reserve(2);
}

// Hash a key we use to look up an output section mapping.

size_t
Layout::Hash_key::operator()(const Layout::Key& k) const
{
 return k.first + k.second.first + k.second.second;
}

// Return whether PREFIX is a prefix of STR.

static inline bool
is_prefix_of(const char* prefix, const char* str)
{
  return strncmp(prefix, str, strlen(prefix)) == 0;
}

// Returns whether the given section is in the list of
// debug-sections-used-by-some-version-of-gdb.  Currently,
// we've checked versions of gdb up to and including 6.7.1.

static const char* gdb_sections[] =
{ ".debug_abbrev",
  // ".debug_aranges",   // not used by gdb as of 6.7.1
  ".debug_frame",
  ".debug_info",
  ".debug_line",
  ".debug_loc",
  ".debug_macinfo",
  // ".debug_pubnames",  // not used by gdb as of 6.7.1
  ".debug_ranges",
  ".debug_str",
};

static const char* lines_only_debug_sections[] =
{ ".debug_abbrev",
  // ".debug_aranges",   // not used by gdb as of 6.7.1
  // ".debug_frame",
  ".debug_info",
  ".debug_line",
  // ".debug_loc",
  // ".debug_macinfo",
  // ".debug_pubnames",  // not used by gdb as of 6.7.1
  // ".debug_ranges",
  ".debug_str",
};

static inline bool
is_gdb_debug_section(const char* str)
{
  // We can do this faster: binary search or a hashtable.  But why bother?
  for (size_t i = 0; i < sizeof(gdb_sections)/sizeof(*gdb_sections); ++i)
    if (strcmp(str, gdb_sections[i]) == 0)
      return true;
  return false;
}

static inline bool
is_lines_only_debug_section(const char* str)
{
  // We can do this faster: binary search or a hashtable.  But why bother?
  for (size_t i = 0;
       i < sizeof(lines_only_debug_sections)/sizeof(*lines_only_debug_sections);
       ++i)
    if (strcmp(str, lines_only_debug_sections[i]) == 0)
      return true;
  return false;
}

// Whether to include this section in the link.

template<int size, bool big_endian>
bool
Layout::include_section(Sized_relobj<size, big_endian>*, const char* name,
			const elfcpp::Shdr<size, big_endian>& shdr)
{
  switch (shdr.get_sh_type())
    {
    case elfcpp::SHT_NULL:
    case elfcpp::SHT_SYMTAB:
    case elfcpp::SHT_DYNSYM:
    case elfcpp::SHT_HASH:
    case elfcpp::SHT_DYNAMIC:
    case elfcpp::SHT_SYMTAB_SHNDX:
      return false;

    case elfcpp::SHT_STRTAB:
      // Discard the sections which have special meanings in the ELF
      // ABI.  Keep others (e.g., .stabstr).  We could also do this by
      // checking the sh_link fields of the appropriate sections.
      return (strcmp(name, ".dynstr") != 0
	      && strcmp(name, ".strtab") != 0
	      && strcmp(name, ".shstrtab") != 0);

    case elfcpp::SHT_RELA:
    case elfcpp::SHT_REL:
    case elfcpp::SHT_GROUP:
      // If we are emitting relocations these should be handled
      // elsewhere.
      gold_assert(!parameters->options().relocatable()
		  && !parameters->options().emit_relocs());
      return false;

    case elfcpp::SHT_PROGBITS:
      if (parameters->options().strip_debug()
	  && (shdr.get_sh_flags() & elfcpp::SHF_ALLOC) == 0)
	{
	  if (is_debug_info_section(name))
	    return false;
	}
      if (parameters->options().strip_debug_non_line()
	  && (shdr.get_sh_flags() & elfcpp::SHF_ALLOC) == 0)
	{
	  // Debugging sections can only be recognized by name.
	  if (is_prefix_of(".debug", name)
              && !is_lines_only_debug_section(name))
	    return false;
	}
      if (parameters->options().strip_debug_gdb()
	  && (shdr.get_sh_flags() & elfcpp::SHF_ALLOC) == 0)
	{
	  // Debugging sections can only be recognized by name.
	  if (is_prefix_of(".debug", name)
              && !is_gdb_debug_section(name))
	    return false;
	}
      return true;

    default:
      return true;
    }
}

// Return an output section named NAME, or NULL if there is none.

Output_section*
Layout::find_output_section(const char* name) const
{
  for (Section_list::const_iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    if (strcmp((*p)->name(), name) == 0)
      return *p;
  return NULL;
}

// Return an output segment of type TYPE, with segment flags SET set
// and segment flags CLEAR clear.  Return NULL if there is none.

Output_segment*
Layout::find_output_segment(elfcpp::PT type, elfcpp::Elf_Word set,
			    elfcpp::Elf_Word clear) const
{
  for (Segment_list::const_iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    if (static_cast<elfcpp::PT>((*p)->type()) == type
	&& ((*p)->flags() & set) == set
	&& ((*p)->flags() & clear) == 0)
      return *p;
  return NULL;
}

// Return the output section to use for section NAME with type TYPE
// and section flags FLAGS.  NAME must be canonicalized in the string
// pool, and NAME_KEY is the key.

Output_section*
Layout::get_output_section(const char* name, Stringpool::Key name_key,
			   elfcpp::Elf_Word type, elfcpp::Elf_Xword flags)
{
  elfcpp::Elf_Xword lookup_flags = flags;

  // Ignoring SHF_WRITE and SHF_EXECINSTR here means that we combine
  // read-write with read-only sections.  Some other ELF linkers do
  // not do this.  FIXME: Perhaps there should be an option
  // controlling this.
  lookup_flags &= ~(elfcpp::SHF_WRITE | elfcpp::SHF_EXECINSTR);

  const Key key(name_key, std::make_pair(type, lookup_flags));
  const std::pair<Key, Output_section*> v(key, NULL);
  std::pair<Section_name_map::iterator, bool> ins(
    this->section_name_map_.insert(v));

  if (!ins.second)
    return ins.first->second;
  else
    {
      // This is the first time we've seen this name/type/flags
      // combination.  For compatibility with the GNU linker, we
      // combine sections with contents and zero flags with sections
      // with non-zero flags.  This is a workaround for cases where
      // assembler code forgets to set section flags.  FIXME: Perhaps
      // there should be an option to control this.
      Output_section* os = NULL;

      if (type == elfcpp::SHT_PROGBITS)
	{
          if (flags == 0)
            {
              Output_section* same_name = this->find_output_section(name);
              if (same_name != NULL
                  && same_name->type() == elfcpp::SHT_PROGBITS
                  && (same_name->flags() & elfcpp::SHF_TLS) == 0)
                os = same_name;
            }
          else if ((flags & elfcpp::SHF_TLS) == 0)
            {
              elfcpp::Elf_Xword zero_flags = 0;
              const Key zero_key(name_key, std::make_pair(type, zero_flags));
              Section_name_map::iterator p =
                  this->section_name_map_.find(zero_key);
              if (p != this->section_name_map_.end())
		os = p->second;
            }
	}

      if (os == NULL)
	os = this->make_output_section(name, type, flags);
      ins.first->second = os;
      return os;
    }
}

// Pick the output section to use for section NAME, in input file
// RELOBJ, with type TYPE and flags FLAGS.  RELOBJ may be NULL for a
// linker created section.  IS_INPUT_SECTION is true if we are
// choosing an output section for an input section found in a input
// file.  This will return NULL if the input section should be
// discarded.

Output_section*
Layout::choose_output_section(const Relobj* relobj, const char* name,
			      elfcpp::Elf_Word type, elfcpp::Elf_Xword flags,
			      bool is_input_section)
{
  // We should not see any input sections after we have attached
  // sections to segments.
  gold_assert(!is_input_section || !this->sections_are_attached_);

  // Some flags in the input section should not be automatically
  // copied to the output section.
  flags &= ~ (elfcpp::SHF_INFO_LINK
	      | elfcpp::SHF_LINK_ORDER
	      | elfcpp::SHF_GROUP
	      | elfcpp::SHF_MERGE
	      | elfcpp::SHF_STRINGS);

  if (this->script_options_->saw_sections_clause())
    {
      // We are using a SECTIONS clause, so the output section is
      // chosen based only on the name.

      Script_sections* ss = this->script_options_->script_sections();
      const char* file_name = relobj == NULL ? NULL : relobj->name().c_str();
      Output_section** output_section_slot;
      name = ss->output_section_name(file_name, name, &output_section_slot);
      if (name == NULL)
	{
	  // The SECTIONS clause says to discard this input section.
	  return NULL;
	}

      // If this is an orphan section--one not mentioned in the linker
      // script--then OUTPUT_SECTION_SLOT will be NULL, and we do the
      // default processing below.

      if (output_section_slot != NULL)
	{
	  if (*output_section_slot != NULL)
	    return *output_section_slot;

	  // We don't put sections found in the linker script into
	  // SECTION_NAME_MAP_.  That keeps us from getting confused
	  // if an orphan section is mapped to a section with the same
	  // name as one in the linker script.

	  name = this->namepool_.add(name, false, NULL);

	  Output_section* os = this->make_output_section(name, type, flags);
	  os->set_found_in_sections_clause();
	  *output_section_slot = os;
	  return os;
	}
    }

  // FIXME: Handle SHF_OS_NONCONFORMING somewhere.

  // Turn NAME from the name of the input section into the name of the
  // output section.

  size_t len = strlen(name);
  if (is_input_section && !parameters->options().relocatable())
    name = Layout::output_section_name(name, &len);

  Stringpool::Key name_key;
  name = this->namepool_.add_with_length(name, len, true, &name_key);

  // Find or make the output section.  The output section is selected
  // based on the section name, type, and flags.
  return this->get_output_section(name, name_key, type, flags);
}

// Return the output section to use for input section SHNDX, with name
// NAME, with header HEADER, from object OBJECT.  RELOC_SHNDX is the
// index of a relocation section which applies to this section, or 0
// if none, or -1U if more than one.  RELOC_TYPE is the type of the
// relocation section if there is one.  Set *OFF to the offset of this
// input section without the output section.  Return NULL if the
// section should be discarded.  Set *OFF to -1 if the section
// contents should not be written directly to the output file, but
// will instead receive special handling.

template<int size, bool big_endian>
Output_section*
Layout::layout(Sized_relobj<size, big_endian>* object, unsigned int shndx,
	       const char* name, const elfcpp::Shdr<size, big_endian>& shdr,
	       unsigned int reloc_shndx, unsigned int, off_t* off)
{
  *off = 0;

  if (!this->include_section(object, name, shdr))
    return NULL;

  Output_section* os;

  // In a relocatable link a grouped section must not be combined with
  // any other sections.
  if (parameters->options().relocatable()
      && (shdr.get_sh_flags() & elfcpp::SHF_GROUP) != 0)
    {
      name = this->namepool_.add(name, true, NULL);
      os = this->make_output_section(name, shdr.get_sh_type(),
				     shdr.get_sh_flags());
    }
  else
    {
      os = this->choose_output_section(object, name, shdr.get_sh_type(),
				       shdr.get_sh_flags(), true);
      if (os == NULL)
	return NULL;
    }

  // By default the GNU linker sorts input sections whose names match
  // .ctor.*, .dtor.*, .init_array.*, or .fini_array.*.  The sections
  // are sorted by name.  This is used to implement constructor
  // priority ordering.  We are compatible.
  if (!this->script_options_->saw_sections_clause()
      && (is_prefix_of(".ctors.", name)
	  || is_prefix_of(".dtors.", name)
	  || is_prefix_of(".init_array.", name)
	  || is_prefix_of(".fini_array.", name)))
    os->set_must_sort_attached_input_sections();

  // FIXME: Handle SHF_LINK_ORDER somewhere.

  *off = os->add_input_section(object, shndx, name, shdr, reloc_shndx,
			       this->script_options_->saw_sections_clause());

  return os;
}

// Handle a relocation section when doing a relocatable link.

template<int size, bool big_endian>
Output_section*
Layout::layout_reloc(Sized_relobj<size, big_endian>* object,
		     unsigned int,
		     const elfcpp::Shdr<size, big_endian>& shdr,
		     Output_section* data_section,
		     Relocatable_relocs* rr)
{
  gold_assert(parameters->options().relocatable()
	      || parameters->options().emit_relocs());

  int sh_type = shdr.get_sh_type();

  std::string name;
  if (sh_type == elfcpp::SHT_REL)
    name = ".rel";
  else if (sh_type == elfcpp::SHT_RELA)
    name = ".rela";
  else
    gold_unreachable();
  name += data_section->name();

  Output_section* os = this->choose_output_section(object, name.c_str(),
						   sh_type,
						   shdr.get_sh_flags(),
						   false);

  os->set_should_link_to_symtab();
  os->set_info_section(data_section);

  Output_section_data* posd;
  if (sh_type == elfcpp::SHT_REL)
    {
      os->set_entsize(elfcpp::Elf_sizes<size>::rel_size);
      posd = new Output_relocatable_relocs<elfcpp::SHT_REL,
					   size,
					   big_endian>(rr);
    }
  else if (sh_type == elfcpp::SHT_RELA)
    {
      os->set_entsize(elfcpp::Elf_sizes<size>::rela_size);
      posd = new Output_relocatable_relocs<elfcpp::SHT_RELA,
					   size,
					   big_endian>(rr);
    }
  else
    gold_unreachable();

  os->add_output_section_data(posd);
  rr->set_output_data(posd);

  return os;
}

// Handle a group section when doing a relocatable link.

template<int size, bool big_endian>
void
Layout::layout_group(Symbol_table* symtab,
		     Sized_relobj<size, big_endian>* object,
		     unsigned int,
		     const char* group_section_name,
		     const char* signature,
		     const elfcpp::Shdr<size, big_endian>& shdr,
		     elfcpp::Elf_Word flags,
		     std::vector<unsigned int>* shndxes)
{
  gold_assert(parameters->options().relocatable());
  gold_assert(shdr.get_sh_type() == elfcpp::SHT_GROUP);
  group_section_name = this->namepool_.add(group_section_name, true, NULL);
  Output_section* os = this->make_output_section(group_section_name,
						 elfcpp::SHT_GROUP,
						 shdr.get_sh_flags());

  // We need to find a symbol with the signature in the symbol table.
  // If we don't find one now, we need to look again later.
  Symbol* sym = symtab->lookup(signature, NULL);
  if (sym != NULL)
    os->set_info_symndx(sym);
  else
    {
      // We will wind up using a symbol whose name is the signature.
      // So just put the signature in the symbol name pool to save it.
      signature = symtab->canonicalize_name(signature);
      this->group_signatures_.push_back(Group_signature(os, signature));
    }

  os->set_should_link_to_symtab();
  os->set_entsize(4);

  section_size_type entry_count =
    convert_to_section_size_type(shdr.get_sh_size() / 4);
  Output_section_data* posd =
    new Output_data_group<size, big_endian>(object, entry_count, flags,
					    shndxes);
  os->add_output_section_data(posd);
}

// Special GNU handling of sections name .eh_frame.  They will
// normally hold exception frame data as defined by the C++ ABI
// (http://codesourcery.com/cxx-abi/).

template<int size, bool big_endian>
Output_section*
Layout::layout_eh_frame(Sized_relobj<size, big_endian>* object,
			const unsigned char* symbols,
			off_t symbols_size,
			const unsigned char* symbol_names,
			off_t symbol_names_size,
			unsigned int shndx,
			const elfcpp::Shdr<size, big_endian>& shdr,
			unsigned int reloc_shndx, unsigned int reloc_type,
			off_t* off)
{
  gold_assert(shdr.get_sh_type() == elfcpp::SHT_PROGBITS);
  gold_assert((shdr.get_sh_flags() & elfcpp::SHF_ALLOC) != 0);

  const char* const name = ".eh_frame";
  Output_section* os = this->choose_output_section(object,
						   name,
						   elfcpp::SHT_PROGBITS,
						   elfcpp::SHF_ALLOC,
						   false);
  if (os == NULL)
    return NULL;

  if (this->eh_frame_section_ == NULL)
    {
      this->eh_frame_section_ = os;
      this->eh_frame_data_ = new Eh_frame();

      if (this->options_.eh_frame_hdr())
	{
	  Output_section* hdr_os =
	    this->choose_output_section(NULL,
					".eh_frame_hdr",
					elfcpp::SHT_PROGBITS,
					elfcpp::SHF_ALLOC,
					false);

	  if (hdr_os != NULL)
	    {
	      Eh_frame_hdr* hdr_posd = new Eh_frame_hdr(os,
							this->eh_frame_data_);
	      hdr_os->add_output_section_data(hdr_posd);

	      hdr_os->set_after_input_sections();

	      if (!this->script_options_->saw_phdrs_clause())
		{
		  Output_segment* hdr_oseg;
		  hdr_oseg = this->make_output_segment(elfcpp::PT_GNU_EH_FRAME,
						       elfcpp::PF_R);
		  hdr_oseg->add_output_section(hdr_os, elfcpp::PF_R);
		}

	      this->eh_frame_data_->set_eh_frame_hdr(hdr_posd);
	    }
	}
    }

  gold_assert(this->eh_frame_section_ == os);

  if (this->eh_frame_data_->add_ehframe_input_section(object,
						      symbols,
						      symbols_size,
						      symbol_names,
						      symbol_names_size,
						      shndx,
						      reloc_shndx,
						      reloc_type))
    {
      os->update_flags_for_input_section(shdr.get_sh_flags());

      // We found a .eh_frame section we are going to optimize, so now
      // we can add the set of optimized sections to the output
      // section.  We need to postpone adding this until we've found a
      // section we can optimize so that the .eh_frame section in
      // crtbegin.o winds up at the start of the output section.
      if (!this->added_eh_frame_data_)
	{
	  os->add_output_section_data(this->eh_frame_data_);
	  this->added_eh_frame_data_ = true;
	}
      *off = -1;
    }
  else
    {
      // We couldn't handle this .eh_frame section for some reason.
      // Add it as a normal section.
      bool saw_sections_clause = this->script_options_->saw_sections_clause();
      *off = os->add_input_section(object, shndx, name, shdr, reloc_shndx,
				   saw_sections_clause);
    }

  return os;
}

// Add POSD to an output section using NAME, TYPE, and FLAGS.  Return
// the output section.

Output_section*
Layout::add_output_section_data(const char* name, elfcpp::Elf_Word type,
				elfcpp::Elf_Xword flags,
				Output_section_data* posd)
{
  Output_section* os = this->choose_output_section(NULL, name, type, flags,
						   false);
  if (os != NULL)
    os->add_output_section_data(posd);
  return os;
}

// Map section flags to segment flags.

elfcpp::Elf_Word
Layout::section_flags_to_segment(elfcpp::Elf_Xword flags)
{
  elfcpp::Elf_Word ret = elfcpp::PF_R;
  if ((flags & elfcpp::SHF_WRITE) != 0)
    ret |= elfcpp::PF_W;
  if ((flags & elfcpp::SHF_EXECINSTR) != 0)
    ret |= elfcpp::PF_X;
  return ret;
}

// Sometimes we compress sections.  This is typically done for
// sections that are not part of normal program execution (such as
// .debug_* sections), and where the readers of these sections know
// how to deal with compressed sections.  (To make it easier for them,
// we will rename the ouput section in such cases from .foo to
// .foo.zlib.nnnn, where nnnn is the uncompressed size.)  This routine
// doesn't say for certain whether we'll compress -- it depends on
// commandline options as well -- just whether this section is a
// candidate for compression.

static bool
is_compressible_debug_section(const char* secname)
{
  return (strncmp(secname, ".debug", sizeof(".debug") - 1) == 0);
}

// Make a new Output_section, and attach it to segments as
// appropriate.

Output_section*
Layout::make_output_section(const char* name, elfcpp::Elf_Word type,
			    elfcpp::Elf_Xword flags)
{
  Output_section* os;
  if ((flags & elfcpp::SHF_ALLOC) == 0
      && strcmp(this->options_.compress_debug_sections(), "none") != 0
      && is_compressible_debug_section(name))
    os = new Output_compressed_section(&this->options_, name, type, flags);

  else if ((flags & elfcpp::SHF_ALLOC) == 0
           && this->options_.strip_debug_non_line()
           && strcmp(".debug_abbrev", name) == 0)
    {
      os = this->debug_abbrev_ = new Output_reduced_debug_abbrev_section(
          name, type, flags);
      if (this->debug_info_)
        this->debug_info_->set_abbreviations(this->debug_abbrev_);
    }
  else if ((flags & elfcpp::SHF_ALLOC) == 0
           && this->options_.strip_debug_non_line()
           && strcmp(".debug_info", name) == 0)
    {
      os = this->debug_info_ = new Output_reduced_debug_info_section(
          name, type, flags);
      if (this->debug_abbrev_)
        this->debug_info_->set_abbreviations(this->debug_abbrev_);
    }
 else
    os = new Output_section(name, type, flags);

  this->section_list_.push_back(os);

  // The GNU linker by default sorts some sections by priority, so we
  // do the same.  We need to know that this might happen before we
  // attach any input sections.
  if (!this->script_options_->saw_sections_clause()
      && (strcmp(name, ".ctors") == 0
	  || strcmp(name, ".dtors") == 0
	  || strcmp(name, ".init_array") == 0
	  || strcmp(name, ".fini_array") == 0))
    os->set_may_sort_attached_input_sections();

  // With -z relro, we have to recognize the special sections by name.
  // There is no other way.
  if (!this->script_options_->saw_sections_clause()
      && parameters->options().relro()
      && type == elfcpp::SHT_PROGBITS
      && (flags & elfcpp::SHF_ALLOC) != 0
      && (flags & elfcpp::SHF_WRITE) != 0)
    {
      if (strcmp(name, ".data.rel.ro") == 0)
	os->set_is_relro();
      else if (strcmp(name, ".data.rel.ro.local") == 0)
	{
	  os->set_is_relro();
	  os->set_is_relro_local();
	}
    }

  // If we have already attached the sections to segments, then we
  // need to attach this one now.  This happens for sections created
  // directly by the linker.
  if (this->sections_are_attached_)
    this->attach_section_to_segment(os);

  return os;
}

// Attach output sections to segments.  This is called after we have
// seen all the input sections.

void
Layout::attach_sections_to_segments()
{
  for (Section_list::iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    this->attach_section_to_segment(*p);

  this->sections_are_attached_ = true;
}

// Attach an output section to a segment.

void
Layout::attach_section_to_segment(Output_section* os)
{
  if ((os->flags() & elfcpp::SHF_ALLOC) == 0)
    this->unattached_section_list_.push_back(os);
  else
    this->attach_allocated_section_to_segment(os);
}

// Attach an allocated output section to a segment.

void
Layout::attach_allocated_section_to_segment(Output_section* os)
{
  elfcpp::Elf_Xword flags = os->flags();
  gold_assert((flags & elfcpp::SHF_ALLOC) != 0);

  if (parameters->options().relocatable())
    return;

  // If we have a SECTIONS clause, we can't handle the attachment to
  // segments until after we've seen all the sections.
  if (this->script_options_->saw_sections_clause())
    return;

  gold_assert(!this->script_options_->saw_phdrs_clause());

  // This output section goes into a PT_LOAD segment.

  elfcpp::Elf_Word seg_flags = Layout::section_flags_to_segment(flags);

  // In general the only thing we really care about for PT_LOAD
  // segments is whether or not they are writable, so that is how we
  // search for them.  People who need segments sorted on some other
  // basis will have to use a linker script.

  Segment_list::const_iterator p;
  for (p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    {
      if ((*p)->type() == elfcpp::PT_LOAD
	  && (parameters->options().omagic()
	      || ((*p)->flags() & elfcpp::PF_W) == (seg_flags & elfcpp::PF_W)))
        {
          // If -Tbss was specified, we need to separate the data
          // and BSS segments.
          if (this->options_.user_set_Tbss())
            {
              if ((os->type() == elfcpp::SHT_NOBITS)
                  == (*p)->has_any_data_sections())
                continue;
            }

          (*p)->add_output_section(os, seg_flags);
          break;
        }
    }

  if (p == this->segment_list_.end())
    {
      Output_segment* oseg = this->make_output_segment(elfcpp::PT_LOAD,
                                                       seg_flags);
      oseg->add_output_section(os, seg_flags);
    }

  // If we see a loadable SHT_NOTE section, we create a PT_NOTE
  // segment.
  if (os->type() == elfcpp::SHT_NOTE)
    {
      // See if we already have an equivalent PT_NOTE segment.
      for (p = this->segment_list_.begin();
           p != segment_list_.end();
           ++p)
        {
          if ((*p)->type() == elfcpp::PT_NOTE
              && (((*p)->flags() & elfcpp::PF_W)
                  == (seg_flags & elfcpp::PF_W)))
            {
              (*p)->add_output_section(os, seg_flags);
              break;
            }
        }

      if (p == this->segment_list_.end())
        {
          Output_segment* oseg = this->make_output_segment(elfcpp::PT_NOTE,
                                                           seg_flags);
          oseg->add_output_section(os, seg_flags);
        }
    }

  // If we see a loadable SHF_TLS section, we create a PT_TLS
  // segment.  There can only be one such segment.
  if ((flags & elfcpp::SHF_TLS) != 0)
    {
      if (this->tls_segment_ == NULL)
	this->make_output_segment(elfcpp::PT_TLS, seg_flags);
      this->tls_segment_->add_output_section(os, seg_flags);
    }

  // If -z relro is in effect, and we see a relro section, we create a
  // PT_GNU_RELRO segment.  There can only be one such segment.
  if (os->is_relro() && parameters->options().relro())
    {
      gold_assert(seg_flags == (elfcpp::PF_R | elfcpp::PF_W));
      if (this->relro_segment_ == NULL)
	this->make_output_segment(elfcpp::PT_GNU_RELRO, seg_flags);
      this->relro_segment_->add_output_section(os, seg_flags);
    }
}

// Make an output section for a script.

Output_section*
Layout::make_output_section_for_script(const char* name)
{
  name = this->namepool_.add(name, false, NULL);
  Output_section* os = this->make_output_section(name, elfcpp::SHT_PROGBITS,
						 elfcpp::SHF_ALLOC);
  os->set_found_in_sections_clause();
  return os;
}

// Return the number of segments we expect to see.

size_t
Layout::expected_segment_count() const
{
  size_t ret = this->segment_list_.size();

  // If we didn't see a SECTIONS clause in a linker script, we should
  // already have the complete list of segments.  Otherwise we ask the
  // SECTIONS clause how many segments it expects, and add in the ones
  // we already have (PT_GNU_STACK, PT_GNU_EH_FRAME, etc.)

  if (!this->script_options_->saw_sections_clause())
    return ret;
  else
    {
      const Script_sections* ss = this->script_options_->script_sections();
      return ret + ss->expected_segment_count(this);
    }
}

// Handle the .note.GNU-stack section at layout time.  SEEN_GNU_STACK
// is whether we saw a .note.GNU-stack section in the object file.
// GNU_STACK_FLAGS is the section flags.  The flags give the
// protection required for stack memory.  We record this in an
// executable as a PT_GNU_STACK segment.  If an object file does not
// have a .note.GNU-stack segment, we must assume that it is an old
// object.  On some targets that will force an executable stack.

void
Layout::layout_gnu_stack(bool seen_gnu_stack, uint64_t gnu_stack_flags)
{
  if (!seen_gnu_stack)
    this->input_without_gnu_stack_note_ = true;
  else
    {
      this->input_with_gnu_stack_note_ = true;
      if ((gnu_stack_flags & elfcpp::SHF_EXECINSTR) != 0)
	this->input_requires_executable_stack_ = true;
    }
}

// Create the dynamic sections which are needed before we read the
// relocs.

void
Layout::create_initial_dynamic_sections(Symbol_table* symtab)
{
  if (parameters->doing_static_link())
    return;

  this->dynamic_section_ = this->choose_output_section(NULL, ".dynamic",
						       elfcpp::SHT_DYNAMIC,
						       (elfcpp::SHF_ALLOC
							| elfcpp::SHF_WRITE),
						       false);
  this->dynamic_section_->set_is_relro();

  symtab->define_in_output_data("_DYNAMIC", NULL, this->dynamic_section_, 0, 0,
				elfcpp::STT_OBJECT, elfcpp::STB_LOCAL,
				elfcpp::STV_HIDDEN, 0, false, false);

  this->dynamic_data_ =  new Output_data_dynamic(&this->dynpool_);

  this->dynamic_section_->add_output_section_data(this->dynamic_data_);
}

// For each output section whose name can be represented as C symbol,
// define __start and __stop symbols for the section.  This is a GNU
// extension.

void
Layout::define_section_symbols(Symbol_table* symtab)
{
  for (Section_list::const_iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    {
      const char* const name = (*p)->name();
      if (name[strspn(name,
		      ("0123456789"
		       "ABCDEFGHIJKLMNOPWRSTUVWXYZ"
		       "abcdefghijklmnopqrstuvwxyz"
		       "_"))]
	  == '\0')
	{
	  const std::string name_string(name);
	  const std::string start_name("__start_" + name_string);
	  const std::string stop_name("__stop_" + name_string);

	  symtab->define_in_output_data(start_name.c_str(),
					NULL, // version
					*p,
					0, // value
					0, // symsize
					elfcpp::STT_NOTYPE,
					elfcpp::STB_GLOBAL,
					elfcpp::STV_DEFAULT,
					0, // nonvis
					false, // offset_is_from_end
					true); // only_if_ref

	  symtab->define_in_output_data(stop_name.c_str(),
					NULL, // version
					*p,
					0, // value
					0, // symsize
					elfcpp::STT_NOTYPE,
					elfcpp::STB_GLOBAL,
					elfcpp::STV_DEFAULT,
					0, // nonvis
					true, // offset_is_from_end
					true); // only_if_ref
	}
    }
}

// Define symbols for group signatures.

void
Layout::define_group_signatures(Symbol_table* symtab)
{
  for (Group_signatures::iterator p = this->group_signatures_.begin();
       p != this->group_signatures_.end();
       ++p)
    {
      Symbol* sym = symtab->lookup(p->signature, NULL);
      if (sym != NULL)
	p->section->set_info_symndx(sym);
      else
	{
	  // Force the name of the group section to the group
	  // signature, and use the group's section symbol as the
	  // signature symbol.
	  if (strcmp(p->section->name(), p->signature) != 0)
	    {
	      const char* name = this->namepool_.add(p->signature,
						     true, NULL);
	      p->section->set_name(name);
	    }
	  p->section->set_needs_symtab_index();
	  p->section->set_info_section_symndx(p->section);
	}
    }

  this->group_signatures_.clear();
}

// Find the first read-only PT_LOAD segment, creating one if
// necessary.

Output_segment*
Layout::find_first_load_seg()
{
  for (Segment_list::const_iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    {
      if ((*p)->type() == elfcpp::PT_LOAD
	  && ((*p)->flags() & elfcpp::PF_R) != 0
	  && (parameters->options().omagic()
	      || ((*p)->flags() & elfcpp::PF_W) == 0))
	return *p;
    }

  gold_assert(!this->script_options_->saw_phdrs_clause());

  Output_segment* load_seg = this->make_output_segment(elfcpp::PT_LOAD,
						       elfcpp::PF_R);
  return load_seg;
}

// Finalize the layout.  When this is called, we have created all the
// output sections and all the output segments which are based on
// input sections.  We have several things to do, and we have to do
// them in the right order, so that we get the right results correctly
// and efficiently.

// 1) Finalize the list of output segments and create the segment
// table header.

// 2) Finalize the dynamic symbol table and associated sections.

// 3) Determine the final file offset of all the output segments.

// 4) Determine the final file offset of all the SHF_ALLOC output
// sections.

// 5) Create the symbol table sections and the section name table
// section.

// 6) Finalize the symbol table: set symbol values to their final
// value and make a final determination of which symbols are going
// into the output symbol table.

// 7) Create the section table header.

// 8) Determine the final file offset of all the output sections which
// are not SHF_ALLOC, including the section table header.

// 9) Finalize the ELF file header.

// This function returns the size of the output file.

off_t
Layout::finalize(const Input_objects* input_objects, Symbol_table* symtab,
		 Target* target, const Task* task)
{
  target->finalize_sections(this);

  this->count_local_symbols(task, input_objects);

  this->create_gold_note();
  this->create_executable_stack_info(target);
  this->create_build_id();

  Output_segment* phdr_seg = NULL;
  if (!parameters->options().relocatable() && !parameters->doing_static_link())
    {
      // There was a dynamic object in the link.  We need to create
      // some information for the dynamic linker.

      // Create the PT_PHDR segment which will hold the program
      // headers.
      if (!this->script_options_->saw_phdrs_clause())
	phdr_seg = this->make_output_segment(elfcpp::PT_PHDR, elfcpp::PF_R);

      // Create the dynamic symbol table, including the hash table.
      Output_section* dynstr;
      std::vector<Symbol*> dynamic_symbols;
      unsigned int local_dynamic_count;
      Versions versions(*this->script_options()->version_script_info(),
                        &this->dynpool_);
      this->create_dynamic_symtab(input_objects, symtab, &dynstr,
				  &local_dynamic_count, &dynamic_symbols,
				  &versions);

      // Create the .interp section to hold the name of the
      // interpreter, and put it in a PT_INTERP segment.
      if (!parameters->options().shared())
        this->create_interp(target);

      // Finish the .dynamic section to hold the dynamic data, and put
      // it in a PT_DYNAMIC segment.
      this->finish_dynamic_section(input_objects, symtab);

      // We should have added everything we need to the dynamic string
      // table.
      this->dynpool_.set_string_offsets();

      // Create the version sections.  We can't do this until the
      // dynamic string table is complete.
      this->create_version_sections(&versions, symtab, local_dynamic_count,
				    dynamic_symbols, dynstr);
    }

  // If there is a SECTIONS clause, put all the input sections into
  // the required order.
  Output_segment* load_seg;
  if (this->script_options_->saw_sections_clause())
    load_seg = this->set_section_addresses_from_script(symtab);
  else if (parameters->options().relocatable())
    load_seg = NULL;
  else
    load_seg = this->find_first_load_seg();

  if (this->options_.oformat_enum() != General_options::OBJECT_FORMAT_ELF)
    load_seg = NULL;

  gold_assert(phdr_seg == NULL || load_seg != NULL);

  // Lay out the segment headers.
  Output_segment_headers* segment_headers;
  if (parameters->options().relocatable())
    segment_headers = NULL;
  else
    {
      segment_headers = new Output_segment_headers(this->segment_list_);
      if (load_seg != NULL)
	load_seg->add_initial_output_data(segment_headers);
      if (phdr_seg != NULL)
	phdr_seg->add_initial_output_data(segment_headers);
    }

  // Lay out the file header.
  Output_file_header* file_header;
  file_header = new Output_file_header(target, symtab, segment_headers,
				       this->options_.entry());
  if (load_seg != NULL)
    load_seg->add_initial_output_data(file_header);

  this->special_output_list_.push_back(file_header);
  if (segment_headers != NULL)
    this->special_output_list_.push_back(segment_headers);

  if (this->script_options_->saw_phdrs_clause()
      && !parameters->options().relocatable())
    {
      // Support use of FILEHDRS and PHDRS attachments in a PHDRS
      // clause in a linker script.
      Script_sections* ss = this->script_options_->script_sections();
      ss->put_headers_in_phdrs(file_header, segment_headers);
    }

  // We set the output section indexes in set_segment_offsets and
  // set_section_indexes.
  unsigned int shndx = 1;

  // Set the file offsets of all the segments, and all the sections
  // they contain.
  off_t off;
  if (!parameters->options().relocatable())
    off = this->set_segment_offsets(target, load_seg, &shndx);
  else
    off = this->set_relocatable_section_offsets(file_header, &shndx);

  // Set the file offsets of all the non-data sections we've seen so
  // far which don't have to wait for the input sections.  We need
  // this in order to finalize local symbols in non-allocated
  // sections.
  off = this->set_section_offsets(off, BEFORE_INPUT_SECTIONS_PASS);

  // Set the section indexes of all unallocated sections seen so far,
  // in case any of them are somehow referenced by a symbol.
  shndx = this->set_section_indexes(shndx);

  // Create the symbol table sections.
  this->create_symtab_sections(input_objects, symtab, shndx, &off);
  if (!parameters->doing_static_link())
    this->assign_local_dynsym_offsets(input_objects);

  // Process any symbol assignments from a linker script.  This must
  // be called after the symbol table has been finalized.
  this->script_options_->finalize_symbols(symtab, this);

  // Create the .shstrtab section.
  Output_section* shstrtab_section = this->create_shstrtab();

  // Set the file offsets of the rest of the non-data sections which
  // don't have to wait for the input sections.
  off = this->set_section_offsets(off, BEFORE_INPUT_SECTIONS_PASS);

  // Now that all sections have been created, set the section indexes
  // for any sections which haven't been done yet.
  shndx = this->set_section_indexes(shndx);

  // Create the section table header.
  this->create_shdrs(shstrtab_section, &off);

  // If there are no sections which require postprocessing, we can
  // handle the section names now, and avoid a resize later.
  if (!this->any_postprocessing_sections_)
    off = this->set_section_offsets(off,
				    STRTAB_AFTER_POSTPROCESSING_SECTIONS_PASS);

  file_header->set_section_info(this->section_headers_, shstrtab_section);

  // Now we know exactly where everything goes in the output file
  // (except for non-allocated sections which require postprocessing).
  Output_data::layout_complete();

  this->output_file_size_ = off;

  return off;
}

// Create a note header following the format defined in the ELF ABI.
// NAME is the name, NOTE_TYPE is the type, DESCSZ is the size of the
// descriptor.  ALLOCATE is true if the section should be allocated in
// memory.  This returns the new note section.  It sets
// *TRAILING_PADDING to the number of trailing zero bytes required.

Output_section*
Layout::create_note(const char* name, int note_type, size_t descsz,
		    bool allocate, size_t* trailing_padding)
{
  // Authorities all agree that the values in a .note field should
  // be aligned on 4-byte boundaries for 32-bit binaries.  However,
  // they differ on what the alignment is for 64-bit binaries.
  // The GABI says unambiguously they take 8-byte alignment:
  //    http://sco.com/developers/gabi/latest/ch5.pheader.html#note_section
  // Other documentation says alignment should always be 4 bytes:
  //    http://www.netbsd.org/docs/kernel/elf-notes.html#note-format
  // GNU ld and GNU readelf both support the latter (at least as of
  // version 2.16.91), and glibc always generates the latter for
  // .note.ABI-tag (as of version 1.6), so that's the one we go with
  // here.
#ifdef GABI_FORMAT_FOR_DOTNOTE_SECTION   // This is not defined by default.
  const int size = parameters->target().get_size();
#else
  const int size = 32;
#endif

  // The contents of the .note section.
  size_t namesz = strlen(name) + 1;
  size_t aligned_namesz = align_address(namesz, size / 8);
  size_t aligned_descsz = align_address(descsz, size / 8);

  size_t notehdrsz = 3 * (size / 8) + aligned_namesz;

  unsigned char* buffer = new unsigned char[notehdrsz];
  memset(buffer, 0, notehdrsz);

  bool is_big_endian = parameters->target().is_big_endian();

  if (size == 32)
    {
      if (!is_big_endian)
	{
	  elfcpp::Swap<32, false>::writeval(buffer, namesz);
	  elfcpp::Swap<32, false>::writeval(buffer + 4, descsz);
	  elfcpp::Swap<32, false>::writeval(buffer + 8, note_type);
	}
      else
	{
	  elfcpp::Swap<32, true>::writeval(buffer, namesz);
	  elfcpp::Swap<32, true>::writeval(buffer + 4, descsz);
	  elfcpp::Swap<32, true>::writeval(buffer + 8, note_type);
	}
    }
  else if (size == 64)
    {
      if (!is_big_endian)
	{
	  elfcpp::Swap<64, false>::writeval(buffer, namesz);
	  elfcpp::Swap<64, false>::writeval(buffer + 8, descsz);
	  elfcpp::Swap<64, false>::writeval(buffer + 16, note_type);
	}
      else
	{
	  elfcpp::Swap<64, true>::writeval(buffer, namesz);
	  elfcpp::Swap<64, true>::writeval(buffer + 8, descsz);
	  elfcpp::Swap<64, true>::writeval(buffer + 16, note_type);
	}
    }
  else
    gold_unreachable();

  memcpy(buffer + 3 * (size / 8), name, namesz);

  const char* note_name = this->namepool_.add(".note", false, NULL);
  elfcpp::Elf_Xword flags = 0;
  if (allocate)
    flags = elfcpp::SHF_ALLOC;
  Output_section* os = this->make_output_section(note_name,
						 elfcpp::SHT_NOTE,
						 flags);
  Output_section_data* posd = new Output_data_const_buffer(buffer, notehdrsz,
							   size / 8,
							   "** note header");
  os->add_output_section_data(posd);

  *trailing_padding = aligned_descsz - descsz;

  return os;
}

// For an executable or shared library, create a note to record the
// version of gold used to create the binary.

void
Layout::create_gold_note()
{
  if (parameters->options().relocatable())
    return;

  std::string desc = std::string("gold ") + gold::get_version_string();

  size_t trailing_padding;
  Output_section *os = this->create_note("GNU", elfcpp::NT_GNU_GOLD_VERSION,
					 desc.size(), false, &trailing_padding);

  Output_section_data* posd = new Output_data_const(desc, 4);
  os->add_output_section_data(posd);

  if (trailing_padding > 0)
    {
      posd = new Output_data_zero_fill(trailing_padding, 0);
      os->add_output_section_data(posd);
    }
}

// Record whether the stack should be executable.  This can be set
// from the command line using the -z execstack or -z noexecstack
// options.  Otherwise, if any input file has a .note.GNU-stack
// section with the SHF_EXECINSTR flag set, the stack should be
// executable.  Otherwise, if at least one input file a
// .note.GNU-stack section, and some input file has no .note.GNU-stack
// section, we use the target default for whether the stack should be
// executable.  Otherwise, we don't generate a stack note.  When
// generating a object file, we create a .note.GNU-stack section with
// the appropriate marking.  When generating an executable or shared
// library, we create a PT_GNU_STACK segment.

void
Layout::create_executable_stack_info(const Target* target)
{
  bool is_stack_executable;
  if (this->options_.is_execstack_set())
    is_stack_executable = this->options_.is_stack_executable();
  else if (!this->input_with_gnu_stack_note_)
    return;
  else
    {
      if (this->input_requires_executable_stack_)
	is_stack_executable = true;
      else if (this->input_without_gnu_stack_note_)
	is_stack_executable = target->is_default_stack_executable();
      else
	is_stack_executable = false;
    }

  if (parameters->options().relocatable())
    {
      const char* name = this->namepool_.add(".note.GNU-stack", false, NULL);
      elfcpp::Elf_Xword flags = 0;
      if (is_stack_executable)
	flags |= elfcpp::SHF_EXECINSTR;
      this->make_output_section(name, elfcpp::SHT_PROGBITS, flags);
    }
  else
    {
      if (this->script_options_->saw_phdrs_clause())
	return;
      int flags = elfcpp::PF_R | elfcpp::PF_W;
      if (is_stack_executable)
	flags |= elfcpp::PF_X;
      this->make_output_segment(elfcpp::PT_GNU_STACK, flags);
    }
}

// If --build-id was used, set up the build ID note.

void
Layout::create_build_id()
{
  if (!parameters->options().user_set_build_id())
    return;

  const char* style = parameters->options().build_id();
  if (strcmp(style, "none") == 0)
    return;

  // Set DESCSZ to the size of the note descriptor.  When possible,
  // set DESC to the note descriptor contents.
  size_t descsz;
  std::string desc;
  if (strcmp(style, "md5") == 0)
    descsz = 128 / 8;
  else if (strcmp(style, "sha1") == 0)
    descsz = 160 / 8;
  else if (strcmp(style, "uuid") == 0)
    {
      const size_t uuidsz = 128 / 8;

      char buffer[uuidsz];
      memset(buffer, 0, uuidsz);

      int descriptor = open_descriptor(-1, "/dev/urandom", O_RDONLY);
      if (descriptor < 0)
	gold_error(_("--build-id=uuid failed: could not open /dev/urandom: %s"),
		   strerror(errno));
      else
	{
	  ssize_t got = ::read(descriptor, buffer, uuidsz);
	  release_descriptor(descriptor, true);
	  if (got < 0)
	    gold_error(_("/dev/urandom: read failed: %s"), strerror(errno));
	  else if (static_cast<size_t>(got) != uuidsz)
	    gold_error(_("/dev/urandom: expected %zu bytes, got %zd bytes"),
		       uuidsz, got);
	}

      desc.assign(buffer, uuidsz);
      descsz = uuidsz;
    }
  else if (strncmp(style, "0x", 2) == 0)
    {
      hex_init();
      const char* p = style + 2;
      while (*p != '\0')
	{
	  if (hex_p(p[0]) && hex_p(p[1]))
	    {
	      char c = (hex_value(p[0]) << 4) | hex_value(p[1]);
	      desc += c;
	      p += 2;
	    }
	  else if (*p == '-' || *p == ':')
	    ++p;
	  else
	    gold_fatal(_("--build-id argument '%s' not a valid hex number"),
		       style);
	}
      descsz = desc.size();
    }
  else
    gold_fatal(_("unrecognized --build-id argument '%s'"), style);

  // Create the note.
  size_t trailing_padding;
  Output_section* os = this->create_note("GNU", elfcpp::NT_GNU_BUILD_ID,
					 descsz, true, &trailing_padding);

  if (!desc.empty())
    {
      // We know the value already, so we fill it in now.
      gold_assert(desc.size() == descsz);

      Output_section_data* posd = new Output_data_const(desc, 4);
      os->add_output_section_data(posd);

      if (trailing_padding != 0)
	{
	  posd = new Output_data_zero_fill(trailing_padding, 0);
	  os->add_output_section_data(posd);
	}
    }
  else
    {
      // We need to compute a checksum after we have completed the
      // link.
      gold_assert(trailing_padding == 0);
      this->build_id_note_ = new Output_data_zero_fill(descsz, 4);
      os->add_output_section_data(this->build_id_note_);
      os->set_after_input_sections();
    }
}

// Return whether SEG1 should be before SEG2 in the output file.  This
// is based entirely on the segment type and flags.  When this is
// called the segment addresses has normally not yet been set.

bool
Layout::segment_precedes(const Output_segment* seg1,
			 const Output_segment* seg2)
{
  elfcpp::Elf_Word type1 = seg1->type();
  elfcpp::Elf_Word type2 = seg2->type();

  // The single PT_PHDR segment is required to precede any loadable
  // segment.  We simply make it always first.
  if (type1 == elfcpp::PT_PHDR)
    {
      gold_assert(type2 != elfcpp::PT_PHDR);
      return true;
    }
  if (type2 == elfcpp::PT_PHDR)
    return false;

  // The single PT_INTERP segment is required to precede any loadable
  // segment.  We simply make it always second.
  if (type1 == elfcpp::PT_INTERP)
    {
      gold_assert(type2 != elfcpp::PT_INTERP);
      return true;
    }
  if (type2 == elfcpp::PT_INTERP)
    return false;

  // We then put PT_LOAD segments before any other segments.
  if (type1 == elfcpp::PT_LOAD && type2 != elfcpp::PT_LOAD)
    return true;
  if (type2 == elfcpp::PT_LOAD && type1 != elfcpp::PT_LOAD)
    return false;

  // We put the PT_TLS segment last except for the PT_GNU_RELRO
  // segment, because that is where the dynamic linker expects to find
  // it (this is just for efficiency; other positions would also work
  // correctly).
  if (type1 == elfcpp::PT_TLS
      && type2 != elfcpp::PT_TLS
      && type2 != elfcpp::PT_GNU_RELRO)
    return false;
  if (type2 == elfcpp::PT_TLS
      && type1 != elfcpp::PT_TLS
      && type1 != elfcpp::PT_GNU_RELRO)
    return true;

  // We put the PT_GNU_RELRO segment last, because that is where the
  // dynamic linker expects to find it (as with PT_TLS, this is just
  // for efficiency).
  if (type1 == elfcpp::PT_GNU_RELRO && type2 != elfcpp::PT_GNU_RELRO)
    return false;
  if (type2 == elfcpp::PT_GNU_RELRO && type1 != elfcpp::PT_GNU_RELRO)
    return true;

  const elfcpp::Elf_Word flags1 = seg1->flags();
  const elfcpp::Elf_Word flags2 = seg2->flags();

  // The order of non-PT_LOAD segments is unimportant.  We simply sort
  // by the numeric segment type and flags values.  There should not
  // be more than one segment with the same type and flags.
  if (type1 != elfcpp::PT_LOAD)
    {
      if (type1 != type2)
	return type1 < type2;
      gold_assert(flags1 != flags2);
      return flags1 < flags2;
    }

  // If the addresses are set already, sort by load address.
  if (seg1->are_addresses_set())
    {
      if (!seg2->are_addresses_set())
	return true;

      unsigned int section_count1 = seg1->output_section_count();
      unsigned int section_count2 = seg2->output_section_count();
      if (section_count1 == 0 && section_count2 > 0)
	return true;
      if (section_count1 > 0 && section_count2 == 0)
	return false;

      uint64_t paddr1 = seg1->first_section_load_address();
      uint64_t paddr2 = seg2->first_section_load_address();
      if (paddr1 != paddr2)
	return paddr1 < paddr2;
    }
  else if (seg2->are_addresses_set())
    return false;

  // We sort PT_LOAD segments based on the flags.  Readonly segments
  // come before writable segments.  Then writable segments with data
  // come before writable segments without data.  Then executable
  // segments come before non-executable segments.  Then the unlikely
  // case of a non-readable segment comes before the normal case of a
  // readable segment.  If there are multiple segments with the same
  // type and flags, we require that the address be set, and we sort
  // by virtual address and then physical address.
  if ((flags1 & elfcpp::PF_W) != (flags2 & elfcpp::PF_W))
    return (flags1 & elfcpp::PF_W) == 0;
  if ((flags1 & elfcpp::PF_W) != 0
      && seg1->has_any_data_sections() != seg2->has_any_data_sections())
    return seg1->has_any_data_sections();
  if ((flags1 & elfcpp::PF_X) != (flags2 & elfcpp::PF_X))
    return (flags1 & elfcpp::PF_X) != 0;
  if ((flags1 & elfcpp::PF_R) != (flags2 & elfcpp::PF_R))
    return (flags1 & elfcpp::PF_R) == 0;

  // We shouldn't get here--we shouldn't create segments which we
  // can't distinguish.
  gold_unreachable();
}

// Set the file offsets of all the segments, and all the sections they
// contain.  They have all been created.  LOAD_SEG must be be laid out
// first.  Return the offset of the data to follow.

off_t
Layout::set_segment_offsets(const Target* target, Output_segment* load_seg,
			    unsigned int *pshndx)
{
  // Sort them into the final order.
  std::sort(this->segment_list_.begin(), this->segment_list_.end(),
	    Layout::Compare_segments());

  // Find the PT_LOAD segments, and set their addresses and offsets
  // and their section's addresses and offsets.
  uint64_t addr;
  if (this->options_.user_set_Ttext())
    addr = this->options_.Ttext();
  else if (parameters->options().shared())
    addr = 0;
  else
    addr = target->default_text_segment_address();
  off_t off = 0;

  // If LOAD_SEG is NULL, then the file header and segment headers
  // will not be loadable.  But they still need to be at offset 0 in
  // the file.  Set their offsets now.
  if (load_seg == NULL)
    {
      for (Data_list::iterator p = this->special_output_list_.begin();
	   p != this->special_output_list_.end();
	   ++p)
	{
	  off = align_address(off, (*p)->addralign());
	  (*p)->set_address_and_file_offset(0, off);
	  off += (*p)->data_size();
	}
    }

  const bool check_sections = parameters->options().check_sections();
  Output_segment* last_load_segment = NULL;

  bool was_readonly = false;
  for (Segment_list::iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    {
      if ((*p)->type() == elfcpp::PT_LOAD)
	{
	  if (load_seg != NULL && load_seg != *p)
	    gold_unreachable();
	  load_seg = NULL;

	  bool are_addresses_set = (*p)->are_addresses_set();
	  if (are_addresses_set)
	    {
	      // When it comes to setting file offsets, we care about
	      // the physical address.
	      addr = (*p)->paddr();
	    }
	  else if (this->options_.user_set_Tdata()
		   && ((*p)->flags() & elfcpp::PF_W) != 0
		   && (!this->options_.user_set_Tbss()
		       || (*p)->has_any_data_sections()))
	    {
	      addr = this->options_.Tdata();
	      are_addresses_set = true;
	    }
	  else if (this->options_.user_set_Tbss()
		   && ((*p)->flags() & elfcpp::PF_W) != 0
		   && !(*p)->has_any_data_sections())
	    {
	      addr = this->options_.Tbss();
	      are_addresses_set = true;
	    }

	  uint64_t orig_addr = addr;
	  uint64_t orig_off = off;

	  uint64_t aligned_addr = 0;
	  uint64_t abi_pagesize = target->abi_pagesize();
	  uint64_t common_pagesize = target->common_pagesize();

	  if (!parameters->options().nmagic()
	      && !parameters->options().omagic())
	    (*p)->set_minimum_p_align(common_pagesize);

	  if (are_addresses_set)
	    {
	      if (!parameters->options().nmagic()
		  && !parameters->options().omagic())
		{
		  // Adjust the file offset to the same address modulo
		  // the page size.
		  uint64_t unsigned_off = off;
		  uint64_t aligned_off = ((unsigned_off & ~(abi_pagesize - 1))
					  | (addr & (abi_pagesize - 1)));
		  if (aligned_off < unsigned_off)
		    aligned_off += abi_pagesize;
		  off = aligned_off;
		}
	    }
	  else
	    {
	      // If the last segment was readonly, and this one is
	      // not, then skip the address forward one page,
	      // maintaining the same position within the page.  This
	      // lets us store both segments overlapping on a single
	      // page in the file, but the loader will put them on
	      // different pages in memory.

	      addr = align_address(addr, (*p)->maximum_alignment());
	      aligned_addr = addr;

	      if (was_readonly && ((*p)->flags() & elfcpp::PF_W) != 0)
		{
		  if ((addr & (abi_pagesize - 1)) != 0)
		    addr = addr + abi_pagesize;
		}

	      off = orig_off + ((addr - orig_addr) & (abi_pagesize - 1));
	    }

	  unsigned int shndx_hold = *pshndx;
	  uint64_t new_addr = (*p)->set_section_addresses(this, false, addr,
                                                          &off, pshndx);

	  // Now that we know the size of this segment, we may be able
	  // to save a page in memory, at the cost of wasting some
	  // file space, by instead aligning to the start of a new
	  // page.  Here we use the real machine page size rather than
	  // the ABI mandated page size.

	  if (!are_addresses_set && aligned_addr != addr)
	    {
	      uint64_t first_off = (common_pagesize
				    - (aligned_addr
				       & (common_pagesize - 1)));
	      uint64_t last_off = new_addr & (common_pagesize - 1);
	      if (first_off > 0
		  && last_off > 0
		  && ((aligned_addr & ~ (common_pagesize - 1))
		      != (new_addr & ~ (common_pagesize - 1)))
		  && first_off + last_off <= common_pagesize)
		{
		  *pshndx = shndx_hold;
		  addr = align_address(aligned_addr, common_pagesize);
		  addr = align_address(addr, (*p)->maximum_alignment());
		  off = orig_off + ((addr - orig_addr) & (abi_pagesize - 1));
		  new_addr = (*p)->set_section_addresses(this, true, addr,
                                                         &off, pshndx);
		}
	    }

	  addr = new_addr;

	  if (((*p)->flags() & elfcpp::PF_W) == 0)
	    was_readonly = true;

	  // Implement --check-sections.  We know that the segments
	  // are sorted by LMA.
	  if (check_sections && last_load_segment != NULL)
	    {
	      gold_assert(last_load_segment->paddr() <= (*p)->paddr());
	      if (last_load_segment->paddr() + last_load_segment->memsz()
		  > (*p)->paddr())
		{
		  unsigned long long lb1 = last_load_segment->paddr();
		  unsigned long long le1 = lb1 + last_load_segment->memsz();
		  unsigned long long lb2 = (*p)->paddr();
		  unsigned long long le2 = lb2 + (*p)->memsz();
		  gold_error(_("load segment overlap [0x%llx -> 0x%llx] and "
			       "[0x%llx -> 0x%llx]"),
			     lb1, le1, lb2, le2);
		}
	    }
	  last_load_segment = *p;
	}
    }

  // Handle the non-PT_LOAD segments, setting their offsets from their
  // section's offsets.
  for (Segment_list::iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    {
      if ((*p)->type() != elfcpp::PT_LOAD)
	(*p)->set_offset();
    }

  // Set the TLS offsets for each section in the PT_TLS segment.
  if (this->tls_segment_ != NULL)
    this->tls_segment_->set_tls_offsets();

  return off;
}

// Set the offsets of all the allocated sections when doing a
// relocatable link.  This does the same jobs as set_segment_offsets,
// only for a relocatable link.

off_t
Layout::set_relocatable_section_offsets(Output_data* file_header,
					unsigned int *pshndx)
{
  off_t off = 0;

  file_header->set_address_and_file_offset(0, 0);
  off += file_header->data_size();

  for (Section_list::iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    {
      // We skip unallocated sections here, except that group sections
      // have to come first.
      if (((*p)->flags() & elfcpp::SHF_ALLOC) == 0
	  && (*p)->type() != elfcpp::SHT_GROUP)
	continue;

      off = align_address(off, (*p)->addralign());

      // The linker script might have set the address.
      if (!(*p)->is_address_valid())
	(*p)->set_address(0);
      (*p)->set_file_offset(off);
      (*p)->finalize_data_size();
      off += (*p)->data_size();

      (*p)->set_out_shndx(*pshndx);
      ++*pshndx;
    }

  return off;
}

// Set the file offset of all the sections not associated with a
// segment.

off_t
Layout::set_section_offsets(off_t off, Layout::Section_offset_pass pass)
{
  for (Section_list::iterator p = this->unattached_section_list_.begin();
       p != this->unattached_section_list_.end();
       ++p)
    {
      // The symtab section is handled in create_symtab_sections.
      if (*p == this->symtab_section_)
	continue;

      // If we've already set the data size, don't set it again.
      if ((*p)->is_offset_valid() && (*p)->is_data_size_valid())
	continue;

      if (pass == BEFORE_INPUT_SECTIONS_PASS
	  && (*p)->requires_postprocessing())
	{
	  (*p)->create_postprocessing_buffer();
	  this->any_postprocessing_sections_ = true;
	}

      if (pass == BEFORE_INPUT_SECTIONS_PASS
          && (*p)->after_input_sections())
        continue;
      else if (pass == POSTPROCESSING_SECTIONS_PASS
               && (!(*p)->after_input_sections()
                   || (*p)->type() == elfcpp::SHT_STRTAB))
        continue;
      else if (pass == STRTAB_AFTER_POSTPROCESSING_SECTIONS_PASS
               && (!(*p)->after_input_sections()
                   || (*p)->type() != elfcpp::SHT_STRTAB))
        continue;

      off = align_address(off, (*p)->addralign());
      (*p)->set_file_offset(off);
      (*p)->finalize_data_size();
      off += (*p)->data_size();

      // At this point the name must be set.
      if (pass != STRTAB_AFTER_POSTPROCESSING_SECTIONS_PASS)
	this->namepool_.add((*p)->name(), false, NULL);
    }
  return off;
}

// Set the section indexes of all the sections not associated with a
// segment.

unsigned int
Layout::set_section_indexes(unsigned int shndx)
{
  for (Section_list::iterator p = this->unattached_section_list_.begin();
       p != this->unattached_section_list_.end();
       ++p)
    {
      if (!(*p)->has_out_shndx())
	{
	  (*p)->set_out_shndx(shndx);
	  ++shndx;
	}
    }
  return shndx;
}

// Set the section addresses according to the linker script.  This is
// only called when we see a SECTIONS clause.  This returns the
// program segment which should hold the file header and segment
// headers, if any.  It will return NULL if they should not be in a
// segment.

Output_segment*
Layout::set_section_addresses_from_script(Symbol_table* symtab)
{
  Script_sections* ss = this->script_options_->script_sections();
  gold_assert(ss->saw_sections_clause());

  // Place each orphaned output section in the script.
  for (Section_list::iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    {
      if (!(*p)->found_in_sections_clause())
	ss->place_orphan(*p);
    }

  return this->script_options_->set_section_addresses(symtab, this);
}

// Count the local symbols in the regular symbol table and the dynamic
// symbol table, and build the respective string pools.

void
Layout::count_local_symbols(const Task* task,
			    const Input_objects* input_objects)
{
  // First, figure out an upper bound on the number of symbols we'll
  // be inserting into each pool.  This helps us create the pools with
  // the right size, to avoid unnecessary hashtable resizing.
  unsigned int symbol_count = 0;
  for (Input_objects::Relobj_iterator p = input_objects->relobj_begin();
       p != input_objects->relobj_end();
       ++p)
    symbol_count += (*p)->local_symbol_count();

  // Go from "upper bound" to "estimate."  We overcount for two
  // reasons: we double-count symbols that occur in more than one
  // object file, and we count symbols that are dropped from the
  // output.  Add it all together and assume we overcount by 100%.
  symbol_count /= 2;

  // We assume all symbols will go into both the sympool and dynpool.
  this->sympool_.reserve(symbol_count);
  this->dynpool_.reserve(symbol_count);

  for (Input_objects::Relobj_iterator p = input_objects->relobj_begin();
       p != input_objects->relobj_end();
       ++p)
    {
      Task_lock_obj<Object> tlo(task, *p);
      (*p)->count_local_symbols(&this->sympool_, &this->dynpool_);
    }
}

// Create the symbol table sections.  Here we also set the final
// values of the symbols.  At this point all the loadable sections are
// fully laid out.  SHNUM is the number of sections so far.

void
Layout::create_symtab_sections(const Input_objects* input_objects,
			       Symbol_table* symtab,
			       unsigned int shnum,
			       off_t* poff)
{
  int symsize;
  unsigned int align;
  if (parameters->target().get_size() == 32)
    {
      symsize = elfcpp::Elf_sizes<32>::sym_size;
      align = 4;
    }
  else if (parameters->target().get_size() == 64)
    {
      symsize = elfcpp::Elf_sizes<64>::sym_size;
      align = 8;
    }
  else
    gold_unreachable();

  off_t off = *poff;
  off = align_address(off, align);
  off_t startoff = off;

  // Save space for the dummy symbol at the start of the section.  We
  // never bother to write this out--it will just be left as zero.
  off += symsize;
  unsigned int local_symbol_index = 1;

  // Add STT_SECTION symbols for each Output section which needs one.
  for (Section_list::iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    {
      if (!(*p)->needs_symtab_index())
	(*p)->set_symtab_index(-1U);
      else
	{
	  (*p)->set_symtab_index(local_symbol_index);
	  ++local_symbol_index;
	  off += symsize;
	}
    }

  for (Input_objects::Relobj_iterator p = input_objects->relobj_begin();
       p != input_objects->relobj_end();
       ++p)
    {
      unsigned int index = (*p)->finalize_local_symbols(local_symbol_index,
                                                        off);
      off += (index - local_symbol_index) * symsize;
      local_symbol_index = index;
    }

  unsigned int local_symcount = local_symbol_index;
  gold_assert(local_symcount * symsize == off - startoff);

  off_t dynoff;
  size_t dyn_global_index;
  size_t dyncount;
  if (this->dynsym_section_ == NULL)
    {
      dynoff = 0;
      dyn_global_index = 0;
      dyncount = 0;
    }
  else
    {
      dyn_global_index = this->dynsym_section_->info();
      off_t locsize = dyn_global_index * this->dynsym_section_->entsize();
      dynoff = this->dynsym_section_->offset() + locsize;
      dyncount = (this->dynsym_section_->data_size() - locsize) / symsize;
      gold_assert(static_cast<off_t>(dyncount * symsize)
		  == this->dynsym_section_->data_size() - locsize);
    }

  off = symtab->finalize(off, dynoff, dyn_global_index, dyncount,
			 &this->sympool_, &local_symcount);

  if (!parameters->options().strip_all())
    {
      this->sympool_.set_string_offsets();

      const char* symtab_name = this->namepool_.add(".symtab", false, NULL);
      Output_section* osymtab = this->make_output_section(symtab_name,
							  elfcpp::SHT_SYMTAB,
							  0);
      this->symtab_section_ = osymtab;

      Output_section_data* pos = new Output_data_fixed_space(off - startoff,
							     align,
							     "** symtab");
      osymtab->add_output_section_data(pos);

      // We generate a .symtab_shndx section if we have more than
      // SHN_LORESERVE sections.  Technically it is possible that we
      // don't need one, because it is possible that there are no
      // symbols in any of sections with indexes larger than
      // SHN_LORESERVE.  That is probably unusual, though, and it is
      // easier to always create one than to compute section indexes
      // twice (once here, once when writing out the symbols).
      if (shnum >= elfcpp::SHN_LORESERVE)
	{
	  const char* symtab_xindex_name = this->namepool_.add(".symtab_shndx",
							       false, NULL);
	  Output_section* osymtab_xindex =
	    this->make_output_section(symtab_xindex_name,
				      elfcpp::SHT_SYMTAB_SHNDX, 0);

	  size_t symcount = (off - startoff) / symsize;
	  this->symtab_xindex_ = new Output_symtab_xindex(symcount);

	  osymtab_xindex->add_output_section_data(this->symtab_xindex_);

	  osymtab_xindex->set_link_section(osymtab);
	  osymtab_xindex->set_addralign(4);
	  osymtab_xindex->set_entsize(4);

	  osymtab_xindex->set_after_input_sections();

	  // This tells the driver code to wait until the symbol table
	  // has written out before writing out the postprocessing
	  // sections, including the .symtab_shndx section.
	  this->any_postprocessing_sections_ = true;
	}

      const char* strtab_name = this->namepool_.add(".strtab", false, NULL);
      Output_section* ostrtab = this->make_output_section(strtab_name,
							  elfcpp::SHT_STRTAB,
							  0);

      Output_section_data* pstr = new Output_data_strtab(&this->sympool_);
      ostrtab->add_output_section_data(pstr);

      osymtab->set_file_offset(startoff);
      osymtab->finalize_data_size();
      osymtab->set_link_section(ostrtab);
      osymtab->set_info(local_symcount);
      osymtab->set_entsize(symsize);

      *poff = off;
    }
}

// Create the .shstrtab section, which holds the names of the
// sections.  At the time this is called, we have created all the
// output sections except .shstrtab itself.

Output_section*
Layout::create_shstrtab()
{
  // FIXME: We don't need to create a .shstrtab section if we are
  // stripping everything.

  const char* name = this->namepool_.add(".shstrtab", false, NULL);

  Output_section* os = this->make_output_section(name, elfcpp::SHT_STRTAB, 0);

  // We can't write out this section until we've set all the section
  // names, and we don't set the names of compressed output sections
  // until relocations are complete.
  os->set_after_input_sections();

  Output_section_data* posd = new Output_data_strtab(&this->namepool_);
  os->add_output_section_data(posd);

  return os;
}

// Create the section headers.  SIZE is 32 or 64.  OFF is the file
// offset.

void
Layout::create_shdrs(const Output_section* shstrtab_section, off_t* poff)
{
  Output_section_headers* oshdrs;
  oshdrs = new Output_section_headers(this,
				      &this->segment_list_,
				      &this->section_list_,
				      &this->unattached_section_list_,
				      &this->namepool_,
				      shstrtab_section);
  off_t off = align_address(*poff, oshdrs->addralign());
  oshdrs->set_address_and_file_offset(0, off);
  off += oshdrs->data_size();
  *poff = off;
  this->section_headers_ = oshdrs;
}

// Count the allocated sections.

size_t
Layout::allocated_output_section_count() const
{
  size_t section_count = 0;
  for (Segment_list::const_iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    section_count += (*p)->output_section_count();
  return section_count;
}

// Create the dynamic symbol table.

void
Layout::create_dynamic_symtab(const Input_objects* input_objects,
                              Symbol_table* symtab,
			      Output_section **pdynstr,
			      unsigned int* plocal_dynamic_count,
			      std::vector<Symbol*>* pdynamic_symbols,
			      Versions* pversions)
{
  // Count all the symbols in the dynamic symbol table, and set the
  // dynamic symbol indexes.

  // Skip symbol 0, which is always all zeroes.
  unsigned int index = 1;

  // Add STT_SECTION symbols for each Output section which needs one.
  for (Section_list::iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    {
      if (!(*p)->needs_dynsym_index())
	(*p)->set_dynsym_index(-1U);
      else
	{
	  (*p)->set_dynsym_index(index);
	  ++index;
	}
    }

  // Count the local symbols that need to go in the dynamic symbol table,
  // and set the dynamic symbol indexes.
  for (Input_objects::Relobj_iterator p = input_objects->relobj_begin();
       p != input_objects->relobj_end();
       ++p)
    {
      unsigned int new_index = (*p)->set_local_dynsym_indexes(index);
      index = new_index;
    }

  unsigned int local_symcount = index;
  *plocal_dynamic_count = local_symcount;

  index = symtab->set_dynsym_indexes(index, pdynamic_symbols,
				     &this->dynpool_, pversions);

  int symsize;
  unsigned int align;
  const int size = parameters->target().get_size();
  if (size == 32)
    {
      symsize = elfcpp::Elf_sizes<32>::sym_size;
      align = 4;
    }
  else if (size == 64)
    {
      symsize = elfcpp::Elf_sizes<64>::sym_size;
      align = 8;
    }
  else
    gold_unreachable();

  // Create the dynamic symbol table section.

  Output_section* dynsym = this->choose_output_section(NULL, ".dynsym",
						       elfcpp::SHT_DYNSYM,
						       elfcpp::SHF_ALLOC,
						       false);

  Output_section_data* odata = new Output_data_fixed_space(index * symsize,
							   align,
							   "** dynsym");
  dynsym->add_output_section_data(odata);

  dynsym->set_info(local_symcount);
  dynsym->set_entsize(symsize);
  dynsym->set_addralign(align);

  this->dynsym_section_ = dynsym;

  Output_data_dynamic* const odyn = this->dynamic_data_;
  odyn->add_section_address(elfcpp::DT_SYMTAB, dynsym);
  odyn->add_constant(elfcpp::DT_SYMENT, symsize);

  // If there are more than SHN_LORESERVE allocated sections, we
  // create a .dynsym_shndx section.  It is possible that we don't
  // need one, because it is possible that there are no dynamic
  // symbols in any of the sections with indexes larger than
  // SHN_LORESERVE.  This is probably unusual, though, and at this
  // time we don't know the actual section indexes so it is
  // inconvenient to check.
  if (this->allocated_output_section_count() >= elfcpp::SHN_LORESERVE)
    {
      Output_section* dynsym_xindex =
	this->choose_output_section(NULL, ".dynsym_shndx",
				    elfcpp::SHT_SYMTAB_SHNDX,
				    elfcpp::SHF_ALLOC,
				    false);

      this->dynsym_xindex_ = new Output_symtab_xindex(index);

      dynsym_xindex->add_output_section_data(this->dynsym_xindex_);

      dynsym_xindex->set_link_section(dynsym);
      dynsym_xindex->set_addralign(4);
      dynsym_xindex->set_entsize(4);

      dynsym_xindex->set_after_input_sections();

      // This tells the driver code to wait until the symbol table has
      // written out before writing out the postprocessing sections,
      // including the .dynsym_shndx section.
      this->any_postprocessing_sections_ = true;
    }

  // Create the dynamic string table section.

  Output_section* dynstr = this->choose_output_section(NULL, ".dynstr",
						       elfcpp::SHT_STRTAB,
						       elfcpp::SHF_ALLOC,
						       false);

  Output_section_data* strdata = new Output_data_strtab(&this->dynpool_);
  dynstr->add_output_section_data(strdata);

  dynsym->set_link_section(dynstr);
  this->dynamic_section_->set_link_section(dynstr);

  odyn->add_section_address(elfcpp::DT_STRTAB, dynstr);
  odyn->add_section_size(elfcpp::DT_STRSZ, dynstr);

  *pdynstr = dynstr;

  // Create the hash tables.

  if (strcmp(parameters->options().hash_style(), "sysv") == 0
      || strcmp(parameters->options().hash_style(), "both") == 0)
    {
      unsigned char* phash;
      unsigned int hashlen;
      Dynobj::create_elf_hash_table(*pdynamic_symbols, local_symcount,
				    &phash, &hashlen);

      Output_section* hashsec = this->choose_output_section(NULL, ".hash",
							    elfcpp::SHT_HASH,
							    elfcpp::SHF_ALLOC,
							    false);

      Output_section_data* hashdata = new Output_data_const_buffer(phash,
								   hashlen,
								   align,
								   "** hash");
      hashsec->add_output_section_data(hashdata);

      hashsec->set_link_section(dynsym);
      hashsec->set_entsize(4);

      odyn->add_section_address(elfcpp::DT_HASH, hashsec);
    }

  if (strcmp(parameters->options().hash_style(), "gnu") == 0
      || strcmp(parameters->options().hash_style(), "both") == 0)
    {
      unsigned char* phash;
      unsigned int hashlen;
      Dynobj::create_gnu_hash_table(*pdynamic_symbols, local_symcount,
				    &phash, &hashlen);

      Output_section* hashsec = this->choose_output_section(NULL, ".gnu.hash",
							    elfcpp::SHT_GNU_HASH,
							    elfcpp::SHF_ALLOC,
							    false);

      Output_section_data* hashdata = new Output_data_const_buffer(phash,
								   hashlen,
								   align,
								   "** hash");
      hashsec->add_output_section_data(hashdata);

      hashsec->set_link_section(dynsym);
      hashsec->set_entsize(4);

      odyn->add_section_address(elfcpp::DT_GNU_HASH, hashsec);
    }
}

// Assign offsets to each local portion of the dynamic symbol table.

void
Layout::assign_local_dynsym_offsets(const Input_objects* input_objects)
{
  Output_section* dynsym = this->dynsym_section_;
  gold_assert(dynsym != NULL);

  off_t off = dynsym->offset();

  // Skip the dummy symbol at the start of the section.
  off += dynsym->entsize();

  for (Input_objects::Relobj_iterator p = input_objects->relobj_begin();
       p != input_objects->relobj_end();
       ++p)
    {
      unsigned int count = (*p)->set_local_dynsym_offset(off);
      off += count * dynsym->entsize();
    }
}

// Create the version sections.

void
Layout::create_version_sections(const Versions* versions,
				const Symbol_table* symtab,
				unsigned int local_symcount,
				const std::vector<Symbol*>& dynamic_symbols,
				const Output_section* dynstr)
{
  if (!versions->any_defs() && !versions->any_needs())
    return;

  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->sized_create_version_sections<32, false>(versions, symtab,
						     local_symcount,
						     dynamic_symbols, dynstr);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->sized_create_version_sections<32, true>(versions, symtab,
						    local_symcount,
						    dynamic_symbols, dynstr);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->sized_create_version_sections<64, false>(versions, symtab,
						     local_symcount,
						     dynamic_symbols, dynstr);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->sized_create_version_sections<64, true>(versions, symtab,
						    local_symcount,
						    dynamic_symbols, dynstr);
      break;
#endif
    default:
      gold_unreachable();
    }
}

// Create the version sections, sized version.

template<int size, bool big_endian>
void
Layout::sized_create_version_sections(
    const Versions* versions,
    const Symbol_table* symtab,
    unsigned int local_symcount,
    const std::vector<Symbol*>& dynamic_symbols,
    const Output_section* dynstr)
{
  Output_section* vsec = this->choose_output_section(NULL, ".gnu.version",
						     elfcpp::SHT_GNU_versym,
						     elfcpp::SHF_ALLOC,
						     false);

  unsigned char* vbuf;
  unsigned int vsize;
  versions->symbol_section_contents<size, big_endian>(symtab, &this->dynpool_,
						      local_symcount,
						      dynamic_symbols,
						      &vbuf, &vsize);

  Output_section_data* vdata = new Output_data_const_buffer(vbuf, vsize, 2,
							    "** versions");

  vsec->add_output_section_data(vdata);
  vsec->set_entsize(2);
  vsec->set_link_section(this->dynsym_section_);

  Output_data_dynamic* const odyn = this->dynamic_data_;
  odyn->add_section_address(elfcpp::DT_VERSYM, vsec);

  if (versions->any_defs())
    {
      Output_section* vdsec;
      vdsec= this->choose_output_section(NULL, ".gnu.version_d",
					 elfcpp::SHT_GNU_verdef,
					 elfcpp::SHF_ALLOC,
					 false);

      unsigned char* vdbuf;
      unsigned int vdsize;
      unsigned int vdentries;
      versions->def_section_contents<size, big_endian>(&this->dynpool_, &vdbuf,
						       &vdsize, &vdentries);

      Output_section_data* vddata =
	new Output_data_const_buffer(vdbuf, vdsize, 4, "** version defs");

      vdsec->add_output_section_data(vddata);
      vdsec->set_link_section(dynstr);
      vdsec->set_info(vdentries);

      odyn->add_section_address(elfcpp::DT_VERDEF, vdsec);
      odyn->add_constant(elfcpp::DT_VERDEFNUM, vdentries);
    }

  if (versions->any_needs())
    {
      Output_section* vnsec;
      vnsec = this->choose_output_section(NULL, ".gnu.version_r",
					  elfcpp::SHT_GNU_verneed,
					  elfcpp::SHF_ALLOC,
					  false);

      unsigned char* vnbuf;
      unsigned int vnsize;
      unsigned int vnentries;
      versions->need_section_contents<size, big_endian>(&this->dynpool_,
							&vnbuf, &vnsize,
							&vnentries);

      Output_section_data* vndata =
	new Output_data_const_buffer(vnbuf, vnsize, 4, "** version refs");

      vnsec->add_output_section_data(vndata);
      vnsec->set_link_section(dynstr);
      vnsec->set_info(vnentries);

      odyn->add_section_address(elfcpp::DT_VERNEED, vnsec);
      odyn->add_constant(elfcpp::DT_VERNEEDNUM, vnentries);
    }
}

// Create the .interp section and PT_INTERP segment.

void
Layout::create_interp(const Target* target)
{
  const char* interp = this->options_.dynamic_linker();
  if (interp == NULL)
    {
      interp = target->dynamic_linker();
      gold_assert(interp != NULL);
    }

  size_t len = strlen(interp) + 1;

  Output_section_data* odata = new Output_data_const(interp, len, 1);

  Output_section* osec = this->choose_output_section(NULL, ".interp",
						     elfcpp::SHT_PROGBITS,
						     elfcpp::SHF_ALLOC,
						     false);
  osec->add_output_section_data(odata);

  if (!this->script_options_->saw_phdrs_clause())
    {
      Output_segment* oseg = this->make_output_segment(elfcpp::PT_INTERP,
						       elfcpp::PF_R);
      oseg->add_output_section(osec, elfcpp::PF_R);
    }
}

// Finish the .dynamic section and PT_DYNAMIC segment.

void
Layout::finish_dynamic_section(const Input_objects* input_objects,
			       const Symbol_table* symtab)
{
  if (!this->script_options_->saw_phdrs_clause())
    {
      Output_segment* oseg = this->make_output_segment(elfcpp::PT_DYNAMIC,
						       (elfcpp::PF_R
							| elfcpp::PF_W));
      oseg->add_output_section(this->dynamic_section_,
			       elfcpp::PF_R | elfcpp::PF_W);
    }

  Output_data_dynamic* const odyn = this->dynamic_data_;

  for (Input_objects::Dynobj_iterator p = input_objects->dynobj_begin();
       p != input_objects->dynobj_end();
       ++p)
    {
      // FIXME: Handle --as-needed.
      odyn->add_string(elfcpp::DT_NEEDED, (*p)->soname());
    }

  if (parameters->options().shared())
    {
      const char* soname = this->options_.soname();
      if (soname != NULL)
	odyn->add_string(elfcpp::DT_SONAME, soname);
    }

  // FIXME: Support --init and --fini.
  Symbol* sym = symtab->lookup("_init");
  if (sym != NULL && sym->is_defined() && !sym->is_from_dynobj())
    odyn->add_symbol(elfcpp::DT_INIT, sym);

  sym = symtab->lookup("_fini");
  if (sym != NULL && sym->is_defined() && !sym->is_from_dynobj())
    odyn->add_symbol(elfcpp::DT_FINI, sym);

  // FIXME: Support DT_INIT_ARRAY and DT_FINI_ARRAY.

  // Add a DT_RPATH entry if needed.
  const General_options::Dir_list& rpath(this->options_.rpath());
  if (!rpath.empty())
    {
      std::string rpath_val;
      for (General_options::Dir_list::const_iterator p = rpath.begin();
           p != rpath.end();
           ++p)
        {
          if (rpath_val.empty())
            rpath_val = p->name();
          else
            {
              // Eliminate duplicates.
              General_options::Dir_list::const_iterator q;
              for (q = rpath.begin(); q != p; ++q)
		if (q->name() == p->name())
                  break;
              if (q == p)
                {
                  rpath_val += ':';
                  rpath_val += p->name();
                }
            }
        }

      odyn->add_string(elfcpp::DT_RPATH, rpath_val);
      if (parameters->options().enable_new_dtags())
	odyn->add_string(elfcpp::DT_RUNPATH, rpath_val);
    }

  // Look for text segments that have dynamic relocations.
  bool have_textrel = false;
  if (!this->script_options_->saw_sections_clause())
    {
      for (Segment_list::const_iterator p = this->segment_list_.begin();
           p != this->segment_list_.end();
           ++p)
        {
          if (((*p)->flags() & elfcpp::PF_W) == 0
              && (*p)->dynamic_reloc_count() > 0)
            {
              have_textrel = true;
              break;
            }
        }
    }
  else
    {
      // We don't know the section -> segment mapping, so we are
      // conservative and just look for readonly sections with
      // relocations.  If those sections wind up in writable segments,
      // then we have created an unnecessary DT_TEXTREL entry.
      for (Section_list::const_iterator p = this->section_list_.begin();
           p != this->section_list_.end();
           ++p)
        {
          if (((*p)->flags() & elfcpp::SHF_ALLOC) != 0
              && ((*p)->flags() & elfcpp::SHF_WRITE) == 0
              && ((*p)->dynamic_reloc_count() > 0))
            {
              have_textrel = true;
              break;
            }
        }
    }

  // Add a DT_FLAGS entry. We add it even if no flags are set so that
  // post-link tools can easily modify these flags if desired.
  unsigned int flags = 0;
  if (have_textrel)
    {
      // Add a DT_TEXTREL for compatibility with older loaders.
      odyn->add_constant(elfcpp::DT_TEXTREL, 0);
      flags |= elfcpp::DF_TEXTREL;
    }
  if (parameters->options().shared() && this->has_static_tls())
    flags |= elfcpp::DF_STATIC_TLS;
  odyn->add_constant(elfcpp::DT_FLAGS, flags);

  flags = 0;
  if (parameters->options().initfirst())
    flags |= elfcpp::DF_1_INITFIRST;
  if (parameters->options().interpose())
    flags |= elfcpp::DF_1_INTERPOSE;
  if (parameters->options().loadfltr())
    flags |= elfcpp::DF_1_LOADFLTR;
  if (parameters->options().nodefaultlib())
    flags |= elfcpp::DF_1_NODEFLIB;
  if (parameters->options().nodelete())
    flags |= elfcpp::DF_1_NODELETE;
  if (parameters->options().nodlopen())
    flags |= elfcpp::DF_1_NOOPEN;
  if (parameters->options().nodump())
    flags |= elfcpp::DF_1_NODUMP;
  if (!parameters->options().shared())
    flags &= ~(elfcpp::DF_1_INITFIRST
	       | elfcpp::DF_1_NODELETE
	       | elfcpp::DF_1_NOOPEN);
  if (flags)
    odyn->add_constant(elfcpp::DT_FLAGS_1, flags);
}

// The mapping of .gnu.linkonce section names to real section names.

#define MAPPING_INIT(f, t) { f, sizeof(f) - 1, t, sizeof(t) - 1 }
const Layout::Linkonce_mapping Layout::linkonce_mapping[] =
{
  MAPPING_INIT("d.rel.ro.local", ".data.rel.ro.local"), // Before "d.rel.ro".
  MAPPING_INIT("d.rel.ro", ".data.rel.ro"),		// Before "d".
  MAPPING_INIT("t", ".text"),
  MAPPING_INIT("r", ".rodata"),
  MAPPING_INIT("d", ".data"),
  MAPPING_INIT("b", ".bss"),
  MAPPING_INIT("s", ".sdata"),
  MAPPING_INIT("sb", ".sbss"),
  MAPPING_INIT("s2", ".sdata2"),
  MAPPING_INIT("sb2", ".sbss2"),
  MAPPING_INIT("wi", ".debug_info"),
  MAPPING_INIT("td", ".tdata"),
  MAPPING_INIT("tb", ".tbss"),
  MAPPING_INIT("lr", ".lrodata"),
  MAPPING_INIT("l", ".ldata"),
  MAPPING_INIT("lb", ".lbss"),
};
#undef MAPPING_INIT

const int Layout::linkonce_mapping_count =
  sizeof(Layout::linkonce_mapping) / sizeof(Layout::linkonce_mapping[0]);

// Return the name of the output section to use for a .gnu.linkonce
// section.  This is based on the default ELF linker script of the old
// GNU linker.  For example, we map a name like ".gnu.linkonce.t.foo"
// to ".text".  Set *PLEN to the length of the name.  *PLEN is
// initialized to the length of NAME.

const char*
Layout::linkonce_output_name(const char* name, size_t *plen)
{
  const char* s = name + sizeof(".gnu.linkonce") - 1;
  if (*s != '.')
    return name;
  ++s;
  const Linkonce_mapping* plm = linkonce_mapping;
  for (int i = 0; i < linkonce_mapping_count; ++i, ++plm)
    {
      if (strncmp(s, plm->from, plm->fromlen) == 0 && s[plm->fromlen] == '.')
	{
	  *plen = plm->tolen;
	  return plm->to;
	}
    }
  return name;
}

// Choose the output section name to use given an input section name.
// Set *PLEN to the length of the name.  *PLEN is initialized to the
// length of NAME.

const char*
Layout::output_section_name(const char* name, size_t* plen)
{
  if (Layout::is_linkonce(name))
    {
      // .gnu.linkonce sections are laid out as though they were named
      // for the sections are placed into.
      return Layout::linkonce_output_name(name, plen);
    }

  // gcc 4.3 generates the following sorts of section names when it
  // needs a section name specific to a function:
  //   .text.FN
  //   .rodata.FN
  //   .sdata2.FN
  //   .data.FN
  //   .data.rel.FN
  //   .data.rel.local.FN
  //   .data.rel.ro.FN
  //   .data.rel.ro.local.FN
  //   .sdata.FN
  //   .bss.FN
  //   .sbss.FN
  //   .tdata.FN
  //   .tbss.FN

  // The GNU linker maps all of those to the part before the .FN,
  // except that .data.rel.local.FN is mapped to .data, and
  // .data.rel.ro.local.FN is mapped to .data.rel.ro.  The sections
  // beginning with .data.rel.ro.local are grouped together.

  // For an anonymous namespace, the string FN can contain a '.'.

  // Also of interest: .rodata.strN.N, .rodata.cstN, both of which the
  // GNU linker maps to .rodata.

  // The .data.rel.ro sections enable a security feature triggered by
  // the -z relro option.  Section which need to be relocated at
  // program startup time but which may be readonly after startup are
  // grouped into .data.rel.ro.  They are then put into a PT_GNU_RELRO
  // segment.  The dynamic linker will make that segment writable,
  // perform relocations, and then make it read-only.  FIXME: We do
  // not yet implement this optimization.

  // It is hard to handle this in a principled way.

  // These are the rules we follow:

  // If the section name has no initial '.', or no dot other than an
  // initial '.', we use the name unchanged (i.e., "mysection" and
  // ".text" are unchanged).

  // If the name starts with ".data.rel.ro.local" we use
  // ".data.rel.ro.local".

  // If the name starts with ".data.rel.ro" we use ".data.rel.ro".

  // Otherwise, we drop the second '.' and everything that comes after
  // it (i.e., ".text.XXX" becomes ".text").

  const char* s = name;
  if (*s != '.')
    return name;
  ++s;
  const char* sdot = strchr(s, '.');
  if (sdot == NULL)
    return name;

  const char* const data_rel_ro_local = ".data.rel.ro.local";
  if (strncmp(name, data_rel_ro_local, strlen(data_rel_ro_local)) == 0)
    {
      *plen = strlen(data_rel_ro_local);
      return data_rel_ro_local;
    }

  const char* const data_rel_ro = ".data.rel.ro";
  if (strncmp(name, data_rel_ro, strlen(data_rel_ro)) == 0)
    {
      *plen = strlen(data_rel_ro);
      return data_rel_ro;
    }

  *plen = sdot - name;
  return name;
}

// Record the signature of a comdat section, and return whether to
// include it in the link.  If GROUP is true, this is a regular
// section group.  If GROUP is false, this is a group signature
// derived from the name of a linkonce section.  We want linkonce
// signatures and group signatures to block each other, but we don't
// want a linkonce signature to block another linkonce signature.

bool
Layout::add_comdat(Relobj* object, unsigned int shndx,
                   const std::string& signature, bool group)
{
  Kept_section kept(object, shndx, group);
  std::pair<Signatures::iterator, bool> ins(
    this->signatures_.insert(std::make_pair(signature, kept)));

  if (ins.second)
    {
      // This is the first time we've seen this signature.
      return true;
    }

  if (ins.first->second.group_)
    {
      // We've already seen a real section group with this signature.
      return false;
    }
  else if (group)
    {
      // This is a real section group, and we've already seen a
      // linkonce section with this signature.  Record that we've seen
      // a section group, and don't include this section group.
      ins.first->second.group_ = true;
      return false;
    }
  else
    {
      // We've already seen a linkonce section and this is a linkonce
      // section.  These don't block each other--this may be the same
      // symbol name with different section types.
      return true;
    }
}

// Find the given comdat signature, and return the object and section
// index of the kept group.
Relobj*
Layout::find_kept_object(const std::string& signature,
                         unsigned int* pshndx) const
{
  Signatures::const_iterator p = this->signatures_.find(signature);
  if (p == this->signatures_.end())
    return NULL;
  if (pshndx != NULL)
    *pshndx = p->second.shndx_;
  return p->second.object_;
}

// Store the allocated sections into the section list.

void
Layout::get_allocated_sections(Section_list* section_list) const
{
  for (Section_list::const_iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    if (((*p)->flags() & elfcpp::SHF_ALLOC) != 0)
      section_list->push_back(*p);
}

// Create an output segment.

Output_segment*
Layout::make_output_segment(elfcpp::Elf_Word type, elfcpp::Elf_Word flags)
{
  gold_assert(!parameters->options().relocatable());
  Output_segment* oseg = new Output_segment(type, flags);
  this->segment_list_.push_back(oseg);

  if (type == elfcpp::PT_TLS)
    this->tls_segment_ = oseg;
  else if (type == elfcpp::PT_GNU_RELRO)
    this->relro_segment_ = oseg;

  return oseg;
}

// Write out the Output_sections.  Most won't have anything to write,
// since most of the data will come from input sections which are
// handled elsewhere.  But some Output_sections do have Output_data.

void
Layout::write_output_sections(Output_file* of) const
{
  for (Section_list::const_iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    {
      if (!(*p)->after_input_sections())
	(*p)->write(of);
    }
}

// Write out data not associated with a section or the symbol table.

void
Layout::write_data(const Symbol_table* symtab, Output_file* of) const
{
  if (!parameters->options().strip_all())
    {
      const Output_section* symtab_section = this->symtab_section_;
      for (Section_list::const_iterator p = this->section_list_.begin();
	   p != this->section_list_.end();
	   ++p)
	{
	  if ((*p)->needs_symtab_index())
	    {
	      gold_assert(symtab_section != NULL);
	      unsigned int index = (*p)->symtab_index();
	      gold_assert(index > 0 && index != -1U);
	      off_t off = (symtab_section->offset()
			   + index * symtab_section->entsize());
	      symtab->write_section_symbol(*p, this->symtab_xindex_, of, off);
	    }
	}
    }

  const Output_section* dynsym_section = this->dynsym_section_;
  for (Section_list::const_iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    {
      if ((*p)->needs_dynsym_index())
	{
	  gold_assert(dynsym_section != NULL);
	  unsigned int index = (*p)->dynsym_index();
	  gold_assert(index > 0 && index != -1U);
	  off_t off = (dynsym_section->offset()
		       + index * dynsym_section->entsize());
	  symtab->write_section_symbol(*p, this->dynsym_xindex_, of, off);
	}
    }

  // Write out the Output_data which are not in an Output_section.
  for (Data_list::const_iterator p = this->special_output_list_.begin();
       p != this->special_output_list_.end();
       ++p)
    (*p)->write(of);
}

// Write out the Output_sections which can only be written after the
// input sections are complete.

void
Layout::write_sections_after_input_sections(Output_file* of)
{
  // Determine the final section offsets, and thus the final output
  // file size.  Note we finalize the .shstrab last, to allow the
  // after_input_section sections to modify their section-names before
  // writing.
  if (this->any_postprocessing_sections_)
    {
      off_t off = this->output_file_size_;
      off = this->set_section_offsets(off, POSTPROCESSING_SECTIONS_PASS);
      
      // Now that we've finalized the names, we can finalize the shstrab.
      off =
	this->set_section_offsets(off,
				  STRTAB_AFTER_POSTPROCESSING_SECTIONS_PASS);

      if (off > this->output_file_size_)
	{
	  of->resize(off);
	  this->output_file_size_ = off;
	}
    }

  for (Section_list::const_iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    {
      if ((*p)->after_input_sections())
	(*p)->write(of);
    }

  this->section_headers_->write(of);
}

// If the build ID requires computing a checksum, do so here, and
// write it out.  We compute a checksum over the entire file because
// that is simplest.

void
Layout::write_build_id(Output_file* of) const
{
  if (this->build_id_note_ == NULL)
    return;

  const unsigned char* iv = of->get_input_view(0, this->output_file_size_);

  unsigned char* ov = of->get_output_view(this->build_id_note_->offset(),
					  this->build_id_note_->data_size());

  const char* style = parameters->options().build_id();
  if (strcmp(style, "sha1") == 0)
    {
      sha1_ctx ctx;
      sha1_init_ctx(&ctx);
      sha1_process_bytes(iv, this->output_file_size_, &ctx);
      sha1_finish_ctx(&ctx, ov);
    }
  else if (strcmp(style, "md5") == 0)
    {
      md5_ctx ctx;
      md5_init_ctx(&ctx);
      md5_process_bytes(iv, this->output_file_size_, &ctx);
      md5_finish_ctx(&ctx, ov);
    }
  else
    gold_unreachable();

  of->write_output_view(this->build_id_note_->offset(),
			this->build_id_note_->data_size(),
			ov);

  of->free_input_view(0, this->output_file_size_, iv);
}

// Write out a binary file.  This is called after the link is
// complete.  IN is the temporary output file we used to generate the
// ELF code.  We simply walk through the segments, read them from
// their file offset in IN, and write them to their load address in
// the output file.  FIXME: with a bit more work, we could support
// S-records and/or Intel hex format here.

void
Layout::write_binary(Output_file* in) const
{
  gold_assert(this->options_.oformat_enum()
	      == General_options::OBJECT_FORMAT_BINARY);

  // Get the size of the binary file.
  uint64_t max_load_address = 0;
  for (Segment_list::const_iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    {
      if ((*p)->type() == elfcpp::PT_LOAD && (*p)->filesz() > 0)
	{
	  uint64_t max_paddr = (*p)->paddr() + (*p)->filesz();
	  if (max_paddr > max_load_address)
	    max_load_address = max_paddr;
	}
    }

  Output_file out(parameters->options().output_file_name());
  out.open(max_load_address);

  for (Segment_list::const_iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    {
      if ((*p)->type() == elfcpp::PT_LOAD && (*p)->filesz() > 0)
	{
	  const unsigned char* vin = in->get_input_view((*p)->offset(),
							(*p)->filesz());
	  unsigned char* vout = out.get_output_view((*p)->paddr(),
						    (*p)->filesz());
	  memcpy(vout, vin, (*p)->filesz());
	  out.write_output_view((*p)->paddr(), (*p)->filesz(), vout);
	  in->free_input_view((*p)->offset(), (*p)->filesz(), vin);
	}
    }

  out.close();
}

// Print the output sections to the map file.

void
Layout::print_to_mapfile(Mapfile* mapfile) const
{
  for (Segment_list::const_iterator p = this->segment_list_.begin();
       p != this->segment_list_.end();
       ++p)
    (*p)->print_sections_to_mapfile(mapfile);
}

// Print statistical information to stderr.  This is used for --stats.

void
Layout::print_stats() const
{
  this->namepool_.print_stats("section name pool");
  this->sympool_.print_stats("output symbol name pool");
  this->dynpool_.print_stats("dynamic name pool");

  for (Section_list::const_iterator p = this->section_list_.begin();
       p != this->section_list_.end();
       ++p)
    (*p)->print_merge_stats();
}

// Write_sections_task methods.

// We can always run this task.

Task_token*
Write_sections_task::is_runnable()
{
  return NULL;
}

// We need to unlock both OUTPUT_SECTIONS_BLOCKER and FINAL_BLOCKER
// when finished.

void
Write_sections_task::locks(Task_locker* tl)
{
  tl->add(this, this->output_sections_blocker_);
  tl->add(this, this->final_blocker_);
}

// Run the task--write out the data.

void
Write_sections_task::run(Workqueue*)
{
  this->layout_->write_output_sections(this->of_);
}

// Write_data_task methods.

// We can always run this task.

Task_token*
Write_data_task::is_runnable()
{
  return NULL;
}

// We need to unlock FINAL_BLOCKER when finished.

void
Write_data_task::locks(Task_locker* tl)
{
  tl->add(this, this->final_blocker_);
}

// Run the task--write out the data.

void
Write_data_task::run(Workqueue*)
{
  this->layout_->write_data(this->symtab_, this->of_);
}

// Write_symbols_task methods.

// We can always run this task.

Task_token*
Write_symbols_task::is_runnable()
{
  return NULL;
}

// We need to unlock FINAL_BLOCKER when finished.

void
Write_symbols_task::locks(Task_locker* tl)
{
  tl->add(this, this->final_blocker_);
}

// Run the task--write out the symbols.

void
Write_symbols_task::run(Workqueue*)
{
  this->symtab_->write_globals(this->input_objects_, this->sympool_,
			       this->dynpool_, this->layout_->symtab_xindex(),
			       this->layout_->dynsym_xindex(), this->of_);
}

// Write_after_input_sections_task methods.

// We can only run this task after the input sections have completed.

Task_token*
Write_after_input_sections_task::is_runnable()
{
  if (this->input_sections_blocker_->is_blocked())
    return this->input_sections_blocker_;
  return NULL;
}

// We need to unlock FINAL_BLOCKER when finished.

void
Write_after_input_sections_task::locks(Task_locker* tl)
{
  tl->add(this, this->final_blocker_);
}

// Run the task.

void
Write_after_input_sections_task::run(Workqueue*)
{
  this->layout_->write_sections_after_input_sections(this->of_);
}

// Close_task_runner methods.

// Run the task--close the file.

void
Close_task_runner::run(Workqueue*, const Task*)
{
  // If we need to compute a checksum for the BUILD if, we do so here.
  this->layout_->write_build_id(this->of_);

  // If we've been asked to create a binary file, we do so here.
  if (this->options_->oformat_enum() != General_options::OBJECT_FORMAT_ELF)
    this->layout_->write_binary(this->of_);

  this->of_->close();
}

// Instantiate the templates we need.  We could use the configure
// script to restrict this to only the ones for implemented targets.

#ifdef HAVE_TARGET_32_LITTLE
template
Output_section*
Layout::layout<32, false>(Sized_relobj<32, false>* object, unsigned int shndx,
			  const char* name,
			  const elfcpp::Shdr<32, false>& shdr,
			  unsigned int, unsigned int, off_t*);
#endif

#ifdef HAVE_TARGET_32_BIG
template
Output_section*
Layout::layout<32, true>(Sized_relobj<32, true>* object, unsigned int shndx,
			 const char* name,
			 const elfcpp::Shdr<32, true>& shdr,
			 unsigned int, unsigned int, off_t*);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
Output_section*
Layout::layout<64, false>(Sized_relobj<64, false>* object, unsigned int shndx,
			  const char* name,
			  const elfcpp::Shdr<64, false>& shdr,
			  unsigned int, unsigned int, off_t*);
#endif

#ifdef HAVE_TARGET_64_BIG
template
Output_section*
Layout::layout<64, true>(Sized_relobj<64, true>* object, unsigned int shndx,
			 const char* name,
			 const elfcpp::Shdr<64, true>& shdr,
			 unsigned int, unsigned int, off_t*);
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
Output_section*
Layout::layout_reloc<32, false>(Sized_relobj<32, false>* object,
				unsigned int reloc_shndx,
				const elfcpp::Shdr<32, false>& shdr,
				Output_section* data_section,
				Relocatable_relocs* rr);
#endif

#ifdef HAVE_TARGET_32_BIG
template
Output_section*
Layout::layout_reloc<32, true>(Sized_relobj<32, true>* object,
			       unsigned int reloc_shndx,
			       const elfcpp::Shdr<32, true>& shdr,
			       Output_section* data_section,
			       Relocatable_relocs* rr);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
Output_section*
Layout::layout_reloc<64, false>(Sized_relobj<64, false>* object,
				unsigned int reloc_shndx,
				const elfcpp::Shdr<64, false>& shdr,
				Output_section* data_section,
				Relocatable_relocs* rr);
#endif

#ifdef HAVE_TARGET_64_BIG
template
Output_section*
Layout::layout_reloc<64, true>(Sized_relobj<64, true>* object,
			       unsigned int reloc_shndx,
			       const elfcpp::Shdr<64, true>& shdr,
			       Output_section* data_section,
			       Relocatable_relocs* rr);
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
void
Layout::layout_group<32, false>(Symbol_table* symtab,
				Sized_relobj<32, false>* object,
				unsigned int,
				const char* group_section_name,
				const char* signature,
				const elfcpp::Shdr<32, false>& shdr,
				elfcpp::Elf_Word flags,
				std::vector<unsigned int>* shndxes);
#endif

#ifdef HAVE_TARGET_32_BIG
template
void
Layout::layout_group<32, true>(Symbol_table* symtab,
			       Sized_relobj<32, true>* object,
			       unsigned int,
			       const char* group_section_name,
			       const char* signature,
			       const elfcpp::Shdr<32, true>& shdr,
			       elfcpp::Elf_Word flags,
			       std::vector<unsigned int>* shndxes);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
void
Layout::layout_group<64, false>(Symbol_table* symtab,
				Sized_relobj<64, false>* object,
				unsigned int,
				const char* group_section_name,
				const char* signature,
				const elfcpp::Shdr<64, false>& shdr,
				elfcpp::Elf_Word flags,
				std::vector<unsigned int>* shndxes);
#endif

#ifdef HAVE_TARGET_64_BIG
template
void
Layout::layout_group<64, true>(Symbol_table* symtab,
			       Sized_relobj<64, true>* object,
			       unsigned int,
			       const char* group_section_name,
			       const char* signature,
			       const elfcpp::Shdr<64, true>& shdr,
			       elfcpp::Elf_Word flags,
			       std::vector<unsigned int>* shndxes);
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
Output_section*
Layout::layout_eh_frame<32, false>(Sized_relobj<32, false>* object,
				   const unsigned char* symbols,
				   off_t symbols_size,
				   const unsigned char* symbol_names,
				   off_t symbol_names_size,
				   unsigned int shndx,
				   const elfcpp::Shdr<32, false>& shdr,
				   unsigned int reloc_shndx,
				   unsigned int reloc_type,
				   off_t* off);
#endif

#ifdef HAVE_TARGET_32_BIG
template
Output_section*
Layout::layout_eh_frame<32, true>(Sized_relobj<32, true>* object,
				   const unsigned char* symbols,
				   off_t symbols_size,
				  const unsigned char* symbol_names,
				  off_t symbol_names_size,
				  unsigned int shndx,
				  const elfcpp::Shdr<32, true>& shdr,
				  unsigned int reloc_shndx,
				  unsigned int reloc_type,
				  off_t* off);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
Output_section*
Layout::layout_eh_frame<64, false>(Sized_relobj<64, false>* object,
				   const unsigned char* symbols,
				   off_t symbols_size,
				   const unsigned char* symbol_names,
				   off_t symbol_names_size,
				   unsigned int shndx,
				   const elfcpp::Shdr<64, false>& shdr,
				   unsigned int reloc_shndx,
				   unsigned int reloc_type,
				   off_t* off);
#endif

#ifdef HAVE_TARGET_64_BIG
template
Output_section*
Layout::layout_eh_frame<64, true>(Sized_relobj<64, true>* object,
				   const unsigned char* symbols,
				   off_t symbols_size,
				  const unsigned char* symbol_names,
				  off_t symbol_names_size,
				  unsigned int shndx,
				  const elfcpp::Shdr<64, true>& shdr,
				  unsigned int reloc_shndx,
				  unsigned int reloc_type,
				  off_t* off);
#endif

} // End namespace gold.
