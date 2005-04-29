/* linebreak.c - line breaking of Unicode strings
   Copyright (C) 2001-2003 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2001.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "linebreak.h"

#include <stdlib.h>
#include <string.h>
#include "c-ctype.h"
#include "xsize.h"

#include "utf8-ucs4.h"

#ifdef unused
#include "utf16-ucs4.h"

static inline int
u32_mbtouc (unsigned int *puc, const unsigned int *s, size_t n)
{
  *puc = *s;
  return 1;
}
#endif


/* Help GCC to generate good code for string comparisons with
   immediate strings. */
#if defined (__GNUC__) && defined (__OPTIMIZE__)

static inline int
streq9 (const char *s1, const char *s2)
{
  return strcmp (s1 + 9, s2 + 9) == 0;
}

static inline int
streq8 (const char *s1, const char *s2, char s28)
{
  if (s1[8] == s28)
    {
      if (s28 == 0)
        return 1;
      else
        return streq9 (s1, s2);
    }
  else
    return 0;
}

static inline int
streq7 (const char *s1, const char *s2, char s27, char s28)
{
  if (s1[7] == s27)
    {
      if (s27 == 0)
        return 1;
      else
        return streq8 (s1, s2, s28);
    }
  else
    return 0;
}

static inline int
streq6 (const char *s1, const char *s2, char s26, char s27, char s28)
{
  if (s1[6] == s26)
    {
      if (s26 == 0)
        return 1;
      else
        return streq7 (s1, s2, s27, s28);
    }
  else
    return 0;
}

static inline int
streq5 (const char *s1, const char *s2, char s25, char s26, char s27, char s28)
{
  if (s1[5] == s25)
    {
      if (s25 == 0)
        return 1;
      else
        return streq6 (s1, s2, s26, s27, s28);
    }
  else
    return 0;
}

static inline int
streq4 (const char *s1, const char *s2, char s24, char s25, char s26, char s27, char s28)
{
  if (s1[4] == s24)
    {
      if (s24 == 0)
        return 1;
      else
        return streq5 (s1, s2, s25, s26, s27, s28);
    }
  else
    return 0;
}

static inline int
streq3 (const char *s1, const char *s2, char s23, char s24, char s25, char s26, char s27, char s28)
{
  if (s1[3] == s23)
    {
      if (s23 == 0)
        return 1;
      else
        return streq4 (s1, s2, s24, s25, s26, s27, s28);
    }
  else
    return 0;
}

static inline int
streq2 (const char *s1, const char *s2, char s22, char s23, char s24, char s25, char s26, char s27, char s28)
{
  if (s1[2] == s22)
    {
      if (s22 == 0)
        return 1;
      else
        return streq3 (s1, s2, s23, s24, s25, s26, s27, s28);
    }
  else
    return 0;
}

static inline int
streq1 (const char *s1, const char *s2, char s21, char s22, char s23, char s24, char s25, char s26, char s27, char s28)
{
  if (s1[1] == s21)
    {
      if (s21 == 0)
        return 1;
      else
        return streq2 (s1, s2, s22, s23, s24, s25, s26, s27, s28);
    }
  else
    return 0;
}

static inline int
streq0 (const char *s1, const char *s2, char s20, char s21, char s22, char s23, char s24, char s25, char s26, char s27, char s28)
{
  if (s1[0] == s20)
    {
      if (s20 == 0)
        return 1;
      else
        return streq1 (s1, s2, s21, s22, s23, s24, s25, s26, s27, s28);
    }
  else
    return 0;
}

#define STREQ(s1,s2,s20,s21,s22,s23,s24,s25,s26,s27,s28) \
  streq0 (s1, s2, s20, s21, s22, s23, s24, s25, s26, s27, s28)

#else

#define STREQ(s1,s2,s20,s21,s22,s23,s24,s25,s26,s27,s28) \
  (strcmp (s1, s2) == 0)

#endif


static int
is_cjk_encoding (const char *encoding)
{
  if (0
      /* Legacy Japanese encodings */
      || STREQ (encoding, "EUC-JP", 'E', 'U', 'C', '-', 'J', 'P', 0, 0, 0)
      /* Legacy Chinese encodings */
      || STREQ (encoding, "GB2312", 'G', 'B', '2', '3', '1', '2', 0, 0, 0)
      || STREQ (encoding, "GBK", 'G', 'B', 'K', 0, 0, 0, 0, 0, 0)
      || STREQ (encoding, "EUC-TW", 'E', 'U', 'C', '-', 'T', 'W', 0, 0, 0)
      || STREQ (encoding, "BIG5", 'B', 'I', 'G', '5', 0, 0, 0, 0, 0)
      /* Legacy Korean encodings */
      || STREQ (encoding, "EUC-KR", 'E', 'U', 'C', '-', 'K', 'R', 0, 0, 0)
      || STREQ (encoding, "CP949", 'C', 'P', '9', '4', '9', 0, 0, 0, 0)
      || STREQ (encoding, "JOHAB", 'J', 'O', 'H', 'A', 'B', 0, 0, 0, 0))
    return 1;
  return 0;
}

static int
is_utf8_encoding (const char *encoding)
{
  if (STREQ (encoding, "UTF-8", 'U', 'T', 'F', '-', '8', 0, 0, 0 ,0))
    return 1;
  return 0;
}


/* Determine number of column positions required for UC. */
int uc_width (unsigned int uc, const char *encoding);

/*
 * Non-spacing attribute table.
 * Consists of:
 * - Non-spacing characters; generated from PropList.txt or
 *   "grep '^[^;]*;[^;]*;[^;]*;[^;]*;NSM;' UnicodeData.txt"
 * - Format control characters; generated from
 *   "grep '^[^;]*;[^;]*;Cf;' UnicodeData.txt"
 * - Zero width characters; generated from
 *   "grep '^[^;]*;ZERO WIDTH ' UnicodeData.txt"
 */
