// dwarf_reader.cc -- parse dwarf2/3 debug information

// Copyright 2007, 2008 Free Software Foundation, Inc.
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

#include <algorithm>
#include <vector>

#include "elfcpp_swap.h"
#include "dwarf.h"
#include "object.h"
#include "parameters.h"
#include "reloc.h"
#include "dwarf_reader.h"

namespace gold {

// Read an unsigned LEB128 number.  Each byte contains 7 bits of
// information, plus one bit saying whether the number continues or
// not.

uint64_t
read_unsigned_LEB_128(const unsigned char* buffer, size_t* len)
{
  uint64_t result = 0;
  size_t num_read = 0;
  unsigned int shift = 0;
  unsigned char byte;

  do
    {
      if (num_read >= 64 / 7) 
        {
          gold_warning(_("Unusually large LEB128 decoded, "
			 "debug information may be corrupted"));
          break;
        }
      byte = *buffer++;
      num_read++;
      result |= (static_cast<uint64_t>(byte & 0x7f)) << shift;
      shift += 7;
    }
  while (byte & 0x80);

  *len = num_read;

  return result;
}

// Read a signed LEB128 number.  These are like regular LEB128
// numbers, except the last byte may have a sign bit set.

int64_t
read_signed_LEB_128(const unsigned char* buffer, size_t* len)
{
  int64_t result = 0;
  int shift = 0;
  size_t num_read = 0;
  unsigned char byte;

  do
    {
      if (num_read >= 64 / 7) 
        {
          gold_warning(_("Unusually large LEB128 decoded, "
			 "debug information may be corrupted"));
          break;
        }
      byte = *buffer++;
      num_read++;
      result |= (static_cast<uint64_t>(byte & 0x7f) << shift);
      shift += 7;
    }
  while (byte & 0x80);

  if ((shift < 8 * static_cast<int>(sizeof(result))) && (byte & 0x40))
    result |= -((static_cast<int64_t>(1)) << shift);
  *len = num_read;
  return result;
}

// This is the format of a DWARF2/3 line state machine that we process
// opcodes using.  There is no need for anything outside the lineinfo
// processor to know how this works.

struct LineStateMachine
{
  int file_num;
  uint64_t address;
  int line_num;
  int column_num;
  unsigned int shndx;    // the section address refers to
  bool is_stmt;          // stmt means statement.
  bool basic_block;
  bool end_sequence;
};

static void
ResetLineStateMachine(struct LineStateMachine* lsm, bool default_is_stmt)
{
  lsm->file_num = 1;
  lsm->address = 0;
  lsm->line_num = 1;
  lsm->column_num = 0;
  lsm->shndx = -1U;
  lsm->is_stmt = default_is_stmt;
  lsm->basic_block = false;
  lsm->end_sequence = false;
}

template<int size, bool big_endian>
Sized_dwarf_line_info<size, big_endian>::Sized_dwarf_line_info(Object* object,
                                                               off_t read_shndx)
  : data_valid_(false), buffer_(NULL), symtab_buffer_(NULL),
    directories_(), files_(), current_header_index_(-1)
{
  unsigned int debug_shndx;
  for (debug_shndx = 0; debug_shndx < object->shnum(); ++debug_shndx)
    // FIXME: do this more efficiently: section_name() isn't super-fast
    if (object->section_name(debug_shndx) == ".debug_line")
      {
        section_size_type buffer_size;
        this->buffer_ = object->section_contents(debug_shndx, &buffer_size,
						 false);
        this->buffer_end_ = this->buffer_ + buffer_size;
        break;
      }
  if (this->buffer_ == NULL)
    return;

  // Find the relocation section for ".debug_line".
  // We expect these for relobjs (.o's) but not dynobjs (.so's).
  bool got_relocs = false;
  for (unsigned int reloc_shndx = 0;
       reloc_shndx < object->shnum();
       ++reloc_shndx)
    {
      unsigned int reloc_sh_type = object->section_type(reloc_shndx);
      if ((reloc_sh_type == elfcpp::SHT_REL
	   || reloc_sh_type == elfcpp::SHT_RELA)
	  && object->section_info(reloc_shndx) == debug_shndx)
	{
	  got_relocs = this->track_relocs_.initialize(object, reloc_shndx,
                                                      reloc_sh_type);
	  break;
	}
    }

  // Finally, we need the symtab section to interpret the relocs.
  if (got_relocs)
    {
      unsigned int symtab_shndx;
      for (symtab_shndx = 0; symtab_shndx < object->shnum(); ++symtab_shndx)
        if (object->section_type(symtab_shndx) == elfcpp::SHT_SYMTAB)
          {
            this->symtab_buffer_ = object->section_contents(
                symtab_shndx, &this->symtab_buffer_size_, false);
            break;
          }
      if (this->symtab_buffer_ == NULL)
        return;
    }

  // Now that we have successfully read all the data, parse the debug
  // info.
  this->data_valid_ = true;
  this->read_line_mappings(object, read_shndx);
}

// Read the DWARF header.

template<int size, bool big_endian>
const unsigned char*
Sized_dwarf_line_info<size, big_endian>::read_header_prolog(
    const unsigned char* lineptr)
{
  uint32_t initial_length = elfcpp::Swap_unaligned<32, big_endian>::readval(lineptr);
  lineptr += 4;

  // In DWARF2/3, if the initial length is all 1 bits, then the offset
  // size is 8 and we need to read the next 8 bytes for the real length.
  if (initial_length == 0xffffffff)
    {
      header_.offset_size = 8;
      initial_length = elfcpp::Swap_unaligned<64, big_endian>::readval(lineptr);
      lineptr += 8;
    }
  else
    header_.offset_size = 4;

  header_.total_length = initial_length;

  gold_assert(lineptr + header_.total_length <= buffer_end_);

  header_.version = elfcpp::Swap_unaligned<16, big_endian>::readval(lineptr);
  lineptr += 2;

  if (header_.offset_size == 4)
    header_.prologue_length = elfcpp::Swap_unaligned<32, big_endian>::readval(lineptr);
  else
    header_.prologue_length = elfcpp::Swap_unaligned<64, big_endian>::readval(lineptr);
  lineptr += header_.offset_size;

  header_.min_insn_length = *lineptr;
  lineptr += 1;

  header_.default_is_stmt = *lineptr;
  lineptr += 1;

  header_.line_base = *reinterpret_cast<const signed char*>(lineptr);
  lineptr += 1;

  header_.line_range = *lineptr;
  lineptr += 1;

  header_.opcode_base = *lineptr;
  lineptr += 1;

  header_.std_opcode_lengths.reserve(header_.opcode_base + 1);
  header_.std_opcode_lengths[0] = 0;
  for (int i = 1; i < header_.opcode_base; i++)
    {
      header_.std_opcode_lengths[i] = *lineptr;
      lineptr += 1;
    }

  return lineptr;
}

// The header for a debug_line section is mildly complicated, because
// the line info is very tightly encoded.

template<int size, bool big_endian>
const unsigned char*
Sized_dwarf_line_info<size, big_endian>::read_header_tables(
    const unsigned char* lineptr)
{
  ++this->current_header_index_;

  // Create a new directories_ entry and a new files_ entry for our new
  // header.  We initialize each with a single empty element, because
  // dwarf indexes directory and filenames starting at 1.
  gold_assert(static_cast<int>(this->directories_.size())
	      == this->current_header_index_);
  gold_assert(static_cast<int>(this->files_.size())
	      == this->current_header_index_);
  this->directories_.push_back(std::vector<std::string>(1));
  this->files_.push_back(std::vector<std::pair<int, std::string> >(1));

  // It is legal for the directory entry table to be empty.
  if (*lineptr)
    {
      int dirindex = 1;
      while (*lineptr)
        {
	  const char* dirname = reinterpret_cast<const char*>(lineptr);
          gold_assert(dirindex
		      == static_cast<int>(this->directories_.back().size()));
          this->directories_.back().push_back(dirname);
          lineptr += this->directories_.back().back().size() + 1;
          dirindex++;
        }
    }
  lineptr++;

  // It is also legal for the file entry table to be empty.
  if (*lineptr)
    {
      int fileindex = 1;
      size_t len;
      while (*lineptr)
        {
          const char* filename = reinterpret_cast<const char*>(lineptr);
          lineptr += strlen(filename) + 1;

          uint64_t dirindex = read_unsigned_LEB_128(lineptr, &len);
          lineptr += len;

          if (dirindex >= this->directories_.back().size())
            dirindex = 0;
	  int dirindexi = static_cast<int>(dirindex);

          read_unsigned_LEB_128(lineptr, &len);   // mod_time
          lineptr += len;

          read_unsigned_LEB_128(lineptr, &len);   // filelength
          lineptr += len;

          gold_assert(fileindex
		      == static_cast<int>(this->files_.back().size()));
          this->files_.back().push_back(std::make_pair(dirindexi, filename));
          fileindex++;
        }
    }
  lineptr++;

  return lineptr;
}

// Process a single opcode in the .debug.line structure.

// Templating on size and big_endian would yield more efficient (and
// simpler) code, but would bloat the binary.  Speed isn't important
// here.

template<int size, bool big_endian>
bool
Sized_dwarf_line_info<size, big_endian>::process_one_opcode(
    const unsigned char* start, struct LineStateMachine* lsm, size_t* len)
{
  size_t oplen = 0;
  size_t templen;
  unsigned char opcode = *start;
  oplen++;
  start++;

  // If the opcode is great than the opcode_base, it is a special
  // opcode. Most line programs consist mainly of special opcodes.
  if (opcode >= header_.opcode_base)
    {
      opcode -= header_.opcode_base;
      const int advance_address = ((opcode / header_.line_range)
                                   * header_.min_insn_length);
      lsm->address += advance_address;

      const int advance_line = ((opcode % header_.line_range)
                                + header_.line_base);
      lsm->line_num += advance_line;
      lsm->basic_block = true;
      *len = oplen;
      return true;
    }

  // Otherwise, we have the regular opcodes
  switch (opcode)
    {
    case elfcpp::DW_LNS_copy:
      lsm->basic_block = false;
      *len = oplen;
      return true;

    case elfcpp::DW_LNS_advance_pc:
      {
        const uint64_t advance_address
            = read_unsigned_LEB_128(start, &templen);
        oplen += templen;
        lsm->address += header_.min_insn_length * advance_address;
      }
      break;

    case elfcpp::DW_LNS_advance_line:
      {
        const uint64_t advance_line = read_signed_LEB_128(start, &templen);
        oplen += templen;
        lsm->line_num += advance_line;
      }
      break;

    case elfcpp::DW_LNS_set_file:
      {
        const uint64_t fileno = read_unsigned_LEB_128(start, &templen);
        oplen += templen;
        lsm->file_num = fileno;
      }
      break;

    case elfcpp::DW_LNS_set_column:
      {
        const uint64_t colno = read_unsigned_LEB_128(start, &templen);
        oplen += templen;
        lsm->column_num = colno;
      }
      break;

    case elfcpp::DW_LNS_negate_stmt:
      lsm->is_stmt = !lsm->is_stmt;
      break;

    case elfcpp::DW_LNS_set_basic_block:
      lsm->basic_block = true;
      break;

    case elfcpp::DW_LNS_fixed_advance_pc:
      {
        int advance_address;
        advance_address = elfcpp::Swap_unaligned<16, big_endian>::readval(start);
        oplen += 2;
        lsm->address += advance_address;
      }
      break;

    case elfcpp::DW_LNS_const_add_pc:
      {
        const int advance_address = (header_.min_insn_length
                                     * ((255 - header_.opcode_base)
                                        / header_.line_range));
        lsm->address += advance_address;
      }
      break;

    case elfcpp::DW_LNS_extended_op:
      {
        const uint64_t extended_op_len
            = read_unsigned_LEB_128(start, &templen);
        start += templen;
        oplen += templen + extended_op_len;

        const unsigned char extended_op = *start;
        start++;

        switch (extended_op)
          {
          case elfcpp::DW_LNE_end_sequence:
            // This means that the current byte is the one immediately
            // after a set of instructions.  Record the current line
            // for up to one less than the current address.
            lsm->line_num = -1;
            lsm->end_sequence = true;
            *len = oplen;
            return true;

          case elfcpp::DW_LNE_set_address:
            {
              lsm->address = elfcpp::Swap_unaligned<size, big_endian>::readval(start);
              typename Reloc_map::const_iterator it
                  = reloc_map_.find(start - this->buffer_);
              if (it != reloc_map_.end())
                {
                  // value + addend.
                  lsm->address += it->second.second;
                  lsm->shndx = it->second.first;
                }
              else
                {
                  // If we're a normal .o file, with relocs, every
                  // set_address should have an associated relocation.
		  if (this->input_is_relobj())
                    this->data_valid_ = false;
                }
              break;
            }
          case elfcpp::DW_LNE_define_file:
            {
              const char* filename  = reinterpret_cast<const char*>(start);
              templen = strlen(filename) + 1;
              start += templen;

              uint64_t dirindex = read_unsigned_LEB_128(start, &templen);
              oplen += templen;

              if (dirindex >= this->directories_.back().size())
                dirindex = 0;
	      int dirindexi = static_cast<int>(dirindex);

              read_unsigned_LEB_128(start, &templen);   // mod_time
              oplen += templen;

              read_unsigned_LEB_128(start, &templen);   // filelength
              oplen += templen;

              this->files_.back().push_back(std::make_pair(dirindexi,
							   filename));
            }
            break;
          }
      }
      break;

    default:
      {
        // Ignore unknown opcode  silently
        for (int i = 0; i < header_.std_opcode_lengths[opcode]; i++)
          {
            size_t templen;
            read_unsigned_LEB_128(start, &templen);
            start += templen;
            oplen += templen;
          }
      }
      break;
  }
  *len = oplen;
  return false;
}

// Read the debug information at LINEPTR and store it in the line
// number map.

template<int size, bool big_endian>
unsigned const char*
Sized_dwarf_line_info<size, big_endian>::read_lines(unsigned const char* lineptr,
                                                    off_t shndx)
{
  struct LineStateMachine lsm;

  // LENGTHSTART is the place the length field is based on.  It is the
  // point in the header after the initial length field.
  const unsigned char* lengthstart = buffer_;

  // In 64 bit dwarf, the initial length is 12 bytes, because of the
  // 0xffffffff at the start.
  if (header_.offset_size == 8)
    lengthstart += 12;
  else
    lengthstart += 4;

  while (lineptr < lengthstart + header_.total_length)
    {
      ResetLineStateMachine(&lsm, header_.default_is_stmt);
      while (!lsm.end_sequence)
        {
          size_t oplength;
          bool add_line = this->process_one_opcode(lineptr, &lsm, &oplength);
          if (add_line
              && (shndx == -1U || lsm.shndx == -1U || shndx == lsm.shndx))
            {
              Offset_to_lineno_entry entry
                  = { lsm.address, this->current_header_index_,
                      lsm.file_num, lsm.line_num };
              line_number_map_[lsm.shndx].push_back(entry);
            }
          lineptr += oplength;
        }
    }

  return lengthstart + header_.total_length;
}

// Looks in the symtab to see what section a symbol is in.

template<int size, bool big_endian>
unsigned int
Sized_dwarf_line_info<size, big_endian>::symbol_section(
    Object* object,
    unsigned int sym,
    typename elfcpp::Elf_types<size>::Elf_Addr* value,
    bool* is_ordinary)
{
  const int symsize = elfcpp::Elf_sizes<size>::sym_size;
  gold_assert(sym * symsize < this->symtab_buffer_size_);
  elfcpp::Sym<size, big_endian> elfsym(this->symtab_buffer_ + sym * symsize);
  *value = elfsym.get_st_value();
  return object->adjust_sym_shndx(sym, elfsym.get_st_shndx(), is_ordinary);
}

// Read the relocations into a Reloc_map.

template<int size, bool big_endian>
void
Sized_dwarf_line_info<size, big_endian>::read_relocs(Object* object)
{
  if (this->symtab_buffer_ == NULL)
    return;

  typename elfcpp::Elf_types<size>::Elf_Addr value;
  off_t reloc_offset;
  while ((reloc_offset = this->track_relocs_.next_offset()) != -1)
    {
      const unsigned int sym = this->track_relocs_.next_symndx();

      bool is_ordinary;
      const unsigned int shndx = this->symbol_section(object, sym, &value,
						      &is_ordinary);

      // There is no reason to record non-ordinary section indexes, or
      // SHN_UNDEF, because they will never match the real section.
      if (is_ordinary && shndx != elfcpp::SHN_UNDEF)
	this->reloc_map_[reloc_offset] = std::make_pair(shndx, value);

      this->track_relocs_.advance(reloc_offset + 1);
    }
}

// Read the line number info.

template<int size, bool big_endian>
void
Sized_dwarf_line_info<size, big_endian>::read_line_mappings(Object* object,
							    off_t shndx)
{
  gold_assert(this->data_valid_ == true);

  this->read_relocs(object);
  while (this->buffer_ < this->buffer_end_)
    {
      const unsigned char* lineptr = this->buffer_;
      lineptr = this->read_header_prolog(lineptr);
      lineptr = this->read_header_tables(lineptr);
      lineptr = this->read_lines(lineptr, shndx);
      this->buffer_ = lineptr;
    }

  // Sort the lines numbers, so addr2line can use binary search.
  for (typename Lineno_map::iterator it = line_number_map_.begin();
       it != line_number_map_.end();
       ++it)
    // Each vector needs to be sorted by offset.
    std::sort(it->second.begin(), it->second.end());
}

// Some processing depends on whether the input is a .o file or not.
// For instance, .o files have relocs, and have .debug_lines
// information on a per section basis.  .so files, on the other hand,
// lack relocs, and offsets are unique, so we can ignore the section
// information.

template<int size, bool big_endian>
bool
Sized_dwarf_line_info<size, big_endian>::input_is_relobj()
{
  // Only .o files have relocs and the symtab buffer that goes with them.
  return this->symtab_buffer_ != NULL;
}

// Given an Offset_to_lineno_entry vector, and an offset, figure out
// if the offset points into a function according to the vector (see
// comments below for the algorithm).  If it does, return an iterator
// into the vector that points to the line-number that contains that
// offset.  If not, it returns vector::end().

static std::vector<Offset_to_lineno_entry>::const_iterator
offset_to_iterator(const std::vector<Offset_to_lineno_entry>* offsets,
                   off_t offset)
{
  const Offset_to_lineno_entry lookup_key = { offset, 0, 0, 0 };

  // lower_bound() returns the smallest offset which is >= lookup_key.
  // If no offset in offsets is >= lookup_key, returns end().
  std::vector<Offset_to_lineno_entry>::const_iterator it
      = std::lower_bound(offsets->begin(), offsets->end(), lookup_key);

  // This code is easiest to understand with a concrete example.
  // Here's a possible offsets array:
  // {{offset = 3211, header_num = 0, file_num = 1, line_num = 16},  // 0
  //  {offset = 3224, header_num = 0, file_num = 1, line_num = 20},  // 1
  //  {offset = 3226, header_num = 0, file_num = 1, line_num = 22},  // 2
  //  {offset = 3231, header_num = 0, file_num = 1, line_num = 25},  // 3
  //  {offset = 3232, header_num = 0, file_num = 1, line_num = -1},  // 4
  //  {offset = 3232, header_num = 0, file_num = 1, line_num = 65},  // 5
  //  {offset = 3235, header_num = 0, file_num = 1, line_num = 66},  // 6
  //  {offset = 3236, header_num = 0, file_num = 1, line_num = -1},  // 7
  //  {offset = 5764, header_num = 0, file_num = 1, line_num = 47},  // 8
  //  {offset = 5765, header_num = 0, file_num = 1, line_num = 48},  // 9
  //  {offset = 5767, header_num = 0, file_num = 1, line_num = 49},  // 10
  //  {offset = 5768, header_num = 0, file_num = 1, line_num = 50},  // 11
  //  {offset = 5773, header_num = 0, file_num = 1, line_num = -1},  // 12
  //  {offset = 5787, header_num = 1, file_num = 1, line_num = 19},  // 13
  //  {offset = 5790, header_num = 1, file_num = 1, line_num = 20},  // 14
  //  {offset = 5793, header_num = 1, file_num = 1, line_num = 67},  // 15
  //  {offset = 5793, header_num = 1, file_num = 1, line_num = -1},  // 16
  //  {offset = 5795, header_num = 1, file_num = 1, line_num = 68},  // 17
  //  {offset = 5798, header_num = 1, file_num = 1, line_num = -1},  // 18
  // The entries with line_num == -1 mark the end of a function: the
  // associated offset is one past the last instruction in the
  // function.  This can correspond to the beginning of the next
  // function (as is true for offset 3232); alternately, there can be
  // a gap between the end of one function and the start of the next
  // (as is true for some others, most obviously from 3236->5764).
  //
  // Case 1: lookup_key has offset == 10.  lower_bound returns
  //         offsets[0].  Since it's not an exact match and we're
  //         at the beginning of offsets, we return end() (invalid).
  // Case 2: lookup_key has offset 10000.  lower_bound returns
  //         offset[19] (end()).  We return end() (invalid).
  // Case 3: lookup_key has offset == 3211.  lower_bound matches
  //         offsets[0] exactly, and that's the entry we return.
  // Case 4: lookup_key has offset == 3232.  lower_bound returns
  //         offsets[4].  That's an exact match, but indicates
  //         end-of-function.  We check if offsets[5] is also an
  //         exact match but not end-of-function.  It is, so we
  //         return offsets[5].
  // Case 5: lookup_key has offset == 3214.  lower_bound returns
  //         offsets[1].  Since it's not an exact match, we back
  //         up to the offset that's < lookup_key, offsets[0].
  //         We note offsets[0] is a valid entry (not end-of-function),
  //         so that's the entry we return.
  // Case 6: lookup_key has offset == 4000.  lower_bound returns
  //         offsets[8].  Since it's not an exact match, we back
  //         up to offsets[7].  Since offsets[7] indicates
  //         end-of-function, we know lookup_key is between
  //         functions, so we return end() (not a valid offset).
  // Case 7: lookup_key has offset == 5794.  lower_bound returns
  //         offsets[17].  Since it's not an exact match, we back
  //         up to offsets[15].  Note we back up to the *first*
  //         entry with offset 5793, not just offsets[17-1].
  //         We note offsets[15] is a valid entry, so we return it.
  //         If offsets[15] had had line_num == -1, we would have
  //         checked offsets[16].  The reason for this is that
  //         15 and 16 can be in an arbitrary order, since we sort
  //         only by offset.  (Note it doesn't help to use line_number
  //         as a secondary sort key, since sometimes we want the -1
  //         to be first and sometimes we want it to be last.)

  // This deals with cases (1) and (2).
  if ((it == offsets->begin() && offset < it->offset)
      || it == offsets->end())
    return offsets->end();

  // This deals with cases (3) and (4).
  if (offset == it->offset)
    {
      while (it != offsets->end()
             && it->offset == offset
             && it->line_num == -1)
        ++it;
      if (it == offsets->end() || it->offset != offset)
        return offsets->end();
      else
        return it;
    }

  // This handles the first part of case (7) -- we back up to the
  // *first* entry that has the offset that's behind us.
  gold_assert(it != offsets->begin());
  std::vector<Offset_to_lineno_entry>::const_iterator range_end = it;
  --it;
  const off_t range_value = it->offset;
  while (it != offsets->begin() && (it-1)->offset == range_value)
    --it;

  // This handles cases (5), (6), and (7): if any entry in the
  // equal_range [it, range_end) has a line_num != -1, it's a valid
  // match.  If not, we're not in a function.
  for (; it != range_end; ++it)
    if (it->line_num != -1)
      return it;
  return offsets->end();
}

// Return a string for a file name and line number.

template<int size, bool big_endian>
std::string
Sized_dwarf_line_info<size, big_endian>::do_addr2line(unsigned int shndx,
                                                      off_t offset)
{
  if (this->data_valid_ == false)
    return "";

  const std::vector<Offset_to_lineno_entry>* offsets;
  // If we do not have reloc information, then our input is a .so or
  // some similar data structure where all the information is held in
  // the offset.  In that case, we ignore the input shndx.
  if (this->input_is_relobj())
    offsets = &this->line_number_map_[shndx];
  else
    offsets = &this->line_number_map_[-1U];
  if (offsets->empty())
    return "";

  typename std::vector<Offset_to_lineno_entry>::const_iterator it
      = offset_to_iterator(offsets, offset);
  if (it == offsets->end())
    return "";

  // Convert the file_num + line_num into a string.
  std::string ret;

  gold_assert(it->header_num < static_cast<int>(this->files_.size()));
  gold_assert(it->file_num
	      < static_cast<int>(this->files_[it->header_num].size()));
  const std::pair<int, std::string>& filename_pair
      = this->files_[it->header_num][it->file_num];
  const std::string& filename = filename_pair.second;

  gold_assert(it->header_num < static_cast<int>(this->directories_.size()));
  gold_assert(filename_pair.first
              < static_cast<int>(this->directories_[it->header_num].size()));
  const std::string& dirname
      = this->directories_[it->header_num][filename_pair.first];

  if (!dirname.empty())
    {
      ret += dirname;
      ret += "/";
    }
  ret += filename;
  if (ret.empty())
    ret = "(unknown)";

  char buffer[64];   // enough to hold a line number
  snprintf(buffer, sizeof(buffer), "%d", it->line_num);
  ret += ":";
  ret += buffer;

  return ret;
}

// Dwarf_line_info routines.

static unsigned int next_generation_count = 0;

struct Addr2line_cache_entry
{
  Object* object;
  unsigned int shndx;
  Dwarf_line_info* dwarf_line_info;
  unsigned int generation_count;
  unsigned int access_count;

