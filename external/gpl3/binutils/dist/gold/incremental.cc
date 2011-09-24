// inremental.cc -- incremental linking support for gold

// Copyright 2009, 2010 Free Software Foundation, Inc.
// Written by Mikolaj Zalewski <mikolajz@google.com>.

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

#include <cstdarg>
#include "libiberty.h"

#include "elfcpp.h"
#include "output.h"
#include "symtab.h"
#include "incremental.h"
#include "archive.h"
#include "output.h"
#include "target-select.h"
#include "target.h"

namespace gold {

// Version information. Will change frequently during the development, later
// we could think about backward (and forward?) compatibility.
const unsigned int INCREMENTAL_LINK_VERSION = 1;

// This class manages the .gnu_incremental_inputs section, which holds
// the header information, a directory of input files, and separate
// entries for each input file.

template<int size, bool big_endian>
class Output_section_incremental_inputs : public Output_section_data
{
 public:
  Output_section_incremental_inputs(const Incremental_inputs* inputs,
				    const Symbol_table* symtab)
    : Output_section_data(size / 8), inputs_(inputs), symtab_(symtab)
  { }

 protected:
  // Set the final data size.
  void
  set_final_data_size();

  // Write the data to the file.
  void
  do_write(Output_file*);

  // Write to a map file.
  void
  do_print_to_mapfile(Mapfile* mapfile) const
  { mapfile->print_output_data(this, _("** incremental_inputs")); }

 private:
  // Write the section header.
  unsigned char*
  write_header(unsigned char* pov, unsigned int input_file_count,
	       section_offset_type command_line_offset);

  // Write the input file entries.
  unsigned char*
  write_input_files(unsigned char* oview, unsigned char* pov,
		    Stringpool* strtab);

  // Write the supplemental information blocks.
  unsigned char*
  write_info_blocks(unsigned char* oview, unsigned char* pov,
		    Stringpool* strtab, unsigned int* global_syms,
		    unsigned int global_sym_count);

  // Write the contents of the .gnu_incremental_symtab section.
  void
  write_symtab(unsigned char* pov, unsigned int* global_syms,
	       unsigned int global_sym_count);

  // Write the contents of the .gnu_incremental_got_plt section.
  void
  write_got_plt(unsigned char* pov, off_t view_size);

  // Typedefs for writing the data to the output sections.
  typedef elfcpp::Swap<size, big_endian> Swap;
  typedef elfcpp::Swap<16, big_endian> Swap16;
  typedef elfcpp::Swap<32, big_endian> Swap32;
  typedef elfcpp::Swap<64, big_endian> Swap64;

  // Sizes of various structures.
  static const int sizeof_addr = size / 8;
  static const int header_size = 16;
  static const int input_entry_size = 24;

  // The Incremental_inputs object.
  const Incremental_inputs* inputs_;

  // The symbol table.
  const Symbol_table* symtab_;
};

// Inform the user why we don't do an incremental link.  Not called in
// the obvious case of missing output file.  TODO: Is this helpful?

void
vexplain_no_incremental(const char* format, va_list args)
{
  char* buf = NULL;
  if (vasprintf(&buf, format, args) < 0)
    gold_nomem();
  gold_info(_("the link might take longer: "
              "cannot perform incremental link: %s"), buf);
  free(buf);
}

void
explain_no_incremental(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vexplain_no_incremental(format, args);
  va_end(args);
}

// Report an error.

void
Incremental_binary::error(const char* format, ...) const
{
  va_list args;
  va_start(args, format);
  // Current code only checks if the file can be used for incremental linking,
  // so errors shouldn't fail the build, but only result in a fallback to a
  // full build.
  // TODO: when we implement incremental editing of the file, we may need a
  // flag that will cause errors to be treated seriously.
  vexplain_no_incremental(format, args);
  va_end(args);
}

// Find the .gnu_incremental_inputs section and related sections.

template<int size, bool big_endian>
bool
Sized_incremental_binary<size, big_endian>::do_find_incremental_inputs_sections(
    unsigned int* p_inputs_shndx,
    unsigned int* p_symtab_shndx,
    unsigned int* p_relocs_shndx,
    unsigned int* p_got_plt_shndx,
    unsigned int* p_strtab_shndx)
{
  unsigned int inputs_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_GNU_INCREMENTAL_INPUTS);
  if (inputs_shndx == elfcpp::SHN_UNDEF)  // Not found.
    return false;

