// ehframe.cc -- handle exception frame sections for gold

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
#include <algorithm>

#include "elfcpp.h"
#include "dwarf.h"
#include "symtab.h"
#include "reloc.h"
#include "ehframe.h"

namespace gold
{

// This file handles generation of the exception frame header that
// gcc's runtime support libraries use to find unwind information at
// runtime.  This file also handles discarding duplicate exception
// frame information.

// The exception frame header starts with four bytes:

// 0: The version number, currently 1.

// 1: The encoding of the pointer to the exception frames.  This can
//    be any DWARF unwind encoding (DW_EH_PE_*).  It is normally a 4
//    byte PC relative offset (DW_EH_PE_pcrel | DW_EH_PE_sdata4).

// 2: The encoding of the count of the number of FDE pointers in the
//    lookup table.  This can be any DWARF unwind encoding, and in
//    particular can be DW_EH_PE_omit if the count is omitted.  It is
//    normally a 4 byte unsigned count (DW_EH_PE_udata4).

// 3: The encoding of the lookup table entries.  Currently gcc's
//    libraries will only support DW_EH_PE_datarel | DW_EH_PE_sdata4,
//    which means that the values are 4 byte offsets from the start of
//    the table.

// The exception frame header is followed by a pointer to the contents
// of the exception frame section (.eh_frame).  This pointer is
// encoded as specified in the byte at offset 1 of the header (i.e.,
// it is normally a 4 byte PC relative offset).

// If there is a lookup table, this is followed by the count of the
// number of FDE pointers, encoded as specified in the byte at offset
// 2 of the header (i.e., normally a 4 byte unsigned integer).

// This is followed by the table, which should start at an 4-byte
// aligned address in memory.  Each entry in the table is 8 bytes.
// Each entry represents an FDE.  The first four bytes of each entry
// are an offset to the starting PC for the FDE.  The last four bytes
// of each entry are an offset to the FDE data.  The offsets are from
// the start of the exception frame header information.  The entries
// are in sorted order by starting PC.

const int eh_frame_hdr_size = 4;

// Construct the exception frame header.

Eh_frame_hdr::Eh_frame_hdr(Output_section* eh_frame_section,
			   const Eh_frame* eh_frame_data)
  : Output_section_data(4),
    eh_frame_section_(eh_frame_section),
    eh_frame_data_(eh_frame_data),
    fde_offsets_(),
    any_unrecognized_eh_frame_sections_(false)
{
}

// Set the size of the exception frame header.

void
Eh_frame_hdr::set_final_data_size()
{
  unsigned int data_size = eh_frame_hdr_size + 4;
  if (!this->any_unrecognized_eh_frame_sections_)
    {
      unsigned int fde_count = this->eh_frame_data_->fde_count();
      if (fde_count != 0)
	data_size += 4 + 8 * fde_count;
      this->fde_offsets_.reserve(fde_count);
    }
  this->set_data_size(data_size);
}

// Write the data to the flie.

void
Eh_frame_hdr::do_write(Output_file* of)
{
  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->do_sized_write<32, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->do_sized_write<32, true>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->do_sized_write<64, false>(of);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->do_sized_write<64, true>(of);
      break;
#endif
    default:
      gold_unreachable();
    }
}

// Write the data to the file with the right endianness.

