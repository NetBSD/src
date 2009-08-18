// reloc.cc -- relocate input files for gold.

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
#include "symtab.h"
#include "output.h"
#include "merge.h"
#include "object.h"
#include "target-reloc.h"
#include "reloc.h"

namespace gold
{

// Read_relocs methods.

// These tasks just read the relocation information from the file.
// After reading it, the start another task to process the
// information.  These tasks requires access to the file.

Task_token*
Read_relocs::is_runnable()
{
  return this->object_->is_locked() ? this->object_->token() : NULL;
}

// Lock the file.

void
Read_relocs::locks(Task_locker* tl)
{
  tl->add(this, this->object_->token());
}

// Read the relocations and then start a Scan_relocs_task.

void
Read_relocs::run(Workqueue* workqueue)
{
  Read_relocs_data *rd = new Read_relocs_data;
  this->object_->read_relocs(rd);
  this->object_->release();

  workqueue->queue_next(new Scan_relocs(this->options_, this->symtab_,
					this->layout_, this->object_, rd,
					this->symtab_lock_, this->blocker_));
}

// Return a debugging name for the task.

std::string
Read_relocs::get_name() const
{
  return "Read_relocs " + this->object_->name();
}

// Scan_relocs methods.

// These tasks scan the relocations read by Read_relocs and mark up
// the symbol table to indicate which relocations are required.  We
// use a lock on the symbol table to keep them from interfering with
// each other.

Task_token*
Scan_relocs::is_runnable()
{
  if (!this->symtab_lock_->is_writable())
    return this->symtab_lock_;
  if (this->object_->is_locked())
    return this->object_->token();
  return NULL;
}

// Return the locks we hold: one on the file, one on the symbol table
// and one blocker.

void
Scan_relocs::locks(Task_locker* tl)
{
  tl->add(this, this->object_->token());
  tl->add(this, this->symtab_lock_);
  tl->add(this, this->blocker_);
}

// Scan the relocs.

void
Scan_relocs::run(Workqueue*)
{
  this->object_->scan_relocs(this->options_, this->symtab_, this->layout_,
			     this->rd_);
  this->object_->release();
  delete this->rd_;
  this->rd_ = NULL;
}

// Return a debugging name for the task.

std::string
Scan_relocs::get_name() const
{
  return "Scan_relocs " + this->object_->name();
}

// Relocate_task methods.

// We may have to wait for the output sections to be written.

Task_token*
Relocate_task::is_runnable()
{
  if (this->object_->relocs_must_follow_section_writes()
      && this->output_sections_blocker_->is_blocked())
    return this->output_sections_blocker_;

  if (this->object_->is_locked())
    return this->object_->token();

  return NULL;
}

// We want to lock the file while we run.  We want to unblock
// INPUT_SECTIONS_BLOCKER and FINAL_BLOCKER when we are done.
// INPUT_SECTIONS_BLOCKER may be NULL.

void
Relocate_task::locks(Task_locker* tl)
{
  if (this->input_sections_blocker_ != NULL)
    tl->add(this, this->input_sections_blocker_);
  tl->add(this, this->final_blocker_);
  tl->add(this, this->object_->token());
}

// Run the task.

void
Relocate_task::run(Workqueue*)
{
  this->object_->relocate(this->options_, this->symtab_, this->layout_,
			  this->of_);

  // This is normally the last thing we will do with an object, so
  // uncache all views.
  this->object_->clear_view_cache_marks();

  this->object_->release();
}

// Return a debugging name for the task.

std::string
Relocate_task::get_name() const
{
  return "Relocate_task " + this->object_->name();
}

// Read the relocs and local symbols from the object file and store
// the information in RD.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::do_read_relocs(Read_relocs_data* rd)
{
  rd->relocs.clear();

  unsigned int shnum = this->shnum();
  if (shnum == 0)
    return;

  rd->relocs.reserve(shnum / 2);

  const Output_sections& out_sections(this->output_sections());
  const std::vector<Address>& out_offsets(this->section_offsets_);

  const unsigned char *pshdrs = this->get_view(this->elf_file_.shoff(),
					       shnum * This::shdr_size,
					       true, true);
  // Skip the first, dummy, section.
  const unsigned char *ps = pshdrs + This::shdr_size;
  for (unsigned int i = 1; i < shnum; ++i, ps += This::shdr_size)
    {
      typename This::Shdr shdr(ps);

      unsigned int sh_type = shdr.get_sh_type();
      if (sh_type != elfcpp::SHT_REL && sh_type != elfcpp::SHT_RELA)
	continue;

      unsigned int shndx = this->adjust_shndx(shdr.get_sh_info());
      if (shndx >= shnum)
	{
	  this->error(_("relocation section %u has bad info %u"),
		      i, shndx);
	  continue;
	}

      Output_section* os = out_sections[shndx];
      if (os == NULL)
	continue;

      // We are scanning relocations in order to fill out the GOT and
      // PLT sections.  Relocations for sections which are not
      // allocated (typically debugging sections) should not add new
      // GOT and PLT entries.  So we skip them unless this is a
      // relocatable link or we need to emit relocations.
      typename This::Shdr secshdr(pshdrs + shndx * This::shdr_size);
      bool is_section_allocated = ((secshdr.get_sh_flags() & elfcpp::SHF_ALLOC)
				   != 0);
      if (!is_section_allocated
	  && !parameters->options().relocatable()
	  && !parameters->options().emit_relocs())
	continue;

      if (this->adjust_shndx(shdr.get_sh_link()) != this->symtab_shndx_)
	{
	  this->error(_("relocation section %u uses unexpected "
			"symbol table %u"),
		      i, this->adjust_shndx(shdr.get_sh_link()));
	  continue;
	}

      off_t sh_size = shdr.get_sh_size();

      unsigned int reloc_size;
      if (sh_type == elfcpp::SHT_REL)
	reloc_size = elfcpp::Elf_sizes<size>::rel_size;
      else
	reloc_size = elfcpp::Elf_sizes<size>::rela_size;
      if (reloc_size != shdr.get_sh_entsize())
	{
	  this->error(_("unexpected entsize for reloc section %u: %lu != %u"),
		      i, static_cast<unsigned long>(shdr.get_sh_entsize()),
		      reloc_size);
	  continue;
	}

      size_t reloc_count = sh_size / reloc_size;
      if (static_cast<off_t>(reloc_count * reloc_size) != sh_size)
	{
	  this->error(_("reloc section %u size %lu uneven"),
		      i, static_cast<unsigned long>(sh_size));
	  continue;
	}

      rd->relocs.push_back(Section_relocs());
      Section_relocs& sr(rd->relocs.back());
      sr.reloc_shndx = i;
      sr.data_shndx = shndx;
      sr.contents = this->get_lasting_view(shdr.get_sh_offset(), sh_size,
					   true, true);
      sr.sh_type = sh_type;
      sr.reloc_count = reloc_count;
      sr.output_section = os;
      sr.needs_special_offset_handling = out_offsets[shndx] == -1U;
      sr.is_data_section_allocated = is_section_allocated;
    }

  // Read the local symbols.
  gold_assert(this->symtab_shndx_ != -1U);
  if (this->symtab_shndx_ == 0 || this->local_symbol_count_ == 0)
    rd->local_symbols = NULL;
  else
    {
      typename This::Shdr symtabshdr(pshdrs
				     + this->symtab_shndx_ * This::shdr_size);
      gold_assert(symtabshdr.get_sh_type() == elfcpp::SHT_SYMTAB);
      const int sym_size = This::sym_size;
      const unsigned int loccount = this->local_symbol_count_;
      gold_assert(loccount == symtabshdr.get_sh_info());
      off_t locsize = loccount * sym_size;
      rd->local_symbols = this->get_lasting_view(symtabshdr.get_sh_offset(),
						 locsize, true, true);
    }
}

// Scan the relocs and adjust the symbol table.  This looks for
// relocations which require GOT/PLT/COPY relocations.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::do_scan_relocs(const General_options& options,
					       Symbol_table* symtab,
					       Layout* layout,
					       Read_relocs_data* rd)
{
  Sized_target<size, big_endian>* target = this->sized_target();

  const unsigned char* local_symbols;
  if (rd->local_symbols == NULL)
    local_symbols = NULL;
  else
    local_symbols = rd->local_symbols->data();

  for (Read_relocs_data::Relocs_list::iterator p = rd->relocs.begin();
       p != rd->relocs.end();
       ++p)
    {
      if (!parameters->options().relocatable())
	{
	  // As noted above, when not generating an object file, we
	  // only scan allocated sections.  We may see a non-allocated
	  // section here if we are emitting relocs.
	  if (p->is_data_section_allocated)
	    target->scan_relocs(options, symtab, layout, this, p->data_shndx,
				p->sh_type, p->contents->data(),
				p->reloc_count, p->output_section,
				p->needs_special_offset_handling,
				this->local_symbol_count_,
				local_symbols);
	  if (parameters->options().emit_relocs())
	    this->emit_relocs_scan(options, symtab, layout, local_symbols, p);
	}
      else
	{
	  Relocatable_relocs* rr = this->relocatable_relocs(p->reloc_shndx);
	  gold_assert(rr != NULL);
	  rr->set_reloc_count(p->reloc_count);
	  target->scan_relocatable_relocs(options, symtab, layout, this,
					  p->data_shndx, p->sh_type,
					  p->contents->data(),
					  p->reloc_count,
					  p->output_section,
					  p->needs_special_offset_handling,
					  this->local_symbol_count_,
					  local_symbols,
					  rr);
	}

      delete p->contents;
      p->contents = NULL;
    }

  if (rd->local_symbols != NULL)
    {
      delete rd->local_symbols;
      rd->local_symbols = NULL;
    }
}

