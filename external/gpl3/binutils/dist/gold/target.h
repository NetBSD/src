// target.h -- target support for gold   -*- C++ -*-

// Copyright 2006, 2007, 2008, 2009, 2010, 2011, 2012
// Free Software Foundation, Inc.
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

// The abstract class Target is the interface for target specific
// support.  It defines abstract methods which each target must
// implement.  Typically there will be one target per processor, but
// in some cases it may be necessary to have subclasses.

// For speed and consistency we want to use inline functions to handle
// relocation processing.  So besides implementations of the abstract
// methods, each target is expected to define a template
// specialization of the relocation functions.

#ifndef GOLD_TARGET_H
#define GOLD_TARGET_H

#include "elfcpp.h"
#include "options.h"
#include "parameters.h"
#include "debug.h"

namespace gold
{

class Object;
class Relobj;
template<int size, bool big_endian>
class Sized_relobj;
template<int size, bool big_endian>
class Sized_relobj_file;
class Relocatable_relocs;
template<int size, bool big_endian>
struct Relocate_info;
class Reloc_symbol_changes;
class Symbol;
template<int size>
class Sized_symbol;
class Symbol_table;
class Output_data;
class Output_data_got_base;
class Output_section;
class Input_objects;
class Task;

// The abstract class for target specific handling.

class Target
{
 public:
  virtual ~Target()
  { }

  // Return the bit size that this target implements.  This should
  // return 32 or 64.
  int
  get_size() const
  { return this->pti_->size; }

  // Return whether this target is big-endian.
  bool
  is_big_endian() const
  { return this->pti_->is_big_endian; }

  // Machine code to store in e_machine field of ELF header.
  elfcpp::EM
  machine_code() const
  { return this->pti_->machine_code; }

  // Processor specific flags to store in e_flags field of ELF header.
  elfcpp::Elf_Word
  processor_specific_flags() const
  { return this->processor_specific_flags_; }

  // Whether processor specific flags are set at least once.
  bool
  are_processor_specific_flags_set() const
  { return this->are_processor_specific_flags_set_; }

  // Whether this target has a specific make_symbol function.
  bool
  has_make_symbol() const
  { return this->pti_->has_make_symbol; }

  // Whether this target has a specific resolve function.
  bool
  has_resolve() const
  { return this->pti_->has_resolve; }

  // Whether this target has a specific code fill function.
  bool
  has_code_fill() const
  { return this->pti_->has_code_fill; }

  // Return the default name of the dynamic linker.
  const char*
  dynamic_linker() const
  { return this->pti_->dynamic_linker; }

  // Return the default address to use for the text segment.
  uint64_t
  default_text_segment_address() const
  { return this->pti_->default_text_segment_address; }

  // Return the ABI specified page size.
  uint64_t
  abi_pagesize() const
  {
    if (parameters->options().max_page_size() > 0)
      return parameters->options().max_page_size();
    else
      return this->pti_->abi_pagesize;
  }

  // Return the common page size used on actual systems.
  uint64_t
  common_pagesize() const
  {
    if (parameters->options().common_page_size() > 0)
      return std::min(parameters->options().common_page_size(),
		      this->abi_pagesize());
    else
      return std::min(this->pti_->common_pagesize,
		      this->abi_pagesize());
  }

  // Return whether PF_X segments must contain nothing but the contents of
  // SHF_EXECINSTR sections (no non-executable data, no headers).
  bool
  isolate_execinstr() const
  { return this->pti_->isolate_execinstr; }

  uint64_t
  rosegment_gap() const
  { return this->pti_->rosegment_gap; }

  // If we see some object files with .note.GNU-stack sections, and
  // some objects files without them, this returns whether we should
  // consider the object files without them to imply that the stack
  // should be executable.
  bool
  is_default_stack_executable() const
  { return this->pti_->is_default_stack_executable; }

  // Return a character which may appear as a prefix for a wrap
  // symbol.  If this character appears, we strip it when checking for
  // wrapping and add it back when forming the final symbol name.
  // This should be '\0' if not special prefix is required, which is
  // the normal case.
  char
  wrap_char() const
  { return this->pti_->wrap_char; }