template<int size, bool big_endian>
void
Eh_frame_hdr::do_sized_write(Output_file* of)
{
  const off_t off = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(off, oview_size);

  // Version number.
  oview[0] = 1;

  // Write out a 4 byte PC relative offset to the address of the
  // .eh_frame section.
  oview[1] = elfcpp::DW_EH_PE_pcrel | elfcpp::DW_EH_PE_sdata4;
  uint64_t eh_frame_address = this->eh_frame_section_->address();
  uint64_t eh_frame_hdr_address = this->address();
  uint64_t eh_frame_offset = (eh_frame_address -
			      (eh_frame_hdr_address + 4));
  elfcpp::Swap<32, big_endian>::writeval(oview + 4, eh_frame_offset);

  if (this->any_unrecognized_eh_frame_sections_
      || this->fde_offsets_.empty())
    {
      // There are no FDEs, or we didn't recognize the format of the
      // some of the .eh_frame sections, so we can't write out the
      // sorted table.
      oview[2] = elfcpp::DW_EH_PE_omit;
      oview[3] = elfcpp::DW_EH_PE_omit;

      gold_assert(oview_size == 8);
    }
  else
    {
      oview[2] = elfcpp::DW_EH_PE_udata4;
      oview[3] = elfcpp::DW_EH_PE_datarel | elfcpp::DW_EH_PE_sdata4;

      elfcpp::Swap<32, big_endian>::writeval(oview + 8,
					     this->fde_offsets_.size());

      // We have the offsets of the FDEs in the .eh_frame section.  We
      // couldn't easily get the PC values before, as they depend on
      // relocations which are, of course, target specific.  This code
      // is run after all those relocations have been applied to the
      // output file.  Here we read the output file again to find the
      // PC values.  Then we sort the list and write it out.

      Fde_addresses<size> fde_addresses(this->fde_offsets_.size());
      this->get_fde_addresses<size, big_endian>(of, &this->fde_offsets_,
						&fde_addresses);

      std::sort(fde_addresses.begin(), fde_addresses.end(),
		Fde_address_compare<size>());

      typename elfcpp::Elf_types<size>::Elf_Addr output_address;
      output_address = this->address();

      unsigned char* pfde = oview + 12;
      for (typename Fde_addresses<size>::iterator p = fde_addresses.begin();
	   p != fde_addresses.end();
	   ++p)
	{
	  elfcpp::Swap<32, big_endian>::writeval(pfde,
						 p->first - output_address);
	  elfcpp::Swap<32, big_endian>::writeval(pfde + 4,
						 p->second - output_address);
	  pfde += 8;
	}

      gold_assert(pfde - oview == oview_size);
    }

  of->write_output_view(off, oview_size, oview);
}

// Given the offset FDE_OFFSET of an FDE in the .eh_frame section, and
// the contents of the .eh_frame section EH_FRAME_CONTENTS, where the
// FDE's encoding is FDE_ENCODING, return the output address of the
// FDE's PC.

template<int size, bool big_endian>
typename elfcpp::Elf_types<size>::Elf_Addr
Eh_frame_hdr::get_fde_pc(
    typename elfcpp::Elf_types<size>::Elf_Addr eh_frame_address,
    const unsigned char* eh_frame_contents,
    section_offset_type fde_offset,
    unsigned char fde_encoding)
{
  // The FDE starts with a 4 byte length and a 4 byte offset to the
  // CIE.  The PC follows.
  const unsigned char* p = eh_frame_contents + fde_offset + 8;

  typename elfcpp::Elf_types<size>::Elf_Addr pc;
  bool is_signed = (fde_encoding & elfcpp::DW_EH_PE_signed) != 0;
  int pc_size = fde_encoding & 7;
  if (pc_size == elfcpp::DW_EH_PE_absptr)
    {
      if (size == 32)
	pc_size = elfcpp::DW_EH_PE_udata4;
      else if (size == 64)
	pc_size = elfcpp::DW_EH_PE_udata8;
      else
	gold_unreachable();
    }

  switch (pc_size)
    {
    case elfcpp::DW_EH_PE_udata2:
      pc = elfcpp::Swap<16, big_endian>::readval(p);
      if (is_signed)
	pc = (pc ^ 0x8000) - 0x8000;
      break;

    case elfcpp::DW_EH_PE_udata4:
      pc = elfcpp::Swap<32, big_endian>::readval(p);
      if (size > 32 && is_signed)
	pc = (pc ^ 0x80000000) - 0x80000000;
      break;

    case elfcpp::DW_EH_PE_udata8:
      gold_assert(size == 64);
      pc = elfcpp::Swap_unaligned<64, big_endian>::readval(p);
      break;

    default:
      // All other cases were rejected in Eh_frame::read_cie.
      gold_unreachable();
    }

  switch (fde_encoding & 0xf0)
    {
    case 0:
      break;

    case elfcpp::DW_EH_PE_pcrel:
      pc += eh_frame_address + fde_offset + 8;
      break;

    default:
      // If other cases arise, then we have to handle them, or we have
      // to reject them by returning false in Eh_frame::read_cie.
      gold_unreachable();
    }

  return pc;
}

// Given an array of FDE offsets in the .eh_frame section, return an
// array of offsets from the exception frame header to the FDE's
// output PC and to the output address of the FDE itself.  We get the
// FDE's PC by actually looking in the .eh_frame section we just wrote
// to the output file.

