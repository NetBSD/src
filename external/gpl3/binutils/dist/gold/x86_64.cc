// x86_64.cc -- x86_64 target support for gold.

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

#include <cstring>

#include "elfcpp.h"
#include "parameters.h"
#include "reloc.h"
#include "x86_64.h"
#include "object.h"
#include "symtab.h"
#include "layout.h"
#include "output.h"
#include "copy-relocs.h"
#include "target.h"
#include "target-reloc.h"
#include "target-select.h"
#include "tls.h"

namespace
{

using namespace gold;

class Output_data_plt_x86_64;

// The x86_64 target class.
// See the ABI at
//   http://www.x86-64.org/documentation/abi.pdf
// TLS info comes from
//   http://people.redhat.com/drepper/tls.pdf
//   http://www.lsd.ic.unicamp.br/~oliva/writeups/TLS/RFC-TLSDESC-x86.txt

class Target_x86_64 : public Sized_target<64, false>
{
 public:
  // In the x86_64 ABI (p 68), it says "The AMD64 ABI architectures
  // uses only Elf64_Rela relocation entries with explicit addends."
  typedef Output_data_reloc<elfcpp::SHT_RELA, true, 64, false> Reloc_section;

  Target_x86_64()
    : Sized_target<64, false>(&x86_64_info),
      got_(NULL), plt_(NULL), got_plt_(NULL), rela_dyn_(NULL),
      copy_relocs_(elfcpp::R_X86_64_COPY), dynbss_(NULL),
      got_mod_index_offset_(-1U), tls_base_symbol_defined_(false)
  { }

  // Scan the relocations to look for symbol adjustments.
  void
  scan_relocs(const General_options& options,
	      Symbol_table* symtab,
	      Layout* layout,
	      Sized_relobj<64, false>* object,
	      unsigned int data_shndx,
	      unsigned int sh_type,
	      const unsigned char* prelocs,
	      size_t reloc_count,
	      Output_section* output_section,
	      bool needs_special_offset_handling,
	      size_t local_symbol_count,
	      const unsigned char* plocal_symbols);

  // Finalize the sections.
  void
  do_finalize_sections(Layout*);

  // Return the value to use for a dynamic which requires special
  // treatment.
  uint64_t
  do_dynsym_value(const Symbol*) const;

  // Relocate a section.
  void
  relocate_section(const Relocate_info<64, false>*,
		   unsigned int sh_type,
		   const unsigned char* prelocs,
		   size_t reloc_count,
		   Output_section* output_section,
		   bool needs_special_offset_handling,
		   unsigned char* view,
		   elfcpp::Elf_types<64>::Elf_Addr view_address,
		   section_size_type view_size);

  // Scan the relocs during a relocatable link.
  void
  scan_relocatable_relocs(const General_options& options,
			  Symbol_table* symtab,
			  Layout* layout,
			  Sized_relobj<64, false>* object,
			  unsigned int data_shndx,
			  unsigned int sh_type,
			  const unsigned char* prelocs,
			  size_t reloc_count,
			  Output_section* output_section,
			  bool needs_special_offset_handling,
			  size_t local_symbol_count,
			  const unsigned char* plocal_symbols,
			  Relocatable_relocs*);

  // Relocate a section during a relocatable link.
  void
  relocate_for_relocatable(const Relocate_info<64, false>*,
			   unsigned int sh_type,
			   const unsigned char* prelocs,
			   size_t reloc_count,
			   Output_section* output_section,
			   off_t offset_in_output_section,
			   const Relocatable_relocs*,
			   unsigned char* view,
			   elfcpp::Elf_types<64>::Elf_Addr view_address,
			   section_size_type view_size,
			   unsigned char* reloc_view,
			   section_size_type reloc_view_size);

  // Return a string used to fill a code section with nops.
  std::string
  do_code_fill(section_size_type length) const;

  // Return whether SYM is defined by the ABI.
  bool
  do_is_defined_by_abi(Symbol* sym) const
  { return strcmp(sym->name(), "__tls_get_addr") == 0; }

  // Return the size of the GOT section.
  section_size_type
  got_size()
  {
    gold_assert(this->got_ != NULL);
    return this->got_->data_size();
  }

 private:
  // The class which scans relocations.
  class Scan
  {
  public:
    Scan()
      : issued_non_pic_error_(false)
    { }

    inline void
    local(const General_options& options, Symbol_table* symtab,
	  Layout* layout, Target_x86_64* target,
	  Sized_relobj<64, false>* object,
	  unsigned int data_shndx,
	  Output_section* output_section,
	  const elfcpp::Rela<64, false>& reloc, unsigned int r_type,
	  const elfcpp::Sym<64, false>& lsym);

    inline void
    global(const General_options& options, Symbol_table* symtab,
	   Layout* layout, Target_x86_64* target,
	   Sized_relobj<64, false>* object,
	   unsigned int data_shndx,
	   Output_section* output_section,
	   const elfcpp::Rela<64, false>& reloc, unsigned int r_type,
	   Symbol* gsym);

  private:
    static void
    unsupported_reloc_local(Sized_relobj<64, false>*, unsigned int r_type);

    static void
    unsupported_reloc_global(Sized_relobj<64, false>*, unsigned int r_type,
			     Symbol*);

    void
    check_non_pic(Relobj*, unsigned int r_type);

    // Whether we have issued an error about a non-PIC compilation.
    bool issued_non_pic_error_;
  };

  // The class which implements relocation.
  class Relocate
  {
   public:
    Relocate()
      : skip_call_tls_get_addr_(false), saw_tls_block_reloc_(false)
    { }

    ~Relocate()
    {
      if (this->skip_call_tls_get_addr_)
	{
	  // FIXME: This needs to specify the location somehow.
	  gold_error(_("missing expected TLS relocation"));
	}
    }

    // Do a relocation.  Return false if the caller should not issue
    // any warnings about this relocation.
    inline bool
    relocate(const Relocate_info<64, false>*, Target_x86_64*, size_t relnum,
	     const elfcpp::Rela<64, false>&,
	     unsigned int r_type, const Sized_symbol<64>*,
	     const Symbol_value<64>*,
	     unsigned char*, elfcpp::Elf_types<64>::Elf_Addr,
	     section_size_type);

   private:
    // Do a TLS relocation.
    inline void
    relocate_tls(const Relocate_info<64, false>*, Target_x86_64*,
                 size_t relnum, const elfcpp::Rela<64, false>&,
		 unsigned int r_type, const Sized_symbol<64>*,
		 const Symbol_value<64>*,
		 unsigned char*, elfcpp::Elf_types<64>::Elf_Addr,
		 section_size_type);

    // Do a TLS General-Dynamic to Initial-Exec transition.
    inline void
    tls_gd_to_ie(const Relocate_info<64, false>*, size_t relnum,
		 Output_segment* tls_segment,
		 const elfcpp::Rela<64, false>&, unsigned int r_type,
		 elfcpp::Elf_types<64>::Elf_Addr value,
		 unsigned char* view,
		 elfcpp::Elf_types<64>::Elf_Addr,
		 section_size_type view_size);

    // Do a TLS General-Dynamic to Local-Exec transition.
    inline void
    tls_gd_to_le(const Relocate_info<64, false>*, size_t relnum,
		 Output_segment* tls_segment,
		 const elfcpp::Rela<64, false>&, unsigned int r_type,
		 elfcpp::Elf_types<64>::Elf_Addr value,
		 unsigned char* view,
		 section_size_type view_size);

    // Do a TLSDESC-style General-Dynamic to Initial-Exec transition.
    inline void
    tls_desc_gd_to_ie(const Relocate_info<64, false>*, size_t relnum,
		      Output_segment* tls_segment,
		      const elfcpp::Rela<64, false>&, unsigned int r_type,
		      elfcpp::Elf_types<64>::Elf_Addr value,
		      unsigned char* view,
		      elfcpp::Elf_types<64>::Elf_Addr,
		      section_size_type view_size);

    // Do a TLSDESC-style General-Dynamic to Local-Exec transition.
    inline void
    tls_desc_gd_to_le(const Relocate_info<64, false>*, size_t relnum,
		      Output_segment* tls_segment,
		      const elfcpp::Rela<64, false>&, unsigned int r_type,
		      elfcpp::Elf_types<64>::Elf_Addr value,
		      unsigned char* view,
		      section_size_type view_size);

    // Do a TLS Local-Dynamic to Local-Exec transition.
    inline void
    tls_ld_to_le(const Relocate_info<64, false>*, size_t relnum,
		 Output_segment* tls_segment,
		 const elfcpp::Rela<64, false>&, unsigned int r_type,
		 elfcpp::Elf_types<64>::Elf_Addr value,
		 unsigned char* view,
		 section_size_type view_size);

    // Do a TLS Initial-Exec to Local-Exec transition.
    static inline void
    tls_ie_to_le(const Relocate_info<64, false>*, size_t relnum,
		 Output_segment* tls_segment,
		 const elfcpp::Rela<64, false>&, unsigned int r_type,
		 elfcpp::Elf_types<64>::Elf_Addr value,
		 unsigned char* view,
		 section_size_type view_size);

    // This is set if we should skip the next reloc, which should be a
    // PLT32 reloc against ___tls_get_addr.
    bool skip_call_tls_get_addr_;

    // This is set if we see a relocation which could load the address
    // of the TLS block.  Whether we see such a relocation determines
    // how we handle the R_X86_64_DTPOFF32 relocation, which is used
    // in debugging sections.
    bool saw_tls_block_reloc_;
  };

  // A class which returns the size required for a relocation type,
  // used while scanning relocs during a relocatable link.
  class Relocatable_size_for_reloc
  {
   public:
    unsigned int
    get_size_for_reloc(unsigned int, Relobj*);
  };

  // Adjust TLS relocation type based on the options and whether this
  // is a local symbol.
  static tls::Tls_optimization
  optimize_tls_reloc(bool is_final, int r_type);

  // Get the GOT section, creating it if necessary.
  Output_data_got<64, false>*
  got_section(Symbol_table*, Layout*);

  // Get the GOT PLT section.
  Output_data_space*
  got_plt_section() const
  {
    gold_assert(this->got_plt_ != NULL);
    return this->got_plt_;
  }

  // Create the PLT section.
  void
  make_plt_section(Symbol_table* symtab, Layout* layout);

  // Create a PLT entry for a global symbol.
  void
  make_plt_entry(Symbol_table*, Layout*, Symbol*);

  // Define the _TLS_MODULE_BASE_ symbol at the end of the TLS segment.
  void
  define_tls_base_symbol(Symbol_table*, Layout*);

  // Create the reserved PLT and GOT entries for the TLS descriptor resolver.
  void
  reserve_tlsdesc_entries(Symbol_table* symtab, Layout* layout);

  // Create a GOT entry for the TLS module index.
  unsigned int
  got_mod_index_entry(Symbol_table* symtab, Layout* layout,
		      Sized_relobj<64, false>* object);

  // Get the PLT section.
  Output_data_plt_x86_64*
  plt_section() const
  {
    gold_assert(this->plt_ != NULL);
    return this->plt_;
  }

  // Get the dynamic reloc section, creating it if necessary.
  Reloc_section*
  rela_dyn_section(Layout*);

  // Return true if the symbol may need a COPY relocation.
  // References from an executable object to non-function symbols
  // defined in a dynamic object may need a COPY relocation.
  bool
  may_need_copy_reloc(Symbol* gsym)
  {
    return (!parameters->options().shared()
            && gsym->is_from_dynobj()
            && gsym->type() != elfcpp::STT_FUNC);
  }