  // Return the special section index which indicates a small common
  // symbol.  This will return SHN_UNDEF if there are no small common
  // symbols.
  elfcpp::Elf_Half
  small_common_shndx() const
  { return this->pti_->small_common_shndx; }

  // Return values to add to the section flags for the section holding
  // small common symbols.
  elfcpp::Elf_Xword
  small_common_section_flags() const
  {
    gold_assert(this->pti_->small_common_shndx != elfcpp::SHN_UNDEF);
    return this->pti_->small_common_section_flags;
  }

  // Return the special section index which indicates a large common
  // symbol.  This will return SHN_UNDEF if there are no large common
  // symbols.
  elfcpp::Elf_Half
  large_common_shndx() const
  { return this->pti_->large_common_shndx; }

  // Return values to add to the section flags for the section holding
  // large common symbols.
  elfcpp::Elf_Xword
  large_common_section_flags() const
  {
    gold_assert(this->pti_->large_common_shndx != elfcpp::SHN_UNDEF);
    return this->pti_->large_common_section_flags;
  }

  // This hook is called when an output section is created.
  void
  new_output_section(Output_section* os) const
  { this->do_new_output_section(os); }

  // This is called to tell the target to complete any sections it is
  // handling.  After this all sections must have their final size.
  void
  finalize_sections(Layout* layout, const Input_objects* input_objects,
		    Symbol_table* symtab)
  { return this->do_finalize_sections(layout, input_objects, symtab); }

  // Return the value to use for a global symbol which needs a special
  // value in the dynamic symbol table.  This will only be called if
  // the backend first calls symbol->set_needs_dynsym_value().
  uint64_t
  dynsym_value(const Symbol* sym) const
  { return this->do_dynsym_value(sym); }

  // Return a string to use to fill out a code section.  This is
  // basically one or more NOPS which must fill out the specified
  // length in bytes.
  std::string
  code_fill(section_size_type length) const
  { return this->do_code_fill(length); }

  // Return whether SYM is known to be defined by the ABI.  This is
  // used to avoid inappropriate warnings about undefined symbols.
  bool
  is_defined_by_abi(const Symbol* sym) const
  { return this->do_is_defined_by_abi(sym); }

  // Adjust the output file header before it is written out.  VIEW
  // points to the header in external form.  LEN is the length.
  void
  adjust_elf_header(unsigned char* view, int len) const
  { return this->do_adjust_elf_header(view, len); }

  // Return whether NAME is a local label name.  This is used to implement the
  // --discard-locals options.
  bool
  is_local_label_name(const char* name) const
  { return this->do_is_local_label_name(name); }

  // Get the symbol index to use for a target specific reloc.
  unsigned int
  reloc_symbol_index(void* arg, unsigned int type) const
  { return this->do_reloc_symbol_index(arg, type); }

  // Get the addend to use for a target specific reloc.
  uint64_t
  reloc_addend(void* arg, unsigned int type, uint64_t addend) const
  { return this->do_reloc_addend(arg, type, addend); }

  // Return the PLT address to use for a global symbol.  This is used
  // for STT_GNU_IFUNC symbols.  The symbol's plt_offset is relative
  // to this PLT address.
  uint64_t
  plt_address_for_global(const Symbol* sym) const
  { return this->do_plt_address_for_global(sym); }

  // Return the PLT address to use for a local symbol.  This is used
  // for STT_GNU_IFUNC symbols.  The symbol's plt_offset is relative
  // to this PLT address.
  uint64_t
  plt_address_for_local(const Relobj* object, unsigned int symndx) const
  { return this->do_plt_address_for_local(object, symndx); }

  // Return whether this target can use relocation types to determine
  // if a function's address is taken.
  bool
  can_check_for_function_pointers() const
  { return this->do_can_check_for_function_pointers(); }

  // Return whether a relocation to a merged section can be processed
  // to retrieve the contents.
  bool
  can_icf_inline_merge_sections () const
  { return this->pti_->can_icf_inline_merge_sections; }

