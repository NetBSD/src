// inremental.h -- incremental linking support for gold   -*- C++ -*-

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

#ifndef GOLD_INCREMENTAL_H
#define GOLD_INCREMENTAL_H

#include <map>
#include <vector>

#include "elfcpp_file.h"
#include "stringpool.h"
#include "workqueue.h"
#include "fileread.h"
#include "output.h"

namespace gold
{

class Archive;
class Input_argument;
class Incremental_inputs_checker;
class Incremental_script_entry;
class Incremental_object_entry;
class Incremental_archive_entry;
class Incremental_inputs;
class Object;

// Incremental input type as stored in .gnu_incremental_inputs.

enum Incremental_input_type
{
  INCREMENTAL_INPUT_OBJECT = 1,
  INCREMENTAL_INPUT_ARCHIVE_MEMBER = 2,
  INCREMENTAL_INPUT_ARCHIVE = 3,
  INCREMENTAL_INPUT_SHARED_LIBRARY = 4,
  INCREMENTAL_INPUT_SCRIPT = 5
};

// An object representing the ELF file we edit during an incremental build.
// Similar to Object or Dynobj, but operates on Output_file and contains
// method specific to file edition (TBD). This is the abstract parent class
// implemented in Sized_incremental_binary<size, big_endian> for a specific
// endianness and size.

class Incremental_binary
{
 public:
  Incremental_binary(Output_file* output, Target* target)
    : output_(output), target_(target)
  { }

  virtual
  ~Incremental_binary()
  { }

  // Functions and types for the elfcpp::Elf_file interface.  This
  // permit us to use Incremental_binary as the File template parameter for
  // elfcpp::Elf_file.

  // The View class is returned by view.  It must support a single
  // method, data().  This is trivial, because Output_file::get_output_view
  // does what we need.
  class View
  {
   public:
    View(const unsigned char* p)
      : p_(p)
    { }

    const unsigned char*
    data() const
    { return this->p_; }

   private:
    const unsigned char* p_;
  };

  // Return a View.
  View
  view(off_t file_offset, section_size_type data_size)
  { return View(this->output_->get_input_view(file_offset, data_size)); }

  // A location in the file.
  struct Location
  {
    off_t file_offset;
    off_t data_size;

    Location(off_t fo, section_size_type ds)
      : file_offset(fo), data_size(ds)
    { }

    Location()
      : file_offset(0), data_size(0)
    { }
  };

  // Get a View given a Location.
  View
  view(Location loc)
  { return View(this->view(loc.file_offset, loc.data_size)); }

  // Report an error.
  void
  error(const char* format, ...) const ATTRIBUTE_PRINTF_2;

  // Find the .gnu_incremental_inputs and related sections.  It selects the
  // first section of type SHT_GNU_INCREMENTAL_INPUTS,
  // SHT_GNU_INCRMENTAL_SYMTAB, and SHT_GNU_INCREMENTAL_RELOCS.
  // Returns false if the sections are not found.
  bool
  find_incremental_inputs_sections(unsigned int* p_inputs_shndx,
				   unsigned int* p_symtab_shndx,
				   unsigned int* p_relocs_shndx,
				   unsigned int* p_got_plt_shndx,
				   unsigned int* p_strtab_shndx)
  {
    return do_find_incremental_inputs_sections(p_inputs_shndx, p_symtab_shndx,
					       p_relocs_shndx, p_got_plt_shndx,
  					       p_strtab_shndx);
  }

  // Check the .gnu_incremental_inputs section to see whether an incremental
  // build is possible.
  // TODO: on success, should report what files needs to be rebuilt.
  // INCREMENTAL_INPUTS is used to read the canonical form of the command line
  // and read the input arguments.  TODO: for items that don't need to be
  // rebuilt, we should also copy the incremental input information.
  virtual bool
  check_inputs(Incremental_inputs* incremental_inputs)
  { return do_check_inputs(incremental_inputs); }

 protected:
  // Find incremental inputs section.
  virtual bool
  do_find_incremental_inputs_sections(unsigned int* p_inputs_shndx,
				      unsigned int* p_symtab_shndx,
				      unsigned int* p_relocs_shndx,
				      unsigned int* p_got_plt_shndx,
				      unsigned int* p_strtab_shndx) = 0;