  unsigned int symtab_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_GNU_INCREMENTAL_SYMTAB);
  if (symtab_shndx == elfcpp::SHN_UNDEF)  // Not found.
    return false;
  if (this->elf_file_.section_link(symtab_shndx) != inputs_shndx)
    return false;

  unsigned int relocs_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_GNU_INCREMENTAL_RELOCS);
  if (relocs_shndx == elfcpp::SHN_UNDEF)  // Not found.
    return false;
  if (this->elf_file_.section_link(relocs_shndx) != inputs_shndx)
    return false;

  unsigned int got_plt_shndx =
      this->elf_file_.find_section_by_type(elfcpp::SHT_GNU_INCREMENTAL_GOT_PLT);
  if (got_plt_shndx == elfcpp::SHN_UNDEF)  // Not found.
    return false;
  if (this->elf_file_.section_link(got_plt_shndx) != inputs_shndx)
    return false;

  unsigned int strtab_shndx = this->elf_file_.section_link(inputs_shndx);
  if (strtab_shndx == elfcpp::SHN_UNDEF
      || strtab_shndx > this->elf_file_.shnum()
      || this->elf_file_.section_type(strtab_shndx) != elfcpp::SHT_STRTAB)
    return false;

  if (p_inputs_shndx != NULL)
    *p_inputs_shndx = inputs_shndx;
  if (p_symtab_shndx != NULL)
    *p_symtab_shndx = symtab_shndx;
  if (p_relocs_shndx != NULL)
    *p_relocs_shndx = relocs_shndx;
  if (p_got_plt_shndx != NULL)
    *p_got_plt_shndx = got_plt_shndx;
  if (p_strtab_shndx != NULL)
    *p_strtab_shndx = strtab_shndx;
  return true;
}

// Determine whether an incremental link based on the existing output file
// can be done.

template<int size, bool big_endian>
bool
Sized_incremental_binary<size, big_endian>::do_check_inputs(
    Incremental_inputs* incremental_inputs)
{
  unsigned int inputs_shndx;
  unsigned int symtab_shndx;
  unsigned int relocs_shndx;
  unsigned int plt_got_shndx;
  unsigned int strtab_shndx;

  if (!do_find_incremental_inputs_sections(&inputs_shndx, &symtab_shndx,
					   &relocs_shndx, &plt_got_shndx,
					   &strtab_shndx))
    {
      explain_no_incremental(_("no incremental data from previous build"));
      return false;
    }

  Location inputs_location(this->elf_file_.section_contents(inputs_shndx));
  Location symtab_location(this->elf_file_.section_contents(symtab_shndx));
  Location relocs_location(this->elf_file_.section_contents(relocs_shndx));
  Location strtab_location(this->elf_file_.section_contents(strtab_shndx));

  View inputs_view(view(inputs_location));
  View symtab_view(view(symtab_location));
  View relocs_view(view(relocs_location));
  View strtab_view(view(strtab_location));

  elfcpp::Elf_strtab strtab(strtab_view.data(), strtab_location.data_size);

  Incremental_inputs_reader<size, big_endian>
      incoming_inputs(inputs_view.data(), strtab);

  if (incoming_inputs.version() != INCREMENTAL_LINK_VERSION)
    {
      explain_no_incremental(_("different version of incremental build data"));
      return false;
    }

  if (incremental_inputs->command_line() != incoming_inputs.command_line())
    {
      explain_no_incremental(_("command line changed"));
      return false;
    }

  // TODO: compare incremental_inputs->inputs() with entries in data_view.

  return true;
}

namespace
{

// Create a Sized_incremental_binary object of the specified size and
// endianness. Fails if the target architecture is not supported.

template<int size, bool big_endian>
Incremental_binary*
make_sized_incremental_binary(Output_file* file,
                              const elfcpp::Ehdr<size, big_endian>& ehdr)
{
  Target* target = select_target(ehdr.get_e_machine(), size, big_endian,
                                 ehdr.get_e_ident()[elfcpp::EI_OSABI],
                                 ehdr.get_e_ident()[elfcpp::EI_ABIVERSION]);
  if (target == NULL)
    {
      explain_no_incremental(_("unsupported ELF machine number %d"),
               ehdr.get_e_machine());
      return NULL;
    }

  if (!parameters->target_valid())
    set_parameters_target(target);
  else if (target != &parameters->target())
    gold_error(_("%s: incompatible target"), file->filename());

  return new Sized_incremental_binary<size, big_endian>(file, ehdr, target);
}

}  // End of anonymous namespace.

// Create an Incremental_binary object for FILE.  Returns NULL is this is not
// possible, e.g. FILE is not an ELF file or has an unsupported target.  FILE
// should be opened.

