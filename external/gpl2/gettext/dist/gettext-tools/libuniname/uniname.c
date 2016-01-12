/* Association between Unicode characters and their names.
   Copyright (C) 2000-2002, 2005-2006 Free Software Foundation, Inc.

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
#include "uniname.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* Table of Unicode character names, derived from UnicodeData.txt.  */
#define uint16_t unsigned short
#define uint32_t unsigned int
#include "uninames.h"
/* It contains:
  static const char unicode_name_words[34594] = ...;
  #define UNICODE_CHARNAME_NUM_WORDS 5906
  static const struct { uint16_t extra_offset; uint16_t ind_offset; } unicode_name_by_length[26] = ...;
  #define UNICODE_CHARNAME_WORD_HANGUL 3624
  #define UNICODE_CHARNAME_WORD_SYLLABLE 4654
  #define UNICODE_CHARNAME_WORD_CJK 401
  #define UNICODE_CHARNAME_WORD_COMPATIBILITY 5755
  static const uint16_t unicode_names[62620] = ...;
  static const struct { uint16_t code; uint16_t name; } unicode_name_to_code[15257] = ...;
  static const struct { uint16_t code; uint16_t name; } unicode_code_to_name[15257] = ...;
  #define UNICODE_CHARNAME_MAX_LENGTH 83
  #define UNICODE_CHARNAME_MAX_WORDS 13
*/

/* Returns the word with a given index.  */
static const char *
unicode_name_word (unsigned int index, unsigned int *lengthp)
{
  unsigned int i1;
  unsigned int i2;
  unsigned int i;

  assert (index < UNICODE_CHARNAME_NUM_WORDS);

  /* Binary search for i with
       unicode_name_by_length[i].ind_offset <= index
     and
       index < unicode_name_by_length[i+1].ind_offset
   */

  i1 = 0;
  i2 = SIZEOF (unicode_name_by_length) - 1;
  while (i2 - i1 > 1)
    {
      unsigned int i = (i1 + i2) >> 1;
      if (unicode_name_by_length[i].ind_offset <= index)
	i1 = i;
      else
	i2 = i;
    }
  i = i1;
  assert (unicode_name_by_length[i].ind_offset <= index
	  && index < unicode_name_by_length[i+1].ind_offset);
  *lengthp = i;
  return &unicode_name_words[unicode_name_by_length[i].extra_offset
			     + (index-unicode_name_by_length[i].ind_offset)*i];
}

/* Looks up the index of a word.  */
static int
unicode_name_word_lookup (const char *word, unsigned int length)
{
  if (length > 0 && length < SIZEOF (unicode_name_by_length) - 1)
    {
      /* Binary search among the words of given length.  */
      unsigned int extra_offset = unicode_name_by_length[length].extra_offset;
      unsigned int i0 = unicode_name_by_length[length].ind_offset;
      unsigned int i1 = i0;
      unsigned int i2 = unicode_name_by_length[length+1].ind_offset;
      while (i2 - i1 > 0)
	{
	  unsigned int i = (i1 + i2) >> 1;
	  const char *p = &unicode_name_words[extra_offset + (i-i0)*length];
	  const char *w = word;
	  unsigned int n = length;
	  for (;;)
	    {
	      if (*p < *w)
		{
		  if (i1 == i)
		    return -1;
		  /* Note here: i1 < i < i2.  */
		  i1 = i;
		  break;
		}
	      if (*p > *w)
		{
		  /* Note here: i1 <= i < i2.  */
		  i2 = i;
		  break;
		}
	      p++; w++; n--;
	      if (n == 0)
		return i;
	    }
	}
    }
  return -1;
}

/* Auxiliary tables for Hangul syllable names, see the Unicode 3.0 book,
   sections 3.11 and 4.4.  */
