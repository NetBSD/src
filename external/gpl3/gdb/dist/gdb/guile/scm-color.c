/* GDB parameters implemented in Guile.

   Copyright (C) 2008-2025 Free Software Foundation, Inc.

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

#include "value.h"
#include "charset.h"
#include "cli/cli-decode.h"
#include "completer.h"
#include "language.h"
#include "arch-utils.h"
#include "guile-internal.h"
#include "cli/cli-style.h"

/* A GDB color.  */

struct color_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* Underlying value.  */
  ui_file_style::color color;
};

static const char color_smob_name[] = "gdb:color";

/* The tag Guile knows the color smob by.  */
static scm_t_bits color_smob_tag;

/* Keywords used by make-color.  */
static SCM colorspace_keyword;

static const char *coscm_colorspace_name (color_space colorspace);

/* Administrivia for color smobs.  */

static int
coscm_print_color_smob (SCM self, SCM port, scm_print_state *pstate)
{
  const ui_file_style::color &color = coscm_get_color (self);

  gdbscm_printf (port, "#<%s", color_smob_name);

  gdbscm_printf (port, " %s", color.to_string ().c_str ());
  gdbscm_printf (port, " %s", coscm_colorspace_name (color.colorspace ()));
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Create an empty (uninitialized) color.  */

static SCM
coscm_make_color_smob (void)
{
  color_smob *c_smob = (color_smob *)
    scm_gc_calloc (sizeof (color_smob), color_smob_name);
  SCM c_scm;

  c_smob->color = ui_file_style::color (ui_file_style::NONE);
  c_scm = scm_new_smob (color_smob_tag, (scm_t_bits) c_smob);
  gdbscm_init_gsmob (&c_smob->base);

  return c_scm;
}

/* Return the <gdb:color> object that encapsulates COLOR.  */

SCM
coscm_scm_from_color (const ui_file_style::color &color)
{
  SCM c_scm = coscm_make_color_smob ();
  color_smob *c_smob = (color_smob *) SCM_SMOB_DATA (c_scm);
  c_smob->color = color;
  return c_scm;
}

/* Return the color field of color_smob.  */

const ui_file_style::color &
coscm_get_color (SCM color_scm)
{
  SCM_ASSERT_TYPE (coscm_is_color (color_scm), color_scm, SCM_ARG1, FUNC_NAME,
		   _("<gdb:color>"));

  color_smob *c_smob = (color_smob *) SCM_SMOB_DATA (color_scm);
  return c_smob->color;

}

/* Returns non-zero if SCM is a <gdb:color> object.  */

int
coscm_is_color (SCM scm)
{
  return SCM_SMOB_PREDICATE (color_smob_tag, scm);
}

/* (gdb:color? scm) -> boolean */

static SCM
gdbscm_color_p (SCM scm)
{
  return scm_from_bool (coscm_is_color (scm));
}

static const scheme_integer_constant colorspaces[] =
{
  { "COLORSPACE_MONOCHROME", (int) color_space::MONOCHROME },
  { "COLORSPACE_ANSI_8COLOR", (int) color_space::ANSI_8COLOR },
  { "COLORSPACE_AIXTERM_16COLOR", (int) color_space::AIXTERM_16COLOR },
  { "COLORSPACE_XTERM_256COLOR", (int) color_space::XTERM_256COLOR },
  { "COLORSPACE_RGB_24BIT", (int) color_space::RGB_24BIT },

  END_INTEGER_CONSTANTS
};

/* Return COLORSPACE as a string.  */

static const char *
coscm_colorspace_name (color_space colorspace)
{
  for (int i = 0; colorspaces[i].name != nullptr; ++i)
    {
      if (colorspaces[i].value == static_cast<int> (colorspace))
	return colorspaces[i].name;
    }

  gdb_assert_not_reached ("bad color space");
}

/* Free function for a color_smob.  */
static size_t
coscm_free_color_smob (SCM self)
{
  (void) self;
  return 0;
}

/* Color Scheme functions.  */

/* (make-color [value
     [#:color-space colorspace]]) -> <gdb:color>

   VALUE is the value of the color.  It may be SCM_UNDEFINED, string, number
   or list.

   COLORSPACE is the color space of the VALUE.  It should be one of the
   COLORSPACE_* constants defined in the gdb module.

   The result is the <gdb:color> Scheme object.  */

static SCM
gdbscm_make_color (SCM value_scm, SCM rest)
{
  SCM colorspace_arg = SCM_UNDEFINED;
  color_space colorspace = color_space::MONOCHROME;

  scm_c_bind_keyword_arguments (FUNC_NAME, rest,
				static_cast<scm_t_keyword_arguments_flags> (0),
				colorspace_keyword, &colorspace_arg,
				SCM_UNDEFINED);

  if (!SCM_UNBNDP (colorspace_arg))
    {
      SCM_ASSERT_TYPE (scm_is_integer (colorspace_arg), colorspace_arg,
		       SCM_ARG2, FUNC_NAME, _("int"));
      int colorspace_int = scm_to_int (colorspace_arg);
      if (!color_space_safe_cast (&colorspace, colorspace_int))
	gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG2,
				   scm_from_int (colorspace_int),
				   _("invalid colorspace argument"));
    }

  ui_file_style::color color = ui_file_style::NONE;
  gdbscm_gdb_exception exc {};

  try
    {
      if (SCM_UNBNDP (value_scm) || scm_is_integer (value_scm))
	{
	  int i = -1;
	  if (scm_is_integer (value_scm))
	    {
	      i = scm_to_int (value_scm);
	      if (i < 0)
		gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, value_scm,
					   _("negative color index"));
	    }

	  if (SCM_UNBNDP (colorspace_arg))
	    color = ui_file_style::color (i);
	  else
	    color = ui_file_style::color (colorspace, i);
	}
      else if (gdbscm_is_true (scm_list_p (value_scm)))
	{
	  if (SCM_UNBNDP (colorspace_arg)
	      || colorspace != color_space::RGB_24BIT)
	    error (_("colorspace must be COLORSPACE_RGB_24BIT with "
		   "value of list type."));

	  if (scm_ilength (value_scm) != 3)
	    error (_("List value with RGB must be of size 3."));

	  uint8_t rgb[3] = {};
	  int i = 0;
	  for (; i < 3 && !scm_is_eq (value_scm, SCM_EOL); ++i)
	    {
	      SCM item = scm_car (value_scm);

	      SCM_ASSERT_TYPE (scm_is_integer (item), item, SCM_ARG1, FUNC_NAME,
			       _("int"));
	      int component = scm_to_int (item);
	      if (component < 0 || component > UINT8_MAX)
		gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, item,
					   _("invalid rgb component"));
	      rgb[i] = static_cast<uint8_t> (component);

	      value_scm = scm_cdr (value_scm);
	    }

	  gdb_assert (i == 3);

	  color = ui_file_style::color (rgb[0], rgb[1], rgb[2]);
	}
      else if (scm_is_string (value_scm))
	{
	  SCM exception;

	  gdb::unique_xmalloc_ptr<char> string
	    = gdbscm_scm_to_host_string (value_scm, nullptr, &exception);
	  if (string == nullptr)
	    gdbscm_throw (exception);

	  color = parse_var_color (string.get ());

	  if (!SCM_UNBNDP (colorspace_arg) && colorspace != color.colorspace ())
	    error (_("colorspace doesn't match to the value."));

	}
      else
	scm_wrong_type_arg_msg (FUNC_NAME, SCM_ARG1, value_scm,
				"integer, string or list");
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);

  return coscm_scm_from_color (color);
}

