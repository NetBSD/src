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

#ifndef GDB_UI_STYLE_H
#define GDB_UI_STYLE_H

/* One of the color spaces that usually supported by terminals.  */
enum class color_space
{
  /* one default terminal color  */
  MONOCHROME,

  /* foreground colors \e[30m ... \e[37m,
     background colors \e[40m ... \e[47m  */
  ANSI_8COLOR,

  /* foreground colors \e[30m ... \e[37m, \e[90m ... \e[97m
     background colors \e[40m ... \e[47m, \e[100m ... \e107m  */
  AIXTERM_16COLOR,

  /* foreground colors \e[38;5;0m ... \e[38;5;255m
     background colors \e[48;5;0m ... \e[48;5;255m  */
  XTERM_256COLOR,

  /* foreground colors \e[38;2;0;0;0m ... \e[38;2;255;255;255m
     background colors \e[48;2;0;0;0m ... \e[48;2;255;255;255m  */
  RGB_24BIT
};

/* Color spaces supported by terminal.  */
extern const std::vector<color_space> & colorsupport ();

/* Textual representation of C.  */
extern const char * color_space_name (color_space c);

/* Cast C to RESULT and return true if it's value is valid; false otherwise.  */
extern bool color_space_safe_cast (color_space *result, long c);

/* Styles that can be applied to a ui_file.  */
struct ui_file_style
{
  /* One of the basic colors that can be handled by ANSI
     terminals.  */
  enum basic_color
  {
    NONE = -1,
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE
  };

  /* Representation of a terminal color.  */
  class color
  {
  public:

    color (basic_color c)
      : m_color_space (c == NONE ? color_space::MONOCHROME
				 : color_space::ANSI_8COLOR),
	m_value (c)
    {
    }

    color (int c)
      : m_value (c)
    {
      if (c < -1 || c > 255)
	error (_("Palette color index %d is out of range."), c);
      if (c == -1)
	m_color_space = color_space::MONOCHROME;
      else if (c <= 7)
	m_color_space = color_space::ANSI_8COLOR;
      else if (c <= 15)
	m_color_space = color_space::AIXTERM_16COLOR;
      else
	m_color_space = color_space::XTERM_256COLOR;
    }

    color (color_space cs, int c)
      : m_color_space (cs),
	m_value (c)
    {
      if (c < -1 || c > 255)
	error (_("Palette color index %d is out of range."), c);

      std::pair<int, int> range;
      switch (cs)
	{
	case color_space::MONOCHROME:
	  range = {-1, -1};
	  break;
	case color_space::ANSI_8COLOR:
	  range = {0, 7};
	  break;
	case color_space::AIXTERM_16COLOR:
	  range = {0, 15};
	  break;
	case color_space::XTERM_256COLOR:
	  range = {0, 255};
	  break;
	default:
	  error (_("Color space %d is incompatible with indexed colors."),
		 static_cast<int> (cs));
	}

      if (c < range.first || c > range.second)
	error (_("Color %d is out of range [%d, %d] of color space %d."),
	       c, range.first, range.second, static_cast<int> (cs));
    }

    color (uint8_t r, uint8_t g, uint8_t b)
      : m_color_space (color_space::RGB_24BIT),
	m_red (r),
	m_green (g),
	m_blue (b)
    {
    }

    bool operator== (const color &other) const
    {
      if (m_color_space != other.m_color_space)
	return false;
      if (is_simple ())
	return m_value == other.m_value;
      return (m_red == other.m_red && m_green == other.m_green
	      && m_blue == other.m_blue);
    }

    bool operator!= (const color &other) const
    {
      return ! (*this == other);
    }

    /* Compute a simple hash code for this object.  */
    size_t hash () const
    {
      if (is_simple ())
	return m_value;
      return (m_red << 16) + (m_green << 8) + m_red;
    }

    color_space colorspace () const
    {
      return m_color_space;
    }

    /* Return true if this is the "NONE" color, false otherwise.  */
    bool is_none () const
    {
      return m_color_space == color_space::MONOCHROME && m_value == NONE;
    }

    /* Return true if this is one of the basic colors, false
       otherwise.  */
    bool is_basic () const
    {
      if (m_color_space == color_space::ANSI_8COLOR
	  || m_color_space == color_space::AIXTERM_16COLOR)
	return BLACK <= m_value && m_value <= WHITE;
      else
	return false;
    }

    /* Return true if this is one of the colors, stored as int, false
       otherwise.  */
    bool is_simple () const
    {
      return m_color_space != color_space::RGB_24BIT;
    }

    /* Return true if this is one of the indexed colors, false
       otherwise.  */
    bool is_indexed () const
    {
      return m_color_space != color_space::RGB_24BIT
	     && m_color_space != color_space::MONOCHROME;
    }

    /* Return true if this is one of the direct colors (RGB, CMY, CMYK), false
       otherwise.  */
    bool is_direct () const
    {
      return m_color_space == color_space::RGB_24BIT;
    }

    /* Return the value of a simple color.  */
    int get_value () const
    {
      gdb_assert (is_simple ());
      return m_value;
    }

