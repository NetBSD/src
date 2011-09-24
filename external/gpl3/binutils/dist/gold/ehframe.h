// ehframe.h -- handle exception frame sections for gold  -*- C++ -*-

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

#ifndef GOLD_EHFRAME_H
#define GOLD_EHFRAME_H

#include <map>
#include <set>
#include <vector>

#include "output.h"
#include "merge.h"

namespace gold
{

template<int size, bool big_endian>
class Track_relocs;

class Eh_frame;

// This class manages the .eh_frame_hdr section, which holds the data
// for the PT_GNU_EH_FRAME segment.  gcc's unwind support code uses
// the PT_GNU_EH_FRAME segment to find the list of FDEs.  This saves
// the time required to register the exception handlers at startup
// time and when a shared object is loaded, and the time required to
// deregister the exception handlers when a shared object is unloaded.

// FIXME: gcc supports using storing a sorted lookup table for the
// FDEs in the PT_GNU_EH_FRAME segment, but we do not yet generate
// that.

class Eh_frame_hdr : public Output_section_data
{
 public:
  Eh_frame_hdr(Output_section* eh_frame_section, const Eh_frame*);

  // Record that we found an unrecognized .eh_frame section.
  void
  found_unrecognized_eh_frame_section()
  { this->any_unrecognized_eh_frame_sections_ = true; }

  // Record an FDE.
  void
  record_fde(section_offset_type fde_offset, unsigned char fde_encoding)
  {
    if (!this->any_unrecognized_eh_frame_sections_)
      this->fde_offsets_.push_back(std::make_pair(fde_offset, fde_encoding));
  }

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
  { mapfile->print_output_data(this, _("** eh_frame_hdr")); }

 private:
  // Write the data to the file with the right endianness.
  template<int size, bool big_endian>
  void
  do_sized_write(Output_file*);

  // The data we record for one FDE: the offset of the FDE within the
  // .eh_frame section, and the FDE encoding.
  typedef std::pair<section_offset_type, unsigned char> Fde_offset;

  // The list of information we record for an FDE.
  typedef std::vector<Fde_offset> Fde_offsets;

  // When writing out the header, we convert the FDE offsets into FDE
  // addresses.  This is a list of pairs of the offset from the header
  // to the FDE PC and to the FDE itself.
  template<int size>
  class Fde_addresses
  {
   public:
    typedef typename elfcpp::Elf_types<size>::Elf_Addr Address;
    typedef typename std::pair<Address, Address> Fde_address;
    typedef typename std::vector<Fde_address> Fde_address_list;
    typedef typename Fde_address_list::iterator iterator;

    Fde_addresses(unsigned int reserve)
      : fde_addresses_()
    { this->fde_addresses_.reserve(reserve); }

    void
    push_back(Address pc_address, Address fde_address)
    {
      this->fde_addresses_.push_back(std::make_pair(pc_address, fde_address));
    }

    iterator
    begin()
    { return this->fde_addresses_.begin(); }

    iterator
    end()
    { return this->fde_addresses_.end(); }

   private:
    Fde_address_list fde_addresses_;
  };

  // Compare Fde_address objects.
  template<int size>
  struct Fde_address_compare
  {
    bool
    operator()(const typename Fde_addresses<size>::Fde_address& f1,
	       const typename Fde_addresses<size>::Fde_address& f2) const
    { return f1.first < f2.first; }
  };

  // Return the PC to which an FDE refers.
  template<int size, bool big_endian>
  typename elfcpp::Elf_types<size>::Elf_Addr
  get_fde_pc(typename elfcpp::Elf_types<size>::Elf_Addr eh_frame_address,
	     const unsigned char* eh_frame_contents,
	     section_offset_type fde_offset, unsigned char fde_encoding);

  // Convert Fde_offsets to Fde_addresses.
  template<int size, bool big_endian>
  void
  get_fde_addresses(Output_file* of,
		    const Fde_offsets* fde_offsets,
		    Fde_addresses<size>* fde_addresses);

  // The .eh_frame section.
  Output_section* eh_frame_section_;
  // The .eh_frame section data.
  const Eh_frame* eh_frame_data_;
  // Data from the FDEs in the .eh_frame sections.
  Fde_offsets fde_offsets_;
  // Whether we found any .eh_frame sections which we could not
  // process.
  bool any_unrecognized_eh_frame_sections_;
};

// This class holds an FDE.

class Fde
{
 public:
  Fde(Relobj* object, unsigned int shndx, section_offset_type input_offset,
      const unsigned char* contents, size_t length)
    : object_(object), shndx_(shndx), input_offset_(input_offset),
      contents_(reinterpret_cast<const char*>(contents), length)
  { }