template<int size, bool big_endian>
void
Eh_frame_hdr::get_fde_addresses(Output_file* of,
				const Fde_offsets* fde_offsets,
				Fde_addresses<size>* fde_addresses)
{
  typename elfcpp::Elf_types<size>::Elf_Addr eh_frame_address;
  eh_frame_address = this->eh_frame_section_->address();
  off_t eh_frame_offset = this->eh_frame_section_->offset();
  off_t eh_frame_size = this->eh_frame_section_->data_size();
  const unsigned char* eh_frame_contents = of->get_input_view(eh_frame_offset,
							      eh_frame_size);

  for (Fde_offsets::const_iterator p = fde_offsets->begin();
       p != fde_offsets->end();
       ++p)
    {
      typename elfcpp::Elf_types<size>::Elf_Addr fde_pc;
      fde_pc = this->get_fde_pc<size, big_endian>(eh_frame_address,
						  eh_frame_contents,
						  p->first, p->second);
      fde_addresses->push_back(fde_pc, eh_frame_address + p->first);
    }

  of->free_input_view(eh_frame_offset, eh_frame_size, eh_frame_contents);
}

// Class Fde.

// Write the FDE to OVIEW starting at OFFSET.  CIE_OFFSET is the
// offset of the CIE in OVIEW.  FDE_ENCODING is the encoding, from the
// CIE.  ADDRALIGN is the required alignment.  Record the FDE pc for
// EH_FRAME_HDR.  Return the new offset.

template<int size, bool big_endian>
section_offset_type
Fde::write(unsigned char* oview, section_offset_type offset,
	   unsigned int addralign, section_offset_type cie_offset,
           unsigned char fde_encoding, Eh_frame_hdr* eh_frame_hdr)
{
  gold_assert((offset & (addralign - 1)) == 0);

  size_t length = this->contents_.length();

  // We add 8 when getting the aligned length to account for the
  // length word and the CIE offset.
  size_t aligned_full_length = align_address(length + 8, addralign);

  // Write the length of the FDE as a 32-bit word.  The length word
  // does not include the four bytes of the length word itself, but it
  // does include the offset to the CIE.
  elfcpp::Swap<32, big_endian>::writeval(oview + offset,
                                         aligned_full_length - 4);

  // Write the offset to the CIE as a 32-bit word.  This is the
  // difference between the address of the offset word itself and the
  // CIE address.
  elfcpp::Swap<32, big_endian>::writeval(oview + offset + 4,
					 offset + 4 - cie_offset);

  // Copy the rest of the FDE.  Note that this is run before
  // relocation processing is done on this section, so the relocations
  // will later be applied to the FDE data.
  memcpy(oview + offset + 8, this->contents_.data(), length);

  if (aligned_full_length > length + 8)
    memset(oview + offset + length + 8, 0, aligned_full_length - (length + 8));

  // Tell the exception frame header about this FDE.
  if (eh_frame_hdr != NULL)
    eh_frame_hdr->record_fde(offset, fde_encoding);

  return offset + aligned_full_length;
}

// Class Cie.

// Destructor.

Cie::~Cie()
{
  for (std::vector<Fde*>::iterator p = this->fdes_.begin();
       p != this->fdes_.end();
       ++p)
    delete *p;
}

// Set the output offset of a CIE.  Return the new output offset.

section_offset_type
Cie::set_output_offset(section_offset_type output_offset,
		       unsigned int addralign,
		       Merge_map* merge_map)
{
  size_t length = this->contents_.length();

  // Add 4 for length and 4 for zero CIE identifier tag.
  length += 8;

  merge_map->add_mapping(this->object_, this->shndx_, this->input_offset_,
			 length, output_offset);

  length = align_address(length, addralign);

  for (std::vector<Fde*>::const_iterator p = this->fdes_.begin();
       p != this->fdes_.end();
       ++p)
    {
      (*p)->add_mapping(output_offset + length, merge_map);

      size_t fde_length = (*p)->length();
      fde_length = align_address(fde_length, addralign);
      length += fde_length;
    }

  return output_offset + length;
}

// Write the CIE to OVIEW starting at OFFSET.  EH_FRAME_HDR is for FDE
// recording.  Round up the bytes to ADDRALIGN.  Return the new
// offset.