static const char jamo_initial_short_name[19][3] =
{
  "G", "GG", "N", "D", "DD", "R", "M", "B", "BB", "S", "SS", "", "J", "JJ",
  "C", "K", "T", "P", "H"
};
static const char jamo_medial_short_name[21][4] =
{
  "A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O", "WA", "WAE", "OE", "YO",
  "U", "WEO", "WE", "WI", "YU", "EU", "YI", "I"
};
static const char jamo_final_short_name[28][3] =
{
  "", "G", "GG", "GS", "N", "NI", "NH", "D", "L", "LG", "LM", "LB", "LS", "LT",
  "LP", "LH", "M", "B", "BS", "S", "SS", "NG", "J", "C", "K", "T", "P", "H"
};

/* Looks up the name of a Unicode character, in uppercase ASCII.
   Returns the filled buf, or NULL if the character does not have a name.  */
char *
unicode_character_name (unsigned int c, char *buf)
{
  if (c >= 0xAC00 && c <= 0xD7A3)
    {
      /* Special case for Hangul syllables. Keeps the tables small.  */
      char *ptr;
      unsigned int tmp;
      unsigned int index1;
      unsigned int index2;
      unsigned int index3;
      const char *q;

      /* buf needs to have at least 16 + 7 bytes here.  */
      memcpy (buf, "HANGUL SYLLABLE ", 16);
      ptr = buf + 16;

      tmp = c - 0xAC00;
      index3 = tmp % 28; tmp = tmp / 28;
      index2 = tmp % 21; tmp = tmp / 21;
      index1 = tmp;

      q = jamo_initial_short_name[index1];
      while (*q != '\0')
	*ptr++ = *q++;
      q = jamo_medial_short_name[index2];
      while (*q != '\0')
	*ptr++ = *q++;
      q = jamo_final_short_name[index3];
      while (*q != '\0')
	*ptr++ = *q++;
      *ptr = '\0';
      return buf;
    }
  else if ((c >= 0xF900 && c <= 0xFA2D) || (c >= 0xFA30 && c <= 0xFA6A)
	   || (c >= 0xFA70 && c <= 0xFAD9) || (c >= 0x2F800 && c <= 0x2FA1D))
    {
      /* Special case for CJK compatibility ideographs. Keeps the tables
	 small.  */
      char *ptr;
      int i;

      /* buf needs to have at least 28 + 5 bytes here.  */
      memcpy (buf, "CJK COMPATIBILITY IDEOGRAPH-", 28);
      ptr = buf + 28;

      for (i = (c < 0x10000 ? 12 : 16); i >= 0; i -= 4)
	{
	  unsigned int x = (c >> i) & 0xf;
	  *ptr++ = (x < 10 ? '0' : 'A' - 10) + x;
	}
      *ptr = '\0';
      return buf;
    }
  else
    {
      const uint16_t *words;

      /* Transform the code so that it fits in 16 bits.  */
      switch (c >> 12)
	{
	case 0x00: case 0x01: case 0x02: case 0x03: case 0x04:
	  break;
	case 0x0A:
	  c -= 0x05000;
	  break;
	case 0x0F:
	  c -= 0x09000;
	  break;
	case 0x10:
	  c -= 0x09000;
	  break;
	case 0x1D:
	  c -= 0x15000;
	  break;
	case 0x2F:
	  c -= 0x26000;
	  break;
	case 0xE0:
	  c -= 0xD6000;
	  break;
	default:
	  return NULL;
	}

      {
	/* Binary search in unicode_code_to_name.  */
	unsigned int i1 = 0;
	unsigned int i2 = SIZEOF (unicode_code_to_name);
	for (;;)
	  {
	    unsigned int i = (i1 + i2) >> 1;
	    if (unicode_code_to_name[i].code == c)
	      {
		words = &unicode_names[unicode_code_to_name[i].name];
		break;
	      }
	    else if (unicode_code_to_name[i].code < c)
	      {
		if (i1 == i)
		  {
		    words = NULL;
		    break;
		  }
		/* Note here: i1 < i < i2.  */
		i1 = i;
	      }
	    else if (unicode_code_to_name[i].code > c)
	      {
		if (i2 == i)
		  {
		    words = NULL;
		    break;
		  }
		/* Note here: i1 <= i < i2.  */
		i2 = i;
	      }
	  }
      }
      if (words != NULL)
	{
	  /* Found it in unicode_code_to_name. Now concatenate the words.  */
	  /* buf needs to have at least UNICODE_CHARNAME_MAX_LENGTH bytes.  */
	  char *ptr = buf;
	  for (;;)
	    {
	      unsigned int wordlen;
	      const char *word = unicode_name_word (*words>>1, &wordlen);
	      do
		*ptr++ = *word++;
	      while (--wordlen > 0);
	      if ((*words & 1) == 0)
		break;
	      *ptr++ = ' ';
	      words++;
	    }
	  *ptr = '\0';
	  return buf;
	}
      return NULL;
    }
}

