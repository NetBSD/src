/* Recode Serbian text from Cyrillic to Latin script.
   Copyright (C) 2006 Free Software Foundation, Inc.
   Written by Danilo Šegan <danilo@gnome.org>, 2006,
   and Bruno Haible <bruno@clisp.org>, 2006.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "filters.h"

#include <stdlib.h>

#include "xalloc.h"


/* Table for Serbian Cyrillic to Latin transcription.
   The table is indexed by the Unicode code point, in the range 0x0400..0x045f.
   The longest table entry is three bytes long.  */
static const char table[96][3 + 1] =
{
  /* U+0400 */ "",
  /* U+0401 */ "",
  /* U+0402 */ "\xC4\x90", /* "Đ" */
  /* U+0403 */ "",
  /* U+0404 */ "",
  /* U+0405 */ "",
  /* U+0406 */ "",
  /* U+0407 */ "",
  /* U+0408 */ "J",
  /* U+0409 */ "Lj",
  /* U+040A */ "Nj",
  /* U+040B */ "\xC4\x86", /* "Ć" */
  /* U+040C */ "",
  /* U+040D */ "",
  /* U+040E */ "",
  /* U+040F */ "D\xC5\xBE", /* "Dž" */
  /* U+0410 */ "A",
  /* U+0411 */ "B",
  /* U+0412 */ "V",
  /* U+0413 */ "G",
  /* U+0414 */ "D",
  /* U+0415 */ "E",
  /* U+0416 */ "\xC5\xBD", /* "Ž" */
  /* U+0417 */ "Z",
  /* U+0418 */ "I",
  /* U+0419 */ "",
  /* U+041A */ "K",
  /* U+041B */ "L",
  /* U+041C */ "M",
  /* U+041D */ "N",
  /* U+041E */ "O",
  /* U+041F */ "P",
  /* U+0420 */ "R",
  /* U+0421 */ "S",
  /* U+0422 */ "T",
  /* U+0423 */ "U",
  /* U+0424 */ "F",
  /* U+0425 */ "H",
  /* U+0426 */ "C",
  /* U+0427 */ "\xC4\x8C", /* "Č" */
  /* U+0428 */ "\xC5\xA0", /* "Š" */
  /* U+0429 */ "",
  /* U+042A */ "",
  /* U+042B */ "",
  /* U+042C */ "",
  /* U+042D */ "",
  /* U+042E */ "",
  /* U+042F */ "",
  /* U+0430 */ "a",
  /* U+0431 */ "b",
  /* U+0432 */ "v",
  /* U+0433 */ "g",
  /* U+0434 */ "d",
  /* U+0435 */ "e",
  /* U+0436 */ "\xC5\xBE", /* "ž" */
  /* U+0437 */ "z",
  /* U+0438 */ "i",
  /* U+0439 */ "",
  /* U+043A */ "k",
  /* U+043B */ "l",
  /* U+043C */ "m",
  /* U+043D */ "n",
  /* U+043E */ "o",
  /* U+043F */ "p",
  /* U+0440 */ "r",
  /* U+0441 */ "s",
  /* U+0442 */ "t",
  /* U+0443 */ "u",
  /* U+0444 */ "f",
  /* U+0445 */ "h",
  /* U+0446 */ "c",
  /* U+0447 */ "\xC4\x8D", /* "č" */
  /* U+0448 */ "\xC5\xA1", /* "š" */
  /* U+0449 */ "",
  /* U+044A */ "",
  /* U+044B */ "",
  /* U+044C */ "",
  /* U+044D */ "",
  /* U+044E */ "",
  /* U+044F */ "",
  /* U+0450 */ "",
  /* U+0451 */ "",
  /* U+0452 */ "\xC4\x91", /* "đ" */
  /* U+0453 */ "",
  /* U+0454 */ "",
  /* U+0455 */ "",
  /* U+0456 */ "",
  /* U+0457 */ "",
  /* U+0458 */ "j",
  /* U+0459 */ "lj",
  /* U+045A */ "nj",
  /* U+045B */ "\xC4\x87", /* "ć" */
  /* U+045C */ "",
  /* U+045D */ "",
  /* U+045E */ "",
  /* U+045F */ "d\xC5\xBE" /* "dž" */
};

