// object.h -- support for an object file for linking in gold  -*- C++ -*-

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

#ifndef GOLD_OBJECT_H
#define GOLD_OBJECT_H

#include <string>
#include <vector>

#include "elfcpp.h"
#include "elfcpp_file.h"
#include "fileread.h"
#include "target.h"

namespace gold
{

class General_options;
class Task;
class Cref;
class Archive;
class Layout;
class Output_section;
class Output_file;
class Output_symtab_xindex;
class Dynobj;
class Object_merge_map;
class Relocatable_relocs;

template<typename Stringpool_char>
class Stringpool_template;

// Data to pass from read_symbols() to add_symbols().

struct Read_symbols_data
{
  // Section headers.
  File_view* section_headers;
  // Section names.
  File_view* section_names;
  // Size of section name data in bytes.
  section_size_type section_names_size;
  // Symbol data.
  File_view* symbols;
  // Size of symbol data in bytes.
  section_size_type symbols_size;
  // Offset of external symbols within symbol data.  This structure
  // sometimes contains only external symbols, in which case this will
  // be zero.  Sometimes it contains all symbols.
  section_offset_type external_symbols_offset;
  // Symbol names.
  File_view* symbol_names;
  // Size of symbol name data in bytes.
  section_size_type symbol_names_size;

  // Version information.  This is only used on dynamic objects.
  // Version symbol data (from SHT_GNU_versym section).
  File_view* versym;
  section_size_type versym_size;
  // Version definition data (from SHT_GNU_verdef section).
  File_view* verdef;
  section_size_type verdef_size;
  unsigned int verdef_info;
  // Needed version data  (from SHT_GNU_verneed section).
  File_view* verneed;
  section_size_type verneed_size;
  unsigned int verneed_info;
};

// Information used to print error messages.

struct Symbol_location_info
{
  std::string source_file;
  std::string enclosing_symbol_name;
  int line_number;
};

// Data about a single relocation section.  This is read in
// read_relocs and processed in scan_relocs.

struct Section_relocs
{
  // Index of reloc section.
  unsigned int reloc_shndx;
  // Index of section that relocs apply to.
  unsigned int data_shndx;
  // Contents of reloc section.
  File_view* contents;
  // Reloc section type.
  unsigned int sh_type;
  // Number of reloc entries.
  size_t reloc_count;
  // Output section.
  Output_section* output_section;
  // Whether this section has special handling for offsets.
  bool needs_special_offset_handling;
  // Whether the data section is allocated (has the SHF_ALLOC flag set).
  bool is_data_section_allocated;
};

// Relocations in an object file.  This is read in read_relocs and
// processed in scan_relocs.

struct Read_relocs_data
{
  typedef std::vector<Section_relocs> Relocs_list;
  // The relocations.
  Relocs_list relocs;
  // The local symbols.
  File_view* local_symbols;
};

// The Xindex class manages section indexes for objects with more than
// 0xff00 sections.

class Xindex
{
 public:
  Xindex(int large_shndx_offset)
    : large_shndx_offset_(large_shndx_offset), symtab_xindex_()
  { }

  // Initialize the symtab_xindex_ array, given the object and the
  // section index of the symbol table to use.
  template<int size, bool big_endian>
  void
  initialize_symtab_xindex(Object*, unsigned int symtab_shndx);

  // Read in the symtab_xindex_ array, given its section index.
  // PSHDRS may optionally point to the section headers.
  template<int size, bool big_endian>
  void
  read_symtab_xindex(Object*, unsigned int xindex_shndx,
		     const unsigned char* pshdrs);

  // Symbol SYMNDX in OBJECT has a section of SHN_XINDEX; return the
  // real section index.
  unsigned int
  sym_xindex_to_shndx(Object* object, unsigned int symndx);

 private:
  // The type of the array giving the real section index for symbols
  // whose st_shndx field holds SHN_XINDEX.
  typedef std::vector<unsigned int> Symtab_xindex;

  // Adjust a section index if necessary.  This should only be called
  // for ordinary section indexes.
  unsigned int
  adjust_shndx(unsigned int shndx)
  {
    if (shndx >= elfcpp::SHN_LORESERVE)
      shndx += this->large_shndx_offset_;
    return shndx;
  }

  // Adjust to apply to large section indexes.
  int large_shndx_offset_;
  // The data from the SHT_SYMTAB_SHNDX section.
  Symtab_xindex symtab_xindex_;
};

// Object is an abstract base class which represents either a 32-bit
// or a 64-bit input object.  This can be a regular object file
// (ET_REL) or a shared object (ET_DYN).

class Object
{
 public:
  // NAME is the name of the object as we would report it to the user
  // (e.g., libfoo.a(bar.o) if this is in an archive.  INPUT_FILE is
  // used to read the file.  OFFSET is the offset within the input
  // file--0 for a .o or .so file, something else for a .a file.
  Object(const std::string& name, Input_file* input_file, bool is_dynamic,
	 off_t offset = 0)
    : name_(name), input_file_(input_file), offset_(offset), shnum_(-1U),
      is_dynamic_(is_dynamic), target_(NULL), xindex_(NULL)
  { input_file->file().add_object(); }

  virtual ~Object()
  { this->input_file_->file().remove_object(); }

  // Return the name of the object as we would report it to the tuser.
  const std::string&
  name() const
  { return this->name_; }

  // Get the offset into the file.
  off_t
  offset() const
  { return this->offset_; }

  // Return whether this is a dynamic object.
  bool
  is_dynamic() const
  { return this->is_dynamic_; }

  // Return the target structure associated with this object.
  Target*
  target() const
  { return this->target_; }

  // Lock the underlying file.
  void
  lock(const Task* t)
  { this->input_file()->file().lock(t); }

  // Unlock the underlying file.
  void
  unlock(const Task* t)
  { this->input_file()->file().unlock(t); }

  // Return whether the underlying file is locked.
  bool
  is_locked() const
  { return this->input_file()->file().is_locked(); }

  // Return the token, so that the task can be queued.
  Task_token*
  token()
  { return this->input_file()->file().token(); }

  // Release the underlying file.
  void
  release()
  { this->input_file_->file().release(); }

  // Return whether we should just read symbols from this file.
  bool
  just_symbols() const
  { return this->input_file()->just_symbols(); }

  // Return the sized target structure associated with this object.
  // This is like the target method but it returns a pointer of
  // appropriate checked type.
  template<int size, bool big_endian>
  Sized_target<size, big_endian>*
  sized_target() const;

  // Get the number of sections.
  unsigned int
  shnum() const
  { return this->shnum_; }

  // Return a view of the contents of a section.  Set *PLEN to the
  // size.  CACHE is a hint as in File_read::get_view.
  const unsigned char*
  section_contents(unsigned int shndx, section_size_type* plen, bool cache);

