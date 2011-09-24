// reloc.h -- relocate input files for gold   -*- C++ -*-

// Copyright 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
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

#ifndef GOLD_RELOC_H
#define GOLD_RELOC_H

#include <vector>
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

#include "elfcpp.h"
#include "workqueue.h"

namespace gold
{

class General_options;
class Object;
class Relobj;
class Read_relocs_data;
class Symbol;
class Layout;
class Output_data;
class Output_section;

template<int size>
class Sized_symbol;

template<int size, bool big_endian>
class Sized_relobj;

template<int size>
class Symbol_value;

template<int sh_type, bool dynamic, int size, bool big_endian>
class Output_data_reloc;

// A class to read the relocations for an object file, and then queue
// up a task to see if they require any GOT/PLT/COPY relocations in
// the symbol table.

class Read_relocs : public Task
{
 public:
  //   THIS_BLOCKER and NEXT_BLOCKER are passed along to a Scan_relocs
  // or Gc_process_relocs task, so that they run in a deterministic
  // order.
  Read_relocs(Symbol_table* symtab, Layout* layout, Relobj* object,
	      Task_token* this_blocker, Task_token* next_blocker)
    : symtab_(symtab), layout_(layout), object_(object),
      this_blocker_(this_blocker), next_blocker_(next_blocker)
  { }

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const;

 private:
  Symbol_table* symtab_;
  Layout* layout_;
  Relobj* object_;
  Task_token* this_blocker_;
  Task_token* next_blocker_;
};

// Process the relocs to figure out which sections are garbage.
// Very similar to scan relocs.

class Gc_process_relocs : public Task
{
 public:
  // THIS_BLOCKER prevents this task from running until the previous
  // one is finished.  NEXT_BLOCKER prevents the next task from
  // running.
  Gc_process_relocs(Symbol_table* symtab, Layout* layout, Relobj* object,
		    Read_relocs_data* rd, Task_token* this_blocker,
		    Task_token* next_blocker)
    : symtab_(symtab), layout_(layout), object_(object), rd_(rd),
      this_blocker_(this_blocker), next_blocker_(next_blocker)
  { }

  ~Gc_process_relocs();

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const;

 private:
  Symbol_table* symtab_;
  Layout* layout_;
  Relobj* object_;
  Read_relocs_data* rd_;
  Task_token* this_blocker_;
  Task_token* next_blocker_;
};

// Scan the relocations for an object to see if they require any
// GOT/PLT/COPY relocations.

class Scan_relocs : public Task
{
 public:
  // THIS_BLOCKER prevents this task from running until the previous
  // one is finished.  NEXT_BLOCKER prevents the next task from
  // running.
  Scan_relocs(Symbol_table* symtab, Layout* layout, Relobj* object,
	      Read_relocs_data* rd, Task_token* this_blocker,
	      Task_token* next_blocker)
    : symtab_(symtab), layout_(layout), object_(object), rd_(rd),
      this_blocker_(this_blocker), next_blocker_(next_blocker)
  { }

  ~Scan_relocs();

  // The standard Task methods.

  Task_token*
  is_runnable();

  void
  locks(Task_locker*);

  void
  run(Workqueue*);

  std::string
  get_name() const;

 private:
  Symbol_table* symtab_;
  Layout* layout_;
  Relobj* object_;
  Read_relocs_data* rd_;
  Task_token* this_blocker_;
  Task_token* next_blocker_;
};

// A class to perform all the relocations for an object file.

class Relocate_task : public Task
{
 public:
  Relocate_task(const Symbol_table* symtab, const Layout* layout,
		Relobj* object, Output_file* of,
		Task_token* input_sections_blocker,
		Task_token* output_sections_blocker, Task_token* final_blocker)
    : symtab_(symtab), layout_(layout), object_(object), of_(of),
      input_sections_blocker_(input_sections_blocker),
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
  get_name() const;