Incremental_binary*
open_incremental_binary(Output_file* file)
{
  off_t filesize = file->filesize();
  int want = elfcpp::Elf_recognizer::max_header_size;
  if (filesize < want)
    want = filesize;

  const unsigned char* p = file->get_input_view(0, want);
  if (!elfcpp::Elf_recognizer::is_elf_file(p, want))
    {
      explain_no_incremental(_("output is not an ELF file."));
      return NULL;
    }

  int size = 0;
  bool big_endian = false;
  std::string error;
  if (!elfcpp::Elf_recognizer::is_valid_header(p, want, &size, &big_endian,
                                               &error))
    {
      explain_no_incremental(error.c_str());
      return NULL;
    }

  Incremental_binary* result = NULL;
  if (size == 32)
    {
      if (big_endian)
        {
#ifdef HAVE_TARGET_32_BIG
          result = make_sized_incremental_binary<32, true>(
              file, elfcpp::Ehdr<32, true>(p));
#else
          explain_no_incremental(_("unsupported file: 32-bit, big-endian"));
#endif
        }
      else
        {
#ifdef HAVE_TARGET_32_LITTLE
          result = make_sized_incremental_binary<32, false>(
              file, elfcpp::Ehdr<32, false>(p));
#else
          explain_no_incremental(_("unsupported file: 32-bit, little-endian"));
#endif
        }
    }
  else if (size == 64)
    {
      if (big_endian)
        {
#ifdef HAVE_TARGET_64_BIG
          result = make_sized_incremental_binary<64, true>(
              file, elfcpp::Ehdr<64, true>(p));
#else
          explain_no_incremental(_("unsupported file: 64-bit, big-endian"));
#endif
        }
      else
        {
#ifdef HAVE_TARGET_64_LITTLE
          result = make_sized_incremental_binary<64, false>(
              file, elfcpp::Ehdr<64, false>(p));
#else
          explain_no_incremental(_("unsupported file: 64-bit, little-endian"));
#endif
        }
    }
  else
    gold_unreachable();

  return result;
}

// Analyzes the output file to check if incremental linking is possible and
// (to be done) what files need to be relinked.

bool
Incremental_checker::can_incrementally_link_output_file()
{
  Output_file output(this->output_name_);
  if (!output.open_for_modification())
    return false;
  Incremental_binary* binary = open_incremental_binary(&output);
  if (binary == NULL)
    return false;
  return binary->check_inputs(this->incremental_inputs_);
}

// Class Incremental_inputs.

// Add the command line to the string table, setting
// command_line_key_.  In incremental builds, the command line is
// stored in .gnu_incremental_inputs so that the next linker run can
// check if the command line options didn't change.

void
Incremental_inputs::report_command_line(int argc, const char* const* argv)
{
  // Always store 'gold' as argv[0] to avoid a full relink if the user used a
  // different path to the linker.
  std::string args("gold");
  // Copied from collect_argv in main.cc.
  for (int i = 1; i < argc; ++i)
    {
      // Adding/removing these options should not result in a full relink.
      if (strcmp(argv[i], "--incremental") == 0
	  || strcmp(argv[i], "--incremental-full") == 0
	  || strcmp(argv[i], "--incremental-update") == 0
	  || strcmp(argv[i], "--incremental-changed") == 0
	  || strcmp(argv[i], "--incremental-unchanged") == 0
	  || strcmp(argv[i], "--incremental-unknown") == 0)
        continue;

      args.append(" '");
      // Now append argv[i], but with all single-quotes escaped
      const char* argpos = argv[i];
      while (1)
        {
          const int len = strcspn(argpos, "'");
          args.append(argpos, len);
          if (argpos[len] == '\0')
            break;
          args.append("'\"'\"'");
          argpos += len + 1;
        }
      args.append("'");
    }

  this->command_line_ = args;
  this->strtab_->add(this->command_line_.c_str(), false,
                     &this->command_line_key_);
}

// Record the input archive file ARCHIVE.  This is called by the
// Add_archive_symbols task before determining which archive members
// to include.  We create the Incremental_archive_entry here and
// attach it to the Archive, but we do not add it to the list of
// input objects until report_archive_end is called.

void
Incremental_inputs::report_archive_begin(Archive* arch)
{
  Stringpool::Key filename_key;
  Timespec mtime = arch->file().get_mtime();

  this->strtab_->add(arch->filename().c_str(), false, &filename_key);
  Incremental_archive_entry* entry =
      new Incremental_archive_entry(filename_key, arch, mtime);
  arch->set_incremental_info(entry);
}

// Finish recording the input archive file ARCHIVE.  This is called by the
// Add_archive_symbols task after determining which archive members
// to include.

void
Incremental_inputs::report_archive_end(Archive* arch)
{
  Incremental_archive_entry* entry = arch->incremental_info();

  gold_assert(entry != NULL);

  // Collect unused global symbols.
  for (Archive::Unused_symbol_iterator p = arch->unused_symbols_begin();
       p != arch->unused_symbols_end();
       ++p)
    {
      Stringpool::Key symbol_key;
      this->strtab_->add(*p, true, &symbol_key);
      entry->add_unused_global_symbol(symbol_key);
    }
  this->inputs_.push_back(entry);
}

// Record the input object file OBJ.  If ARCH is not NULL, attach
// the object file to the archive.  This is called by the
// Add_symbols task after finding out the type of the file.

