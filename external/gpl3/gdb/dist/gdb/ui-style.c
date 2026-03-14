/* Styling for ui_file
   Copyright (C) 2018-2025 Free Software Foundation, Inc.

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

#include "ui-style.h"
#include "gdb_curses.h"
#include "gdbsupport/gdb_regex.h"

/* A regular expression that is used for matching ANSI terminal escape
   sequences.  Note that this will actually match any prefix of such a
   sequence.  This property is used so that other code can buffer
   incomplete sequences as needed.  */

static const char ansi_regex_text[] =
  /* Introduction.  Only the escape character is truly required.  */
  "^\033(\\["
#define DATA_SUBEXP 2
  /* Capture parameter and intermediate bytes.  */
  "("
  /* Parameter bytes.  */
  "[\x30-\x3f]*"
  /* Intermediate bytes.  */
  "[\x20-\x2f]*"
  /* End the first capture.  */
  ")"
  /* The final byte.  */
#define FINAL_SUBEXP 3
  "([\x40-\x7e]))?";

/* The number of subexpressions to allocate space for, including the
   "0th" whole match subexpression.  */
#define NUM_SUBEXPRESSIONS 4

/* The compiled form of ansi_regex_text.  */

static compiled_regex ansi_regex (ansi_regex_text, REG_EXTENDED,
  _("Error in ANSI terminal escape sequences regex"));

/* This maps 8-color palette to RGB triples.  The values come from
   plain linux terminal.  */

static const uint8_t palette_8colors[][3] = {
  { 1, 1, 1 },			/* Black.  */
  { 222, 56, 43 },		/* Red.  */
  { 57, 181, 74 },		/* Green.  */
  { 255, 199, 6 },		/* Yellow.  */
  { 0, 111, 184 },		/* Blue.  */
  { 118, 38, 113 },		/* Magenta.  */
  { 44, 181, 233 },		/* Cyan.  */
  { 204, 204, 204 },		/* White.  */
};

/* This maps 16-color palette to RGB triples.  The values come from xterm.  */

static const uint8_t palette_16colors[][3] = {
  { 0, 0, 0 },			/* Black.  */
  { 205, 0, 0 },		/* Red.  */
  { 0, 205, 0 },		/* Green.  */
  { 205, 205, 0 },		/* Yellow.  */
  { 0, 0, 238 },		/* Blue.  */
  { 205, 0, 205 },		/* Magenta.  */
  { 0, 205, 205 },		/* Cyan.  */
  { 229, 229, 229 },		/* White.  */
  { 127, 127, 127 },		/* Bright Black.  */
  { 255, 0, 0 },		/* Bright Red.  */
  { 0, 255, 0 },		/* Bright Green.  */
  { 255, 255, 0 },		/* Bright Yellow.  */
  { 92, 92, 255 },		/* Bright Blue.  */
  { 255, 0, 255 },		/* Bright Magenta.  */
  { 0, 255, 255 },		/* Bright Cyan.  */
  { 255, 255, 255 }		/* Bright White.  */
};

/* See ui-style.h.  */
/* Must correspond to ui_file_style::basic_color.  */
const std::vector<const char *> ui_file_style::basic_color_enums = {
  "none",
  "black",
  "red",
  "green",
  "yellow",
  "blue",
  "magenta",
  "cyan",
  "white",
  nullptr
};

/* Returns text representation of a basic COLOR.  */

static const char *
basic_color_name (int color)
{
  int pos = color - ui_file_style::NONE;
  if (0 <= pos && pos < ui_file_style::basic_color_enums.size ())
    if (const char *s = ui_file_style::basic_color_enums[pos])
      return s;
  error (_("Basic color %d has no name."), color);
}

/* See ui-style.h.  */

void
ui_file_style::color::append_ansi (bool is_fg, std::string *str) const
{
  if (m_color_space == color_space::MONOCHROME)
    str->append (is_fg ? "39" : "49");
  else if (is_basic ())
    str->append (std::to_string (m_value + (is_fg ? 30 : 40)));
  else if (m_color_space == color_space::AIXTERM_16COLOR)
    str->append (std::to_string (m_value - WHITE - 1 + (is_fg ? 90 : 100)));
  else if (m_color_space == color_space::XTERM_256COLOR)
    str->append (is_fg ? "38;5;" : "48;5;").append (std::to_string (m_value));
  else if (m_color_space == color_space::RGB_24BIT)
    {
      // See ISO/IEC 8613-6 (or ITU T.416) 13.1.8 Select Graphic Rendition (SGR)
      str->append (is_fg ? "38;2;" : "48;2;");
      str->append (std::to_string (m_red)
		   + ";" + std::to_string (m_green)
		   + ";" + std::to_string (m_blue));
    }
  else
    gdb_assert_not_reached ("no valid ansi representation of the color");
}