  // Adjust a symbol's section index as needed.  SYMNDX is the index
  // of the symbol and SHNDX is the symbol's section from
  // get_st_shndx.  This returns the section index.  It sets
  // *IS_ORDINARY to indicate whether this is a normal section index,
  // rather than a special code between SHN_LORESERVE and
  // SHN_HIRESERVE.
  unsigned int
  adjust_sym_shndx(unsigned int symndx, unsigned int shndx, bool* is_ordinary)
  {
    if (shndx < elfcpp::SHN_LORESERVE)
      *is_ordinary = true;
    else if (shndx == elfcpp::SHN_XINDEX)
      {
	if (this->xindex_ == NULL)
	  this->xindex_ = this->do_initialize_xindex();
	shndx = this->xindex_->sym_xindex_to_shndx(this, symndx);
	*is_ordinary = true;
      }
    else
      *is_ordinary = false;
    return shndx;
  }

  // Return the size of a section given a section index.
  uint64_t
  section_size(unsigned int shndx)
  { return this->do_section_size(shndx); }

  // Return the name of a section given a section index.
  std::string
  section_name(unsigned int shndx)
  { return this->do_section_name(shndx); }

  // Return the section flags given a section index.
  uint64_t
  section_flags(unsigned int shndx)
  { return this->do_section_flags(shndx); }

  // Return the section address given a section index.
  uint64_t
  section_address(unsigned int shndx)
  { return this->do_section_address(shndx); }

  // Return the section type given a section index.
  unsigned int
  section_type(unsigned int shndx)
  { return this->do_section_type(shndx); }

  // Return the section link field given a section index.
  unsigned int
  section_link(unsigned int shndx)
  { return this->do_section_link(shndx); }

  // Return the section info field given a section index.
  unsigned int
  section_info(unsigned int shndx)
  { return this->do_section_info(shndx); }

  // Return the required section alignment given a section index.
  uint64_t
  section_addralign(unsigned int shndx)
  { return this->do_section_addralign(shndx); }

  // Read the symbol information.
  void
  read_symbols(Read_symbols_data* sd)
  { return this->do_read_symbols(sd); }

  // Pass sections which should be included in the link to the Layout
  // object, and record where the sections go in the output file.
  void
  layout(Symbol_table* symtab, Layout* layout, Read_symbols_data* sd)
  { this->do_layout(symtab, layout, sd); }

  // Add symbol information to the global symbol table.
  void
  add_symbols(Symbol_table* symtab, Read_symbols_data* sd)
  { this->do_add_symbols(symtab, sd); }

  // Functions and types for the elfcpp::Elf_file interface.  This
  // permit us to use Object as the File template parameter for
  // elfcpp::Elf_file.

  // The View class is returned by view.  It must support a single
  // method, data().  This is trivial, because get_view does what we
  // need.
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
  { return View(this->get_view(file_offset, data_size, true, true)); }

  // Report an error.
  void
  error(const char* format, ...) const ATTRIBUTE_PRINTF_2;

  // A location in the file.
  struct Location
  {
    off_t file_offset;
    off_t data_size;

    Location(off_t fo, section_size_type ds)
      : file_offset(fo), data_size(ds)
    { }
  };

  // Get a View given a Location.
  View view(Location loc)
  { return View(this->get_view(loc.file_offset, loc.data_size, true, true)); }

  // Get a view into the underlying file.
  const unsigned char*
  get_view(off_t start, section_size_type size, bool aligned, bool cache)
  {
    return this->input_file()->file().get_view(this->offset_, start, size,
					       aligned, cache);
  }

  // Get a lasting view into the underlying file.
  File_view*
  get_lasting_view(off_t start, section_size_type size, bool aligned,
		   bool cache)
  {
    return this->input_file()->file().get_lasting_view(this->offset_, start,
						       size, aligned, cache);
  }

  // Read data from the underlying file.
  void
  read(off_t start, section_size_type size, void* p)
  { this->input_file()->file().read(start + this->offset_, size, p); }

  // Read multiple data from the underlying file.
  void
  read_multiple(const File_read::Read_multiple& rm)
  { this->input_file()->file().read_multiple(this->offset_, rm); }

  // Stop caching views in the underlying file.
  void
  clear_view_cache_marks()
  { this->input_file()->file().clear_view_cache_marks(); }

  // Get the number of global symbols defined by this object, and the
  // number of the symbols whose final definition came from this
  // object.
  void
  get_global_symbol_counts(const Symbol_table* symtab, size_t* defined,
			   size_t* used) const
  { this->do_get_global_symbol_counts(symtab, defined, used); }

 protected:
  // Read the symbols--implemented by child class.
  virtual void
  do_read_symbols(Read_symbols_data*) = 0;

  // Lay out sections--implemented by child class.
  virtual void
  do_layout(Symbol_table*, Layout*, Read_symbols_data*) = 0;

  // Add symbol information to the global symbol table--implemented by
  // child class.
  virtual void
  do_add_symbols(Symbol_table*, Read_symbols_data*) = 0;

  // Return the location of the contents of a section.  Implemented by
  // child class.
  virtual Location
  do_section_contents(unsigned int shndx) = 0;

  // Get the size of a section--implemented by child class.
  virtual uint64_t
  do_section_size(unsigned int shndx) = 0;

  // Get the name of a section--implemented by child class.
  virtual std::string
  do_section_name(unsigned int shndx) = 0;

  // Get section flags--implemented by child class.
  virtual uint64_t
  do_section_flags(unsigned int shndx) = 0;

  // Get section address--implemented by child class.
  virtual uint64_t
  do_section_address(unsigned int shndx) = 0;

  // Get section type--implemented by child class.
  virtual unsigned int
  do_section_type(unsigned int shndx) = 0;

  // Get section link field--implemented by child class.
  virtual unsigned int
  do_section_link(unsigned int shndx) = 0;

  // Get section info field--implemented by child class.
  virtual unsigned int
  do_section_info(unsigned int shndx) = 0;

  // Get section alignment--implemented by child class.
  virtual uint64_t
  do_section_addralign(unsigned int shndx) = 0;

  // Return the Xindex structure to use.
  virtual Xindex*
  do_initialize_xindex() = 0;

  // Implement get_global_symbol_counts--implemented by child class.
  virtual void
  do_get_global_symbol_counts(const Symbol_table*, size_t*, size_t*) const = 0;

  // Get the file.  We pass on const-ness.
  Input_file*
  input_file()
  { return this->input_file_; }

  const Input_file*
  input_file() const
  { return this->input_file_; }

  // Set the target.
  void
  set_target(int machine, int size, bool big_endian, int osabi,
	     int abiversion);

  // Set the number of sections.
  void
  set_shnum(int shnum)
  { this->shnum_ = shnum; }

  // Functions used by both Sized_relobj and Sized_dynobj.

  // Read the section data into a Read_symbols_data object.
  template<int size, bool big_endian>
  void
  read_section_data(elfcpp::Elf_file<size, big_endian, Object>*,
		    Read_symbols_data*);