static const unsigned char nonspacing_table_data[16*64] = {
  /* 0x0000-0x01ff */
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, /* 0x0000-0x003f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, /* 0x0040-0x007f */
  0xff, 0xff, 0xff, 0xff, 0x00, 0x20, 0x00, 0x00, /* 0x0080-0x00bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x00c0-0x00ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0100-0x013f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0140-0x017f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0180-0x01bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x01c0-0x01ff */
  /* 0x0200-0x03ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0200-0x023f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0240-0x027f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0280-0x02bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x02c0-0x02ff */
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 0x0300-0x033f */
  0xff, 0xff, 0xff, 0xe0, 0xff, 0xff, 0x00, 0x00, /* 0x0340-0x037f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0380-0x03bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x03c0-0x03ff */
  /* 0x0400-0x05ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0400-0x043f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0440-0x047f */
  0x78, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0480-0x04bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x04c0-0x04ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0500-0x053f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0540-0x057f */
  0x00, 0x00, 0xfe, 0xff, 0xfb, 0xff, 0xff, 0xbb, /* 0x0580-0x05bf */
  0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x05c0-0x05ff */
  /* 0x0600-0x07ff */
  0x0f, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0600-0x063f */
  0x00, 0xf8, 0xff, 0x01, 0x00, 0x00, 0x01, 0x00, /* 0x0640-0x067f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0680-0x06bf */
  0x00, 0x00, 0xc0, 0xff, 0x9f, 0x3d, 0x00, 0x00, /* 0x06c0-0x06ff */
  0x00, 0x80, 0x02, 0x00, 0x00, 0x00, 0xff, 0xff, /* 0x0700-0x073f */
  0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0740-0x077f */
  0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x01, 0x00, /* 0x0780-0x07bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x07c0-0x07ff */
  /* 0x0800-0x09ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0800-0x083f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0840-0x087f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0880-0x08bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x08c0-0x08ff */
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0900-0x093f */
  0xfe, 0x21, 0x1e, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x0940-0x097f */
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0980-0x09bf */
  0x1e, 0x20, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x09c0-0x09ff */
  /* 0x0a00-0x0bff */
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0a00-0x0a3f */
  0x86, 0x39, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, /* 0x0a40-0x0a7f */
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0a80-0x0abf */
  0xbe, 0x21, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x0ac0-0x0aff */
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, /* 0x0b00-0x0b3f */
  0x0e, 0x20, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0b40-0x0b7f */
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0b80-0x0bbf */
  0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0bc0-0x0bff */
  /* 0x0c00-0x0dff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, /* 0x0c00-0x0c3f */
  0xc1, 0x3d, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0c40-0x0c7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0c80-0x0cbf */
  0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0cc0-0x0cff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0d00-0x0d3f */
  0x0e, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0d40-0x0d7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0d80-0x0dbf */
  0x00, 0x04, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0dc0-0x0dff */
  /* 0x0e00-0x0fff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0x07, /* 0x0e00-0x0e3f */
  0x80, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0e40-0x0e7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0x1b, /* 0x0e80-0x0ebf */
  0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0ec0-0x0eff */
  0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0xa0, 0x02, /* 0x0f00-0x0f3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x7f, /* 0x0f40-0x0f7f */
  0xdf, 0x00, 0xff, 0xfe, 0xff, 0xff, 0xff, 0x1f, /* 0x0f80-0x0fbf */
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0fc0-0x0fff */
  /* 0x1000-0x11ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xc5, 0x02, /* 0x1000-0x103f */
  0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, /* 0x1040-0x107f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1080-0x10bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10c0-0x10ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1100-0x113f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1140-0x117f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1180-0x11bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11c0-0x11ff */
  /* 0x1600-0x17ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1600-0x163f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1640-0x167f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1680-0x16bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16c0-0x16ff */
  0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x00, /* 0x1700-0x173f */
  0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, /* 0x1740-0x177f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x3f, /* 0x1780-0x17bf */
  0x40, 0xfe, 0x0f, 0x20, 0x00, 0x00, 0x00, 0x00, /* 0x17c0-0x17ff */
  /* 0x1800-0x19ff */
  0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1800-0x183f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1840-0x187f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, /* 0x1880-0x18bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x18c0-0x18ff */
  0x00, 0x00, 0x00, 0x00, 0x87, 0x0f, 0x04, 0x0e, /* 0x1900-0x193f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1940-0x197f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1980-0x19bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x19c0-0x19ff */
  /* 0x2000-0x21ff */
  0x00, 0xf8, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, /* 0x2000-0x203f */
  0x00, 0x00, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, /* 0x2040-0x207f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2080-0x20bf */
  0x00, 0x00, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, /* 0x20c0-0x20ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2100-0x213f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2140-0x217f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2180-0x21bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x21c0-0x21ff */
  /* 0x3000-0x31ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, /* 0x3000-0x303f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x3040-0x307f */
  0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, /* 0x3080-0x30bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30c0-0x30ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x3100-0x313f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x3140-0x317f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x3180-0x31bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x31c0-0x31ff */
  /* 0xfa00-0xfbff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfa00-0xfa3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfa40-0xfa7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfa80-0xfabf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfac0-0xfaff */
  0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* 0xfb00-0xfb3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfb40-0xfb7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfb80-0xfbbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfbc0-0xfbff */
  /* 0xfe00-0xffff */
  0xff, 0xff, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, /* 0xfe00-0xfe3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfe40-0xfe7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfe80-0xfebf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, /* 0xfec0-0xfeff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xff00-0xff3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xff40-0xff7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xff80-0xffbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, /* 0xffc0-0xffff */
  /* 0x1d000-0x1d1ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d000-0x1d03f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d040-0x1d07f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d080-0x1d0bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d0c0-0x1d0ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d100-0x1d13f */
  0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0xf8, /* 0x1d140-0x1d17f */
  0xe7, 0x0f, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, /* 0x1d180-0x1d1bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* 0x1d1c0-0x1d1ff */
};
static const signed char nonspacing_table_ind[240] = {
   0,  1,  2,  3,  4,  5,  6,  7, /* 0x0000-0x0fff */
   8, -1, -1,  9, 10, -1, -1, -1, /* 0x1000-0x1fff */
  11, -1, -1, -1, -1, -1, -1, -1, /* 0x2000-0x2fff */
  12, -1, -1, -1, -1, -1, -1, -1, /* 0x3000-0x3fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x4000-0x4fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x5000-0x5fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x6000-0x6fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x7000-0x7fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x8000-0x8fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x9000-0x9fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xa000-0xafff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xb000-0xbfff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xc000-0xcfff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xd000-0xdfff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xe000-0xefff */
  -1, -1, -1, -1, -1, 13, -1, 14, /* 0xf000-0xffff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10000-0x10fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x11000-0x11fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x12000-0x12fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x13000-0x13fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x14000-0x14fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x15000-0x15fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x16000-0x16fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x17000-0x17fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x18000-0x18fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x19000-0x19fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x1a000-0x1afff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x1b000-0x1bfff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x1c000-0x1cfff */
  15, -1, -1, -1, -1, -1, -1, -1  /* 0x1d000-0x1dfff */
};

/* Determine number of column positions required for UC. */
int
uc_width (unsigned int uc, const char *encoding)
{
  /* Test for non-spacing or control character.  */
  if ((uc >> 9) < 240)
    {
      int ind = nonspacing_table_ind[uc >> 9];
      if (ind >= 0)
	if ((nonspacing_table_data[64*ind + ((uc >> 3) & 63)] >> (uc & 7)) & 1)
	  {
	    if (uc > 0 && uc < 0xa0)
	      return -1;
	    else
	      return 0;
	  }
    }
  else if ((uc >> 9) == (0xe0000 >> 9))
    {
      if (uc < 0xe0100
	  ? (uc >= 0xe0020 ? uc <= 0xe007f : uc == 0xe0001)
	  : (uc <= 0xe01ef))
	return 0;
    }
  /* Test for double-width character.
   * Generated from "grep '^....;[WF]' EastAsianWidth.txt"
   * and            "grep '^....;[^WF]' EastAsianWidth.txt"
   */
  if (uc >= 0x1100
      && ((uc < 0x1160) /* Hangul Jamo */
	  || (uc >= 0x2e80 && uc < 0x4dc0  /* CJK */
	      && !(uc == 0x303f))
	  || (uc >= 0x4e00 && uc < 0xa4d0) /* CJK ... Yi */
	  || (uc >= 0xac00 && uc < 0xd7a4) /* Hangul Syllables */
	  || (uc >= 0xf900 && uc < 0xfb00) /* CJK Compatibility Ideographs */
	  || (uc >= 0xfe30 && uc < 0xfe70) /* CJK Compatibility Forms */
	  || (uc >= 0xff00 && uc < 0xff61) /* Fullwidth Forms */
	  || (uc >= 0xffe0 && uc < 0xffe7)
	  || (uc >= 0x20000 && uc <= 0x2fffd) /* CJK, CJK Compatibility Ideographs */
	  || (uc >= 0x30000 && uc <= 0x3fffd)
     )   )
    return 2;
  /* In ancient CJK encodings, Cyrillic and most other characters are
     double-width as well.  */
  if (uc >= 0x00A1 && uc < 0xFF61 && uc != 0x20A9
      && is_cjk_encoding (encoding))
    return 2;
  return 1;
}


