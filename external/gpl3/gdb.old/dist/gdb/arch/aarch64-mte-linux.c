/* Common Linux target-dependent functionality for AArch64 MTE

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "arch/aarch64-mte-linux.h"

/* See arch/aarch64-mte-linux.h */

void
aarch64_mte_pack_tags (gdb::byte_vector &tags)
{
  /* Nothing to pack?  */
  if (tags.empty ())
    return;

  /* If the tags vector has an odd number of elements, add another
     zeroed-out element to make it even.  This facilitates packing.  */
  if ((tags.size () % 2) != 0)
    tags.emplace_back (0);

  for (int unpacked = 0, packed = 0; unpacked < tags.size ();
       unpacked += 2, packed++)
    tags[packed] = (tags[unpacked + 1] << 4) | tags[unpacked];

  /* Now we have half the size.  */
  tags.resize (tags.size () / 2);
}

/* See arch/aarch64-mte-linux.h */

void
aarch64_mte_unpack_tags (gdb::byte_vector &tags, bool skip_first)
{
  /* Nothing to unpack?  */
  if (tags.empty ())
    return;

  /* An unpacked MTE tags vector will have twice the number of elements
     compared to an unpacked one.  */
  gdb::byte_vector unpacked_tags (tags.size () * 2);

  int unpacked = 0, packed = 0;

  if (skip_first)
    {
      /* We are not interested in the first unpacked element, just discard
	 it.  */
      unpacked_tags[unpacked] = (tags[packed] >> 4) & 0xf;
      unpacked++;
      packed++;
    }

  for (; packed < tags.size (); unpacked += 2, packed++)
    {
      unpacked_tags[unpacked] = tags[packed] & 0xf;
      unpacked_tags[unpacked + 1] = (tags[packed] >> 4) & 0xf;
    }

  /* Update the original tags vector.  */
  tags = std::move (unpacked_tags);
}