  // Let the child class initialize the xindex object directly.
  void
  set_xindex(Xindex* xindex)
  {
    gold_assert(this->xindex_ == NULL);
    this->xindex_ = xindex;
  }

  // If NAME is the name of a special .gnu.warning section, arrange
  // for the warning to be issued.  SHNDX is the section index.
  // Return whether it is a warning section.
  bool
  handle_gnu_warning_section(const char* name, unsigned int shndx,
			     Symbol_table*);

 private:
  // This class may not be copied.
  Object(const Object&);
  Object& operator=(const Object&);

  // Name of object as printed to user.
  std::string name_;
  // For reading the file.
  Input_file* input_file_;
  // Offset within the file--0 for an object file, non-0 for an
  // archive.
  off_t offset_;
  // Number of input sections.
  unsigned int shnum_;
  // Whether this is a dynamic object.
  bool is_dynamic_;
  // Target functions--may be NULL if the target is not known.
  Target* target_;
  // Many sections for objects with more than SHN_LORESERVE sections.
  Xindex* xindex_;
};

// Implement sized_target inline for efficiency.  This approach breaks
// static type checking, but is made safe using asserts.

template<int size, bool big_endian>
inline Sized_target<size, big_endian>*
Object::sized_target() const
{
  gold_assert(this->target_->get_size() == size);
  gold_assert(this->target_->is_big_endian() ? big_endian : !big_endian);
  return static_cast<Sized_target<size, big_endian>*>(this->target_);
}

// A regular object (ET_REL).  This is an abstract base class itself.
// The implementation is the template class Sized_relobj.

class Relobj : public Object
{
 public:
  Relobj(const std::string& name, Input_file* input_file, off_t offset = 0)
    : Object(name, input_file, false, offset),
      output_sections_(),
      map_to_relocatable_relocs_(NULL),
      object_merge_map_(NULL),
      relocs_must_follow_section_writes_(false)
  { }

  // Read the relocs.
  void
  read_relocs(Read_relocs_data* rd)
  { return this->do_read_relocs(rd); }

  // Scan the relocs and adjust the symbol table.
  void
  scan_relocs(const General_options& options, Symbol_table* symtab,
	      Layout* layout, Read_relocs_data* rd)
  { return this->do_scan_relocs(options, symtab, layout, rd); }

  // The number of local symbols in the input symbol table.
  virtual unsigned int
  local_symbol_count() const
  { return this->do_local_symbol_count(); }

  // Initial local symbol processing: count the number of local symbols
  // in the output symbol table and dynamic symbol table; add local symbol
  // names to *POOL and *DYNPOOL.
  void
  count_local_symbols(Stringpool_template<char>* pool,
                      Stringpool_template<char>* dynpool)
  { return this->do_count_local_symbols(pool, dynpool); }

  // Set the values of the local symbols, set the output symbol table
  // indexes for the local variables, and set the offset where local
  // symbol information will be stored. Returns the new local symbol index.
  unsigned int
  finalize_local_symbols(unsigned int index, off_t off)
  { return this->do_finalize_local_symbols(index, off); }

  // Set the output dynamic symbol table indexes for the local variables.
  unsigned int
  set_local_dynsym_indexes(unsigned int index)
  { return this->do_set_local_dynsym_indexes(index); }

  // Set the offset where local dynamic symbol information will be stored.
  unsigned int
  set_local_dynsym_offset(off_t off)
  { return this->do_set_local_dynsym_offset(off); }

  // Relocate the input sections and write out the local symbols.
  void
  relocate(const General_options& options, const Symbol_table* symtab,
	   const Layout* layout, Output_file* of)
  { return this->do_relocate(options, symtab, layout, of); }

  // Return whether an input section is being included in the link.
  bool
  is_section_included(unsigned int shndx) const
  {
    gold_assert(shndx < this->output_sections_.size());
    return this->output_sections_[shndx] != NULL;
  }

  // Given a section index, return the corresponding Output_section.
  // The return value will be NULL if the section is not included in
  // the link.
  Output_section*
  output_section(unsigned int shndx) const
  {
    gold_assert(shndx < this->output_sections_.size());
    return this->output_sections_[shndx];
  }

  // Given a section index, return the offset in the Output_section.
  // The return value will be -1U if the section is specially mapped,
  // such as a merge section.
  uint64_t
  output_section_offset(unsigned int shndx) const
  { return this->do_output_section_offset(shndx); }

  // Set the offset of an input section within its output section.
  virtual void
  set_section_offset(unsigned int shndx, uint64_t off)
  { this->do_set_section_offset(shndx, off); }

  // Return true if we need to wait for output sections to be written
  // before we can apply relocations.  This is true if the object has
  // any relocations for sections which require special handling, such
  // as the exception frame section.
  bool
  relocs_must_follow_section_writes() const
  { return this->relocs_must_follow_section_writes_; }

  // Return the object merge map.
  Object_merge_map*
  merge_map() const
  { return this->object_merge_map_; }

  // Set the object merge map.
  void
  set_merge_map(Object_merge_map* object_merge_map)
  {
    gold_assert(this->object_merge_map_ == NULL);
    this->object_merge_map_ = object_merge_map;
  }

  // Record the relocatable reloc info for an input reloc section.
  void
  set_relocatable_relocs(unsigned int reloc_shndx, Relocatable_relocs* rr)
  {
    gold_assert(reloc_shndx < this->shnum());
    (*this->map_to_relocatable_relocs_)[reloc_shndx] = rr;
  }

  // Get the relocatable reloc info for an input reloc section.
  Relocatable_relocs*
  relocatable_relocs(unsigned int reloc_shndx)
  {
    gold_assert(reloc_shndx < this->shnum());
    return (*this->map_to_relocatable_relocs_)[reloc_shndx];
  }

 protected:
  // The output section to be used for each input section, indexed by
  // the input section number.  The output section is NULL if the
  // input section is to be discarded.
  typedef std::vector<Output_section*> Output_sections;

  // Read the relocs--implemented by child class.
  virtual void
  do_read_relocs(Read_relocs_data*) = 0;

  // Scan the relocs--implemented by child class.
  virtual void
  do_scan_relocs(const General_options&, Symbol_table*, Layout*,
		 Read_relocs_data*) = 0;

  // Return the number of local symbols--implemented by child class.
  virtual unsigned int
  do_local_symbol_count() const = 0;

  // Count local symbols--implemented by child class.
  virtual void
  do_count_local_symbols(Stringpool_template<char>*,
			 Stringpool_template<char>*) = 0;

  // Finalize the local symbols.  Set the output symbol table indexes
  // for the local variables, and set the offset where local symbol
  // information will be stored.
  virtual unsigned int
  do_finalize_local_symbols(unsigned int, off_t) = 0;