#ifdef unused

/* Determine number of column positions required for first N units
   (or fewer if S ends before this) in S.  */

int
u8_width (const unsigned char *s, size_t n, const char *encoding)
{
  const unsigned char *s_end = s + n;
  int width = 0;

  while (s < s_end)
    {
      unsigned int uc;
      int w;

      s += u8_mbtouc (&uc, s, s_end - s);

      if (uc == 0)
        break; /* end of string reached */

      w = uc_width (uc, encoding);
      if (w >= 0) /* ignore control characters in the string */
        width += w;
    }

  return width;
}

int
u16_width (const unsigned short *s, size_t n, const char *encoding)
{
  const unsigned short *s_end = s + n;
  int width = 0;

  while (s < s_end)
    {
      unsigned int uc;
      int w;

      s += u16_mbtouc (&uc, s, s_end - s);

      if (uc == 0)
        break; /* end of string reached */

      w = uc_width (uc, encoding);
      if (w >= 0) /* ignore control characters in the string */
        width += w;
    }

  return width;
}

int
u32_width (const unsigned int *s, size_t n, const char *encoding)
{
  const unsigned int *s_end = s + n;
  int width = 0;

  while (s < s_end)
    {
      unsigned int uc = *s++;
      int w;

      if (uc == 0)
        break; /* end of string reached */

      w = uc_width (uc, encoding);
      if (w >= 0) /* ignore control characters in the string */
        width += w;
    }

  return width;
}

#endif


/* Determine the line break points in S, and store the result at p[0..n-1].  */
/* We don't support line breaking of complex-context dependent characters
   (Thai, Lao, Myanmar, Khmer) yet, because it requires dictionary lookup. */

/* Line breaking classification.  */

enum
{
  /* Values >= 20 are resolved at run time. */
  LBP_BK =  0, /* mandatory break */
/*LBP_CR,         carriage return - not used here because it's a DOSism */
/*LBP_LF,         line feed - not used here because it's a DOSism */
  LBP_CM = 20, /* attached characters and combining marks */
/*LBP_SG,         surrogates - not used here because they are not characters */
  LBP_ZW =  1, /* zero width space */
  LBP_IN =  2, /* inseparable */
  LBP_GL =  3, /* non-breaking (glue) */
  LBP_CB = 22, /* contingent break opportunity */
  LBP_SP = 21, /* space */
  LBP_BA =  4, /* break opportunity after */
  LBP_BB =  5, /* break opportunity before */
  LBP_B2 =  6, /* break opportunity before and after */
  LBP_HY =  7, /* hyphen */
  LBP_NS =  8, /* non starter */
  LBP_OP =  9, /* opening punctuation */
  LBP_CL = 10, /* closing punctuation */
  LBP_QU = 11, /* ambiguous quotation */
  LBP_EX = 12, /* exclamation/interrogation */
  LBP_ID = 13, /* ideographic */
  LBP_NU = 14, /* numeric */
  LBP_IS = 15, /* infix separator (numeric) */
  LBP_SY = 16, /* symbols allowing breaks */
  LBP_AL = 17, /* ordinary alphabetic and symbol characters */
  LBP_PR = 18, /* prefix (numeric) */
  LBP_PO = 19, /* postfix (numeric) */
  LBP_SA = 23, /* complex context (South East Asian) */
  LBP_AI = 24, /* ambiguous (alphabetic or ideograph) */
  LBP_XX = 25  /* unknown */
};

#include "lbrkprop.h"

static inline unsigned char
lbrkprop_lookup (unsigned int uc)
{
  unsigned int index1 = uc >> lbrkprop_header_0;
  if (index1 < lbrkprop_header_1)
    {
      int lookup1 = lbrkprop.level1[index1];
      if (lookup1 >= 0)
        {
          unsigned int index2 = (uc >> lbrkprop_header_2) & lbrkprop_header_3;
          int lookup2 = lbrkprop.level2[lookup1 + index2];
          if (lookup2 >= 0)
            {
              unsigned int index3 = uc & lbrkprop_header_4;
              return lbrkprop.level3[lookup2 + index3];
            }
        }
    }
  return LBP_XX;
}

/* Table indexed by two line breaking classifications.  */
#define D 1  /* direct break opportunity, empty in table 7.3 of UTR #14 */
#define I 2  /* indirect break opportunity, '%' in table 7.3 of UTR #14 */
#define P 3  /* prohibited break,           '^' in table 7.3 of UTR #14 */
static const unsigned char lbrk_table[19][19] = {
                                /* after */
        /* ZW IN GL BA BB B2 HY NS OP CL QU EX ID NU IS SY AL PR PO */
/* ZW */ { P, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, D, },
/* IN */ { P, I, I, I, D, D, I, I, D, P, I, P, D, D, P, P, D, D, D, },
/* GL */ { P, I, I, I, I, I, I, I, I, P, I, P, I, I, P, P, I, I, I, },
/* BA */ { P, D, I, I, D, D, I, I, D, P, I, P, D, D, P, P, D, D, D, },
/* BB */ { P, I, I, I, I, I, I, I, I, P, I, P, I, I, P, P, I, I, I, },
/* B2 */ { P, D, I, I, D, P, I, I, D, P, I, P, D, D, P, P, D, D, D, },
/* HY */ { P, D, I, I, D, D, I, I, D, P, I, P, D, D, P, P, D, D, D, },
/* NS */ { P, D, I, I, D, D, I, I, D, P, I, P, D, D, P, P, D, D, D, },
/* OP */ { P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, P, },
/* CL */ { P, D, I, I, D, D, I, P, D, P, I, P, D, D, P, P, D, D, I, },
/* QU */ { P, I, I, I, I, I, I, I, P, P, I, P, I, I, P, P, I, I, I, },
/* EX */ { P, D, I, I, D, D, I, I, D, P, I, P, D, D, P, P, D, D, D, },
/* ID */ { P, I, I, I, D, D, I, I, D, P, I, P, D, D, P, P, D, D, I, },
/* NU */ { P, I, I, I, D, D, I, I, D, P, I, P, D, I, P, P, I, D, I, },
/* IS */ { P, D, I, I, D, D, I, I, D, P, I, P, D, I, P, P, D, D, D, },
/* SY */ { P, D, I, I, D, D, I, I, D, P, I, P, D, I, P, P, D, D, D, },
/* AL */ { P, I, I, I, D, D, I, I, D, P, I, P, D, I, P, P, I, D, D, },
/* PR */ { P, D, I, I, D, D, I, I, I, P, I, P, I, I, P, P, I, D, D, },
/* PO */ { P, D, I, I, D, D, I, I, D, P, I, P, D, D, P, P, D, D, D, },
/* "" */
/* before */
};
/* Note: The (B2,B2) entry should probably be D instead of P.  */
/* Note: The (PR,ID) entry should probably be D instead of I.  */

