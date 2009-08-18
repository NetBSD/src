// layout.h -- lay out output file sections for gold  -*- C++ -*-

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

#ifndef GOLD_LAYOUT_H
#define GOLD_LAYOUT_H

#include <cstring>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "script.h"
#include "workqueue.h"
#include "object.h"
#include "dynobj.h"
#include "stringpool.h"

namespace gold
{

class General_options;
class Input_objects;
class Mapfile;
class Symbol_table;
class Output_section_data;
class Output_section;
class Output_section_headers;
class Output_segment;
class Output_data;
class Output_data_dynamic;
class Output_symtab_xindex;
class Output_reduced_debug_abbrev_section;
class Output_reduced_debug_info_section;
class Eh_frame;
class Target;

// This task function handles mapping the input sections to output
// sections and laying them out in memory.

class Layout_task_runner : public Task_function_runner
{
 public:
  // OPTIONS is the command line options, INPUT_OBJECTS is the list of
  // input objects, SYMTAB is the symbol table, LAYOUT is the layout
  // object.
  Layout_task_runner(const General_options& options,
		     const Input_objects* input_objects,
		     Symbol_table* symtab,
                     Target* target,
		     Layout* layout,
		     Mapfile* mapfile)
    : options_(options), input_objects_(input_objects), symtab_(symtab),
      target_(target), layout_(layout), mapfile_(mapfile)
  { }

  // Run the operation.
  void
  run(Workqueue*, const Task*);

 private:
  Layout_task_runner(const Layout_task_runner&);
  Layout_task_runner& operator=(const Layout_task_runner&);

  const General_options& options_;
  const Input_objects* input_objects_;
  Symbol_table* symtab_;
  Target* target_;
  Layout* layout_;
  Mapfile* mapfile_;
};

// This class handles the details of laying out input sections.

class Layout
{
 public:
  Layout(const General_options& options, Script_options*);

  // Given an input section SHNDX, named NAME, with data in SHDR, from
  // the object file OBJECT, return the output section where this
  // input section should go.  RELOC_SHNDX is the index of a
  // relocation section which applies to this section, or 0 if none,
  // or -1U if more than one.  RELOC_TYPE is the type of the
  // relocation section if there is one.  Set *OFFSET to the offset
  // within the output section.
  template<int size, bool big_endian>
  Output_section*
  layout(Sized_relobj<size, big_endian> *object, unsigned int shndx,
	 const char* name, const elfcpp::Shdr<size, big_endian>& shdr,
	 unsigned int reloc_shndx, unsigned int reloc_type, off_t* offset);

  // Layout an input reloc section when doing a relocatable link.  The
  // section is RELOC_SHNDX in OBJECT, with data in SHDR.
  // DATA_SECTION is the reloc section to which it refers.  RR is the
  // relocatable information.
  template<int size, bool big_endian>
  Output_section*
  layout_reloc(Sized_relobj<size, big_endian>* object,
	       unsigned int reloc_shndx,
	       const elfcpp::Shdr<size, big_endian>& shdr,
	       Output_section* data_section,
	       Relocatable_relocs* rr);

  // Layout a group section when doing a relocatable link.
  template<int size, bool big_endian>
  void
  layout_group(Symbol_table* symtab,
	       Sized_relobj<size, big_endian>* object,
	       unsigned int group_shndx,
	       const char* group_section_name,
	       const char* signature,
	       const elfcpp::Shdr<size, big_endian>& shdr,
	       elfcpp::Elf_Word flags,
	       std::vector<unsigned int>* shndxes);

  // Like layout, only for exception frame sections.  OBJECT is an
  // object file.  SYMBOLS is the contents of the symbol table
  // section, with size SYMBOLS_SIZE.  SYMBOL_NAMES is the contents of
  // the symbol name section, with size SYMBOL_NAMES_SIZE.  SHNDX is a
  // .eh_frame section in OBJECT.  SHDR is the section header.
  // RELOC_SHNDX is the index of a relocation section which applies to
  // this section, or 0 if none, or -1U if more than one.  RELOC_TYPE
  // is the type of the relocation section if there is one.  This
  // returns the output section, and sets *OFFSET to the offset.
  template<int size, bool big_endian>
  Output_section*
  layout_eh_frame(Sized_relobj<size, big_endian>* object,
		  const unsigned char* symbols,
		  off_t symbols_size,
		  const unsigned char* symbol_names,
		  off_t symbol_names_size,
		  unsigned int shndx,
		  const elfcpp::Shdr<size, big_endian>& shdr,
		  unsigned int reloc_shndx, unsigned int reloc_type,
		  off_t* offset);