  // Set the output dynamic symbol table indexes for the local variables.
  virtual unsigned int
  do_set_local_dynsym_indexes(unsigned int) = 0;

  // Set the offset where local dynamic symbol information will be stored.
  virtual unsigned int
  do_set_local_dynsym_offset(off_t) = 0;

  // Relocate the input sections and write out the local
  // symbols--implemented by child class.
  virtual void
  do_relocate(const General_options& options, const Symbol_table* symtab,
	      const Layout*, Output_file* of) = 0;

  // Get the offset of a section--implemented by child class.
  virtual uint64_t
  do_output_section_offset(unsigned int shndx) const = 0;

  // Set the offset of a section--implemented by child class.
  virtual void
  do_set_section_offset(unsigned int shndx, uint64_t off) = 0;

  // Return the vector mapping input sections to output sections.
  Output_sections&
  output_sections()
  { return this->output_sections_; }

  const Output_sections&
  output_sections() const
  { return this->output_sections_; }

  // Set the size of the relocatable relocs array.
  void
  size_relocatable_relocs()
  {
    this->map_to_relocatable_relocs_ =
      new std::vector<Relocatable_relocs*>(this->shnum());
  }

  // Record that we must wait for the output sections to be written
  // before applying relocations.
  void
  set_relocs_must_follow_section_writes()
  { this->relocs_must_follow_section_writes_ = true; }

 private:
  // Mapping from input sections to output section.
  Output_sections output_sections_;
  // Mapping from input section index to the information recorded for
  // the relocations.  This is only used for a relocatable link.
  std::vector<Relocatable_relocs*>* map_to_relocatable_relocs_;
  // Mappings for merge sections.  This is managed by the code in the
  // Merge_map class.
  Object_merge_map* object_merge_map_;
  // Whether we need to wait for output sections to be written before
  // we can apply relocations.
  bool relocs_must_follow_section_writes_;
};

// This class is used to handle relocations against a section symbol
// in an SHF_MERGE section.  For such a symbol, we need to know the
// addend of the relocation before we can determine the final value.
// The addend gives us the location in the input section, and we can
// determine how it is mapped to the output section.  For a
// non-section symbol, we apply the addend to the final value of the
// symbol; that is done in finalize_local_symbols, and does not use
// this class.

template<int size>
class Merged_symbol_value
{
 public:
  typedef typename elfcpp::Elf_types<size>::Elf_Addr Value;

  // We use a hash table to map offsets in the input section to output
  // addresses.
  typedef Unordered_map<section_offset_type, Value> Output_addresses;

  Merged_symbol_value(Value input_value, Value output_start_address)
    : input_value_(input_value), output_start_address_(output_start_address),
      output_addresses_()
  { }

  // Initialize the hash table.
  void
  initialize_input_to_output_map(const Relobj*, unsigned int input_shndx);

  // Release the hash table to save space.
  void
  free_input_to_output_map()
  { this->output_addresses_.clear(); }

  // Get the output value corresponding to an addend.  The object and
  // input section index are passed in because the caller will have
  // them; otherwise we could store them here.
  Value
  value(const Relobj* object, unsigned int input_shndx, Value addend) const
  {
    // This is a relocation against a section symbol.  ADDEND is the
    // offset in the section.  The result should be the start of some
    // merge area.  If the object file wants something else, it should
    // use a regular symbol rather than a section symbol.
    // Unfortunately, PR 6658 shows a case in which the object file
    // refers to the section symbol, but uses a negative ADDEND to
    // compensate for a PC relative reloc.  We can't handle the
    // general case.  However, we can handle the special case of a
    // negative addend, by assuming that it refers to the start of the
    // section.  Of course, that means that we have to guess when
    // ADDEND is negative.  It is normal to see a 32-bit value here
    // even when the template parameter size is 64, as 64-bit object
    // file formats have 32-bit relocations.  We know this is a merge
    // section, so we know it has to fit into memory.  So we assume
    // that we won't see a value larger than a large 32-bit unsigned
    // value.  This will break objects with very very large merge
    // sections; they probably break in other ways anyhow.
    Value input_offset = this->input_value_;
    if (addend < 0xffffff00)
      {
	input_offset += addend;
	addend = 0;
      }
    typename Output_addresses::const_iterator p =
      this->output_addresses_.find(input_offset);
    if (p != this->output_addresses_.end())
      return p->second + addend;

    return (this->value_from_output_section(object, input_shndx, input_offset)
	    + addend);
  }

 private:
  // Get the output value for an input offset if we couldn't find it
  // in the hash table.
  Value
  value_from_output_section(const Relobj*, unsigned int input_shndx,
			    Value input_offset) const;

  // The value of the section symbol in the input file.  This is
  // normally zero, but could in principle be something else.
  Value input_value_;
  // The start address of this merged section in the output file.
  Value output_start_address_;
  // A hash table which maps offsets in the input section to output
  // addresses.  This only maps specific offsets, not all offsets.
  Output_addresses output_addresses_;
};

// This POD class is holds the value of a symbol.  This is used for
// local symbols, and for all symbols during relocation processing.
// For special sections, such as SHF_MERGE sections, this calls a
// function to get the final symbol value.

template<int size>
class Symbol_value
{
 public:
  typedef typename elfcpp::Elf_types<size>::Elf_Addr Value;

  Symbol_value()
    : output_symtab_index_(0), output_dynsym_index_(-1U), input_shndx_(0),
      is_ordinary_shndx_(false), is_section_symbol_(false),
      is_tls_symbol_(false), has_output_value_(true)
  { this->u_.value = 0; }

  // Get the value of this symbol.  OBJECT is the object in which this
  // symbol is defined, and ADDEND is an addend to add to the value.
  template<bool big_endian>
  Value
  value(const Sized_relobj<size, big_endian>* object, Value addend) const
  {
    if (this->has_output_value_)
      return this->u_.value + addend;
    else
      {
	gold_assert(this->is_ordinary_shndx_);
	return this->u_.merged_symbol_value->value(object, this->input_shndx_,
						   addend);
      }
  }

  // Set the value of this symbol in the output symbol table.
  void
  set_output_value(Value value)
  { this->u_.value = value; }

  // For a section symbol in a merged section, we need more
  // information.
  void
  set_merged_symbol_value(Merged_symbol_value<size>* msv)
  {
    gold_assert(this->is_section_symbol_);
    this->has_output_value_ = false;
    this->u_.merged_symbol_value = msv;
  }

  // Initialize the input to output map for a section symbol in a
  // merged section.  We also initialize the value of a non-section
  // symbol in a merged section.
  void
  initialize_input_to_output_map(const Relobj* object)
  {
    if (!this->has_output_value_)
      {
	gold_assert(this->is_section_symbol_ && this->is_ordinary_shndx_);
	Merged_symbol_value<size>* msv = this->u_.merged_symbol_value;
	msv->initialize_input_to_output_map(object, this->input_shndx_);
      }
  }