/* See ui-style.h.  */
std::string
ui_file_style::color::to_ansi (bool is_fg) const
{
  std::string s = "\033[";
  append_ansi (is_fg, &s);
  s.push_back ('m');
  return s;
}

/* See ui-style.h.  */

std::string
ui_file_style::color::to_string () const
{
  if (m_color_space == color_space::RGB_24BIT)
    {
      char s[64];
      snprintf (s, sizeof s, "#%02X%02X%02X", m_red, m_green, m_blue);
      return s;
    }
  else if (is_none () || is_basic ())
    return basic_color_name (m_value);
  else
    return std::to_string (get_value ());
}

/* See ui-style.h.  */

void
ui_file_style::color::get_rgb (uint8_t *rgb) const
{
  if (m_color_space == color_space::RGB_24BIT)
    {
      rgb[0] = m_red;
      rgb[1] = m_green;
      rgb[2] = m_blue;
    }
  else if (m_color_space == color_space::ANSI_8COLOR
	   && 0 <= m_value && m_value <= 7)
    memcpy (rgb, palette_8colors[m_value], 3 * sizeof (uint8_t));
  else if (m_color_space == color_space::AIXTERM_16COLOR
	   && 0 <= m_value && m_value <= 15)
    memcpy (rgb, palette_16colors[m_value], 3 * sizeof (uint8_t));
  else if (m_color_space != color_space::XTERM_256COLOR)
    gdb_assert_not_reached ("get_rgb called on invalid color");
  else if (0 <= m_value && m_value <= 15)
    memcpy (rgb, palette_16colors[m_value], 3 * sizeof (uint8_t));
  else if (m_value >= 16 && m_value <= 231)
    {
      int value = m_value;
      value -= 16;
      /* This obscure formula seems to be what terminals actually
	 do.  */
      int component = value / 36;
      rgb[0] = component == 0 ? 0 : (55 + component * 40);
      value %= 36;
      component = value / 6;
      rgb[1] = component == 0 ? 0 : (55 + component * 40);
      value %= 6;
      rgb[2] = value == 0 ? 0 : (55 + value * 40);
    }
  else if (232 <= m_value && m_value <= 255)
    {
      uint8_t v = (m_value - 232) * 10 + 8;
      rgb[0] = v;
      rgb[1] = v;
      rgb[2] = v;
    }
  else
    gdb_assert_not_reached ("get_rgb called on invalid color");
}

/* See ui-style.h.  */

ui_file_style::color
ui_file_style::color::approximate (const std::vector<color_space> &spaces) const
{
  if (spaces.empty () || is_none ())
    return NONE;

  color_space target_space = color_space::MONOCHROME;
  for (color_space sp : spaces)
    if (sp == m_color_space)
      return *this;
    else if (sp > target_space)
      target_space = sp;

  if (target_space == color_space::RGB_24BIT)
    {
      uint8_t rgb[3];
      get_rgb (rgb);
      return color (rgb[0], rgb[1], rgb[2]);
    }

  int target_size = 0;
  switch (target_space)
    {
    case color_space::ANSI_8COLOR:
      target_size = 8;
      break;
    case color_space::AIXTERM_16COLOR:
      target_size = 16;
      break;
    case color_space::XTERM_256COLOR:
      target_size = 256;
      break;
    }

  if (is_simple() && m_value < target_size)
    return color (target_space, m_value);

  color result = NONE;
  int best_distance = std::numeric_limits<int>::max ();
  uint8_t rgb[3];
  get_rgb (rgb);

  for (int i = 0; i < target_size; ++i)
    {
      uint8_t c_rgb[3];
      color c (target_space, i);
      c.get_rgb (c_rgb);
      int d_red = std::abs (rgb[0] - c_rgb[0]);
      int d_green = std::abs (rgb[1] - c_rgb[1]);
      int d_blue = std::abs (rgb[2] - c_rgb[2]);
      int dist = d_red * d_red + d_green * d_green + d_blue * d_blue;
      if (dist < best_distance)
	{
	  best_distance = dist;
	  result = c;
	}
    }

  return result;
}

/* See ui-style.h.  */

