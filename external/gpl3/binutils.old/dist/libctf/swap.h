/* Interface to byteswapping functions.
   Copyright (C) 2006-2020 Free Software Foundation, Inc.

   This file is part of libctf.

   libctf is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#ifndef _CTF_SWAP_H
#define _CTF_SWAP_H

#include "config.h"
#include <stdint.h>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#else

/* Provide our own versions of the byteswap functions.  */
static inline uint16_t
bswap_16 (uint16_t v)
{
  return ((v >> 8) & 0xff) | ((v & 0xff) << 8);
}

static inline uint32_t
bswap_32 (uint32_t v)
{
  return (  ((v & 0xff000000) >> 24)
	  | ((v & 0x00ff0000) >>  8)
	  | ((v & 0x0000ff00) <<  8)
	  | ((v & 0x000000ff) << 24));
}

inline uint64_t
bswap_identity_64 (uint64_t v)
{
  return v;
}

static inline uint64_t
bswap_64 (uint64_t v)
{
  return (  ((v & 0xff00000000000000ULL) >> 56)
	  | ((v & 0x00ff000000000000ULL) >> 40)
	  | ((v & 0x0000ff0000000000ULL) >> 24)
	  | ((v & 0x000000ff00000000ULL) >>  8)
	  | ((v & 0x00000000ff000000ULL) <<  8)
	  | ((v & 0x0000000000ff0000ULL) << 24)
	  | ((v & 0x000000000000ff00ULL) << 40)
	  | ((v & 0x00000000000000ffULL) << 56));
}
#endif /* !defined(HAVE_BYTESWAP_H) */

#endif /* !defined(_CTF_SWAP_H) */