  // Free the input to output map for a section symbol in a merged
  // section.
  void
  free_input_to_output_map()
  {
    if (!this->has_output_value_)
      this->u_.merged_symbol_value->free_input_to_output_map();
  }

  // Set the value of the symbol from the input file.  This is only
  // called by count_local_symbols, to communicate the value to
  // finalize_local_symbols.
  void
  set_input_value(Value value)
  { this->u_.value = value; }

  // Return the input value.  This is only called by
  // finalize_local_symbols and (in special cases) relocate_section.
  Value
  input_value() const
  { return this->u_.value; }

  // Return whether this symbol should go into the output symbol
  // table.
  bool
  needs_output_symtab_entry() const
  { return this->output_symtab_index_ != -1U; }

  // Return the index in the output symbol table.
  unsigned int
  output_symtab_index() const
  {
    gold_assert(this->output_symtab_index_ != 0);
    return this->output_symtab_index_;
  }

  // Set the index in the output symbol table.
  void
  set_output_symtab_index(unsigned int i)
  {
    gold_assert(this->output_symtab_index_ == 0);
    this->output_symtab_index_ = i;
  }

  // Record that this symbol should not go into the output symbol
  // table.
  void
  set_no_output_symtab_entry()
  {
    gold_assert(this->output_symtab_index_ == 0);
    this->output_symtab_index_ = -1U;
  }

  // Set the index in the output dynamic symbol table.
  void
  set_needs_output_dynsym_entry()
  {
    gold_assert(!this->is_section_symbol());
    this->output_dynsym_index_ = 0;
  }

  // Return whether this symbol should go into the output symbol
  // table.
  bool
  needs_output_dynsym_entry() const
  {
    return this->output_dynsym_index_ != -1U;
  }

  // Record that this symbol should go into the dynamic symbol table.
  void
  set_output_dynsym_index(unsigned int i)
  {
    gold_assert(this->output_dynsym_index_ == 0);
    this->output_dynsym_index_ = i;
  }

  // Return the index in the output dynamic symbol table.
  unsigned int
  output_dynsym_index() const
  {
    gold_assert(this->output_dynsym_index_ != 0
                && this->output_dynsym_index_ != -1U);
    return this->output_dynsym_index_;
  }

  // Set the index of the input section in the input file.
  void
  set_input_shndx(unsigned int i, bool is_ordinary)
  {
    this->input_shndx_ = i;
    // input_shndx_ field is a bitfield, so make sure that the value
    // fits.
    gold_assert(this->input_shndx_ == i);
    this->is_ordinary_shndx_ = is_ordinary;
  }

  // Return the index of the input section in the input file.
  unsigned int
  input_shndx(bool* is_ordinary) const
  {
    *is_ordinary = this->is_ordinary_shndx_;
    return this->input_shndx_;
  }

  // Whether this is a section symbol.
  bool
  is_section_symbol() const
  { return this->is_section_symbol_; }

  // Record that this is a section symbol.
  void
  set_is_section_symbol()
  {
    gold_assert(!this->needs_output_dynsym_entry());
    this->is_section_symbol_ = true;
  }

  // Record that this is a TLS symbol.
  void
  set_is_tls_symbol()
  { this->is_tls_symbol_ = true; }

  // Return TRUE if this is a TLS symbol.
  bool
  is_tls_symbol() const
  { return this->is_tls_symbol_; }

 private:
  // The index of this local symbol in the output symbol table.  This
  // will be -1 if the symbol should not go into the symbol table.
  unsigned int output_symtab_index_;
  // The index of this local symbol in the dynamic symbol table.  This
  // will be -1 if the symbol should not go into the symbol table.
  unsigned int output_dynsym_index_;
  // The section index in the input file in which this symbol is
  // defined.
  unsigned int input_shndx_ : 28;
  // Whether the section index is an ordinary index, not a special
  // value.
  bool is_ordinary_shndx_ : 1;
  // Whether this is a STT_SECTION symbol.
  bool is_section_symbol_ : 1;
  // Whether this is a STT_TLS symbol.
  bool is_tls_symbol_ : 1;
  // Whether this symbol has a value for the output file.  This is
  // normally set to true during Layout::finalize, by
  // finalize_local_symbols.  It will be false for a section symbol in
  // a merge section, as for such symbols we can not determine the
  // value to use in a relocation until we see the addend.
  bool has_output_value_ : 1;
  union
  {
    // This is used if has_output_value_ is true.  Between
    // count_local_symbols and finalize_local_symbols, this is the
    // value in the input file.  After finalize_local_symbols, it is
    // the value in the output file.
    Value value;
    // This is used if has_output_value_ is false.  It points to the
    // information we need to get the value for a merge section.
    Merged_symbol_value<size>* merged_symbol_value;
  } u_;
};

// A GOT offset list.  A symbol may have more than one GOT offset
// (e.g., when mixing modules compiled with two different TLS models),
// but will usually have at most one.  GOT_TYPE identifies the type of
// GOT entry; its values are specific to each target.

class Got_offset_list
{
 public:
  Got_offset_list()
    : got_type_(-1U), got_offset_(0), got_next_(NULL)
  { }

  Got_offset_list(unsigned int got_type, unsigned int got_offset)
    : got_type_(got_type), got_offset_(got_offset), got_next_(NULL)
  { }

  ~Got_offset_list()
  { 
    if (this->got_next_ != NULL)
      {
        delete this->got_next_;
        this->got_next_ = NULL;
      }
  }

  // Initialize the fields to their default values.
  void
  init()
  {
    this->got_type_ = -1U;
    this->got_offset_ = 0;
    this->got_next_ = NULL;
  }

  // Set the offset for the GOT entry of type GOT_TYPE.
  void
  set_offset(unsigned int got_type, unsigned int got_offset)
  {
    if (this->got_type_ == -1U)
      {
        this->got_type_ = got_type;
        this->got_offset_ = got_offset;
      }
    else
      {
        for (Got_offset_list* g = this; g != NULL; g = g->got_next_)
          {
            if (g->got_type_ == got_type)
              {
                g->got_offset_ = got_offset;
                return;
              }
          }
        Got_offset_list* g = new Got_offset_list(got_type, got_offset);
        g->got_next_ = this->got_next_;
        this->got_next_ = g;
      }
  }

  // Return the offset for a GOT entry of type GOT_TYPE.
  unsigned int
  get_offset(unsigned int got_type) const
  {
    for (const Got_offset_list* g = this; g != NULL; g = g->got_next_)
      {
        if (g->got_type_ == got_type)
          return g->got_offset_;
      }
    return -1U;
  }