 private:
  const Symbol_table* symtab_;
  const Layout* layout_;
  Relobj* object_;
  Output_file* of_;
  Task_token* input_sections_blocker_;
  Task_token* output_sections_blocker_;
  Task_token* final_blocker_;
};

// During a relocatable link, this class records how relocations
// should be handled for a single input reloc section.  An instance of
// this class is created while scanning relocs, and it is used while
// processing relocs.

class Relocatable_relocs
{
 public:
  // We use a vector of unsigned char to indicate how the input relocs
  // should be handled.  Each element is one of the following values.
  // We create this vector when we initially scan the relocations.
  enum Reloc_strategy
  {
    // Copy the input reloc.  Don't modify it other than updating the
    // r_offset field and the r_sym part of the r_info field.
    RELOC_COPY,
    // Copy the input reloc which is against an STT_SECTION symbol.
    // Update the r_offset and r_sym part of the r_info field.  Adjust
    // the addend by subtracting the value of the old local symbol and
    // adding the value of the new local symbol.  The addend is in the
    // SHT_RELA reloc and the contents of the data section do not need
    // to be changed.
    RELOC_ADJUST_FOR_SECTION_RELA,
    // Like RELOC_ADJUST_FOR_SECTION_RELA but the addend should not be
    // adjusted.
    RELOC_ADJUST_FOR_SECTION_0,
    // Like RELOC_ADJUST_FOR_SECTION_RELA but the contents of the
    // section need to be changed.  The number indicates the number of
    // bytes in the addend in the section contents.
    RELOC_ADJUST_FOR_SECTION_1,
    RELOC_ADJUST_FOR_SECTION_2,
    RELOC_ADJUST_FOR_SECTION_4,
    RELOC_ADJUST_FOR_SECTION_8,
    // Discard the input reloc--process it completely when relocating
    // the data section contents.
    RELOC_DISCARD,
    // An input reloc which is not discarded, but which requires
    // target specific processing in order to update it.
    RELOC_SPECIAL
  };

  Relocatable_relocs()
    : reloc_strategies_(), output_reloc_count_(0), posd_(NULL)
  { }

  // Record the number of relocs.
  void
  set_reloc_count(size_t reloc_count)
  { this->reloc_strategies_.reserve(reloc_count); }

  // Record what to do for the next reloc.
  void
  set_next_reloc_strategy(Reloc_strategy strategy)
  {
    this->reloc_strategies_.push_back(static_cast<unsigned char>(strategy));
    if (strategy != RELOC_DISCARD)
      ++this->output_reloc_count_;
  }

  // Record the Output_data associated with this reloc section.
  void
  set_output_data(Output_data* posd)
  {
    gold_assert(this->posd_ == NULL);
    this->posd_ = posd;
  }

  // Return the Output_data associated with this reloc section.
  Output_data*
  output_data() const
  { return this->posd_; }

  // Return what to do for reloc I.
  Reloc_strategy
  strategy(unsigned int i) const
  {
    gold_assert(i < this->reloc_strategies_.size());
    return static_cast<Reloc_strategy>(this->reloc_strategies_[i]);
  }

  // Return the number of relocations to create in the output file.
  size_t
  output_reloc_count() const
  { return this->output_reloc_count_; }

 private:
  typedef std::vector<unsigned char> Reloc_strategies;

  // The strategies for the input reloc.  There is one entry in this
  // vector for each relocation in the input section.
  Reloc_strategies reloc_strategies_;
  // The number of relocations to be created in the output file.
  size_t output_reloc_count_;
  // The output data structure associated with this relocation.
  Output_data* posd_;
};

// Standard relocation routines which are used on many targets.  Here
// SIZE and BIG_ENDIAN refer to the target, not the relocation type.

template<int size, bool big_endian>
class Relocate_functions
{
private:
  // Do a simple relocation with the addend in the section contents.
  // VALSIZE is the size of the value.
  template<int valsize>
  static inline void
  rel(unsigned char* view,
      typename elfcpp::Swap<valsize, big_endian>::Valtype value)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype x = elfcpp::Swap<valsize, big_endian>::readval(wv);
    elfcpp::Swap<valsize, big_endian>::writeval(wv, x + value);
  }