// This is a strategy class we use when scanning for --emit-relocs.

template<int sh_type>
class Emit_relocs_strategy
{
 public:
  // A local non-section symbol.
  inline Relocatable_relocs::Reloc_strategy
  local_non_section_strategy(unsigned int, Relobj*)
  { return Relocatable_relocs::RELOC_COPY; }

  // A local section symbol.
  inline Relocatable_relocs::Reloc_strategy
  local_section_strategy(unsigned int, Relobj*)
  {
    if (sh_type == elfcpp::SHT_RELA)
      return Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_RELA;
    else
      {
	// The addend is stored in the section contents.  Since this
	// is not a relocatable link, we are going to apply the
	// relocation contents to the section as usual.  This means
	// that we have no way to record the original addend.  If the
	// original addend is not zero, there is basically no way for
	// the user to handle this correctly.  Caveat emptor.
	return Relocatable_relocs::RELOC_ADJUST_FOR_SECTION_0;
      }
  }

  // A global symbol.
  inline Relocatable_relocs::Reloc_strategy
  global_strategy(unsigned int, Relobj*, unsigned int)
  { return Relocatable_relocs::RELOC_COPY; }
};

// Scan the input relocations for --emit-relocs.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::emit_relocs_scan(
    const General_options& options,
    Symbol_table* symtab,
    Layout* layout,
    const unsigned char* plocal_syms,
    const Read_relocs_data::Relocs_list::iterator& p)
{
  Relocatable_relocs* rr = this->relocatable_relocs(p->reloc_shndx);
  gold_assert(rr != NULL);
  rr->set_reloc_count(p->reloc_count);

  if (p->sh_type == elfcpp::SHT_REL)
    this->emit_relocs_scan_reltype<elfcpp::SHT_REL>(options, symtab, layout,
						    plocal_syms, p, rr);
  else
    {
      gold_assert(p->sh_type == elfcpp::SHT_RELA);
      this->emit_relocs_scan_reltype<elfcpp::SHT_RELA>(options, symtab,
						       layout, plocal_syms, p,
						       rr);
    }
}