void
u8_possible_linebreaks (const unsigned char *s, size_t n, const char *encoding, char *p)
{
  int LBP_AI_REPLACEMENT = (is_cjk_encoding (encoding) ? LBP_ID : LBP_AL);
  const unsigned char *s_end = s + n;
  int last_prop = LBP_BK; /* line break property of last non-space character */
  char *seen_space = NULL; /* Was a space seen after the last non-space character? */
  char *seen_space2 = NULL; /* At least two spaces after the last non-space? */

  /* Don't break inside multibyte characters.  */
  memset (p, UC_BREAK_PROHIBITED, n);

  while (s < s_end)
    {
      unsigned int uc;
      int count = u8_mbtouc (&uc, s, s_end - s);
      int prop = lbrkprop_lookup (uc);

      if (prop == LBP_BK)
        {
          /* Mandatory break.  */
          *p = UC_BREAK_MANDATORY;
          last_prop = LBP_BK;
          seen_space = NULL;
          seen_space2 = NULL;
        }
      else
        {
          char *q;

          /* Resolve property values whose behaviour is not fixed.  */
          switch (prop)
            {
              case LBP_AI:
                /* Resolve ambiguous.  */
                prop = LBP_AI_REPLACEMENT;
                break;
              case LBP_CB:
                /* This is arbitrary.  */
                prop = LBP_ID;
                break;
              case LBP_SA:
                /* We don't handle complex scripts yet.
                   Treat LBP_SA like LBP_XX.  */
              case LBP_XX:
                /* This is arbitrary.  */
                prop = LBP_AL;
                break;
            }

          /* Deal with combining characters.  */
          q = p;
          if (prop == LBP_CM)
            {
              /* Don't break just before a combining character.  */
              *p = UC_BREAK_PROHIBITED;
              /* A combining character turns a preceding space into LBP_AL.  */
              if (seen_space != NULL)
                {
                  q = seen_space;
                  seen_space = seen_space2;
                  prop = LBP_AL;
                  goto lookup_via_table;
                }
            }
          else if (prop == LBP_SP)
            {
              /* Don't break just before a space.  */
              *p = UC_BREAK_PROHIBITED;
              seen_space2 = seen_space;
              seen_space = p;
            }
          else
            {
             lookup_via_table:
              /* prop must be usable as an index for table 7.3 of UTR #14.  */
              if (!(prop >= 1 && prop <= sizeof(lbrk_table) / sizeof(lbrk_table[0])))
                abort ();

              if (last_prop == LBP_BK)
                {
                  /* Don't break at the beginning of a line.  */
                  *q = UC_BREAK_PROHIBITED;
                }
              else
                {
                  switch (lbrk_table [last_prop-1] [prop-1])
                    {
                      case D:
                        *q = UC_BREAK_POSSIBLE;
                        break;
                      case I:
                        *q = (seen_space != NULL ? UC_BREAK_POSSIBLE : UC_BREAK_PROHIBITED);
                        break;
                      case P:
                        *q = UC_BREAK_PROHIBITED;
                        break;
                      default:
                        abort ();
                    }
                }
              last_prop = prop;
              seen_space = NULL;
              seen_space2 = NULL;
            }
        }

      s += count;
      p += count;
    }
}

#ifdef unused

void
u16_possible_linebreaks (const unsigned short *s, size_t n, const char *encoding, char *p)
{
  int LBP_AI_REPLACEMENT = (is_cjk_encoding (encoding) ? LBP_ID : LBP_AL);
  const unsigned short *s_end = s + n;
  int last_prop = LBP_BK; /* line break property of last non-space character */
  char *seen_space = NULL; /* Was a space seen after the last non-space character? */
  char *seen_space2 = NULL; /* At least two spaces after the last non-space? */

  /* Don't break inside multibyte characters.  */
  memset (p, UC_BREAK_PROHIBITED, n);

  while (s < s_end)
    {
      unsigned int uc;
      int count = u16_mbtouc (&uc, s, s_end - s);
      int prop = lbrkprop_lookup (uc);

      if (prop == LBP_BK)
        {
          /* Mandatory break.  */
          *p = UC_BREAK_MANDATORY;
          last_prop = LBP_BK;
          seen_space = NULL;
          seen_space2 = NULL;
        }
      else
        {
          char *q;

          /* Resolve property values whose behaviour is not fixed.  */
          switch (prop)
            {
              case LBP_AI:
                /* Resolve ambiguous.  */
                prop = LBP_AI_REPLACEMENT;
                break;
              case LBP_CB:
                /* This is arbitrary.  */
                prop = LBP_ID;
                break;
              case LBP_SA:
                /* We don't handle complex scripts yet.
                   Treat LBP_SA like LBP_XX.  */
              case LBP_XX:
                /* This is arbitrary.  */
                prop = LBP_AL;
                break;
            }

          /* Deal with combining characters.  */
          q = p;
          if (prop == LBP_CM)
            {
              /* Don't break just before a combining character.  */
              *p = UC_BREAK_PROHIBITED;
              /* A combining character turns a preceding space into LBP_AL.  */
              if (seen_space != NULL)
                {
                  q = seen_space;
                  seen_space = seen_space2;
                  prop = LBP_AL;
                  goto lookup_via_table;
                }
            }
          else if (prop == LBP_SP)
            {
              /* Don't break just before a space.  */
              *p = UC_BREAK_PROHIBITED;
              seen_space2 = seen_space;
              seen_space = p;
            }
          else
            {
             lookup_via_table:
              /* prop must be usable as an index for table 7.3 of UTR #14.  */
              if (!(prop >= 1 && prop <= sizeof(lbrk_table) / sizeof(lbrk_table[0])))
                abort ();

              if (last_prop == LBP_BK)
                {
                  /* Don't break at the beginning of a line.  */
                  *q = UC_BREAK_PROHIBITED;
                }
              else
                {
                  switch (lbrk_table [last_prop-1] [prop-1])
                    {
                      case D:
                        *q = UC_BREAK_POSSIBLE;
                        break;
                      case I:
                        *q = (seen_space != NULL ? UC_BREAK_POSSIBLE : UC_BREAK_PROHIBITED);
                        break;
                      case P:
                        *q = UC_BREAK_PROHIBITED;
                        break;
                      default:
                        abort ();
                    }
                }
              last_prop = prop;
              seen_space = NULL;
              seen_space2 = NULL;
            }
        }

      s += count;
      p += count;
    }
}