  // Check the .gnu_incremental_inputs section to see whether an incremental
  // build is possible.
  virtual bool
  do_check_inputs(Incremental_inputs* incremental_inputs) = 0;

 private:
  // Edited output file object.
  Output_file* output_;
  // Target of the output file.
  Target* target_;
};

template<int size, bool big_endian>
class Sized_incremental_binary : public Incremental_binary
{
 public:
  Sized_incremental_binary(Output_file* output,
                           const elfcpp::Ehdr<size, big_endian>& ehdr,
                           Target* target)
    : Incremental_binary(output, target), elf_file_(this, ehdr)
  { }

 protected:
  virtual bool
  do_find_incremental_inputs_sections(unsigned int* p_inputs_shndx,
				      unsigned int* p_symtab_shndx,
				      unsigned int* p_relocs_shndx,
				      unsigned int* p_got_plt_shndx,
				      unsigned int* p_strtab_shndx);

  virtual bool
  do_check_inputs(Incremental_inputs* incremental_inputs);

 private:
  // Output as an ELF file.
  elfcpp::Elf_file<size, big_endian, Incremental_binary> elf_file_;
};

// Create an Incremental_binary object for FILE. Returns NULL is this is not
// possible, e.g. FILE is not an ELF file or has an unsupported target.

Incremental_binary*
open_incremental_binary(Output_file* file);

// Code invoked early during an incremental link that checks what files need
// to be relinked.

class Incremental_checker
{
 public:
  // Check if the file named OUTPUT_NAME can be linked incrementally.
  // INCREMENTAL_INPUTS must have the canonical form of the command line
  // and input arguments filled - at this point of linking other fields are
  // probably not filled yet.  TODO: for inputs that don't need to be
  // rebuilt, this function should fill the incremental input information.
  Incremental_checker(const char* output_name,
                      Incremental_inputs* incremental_inputs)
    : output_name_(output_name), incremental_inputs_(incremental_inputs)
  { }

  // Analyzes the output file to check if incremental linking is possible and
  // what files needs to be relinked.
  bool
  can_incrementally_link_output_file();

 private:
  // Name of the output file to analyze.
  const char* output_name_;

  // The Incremental_inputs object. At this stage of link, only the command
  // line and inputs are filled.
  Incremental_inputs* incremental_inputs_;
};

// Base class for recording each input file.

class Incremental_input_entry
{
 public:
  Incremental_input_entry(Stringpool::Key filename_key, Timespec mtime)
    : filename_key_(filename_key), offset_(0), info_offset_(0), mtime_(mtime)
  { }

  virtual
  ~Incremental_input_entry()
  { }

  // Return the type of input file.
  Incremental_input_type
  type() const
  { return this->do_type(); }

  // Set the section offset of this input file entry.
  void
  set_offset(unsigned int offset)
  { this->offset_ = offset; }

  // Set the section offset of the supplemental information for this entry.
  void
  set_info_offset(unsigned int info_offset)
  { this->info_offset_ = info_offset; }

  // Get the section offset of this input file entry.
  unsigned int
  get_offset() const
  { return this->offset_; }

  // Get the section offset of the supplemental information for this entry.
  unsigned int
  get_info_offset() const
  { return this->info_offset_; }

  // Get the stringpool key for the input filename.
  Stringpool::Key
  get_filename_key() const
  { return this->filename_key_; }

  // Get the modification time of the input file.
  const Timespec&
  get_mtime() const
  { return this->mtime_; }

  // Return a pointer to the derived Incremental_script_entry object.
  // Return NULL for input entries that are not script files.
  Incremental_script_entry*
  script_entry()
  { return this->do_script_entry(); }

  // Return a pointer to the derived Incremental_object_entry object.
  // Return NULL for input entries that are not object files.
  Incremental_object_entry*
  object_entry()
  { return this->do_object_entry(); }

  // Return a pointer to the derived Incremental_archive_entry object.
  // Return NULL for input entries that are not archive files.
  Incremental_archive_entry*
  archive_entry()
  { return this->do_archive_entry(); }