// Scan the input relocation for --emit-relocs, templatized on the
// type of the relocation section.

template<int size, bool big_endian>
template<int sh_type>
void
Sized_relobj<size, big_endian>::emit_relocs_scan_reltype(
    const General_options& options,
    Symbol_table* symtab,
    Layout* layout,
    const unsigned char* plocal_syms,
    const Read_relocs_data::Relocs_list::iterator& p,
    Relocatable_relocs* rr)
{
  scan_relocatable_relocs<size, big_endian, sh_type,
			  Emit_relocs_strategy<sh_type> >(
    options,
    symtab,
    layout,
    this,
    p->data_shndx,
    p->contents->data(),
    p->reloc_count,
    p->output_section,
    p->needs_special_offset_handling,
    this->local_symbol_count_,
    plocal_syms,
    rr);
}

// Relocate the input sections and write out the local symbols.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::do_relocate(const General_options& options,
					    const Symbol_table* symtab,
					    const Layout* layout,
					    Output_file* of)
{
  unsigned int shnum = this->shnum();

  // Read the section headers.
  const unsigned char* pshdrs = this->get_view(this->elf_file_.shoff(),
					       shnum * This::shdr_size,
					       true, true);

  Views views;
  views.resize(shnum);

  // Make two passes over the sections.  The first one copies the
  // section data to the output file.  The second one applies
  // relocations.

  this->write_sections(pshdrs, of, &views);

  // To speed up relocations, we set up hash tables for fast lookup of
  // input offsets to output addresses.
  this->initialize_input_to_output_maps();

  // Apply relocations.

  this->relocate_sections(options, symtab, layout, pshdrs, &views);

  // After we've done the relocations, we release the hash tables,
  // since we no longer need them.
  this->free_input_to_output_maps();

  // Write out the accumulated views.
  for (unsigned int i = 1; i < shnum; ++i)
    {
      if (views[i].view != NULL)
	{
	  if (!views[i].is_postprocessing_view)
	    {
	      if (views[i].is_input_output_view)
		of->write_input_output_view(views[i].offset,
					    views[i].view_size,
					    views[i].view);
	      else
		of->write_output_view(views[i].offset, views[i].view_size,
				      views[i].view);
	    }
	}
    }

  // Write out the local symbols.
  this->write_local_symbols(of, layout->sympool(), layout->dynpool(),
			    layout->symtab_xindex(), layout->dynsym_xindex());

  // We should no longer need the local symbol values.
  this->clear_local_symbols();
}