/* (color-string <gdb:color>) -> value */

static SCM
gdbscm_color_string (SCM self)
{
  const ui_file_style::color &color = coscm_get_color (self);
  std::string s = color.to_string ();
  return gdbscm_scm_from_host_string (s.c_str (), s.size ());
}

/* (color-colorspace <gdb:color>) -> value */

static SCM
gdbscm_color_colorspace (SCM self)
{
  const ui_file_style::color &color = coscm_get_color (self);
  return scm_from_int (static_cast<int> (color.colorspace ()));
}

/* (color-none? scm) -> boolean */

static SCM
gdbscm_color_none_p (SCM self)
{
  const ui_file_style::color &color = coscm_get_color (self);
  return scm_from_bool (color.is_none ());
}

/* (color-indexed? scm) -> boolean */

static SCM
gdbscm_color_indexed_p (SCM self)
{
  const ui_file_style::color &color = coscm_get_color (self);
  return scm_from_bool (color.is_indexed ());
}

/* (color-direct? scm) -> boolean */

static SCM
gdbscm_color_direct_p (SCM self)
{
  const ui_file_style::color &color = coscm_get_color (self);
  return scm_from_bool (color.is_direct ());
}

/* (color-index <gdb:color>) -> value */

static SCM
gdbscm_color_index (SCM self)
{
  const ui_file_style::color &color = coscm_get_color (self);

  if (!color.is_indexed ())
    gdbscm_misc_error (FUNC_NAME, SCM_ARG1, self, "color is not indexed");
  return scm_from_int (color.get_value ());
}