  // Handle a GNU stack note.  This is called once per input object
  // file.  SEEN_GNU_STACK is true if the object file has a
  // .note.GNU-stack section.  GNU_STACK_FLAGS is the section flags
  // from that section if there was one.
  void
  layout_gnu_stack(bool seen_gnu_stack, uint64_t gnu_stack_flags);

  // Add an Output_section_data to the layout.  This is used for
  // special sections like the GOT section.
  Output_section*
  add_output_section_data(const char* name, elfcpp::Elf_Word type,
			  elfcpp::Elf_Xword flags,
			  Output_section_data*);

  // Create dynamic sections if necessary.
  void
  create_initial_dynamic_sections(Symbol_table*);

  // Define __start and __stop symbols for output sections.
  void
  define_section_symbols(Symbol_table*);

  // Create sections for linker scripts.
  void
  create_script_sections()
  { this->script_options_->create_script_sections(this); }

  // Define symbols from any linker script.
  void
  define_script_symbols(Symbol_table* symtab)
  { this->script_options_->add_symbols_to_table(symtab); }

  // Define symbols for group signatures.
  void
  define_group_signatures(Symbol_table*);

  // Return the Stringpool used for symbol names.
  const Stringpool*
  sympool() const
  { return &this->sympool_; }

  // Return the Stringpool used for dynamic symbol names and dynamic
  // tags.
  const Stringpool*
  dynpool() const
  { return &this->dynpool_; }

  // Return the symtab_xindex section used to hold large section
  // indexes for the normal symbol table.
  Output_symtab_xindex*
  symtab_xindex() const
  { return this->symtab_xindex_; }

  // Return the dynsym_xindex section used to hold large section
  // indexes for the dynamic symbol table.
  Output_symtab_xindex*
  dynsym_xindex() const
  { return this->dynsym_xindex_; }

  // Return whether a section is a .gnu.linkonce section, given the
  // section name.
  static inline bool
  is_linkonce(const char* name)
  { return strncmp(name, ".gnu.linkonce", sizeof(".gnu.linkonce") - 1) == 0; }

  // Return true if a section is a debugging section.
  static inline bool
  is_debug_info_section(const char* name)
  {
    // Debugging sections can only be recognized by name.
    return (strncmp(name, ".debug", sizeof(".debug") - 1) == 0
            || strncmp(name, ".gnu.linkonce.wi.", 
                       sizeof(".gnu.linkonce.wi.") - 1) == 0
            || strncmp(name, ".line", sizeof(".line") - 1) == 0
            || strncmp(name, ".stab", sizeof(".stab") - 1) == 0);
  }

  // Record the signature of a comdat section, and return whether to
  // include it in the link.  The GROUP parameter is true for a
  // section group signature, false for a signature derived from a
  // .gnu.linkonce section.
  bool
  add_comdat(Relobj*, unsigned int, const std::string&, bool group);

  // Find the given comdat signature, and return the object and section
  // index of the kept group.
  Relobj*
  find_kept_object(const std::string&, unsigned int*) const;

  // Finalize the layout after all the input sections have been added.
  off_t
  finalize(const Input_objects*, Symbol_table*, Target*, const Task*);

  // Return whether any sections require postprocessing.
  bool
  any_postprocessing_sections() const
  { return this->any_postprocessing_sections_; }

  // Return the size of the output file.
  off_t
  output_file_size() const
  { return this->output_file_size_; }

  // Return the TLS segment.  This will return NULL if there isn't
  // one.
  Output_segment*
  tls_segment() const
  { return this->tls_segment_; }