  Addr2line_cache_entry(Object* o, unsigned int s, Dwarf_line_info* d)
      : object(o), shndx(s), dwarf_line_info(d),
        generation_count(next_generation_count), access_count(0)
  {
    if (next_generation_count < (1U << 31))
      ++next_generation_count;
  }
};
// We expect this cache to be small, so don't bother with a hashtable
// or priority queue or anything: just use a simple vector.
static std::vector<Addr2line_cache_entry> addr2line_cache;

std::string
Dwarf_line_info::one_addr2line(Object* object,
                               unsigned int shndx, off_t offset,
                               size_t cache_size)
{
  Dwarf_line_info* lineinfo = NULL;
  std::vector<Addr2line_cache_entry>::iterator it;

  // First, check the cache.  If we hit, update the counts.
  for (it = addr2line_cache.begin(); it != addr2line_cache.end(); ++it)
    {
      if (it->object == object && it->shndx == shndx)
        {
          lineinfo = it->dwarf_line_info;
          it->generation_count = next_generation_count;
          // We cap generation_count at 2^31 -1 to avoid overflow.
          if (next_generation_count < (1U << 31))
            ++next_generation_count;
          // We cap access_count at 31 so 2^access_count doesn't overflow
          if (it->access_count < 31)
            ++it->access_count;
          break;
        }
    }

  // If we don't hit the cache, create a new object and insert into the
  // cache.
  if (lineinfo == NULL)
  {
    switch (parameters->size_and_endianness())
      {
#ifdef HAVE_TARGET_32_LITTLE
        case Parameters::TARGET_32_LITTLE:
          lineinfo = new Sized_dwarf_line_info<32, false>(object, shndx); break;
#endif
#ifdef HAVE_TARGET_32_BIG
        case Parameters::TARGET_32_BIG:
          lineinfo = new Sized_dwarf_line_info<32, true>(object, shndx); break;
#endif
#ifdef HAVE_TARGET_64_LITTLE
        case Parameters::TARGET_64_LITTLE:
          lineinfo = new Sized_dwarf_line_info<64, false>(object, shndx); break;
#endif
#ifdef HAVE_TARGET_64_BIG
        case Parameters::TARGET_64_BIG:
          lineinfo = new Sized_dwarf_line_info<64, true>(object, shndx); break;
#endif
        default:
          gold_unreachable();
      }
    addr2line_cache.push_back(Addr2line_cache_entry(object, shndx, lineinfo));
  }

  // Now that we have our object, figure out the answer
  std::string retval = lineinfo->addr2line(shndx, offset);

  // Finally, if our cache has grown too big, delete old objects.  We
  // assume the common (probably only) case is deleting only one object.
  // We use a pretty simple scheme to evict: function of LRU and MFU.
  while (addr2line_cache.size() > cache_size)
    {
      unsigned int lowest_score = ~0U;
      std::vector<Addr2line_cache_entry>::iterator lowest
          = addr2line_cache.end();
      for (it = addr2line_cache.begin(); it != addr2line_cache.end(); ++it)
        {
          const unsigned int score = (it->generation_count
                                      + (1U << it->access_count));
          if (score < lowest_score)
            {
              lowest_score = score;
              lowest = it;
            }
        }
      if (lowest != addr2line_cache.end())
        {
          delete lowest->dwarf_line_info;
          addr2line_cache.erase(lowest);
        }
    }

  return retval;
}

void
Dwarf_line_info::clear_addr2line_cache()
{
  for (std::vector<Addr2line_cache_entry>::iterator it = addr2line_cache.begin();
       it != addr2line_cache.end();
       ++it)
    delete it->dwarf_line_info;
  addr2line_cache.clear();
}

#ifdef HAVE_TARGET_32_LITTLE
template
class Sized_dwarf_line_info<32, false>;
#endif

#ifdef HAVE_TARGET_32_BIG
template
class Sized_dwarf_line_info<32, true>;
#endif

#ifdef HAVE_TARGET_64_LITTLE
template
class Sized_dwarf_line_info<64, false>;
#endif

#ifdef HAVE_TARGET_64_BIG
template
class Sized_dwarf_line_info<64, true>;
#endif

} // End namespace gold.
