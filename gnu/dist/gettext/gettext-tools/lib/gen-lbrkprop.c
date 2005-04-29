/* Generate a Unicode conforming Line Break Properties tables from a
   UnicodeData file.
   Written by Bruno Haible <bruno@clisp.org>, 2000-2004.

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

/* Usage example:
     $ gen-lbrkprop /usr/local/share/Unidata/UnicodeData.txt \
		    Combining.txt \
		    /usr/local/share/Unidata/EastAsianWidth.txt \
		    /usr/local/share/Unidata/LineBreak.txt \
		    3.1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/* This structure represents one line in the UnicodeData.txt file.  */
struct unicode_attribute
{
  const char *name;           /* Character name */
  const char *category;       /* General category */
  const char *combining;      /* Canonical combining classes */
  const char *bidi;           /* Bidirectional category */
  const char *decomposition;  /* Character decomposition mapping */
  const char *decdigit;       /* Decimal digit value */
  const char *digit;          /* Digit value */
  const char *numeric;        /* Numeric value */
  int mirrored;               /* mirrored */
  const char *oldname;        /* Old Unicode 1.0 name */
  const char *comment;        /* Comment */
  unsigned int upper;         /* Uppercase mapping */
  unsigned int lower;         /* Lowercase mapping */
  unsigned int title;         /* Titlecase mapping */
};

/* Missing fields are represented with "" for strings, and NONE for
   characters.  */
#define NONE (~(unsigned int)0)

/* The entire contents of the UnicodeData.txt file.  */
struct unicode_attribute unicode_attributes [0x110000];

/* Stores in unicode_attributes[i] the values from the given fields.  */
static void
fill_attribute (unsigned int i,
		const char *field1, const char *field2,
		const char *field3, const char *field4,
		const char *field5, const char *field6,
		const char *field7, const char *field8,
		const char *field9, const char *field10,
		const char *field11, const char *field12,
		const char *field13, const char *field14)
{
  struct unicode_attribute * uni;

  if (i >= 0x110000)
    {
      fprintf (stderr, "index too large\n");
      exit (1);
    }
  uni = &unicode_attributes[i];
  /* Copy the strings.  */
  uni->name          = strdup (field1);
  uni->category      = (field2[0] == '\0' ? "" : strdup (field2));
  uni->combining     = (field3[0] == '\0' ? "" : strdup (field3));
  uni->bidi          = (field4[0] == '\0' ? "" : strdup (field4));
  uni->decomposition = (field5[0] == '\0' ? "" : strdup (field5));
  uni->decdigit      = (field6[0] == '\0' ? "" : strdup (field6));
  uni->digit         = (field7[0] == '\0' ? "" : strdup (field7));
  uni->numeric       = (field8[0] == '\0' ? "" : strdup (field8));
  uni->mirrored      = (field9[0] == 'Y');
  uni->oldname       = (field10[0] == '\0' ? "" : strdup (field10));
  uni->comment       = (field11[0] == '\0' ? "" : strdup (field11));
  uni->upper = (field12[0] =='\0' ? NONE : strtoul (field12, NULL, 16));
  uni->lower = (field13[0] =='\0' ? NONE : strtoul (field13, NULL, 16));
  uni->title = (field14[0] =='\0' ? NONE : strtoul (field14, NULL, 16));
}

/* Maximum length of a field in the UnicodeData.txt file.  */
#define FIELDLEN 120

/* Reads the next field from STREAM.  The buffer BUFFER has size FIELDLEN.
   Reads up to (but excluding) DELIM.
   Returns 1 when a field was successfully read, otherwise 0.  */
static int
getfield (FILE *stream, char *buffer, int delim)
{
  int count = 0;
  int c;

  for (; (c = getc (stream)), (c != EOF && c != delim); )
    {
      /* The original unicode.org UnicodeData.txt file happens to have
	 CR/LF line terminators.  Silently convert to LF.  */
      if (c == '\r')
	continue;

      /* Put c into the buffer.  */
      if (++count >= FIELDLEN - 1)
	{
	  fprintf (stderr, "field too long\n");
	  exit (1);
	}
      *buffer++ = c;
    }

  if (c == EOF)
    return 0;

  *buffer = '\0';
  return 1;
}

/* Stores in unicode_attributes[] the entire contents of the UnicodeData.txt
   file.  */
static void
fill_attributes (const char *unicodedata_filename)
{
  unsigned int i, j;
  FILE *stream;
  char field0[FIELDLEN];
  char field1[FIELDLEN];
  char field2[FIELDLEN];
  char field3[FIELDLEN];
  char field4[FIELDLEN];
  char field5[FIELDLEN];
  char field6[FIELDLEN];
  char field7[FIELDLEN];
  char field8[FIELDLEN];
  char field9[FIELDLEN];
  char field10[FIELDLEN];
  char field11[FIELDLEN];
  char field12[FIELDLEN];
  char field13[FIELDLEN];
  char field14[FIELDLEN];
  int lineno = 0;

  for (i = 0; i < 0x110000; i++)
    unicode_attributes[i].name = NULL;

  stream = fopen (unicodedata_filename, "r");
  if (stream == NULL)
    {
      fprintf (stderr, "error during fopen of '%s'\n", unicodedata_filename);
      exit (1);
    }

  for (;;)
    {
      int n;

      lineno++;
      n = getfield (stream, field0, ';');
      n += getfield (stream, field1, ';');
      n += getfield (stream, field2, ';');
      n += getfield (stream, field3, ';');
      n += getfield (stream, field4, ';');
      n += getfield (stream, field5, ';');
      n += getfield (stream, field6, ';');
      n += getfield (stream, field7, ';');
      n += getfield (stream, field8, ';');
      n += getfield (stream, field9, ';');
      n += getfield (stream, field10, ';');
      n += getfield (stream, field11, ';');
      n += getfield (stream, field12, ';');
      n += getfield (stream, field13, ';');
      n += getfield (stream, field14, '\n');
      if (n == 0)
	break;
      if (n != 15)
	{
	  fprintf (stderr, "short line in'%s':%d\n",
		   unicodedata_filename, lineno);
	  exit (1);
	}
      i = strtoul (field0, NULL, 16);
      if (field1[0] == '<'
	  && strlen (field1) >= 9
	  && !strcmp (field1 + strlen(field1) - 8, ", First>"))
	{
	  /* Deal with a range. */
	  lineno++;
	  n = getfield (stream, field0, ';');
	  n += getfield (stream, field1, ';');
	  n += getfield (stream, field2, ';');
	  n += getfield (stream, field3, ';');
	  n += getfield (stream, field4, ';');
	  n += getfield (stream, field5, ';');
	  n += getfield (stream, field6, ';');
	  n += getfield (stream, field7, ';');
	  n += getfield (stream, field8, ';');
	  n += getfield (stream, field9, ';');
	  n += getfield (stream, field10, ';');
	  n += getfield (stream, field11, ';');
	  n += getfield (stream, field12, ';');
	  n += getfield (stream, field13, ';');
	  n += getfield (stream, field14, '\n');
	  if (n != 15)
	    {
	      fprintf (stderr, "missing end range in '%s':%d\n",
		       unicodedata_filename, lineno);
	      exit (1);
	    }
	  if (!(field1[0] == '<'
		&& strlen (field1) >= 8
		&& !strcmp (field1 + strlen (field1) - 7, ", Last>")))
	    {
	      fprintf (stderr, "missing end range in '%s':%d\n",
		       unicodedata_filename, lineno);
	      exit (1);
	    }
	  field1[strlen (field1) - 7] = '\0';
	  j = strtoul (field0, NULL, 16);
	  for (; i <= j; i++)
	    fill_attribute (i, field1+1, field2, field3, field4, field5,
			       field6, field7, field8, field9, field10,
			       field11, field12, field13, field14);
	}
      else
	{
	  /* Single character line */
	  fill_attribute (i, field1, field2, field3, field4, field5,
			     field6, field7, field8, field9, field10,
			     field11, field12, field13, field14);
	}
    }
  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error reading from '%s'\n", unicodedata_filename);
      exit (1);
    }
}

