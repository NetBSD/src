// powerpc.cc -- powerpc target support for gold.

// Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
// Written by David S. Miller <davem@davemloft.net>
//        and David Edelsohn <edelsohn@gnu.org>

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

#include "elfcpp.h"
#include "parameters.h"
#include "reloc.h"
#include "powerpc.h"
#include "object.h"
#include "symtab.h"
#include "layout.h"
#include "output.h"
#include "copy-relocs.h"
#include "target.h"
#include "target-reloc.h"
#include "target-select.h"
#include "tls.h"
#include "errors.h"
#include "gc.h"

namespace
{

using namespace gold;

template<int size, bool big_endian>
class Output_data_plt_powerpc;

template<int size, bool big_endian>
class Target_powerpc : public Sized_target<size, big_endian>
{
 public:
  typedef Output_data_reloc<elfcpp::SHT_RELA, true, size, big_endian> Reloc_section;

  Target_powerpc()
    : Sized_target<size, big_endian>(&powerpc_info),
      got_(NULL), got2_(NULL), toc_(NULL),
      plt_(NULL), rela_dyn_(NULL),
      copy_relocs_(elfcpp::R_POWERPC_COPY),
      dynbss_(NULL), got_mod_index_offset_(-1U)
  {
  }

  // Process the relocations to determine unreferenced sections for 
  // garbage collection.
  void
  gc_process_relocs(Symbol_table* symtab,
	            Layout* layout,
	            Sized_relobj<size, big_endian>* object,
	            unsigned int data_shndx,
	            unsigned int sh_type,
	            const unsigned char* prelocs,
	            size_t reloc_count,
	            Output_section* output_section,
	            bool needs_special_offset_handling,
	            size_t local_symbol_count,
	            const unsigned char* plocal_symbols);