std::string
ui_file_style::to_ansi () const
{
  std::string result ("\033[");
  if (!is_default ())
    {
      m_foreground.append_ansi (true, &result);
      result.push_back (';');
      m_background.append_ansi (false, &result);
      result.push_back (';');
      if (m_intensity == NORMAL)
	result.append ("22");
      else
	result.append (std::to_string (m_intensity));
      result.push_back (';');
      if (m_italic)
	result.append ("3");
      else
	result.append ("23");
      result.push_back (';');
      if (m_underline)
	result.append ("4");
      else
	result.append ("24");
      result.push_back (';');
      if (m_reverse)
	result.push_back ('7');
      else
	result.append ("27");
    }
  result.push_back ('m');
  return result;
}

/* Read a ";" and a number from STRING.  Return the number of
   characters read and put the number into *NUM.  */

static bool
read_semi_number (const char *string, regoff_t *idx, long *num)
{
  if (string[*idx] != ';')
    return false;
  ++*idx;
  if (string[*idx] < '0' || string[*idx] > '9')
    return false;
  char *tail;
  *num = strtol (string + *idx, &tail, 10);
  *idx = tail - string;
  return true;
}

/* A helper for ui_file_style::parse that reads an extended color
   sequence; that is, and 8- or 24- bit color.  */

static bool
extended_color (const char *str, regoff_t *idx, ui_file_style::color *color)
{
  long value;

  if (!read_semi_number (str, idx, &value))
    return false;

  if (value == 5)
    {
      /* 8-bit color.  */
      if (!read_semi_number (str, idx, &value))
	return false;

      if (value >= 0 && value <= 255)
	*color = ui_file_style::color (value);
      else
	return false;
    }
  else if (value == 2)
    {
      /* 24-bit color.  */
      long r, g, b;
      if (!read_semi_number (str, idx, &r)
	  || r > 255
	  || !read_semi_number (str, idx, &g)
	  || g > 255
	  || !read_semi_number (str, idx, &b)
	  || b > 255)
	return false;
      *color = ui_file_style::color (r, g, b);
    }
  else
    {
      /* Unrecognized sequence.  */
      return false;
    }

  return true;
}

/* See ui-style.h.  */

bool
ui_file_style::parse (const char *buf, size_t *n_read)
{
  regmatch_t subexps[NUM_SUBEXPRESSIONS];

  int match = ansi_regex.exec (buf, ARRAY_SIZE (subexps), subexps, 0);
  if (match == REG_NOMATCH)
    {
      *n_read = 0;
      return false;
    }

  /* If the final subexpression did not match, then that means there
     was an incomplete sequence.  These are ignored here.  */
  if (subexps[FINAL_SUBEXP].rm_so == -1)
    {
      *n_read = 0;
      return false;
    }

  /* Other failures mean the regexp is broken.  */
  gdb_assert (match == 0);
  /* The regexp is anchored.  */
  gdb_assert (subexps[0].rm_so == 0);
  /* The final character exists.  */
  gdb_assert (subexps[FINAL_SUBEXP].rm_eo - subexps[FINAL_SUBEXP].rm_so == 1);

  if (buf[subexps[FINAL_SUBEXP].rm_so] != 'm')
    {
      /* We don't handle this sequence, so just drop it.  */
      *n_read = subexps[0].rm_eo;
      return false;
    }

  /* Examine each setting in the match and apply it to the result.
     See the Select Graphic Rendition section of
     https://en.wikipedia.org/wiki/ANSI_escape_code.  In essence each
     code is just a number, separated by ";"; there are some more
     wrinkles but we don't support them all..  */

  /* "\033[m" means the same thing as "\033[0m", so handle that
     specially here.  */
  if (subexps[DATA_SUBEXP].rm_so == subexps[DATA_SUBEXP].rm_eo)
    *this = ui_file_style ();

  for (regoff_t i = subexps[DATA_SUBEXP].rm_so;
       i < subexps[DATA_SUBEXP].rm_eo;
       ++i)
    {
      if (buf[i] == ';')
	{
	  /* Skip.  */
	}
      else if (buf[i] >= '0' && buf[i] <= '9')
	{
	  char *tail;
	  long value = strtol (buf + i, &tail, 10);
	  i = tail - buf;

	  switch (value)
	    {
	    case 0:
	      /* Reset.  */
	      *this = ui_file_style ();
	      break;
	    case 1:
	      /* Bold.  */
	      m_intensity = BOLD;
	      break;
	    case 2:
	      /* Dim.  */
	      m_intensity = DIM;
	      break;
	    case 3:
	      /* Italic.  */
	      m_italic = true;
	      break;
	    case 4:
	      /* Underline.  */
	      m_underline = true;
	      break;
	    case 7:
	      /* Reverse.  */
	      m_reverse = true;
	      break;
	    case 21:
	      m_intensity = NORMAL;
	      break;
	    case 22:
	      /* Normal.  */
	      m_intensity = NORMAL;
	      break;
	    case 23:
	      /* Non-italic.  */
	      m_italic = false;
	      break;
	    case 24:
	      /* Non-underline.  */
	      m_underline = false;
	      break;
	    case 27:
	      /* Inverse off.  */
	      m_reverse = false;
	      break;

	    case 30:
	    case 31:
	    case 32:
	    case 33:
	    case 34:
	    case 35:
	    case 36:
	    case 37:
	      m_foreground = color (value - 30);
	      break;
	      /* Note: not 38.  */
	    case 39:
	      m_foreground = NONE;
	      break;

	    case 40:
	    case 41:
	    case 42:
	    case 43:
	    case 44:
	    case 45:
	    case 46:
	    case 47:
	      m_background = color (value - 40);
	      break;
	      /* Note: not 48.  */
	    case 49:
	      m_background = NONE;
	      break;

	    case 90:
	    case 91:
	    case 92:
	    case 93:
	    case 94:
	    case 95:
	    case 96:
	    case 97:
	      m_foreground = color (value - 90 + 8);
	      break;

	    case 100:
	    case 101:
	    case 102:
	    case 103:
	    case 104:
	    case 105:
	    case 106:
	    case 107:
	      m_background = color (value - 100 + 8);
	      break;

	    case 38:
	      /* If we can't parse the extended color, fail.  */
	      if (!extended_color (buf, &i, &m_foreground))
		{
		  *n_read = subexps[0].rm_eo;
		  return false;
		}
	      break;

	    case 48:
	      /* If we can't parse the extended color, fail.  */
	      if (!extended_color (buf, &i, &m_background))
		{
		  *n_read = subexps[0].rm_eo;
		  return false;
		}
	      break;

	    default:
	      /* Ignore everything else.  */
	      break;
	    }
	}
      else
	{
	  /* Unknown, let's just ignore.  */
	}
    }

  *n_read = subexps[0].rm_eo;
  return true;
}