/* The combining property from the PropList.txt file.  */
char unicode_combining[0x110000];

/* Stores in unicode_combining[] the Combining property from the
   Unicode 3.0 PropList.txt file.  */
static void
fill_combining (const char *proplist_filename)
{
  unsigned int i;
  FILE *stream;
  char buf[100+1];

  for (i = 0; i < 0x110000; i++)
    unicode_combining[i] = 0;

  stream = fopen (proplist_filename, "r");
  if (stream == NULL)
    {
      fprintf (stderr, "error during fopen of '%s'\n", proplist_filename);
      exit (1);
    }

  /* Search for the "Property dump for: 0x20000004 (Combining)" line.  */
  do
    {
      if (fscanf (stream, "%100[^\n]\n", buf) < 1)
	{
	  fprintf (stderr, "no combining property found in '%s'\n",
		   proplist_filename);
	  exit (1);
	}
    }
  while (strstr (buf, "(Combining)") == NULL);

  for (;;)
    {
      unsigned int i1, i2;

      if (fscanf (stream, "%100[^\n]\n", buf) < 1)
	{
	  fprintf (stderr, "premature end of combining property in '%s'\n",
		   proplist_filename);
	  exit (1);
	}
      if (buf[0] == '*')
	break;
      if (strlen (buf) >= 10 && buf[4] == '.' && buf[5] == '.')
	{
	  if (sscanf (buf, "%4X..%4X", &i1, &i2) < 2)
	    {
	      fprintf (stderr, "parse error in combining property in '%s'\n",
		       proplist_filename);
	      exit (1);
	    }
	}
      else if (strlen (buf) >= 4)
	{
	  if (sscanf (buf, "%4X", &i1) < 1)
	    {
	      fprintf (stderr, "parse error in combining property in '%s'\n",
		       proplist_filename);
	      exit (1);
	    }
	  i2 = i1;
	}
      else
	{
	  fprintf (stderr, "parse error in combining property in '%s'\n",
		   proplist_filename);
	  exit (1);
	}
      for (i = i1; i <= i2; i++)
	unicode_combining[i] = 1;
    }
  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error reading from '%s'\n", proplist_filename);
      exit (1);
    }
}

/* The width property from the EastAsianWidth.txt file.
   Each is NULL (unassigned) or "N", "A", "H", "W", "F", "Na".  */
const char * unicode_width[0x110000];

/* Stores in unicode_width[] the width property from the EastAsianWidth.txt
   file.  */
static void
fill_width (const char *width_filename)
{
  unsigned int i, j;
  FILE *stream;
  char field0[FIELDLEN];
  char field1[FIELDLEN];
  char field2[FIELDLEN];
  int lineno = 0;

  for (i = 0; i < 0x110000; i++)
    unicode_width[i] = (unicode_attributes[i].name != NULL ? "N" : NULL);

  stream = fopen (width_filename, "r");
  if (stream == NULL)
    {
      fprintf (stderr, "error during fopen of '%s'\n", width_filename);
      exit (1);
    }

  for (;;)
    {
      int n;
      int c;

      lineno++;
      c = getc (stream);
      if (c == EOF)
	break;
      if (c == '#')
	{
	  do c = getc (stream); while (c != EOF && c != '\n');
	  continue;
	}
      ungetc (c, stream);
      n = getfield (stream, field0, ';');
      n += getfield (stream, field1, ' ');
      n += getfield (stream, field2, '\n');
      if (n == 0)
	break;
      if (n != 3)
	{
	  fprintf (stderr, "short line in '%s':%d\n", width_filename, lineno);
	  exit (1);
	}
      i = strtoul (field0, NULL, 16);
      if (strstr (field0, "..") != NULL)
	{
	  /* Deal with a range.  */
	  j = strtoul (strstr (field0, "..") + 2, NULL, 16);
	  for (; i <= j; i++)
	    unicode_width[i] = strdup (field1);
	}
      else
	{
	  /* Single character line.  */
	  unicode_width[i] = strdup (field1);
	}
    }
  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error reading from '%s'\n", width_filename);
      exit (1);
    }
}

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