template<int size, bool big_endian>
section_offset_type
Cie::write(unsigned char* oview, section_offset_type offset,
	   unsigned int addralign, Eh_frame_hdr* eh_frame_hdr)
{
  gold_assert((offset & (addralign - 1)) == 0);

  section_offset_type cie_offset = offset;

  size_t length = this->contents_.length();

  // We add 8 when getting the aligned length to account for the
  // length word and the CIE tag.
  size_t aligned_full_length = align_address(length + 8, addralign);

  // Write the length of the CIE as a 32-bit word.  The length word
  // does not include the four bytes of the length word itself.
  elfcpp::Swap<32, big_endian>::writeval(oview + offset,
                                         aligned_full_length - 4);

  // Write the tag which marks this as a CIE: a 32-bit zero.
  elfcpp::Swap<32, big_endian>::writeval(oview + offset + 4, 0);

  // Write out the CIE data.
  memcpy(oview + offset + 8, this->contents_.data(), length);

  if (aligned_full_length > length + 8)
    memset(oview + offset + length + 8, 0, aligned_full_length - (length + 8));

  offset += aligned_full_length;

  // Write out the associated FDEs.
  unsigned char fde_encoding = this->fde_encoding_;
  for (std::vector<Fde*>::const_iterator p = this->fdes_.begin();
       p != this->fdes_.end();
       ++p)
    offset = (*p)->write<size, big_endian>(oview, offset, addralign,
                                           cie_offset, fde_encoding,
                                           eh_frame_hdr);

  return offset;
}

// We track all the CIEs we see, and merge them when possible.  This
// works because each FDE holds an offset to the relevant CIE: we
// rewrite the FDEs to point to the merged CIE.  This is worthwhile
// because in a typical C++ program many FDEs in many different object
// files will use the same CIE.

// An equality operator for Cie.

bool
operator==(const Cie& cie1, const Cie& cie2)
{
  return (cie1.personality_name_ == cie2.personality_name_
	  && cie1.contents_ == cie2.contents_);
}

// A less-than operator for Cie.

bool
operator<(const Cie& cie1, const Cie& cie2)
{
  if (cie1.personality_name_ != cie2.personality_name_)
    return cie1.personality_name_ < cie2.personality_name_;
  return cie1.contents_ < cie2.contents_;
}

// Class Eh_frame.

Eh_frame::Eh_frame()
  : Output_section_data(Output_data::default_alignment()),
    eh_frame_hdr_(NULL),
    cie_offsets_(),
    unmergeable_cie_offsets_(),
    merge_map_(),
    mappings_are_done_(false),
    final_data_size_(0)
{
}

// Skip an LEB128, updating *PP to point to the next character.
// Return false if we ran off the end of the string.

bool
Eh_frame::skip_leb128(const unsigned char** pp, const unsigned char* pend)
{
  const unsigned char* p;
  for (p = *pp; p < pend; ++p)
    {
      if ((*p & 0x80) == 0)
	{
	  *pp = p + 1;
	  return true;
	}
    }
  return false;
}

// Add input section SHNDX in OBJECT to an exception frame section.
// SYMBOLS is the contents of the symbol table section (size
// SYMBOLS_SIZE), SYMBOL_NAMES is the symbol names section (size
// SYMBOL_NAMES_SIZE).  RELOC_SHNDX is the index of a relocation
// section applying to SHNDX, or 0 if none, or -1U if more than one.
// RELOC_TYPE is the type of the reloc section if there is one, either
// SHT_REL or SHT_RELA.  We try to parse the input exception frame
// data into our data structures.  If we can't do it, we return false
// to mean that the section should be handled as a normal input
// section.

template<int size, bool big_endian>
bool
Eh_frame::add_ehframe_input_section(
    Sized_relobj<size, big_endian>* object,
    const unsigned char* symbols,
    section_size_type symbols_size,
    const unsigned char* symbol_names,
    section_size_type symbol_names_size,
    unsigned int shndx,
    unsigned int reloc_shndx,
    unsigned int reloc_type)
{
  // Get the section contents.
  section_size_type contents_len;
  const unsigned char* pcontents = object->section_contents(shndx,
							    &contents_len,
							    false);
  if (contents_len == 0)
    return false;

  // If this is the marker section for the end of the data, then
  // return false to force it to be handled as an ordinary input
  // section.  If we don't do this, we won't correctly handle the case
  // of unrecognized .eh_frame sections.
  if (contents_len == 4
      && elfcpp::Swap<32, big_endian>::readval(pcontents) == 0)
    return false;

  New_cies new_cies;
  if (!this->do_add_ehframe_input_section(object, symbols, symbols_size,
					  symbol_names, symbol_names_size,
					  shndx, reloc_shndx,
					  reloc_type, pcontents,
					  contents_len, &new_cies))
    {
      if (this->eh_frame_hdr_ != NULL)
	this->eh_frame_hdr_->found_unrecognized_eh_frame_section();

      for (New_cies::iterator p = new_cies.begin();
	   p != new_cies.end();
	   ++p)
	delete p->first;

      return false;
    }

  // Now that we know we are using this section, record any new CIEs
  // that we found.
  for (New_cies::const_iterator p = new_cies.begin();
       p != new_cies.end();
       ++p)
    {
      if (p->second)
	this->cie_offsets_.insert(p->first);
      else
	this->unmergeable_cie_offsets_.push_back(p->first);
    }

  return true;
}