// Sort a Read_multiple vector by file offset.
struct Read_multiple_compare
{
  inline bool
  operator()(const File_read::Read_multiple_entry& rme1,
	     const File_read::Read_multiple_entry& rme2) const
  { return rme1.file_offset < rme2.file_offset; }
};

// Write section data to the output file.  PSHDRS points to the
// section headers.  Record the views in *PVIEWS for use when
// relocating.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::write_sections(const unsigned char* pshdrs,
					       Output_file* of,
					       Views* pviews)
{
  unsigned int shnum = this->shnum();
  const Output_sections& out_sections(this->output_sections());
  const std::vector<Address>& out_offsets(this->section_offsets_);

  File_read::Read_multiple rm;
  bool is_sorted = true;

  const unsigned char* p = pshdrs + This::shdr_size;
  for (unsigned int i = 1; i < shnum; ++i, p += This::shdr_size)
    {
      View_size* pvs = &(*pviews)[i];

      pvs->view = NULL;

      const Output_section* os = out_sections[i];
      if (os == NULL)
	continue;
      Address output_offset = out_offsets[i];

      typename This::Shdr shdr(p);

      if (shdr.get_sh_type() == elfcpp::SHT_NOBITS)
	continue;

      if ((parameters->options().relocatable()
	   || parameters->options().emit_relocs())
	  && (shdr.get_sh_type() == elfcpp::SHT_REL
	      || shdr.get_sh_type() == elfcpp::SHT_RELA)
	  && (shdr.get_sh_flags() & elfcpp::SHF_ALLOC) == 0)
	{
	  // This is a reloc section in a relocatable link or when
	  // emitting relocs.  We don't need to read the input file.
	  // The size and file offset are stored in the
	  // Relocatable_relocs structure.
	  Relocatable_relocs* rr = this->relocatable_relocs(i);
	  gold_assert(rr != NULL);
	  Output_data* posd = rr->output_data();
	  gold_assert(posd != NULL);

	  pvs->offset = posd->offset();
	  pvs->view_size = posd->data_size();
	  pvs->view = of->get_output_view(pvs->offset, pvs->view_size);
	  pvs->address = posd->address();
	  pvs->is_input_output_view = false;
	  pvs->is_postprocessing_view = false;

	  continue;
	}

      // In the normal case, this input section is simply mapped to
      // the output section at offset OUTPUT_OFFSET.

      // However, if OUTPUT_OFFSET == -1U, then input data is handled
      // specially--e.g., a .eh_frame section.  The relocation
      // routines need to check for each reloc where it should be
      // applied.  For this case, we need an input/output view for the
      // entire contents of the section in the output file.  We don't
      // want to copy the contents of the input section to the output
      // section; the output section contents were already written,
      // and we waited for them in Relocate_task::is_runnable because
      // relocs_must_follow_section_writes is set for the object.

      // Regardless of which of the above cases is true, we have to
      // check requires_postprocessing of the output section.  If that
      // is false, then we work with views of the output file
      // directly.  If it is true, then we work with a separate
      // buffer, and the output section is responsible for writing the
      // final data to the output file.

      off_t output_section_offset;
      Address output_section_size;
      if (!os->requires_postprocessing())
	{
	  output_section_offset = os->offset();
	  output_section_size = convert_types<Address, off_t>(os->data_size());
	}
      else
	{
	  output_section_offset = 0;
	  output_section_size =
              convert_types<Address, off_t>(os->postprocessing_buffer_size());
	}

      off_t view_start;
      section_size_type view_size;
      if (output_offset != -1U)
	{
	  view_start = output_section_offset + output_offset;
	  view_size = convert_to_section_size_type(shdr.get_sh_size());
	}
      else
	{
	  view_start = output_section_offset;
	  view_size = convert_to_section_size_type(output_section_size);
	}

      if (view_size == 0)
	continue;

      gold_assert(output_offset == -1U
		  || output_offset + view_size <= output_section_size);

      unsigned char* view;
      if (os->requires_postprocessing())
	{
	  unsigned char* buffer = os->postprocessing_buffer();
	  view = buffer + view_start;
	  if (output_offset != -1U)
	    {
	      off_t sh_offset = shdr.get_sh_offset();
	      if (!rm.empty() && rm.back().file_offset > sh_offset)
		is_sorted = false;
	      rm.push_back(File_read::Read_multiple_entry(sh_offset,
							  view_size, view));
	    }
	}
      else
	{
	  if (output_offset == -1U)
	    view = of->get_input_output_view(view_start, view_size);
	  else
	    {
	      view = of->get_output_view(view_start, view_size);
	      off_t sh_offset = shdr.get_sh_offset();
	      if (!rm.empty() && rm.back().file_offset > sh_offset)
		is_sorted = false;
	      rm.push_back(File_read::Read_multiple_entry(sh_offset,
							  view_size, view));
	    }
	}

      pvs->view = view;
      pvs->address = os->address();
      if (output_offset != -1U)
	pvs->address += output_offset;
      pvs->offset = view_start;
      pvs->view_size = view_size;
      pvs->is_input_output_view = output_offset == -1U;
      pvs->is_postprocessing_view = os->requires_postprocessing();
    }

  // Actually read the data.
  if (!rm.empty())
    {
      if (!is_sorted)
	std::sort(rm.begin(), rm.end(), Read_multiple_compare());
      this->read_multiple(rm);
    }
}