  // Scan the relocations to look for symbol adjustments.
  void
  scan_relocs(Symbol_table* symtab,
	      Layout* layout,
	      Sized_relobj<size, big_endian>* object,
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
  do_finalize_sections(Layout*, const Input_objects*, Symbol_table*);

  // Return the value to use for a dynamic which requires special
  // treatment.
  uint64_t
  do_dynsym_value(const Symbol*) const;

  // Relocate a section.
  void
  relocate_section(const Relocate_info<size, big_endian>*,
		   unsigned int sh_type,
		   const unsigned char* prelocs,
		   size_t reloc_count,
		   Output_section* output_section,
		   bool needs_special_offset_handling,
		   unsigned char* view,
		   typename elfcpp::Elf_types<size>::Elf_Addr view_address,
		   section_size_type view_size,
		   const Reloc_symbol_changes*);

  // Scan the relocs during a relocatable link.
  void
  scan_relocatable_relocs(Symbol_table* symtab,
			  Layout* layout,
			  Sized_relobj<size, big_endian>* object,
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
  relocate_for_relocatable(const Relocate_info<size, big_endian>*,
			   unsigned int sh_type,
			   const unsigned char* prelocs,
			   size_t reloc_count,
			   Output_section* output_section,
			   off_t offset_in_output_section,
			   const Relocatable_relocs*,
			   unsigned char* view,
			   typename elfcpp::Elf_types<size>::Elf_Addr view_address,
			   section_size_type view_size,
			   unsigned char* reloc_view,
			   section_size_type reloc_view_size);

  // Return whether SYM is defined by the ABI.
  bool
  do_is_defined_by_abi(const Symbol* sym) const
  {
    return strcmp(sym->name(), "___tls_get_addr") == 0;
  }

  // Return the size of the GOT section.
  section_size_type
  got_size() const
  {
    gold_assert(this->got_ != NULL);
    return this->got_->data_size();
  }

  // Return the number of entries in the GOT.
  unsigned int
  got_entry_count() const
  {
    if (this->got_ == NULL)
      return 0;
    return this->got_size() / (size / 8);
  }

  // Return the number of entries in the PLT.
  unsigned int
  plt_entry_count() const;

  // Return the offset of the first non-reserved PLT entry.
  unsigned int
  first_plt_entry_offset() const;

  // Return the size of each PLT entry.
  unsigned int
  plt_entry_size() const;

 private:

  // The class which scans relocations.
  class Scan
  {
  public:
    Scan()
      : issued_non_pic_error_(false)
    { }

    inline void
    local(Symbol_table* symtab, Layout* layout, Target_powerpc* target,
	  Sized_relobj<size, big_endian>* object,
	  unsigned int data_shndx,
	  Output_section* output_section,
	  const elfcpp::Rela<size, big_endian>& reloc, unsigned int r_type,
	  const elfcpp::Sym<size, big_endian>& lsym);

    inline void
    global(Symbol_table* symtab, Layout* layout, Target_powerpc* target,
	   Sized_relobj<size, big_endian>* object,
	   unsigned int data_shndx,
	   Output_section* output_section,
	   const elfcpp::Rela<size, big_endian>& reloc, unsigned int r_type,
	   Symbol* gsym);

    inline bool
    local_reloc_may_be_function_pointer(Symbol_table* , Layout* ,
					Target_powerpc* ,
	          			Sized_relobj<size, big_endian>* ,
			                unsigned int ,
	          			Output_section* ,
	          			const elfcpp::Rela<size, big_endian>& ,
					unsigned int ,
	          			const elfcpp::Sym<size, big_endian>&)
    { return false; }

    inline bool
    global_reloc_may_be_function_pointer(Symbol_table* , Layout* ,
					 Target_powerpc* ,
		   			 Sized_relobj<size, big_endian>* ,
		   			 unsigned int ,
		   			 Output_section* ,
		   			 const elfcpp::Rela<size,
							    big_endian>& ,
					 unsigned int , Symbol*)
    { return false; }

  private:
    static void
    unsupported_reloc_local(Sized_relobj<size, big_endian>*,
			    unsigned int r_type);

    static void
    unsupported_reloc_global(Sized_relobj<size, big_endian>*,
			     unsigned int r_type, Symbol*);

    static void
    generate_tls_call(Symbol_table* symtab, Layout* layout,
		      Target_powerpc* target);

    void
    check_non_pic(Relobj*, unsigned int r_type);

    // Whether we have issued an error about a non-PIC compilation.
    bool issued_non_pic_error_;
  };

  // The class which implements relocation.
  class Relocate
  {
   public:
    // Do a relocation.  Return false if the caller should not issue
    // any warnings about this relocation.
    inline bool
    relocate(const Relocate_info<size, big_endian>*, Target_powerpc*,
	     Output_section*, size_t relnum,
	     const elfcpp::Rela<size, big_endian>&,
	     unsigned int r_type, const Sized_symbol<size>*,
	     const Symbol_value<size>*,
	     unsigned char*,
	     typename elfcpp::Elf_types<size>::Elf_Addr,
	     section_size_type);

   private:
    // Do a TLS relocation.
    inline void
    relocate_tls(const Relocate_info<size, big_endian>*,
		 Target_powerpc* target,
                 size_t relnum, const elfcpp::Rela<size, big_endian>&,
		 unsigned int r_type, const Sized_symbol<size>*,
		 const Symbol_value<size>*,
		 unsigned char*,
		 typename elfcpp::Elf_types<size>::Elf_Addr,
		 section_size_type);
  };

  // A class which returns the size required for a relocation type,
  // used while scanning relocs during a relocatable link.
  class Relocatable_size_for_reloc
  {
   public:
    unsigned int
    get_size_for_reloc(unsigned int, Relobj*);
  };

  // Get the GOT section, creating it if necessary.
  Output_data_got<size, big_endian>*
  got_section(Symbol_table*, Layout*);

  Output_data_space*
  got2_section() const
  {
    gold_assert(this->got2_ != NULL);
    return this->got2_;
  }

  // Get the TOC section.
  Output_data_space*
  toc_section() const
  {
    gold_assert(this->toc_ != NULL);
    return this->toc_;
  }

  // Create a PLT entry for a global symbol.
  void
  make_plt_entry(Symbol_table*, Layout*, Symbol*);

  // Create a GOT entry for the TLS module index.
  unsigned int
  got_mod_index_entry(Symbol_table* symtab, Layout* layout,
		      Sized_relobj<size, big_endian>* object);

  // Get the PLT section.
  const Output_data_plt_powerpc<size, big_endian>*
  plt_section() const
  {
    gold_assert(this->plt_ != NULL);
    return this->plt_;
  }

  // Get the dynamic reloc section, creating it if necessary.
  Reloc_section*
  rela_dyn_section(Layout*);

  // Copy a relocation against a global symbol.
  void
  copy_reloc(Symbol_table* symtab, Layout* layout,
             Sized_relobj<size, big_endian>* object,
	     unsigned int shndx, Output_section* output_section,
	     Symbol* sym, const elfcpp::Rela<size, big_endian>& reloc)
  {
    this->copy_relocs_.copy_reloc(symtab, layout,
				  symtab->get_sized_symbol<size>(sym),
				  object, shndx, output_section,
				  reloc, this->rela_dyn_section(layout));
  }

  // Information about this specific target which we pass to the
  // general Target structure.
  static Target::Target_info powerpc_info;

  // The types of GOT entries needed for this platform.
  // These values are exposed to the ABI in an incremental link.
  // Do not renumber existing values without changing the version
  // number of the .gnu_incremental_inputs section.
  enum Got_type
  {
    GOT_TYPE_STANDARD = 0,      // GOT entry for a regular symbol
    GOT_TYPE_TLS_OFFSET = 1,    // GOT entry for TLS offset
    GOT_TYPE_TLS_PAIR = 2,      // GOT entry for TLS module/offset pair
  };

  // The GOT section.
  Output_data_got<size, big_endian>* got_;
  // The GOT2 section.
  Output_data_space* got2_;
  // The TOC section.
  Output_data_space* toc_;
  // The PLT section.
  Output_data_plt_powerpc<size, big_endian>* plt_;
  // The dynamic reloc section.
  Reloc_section* rela_dyn_;
  // Relocs saved to avoid a COPY reloc.
  Copy_relocs<elfcpp::SHT_RELA, size, big_endian> copy_relocs_;
  // Space for variables copied with a COPY reloc.
  Output_data_space* dynbss_;
  // Offset of the GOT entry for the TLS module index;
  unsigned int got_mod_index_offset_;
};

template<>
Target::Target_info Target_powerpc<32, true>::powerpc_info =
{
  32,			// size
  true,			// is_big_endian
  elfcpp::EM_PPC,	// machine_code
  false,		// has_make_symbol
  false,		// has_resolve
  false,		// has_code_fill
  true,			// is_default_stack_executable
  '\0',			// wrap_char
  "/usr/lib/ld.so.1",	// dynamic_linker
  0x10000000,		// default_text_segment_address
  64 * 1024,		// abi_pagesize (overridable by -z max-page-size)
  4 * 1024,		// common_pagesize (overridable by -z common-page-size)
  elfcpp::SHN_UNDEF,	// small_common_shndx
  elfcpp::SHN_UNDEF,	// large_common_shndx
  0,			// small_common_section_flags
  0,			// large_common_section_flags
  NULL,			// attributes_section
  NULL			// attributes_vendor
};

template<>
Target::Target_info Target_powerpc<32, false>::powerpc_info =
{
  32,			// size
  false,		// is_big_endian
  elfcpp::EM_PPC,	// machine_code
  false,		// has_make_symbol
  false,		// has_resolve
  false,		// has_code_fill
  true,			// is_default_stack_executable
  '\0',			// wrap_char
  "/usr/lib/ld.so.1",	// dynamic_linker
  0x10000000,		// default_text_segment_address
  64 * 1024,		// abi_pagesize (overridable by -z max-page-size)
  4 * 1024,		// common_pagesize (overridable by -z common-page-size)
  elfcpp::SHN_UNDEF,	// small_common_shndx
  elfcpp::SHN_UNDEF,	// large_common_shndx
  0,			// small_common_section_flags
  0,			// large_common_section_flags
  NULL,			// attributes_section
  NULL			// attributes_vendor
};

template<>
Target::Target_info Target_powerpc<64, true>::powerpc_info =
{
  64,			// size
  true,			// is_big_endian
  elfcpp::EM_PPC64,	// machine_code
  false,		// has_make_symbol
  false,		// has_resolve
  false,		// has_code_fill
  true,			// is_default_stack_executable
  '\0',			// wrap_char
  "/usr/lib/ld.so.1",	// dynamic_linker
  0x10000000,		// default_text_segment_address
  64 * 1024,		// abi_pagesize (overridable by -z max-page-size)
  8 * 1024,		// common_pagesize (overridable by -z common-page-size)
  elfcpp::SHN_UNDEF,	// small_common_shndx
  elfcpp::SHN_UNDEF,	// large_common_shndx
  0,			// small_common_section_flags
  0,			// large_common_section_flags
  NULL,			// attributes_section
  NULL			// attributes_vendor
};

template<>
Target::Target_info Target_powerpc<64, false>::powerpc_info =
{
  64,			// size
  false,		// is_big_endian
  elfcpp::EM_PPC64,	// machine_code
  false,		// has_make_symbol
  false,		// has_resolve
  false,		// has_code_fill
  true,			// is_default_stack_executable
  '\0',			// wrap_char
  "/usr/lib/ld.so.1",	// dynamic_linker
  0x10000000,		// default_text_segment_address
  64 * 1024,		// abi_pagesize (overridable by -z max-page-size)
  8 * 1024,		// common_pagesize (overridable by -z common-page-size)
  elfcpp::SHN_UNDEF,	// small_common_shndx
  elfcpp::SHN_UNDEF,	// large_common_shndx
  0,			// small_common_section_flags
  0,			// large_common_section_flags
  NULL,			// attributes_section
  NULL			// attributes_vendor
};

template<int size, bool big_endian>
class Powerpc_relocate_functions
{
private:
  // Do a simple relocation with the addend in the relocation.
  template<int valsize>
  static inline void
  rela(unsigned char* view,
       unsigned int right_shift,
       elfcpp::Elf_Xword dst_mask,
       typename elfcpp::Swap<size, big_endian>::Valtype value,
       typename elfcpp::Swap<size, big_endian>::Valtype addend)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype val = elfcpp::Swap<valsize, big_endian>::readval(wv);
    Valtype reloc = ((value + addend) >> right_shift);

    val &= ~dst_mask;
    reloc &= dst_mask;

    elfcpp::Swap<valsize, big_endian>::writeval(wv, val | reloc);
  }

  // Do a simple relocation using a symbol value with the addend in
  // the relocation.
  template<int valsize>
  static inline void
  rela(unsigned char* view,
       unsigned int right_shift,
       elfcpp::Elf_Xword dst_mask,
       const Sized_relobj<size, big_endian>* object,
       const Symbol_value<size>* psymval,
       typename elfcpp::Swap<valsize, big_endian>::Valtype addend)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype val = elfcpp::Swap<valsize, big_endian>::readval(wv);
    Valtype reloc = (psymval->value(object, addend) >> right_shift);

    val &= ~dst_mask;
    reloc &= dst_mask;

    elfcpp::Swap<valsize, big_endian>::writeval(wv, val | reloc);
  }

  // Do a simple relocation using a symbol value with the addend in
  // the relocation, unaligned.
  template<int valsize>
  static inline void
  rela_ua(unsigned char* view, unsigned int right_shift,
	  elfcpp::Elf_Xword dst_mask,
	  const Sized_relobj<size, big_endian>* object,
	  const Symbol_value<size>* psymval,
	  typename elfcpp::Swap<size, big_endian>::Valtype addend)
  {
    typedef typename elfcpp::Swap_unaligned<valsize,
	    big_endian>::Valtype Valtype;
    unsigned char* wv = view;
    Valtype val = elfcpp::Swap_unaligned<valsize, big_endian>::readval(wv);
    Valtype reloc = (psymval->value(object, addend) >> right_shift);

    val &= ~dst_mask;
    reloc &= dst_mask;

    elfcpp::Swap_unaligned<valsize, big_endian>::writeval(wv, val | reloc);
  }

  // Do a simple PC relative relocation with a Symbol_value with the
  // addend in the relocation.
  template<int valsize>
  static inline void
  pcrela(unsigned char* view, unsigned int right_shift,
	 elfcpp::Elf_Xword dst_mask,
	 const Sized_relobj<size, big_endian>* object,
	 const Symbol_value<size>* psymval,
	 typename elfcpp::Swap<size, big_endian>::Valtype addend,
	 typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype val = elfcpp::Swap<valsize, big_endian>::readval(wv);
    Valtype reloc = ((psymval->value(object, addend) - address)
		     >> right_shift);

    val &= ~dst_mask;
    reloc &= dst_mask;

    elfcpp::Swap<valsize, big_endian>::writeval(wv, val | reloc);
  }