  // Whether a section called SECTION_NAME may have function pointers to
  // sections not eligible for safe ICF folding.
  virtual bool
  section_may_have_icf_unsafe_pointers(const char* section_name) const
  { return this->do_section_may_have_icf_unsafe_pointers(section_name); }

  // Return the base to use for the PC value in an FDE when it is
  // encoded using DW_EH_PE_datarel.  This does not appear to be
  // documented anywhere, but it is target specific.  Any use of
  // DW_EH_PE_datarel in gcc requires defining a special macro
  // (ASM_MAYBE_OUTPUT_ENCODED_ADDR_RTX) to output the value.
  uint64_t
  ehframe_datarel_base() const
  { return this->do_ehframe_datarel_base(); }

  // Return true if a reference to SYM from a reloc of type R_TYPE
  // means that the current function may call an object compiled
  // without -fsplit-stack.  SYM is known to be defined in an object
  // compiled without -fsplit-stack.
  bool
  is_call_to_non_split(const Symbol* sym, unsigned int r_type) const
  { return this->do_is_call_to_non_split(sym, r_type); }

  // A function starts at OFFSET in section SHNDX in OBJECT.  That
  // function was compiled with -fsplit-stack, but it refers to a
  // function which was compiled without -fsplit-stack.  VIEW is a
  // modifiable view of the section; VIEW_SIZE is the size of the
  // view.  The target has to adjust the function so that it allocates
  // enough stack.
  void
  calls_non_split(Relobj* object, unsigned int shndx,
		  section_offset_type fnoffset, section_size_type fnsize,
		  unsigned char* view, section_size_type view_size,
		  std::string* from, std::string* to) const
  {
    this->do_calls_non_split(object, shndx, fnoffset, fnsize, view, view_size,
			     from, to);
  }

  // Make an ELF object.
  template<int size, bool big_endian>
  Object*
  make_elf_object(const std::string& name, Input_file* input_file,
		  off_t offset, const elfcpp::Ehdr<size, big_endian>& ehdr)
  { return this->do_make_elf_object(name, input_file, offset, ehdr); }

  // Make an output section.
  Output_section*
  make_output_section(const char* name, elfcpp::Elf_Word type,
		      elfcpp::Elf_Xword flags)
  { return this->do_make_output_section(name, type, flags); }

  // Return true if target wants to perform relaxation.
  bool
  may_relax() const
  {
    // Run the dummy relaxation pass twice if relaxation debugging is enabled.
    if (is_debugging_enabled(DEBUG_RELAXATION))
      return true;

     return this->do_may_relax();
  }

  // Perform a relaxation pass.  Return true if layout may be changed.
  bool
  relax(int pass, const Input_objects* input_objects, Symbol_table* symtab,
	Layout* layout, const Task* task)
  {
    // Run the dummy relaxation pass twice if relaxation debugging is enabled.
    if (is_debugging_enabled(DEBUG_RELAXATION))
      return pass < 2;

    return this->do_relax(pass, input_objects, symtab, layout, task);
  }

  // Return the target-specific name of attributes section.  This is
  // NULL if a target does not use attributes section or if it uses
  // the default section name ".gnu.attributes".
  const char*
  attributes_section() const
  { return this->pti_->attributes_section; }

  // Return the vendor name of vendor attributes.
  const char*
  attributes_vendor() const
  { return this->pti_->attributes_vendor; }

  // Whether a section called NAME is an attribute section.
  bool
  is_attributes_section(const char* name) const
  {
    return ((this->pti_->attributes_section != NULL
	     && strcmp(name, this->pti_->attributes_section) == 0)
	    || strcmp(name, ".gnu.attributes") == 0);
  }

  // Return a bit mask of argument types for attribute with TAG.
  int
  attribute_arg_type(int tag) const
  { return this->do_attribute_arg_type(tag); }

  // Return the attribute tag of the position NUM in the list of fixed
  // attributes.  Normally there is no reordering and
  // attributes_order(NUM) == NUM.
  int
  attributes_order(int num) const
  { return this->do_attributes_order(num); }