/* Returns the line breaking classification for ch, as a bit mask.  */
static int
get_lbp (unsigned int ch)
{
  int attr = 0;

  if (unicode_attributes[ch].name != NULL)
    {
      /* mandatory break */
      if (ch == 0x000A || ch == 0x000D || ch == 0x0085 /* newline */
	  || ch == 0x000C /* form feed */
	  || ch == 0x2028 /* LINE SEPARATOR */
	  || ch == 0x2029 /* PARAGRAPH SEPARATOR */)
	attr |= 1 << LBP_BK;

      /* zero width space */
      if (ch == 0x200B /* ZERO WIDTH SPACE */)
	attr |= 1 << LBP_ZW;

      /* inseparable */
      if (ch == 0x2024 /* ONE DOT LEADER */
	  || ch == 0x2025 /* TWO DOT LEADER */
	  || ch == 0x2026 /* HORIZONTAL ELLIPSIS */)
	attr |= 1 << LBP_IN;

      /* non-breaking (glue) */
      if (ch == 0xFEFF /* ZERO WIDTH NO-BREAK SPACE */
	  || ch == 0x00A0 /* NO-BREAK SPACE */
	  || ch == 0x202F /* NARROW NO-BREAK SPACE */
	  || ch == 0x2007 /* FIGURE SPACE */
	  || ch == 0x2011 /* NON-BREAKING HYPHEN */
	  || ch == 0x0F0C /* TIBETAN MARK DELIMITER TSHEG BSTAR */)
	attr |= 1 << LBP_GL;

      /* contingent break opportunity */
      if (ch == 0xFFFC /* OBJECT REPLACEMENT CHARACTER */)
	attr |= 1 << LBP_CB;

      /* space */
      if (ch == 0x0020 /* SPACE */)
	attr |= 1 << LBP_SP;

      /* break opportunity after */
      if (ch == 0x2000 /* EN QUAD */
	  || ch == 0x2001 /* EM QUAD */
	  || ch == 0x2002 /* EN SPACE */
	  || ch == 0x2003 /* EM SPACE */
	  || ch == 0x2004 /* THREE-PER-EM SPACE */
	  || ch == 0x2005 /* FOUR-PER-EM SPACE */
	  || ch == 0x2006 /* SIX-PER-EM SPACE */
	  || ch == 0x2008 /* PUNCTUATION SPACE */
	  || ch == 0x2009 /* THIN SPACE */
	  || ch == 0x200A /* HAIR SPACE */
	  || ch == 0x0009 /* tab */
	  || ch == 0x058A /* ARMENIAN HYPHEN */
	  || ch == 0x2010 /* HYPHEN */
	  || ch == 0x2012 /* FIGURE DASH */
	  || ch == 0x2013 /* EN DASH */
	  || ch == 0x00AD /* SOFT HYPHEN */
	  || ch == 0x0F0B /* TIBETAN MARK INTERSYLLABIC TSHEG */
	  || ch == 0x1361 /* ETHIOPIC WORDSPACE */
	  || ch == 0x1680 /* OGHAM SPACE MARK */
	  || ch == 0x17D5 /* KHMER SIGN BARIYOOSAN */
	  || ch == 0x2027 /* HYPHENATION POINT */
	  || ch == 0x007C /* VERTICAL LINE */)
	attr |= 1 << LBP_BA;

      /* break opportunity before */
      if (ch == 0x00B4 /* ACUTE ACCENT */
	  || ch == 0x02C8 /* MODIFIER LETTER VERTICAL LINE */
	  || ch == 0x02CC /* MODIFIER LETTER LOW VERTICAL LINE */
	  || ch == 0x1806 /* MONGOLIAN TODO SOFT HYPHEN */)
	attr |= 1 << LBP_BB;

      /* break opportunity before and after */
      if (ch == 0x2014 /* EM DASH */)
	attr |= 1 << LBP_B2;

      /* hyphen */
      if (ch == 0x002D /* HYPHEN-MINUS */)
	attr |= 1 << LBP_HY;

      /* exclamation/interrogation */
      if (ch == 0x0021 /* EXCLAMATION MARK */
	  || ch == 0x003F /* QUESTION MARK */
	  || ch == 0xFE56 /* SMALL QUESTION MARK */
	  || ch == 0xFE57 /* SMALL EXCLAMATION MARK */
	  || ch == 0xFF01 /* FULLWIDTH EXCLAMATION MARK */
	  || ch == 0xFF1F /* FULLWIDTH QUESTION MARK */)
	attr |= 1 << LBP_EX;

      /* opening punctuation */
      if (unicode_attributes[ch].category[0] == 'P'
	  && unicode_attributes[ch].category[1] == 's')
	attr |= 1 << LBP_OP;

      /* closing punctuation */
      if (ch == 0x3001 /* IDEOGRAPHIC COMMA */
	  || ch == 0x3002 /* IDEOGRAPHIC FULL STOP */
	  || ch == 0xFE50 /* SMALL COMMA */
	  || ch == 0xFE52 /* SMALL FULL STOP */
	  || ch == 0xFF0C /* FULLWIDTH COMMA */
	  || ch == 0xFF0E /* FULLWIDTH FULL STOP */
	  || ch == 0xFF61 /* HALFWIDTH IDEOGRAPHIC FULL STOP */
	  || ch == 0xFF64 /* HALFWIDTH IDEOGRAPHIC COMMA */
	  || (unicode_attributes[ch].category[0] == 'P'
	      && unicode_attributes[ch].category[1] == 'e'))
	attr |= 1 << LBP_CL;

      /* ambiguous quotation */
      if (ch == 0x0022 /* QUOTATION MARK */
	  || ch == 0x0027 /* APOSTROPHE */
	  || (unicode_attributes[ch].category[0] == 'P'
	      && (unicode_attributes[ch].category[1] == 'f'
		  || unicode_attributes[ch].category[1] == 'i')))
	attr |= 1 << LBP_QU;

      /* attached characters and combining marks */
      if ((unicode_attributes[ch].category[0] == 'M'
	   && (unicode_attributes[ch].category[1] == 'n'
	       || unicode_attributes[ch].category[1] == 'c'
	       || unicode_attributes[ch].category[1] == 'e'))
	  || (ch >= 0x1160 && ch <= 0x11F9)
	  || (unicode_attributes[ch].category[0] == 'C'
	      && (unicode_attributes[ch].category[1] == 'c'
		  || unicode_attributes[ch].category[1] == 'f')))
	if (!(attr & ((1 << LBP_BK) | (1 << LBP_BA) | (1 << LBP_GL))))
	  attr |= 1 << LBP_CM;

      /* non starter */
      if (ch == 0x0E5A /* THAI CHARACTER ANGKHANKHU */
	  || ch == 0x0E5B /* THAI CHARACTER KHOMUT */
	  || ch == 0x17D4 /* KHMER SIGN KHAN */
	  || ch == 0x17D6 /* KHMER SIGN CAMNUC PII KUUH */
	  || ch == 0x17D7 /* KHMER SIGN LEK TOO */
	  || ch == 0x17D8 /* KHMER SIGN BEYYAL */
	  || ch == 0x17D9 /* KHMER SIGN PHNAEK MUAN */
	  || ch == 0x17DA /* KHMER SIGN KOOMUUT */
	  || ch == 0x203C /* DOUBLE EXCLAMATION MARK */
	  || ch == 0x2044 /* FRACTION SLASH */
	  || ch == 0x3005 /* IDEOGRAPHIC ITERATION MARK */
	  || ch == 0x301C /* WAVE DASH */
	  || ch == 0x309B /* KATAKANA-HIRAGANA VOICED SOUND MARK */
	  || ch == 0x309C /* KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
	  || ch == 0x309D /* HIRAGANA ITERATION MARK */
	  || ch == 0x309E /* HIRAGANA VOICED ITERATION MARK */
	  || ch == 0x30FB /* KATAKANA MIDDLE DOT */
	  || ch == 0x30FD /* KATAKANA ITERATION MARK */
	  || ch == 0xFE54 /* SMALL SEMICOLON */
	  || ch == 0xFE55 /* SMALL COLON */
	  || ch == 0xFF1A /* FULLWIDTH COLON */
	  || ch == 0xFF1B /* FULLWIDTH SEMICOLON */
	  || ch == 0xFF65 /* HALFWIDTH KATAKANA MIDDLE DOT */
	  || ch == 0xFF70 /* HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK */
	  || ch == 0xFF9E /* HALFWIDTH KATAKANA VOICED SOUND MARK */
	  || ch == 0xFF9F /* HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK */
	  || (unicode_attributes[ch].category[0] == 'L'
	      && unicode_attributes[ch].category[1] == 'm'
	      && (unicode_width[ch][0] == 'W'
		  || unicode_width[ch][0] == 'H'))
	  || (unicode_attributes[ch].category[0] == 'S'
	      && unicode_attributes[ch].category[1] == 'k'
	      && unicode_width[ch][0] == 'W')
	  || strstr (unicode_attributes[ch].name, "HIRAGANA LETTER SMALL ") != NULL
	  || strstr (unicode_attributes[ch].name, "KATAKANA LETTER SMALL ") != NULL)
	attr |= 1 << LBP_NS;

      /* numeric */
      if (unicode_attributes[ch].category[0] == 'N'
	  && unicode_attributes[ch].category[1] == 'd'
	  && strstr (unicode_attributes[ch].name, "FULLWIDTH") == NULL)
	attr |= 1 << LBP_NU;

      /* infix separator (numeric) */
      if (ch == 0x002C /* COMMA */
	  || ch == 0x002E /* FULL STOP */
	  || ch == 0x003A /* COLON */
	  || ch == 0x003B /* SEMICOLON */
	  || ch == 0x0589 /* ARMENIAN FULL STOP */)
	attr |= 1 << LBP_IS;

      /* symbols allowing breaks */
      if (ch == 0x002F /* SOLIDUS */)
	attr |= 1 << LBP_SY;

      /* postfix (numeric) */
      if (ch == 0x0025 /* PERCENT SIGN */
	  || ch == 0x00A2 /* CENT SIGN */
	  || ch == 0x00B0 /* DEGREE SIGN */
	  || ch == 0x2030 /* PER MILLE SIGN */
	  || ch == 0x2031 /* PER TEN THOUSAND SIGN */
	  || ch == 0x2032 /* PRIME */
	  || ch == 0x2033 /* DOUBLE PRIME */
	  || ch == 0x2034 /* TRIPLE PRIME */
	  || ch == 0x2035 /* REVERSED PRIME */
	  || ch == 0x2036 /* REVERSED DOUBLE PRIME */
	  || ch == 0x2037 /* REVERSED TRIPLE PRIME */
	  || ch == 0x20A7 /* PESETA SIGN */
	  || ch == 0x2103 /* DEGREE CELSIUS */
	  || ch == 0x2109 /* DEGREE FAHRENHEIT */
	  || ch == 0x2126 /* OHM SIGN */
	  || ch == 0xFE6A /* SMALL PERCENT SIGN */
	  || ch == 0xFF05 /* FULLWIDTH PERCENT SIGN */
	  || ch == 0xFFE0 /* FULLWIDTH DIGIT ZERO */)
	attr |= 1 << LBP_PO;

      /* prefix (numeric) */
      if (ch == 0x002B /* PLUS SIGN */
	  || ch == 0x005C /* REVERSE SOLIDUS */
	  || ch == 0x00B1 /* PLUS-MINUS SIGN */
	  || ch == 0x2116 /* NUMERO SIGN */
	  || ch == 0x2212 /* MINUS SIGN */
	  || ch == 0x2213 /* MINUS-OR-PLUS SIGN */
	  || (unicode_attributes[ch].category[0] == 'S'
	      && unicode_attributes[ch].category[1] == 'c'))
	if (!(attr & (1 << LBP_PO)))
	  attr |= 1 << LBP_PR;

      /* complex context (South East Asian) */
      if (((ch >= 0x0E00 && ch <= 0x0EFF)
	   || (ch >= 0x1000 && ch <= 0x109F)
	   || (ch >= 0x1780 && ch <= 0x17FF))
	  && unicode_attributes[ch].category[0] == 'L'
	  && (unicode_attributes[ch].category[1] == 'm'
	      || unicode_attributes[ch].category[1] == 'o'))
	if (!(attr & ((1 << LBP_CM) | (1 << LBP_NS) | (1 << LBP_NU) | (1 << LBP_BA) | (1 << LBP_PR))))
	  attr |= 1 << LBP_SA;

      /* ideographic */
      if ((ch >= 0x1100 && ch <= 0x115F) /* HANGUL CHOSEONG */
	  || (ch >= 0x2E80 && ch <= 0x2FFF) /* CJK RADICAL, KANGXI RADICAL, IDEOGRAPHIC DESCRIPTION */
	  || ch == 0x3000 /* IDEOGRAPHIC SPACE */
	  || (ch >= 0x3130 && ch <= 0x318F) /* HANGUL LETTER */
	  || (ch >= 0x3400 && ch <= 0x4DBF) /* CJK Ideograph Extension A */
	  || (ch >= 0x4E00 && ch <= 0x9FAF) /* CJK Ideograph */
	  || (ch >= 0xF900 && ch <= 0xFAFF) /* CJK COMPATIBILITY IDEOGRAPH */
	  || (ch >= 0xAC00 && ch <= 0xD7AF) /* HANGUL SYLLABLE */
	  || (ch >= 0xA000 && ch <= 0xA48C) /* YI SYLLABLE */
	  || (ch >= 0xA490 && ch <= 0xA4C6) /* YI RADICAL */
	  || ch == 0xFE62 /* SMALL PLUS SIGN */
	  || ch == 0xFE63 /* SMALL HYPHEN-MINUS */
	  || ch == 0xFE64 /* SMALL LESS-THAN SIGN */
	  || ch == 0xFE65 /* SMALL GREATER-THAN SIGN */
	  || ch == 0xFE66 /* SMALL EQUALS SIGN */
	  || (ch >= 0xFF10 && ch <= 0xFF19) /* FULLWIDTH DIGIT */
	  || (ch >= 0x20000 && ch <= 0x2A6D6) /* CJK Ideograph Extension B */
	  || (ch >= 0x2F800 && ch <= 0x2FA1D) /* CJK COMPATIBILITY IDEOGRAPH */
	  || strstr (unicode_attributes[ch].name, "FULLWIDTH LATIN ") != NULL
	  || (ch >= 0x3000 && ch <= 0x33FF
	      && !(attr & ((1 << LBP_CM) | (1 << LBP_NS) | (1 << LBP_OP) | (1 << LBP_CL))))
	  /* Extra characters for compatibility with Unicode LineBreak.txt.  */
	  || ch == 0xFE30 /* PRESENTATION FORM FOR VERTICAL TWO DOT LEADER */
	  || ch == 0xFE31 /* PRESENTATION FORM FOR VERTICAL EM DASH */
	  || ch == 0xFE32 /* PRESENTATION FORM FOR VERTICAL EN DASH */
	  || ch == 0xFE33 /* PRESENTATION FORM FOR VERTICAL LOW LINE */
	  || ch == 0xFE34 /* PRESENTATION FORM FOR VERTICAL WAVY LOW LINE */
	  || ch == 0xFE49 /* DASHED OVERLINE */
	  || ch == 0xFE4A /* CENTRELINE OVERLINE */
	  || ch == 0xFE4B /* WAVY OVERLINE */
	  || ch == 0xFE4C /* DOUBLE WAVY OVERLINE */
	  || ch == 0xFE4D /* DASHED LOW LINE */
	  || ch == 0xFE4E /* CENTRELINE LOW LINE */
	  || ch == 0xFE4F /* WAVY LOW LINE */
	  || ch == 0xFE51 /* SMALL IDEOGRAPHIC COMMA */
	  || ch == 0xFE58 /* SMALL EM DASH */
	  || ch == 0xFE5F /* SMALL NUMBER SIGN */
	  || ch == 0xFE60 /* SMALL AMPERSAND */
	  || ch == 0xFE61 /* SMALL ASTERISK */
	  || ch == 0xFE68 /* SMALL REVERSE SOLIDUS */
	  || ch == 0xFE6B /* SMALL COMMERCIAL AT */
	  || ch == 0xFF02 /* FULLWIDTH QUOTATION MARK */
	  || ch == 0xFF03 /* FULLWIDTH NUMBER SIGN */
	  || ch == 0xFF06 /* FULLWIDTH AMPERSAND */
	  || ch == 0xFF07 /* FULLWIDTH APOSTROPHE */
	  || ch == 0xFF0A /* FULLWIDTH ASTERISK */
	  || ch == 0xFF0B /* FULLWIDTH PLUS SIGN */
	  || ch == 0xFF0D /* FULLWIDTH HYPHEN-MINUS */
	  || ch == 0xFF0F /* FULLWIDTH SOLIDUS */
	  || ch == 0xFF1C /* FULLWIDTH LESS-THAN SIGN */
	  || ch == 0xFF1D /* FULLWIDTH EQUALS SIGN */
	  || ch == 0xFF1E /* FULLWIDTH GREATER-THAN SIGN */
	  || ch == 0xFF20 /* FULLWIDTH COMMERCIAL AT */
	  || ch == 0xFF3C /* FULLWIDTH REVERSE SOLIDUS */
	  || ch == 0xFF3E /* FULLWIDTH CIRCUMFLEX ACCENT */
	  || ch == 0xFF3F /* FULLWIDTH LOW LINE */
	  || ch == 0xFF40 /* FULLWIDTH GRAVE ACCENT */
	  || ch == 0xFF5C /* FULLWIDTH VERTICAL LINE */
	  || ch == 0xFF5E /* FULLWIDTH TILDE */
	  || ch == 0xFFE2 /* FULLWIDTH NOT SIGN */
	  || ch == 0xFFE3 /* FULLWIDTH MACRON */
	  || ch == 0xFFE4) /* FULLWIDTH BROKEN BAR */
	{
	  /* ambiguous (ideograph) ? */
	  if (unicode_width[ch] != NULL
	      && unicode_width[ch][0] == 'A')
	    attr |= 1 << LBP_AI;
	  else
	    attr |= 1 << LBP_ID;
	}

      /* ordinary alphabetic and symbol characters */
      if ((unicode_attributes[ch].category[0] == 'L'
	   && (unicode_attributes[ch].category[1] == 'u'
	       || unicode_attributes[ch].category[1] == 'l'
	       || unicode_attributes[ch].category[1] == 't'
	       || unicode_attributes[ch].category[1] == 'm'
	       || unicode_attributes[ch].category[1] == 'o'))
	  || (unicode_attributes[ch].category[0] == 'S'
	      && (unicode_attributes[ch].category[1] == 'm'
		  || unicode_attributes[ch].category[1] == 'c'
		  || unicode_attributes[ch].category[1] == 'k'
		  || unicode_attributes[ch].category[1] == 'o'))
	  /* Extra characters for compatibility with Unicode LineBreak.txt.  */
	  || ch == 0x0023 /* NUMBER SIGN */
	  || ch == 0x0026 /* AMPERSAND */
	  || ch == 0x002A /* ASTERISK */
	  || ch == 0x0040 /* COMMERCIAL AT */
	  || ch == 0x005F /* LOW LINE */
	  || ch == 0x00A1 /* INVERTED EXCLAMATION MARK */
	  || ch == 0x00B2 /* SUPERSCRIPT TWO */
	  || ch == 0x00B3 /* SUPERSCRIPT THREE */
	  || ch == 0x00B7 /* MIDDLE DOT */
	  || ch == 0x00B9 /* SUPERSCRIPT ONE */
	  || ch == 0x00BC /* VULGAR FRACTION ONE QUARTER */
	  || ch == 0x00BD /* VULGAR FRACTION ONE HALF */
	  || ch == 0x00BE /* VULGAR FRACTION THREE QUARTERS */
	  || ch == 0x00BF /* INVERTED QUESTION MARK */
	  || ch == 0x037E /* GREEK QUESTION MARK */
	  || ch == 0x0387 /* GREEK ANO TELEIA */
	  || ch == 0x055A /* ARMENIAN APOSTROPHE */
	  || ch == 0x055B /* ARMENIAN EMPHASIS MARK */
	  || ch == 0x055C /* ARMENIAN EXCLAMATION MARK */
	  || ch == 0x055D /* ARMENIAN COMMA */
	  || ch == 0x055E /* ARMENIAN QUESTION MARK */
	  || ch == 0x055F /* ARMENIAN ABBREVIATION MARK */
	  || ch == 0x05BE /* HEBREW PUNCTUATION MAQAF */
	  || ch == 0x05C0 /* HEBREW PUNCTUATION PASEQ */
	  || ch == 0x05C3 /* HEBREW PUNCTUATION SOF PASUQ */
	  || ch == 0x05F3 /* HEBREW PUNCTUATION GERESH */
	  || ch == 0x05F4 /* HEBREW PUNCTUATION GERSHAYIM */
	  || ch == 0x060C /* ARABIC COMMA */
	  || ch == 0x061B /* ARABIC SEMICOLON */
	  || ch == 0x061F /* ARABIC QUESTION MARK */
	  || ch == 0x066A /* ARABIC PERCENT SIGN */
	  || ch == 0x066B /* ARABIC DECIMAL SEPARATOR */
	  || ch == 0x066C /* ARABIC THOUSANDS SEPARATOR */
	  || ch == 0x066D /* ARABIC FIVE POINTED STAR */
	  || ch == 0x06D4 /* ARABIC FULL STOP */
	  || ch == 0x0700 /* SYRIAC END OF PARAGRAPH */
	  || ch == 0x0701 /* SYRIAC SUPRALINEAR FULL STOP */
	  || ch == 0x0702 /* SYRIAC SUBLINEAR FULL STOP */
	  || ch == 0x0703 /* SYRIAC SUPRALINEAR COLON */
	  || ch == 0x0704 /* SYRIAC SUBLINEAR COLON */
	  || ch == 0x0705 /* SYRIAC HORIZONTAL COLON */
	  || ch == 0x0706 /* SYRIAC COLON SKEWED LEFT */
	  || ch == 0x0707 /* SYRIAC COLON SKEWED RIGHT */
	  || ch == 0x0708 /* SYRIAC SUPRALINEAR COLON SKEWED LEFT */
	  || ch == 0x0709 /* SYRIAC SUBLINEAR COLON SKEWED RIGHT */
	  || ch == 0x070A /* SYRIAC CONTRACTION */
	  || ch == 0x070B /* SYRIAC HARKLEAN OBELUS */
	  || ch == 0x070C /* SYRIAC HARKLEAN METOBELUS */
	  || ch == 0x070D /* SYRIAC HARKLEAN ASTERISCUS */
	  || ch == 0x0964 /* DEVANAGARI DANDA */
	  || ch == 0x0965 /* DEVANAGARI DOUBLE DANDA */
	  || ch == 0x0970 /* DEVANAGARI ABBREVIATION SIGN */
	  || ch == 0x09F4 /* BENGALI CURRENCY NUMERATOR ONE */
	  || ch == 0x09F5 /* BENGALI CURRENCY NUMERATOR TWO */
	  || ch == 0x09F6 /* BENGALI CURRENCY NUMERATOR THREE */
	  || ch == 0x09F7 /* BENGALI CURRENCY NUMERATOR FOUR */
	  || ch == 0x09F8 /* BENGALI CURRENCY NUMERATOR ONE LESS THAN THE DENOMINATOR */
	  || ch == 0x09F9 /* BENGALI CURRENCY DENOMINATOR SIXTEEN */
	  || ch == 0x0BF0 /* TAMIL NUMBER TEN */
	  || ch == 0x0BF1 /* TAMIL NUMBER ONE HUNDRED */
	  || ch == 0x0BF2 /* TAMIL NUMBER ONE THOUSAND */
	  || ch == 0x0DF4 /* SINHALA PUNCTUATION KUNDDALIYA */
	  || ch == 0x0E4F /* THAI CHARACTER FONGMAN */
	  || ch == 0x0F04 /* TIBETAN MARK INITIAL YIG MGO MDUN MA */
	  || ch == 0x0F05 /* TIBETAN MARK CLOSING YIG MGO SGAB MA */
	  || ch == 0x0F06 /* TIBETAN MARK CARET YIG MGO PHUR SHAD MA */
	  || ch == 0x0F07 /* TIBETAN MARK YIG MGO TSHEG SHAD MA */
	  || ch == 0x0F08 /* TIBETAN MARK SBRUL SHAD */
	  || ch == 0x0F09 /* TIBETAN MARK BSKUR YIG MGO */
	  || ch == 0x0F0A /* TIBETAN MARK BKA- SHOG YIG MGO */
	  || ch == 0x0F0D /* TIBETAN MARK SHAD */
	  || ch == 0x0F0E /* TIBETAN MARK NYIS SHAD */
	  || ch == 0x0F0F /* TIBETAN MARK TSHEG SHAD */
	  || ch == 0x0F10 /* TIBETAN MARK NYIS TSHEG SHAD */
	  || ch == 0x0F11 /* TIBETAN MARK RIN CHEN SPUNGS SHAD */
	  || ch == 0x0F12 /* TIBETAN MARK RGYA GRAM SHAD */
	  || ch == 0x0F2A /* TIBETAN DIGIT HALF ONE */
	  || ch == 0x0F2B /* TIBETAN DIGIT HALF TWO */
	  || ch == 0x0F2C /* TIBETAN DIGIT HALF THREE */
	  || ch == 0x0F2D /* TIBETAN DIGIT HALF FOUR */
	  || ch == 0x0F2E /* TIBETAN DIGIT HALF FIVE */
	  || ch == 0x0F2F /* TIBETAN DIGIT HALF SIX */
	  || ch == 0x0F30 /* TIBETAN DIGIT HALF SEVEN */
	  || ch == 0x0F31 /* TIBETAN DIGIT HALF EIGHT */
	  || ch == 0x0F32 /* TIBETAN DIGIT HALF NINE */
	  || ch == 0x0F33 /* TIBETAN DIGIT HALF ZERO */
	  || ch == 0x0F85 /* TIBETAN MARK PALUTA */
	  || ch == 0x104A /* MYANMAR SIGN LITTLE SECTION */
	  || ch == 0x104B /* MYANMAR SIGN SECTION */
	  || ch == 0x104C /* MYANMAR SYMBOL LOCATIVE */
	  || ch == 0x104D /* MYANMAR SYMBOL COMPLETED */
	  || ch == 0x104E /* MYANMAR SYMBOL AFOREMENTIONED */
	  || ch == 0x104F /* MYANMAR SYMBOL GENITIVE */
	  || ch == 0x10FB /* GEORGIAN PARAGRAPH SEPARATOR */
	  || ch == 0x1362 /* ETHIOPIC FULL STOP */
	  || ch == 0x1363 /* ETHIOPIC COMMA */
	  || ch == 0x1364 /* ETHIOPIC SEMICOLON */
	  || ch == 0x1365 /* ETHIOPIC COLON */
	  || ch == 0x1366 /* ETHIOPIC PREFACE COLON */
	  || ch == 0x1367 /* ETHIOPIC QUESTION MARK */
	  || ch == 0x1368 /* ETHIOPIC PARAGRAPH SEPARATOR */
	  || ch == 0x1372 /* ETHIOPIC NUMBER TEN */
	  || ch == 0x1373 /* ETHIOPIC NUMBER TWENTY */
	  || ch == 0x1374 /* ETHIOPIC NUMBER THIRTY */
	  || ch == 0x1375 /* ETHIOPIC NUMBER FORTY */
	  || ch == 0x1376 /* ETHIOPIC NUMBER FIFTY */
	  || ch == 0x1377 /* ETHIOPIC NUMBER SIXTY */
	  || ch == 0x1378 /* ETHIOPIC NUMBER SEVENTY */
	  || ch == 0x1379 /* ETHIOPIC NUMBER EIGHTY */
	  || ch == 0x137A /* ETHIOPIC NUMBER NINETY */
	  || ch == 0x137B /* ETHIOPIC NUMBER HUNDRED */
	  || ch == 0x137C /* ETHIOPIC NUMBER TEN THOUSAND */
	  || ch == 0x166D /* CANADIAN SYLLABICS CHI SIGN */
	  || ch == 0x166E /* CANADIAN SYLLABICS FULL STOP */
	  || ch == 0x16EB /* RUNIC SINGLE PUNCTUATION */
	  || ch == 0x16EC /* RUNIC MULTIPLE PUNCTUATION */
	  || ch == 0x16ED /* RUNIC CROSS PUNCTUATION */
	  || ch == 0x16EE /* RUNIC ARLAUG SYMBOL */
	  || ch == 0x16EF /* RUNIC TVIMADUR SYMBOL */
	  || ch == 0x16F0 /* RUNIC BELGTHOR SYMBOL */
	  || ch == 0x17DC /* KHMER SIGN AVAKRAHASANYA */
	  || ch == 0x1800 /* MONGOLIAN BIRGA */
	  || ch == 0x1801 /* MONGOLIAN ELLIPSIS */
	  || ch == 0x1802 /* MONGOLIAN COMMA */
	  || ch == 0x1803 /* MONGOLIAN FULL STOP */
	  || ch == 0x1804 /* MONGOLIAN COLON */
	  || ch == 0x1805 /* MONGOLIAN FOUR DOTS */
	  || ch == 0x1807 /* MONGOLIAN SIBE SYLLABLE BOUNDARY MARKER */
	  || ch == 0x1808 /* MONGOLIAN MANCHU COMMA */
	  || ch == 0x1809 /* MONGOLIAN MANCHU FULL STOP */
	  || ch == 0x180A /* MONGOLIAN NIRUGU */
	  || ch == 0x2015 /* HORIZONTAL BAR */
	  || ch == 0x2016 /* DOUBLE VERTICAL LINE */
	  || ch == 0x2017 /* DOUBLE LOW LINE */
	  || ch == 0x2020 /* DAGGER */
	  || ch == 0x2021 /* DOUBLE DAGGER */
	  || ch == 0x2022 /* BULLET */
	  || ch == 0x2023 /* TRIANGULAR BULLET */
	  || ch == 0x2038 /* CARET */
	  || ch == 0x203B /* REFERENCE MARK */
	  || ch == 0x203D /* INTERROBANG */
	  || ch == 0x203E /* OVERLINE */
	  || ch == 0x203F /* UNDERTIE */
	  || ch == 0x2040 /* CHARACTER TIE */
	  || ch == 0x2041 /* CARET INSERTION POINT */
	  || ch == 0x2042 /* ASTERISM */
	  || ch == 0x2043 /* HYPHEN BULLET */
	  || ch == 0x2048 /* QUESTION EXCLAMATION MARK */
	  || ch == 0x2049 /* EXCLAMATION QUESTION MARK */
	  || ch == 0x204A /* TIRONIAN SIGN ET */
	  || ch == 0x204B /* REVERSED PILCROW SIGN */
	  || ch == 0x204C /* BLACK LEFTWARDS BULLET */
	  || ch == 0x204D /* BLACK RIGHTWARDS BULLET */
	  || ch == 0x2070 /* SUPERSCRIPT ZERO */
	  || ch == 0x2074 /* SUPERSCRIPT FOUR */
	  || ch == 0x2075 /* SUPERSCRIPT FIVE */
	  || ch == 0x2076 /* SUPERSCRIPT SIX */
	  || ch == 0x2077 /* SUPERSCRIPT SEVEN */
	  || ch == 0x2078 /* SUPERSCRIPT EIGHT */
	  || ch == 0x2079 /* SUPERSCRIPT NINE */
	  || ch == 0x2080 /* SUBSCRIPT ZERO */
	  || ch == 0x2081 /* SUBSCRIPT ONE */
	  || ch == 0x2082 /* SUBSCRIPT TWO */
	  || ch == 0x2083 /* SUBSCRIPT THREE */
	  || ch == 0x2084 /* SUBSCRIPT FOUR */
	  || ch == 0x2085 /* SUBSCRIPT FIVE */
	  || ch == 0x2086 /* SUBSCRIPT SIX */
	  || ch == 0x2087 /* SUBSCRIPT SEVEN */
	  || ch == 0x2088 /* SUBSCRIPT EIGHT */
	  || ch == 0x2089 /* SUBSCRIPT NINE */
	  || (ch >= 0x2153 && ch <= 0x215E) /* VULGAR FRACTION */
	  || ch == 0x215F /* FRACTION NUMERATOR ONE */
	  || (ch >= 0x2160 && ch <= 0x2183) /* ROMAN NUMERAL */
	  || (ch >= 0x2460 && ch <= 0x2473) /* CIRCLED NUMBER */
	  || (ch >= 0x2474 && ch <= 0x2487) /* PARENTHESIZED NUMBER */
	  || (ch >= 0x2488 && ch <= 0x249B) /* NUMBER FULL STOP */
	  || ch == 0x24EA /* CIRCLED DIGIT ZERO */
	  || (ch >= 0x2776 && ch <= 0x2793) /* DINGBAT CIRCLED DIGIT */
	  || ch == 0x10320 /* OLD ITALIC NUMERAL ONE */
	  || ch == 0x10321 /* OLD ITALIC NUMERAL FIVE */
	  || ch == 0x10322 /* OLD ITALIC NUMERAL TEN */
	  || ch == 0x10323 /* OLD ITALIC NUMERAL FIFTY */
	  || ch == 0x1034A) /* GOTHIC LETTER NINE HUNDRED */
	if (!(attr & ((1 << LBP_CM) | (1 << LBP_NS) | (1 << LBP_ID) | (1 << LBP_BA) | (1 << LBP_BB) | (1 << LBP_PO) | (1 << LBP_PR) | (1 << LBP_SA) | (1 << LBP_CB))))
	  {
	    /* ambiguous (alphabetic) ? */
	    if (unicode_width[ch] != NULL
		&& unicode_width[ch][0] == 'A')
	      attr |= 1 << LBP_AI;
	    else
	      attr |= 1 << LBP_AL;
	  }
    }

  if (attr == 0)
    /* unknown */
    attr |= 1 << LBP_XX;

  return attr;
}