/* Quick test for an uppercase character in the range U+0041..U+005A.
   The argument must be a byte in the range 0..UCHAR_MAX.  */
#define IS_UPPERCASE_LATIN(byte) \
  ((unsigned char) ((byte) - 'A') <= 'Z' - 'A')

/* Quick test for an uppercase character in the range U+0400..U+042F.
   The arguments must be bytes in the range 0..UCHAR_MAX.  */
#define IS_UPPERCASE_CYRILLIC(byte1,byte2) \
  ((byte1) == 0xd0 && (unsigned char) ((byte2) - 0x80) < 0x30)

void
serbian_to_latin (const char *input, size_t input_len,
		  char **output_p, size_t *output_len_p)
{
  /* Loop through the input string, producing a replacement for each character.
     Only characters in the range U+0400..U+045F (\xD0\x80..\xD1\x9F) need to
     be handled, and more precisely only those for which a replacement exists
     in the table.  Other characters are copied without modification.
     The characters U+0409, U+040A, U+040F are transliterated to uppercase or
     mixed-case replacements ("LJ" / "Lj", "NJ" / "Nj", "DŽ" / "Dž"), depending
     on the case of the surrounding characters.
     Since we assume UTF-8 encoding, the bytes \xD0..\xD1 can only occur at the
     beginning of a character; the second and further bytes of a character are
     all in the range \x80..\xBF.  */

  /* Since sequences of 2 bytes are sequences of at most 3 bytes, the size
     of the output will be at most 1.5 * input_len.  */
  size_t allocated = input_len + (input_len >> 1);
  char *output = (char *) xmalloc (allocated);

  const char *input_end = input + input_len;
  const char *ip;
  char *op;

  for (ip = input, op = output; ip < input_end; )
    {
      unsigned char byte = (unsigned char) *ip;

      /* Test for the first byte of a Cyrillic character.  */
      if ((byte >= 0xd0 && byte <= 0xd1) && (ip + 1 < input_end))
	{
	  unsigned char second_byte = (unsigned char) ip[1];

	  /* Verify the second byte is valid.  */
	  if (second_byte >= 0x80 && second_byte < 0xc0)
	    {
	      unsigned int uc = ((byte & 0x1f) << 6) | (second_byte & 0x3f);

	      if (uc >= 0x0400 && uc <= 0x045f)
		{
		  /* Look up replacement from the table.  */
		  const char *repl = table[uc - 0x0400];

		  if (repl[0] != '\0')
		    {
		      /* Found a replacement.
			 Now handle the special cases.  */
		      if (uc == 0x0409 || uc == 0x040a || uc == 0x040f)
			if ((ip + 2 < input_end
			     && IS_UPPERCASE_LATIN ((unsigned char) ip[2]))
			    || (ip + 3 < input_end
				&& IS_UPPERCASE_CYRILLIC ((unsigned char) ip[2],
							  (unsigned char) ip[3]))
			    || (ip >= input + 1
				&& IS_UPPERCASE_LATIN ((unsigned char) ip[-1]))
			    || (ip >= input + 2
				&& IS_UPPERCASE_CYRILLIC ((unsigned char) ip[-2],
							  (unsigned char) ip[-1])))
			  {
			    /* Use the upper-case replacement instead of
			       the mixed-case replacement.  */
			    switch (uc)
			      {
			      case 0x0409:
				repl = "LJ"; break;
			      case 0x040a:
				repl = "NJ"; break;
			      case 0x040f:
				repl = "D\xC5\xBD"/* "DŽ" */; break;
			      default:
				abort ();
			      }
			  }

		      /* Use the replacement.  */
		      *op++ = *repl++;
		      if (*repl != '\0')
			{
			  *op++ = *repl++;
			  if (*repl != '\0')
			    {
			      *op++ = *repl++;
			      /* All replacements have at most 3 bytes.  */
			      if (*repl != '\0')
				abort ();
			    }
			}
		      ip += 2;
		      continue;
		    }
		}
	    }
	}
      *op++ = *ip++;
    }

  {
    size_t output_len = op - output;

    /* Verify that the allocated size was not exceeded.  */
    if (output_len > allocated)
      abort ();
    /* Shrink the result.  */
    if (output_len < allocated)
      output = (char *) xrealloc (output, output_len);

    /* Done.  */
    *output_p = output;
    *output_len_p = output_len;
  }
}