void
u32_possible_linebreaks (const unsigned int *s, size_t n, const char *encoding, char *p)
{
  int LBP_AI_REPLACEMENT = (is_cjk_encoding (encoding) ? LBP_ID : LBP_AL);
  const unsigned int *s_end = s + n;
  int last_prop = LBP_BK; /* line break property of last non-space character */
  char *seen_space = NULL; /* Was a space seen after the last non-space character? */
  char *seen_space2 = NULL; /* At least two spaces after the last non-space? */

  while (s < s_end)
    {
      unsigned int uc = *s;
      int prop = lbrkprop_lookup (uc);

      if (prop == LBP_BK)
        {
          /* Mandatory break.  */
          *p = UC_BREAK_MANDATORY;
          last_prop = LBP_BK;
          seen_space = NULL;
          seen_space2 = NULL;
        }
      else
        {
          char *q;

          /* Resolve property values whose behaviour is not fixed.  */
          switch (prop)
            {
              case LBP_AI:
                /* Resolve ambiguous.  */
                prop = LBP_AI_REPLACEMENT;
                break;
              case LBP_CB:
                /* This is arbitrary.  */
                prop = LBP_ID;
                break;
              case LBP_SA:
                /* We don't handle complex scripts yet.
                   Treat LBP_SA like LBP_XX.  */
              case LBP_XX:
                /* This is arbitrary.  */
                prop = LBP_AL;
                break;
            }

          /* Deal with combining characters.  */
          q = p;
          if (prop == LBP_CM)
            {
              /* Don't break just before a combining character.  */
              *p = UC_BREAK_PROHIBITED;
              /* A combining character turns a preceding space into LBP_AL.  */
              if (seen_space != NULL)
                {
                  q = seen_space;
                  seen_space = seen_space2;
                  prop = LBP_AL;
                  goto lookup_via_table;
                }
            }
          else if (prop == LBP_SP)
            {
              /* Don't break just before a space.  */
              *p = UC_BREAK_PROHIBITED;
              seen_space2 = seen_space;
              seen_space = p;
            }
          else
            {
             lookup_via_table:
              /* prop must be usable as an index for table 7.3 of UTR #14.  */
              if (!(prop >= 1 && prop <= sizeof(lbrk_table) / sizeof(lbrk_table[0])))
                abort ();

              if (last_prop == LBP_BK)
                {
                  /* Don't break at the beginning of a line.  */
                  *q = UC_BREAK_PROHIBITED;
                }
              else
                {
                  switch (lbrk_table [last_prop-1] [prop-1])
                    {
                      case D:
                        *q = UC_BREAK_POSSIBLE;
                        break;
                      case I:
                        *q = (seen_space != NULL ? UC_BREAK_POSSIBLE : UC_BREAK_PROHIBITED);
                        break;
                      case P:
                        *q = UC_BREAK_PROHIBITED;
                        break;
                      default:
                        abort ();
                    }
                }
              last_prop = prop;
              seen_space = NULL;
              seen_space2 = NULL;
            }
        }

      s++;
      p++;
    }
}

#endif


/* Choose the best line breaks, assuming the uc_width function.
   Return the column after the end of the string.  */

int
u8_width_linebreaks (const unsigned char *s, size_t n,
                     int width, int start_column, int at_end_columns,
                     const char *o, const char *encoding,
                     char *p)
{
  const unsigned char *s_end;
  char *last_p;
  int last_column;
  int piece_width;

  u8_possible_linebreaks (s, n, encoding, p);

  s_end = s + n;
  last_p = NULL;
  last_column = start_column;
  piece_width = 0;
  while (s < s_end)
    {
      unsigned int uc;
      int count = u8_mbtouc (&uc, s, s_end - s);

      /* Respect the override.  */
      if (o != NULL && *o != UC_BREAK_UNDEFINED)
        *p = *o;

      if (*p == UC_BREAK_POSSIBLE || *p == UC_BREAK_MANDATORY)
        {
          /* An atomic piece of text ends here.  */
          if (last_p != NULL && last_column + piece_width > width)
            {
              /* Insert a line break.  */
              *last_p = UC_BREAK_POSSIBLE;
              last_column = 0;
            }
        }

      if (*p == UC_BREAK_MANDATORY)
        {
          /* uc is a line break character.  */
          /* Start a new piece at column 0.  */
          last_p = NULL;
          last_column = 0;
          piece_width = 0;
        }
      else
        {
          /* uc is not a line break character.  */
          int w;

          if (*p == UC_BREAK_POSSIBLE)
            {
              /* Start a new piece.  */
              last_p = p;
              last_column += piece_width;
              piece_width = 0;
              /* No line break for the moment, may be turned into
                 UC_BREAK_POSSIBLE later, via last_p. */
            }

          *p = UC_BREAK_PROHIBITED;

          w = uc_width (uc, encoding);
          if (w >= 0) /* ignore control characters in the string */
            piece_width += w;
         }

      s += count;
      p += count;
      if (o != NULL)
        o += count;
    }

  /* The last atomic piece of text ends here.  */
  if (last_p != NULL && last_column + piece_width + at_end_columns > width)
    {
      /* Insert a line break.  */
      *last_p = UC_BREAK_POSSIBLE;
      last_column = 0;
    }

  return last_column + piece_width;
}

#ifdef unused

int
u16_width_linebreaks (const unsigned short *s, size_t n,
                      int width, int start_column, int at_end_columns,
                      const char *o, const char *encoding,
                      char *p)
{
  const unsigned short *s_end;
  char *last_p;
  int last_column;
  int piece_width;

  u16_possible_linebreaks (s, n, encoding, p);

  s_end = s + n;
  last_p = NULL;
  last_column = start_column;
  piece_width = 0;
  while (s < s_end)
    {
      unsigned int uc;
      int count = u16_mbtouc (&uc, s, s_end - s);

      /* Respect the override.  */
      if (o != NULL && *o != UC_BREAK_UNDEFINED)
        *p = *o;

      if (*p == UC_BREAK_POSSIBLE || *p == UC_BREAK_MANDATORY)
        {
          /* An atomic piece of text ends here.  */
          if (last_p != NULL && last_column + piece_width > width)
            {
              /* Insert a line break.  */
              *last_p = UC_BREAK_POSSIBLE;
              last_column = 0;
            }
        }

      if (*p == UC_BREAK_MANDATORY)
        {
          /* uc is a line break character.  */
          /* Start a new piece at column 0.  */
          last_p = NULL;
          last_column = 0;
          piece_width = 0;
        }
      else
        {
          /* uc is not a line break character.  */
          int w;

          if (*p == UC_BREAK_POSSIBLE)
            {
              /* Start a new piece.  */
              last_p = p;
              last_column += piece_width;
              piece_width = 0;
              /* No line break for the moment, may be turned into
                 UC_BREAK_POSSIBLE later, via last_p. */
            }

          *p = UC_BREAK_PROHIBITED;

          w = uc_width (uc, encoding);
          if (w >= 0) /* ignore control characters in the string */
            piece_width += w;
         }

      s += count;
      p += count;
      if (o != NULL)
        o += count;
    }

  /* The last atomic piece of text ends here.  */
  if (last_p != NULL && last_column + piece_width + at_end_columns > width)
    {
      /* Insert a line break.  */
      *last_p = UC_BREAK_POSSIBLE;
      last_column = 0;
    }

  return last_column + piece_width;
}