  // Add a potential copy relocation.
  void
  copy_reloc(Symbol_table* symtab, Layout* layout,
             Sized_relobj<64, false>* object,
	     unsigned int shndx, Output_section* output_section,
	     Symbol* sym, const elfcpp::Rela<64, false>& reloc)
  {
    this->copy_relocs_.copy_reloc(symtab, layout,
				  symtab->get_sized_symbol<64>(sym),
				  object, shndx, output_section,
				  reloc, this->rela_dyn_section(layout));
  }

  // Information about this specific target which we pass to the
  // general Target structure.
  static const Target::Target_info x86_64_info;

  enum Got_type
  {
    GOT_TYPE_STANDARD = 0,      // GOT entry for a regular symbol
    GOT_TYPE_TLS_OFFSET = 1,    // GOT entry for TLS offset
    GOT_TYPE_TLS_PAIR = 2,      // GOT entry for TLS module/offset pair
    GOT_TYPE_TLS_DESC = 3       // GOT entry for TLS_DESC pair
  };

  // The GOT section.
  Output_data_got<64, false>* got_;
  // The PLT section.
  Output_data_plt_x86_64* plt_;
  // The GOT PLT section.
  Output_data_space* got_plt_;
  // The dynamic reloc section.
  Reloc_section* rela_dyn_;
  // Relocs saved to avoid a COPY reloc.
  Copy_relocs<elfcpp::SHT_RELA, 64, false> copy_relocs_;
  // Space for variables copied with a COPY reloc.
  Output_data_space* dynbss_;
  // Offset of the GOT entry for the TLS module index.
  unsigned int got_mod_index_offset_;
  // True if the _TLS_MODULE_BASE_ symbol has been defined.
  bool tls_base_symbol_defined_;
};

const Target::Target_info Target_x86_64::x86_64_info =
{
  64,			// size
  false,		// is_big_endian
  elfcpp::EM_X86_64,	// machine_code
  false,		// has_make_symbol
  false,		// has_resolve
  true,			// has_code_fill
  true,			// is_default_stack_executable
  '\0',			// wrap_char
  "/lib/ld64.so.1",     // program interpreter
  0x400000,		// default_text_segment_address
  0x1000,		// abi_pagesize (overridable by -z max-page-size)
  0x1000		// common_pagesize (overridable by -z common-page-size)
};

// Get the GOT section, creating it if necessary.

Output_data_got<64, false>*
Target_x86_64::got_section(Symbol_table* symtab, Layout* layout)
{
  if (this->got_ == NULL)
    {
      gold_assert(symtab != NULL && layout != NULL);

      this->got_ = new Output_data_got<64, false>();

      Output_section* os;
      os = layout->add_output_section_data(".got", elfcpp::SHT_PROGBITS,
					   (elfcpp::SHF_ALLOC
					    | elfcpp::SHF_WRITE),
					   this->got_);
      os->set_is_relro();

      // The old GNU linker creates a .got.plt section.  We just
      // create another set of data in the .got section.  Note that we
      // always create a PLT if we create a GOT, although the PLT
      // might be empty.
      this->got_plt_ = new Output_data_space(8, "** GOT PLT");
      os = layout->add_output_section_data(".got", elfcpp::SHT_PROGBITS,
					   (elfcpp::SHF_ALLOC
					    | elfcpp::SHF_WRITE),
					   this->got_plt_);
      os->set_is_relro();

      // The first three entries are reserved.
      this->got_plt_->set_current_data_size(3 * 8);

      // Define _GLOBAL_OFFSET_TABLE_ at the start of the PLT.
      symtab->define_in_output_data("_GLOBAL_OFFSET_TABLE_", NULL,
				    this->got_plt_,
				    0, 0, elfcpp::STT_OBJECT,
				    elfcpp::STB_LOCAL,
				    elfcpp::STV_HIDDEN, 0,
				    false, false);
    }

  return this->got_;
}

// Get the dynamic reloc section, creating it if necessary.

Target_x86_64::Reloc_section*
Target_x86_64::rela_dyn_section(Layout* layout)
{
  if (this->rela_dyn_ == NULL)
    {
      gold_assert(layout != NULL);
      this->rela_dyn_ = new Reloc_section(parameters->options().combreloc());
      layout->add_output_section_data(".rela.dyn", elfcpp::SHT_RELA,
				      elfcpp::SHF_ALLOC, this->rela_dyn_);
    }
  return this->rela_dyn_;
}

// A class to handle the PLT data.

class Output_data_plt_x86_64 : public Output_section_data
{
 public:
  typedef Output_data_reloc<elfcpp::SHT_RELA, true, 64, false> Reloc_section;

  Output_data_plt_x86_64(Layout*, Output_data_got<64, false>*,
                         Output_data_space*);

  // Add an entry to the PLT.
  void
  add_entry(Symbol* gsym);

  // Add the reserved TLSDESC_PLT entry to the PLT.
  void
  reserve_tlsdesc_entry(unsigned int got_offset)
  { this->tlsdesc_got_offset_ = got_offset; }

  // Return true if a TLSDESC_PLT entry has been reserved.
  bool
  has_tlsdesc_entry() const
  { return this->tlsdesc_got_offset_ != -1U; }

  // Return the GOT offset for the reserved TLSDESC_PLT entry.
  unsigned int
  get_tlsdesc_got_offset() const
  { return this->tlsdesc_got_offset_; }

  // Return the offset of the reserved TLSDESC_PLT entry.
  unsigned int
  get_tlsdesc_plt_offset() const
  { return (this->count_ + 1) * plt_entry_size; }

  // Return the .rel.plt section data.
  const Reloc_section*
  rel_plt() const
  { return this->rel_; }

 protected:
  void
  do_adjust_output_section(Output_section* os);

  // Write to a map file.
  void
  do_print_to_mapfile(Mapfile* mapfile) const
  { mapfile->print_output_data(this, _("** PLT")); }

 private:
  // The size of an entry in the PLT.
  static const int plt_entry_size = 16;

  // The first entry in the PLT.
  // From the AMD64 ABI: "Unlike Intel386 ABI, this ABI uses the same
  // procedure linkage table for both programs and shared objects."
  static unsigned char first_plt_entry[plt_entry_size];

  // Other entries in the PLT for an executable.
  static unsigned char plt_entry[plt_entry_size];

  // The reserved TLSDESC entry in the PLT for an executable.
  static unsigned char tlsdesc_plt_entry[plt_entry_size];

  // Set the final size.
  void
  set_final_data_size();

  // Write out the PLT data.
  void
  do_write(Output_file*);