  // Do a simple relocation using a Symbol_value with the addend in
  // the section contents.  VALSIZE is the size of the value to
  // relocate.
  template<int valsize>
  static inline void
  rel(unsigned char* view,
      const Sized_relobj<size, big_endian>* object,
      const Symbol_value<size>* psymval)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype x = elfcpp::Swap<valsize, big_endian>::readval(wv);
    x = psymval->value(object, x);
    elfcpp::Swap<valsize, big_endian>::writeval(wv, x);
  }

  // Do a simple relocation with the addend in the relocation.
  // VALSIZE is the size of the value.
  template<int valsize>
  static inline void
  rela(unsigned char* view,
       typename elfcpp::Swap<valsize, big_endian>::Valtype value,
       typename elfcpp::Swap<valsize, big_endian>::Valtype addend)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    elfcpp::Swap<valsize, big_endian>::writeval(wv, value + addend);
  }

  // Do a simple relocation using a symbol value with the addend in
  // the relocation.  VALSIZE is the size of the value.
  template<int valsize>
  static inline void
  rela(unsigned char* view,
       const Sized_relobj<size, big_endian>* object,
       const Symbol_value<size>* psymval,
       typename elfcpp::Swap<valsize, big_endian>::Valtype addend)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype x = psymval->value(object, addend);
    elfcpp::Swap<valsize, big_endian>::writeval(wv, x);
  }

  // Do a simple PC relative relocation with the addend in the section
  // contents.  VALSIZE is the size of the value.
  template<int valsize>
  static inline void
  pcrel(unsigned char* view,
	typename elfcpp::Swap<valsize, big_endian>::Valtype value,
	typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype x = elfcpp::Swap<valsize, big_endian>::readval(wv);
    elfcpp::Swap<valsize, big_endian>::writeval(wv, x + value - address);
  }

  // Do a simple PC relative relocation with a Symbol_value with the
  // addend in the section contents.  VALSIZE is the size of the
  // value.
  template<int valsize>
  static inline void
  pcrel(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval,
	typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype x = elfcpp::Swap<valsize, big_endian>::readval(wv);
    x = psymval->value(object, x);
    elfcpp::Swap<valsize, big_endian>::writeval(wv, x - address);
  }

  // Do a simple PC relative relocation with the addend in the
  // relocation.  VALSIZE is the size of the value.
  template<int valsize>
  static inline void
  pcrela(unsigned char* view,
	 typename elfcpp::Swap<valsize, big_endian>::Valtype value,
	 typename elfcpp::Swap<valsize, big_endian>::Valtype addend,
	 typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    elfcpp::Swap<valsize, big_endian>::writeval(wv, value + addend - address);
  }

  // Do a simple PC relative relocation with a Symbol_value with the
  // addend in the relocation.  VALSIZE is the size of the value.
  template<int valsize>
  static inline void
  pcrela(unsigned char* view,
	 const Sized_relobj<size, big_endian>* object,
	 const Symbol_value<size>* psymval,
	 typename elfcpp::Swap<valsize, big_endian>::Valtype addend,
	 typename elfcpp::Elf_types<size>::Elf_Addr address)
  {
    typedef typename elfcpp::Swap<valsize, big_endian>::Valtype Valtype;
    Valtype* wv = reinterpret_cast<Valtype*>(view);
    Valtype x = psymval->value(object, addend);
    elfcpp::Swap<valsize, big_endian>::writeval(wv, x - address);
  }

  typedef Relocate_functions<size, big_endian> This;