int
u32_width_linebreaks (const unsigned int *s, size_t n,
                      int width, int start_column, int at_end_columns,
                      const char *o, const char *encoding,
                      char *p)
{
  const unsigned int *s_end;
  char *last_p;
  int last_column;
  int piece_width;

  u32_possible_linebreaks (s, n, encoding, p);

  s_end = s + n;
  last_p = NULL;
  last_column = start_column;
  piece_width = 0;
  while (s < s_end)
    {
      unsigned int uc = *s;

      /* Respect the override.  */
      if (o != NULL && *o != UC_BREAK_UNDEFINED)
        *p = *o;

      if (*p == UC_BREAK_POSSIBLE || *p == UC_BREAK_MANDATORY)
        {
          /* An atomic piece of text ends here.  */
          if (last_p != NULL && last_column + piece_width > width)
            {
              /* Insert a line break.  */
              *last_p = UC_BREAK_POSSIBLE;
              last_column = 0;
            }
        }

      if (*p == UC_BREAK_MANDATORY)
        {
          /* uc is a line break character.  */
          /* Start a new piece at column 0.  */
          last_p = NULL;
          last_column = 0;
          piece_width = 0;
        }
      else
        {
          /* uc is not a line break character.  */
          int w;

          if (*p == UC_BREAK_POSSIBLE)
            {
              /* Start a new piece.  */
              last_p = p;
              last_column += piece_width;
              piece_width = 0;
              /* No line break for the moment, may be turned into
                 UC_BREAK_POSSIBLE later, via last_p. */
            }

          *p = UC_BREAK_PROHIBITED;

          w = uc_width (uc, encoding);
          if (w >= 0) /* ignore control characters in the string */
            piece_width += w;
         }

      s++;
      p++;
      if (o != NULL)
        o++;
    }

  /* The last atomic piece of text ends here.  */
  if (last_p != NULL && last_column + piece_width + at_end_columns > width)
    {
      /* Insert a line break.  */
      *last_p = UC_BREAK_POSSIBLE;
      last_column = 0;
    }

  return last_column + piece_width;
}

#endif


#ifdef TEST1

#include <stdio.h>

/* Read the contents of an input stream, and return it, terminated with a NUL
   byte. */
char *
read_file (FILE *stream)
{
#define BUFSIZE 4096
  char *buf = NULL;
  int alloc = 0;
  int size = 0;
  int count;

  while (! feof (stream))
    {
      if (size + BUFSIZE > alloc)
        {
          alloc = alloc + alloc / 2;
          if (alloc < size + BUFSIZE)
            alloc = size + BUFSIZE;
          buf = realloc (buf, alloc);
          if (buf == NULL)
            {
              fprintf (stderr, "out of memory\n");
              exit (1);
            }
        }
      count = fread (buf + size, 1, BUFSIZE, stream);
      if (count == 0)
        {
          if (ferror (stream))
            {
              perror ("fread");
              exit (1);
            }
        }
      else
        size += count;
    }
  buf = realloc (buf, size + 1);
  if (buf == NULL)
    {
      fprintf (stderr, "out of memory\n");
      exit (1);
    }
  buf[size] = '\0';
  return buf;
#undef BUFSIZE
}

int
main (int argc, char * argv[])
{
  if (argc == 1)
    {
      /* Display all the break opportunities in the input string.  */
      char *input = read_file (stdin);
      int length = strlen (input);
      char *breaks = malloc (length);
      int i;

      u8_possible_linebreaks ((unsigned char *) input, length, "UTF-8", breaks);

      for (i = 0; i < length; i++)
        {
          switch (breaks[i])
            {
              case UC_BREAK_POSSIBLE:
                /* U+2027 in UTF-8 encoding */
                putc (0xe2, stdout); putc (0x80, stdout); putc (0xa7, stdout);
                break;
              case UC_BREAK_MANDATORY:
                /* U+21B2 (or U+21B5) in UTF-8 encoding */
                putc (0xe2, stdout); putc (0x86, stdout); putc (0xb2, stdout);
                break;
              case UC_BREAK_PROHIBITED:
                break;
              default:
                abort ();
            }
          putc (input[i], stdout);
        }

      free (breaks);

      return 0;
    }
  else if (argc == 2)
    {
      /* Insert line breaks for a given width.  */
      int width = atoi (argv[1]);
      char *input = read_file (stdin);
      int length = strlen (input);
      char *breaks = malloc (length);
      int i;

      u8_width_linebreaks ((unsigned char *) input, length, width, 0, 0, NULL, "UTF-8", breaks);

      for (i = 0; i < length; i++)
        {
          switch (breaks[i])
            {
              case UC_BREAK_POSSIBLE:
                putc ('\n', stdout);
                break;
              case UC_BREAK_MANDATORY:
                break;
              case UC_BREAK_PROHIBITED:
                break;
              default:
                abort ();
            }
          putc (input[i], stdout);
        }

      free (breaks);

      return 0;
    }
  else
    return 1;
}

#endif /* TEST1 */


/* Now the same thing with an arbitrary encoding.

   We convert the input string to Unicode.

   The standardized Unicode encodings are UTF-8, UCS-2, UCS-4, UTF-16,
   UTF-16BE, UTF-16LE, UTF-7.  UCS-2 supports only characters up to
   \U0000FFFF.  UTF-16 and variants support only characters up to
   \U0010FFFF.  UTF-7 is way too complex and not supported by glibc-2.1.
   UCS-4 specification leaves doubts about endianness and byte order mark.
   glibc currently interprets it as big endian without byte order mark,
   but this is not backed by an RFC.  So we use UTF-8. It supports
   characters up to \U7FFFFFFF and is unambiguously defined.  */

#if HAVE_ICONV

#include <iconv.h>
#include <errno.h>

/* Luckily, the encoding's name is platform independent.  */
#define UTF8_NAME "UTF-8"

/* Return the length of a string after conversion through an iconv_t.  */
static size_t
iconv_string_length (iconv_t cd, const char *s, size_t n)
{
#define TMPBUFSIZE 4096
  size_t count = 0;
  char tmpbuf[TMPBUFSIZE];
  const char *inptr = s;
  size_t insize = n;
  while (insize > 0)
    {
      char *outptr = tmpbuf;
      size_t outsize = TMPBUFSIZE;
      size_t res = iconv (cd, (ICONV_CONST char **) &inptr, &insize, &outptr, &outsize);
      if (res == (size_t)(-1) && errno != E2BIG)
        return (size_t)(-1);
      count += outptr - tmpbuf;
    }
  /* Avoid glibc-2.1 bug and Solaris 7 through 9 bug.  */
#if defined _LIBICONV_VERSION \
    || !((__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) || defined __sun)
  {
    char *outptr = tmpbuf;
    size_t outsize = TMPBUFSIZE;
    size_t res = iconv (cd, NULL, NULL, &outptr, &outsize);
    if (res == (size_t)(-1))
      return (size_t)(-1);
    count += outptr - tmpbuf;
  }
  /* Return to the initial state.  */
  iconv (cd, NULL, NULL, NULL, NULL);
#endif
  return count;
#undef TMPBUFSIZE
}