  template<int valsize>
  static inline void
  pcrela_unaligned(unsigned char* view,
		   const Sized_relobj<size, big_endian>* object,
		   const Symbol_value<size>* psymval,
		   typename elfcpp::Swap<size, big_endian>::Valtype addend,
		   typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    typedef typename elfcpp::Swap_unaligned<valsize,
	    big_endian>::Valtype Valtype;
    unsigned char* wv = view;
    Valtype reloc = (psymval->value(object, addend) - address);

    elfcpp::Swap_unaligned<valsize, big_endian>::writeval(wv, reloc);
  }

  typedef Powerpc_relocate_functions<size, big_endian> This;
  typedef Relocate_functions<size, big_endian> This_reloc;
public:
  // R_POWERPC_REL32: (Symbol + Addend - Address)
  static inline void
  rel32(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval,
	typename elfcpp::Elf_types<size>::Elf_Addr addend,
	typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This_reloc::pcrela32(view, object, psymval, addend, address); }

  // R_POWERPC_REL24: (Symbol + Addend - Address) & 0x3fffffc
  static inline void
  rel24(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval,
	typename elfcpp::Elf_types<size>::Elf_Addr addend,
	typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    This::template pcrela<32>(view, 0, 0x03fffffc, object,
			      psymval, addend, address);
  }

  // R_POWERPC_REL14: (Symbol + Addend - Address) & 0xfffc
  static inline void
  rel14(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval,
	typename elfcpp::Elf_types<size>::Elf_Addr addend,
	typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    This::template pcrela<32>(view, 0, 0x0000fffc, object,
			      psymval, addend, address);
  }

  // R_POWERPC_ADDR16: (Symbol + Addend) & 0xffff
  static inline void
  addr16(unsigned char* view,
	 typename elfcpp::Elf_types<size>::Elf_Addr value,
	 typename elfcpp::Elf_types<size>::Elf_Addr addend)
  { This_reloc::rela16(view, value, addend); }

  static inline void
  addr16(unsigned char* view,
	 const Sized_relobj<size, big_endian>* object,
	 const Symbol_value<size>* psymval,
	 typename elfcpp::Elf_types<size>::Elf_Addr addend)
  { This_reloc::rela16(view, object, psymval, addend); }

  // R_POWERPC_ADDR16_DS: (Symbol + Addend) & 0xfffc
  static inline void
  addr16_ds(unsigned char* view,
	    typename elfcpp::Elf_types<size>::Elf_Addr value,
	    typename elfcpp::Elf_types<size>::Elf_Addr addend)
  {
    This::template rela<16>(view, 0, 0xfffc, value, addend);
  }

  // R_POWERPC_ADDR16_LO: (Symbol + Addend) & 0xffff
  static inline void
  addr16_lo(unsigned char* view,
	 typename elfcpp::Elf_types<size>::Elf_Addr value,
	 typename elfcpp::Elf_types<size>::Elf_Addr addend)
  { This_reloc::rela16(view, value, addend); }

  static inline void
  addr16_lo(unsigned char* view,
	    const Sized_relobj<size, big_endian>* object,
	    const Symbol_value<size>* psymval,
	    typename elfcpp::Elf_types<size>::Elf_Addr addend)
  { This_reloc::rela16(view, object, psymval, addend); }

  // R_POWERPC_ADDR16_HI: ((Symbol + Addend) >> 16) & 0xffff
  static inline void
  addr16_hi(unsigned char* view,
	    typename elfcpp::Elf_types<size>::Elf_Addr value,
	    typename elfcpp::Elf_types<size>::Elf_Addr addend)
  {
    This::template rela<16>(view, 16, 0xffff, value, addend);
  }

  static inline void
  addr16_hi(unsigned char* view,
	    const Sized_relobj<size, big_endian>* object,
	    const Symbol_value<size>* psymval,
	    typename elfcpp::Elf_types<size>::Elf_Addr addend)
  {
    This::template rela<16>(view, 16, 0xffff, object, psymval, addend);
  }

  // R_POWERPC_ADDR16_HA: Same as R_POWERPC_ADDR16_HI except that if the
  //                      final value of the low 16 bits of the
  //                      relocation is negative, add one.
  static inline void
  addr16_ha(unsigned char* view,
	    typename elfcpp::Elf_types<size>::Elf_Addr value,
	    typename elfcpp::Elf_types<size>::Elf_Addr addend)
  {
    typename elfcpp::Elf_types<size>::Elf_Addr reloc;

    reloc = value + addend;

    if (reloc & 0x8000)
      reloc += 0x10000;
    reloc >>= 16;

    elfcpp::Swap<16, big_endian>::writeval(view, reloc);
  }

  static inline void
  addr16_ha(unsigned char* view,
	    const Sized_relobj<size, big_endian>* object,
	    const Symbol_value<size>* psymval,
	    typename elfcpp::Elf_types<size>::Elf_Addr addend)
  {
    typename elfcpp::Elf_types<size>::Elf_Addr reloc;

    reloc = psymval->value(object, addend);

    if (reloc & 0x8000)
      reloc += 0x10000;
    reloc >>= 16;

    elfcpp::Swap<16, big_endian>::writeval(view, reloc);
  }

  // R_PPC_REL16: (Symbol + Addend - Address) & 0xffff
  static inline void
  rel16(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval,
	typename elfcpp::Elf_types<size>::Elf_Addr addend,
	typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This_reloc::pcrela16(view, object, psymval, addend, address); }

  // R_PPC_REL16_LO: (Symbol + Addend - Address) & 0xffff
  static inline void
  rel16_lo(unsigned char* view,
	   const Sized_relobj<size, big_endian>* object,
	   const Symbol_value<size>* psymval,
	   typename elfcpp::Elf_types<size>::Elf_Addr addend,
	   typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This_reloc::pcrela16(view, object, psymval, addend, address); }

  // R_PPC_REL16_HI: ((Symbol + Addend - Address) >> 16) & 0xffff
  static inline void
  rel16_hi(unsigned char* view,
	   const Sized_relobj<size, big_endian>* object,
	   const Symbol_value<size>* psymval,
	   typename elfcpp::Elf_types<size>::Elf_Addr addend,
	   typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    This::template pcrela<16>(view, 16, 0xffff, object,
			      psymval, addend, address);
  }