  // When a target is selected as the default target, we call this method,
  // which may be used for expensive, target-specific initialization.
  void
  select_as_default_target()
  { this->do_select_as_default_target(); }

  // Return the value to store in the EI_OSABI field in the ELF
  // header.
  elfcpp::ELFOSABI
  osabi() const
  { return this->osabi_; }

  // Set the value to store in the EI_OSABI field in the ELF header.
  void
  set_osabi(elfcpp::ELFOSABI osabi)
  { this->osabi_ = osabi; }

  // Define target-specific standard symbols.
  void
  define_standard_symbols(Symbol_table* symtab, Layout* layout)
  { this->do_define_standard_symbols(symtab, layout); }

 protected:
  // This struct holds the constant information for a child class.  We
  // use a struct to avoid the overhead of virtual function calls for
  // simple information.
  struct Target_info
  {
    // Address size (32 or 64).
    int size;
    // Whether the target is big endian.
    bool is_big_endian;
    // The code to store in the e_machine field of the ELF header.
    elfcpp::EM machine_code;
    // Whether this target has a specific make_symbol function.
    bool has_make_symbol;
    // Whether this target has a specific resolve function.
    bool has_resolve;
    // Whether this target has a specific code fill function.
    bool has_code_fill;
    // Whether an object file with no .note.GNU-stack sections implies
    // that the stack should be executable.
    bool is_default_stack_executable;
    // Whether a relocation to a merged section can be processed to
    // retrieve the contents.
    bool can_icf_inline_merge_sections;
    // Prefix character to strip when checking for wrapping.
    char wrap_char;
    // The default dynamic linker name.
    const char* dynamic_linker;
    // The default text segment address.
    uint64_t default_text_segment_address;
    // The ABI specified page size.
    uint64_t abi_pagesize;
    // The common page size used by actual implementations.
    uint64_t common_pagesize;
    // Whether PF_X segments must contain nothing but the contents of
    // SHF_EXECINSTR sections (no non-executable data, no headers).
    bool isolate_execinstr;
    // If nonzero, distance from the text segment to the read-only segment.
    uint64_t rosegment_gap;
    // The special section index for small common symbols; SHN_UNDEF
    // if none.
    elfcpp::Elf_Half small_common_shndx;
    // The special section index for large common symbols; SHN_UNDEF
    // if none.
    elfcpp::Elf_Half large_common_shndx;
    // Section flags for small common section.
    elfcpp::Elf_Xword small_common_section_flags;
    // Section flags for large common section.
    elfcpp::Elf_Xword large_common_section_flags;
    // Name of attributes section if it is not ".gnu.attributes".
    const char* attributes_section;
    // Vendor name of vendor attributes.
    const char* attributes_vendor;
  };

  Target(const Target_info* pti)
    : pti_(pti), processor_specific_flags_(0),
      are_processor_specific_flags_set_(false), osabi_(elfcpp::ELFOSABI_NONE)
  { }

  // Virtual function which may be implemented by the child class.
  virtual void
  do_new_output_section(Output_section*) const
  { }

  // Virtual function which may be implemented by the child class.
  virtual void
  do_finalize_sections(Layout*, const Input_objects*, Symbol_table*)
  { }

  // Virtual function which may be implemented by the child class.
  virtual uint64_t
  do_dynsym_value(const Symbol*) const
  { gold_unreachable(); }

  // Virtual function which must be implemented by the child class if
  // needed.
  virtual std::string
  do_code_fill(section_size_type) const
  { gold_unreachable(); }

  // Virtual function which may be implemented by the child class.
  virtual bool
  do_is_defined_by_abi(const Symbol*) const
  { return false; }

  // Adjust the output file header before it is written out.  VIEW
  // points to the header in external form.  LEN is the length, and
  // will be one of the values of elfcpp::Elf_sizes<size>::ehdr_size.
  // By default, we set the EI_OSABI field if requested (in
  // Sized_target).
  virtual void
  do_adjust_elf_header(unsigned char*, int) const = 0;