public:
  // Do a simple 8-bit REL relocation with the addend in the section
  // contents.
  static inline void
  rel8(unsigned char* view, unsigned char value)
  { This::template rel<8>(view, value); }

  static inline void
  rel8(unsigned char* view,
       const Sized_relobj<size, big_endian>* object,
       const Symbol_value<size>* psymval)
  { This::template rel<8>(view, object, psymval); }

  // Do an 8-bit RELA relocation with the addend in the relocation.
  static inline void
  rela8(unsigned char* view, unsigned char value, unsigned char addend)
  { This::template rela<8>(view, value, addend); }

  static inline void
  rela8(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval,
	unsigned char addend)
  { This::template rela<8>(view, object, psymval, addend); }

  // Do a simple 8-bit PC relative relocation with the addend in the
  // section contents.
  static inline void
  pcrel8(unsigned char* view, unsigned char value,
	 typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrel<8>(view, value, address); }

  static inline void
  pcrel8(unsigned char* view,
	 const Sized_relobj<size, big_endian>* object,
	 const Symbol_value<size>* psymval,
	 typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrel<8>(view, object, psymval, address); }

  // Do a simple 8-bit PC relative RELA relocation with the addend in
  // the reloc.
  static inline void
  pcrela8(unsigned char* view, unsigned char value, unsigned char addend,
	  typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrela<8>(view, value, addend, address); }

  static inline void
  pcrela8(unsigned char* view,
	  const Sized_relobj<size, big_endian>* object,
	  const Symbol_value<size>* psymval,
	  unsigned char addend,
	  typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrela<8>(view, object, psymval, addend, address); }

  // Do a simple 16-bit REL relocation with the addend in the section
  // contents.
  static inline void
  rel16(unsigned char* view, elfcpp::Elf_Half value)
  { This::template rel<16>(view, value); }

  static inline void
  rel16(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval)
  { This::template rel<16>(view, object, psymval); }

  // Do an 16-bit RELA relocation with the addend in the relocation.
  static inline void
  rela16(unsigned char* view, elfcpp::Elf_Half value, elfcpp::Elf_Half addend)
  { This::template rela<16>(view, value, addend); }

  static inline void
  rela16(unsigned char* view,
	 const Sized_relobj<size, big_endian>* object,
	 const Symbol_value<size>* psymval,
	 elfcpp::Elf_Half addend)
  { This::template rela<16>(view, object, psymval, addend); }

  // Do a simple 16-bit PC relative REL relocation with the addend in
  // the section contents.
  static inline void
  pcrel16(unsigned char* view, elfcpp::Elf_Half value,
	  typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrel<16>(view, value, address); }

  static inline void
  pcrel16(unsigned char* view,
	  const Sized_relobj<size, big_endian>* object,
	  const Symbol_value<size>* psymval,
	  typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrel<16>(view, object, psymval, address); }

  // Do a simple 16-bit PC relative RELA relocation with the addend in
  // the reloc.
  static inline void
  pcrela16(unsigned char* view, elfcpp::Elf_Half value,
	   elfcpp::Elf_Half addend,
           typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrela<16>(view, value, addend, address); }

  static inline void
  pcrela16(unsigned char* view,
	   const Sized_relobj<size, big_endian>* object,
	   const Symbol_value<size>* psymval,
	   elfcpp::Elf_Half addend,
	   typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrela<16>(view, object, psymval, addend, address); }

  // Do a simple 32-bit REL relocation with the addend in the section
  // contents.
  static inline void
  rel32(unsigned char* view, elfcpp::Elf_Word value)
  { This::template rel<32>(view, value); }

  static inline void
  rel32(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval)
  { This::template rel<32>(view, object, psymval); }

  // Do an 32-bit RELA relocation with the addend in the relocation.
  static inline void
  rela32(unsigned char* view, elfcpp::Elf_Word value, elfcpp::Elf_Word addend)
  { This::template rela<32>(view, value, addend); }

  static inline void
  rela32(unsigned char* view,
	 const Sized_relobj<size, big_endian>* object,
	 const Symbol_value<size>* psymval,
	 elfcpp::Elf_Word addend)
  { This::template rela<32>(view, object, psymval, addend); }

  // Do a simple 32-bit PC relative REL relocation with the addend in
  // the section contents.
  static inline void
  pcrel32(unsigned char* view, elfcpp::Elf_Word value,
	  typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrel<32>(view, value, address); }

  static inline void
  pcrel32(unsigned char* view,
	  const Sized_relobj<size, big_endian>* object,
	  const Symbol_value<size>* psymval,
	  typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrel<32>(view, object, psymval, address); }

  // Do a simple 32-bit PC relative RELA relocation with the addend in
  // the relocation.
  static inline void
  pcrela32(unsigned char* view, elfcpp::Elf_Word value,
           elfcpp::Elf_Word addend,
           typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrela<32>(view, value, addend, address); }

  static inline void
  pcrela32(unsigned char* view,
	   const Sized_relobj<size, big_endian>* object,
	   const Symbol_value<size>* psymval,
	   elfcpp::Elf_Word addend,
	   typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrela<32>(view, object, psymval, addend, address); }

  // Do a simple 64-bit REL relocation with the addend in the section
  // contents.
  static inline void
  rel64(unsigned char* view, elfcpp::Elf_Xword value)
  { This::template rel<64>(view, value); }

  static inline void
  rel64(unsigned char* view,
	const Sized_relobj<size, big_endian>* object,
	const Symbol_value<size>* psymval)
  { This::template rel<64>(view, object, psymval); }

  // Do a 64-bit RELA relocation with the addend in the relocation.
  static inline void
  rela64(unsigned char* view, elfcpp::Elf_Xword value,
         elfcpp::Elf_Xword addend)
  { This::template rela<64>(view, value, addend); }

  static inline void
  rela64(unsigned char* view,
	 const Sized_relobj<size, big_endian>* object,
	 const Symbol_value<size>* psymval,
	 elfcpp::Elf_Xword addend)
  { This::template rela<64>(view, object, psymval, addend); }

  // Do a simple 64-bit PC relative REL relocation with the addend in
  // the section contents.
  static inline void
  pcrel64(unsigned char* view, elfcpp::Elf_Xword value,
	  typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrel<64>(view, value, address); }

  static inline void
  pcrel64(unsigned char* view,
	  const Sized_relobj<size, big_endian>* object,
	  const Symbol_value<size>* psymval,
	  typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrel<64>(view, object, psymval, address); }

  // Do a simple 64-bit PC relative RELA relocation with the addend in
  // the relocation.
  static inline void
  pcrela64(unsigned char* view, elfcpp::Elf_Xword value,
           elfcpp::Elf_Xword addend,
           typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrela<64>(view, value, addend, address); }

  static inline void
  pcrela64(unsigned char* view,
	   const Sized_relobj<size, big_endian>* object,
	   const Symbol_value<size>* psymval,
	   elfcpp::Elf_Xword addend,
	   typename elfcpp::Elf_types<size>::Elf_Addr address)
  { This::template pcrela<64>(view, object, psymval, addend, address); }
};