// Relocate section data.  VIEWS points to the section data as views
// in the output file.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::relocate_sections(
    const General_options& options,
    const Symbol_table* symtab,
    const Layout* layout,
    const unsigned char* pshdrs,
    Views* pviews)
{
  unsigned int shnum = this->shnum();
  Sized_target<size, big_endian>* target = this->sized_target();

  const Output_sections& out_sections(this->output_sections());
  const std::vector<Address>& out_offsets(this->section_offsets_);

  Relocate_info<size, big_endian> relinfo;
  relinfo.options = &options;
  relinfo.symtab = symtab;
  relinfo.layout = layout;
  relinfo.object = this;

  const unsigned char* p = pshdrs + This::shdr_size;
  for (unsigned int i = 1; i < shnum; ++i, p += This::shdr_size)
    {
      typename This::Shdr shdr(p);

      unsigned int sh_type = shdr.get_sh_type();
      if (sh_type != elfcpp::SHT_REL && sh_type != elfcpp::SHT_RELA)
	continue;

      unsigned int index = this->adjust_shndx(shdr.get_sh_info());
      if (index >= this->shnum())
	{
	  this->error(_("relocation section %u has bad info %u"),
		      i, index);
	  continue;
	}

      Output_section* os = out_sections[index];
      if (os == NULL)
	{
	  // This relocation section is against a section which we
	  // discarded.
	  continue;
	}
      Address output_offset = out_offsets[index];

      gold_assert((*pviews)[index].view != NULL);
      if (parameters->options().relocatable())
	gold_assert((*pviews)[i].view != NULL);

      if (this->adjust_shndx(shdr.get_sh_link()) != this->symtab_shndx_)
	{
	  gold_error(_("relocation section %u uses unexpected "
		       "symbol table %u"),
		     i, this->adjust_shndx(shdr.get_sh_link()));
	  continue;
	}

      off_t sh_size = shdr.get_sh_size();
      const unsigned char* prelocs = this->get_view(shdr.get_sh_offset(),
						    sh_size, true, false);

      unsigned int reloc_size;
      if (sh_type == elfcpp::SHT_REL)
	reloc_size = elfcpp::Elf_sizes<size>::rel_size;
      else
	reloc_size = elfcpp::Elf_sizes<size>::rela_size;

      if (reloc_size != shdr.get_sh_entsize())
	{
	  gold_error(_("unexpected entsize for reloc section %u: %lu != %u"),
		     i, static_cast<unsigned long>(shdr.get_sh_entsize()),
		     reloc_size);
	  continue;
	}

      size_t reloc_count = sh_size / reloc_size;
      if (static_cast<off_t>(reloc_count * reloc_size) != sh_size)
	{
	  gold_error(_("reloc section %u size %lu uneven"),
		     i, static_cast<unsigned long>(sh_size));
	  continue;
	}

      gold_assert(output_offset != -1U
		  || this->relocs_must_follow_section_writes());

      relinfo.reloc_shndx = i;
      relinfo.data_shndx = index;
      if (!parameters->options().relocatable())
	{
	  target->relocate_section(&relinfo,
				   sh_type,
				   prelocs,
				   reloc_count,
				   os,
				   output_offset == -1U,
				   (*pviews)[index].view,
				   (*pviews)[index].address,
				   (*pviews)[index].view_size);
	  if (parameters->options().emit_relocs())
	    this->emit_relocs(&relinfo, i, sh_type, prelocs, reloc_count,
			      os, output_offset,
			      (*pviews)[index].view,
			      (*pviews)[index].address,
			      (*pviews)[index].view_size,
			      (*pviews)[i].view,
			      (*pviews)[i].view_size);
	}
      else
	{
	  Relocatable_relocs* rr = this->relocatable_relocs(i);
	  target->relocate_for_relocatable(&relinfo,
					   sh_type,
					   prelocs,
					   reloc_count,
					   os,
					   output_offset,
					   rr,
					   (*pviews)[index].view,
					   (*pviews)[index].address,
					   (*pviews)[index].view_size,
					   (*pviews)[i].view,
					   (*pviews)[i].view_size);
	}
    }
}