// The bulk of the implementation of add_ehframe_input_section.

template<int size, bool big_endian>
bool
Eh_frame::do_add_ehframe_input_section(
    Sized_relobj<size, big_endian>* object,
    const unsigned char* symbols,
    section_size_type symbols_size,
    const unsigned char* symbol_names,
    section_size_type symbol_names_size,
    unsigned int shndx,
    unsigned int reloc_shndx,
    unsigned int reloc_type,
    const unsigned char* pcontents,
    section_size_type contents_len,
    New_cies* new_cies)
{
  typedef typename elfcpp::Elf_types<size>::Elf_Addr Address;
  Track_relocs<size, big_endian> relocs;

  const unsigned char* p = pcontents;
  const unsigned char* pend = p + contents_len;

  // Get the contents of the reloc section if any.
  if (!relocs.initialize(object, reloc_shndx, reloc_type))
    return false;

  // Keep track of which CIEs are at which offsets.
  Offsets_to_cie cies;

  while (p < pend)
    {
      if (pend - p < 4)
	return false;

      // There shouldn't be any relocations here.
      if (relocs.advance(p + 4 - pcontents) > 0)
	return false;

      unsigned int len = elfcpp::Swap<32, big_endian>::readval(p);
      p += 4;
      if (len == 0)
	{
	  // We should only find a zero-length entry at the end of the
	  // section.
	  if (p < pend)
	    return false;
	  break;
	}
      // We don't support a 64-bit .eh_frame.
      if (len == 0xffffffff)
	return false;
      if (static_cast<unsigned int>(pend - p) < len)
	return false;

      const unsigned char* const pentend = p + len;

      if (pend - p < 4)
	return false;
      if (relocs.advance(p + 4 - pcontents) > 0)
	return false;

      unsigned int id = elfcpp::Swap<32, big_endian>::readval(p);
      p += 4;

      if (id == 0)
	{
	  // CIE.
	  if (!this->read_cie(object, shndx, symbols, symbols_size,
			      symbol_names, symbol_names_size,
			      pcontents, p, pentend, &relocs, &cies,
			      new_cies))
	    return false;
	}
      else
	{
	  // FDE.
	  if (!this->read_fde(object, shndx, symbols, symbols_size,
			      pcontents, id, p, pentend, &relocs, &cies))
	    return false;
	}

      p = pentend;
    }

  return true;
}

// Read a CIE.  Return false if we can't parse the information.