/* Output the line breaking properties in a human readable format.  */
static void
debug_output_lbp (FILE *stream)
{
  unsigned int i;

  for (i = 0; i < 0x110000; i++)
    {
      int attr = get_lbp (i);
      if (attr != 1 << LBP_XX)
	{
	  fprintf (stream, "0x%04X", i);
#define PRINT_BIT(attr,bit) \
  if (attr & (1 << bit)) fprintf (stream, " " #bit);
	  PRINT_BIT(attr,LBP_BK);
	  PRINT_BIT(attr,LBP_CM);
	  PRINT_BIT(attr,LBP_ZW);
	  PRINT_BIT(attr,LBP_IN);
	  PRINT_BIT(attr,LBP_GL);
	  PRINT_BIT(attr,LBP_CB);
	  PRINT_BIT(attr,LBP_SP);
	  PRINT_BIT(attr,LBP_BA);
	  PRINT_BIT(attr,LBP_BB);
	  PRINT_BIT(attr,LBP_B2);
	  PRINT_BIT(attr,LBP_HY);
	  PRINT_BIT(attr,LBP_NS);
	  PRINT_BIT(attr,LBP_OP);
	  PRINT_BIT(attr,LBP_CL);
	  PRINT_BIT(attr,LBP_QU);
	  PRINT_BIT(attr,LBP_EX);
	  PRINT_BIT(attr,LBP_ID);
	  PRINT_BIT(attr,LBP_NU);
	  PRINT_BIT(attr,LBP_IS);
	  PRINT_BIT(attr,LBP_SY);
	  PRINT_BIT(attr,LBP_AL);
	  PRINT_BIT(attr,LBP_PR);
	  PRINT_BIT(attr,LBP_PO);
	  PRINT_BIT(attr,LBP_SA);
	  PRINT_BIT(attr,LBP_XX);
	  PRINT_BIT(attr,LBP_AI);
#undef PRINT_BIT
	  fprintf (stream, "\n");
	}
    }
}

static void
debug_output_tables (const char *filename)
{
  FILE *stream;

  stream = fopen (filename, "w");
  if (stream == NULL)
    {
      fprintf (stderr, "cannot open '%s' for writing\n", filename);
      exit (1);
    }

  debug_output_lbp (stream);

  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error writing to '%s'\n", filename);
      exit (1);
    }
}