  // R_PPC_REL16_HA: Same as R_PPC_REL16_HI except that if the
  //                 final value of the low 16 bits of the
  //                 relocation is negative, add one.
  static inline void
  rel16_ha(unsigned char* view,
	   const Sized_relobj<size, big_endian>* object,
	   const Symbol_value<size>* psymval,
	   typename elfcpp::Elf_types<size>::Elf_Addr addend,
	   typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    typename elfcpp::Elf_types<size>::Elf_Addr reloc;

    reloc = (psymval->value(object, addend) - address);
    if (reloc & 0x8000)
      reloc += 0x10000;
    reloc >>= 16;

    elfcpp::Swap<16, big_endian>::writeval(view, reloc);
  }
};

// Get the GOT section, creating it if necessary.

template<int size, bool big_endian>
Output_data_got<size, big_endian>*
Target_powerpc<size, big_endian>::got_section(Symbol_table* symtab,
					      Layout* layout)
{
  if (this->got_ == NULL)
    {
      gold_assert(symtab != NULL && layout != NULL);

      this->got_ = new Output_data_got<size, big_endian>();

      layout->add_output_section_data(".got", elfcpp::SHT_PROGBITS,
				      elfcpp::SHF_ALLOC | elfcpp::SHF_WRITE,
				      this->got_, ORDER_DATA, false);

      // Create the GOT2 or TOC in the .got section.
      if (size == 32)
	{
	  this->got2_ = new Output_data_space(4, "** GOT2");
	  layout->add_output_section_data(".got2", elfcpp::SHT_PROGBITS,
					  elfcpp::SHF_ALLOC
					  | elfcpp::SHF_WRITE,
					  this->got2_, ORDER_DATA, false);
	}
      else
	{
	  this->toc_ = new Output_data_space(8, "** TOC");
	  layout->add_output_section_data(".toc", elfcpp::SHT_PROGBITS,
					  elfcpp::SHF_ALLOC
					  | elfcpp::SHF_WRITE,
					  this->toc_, ORDER_DATA, false);
	}

      // Define _GLOBAL_OFFSET_TABLE_ at the start of the .got section.
      symtab->define_in_output_data("_GLOBAL_OFFSET_TABLE_", NULL,
				    Symbol_table::PREDEFINED,
				    this->got_,
				    0, 0, elfcpp::STT_OBJECT,
				    elfcpp::STB_LOCAL,
				    elfcpp::STV_HIDDEN, 0,
				    false, false);
    }

  return this->got_;
}

// Get the dynamic reloc section, creating it if necessary.

template<int size, bool big_endian>
typename Target_powerpc<size, big_endian>::Reloc_section*
Target_powerpc<size, big_endian>::rela_dyn_section(Layout* layout)
{
  if (this->rela_dyn_ == NULL)
    {
      gold_assert(layout != NULL);
      this->rela_dyn_ = new Reloc_section(parameters->options().combreloc());
      layout->add_output_section_data(".rela.dyn", elfcpp::SHT_RELA,
				      elfcpp::SHF_ALLOC, this->rela_dyn_,
				      ORDER_DYNAMIC_RELOCS, false);
    }
  return this->rela_dyn_;
}

// A class to handle the PLT data.

template<int size, bool big_endian>
class Output_data_plt_powerpc : public Output_section_data
{
 public:
  typedef Output_data_reloc<elfcpp::SHT_RELA, true,
			    size, big_endian> Reloc_section;

  Output_data_plt_powerpc(Layout*);

  // Add an entry to the PLT.
  void add_entry(Symbol* gsym);

  // Return the .rela.plt section data.
  const Reloc_section* rel_plt() const
 {
    return this->rel_;
  }

  // Return the number of PLT entries.
  unsigned int
  entry_count() const
  { return this->count_; }

  // Return the offset of the first non-reserved PLT entry.
  static unsigned int
  first_plt_entry_offset()
  { return 4 * base_plt_entry_size; }

  // Return the size of a PLT entry.
  static unsigned int
  get_plt_entry_size()
  { return base_plt_entry_size; }

 protected:
  void do_adjust_output_section(Output_section* os);

 private:
  // The size of an entry in the PLT.
  static const int base_plt_entry_size = (size == 32 ? 16 : 24);

  // Set the final size.
  void
  set_final_data_size()
  {
    unsigned int full_count = this->count_ + 4;

    this->set_data_size(full_count * base_plt_entry_size);
  }

  // Write out the PLT data.
  void
  do_write(Output_file*);