  // Virtual function which may be overridden by the child class.
  virtual bool
  do_is_local_label_name(const char*) const;

  // Virtual function that must be overridden by a target which uses
  // target specific relocations.
  virtual unsigned int
  do_reloc_symbol_index(void*, unsigned int) const
  { gold_unreachable(); }

  // Virtual function that must be overridden by a target which uses
  // target specific relocations.
  virtual uint64_t
  do_reloc_addend(void*, unsigned int, uint64_t) const
  { gold_unreachable(); }

  // Virtual functions that must be overridden by a target that uses
  // STT_GNU_IFUNC symbols.
  virtual uint64_t
  do_plt_address_for_global(const Symbol*) const
  { gold_unreachable(); }

  virtual uint64_t
  do_plt_address_for_local(const Relobj*, unsigned int) const
  { gold_unreachable(); }

  // Virtual function which may be overriden by the child class.
  virtual bool
  do_can_check_for_function_pointers() const
  { return false; }

  // Virtual function which may be overridden by the child class.  We
  // recognize some default sections for which we don't care whether
  // they have function pointers.
  virtual bool
  do_section_may_have_icf_unsafe_pointers(const char* section_name) const
  {
    // We recognize sections for normal vtables, construction vtables and
    // EH frames.
    return (!is_prefix_of(".rodata._ZTV", section_name)
	    && !is_prefix_of(".data.rel.ro._ZTV", section_name)
	    && !is_prefix_of(".rodata._ZTC", section_name)
	    && !is_prefix_of(".data.rel.ro._ZTC", section_name)
	    && !is_prefix_of(".eh_frame", section_name));
  }

  virtual uint64_t
  do_ehframe_datarel_base() const
  { gold_unreachable(); }

  // Virtual function which may be overridden by the child class.  The
  // default implementation is that any function not defined by the
  // ABI is a call to a non-split function.
  virtual bool
  do_is_call_to_non_split(const Symbol* sym, unsigned int) const;

  // Virtual function which may be overridden by the child class.
  virtual void
  do_calls_non_split(Relobj* object, unsigned int, section_offset_type,
		     section_size_type, unsigned char*, section_size_type,
		     std::string*, std::string*) const;

  // make_elf_object hooks.  There are four versions of these for
  // different address sizes and endianness.

  // Set processor specific flags.
  void
  set_processor_specific_flags(elfcpp::Elf_Word flags)
  {
    this->processor_specific_flags_ = flags;
    this->are_processor_specific_flags_set_ = true;
  }

#ifdef HAVE_TARGET_32_LITTLE
  // Virtual functions which may be overridden by the child class.
  virtual Object*
  do_make_elf_object(const std::string&, Input_file*, off_t,
		     const elfcpp::Ehdr<32, false>&);
#endif

#ifdef HAVE_TARGET_32_BIG
  // Virtual functions which may be overridden by the child class.
  virtual Object*
  do_make_elf_object(const std::string&, Input_file*, off_t,
		     const elfcpp::Ehdr<32, true>&);
#endif

#ifdef HAVE_TARGET_64_LITTLE
  // Virtual functions which may be overridden by the child class.
  virtual Object*
  do_make_elf_object(const std::string&, Input_file*, off_t,
		     const elfcpp::Ehdr<64, false>& ehdr);
#endif

#ifdef HAVE_TARGET_64_BIG
  // Virtual functions which may be overridden by the child class.
  virtual Object*
  do_make_elf_object(const std::string& name, Input_file* input_file,
		     off_t offset, const elfcpp::Ehdr<64, true>& ehdr);
#endif

  // Virtual functions which may be overridden by the child class.
  virtual Output_section*
  do_make_output_section(const char* name, elfcpp::Elf_Word type,
			 elfcpp::Elf_Xword flags);

  // Virtual function which may be overridden by the child class.
  virtual bool
  do_may_relax() const
  { return parameters->options().relax(); }

  // Virtual function which may be overridden by the child class.
  virtual bool
  do_relax(int, const Input_objects*, Symbol_table*, Layout*, const Task*)
  { return false; }