/* The line breaking property from the LineBreak.txt file.  */
int unicode_org_lbp[0x110000];

/* Stores in unicode_org_lbp[] the line breaking property from the
   LineBreak.txt file.  */
static void
fill_org_lbp (const char *linebreak_filename)
{
  unsigned int i, j;
  FILE *stream;
  char field0[FIELDLEN];
  char field1[FIELDLEN];
  char field2[FIELDLEN];
  int lineno = 0;

  for (i = 0; i < 0x110000; i++)
    unicode_org_lbp[i] = LBP_XX;

  stream = fopen (linebreak_filename, "r");
  if (stream == NULL)
    {
      fprintf (stderr, "error during fopen of '%s'\n", linebreak_filename);
      exit (1);
    }

  for (;;)
    {
      int n;
      int c;
      int value;

      lineno++;
      c = getc (stream);
      if (c == EOF)
	break;
      if (c == '#')
	{
	  do c = getc (stream); while (c != EOF && c != '\n');
	  continue;
	}
      ungetc (c, stream);
      n = getfield (stream, field0, ';');
      n += getfield (stream, field1, ' ');
      n += getfield (stream, field2, '\n');
      if (n == 0)
	break;
      if (n != 3)
	{
	  fprintf (stderr, "short line in '%s':%d\n", linebreak_filename,
		   lineno);
	  exit (1);
	}
#define TRY(bit) else if (strcmp (field1, #bit + 4) == 0) value = bit;
      if (false) {}
      TRY(LBP_BK)
      TRY(LBP_CM)
      TRY(LBP_ZW)
      TRY(LBP_IN)
      TRY(LBP_GL)
      TRY(LBP_CB)
      TRY(LBP_SP)
      TRY(LBP_BA)
      TRY(LBP_BB)
      TRY(LBP_B2)
      TRY(LBP_HY)
      TRY(LBP_NS)
      TRY(LBP_OP)
      TRY(LBP_CL)
      TRY(LBP_QU)
      TRY(LBP_EX)
      TRY(LBP_ID)
      TRY(LBP_NU)
      TRY(LBP_IS)
      TRY(LBP_SY)
      TRY(LBP_AL)
      TRY(LBP_PR)
      TRY(LBP_PO)
      TRY(LBP_SA)
      TRY(LBP_XX)
      TRY(LBP_AI)
#undef TRY
      else if (strcmp (field1, "LF") == 0) value = LBP_BK;
      else if (strcmp (field1, "CR") == 0) value = LBP_BK;
      else if (strcmp (field1, "SG") == 0) value = LBP_XX;
      else
	{
	  fprintf (stderr, "unknown property value \"%s\" in '%s':%d\n",
		   field1, linebreak_filename, lineno);
	  exit (1);
	}
      i = strtoul (field0, NULL, 16);
      if (strstr (field0, "..") != NULL)
	{
	  /* Deal with a range.  */
	  j = strtoul (strstr (field0, "..") + 2, NULL, 16);
	  for (; i <= j; i++)
	    unicode_org_lbp[i] = value;
	}
      else
	{
	  /* Single character line.  */
	  unicode_org_lbp[i] = value;
	}
    }
  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error reading from '%s'\n", linebreak_filename);
      exit (1);
    }
}