  // The reloc section.
  Reloc_section* rel_;
  // The .got section.
  Output_data_got<64, false>* got_;
  // The .got.plt section.
  Output_data_space* got_plt_;
  // The number of PLT entries.
  unsigned int count_;
  // Offset of the reserved TLSDESC_GOT entry when needed.
  unsigned int tlsdesc_got_offset_;
};

// Create the PLT section.  The ordinary .got section is an argument,
// since we need to refer to the start.  We also create our own .got
// section just for PLT entries.

Output_data_plt_x86_64::Output_data_plt_x86_64(Layout* layout,
                                               Output_data_got<64, false>* got,
                                               Output_data_space* got_plt)
  : Output_section_data(8), got_(got), got_plt_(got_plt), count_(0),
    tlsdesc_got_offset_(-1U)
{
  this->rel_ = new Reloc_section(false);
  layout->add_output_section_data(".rela.plt", elfcpp::SHT_RELA,
				  elfcpp::SHF_ALLOC, this->rel_);
}

void
Output_data_plt_x86_64::do_adjust_output_section(Output_section* os)
{
  // UnixWare sets the entsize of .plt to 4, and so does the old GNU
  // linker, and so do we.
  os->set_entsize(4);
}

// Add an entry to the PLT.

void
Output_data_plt_x86_64::add_entry(Symbol* gsym)
{
  gold_assert(!gsym->has_plt_offset());

  // Note that when setting the PLT offset we skip the initial
  // reserved PLT entry.
  gsym->set_plt_offset((this->count_ + 1) * plt_entry_size);

  ++this->count_;

  section_offset_type got_offset = this->got_plt_->current_data_size();

  // Every PLT entry needs a GOT entry which points back to the PLT
  // entry (this will be changed by the dynamic linker, normally
  // lazily when the function is called).
  this->got_plt_->set_current_data_size(got_offset + 8);

  // Every PLT entry needs a reloc.
  gsym->set_needs_dynsym_entry();
  this->rel_->add_global(gsym, elfcpp::R_X86_64_JUMP_SLOT, this->got_plt_,
			 got_offset, 0);

  // Note that we don't need to save the symbol.  The contents of the
  // PLT are independent of which symbols are used.  The symbols only
  // appear in the relocations.
}

// Set the final size.
void
Output_data_plt_x86_64::set_final_data_size()
{
  unsigned int count = this->count_;
  if (this->has_tlsdesc_entry())
    ++count;
  this->set_data_size((count + 1) * plt_entry_size);
}

// The first entry in the PLT for an executable.

unsigned char Output_data_plt_x86_64::first_plt_entry[plt_entry_size] =
{
  // From AMD64 ABI Draft 0.98, page 76
  0xff, 0x35,	// pushq contents of memory address
  0, 0, 0, 0,	// replaced with address of .got + 8
  0xff, 0x25,	// jmp indirect
  0, 0, 0, 0,	// replaced with address of .got + 16
  0x90, 0x90, 0x90, 0x90   // noop (x4)
};

// Subsequent entries in the PLT for an executable.

unsigned char Output_data_plt_x86_64::plt_entry[plt_entry_size] =
{
  // From AMD64 ABI Draft 0.98, page 76
  0xff, 0x25,	// jmpq indirect
  0, 0, 0, 0,	// replaced with address of symbol in .got
  0x68,		// pushq immediate
  0, 0, 0, 0,	// replaced with offset into relocation table
  0xe9,		// jmpq relative
  0, 0, 0, 0	// replaced with offset to start of .plt
};

// The reserved TLSDESC entry in the PLT for an executable.

unsigned char Output_data_plt_x86_64::tlsdesc_plt_entry[plt_entry_size] =
{
  // From Alexandre Oliva, "Thread-Local Storage Descriptors for IA32
  // and AMD64/EM64T", Version 0.9.4 (2005-10-10).
  0xff, 0x35,	// pushq x(%rip)
  0, 0, 0, 0,	// replaced with address of linkmap GOT entry (at PLTGOT + 8)
  0xff,	0x25,	// jmpq *y(%rip)
  0, 0, 0, 0,	// replaced with offset of reserved TLSDESC_GOT entry
  0x0f,	0x1f,	// nop
  0x40, 0
};

// Write out the PLT.  This uses the hand-coded instructions above,
// and adjusts them as needed.  This is specified by the AMD64 ABI.

void
Output_data_plt_x86_64::do_write(Output_file* of)
{
  const off_t offset = this->offset();
  const section_size_type oview_size =
    convert_to_section_size_type(this->data_size());
  unsigned char* const oview = of->get_output_view(offset, oview_size);

  const off_t got_file_offset = this->got_plt_->offset();
  const section_size_type got_size =
    convert_to_section_size_type(this->got_plt_->data_size());
  unsigned char* const got_view = of->get_output_view(got_file_offset,
						      got_size);

  unsigned char* pov = oview;

  // The base address of the .plt section.
  elfcpp::Elf_types<64>::Elf_Addr plt_address = this->address();
  // The base address of the .got section.
  elfcpp::Elf_types<64>::Elf_Addr got_base = this->got_->address();
  // The base address of the PLT portion of the .got section,
  // which is where the GOT pointer will point, and where the
  // three reserved GOT entries are located.
  elfcpp::Elf_types<64>::Elf_Addr got_address = this->got_plt_->address();

  memcpy(pov, first_plt_entry, plt_entry_size);
  // We do a jmp relative to the PC at the end of this instruction.
  elfcpp::Swap_unaligned<32, false>::writeval(pov + 2,
					      (got_address + 8
					       - (plt_address + 6)));
  elfcpp::Swap<32, false>::writeval(pov + 8,
				    (got_address + 16
				     - (plt_address + 12)));
  pov += plt_entry_size;

  unsigned char* got_pov = got_view;

  memset(got_pov, 0, 24);
  got_pov += 24;

  unsigned int plt_offset = plt_entry_size;
  unsigned int got_offset = 24;
  const unsigned int count = this->count_;
  for (unsigned int plt_index = 0;
       plt_index < count;
       ++plt_index,
	 pov += plt_entry_size,
	 got_pov += 8,
	 plt_offset += plt_entry_size,
	 got_offset += 8)
    {
      // Set and adjust the PLT entry itself.
      memcpy(pov, plt_entry, plt_entry_size);
      elfcpp::Swap_unaligned<32, false>::writeval(pov + 2,
						  (got_address + got_offset
						   - (plt_address + plt_offset
						      + 6)));

      elfcpp::Swap_unaligned<32, false>::writeval(pov + 7, plt_index);
      elfcpp::Swap<32, false>::writeval(pov + 12,
					- (plt_offset + plt_entry_size));

      // Set the entry in the GOT.
      elfcpp::Swap<64, false>::writeval(got_pov, plt_address + plt_offset + 6);
    }

  if (this->has_tlsdesc_entry())
    {
      // Set and adjust the reserved TLSDESC PLT entry.
      unsigned int tlsdesc_got_offset = this->get_tlsdesc_got_offset();
      memcpy(pov, tlsdesc_plt_entry, plt_entry_size);
      elfcpp::Swap_unaligned<32, false>::writeval(pov + 2,
						  (got_address + 8
						   - (plt_address + plt_offset
						      + 6)));
      elfcpp::Swap_unaligned<32, false>::writeval(pov + 8,
						  (got_base
						   + tlsdesc_got_offset
						   - (plt_address + plt_offset
						      + 12)));
      pov += plt_entry_size;
    }

  gold_assert(static_cast<section_size_type>(pov - oview) == oview_size);
  gold_assert(static_cast<section_size_type>(got_pov - got_view) == got_size);

  of->write_output_view(offset, oview_size, oview);
  of->write_output_view(got_file_offset, got_size, got_view);
}

// Create the PLT section.

void
Target_x86_64::make_plt_section(Symbol_table* symtab, Layout* layout)
{
  if (this->plt_ == NULL)
    {
      // Create the GOT sections first.
      this->got_section(symtab, layout);

      this->plt_ = new Output_data_plt_x86_64(layout, this->got_,
                                              this->got_plt_);
      layout->add_output_section_data(".plt", elfcpp::SHT_PROGBITS,
				      (elfcpp::SHF_ALLOC
				       | elfcpp::SHF_EXECINSTR),
				      this->plt_);
    }
}

// Create a PLT entry for a global symbol.

void
Target_x86_64::make_plt_entry(Symbol_table* symtab, Layout* layout,
                              Symbol* gsym)
{
  if (gsym->has_plt_offset())
    return;

  if (this->plt_ == NULL)
    this->make_plt_section(symtab, layout);

  this->plt_->add_entry(gsym);
}

// Define the _TLS_MODULE_BASE_ symbol at the end of the TLS segment.

void
Target_x86_64::define_tls_base_symbol(Symbol_table* symtab, Layout* layout)
{
  if (this->tls_base_symbol_defined_)
    return;

  Output_segment* tls_segment = layout->tls_segment();
  if (tls_segment != NULL)
    {
      symtab->define_in_output_segment("_TLS_MODULE_BASE_", NULL,
				       tls_segment, 0, 0,
				       elfcpp::STT_TLS,
				       elfcpp::STB_LOCAL,
				       elfcpp::STV_HIDDEN, 0,
				       Symbol::SEGMENT_END, true);
    }
  this->tls_base_symbol_defined_ = true;
}

// Create the reserved PLT and GOT entries for the TLS descriptor resolver.

void
Target_x86_64::reserve_tlsdesc_entries(Symbol_table* symtab,
                                             Layout* layout)
{
  if (this->plt_ == NULL)
    this->make_plt_section(symtab, layout);

  if (!this->plt_->has_tlsdesc_entry())
    {
      // Allocate the TLSDESC_GOT entry.
      Output_data_got<64, false>* got = this->got_section(symtab, layout);
      unsigned int got_offset = got->add_constant(0);

      // Allocate the TLSDESC_PLT entry.
      this->plt_->reserve_tlsdesc_entry(got_offset);
    }
}

// Create a GOT entry for the TLS module index.

unsigned int
Target_x86_64::got_mod_index_entry(Symbol_table* symtab, Layout* layout,
			           Sized_relobj<64, false>* object)
{
  if (this->got_mod_index_offset_ == -1U)
    {
      gold_assert(symtab != NULL && layout != NULL && object != NULL);
      Reloc_section* rela_dyn = this->rela_dyn_section(layout);
      Output_data_got<64, false>* got = this->got_section(symtab, layout);
      unsigned int got_offset = got->add_constant(0);
      rela_dyn->add_local(object, 0, elfcpp::R_X86_64_DTPMOD64, got,
                          got_offset, 0);
      got->add_constant(0);
      this->got_mod_index_offset_ = got_offset;
    }
  return this->got_mod_index_offset_;
}

// Optimize the TLS relocation type based on what we know about the
// symbol.  IS_FINAL is true if the final address of this symbol is
// known at link time.

tls::Tls_optimization
Target_x86_64::optimize_tls_reloc(bool is_final, int r_type)
{
  // If we are generating a shared library, then we can't do anything
  // in the linker.
  if (parameters->options().shared())
    return tls::TLSOPT_NONE;

  switch (r_type)
    {
    case elfcpp::R_X86_64_TLSGD:
    case elfcpp::R_X86_64_GOTPC32_TLSDESC:
    case elfcpp::R_X86_64_TLSDESC_CALL:
      // These are General-Dynamic which permits fully general TLS
      // access.  Since we know that we are generating an executable,
      // we can convert this to Initial-Exec.  If we also know that
      // this is a local symbol, we can further switch to Local-Exec.
      if (is_final)
	return tls::TLSOPT_TO_LE;
      return tls::TLSOPT_TO_IE;

    case elfcpp::R_X86_64_TLSLD:
      // This is Local-Dynamic, which refers to a local symbol in the
      // dynamic TLS block.  Since we know that we generating an
      // executable, we can switch to Local-Exec.
      return tls::TLSOPT_TO_LE;

    case elfcpp::R_X86_64_DTPOFF32:
    case elfcpp::R_X86_64_DTPOFF64:
      // Another Local-Dynamic reloc.
      return tls::TLSOPT_TO_LE;

    case elfcpp::R_X86_64_GOTTPOFF:
      // These are Initial-Exec relocs which get the thread offset
      // from the GOT.  If we know that we are linking against the
      // local symbol, we can switch to Local-Exec, which links the
      // thread offset into the instruction.
      if (is_final)
	return tls::TLSOPT_TO_LE;
      return tls::TLSOPT_NONE;

    case elfcpp::R_X86_64_TPOFF32:
      // When we already have Local-Exec, there is nothing further we
      // can do.
      return tls::TLSOPT_NONE;

    default:
      gold_unreachable();
    }
}

// Report an unsupported relocation against a local symbol.

void
Target_x86_64::Scan::unsupported_reloc_local(Sized_relobj<64, false>* object,
                                             unsigned int r_type)
{
  gold_error(_("%s: unsupported reloc %u against local symbol"),
	     object->name().c_str(), r_type);
}

// We are about to emit a dynamic relocation of type R_TYPE.  If the
// dynamic linker does not support it, issue an error.  The GNU linker
// only issues a non-PIC error for an allocated read-only section.
// Here we know the section is allocated, but we don't know that it is
// read-only.  But we check for all the relocation types which the
// glibc dynamic linker supports, so it seems appropriate to issue an
// error even if the section is not read-only.

void
Target_x86_64::Scan::check_non_pic(Relobj* object, unsigned int r_type)
{
  switch (r_type)
    {
      // These are the relocation types supported by glibc for x86_64.
    case elfcpp::R_X86_64_RELATIVE:
    case elfcpp::R_X86_64_GLOB_DAT:
    case elfcpp::R_X86_64_JUMP_SLOT:
    case elfcpp::R_X86_64_DTPMOD64:
    case elfcpp::R_X86_64_DTPOFF64:
    case elfcpp::R_X86_64_TPOFF64:
    case elfcpp::R_X86_64_64:
    case elfcpp::R_X86_64_32:
    case elfcpp::R_X86_64_PC32:
    case elfcpp::R_X86_64_COPY:
      return;

    default:
      // This prevents us from issuing more than one error per reloc
      // section.  But we can still wind up issuing more than one
      // error per object file.
      if (this->issued_non_pic_error_)
        return;
      object->error(_("requires unsupported dynamic reloc; "
                      "recompile with -fPIC"));
      this->issued_non_pic_error_ = true;
      return;

    case elfcpp::R_X86_64_NONE:
      gold_unreachable();
    }
}

// Scan a relocation for a local symbol.

inline void
Target_x86_64::Scan::local(const General_options&,
                           Symbol_table* symtab,
                           Layout* layout,
                           Target_x86_64* target,
                           Sized_relobj<64, false>* object,
                           unsigned int data_shndx,
                           Output_section* output_section,
                           const elfcpp::Rela<64, false>& reloc,
                           unsigned int r_type,
                           const elfcpp::Sym<64, false>& lsym)
{
  switch (r_type)
    {
    case elfcpp::R_X86_64_NONE:
    case elfcpp::R_386_GNU_VTINHERIT:
    case elfcpp::R_386_GNU_VTENTRY:
      break;

    case elfcpp::R_X86_64_64:
      // If building a shared library (or a position-independent
      // executable), we need to create a dynamic relocation for this
      // location.  The relocation applied at link time will apply the
      // link-time value, so we flag the location with an
      // R_X86_64_RELATIVE relocation so the dynamic loader can
      // relocate it easily.
      if (parameters->options().output_is_position_independent())
        {
          unsigned int r_sym = elfcpp::elf_r_sym<64>(reloc.get_r_info());
          Reloc_section* rela_dyn = target->rela_dyn_section(layout);
          rela_dyn->add_local_relative(object, r_sym,
                                       elfcpp::R_X86_64_RELATIVE,
                                       output_section, data_shndx,
                                       reloc.get_r_offset(),
                                       reloc.get_r_addend());
        }
      break;

    case elfcpp::R_X86_64_32:
    case elfcpp::R_X86_64_32S:
    case elfcpp::R_X86_64_16:
    case elfcpp::R_X86_64_8:
      // If building a shared library (or a position-independent
      // executable), we need to create a dynamic relocation for this
      // location.  We can't use an R_X86_64_RELATIVE relocation
      // because that is always a 64-bit relocation.
      if (parameters->options().output_is_position_independent())
        {
          this->check_non_pic(object, r_type);

          Reloc_section* rela_dyn = target->rela_dyn_section(layout);
	  unsigned int r_sym = elfcpp::elf_r_sym<64>(reloc.get_r_info());
          if (lsym.get_st_type() != elfcpp::STT_SECTION)
	    rela_dyn->add_local(object, r_sym, r_type, output_section,
				data_shndx, reloc.get_r_offset(),
				reloc.get_r_addend());
          else
            {
              gold_assert(lsym.get_st_value() == 0);
	      unsigned int shndx = lsym.get_st_shndx();
	      bool is_ordinary;
	      shndx = object->adjust_sym_shndx(r_sym, shndx,
					       &is_ordinary);
	      if (!is_ordinary)
		object->error(_("section symbol %u has bad shndx %u"),
			      r_sym, shndx);
	      else
		rela_dyn->add_local_section(object, shndx,
					    r_type, output_section,
					    data_shndx, reloc.get_r_offset(),
					    reloc.get_r_addend());
            }
        }
      break;

    case elfcpp::R_X86_64_PC64:
    case elfcpp::R_X86_64_PC32:
    case elfcpp::R_X86_64_PC16:
    case elfcpp::R_X86_64_PC8:
      break;

    case elfcpp::R_X86_64_PLT32:
      // Since we know this is a local symbol, we can handle this as a
      // PC32 reloc.
      break;

    case elfcpp::R_X86_64_GOTPC32:
    case elfcpp::R_X86_64_GOTOFF64:
    case elfcpp::R_X86_64_GOTPC64:
    case elfcpp::R_X86_64_PLTOFF64:
      // We need a GOT section.
      target->got_section(symtab, layout);
      // For PLTOFF64, we'd normally want a PLT section, but since we
      // know this is a local symbol, no PLT is needed.
      break;

    case elfcpp::R_X86_64_GOT64:
    case elfcpp::R_X86_64_GOT32:
    case elfcpp::R_X86_64_GOTPCREL64:
    case elfcpp::R_X86_64_GOTPCREL:
    case elfcpp::R_X86_64_GOTPLT64:
      {
        // The symbol requires a GOT entry.
        Output_data_got<64, false>* got = target->got_section(symtab, layout);
        unsigned int r_sym = elfcpp::elf_r_sym<64>(reloc.get_r_info());
        if (got->add_local(object, r_sym, GOT_TYPE_STANDARD))
          {
            // If we are generating a shared object, we need to add a
            // dynamic relocation for this symbol's GOT entry.
            if (parameters->options().output_is_position_independent())
              {
                Reloc_section* rela_dyn = target->rela_dyn_section(layout);
		// R_X86_64_RELATIVE assumes a 64-bit relocation.
		if (r_type != elfcpp::R_X86_64_GOT32)
                  rela_dyn->add_local_relative(
                      object, r_sym, elfcpp::R_X86_64_RELATIVE, got,
                      object->local_got_offset(r_sym, GOT_TYPE_STANDARD), 0);
                else
                  {
                    this->check_non_pic(object, r_type);

                    gold_assert(lsym.get_st_type() != elfcpp::STT_SECTION);
                    rela_dyn->add_local(
                        object, r_sym, r_type, got,
                        object->local_got_offset(r_sym, GOT_TYPE_STANDARD), 0);
                  }
              }
          }
        // For GOTPLT64, we'd normally want a PLT section, but since
        // we know this is a local symbol, no PLT is needed.
      }
      break;

    case elfcpp::R_X86_64_COPY:
    case elfcpp::R_X86_64_GLOB_DAT:
    case elfcpp::R_X86_64_JUMP_SLOT:
    case elfcpp::R_X86_64_RELATIVE:
      // These are outstanding tls relocs, which are unexpected when linking
    case elfcpp::R_X86_64_TPOFF64:
    case elfcpp::R_X86_64_DTPMOD64:
    case elfcpp::R_X86_64_TLSDESC:
      gold_error(_("%s: unexpected reloc %u in object file"),
		 object->name().c_str(), r_type);
      break;

      // These are initial tls relocs, which are expected when linking
    case elfcpp::R_X86_64_TLSGD:            // Global-dynamic
    case elfcpp::R_X86_64_GOTPC32_TLSDESC:  // Global-dynamic (from ~oliva url)
    case elfcpp::R_X86_64_TLSDESC_CALL:
    case elfcpp::R_X86_64_TLSLD:            // Local-dynamic
    case elfcpp::R_X86_64_DTPOFF32:
    case elfcpp::R_X86_64_DTPOFF64:
    case elfcpp::R_X86_64_GOTTPOFF:         // Initial-exec
    case elfcpp::R_X86_64_TPOFF32:          // Local-exec
      {
	bool output_is_shared = parameters->options().shared();
	const tls::Tls_optimization optimized_type
            = Target_x86_64::optimize_tls_reloc(!output_is_shared, r_type);
	switch (r_type)
	  {
          case elfcpp::R_X86_64_TLSGD:       // General-dynamic
            if (optimized_type == tls::TLSOPT_NONE)
              {
                // Create a pair of GOT entries for the module index and
                // dtv-relative offset.
                Output_data_got<64, false>* got
                    = target->got_section(symtab, layout);
                unsigned int r_sym = elfcpp::elf_r_sym<64>(reloc.get_r_info());
		unsigned int shndx = lsym.get_st_shndx();
		bool is_ordinary;
		shndx = object->adjust_sym_shndx(r_sym, shndx, &is_ordinary);
		if (!is_ordinary)
		  object->error(_("local symbol %u has bad shndx %u"),
			      r_sym, shndx);
                else
		  got->add_local_pair_with_rela(object, r_sym,
						shndx,
						GOT_TYPE_TLS_PAIR,
						target->rela_dyn_section(layout),
						elfcpp::R_X86_64_DTPMOD64, 0);
              }
            else if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_local(object, r_type);
            break;

          case elfcpp::R_X86_64_GOTPC32_TLSDESC:
            target->define_tls_base_symbol(symtab, layout);
	    if (optimized_type == tls::TLSOPT_NONE)
	      {
	        // Create reserved PLT and GOT entries for the resolver.
	        target->reserve_tlsdesc_entries(symtab, layout);

	        // Generate a double GOT entry with an R_X86_64_TLSDESC reloc.
                Output_data_got<64, false>* got
                    = target->got_section(symtab, layout);
                unsigned int r_sym = elfcpp::elf_r_sym<64>(reloc.get_r_info());
		unsigned int shndx = lsym.get_st_shndx();
		bool is_ordinary;
		shndx = object->adjust_sym_shndx(r_sym, shndx, &is_ordinary);
		if (!is_ordinary)
		  object->error(_("local symbol %u has bad shndx %u"),
			      r_sym, shndx);
                else
		  got->add_local_pair_with_rela(object, r_sym,
						shndx,
						GOT_TYPE_TLS_DESC,
						target->rela_dyn_section(layout),
						elfcpp::R_X86_64_TLSDESC, 0);
	      }
	    else if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_local(object, r_type);
	    break;

          case elfcpp::R_X86_64_TLSDESC_CALL:
	    break;

          case elfcpp::R_X86_64_TLSLD:       // Local-dynamic
	    if (optimized_type == tls::TLSOPT_NONE)
	      {
	        // Create a GOT entry for the module index.
	        target->got_mod_index_entry(symtab, layout, object);
	      }
	    else if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_local(object, r_type);
	    break;

          case elfcpp::R_X86_64_DTPOFF32:
          case elfcpp::R_X86_64_DTPOFF64:
	    break;

          case elfcpp::R_X86_64_GOTTPOFF:    // Initial-exec
	    layout->set_has_static_tls();
            if (optimized_type == tls::TLSOPT_NONE)
              {
	        // Create a GOT entry for the tp-relative offset.
	        Output_data_got<64, false>* got
	            = target->got_section(symtab, layout);
	        unsigned int r_sym = elfcpp::elf_r_sym<64>(reloc.get_r_info());
	        got->add_local_with_rela(object, r_sym, GOT_TYPE_TLS_OFFSET,
	                                 target->rela_dyn_section(layout),
	                                 elfcpp::R_X86_64_TPOFF64);
              }
            else if (optimized_type != tls::TLSOPT_TO_LE)
              unsupported_reloc_local(object, r_type);
            break;

          case elfcpp::R_X86_64_TPOFF32:     // Local-exec
	    layout->set_has_static_tls();
            if (output_is_shared)
              unsupported_reloc_local(object, r_type);
	    break;

          default:
            gold_unreachable();
	  }
      }
      break;

    case elfcpp::R_X86_64_SIZE32:
    case elfcpp::R_X86_64_SIZE64:
    default:
      gold_error(_("%s: unsupported reloc %u against local symbol"),
		 object->name().c_str(), r_type);
      break;
    }
}


// Report an unsupported relocation against a global symbol.

void
Target_x86_64::Scan::unsupported_reloc_global(Sized_relobj<64, false>* object,
                                              unsigned int r_type,
                                              Symbol* gsym)
{
  gold_error(_("%s: unsupported reloc %u against global symbol %s"),
	     object->name().c_str(), r_type, gsym->demangled_name().c_str());
}

// Scan a relocation for a global symbol.

inline void
Target_x86_64::Scan::global(const General_options&,
                            Symbol_table* symtab,
                            Layout* layout,
                            Target_x86_64* target,
                            Sized_relobj<64, false>* object,
                            unsigned int data_shndx,
                            Output_section* output_section,
                            const elfcpp::Rela<64, false>& reloc,
                            unsigned int r_type,
                            Symbol* gsym)
{
  switch (r_type)
    {
    case elfcpp::R_X86_64_NONE:
    case elfcpp::R_386_GNU_VTINHERIT:
    case elfcpp::R_386_GNU_VTENTRY:
      break;

    case elfcpp::R_X86_64_64:
    case elfcpp::R_X86_64_32:
    case elfcpp::R_X86_64_32S:
    case elfcpp::R_X86_64_16:
    case elfcpp::R_X86_64_8:
      {
        // Make a PLT entry if necessary.
        if (gsym->needs_plt_entry())
          {
            target->make_plt_entry(symtab, layout, gsym);
            // Since this is not a PC-relative relocation, we may be
            // taking the address of a function. In that case we need to
            // set the entry in the dynamic symbol table to the address of
            // the PLT entry.
            if (gsym->is_from_dynobj() && !parameters->options().shared())
              gsym->set_needs_dynsym_value();
          }
        // Make a dynamic relocation if necessary.
        if (gsym->needs_dynamic_reloc(Symbol::ABSOLUTE_REF))
          {
            if (target->may_need_copy_reloc(gsym))
              {
                target->copy_reloc(symtab, layout, object,
                                   data_shndx, output_section, gsym, reloc);
              }
            else if (r_type == elfcpp::R_X86_64_64
                     && gsym->can_use_relative_reloc(false))
              {
                Reloc_section* rela_dyn = target->rela_dyn_section(layout);
                rela_dyn->add_global_relative(gsym, elfcpp::R_X86_64_RELATIVE,
                                              output_section, object,
                                              data_shndx, reloc.get_r_offset(),
                                              reloc.get_r_addend());
              }
            else
              {
                this->check_non_pic(object, r_type);
                Reloc_section* rela_dyn = target->rela_dyn_section(layout);
                rela_dyn->add_global(gsym, r_type, output_section, object,
                                     data_shndx, reloc.get_r_offset(),
                                     reloc.get_r_addend());
              }
          }
      }
      break;

    case elfcpp::R_X86_64_PC64:
    case elfcpp::R_X86_64_PC32:
    case elfcpp::R_X86_64_PC16:
    case elfcpp::R_X86_64_PC8:
      {
        // Make a PLT entry if necessary.
        if (gsym->needs_plt_entry())
          target->make_plt_entry(symtab, layout, gsym);
        // Make a dynamic relocation if necessary.
        int flags = Symbol::NON_PIC_REF;
        if (gsym->type() == elfcpp::STT_FUNC)
          flags |= Symbol::FUNCTION_CALL;
        if (gsym->needs_dynamic_reloc(flags))
          {
            if (target->may_need_copy_reloc(gsym))
              {
                target->copy_reloc(symtab, layout, object,
                                   data_shndx, output_section, gsym, reloc);
              }
            else
              {
                this->check_non_pic(object, r_type);
                Reloc_section* rela_dyn = target->rela_dyn_section(layout);
                rela_dyn->add_global(gsym, r_type, output_section, object,
                                     data_shndx, reloc.get_r_offset(),
                                     reloc.get_r_addend());
              }
          }
      }
      break;

    case elfcpp::R_X86_64_GOT64:
    case elfcpp::R_X86_64_GOT32:
    case elfcpp::R_X86_64_GOTPCREL64:
    case elfcpp::R_X86_64_GOTPCREL:
    case elfcpp::R_X86_64_GOTPLT64:
      {
        // The symbol requires a GOT entry.
        Output_data_got<64, false>* got = target->got_section(symtab, layout);
        if (gsym->final_value_is_known())
          got->add_global(gsym, GOT_TYPE_STANDARD);
        else
          {
            // If this symbol is not fully resolved, we need to add a
            // dynamic relocation for it.
            Reloc_section* rela_dyn = target->rela_dyn_section(layout);
            if (gsym->is_from_dynobj()
                || gsym->is_undefined()
                || gsym->is_preemptible())
              got->add_global_with_rela(gsym, GOT_TYPE_STANDARD, rela_dyn,
                                        elfcpp::R_X86_64_GLOB_DAT);
            else
              {
                if (got->add_global(gsym, GOT_TYPE_STANDARD))
                  rela_dyn->add_global_relative(
                      gsym, elfcpp::R_X86_64_RELATIVE, got,
                      gsym->got_offset(GOT_TYPE_STANDARD), 0);
              }
          }
        // For GOTPLT64, we also need a PLT entry (but only if the
        // symbol is not fully resolved).
        if (r_type == elfcpp::R_X86_64_GOTPLT64
            && !gsym->final_value_is_known())
          target->make_plt_entry(symtab, layout, gsym);
      }
      break;

    case elfcpp::R_X86_64_PLT32:
      // If the symbol is fully resolved, this is just a PC32 reloc.
      // Otherwise we need a PLT entry.
      if (gsym->final_value_is_known())
	break;
      // If building a shared library, we can also skip the PLT entry
      // if the symbol is defined in the output file and is protected
      // or hidden.
      if (gsym->is_defined()
          && !gsym->is_from_dynobj()
          && !gsym->is_preemptible())
	break;
      target->make_plt_entry(symtab, layout, gsym);
      break;

    case elfcpp::R_X86_64_GOTPC32:
    case elfcpp::R_X86_64_GOTOFF64:
    case elfcpp::R_X86_64_GOTPC64:
    case elfcpp::R_X86_64_PLTOFF64:
      // We need a GOT section.
      target->got_section(symtab, layout);
      // For PLTOFF64, we also need a PLT entry (but only if the
      // symbol is not fully resolved).
      if (r_type == elfcpp::R_X86_64_PLTOFF64
	  && !gsym->final_value_is_known())
	target->make_plt_entry(symtab, layout, gsym);
      break;

    case elfcpp::R_X86_64_COPY:
    case elfcpp::R_X86_64_GLOB_DAT:
    case elfcpp::R_X86_64_JUMP_SLOT:
    case elfcpp::R_X86_64_RELATIVE:
      // These are outstanding tls relocs, which are unexpected when linking
    case elfcpp::R_X86_64_TPOFF64:
    case elfcpp::R_X86_64_DTPMOD64:
    case elfcpp::R_X86_64_TLSDESC:
      gold_error(_("%s: unexpected reloc %u in object file"),
		 object->name().c_str(), r_type);
      break;

      // These are initial tls relocs, which are expected for global()
    case elfcpp::R_X86_64_TLSGD:            // Global-dynamic
    case elfcpp::R_X86_64_GOTPC32_TLSDESC:  // Global-dynamic (from ~oliva url)
    case elfcpp::R_X86_64_TLSDESC_CALL:
    case elfcpp::R_X86_64_TLSLD:            // Local-dynamic
    case elfcpp::R_X86_64_DTPOFF32:
    case elfcpp::R_X86_64_DTPOFF64:
    case elfcpp::R_X86_64_GOTTPOFF:         // Initial-exec
    case elfcpp::R_X86_64_TPOFF32:          // Local-exec
      {
	const bool is_final = gsym->final_value_is_known();
	const tls::Tls_optimization optimized_type
            = Target_x86_64::optimize_tls_reloc(is_final, r_type);
	switch (r_type)
	  {
          case elfcpp::R_X86_64_TLSGD:       // General-dynamic
	    if (optimized_type == tls::TLSOPT_NONE)
	      {
                // Create a pair of GOT entries for the module index and
                // dtv-relative offset.
                Output_data_got<64, false>* got
                    = target->got_section(symtab, layout);
                got->add_global_pair_with_rela(gsym, GOT_TYPE_TLS_PAIR,
                                               target->rela_dyn_section(layout),
                                               elfcpp::R_X86_64_DTPMOD64,
                                               elfcpp::R_X86_64_DTPOFF64);
	      }
	    else if (optimized_type == tls::TLSOPT_TO_IE)
	      {
                // Create a GOT entry for the tp-relative offset.
                Output_data_got<64, false>* got
                    = target->got_section(symtab, layout);
                got->add_global_with_rela(gsym, GOT_TYPE_TLS_OFFSET,
                                          target->rela_dyn_section(layout),
                                          elfcpp::R_X86_64_TPOFF64);
	      }
	    else if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_global(object, r_type, gsym);
	    break;

          case elfcpp::R_X86_64_GOTPC32_TLSDESC:
            target->define_tls_base_symbol(symtab, layout);
	    if (optimized_type == tls::TLSOPT_NONE)
	      {
	        // Create reserved PLT and GOT entries for the resolver.
	        target->reserve_tlsdesc_entries(symtab, layout);

	        // Create a double GOT entry with an R_X86_64_TLSDESC reloc.
                Output_data_got<64, false>* got
                    = target->got_section(symtab, layout);
                got->add_global_pair_with_rela(gsym, GOT_TYPE_TLS_DESC,
                                               target->rela_dyn_section(layout),
                                               elfcpp::R_X86_64_TLSDESC, 0);
	      }
	    else if (optimized_type == tls::TLSOPT_TO_IE)
	      {
	        // Create a GOT entry for the tp-relative offset.
                Output_data_got<64, false>* got
                    = target->got_section(symtab, layout);
                got->add_global_with_rela(gsym, GOT_TYPE_TLS_OFFSET,
                                          target->rela_dyn_section(layout),
                                          elfcpp::R_X86_64_TPOFF64);
	      }
	    else if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_global(object, r_type, gsym);
	    break;

          case elfcpp::R_X86_64_TLSDESC_CALL:
	    break;

          case elfcpp::R_X86_64_TLSLD:       // Local-dynamic
	    if (optimized_type == tls::TLSOPT_NONE)
	      {
	        // Create a GOT entry for the module index.
	        target->got_mod_index_entry(symtab, layout, object);
	      }
	    else if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_global(object, r_type, gsym);
	    break;

          case elfcpp::R_X86_64_DTPOFF32:
          case elfcpp::R_X86_64_DTPOFF64:
	    break;

          case elfcpp::R_X86_64_GOTTPOFF:    // Initial-exec
	    layout->set_has_static_tls();
            if (optimized_type == tls::TLSOPT_NONE)
              {
	        // Create a GOT entry for the tp-relative offset.
	        Output_data_got<64, false>* got
	            = target->got_section(symtab, layout);
	        got->add_global_with_rela(gsym, GOT_TYPE_TLS_OFFSET,
	                                  target->rela_dyn_section(layout),
	                                  elfcpp::R_X86_64_TPOFF64);
              }
            else if (optimized_type != tls::TLSOPT_TO_LE)
              unsupported_reloc_global(object, r_type, gsym);
            break;

          case elfcpp::R_X86_64_TPOFF32:     // Local-exec
	    layout->set_has_static_tls();
            if (parameters->options().shared())
              unsupported_reloc_local(object, r_type);
	    break;

          default:
            gold_unreachable();
	  }
      }
      break;

    case elfcpp::R_X86_64_SIZE32:
    case elfcpp::R_X86_64_SIZE64:
    default:
      gold_error(_("%s: unsupported reloc %u against global symbol %s"),
		 object->name().c_str(), r_type,
                 gsym->demangled_name().c_str());
      break;
    }
}

// Scan relocations for a section.

void
Target_x86_64::scan_relocs(const General_options& options,
                           Symbol_table* symtab,
                           Layout* layout,
                           Sized_relobj<64, false>* object,
                           unsigned int data_shndx,
                           unsigned int sh_type,
                           const unsigned char* prelocs,
                           size_t reloc_count,
			   Output_section* output_section,
			   bool needs_special_offset_handling,
                           size_t local_symbol_count,
                           const unsigned char* plocal_symbols)
{
  if (sh_type == elfcpp::SHT_REL)
    {
      gold_error(_("%s: unsupported REL reloc section"),
		 object->name().c_str());
      return;
    }

  gold::scan_relocs<64, false, Target_x86_64, elfcpp::SHT_RELA,
      Target_x86_64::Scan>(
    options,
    symtab,
    layout,
    this,
    object,
    data_shndx,
    prelocs,
    reloc_count,
    output_section,
    needs_special_offset_handling,
    local_symbol_count,
    plocal_symbols);
}

// Finalize the sections.

void
Target_x86_64::do_finalize_sections(Layout* layout)
{
  // Fill in some more dynamic tags.
  Output_data_dynamic* const odyn = layout->dynamic_data();
  if (odyn != NULL)
    {
      if (this->got_plt_ != NULL)
	odyn->add_section_address(elfcpp::DT_PLTGOT, this->got_plt_);

      if (this->plt_ != NULL)
	{
	  const Output_data* od = this->plt_->rel_plt();
	  odyn->add_section_size(elfcpp::DT_PLTRELSZ, od);
	  odyn->add_section_address(elfcpp::DT_JMPREL, od);
	  odyn->add_constant(elfcpp::DT_PLTREL, elfcpp::DT_RELA);
	  if (this->plt_->has_tlsdesc_entry())
	    {
              unsigned int plt_offset = this->plt_->get_tlsdesc_plt_offset();
              unsigned int got_offset = this->plt_->get_tlsdesc_got_offset();
              this->got_->finalize_data_size();
              odyn->add_section_plus_offset(elfcpp::DT_TLSDESC_PLT,
                                            this->plt_, plt_offset);
              odyn->add_section_plus_offset(elfcpp::DT_TLSDESC_GOT,
                                            this->got_, got_offset);
	    }
	}

      if (this->rela_dyn_ != NULL)
	{
	  const Output_data* od = this->rela_dyn_;
	  odyn->add_section_address(elfcpp::DT_RELA, od);
	  odyn->add_section_size(elfcpp::DT_RELASZ, od);
	  odyn->add_constant(elfcpp::DT_RELAENT,
			     elfcpp::Elf_sizes<64>::rela_size);
	}

      if (!parameters->options().shared())
	{
	  // The value of the DT_DEBUG tag is filled in by the dynamic
	  // linker at run time, and used by the debugger.
	  odyn->add_constant(elfcpp::DT_DEBUG, 0);
	}
    }

  // Emit any relocs we saved in an attempt to avoid generating COPY
  // relocs.
  if (this->copy_relocs_.any_saved_relocs())
    this->copy_relocs_.emit(this->rela_dyn_section(layout));
}

// Perform a relocation.

inline bool
Target_x86_64::Relocate::relocate(const Relocate_info<64, false>* relinfo,
                                  Target_x86_64* target,
                                  size_t relnum,
                                  const elfcpp::Rela<64, false>& rela,
                                  unsigned int r_type,
                                  const Sized_symbol<64>* gsym,
                                  const Symbol_value<64>* psymval,
                                  unsigned char* view,
                                  elfcpp::Elf_types<64>::Elf_Addr address,
                                  section_size_type view_size)
{
  if (this->skip_call_tls_get_addr_)
    {
      if (r_type != elfcpp::R_X86_64_PLT32
	  || gsym == NULL
	  || strcmp(gsym->name(), "__tls_get_addr") != 0)
	{
	  gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
				 _("missing expected TLS relocation"));
	}
      else
	{
	  this->skip_call_tls_get_addr_ = false;
	  return false;
	}
    }