  // Return the normal symbol table.
  Output_section*
  symtab_section() const
  {
    gold_assert(this->symtab_section_ != NULL);
    return this->symtab_section_;
  }

  // Return the dynamic symbol table.
  Output_section*
  dynsym_section() const
  {
    gold_assert(this->dynsym_section_ != NULL);
    return this->dynsym_section_;
  }

  // Return the dynamic tags.
  Output_data_dynamic*
  dynamic_data() const
  { return this->dynamic_data_; }

  // Write out the output sections.
  void
  write_output_sections(Output_file* of) const;

  // Write out data not associated with an input file or the symbol
  // table.
  void
  write_data(const Symbol_table*, Output_file*) const;

  // Write out output sections which can not be written until all the
  // input sections are complete.
  void
  write_sections_after_input_sections(Output_file* of);

  // Return an output section named NAME, or NULL if there is none.
  Output_section*
  find_output_section(const char* name) const;

  // Return an output segment of type TYPE, with segment flags SET set
  // and segment flags CLEAR clear.  Return NULL if there is none.
  Output_segment*
  find_output_segment(elfcpp::PT type, elfcpp::Elf_Word set,
		      elfcpp::Elf_Word clear) const;

  // Return the number of segments we expect to produce.
  size_t
  expected_segment_count() const;

  // Set a flag to indicate that an object file uses the static TLS model.
  void
  set_has_static_tls()
  { this->has_static_tls_ = true; }

  // Return true if any object file uses the static TLS model.
  bool
  has_static_tls() const
  { return this->has_static_tls_; }

  // Return the options which may be set by a linker script.
  Script_options*
  script_options()
  { return this->script_options_; }

  const Script_options*
  script_options() const
  { return this->script_options_; }

  // Compute and write out the build ID if needed.
  void
  write_build_id(Output_file*) const;

  // Rewrite output file in binary format.
  void
  write_binary(Output_file* in) const;

  // Print output sections to the map file.
  void
  print_to_mapfile(Mapfile*) const;

  // Dump statistical information to stderr.
  void
  print_stats() const;

  // A list of segments.

  typedef std::vector<Output_segment*> Segment_list;

  // A list of sections.

  typedef std::vector<Output_section*> Section_list;

  // The list of information to write out which is not attached to
  // either a section or a segment.
  typedef std::vector<Output_data*> Data_list;

  // Store the allocated sections into the section list.  This is used
  // by the linker script code.
  void
  get_allocated_sections(Section_list*) const;

  // Make a section for a linker script to hold data.
  Output_section*
  make_output_section_for_script(const char* name);

  // Make a segment.  This is used by the linker script code.
  Output_segment*
  make_output_segment(elfcpp::Elf_Word type, elfcpp::Elf_Word flags);

  // Return the number of segments.
  size_t
  segment_count() const
  { return this->segment_list_.size(); }

  // Map from section flags to segment flags.
  static elfcpp::Elf_Word
  section_flags_to_segment(elfcpp::Elf_Xword flags);

  // Attach sections to segments.
  void
  attach_sections_to_segments();

 private:
  Layout(const Layout&);
  Layout& operator=(const Layout&);

  // Mapping from .gnu.linkonce section names to output section names.
  struct Linkonce_mapping
  {
    const char* from;
    int fromlen;
    const char* to;
    int tolen;
  };
  static const Linkonce_mapping linkonce_mapping[];
  static const int linkonce_mapping_count;

  // During a relocatable link, a list of group sections and
  // signatures.
  struct Group_signature
  {
    // The group section.
    Output_section* section;
    // The signature.
    const char* signature;

    Group_signature()
      : section(NULL), signature(NULL)
    { }

    Group_signature(Output_section* sectiona, const char* signaturea)
      : section(sectiona), signature(signaturea)
    { }
  };
  typedef std::vector<Group_signature> Group_signatures;

  // Create a .note section, filling in the header.
  Output_section*
  create_note(const char* name, int note_type, size_t descsz,
	      bool allocate, size_t* trailing_padding);

  // Create a .note section for gold.
  void
  create_gold_note();