/* Output the line breaking properties in a human readable format.  */
static void
debug_output_org_lbp (FILE *stream)
{
  unsigned int i;

  for (i = 0; i < 0x110000; i++)
    {
      int attr = unicode_org_lbp[i];
      if (attr != LBP_XX)
	{
	  fprintf (stream, "0x%04X", i);
#define PRINT_BIT(attr,bit) \
  if (attr == bit) fprintf (stream, " " #bit);
	  PRINT_BIT(attr,LBP_BK);
	  PRINT_BIT(attr,LBP_CM);
	  PRINT_BIT(attr,LBP_ZW);
	  PRINT_BIT(attr,LBP_IN);
	  PRINT_BIT(attr,LBP_GL);
	  PRINT_BIT(attr,LBP_CB);
	  PRINT_BIT(attr,LBP_SP);
	  PRINT_BIT(attr,LBP_BA);
	  PRINT_BIT(attr,LBP_BB);
	  PRINT_BIT(attr,LBP_B2);
	  PRINT_BIT(attr,LBP_HY);
	  PRINT_BIT(attr,LBP_NS);
	  PRINT_BIT(attr,LBP_OP);
	  PRINT_BIT(attr,LBP_CL);
	  PRINT_BIT(attr,LBP_QU);
	  PRINT_BIT(attr,LBP_EX);
	  PRINT_BIT(attr,LBP_ID);
	  PRINT_BIT(attr,LBP_NU);
	  PRINT_BIT(attr,LBP_IS);
	  PRINT_BIT(attr,LBP_SY);
	  PRINT_BIT(attr,LBP_AL);
	  PRINT_BIT(attr,LBP_PR);
	  PRINT_BIT(attr,LBP_PO);
	  PRINT_BIT(attr,LBP_SA);
	  PRINT_BIT(attr,LBP_XX);
	  PRINT_BIT(attr,LBP_AI);
#undef PRINT_BIT
	  fprintf (stream, "\n");
	}
    }
}

static void
debug_output_org_tables (const char *filename)
{
  FILE *stream;

  stream = fopen (filename, "w");
  if (stream == NULL)
    {
      fprintf (stderr, "cannot open '%s' for writing\n", filename);
      exit (1);
    }

  debug_output_org_lbp (stream);

  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error writing to '%s'\n", filename);
      exit (1);
    }
}