 private:
  unsigned int got_type_;
  unsigned int got_offset_;
  Got_offset_list* got_next_;
};

// A regular object file.  This is size and endian specific.

template<int size, bool big_endian>
class Sized_relobj : public Relobj
{
 public:
  typedef typename elfcpp::Elf_types<size>::Elf_Addr Address;
  typedef std::vector<Symbol*> Symbols;
  typedef std::vector<Symbol_value<size> > Local_values;

  Sized_relobj(const std::string& name, Input_file* input_file, off_t offset,
	       const typename elfcpp::Ehdr<size, big_endian>&);

  ~Sized_relobj();

  // Set up the object file based on the ELF header.
  void
  setup(const typename elfcpp::Ehdr<size, big_endian>&);

  // Return the number of symbols.  This is only valid after
  // Object::add_symbols has been called.
  unsigned int
  symbol_count() const
  { return this->local_symbol_count_ + this->symbols_.size(); }

  // If SYM is the index of a global symbol in the object file's
  // symbol table, return the Symbol object.  Otherwise, return NULL.
  Symbol*
  global_symbol(unsigned int sym) const
  {
    if (sym >= this->local_symbol_count_)
      {
	gold_assert(sym - this->local_symbol_count_ < this->symbols_.size());
	return this->symbols_[sym - this->local_symbol_count_];
      }
    return NULL;
  }

  // Return the section index of symbol SYM.  Set *VALUE to its value
  // in the object file.  Set *IS_ORDINARY if this is an ordinary
  // section index, not a special code between SHN_LORESERVE and
  // SHN_HIRESERVE.  Note that for a symbol which is not defined in
  // this object file, this will set *VALUE to 0 and return SHN_UNDEF;
  // it will not return the final value of the symbol in the link.
  unsigned int
  symbol_section_and_value(unsigned int sym, Address* value, bool* is_ordinary);

  // Return a pointer to the Symbol_value structure which holds the
  // value of a local symbol.
  const Symbol_value<size>*
  local_symbol(unsigned int sym) const
  {
    gold_assert(sym < this->local_values_.size());
    return &this->local_values_[sym];
  }

  // Return the index of local symbol SYM in the ordinary symbol
  // table.  A value of -1U means that the symbol is not being output.
  unsigned int
  symtab_index(unsigned int sym) const
  {
    gold_assert(sym < this->local_values_.size());
    return this->local_values_[sym].output_symtab_index();
  }

  // Return the index of local symbol SYM in the dynamic symbol
  // table.  A value of -1U means that the symbol is not being output.
  unsigned int
  dynsym_index(unsigned int sym) const
  {
    gold_assert(sym < this->local_values_.size());
    return this->local_values_[sym].output_dynsym_index();
  }

  // Return the input section index of local symbol SYM.
  unsigned int
  local_symbol_input_shndx(unsigned int sym, bool* is_ordinary) const
  {
    gold_assert(sym < this->local_values_.size());
    return this->local_values_[sym].input_shndx(is_ordinary);
  }

  // Return the appropriate Sized_target structure.
  Sized_target<size, big_endian>*
  sized_target()
  { return this->Object::sized_target<size, big_endian>(); }

  // Record that local symbol SYM needs a dynamic symbol entry.
  void
  set_needs_output_dynsym_entry(unsigned int sym)
  {
    gold_assert(sym < this->local_values_.size());
    this->local_values_[sym].set_needs_output_dynsym_entry();
  }

  // Return whether the local symbol SYMNDX has a GOT offset.
  // For TLS symbols, the GOT entry will hold its tp-relative offset.
  bool
  local_has_got_offset(unsigned int symndx, unsigned int got_type) const
  {
    Local_got_offsets::const_iterator p =
        this->local_got_offsets_.find(symndx);
    return (p != this->local_got_offsets_.end()
            && p->second->get_offset(got_type) != -1U);
  }

  // Return the GOT offset of the local symbol SYMNDX.
  unsigned int
  local_got_offset(unsigned int symndx, unsigned int got_type) const
  {
    Local_got_offsets::const_iterator p =
        this->local_got_offsets_.find(symndx);
    gold_assert(p != this->local_got_offsets_.end());
    unsigned int off = p->second->get_offset(got_type);
    gold_assert(off != -1U);
    return off;
  }

  // Set the GOT offset of the local symbol SYMNDX to GOT_OFFSET.
  void
  set_local_got_offset(unsigned int symndx, unsigned int got_type,
                       unsigned int got_offset)
  {
    Local_got_offsets::const_iterator p =
        this->local_got_offsets_.find(symndx);
    if (p != this->local_got_offsets_.end())
      p->second->set_offset(got_type, got_offset);
    else
      {
        Got_offset_list* g = new Got_offset_list(got_type, got_offset);
        std::pair<Local_got_offsets::iterator, bool> ins =
            this->local_got_offsets_.insert(std::make_pair(symndx, g));
        gold_assert(ins.second);
      }
  }

  // Get the offset of input section SHNDX within its output section.
  // This is -1 if the input section requires a special mapping, such
  // as a merge section.  The output section can be found in the
  // output_sections_ field of the parent class Relobj.
  Address
  get_output_section_offset(unsigned int shndx) const
  {
    gold_assert(shndx < this->section_offsets_.size());
    return this->section_offsets_[shndx];
  }

  // Return the name of the symbol that spans the given offset in the
  // specified section in this object.  This is used only for error
  // messages and is not particularly efficient.
  bool
  get_symbol_location_info(unsigned int shndx, off_t offset,
			   Symbol_location_info* info);

  // Look for a kept section corresponding to the given discarded section,
  // and return its output address.  This is used only for relocations in
  // debugging sections.
  Address
  map_to_kept_section(unsigned int shndx, bool* found) const;

 protected:
  // Read the symbols.
  void
  do_read_symbols(Read_symbols_data*);

  // Return the number of local symbols.
  unsigned int
  do_local_symbol_count() const
  { return this->local_symbol_count_; }

  // Lay out the input sections.
  void
  do_layout(Symbol_table*, Layout*, Read_symbols_data*);

  // Add the symbols to the symbol table.
  void
  do_add_symbols(Symbol_table*, Read_symbols_data*);

  // Read the relocs.
  void
  do_read_relocs(Read_relocs_data*);

  // Scan the relocs and adjust the symbol table.
  void
  do_scan_relocs(const General_options&, Symbol_table*, Layout*,
		 Read_relocs_data*);

  // Count the local symbols.
  void
  do_count_local_symbols(Stringpool_template<char>*,
                            Stringpool_template<char>*);

  // Finalize the local symbols.
  unsigned int
  do_finalize_local_symbols(unsigned int, off_t);

  // Set the offset where local dynamic symbol information will be stored.
  unsigned int
  do_set_local_dynsym_indexes(unsigned int);

  // Set the offset where local dynamic symbol information will be stored.
  unsigned int
  do_set_local_dynsym_offset(off_t);

  // Relocate the input sections and write out the local symbols.
  void
  do_relocate(const General_options& options, const Symbol_table* symtab,
	      const Layout*, Output_file* of);