  // Pick the value to use for symbols defined in shared objects.
  Symbol_value<64> symval;
  if (gsym != NULL
      && gsym->use_plt_offset(r_type == elfcpp::R_X86_64_PC64
			      || r_type == elfcpp::R_X86_64_PC32
			      || r_type == elfcpp::R_X86_64_PC16
			      || r_type == elfcpp::R_X86_64_PC8))
    {
      symval.set_output_value(target->plt_section()->address()
			      + gsym->plt_offset());
      psymval = &symval;
    }

  const Sized_relobj<64, false>* object = relinfo->object;
  const elfcpp::Elf_Xword addend = rela.get_r_addend();

  // Get the GOT offset if needed.
  // The GOT pointer points to the end of the GOT section.
  // We need to subtract the size of the GOT section to get
  // the actual offset to use in the relocation.
  bool have_got_offset = false;
  unsigned int got_offset = 0;
  switch (r_type)
    {
    case elfcpp::R_X86_64_GOT32:
    case elfcpp::R_X86_64_GOT64:
    case elfcpp::R_X86_64_GOTPLT64:
    case elfcpp::R_X86_64_GOTPCREL:
    case elfcpp::R_X86_64_GOTPCREL64:
      if (gsym != NULL)
        {
          gold_assert(gsym->has_got_offset(GOT_TYPE_STANDARD));
          got_offset = gsym->got_offset(GOT_TYPE_STANDARD) - target->got_size();
        }
      else
        {
          unsigned int r_sym = elfcpp::elf_r_sym<64>(rela.get_r_info());
          gold_assert(object->local_has_got_offset(r_sym, GOT_TYPE_STANDARD));
          got_offset = (object->local_got_offset(r_sym, GOT_TYPE_STANDARD)
                        - target->got_size());
        }
      have_got_offset = true;
      break;

    default:
      break;
    }