  // A function for targets to call.  Return whether BYTES/LEN matches
  // VIEW/VIEW_SIZE at OFFSET.
  bool
  match_view(const unsigned char* view, section_size_type view_size,
	     section_offset_type offset, const char* bytes, size_t len) const;

  // Set the contents of a VIEW/VIEW_SIZE to nops starting at OFFSET
  // for LEN bytes.
  void
  set_view_to_nop(unsigned char* view, section_size_type view_size,
		  section_offset_type offset, size_t len) const;

  // This must be overridden by the child class if it has target-specific
  // attributes subsection in the attribute section.
  virtual int
  do_attribute_arg_type(int) const
  { gold_unreachable(); }

  // This may be overridden by the child class.
  virtual int
  do_attributes_order(int num) const
  { return num; }

  // This may be overridden by the child class.
  virtual void
  do_select_as_default_target()
  { }

  // This may be overridden by the child class.
  virtual void
  do_define_standard_symbols(Symbol_table*, Layout*)
  { }

 private:
  // The implementations of the four do_make_elf_object virtual functions are
  // almost identical except for their sizes and endianness.  We use a template.
  // for their implementations.
  template<int size, bool big_endian>
  inline Object*
  do_make_elf_object_implementation(const std::string&, Input_file*, off_t,
				    const elfcpp::Ehdr<size, big_endian>&);

  Target(const Target&);
  Target& operator=(const Target&);

  // The target information.
  const Target_info* pti_;
  // Processor-specific flags.
  elfcpp::Elf_Word processor_specific_flags_;
  // Whether the processor-specific flags are set at least once.
  bool are_processor_specific_flags_set_;
  // If not ELFOSABI_NONE, the value to put in the EI_OSABI field of
  // the ELF header.  This is handled at this level because it is
  // OS-specific rather than processor-specific.
  elfcpp::ELFOSABI osabi_;
};

// The abstract class for a specific size and endianness of target.
// Each actual target implementation class should derive from an
// instantiation of Sized_target.

template<int size, bool big_endian>
class Sized_target : public Target
{
 public:
  // Make a new symbol table entry for the target.  This should be
  // overridden by a target which needs additional information in the
  // symbol table.  This will only be called if has_make_symbol()
  // returns true.
  virtual Sized_symbol<size>*
  make_symbol() const
  { gold_unreachable(); }

  // Resolve a symbol for the target.  This should be overridden by a
  // target which needs to take special action.  TO is the
  // pre-existing symbol.  SYM is the new symbol, seen in OBJECT.
  // VERSION is the version of SYM.  This will only be called if
  // has_resolve() returns true.
  virtual void
  resolve(Symbol*, const elfcpp::Sym<size, big_endian>&, Object*,
	  const char*)
  { gold_unreachable(); }

  // Process the relocs for a section, and record information of the
  // mapping from source to destination sections. This mapping is later
  // used to determine unreferenced garbage sections. This procedure is
  // only called during garbage collection.
  virtual void
  gc_process_relocs(Symbol_table* symtab,
		    Layout* layout,
		    Sized_relobj_file<size, big_endian>* object,
		    unsigned int data_shndx,
		    unsigned int sh_type,
		    const unsigned char* prelocs,
		    size_t reloc_count,
		    Output_section* output_section,
		    bool needs_special_offset_handling,
		    size_t local_symbol_count,
		    const unsigned char* plocal_symbols) = 0;