static void
iconv_string_keeping_offsets (iconv_t cd, const char *s, size_t n,
                              size_t *offtable, char *t, size_t m)
{
  size_t i;
  const char *s_end;
  const char *inptr;
  char *outptr;
  size_t outsize;
  /* Avoid glibc-2.1 bug.  */
#if !defined _LIBICONV_VERSION && (__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1)
  const size_t extra = 1;
#else
  const size_t extra = 0;
#endif

  for (i = 0; i < n; i++)
    offtable[i] = (size_t)(-1);

  s_end = s + n;
  inptr = s;
  outptr = t;
  outsize = m + extra;
  while (inptr < s_end)
    {
      const char *saved_inptr;
      size_t insize;
      size_t res;

      offtable[inptr - s] = outptr - t;

      saved_inptr = inptr;
      res = (size_t)(-1);
      for (insize = 1; inptr + insize <= s_end; insize++)
        {
          res = iconv (cd, (ICONV_CONST char **) &inptr, &insize, &outptr, &outsize);
          if (!(res == (size_t)(-1) && errno == EINVAL))
            break;
          /* We expect that no input bytes have been consumed so far.  */
          if (inptr != saved_inptr)
            abort ();
        }
      /* After we verified the convertibility and computed the translation's
         size m, there shouldn't be any conversion error here. */
      if (res == (size_t)(-1))
        abort ();
    }
  /* Avoid glibc-2.1 bug and Solaris 7 bug.  */
#if defined _LIBICONV_VERSION \
    || !((__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) || defined __sun)
  if (iconv (cd, NULL, NULL, &outptr, &outsize) == (size_t)(-1))
    abort ();
#endif
  /* We should have produced exactly m output bytes.  */
  if (outsize != extra)
    abort ();
}

#endif /* HAVE_ICONV */

#if C_CTYPE_ASCII

/* Tests whether a string is entirely ASCII.  Returns 1 if yes.
   Returns 0 if the string is in an 8-bit encoding or an ISO-2022 encoding.  */
static int
is_all_ascii (const char *s, size_t n)
{
  for (; n > 0; s++, n--)
    {
      unsigned char c = (unsigned char) *s;

      if (!(c_isprint (c) || c_isspace (c)))
	return 0;
    }
  return 1;
}

#endif /* C_CTYPE_ASCII */

#if defined unused || defined TEST2

void
mbs_possible_linebreaks (const char *s, size_t n, const char *encoding,
                         char *p)
{
  if (n == 0)
    return;
  if (is_utf8_encoding (encoding))
    u8_possible_linebreaks ((const unsigned char *) s, n, encoding, p);
  else
    {
#if HAVE_ICONV
      iconv_t to_utf8;
      /* Avoid glibc-2.1 bug with EUC-KR.  */
# if (__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) && !defined _LIBICONV_VERSION
      if (STREQ (encoding, "EUC-KR", 'E', 'U', 'C', '-', 'K', 'R', 0, 0, 0))
	to_utf8 = (iconv_t)(-1);
      else
# endif
      /* Avoid Solaris 9 bug with GB2312, EUC-TW, BIG5, BIG5-HKSCS, GBK,
         GB18030.  */
# if defined __sun && !defined _LIBICONV_VERSION
      if (   STREQ (encoding, "GB2312", 'G', 'B', '2', '3', '1', '2', 0, 0, 0)
          || STREQ (encoding, "EUC-TW", 'E', 'U', 'C', '-', 'T', 'W', 0, 0, 0)
          || STREQ (encoding, "BIG5", 'B', 'I', 'G', '5', 0, 0, 0, 0, 0)
          || STREQ (encoding, "BIG5-HKSCS", 'B', 'I', 'G', '5', '-', 'H', 'K', 'S', 'C')
          || STREQ (encoding, "GBK", 'G', 'B', 'K', 0, 0, 0, 0, 0, 0)
          || STREQ (encoding, "GB18030", 'G', 'B', '1', '8', '0', '3', '0', 0, 0))
        to_utf8 = (iconv_t)(-1);
      else
# endif
      to_utf8 = iconv_open (UTF8_NAME, encoding);
      if (to_utf8 != (iconv_t)(-1))
        {
          /* Determine the length of the resulting UTF-8 string.  */
          size_t m = iconv_string_length (to_utf8, s, n);
          if (m != (size_t)(-1))
            {
              /* Convert the string to UTF-8 and build a translation table
                 from offsets into s to offsets into the translated string.  */
	      size_t memory_size = xsum3 (xtimes (n, sizeof (size_t)), m, m);
              char *memory =
		(size_in_bounds_p (memory_size) ? malloc (memory_size) : NULL);
              if (memory != NULL)
                {
                  size_t *offtable = (size_t *) memory;
                  char *t = (char *) (offtable + n);
                  char *q = (char *) (t + m);
                  size_t i;

                  iconv_string_keeping_offsets (to_utf8, s, n, offtable, t, m);

                  /* Determine the possible line breaks of the UTF-8 string.  */
                  u8_possible_linebreaks ((const unsigned char *) t, m, encoding, q);

                  /* Translate the result back to the original string.  */
                  memset (p, UC_BREAK_PROHIBITED, n);
                  for (i = 0; i < n; i++)
                    if (offtable[i] != (size_t)(-1))
                      p[i] = q[offtable[i]];

                  free (memory);
                  iconv_close (to_utf8);
                  return;
                }
            }
          iconv_close (to_utf8);
        }
#endif
      /* Impossible to convert.  */
#if C_CTYPE_ASCII
      if (is_all_ascii (s, n))
	{
	  /* ASCII is a subset of UTF-8.  */
	  u8_possible_linebreaks ((const unsigned char *) s, n, encoding, p);
	  return;
	}
#endif
      /* We have a non-ASCII string and cannot convert it.
	 Don't produce line breaks except those already present in the
	 input string.  All we assume here is that the encoding is
	 minimally ASCII compatible.  */
      {
        const char *s_end = s + n;
        while (s < s_end)
          {
            *p = (*s == '\n' ? UC_BREAK_MANDATORY : UC_BREAK_PROHIBITED);
            s++;
            p++;
          }
      }
    }
}

#endif