  switch (r_type)
    {
    case elfcpp::R_X86_64_NONE:
    case elfcpp::R_386_GNU_VTINHERIT:
    case elfcpp::R_386_GNU_VTENTRY:
      break;

    case elfcpp::R_X86_64_64:
      Relocate_functions<64, false>::rela64(view, object, psymval, addend);
      break;

    case elfcpp::R_X86_64_PC64:
      Relocate_functions<64, false>::pcrela64(view, object, psymval, addend,
                                              address);
      break;

    case elfcpp::R_X86_64_32:
      // FIXME: we need to verify that value + addend fits into 32 bits:
      //    uint64_t x = value + addend;
      //    x == static_cast<uint64_t>(static_cast<uint32_t>(x))
      // Likewise for other <=32-bit relocations (but see R_X86_64_32S).
      Relocate_functions<64, false>::rela32(view, object, psymval, addend);
      break;

    case elfcpp::R_X86_64_32S:
      // FIXME: we need to verify that value + addend fits into 32 bits:
      //    int64_t x = value + addend;   // note this quantity is signed!
      //    x == static_cast<int64_t>(static_cast<int32_t>(x))
      Relocate_functions<64, false>::rela32(view, object, psymval, addend);
      break;

    case elfcpp::R_X86_64_PC32:
      Relocate_functions<64, false>::pcrela32(view, object, psymval, addend,
                                              address);
      break;

    case elfcpp::R_X86_64_16:
      Relocate_functions<64, false>::rela16(view, object, psymval, addend);
      break;

    case elfcpp::R_X86_64_PC16:
      Relocate_functions<64, false>::pcrela16(view, object, psymval, addend,
                                              address);
      break;

    case elfcpp::R_X86_64_8:
      Relocate_functions<64, false>::rela8(view, object, psymval, addend);
      break;

    case elfcpp::R_X86_64_PC8:
      Relocate_functions<64, false>::pcrela8(view, object, psymval, addend,
                                             address);
      break;

    case elfcpp::R_X86_64_PLT32:
      gold_assert(gsym == NULL
                  || gsym->has_plt_offset()
		  || gsym->final_value_is_known()
		  || (gsym->is_defined()
		      && !gsym->is_from_dynobj()
		      && !gsym->is_preemptible()));
      // Note: while this code looks the same as for R_X86_64_PC32, it
      // behaves differently because psymval was set to point to
      // the PLT entry, rather than the symbol, in Scan::global().
      Relocate_functions<64, false>::pcrela32(view, object, psymval, addend,
                                              address);
      break;

    case elfcpp::R_X86_64_PLTOFF64:
      {
        gold_assert(gsym);
        gold_assert(gsym->has_plt_offset()
                    || gsym->final_value_is_known());
	elfcpp::Elf_types<64>::Elf_Addr got_address;
	got_address = target->got_section(NULL, NULL)->address();
	Relocate_functions<64, false>::rela64(view, object, psymval,
					      addend - got_address);
      }

    case elfcpp::R_X86_64_GOT32:
      gold_assert(have_got_offset);
      Relocate_functions<64, false>::rela32(view, got_offset, addend);
      break;

    case elfcpp::R_X86_64_GOTPC32:
      {
        gold_assert(gsym);
	elfcpp::Elf_types<64>::Elf_Addr value;
	value = target->got_plt_section()->address();
	Relocate_functions<64, false>::pcrela32(view, value, addend, address);
      }
      break;

    case elfcpp::R_X86_64_GOT64:
      // The ABI doc says "Like GOT64, but indicates a PLT entry is needed."
      // Since we always add a PLT entry, this is equivalent.
    case elfcpp::R_X86_64_GOTPLT64:
      gold_assert(have_got_offset);
      Relocate_functions<64, false>::rela64(view, got_offset, addend);
      break;

    case elfcpp::R_X86_64_GOTPC64:
      {
        gold_assert(gsym);
	elfcpp::Elf_types<64>::Elf_Addr value;
	value = target->got_plt_section()->address();
	Relocate_functions<64, false>::pcrela64(view, value, addend, address);
      }
      break;

    case elfcpp::R_X86_64_GOTOFF64:
      {
	elfcpp::Elf_types<64>::Elf_Addr value;
	value = (psymval->value(object, 0)
		 - target->got_plt_section()->address());
	Relocate_functions<64, false>::rela64(view, value, addend);
      }
      break;

    case elfcpp::R_X86_64_GOTPCREL:
      {
        gold_assert(have_got_offset);
        elfcpp::Elf_types<64>::Elf_Addr value;
        value = target->got_plt_section()->address() + got_offset;
        Relocate_functions<64, false>::pcrela32(view, value, addend, address);
      }
      break;

    case elfcpp::R_X86_64_GOTPCREL64:
      {
        gold_assert(have_got_offset);
        elfcpp::Elf_types<64>::Elf_Addr value;
        value = target->got_plt_section()->address() + got_offset;
        Relocate_functions<64, false>::pcrela64(view, value, addend, address);
      }
      break;

    case elfcpp::R_X86_64_COPY:
    case elfcpp::R_X86_64_GLOB_DAT:
    case elfcpp::R_X86_64_JUMP_SLOT:
    case elfcpp::R_X86_64_RELATIVE:
      // These are outstanding tls relocs, which are unexpected when linking
    case elfcpp::R_X86_64_TPOFF64:
    case elfcpp::R_X86_64_DTPMOD64:
    case elfcpp::R_X86_64_TLSDESC:
      gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
			     _("unexpected reloc %u in object file"),
			     r_type);
      break;