  // Scan the relocs for a section, and record any information
  // required for the symbol.  SYMTAB is the symbol table.  OBJECT is
  // the object in which the section appears.  DATA_SHNDX is the
  // section index that these relocs apply to.  SH_TYPE is the type of
  // the relocation section, SHT_REL or SHT_RELA.  PRELOCS points to
  // the relocation data.  RELOC_COUNT is the number of relocs.
  // LOCAL_SYMBOL_COUNT is the number of local symbols.
  // OUTPUT_SECTION is the output section.
  // NEEDS_SPECIAL_OFFSET_HANDLING is true if offsets to the output
  // sections are not mapped as usual.  PLOCAL_SYMBOLS points to the
  // local symbol data from OBJECT.  GLOBAL_SYMBOLS is the array of
  // pointers to the global symbol table from OBJECT.
  virtual void
  scan_relocs(Symbol_table* symtab,
	      Layout* layout,
	      Sized_relobj_file<size, big_endian>* object,
	      unsigned int data_shndx,
	      unsigned int sh_type,
	      const unsigned char* prelocs,
	      size_t reloc_count,
	      Output_section* output_section,
	      bool needs_special_offset_handling,
	      size_t local_symbol_count,
	      const unsigned char* plocal_symbols) = 0;

  // Relocate section data.  SH_TYPE is the type of the relocation
  // section, SHT_REL or SHT_RELA.  PRELOCS points to the relocation
  // information.  RELOC_COUNT is the number of relocs.
  // OUTPUT_SECTION is the output section.
  // NEEDS_SPECIAL_OFFSET_HANDLING is true if offsets must be mapped
  // to correspond to the output section.  VIEW is a view into the
  // output file holding the section contents, VIEW_ADDRESS is the
  // virtual address of the view, and VIEW_SIZE is the size of the
  // view.  If NEEDS_SPECIAL_OFFSET_HANDLING is true, the VIEW_xx
  // parameters refer to the complete output section data, not just
  // the input section data.
  virtual void
  relocate_section(const Relocate_info<size, big_endian>*,
		   unsigned int sh_type,
		   const unsigned char* prelocs,
		   size_t reloc_count,
		   Output_section* output_section,
		   bool needs_special_offset_handling,
		   unsigned char* view,
		   typename elfcpp::Elf_types<size>::Elf_Addr view_address,
		   section_size_type view_size,
		   const Reloc_symbol_changes*) = 0;

  // Scan the relocs during a relocatable link.  The parameters are
  // like scan_relocs, with an additional Relocatable_relocs
  // parameter, used to record the disposition of the relocs.
  virtual void
  scan_relocatable_relocs(Symbol_table* symtab,
			  Layout* layout,
			  Sized_relobj_file<size, big_endian>* object,
			  unsigned int data_shndx,
			  unsigned int sh_type,
			  const unsigned char* prelocs,
			  size_t reloc_count,
			  Output_section* output_section,
			  bool needs_special_offset_handling,
			  size_t local_symbol_count,
			  const unsigned char* plocal_symbols,
			  Relocatable_relocs*) = 0;

  // Relocate a section during a relocatable link.  The parameters are
  // like relocate_section, with additional parameters for the view of
  // the output reloc section.
  virtual void
  relocate_for_relocatable(const Relocate_info<size, big_endian>*,
			   unsigned int sh_type,
			   const unsigned char* prelocs,
			   size_t reloc_count,
			   Output_section* output_section,
			   typename elfcpp::Elf_types<size>::Elf_Off
                             offset_in_output_section,
			   const Relocatable_relocs*,
			   unsigned char* view,
			   typename elfcpp::Elf_types<size>::Elf_Addr
			     view_address,
			   section_size_type view_size,
			   unsigned char* reloc_view,
			   section_size_type reloc_view_size) = 0;

  // Perform target-specific processing in a relocatable link.  This is
  // only used if we use the relocation strategy RELOC_SPECIAL.
  // RELINFO points to a Relocation_info structure. SH_TYPE is the relocation
  // section type. PRELOC_IN points to the original relocation.  RELNUM is
  // the index number of the relocation in the relocation section.
  // OUTPUT_SECTION is the output section to which the relocation is applied.
  // OFFSET_IN_OUTPUT_SECTION is the offset of the relocation input section
  // within the output section.  VIEW points to the output view of the
  // output section.  VIEW_ADDRESS is output address of the view.  VIEW_SIZE
  // is the size of the output view and PRELOC_OUT points to the new
  // relocation in the output object.
  //
  // A target only needs to override this if the generic code in
  // target-reloc.h cannot handle some relocation types.