  // Return the length of this FDE.  Add 4 for the length and 4 for
  // the offset to the CIE.
  size_t
  length() const
  { return this->contents_.length() + 8; }

  // Add a mapping for this FDE to MERGE_MAP.
  void
  add_mapping(section_offset_type output_offset, Merge_map* merge_map) const
  {
    merge_map->add_mapping(this->object_, this->shndx_,
			   this->input_offset_, this->length(),
			   output_offset);
  }

  // Write the FDE to OVIEW starting at OFFSET.  FDE_ENCODING is the
  // encoding, from the CIE.  Round up the bytes to ADDRALIGN if
  // necessary.  Record the FDE in EH_FRAME_HDR.  Return the new
  // offset.
  template<int size, bool big_endian>
  section_offset_type
  write(unsigned char* oview, section_offset_type offset,
	unsigned int addralign, section_offset_type cie_offset,
        unsigned char fde_encoding, Eh_frame_hdr* eh_frame_hdr);

 private:
  // The object in which this FDE was seen.
  Relobj* object_;
  // Input section index for this FDE.
  unsigned int shndx_;
  // Offset within the input section for this FDE.
  section_offset_type input_offset_;
  // FDE data.
  std::string contents_;
};

// This class holds a CIE.

class Cie
{
 public:
  Cie(Relobj* object, unsigned int shndx, section_offset_type input_offset,
      unsigned char fde_encoding, const char* personality_name,
      const unsigned char* contents, size_t length)
    : object_(object),
      shndx_(shndx),
      input_offset_(input_offset),
      fde_encoding_(fde_encoding),
      personality_name_(personality_name),
      fdes_(),
      contents_(reinterpret_cast<const char*>(contents), length)
  { }

  ~Cie();

  // We permit copying a CIE when there are no FDEs.  This is
  // convenient in the code which creates them.
  Cie(const Cie& cie)
    : object_(cie.object_),
      shndx_(cie.shndx_),
      input_offset_(cie.input_offset_),
      fde_encoding_(cie.fde_encoding_),
      personality_name_(cie.personality_name_),
      fdes_(),
      contents_(cie.contents_)
  { gold_assert(cie.fdes_.empty()); }

  // Add an FDE associated with this CIE.
  void
  add_fde(Fde* fde)
  { this->fdes_.push_back(fde); }

  // Return the number of FDEs.
  unsigned int
  fde_count() const
  { return this->fdes_.size(); }

  // Set the output offset of this CIE to OUTPUT_OFFSET.  It will be
  // followed by all its FDEs.  ADDRALIGN is the required address
  // alignment, typically 4 or 8.  This updates MERGE_MAP with the
  // mapping.  It returns the new output offset.
  section_offset_type
  set_output_offset(section_offset_type output_offset, unsigned int addralign,
		    Merge_map*);

  // Write the CIE to OVIEW starting at OFFSET.  EH_FRAME_HDR is the
  // exception frame header for FDE recording.  Round up the bytes to
  // ADDRALIGN.  Return the new offset.
  template<int size, bool big_endian>
  section_offset_type
  write(unsigned char* oview, section_offset_type offset,
	unsigned int addralign, Eh_frame_hdr* eh_frame_hdr);

  friend bool operator<(const Cie&, const Cie&);
  friend bool operator==(const Cie&, const Cie&);

 private:
  // The class is not assignable.
  Cie& operator=(const Cie&);

  // The object in which this CIE was first seen.
  Relobj* object_;
  // Input section index for this CIE.
  unsigned int shndx_;
  // Offset within the input section for this CIE.
  section_offset_type input_offset_;
  // The encoding of the FDE.  This is a DW_EH_PE code.
  unsigned char fde_encoding_;
  // The name of the personality routine.  This will be the name of a
  // global symbol, or will be the empty string.
  std::string personality_name_;
  // List of FDEs.
  std::vector<Fde*> fdes_;
  // CIE data.
  std::string contents_;
};

extern bool operator<(const Cie&, const Cie&);
extern bool operator==(const Cie&, const Cie&);

// This class manages .eh_frame sections.  It discards duplicate
// exception information.

class Eh_frame : public Output_section_data
{
 public:
  Eh_frame();

  // Record the associated Eh_frame_hdr, if any.
  void
  set_eh_frame_hdr(Eh_frame_hdr* hdr)
  { this->eh_frame_hdr_ = hdr; }