    /* Fill in RGB with the red/green/blue values for this color.
       This may not be called for basic colors or for the "NONE"
       color.  */
    void get_rgb (uint8_t *rgb) const;

    /* Append the ANSI terminal escape sequence for this color to STR.
       IS_FG indicates whether this is a foreground or background
       color.  */
    void append_ansi (bool is_fg, std::string *str) const;

    /* Return the ANSI escape sequence for this color.
       IS_FG indicates whether this is a foreground or background color.  */
    std::string to_ansi (bool is_fg) const;

    /* Returns text representation of this object.
       It is "none", name of a basic color, number or a #RRGGBB hex triplet.  */
    std::string to_string () const;

    /* Approximates THIS color by closest one from SPACES.  */
    color approximate (const std::vector<color_space> &spaces) const;

  private:

    color_space m_color_space;
    union
    {
      int m_value;
      struct
      {
	uint8_t m_red, m_green, m_blue;
      };
    };
  };

  /* Intensity settings that are available.  */
  enum intensity
  {
    NORMAL = 0,
    BOLD,
    DIM
  };

  ui_file_style () = default;

  ui_file_style (color f, color b, intensity i = NORMAL)
    : m_foreground (f),
      m_background (b),
      m_intensity (i)
  {
  }

  bool operator== (const ui_file_style &other) const
  {
    return (m_foreground == other.m_foreground
	    && m_background == other.m_background
	    && m_intensity == other.m_intensity
	    && m_reverse == other.m_reverse
	    && m_italic == other.m_italic
	    && m_underline == other.m_underline);
  }

  bool operator!= (const ui_file_style &other) const
  {
    return !(*this == other);
  }

  /* Return the ANSI escape sequence for this style.  */
  std::string to_ansi () const;

  /* Return true if this style is the default style; false
     otherwise.  */
  bool is_default () const
  {
    return (m_foreground == NONE
	    && m_background == NONE
	    && m_intensity == NORMAL
	    && !m_reverse
	    && !m_italic
	    && !m_underline);
  }

  /* Return true if this style specified reverse display; false
     otherwise.  */
  bool is_reverse () const
  {
    return m_reverse;
  }

  /* Set/clear the reverse display flag.  */
  void set_reverse (bool reverse)
  {
    m_reverse = reverse;
  }

  /* Return the foreground color of this style.  */
  const color &get_foreground () const
  {
    return m_foreground;
  }

  /* Set the foreground color of this style.  */
  void set_fg (color c)
  {
    m_foreground = c;
  }

  /* Return the background color of this style.  */
  const color &get_background () const
  {
    return m_background;
  }

  /* Set the background color of this style.  */
  void set_bg (color c)
  {
    m_background = c;
  }

  /* Return the intensity of this style.  */
  intensity get_intensity () const
  {
    return m_intensity;
  }

  /* Return true if this style specified italic display; false
     otherwise.  */
  bool is_italic () const
  {
    return m_italic;
  }

  /* Set/clear the italic display flag.  */
  void set_italic (bool italic)
  {
    m_italic = italic;
  }

  /* Return true if this style specified underline display; false
     otherwise.  */
  bool is_underline () const
  {
    return m_underline;
  }

  /* Set/clear the underline display flag.  */
  void set_underline (bool underline)
  {
    m_underline = underline;
  }

  /* Parse an ANSI escape sequence in BUF, modifying this style.  BUF
     must begin with an ESC character.  Return true if an escape
     sequence was successfully parsed; false otherwise.  In either
     case, N_READ is updated to reflect the number of chars read from
     BUF.  */
  bool parse (const char *buf, size_t *n_read);

  /* We need this because we can't pass a reference via va_args.  */
  const ui_file_style *ptr () const
  {
    return this;
  }

  /* nullptr-terminated list of names corresponding to enum basic_color.  */
  static const std::vector<const char *> basic_color_enums;

private:

  color m_foreground = NONE;
  color m_background = NONE;
  intensity m_intensity = NORMAL;
  bool m_italic = false;
  bool m_underline = false;
  bool m_reverse = false;
};

/* Possible results for checking an ANSI escape sequence.  */
enum class ansi_escape_result
{
  /* The escape sequence is definitely not recognizable.  */
  NO_MATCH,

  /* The escape sequence might be recognizable with more input.  */
  INCOMPLETE,

  /* The escape sequence is definitely recognizable.  */
  MATCHED,
};

/* Examine an ANSI escape sequence in BUF.  BUF must begin with an ESC
   character.  Return a value indicating whether the sequence was
   recognizable.  If MATCHED is returned, then N_READ is updated to
   reflect the number of chars read from BUF.  */

extern ansi_escape_result examine_ansi_escape (const char *buf, int *n_read);

/* Skip an ANSI escape sequence in BUF.  BUF must begin with an ESC
   character.  Return true if an escape sequence was successfully
   skipped; false otherwise.  If an escape sequence was skipped,
   N_READ is updated to reflect the number of chars read from BUF.  */

static inline bool
skip_ansi_escape (const char *buf, int *n_read)
{
  return examine_ansi_escape (buf, n_read) == ansi_escape_result::MATCHED;
}

#endif /* GDB_UI_STYLE_H */