template<int size, bool big_endian>
bool
Eh_frame::read_cie(Sized_relobj<size, big_endian>* object,
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
		   New_cies* new_cies)
{
  bool mergeable = true;

  // We need to find the personality routine if there is one, since we
  // can only merge CIEs which use the same routine.  We also need to
  // find the FDE encoding if there is one, so that we can read the PC
  // from the FDE.

  const unsigned char* p = pcie;

  if (pcieend - p < 1)
    return false;
  unsigned char version = *p++;
  if (version != 1 && version != 3)
    return false;

  const unsigned char* paug = p;
  const void* paugendv = memchr(p, '\0', pcieend - p);
  const unsigned char* paugend = static_cast<const unsigned char*>(paugendv);
  if (paugend == NULL)
    return false;
  p = paugend + 1;

  if (paug[0] == 'e' && paug[1] == 'h')
    {
      // This is a CIE from gcc before version 3.0.  We can't merge
      // these.  We can still read the FDEs.
      mergeable = false;
      paug += 2;
      if (*paug != '\0')
	return false;
      if (pcieend - p < size / 8)
	return false;
      p += size / 8;
    }

  // Skip the code alignment.
  if (!skip_leb128(&p, pcieend))
    return false;

  // Skip the data alignment.
  if (!skip_leb128(&p, pcieend))
    return false;

  // Skip the return column.
  if (version == 1)
    {
      if (pcieend - p < 1)
	return false;
      ++p;
    }
  else
    {
      if (!skip_leb128(&p, pcieend))
	return false;
    }

  if (*paug == 'z')
    {
      ++paug;
      // Skip the augmentation size.
      if (!skip_leb128(&p, pcieend))
	return false;
    }

  unsigned char fde_encoding = elfcpp::DW_EH_PE_absptr;
  int per_offset = -1;
  while (*paug != '\0')
    {
      switch (*paug)
	{
	case 'L': // LSDA encoding.
	  if (pcieend - p < 1)
	    return false;
	  ++p;
	  break;

	case 'R': // FDE encoding.
	  if (pcieend - p < 1)
	    return false;
	  fde_encoding = *p;
	  switch (fde_encoding & 7)
	    {
	    case elfcpp::DW_EH_PE_absptr:
	    case elfcpp::DW_EH_PE_udata2:
	    case elfcpp::DW_EH_PE_udata4:
	    case elfcpp::DW_EH_PE_udata8:
	      break;
	    default:
	      // We don't expect to see any other cases here, and
	      // we're not prepared to handle them.
	      return false;
	    }
	  ++p;
	  break;

	case 'S':
	  break;

	case 'P':
	  // Personality encoding.
	  {
	    if (pcieend - p < 1)
	      return false;
	    unsigned char per_encoding = *p;
	    ++p;

	    if ((per_encoding & 0x60) == 0x60)
	      return false;
	    unsigned int per_width;
	    switch (per_encoding & 7)
	      {
	      case elfcpp::DW_EH_PE_udata2:
		per_width = 2;
		break;
	      case elfcpp::DW_EH_PE_udata4:
		per_width = 4;
		break;
	      case elfcpp::DW_EH_PE_udata8:
		per_width = 8;
		break;
	      case elfcpp::DW_EH_PE_absptr:
		per_width = size / 8;
		break;
	      default:
		return false;
	      }

	    if ((per_encoding & 0xf0) == elfcpp::DW_EH_PE_aligned)
	      {
		unsigned int len = p - pcie;
		len += per_width - 1;
		len &= ~ (per_width - 1);
		if (static_cast<unsigned int>(pcieend - p) < len)
		  return false;
		p += len;
	      }

	    per_offset = p - pcontents;

	    if (static_cast<unsigned int>(pcieend - p) < per_width)
	      return false;
	    p += per_width;
	  }
	  break;

	default:
	  return false;
	}

      ++paug;
    }

  const char* personality_name = "";
  if (per_offset != -1)
    {
      if (relocs->advance(per_offset) > 0)
	return false;
      if (relocs->next_offset() != per_offset)
	return false;

      unsigned int personality_symndx = relocs->next_symndx();
      if (personality_symndx == -1U)
	return false;

      if (personality_symndx < object->local_symbol_count())
	{
	  // We can only merge this CIE if the personality routine is
	  // a global symbol.  We can still read the FDEs.
	  mergeable = false;
	}
      else
	{
	  const int sym_size = elfcpp::Elf_sizes<size>::sym_size;
	  if (personality_symndx >= symbols_size / sym_size)
	    return false;
	  elfcpp::Sym<size, big_endian> sym(symbols
					    + (personality_symndx * sym_size));
	  unsigned int name_offset = sym.get_st_name();
	  if (name_offset >= symbol_names_size)
	    return false;
	  personality_name = (reinterpret_cast<const char*>(symbol_names)
			      + name_offset);
	}

      int r = relocs->advance(per_offset + 1);
      gold_assert(r == 1);
    }

  if (relocs->advance(pcieend - pcontents) > 0)
    return false;

  Cie cie(object, shndx, (pcie - 8) - pcontents, fde_encoding, 
	  personality_name, pcie, pcieend - pcie);
  Cie* cie_pointer = NULL;
  if (mergeable)
    {
      Cie_offsets::iterator find_cie = this->cie_offsets_.find(&cie);
      if (find_cie != this->cie_offsets_.end())
	cie_pointer = *find_cie;
      else
	{
	  // See if we already saw this CIE in this object file.
	  for (New_cies::const_iterator pc = new_cies->begin();
	       pc != new_cies->end();
	       ++pc)
	    {
	      if (*(pc->first) == cie)
		{
		  cie_pointer = pc->first;
		  break;
		}
	    }
	}
    }

  if (cie_pointer == NULL)
    {
      cie_pointer = new Cie(cie);
      new_cies->push_back(std::make_pair(cie_pointer, mergeable));
    }
  else
    {
      // We are deleting this CIE.  Record that in our mapping from
      // input sections to the output section.  At this point we don't
      // know for sure that we are doing a special mapping for this
      // input section, but that's OK--if we don't do a special
      // mapping, nobody will ever ask for the mapping we add here.
      this->merge_map_.add_mapping(object, shndx, (pcie - 8) - pcontents,
				   pcieend - (pcie - 8), -1);
    }

  // Record this CIE plus the offset in the input section.
  cies->insert(std::make_pair(pcie - pcontents, cie_pointer));

  return true;
}