  virtual void
  relocate_special_relocatable(const Relocate_info<size, big_endian>*
				/*relinfo */,
			       unsigned int /* sh_type */,
			       const unsigned char* /* preloc_in */,
			       size_t /* relnum */,
			       Output_section* /* output_section */,
			       typename elfcpp::Elf_types<size>::Elf_Off
                                 /* offset_in_output_section */,
			       unsigned char* /* view */,
			       typename elfcpp::Elf_types<size>::Elf_Addr
				 /* view_address */,
			       section_size_type /* view_size */,
			       unsigned char* /* preloc_out*/)
  { gold_unreachable(); }

  // Return the number of entries in the GOT.  This is only used for
  // laying out the incremental link info sections.  A target needs
  // to implement this to support incremental linking.

  virtual unsigned int
  got_entry_count() const
  { gold_unreachable(); }

  // Return the number of entries in the PLT.  This is only used for
  // laying out the incremental link info sections.  A target needs
  // to implement this to support incremental linking.

  virtual unsigned int
  plt_entry_count() const
  { gold_unreachable(); }

  // Return the offset of the first non-reserved PLT entry.  This is
  // only used for laying out the incremental link info sections.
  // A target needs to implement this to support incremental linking.

  virtual unsigned int
  first_plt_entry_offset() const
  { gold_unreachable(); }

  // Return the size of each PLT entry.  This is only used for
  // laying out the incremental link info sections.  A target needs
  // to implement this to support incremental linking.

  virtual unsigned int
  plt_entry_size() const
  { gold_unreachable(); }

  // Create the GOT and PLT sections for an incremental update.
  // A target needs to implement this to support incremental linking.

  virtual Output_data_got_base*
  init_got_plt_for_update(Symbol_table*,
			  Layout*,
			  unsigned int /* got_count */,
			  unsigned int /* plt_count */)
  { gold_unreachable(); }

  // Reserve a GOT entry for a local symbol, and regenerate any
  // necessary dynamic relocations.
  virtual void
  reserve_local_got_entry(unsigned int /* got_index */,
			  Sized_relobj<size, big_endian>* /* obj */,
			  unsigned int /* r_sym */,
			  unsigned int /* got_type */)
  { gold_unreachable(); }

  // Reserve a GOT entry for a global symbol, and regenerate any
  // necessary dynamic relocations.
  virtual void
  reserve_global_got_entry(unsigned int /* got_index */, Symbol* /* gsym */,
			   unsigned int /* got_type */)
  { gold_unreachable(); }

  // Register an existing PLT entry for a global symbol.
  // A target needs to implement this to support incremental linking.

  virtual void
  register_global_plt_entry(Symbol_table*, Layout*,
			    unsigned int /* plt_index */,
			    Symbol*)
  { gold_unreachable(); }

  // Force a COPY relocation for a given symbol.
  // A target needs to implement this to support incremental linking.

  virtual void
  emit_copy_reloc(Symbol_table*, Symbol*, Output_section*, off_t)
  { gold_unreachable(); }

  // Apply an incremental relocation.

  virtual void
  apply_relocation(const Relocate_info<size, big_endian>* /* relinfo */,
		   typename elfcpp::Elf_types<size>::Elf_Addr /* r_offset */,
		   unsigned int /* r_type */,
		   typename elfcpp::Elf_types<size>::Elf_Swxword /* r_addend */,
		   const Symbol* /* gsym */,
		   unsigned char* /* view */,
		   typename elfcpp::Elf_types<size>::Elf_Addr /* address */,
		   section_size_type /* view_size */)
  { gold_unreachable(); }

 protected:
  Sized_target(const Target::Target_info* pti)
    : Target(pti)
  {
    gold_assert(pti->size == size);
    gold_assert(pti->is_big_endian ? big_endian : !big_endian);
  }

  // Set the EI_OSABI field if requested.
  virtual void
  do_adjust_elf_header(unsigned char*, int) const;
};

} // End namespace gold.

#endif // !defined(GOLD_TARGET_H)