 protected:
  // Return the type of input file.
  virtual Incremental_input_type
  do_type() const = 0;

  // Return a pointer to the derived Incremental_script_entry object.
  // Return NULL for input entries that are not script files.
  virtual Incremental_script_entry*
  do_script_entry()
  { return NULL; }

  // Return a pointer to the derived Incremental_object_entry object.
  // Return NULL for input entries that are not object files.
  virtual Incremental_object_entry*
  do_object_entry()
  { return NULL; }

  // Return a pointer to the derived Incremental_archive_entry object.
  // Return NULL for input entries that are not archive files.
  virtual Incremental_archive_entry*
  do_archive_entry()
  { return NULL; }

 private:
  // Key of the filename string in the section stringtable.
  Stringpool::Key filename_key_;

  // Offset of the entry in the output section.
  unsigned int offset_;

  // Offset of the extra information in the output section.
  unsigned int info_offset_;

  // Last modification time of the file.
  Timespec mtime_;
};

// Class for recording input scripts.

class Incremental_script_entry : public Incremental_input_entry
{
 public:
  Incremental_script_entry(Stringpool::Key filename_key, Script_info* script,
			   Timespec mtime)
    : Incremental_input_entry(filename_key, mtime), script_(script)
  { }

 protected:
  virtual Incremental_input_type
  do_type() const
  { return INCREMENTAL_INPUT_SCRIPT; }

  // Return a pointer to the derived Incremental_script_entry object.
  virtual Incremental_script_entry*
  do_script_entry()
  { return this; }

 private:
  // Information about the script file.
  Script_info* script_;
};

// Class for recording input object files.

class Incremental_object_entry : public Incremental_input_entry
{
 public:
  Incremental_object_entry(Stringpool::Key filename_key, Object* obj,
			   Timespec mtime)
    : Incremental_input_entry(filename_key, mtime), obj_(obj),
      is_member_(false), sections_()
  {
    if (!obj_->is_dynamic())
      this->sections_.reserve(obj->shnum());
  }

  // Get the object.
  Object*
  object() const
  { return this->obj_; }

  // Record that this object is an archive member.
  void
  set_is_member()
  { this->is_member_ = true; }

  // Return true if this object is an archive member.
  bool
  is_member() const
  { return this->is_member_; }

  // Add an input section.
  void
  add_input_section(unsigned int shndx, Stringpool::Key name_key, off_t sh_size)
  {
    if (shndx >= this->sections_.size())
      this->sections_.resize(shndx + 1);
    this->sections_[shndx].name_key = name_key;
    this->sections_[shndx].sh_size = sh_size;
  }

  // Return the number of input sections in this object.
  unsigned int
  get_input_section_count() const
  { return this->sections_.size(); }

  // Return the stringpool key of the Nth input section.
  Stringpool::Key
  get_input_section_name_key(unsigned int n) const
  { return this->sections_[n].name_key; }

  // Return the size of the Nth input section.
  off_t
  get_input_section_size(unsigned int n) const
  { return this->sections_[n].sh_size; }

 protected:
  virtual Incremental_input_type
  do_type() const
  {
    return (this->is_member_
	    ? INCREMENTAL_INPUT_ARCHIVE_MEMBER
	    : (this->obj_->is_dynamic()
	       ? INCREMENTAL_INPUT_SHARED_LIBRARY
	       : INCREMENTAL_INPUT_OBJECT));
  }

  // Return a pointer to the derived Incremental_object_entry object.
  virtual Incremental_object_entry*
  do_object_entry()
  { return this; }

 private:
  // The object file itself.
  Object* obj_;

  // Whether this object is an archive member.
  bool is_member_;

  // Input sections.
  struct Input_section
  {
    Stringpool::Key name_key;
    off_t sh_size;
  };
  std::vector<Input_section> sections_;
};

// Class for recording archive library input files.

class Incremental_archive_entry : public Incremental_input_entry
{
 public:
  Incremental_archive_entry(Stringpool::Key filename_key, Archive*,
			    Timespec mtime)
    : Incremental_input_entry(filename_key, mtime), members_(), unused_syms_()
  { }

  // Add a member object to the archive.
  void
  add_object(Incremental_object_entry* obj_entry)
  {
    this->members_.push_back(obj_entry);
    obj_entry->set_is_member();
  }