  // Record whether the stack must be executable.
  void
  create_executable_stack_info(const Target*);

  // Create a build ID note if needed.
  void
  create_build_id();

  // Find the first read-only PT_LOAD segment, creating one if
  // necessary.
  Output_segment*
  find_first_load_seg();

  // Count the local symbols in the regular symbol table and the dynamic
  // symbol table, and build the respective string pools.
  void
  count_local_symbols(const Task*, const Input_objects*);

  // Create the output sections for the symbol table.
  void
  create_symtab_sections(const Input_objects*, Symbol_table*,
			 unsigned int, off_t*);

  // Create the .shstrtab section.
  Output_section*
  create_shstrtab();

  // Create the section header table.
  void
  create_shdrs(const Output_section* shstrtab_section, off_t*);

  // Create the dynamic symbol table.
  void
  create_dynamic_symtab(const Input_objects*, Symbol_table*,
			Output_section** pdynstr,
			unsigned int* plocal_dynamic_count,
			std::vector<Symbol*>* pdynamic_symbols,
			Versions* versions);

  // Assign offsets to each local portion of the dynamic symbol table.
  void
  assign_local_dynsym_offsets(const Input_objects*);

  // Finish the .dynamic section and PT_DYNAMIC segment.
  void
  finish_dynamic_section(const Input_objects*, const Symbol_table*);

  // Create the .interp section and PT_INTERP segment.
  void
  create_interp(const Target* target);

  // Create the version sections.
  void
  create_version_sections(const Versions*,
			  const Symbol_table*,
			  unsigned int local_symcount,
			  const std::vector<Symbol*>& dynamic_symbols,
			  const Output_section* dynstr);

  template<int size, bool big_endian>
  void
  sized_create_version_sections(const Versions* versions,
				const Symbol_table*,
				unsigned int local_symcount,
				const std::vector<Symbol*>& dynamic_symbols,
				const Output_section* dynstr);

  // Return whether to include this section in the link.
  template<int size, bool big_endian>
  bool
  include_section(Sized_relobj<size, big_endian>* object, const char* name,
		  const elfcpp::Shdr<size, big_endian>&);

  // Return the output section name to use given an input section
  // name.  Set *PLEN to the length of the name.  *PLEN must be
  // initialized to the length of NAME.
  static const char*
  output_section_name(const char* name, size_t* plen);

  // Return the output section name to use for a linkonce section
  // name.  PLEN is as for output_section_name.
  static const char*
  linkonce_output_name(const char* name, size_t* plen);

  // Return the number of allocated output sections.
  size_t
  allocated_output_section_count() const;

  // Return the output section for NAME, TYPE and FLAGS.
  Output_section*
  get_output_section(const char* name, Stringpool::Key name_key,
		     elfcpp::Elf_Word type, elfcpp::Elf_Xword flags);

  // Choose the output section for NAME in RELOBJ.
  Output_section*
  choose_output_section(const Relobj* relobj, const char* name,
			elfcpp::Elf_Word type, elfcpp::Elf_Xword flags,
			bool is_input_section);

  // Create a new Output_section.
  Output_section*
  make_output_section(const char* name, elfcpp::Elf_Word type,
		      elfcpp::Elf_Xword flags);

  // Attach a section to a segment.
  void
  attach_section_to_segment(Output_section*);

  // Attach an allocated section to a segment.
  void
  attach_allocated_section_to_segment(Output_section*);

  // Set the final file offsets of all the segments.
  off_t
  set_segment_offsets(const Target*, Output_segment*, unsigned int* pshndx);

  // Set the file offsets of the sections when doing a relocatable
  // link.
  off_t
  set_relocatable_section_offsets(Output_data*, unsigned int* pshndx);

  // Set the final file offsets of all the sections not associated
  // with a segment.  We set section offsets in three passes: the
  // first handles all allocated sections, the second sections that
  // require postprocessing, and the last the late-bound STRTAB
  // sections (probably only shstrtab, which is the one we care about
  // because it holds section names).
  enum Section_offset_pass
  {
    BEFORE_INPUT_SECTIONS_PASS,
    POSTPROCESSING_SECTIONS_PASS,
    STRTAB_AFTER_POSTPROCESSING_SECTIONS_PASS
  };
  off_t
  set_section_offsets(off_t, Section_offset_pass pass);