void
Incremental_inputs::report_object(Object* obj, Archive* arch)
{
  Stringpool::Key filename_key;
  Timespec mtime = obj->input_file()->file().get_mtime();

  this->strtab_->add(obj->name().c_str(), false, &filename_key);
  Incremental_object_entry* obj_entry =
      new Incremental_object_entry(filename_key, obj, mtime);
  this->inputs_.push_back(obj_entry);

  if (arch != NULL)
    {
      Incremental_archive_entry* arch_entry = arch->incremental_info();
      gold_assert(arch_entry != NULL);
      arch_entry->add_object(obj_entry);
    }

  this->current_object_ = obj;
  this->current_object_entry_ = obj_entry;
}

// Record the input object file OBJ.  If ARCH is not NULL, attach
// the object file to the archive.  This is called by the
// Add_symbols task after finding out the type of the file.

void
Incremental_inputs::report_input_section(Object* obj, unsigned int shndx,
					 const char* name, off_t sh_size)
{
  Stringpool::Key key = 0;

  if (name != NULL)
      this->strtab_->add(name, true, &key);

  gold_assert(obj == this->current_object_);
  this->current_object_entry_->add_input_section(shndx, key, sh_size);
}

// Record that the input argument INPUT is a script SCRIPT.  This is
// called by read_script after parsing the script and reading the list
// of inputs added by this script.

void
Incremental_inputs::report_script(const std::string& filename,
				  Script_info* script, Timespec mtime)
{
  Stringpool::Key filename_key;

  this->strtab_->add(filename.c_str(), false, &filename_key);
  Incremental_script_entry* entry =
      new Incremental_script_entry(filename_key, script, mtime);
  this->inputs_.push_back(entry);
}

// Finalize the incremental link information.  Called from
// Layout::finalize.

void
Incremental_inputs::finalize()
{
  // Finalize the string table.
  this->strtab_->set_string_offsets();
}

// Create the .gnu_incremental_inputs, _symtab, and _relocs input sections.

void
Incremental_inputs::create_data_sections(Symbol_table* symtab)
{
  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->inputs_section_ =
          new Output_section_incremental_inputs<32, false>(this, symtab);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->inputs_section_ =
          new Output_section_incremental_inputs<32, true>(this, symtab);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->inputs_section_ =
          new Output_section_incremental_inputs<64, false>(this, symtab);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->inputs_section_ =
          new Output_section_incremental_inputs<64, true>(this, symtab);
      break;
#endif
    default:
      gold_unreachable();
    }
  this->symtab_section_ = new Output_data_space(4, "** incremental_symtab");
  this->relocs_section_ = new Output_data_space(4, "** incremental_relocs");
  this->got_plt_section_ = new Output_data_space(4, "** incremental_got_plt");
}

// Return the sh_entsize value for the .gnu_incremental_relocs section.
unsigned int
Incremental_inputs::relocs_entsize() const
{
  return 8 + 2 * parameters->target().get_size() / 8;
}

// Class Output_section_incremental_inputs.

// Finalize the offsets for each input section and supplemental info block,
// and set the final data size of the incremental output sections.

template<int size, bool big_endian>
void
Output_section_incremental_inputs<size, big_endian>::set_final_data_size()
{
  const Incremental_inputs* inputs = this->inputs_;
  const unsigned int sizeof_addr = size / 8;
  const unsigned int rel_size = 8 + 2 * sizeof_addr;

  // Offset of each input entry.
  unsigned int input_offset = this->header_size;

  // Offset of each supplemental info block.
  unsigned int info_offset = this->header_size;
  info_offset += this->input_entry_size * inputs->input_file_count();

  // Count each input file and its supplemental information block.
  for (Incremental_inputs::Input_list::const_iterator p =
	   inputs->input_files().begin();
       p != inputs->input_files().end();
       ++p)
    {
      // Set the offset of the input file entry.
      (*p)->set_offset(input_offset);
      input_offset += this->input_entry_size;

      // Set the offset of the supplemental info block.
      switch ((*p)->type())
	{
	case INCREMENTAL_INPUT_SCRIPT:
	  // No supplemental info for a script.
	  (*p)->set_info_offset(0);
	  break;
	case INCREMENTAL_INPUT_OBJECT:
	case INCREMENTAL_INPUT_ARCHIVE_MEMBER:
	  {
	    Incremental_object_entry* entry = (*p)->object_entry();
	    gold_assert(entry != NULL);
	    (*p)->set_info_offset(info_offset);
	    // Input section count + global symbol count.
	    info_offset += 8;
	    // Each input section.
	    info_offset += (entry->get_input_section_count()
			    * (8 + 2 * sizeof_addr));
	    // Each global symbol.
	    const Object::Symbols* syms = entry->object()->get_global_symbols();
	    info_offset += syms->size() * 16;
	  }
	  break;
	case INCREMENTAL_INPUT_SHARED_LIBRARY:
	  {
	    Incremental_object_entry* entry = (*p)->object_entry();
	    gold_assert(entry != NULL);
	    (*p)->set_info_offset(info_offset);
	    // Global symbol count.
	    info_offset += 4;
	    // Each global symbol.
	    const Object::Symbols* syms = entry->object()->get_global_symbols();
	    unsigned int nsyms = syms != NULL ? syms->size() : 0;
	    info_offset += nsyms * 4;
	  }
	  break;
	case INCREMENTAL_INPUT_ARCHIVE:
	  {
	    Incremental_archive_entry* entry = (*p)->archive_entry();
	    gold_assert(entry != NULL);
	    (*p)->set_info_offset(info_offset);
	    // Member count + unused global symbol count.
	    info_offset += 8;
	    // Each member.
	    info_offset += (entry->get_member_count() * 4);
	    // Each global symbol.
	    info_offset += (entry->get_unused_global_symbol_count() * 4);
	  }
	  break;
	default:
	  gold_unreachable();
	}
    }

  this->set_data_size(info_offset);

  // Set the size of the .gnu_incremental_symtab section.
  inputs->symtab_section()->set_current_data_size(this->symtab_->output_count()
						  * sizeof(unsigned int));

  // Set the size of the .gnu_incremental_relocs section.
  inputs->relocs_section()->set_current_data_size(inputs->get_reloc_count()
						  * rel_size);

  // Set the size of the .gnu_incremental_got_plt section.
  Sized_target<size, big_endian>* target =
    parameters->sized_target<size, big_endian>();
  unsigned int got_count = target->got_entry_count();
  unsigned int plt_count = target->plt_entry_count();
  unsigned int got_plt_size = 8;  // GOT entry count, PLT entry count.
  got_plt_size = (got_plt_size + got_count + 3) & ~3;  // GOT type array.
  got_plt_size += got_count * 4 + plt_count * 4;  // GOT array, PLT array.
  inputs->got_plt_section()->set_current_data_size(got_plt_size);
}