// Emit the relocs for --emit-relocs.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::emit_relocs(
    const Relocate_info<size, big_endian>* relinfo,
    unsigned int i,
    unsigned int sh_type,
    const unsigned char* prelocs,
    size_t reloc_count,
    Output_section* output_section,
    typename elfcpp::Elf_types<size>::Elf_Addr offset_in_output_section,
    unsigned char* view,
    typename elfcpp::Elf_types<size>::Elf_Addr address,
    section_size_type view_size,
    unsigned char* reloc_view,
    section_size_type reloc_view_size)
{
  if (sh_type == elfcpp::SHT_REL)
    this->emit_relocs_reltype<elfcpp::SHT_REL>(relinfo, i, prelocs,
					       reloc_count, output_section,
					       offset_in_output_section,
					       view, address, view_size,
					       reloc_view, reloc_view_size);
  else
    {
      gold_assert(sh_type == elfcpp::SHT_RELA);
      this->emit_relocs_reltype<elfcpp::SHT_RELA>(relinfo, i, prelocs,
						  reloc_count, output_section,
						  offset_in_output_section,
						  view, address, view_size,
						  reloc_view, reloc_view_size);
    }
}

// Emit the relocs for --emit-relocs, templatized on the type of the
// relocation section.

template<int size, bool big_endian>
template<int sh_type>
void
Sized_relobj<size, big_endian>::emit_relocs_reltype(
    const Relocate_info<size, big_endian>* relinfo,
    unsigned int i,
    const unsigned char* prelocs,
    size_t reloc_count,
    Output_section* output_section,
    typename elfcpp::Elf_types<size>::Elf_Addr offset_in_output_section,
    unsigned char* view,
    typename elfcpp::Elf_types<size>::Elf_Addr address,
    section_size_type view_size,
    unsigned char* reloc_view,
    section_size_type reloc_view_size)
{
  const Relocatable_relocs* rr = this->relocatable_relocs(i);
  relocate_for_relocatable<size, big_endian, sh_type>(
    relinfo,
    prelocs,
    reloc_count,
    output_section,
    offset_in_output_section,
    rr,
    view,
    address,
    view_size,
    reloc_view,
    reloc_view_size);
}