  // Get the size of a section.
  uint64_t
  do_section_size(unsigned int shndx)
  { return this->elf_file_.section_size(shndx); }

  // Get the name of a section.
  std::string
  do_section_name(unsigned int shndx)
  { return this->elf_file_.section_name(shndx); }

  // Return the location of the contents of a section.
  Object::Location
  do_section_contents(unsigned int shndx)
  { return this->elf_file_.section_contents(shndx); }

  // Return section flags.
  uint64_t
  do_section_flags(unsigned int shndx)
  { return this->elf_file_.section_flags(shndx); }

  // Return section address.
  uint64_t
  do_section_address(unsigned int shndx)
  { return this->elf_file_.section_addr(shndx); }

  // Return section type.
  unsigned int
  do_section_type(unsigned int shndx)
  { return this->elf_file_.section_type(shndx); }

  // Return the section link field.
  unsigned int
  do_section_link(unsigned int shndx)
  { return this->elf_file_.section_link(shndx); }

  // Return the section info field.
  unsigned int
  do_section_info(unsigned int shndx)
  { return this->elf_file_.section_info(shndx); }

  // Return the section alignment.
  uint64_t
  do_section_addralign(unsigned int shndx)
  { return this->elf_file_.section_addralign(shndx); }

  // Return the Xindex structure to use.
  Xindex*
  do_initialize_xindex();

  // Get symbol counts.
  void
  do_get_global_symbol_counts(const Symbol_table*, size_t*, size_t*) const;

  // Get the offset of a section.
  uint64_t
  do_output_section_offset(unsigned int shndx) const
  { return this->get_output_section_offset(shndx); }

  // Set the offset of a section.
  void
  do_set_section_offset(unsigned int shndx, uint64_t off)
  {
    gold_assert(shndx < this->section_offsets_.size());
    this->section_offsets_[shndx] = convert_types<Address, uint64_t>(off);
  }

 private:
  // For convenience.
  typedef Sized_relobj<size, big_endian> This;
  static const int ehdr_size = elfcpp::Elf_sizes<size>::ehdr_size;
  static const int shdr_size = elfcpp::Elf_sizes<size>::shdr_size;
  static const int sym_size = elfcpp::Elf_sizes<size>::sym_size;
  typedef elfcpp::Shdr<size, big_endian> Shdr;

  // To keep track of discarded comdat sections, we need to map a member
  // section index to the object and section index of the corresponding
  // kept section.
  struct Kept_comdat_section
  {
    Kept_comdat_section(Sized_relobj<size, big_endian>* object,
                        unsigned int shndx)
      : object_(object), shndx_(shndx)
    { }
    Sized_relobj<size, big_endian>* object_;
    unsigned int shndx_;
  };
  typedef std::map<unsigned int, Kept_comdat_section*>
      Kept_comdat_section_table;

  // Information needed to keep track of kept comdat groups.  This is
  // simply a map from the section name to its section index.  This may
  // not be a one-to-one mapping, but we ignore that possibility since
  // this is used only to attempt to handle stray relocations from
  // non-comdat debug sections that refer to comdat loadable sections.
  typedef Unordered_map<std::string, unsigned int> Comdat_group;

  // A map from group section index to the table of group members.
  typedef std::map<unsigned int, Comdat_group*> Comdat_group_table;

  // Find a comdat group table given its group section SHNDX.
  Comdat_group*
  find_comdat_group(unsigned int shndx) const
  {
    Comdat_group_table::const_iterator p =
      this->comdat_groups_.find(shndx);
    if (p != this->comdat_groups_.end())
      return p->second;
    return NULL;
  }

  // Record a new comdat group whose group section index is SHNDX.
  void
  add_comdat_group(unsigned int shndx, Comdat_group* group)
  { this->comdat_groups_[shndx] = group; }

  // Adjust a section index if necessary.
  unsigned int
  adjust_shndx(unsigned int shndx)
  {
    if (shndx >= elfcpp::SHN_LORESERVE)
      shndx += this->elf_file_.large_shndx_offset();
    return shndx;
  }

  // Find the SHT_SYMTAB section, given the section headers.
  void
  find_symtab(const unsigned char* pshdrs);

  // Return whether SHDR has the right flags for a GNU style exception
  // frame section.
  bool
  check_eh_frame_flags(const elfcpp::Shdr<size, big_endian>* shdr) const;

  // Return whether there is a section named .eh_frame which might be
  // a GNU style exception frame section.
  bool
  find_eh_frame(const unsigned char* pshdrs, const char* names,
		section_size_type names_size) const;

  // Whether to include a section group in the link.
  bool
  include_section_group(Symbol_table*, Layout*, unsigned int, const char*,
			const unsigned char*, const char *, section_size_type,
			std::vector<bool>*);

  // Whether to include a linkonce section in the link.
  bool
  include_linkonce_section(Layout*, unsigned int, const char*,
			   const elfcpp::Shdr<size, big_endian>&);

  // Views and sizes when relocating.
  struct View_size
  {
    unsigned char* view;
    typename elfcpp::Elf_types<size>::Elf_Addr address;
    off_t offset;
    section_size_type view_size;
    bool is_input_output_view;
    bool is_postprocessing_view;
  };

  typedef std::vector<View_size> Views;

  // Write section data to the output file.  Record the views and
  // sizes in VIEWS for use when relocating.
  void
  write_sections(const unsigned char* pshdrs, Output_file*, Views*);

  // Relocate the sections in the output file.
  void
  relocate_sections(const General_options& options, const Symbol_table*,
		    const Layout*, const unsigned char* pshdrs, Views*);

  // Scan the input relocations for --emit-relocs.
  void
  emit_relocs_scan(const General_options&, Symbol_table*, Layout*,
		   const unsigned char* plocal_syms,
		   const Read_relocs_data::Relocs_list::iterator&);

  // Scan the input relocations for --emit-relocs, templatized on the
  // type of the relocation section.
  template<int sh_type>
  void
  emit_relocs_scan_reltype(const General_options&, Symbol_table*, Layout*,
			   const unsigned char* plocal_syms,
			   const Read_relocs_data::Relocs_list::iterator&,
			   Relocatable_relocs*);

  // Emit the relocs for --emit-relocs.
  void
  emit_relocs(const Relocate_info<size, big_endian>*, unsigned int,
	      unsigned int sh_type, const unsigned char* prelocs,
	      size_t reloc_count, Output_section*, Address output_offset,
	      unsigned char* view, Address address,
	      section_size_type view_size,
	      unsigned char* reloc_view, section_size_type reloc_view_size);

  // Emit the relocs for --emit-relocs, templatized on the type of the
  // relocation section.
  template<int sh_type>
  void
  emit_relocs_reltype(const Relocate_info<size, big_endian>*, unsigned int,
		      const unsigned char* prelocs, size_t reloc_count,
		      Output_section*, Address output_offset,
		      unsigned char* view, Address address,
		      section_size_type view_size,
		      unsigned char* reloc_view,
		      section_size_type reloc_view_size);