  // Add an unused global symbol to the archive.
  void
  add_unused_global_symbol(Stringpool::Key symbol_key)
  { this->unused_syms_.push_back(symbol_key); }

  // Return the number of member objects included in the link.
  unsigned int
  get_member_count()
  { return this->members_.size(); }

  // Return the Nth member object.
  Incremental_object_entry*
  get_member(unsigned int n)
  { return this->members_[n]; }

  // Return the number of unused global symbols in this archive.
  unsigned int
  get_unused_global_symbol_count()
  { return this->unused_syms_.size(); }

  // Return the Nth unused global symbol.
  Stringpool::Key
  get_unused_global_symbol(unsigned int n)
  { return this->unused_syms_[n]; }

 protected:
  virtual Incremental_input_type
  do_type() const
  { return INCREMENTAL_INPUT_ARCHIVE; }

  // Return a pointer to the derived Incremental_archive_entry object.
  virtual Incremental_archive_entry*
  do_archive_entry()
  { return this; }

 private:
  // Members of the archive that have been included in the link.
  std::vector<Incremental_object_entry*> members_;

  // Unused global symbols from this archive.
  std::vector<Stringpool::Key> unused_syms_;
};

// This class contains the information needed during an incremental
// build about the inputs necessary to build the .gnu_incremental_inputs.

class Incremental_inputs
{
 public:
  typedef std::vector<Incremental_input_entry*> Input_list;

  Incremental_inputs()
    : inputs_(), command_line_(), command_line_key_(0),
      strtab_(new Stringpool()), current_object_(NULL),
      current_object_entry_(NULL), inputs_section_(NULL),
      symtab_section_(NULL), relocs_section_(NULL),
      reloc_count_(0)
  { }

  ~Incremental_inputs() { delete this->strtab_; }

  // Record the command line.
  void
  report_command_line(int argc, const char* const* argv);

  // Record the initial info for archive file ARCHIVE.
  void
  report_archive_begin(Archive* arch);

  // Record the final info for archive file ARCHIVE.
  void
  report_archive_end(Archive* arch);

  // Record the info for object file OBJ.  If ARCH is not NULL,
  // attach the object file to the archive.
  void
  report_object(Object* obj, Archive* arch);

  // Record an input section belonging to object file OBJ.
  void
  report_input_section(Object* obj, unsigned int shndx, const char* name,
		       off_t sh_size);

  // Record the info for input script SCRIPT.
  void
  report_script(const std::string& filename, Script_info* script,
		Timespec mtime);

  // Return the running count of incremental relocations.
  unsigned int
  get_reloc_count() const
  { return this->reloc_count_; }

  // Update the running count of incremental relocations.
  void
  set_reloc_count(unsigned int count)
  { this->reloc_count_ = count; }

  // Prepare for layout.  Called from Layout::finalize.
  void
  finalize();

  // Create the .gnu_incremental_inputs and related sections.
  void
  create_data_sections(Symbol_table* symtab);

  // Return the .gnu_incremental_inputs section.
  Output_section_data*
  inputs_section() const
  { return this->inputs_section_; }

  // Return the .gnu_incremental_symtab section.
  Output_data_space*
  symtab_section() const
  { return this->symtab_section_; }

  // Return the .gnu_incremental_relocs section.
  Output_data_space*
  relocs_section() const
  { return this->relocs_section_; }

  // Return the .gnu_incremental_got_plt section.
  Output_data_space*
  got_plt_section() const
  { return this->got_plt_section_; }

  // Return the .gnu_incremental_strtab stringpool.
  Stringpool*
  get_stringpool() const
  { return this->strtab_; }

  // Return the canonical form of the command line, as will be stored in
  // .gnu_incremental_strtab.
  const std::string&
  command_line() const
  { return this->command_line_; }

  // Return the stringpool key of the command line.
  Stringpool::Key
  command_line_key() const
  { return this->command_line_key_; }

  // Return the number of input files.
  int
  input_file_count() const
  { return this->inputs_.size(); }

  // Return the input files.
  const Input_list&
  input_files() const
  { return this->inputs_; }