// Write the contents of the .gnu_incremental_inputs and
// .gnu_incremental_symtab sections.

template<int size, bool big_endian>
void
Output_section_incremental_inputs<size, big_endian>::do_write(Output_file* of)
{
  const Incremental_inputs* inputs = this->inputs_;
  Stringpool* strtab = inputs->get_stringpool();

  // Get a view into the .gnu_incremental_inputs section.
  const off_t off = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(off, oview_size);
  unsigned char* pov = oview;

  // Get a view into the .gnu_incremental_symtab section.
  const off_t symtab_off = inputs->symtab_section()->offset();
  const off_t symtab_size = inputs->symtab_section()->data_size();
  unsigned char* const symtab_view = of->get_output_view(symtab_off,
							 symtab_size);

  // Allocate an array of linked list heads for the .gnu_incremental_symtab
  // section.  Each element corresponds to a global symbol in the output
  // symbol table, and points to the head of the linked list that threads
  // through the object file input entries.  The value of each element
  // is the section-relative offset to a global symbol entry in a
  // supplemental information block.
  unsigned int global_sym_count = this->symtab_->output_count();
  unsigned int* global_syms = new unsigned int[global_sym_count];
  memset(global_syms, 0, global_sym_count * sizeof(unsigned int));

  // Write the section header.
  Stringpool::Key command_line_key = inputs->command_line_key();
  pov = this->write_header(pov, inputs->input_file_count(),
			   strtab->get_offset_from_key(command_line_key));

  // Write the list of input files.
  pov = this->write_input_files(oview, pov, strtab);

  // Write the supplemental information blocks for each input file.
  pov = this->write_info_blocks(oview, pov, strtab, global_syms,
				global_sym_count);

  gold_assert(pov - oview == oview_size);

  // Write the .gnu_incremental_symtab section.
  gold_assert(global_sym_count * 4 == symtab_size);
  this->write_symtab(symtab_view, global_syms, global_sym_count);

  delete[] global_syms;

  // Write the .gnu_incremental_got_plt section.
  const off_t got_plt_off = inputs->got_plt_section()->offset();
  const off_t got_plt_size = inputs->got_plt_section()->data_size();
  unsigned char* const got_plt_view = of->get_output_view(got_plt_off,
							  got_plt_size);
  this->write_got_plt(got_plt_view, got_plt_size);

  of->write_output_view(off, oview_size, oview);
  of->write_output_view(symtab_off, symtab_size, symtab_view);
  of->write_output_view(got_plt_off, got_plt_size, got_plt_view);
}

// Write the section header: version, input file count, offset of command line
// in the string table, and 4 bytes of padding.

template<int size, bool big_endian>
unsigned char*
Output_section_incremental_inputs<size, big_endian>::write_header(
    unsigned char* pov,
    unsigned int input_file_count,
    section_offset_type command_line_offset)
{
  Swap32::writeval(pov, INCREMENTAL_LINK_VERSION);
  Swap32::writeval(pov + 4, input_file_count);
  Swap32::writeval(pov + 8, command_line_offset);
  Swap32::writeval(pov + 12, 0);
  return pov + this->header_size;
}