// Track relocations while reading a section.  This lets you ask for
// the relocation at a certain offset, and see how relocs occur
// between points of interest.

template<int size, bool big_endian>
class Track_relocs
{
 public:
  Track_relocs()
    : prelocs_(NULL), len_(0), pos_(0), reloc_size_(0)
  { }

  // Initialize the Track_relocs object.  OBJECT is the object holding
  // the reloc section, RELOC_SHNDX is the section index of the reloc
  // section, and RELOC_TYPE is the type of the reloc section
  // (elfcpp::SHT_REL or elfcpp::SHT_RELA).  This returns false if
  // something went wrong.
  bool
  initialize(Object* object, unsigned int reloc_shndx,
	     unsigned int reloc_type);

  // Return the offset in the data section to which the next reloc
  // applies.  THis returns -1 if there is no next reloc.
  off_t
  next_offset() const;

  // Return the symbol index of the next reloc.  This returns -1U if
  // there is no next reloc.
  unsigned int
  next_symndx() const;

  // Advance to OFFSET within the data section, and return the number
  // of relocs which would be skipped.
  int
  advance(off_t offset);

 private:
  // The contents of the input object's reloc section.
  const unsigned char* prelocs_;
  // The length of the reloc section.
  section_size_type len_;
  // Our current position in the reloc section.
  section_size_type pos_;
  // The size of the relocs in the section.
  int reloc_size_;
};

} // End namespace gold.

#endif // !defined(GOLD_RELOC_H)