int
mbs_width_linebreaks (const char *s, size_t n,
                      int width, int start_column, int at_end_columns,
                      const char *o, const char *encoding,
                      char *p)
{
  if (n == 0)
    return start_column;
  if (is_utf8_encoding (encoding))
    return u8_width_linebreaks ((const unsigned char *) s, n, width, start_column, at_end_columns, o, encoding, p);
  else
    {
#if HAVE_ICONV
      iconv_t to_utf8;
      /* Avoid glibc-2.1 bug with EUC-KR.  */
# if (__GLIBC__ - 0 == 2 && __GLIBC_MINOR__ - 0 <= 1) && !defined _LIBICONV_VERSION
      if (STREQ (encoding, "EUC-KR", 'E', 'U', 'C', '-', 'K', 'R', 0, 0, 0))
	to_utf8 = (iconv_t)(-1);
      else
# endif
      /* Avoid Solaris 9 bug with GB2312, EUC-TW, BIG5, BIG5-HKSCS, GBK,
         GB18030.  */
# if defined __sun && !defined _LIBICONV_VERSION
      if (   STREQ (encoding, "GB2312", 'G', 'B', '2', '3', '1', '2', 0, 0, 0)
          || STREQ (encoding, "EUC-TW", 'E', 'U', 'C', '-', 'T', 'W', 0, 0, 0)
          || STREQ (encoding, "BIG5", 'B', 'I', 'G', '5', 0, 0, 0, 0, 0)
          || STREQ (encoding, "BIG5-HKSCS", 'B', 'I', 'G', '5', '-', 'H', 'K', 'S', 'C')
          || STREQ (encoding, "GBK", 'G', 'B', 'K', 0, 0, 0, 0, 0, 0)
          || STREQ (encoding, "GB18030", 'G', 'B', '1', '8', '0', '3', '0', 0, 0))
        to_utf8 = (iconv_t)(-1);
      else
# endif
      to_utf8 = iconv_open (UTF8_NAME, encoding);
      if (to_utf8 != (iconv_t)(-1))
        {
          /* Determine the length of the resulting UTF-8 string.  */
          size_t m = iconv_string_length (to_utf8, s, n);
          if (m != (size_t)(-1))
            {
              /* Convert the string to UTF-8 and build a translation table
                 from offsets into s to offsets into the translated string.  */
	      size_t memory_size =
		xsum4 (xtimes (n, sizeof (size_t)), m, m,
		       (o != NULL ? m : 0));
	      char *memory =
		(size_in_bounds_p (memory_size) ? malloc (memory_size) : NULL);
              if (memory != NULL)
                {
                  size_t *offtable = (size_t *) memory;
                  char *t = (char *) (offtable + n);
                  char *q = (char *) (t + m);
                  char *o8 = (o != NULL ? (char *) (q + m) : NULL);
                  int res_column;
                  size_t i;

                  iconv_string_keeping_offsets (to_utf8, s, n, offtable, t, m);

                  /* Translate the overrides to the UTF-8 string.  */
                  if (o != NULL)
                    {
                      memset (o8, UC_BREAK_UNDEFINED, m);
                      for (i = 0; i < n; i++)
                        if (offtable[i] != (size_t)(-1))
                          o8[offtable[i]] = o[i];
                    }

                  /* Determine the line breaks of the UTF-8 string.  */
                  res_column =
                    u8_width_linebreaks ((const unsigned char *) t, m, width, start_column, at_end_columns, o8, encoding, q);

                  /* Translate the result back to the original string.  */
                  memset (p, UC_BREAK_PROHIBITED, n);
                  for (i = 0; i < n; i++)
                    if (offtable[i] != (size_t)(-1))
                      p[i] = q[offtable[i]];

                  free (memory);
                  iconv_close (to_utf8);
                  return res_column;
                }
            }
          iconv_close (to_utf8);
        }
#endif
      /* Impossible to convert.  */
#if C_CTYPE_ASCII
      if (is_all_ascii (s, n))
	{
	  /* ASCII is a subset of UTF-8.  */
	  return u8_width_linebreaks ((const unsigned char *) s, n, width, start_column, at_end_columns, o, encoding, p);
	}
#endif
      /* We have a non-ASCII string and cannot convert it.
	 Don't produce line breaks except those already present in the
	 input string.  All we assume here is that the encoding is
	 minimally ASCII compatible.  */
      {
        const char *s_end = s + n;
        while (s < s_end)
          {
            *p = ((o != NULL && *o == UC_BREAK_MANDATORY) || *s == '\n'
                  ? UC_BREAK_MANDATORY
                  : UC_BREAK_PROHIBITED);
            s++;
            p++;
            if (o != NULL)
              o++;
          }
        /* We cannot compute widths in this case.  */
        return start_column;
      }
    }
}


#ifdef TEST2

#include <stdio.h>
#include <locale.h>

/* Read the contents of an input stream, and return it, terminated with a NUL
   byte. */
char *
read_file (FILE *stream)
{
#define BUFSIZE 4096
  char *buf = NULL;
  int alloc = 0;
  int size = 0;
  int count;

  while (! feof (stream))
    {
      if (size + BUFSIZE > alloc)
        {
          alloc = alloc + alloc / 2;
          if (alloc < size + BUFSIZE)
            alloc = size + BUFSIZE;
          buf = realloc (buf, alloc);
          if (buf == NULL)
            {
              fprintf (stderr, "out of memory\n");
              exit (1);
            }
        }
      count = fread (buf + size, 1, BUFSIZE, stream);
      if (count == 0)
        {
          if (ferror (stream))
            {
              perror ("fread");
              exit (1);
            }
        }
      else
        size += count;
    }
  buf = realloc (buf, size + 1);
  if (buf == NULL)
    {
      fprintf (stderr, "out of memory\n");
      exit (1);
    }
  buf[size] = '\0';
  return buf;
#undef BUFSIZE
}

int
main (int argc, char * argv[])
{
  setlocale (LC_CTYPE, "");
  if (argc == 1)
    {
      /* Display all the break opportunities in the input string.  */
      char *input = read_file (stdin);
      int length = strlen (input);
      char *breaks = malloc (length);
      int i;

      mbs_possible_linebreaks (input, length, locale_charset (), breaks);

      for (i = 0; i < length; i++)
        {
          switch (breaks[i])
            {
              case UC_BREAK_POSSIBLE:
                putc ('|', stdout);
                break;
              case UC_BREAK_MANDATORY:
                break;
              case UC_BREAK_PROHIBITED:
                break;
              default:
                abort ();
            }
          putc (input[i], stdout);
        }

      free (breaks);

      return 0;
    }
  else if (argc == 2)
    {
      /* Insert line breaks for a given width.  */
      int width = atoi (argv[1]);
      char *input = read_file (stdin);
      int length = strlen (input);
      char *breaks = malloc (length);
      int i;

      mbs_width_linebreaks (input, length, width, 0, 0, NULL, locale_charset (), breaks);

      for (i = 0; i < length; i++)
        {
          switch (breaks[i])
            {
              case UC_BREAK_POSSIBLE:
                putc ('\n', stdout);
                break;
              case UC_BREAK_MANDATORY:
                break;
              case UC_BREAK_PROHIBITED:
                break;
              default:
                abort ();
            }
          putc (input[i], stdout);
        }

      free (breaks);

      return 0;
    }
  else
    return 1;
}

#endif /* TEST2 */