// Write the input file entries.

template<int size, bool big_endian>
unsigned char*
Output_section_incremental_inputs<size, big_endian>::write_input_files(
    unsigned char* oview,
    unsigned char* pov,
    Stringpool* strtab)
{
  const Incremental_inputs* inputs = this->inputs_;

  for (Incremental_inputs::Input_list::const_iterator p =
	   inputs->input_files().begin();
       p != inputs->input_files().end();
       ++p)
    {
      gold_assert(static_cast<unsigned int>(pov - oview) == (*p)->get_offset());
      section_offset_type filename_offset =
          strtab->get_offset_from_key((*p)->get_filename_key());
      const Timespec& mtime = (*p)->get_mtime();
      Swap32::writeval(pov, filename_offset);
      Swap32::writeval(pov + 4, (*p)->get_info_offset());
      Swap64::writeval(pov + 8, mtime.seconds);
      Swap32::writeval(pov + 16, mtime.nanoseconds);
      Swap16::writeval(pov + 20, (*p)->type());
      Swap16::writeval(pov + 22, 0);
      pov += this->input_entry_size;
    }
  return pov;
}

// Write the supplemental information blocks.

template<int size, bool big_endian>
unsigned char*
Output_section_incremental_inputs<size, big_endian>::write_info_blocks(
    unsigned char* oview,
    unsigned char* pov,
    Stringpool* strtab,
    unsigned int* global_syms,
    unsigned int global_sym_count)
{
  const Incremental_inputs* inputs = this->inputs_;
  unsigned int first_global_index = this->symtab_->first_global_index();

  for (Incremental_inputs::Input_list::const_iterator p =
	   inputs->input_files().begin();
       p != inputs->input_files().end();
       ++p)
    {
      switch ((*p)->type())
	{
	case INCREMENTAL_INPUT_SCRIPT:
	  // No supplemental info for a script.
	  break;

	case INCREMENTAL_INPUT_OBJECT:
	case INCREMENTAL_INPUT_ARCHIVE_MEMBER:
	  {
	    gold_assert(static_cast<unsigned int>(pov - oview)
			== (*p)->get_info_offset());
	    Incremental_object_entry* entry = (*p)->object_entry();
	    gold_assert(entry != NULL);
	    const Object* obj = entry->object();
	    const Object::Symbols* syms = obj->get_global_symbols();
	    // Write the input section count and global symbol count.
	    unsigned int nsections = entry->get_input_section_count();
	    unsigned int nsyms = syms->size();
	    Swap32::writeval(pov, nsections);
	    Swap32::writeval(pov + 4, nsyms);
	    pov += 8;

	    // For each input section, write the name, output section index,
	    // offset within output section, and input section size.
	    for (unsigned int i = 0; i < nsections; i++)
	      {
		Stringpool::Key key = entry->get_input_section_name_key(i);
		off_t name_offset = 0;
		if (key != 0)
		  name_offset = strtab->get_offset_from_key(key);
		int out_shndx = 0;
		off_t out_offset = 0;
		off_t sh_size = 0;
		Output_section* os = obj->output_section(i);
		if (os != NULL)
		  {
		    out_shndx = os->out_shndx();
		    out_offset = obj->output_section_offset(i);
		    sh_size = entry->get_input_section_size(i);
		  }
		Swap32::writeval(pov, name_offset);
		Swap32::writeval(pov + 4, out_shndx);
		Swap::writeval(pov + 8, out_offset);
		Swap::writeval(pov + 8 + sizeof_addr, sh_size);
		pov += 8 + 2 * sizeof_addr;
	      }

	    // For each global symbol, write its associated relocations,
	    // add it to the linked list of globals, then write the
	    // supplemental information:  global symbol table index,
	    // linked list chain pointer, relocation count, and offset
	    // to the relocations.
	    for (unsigned int i = 0; i < nsyms; i++)
	      {
		const Symbol* sym = (*syms)[i];
		if (sym->is_forwarder())
		  sym = this->symtab_->resolve_forwards(sym);
		unsigned int symtab_index = sym->symtab_index();
		unsigned int chain = 0;
		unsigned int first_reloc = 0;
		unsigned int nrelocs = obj->get_incremental_reloc_count(i);
		if (nrelocs > 0)
		  {
		    gold_assert(symtab_index != -1U
				&& (symtab_index - first_global_index
				    < global_sym_count));
		    first_reloc = obj->get_incremental_reloc_base(i);
		    chain = global_syms[symtab_index - first_global_index];
		    global_syms[symtab_index - first_global_index] =
			pov - oview;
		  }
		Swap32::writeval(pov, symtab_index);
		Swap32::writeval(pov + 4, chain);
		Swap32::writeval(pov + 8, nrelocs);
		Swap32::writeval(pov + 12, first_reloc * 3 * sizeof_addr);
		pov += 16;
	      }
	  }
	  break;

	case INCREMENTAL_INPUT_SHARED_LIBRARY:
	  {
	    gold_assert(static_cast<unsigned int>(pov - oview)
			== (*p)->get_info_offset());
	    Incremental_object_entry* entry = (*p)->object_entry();
	    gold_assert(entry != NULL);
	    const Object* obj = entry->object();
	    const Object::Symbols* syms = obj->get_global_symbols();

	    // Write the global symbol count.
	    unsigned int nsyms = syms != NULL ? syms->size() : 0;
	    Swap32::writeval(pov, nsyms);
	    pov += 4;

	    // For each global symbol, write the global symbol table index.
	    for (unsigned int i = 0; i < nsyms; i++)
	      {
		const Symbol* sym = (*syms)[i];
		Swap32::writeval(pov, sym->symtab_index());
		pov += 4;
	      }
	  }
	  break;

	case INCREMENTAL_INPUT_ARCHIVE:
	  {
	    gold_assert(static_cast<unsigned int>(pov - oview)
			== (*p)->get_info_offset());
	    Incremental_archive_entry* entry = (*p)->archive_entry();
	    gold_assert(entry != NULL);

	    // Write the member count and unused global symbol count.
	    unsigned int nmembers = entry->get_member_count();
	    unsigned int nsyms = entry->get_unused_global_symbol_count();
	    Swap32::writeval(pov, nmembers);
	    Swap32::writeval(pov + 4, nsyms);
	    pov += 8;

	    // For each member, write the offset to its input file entry.
	    for (unsigned int i = 0; i < nmembers; ++i)
	      {
		Incremental_object_entry* member = entry->get_member(i);
		Swap32::writeval(pov, member->get_offset());
		pov += 4;
	      }

	    // For each global symbol, write the name offset.
	    for (unsigned int i = 0; i < nsyms; ++i)
	      {
		Stringpool::Key key = entry->get_unused_global_symbol(i);
		Swap32::writeval(pov, strtab->get_offset_from_key(key));
		pov += 4;
	      }
	  }
	  break;

	default:
	  gold_unreachable();
	}
    }
  return pov;
}