/* See ui-style.h.  */

ansi_escape_result
examine_ansi_escape (const char *buf, int *n_read)
{
  gdb_assert (*buf == '\033');

  regmatch_t subexps[NUM_SUBEXPRESSIONS];

  int match = ansi_regex.exec (buf, ARRAY_SIZE (subexps), subexps, 0);
  if (match == REG_NOMATCH)
    return ansi_escape_result::NO_MATCH;

  if (subexps[FINAL_SUBEXP].rm_so == -1)
    return ansi_escape_result::INCOMPLETE;

  if (buf[subexps[FINAL_SUBEXP].rm_so] != 'm')
    return ansi_escape_result::NO_MATCH;

  *n_read = subexps[FINAL_SUBEXP].rm_eo;
  return ansi_escape_result::MATCHED;
}

/* See ui-style.h.  */

const std::vector<color_space> &
colorsupport ()
{
  static const std::vector<color_space> value = []
    {
      std::vector<color_space> result = {color_space::MONOCHROME};

      int colors = tgetnum ("Co");
      if (colors >= 8)
	result.push_back (color_space::ANSI_8COLOR);
      if (colors >= 16)
	result.push_back (color_space::AIXTERM_16COLOR);
      if (colors >= 256)
	result.push_back (color_space::XTERM_256COLOR);

      const char *colorterm = getenv ("COLORTERM");
      if (colorterm != nullptr && (!strcmp (colorterm, "truecolor")
	  || !strcmp (colorterm, "24bit")))
	result.push_back (color_space::RGB_24BIT);

      return result;
    } ();
  return value;
}

const char *
color_space_name (color_space c)
{
  switch (c)
    {
    case color_space::MONOCHROME: return "monochrome";
    case color_space::ANSI_8COLOR: return "ansi_8color";
    case color_space::AIXTERM_16COLOR: return "aixterm_16color";
    case color_space::XTERM_256COLOR: return "xterm_256color";
    case color_space::RGB_24BIT: return "rgb_24bit";
    }
  gdb_assert_not_reached ("color_space_name called on invalid color");
}

bool
color_space_safe_cast (color_space *result, long c)
{
  switch (static_cast<color_space>(c))
    {
    case color_space::MONOCHROME:
    case color_space::ANSI_8COLOR:
    case color_space::AIXTERM_16COLOR:
    case color_space::XTERM_256COLOR:
    case color_space::RGB_24BIT:
      *result = static_cast<color_space>(c);
      return true;
    }
  return false;
}