  // Set the final section indexes of all the sections not associated
  // with a segment.  Returns the next unused index.
  unsigned int
  set_section_indexes(unsigned int pshndx);

  // Set the section addresses when using a script.
  Output_segment*
  set_section_addresses_from_script(Symbol_table*);

  // Return whether SEG1 comes before SEG2 in the output file.
  static bool
  segment_precedes(const Output_segment* seg1, const Output_segment* seg2);

  // A mapping used for group signatures.
  struct Kept_section
    {
      Kept_section()
        : object_(NULL), shndx_(0), group_(false)
      { }
      Kept_section(Relobj* object, unsigned int shndx, bool group)
        : object_(object), shndx_(shndx), group_(group)
      { }
      Relobj* object_;
      unsigned int shndx_;
      bool group_;
    };
  typedef Unordered_map<std::string, Kept_section> Signatures;

  // Mapping from input section name/type/flags to output section.  We
  // use canonicalized strings here.

  typedef std::pair<Stringpool::Key,
		    std::pair<elfcpp::Elf_Word, elfcpp::Elf_Xword> > Key;

  struct Hash_key
  {
    size_t
    operator()(const Key& k) const;
  };

  typedef Unordered_map<Key, Output_section*, Hash_key> Section_name_map;

  // A comparison class for segments.

  struct Compare_segments
  {
    bool
    operator()(const Output_segment* seg1, const Output_segment* seg2)
    { return Layout::segment_precedes(seg1, seg2); }
  };

  // A reference to the options on the command line.
  const General_options& options_;
  // Information set by scripts or by command line options.
  Script_options* script_options_;
  // The output section names.
  Stringpool namepool_;
  // The output symbol names.
  Stringpool sympool_;
  // The dynamic strings, if needed.
  Stringpool dynpool_;
  // The list of group sections and linkonce sections which we have seen.
  Signatures signatures_;
  // The mapping from input section name/type/flags to output sections.
  Section_name_map section_name_map_;
  // The list of output segments.
  Segment_list segment_list_;
  // The list of output sections.
  Section_list section_list_;
  // The list of output sections which are not attached to any output
  // segment.
  Section_list unattached_section_list_;
  // Whether we have attached the sections to the segments.
  bool sections_are_attached_;
  // The list of unattached Output_data objects which require special
  // handling because they are not Output_sections.
  Data_list special_output_list_;
  // The section headers.
  Output_section_headers* section_headers_;
  // A pointer to the PT_TLS segment if there is one.
  Output_segment* tls_segment_;
  // A pointer to the PT_GNU_RELRO segment if there is one.
  Output_segment* relro_segment_;
  // The SHT_SYMTAB output section.
  Output_section* symtab_section_;
  // The SHT_SYMTAB_SHNDX for the regular symbol table if there is one.
  Output_symtab_xindex* symtab_xindex_;
  // The SHT_DYNSYM output section if there is one.
  Output_section* dynsym_section_;
  // The SHT_SYMTAB_SHNDX for the dynamic symbol table if there is one.
  Output_symtab_xindex* dynsym_xindex_;
  // The SHT_DYNAMIC output section if there is one.
  Output_section* dynamic_section_;
  // The dynamic data which goes into dynamic_section_.
  Output_data_dynamic* dynamic_data_;
  // The exception frame output section if there is one.
  Output_section* eh_frame_section_;
  // The exception frame data for eh_frame_section_.
  Eh_frame* eh_frame_data_;
  // Whether we have added eh_frame_data_ to the .eh_frame section.
  bool added_eh_frame_data_;
  // The exception frame header output section if there is one.
  Output_section* eh_frame_hdr_section_;
  // The space for the build ID checksum if there is one.
  Output_section_data* build_id_note_;
  // The output section containing dwarf abbreviations
  Output_reduced_debug_abbrev_section* debug_abbrev_;
  // The output section containing the dwarf debug info tree
  Output_reduced_debug_info_section* debug_info_;
  // A list of group sections and their signatures.
  Group_signatures group_signatures_;
  // The size of the output file.
  off_t output_file_size_;
  // Whether we have seen an object file marked to require an
  // executable stack.
  bool input_requires_executable_stack_;
  // Whether we have seen at least one object file with an executable
  // stack marker.
  bool input_with_gnu_stack_note_;
  // Whether we have seen at least one object file without an
  // executable stack marker.
  bool input_without_gnu_stack_note_;
  // Whether we have seen an object file that uses the static TLS model.
  bool has_static_tls_;
  // Whether any sections require postprocessing.
  bool any_postprocessing_sections_;
};

// This task handles writing out data in output sections which is not
// part of an input section, or which requires special handling.  When
// this is done, it unblocks both output_sections_blocker and
// final_blocker.

class Write_sections_task : public Task
{
 public:
  Write_sections_task(const Layout* layout, Output_file* of,
		      Task_token* output_sections_blocker,
		      Task_token* final_blocker)
    : layout_(layout), of_(of),
      output_sections_blocker_(output_sections_blocker),
      final_blocker_(final_blocker)
  { }

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const
  { return "Write_sections_task"; }