// Read an FDE.  Return false if we can't parse the information.

template<int size, bool big_endian>
bool
Eh_frame::read_fde(Sized_relobj<size, big_endian>* object,
		   unsigned int shndx,
		   const unsigned char* symbols,
		   section_size_type symbols_size,
		   const unsigned char* pcontents,
		   unsigned int offset,
		   const unsigned char* pfde,
		   const unsigned char* pfdeend,
		   Track_relocs<size, big_endian>* relocs,
		   Offsets_to_cie* cies)
{
  // OFFSET is the distance between the 4 bytes before PFDE to the
  // start of the CIE.  The offset we recorded for the CIE is 8 bytes
  // after the start of the CIE--after the length and the zero tag.
  unsigned int cie_offset = (pfde - 4 - pcontents) - offset + 8;
  Offsets_to_cie::const_iterator pcie = cies->find(cie_offset);
  if (pcie == cies->end())
    return false;
  Cie* cie = pcie->second;

  // The FDE should start with a reloc to the start of the code which
  // it describes.
  if (relocs->advance(pfde - pcontents) > 0)
    return false;

  if (relocs->next_offset() != pfde - pcontents)
    return false;

  unsigned int symndx = relocs->next_symndx();
  if (symndx == -1U)
    return false;

  // There can be another reloc in the FDE, if the CIE specifies an
  // LSDA (language specific data area).  We currently don't care.  We
  // will care later if we want to optimize the LSDA from an absolute
  // pointer to a PC relative offset when generating a shared library.
  relocs->advance(pfdeend - pcontents);

  unsigned int fde_shndx;
  const int sym_size = elfcpp::Elf_sizes<size>::sym_size;
  if (symndx >= symbols_size / sym_size)
    return false;
  elfcpp::Sym<size, big_endian> sym(symbols + symndx * sym_size);
  bool is_ordinary;
  fde_shndx = object->adjust_sym_shndx(symndx, sym.get_st_shndx(),
				       &is_ordinary);

  if (is_ordinary
      && fde_shndx != elfcpp::SHN_UNDEF
      && fde_shndx < object->shnum()
      && !object->is_section_included(fde_shndx))
    {
      // This FDE applies to a section which we are discarding.  We
      // can discard this FDE.
      this->merge_map_.add_mapping(object, shndx, (pfde - 8) - pcontents,
				   pfdeend - (pfde - 8), -1);
      return true;
    }

  cie->add_fde(new Fde(object, shndx, (pfde - 8) - pcontents,
		       pfde, pfdeend - pfde));

  return true;
}

// Return the number of FDEs.

unsigned int
Eh_frame::fde_count() const
{
  unsigned int ret = 0;
  for (Unmergeable_cie_offsets::const_iterator p =
	 this->unmergeable_cie_offsets_.begin();
       p != this->unmergeable_cie_offsets_.end();
       ++p)
    ret += (*p)->fde_count();
  for (Cie_offsets::const_iterator p = this->cie_offsets_.begin();
       p != this->cie_offsets_.end();
       ++p)
    ret += (*p)->fde_count();
  return ret;
}

// Set the final data size.