      // These are initial tls relocs, which are expected when linking
    case elfcpp::R_X86_64_TLSGD:            // Global-dynamic
    case elfcpp::R_X86_64_GOTPC32_TLSDESC:  // Global-dynamic (from ~oliva url)
    case elfcpp::R_X86_64_TLSDESC_CALL:
    case elfcpp::R_X86_64_TLSLD:            // Local-dynamic
    case elfcpp::R_X86_64_DTPOFF32:
    case elfcpp::R_X86_64_DTPOFF64:
    case elfcpp::R_X86_64_GOTTPOFF:         // Initial-exec
    case elfcpp::R_X86_64_TPOFF32:          // Local-exec
      this->relocate_tls(relinfo, target, relnum, rela, r_type, gsym, psymval,
                         view, address, view_size);
      break;

    case elfcpp::R_X86_64_SIZE32:
    case elfcpp::R_X86_64_SIZE64:
    default:
      gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
			     _("unsupported reloc %u"),
			     r_type);
      break;
    }

  return true;
}

// Perform a TLS relocation.

inline void
Target_x86_64::Relocate::relocate_tls(const Relocate_info<64, false>* relinfo,
                                      Target_x86_64* target,
                                      size_t relnum,
                                      const elfcpp::Rela<64, false>& rela,
                                      unsigned int r_type,
                                      const Sized_symbol<64>* gsym,
                                      const Symbol_value<64>* psymval,
                                      unsigned char* view,
                                      elfcpp::Elf_types<64>::Elf_Addr address,
                                      section_size_type view_size)
{
  Output_segment* tls_segment = relinfo->layout->tls_segment();

  const Sized_relobj<64, false>* object = relinfo->object;
  const elfcpp::Elf_Xword addend = rela.get_r_addend();

  elfcpp::Elf_types<64>::Elf_Addr value = psymval->value(relinfo->object, 0);

  const bool is_final = (gsym == NULL
			 ? !parameters->options().output_is_position_independent()
			 : gsym->final_value_is_known());
  const tls::Tls_optimization optimized_type
      = Target_x86_64::optimize_tls_reloc(is_final, r_type);
  switch (r_type)
    {
    case elfcpp::R_X86_64_TLSGD:            // Global-dynamic
      this->saw_tls_block_reloc_ = true;
      if (optimized_type == tls::TLSOPT_TO_LE)
	{
	  gold_assert(tls_segment != NULL);
	  this->tls_gd_to_le(relinfo, relnum, tls_segment,
			     rela, r_type, value, view,
			     view_size);
	  break;
	}
      else
        {
          unsigned int got_type = (optimized_type == tls::TLSOPT_TO_IE
                                   ? GOT_TYPE_TLS_OFFSET
                                   : GOT_TYPE_TLS_PAIR);
          unsigned int got_offset;
          if (gsym != NULL)
            {
              gold_assert(gsym->has_got_offset(got_type));
              got_offset = gsym->got_offset(got_type) - target->got_size();
            }
          else
            {
              unsigned int r_sym = elfcpp::elf_r_sym<64>(rela.get_r_info());
              gold_assert(object->local_has_got_offset(r_sym, got_type));
              got_offset = (object->local_got_offset(r_sym, got_type)
                            - target->got_size());
            }
          if (optimized_type == tls::TLSOPT_TO_IE)
            {
              gold_assert(tls_segment != NULL);
              value = target->got_plt_section()->address() + got_offset;
              this->tls_gd_to_ie(relinfo, relnum, tls_segment, rela, r_type,
                                 value, view, address, view_size);
              break;
            }
          else if (optimized_type == tls::TLSOPT_NONE)
            {
              // Relocate the field with the offset of the pair of GOT
              // entries.
	      value = target->got_plt_section()->address() + got_offset;
              Relocate_functions<64, false>::pcrela32(view, value, addend,
                                                      address);
              break;
            }
        }
      gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
			     _("unsupported reloc %u"), r_type);
      break;

    case elfcpp::R_X86_64_GOTPC32_TLSDESC:  // Global-dynamic (from ~oliva url)
    case elfcpp::R_X86_64_TLSDESC_CALL:
      this->saw_tls_block_reloc_ = true;
      if (optimized_type == tls::TLSOPT_TO_LE)
	{
	  gold_assert(tls_segment != NULL);
	  this->tls_desc_gd_to_le(relinfo, relnum, tls_segment,
			          rela, r_type, value, view,
			          view_size);
	  break;
	}
      else
        {
          unsigned int got_type = (optimized_type == tls::TLSOPT_TO_IE
                                   ? GOT_TYPE_TLS_OFFSET
                                   : GOT_TYPE_TLS_DESC);
          unsigned int got_offset;
          if (gsym != NULL)
            {
              gold_assert(gsym->has_got_offset(got_type));
              got_offset = gsym->got_offset(got_type) - target->got_size();
            }
          else
            {
              unsigned int r_sym = elfcpp::elf_r_sym<64>(rela.get_r_info());
              gold_assert(object->local_has_got_offset(r_sym, got_type));
              got_offset = (object->local_got_offset(r_sym, got_type)
                            - target->got_size());
            }
          if (optimized_type == tls::TLSOPT_TO_IE)
            {
              gold_assert(tls_segment != NULL);
              value = target->got_plt_section()->address() + got_offset;
              this->tls_desc_gd_to_ie(relinfo, relnum, tls_segment,
                                      rela, r_type, value, view, address,
                                      view_size);
              break;
            }
          else if (optimized_type == tls::TLSOPT_NONE)
            {
              if (r_type == elfcpp::R_X86_64_GOTPC32_TLSDESC)
                {
                  // Relocate the field with the offset of the pair of GOT
                  // entries.
	          value = target->got_plt_section()->address() + got_offset;
                  Relocate_functions<64, false>::pcrela32(view, value, addend,
                                                          address);
                }
              break;
            }
        }
      gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
			     _("unsupported reloc %u"), r_type);
      break;

    case elfcpp::R_X86_64_TLSLD:            // Local-dynamic
      this->saw_tls_block_reloc_ = true;
      if (optimized_type == tls::TLSOPT_TO_LE)
        {
          gold_assert(tls_segment != NULL);
	  this->tls_ld_to_le(relinfo, relnum, tls_segment, rela, r_type,
			     value, view, view_size);
	  break;
        }
      else if (optimized_type == tls::TLSOPT_NONE)
        {
          // Relocate the field with the offset of the GOT entry for
          // the module index.
          unsigned int got_offset;
          got_offset = (target->got_mod_index_entry(NULL, NULL, NULL)
			- target->got_size());
	  value = target->got_plt_section()->address() + got_offset;
          Relocate_functions<64, false>::pcrela32(view, value, addend,
                                                  address);
          break;
        }
      gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
			     _("unsupported reloc %u"), r_type);
      break;

    case elfcpp::R_X86_64_DTPOFF32:
      gold_assert(tls_segment != NULL);
      if (optimized_type == tls::TLSOPT_TO_LE)
        {
          // This relocation type is used in debugging information.
          // In that case we need to not optimize the value.  If we
          // haven't seen a TLSLD reloc, then we assume we should not
          // optimize this reloc.
          if (this->saw_tls_block_reloc_)
            value -= tls_segment->memsz();
        }
      Relocate_functions<64, false>::rela32(view, value, addend);
      break;

    case elfcpp::R_X86_64_DTPOFF64:
      gold_assert(tls_segment != NULL);
      if (optimized_type == tls::TLSOPT_TO_LE)
        {
          // See R_X86_64_DTPOFF32, just above, for why we test this.
          if (this->saw_tls_block_reloc_)
            value -= tls_segment->memsz();
        }
      Relocate_functions<64, false>::rela64(view, value, addend);
      break;

    case elfcpp::R_X86_64_GOTTPOFF:         // Initial-exec
      if (optimized_type == tls::TLSOPT_TO_LE)
	{
          gold_assert(tls_segment != NULL);
	  Target_x86_64::Relocate::tls_ie_to_le(relinfo, relnum, tls_segment,
                                                rela, r_type, value, view,
                                                view_size);
	  break;
	}
      else if (optimized_type == tls::TLSOPT_NONE)
        {
          // Relocate the field with the offset of the GOT entry for
          // the tp-relative offset of the symbol.
          unsigned int got_offset;
          if (gsym != NULL)
            {
              gold_assert(gsym->has_got_offset(GOT_TYPE_TLS_OFFSET));
              got_offset = (gsym->got_offset(GOT_TYPE_TLS_OFFSET)
                            - target->got_size());
            }
          else
            {
              unsigned int r_sym = elfcpp::elf_r_sym<64>(rela.get_r_info());
              gold_assert(object->local_has_got_offset(r_sym,
                                                       GOT_TYPE_TLS_OFFSET));
              got_offset = (object->local_got_offset(r_sym, GOT_TYPE_TLS_OFFSET)
                            - target->got_size());
            }
	  value = target->got_plt_section()->address() + got_offset;
          Relocate_functions<64, false>::pcrela32(view, value, addend, address);
          break;
        }
      gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
			     _("unsupported reloc type %u"),
			     r_type);
      break;

    case elfcpp::R_X86_64_TPOFF32:          // Local-exec
      value -= tls_segment->memsz();
      Relocate_functions<64, false>::rela32(view, value, addend);
      break;
    }
}