  // Return the sh_entsize value for the .gnu_incremental_relocs section.
  unsigned int
  relocs_entsize() const;

 private:
  // The list of input files.
  Input_list inputs_;

  // Canonical form of the command line, as will be stored in
  // .gnu_incremental_strtab.
  std::string command_line_;

  // The key of the command line string in the string pool.
  Stringpool::Key command_line_key_;

  // The .gnu_incremental_strtab string pool associated with the
  // .gnu_incremental_inputs.
  Stringpool* strtab_;

  // Keep track of the object currently being processed.
  Object* current_object_;
  Incremental_object_entry* current_object_entry_;

  // The .gnu_incremental_inputs section.
  Output_section_data* inputs_section_;

  // The .gnu_incremental_symtab section.
  Output_data_space* symtab_section_;

  // The .gnu_incremental_relocs section.
  Output_data_space* relocs_section_;

  // The .gnu_incremental_got_plt section.
  Output_data_space* got_plt_section_;

  // Total count of incremental relocations.  Updated during Scan_relocs
  // phase at the completion of each object file.
  unsigned int reloc_count_;
};

// Reader class for .gnu_incremental_inputs section.

template<int size, bool big_endian>
class Incremental_inputs_reader
{
 private:
  typedef elfcpp::Swap<size, big_endian> Swap;
  typedef elfcpp::Swap<16, big_endian> Swap16;
  typedef elfcpp::Swap<32, big_endian> Swap32;
  typedef elfcpp::Swap<64, big_endian> Swap64;

 public:
  Incremental_inputs_reader(const unsigned char* p, elfcpp::Elf_strtab& strtab)
    : p_(p), strtab_(strtab)
  { this->input_file_count_ = Swap32::readval(this->p_ + 4); }

  // Return the version number.
  unsigned int
  version() const
  { return Swap32::readval(this->p_); }

  // Return the count of input file entries.
  unsigned int
  input_file_count() const
  { return this->input_file_count_; }

  // Return the command line.
  const char*
  command_line() const
  {
    unsigned int offset = Swap32::readval(this->p_ + 8);
    return this->get_string(offset);
  }

  // Reader class for an input file entry and its supplemental info.
  class Incremental_input_entry_reader
  {
   public:
    Incremental_input_entry_reader(const Incremental_inputs_reader* inputs,
				   unsigned int offset)
      : inputs_(inputs), offset_(offset)
    {
      this->info_offset_ = Swap32::readval(inputs->p_ + offset + 4);
      int type = Swap16::readval(this->inputs_->p_ + offset + 20);
      this->type_ = static_cast<Incremental_input_type>(type);
    }

    // Return the filename.
    const char*
    filename() const
    {
      unsigned int offset = Swap32::readval(this->inputs_->p_ + this->offset_);
      return this->inputs_->get_string(offset);
    }

    // Return the timestamp.
    Timespec
    get_mtime() const
    {
      Timespec t;
      const unsigned char* p = this->inputs_->p_ + this->offset_ + 8;
      t.seconds = Swap64::readval(p);
      t.nanoseconds = Swap32::readval(p+8);
      return t;
    }

    // Return the type of input file.
    Incremental_input_type
    type() const
    { return this->type_; }

    // Return the input section count -- for objects only.
    unsigned int
    get_input_section_count() const
    {
      gold_assert(this->type_ == INCREMENTAL_INPUT_OBJECT
		  || this->type_ == INCREMENTAL_INPUT_ARCHIVE_MEMBER);
      return Swap32::readval(this->inputs_->p_ + this->info_offset_);
    }

    // Return the offset of the supplemental info for symbol SYMNDX --
    // for objects only.
    unsigned int
    get_symbol_offset(unsigned int symndx) const
    {
      gold_assert(this->type_ == INCREMENTAL_INPUT_OBJECT
		  || this->type_ == INCREMENTAL_INPUT_ARCHIVE_MEMBER);

      unsigned int section_count = this->get_input_section_count();
      return (this->info_offset_ + 8
	      + section_count * input_section_entry_size
	      + symndx * 16);
    }