// Create merge hash tables for the local symbols.  These are used to
// speed up relocations.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::initialize_input_to_output_maps()
{
  const unsigned int loccount = this->local_symbol_count_;
  for (unsigned int i = 1; i < loccount; ++i)
    {
      Symbol_value<size>& lv(this->local_values_[i]);
      lv.initialize_input_to_output_map(this);
    }
}

// Free merge hash tables for the local symbols.

template<int size, bool big_endian>
void
Sized_relobj<size, big_endian>::free_input_to_output_maps()
{
  const unsigned int loccount = this->local_symbol_count_;
  for (unsigned int i = 1; i < loccount; ++i)
    {
      Symbol_value<size>& lv(this->local_values_[i]);
      lv.free_input_to_output_map();
    }
}

// Class Merged_symbol_value.

template<int size>
void
Merged_symbol_value<size>::initialize_input_to_output_map(
    const Relobj* object,
    unsigned int input_shndx)
{
  Object_merge_map* map = object->merge_map();
  map->initialize_input_to_output_map<size>(input_shndx,
					    this->output_start_address_,
					    &this->output_addresses_);
}

// Get the output value corresponding to an input offset if we
// couldn't find it in the hash table.

template<int size>
typename elfcpp::Elf_types<size>::Elf_Addr
Merged_symbol_value<size>::value_from_output_section(
    const Relobj* object,
    unsigned int input_shndx,
    typename elfcpp::Elf_types<size>::Elf_Addr input_offset) const
{
  section_offset_type output_offset;
  bool found = object->merge_map()->get_output_offset(NULL, input_shndx,
						      input_offset,
						      &output_offset);

  // If this assertion fails, it means that some relocation was
  // against a portion of an input merge section which we didn't map
  // to the output file and we didn't explicitly discard.  We should
  // always map all portions of input merge sections.
  gold_assert(found);

  if (output_offset == -1)
    return 0;
  else
    return this->output_start_address_ + output_offset;
}

// Track_relocs methods.

// Initialize the class to track the relocs.  This gets the object,
// the reloc section index, and the type of the relocs.  This returns
// false if something goes wrong.

template<int size, bool big_endian>
bool
Track_relocs<size, big_endian>::initialize(
    Object* object,
    unsigned int reloc_shndx,
    unsigned int reloc_type)
{
  // If RELOC_SHNDX is -1U, it means there is more than one reloc
  // section for the .eh_frame section.  We can't handle that case.
  if (reloc_shndx == -1U)
    return false;

  // If RELOC_SHNDX is 0, there is no reloc section.
  if (reloc_shndx == 0)
    return true;

  // Get the contents of the reloc section.
  this->prelocs_ = object->section_contents(reloc_shndx, &this->len_, false);

  if (reloc_type == elfcpp::SHT_REL)
    this->reloc_size_ = elfcpp::Elf_sizes<size>::rel_size;
  else if (reloc_type == elfcpp::SHT_RELA)
    this->reloc_size_ = elfcpp::Elf_sizes<size>::rela_size;
  else
    gold_unreachable();

  if (this->len_ % this->reloc_size_ != 0)
    {
      object->error(_("reloc section size %zu is not a multiple of "
		      "reloc size %d\n"),
		    static_cast<size_t>(this->len_),
		    this->reloc_size_);
      return false;
    }

  return true;
}

// Return the offset of the next reloc, or -1 if there isn't one.

template<int size, bool big_endian>
off_t
Track_relocs<size, big_endian>::next_offset() const
{
  if (this->pos_ >= this->len_)
    return -1;

  // Rel and Rela start out the same, so we can always use Rel to find
  // the r_offset value.
  elfcpp::Rel<size, big_endian> rel(this->prelocs_ + this->pos_);
  return rel.get_r_offset();
}