/* Construction of sparse 3-level tables.  */
#define TABLE lbp_table
#define ELEMENT unsigned char
#define DEFAULT LBP_XX
#define xmalloc malloc
#define xrealloc realloc
#include "3level.h"

static void
output_lbp (FILE *stream)
{
  unsigned int i;
  struct lbp_table t;
  unsigned int level1_offset, level2_offset, level3_offset;

  t.p = 7;
  t.q = 9;
  lbp_table_init (&t);

  for (i = 0; i < 0x110000; i++)
    {
      int attr = get_lbp (i);

      /* Now attr should contain exactly one bit.  */
      if (attr == 0 || ((attr & (attr - 1)) != 0))
	abort ();

      if (attr != 1 << LBP_XX)
	{
	  unsigned int log2_attr;
	  for (log2_attr = 0; attr > 1; attr >>= 1, log2_attr++);

	  lbp_table_add (&t, i, log2_attr);
	}
    }

  lbp_table_finalize (&t);

  level1_offset =
    5 * sizeof (uint32_t);
  level2_offset =
    5 * sizeof (uint32_t)
    + t.level1_size * sizeof (uint32_t);
  level3_offset =
    5 * sizeof (uint32_t)
    + t.level1_size * sizeof (uint32_t)
    + (t.level2_size << t.q) * sizeof (uint32_t);

  for (i = 0; i < 5; i++)
    fprintf (stream, "#define lbrkprop_header_%d %d\n", i,
	     ((uint32_t *) t.result)[i]);
  fprintf (stream, "static const\n");
  fprintf (stream, "struct\n");
  fprintf (stream, "  {\n");
  fprintf (stream, "    int level1[%d];\n", t.level1_size);
  fprintf (stream, "    int level2[%d << %d];\n", t.level2_size, t.q);
  fprintf (stream, "    unsigned char level3[%d << %d];\n", t.level3_size, t.p);
  fprintf (stream, "  }\n");
  fprintf (stream, "lbrkprop =\n");
  fprintf (stream, "{\n");
  fprintf (stream, "  {");
  for (i = 0; i < t.level1_size; i++)
    {
      uint32_t offset;
      if (i > 0 && (i % 8) == 0)
	fprintf (stream, "\n   ");
      offset = ((uint32_t *) (t.result + level1_offset))[i];
      fprintf (stream, " %5d%s",
	       offset == 0 ? -1 : (offset - level2_offset) / sizeof (uint32_t),
	       (i+1 < t.level1_size ? "," : ""));
    }
  fprintf (stream, " },\n");
  fprintf (stream, "  {");
  if (t.level2_size << t.q > 8)
    fprintf (stream, "\n   ");
  for (i = 0; i < t.level2_size << t.q; i++)
    {
      uint32_t offset;
      if (i > 0 && (i % 8) == 0)
	fprintf (stream, "\n   ");
      offset = ((uint32_t *) (t.result + level2_offset))[i];
      fprintf (stream, " %5d%s",
	       offset == 0 ? -1 : (offset - level3_offset) / sizeof (uint8_t),
	       (i+1 < t.level2_size << t.q ? "," : ""));
    }
  if (t.level2_size << t.q > 8)
    fprintf (stream, "\n ");
  fprintf (stream, " },\n");
  fprintf (stream, "  {");
  if (t.level3_size << t.p > 8)
    fprintf (stream, "\n   ");
  for (i = 0; i < t.level3_size << t.p; i++)
    {
      unsigned char value = ((unsigned char *) (t.result + level3_offset))[i];
      const char *value_string;
      switch (value)
	{
#define CASE(x) case x: value_string = #x; break;
	  CASE(LBP_BK);
	  CASE(LBP_CM);
	  CASE(LBP_ZW);
	  CASE(LBP_IN);
	  CASE(LBP_GL);
	  CASE(LBP_CB);
	  CASE(LBP_SP);
	  CASE(LBP_BA);
	  CASE(LBP_BB);
	  CASE(LBP_B2);
	  CASE(LBP_HY);
	  CASE(LBP_NS);
	  CASE(LBP_OP);
	  CASE(LBP_CL);
	  CASE(LBP_QU);
	  CASE(LBP_EX);
	  CASE(LBP_ID);
	  CASE(LBP_NU);
	  CASE(LBP_IS);
	  CASE(LBP_SY);
	  CASE(LBP_AL);
	  CASE(LBP_PR);
	  CASE(LBP_PO);
	  CASE(LBP_SA);
	  CASE(LBP_XX);
	  CASE(LBP_AI);
#undef CASE
	  default:
	    abort ();
	}
      if (i > 0 && (i % 8) == 0)
	fprintf (stream, "\n   ");
      fprintf (stream, " %s%s", value_string,
	       (i+1 < t.level3_size << t.p ? "," : ""));
    }
  if (t.level3_size << t.p > 8)
    fprintf (stream, "\n ");
  fprintf (stream, " }\n");
  fprintf (stream, "};\n");
}