  // The reloc section.
  Reloc_section* rel_;
  // The number of PLT entries.
  unsigned int count_;
};

// Create the PLT section.  The ordinary .got section is an argument,
// since we need to refer to the start.

template<int size, bool big_endian>
Output_data_plt_powerpc<size, big_endian>::Output_data_plt_powerpc(Layout* layout)
  : Output_section_data(size == 32 ? 4 : 8), count_(0)
{
  this->rel_ = new Reloc_section(false);
  layout->add_output_section_data(".rela.plt", elfcpp::SHT_RELA,
				  elfcpp::SHF_ALLOC, this->rel_,
				  ORDER_DYNAMIC_PLT_RELOCS, false);
}

template<int size, bool big_endian>
void
Output_data_plt_powerpc<size, big_endian>::do_adjust_output_section(Output_section* os)
{
  os->set_entsize(0);
}

// Add an entry to the PLT.

template<int size, bool big_endian>
void
Output_data_plt_powerpc<size, big_endian>::add_entry(Symbol* gsym)
{
  gold_assert(!gsym->has_plt_offset());
  unsigned int index = this->count_+ + 4;
  section_offset_type plt_offset;

  if (index < 8192)
    plt_offset = index * base_plt_entry_size;
  else
    gold_unreachable();

  gsym->set_plt_offset(plt_offset);

  ++this->count_;

  gsym->set_needs_dynsym_entry();
  this->rel_->add_global(gsym, elfcpp::R_POWERPC_JMP_SLOT, this,
			 plt_offset, 0);
}

static const unsigned int addis_11_11     = 0x3d6b0000;
static const unsigned int addis_11_30     = 0x3d7e0000;
static const unsigned int addis_12_12     = 0x3d8c0000;
static const unsigned int addi_11_11      = 0x396b0000;
static const unsigned int add_0_11_11     = 0x7c0b5a14;
static const unsigned int add_11_0_11     = 0x7d605a14;
static const unsigned int b               = 0x48000000;
static const unsigned int bcl_20_31       = 0x429f0005;
static const unsigned int bctr            = 0x4e800420;
static const unsigned int lis_11          = 0x3d600000;
static const unsigned int lis_12          = 0x3d800000;
static const unsigned int lwzu_0_12       = 0x840c0000;
static const unsigned int lwz_0_12        = 0x800c0000;
static const unsigned int lwz_11_11       = 0x816b0000;
static const unsigned int lwz_11_30       = 0x817e0000;
static const unsigned int lwz_12_12       = 0x818c0000;
static const unsigned int mflr_0          = 0x7c0802a6;
static const unsigned int mflr_12         = 0x7d8802a6;
static const unsigned int mtctr_0         = 0x7c0903a6;
static const unsigned int mtctr_11        = 0x7d6903a6;
static const unsigned int mtlr_0          = 0x7c0803a6;
static const unsigned int nop             = 0x60000000;
static const unsigned int sub_11_11_12    = 0x7d6c5850;

static const unsigned int addis_r12_r2    = 0x3d820000;  /* addis %r12,%r2,xxx@ha     */
static const unsigned int std_r2_40r1     = 0xf8410028;  /* std   %r2,40(%r1)         */
static const unsigned int ld_r11_0r12     = 0xe96c0000;  /* ld    %r11,xxx+0@l(%r12)  */
static const unsigned int ld_r2_0r12      = 0xe84c0000;  /* ld    %r2,xxx+8@l(%r12)   */
                                                         /* ld    %r11,xxx+16@l(%r12) */


// Write out the PLT.

template<int size, bool big_endian>
void
Output_data_plt_powerpc<size, big_endian>::do_write(Output_file* of)
{
  const off_t offset = this->offset();
  const section_size_type oview_size =
    convert_to_section_size_type(this->data_size());
  unsigned char* const oview = of->get_output_view(offset, oview_size);
  unsigned char* pov = oview;

  memset(pov, 0, base_plt_entry_size * 4);
  pov += base_plt_entry_size * 4;

  unsigned int plt_offset = base_plt_entry_size * 4;
  const unsigned int count = this->count_;

  if (size == 64)
    {
      for (unsigned int i = 0; i < count; i++)
	{
	}
    }
  else
    {
      for (unsigned int i = 0; i < count; i++)
	{
	  elfcpp::Swap<32, true>::writeval(pov + 0x00,
					   lwz_11_30 + plt_offset);
	  elfcpp::Swap<32, true>::writeval(pov + 0x04, mtctr_11);
	  elfcpp::Swap<32, true>::writeval(pov + 0x08, bctr);
	  elfcpp::Swap<32, true>::writeval(pov + 0x0c, nop);
	  pov += base_plt_entry_size;
	  plt_offset += base_plt_entry_size;
	}
    }

  gold_assert(static_cast<section_size_type>(pov - oview) == oview_size);

  of->write_output_view(offset, oview_size, oview);
}

// Create a PLT entry for a global symbol.

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::make_plt_entry(Symbol_table* symtab,
						 Layout* layout,
						 Symbol* gsym)
{
  if (gsym->has_plt_offset())
    return;

  if (this->plt_ == NULL)
    {
      // Create the GOT section first.
      this->got_section(symtab, layout);

      // Ensure that .rela.dyn always appears before .rela.plt  This is
      // necessary due to how, on PowerPC and some other targets, .rela.dyn
      // needs to include .rela.plt in it's range.
      this->rela_dyn_section(layout);

      this->plt_ = new Output_data_plt_powerpc<size, big_endian>(layout);
      layout->add_output_section_data(".plt", elfcpp::SHT_PROGBITS,
				      (elfcpp::SHF_ALLOC
				       | elfcpp::SHF_EXECINSTR
				       | elfcpp::SHF_WRITE),
				      this->plt_, ORDER_PLT, false);

      // Define _PROCEDURE_LINKAGE_TABLE_ at the start of the .plt section.
      symtab->define_in_output_data("_PROCEDURE_LINKAGE_TABLE_", NULL,
				    Symbol_table::PREDEFINED,
				    this->plt_,
				    0, 0, elfcpp::STT_OBJECT,
				    elfcpp::STB_LOCAL,
				    elfcpp::STV_HIDDEN, 0,
				    false, false);
    }

  this->plt_->add_entry(gsym);
}

// Return the number of entries in the PLT.

template<int size, bool big_endian>
unsigned int
Target_powerpc<size, big_endian>::plt_entry_count() const
{
  if (this->plt_ == NULL)
    return 0;
  return this->plt_->entry_count();
}

// Return the offset of the first non-reserved PLT entry.

template<int size, bool big_endian>
unsigned int
Target_powerpc<size, big_endian>::first_plt_entry_offset() const
{
  return Output_data_plt_powerpc<size, big_endian>::first_plt_entry_offset();
}

// Return the size of each PLT entry.

template<int size, bool big_endian>
unsigned int
Target_powerpc<size, big_endian>::plt_entry_size() const
{
  return Output_data_plt_powerpc<size, big_endian>::get_plt_entry_size();
}

// Create a GOT entry for the TLS module index.

template<int size, bool big_endian>
unsigned int
Target_powerpc<size, big_endian>::got_mod_index_entry(Symbol_table* symtab,
						      Layout* layout,
						      Sized_relobj<size, big_endian>* object)
{
  if (this->got_mod_index_offset_ == -1U)
    {
      gold_assert(symtab != NULL && layout != NULL && object != NULL);
      Reloc_section* rela_dyn = this->rela_dyn_section(layout);
      Output_data_got<size, big_endian>* got;
      unsigned int got_offset;

      got = this->got_section(symtab, layout);
      got_offset = got->add_constant(0);
      rela_dyn->add_local(object, 0, elfcpp::R_POWERPC_DTPMOD, got,
			  got_offset, 0);
      got->add_constant(0);
      this->got_mod_index_offset_ = got_offset;
    }
  return this->got_mod_index_offset_;
}

// Optimize the TLS relocation type based on what we know about the
// symbol.  IS_FINAL is true if the final address of this symbol is
// known at link time.

static tls::Tls_optimization
optimize_tls_reloc(bool /* is_final */, int r_type)
{
  // If we are generating a shared library, then we can't do anything
  // in the linker.
  if (parameters->options().shared())
    return tls::TLSOPT_NONE;
  switch (r_type)
    {
      // XXX
    default:
      gold_unreachable();
    }
}

// Report an unsupported relocation against a local symbol.

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::Scan::unsupported_reloc_local(
			Sized_relobj<size, big_endian>* object,
			unsigned int r_type)
{
  gold_error(_("%s: unsupported reloc %u against local symbol"),
	     object->name().c_str(), r_type);
}

// We are about to emit a dynamic relocation of type R_TYPE.  If the
// dynamic linker does not support it, issue an error.

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::Scan::check_non_pic(Relobj* object,
						      unsigned int r_type)
{
  gold_assert(r_type != elfcpp::R_POWERPC_NONE);

  // These are the relocation types supported by glibc for both 32-bit
  // and 64-bit powerpc.
  switch (r_type)
    {
    case elfcpp::R_POWERPC_RELATIVE:
    case elfcpp::R_POWERPC_GLOB_DAT:
    case elfcpp::R_POWERPC_DTPMOD:
    case elfcpp::R_POWERPC_DTPREL:
    case elfcpp::R_POWERPC_TPREL:
    case elfcpp::R_POWERPC_JMP_SLOT:
    case elfcpp::R_POWERPC_COPY:
    case elfcpp::R_POWERPC_ADDR32:
    case elfcpp::R_POWERPC_ADDR24:
    case elfcpp::R_POWERPC_REL24:
      return;

    default:
      break;
    }

  if (size == 64)
    {
      switch (r_type)
	{
	  // These are the relocation types supported only on 64-bit.
	case elfcpp::R_PPC64_ADDR64:
	case elfcpp::R_PPC64_TPREL16_LO_DS:
	case elfcpp::R_PPC64_TPREL16_DS:
	case elfcpp::R_POWERPC_TPREL16:
	case elfcpp::R_POWERPC_TPREL16_LO:
	case elfcpp::R_POWERPC_TPREL16_HI:
	case elfcpp::R_POWERPC_TPREL16_HA:
	case elfcpp::R_PPC64_TPREL16_HIGHER:
	case elfcpp::R_PPC64_TPREL16_HIGHEST:
	case elfcpp::R_PPC64_TPREL16_HIGHERA:
	case elfcpp::R_PPC64_TPREL16_HIGHESTA:
	case elfcpp::R_PPC64_ADDR16_LO_DS:
	case elfcpp::R_POWERPC_ADDR16_LO:
	case elfcpp::R_POWERPC_ADDR16_HI:
	case elfcpp::R_POWERPC_ADDR16_HA:
	case elfcpp::R_POWERPC_ADDR30:
	case elfcpp::R_PPC64_UADDR64:
	case elfcpp::R_POWERPC_UADDR32:
	case elfcpp::R_POWERPC_ADDR16:
	case elfcpp::R_POWERPC_UADDR16:
	case elfcpp::R_PPC64_ADDR16_DS:
	case elfcpp::R_PPC64_ADDR16_HIGHER:
	case elfcpp::R_PPC64_ADDR16_HIGHEST:
	case elfcpp::R_PPC64_ADDR16_HIGHERA:
	case elfcpp::R_PPC64_ADDR16_HIGHESTA:
	case elfcpp::R_POWERPC_ADDR14_BRTAKEN:
	case elfcpp::R_POWERPC_ADDR14_BRNTAKEN:
	case elfcpp::R_POWERPC_REL32:
	case elfcpp::R_PPC64_REL64:
	  return;

	default:
	  break;
	}
    }
  else
    {
      switch (r_type)
	{
	  // These are the relocation types supported only on 32-bit.

	default:
	  break;
	}
    }

  // This prevents us from issuing more than one error per reloc
  // section.  But we can still wind up issuing more than one
  // error per object file.
  if (this->issued_non_pic_error_)
    return;
  gold_assert(parameters->options().output_is_position_independent());
  object->error(_("requires unsupported dynamic reloc; "
		  "recompile with -fPIC"));
  this->issued_non_pic_error_ = true;
  return;
}

// Scan a relocation for a local symbol.

template<int size, bool big_endian>
inline void
Target_powerpc<size, big_endian>::Scan::local(
			Symbol_table* symtab,
			Layout* layout,
			Target_powerpc<size, big_endian>* target,
			Sized_relobj<size, big_endian>* object,
			unsigned int data_shndx,
			Output_section* output_section,
			const elfcpp::Rela<size, big_endian>& reloc,
			unsigned int r_type,
			const elfcpp::Sym<size, big_endian>& lsym)
{
  switch (r_type)
    {
    case elfcpp::R_POWERPC_NONE:
    case elfcpp::R_POWERPC_GNU_VTINHERIT:
    case elfcpp::R_POWERPC_GNU_VTENTRY:
      break;

    case elfcpp::R_PPC64_ADDR64:
    case elfcpp::R_POWERPC_ADDR32:
    case elfcpp::R_POWERPC_ADDR16_HA:
    case elfcpp::R_POWERPC_ADDR16_LO:
      // If building a shared library (or a position-independent
      // executable), we need to create a dynamic relocation for
      // this location.
      if (parameters->options().output_is_position_independent())
        {
          Reloc_section* rela_dyn = target->rela_dyn_section(layout);

	  check_non_pic(object, r_type);
          if (lsym.get_st_type() != elfcpp::STT_SECTION)
            {
              unsigned int r_sym = elfcpp::elf_r_sym<size>(reloc.get_r_info());
              rela_dyn->add_local(object, r_sym, r_type, output_section,
				  data_shndx, reloc.get_r_offset(),
				  reloc.get_r_addend());
            }
          else
            {
	      unsigned int r_sym = elfcpp::elf_r_sym<size>(reloc.get_r_info());
              gold_assert(lsym.get_st_value() == 0);
              rela_dyn->add_local_relative(object, r_sym, r_type,
					   output_section, data_shndx,
					   reloc.get_r_offset(),
					   reloc.get_r_addend());
            }
        }
      break;

    case elfcpp::R_POWERPC_REL24:
    case elfcpp::R_PPC_LOCAL24PC:
    case elfcpp::R_POWERPC_REL32:
    case elfcpp::R_PPC_REL16_LO:
    case elfcpp::R_PPC_REL16_HA:
      break;

    case elfcpp::R_POWERPC_GOT16:
    case elfcpp::R_POWERPC_GOT16_LO:
    case elfcpp::R_POWERPC_GOT16_HI:
    case elfcpp::R_POWERPC_GOT16_HA:
    case elfcpp::R_PPC64_TOC16:
    case elfcpp::R_PPC64_TOC16_LO:
    case elfcpp::R_PPC64_TOC16_HI:
    case elfcpp::R_PPC64_TOC16_HA:
    case elfcpp::R_PPC64_TOC16_DS:
    case elfcpp::R_PPC64_TOC16_LO_DS:
      {
        // The symbol requires a GOT entry.
        Output_data_got<size, big_endian>* got;
	unsigned int r_sym;

	got = target->got_section(symtab, layout);
	r_sym = elfcpp::elf_r_sym<size>(reloc.get_r_info());

	// If we are generating a shared object, we need to add a
	// dynamic relocation for this symbol's GOT entry.
	if (parameters->options().output_is_position_independent())
	  {
	    if (!object->local_has_got_offset(r_sym, GOT_TYPE_STANDARD))
	      {
		Reloc_section* rela_dyn = target->rela_dyn_section(layout);
		unsigned int off;

		off = got->add_constant(0);
		object->set_local_got_offset(r_sym, GOT_TYPE_STANDARD, off);
		rela_dyn->add_local_relative(object, r_sym,
					     elfcpp::R_POWERPC_RELATIVE,
					     got, off, 0);
	      }
          }
	else
	  got->add_local(object, r_sym, GOT_TYPE_STANDARD);
      }
      break;

    case elfcpp::R_PPC64_TOC:
      // We need a GOT section.
      target->got_section(symtab, layout);
      break;

      // These are relocations which should only be seen by the
      // dynamic linker, and should never be seen here.
    case elfcpp::R_POWERPC_COPY:
    case elfcpp::R_POWERPC_GLOB_DAT:
    case elfcpp::R_POWERPC_JMP_SLOT:
    case elfcpp::R_POWERPC_RELATIVE:
    case elfcpp::R_POWERPC_DTPMOD:
      gold_error(_("%s: unexpected reloc %u in object file"),
		 object->name().c_str(), r_type);
      break;

    default:
      unsupported_reloc_local(object, r_type);
      break;
    }
}

// Report an unsupported relocation against a global symbol.

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::Scan::unsupported_reloc_global(
			Sized_relobj<size, big_endian>* object,
			unsigned int r_type,
			Symbol* gsym)
{
  gold_error(_("%s: unsupported reloc %u against global symbol %s"),
	     object->name().c_str(), r_type, gsym->demangled_name().c_str());
}

// Scan a relocation for a global symbol.

template<int size, bool big_endian>
inline void
Target_powerpc<size, big_endian>::Scan::global(
				Symbol_table* symtab,
				Layout* layout,
				Target_powerpc<size, big_endian>* target,
				Sized_relobj<size, big_endian>* object,
				unsigned int data_shndx,
				Output_section* output_section,
				const elfcpp::Rela<size, big_endian>& reloc,
				unsigned int r_type,
				Symbol* gsym)
{
  switch (r_type)
    {
    case elfcpp::R_POWERPC_NONE:
    case elfcpp::R_POWERPC_GNU_VTINHERIT:
    case elfcpp::R_POWERPC_GNU_VTENTRY:
      break;

    case elfcpp::R_PPC_PLTREL24:
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

    case elfcpp::R_POWERPC_ADDR16:
    case elfcpp::R_POWERPC_ADDR16_LO:
    case elfcpp::R_POWERPC_ADDR16_HI:
    case elfcpp::R_POWERPC_ADDR16_HA:
    case elfcpp::R_POWERPC_ADDR32:
    case elfcpp::R_PPC64_ADDR64:
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
            if (gsym->may_need_copy_reloc())
              {
	        target->copy_reloc(symtab, layout, object,
	                           data_shndx, output_section, gsym, reloc);
              }
            else if ((r_type == elfcpp::R_POWERPC_ADDR32
		      || r_type == elfcpp::R_PPC64_ADDR64)
                     && gsym->can_use_relative_reloc(false))
              {
                Reloc_section* rela_dyn = target->rela_dyn_section(layout);
                rela_dyn->add_global_relative(gsym, elfcpp::R_POWERPC_RELATIVE,
					      output_section, object,
					      data_shndx, reloc.get_r_offset(),
					      reloc.get_r_addend());
              }
            else
              {
                Reloc_section* rela_dyn = target->rela_dyn_section(layout);

		check_non_pic(object, r_type);
		if (gsym->is_from_dynobj()
		    || gsym->is_undefined()
		    || gsym->is_preemptible())
		  rela_dyn->add_global(gsym, r_type, output_section,
				       object, data_shndx,
				       reloc.get_r_offset(),
				       reloc.get_r_addend());
		else
		  rela_dyn->add_global_relative(gsym, r_type,
						output_section, object,
						data_shndx,
						reloc.get_r_offset(),
						reloc.get_r_addend());
              }
          }
      }
      break;