    // Return the global symbol count -- for objects & shared libraries only.
    unsigned int
    get_global_symbol_count() const
    {
      switch (this->type_)
	{
	case INCREMENTAL_INPUT_OBJECT:
	case INCREMENTAL_INPUT_ARCHIVE_MEMBER:
	  return Swap32::readval(this->inputs_->p_ + this->info_offset_ + 4);
	case INCREMENTAL_INPUT_SHARED_LIBRARY:
	  return Swap32::readval(this->inputs_->p_ + this->info_offset_);
	default:
	  gold_unreachable();
	}
    }

    // Return the member count -- for archives only.
    unsigned int
    get_member_count() const
    {
      gold_assert(this->type_ == INCREMENTAL_INPUT_ARCHIVE);
      return Swap32::readval(this->inputs_->p_ + this->info_offset_);
    }

    // Return the unused symbol count -- for archives only.
    unsigned int
    get_unused_symbol_count() const
    {
      gold_assert(this->type_ == INCREMENTAL_INPUT_ARCHIVE);
      return Swap32::readval(this->inputs_->p_ + this->info_offset_ + 4);
    }

    // Return the input file offset for archive member N -- for archives only.
    unsigned int
    get_member_offset(unsigned int n) const
    {
      gold_assert(this->type_ == INCREMENTAL_INPUT_ARCHIVE);
      return Swap32::readval(this->inputs_->p_ + this->info_offset_
			     + 8 + n * 4);
    }

    // Return the Nth unused global symbol -- for archives only.
    const char*
    get_unused_symbol(unsigned int n) const
    {
      gold_assert(this->type_ == INCREMENTAL_INPUT_ARCHIVE);
      unsigned int member_count = this->get_member_count();
      unsigned int offset = Swap32::readval(this->inputs_->p_
					    + this->info_offset_ + 8
					    + member_count * 4
					    + n * 4);
      return this->inputs_->get_string(offset);
    }

    // Information about an input section.
    struct Input_section_info
    {
      const char* name;
      unsigned int output_shndx;
      off_t sh_offset;
      off_t sh_size;
    };

    // Return info about the Nth input section -- for objects only.
    Input_section_info
    get_input_section(unsigned int n) const
    {
      Input_section_info info;
      const unsigned char* p = (this->inputs_->p_
				+ this->info_offset_ + 8
				+ n * input_section_entry_size);
      unsigned int name_offset = Swap32::readval(p);
      info.name = this->inputs_->get_string(name_offset);
      info.output_shndx = Swap32::readval(p + 4);
      info.sh_offset = Swap::readval(p + 8);
      info.sh_size = Swap::readval(p + 8 + size / 8);
      return info;
    }

    // Information about a global symbol.
    struct Global_symbol_info
    {
      unsigned int output_symndx;
      unsigned int next_offset;
      unsigned int reloc_count;
      unsigned int reloc_offset;
    };

    // Return info about the Nth global symbol -- for objects only.
    Global_symbol_info
    get_global_symbol_info(unsigned int n) const
    {
      Global_symbol_info info;
      unsigned int section_count = this->get_input_section_count();
      const unsigned char* p = (this->inputs_->p_
				+ this->info_offset_ + 8
				+ section_count * input_section_entry_size
				+ n * 16);
      info.output_symndx = Swap32::readval(p);
      info.next_offset = Swap32::readval(p + 4);
      info.reloc_count = Swap::readval(p + 8);
      info.reloc_offset = Swap::readval(p + 12);
      return info;
    }

   private:
    // Size of an input section entry.
    static const unsigned int input_section_entry_size = 8 + 2 * size / 8;
    // The reader instance for the containing section.
    const Incremental_inputs_reader* inputs_;
    // The type of input file.
    Incremental_input_type type_;
    // Section offset to the input file entry.
    unsigned int offset_;
    // Section offset to the supplemental info for the input file.
    unsigned int info_offset_;
  };

  // Return a reader for the Nth input file entry.
  Incremental_input_entry_reader
  input_file(unsigned int n) const
  {
    gold_assert(n < this->input_file_count_);
    Incremental_input_entry_reader input(this, 16 + n * 24);
    return input;
  }

 private:
  // Lookup a string in the ELF string table.
  const char* get_string(unsigned int offset) const
  {
    const char* s;
    if (this->strtab_.get_c_string(offset, &s))
      return s;
    return NULL;
  }