// Return the index of the symbol referenced by the next reloc, or -1U
// if there aren't any more relocs.

template<int size, bool big_endian>
unsigned int
Track_relocs<size, big_endian>::next_symndx() const
{
  if (this->pos_ >= this->len_)
    return -1U;

  // Rel and Rela start out the same, so we can use Rel to find the
  // symbol index.
  elfcpp::Rel<size, big_endian> rel(this->prelocs_ + this->pos_);
  return elfcpp::elf_r_sym<size>(rel.get_r_info());
}

// Advance to the next reloc whose r_offset is greater than or equal
// to OFFSET.  Return the number of relocs we skip.

template<int size, bool big_endian>
int
Track_relocs<size, big_endian>::advance(off_t offset)
{
  int ret = 0;
  while (this->pos_ < this->len_)
    {
      // Rel and Rela start out the same, so we can always use Rel to
      // find the r_offset value.
      elfcpp::Rel<size, big_endian> rel(this->prelocs_ + this->pos_);
      if (static_cast<off_t>(rel.get_r_offset()) >= offset)
	break;
      ++ret;
      this->pos_ += this->reloc_size_;
    }
  return ret;
}

// Instantiate the templates we need.

#ifdef HAVE_TARGET_32_LITTLE
template
void
Sized_relobj<32, false>::do_read_relocs(Read_relocs_data* rd);
#endif

#ifdef HAVE_TARGET_32_BIG
template
void
Sized_relobj<32, true>::do_read_relocs(Read_relocs_data* rd);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
void
Sized_relobj<64, false>::do_read_relocs(Read_relocs_data* rd);
#endif

#ifdef HAVE_TARGET_64_BIG
template
void
Sized_relobj<64, true>::do_read_relocs(Read_relocs_data* rd);
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
void
Sized_relobj<32, false>::do_scan_relocs(const General_options& options,
					Symbol_table* symtab,
					Layout* layout,
					Read_relocs_data* rd);
#endif

#ifdef HAVE_TARGET_32_BIG
template
void
Sized_relobj<32, true>::do_scan_relocs(const General_options& options,
				       Symbol_table* symtab,
				       Layout* layout,
				       Read_relocs_data* rd);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
void
Sized_relobj<64, false>::do_scan_relocs(const General_options& options,
					Symbol_table* symtab,
					Layout* layout,
					Read_relocs_data* rd);
#endif

#ifdef HAVE_TARGET_64_BIG
template
void
Sized_relobj<64, true>::do_scan_relocs(const General_options& options,
				       Symbol_table* symtab,
				       Layout* layout,
				       Read_relocs_data* rd);
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
void
Sized_relobj<32, false>::do_relocate(const General_options& options,
				     const Symbol_table* symtab,
				     const Layout* layout,
				     Output_file* of);
#endif

#ifdef HAVE_TARGET_32_BIG
template
void
Sized_relobj<32, true>::do_relocate(const General_options& options,
				    const Symbol_table* symtab,
				    const Layout* layout,
				    Output_file* of);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
void
Sized_relobj<64, false>::do_relocate(const General_options& options,
				     const Symbol_table* symtab,
				     const Layout* layout,
				     Output_file* of);
#endif

#ifdef HAVE_TARGET_64_BIG
template
void
Sized_relobj<64, true>::do_relocate(const General_options& options,
				    const Symbol_table* symtab,
				    const Layout* layout,
				    Output_file* of);
#endif

#if defined(HAVE_TARGET_32_LITTLE) || defined(HAVE_TARGET_32_BIG)
template
class Merged_symbol_value<32>;
#endif

#if defined(HAVE_TARGET_64_LITTLE) || defined(HAVE_TARGET_64_BIG)
template
class Merged_symbol_value<64>;
#endif

#if defined(HAVE_TARGET_32_LITTLE) || defined(HAVE_TARGET_32_BIG)
template
class Symbol_value<32>;
#endif

#if defined(HAVE_TARGET_64_LITTLE) || defined(HAVE_TARGET_64_BIG)
template
class Symbol_value<64>;
#endif

#ifdef HAVE_TARGET_32_LITTLE
template
class Track_relocs<32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Track_relocs<32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Track_relocs<64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Track_relocs<64, true>;
#endif

} // End namespace gold.