// Do a relocation in which we convert a TLS General-Dynamic to an
// Initial-Exec.

inline void
Target_x86_64::Relocate::tls_gd_to_ie(const Relocate_info<64, false>* relinfo,
                                      size_t relnum,
                                      Output_segment*,
                                      const elfcpp::Rela<64, false>& rela,
                                      unsigned int,
                                      elfcpp::Elf_types<64>::Elf_Addr value,
                                      unsigned char* view,
                                      elfcpp::Elf_types<64>::Elf_Addr address,
                                      section_size_type view_size)
{
  // .byte 0x66; leaq foo@tlsgd(%rip),%rdi;
  // .word 0x6666; rex64; call __tls_get_addr
  // ==> movq %fs:0,%rax; addq x@gottpoff(%rip),%rax

  tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, -4);
  tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, 12);

  tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                 (memcmp(view - 4, "\x66\x48\x8d\x3d", 4) == 0));
  tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                 (memcmp(view + 4, "\x66\x66\x48\xe8", 4) == 0));

  memcpy(view - 4, "\x64\x48\x8b\x04\x25\0\0\0\0\x48\x03\x05\0\0\0\0", 16);

  const elfcpp::Elf_Xword addend = rela.get_r_addend();
  Relocate_functions<64, false>::pcrela32(view + 8, value, addend - 8, address);

  // The next reloc should be a PLT32 reloc against __tls_get_addr.
  // We can skip it.
  this->skip_call_tls_get_addr_ = true;
}

// Do a relocation in which we convert a TLS General-Dynamic to a
// Local-Exec.

inline void
Target_x86_64::Relocate::tls_gd_to_le(const Relocate_info<64, false>* relinfo,
                                      size_t relnum,
                                      Output_segment* tls_segment,
                                      const elfcpp::Rela<64, false>& rela,
                                      unsigned int,
                                      elfcpp::Elf_types<64>::Elf_Addr value,
                                      unsigned char* view,
                                      section_size_type view_size)
{
  // .byte 0x66; leaq foo@tlsgd(%rip),%rdi;
  // .word 0x6666; rex64; call __tls_get_addr
  // ==> movq %fs:0,%rax; leaq x@tpoff(%rax),%rax

  tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, -4);
  tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, 12);

  tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                 (memcmp(view - 4, "\x66\x48\x8d\x3d", 4) == 0));
  tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                 (memcmp(view + 4, "\x66\x66\x48\xe8", 4) == 0));

  memcpy(view - 4, "\x64\x48\x8b\x04\x25\0\0\0\0\x48\x8d\x80\0\0\0\0", 16);

  value -= tls_segment->memsz();
  Relocate_functions<64, false>::rela32(view + 8, value, 0);

  // The next reloc should be a PLT32 reloc against __tls_get_addr.
  // We can skip it.
  this->skip_call_tls_get_addr_ = true;
}

// Do a TLSDESC-style General-Dynamic to Initial-Exec transition.

inline void
Target_x86_64::Relocate::tls_desc_gd_to_ie(
    const Relocate_info<64, false>* relinfo,
    size_t relnum,
    Output_segment*,
    const elfcpp::Rela<64, false>& rela,
    unsigned int r_type,
    elfcpp::Elf_types<64>::Elf_Addr value,
    unsigned char* view,
    elfcpp::Elf_types<64>::Elf_Addr address,
    section_size_type view_size)
{
  if (r_type == elfcpp::R_X86_64_GOTPC32_TLSDESC)
    {
      // leaq foo@tlsdesc(%rip), %rax
      // ==> movq foo@gottpoff(%rip), %rax
      tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, -3);
      tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, 4);
      tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                     view[-3] == 0x48 && view[-2] == 0x8d && view[-1] == 0x05);
      view[-2] = 0x8b;
      const elfcpp::Elf_Xword addend = rela.get_r_addend();
      Relocate_functions<64, false>::pcrela32(view, value, addend, address);
    }
  else
    {
      // call *foo@tlscall(%rax)
      // ==> nop; nop
      gold_assert(r_type == elfcpp::R_X86_64_TLSDESC_CALL);
      tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, 2);
      tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                     view[0] == 0xff && view[1] == 0x10);
      view[0] = 0x66;
      view[1] = 0x90;
    }
}

// Do a TLSDESC-style General-Dynamic to Local-Exec transition.

inline void
Target_x86_64::Relocate::tls_desc_gd_to_le(
    const Relocate_info<64, false>* relinfo,
    size_t relnum,
    Output_segment* tls_segment,
    const elfcpp::Rela<64, false>& rela,
    unsigned int r_type,
    elfcpp::Elf_types<64>::Elf_Addr value,
    unsigned char* view,
    section_size_type view_size)
{
  if (r_type == elfcpp::R_X86_64_GOTPC32_TLSDESC)
    {
      // leaq foo@tlsdesc(%rip), %rax
      // ==> movq foo@tpoff, %rax
      tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, -3);
      tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, 4);
      tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                     view[-3] == 0x48 && view[-2] == 0x8d && view[-1] == 0x05);
      view[-2] = 0xc7;
      view[-1] = 0xc0;
      value -= tls_segment->memsz();
      Relocate_functions<64, false>::rela32(view, value, 0);
    }
  else
    {
      // call *foo@tlscall(%rax)
      // ==> nop; nop
      gold_assert(r_type == elfcpp::R_X86_64_TLSDESC_CALL);
      tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, 2);
      tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                     view[0] == 0xff && view[1] == 0x10);
      view[0] = 0x66;
      view[1] = 0x90;
    }
}

inline void
Target_x86_64::Relocate::tls_ld_to_le(const Relocate_info<64, false>* relinfo,
                                      size_t relnum,
                                      Output_segment*,
                                      const elfcpp::Rela<64, false>& rela,
                                      unsigned int,
                                      elfcpp::Elf_types<64>::Elf_Addr,
                                      unsigned char* view,
                                      section_size_type view_size)
{
  // leaq foo@tlsld(%rip),%rdi; call __tls_get_addr@plt;
  // ... leq foo@dtpoff(%rax),%reg
  // ==> .word 0x6666; .byte 0x66; movq %fs:0,%rax ... leaq x@tpoff(%rax),%rdx

  tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, -3);
  tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, 9);

  tls::check_tls(relinfo, relnum, rela.get_r_offset(),
                 view[-3] == 0x48 && view[-2] == 0x8d && view[-1] == 0x3d);

  tls::check_tls(relinfo, relnum, rela.get_r_offset(), view[4] == 0xe8);

  memcpy(view - 3, "\x66\x66\x66\x64\x48\x8b\x04\x25\0\0\0\0", 12);

  // The next reloc should be a PLT32 reloc against __tls_get_addr.
  // We can skip it.
  this->skip_call_tls_get_addr_ = true;
}

// Do a relocation in which we convert a TLS Initial-Exec to a
// Local-Exec.