static void
output_tables (const char *filename, const char *version)
{
  FILE *stream;

  stream = fopen (filename, "w");
  if (stream == NULL)
    {
      fprintf (stderr, "cannot open '%s' for writing\n", filename);
      exit (1);
    }

  fprintf (stream, "/* Line breaking properties of Unicode characters.  */\n");
  fprintf (stream, "/* Generated automatically by gen-lbrkprop for Unicode %s.  */\n",
	   version);
  fprintf (stream, "\n");

  /* Put a GPL header on it.  The gnulib module is under LGPL (although it
     still carries the GPL header), and it's gnulib-tool which replaces the
     GPL header with an LGPL header.  */
  fprintf (stream, "/* Copyright (C) 2000-2004 Free Software Foundation, Inc.\n");
  fprintf (stream, "\n");
  fprintf (stream, "This program is free software; you can redistribute it and/or modify\n");
  fprintf (stream, "it under the terms of the GNU General Public License as published by\n");
  fprintf (stream, "the Free Software Foundation; either version 2, or (at your option)\n");
  fprintf (stream, "any later version.\n");
  fprintf (stream, "\n");
  fprintf (stream, "This program is distributed in the hope that it will be useful,\n");
  fprintf (stream, "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  fprintf (stream, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
  fprintf (stream, "GNU General Public License for more details.\n");
  fprintf (stream, "\n");
  fprintf (stream, "You should have received a copy of the GNU General Public License\n");
  fprintf (stream, "along with this program; if not, write to the Free Software\n");
  fprintf (stream, "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */\n");
  fprintf (stream, "\n");

  output_lbp (stream);

  if (ferror (stream) || fclose (stream))
    {
      fprintf (stderr, "error writing to '%s'\n", filename);
      exit (1);
    }
}

int
main (int argc, char * argv[])
{
  if (argc != 6)
    {
      fprintf (stderr, "Usage: %s UnicodeData.txt Combining.txt EastAsianWidth.txt LineBreak.txt version\n",
	       argv[0]);
      exit (1);
    }

  fill_attributes (argv[1]);
  fill_combining (argv[2]);
  fill_width (argv[3]);
  fill_org_lbp (argv[4]);

  debug_output_tables ("lbrkprop.txt");
  debug_output_org_tables ("lbrkprop_org.txt");

  output_tables ("lbrkprop.h", argv[5]);

  return 0;
}