/* Looks up the Unicode character with a given name, in upper- or lowercase
   ASCII.  Returns the character if found, or UNINAME_INVALID if not found.  */
unsigned int
unicode_name_character (const char *name)
{
  unsigned int len = strlen (name);
  if (len > 1 && len <= UNICODE_CHARNAME_MAX_LENGTH)
    {
      /* Test for "word1 word2 ..." syntax.  */
      char buf[UNICODE_CHARNAME_MAX_LENGTH];
      char *ptr = buf;
      for (;;)
	{
	  char c = *name++;
	  if (!(c >= ' ' && c <= '~'))
	    break;
	  *ptr++ = (c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
	  if (--len == 0)
	    goto filled_buf;
	}
      if (false)
      filled_buf:
	{
	  /* Convert the constituents to uint16_t words.  */
	  uint16_t words[UNICODE_CHARNAME_MAX_WORDS];
	  uint16_t *wordptr = words;
	  {
	    const char *p1 = buf;
	    for (;;)
	      {
		{
		  int word;
		  const char *p2 = p1;
		  while (p2 < ptr && *p2 != ' ')
		    p2++;
		  word = unicode_name_word_lookup (p1, p2 - p1);
		  if (word < 0)
		    break;
		  if (wordptr == &words[UNICODE_CHARNAME_MAX_WORDS])
		    break;
		  *wordptr++ = word;
		  if (p2 == ptr)
		    goto filled_words;
		  p1 = p2 + 1;
		}
		/* Special case for Hangul syllables. Keeps the tables small. */
		if (wordptr == &words[2]
		    && words[0] == UNICODE_CHARNAME_WORD_HANGUL
		    && words[1] == UNICODE_CHARNAME_WORD_SYLLABLE)
		  {
		    /* Split the last word [p1..ptr) into three parts:
			 1) [BCDGHJKMNPRST]
			 2) [AEIOUWY]
			 3) [BCDGHIJKLMNPST]
		     */
		    const char *p2;
		    const char *p3;
		    const char *p4;

		    p2 = p1;
		    while (p2 < ptr
			   && (*p2 == 'B' || *p2 == 'C' || *p2 == 'D'
			       || *p2 == 'G' || *p2 == 'H' || *p2 == 'J'
			       || *p2 == 'K' || *p2 == 'M' || *p2 == 'N'
			       || *p2 == 'P' || *p2 == 'R' || *p2 == 'S'
			       || *p2 == 'T'))
		      p2++;
		    p3 = p2;
		    while (p3 < ptr
			   && (*p3 == 'A' || *p3 == 'E' || *p3 == 'I'
			       || *p3 == 'O' || *p3 == 'U' || *p3 == 'W'
			       || *p3 == 'Y'))
		      p3++;
		    p4 = p3;
		    while (p4 < ptr
			   && (*p4 == 'B' || *p4 == 'C' || *p4 == 'D'
			       || *p4 == 'G' || *p4 == 'H' || *p4 == 'I'
			       || *p4 == 'J' || *p4 == 'K' || *p4 == 'L'
			       || *p4 == 'M' || *p4 == 'N' || *p4 == 'P'
			       || *p4 == 'S' || *p4 == 'T'))
		      p4++;
		    if (p4 == ptr)
		      {
			unsigned int n1 = p2 - p1;
			unsigned int n2 = p3 - p2;
			unsigned int n3 = p4 - p3;

			if (n1 <= 2 && (n2 >= 1 && n2 <= 3) && n3 <= 2)
			  {
			    unsigned int index1;

			    for (index1 = 0; index1 < 19; index1++)
			      if (memcmp(jamo_initial_short_name[index1], p1, n1) == 0
				  && jamo_initial_short_name[index1][n1] == '\0')
				{
				  unsigned int index2;

				  for (index2 = 0; index2 < 21; index2++)
				    if (memcmp(jamo_medial_short_name[index2], p2, n2) == 0
					&& jamo_medial_short_name[index2][n2] == '\0')
				      {
					unsigned int index3;

					for (index3 = 0; index3 < 28; index3++)
					  if (memcmp(jamo_final_short_name[index3], p3, n3) == 0
					      && jamo_final_short_name[index3][n3] == '\0')
					    {
					      return 0xAC00 + (index1 * 21 + index2) * 28 + index3;
					    }
					break;
				      }
				  break;
				}
			  }
		      }
		  }
		/* Special case for CJK compatibility ideographs. Keeps the
		   tables small.  */
		if (wordptr == &words[2]
		    && words[0] == UNICODE_CHARNAME_WORD_CJK
		    && words[1] == UNICODE_CHARNAME_WORD_COMPATIBILITY
		    && p1 + 14 <= ptr
		    && p1 + 15 >= ptr
		    && memcmp (p1, "IDEOGRAPH-", 10) == 0)
		  {
		    const char *p2 = p1 + 10;

		    if (*p2 != '0')
		      {
			unsigned int c = 0;

			for (;;)
			  {
			    if (*p2 >= '0' && *p2 <= '9')
			      c += (*p2 - '0');
			    else if (*p2 >= 'A' && *p2 <= 'F')
			      c += (*p2 - 'A' + 10);
			    else
			      break;
			    p2++;
			    if (p2 == ptr)
			      {
				if ((c >= 0xF900 && c <= 0xFA2D)
				    || (c >= 0xFA30 && c <= 0xFA6A)
				    || (c >= 0xFA70 && c <= 0xFAD9)
				    || (c >= 0x2F800 && c <= 0x2FA1D))
				  return c;
				else
				  break;
			      }
			    c = c << 4;
			  }
		      }
		  }
	      }
	  }
	  if (false)
	  filled_words:
	    {
	      /* Multiply by 2, to simplify later comparisons.  */
	      unsigned int words_length = wordptr - words;
	      {
		int i = words_length - 1;
		words[i] = 2 * words[i];
		for (; --i >= 0; )
		  words[i] = 2 * words[i] + 1;
	      }
	      /* Binary search in unicode_name_to_code.  */
	      {
		unsigned int i1 = 0;
		unsigned int i2 = SIZEOF (unicode_name_to_code);
		for (;;)
		  {
		    unsigned int i = (i1 + i2) >> 1;
		    const uint16_t *w = words;
		    const uint16_t *p = &unicode_names[unicode_name_to_code[i].name];
		    unsigned int n = words_length;
		    for (;;)
		      {
			if (*p < *w)
			  {
			    if (i1 == i)
			      goto name_not_found;
			    /* Note here: i1 < i < i2.  */
			    i1 = i;
			    break;
			  }
			else if (*p > *w)
			  {
			    if (i2 == i)
			      goto name_not_found;
			    /* Note here: i1 <= i < i2.  */
			    i2 = i;
			    break;
			  }
			p++; w++; n--;
			if (n == 0)
			  {
			    unsigned int c = unicode_name_to_code[i].code;

			    /* Undo the transformation to 16-bit space.  */
			    static const unsigned int offset[11] =
			      {
				0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
				0x05000, 0x09000, 0x09000, 0x15000, 0x26000,
				0xD6000
			      };
			    return c + offset[c >> 12];
			  }
		      }
		  }
	      }
	    name_not_found: ;
	    }
	}
    }
  return UNINAME_INVALID;
}