void
Eh_frame::set_final_data_size()
{
  // We can be called more than once if Layout::set_segment_offsets
  // finds a better mapping.  We don't want to add all the mappings
  // again.
  if (this->mappings_are_done_)
    {
      this->set_data_size(this->final_data_size_);
      return;
    }

  section_offset_type output_offset = 0;

  for (Unmergeable_cie_offsets::iterator p =
	 this->unmergeable_cie_offsets_.begin();
       p != this->unmergeable_cie_offsets_.end();
       ++p)
    output_offset = (*p)->set_output_offset(output_offset,
					    this->addralign(),
					    &this->merge_map_);

  for (Cie_offsets::iterator p = this->cie_offsets_.begin();
       p != this->cie_offsets_.end();
       ++p)
    output_offset = (*p)->set_output_offset(output_offset,
					    this->addralign(),
					    &this->merge_map_);

  this->mappings_are_done_ = true;
  this->final_data_size_ = output_offset;

  gold_assert((output_offset & (this->addralign() - 1)) == 0);
  this->set_data_size(output_offset);
}

// Return an output offset for an input offset.

bool
Eh_frame::do_output_offset(const Relobj* object, unsigned int shndx,
			   section_offset_type offset,
			   section_offset_type* poutput) const
{
  return this->merge_map_.get_output_offset(object, shndx, offset, poutput);
}

// Return whether this is the merge section for an input section.

bool
Eh_frame::do_is_merge_section_for(const Relobj* object,
				  unsigned int shndx) const
{
  return this->merge_map_.is_merge_section_for(object, shndx);
}

// Write the data to the output file.

void
Eh_frame::do_write(Output_file* of)
{
  const off_t offset = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(offset, oview_size);

  switch (parameters->size_and_endianness())
    {
#ifdef HAVE_TARGET_32_LITTLE
    case Parameters::TARGET_32_LITTLE:
      this->do_sized_write<32, false>(oview);
      break;
#endif
#ifdef HAVE_TARGET_32_BIG
    case Parameters::TARGET_32_BIG:
      this->do_sized_write<32, true>(oview);
      break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
    case Parameters::TARGET_64_LITTLE:
      this->do_sized_write<64, false>(oview);
      break;
#endif
#ifdef HAVE_TARGET_64_BIG
    case Parameters::TARGET_64_BIG:
      this->do_sized_write<64, true>(oview);
      break;
#endif
    default:
      gold_unreachable();
    }

  of->write_output_view(offset, oview_size, oview);
}

// Write the data to the output file--template version.

template<int size, bool big_endian>
void
Eh_frame::do_sized_write(unsigned char* oview)
{
  unsigned int addralign = this->addralign();
  section_offset_type o = 0;
  for (Unmergeable_cie_offsets::iterator p =
	 this->unmergeable_cie_offsets_.begin();
       p != this->unmergeable_cie_offsets_.end();
       ++p)
    o = (*p)->write<size, big_endian>(oview, o, addralign,
                                      this->eh_frame_hdr_);
  for (Cie_offsets::iterator p = this->cie_offsets_.begin();
       p != this->cie_offsets_.end();
       ++p)
    o = (*p)->write<size, big_endian>(oview, o, addralign,
                                      this->eh_frame_hdr_);
}

#ifdef HAVE_TARGET_32_LITTLE
template
bool
Eh_frame::add_ehframe_input_section<32, false>(
    Sized_relobj<32, false>* object,
    const unsigned char* symbols,
    section_size_type symbols_size,
    const unsigned char* symbol_names,
    section_size_type symbol_names_size,
    unsigned int shndx,
    unsigned int reloc_shndx,
    unsigned int reloc_type);
#endif

#ifdef HAVE_TARGET_32_BIG
template
bool
Eh_frame::add_ehframe_input_section<32, true>(
    Sized_relobj<32, true>* object,
    const unsigned char* symbols,
    section_size_type symbols_size,
    const unsigned char* symbol_names,
    section_size_type symbol_names_size,
    unsigned int shndx,
    unsigned int reloc_shndx,
    unsigned int reloc_type);
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
bool
Eh_frame::add_ehframe_input_section<64, false>(
    Sized_relobj<64, false>* object,
    const unsigned char* symbols,
    section_size_type symbols_size,
    const unsigned char* symbol_names,
    section_size_type symbol_names_size,
    unsigned int shndx,
    unsigned int reloc_shndx,
    unsigned int reloc_type);
#endif

#ifdef HAVE_TARGET_64_BIG
template
bool
Eh_frame::add_ehframe_input_section<64, true>(
    Sized_relobj<64, true>* object,
    const unsigned char* symbols,
    section_size_type symbols_size,
    const unsigned char* symbol_names,
    section_size_type symbol_names_size,
    unsigned int shndx,
    unsigned int reloc_shndx,
    unsigned int reloc_type);
#endif

} // End namespace gold.