// Write the contents of the .gnu_incremental_symtab section.

template<int size, bool big_endian>
void
Output_section_incremental_inputs<size, big_endian>::write_symtab(
    unsigned char* pov,
    unsigned int* global_syms,
    unsigned int global_sym_count)
{
  for (unsigned int i = 0; i < global_sym_count; ++i)
    {
      Swap32::writeval(pov, global_syms[i]);
      pov += 4;
    }
}

// This struct holds the view information needed to write the
// .gnu_incremental_got_plt section.

struct Got_plt_view_info
{
  // Start of the GOT type array in the output view.
  unsigned char* got_type_p;
  // Start of the GOT descriptor array in the output view.
  unsigned char* got_desc_p;
  // Start of the PLT descriptor array in the output view.
  unsigned char* plt_desc_p;
  // Number of GOT entries.
  unsigned int got_count;
  // Number of PLT entries.
  unsigned int plt_count;
  // Offset of the first non-reserved PLT entry (this is a target-dependent value).
  unsigned int first_plt_entry_offset;
  // Size of a PLT entry (this is a target-dependent value).
  unsigned int plt_entry_size;
  // Value to write in the GOT descriptor array.  For global symbols,
  // this is the global symbol table index; for local symbols, it is
  // the offset of the input file entry in the .gnu_incremental_inputs
  // section.
  unsigned int got_descriptor;
};

// Functor class for processing a GOT offset list for local symbols.
// Writes the GOT type and symbol index into the GOT type and descriptor
// arrays in the output section.

template<int size, bool big_endian>
class Local_got_offset_visitor
{
 public:
  Local_got_offset_visitor(struct Got_plt_view_info& info)
    : info_(info)
  { }

  void
  operator()(unsigned int got_type, unsigned int got_offset)
  {
    unsigned int got_index = got_offset / this->got_entry_size_;
    gold_assert(got_index < this->info_.got_count);
    // We can only handle GOT entry types in the range 0..0x7e
    // because we use a byte array to store them, and we use the
    // high bit to flag a local symbol.
    gold_assert(got_type < 0x7f);
    this->info_.got_type_p[got_index] = got_type | 0x80;
    unsigned char* pov = this->info_.got_desc_p + got_index * 4;
    elfcpp::Swap<32, big_endian>::writeval(pov, this->info_.got_descriptor);
  }

 private:
  static const unsigned int got_entry_size_ = size / 8;
  struct Got_plt_view_info& info_;
};

// Functor class for processing a GOT offset list.  Writes the GOT type
// and symbol index into the GOT type and descriptor arrays in the output
// section.

template<int size, bool big_endian>
class Global_got_offset_visitor
{
 public:
  Global_got_offset_visitor(struct Got_plt_view_info& info)
    : info_(info)
  { }