    case elfcpp::R_POWERPC_REL24:
    case elfcpp::R_PPC_LOCAL24PC:
    case elfcpp::R_PPC_REL16:
    case elfcpp::R_PPC_REL16_LO:
    case elfcpp::R_PPC_REL16_HI:
    case elfcpp::R_PPC_REL16_HA:
      {
	if (gsym->needs_plt_entry())
	  target->make_plt_entry(symtab, layout, gsym);
	// Make a dynamic relocation if necessary.
	int flags = Symbol::NON_PIC_REF;
	if (gsym->type() == elfcpp::STT_FUNC)
	  flags |= Symbol::FUNCTION_CALL;
	if (gsym->needs_dynamic_reloc(flags))
	  {
	    if (gsym->may_need_copy_reloc())
	      {
		target->copy_reloc(symtab, layout, object,
				   data_shndx, output_section, gsym,
				   reloc);
	      }
	    else
	      {
		Reloc_section* rela_dyn = target->rela_dyn_section(layout);
		check_non_pic(object, r_type);
		rela_dyn->add_global(gsym, r_type, output_section, object,
				     data_shndx, reloc.get_r_offset(),
				     reloc.get_r_addend());
	      }
	  }
      }
      break;

    case elfcpp::R_POWERPC_GOT16:
    case elfcpp::R_POWERPC_GOT16_LO:
    case elfcpp::R_POWERPC_GOT16_HI:
    case elfcpp::R_POWERPC_GOT16_HA:
    case elfcpp::R_PPC64_TOC16:
    case elfcpp::R_PPC64_TOC16_LO:
    case elfcpp::R_PPC64_TOC16_HI:
    case elfcpp::R_PPC64_TOC16_HA:
    case elfcpp::R_PPC64_TOC16_DS:
    case elfcpp::R_PPC64_TOC16_LO_DS:
      {
        // The symbol requires a GOT entry.
        Output_data_got<size, big_endian>* got;

	got = target->got_section(symtab, layout);
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
                                        elfcpp::R_POWERPC_GLOB_DAT);
            else if (!gsym->has_got_offset(GOT_TYPE_STANDARD))
              {
		unsigned int off = got->add_constant(0);

		gsym->set_got_offset(GOT_TYPE_STANDARD, off);
		rela_dyn->add_global_relative(gsym, elfcpp::R_POWERPC_RELATIVE,
					      got, off, 0);
	      }
          }
      }
      break;

    case elfcpp::R_PPC64_TOC:
      // We need a GOT section.
      target->got_section(symtab, layout);
      break;

    case elfcpp::R_POWERPC_GOT_TPREL16:
    case elfcpp::R_POWERPC_TLS:
      // XXX TLS
      break;

      // These are relocations which should only be seen by the
      // dynamic linker, and should never be seen here.
    case elfcpp::R_POWERPC_COPY:
    case elfcpp::R_POWERPC_GLOB_DAT:
    case elfcpp::R_POWERPC_JMP_SLOT:
    case elfcpp::R_POWERPC_RELATIVE:
    case elfcpp::R_POWERPC_DTPMOD:
      gold_error(_("%s: unexpected reloc %u in object file"),
		 object->name().c_str(), r_type);
      break;

    default:
      unsupported_reloc_global(object, r_type, gsym);
      break;
    }
}

