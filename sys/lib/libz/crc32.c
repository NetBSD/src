/* $NetBSD: crc32.c,v 1.6 2003/03/29 22:25:26 thorpej Exp $ */

/* crc32.c -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/* @(#) $Id: crc32.c,v 1.6 2003/03/29 22:25:26 thorpej Exp $ */

#include "zlib.h"

#define local static

local const uLongf crc_table[16] = {
  0x00000000L, 0x1db71064L, 0x3b6e20c8L, 0x26d930acL,
  0x76dc4190L, 0x6b6b51f4L, 0x4db26158L, 0x5005713cL,
  0xedb88320L, 0xf00f9344L, 0xd6d6a3e8L, 0xcb61b38cL,
  0x9b64c2b0L, 0x86d3d2d4L, 0xa00ae278L, 0xbdbdf21cL
};

/* ========================================================================= */
#define DO1(buf) do { crc ^= *buf++; crc = (crc >> 4) ^ crc_table[crc & 0xf]; crc = (crc >> 4) ^ crc_table[crc & 0xf]; } while (0)
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================= */
uLong ZEXPORT crc32(crc, buf, len)
    uLong crc;
    const Bytef *buf;
    uInt len;
{
    if (buf == Z_NULL) return 0L;
    crc = crc ^ 0xffffffffL;
    while (len >= 8)
    {
      DO8(buf);
      len -= 8;
    }
    if (len) do {
      DO1(buf);
    } while (--len);
    return crc ^ 0xffffffffL;
}