  void
  operator()(unsigned int got_type, unsigned int got_offset)
  {
    unsigned int got_index = got_offset / this->got_entry_size_;
    gold_assert(got_index < this->info_.got_count);
    // We can only handle GOT entry types in the range 0..0x7e
    // because we use a byte array to store them, and we use the
    // high bit to flag a local symbol.
    gold_assert(got_type < 0x7f);
    this->info_.got_type_p[got_index] = got_type;
    unsigned char* pov = this->info_.got_desc_p + got_index * 4;
    elfcpp::Swap<32, big_endian>::writeval(pov, this->info_.got_descriptor);
  }

 private:
  static const unsigned int got_entry_size_ = size / 8;
  struct Got_plt_view_info& info_;
};

// Functor class for processing the global symbol table.  Processes the
// GOT offset list for the symbol, and writes the symbol table index
// into the PLT descriptor array in the output section.

template<int size, bool big_endian>
class Global_symbol_visitor_got_plt
{
 public:
  Global_symbol_visitor_got_plt(struct Got_plt_view_info& info)
    : info_(info)
  { }

  void
  operator()(const Sized_symbol<size>* sym)
  {
    typedef Global_got_offset_visitor<size, big_endian> Got_visitor;
    const Got_offset_list* got_offsets = sym->got_offset_list();
    if (got_offsets != NULL)
      {
        info_.got_descriptor = sym->symtab_index();
	got_offsets->for_all_got_offsets(Got_visitor(info_));
      }
    if (sym->has_plt_offset())
      {
	unsigned int plt_index =
	    ((sym->plt_offset() - this->info_.first_plt_entry_offset)
	     / this->info_.plt_entry_size);
	gold_assert(plt_index < this->info_.plt_count);
	unsigned char* pov = this->info_.plt_desc_p + plt_index * 4;
	elfcpp::Swap<32, big_endian>::writeval(pov, sym->symtab_index());
      }
  }

 private:
  struct Got_plt_view_info& info_;
};

// Write the contents of the .gnu_incremental_got_plt section.

template<int size, bool big_endian>
void
Output_section_incremental_inputs<size, big_endian>::write_got_plt(
    unsigned char* pov,
    off_t view_size)
{
  Sized_target<size, big_endian>* target =
    parameters->sized_target<size, big_endian>();

  // Set up the view information for the functors.
  struct Got_plt_view_info view_info;
  view_info.got_count = target->got_entry_count();
  view_info.plt_count = target->plt_entry_count();
  view_info.first_plt_entry_offset = target->first_plt_entry_offset();
  view_info.plt_entry_size = target->plt_entry_size();
  view_info.got_type_p = pov + 8;
  view_info.got_desc_p = (view_info.got_type_p
			  + ((view_info.got_count + 3) & ~3));
  view_info.plt_desc_p = view_info.got_desc_p + view_info.got_count * 4;

  gold_assert(pov + view_size ==
	      view_info.plt_desc_p + view_info.plt_count * 4);

  // Write the section header.
  Swap32::writeval(pov, view_info.got_count);
  Swap32::writeval(pov + 4, view_info.plt_count);

  // Initialize the GOT type array to 0xff (reserved).
  memset(view_info.got_type_p, 0xff, view_info.got_count);

  // Write the incremental GOT descriptors for local symbols.
  for (Incremental_inputs::Input_list::const_iterator p =
	   this->inputs_->input_files().begin();
       p != this->inputs_->input_files().end();
       ++p)
    {
      if ((*p)->type() != INCREMENTAL_INPUT_OBJECT
	  && (*p)->type() != INCREMENTAL_INPUT_ARCHIVE_MEMBER)
	continue;
      Incremental_object_entry* entry = (*p)->object_entry();
      gold_assert(entry != NULL);
      const Sized_relobj<size, big_endian>* obj =
          static_cast<Sized_relobj<size, big_endian>*>(entry->object());
      gold_assert(obj != NULL);
      unsigned int nsyms = obj->local_symbol_count();
      for (unsigned int i = 0; i < nsyms; i++)
        {
          const Got_offset_list* got_offsets = obj->local_got_offset_list(i);
          if (got_offsets != NULL)
            {
	      typedef Local_got_offset_visitor<size, big_endian> Got_visitor;
	      view_info.got_descriptor = (*p)->get_offset();
	      got_offsets->for_all_got_offsets(Got_visitor(view_info));
	    }
	}
    }

  // Write the incremental GOT and PLT descriptors for global symbols.
  typedef Global_symbol_visitor_got_plt<size, big_endian> Symbol_visitor;
  symtab_->for_all_symbols<size, Symbol_visitor>(Symbol_visitor(view_info));
}

// Instantiate the templates we need.

#ifdef HAVE_TARGET_32_LITTLE
template
class Sized_incremental_binary<32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Sized_incremental_binary<32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Sized_incremental_binary<64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Sized_incremental_binary<64, true>;
#endif

} // End namespace gold.