// Process relocations for gc.

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::gc_process_relocs(
			Symbol_table* symtab,
			Layout* layout,
			Sized_relobj<size, big_endian>* object,
			unsigned int data_shndx,
			unsigned int,
			const unsigned char* prelocs,
			size_t reloc_count,
			Output_section* output_section,
			bool needs_special_offset_handling,
			size_t local_symbol_count,
			const unsigned char* plocal_symbols)
{
  typedef Target_powerpc<size, big_endian> Powerpc;
  typedef typename Target_powerpc<size, big_endian>::Scan Scan;

  gold::gc_process_relocs<size, big_endian, Powerpc, elfcpp::SHT_RELA, Scan,
			  typename Target_powerpc::Relocatable_size_for_reloc>(
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

// Scan relocations for a section.

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::scan_relocs(
			Symbol_table* symtab,
			Layout* layout,
			Sized_relobj<size, big_endian>* object,
			unsigned int data_shndx,
			unsigned int sh_type,
			const unsigned char* prelocs,
			size_t reloc_count,
			Output_section* output_section,
			bool needs_special_offset_handling,
			size_t local_symbol_count,
			const unsigned char* plocal_symbols)
{
  typedef Target_powerpc<size, big_endian> Powerpc;
  typedef typename Target_powerpc<size, big_endian>::Scan Scan;
  static Output_data_space* sdata;

  if (sh_type == elfcpp::SHT_REL)
    {
      gold_error(_("%s: unsupported REL reloc section"),
		 object->name().c_str());
      return;
    }

  // Define _SDA_BASE_ at the start of the .sdata section.
  if (sdata == NULL)
  {
    // layout->find_output_section(".sdata") == NULL
    sdata = new Output_data_space(4, "** sdata");
    Output_section* os = layout->add_output_section_data(".sdata", 0,
							 elfcpp::SHF_ALLOC
							 | elfcpp::SHF_WRITE,
							 sdata,
							 ORDER_SMALL_DATA,
							 false);
    symtab->define_in_output_data("_SDA_BASE_", NULL,
				  Symbol_table::PREDEFINED,
				  os,
				  32768, 0,
				  elfcpp::STT_OBJECT,
				  elfcpp::STB_LOCAL,
				  elfcpp::STV_HIDDEN, 0,
				  false, false);
  }

  gold::scan_relocs<size, big_endian, Powerpc, elfcpp::SHT_RELA, Scan>(
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

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::do_finalize_sections(
    Layout* layout,
    const Input_objects*,
    Symbol_table*)
{
  // Fill in some more dynamic tags.
  const Reloc_section* rel_plt = (this->plt_ == NULL
				  ? NULL
				  : this->plt_->rel_plt());
  layout->add_target_dynamic_tags(false, this->plt_, rel_plt,
				  this->rela_dyn_, true, size == 32);

  // Emit any relocs we saved in an attempt to avoid generating COPY
  // relocs.
  if (this->copy_relocs_.any_saved_relocs())
    this->copy_relocs_.emit(this->rela_dyn_section(layout));
}

// Perform a relocation.

template<int size, bool big_endian>
inline bool
Target_powerpc<size, big_endian>::Relocate::relocate(
			const Relocate_info<size, big_endian>* relinfo,
			Target_powerpc* target,
			Output_section*,
			size_t relnum,
			const elfcpp::Rela<size, big_endian>& rela,
			unsigned int r_type,
			const Sized_symbol<size>* gsym,
			const Symbol_value<size>* psymval,
			unsigned char* view,
			typename elfcpp::Elf_types<size>::Elf_Addr address,
			section_size_type /* view_size */)
{
  const unsigned int toc_base_offset = 0x8000;
  typedef Powerpc_relocate_functions<size, big_endian> Reloc;

  // Pick the value to use for symbols defined in shared objects.
  Symbol_value<size> symval;
  if (gsym != NULL
      && gsym->use_plt_offset(r_type == elfcpp::R_POWERPC_REL24
			      || r_type == elfcpp::R_PPC_LOCAL24PC
			      || r_type == elfcpp::R_PPC_REL16
			      || r_type == elfcpp::R_PPC_REL16_LO
			      || r_type == elfcpp::R_PPC_REL16_HI
			      || r_type == elfcpp::R_PPC_REL16_HA))
    {
      elfcpp::Elf_Xword value;

      value = target->plt_section()->address() + gsym->plt_offset();

      symval.set_output_value(value);

      psymval = &symval;
    }

  const Sized_relobj<size, big_endian>* object = relinfo->object;
  elfcpp::Elf_Xword addend = rela.get_r_addend();

  // Get the GOT offset if needed.  Unlike i386 and x86_64, our GOT
  // pointer points to the beginning, not the end, of the table.
  // So we just use the plain offset.
  unsigned int got_offset = 0;
  unsigned int got2_offset = 0;
  switch (r_type)
    {
    case elfcpp::R_PPC64_TOC16:
    case elfcpp::R_PPC64_TOC16_LO:
    case elfcpp::R_PPC64_TOC16_HI:
    case elfcpp::R_PPC64_TOC16_HA:
    case elfcpp::R_PPC64_TOC16_DS:
    case elfcpp::R_PPC64_TOC16_LO_DS:
	// Subtract the TOC base address.
	addend -= target->toc_section()->address() + toc_base_offset;
	/* FALLTHRU */

    case elfcpp::R_POWERPC_GOT16:
    case elfcpp::R_POWERPC_GOT16_LO:
    case elfcpp::R_POWERPC_GOT16_HI:
    case elfcpp::R_POWERPC_GOT16_HA:
    case elfcpp::R_PPC64_GOT16_DS:
    case elfcpp::R_PPC64_GOT16_LO_DS:
      if (gsym != NULL)
        {
          gold_assert(gsym->has_got_offset(GOT_TYPE_STANDARD));
          got_offset = gsym->got_offset(GOT_TYPE_STANDARD);
        }
      else
        {
          unsigned int r_sym = elfcpp::elf_r_sym<size>(rela.get_r_info());
          gold_assert(object->local_has_got_offset(r_sym, GOT_TYPE_STANDARD));
          got_offset = object->local_got_offset(r_sym, GOT_TYPE_STANDARD);
        }
      break;

      // R_PPC_PLTREL24 is rather special.  If non-zero,
      // the addend specifies the GOT pointer offset within .got2.  
    case elfcpp::R_PPC_PLTREL24:
      if (addend >= 32768)
	{
	  Output_data_space* got2;
	  got2 = target->got2_section();
	  got2_offset = got2->offset();
	  addend += got2_offset;
	}
      break;

    default:
      break;
    }

  switch (r_type)
    {
    case elfcpp::R_POWERPC_NONE:
    case elfcpp::R_POWERPC_GNU_VTINHERIT:
    case elfcpp::R_POWERPC_GNU_VTENTRY:
      break;

    case elfcpp::R_POWERPC_REL32:
      Reloc::rel32(view, object, psymval, addend, address);
      break;

    case elfcpp::R_POWERPC_REL24:
      Reloc::rel24(view, object, psymval, addend, address);
      break;

    case elfcpp::R_POWERPC_REL14:
      Reloc::rel14(view, object, psymval, addend, address);
      break;

    case elfcpp::R_PPC_PLTREL24:
      Reloc::rel24(view, object, psymval, addend, address);
      break;

    case elfcpp::R_PPC_LOCAL24PC:
      Reloc::rel24(view, object, psymval, addend, address);
      break;

    case elfcpp::R_PPC64_ADDR64:
      if (!parameters->options().output_is_position_independent())
	Relocate_functions<size, big_endian>::rela64(view, object,
						     psymval, addend);
      break;

    case elfcpp::R_POWERPC_ADDR32:
      if (!parameters->options().output_is_position_independent())
	Relocate_functions<size, big_endian>::rela32(view, object,
						     psymval, addend);
      break;

    case elfcpp::R_POWERPC_ADDR16_LO:
      Reloc::addr16_lo(view, object, psymval, addend);
      break;

    case elfcpp::R_POWERPC_ADDR16_HI:
      Reloc::addr16_hi(view, object, psymval, addend);
      break;

    case elfcpp::R_POWERPC_ADDR16_HA:
      Reloc::addr16_ha(view, object, psymval, addend);
      break;

    case elfcpp::R_PPC_REL16_LO:
      Reloc::rel16_lo(view, object, psymval, addend, address);
      break;

    case elfcpp::R_PPC_REL16_HI:
      Reloc::rel16_lo(view, object, psymval, addend, address);
      break;

    case elfcpp::R_PPC_REL16_HA:
      Reloc::rel16_ha(view, object, psymval, addend, address);
      break;

    case elfcpp::R_POWERPC_GOT16:
      Reloc::addr16(view, got_offset, addend);
      break;

    case elfcpp::R_POWERPC_GOT16_LO:
      Reloc::addr16_lo(view, got_offset, addend);
      break;

    case elfcpp::R_POWERPC_GOT16_HI:
      Reloc::addr16_hi(view, got_offset, addend);
      break;

    case elfcpp::R_POWERPC_GOT16_HA:
      Reloc::addr16_ha(view, got_offset, addend);
      break;

    case elfcpp::R_PPC64_TOC16:
      Reloc::addr16(view, got_offset, addend);
      break;

    case elfcpp::R_PPC64_TOC16_LO:
      Reloc::addr16_lo(view, got_offset, addend);
      break;

    case elfcpp::R_PPC64_TOC16_HI:
      Reloc::addr16_hi(view, got_offset, addend);
      break;

    case elfcpp::R_PPC64_TOC16_HA:
      Reloc::addr16_ha(view, got_offset, addend);
      break;

    case elfcpp::R_PPC64_TOC16_DS:
    case elfcpp::R_PPC64_TOC16_LO_DS:
      Reloc::addr16_ds(view, got_offset, addend);
      break;

    case elfcpp::R_PPC64_TOC:
      {
	elfcpp::Elf_types<64>::Elf_Addr value;
	value = target->toc_section()->address() + toc_base_offset;
	Relocate_functions<64, false>::rela64(view, value, addend);
      }
      break;

    case elfcpp::R_POWERPC_COPY:
    case elfcpp::R_POWERPC_GLOB_DAT:
    case elfcpp::R_POWERPC_JMP_SLOT:
    case elfcpp::R_POWERPC_RELATIVE:
      // This is an outstanding tls reloc, which is unexpected when
      // linking.
    case elfcpp::R_POWERPC_DTPMOD:
      gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
			     _("unexpected reloc %u in object file"),
			     r_type);
      break;

    default:
      gold_error_at_location(relinfo, relnum, rela.get_r_offset(),
			     _("unsupported reloc %u"),
			     r_type);
      break;
    }

  return true;
}

// Perform a TLS relocation.

template<int size, bool big_endian>
inline void
Target_powerpc<size, big_endian>::Relocate::relocate_tls(
			const Relocate_info<size, big_endian>* relinfo,
			Target_powerpc<size, big_endian>* target,
			size_t relnum,
			const elfcpp::Rela<size, big_endian>& rela,
			unsigned int r_type,
			const Sized_symbol<size>* gsym,
			const Symbol_value<size>* psymval,
			unsigned char* view,
			typename elfcpp::Elf_types<size>::Elf_Addr address,
			section_size_type)
{
  Output_segment* tls_segment = relinfo->layout->tls_segment();
  typedef Powerpc_relocate_functions<size, big_endian> Reloc;
  const Sized_relobj<size, big_endian>* object = relinfo->object;

  const elfcpp::Elf_Xword addend = rela.get_r_addend();
  typename elfcpp::Elf_types<size>::Elf_Addr value = psymval->value(object, 0);

  const bool is_final =
    (gsym == NULL
     ? !parameters->options().output_is_position_independent()
     : gsym->final_value_is_known());
  const tls::Tls_optimization optimized_type
      = optimize_tls_reloc(is_final, r_type);

  switch (r_type)
    {
      // XXX
    }
}

// Relocate section data.

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::relocate_section(
			const Relocate_info<size, big_endian>* relinfo,
			unsigned int sh_type,
			const unsigned char* prelocs,
			size_t reloc_count,
			Output_section* output_section,
			bool needs_special_offset_handling,
			unsigned char* view,
			typename elfcpp::Elf_types<size>::Elf_Addr address,
			section_size_type view_size,
			const Reloc_symbol_changes* reloc_symbol_changes)
{
  typedef Target_powerpc<size, big_endian> Powerpc;
  typedef typename Target_powerpc<size, big_endian>::Relocate Powerpc_relocate;

  gold_assert(sh_type == elfcpp::SHT_RELA);

  gold::relocate_section<size, big_endian, Powerpc, elfcpp::SHT_RELA,
    Powerpc_relocate>(
    relinfo,
    this,
    prelocs,
    reloc_count,
    output_section,
    needs_special_offset_handling,
    view,
    address,
    view_size,
    reloc_symbol_changes);
}

// Return the size of a relocation while scanning during a relocatable
// link.

template<int size, bool big_endian>
unsigned int
Target_powerpc<size, big_endian>::Relocatable_size_for_reloc::get_size_for_reloc(
    unsigned int,
    Relobj*)
{
  // We are always SHT_RELA, so we should never get here.
  gold_unreachable();
  return 0;
}

// Scan the relocs during a relocatable link.

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::scan_relocatable_relocs(
			Symbol_table* symtab,
			Layout* layout,
			Sized_relobj<size, big_endian>* object,
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

  gold::scan_relocatable_relocs<size, big_endian, elfcpp::SHT_RELA,
      Scan_relocatable_relocs>(
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

template<int size, bool big_endian>
void
Target_powerpc<size, big_endian>::relocate_for_relocatable(
    const Relocate_info<size, big_endian>* relinfo,
    unsigned int sh_type,
    const unsigned char* prelocs,
    size_t reloc_count,
    Output_section* output_section,
    off_t offset_in_output_section,
    const Relocatable_relocs* rr,
    unsigned char* view,
    typename elfcpp::Elf_types<size>::Elf_Addr view_address,
    section_size_type view_size,
    unsigned char* reloc_view,
    section_size_type reloc_view_size)
{
  gold_assert(sh_type == elfcpp::SHT_RELA);

  gold::relocate_for_relocatable<size, big_endian, elfcpp::SHT_RELA>(
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

template<int size, bool big_endian>
uint64_t
Target_powerpc<size, big_endian>::do_dynsym_value(const Symbol* gsym) const
{
  gold_assert(gsym->is_from_dynobj() && gsym->has_plt_offset());
  return this->plt_section()->address() + gsym->plt_offset();
}

// The selector for powerpc object files.

template<int size, bool big_endian>
class Target_selector_powerpc : public Target_selector
{
public:
  Target_selector_powerpc()
    : Target_selector(elfcpp::EM_NONE, size, big_endian,
		      (size == 64 ?
		       (big_endian ? "elf64-powerpc" : "elf64-powerpcle") :
		       (big_endian ? "elf32-powerpc" : "elf32-powerpcle")))
  { }

  Target* do_recognize(int machine, int, int)
  {
    switch (size)
      {
      case 64:
	if (machine != elfcpp::EM_PPC64)
	  return NULL;
	break;

      case 32:
	if (machine != elfcpp::EM_PPC)
	  return NULL;
	break;

      default:
	return NULL;
      }

    return this->instantiate_target();
  }

  Target* do_instantiate_target()
  { return new Target_powerpc<size, big_endian>(); }
};

Target_selector_powerpc<32, true> target_selector_ppc32;
Target_selector_powerpc<32, false> target_selector_ppc32le;
Target_selector_powerpc<64, true> target_selector_ppc64;
Target_selector_powerpc<64, false> target_selector_ppc64le;

} // End anonymous namespace.