inline void
Target_x86_64::Relocate::tls_ie_to_le(const Relocate_info<64, false>* relinfo,
                                      size_t relnum,
                                      Output_segment* tls_segment,
                                      const elfcpp::Rela<64, false>& rela,
                                      unsigned int,
                                      elfcpp::Elf_types<64>::Elf_Addr value,
                                      unsigned char* view,
                                      section_size_type view_size)
{
  // We need to examine the opcodes to figure out which instruction we
  // are looking at.

  // movq foo@gottpoff(%rip),%reg  ==>  movq $YY,%reg
  // addq foo@gottpoff(%rip),%reg  ==>  addq $YY,%reg

  tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, -3);
  tls::check_range(relinfo, relnum, rela.get_r_offset(), view_size, 4);

  unsigned char op1 = view[-3];
  unsigned char op2 = view[-2];
  unsigned char op3 = view[-1];
  unsigned char reg = op3 >> 3;

  if (op2 == 0x8b)
    {
      // movq
      if (op1 == 0x4c)
        view[-3] = 0x49;
      view[-2] = 0xc7;
      view[-1] = 0xc0 | reg;
    }
  else if (reg == 4)
    {
      // Special handling for %rsp.
      if (op1 == 0x4c)
        view[-3] = 0x49;
      view[-2] = 0x81;
      view[-1] = 0xc0 | reg;
    }
  else
    {
      // addq
      if (op1 == 0x4c)
        view[-3] = 0x4d;
      view[-2] = 0x8d;
      view[-1] = 0x80 | reg | (reg << 3);
    }

  value -= tls_segment->memsz();
  Relocate_functions<64, false>::rela32(view, value, 0);
}

// Relocate section data.

void
Target_x86_64::relocate_section(const Relocate_info<64, false>* relinfo,
                                unsigned int sh_type,
                                const unsigned char* prelocs,
                                size_t reloc_count,
				Output_section* output_section,
				bool needs_special_offset_handling,
                                unsigned char* view,
                                elfcpp::Elf_types<64>::Elf_Addr address,
                                section_size_type view_size)
{
  gold_assert(sh_type == elfcpp::SHT_RELA);

  gold::relocate_section<64, false, Target_x86_64, elfcpp::SHT_RELA,
			 Target_x86_64::Relocate>(
    relinfo,
    this,
    prelocs,
    reloc_count,
    output_section,
    needs_special_offset_handling,
    view,
    address,
    view_size);
}

// Return the size of a relocation while scanning during a relocatable
// link.

unsigned int
Target_x86_64::Relocatable_size_for_reloc::get_size_for_reloc(
    unsigned int r_type,
    Relobj* object)
{
  switch (r_type)
    {
    case elfcpp::R_X86_64_NONE:
    case elfcpp::R_386_GNU_VTINHERIT:
    case elfcpp::R_386_GNU_VTENTRY:
    case elfcpp::R_X86_64_TLSGD:            // Global-dynamic
    case elfcpp::R_X86_64_GOTPC32_TLSDESC:  // Global-dynamic (from ~oliva url)
    case elfcpp::R_X86_64_TLSDESC_CALL:
    case elfcpp::R_X86_64_TLSLD:            // Local-dynamic
    case elfcpp::R_X86_64_DTPOFF32:
    case elfcpp::R_X86_64_DTPOFF64:
    case elfcpp::R_X86_64_GOTTPOFF:         // Initial-exec
    case elfcpp::R_X86_64_TPOFF32:          // Local-exec
      return 0;

    case elfcpp::R_X86_64_64:
    case elfcpp::R_X86_64_PC64:
    case elfcpp::R_X86_64_GOTOFF64:
    case elfcpp::R_X86_64_GOTPC64:
    case elfcpp::R_X86_64_PLTOFF64:
    case elfcpp::R_X86_64_GOT64:
    case elfcpp::R_X86_64_GOTPCREL64:
    case elfcpp::R_X86_64_GOTPCREL:
    case elfcpp::R_X86_64_GOTPLT64:
      return 8;

    case elfcpp::R_X86_64_32:
    case elfcpp::R_X86_64_32S:
    case elfcpp::R_X86_64_PC32:
    case elfcpp::R_X86_64_PLT32:
    case elfcpp::R_X86_64_GOTPC32:
    case elfcpp::R_X86_64_GOT32:
      return 4;

    case elfcpp::R_X86_64_16:
    case elfcpp::R_X86_64_PC16:
      return 2;

    case elfcpp::R_X86_64_8:
    case elfcpp::R_X86_64_PC8:
      return 1;

    case elfcpp::R_X86_64_COPY:
    case elfcpp::R_X86_64_GLOB_DAT:
    case elfcpp::R_X86_64_JUMP_SLOT:
    case elfcpp::R_X86_64_RELATIVE:
      // These are outstanding tls relocs, which are unexpected when linking
    case elfcpp::R_X86_64_TPOFF64:
    case elfcpp::R_X86_64_DTPMOD64:
    case elfcpp::R_X86_64_TLSDESC:
      object->error(_("unexpected reloc %u in object file"), r_type);
      return 0;

    case elfcpp::R_X86_64_SIZE32:
    case elfcpp::R_X86_64_SIZE64:
    default:
      object->error(_("unsupported reloc %u against local symbol"), r_type);
      return 0;
    }
}

// Scan the relocs during a relocatable link.

void
Target_x86_64::scan_relocatable_relocs(const General_options& options,
				       Symbol_table* symtab,
				       Layout* layout,
				       Sized_relobj<64, false>* object,
				       unsigned int data_shndx,
				       unsigned int sh_type,
				       const unsigned char* prelocs,
				       size_t reloc_count,
				       Output_section* output_section,
				       bool needs_special_offset_handling,
				       size_t local_symbol_count,
				       const unsigned char* plocal_symbols,
				       Relocatable_relocs* rr)
{
  gold_assert(sh_type == elfcpp::SHT_RELA);

  typedef gold::Default_scan_relocatable_relocs<elfcpp::SHT_RELA,
    Relocatable_size_for_reloc> Scan_relocatable_relocs;

  gold::scan_relocatable_relocs<64, false, elfcpp::SHT_RELA,
      Scan_relocatable_relocs>(
    options,
    symtab,
    layout,
    object,
    data_shndx,
    prelocs,
    reloc_count,
    output_section,
    needs_special_offset_handling,
    local_symbol_count,
    plocal_symbols,
    rr);
}

// Relocate a section during a relocatable link.

void
Target_x86_64::relocate_for_relocatable(
    const Relocate_info<64, false>* relinfo,
    unsigned int sh_type,
    const unsigned char* prelocs,
    size_t reloc_count,
    Output_section* output_section,
    off_t offset_in_output_section,
    const Relocatable_relocs* rr,
    unsigned char* view,
    elfcpp::Elf_types<64>::Elf_Addr view_address,
    section_size_type view_size,
    unsigned char* reloc_view,
    section_size_type reloc_view_size)
{
  gold_assert(sh_type == elfcpp::SHT_RELA);

  gold::relocate_for_relocatable<64, false, elfcpp::SHT_RELA>(
    relinfo,
    prelocs,
    reloc_count,
    output_section,
    offset_in_output_section,
    rr,
    view,
    view_address,
    view_size,
    reloc_view,
    reloc_view_size);
}

// Return the value to use for a dynamic which requires special
// treatment.  This is how we support equality comparisons of function
// pointers across shared library boundaries, as described in the
// processor specific ABI supplement.

uint64_t
Target_x86_64::do_dynsym_value(const Symbol* gsym) const
{
  gold_assert(gsym->is_from_dynobj() && gsym->has_plt_offset());
  return this->plt_section()->address() + gsym->plt_offset();
}

// Return a string used to fill a code section with nops to take up
// the specified length.

std::string
Target_x86_64::do_code_fill(section_size_type length) const
{
  if (length >= 16)
    {
      // Build a jmpq instruction to skip over the bytes.
      unsigned char jmp[5];
      jmp[0] = 0xe9;
      elfcpp::Swap_unaligned<32, false>::writeval(jmp + 1, length - 5);
      return (std::string(reinterpret_cast<char*>(&jmp[0]), 5)
              + std::string(length - 5, '\0'));
    }

  // Nop sequences of various lengths.
  const char nop1[1] = { 0x90 };                   // nop
  const char nop2[2] = { 0x66, 0x90 };             // xchg %ax %ax
  const char nop3[3] = { 0x8d, 0x76, 0x00 };       // leal 0(%esi),%esi
  const char nop4[4] = { 0x8d, 0x74, 0x26, 0x00};  // leal 0(%esi,1),%esi
  const char nop5[5] = { 0x90, 0x8d, 0x74, 0x26,   // nop
                         0x00 };                   // leal 0(%esi,1),%esi
  const char nop6[6] = { 0x8d, 0xb6, 0x00, 0x00,   // leal 0L(%esi),%esi
                         0x00, 0x00 };
  const char nop7[7] = { 0x8d, 0xb4, 0x26, 0x00,   // leal 0L(%esi,1),%esi
                         0x00, 0x00, 0x00 };
  const char nop8[8] = { 0x90, 0x8d, 0xb4, 0x26,   // nop
                         0x00, 0x00, 0x00, 0x00 }; // leal 0L(%esi,1),%esi
  const char nop9[9] = { 0x89, 0xf6, 0x8d, 0xbc,   // movl %esi,%esi
                         0x27, 0x00, 0x00, 0x00,   // leal 0L(%edi,1),%edi
                         0x00 };
  const char nop10[10] = { 0x8d, 0x76, 0x00, 0x8d, // leal 0(%esi),%esi
                           0xbc, 0x27, 0x00, 0x00, // leal 0L(%edi,1),%edi
                           0x00, 0x00 };
  const char nop11[11] = { 0x8d, 0x74, 0x26, 0x00, // leal 0(%esi,1),%esi
                           0x8d, 0xbc, 0x27, 0x00, // leal 0L(%edi,1),%edi
                           0x00, 0x00, 0x00 };
  const char nop12[12] = { 0x8d, 0xb6, 0x00, 0x00, // leal 0L(%esi),%esi
                           0x00, 0x00, 0x8d, 0xbf, // leal 0L(%edi),%edi
                           0x00, 0x00, 0x00, 0x00 };
  const char nop13[13] = { 0x8d, 0xb6, 0x00, 0x00, // leal 0L(%esi),%esi
                           0x00, 0x00, 0x8d, 0xbc, // leal 0L(%edi,1),%edi
                           0x27, 0x00, 0x00, 0x00,
                           0x00 };
  const char nop14[14] = { 0x8d, 0xb4, 0x26, 0x00, // leal 0L(%esi,1),%esi
                           0x00, 0x00, 0x00, 0x8d, // leal 0L(%edi,1),%edi
                           0xbc, 0x27, 0x00, 0x00,
                           0x00, 0x00 };
  const char nop15[15] = { 0xeb, 0x0d, 0x90, 0x90, // jmp .+15
                           0x90, 0x90, 0x90, 0x90, // nop,nop,nop,...
                           0x90, 0x90, 0x90, 0x90,
                           0x90, 0x90, 0x90 };

  const char* nops[16] = {
    NULL,
    nop1, nop2, nop3, nop4, nop5, nop6, nop7,
    nop8, nop9, nop10, nop11, nop12, nop13, nop14, nop15
  };

  return std::string(nops[length], length);
}

// The selector for x86_64 object files.

class Target_selector_x86_64 : public Target_selector
{
public:
  Target_selector_x86_64()
    : Target_selector(elfcpp::EM_X86_64, 64, false, "elf64-x86-64")
  { }

  Target*
  do_instantiate_target()
  { return new Target_x86_64(); }
};

Target_selector_x86_64 target_selector_x86_64;

} // End anonymous namespace.