  // Initialize input to output maps for section symbols in merged
  // sections.
  void
  initialize_input_to_output_maps();

  // Free the input to output maps for section symbols in merged
  // sections.
  void
  free_input_to_output_maps();

  // Write out the local symbols.
  void
  write_local_symbols(Output_file*,
		      const Stringpool_template<char>*,
		      const Stringpool_template<char>*,
		      Output_symtab_xindex*,
		      Output_symtab_xindex*);

  // Clear the local symbol information.
  void
  clear_local_symbols()
  {
    this->local_values_.clear();
    this->local_got_offsets_.clear();
  }

  // Record a mapping from discarded section SHNDX to the corresponding
  // kept section.
  void
  set_kept_comdat_section(unsigned int shndx, Kept_comdat_section* kept)
  {
    this->kept_comdat_sections_[shndx] = kept;
  }

  // Find the kept section corresponding to the discarded section SHNDX.
  Kept_comdat_section*
  get_kept_comdat_section(unsigned int shndx) const
  {
    typename Kept_comdat_section_table::const_iterator p =
      this->kept_comdat_sections_.find(shndx);
    if (p == this->kept_comdat_sections_.end())
      return NULL;
    return p->second;
  }

  // The GOT offsets of local symbols. This map also stores GOT offsets
  // for tp-relative offsets for TLS symbols.
  typedef Unordered_map<unsigned int, Got_offset_list*> Local_got_offsets;

  // The TLS GOT offsets of local symbols. The map stores the offsets
  // for either a single GOT entry that holds the module index of a TLS
  // symbol, or a pair of GOT entries containing the module index and
  // dtv-relative offset.
  struct Tls_got_entry
  {
    Tls_got_entry(int got_offset, bool have_pair)
      : got_offset_(got_offset),
        have_pair_(have_pair)
    { }
    int got_offset_;
    bool have_pair_;
  };
  typedef Unordered_map<unsigned int, Tls_got_entry> Local_tls_got_offsets;

  // General access to the ELF file.
  elfcpp::Elf_file<size, big_endian, Object> elf_file_;
  // Index of SHT_SYMTAB section.
  unsigned int symtab_shndx_;
  // The number of local symbols.
  unsigned int local_symbol_count_;
  // The number of local symbols which go into the output file.
  unsigned int output_local_symbol_count_;
  // The number of local symbols which go into the output file's dynamic
  // symbol table.
  unsigned int output_local_dynsym_count_;
  // The entries in the symbol table for the external symbols.
  Symbols symbols_;
  // Number of symbols defined in object file itself.
  size_t defined_count_;
  // File offset for local symbols.
  off_t local_symbol_offset_;
  // File offset for local dynamic symbols.
  off_t local_dynsym_offset_;
  // Values of local symbols.
  Local_values local_values_;
  // GOT offsets for local non-TLS symbols, and tp-relative offsets
  // for TLS symbols, indexed by symbol number.
  Local_got_offsets local_got_offsets_;
  // For each input section, the offset of the input section in its
  // output section.  This is -1U if the input section requires a
  // special mapping.
  std::vector<Address> section_offsets_;
  // Table mapping discarded comdat sections to corresponding kept sections.
  Kept_comdat_section_table kept_comdat_sections_;
  // Table of kept comdat groups.
  Comdat_group_table comdat_groups_;
  // Whether this object has a GNU style .eh_frame section.
  bool has_eh_frame_;
};

// A class to manage the list of all objects.

class Input_objects
{
 public:
  Input_objects()
    : relobj_list_(), dynobj_list_(), sonames_(), system_library_directory_(),
      cref_(NULL)
  { }

  // The type of the list of input relocateable objects.
  typedef std::vector<Relobj*> Relobj_list;
  typedef Relobj_list::const_iterator Relobj_iterator;

  // The type of the list of input dynamic objects.
  typedef std::vector<Dynobj*> Dynobj_list;
  typedef Dynobj_list::const_iterator Dynobj_iterator;

  // Add an object to the list.  Return true if all is well, or false
  // if this object should be ignored.
  bool
  add_object(Object*);

  // Start processing an archive.
  void
  archive_start(Archive*);

  // Stop processing an archive.
  void
  archive_stop(Archive*);

  // For each dynamic object, check whether we've seen all of its
  // explicit dependencies.
  void
  check_dynamic_dependencies() const;

  // Return whether an object was found in the system library
  // directory.
  bool
  found_in_system_library_directory(const Object*) const;

  // Print symbol counts.
  void
  print_symbol_counts(const Symbol_table*) const;

  // Iterate over all regular objects.

  Relobj_iterator
  relobj_begin() const
  { return this->relobj_list_.begin(); }

  Relobj_iterator
  relobj_end() const
  { return this->relobj_list_.end(); }

  // Iterate over all dynamic objects.

  Dynobj_iterator
  dynobj_begin() const
  { return this->dynobj_list_.begin(); }

  Dynobj_iterator
  dynobj_end() const
  { return this->dynobj_list_.end(); }

  // Return whether we have seen any dynamic objects.
  bool
  any_dynamic() const
  { return !this->dynobj_list_.empty(); }

  // Return the number of input objects.
  int
  number_of_input_objects() const
  { return this->relobj_list_.size() + this->dynobj_list_.size(); }

 private:
  Input_objects(const Input_objects&);
  Input_objects& operator=(const Input_objects&);

  // The list of ordinary objects included in the link.
  Relobj_list relobj_list_;
  // The list of dynamic objects included in the link.
  Dynobj_list dynobj_list_;
  // SONAMEs that we have seen.
  Unordered_set<std::string> sonames_;
  // The directory in which we find the libc.so.
  std::string system_library_directory_;
  // Manage cross-references if requested.
  Cref* cref_;
};

// Some of the information we pass to the relocation routines.  We
// group this together to avoid passing a dozen different arguments.

template<int size, bool big_endian>
struct Relocate_info
{
  // Command line options.
  const General_options* options;
  // Symbol table.
  const Symbol_table* symtab;
  // Layout.
  const Layout* layout;
  // Object being relocated.
  Sized_relobj<size, big_endian>* object;
  // Section index of relocation section.
  unsigned int reloc_shndx;
  // Section index of section being relocated.
  unsigned int data_shndx;

  // Return a string showing the location of a relocation.  This is
  // only used for error messages.
  std::string
  location(size_t relnum, off_t reloffset) const;
};

// Return an Object appropriate for the input file.  P is BYTES long,
// and holds the ELF header.

extern Object*
make_elf_object(const std::string& name, Input_file*,
		off_t offset, const unsigned char* p,
		section_offset_type bytes);

} // end namespace gold

#endif // !defined(GOLD_OBJECT_H)
