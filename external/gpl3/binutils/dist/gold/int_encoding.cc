// varint.cc -- variable length and unaligned integer encoding support.

// Copyright 2009 Free Software Foundation, Inc.
// Written by Doug Kwan <dougkwan@google.com> by refactoring scattered
// contents from other files in gold.  Original code written by Ian
// Lance Taylor <iant@google.com> and Caleb Howe <cshowe@google.com>.

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

#include <vector>

#include "int_encoding.h"

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

void
write_unsigned_LEB_128(std::vector<unsigned char>* buffer, uint64_t value)
{
  do
    {
      unsigned char current_byte = value & 0x7f;
      value >>= 7;
      if (value != 0)
        {
          current_byte |= 0x80;
        }
      buffer->push_back(current_byte);
    }
  while (value != 0);
}

size_t
get_length_as_unsigned_LEB_128(uint64_t value)
{
  size_t length = 0;
  do
    {
      unsigned char current_byte = value & 0x7f;
      value >>= 7;
      if (value != 0)
        {
          current_byte |= 0x80;
        }
      length++;
    }
  while (value != 0);
  return length;
}

} // End namespace gold.