 private:
  class Write_sections_locker;

  const Layout* layout_;
  Output_file* of_;
  Task_token* output_sections_blocker_;
  Task_token* final_blocker_;
};

// This task handles writing out data which is not part of a section
// or segment.

class Write_data_task : public Task
{
 public:
  Write_data_task(const Layout* layout, const Symbol_table* symtab,
		  Output_file* of, Task_token* final_blocker)
    : layout_(layout), symtab_(symtab), of_(of), final_blocker_(final_blocker)
  { }

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const
  { return "Write_data_task"; }

 private:
  const Layout* layout_;
  const Symbol_table* symtab_;
  Output_file* of_;
  Task_token* final_blocker_;
};

// This task handles writing out the global symbols.

class Write_symbols_task : public Task
{
 public:
  Write_symbols_task(const Layout* layout, const Symbol_table* symtab,
		     const Input_objects* input_objects,
		     const Stringpool* sympool, const Stringpool* dynpool,
		     Output_file* of, Task_token* final_blocker)
    : layout_(layout), symtab_(symtab), input_objects_(input_objects),
      sympool_(sympool), dynpool_(dynpool), of_(of),
      final_blocker_(final_blocker)
  { }

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const
  { return "Write_symbols_task"; }

 private:
  const Layout* layout_;
  const Symbol_table* symtab_;
  const Input_objects* input_objects_;
  const Stringpool* sympool_;
  const Stringpool* dynpool_;
  Output_file* of_;
  Task_token* final_blocker_;
};

// This task handles writing out data in output sections which can't
// be written out until all the input sections have been handled.
// This is for sections whose contents is based on the contents of
// other output sections.

class Write_after_input_sections_task : public Task
{
 public:
  Write_after_input_sections_task(Layout* layout, Output_file* of,
				  Task_token* input_sections_blocker,
				  Task_token* final_blocker)
    : layout_(layout), of_(of),
      input_sections_blocker_(input_sections_blocker),
      final_blocker_(final_blocker)
  { }

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const
  { return "Write_after_input_sections_task"; }

 private:
  Layout* layout_;
  Output_file* of_;
  Task_token* input_sections_blocker_;
  Task_token* final_blocker_;
};

// This task function handles closing the file.

class Close_task_runner : public Task_function_runner
{
 public:
  Close_task_runner(const General_options* options, const Layout* layout,
		    Output_file* of)
    : options_(options), layout_(layout), of_(of)
  { }

  // Run the operation.
  void
  run(Workqueue*, const Task*);

 private:
  const General_options* options_;
  const Layout* layout_;
  Output_file* of_;
};

// A small helper function to align an address.

inline uint64_t
align_address(uint64_t address, uint64_t addralign)
{
  if (addralign != 0)
    address = (address + addralign - 1) &~ (addralign - 1);
  return address;
}

} // End namespace gold.

#endif // !defined(GOLD_LAYOUT_H)