/* (color-components <gdb:color>) -> value */

static SCM
gdbscm_color_components (SCM self)
{
  const ui_file_style::color &color = coscm_get_color (self);

  if (!color.is_direct ())
    gdbscm_misc_error (FUNC_NAME, SCM_ARG1, self, "color is not direct");

  uint8_t rgb[3] = {};
  color.get_rgb (rgb);
  SCM red = scm_from_uint8 (rgb[0]);
  SCM green = scm_from_uint8 (rgb[1]);
  SCM blue = scm_from_uint8 (rgb[2]);
  return scm_list_3 (red, green, blue);
}

/* (color-escape-sequence <gdb:color> is_fg) -> value */

static SCM
gdbscm_color_escape_sequence (SCM self, SCM is_fg_scm)
{
  const ui_file_style::color &color = coscm_get_color (self);
  SCM_ASSERT_TYPE (gdbscm_is_bool (is_fg_scm), is_fg_scm, SCM_ARG2, FUNC_NAME,
		   _("boolean"));

  std::string s;
  if (term_cli_styling ())
    {
      bool is_fg = gdbscm_is_true (is_fg_scm);
      s = color.to_ansi (is_fg);
    }

  return gdbscm_scm_from_host_string (s.c_str (), s.size ());
}

/* Initialize the Scheme color support.  */

static const scheme_function color_functions[] =
{
  { "make-color", 0, 1, 1, as_a_scm_t_subr (gdbscm_make_color),
    "\
Make a GDB color object.\n\
\n\
  Arguments: [value\n\
      [#:color-space <colorspace>]]\n\
    value: The name of the color.  It may be string, number with color index\n\
      or list with RGB components.\n\
    colorspace: The color space of the color, one of COLORSPACE_*." },

  { "color?", 1, 0, 0, as_a_scm_t_subr (gdbscm_color_p),
    "\
Return #t if the object is a <gdb:color> object." },

  { "color-none?", 1, 0, 0, as_a_scm_t_subr (gdbscm_color_none_p),
    "\
Return #t if the <gdb:color> object has default color." },

  { "color-indexed?", 1, 0, 0, as_a_scm_t_subr (gdbscm_color_indexed_p),
    "\
Return #t if the <gdb:color> object is from indexed color space." },

  { "color-direct?", 1, 0, 0, as_a_scm_t_subr (gdbscm_color_direct_p),
    "\
Return #t if the <gdb:color> object has direct color (e.g. RGB, CMY, CMYK)." },

  { "color-string", 1, 0, 0, as_a_scm_t_subr (gdbscm_color_string),
    "\
Return the textual representation of a <gdb:color> object." },

  { "color-colorspace", 1, 0, 0, as_a_scm_t_subr (gdbscm_color_colorspace),
    "\
Return the color space of a <gdb:color> object." },

  { "color-index", 1, 0, 0, as_a_scm_t_subr (gdbscm_color_index),
    "\
Return index of the color of a <gdb:color> object in a palette." },

  { "color-components", 1, 0, 0, as_a_scm_t_subr (gdbscm_color_components),
    "\
Return components of the direct <gdb:color> object." },

  { "color-escape-sequence", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_color_escape_sequence),
    "\
Return string to change terminal's color to this." },

  END_FUNCTIONS
};

void
gdbscm_initialize_colors (void)
{
  color_smob_tag = gdbscm_make_smob_type (color_smob_name, sizeof (color_smob));
  scm_set_smob_free (color_smob_tag, coscm_free_color_smob);
  scm_set_smob_print (color_smob_tag, coscm_print_color_smob);

  gdbscm_define_integer_constants (colorspaces, 1);
  gdbscm_define_functions (color_functions, 1);

  colorspace_keyword = scm_from_latin1_keyword ("color-space");
}