  // Base address of the .gnu_incremental_inputs section.
  const unsigned char* p_;
  // The associated ELF string table.
  elfcpp::Elf_strtab strtab_;
  // The number of input file entries in this section.
  unsigned int input_file_count_;
};

// Reader class for the .gnu_incremental_symtab section.

template<bool big_endian>
class Incremental_symtab_reader
{
 public:
  Incremental_symtab_reader(const unsigned char* p) : p_(p)
  { }

  // Return the list head for symbol table entry N.
  unsigned int get_list_head(unsigned int n) const
  { return elfcpp::Swap<32, big_endian>::readval(this->p_ + 4 * n); }

 private:
  // Base address of the .gnu_incremental_relocs section.
  const unsigned char* p_;
};

// Reader class for the .gnu_incremental_relocs section.

template<int size, bool big_endian>
class Incremental_relocs_reader
{
 private:
  // Size of each field.
  static const unsigned int field_size = size / 8;

 public:
  typedef typename elfcpp::Elf_types<size>::Elf_Addr Address;
  typedef typename elfcpp::Elf_types<size>::Elf_Swxword Addend;

  // Size of each entry.
  static const unsigned int reloc_size = 8 + 2 * field_size;

  Incremental_relocs_reader(const unsigned char* p) : p_(p)
  { }

  // Return the relocation type for relocation entry at offset OFF.
  unsigned int
  get_r_type(unsigned int off) const
  {
    return elfcpp::Swap<32, big_endian>::readval(this->p_ + off);
  }

  // Return the output section index for relocation entry at offset OFF.
  unsigned int
  get_r_shndx(unsigned int off) const
  {
    return elfcpp::Swap<32, big_endian>::readval(this->p_ + off + 4);
  }

  // Return the output section offset for relocation entry at offset OFF.
  Address
  get_r_offset(unsigned int off) const
  {
    return elfcpp::Swap<size, big_endian>::readval(this->p_ + off + 8);
  }

  // Return the addend for relocation entry at offset OFF.
  Addend
  get_r_addend(unsigned int off) const
  {
    return elfcpp::Swap<size, big_endian>::readval(this->p_ + off + 8
						   + this->field_size);
  }

 private:
  // Base address of the .gnu_incremental_relocs section.
  const unsigned char* p_;
};

// Reader class for the .gnu_incremental_got_plt section.

template<bool big_endian>
class Incremental_got_plt_reader
{
 public:
  Incremental_got_plt_reader(const unsigned char* p) : p_(p)
  {
    this->got_count_ = elfcpp::Swap<32, big_endian>::readval(p);
    this->got_desc_p_ = p + 8 + ((this->got_count_ + 3) & ~3);
    this->plt_desc_p_ = this->got_desc_p_ + this->got_count_ * 4;
  }

  // Return the GOT entry count.
  unsigned int
  get_got_entry_count() const
  {
    return this->got_count_;
  }

  // Return the PLT entry count.
  unsigned int
  get_plt_entry_count() const
  {
    return elfcpp::Swap<32, big_endian>::readval(this->p_ + 4);
  }

  // Return the GOT type for GOT entry N.
  unsigned int
  get_got_type(unsigned int n)
  {
    return this->p_[8 + n];
  }

  // Return the GOT descriptor for GOT entry N.
  unsigned int
  get_got_desc(unsigned int n)
  {
    return elfcpp::Swap<32, big_endian>::readval(this->got_desc_p_ + n * 4);
  }

  // Return the PLT descriptor for PLT entry N.
  unsigned int
  get_plt_desc(unsigned int n)
  {
    return elfcpp::Swap<32, big_endian>::readval(this->plt_desc_p_ + n * 4);
  }

 private:
  // Base address of the .gnu_incremental_got_plt section.
  const unsigned char* p_;
  // GOT entry count.
  unsigned int got_count_;
  // Base address of the GOT descriptor array.
  const unsigned char* got_desc_p_;
  // Base address of the PLT descriptor array.
  const unsigned char* plt_desc_p_;
};

} // End namespace gold.

#endif // !defined(GOLD_INCREMENTAL_H)