  // Add the input section SHNDX in OBJECT.  SYMBOLS is the contents
  // of the symbol table section (size SYMBOLS_SIZE), SYMBOL_NAMES is
  // the symbol names section (size SYMBOL_NAMES_SIZE).  RELOC_SHNDX
  // is the relocation section if any (0 for none, -1U for multiple).
  // RELOC_TYPE is the type of the relocation section if any.  This
  // returns whether the section was incorporated into the .eh_frame
  // data.
  template<int size, bool big_endian>
  bool
  add_ehframe_input_section(Sized_relobj<size, big_endian>* object,
			    const unsigned char* symbols,
			    section_size_type symbols_size,
			    const unsigned char* symbol_names,
			    section_size_type symbol_names_size,
			    unsigned int shndx, unsigned int reloc_shndx,
			    unsigned int reloc_type);

  // Return the number of FDEs.
  unsigned int
  fde_count() const;

 protected:
  // Set the final data size.
  void
  set_final_data_size();

  // Return the output address for an input address.
  bool
  do_output_offset(const Relobj*, unsigned int shndx,
		   section_offset_type offset,
		   section_offset_type* poutput) const;

  // Return whether this is the merge section for an input section.
  bool
  do_is_merge_section_for(const Relobj*, unsigned int shndx) const;

  // Write the data to the file.
  void
  do_write(Output_file*);

  // Write to a map file.
  void
  do_print_to_mapfile(Mapfile* mapfile) const
  { mapfile->print_output_data(this, _("** eh_frame")); }

 private:
  // The comparison routine for the CIE map.
  struct Cie_less
  {
    bool
    operator()(const Cie* cie1, const Cie* cie2) const
    { return *cie1 < *cie2; }
  };

  // A set of unique CIEs.
  typedef std::set<Cie*, Cie_less> Cie_offsets;

  // A list of unmergeable CIEs.
  typedef std::vector<Cie*> Unmergeable_cie_offsets;

  // A mapping from offsets to CIEs.  This is used while reading an
  // input section.
  typedef std::map<uint64_t, Cie*> Offsets_to_cie;

  // A list of CIEs, and a bool indicating whether the CIE is
  // mergeable.
  typedef std::vector<std::pair<Cie*, bool> > New_cies;

  // Skip an LEB128.
  static bool
  skip_leb128(const unsigned char**, const unsigned char*);

  // The implementation of add_ehframe_input_section.
  template<int size, bool big_endian>
  bool
  do_add_ehframe_input_section(Sized_relobj<size, big_endian>* object,
			       const unsigned char* symbols,
			       section_size_type symbols_size,
			       const unsigned char* symbol_names,
			       section_size_type symbol_names_size,
			       unsigned int shndx,
			       unsigned int reloc_shndx,
			       unsigned int reloc_type,
			       const unsigned char* pcontents,
			       section_size_type contents_len,
			       New_cies*);

  // Read a CIE.
  template<int size, bool big_endian>
  bool
  read_cie(Sized_relobj<size, big_endian>* object,
	   unsigned int shndx,
	   const unsigned char* symbols,
	   section_size_type symbols_size,
	   const unsigned char* symbol_names,
	   section_size_type symbol_names_size,
	   const unsigned char* pcontents,
	   const unsigned char* pcie,
	   const unsigned char* pcieend,
	   Track_relocs<size, big_endian>* relocs,
	   Offsets_to_cie* cies,
	   New_cies* new_cies);

  // Read an FDE.
  template<int size, bool big_endian>
  bool
  read_fde(Sized_relobj<size, big_endian>* object,
	   unsigned int shndx,
	   const unsigned char* symbols,
	   section_size_type symbols_size,
	   const unsigned char* pcontents,
	   unsigned int offset,
	   const unsigned char* pfde,
	   const unsigned char* pfdeend,
	   Track_relocs<size, big_endian>* relocs,
	   Offsets_to_cie* cies);

  // Template version of write function.
  template<int size, bool big_endian>
  void
  do_sized_write(unsigned char* oview);

  // The exception frame header, if any.
  Eh_frame_hdr* eh_frame_hdr_;
  // A mapping from all unique CIEs to their offset in the output
  // file.
  Cie_offsets cie_offsets_;
  // A mapping from unmergeable CIEs to their offset in the output
  // file.
  Unmergeable_cie_offsets unmergeable_cie_offsets_;
  // A mapping from input sections to the output section.
  Merge_map merge_map_;
  // Whether we have created the mappings to the output section.
  bool mappings_are_done_;
  // The final data size.  This is only set if mappings_are_done_ is
  // true.
  section_size_type final_data_size_;
};

} // End namespace gold.

#endif // !defined(GOLD_EHFRAME_H)
